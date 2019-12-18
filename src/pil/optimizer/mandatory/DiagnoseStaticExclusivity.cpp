//===--- DiagnoseStaticExclusivity.cpp - Find violations of exclusivity ---===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file implements a diagnostic pass that finds violations of the
// "Law of Exclusivity" at compile time. The Law of Exclusivity requires
// that the access duration of any access to an address not overlap
// with an access to the same address unless both accesses are reads.
//
// This pass relies on 'begin_access' and 'end_access' PIL instruction
// markers inserted during PILGen to determine when an access to an address
// begins and ends. It models the in-progress accesses with a map from
// storage locations to the counts of read and write-like accesses in progress
// for that location.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "static-exclusivity"

#include "polarphp/ast/AstWalker.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/DiagnosticsPIL.h"
#include "polarphp/ast/Expr.h"
#include "polarphp/ast/Stmt.h"
#include "polarphp/basic/SourceLoc.h"
#include "polarphp/llparser/Lexer.h"
#include "polarphp/pil/lang/InstructionUtils.h"
#include "polarphp/pil/lang/MemAccessUtils.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/optimizer/analysis/AccessSummaryAnalysis.h"
#include "polarphp/pil/optimizer/analysis/PostOrderAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

using namespace polar;

using polar::llparser::Lexer;

template <typename... T, typename... U>
static InFlightDiagnostic diagnose(AstContext &Context, SourceLoc loc,
                                   Diag<T...> diag, U &&... args) {
   return Context.Diags.diagnose(loc, diag, std::forward<U>(args)...);
}

namespace {

enum class RecordedAccessKind {
   /// The access was for a 'begin_access' instruction in the current function
   /// being checked.
      BeginInstruction,

   /// The access was inside noescape closure that we either
   /// passed to function or called directly. It results from applying the
   /// the summary of the closure to the closure's captures.
      NoescapeClosureCapture
};

/// Records an access to an address and the single subpath of projections
/// that was performed on the address, if such a single subpath exists.
class RecordedAccess {
private:
   RecordedAccessKind RecordKind;

   union {
      BeginAccessInst *Inst;
      struct {
         PILAccessKind AccessKind;
         PILLocation AccessLoc;
      } Closure;
   };

   const IndexTrieNode *SubPath;
public:
   RecordedAccess(BeginAccessInst *BAI, const IndexTrieNode *SubPath) :
      RecordKind(RecordedAccessKind::BeginInstruction), Inst(BAI),
      SubPath(SubPath) { }

   RecordedAccess(PILAccessKind ClosureAccessKind,
                  PILLocation ClosureAccessLoc, const IndexTrieNode *SubPath) :
      RecordKind(RecordedAccessKind::NoescapeClosureCapture),
      Closure({ClosureAccessKind, ClosureAccessLoc}),
      SubPath(SubPath) { }

   RecordedAccessKind getRecordKind() const {
      return RecordKind;
   }

   BeginAccessInst *getInstruction() const {
      assert(RecordKind == RecordedAccessKind::BeginInstruction);
      return Inst;
   }

   PILAccessKind getAccessKind() const {
      switch (RecordKind) {
         case RecordedAccessKind::BeginInstruction:
            return Inst->getAccessKind();
         case RecordedAccessKind::NoescapeClosureCapture:
            return Closure.AccessKind;
      };
      llvm_unreachable("unhandled kind");
   }

   PILLocation getAccessLoc() const {
      switch (RecordKind) {
         case RecordedAccessKind::BeginInstruction:
            return Inst->getLoc();
         case RecordedAccessKind::NoescapeClosureCapture:
            return Closure.AccessLoc;
      };
      llvm_unreachable("unhandled kind");
   }

   const IndexTrieNode *getSubPath() const {
      return SubPath;
   }
};


/// Records the in-progress accesses to a given sub path.
class SubAccessInfo {
public:
   SubAccessInfo(const IndexTrieNode *P) : Path(P) {}

   const IndexTrieNode *Path;

   /// The number of in-progress 'read' accesses (that is 'begin_access [read]'
   /// instructions that have not yet had the corresponding 'end_access').
   unsigned Reads = 0;

   /// The number of in-progress write-like accesses.
   unsigned NonReads = 0;

   /// The instruction that began the first in-progress access to the storage
   /// location. Used for diagnostic purposes.
   Optional<RecordedAccess> FirstAccess = None;

public:
   /// Increment the count for given access.
   void beginAccess(BeginAccessInst *BAI, const IndexTrieNode *SubPath) {
      if (!FirstAccess) {
         assert(Reads == 0 && NonReads == 0);
         FirstAccess = RecordedAccess(BAI, SubPath);
      }

      if (BAI->getAccessKind() == PILAccessKind::Read)
         Reads++;
      else
         NonReads++;
   }

   /// Decrement the count for given access.
   void endAccess(EndAccessInst *EAI) {
      if (EAI->getBeginAccess()->getAccessKind() == PILAccessKind::Read)
         Reads--;
      else
         NonReads--;

      // If all open accesses are now ended, forget the location of the
      // first access.
      if (Reads == 0 && NonReads == 0)
         FirstAccess = None;
   }

   /// Returns true when there are any accesses to this location in progress.
   bool hasAccessesInProgress() const { return Reads > 0 || NonReads > 0; }

   /// Returns true when there must have already been a conflict diagnosed
   /// for an in-progress access. Used to suppress multiple diagnostics for
   /// the same underlying access violation.
   bool alreadyHadConflict() const {
      return (NonReads > 0 && Reads > 0) || (NonReads > 1);
   }

   // Returns true when beginning an access of the given Kind can
   // result in a conflict with a previous access.
   bool canConflictWithAccessOfKind(PILAccessKind Kind) const {
      if (Kind == PILAccessKind::Read) {
         // A read conflicts with any non-read accesses.
         return NonReads > 0;
      }

      // A non-read access conflicts with any other access.
      return NonReads > 0 || Reads > 0;
   }

   bool conflictsWithAccess(PILAccessKind Kind,
                            const IndexTrieNode *SubPath) const {
      if (!canConflictWithAccessOfKind(Kind))
         return false;

      return pathsConflict(Path, SubPath);
   }

   /// Returns true when the two subpaths access overlapping memory.
   bool pathsConflict(const IndexTrieNode *Path1,
                      const IndexTrieNode *Path2) const {
      return Path1->isPrefixOf(Path2) || Path2->isPrefixOf(Path1);
   }
};

/// Models the in-progress accesses for an address on which access has begun
/// with a begin_access instruction. For a given address, tracks the
/// count and kinds of accesses as well as the subpaths (i.e., projections) that
/// were accessed.
class AccessInfo {
   using SubAccessVector = SmallVector<SubAccessInfo, 4>;

   SubAccessVector SubAccesses;

   /// Returns the SubAccess info for accessing at the given SubPath.
   SubAccessInfo &findOrCreateSubAccessInfo(const IndexTrieNode *SubPath) {
      for (auto &Info : SubAccesses) {
         if (Info.Path == SubPath)
            return Info;
      }

      SubAccesses.emplace_back(SubPath);
      return SubAccesses.back();
   }

   SubAccessVector::const_iterator
   findFirstSubPathWithConflict(PILAccessKind OtherKind,
                                const IndexTrieNode *OtherSubPath) const {
      // Note this iteration requires deterministic ordering for repeatable
      // diagnostics.
      for (auto I = SubAccesses.begin(), E = SubAccesses.end(); I != E; ++I) {
         const SubAccessInfo &Access = *I;
         if (Access.conflictsWithAccess(OtherKind, OtherSubPath))
            return I;
      }

      return SubAccesses.end();
   }

public:
   // Returns the previous access when beginning an access of the given Kind will
   // result in a conflict with a previous access.
   Optional<RecordedAccess>
   conflictsWithAccess(PILAccessKind Kind, const IndexTrieNode *SubPath) const {
      auto I = findFirstSubPathWithConflict(Kind, SubPath);
      if (I == SubAccesses.end())
         return None;

      return I->FirstAccess;
   }

   /// Returns true if any subpath of has already had a conflict.
   bool alreadyHadConflict() const {
      for (const auto &SubAccess : SubAccesses) {
         if (SubAccess.alreadyHadConflict())
            return true;
      }
      return false;
   }

   /// Returns true when there are any accesses to this location in progress.
   bool hasAccessesInProgress() const {
      for (const auto &SubAccess : SubAccesses) {
         if (SubAccess.hasAccessesInProgress())
            return true;
      }
      return false;
   }

   /// Increment the count for given access.
   void beginAccess(BeginAccessInst *BAI, const IndexTrieNode *SubPath) {
      SubAccessInfo &SubAccess = findOrCreateSubAccessInfo(SubPath);
      SubAccess.beginAccess(BAI, SubPath);
   }

   /// Decrement the count for given access.
   void endAccess(EndAccessInst *EAI, const IndexTrieNode *SubPath) {
      SubAccessInfo &SubAccess = findOrCreateSubAccessInfo(SubPath);
      SubAccess.endAccess(EAI);
   }
};

/// Indicates whether a 'begin_access' requires exclusive access
/// or allows shared access. This needs to be kept in sync with
/// diag::exclusivity_access_required, exclusivity_access_required_swift3,
/// and diag::exclusivity_conflicting_access.
enum class ExclusiveOrShared_t : unsigned {
   ExclusiveAccess = 0,
   SharedAccess = 1
};


/// Tracks the in-progress accesses on per-storage-location basis.
using StorageMap = llvm::SmallDenseMap<AccessedStorage, AccessInfo, 4>;

/// Represents two accesses that conflict and their underlying storage.
struct ConflictingAccess {
   /// Create a conflict for two begin_access instructions in the same function.
   ConflictingAccess(const AccessedStorage &Storage, const RecordedAccess &First,
                     const RecordedAccess &Second)
      : Storage(Storage), FirstAccess(First), SecondAccess(Second) {}

   const AccessedStorage Storage;
   const RecordedAccess FirstAccess;
   const RecordedAccess SecondAccess;
};

} // end anonymous namespace

/// Returns whether an access of the given kind requires exclusive or shared
/// access to its storage.
static ExclusiveOrShared_t getRequiredAccess(PILAccessKind Kind) {
   if (Kind == PILAccessKind::Read)
      return ExclusiveOrShared_t::SharedAccess;

   return ExclusiveOrShared_t::ExclusiveAccess;
}

/// Extract the text for the given expression.
static StringRef extractExprText(const Expr *E, SourceManager &SM) {
   const auto CSR = Lexer::getCharSourceRangeFromSourceRange(SM,
                                                             E->getSourceRange());
   return SM.extractText(CSR);
}

/// Returns true when the call expression is a call to swap() in the Standard
/// Library.
/// This is a helper function that is only used in an assertion, which is why
/// it is in the ifndef.
#ifndef NDEBUG
static bool isCallToStandardLibrarySwap(CallExpr *CE, AstContext &Ctx) {
   if (CE->getCalledValue() == Ctx.getSwap())
      return true;

   // Is the call module qualified, i.e. Swift.swap(&a[i], &[j)?
   if (auto *DSBIE = dyn_cast<DotSyntaxBaseIgnoredExpr>(CE->getFn())) {
      if (auto *DRE = dyn_cast<DeclRefExpr>(DSBIE->getRHS())) {
         return DRE->getDecl() == Ctx.getSwap();
      }
   }

   return false;
}
#endif

/// Do a syntactic pattern match to determine whether the call is a call
/// to swap(&base[index1], &base[index2]), which can
/// be replaced with a call to MutableCollection.swapAt(_:_:) on base.
///
/// Returns true if the call can be replaced. Returns the call expression,
/// the base expression, and the two indices as out expressions.
///
/// This method takes an array of all the ApplyInsts for calls to swap()
/// in the function to avoid needing to construct a parent map over the Ast
/// to find the CallExpr for the inout accesses.
static bool
canReplaceWithCallToCollectionSwapAt(const BeginAccessInst *Access1,
                                     const BeginAccessInst *Access2,
                                     ArrayRef<ApplyInst *> CallsToSwap,
                                     AstContext &Ctx,
                                     CallExpr *&FoundCall,
                                     Expr *&Base,
                                     Expr *&Index1,
                                     Expr *&Index2) {
   if (CallsToSwap.empty())
      return false;

   // Inout arguments must be modifications.
   if (Access1->getAccessKind() != PILAccessKind::Modify ||
       Access2->getAccessKind() != PILAccessKind::Modify) {
      return false;
   }

   PILLocation Loc1 = Access1->getLoc();
   PILLocation Loc2 = Access2->getLoc();
   if (Loc1.isNull() || Loc2.isNull())
      return false;

   auto *InOut1 = Loc1.getAsAstNode<InOutExpr>();
   auto *InOut2 = Loc2.getAsAstNode<InOutExpr>();
   if (!InOut1 || !InOut2)
      return false;

   FoundCall = nullptr;
   // Look through all the calls to swap() recorded in the function to find
   // which one we're diagnosing.
   for (ApplyInst *AI : CallsToSwap) {
      PILLocation CallLoc = AI->getLoc();
      if (CallLoc.isNull())
         continue;

      auto *CE = CallLoc.getAsAstNode<CallExpr>();
      if (!CE)
         continue;

      assert(isCallToStandardLibrarySwap(CE, Ctx));
      // swap() takes two arguments.
      auto *ArgTuple = cast<TupleExpr>(CE->getArg());
      const Expr *Arg1 = ArgTuple->getElement(0);
      const Expr *Arg2 = ArgTuple->getElement(1);
      if ((Arg1 == InOut1 && Arg2 == InOut2)) {
         FoundCall = CE;
         break;
      }
   }
   if (!FoundCall)
      return false;

   // We found a call to swap(&e1, &e2). Now check to see whether it
   // matches the form swap(&someCollection[index1], &someCollection[index2]).
   auto *SE1 = dyn_cast<SubscriptExpr>(InOut1->getSubExpr());
   if (!SE1)
      return false;
   auto *SE2 = dyn_cast<SubscriptExpr>(InOut2->getSubExpr());
   if (!SE2)
      return false;

   // Do the two subscripts refer to the same subscript declaration?
   auto *Decl1 = cast<SubscriptDecl>(SE1->getDecl().getDecl());
   auto *Decl2 = cast<SubscriptDecl>(SE2->getDecl().getDecl());
   if (Decl1 != Decl2)
      return false;

   InterfaceDecl *MutableCollectionDecl = Ctx.getMutableCollectionDecl();

   // Is the subcript either (1) on MutableCollection itself or (2) a
   // a witness for a subscript on MutableCollection?
   bool IsSubscriptOnMutableCollection = false;
   InterfaceDecl *InterfaceForDecl =
      Decl1->getDeclContext()->getSelfInterfaceDecl();
   if (InterfaceForDecl) {
      IsSubscriptOnMutableCollection = (InterfaceForDecl == MutableCollectionDecl);
   } else {
      for (ValueDecl *Req : Decl1->getSatisfiedInterfaceRequirements()) {
         DeclContext *ReqDC = Req->getDeclContext();
         InterfaceDecl *ReqProto = ReqDC->getSelfInterfaceDecl();
         assert(ReqProto && "Interface requirement not in a protocol?");

         if (ReqProto == MutableCollectionDecl) {
            IsSubscriptOnMutableCollection = true;
            break;
         }
      }
   }

   if (!IsSubscriptOnMutableCollection)
      return false;

   // We're swapping two subscripts on mutable collections -- but are they
   // the same collection? Approximate this by checking for textual
   // equality on the base expressions. This is just an approximation,
   // but is fine for a best-effort Fix-It.
   SourceManager &SM = Ctx.SourceMgr;
   StringRef Base1Text = extractExprText(SE1->getBase(), SM);
   StringRef Base2Text = extractExprText(SE2->getBase(), SM);

   if (Base1Text != Base2Text)
      return false;

   auto *Index1Paren = dyn_cast<ParenExpr>(SE1->getIndex());
   if (!Index1Paren)
      return false;

   auto *Index2Paren = dyn_cast<ParenExpr>(SE2->getIndex());
   if (!Index2Paren)
      return false;

   Base = SE1->getBase();
   Index1 = Index1Paren->getSubExpr();
   Index2 = Index2Paren->getSubExpr();
   return true;
}

/// Suggest replacing with call with a call to swapAt().
static void addSwapAtFixit(InFlightDiagnostic &Diag, CallExpr *&FoundCall,
                           Expr *Base, Expr *&Index1, Expr *&Index2,
                           SourceManager &SM) {
   StringRef BaseText = extractExprText(Base, SM);
   StringRef Index1Text = extractExprText(Index1, SM);
   StringRef Index2Text = extractExprText(Index2, SM);
   SmallString<64> FixItText;
   {
      llvm::raw_svector_ostream Out(FixItText);
      Out << BaseText << ".swapAt(" << Index1Text << ", " << Index2Text << ")";
   }

   Diag.fixItReplace(FoundCall->getSourceRange(), FixItText);
}

/// Returns a string representation of the BaseName and the SubPath
/// suitable for use in diagnostic text. Only supports the Projections
/// that stored-property relaxation supports: struct stored properties
/// and tuple elements.
static std::string getPathDescription(DeclName BaseName, PILType BaseType,
                                      const IndexTrieNode *SubPath,
                                      PILModule &M,
                                      TypeExpansionContext context) {
   std::string sbuf;
   llvm::raw_string_ostream os(sbuf);

   os << "'" << BaseName;
   os << AccessSummaryAnalysis::getSubPathDescription(BaseType, SubPath, M,
                                                      context);
   os << "'";

   return os.str();
}

/// Emits a diagnostic if beginning an access with the given in-progress
/// accesses violates the law of exclusivity. Returns true when a
/// diagnostic was emitted.
static void diagnoseExclusivityViolation(const ConflictingAccess &Violation,
                                         ArrayRef<ApplyInst *> CallsToSwap,
                                         AstContext &Ctx) {

   const AccessedStorage &Storage = Violation.Storage;
   const RecordedAccess &FirstAccess = Violation.FirstAccess;
   const RecordedAccess &SecondAccess = Violation.SecondAccess;
   PILFunction *F = FirstAccess.getInstruction()->getFunction();

   LLVM_DEBUG(llvm::dbgs() << "Conflict on " << *FirstAccess.getInstruction()
                           << "\n  vs " << *SecondAccess.getInstruction()
                           << "\n  in function " << *F);

   // Can't have a conflict if both accesses are reads.
   assert(!(FirstAccess.getAccessKind() == PILAccessKind::Read &&
            SecondAccess.getAccessKind() == PILAccessKind::Read));

   ExclusiveOrShared_t FirstRequires =
      getRequiredAccess(FirstAccess.getAccessKind());

   // Diagnose on the first access that requires exclusivity.
   bool FirstIsMain = (FirstRequires == ExclusiveOrShared_t::ExclusiveAccess);
   const RecordedAccess &MainAccess = (FirstIsMain ? FirstAccess : SecondAccess);
   const RecordedAccess &NoteAccess = (FirstIsMain ? SecondAccess : FirstAccess);

   SourceRange RangeForMain = MainAccess.getAccessLoc().getSourceRange();
   unsigned AccessKindForMain =
      static_cast<unsigned>(MainAccess.getAccessKind());

   if (const ValueDecl *VD = Storage.getDecl()) {
      // We have a declaration, so mention the identifier in the diagnostic.
      PILType BaseType = FirstAccess.getInstruction()->getType().getAddressType();
      PILModule &M = FirstAccess.getInstruction()->getModule();
      std::string PathDescription = getPathDescription(
         VD->getBaseName(), BaseType, MainAccess.getSubPath(), M,
         TypeExpansionContext(*FirstAccess.getInstruction()->getFunction()));

      // Determine whether we can safely suggest replacing the violation with
      // a call to MutableCollection.swapAt().
      bool SuggestSwapAt = false;
      CallExpr *CallToReplace = nullptr;
      Expr *Base = nullptr;
      Expr *SwapIndex1 = nullptr;
      Expr *SwapIndex2 = nullptr;
      if (SecondAccess.getRecordKind() == RecordedAccessKind::BeginInstruction) {
         SuggestSwapAt = canReplaceWithCallToCollectionSwapAt(
            FirstAccess.getInstruction(), SecondAccess.getInstruction(),
            CallsToSwap, Ctx, CallToReplace, Base, SwapIndex1, SwapIndex2);
      }

      auto D =
         diagnose(Ctx, MainAccess.getAccessLoc().getSourceLoc(),
                  diag::exclusivity_access_required,
                  PathDescription, AccessKindForMain, SuggestSwapAt);
      D.highlight(RangeForMain);
      if (SuggestSwapAt)
         addSwapAtFixit(D, CallToReplace, Base, SwapIndex1, SwapIndex2,
                        Ctx.SourceMgr);
   } else {
      diagnose(Ctx, MainAccess.getAccessLoc().getSourceLoc(),
               diag::exclusivity_access_required_unknown_decl, AccessKindForMain)
         .highlight(RangeForMain);
   }
   diagnose(Ctx, NoteAccess.getAccessLoc().getSourceLoc(),
            diag::exclusivity_conflicting_access)
      .highlight(NoteAccess.getAccessLoc().getSourceRange());
}

/// Look through a value to find the underlying storage accessed.
static AccessedStorage findValidAccessedStorage(PILValue Source) {
   const AccessedStorage &Storage = findAccessedStorage(Source);
   if (!Storage) {
      llvm::dbgs() << "Bad memory access source: " << Source;
      llvm_unreachable("Unexpected access source.");
   }
   return Storage;
}

/// Returns true when the apply calls the Standard Library swap().
/// Used for fix-its to suggest replacing with Collection.swapAt()
/// on exclusivity violations.
static bool isCallToStandardLibrarySwap(ApplyInst *AI, AstContext &Ctx) {
   PILFunction *SF = AI->getReferencedFunctionOrNull();
   if (!SF)
      return false;

   if (!SF->hasLocation())
      return false;

   auto *FD = SF->getLocation().getAsAstNode<FuncDecl>();
   if (!FD)
      return false;

   return FD == Ctx.getSwap();
}

static llvm::cl::opt<bool> ShouldAssertOnFailure(
   "sil-assert-on-exclusivity-failure",
   llvm::cl::desc("Should the compiler assert when it diagnoses conflicting "
                  "accesses rather than emitting a diagnostic? Intended for "
                  "use only with debugging."));

/// If making an access of the given kind at the given subpath would
/// would conflict, returns the first recorded access it would conflict
/// with. Otherwise, returns None.
static Optional<RecordedAccess>
shouldReportAccess(const AccessInfo &Info,polar::PILAccessKind Kind,
                   const IndexTrieNode *SubPath) {
   if (Info.alreadyHadConflict())
      return None;

   auto result = Info.conflictsWithAccess(Kind, SubPath);
   if (ShouldAssertOnFailure && result.hasValue())
      llvm_unreachable("Standard assertion routine.");
   return result;
}

/// For each projection that the summarized function accesses on its
/// capture, check whether the access conflicts with already-in-progress
/// access. Returns the most general summarized conflict -- so if there are
/// two conflicts in the called function and one is for an access to an
/// aggregate and another is for an access to a projection from the aggregate,
/// this will return the conflict for the aggregate. This approach guarantees
/// determinism and makes it more  likely that we'll diagnose the most helpful
/// conflict.
static Optional<ConflictingAccess>
findConflictingArgumentAccess(const AccessSummaryAnalysis::ArgumentSummary &AS,
                              const AccessedStorage &AccessedStorage,
                              const AccessInfo &InProgressInfo) {
   Optional<RecordedAccess> BestInProgressAccess;
   Optional<RecordedAccess> BestArgAccess;

   for (const auto &MapPair : AS.getSubAccesses()) {
      const IndexTrieNode *SubPath = MapPair.getFirst();
      const auto &SubAccess = MapPair.getSecond();
      PILAccessKind Kind = SubAccess.getAccessKind();
      auto InProgressAccess = shouldReportAccess(InProgressInfo, Kind, SubPath);
      if (!InProgressAccess)
         continue;

      if (!BestArgAccess ||
          AccessSummaryAnalysis::compareSubPaths(SubPath,
                                                 BestArgAccess->getSubPath())) {
         PILLocation AccessLoc = SubAccess.getAccessLoc();

         BestArgAccess = RecordedAccess(Kind, AccessLoc, SubPath);
         BestInProgressAccess = InProgressAccess;
      }
   }

   if (!BestArgAccess)
      return None;

   return ConflictingAccess(AccessedStorage, *BestInProgressAccess,
                            *BestArgAccess);
}

// =============================================================================
// The data flow algorithm that drives diagnostics.

// Forward declare verification entry point.
static void checkAccessedAddress(Operand *memOper, StorageMap &Accesses);

namespace {
/// Track the current state of formal accesses, including exclusivity
/// violations, and function summaries at a particular point in the program.
struct AccessState {
   AccessSummaryAnalysis *ASA;

   // Stores the accesses that have been found to conflict. Used to defer
   // emitting diagnostics until we can determine whether they should
   // be suppressed.
   llvm::SmallVector<ConflictingAccess, 4> ConflictingAccesses;

   // Collects calls the Standard Library swap() for Fix-Its.
   llvm::SmallVector<ApplyInst *, 8> CallsToSwap;

   StorageMap *Accesses = nullptr;

   AccessState(AccessSummaryAnalysis *ASA) : ASA(ASA) {}
};
} // namespace

// Find conflicting access on each argument using AccessSummaryAnalysis.
static void
checkCaptureAccess(ApplySite Apply, AccessState &State,
                   const AccessSummaryAnalysis::FunctionSummary &FS) {
   for (unsigned ArgumentIndex : range(Apply.getNumArguments())) {

      unsigned CalleeIndex =
         Apply.getCalleeArgIndexOfFirstAppliedArg() + ArgumentIndex;

      const AccessSummaryAnalysis::ArgumentSummary &AS =
         FS.getAccessForArgument(CalleeIndex);

      const auto &SubAccesses = AS.getSubAccesses();

      // Is the capture accessed in the callee?
      if (SubAccesses.empty())
         continue;

      PILValue Argument = Apply.getArgument(ArgumentIndex);
      assert(Argument->getType().isAddress());

      // A valid AccessedStorage should always be found because Unsafe accesses
      // are not tracked by AccessSummaryAnalysis.
      const AccessedStorage &Storage = findValidAccessedStorage(Argument);
      auto AccessIt = State.Accesses->find(Storage);

      // Are there any accesses in progress at the time of the call?
      if (AccessIt == State.Accesses->end())
         continue;

      const AccessInfo &Info = AccessIt->getSecond();
      if (auto Conflict = findConflictingArgumentAccess(AS, Storage, Info))
         State.ConflictingAccesses.push_back(*Conflict);
   }
}

/// For each argument in the range of the callee arguments being applied at the
/// given apply site, use the summary analysis to determine whether the
/// arguments will be accessed in a way that conflicts with any currently in
/// progress accesses. If so, diagnose.
static void checkCaptureAccess(ApplySite Apply, AccessState &State) {
   // A callee may be nullptr or empty for various reasons, such as being
   // dynamically replaceable.
   PILFunction *Callee = Apply.getCalleeFunction();
   if (Callee && !Callee->empty()) {
      checkCaptureAccess(Apply, State, State.ASA->getOrCreateSummary(Callee));
      return;
   }
   // In the absence of AccessSummaryAnalysis, conservatively assume by-address
   // captures are fully accessed by the callee.
   for (Operand &argOper : Apply.getArgumentOperands()) {
      auto convention = Apply.getArgumentConvention(argOper);
      if (convention != PILArgumentConvention::Indirect_InoutAliasable)
         continue;

      // A valid AccessedStorage should always be found because Unsafe accesses
      // are not tracked by AccessSummaryAnalysis.
      const AccessedStorage &Storage = findValidAccessedStorage(argOper.get());

      // Are there any accesses in progress at the time of the call?
      auto AccessIt = State.Accesses->find(Storage);
      if (AccessIt == State.Accesses->end())
         continue;

      // The unknown argument access is considered a modify of the root subpath.
      auto argAccess = RecordedAccess(PILAccessKind::Modify, Apply.getLoc(),
                                      State.ASA->getSubPathTrieRoot());

      // Construct a conflicting RecordedAccess if one doesn't already exist.
      const AccessInfo &info = AccessIt->getSecond();
      auto inProgressAccess =
         shouldReportAccess(info, PILAccessKind::Modify, argAccess.getSubPath());
      if (!inProgressAccess)
         continue;

      State.ConflictingAccesses.emplace_back(Storage, *inProgressAccess,
                                             argAccess);
   }
}

/// If the given values has a PILFunctionType or an Optional<PILFunctionType>,
/// return the PILFunctionType. Otherwise, return an invalid type.
static CanPILFunctionType getPILFunctionTypeForValue(PILValue arg) {
   PILType argTy = arg->getType();
   // Handle `Optional<@convention(block) @noescape (_)->(_)>`
   if (auto optionalObjTy = argTy.getOptionalObjectType())
      argTy = optionalObjTy;

   return argTy.getAs<PILFunctionType>();
}

/// Recursively check for conflicts with in-progress accesses at the given
/// apply.
///
/// Any captured variable accessed by a noescape closure is considered to be
/// accessed at the point that the closure is fully applied. This includes
/// variables captured by address by the noescape closure being applied or by
/// any other noescape closure that is itself passed as an argument to that
/// closure.
///
/// (1) Use AccessSummaryAnalysis to check each argument for statically
/// enforced accesses nested within the callee.
///
/// (2) If an applied argument is itself a function type, recursively check for
/// violations on the closure being passed as an argument.
///
/// (3) Walk up the chain of partial applies to recursively visit all arguments.
///
/// Note: This handles closures that are called immediately:
///  var i = 7
///  ({ (p: inout Int) in i = 8})(&i) // Overlapping access to 'i'
///
/// Note: This handles chains of partial applies:
///   pa1 = partial_apply f(c) : $(a, b, c)
///   pa2 = partial_apply pa1(b) : $(a, b)
///   apply pa2(a)
static void checkForViolationAtApply(ApplySite Apply, AccessState &State) {
   // First, check access to variables immediately captured at this apply site.
   checkCaptureAccess(Apply, State);

   // Next, recursively check any noescape closures passed as arguments at this
   // apply site.
   TinyPtrVector<PartialApplyInst *> partialApplies;
   for (PILValue Argument : Apply.getArguments()) {
      auto FnType = getPILFunctionTypeForValue(Argument);
      if (!FnType || !FnType->isNoEscape())
         continue;

      findClosuresForFunctionValue(Argument, partialApplies);
   }
   // Continue recursively walking up the chain of applies if necessary.
   findClosuresForFunctionValue(Apply.getCallee(), partialApplies);

   for (auto *PAI : partialApplies)
      checkForViolationAtApply(ApplySite(PAI), State);
}

// Apply transfer function to the AccessState. Beginning an access
// increments the read or write count for the storage location;
// Ending one decrements the count.
static void checkForViolationsAtInstruction(PILInstruction &I,
                                            AccessState &State) {
   if (auto *BAI = dyn_cast<BeginAccessInst>(&I)) {
      if (BAI->getEnforcement() == PILAccessEnforcement::Unsafe)
         return;

      PILAccessKind Kind = BAI->getAccessKind();
      const AccessedStorage &Storage =
         findValidAccessedStorage(BAI->getSource());
      // Storage may be associated with a nested access where the outer access is
      // "unsafe". That's ok because the outer access can itself be treated like a
      // valid source, as long as we don't ask for its source.
      AccessInfo &Info = (*State.Accesses)[Storage];
      const IndexTrieNode *SubPath = State.ASA->findSubPathAccessed(BAI);
      if (auto Conflict = shouldReportAccess(Info, Kind, SubPath)) {
         State.ConflictingAccesses.emplace_back(Storage, *Conflict,
                                                RecordedAccess(BAI, SubPath));
      }

      Info.beginAccess(BAI, SubPath);
      return;
   }

   if (auto *EAI = dyn_cast<EndAccessInst>(&I)) {
      if (EAI->getBeginAccess()->getEnforcement() == PILAccessEnforcement::Unsafe)
         return;

      auto It =
         State.Accesses->find(findValidAccessedStorage(EAI->getSource()));
      AccessInfo &Info = It->getSecond();

      BeginAccessInst *BAI = EAI->getBeginAccess();
      const IndexTrieNode *SubPath = State.ASA->findSubPathAccessed(BAI);
      Info.endAccess(EAI, SubPath);

      // If the storage location has no more in-progress accesses, remove
      // it to keep the StorageMap lean.
      if (!Info.hasAccessesInProgress())
         State.Accesses->erase(It);
      return;
   }

   if (I.getModule().getOptions().VerifyExclusivity && I.mayReadOrWriteMemory()) {
      visitAccessedAddress(&I, [&State](Operand *memOper) {
         checkAccessedAddress(memOper, *State.Accesses);
      });
   }

   if (auto *AI = dyn_cast<ApplyInst>(&I)) {
      // Record calls to swap() for potential Fix-Its.
      if (isCallToStandardLibrarySwap(AI, I.getFunction()->getAstContext()))
         State.CallsToSwap.push_back(AI);
      else
         checkForViolationAtApply(AI, State);
      return;
   }

   if (auto *TAI = dyn_cast<TryApplyInst>(&I)) {
      checkForViolationAtApply(TAI, State);
      return;
   }

   // Sanity check to make sure entries are properly removed.
   assert((!isa<ReturnInst>(&I) || State.Accesses->empty())
          && "Entries were not properly removed?!");
}

static void checkStaticExclusivity(PILFunction &Fn, PostOrderFunctionInfo *PO,
                                   AccessSummaryAnalysis *ASA) {
   // The implementation relies on the following PIL invariants:
   //    - All incoming edges to a block must have the same in-progress
   //      accesses. This enables the analysis to not perform a data flow merge
   //      on incoming edges.
   //    - Further, for a given address each of the in-progress
   //      accesses must have begun in the same order on all edges. This ensures
   //      consistent diagnostics across changes to the exploration of the CFG.
   //    - On return from a function there are no in-progress accesses. This
   //      enables a sanity check for lean analysis state at function exit.
   //    - Each end_access instruction corresponds to exactly one begin access
   //      instruction. (This is encoded in the EndAccessInst itself)
   //    - begin_access arguments cannot be basic block arguments.
   //      This enables the analysis to look back to find the *single* storage
   //      storage location accessed.

   if (Fn.empty())
      return;

   AccessState State(ASA);

   // For each basic block, track the stack of current accesses on
   // exit from that block.
   llvm::SmallDenseMap<PILBasicBlock *, Optional<StorageMap>, 32>
      BlockOutAccesses;

   BlockOutAccesses[Fn.getEntryBlock()] = StorageMap();

   for (auto *BB : PO->getReversePostOrder()) {
      Optional<StorageMap> &BBState = BlockOutAccesses[BB];

      // Because we use a reverse post-order traversal, unless this is the entry
      // at least one of its predecessors must have been reached. Use the out
      // state for that predecessor as our in state. The PIL verifier guarantees
      // that all incoming edges must have the same current accesses.
      for (auto *Pred : BB->getPredecessorBlocks()) {
         auto it = BlockOutAccesses.find(Pred);
         if (it == BlockOutAccesses.end())
            continue;

         const Optional<StorageMap> &PredAccesses = it->getSecond();
         if (PredAccesses) {
            BBState = PredAccesses;
            break;
         }
      }

      // The in-progress accesses for the current program point, represented
      // as map from storage locations to the accesses in progress for the
      // location.
      State.Accesses = BBState.getPointer();
      for (auto &I : *BB)
         checkForViolationsAtInstruction(I, State);
   }

   // Now that we've collected violations and suppressed calls, emit
   // diagnostics.
   for (auto &Violation : State.ConflictingAccesses) {
      diagnoseExclusivityViolation(Violation, State.CallsToSwap,
                                   Fn.getAstContext());
   }
}

// =============================================================================
// Verification

// Check that the given address-type operand is guarded by begin/end access
// markers.
static void checkAccessedAddress(Operand *memOper, StorageMap &Accesses) {
   PILValue address = memOper->get();
   PILInstruction *memInst = memOper->getUser();

   auto error = [address, memInst]() {
      llvm::dbgs() << "Memory access not protected by begin_access:\n";
      memInst->printInContext(llvm::dbgs());
      llvm::dbgs() << "Accessing: " << address;
      llvm::dbgs() << "In function:\n";
      memInst->getFunction()->print(llvm::dbgs());
      abort();
   };

   // If the memory instruction is only used for initialization, it doesn't need
   // an access marker.
   if (memInstMustInitialize(memOper))
      return;

   if (auto apply = ApplySite::isa(memInst)) {
      PILArgumentConvention conv = apply.getArgumentConvention(*memOper);
      // Captured addresses currently use the @inout_aliasable convention. They
      // are considered an access at any call site that uses the closure. However,
      // those accesses are never explictly protected by access markers. Instead,
      // exclusivity uses AccessSummaryAnalysis to check for conflicts. Here, we
      // can simply ignore any @inout_aliasable arguments.
      if (conv == PILArgumentConvention::Indirect_InoutAliasable)
         return;

      assert(!isa<PartialApplyInst>(memInst)
             && "partial apply can only capture an address as inout_aliasable");
      // TODO: We currently assume @in/@in_guaranteed are only used for
      // pass-by-value arguments. i.e. the address points a local copy of the
      // argument, which is only passed by address for abstraction
      // reasons. However, in the future, @in_guaranteed may be used for
      // borrowed values, which should be recognized as a formal read.
      if (conv != PILArgumentConvention::Indirect_Inout)
         return;
   }

   // Strip off address projections, but not ref_element_addr.
   const AccessedStorage &storage = findAccessedStorage(address);
   // findAccessedStorage may return an invalid storage object if the address
   // producer is not recognized by its whitelist. For the purpose of
   // verification, we assume that this can only happen for local
   // initialization, not a formal memory access. The strength of
   // verification rests on the completeness of the opcode list inside
   // findAccessedStorage.
   //
   // For the purpose of verification, an unidentified access is
   // unenforced. These occur in cases like global addressors and local buffers
   // that make use of RawPointers.
   if (!storage || storage.getKind() == AccessedStorage::Unidentified)
      return;

   // Some identifiable addresses can also be recognized as local initialization
   // or other patterns that don't qualify as formal access.
   if (!isPossibleFormalAccessBase(storage, memInst->getFunction()))
      return;

   // A box or stack variable may represent lvalues, but they can only conflict
   // with call sites in the same scope. Some initialization patters (stores to
   // the local value) aren't protected by markers, so we need this check.
   if (!isa<ApplySite>(memInst)
       && (storage.getKind() == AccessedStorage::Box
           || storage.getKind() == AccessedStorage::Stack)) {
      return;
   }

   // Otherwise, the address base should be an in-scope begin_access.
   if (storage.getKind() == AccessedStorage::Nested) {
      auto *BAI = cast<BeginAccessInst>(storage.getValue());
      if (BAI->getEnforcement() == PILAccessEnforcement::Unsafe)
         return;

      const AccessedStorage &Storage = findValidAccessedStorage(BAI->getSource());
      AccessInfo &Info = Accesses[Storage];
      if (!Info.hasAccessesInProgress())
         error();
      return;
   }
   error();
}

// =============================================================================
// Function Pass Driver

namespace {

class DiagnoseStaticExclusivity : public PILFunctionTransform {
public:
   DiagnoseStaticExclusivity() {}

private:
   void run() override {
      // Don't rerun diagnostics on deserialized functions.
      if (getFunction()->wasDeserializedCanonical())
         return;

      PILFunction *Fn = getFunction();
      // This is a staging flag. Eventually the ability to turn off static
      // enforcement will be removed.
      if (!Fn->getModule().getOptions().EnforceExclusivityStatic)
         return;

      PostOrderFunctionInfo *PO = getAnalysis<PostOrderAnalysis>()->get(Fn);
      auto *ASA = getAnalysis<AccessSummaryAnalysis>();
      checkStaticExclusivity(*Fn, PO, ASA);
   }
};

} // end anonymous namespace

PILTransform *polar::createDiagnoseStaticExclusivity() {
   return new DiagnoseStaticExclusivity();
}
