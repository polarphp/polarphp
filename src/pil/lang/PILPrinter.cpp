//===--- PILPrinter.cpp - Pretty-printing of PIL Code ---------------------===//
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
///
/// \file
///
/// This file defines the logic to pretty-print PIL, Instructions, etc.
///
//===----------------------------------------------------------------------===//

#include "polarphp/global/NameStrings.h"
#include "polarphp/demangling/Demangle.h"
#include "polarphp/basic/QuotedString.h"
#include "polarphp/pil/lang/PILPrintContext.h"
#include "polarphp/pil/lang/ApplySite.h"
#include "polarphp/pil/lang/PILBasicBlockCFG.h"
#include "polarphp/pil/lang/PILFunctionCFG.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILCoverageMap.h"
#include "polarphp/pil/lang/PILDebugScope.h"
#include "polarphp/pil/lang/PILDeclRef.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/lang/PILVisitor.h"
#include "polarphp/pil/lang/PILVTable.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/GenericEnvironment.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/PrintOptions.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/ast/Types.h"
#include "polarphp/basic/StlExtras.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/FileSystem.h"

using namespace polar;

using ID = PILPrintContext::ID;

llvm::cl::opt<bool>
   PILPrintNoColor("pil-print-no-color", llvm::cl::init(""),
                   llvm::cl::desc("Don't use color when printing PIL"));

llvm::cl::opt<bool>
   PILFullDemangle("pil-full-demangle", llvm::cl::init(false),
                   llvm::cl::desc("Fully demangle symbol names in PIL output"));

llvm::cl::opt<bool>
   PILPrintDebugInfo("pil-print-debuginfo", llvm::cl::init(false),
                     llvm::cl::desc("Include debug info in PIL output"));

llvm::cl::opt<bool> PILPrintGenericSpecializationInfo(
   "pil-print-generic-specialization-info", llvm::cl::init(false),
   llvm::cl::desc("Include generic specialization"
                  "information info in PIL output"));

static std::string demangleSymbol(StringRef Name) {
   if (PILFullDemangle)
      return demangling::demangleSymbolAsString(Name);
   return demangling::demangleSymbolAsString(Name,
                                             demangling::DemangleOptions::SimplifiedUIDemangleOptions());
}

enum PILColorKind {
   SC_Type,
};

namespace {
/// RAII based coloring of PIL output.
class PILColor {
   raw_ostream &OS;
   enum raw_ostream::Colors Color;
public:
#define DEF_COL(NAME, RAW) case NAME: Color = raw_ostream::RAW; break;

   explicit PILColor(raw_ostream &OS, PILColorKind K) : OS(OS) {
      if (!OS.has_colors() || PILPrintNoColor)
         return;
      switch (K) {
         DEF_COL(SC_Type, YELLOW)
      }
      OS.resetColor();
      OS.changeColor(Color);
   }

   explicit PILColor(raw_ostream &OS, ID::ID_Kind K) : OS(OS) {
      if (!OS.has_colors() || PILPrintNoColor)
         return;
      switch (K) {
         DEF_COL(ID::PILUndef, RED)
         DEF_COL(ID::PILBasicBlock, GREEN)
         DEF_COL(ID::SSAValue, MAGENTA)
         DEF_COL(ID::Null, YELLOW)
      }
      OS.resetColor();
      OS.changeColor(Color);
   }

   ~PILColor() {
      if (!OS.has_colors() || PILPrintNoColor)
         return;
      // FIXME: instead of resetColor(), we can look into
      // capturing the current active color and restoring it.
      OS.resetColor();
   }
#undef DEF_COL
};
} // end anonymous namespace

void PILPrintContext::ID::print(raw_ostream &OS) {
   PILColor C(OS, Kind);
   switch (Kind) {
      case ID::PILUndef:
         OS << "undef";
         return;
      case ID::PILBasicBlock: OS << "bb"; break;
      case ID::SSAValue: OS << '%'; break;
      case ID::Null: OS << "<<NULL OPERAND>>"; return;
   }
   OS << Number;
}

namespace swift {
raw_ostream &operator<<(raw_ostream &OS, PILPrintContext::ID i) {
   i.print(OS);
   return OS;
}
} // namespace swift

/// IDAndType - Used when a client wants to print something like "%0 : $Int".
struct PILValuePrinterInfo {
   ID ValueID;
   PILType Type;
   Optional<ValueOwnershipKind> OwnershipKind;

   PILValuePrinterInfo(ID ValueID) : ValueID(ValueID), Type(), OwnershipKind() {}
   PILValuePrinterInfo(ID ValueID, PILType Type)
      : ValueID(ValueID), Type(Type), OwnershipKind() {}
   PILValuePrinterInfo(ID ValueID, PILType Type,
                       ValueOwnershipKind OwnershipKind)
      : ValueID(ValueID), Type(Type), OwnershipKind(OwnershipKind) {}
};

/// Return the fully qualified dotted path for DeclContext.
static void printFullContext(const DeclContext *Context, raw_ostream &Buffer) {
   if (!Context)
      return;
   switch (Context->getContextKind()) {
      case DeclContextKind::Module:
         if (Context == cast<ModuleDecl>(Context)->getAstContext().TheBuiltinModule)
            Buffer << cast<ModuleDecl>(Context)->getName() << ".";
         return;

      case DeclContextKind::FileUnit:
         // Ignore the file; just print the module.
         printFullContext(Context->getParent(), Buffer);
         return;

      case DeclContextKind::Initializer:
         // FIXME
         Buffer << "<initializer>";
         return;

      case DeclContextKind::AbstractClosureExpr:
         // FIXME
         Buffer << "<anonymous function>";
         return;

      case DeclContextKind::SerializedLocal:
         Buffer << "<serialized local context>";
         return;

      case DeclContextKind::GenericTypeDecl: {
         auto *generic = cast<GenericTypeDecl>(Context);
         printFullContext(generic->getDeclContext(), Buffer);
         Buffer << generic->getName() << ".";
         return;
      }

      case DeclContextKind::ExtensionDecl: {
         const NominalTypeDecl *ExtNominal =
            cast<ExtensionDecl>(Context)->getExtendedNominal();
         printFullContext(ExtNominal->getDeclContext(), Buffer);
         Buffer << ExtNominal->getName() << ".";
         return;
      }

      case DeclContextKind::TopLevelCodeDecl:
         // FIXME
         Buffer << "<top level code>";
         return;
      case DeclContextKind::AbstractFunctionDecl:
         // FIXME
         Buffer << "<abstract function>";
         return;
      case DeclContextKind::SubscriptDecl:
         // FIXME
         Buffer << "<subscript>";
         return;
      case DeclContextKind::EnumElementDecl:
         // FIXME
         Buffer << "<enum element>";
         return;
   }
   llvm_unreachable("bad decl context");
}

static void printValueDecl(ValueDecl *Decl, raw_ostream &OS) {
   printFullContext(Decl->getDeclContext(), OS);

   if (!Decl->hasName()) {
      OS << "anonname=" << (const void*)Decl;
   } else if (Decl->isOperator()) {
      OS << '"' << Decl->getBaseName() << '"';
   } else {
      bool shouldEscape = !Decl->getBaseName().isSpecial() &&
                          llvm::StringSwitch<bool>(Decl->getBaseName().userFacingName())
                             // FIXME: Represent "init" by a special name and remove this case
                             .Case("init", false)
#define KEYWORD(kw) \
            .Case(#kw, true)
#include "polarphp/llparser/TokenKindsDef.h"
                             .Default(false);

      if (shouldEscape) {
         OS << '`' << Decl->getBaseName().userFacingName() << '`';
      } else {
         OS << Decl->getBaseName().userFacingName();
      }
   }
}

/// PILDeclRef uses sigil "#" and prints the fully qualified dotted path.
void PILDeclRef::print(raw_ostream &OS) const {
   OS << "#";
   if (isNull()) {
      OS << "<null>";
      return;
   }

   bool isDot = true;
   if (!hasDecl()) {
      OS << "<anonymous function>";
   } else if (kind == PILDeclRef::Kind::Func) {
      auto *FD = cast<FuncDecl>(getDecl());
      auto accessor = dyn_cast<AccessorDecl>(FD);
      if (!accessor) {
         printValueDecl(FD, OS);
         isDot = false;
      } else {
         switch (accessor->getAccessorKind()) {
            case AccessorKind::WillSet:
               printValueDecl(accessor->getStorage(), OS);
               OS << "!willSet";
               break;
            case AccessorKind::DidSet:
               printValueDecl(accessor->getStorage(), OS);
               OS << "!didSet";
               break;
            case AccessorKind::Get:
               printValueDecl(accessor->getStorage(), OS);
               OS << "!getter";
               break;
            case AccessorKind::Set:
               printValueDecl(accessor->getStorage(), OS);
               OS << "!setter";
               break;
            case AccessorKind::Address:
               printValueDecl(accessor->getStorage(), OS);
               OS << "!addressor";
               break;
            case AccessorKind::MutableAddress:
               printValueDecl(accessor->getStorage(), OS);
               OS << "!mutableAddressor";
               break;
            case AccessorKind::Read:
               printValueDecl(accessor->getStorage(), OS);
               OS << "!read";
               break;
            case AccessorKind::Modify:
               printValueDecl(accessor->getStorage(), OS);
               OS << "!modify";
               break;
         }
      }
   } else {
      printValueDecl(getDecl(), OS);
   }
   switch (kind) {
      case PILDeclRef::Kind::Func:
         break;
      case PILDeclRef::Kind::Allocator:
         OS << "!allocator";
         break;
      case PILDeclRef::Kind::Initializer:
         OS << "!initializer";
         break;
      case PILDeclRef::Kind::EnumElement:
         OS << "!enumelt";
         break;
      case PILDeclRef::Kind::Destroyer:
         OS << "!destroyer";
         break;
      case PILDeclRef::Kind::Deallocator:
         OS << "!deallocator";
         break;
      case PILDeclRef::Kind::IVarInitializer:
         OS << "!ivarinitializer";
         break;
      case PILDeclRef::Kind::IVarDestroyer:
         OS << "!ivardestroyer";
         break;
      case PILDeclRef::Kind::GlobalAccessor:
         OS << "!globalaccessor";
         break;
      case PILDeclRef::Kind::DefaultArgGenerator:
         OS << "!defaultarg" << "." << defaultArgIndex;
         break;
      case PILDeclRef::Kind::StoredPropertyInitializer:
         OS << "!propertyinit";
         break;
      case PILDeclRef::Kind::PropertyWrapperBackingInitializer:
         OS << "!backinginit";
         break;
   }

   auto uncurryLevel = getParameterListCount() - 1;
   if (uncurryLevel != 0)
      OS << (isDot ? '.' : '!')  << uncurryLevel;

   if (isForeign)
      OS << ((isDot || uncurryLevel != 0) ? '.' : '!')  << "foreign";

   if (isDirectReference)
      OS << ((isDot || uncurryLevel != 0) ? '.' : '!')  << "direct";
}

void PILDeclRef::dump() const {
   print(llvm::errs());
   llvm::errs() << '\n';
}

/// Pretty-print the generic specialization information.
static void printGenericSpecializationInfo(
   raw_ostream &OS, StringRef Kind, StringRef Name,
   const GenericSpecializationInformation *SpecializationInfo,
   SubstitutionMap Subs = { }) {
   if (!SpecializationInfo)
      return;

   auto PrintSubstitutions = [&](SubstitutionMap Subs) {
      OS << '<';
      interleave(Subs.getReplacementTypes(),
                 [&](Type type) { OS << type; },
                 [&] { OS << ", "; });
      OS << '>';
   };

   OS << "// Generic specialization information for " << Kind << " " << Name;
   if (!Subs.empty()) {
      OS << " ";
      PrintSubstitutions(Subs);
   }

   OS << ":\n";

   while (SpecializationInfo) {
      OS << "// Caller: " << SpecializationInfo->getCaller()->getName() << '\n';
      OS << "// Parent: " << SpecializationInfo->getParent()->getName() << '\n';
      OS << "// Substitutions: ";
      PrintSubstitutions(SpecializationInfo->getSubstitutions());
      OS << '\n';
      OS << "//\n";
      if (!SpecializationInfo->getCaller()->isSpecialization())
         return;
      SpecializationInfo =
         SpecializationInfo->getCaller()->getSpecializationInfo();
   }
}

static void print(raw_ostream &OS, PILValueCategory category) {
   switch (category) {
      case PILValueCategory::Object: return;
      case PILValueCategory::Address: OS << '*'; return;
   }
   llvm_unreachable("bad value category!");
}

static StringRef getCastConsumptionKindName(CastConsumptionKind kind) {
   switch (kind) {
      case CastConsumptionKind::TakeAlways: return "take_always";
      case CastConsumptionKind::TakeOnSuccess: return "take_on_success";
      case CastConsumptionKind::CopyOnSuccess: return "copy_on_success";
      case CastConsumptionKind::BorrowAlways: return "borrow_always";
   }
   llvm_unreachable("bad cast consumption kind");
}

static void printPILTypeColorAndSigil(raw_ostream &OS, PILType t) {
   PILColor C(OS, SC_Type);
   OS << '$';

   // Potentially add a leading sigil for the value category.
   ::print(OS, t.getCategory());
}

void PILType::print(raw_ostream &OS) const {
   printPILTypeColorAndSigil(OS, *this);

   // Print other types as their Swift representation.
   PrintOptions SubPrinter = PrintOptions::printPIL();
   getAstType().print(OS, SubPrinter);
}

void PILType::dump() const {
   print(llvm::errs());
   llvm::errs() << '\n';
}

namespace {

class PILPrinter;

/// PILPrinter class - This holds the internal implementation details of
/// printing PIL structures.
class PILPrinter : public PILInstructionVisitor<PILPrinter> {
   PILPrintContext &Ctx;
   struct {
      llvm::formatted_raw_ostream OS;
      PrintOptions ASTOptions;
   } PrintState;
   unsigned LastBufferID;

   // Printers for the underlying stream.
#define SIMPLE_PRINTER(TYPE) \
  PILPrinter &operator<<(TYPE value) { \
    PrintState.OS << value;            \
    return *this;                      \
  }
   SIMPLE_PRINTER(char)
   SIMPLE_PRINTER(unsigned)
   SIMPLE_PRINTER(uint64_t)
   SIMPLE_PRINTER(StringRef)
   SIMPLE_PRINTER(Identifier)
   SIMPLE_PRINTER(ID)
   SIMPLE_PRINTER(QuotedString)
   SIMPLE_PRINTER(PILDeclRef)
   SIMPLE_PRINTER(APInt)
   SIMPLE_PRINTER(ValueOwnershipKind)
#undef SIMPLE_PRINTER

   PILPrinter &operator<<(PILValuePrinterInfo i) {
      PILColor C(PrintState.OS, SC_Type);
      *this << i.ValueID;
      if (!i.Type)
         return *this;
      *this << " : ";
      if (i.OwnershipKind && *i.OwnershipKind != ValueOwnershipKind::None) {
         *this << "@" << i.OwnershipKind.getValue() << " ";
      }
      return *this << i.Type;
   }

   PILPrinter &operator<<(Type t) {
      // Print the type using our print options.
      t.print(PrintState.OS, PrintState.ASTOptions);
      return *this;
   }

   PILPrinter &operator<<(PILType t) {
      printPILTypeColorAndSigil(PrintState.OS, t);
      t.getAstType().print(PrintState.OS, PrintState.ASTOptions);
      return *this;
   }

public:
   PILPrinter(
      PILPrintContext &PrintCtx,
      llvm::DenseMap<CanType, Identifier> *AlternativeTypeNames = nullptr)
      : Ctx(PrintCtx),
        PrintState{{PrintCtx.OS()}, PrintOptions::printPIL()},
        LastBufferID(0) {
      PrintState.ASTOptions.AlternativeTypeNames = AlternativeTypeNames;
      PrintState.ASTOptions.PrintForPIL = true;
   }

   PILValuePrinterInfo getIDAndType(PILValue V) {
      return {Ctx.getID(V), V ? V->getType() : PILType()};
   }
   PILValuePrinterInfo getIDAndTypeAndOwnership(PILValue V) {
      return {Ctx.getID(V), V ? V->getType() : PILType(), V.getOwnershipKind()};
   }

   //===--------------------------------------------------------------------===//
   // Big entrypoints.
   void print(const PILFunction *F) {
      // If we are asked to emit sorted PIL, print out our BBs in RPOT order.
      if (Ctx.sortPIL()) {
         std::vector<PILBasicBlock *> RPOT;
         auto *UnsafeF = const_cast<PILFunction *>(F);
         std::copy(po_begin(UnsafeF), po_end(UnsafeF),
                   std::back_inserter(RPOT));
         std::reverse(RPOT.begin(), RPOT.end());
         Ctx.initBlockIDs(RPOT);
         interleave(RPOT,
                    [&](PILBasicBlock *B) { print(B); },
                    [&] { *this << '\n'; });
         return;
      }

      interleave(*F,
                 [&](const PILBasicBlock &B) { print(&B); },
                 [&] { *this << '\n'; });
   }

   void printBlockArgumentUses(const PILBasicBlock *BB) {
      if (BB->args_empty())
         return;

      for (PILValue V : BB->getArguments()) {
         if (V->use_empty())
            continue;
         *this << "// " << Ctx.getID(V);
         PrintState.OS.PadToColumn(50);
         *this << "// user";
         if (std::next(V->use_begin()) != V->use_end())
            *this << 's';
         *this << ": ";

         llvm::SmallVector<ID, 32> UserIDs;
         for (auto *Op : V->getUses())
            UserIDs.push_back(Ctx.getID(Op->getUser()));

         // Display the user ids sorted to give a stable use order in the
         // printer's output if we are asked to do so. This makes diffing large
         // sections of PIL significantly easier at the expense of not showing
         // the _TRUE_ order of the users in the use list.
         if (Ctx.sortPIL()) {
            std::sort(UserIDs.begin(), UserIDs.end());
         }

         interleave(UserIDs.begin(), UserIDs.end(),
                    [&] (ID id) { *this << id; },
                    [&] { *this << ", "; });
         *this << '\n';
      }
   }

   void printBlockArguments(const PILBasicBlock *BB) {
      if (BB->args_empty())
         return;
      *this << '(';
      ArrayRef<PILArgument *> Args = BB->getArguments();

      // If PIL ownership is enabled and the given function has not had ownership
      // stripped out, print out ownership of PILArguments.
      if (BB->getParent()->hasOwnership()) {
         *this << getIDAndTypeAndOwnership(Args[0]);
         for (PILArgument *Arg : Args.drop_front()) {
            *this << ", " << getIDAndTypeAndOwnership(Arg);
         }
         *this << ')';
         return;
      }

      // Otherwise, fall back to the old behavior
      *this << getIDAndType(Args[0]);
      for (PILArgument *Arg : Args.drop_front()) {
         *this << ", " << getIDAndType(Arg);
      }
      *this << ')';
   }

   void print(const PILBasicBlock *BB) {
      // Output uses for BB arguments. These are put into place as comments before
      // the block header.
      printBlockArgumentUses(BB);

      // Then print the name of our block, the arguments, and the block colon.
      *this << Ctx.getID(BB);
      printBlockArguments(BB);
      *this << ":";

      if (!BB->pred_empty()) {
         PrintState.OS.PadToColumn(50);

         *this << "// Preds:";

         llvm::SmallVector<ID, 32> PredIDs;
         for (auto *BBI : BB->getPredecessorBlocks())
            PredIDs.push_back(Ctx.getID(BBI));

         // Display the pred ids sorted to give a stable use order in the printer's
         // output if we are asked to do so. This makes diffing large sections of
         // PIL significantly easier at the expense of not showing the _TRUE_ order
         // of the users in the use list.
         if (Ctx.sortPIL()) {
            std::sort(PredIDs.begin(), PredIDs.end());
         }

         for (auto Id : PredIDs)
            *this << ' ' << Id;
      }
      *this << '\n';

      for (const PILInstruction &I : *BB) {
         Ctx.printInstructionCallBack(&I);
         if (PILPrintGenericSpecializationInfo) {
            if (auto AI = ApplySite::isa(const_cast<PILInstruction *>(&I)))
               if (AI.getSpecializationInfo() && AI.getCalleeFunction())
                  printGenericSpecializationInfo(
                     PrintState.OS, "call-site", AI.getCalleeFunction()->getName(),
                     AI.getSpecializationInfo(), AI.getSubstitutionMap());
         }
         print(&I);
      }
   }

   //===--------------------------------------------------------------------===//
   // PILInstruction Printing Logic

   bool printTypeDependentOperands(const PILInstruction *I) {
      ArrayRef<Operand> TypeDepOps = I->getTypeDependentOperands();
      if (TypeDepOps.empty())
         return false;

      PrintState.OS.PadToColumn(50);
      *this << "// type-defs: ";
      interleave(TypeDepOps,
                 [&](const Operand &op) { *this << Ctx.getID(op.get()); },
                 [&] { *this << ", "; });
      return true;
   }

   /// Print out the users of the PILValue \p V. Return true if we printed out
   /// either an id or a use list. Return false otherwise.
   bool printUsersOfPILNode(const PILNode *node, bool printedSlashes) {
      llvm::SmallVector<PILValue, 8> values;
      if (auto *value = dyn_cast<ValueBase>(node)) {
         values.push_back(value);
      } else if (auto *inst = dyn_cast<PILInstruction>(node)) {
         assert(!isa<SingleValueInstruction>(inst) && "SingleValueInstruction was "
                                                      "handled by the previous "
                                                      "value base check.");
         llvm::copy(inst->getResults(), std::back_inserter(values));
      }

      // If the set of values is empty, we need to print the ID of
      // the instruction.  Otherwise, if none of the values has a use,
      // we don't need to do anything.
      if (!values.empty()) {
         bool hasUse = false;
         for (auto value : values) {
            if (!value->use_empty()) hasUse = true;
         }
         if (!hasUse)
            return printedSlashes;
      }

      if (printedSlashes) {
         *this << "; ";
      } else {
         PrintState.OS.PadToColumn(50);
         *this << "// ";
      }
      if (values.empty()) {
         *this << "id: " << Ctx.getID(node);
         return true;
      }

      llvm::SmallVector<ID, 32> UserIDs;
      for (auto value : values)
         for (auto *Op : value->getUses())
            UserIDs.push_back(Ctx.getID(Op->getUser()));

      *this << "user";
      if (UserIDs.size() != 1)
         *this << 's';
      *this << ": ";

      // If we are asked to, display the user ids sorted to give a stable use
      // order in the printer's output. This makes diffing large sections of PIL
      // significantly easier.
      if (Ctx.sortPIL()) {
         std::sort(UserIDs.begin(), UserIDs.end());
      }

      interleave(UserIDs.begin(), UserIDs.end(), [&](ID id) { *this << id; },
                 [&] { *this << ", "; });
      return true;
   }

   void printDebugLocRef(PILLocation Loc, const SourceManager &SM,
                         bool PrintComma = true) {
      auto DL = Loc.decodeDebugLoc(SM);
      if (!DL.Filename.empty()) {
         if (PrintComma)
            *this << ", ";
         *this << "loc " << QuotedString(DL.Filename) << ':' << DL.Line << ':'
               << DL.Column;
      }
   }

   void printDebugScope(const PILDebugScope *DS, const SourceManager &SM) {
      if (!DS)
         return;

      if (!Ctx.hasScopeID(DS)) {
         printDebugScope(DS->Parent.dyn_cast<const PILDebugScope *>(), SM);
         printDebugScope(DS->InlinedCallSite, SM);
         unsigned ID = Ctx.assignScopeID(DS);
         *this << "pil_scope " << ID << " { ";
         printDebugLocRef(DS->Loc, SM, false);
         *this << " parent ";
         if (auto *F = DS->Parent.dyn_cast<PILFunction *>())
            *this << "@" << F->getName() << " : $" << F->getLoweredFunctionType();
         else {
            auto *PS = DS->Parent.get<const PILDebugScope *>();
            *this << Ctx.getScopeID(PS);
         }
         if (auto *CS = DS->InlinedCallSite)
            *this << " inlined_at " << Ctx.getScopeID(CS);
         *this << " }\n";
      }
   }

   void printDebugScopeRef(const PILDebugScope *DS, const SourceManager &SM,
                           bool PrintComma = true) {
      if (DS) {
         if (PrintComma)
            *this << ", ";
         *this << "scope " << Ctx.getScopeID(DS);
      }
   }

   void printPILLocation(PILLocation L, PILModule &M, const PILDebugScope *DS,
                         bool printedSlashes) {
      if (!L.isNull()) {
         if (!printedSlashes) {
            PrintState.OS.PadToColumn(50);
            *this << "//";
         }
         *this << " ";

         // To minimize output, only print the line and column number for
         // everything but the first instruction.
         L.getSourceLoc().printLineAndColumn(PrintState.OS,
                                             M.getAstContext().SourceMgr);

         // Print the type of location.
         switch (L.getKind()) {
            case PILLocation::RegularKind:
               break;
            case PILLocation::ReturnKind:
               *this << ":return";
               break;
            case PILLocation::ImplicitReturnKind:
               *this << ":imp_return";
               break;
            case PILLocation::InlinedKind:
               *this << ":inlined";
               break;
            case PILLocation::MandatoryInlinedKind:
               *this << ":minlined";
               break;
            case PILLocation::CleanupKind:
               *this << ":cleanup";
               break;
            case PILLocation::ArtificialUnreachableKind:
               *this << ":art_unreach";
               break;
         }
         if (L.isPILFile())
            *this << ":pil";
         if (L.isAutoGenerated())
            *this << ":auto_gen";
         if (L.isInPrologue())
            *this << ":in_prologue";
      }
      if (L.isNull()) {
         if (!printedSlashes) {
            PrintState.OS.PadToColumn(50);
            *this << "//";
         }
         if (L.isInTopLevel())
            *this << " top_level";
         else if (L.isAutoGenerated())
            *this << " auto_gen";
         else
            *this << " no_loc";
         if (L.isInPrologue())
            *this << ":in_prologue";
      }

      if (!DS)
         return;

      // Print inlined-at location, if any.
      const PILDebugScope *CS = DS;
      while ((CS = CS->InlinedCallSite)) {
         *this << ": ";
         if (auto *InlinedF = CS->getInlinedFunction())
            *this << demangleSymbol(InlinedF->getName());
         else
            *this << '?';
         *this << " perf_inlined_at ";
         auto CallSite = CS->Loc;
         if (!CallSite.isNull() && CallSite.isAstNode())
            CallSite.getSourceLoc().print(
               PrintState.OS, M.getAstContext().SourceMgr, LastBufferID);
         else
            *this << "?";
      }
   }

   void printInstOpCode(const PILInstruction *I) {
      *this << getPILInstructionName(I->getKind()) << " ";
   }

   void print(const PILInstruction *I) {
      if (auto *FRI = dyn_cast<FunctionRefInst>(I))
         *this << "  // function_ref "
               << demangleSymbol(FRI->getInitiallyReferencedFunction()->getName())
               << "\n";
      else if (auto *FRI = dyn_cast<DynamicFunctionRefInst>(I))
         *this << "  // dynamic_function_ref "
               << demangleSymbol(FRI->getInitiallyReferencedFunction()->getName())
               << "\n";
      else if (auto *FRI = dyn_cast<PreviousDynamicFunctionRefInst>(I))
         *this << "  // prev_dynamic_function_ref "
               << demangleSymbol(FRI->getInitiallyReferencedFunction()->getName())
               << "\n";

      *this << "  ";

      // Print results.
      auto results = I->getResults();
      if (results.size() == 1 &&
          I->isStaticInitializerInst() &&
          I == &I->getParent()->back()) {
         *this << "%initval = ";
      } else if (results.size() == 1) {
         ID Name = Ctx.getID(results[0]);
         *this << Name << " = ";
      } else if (results.size() > 1) {
         *this << '(';
         bool first = true;
         for (auto result : results) {
            if (first) {
               first = false;
            } else {
               *this << ", ";
            }
            ID Name = Ctx.getID(result);
            *this << Name;
         }
         *this << ") = ";
      }

      // Print the opcode.
      printInstOpCode(I);

      // Use the visitor to print the rest of the instruction.
      visit(const_cast<PILInstruction*>(I));

      // Maybe print debugging information.
      bool printedSlashes = false;
      if (Ctx.printDebugInfo() && !I->isStaticInitializerInst()) {
         auto &SM = I->getModule().getAstContext().SourceMgr;
         printDebugLocRef(I->getLoc(), SM);
         printDebugScopeRef(I->getDebugScope(), SM);
      }
      printedSlashes = printTypeDependentOperands(I);

      // Print users, or id for valueless instructions.
      printedSlashes = printUsersOfPILNode(I, printedSlashes);

      // Print PIL location.
      if (Ctx.printVerbose()) {
         printPILLocation(I->getLoc(), I->getModule(), I->getDebugScope(),
                          printedSlashes);
      }

      *this << '\n';
   }

   void print(const PILNode *node) {
      switch (node->getKind()) {
#define INST(ID, PARENT) \
    case PILNodeKind::ID:
#include "polarphp/pil/lang/PILNodesDef.h"
            print(cast<PILInstruction>(node));
            return;

#define ARGUMENT(ID, PARENT) \
    case PILNodeKind::ID:
#include "polarphp/pil/lang/PILNodesDef.h"
            printPILArgument(cast<PILArgument>(node));
            return;

         case PILNodeKind::PILUndef:
            printPILUndef(cast<PILUndef>(node));
            return;

#define MULTIPLE_VALUE_INST_RESULT(ID, PARENT) \
    case PILNodeKind::ID:
#include "polarphp/pil/lang/PILNodesDef.h"
            printPILMultipleValueInstructionResult(
               cast<MultipleValueInstructionResult>(node));
            return;
      }
      llvm_unreachable("bad kind");
   }

   void printPILArgument(const PILArgument *arg) {
      // This should really only happen during debugging.
      *this << Ctx.getID(arg) << " = argument of "
            << Ctx.getID(arg->getParent()) << " : " << arg->getType();

      // Print users.
      (void) printUsersOfPILNode(arg, false);

      *this << '\n';
   }

   void printPILUndef(const PILUndef *undef) {
      // This should really only happen during debugging.
      *this << "undef<" << undef->getType() << ">\n";
   }

   void printPILMultipleValueInstructionResult(
      const MultipleValueInstructionResult *result) {
      // This should really only happen during debugging.
      if (result->getParent()->getNumResults() == 1) {
         *this << "**" << Ctx.getID(result) << "** = ";
      } else {
         *this << '(';
         interleave(result->getParent()->getResults(),
                    [&](PILValue value) {
                       if (value == PILValue(result)) {
                          *this << "**" << Ctx.getID(result) << "**";
                          return;
                       }
                       *this << Ctx.getID(value);
                    },
                    [&] { *this << ", "; });
         *this << ')';
      }

      *this << " = ";
      printInstOpCode(result->getParent());
      auto *nonConstParent =
         const_cast<MultipleValueInstruction *>(result->getParent());
      visit(static_cast<PILInstruction *>(nonConstParent));

      // Print users.
      (void)printUsersOfPILNode(result, false);

      *this << '\n';
   }

   void printInContext(const PILNode *node) {
      auto sortByID = [&](const PILNode *a, const PILNode *b) {
         return Ctx.getID(a).Number < Ctx.getID(b).Number;
      };

      if (auto *I = dyn_cast<PILInstruction>(node)) {
         auto operands = map<SmallVector<PILValue,4>>(I->getAllOperands(),
                                                      [](Operand const &o) {
                                                         return o.get();
                                                      });
         std::sort(operands.begin(), operands.end(), sortByID);
         for (auto &operand : operands) {
            *this << "   ";
            print(operand);
         }
      }

      *this << "-> ";
      print(node);

      if (auto V = dyn_cast<ValueBase>(node)) {
         auto users = map<SmallVector<const PILInstruction*,4>>(V->getUses(),
                                                                [](Operand *o) {
                                                                   return o->getUser();
                                                                });
         std::sort(users.begin(), users.end(), sortByID);
         for (auto &user : users) {
            *this << "   ";
            print(user);
         }
      }
   }

   void printDebugVar(Optional<PILDebugVariable> Var) {
      if (!Var || Var->Name.empty())
         return;
      if (Var->Constant)
         *this << ", let";
      else
         *this << ", var";
      *this << ", name \"" << Var->Name << '"';
      if (Var->ArgNo)
         *this << ", argno " << Var->ArgNo;
   }

   void visitAllocStackInst(AllocStackInst *AVI) {
      if (AVI->hasDynamicLifetime())
         *this << "[dynamic_lifetime] ";
      *this << AVI->getElementType();
      printDebugVar(AVI->getVarInfo());
   }

   void printAllocRefInstBase(AllocRefInstBase *ARI) {
      if (ARI->isObjC())
         *this << "[objc] ";
      if (ARI->canAllocOnStack())
         *this << "[stack] ";
      auto Types = ARI->getTailAllocatedTypes();
      auto Counts = ARI->getTailAllocatedCounts();
      for (unsigned Idx = 0, NumTypes = Types.size(); Idx < NumTypes; ++Idx) {
         *this << "[tail_elems " << Types[Idx] << " * "
               << getIDAndType(Counts[Idx].get()) << "] ";
      }
   }

   void visitAllocRefInst(AllocRefInst *ARI) {
      printAllocRefInstBase(ARI);
      *this << ARI->getType();
   }

   void visitAllocRefDynamicInst(AllocRefDynamicInst *ARDI) {
      printAllocRefInstBase(ARDI);
      *this << getIDAndType(ARDI->getMetatypeOperand());
      *this << ", " << ARDI->getType();
   }

   void visitAllocValueBufferInst(AllocValueBufferInst *AVBI) {
      *this << AVBI->getValueType() << " in " << getIDAndType(AVBI->getOperand());
   }

   void visitAllocBoxInst(AllocBoxInst *ABI) {
      if (ABI->hasDynamicLifetime())
         *this << "[dynamic_lifetime] ";
      *this << ABI->getType();
      printDebugVar(ABI->getVarInfo());
   }

   void printSubstitutions(SubstitutionMap Subs,
                           GenericSignature Sig = GenericSignature()) {
      if (!Subs.hasAnySubstitutableParams()) return;

      // FIXME: This is a hack to cope with cases where the substitution map uses
      // a generic signature that's close-to-but-not-the-same-as expected.
      auto genericSig = Sig ? Sig : Subs.getGenericSignature();

      *this << '<';
      bool first = true;
      for (auto gp : genericSig->getGenericParams()) {
         if (first) first = false;
         else *this << ", ";

         *this << Type(gp).subst(Subs);
      }
      *this << '>';
   }

   template <class Inst>
   void visitApplyInstBase(Inst *AI) {
      *this << Ctx.getID(AI->getCallee());
      printSubstitutions(AI->getSubstitutionMap(),
                         AI->getOrigCalleeType()->getInvocationGenericSignature());
      *this << '(';
      interleave(AI->getArguments(),
                 [&](const PILValue &arg) { *this << Ctx.getID(arg); },
                 [&] { *this << ", "; });
      *this << ") : ";
      if (auto callee = AI->getCallee())
         *this << callee->getType();
      else
         *this << "<<NULL CALLEE>>";
   }

   void visitApplyInst(ApplyInst *AI) {
      if (AI->isNonThrowing())
         *this << "[nothrow] ";
      visitApplyInstBase(AI);
   }

   void visitBeginApplyInst(BeginApplyInst *AI) {
      if (AI->isNonThrowing())
         *this << "[nothrow] ";
      visitApplyInstBase(AI);
   }

   void visitTryApplyInst(TryApplyInst *AI) {
      visitApplyInstBase(AI);
      *this << ", normal " << Ctx.getID(AI->getNormalBB());
      *this << ", error " << Ctx.getID(AI->getErrorBB());
   }

   void visitPartialApplyInst(PartialApplyInst *CI) {
      switch (CI->getFunctionType()->getCalleeConvention()) {
         case ParameterConvention::Direct_Owned:
            // Default; do nothing.
            break;
         case ParameterConvention::Direct_Guaranteed:
            *this << "[callee_guaranteed] ";
            break;

            // Should not apply to callees.
         case ParameterConvention::Direct_Unowned:
         case ParameterConvention::Indirect_In:
         case ParameterConvention::Indirect_In_Constant:
         case ParameterConvention::Indirect_Inout:
         case ParameterConvention::Indirect_In_Guaranteed:
         case ParameterConvention::Indirect_InoutAliasable:
            llvm_unreachable("unexpected callee convention!");
      }
      if (CI->isOnStack())
         *this << "[on_stack] ";
      visitApplyInstBase(CI);
   }

   void visitAbortApplyInst(AbortApplyInst *AI) {
      *this << Ctx.getID(AI->getOperand());
   }

   void visitEndApplyInst(EndApplyInst *AI) {
      *this << Ctx.getID(AI->getOperand());
   }

   void visitFunctionRefInst(FunctionRefInst *FRI) {
      FRI->getInitiallyReferencedFunction()->printName(PrintState.OS);
      *this << " : " << FRI->getType();
   }
   void visitDynamicFunctionRefInst(DynamicFunctionRefInst *FRI) {
      FRI->getInitiallyReferencedFunction()->printName(PrintState.OS);
      *this << " : " << FRI->getType();
   }
   void
   visitPreviousDynamicFunctionRefInst(PreviousDynamicFunctionRefInst *FRI) {
      FRI->getInitiallyReferencedFunction()->printName(PrintState.OS);
      *this << " : " << FRI->getType();
   }

   void visitBuiltinInst(BuiltinInst *BI) {
      *this << QuotedString(BI->getName().str());
      printSubstitutions(BI->getSubstitutions());
      *this << "(";

      interleave(BI->getArguments(), [&](PILValue v) {
         *this << getIDAndType(v);
      }, [&]{
         *this << ", ";
      });

      *this << ") : ";
      *this << BI->getType();
   }

   void visitAllocGlobalInst(AllocGlobalInst *AGI) {
      if (AGI->getReferencedGlobal()) {
         AGI->getReferencedGlobal()->printName(PrintState.OS);
      } else {
         *this << "<<placeholder>>";
      }
   }

   void visitGlobalAddrInst(GlobalAddrInst *GAI) {
      if (GAI->getReferencedGlobal()) {
         GAI->getReferencedGlobal()->printName(PrintState.OS);
      } else {
         *this << "<<placeholder>>";
      }
      *this << " : " << GAI->getType();
   }

   void visitGlobalValueInst(GlobalValueInst *GVI) {
      GVI->getReferencedGlobal()->printName(PrintState.OS);
      *this << " : " << GVI->getType();
   }

   void visitIntegerLiteralInst(IntegerLiteralInst *ILI) {
      const auto &lit = ILI->getValue();
      *this << ILI->getType() << ", " << lit;
   }
   void visitFloatLiteralInst(FloatLiteralInst *FLI) {
      *this << FLI->getType() << ", 0x";
      APInt bits = FLI->getBits();
      *this << bits.toString(16, /*Signed*/ false);
      llvm::SmallString<12> decimal;
      FLI->getValue().toString(decimal);
      *this << " // " << decimal;
   }
   static StringRef getStringEncodingName(StringLiteralInst::Encoding kind) {
      switch (kind) {
         case StringLiteralInst::Encoding::Bytes: return "bytes ";
         case StringLiteralInst::Encoding::UTF8: return "utf8 ";
         case StringLiteralInst::Encoding::UTF16: return "utf16 ";
         case StringLiteralInst::Encoding::ObjCSelector: return "objc_selector ";
      }
      llvm_unreachable("bad string literal encoding");
   }

   void visitStringLiteralInst(StringLiteralInst *SLI) {
      *this << getStringEncodingName(SLI->getEncoding());

      if (SLI->getEncoding() != StringLiteralInst::Encoding::Bytes) {
         // FIXME: this isn't correct: this doesn't properly handle translating
         // UTF16 into UTF8, and the PIL parser always parses as UTF8.
         *this << QuotedString(SLI->getValue());
         return;
      }

      // "Bytes" are always output in a hexadecimal form.
      *this << '"' << llvm::toHex(SLI->getValue()) << '"';
   }

   void printLoadOwnershipQualifier(LoadOwnershipQualifier Qualifier) {
      switch (Qualifier) {
         case LoadOwnershipQualifier::Unqualified:
            return;
         case LoadOwnershipQualifier::Take:
            *this << "[take] ";
            return;
         case LoadOwnershipQualifier::Copy:
            *this << "[copy] ";
            return;
         case LoadOwnershipQualifier::Trivial:
            *this << "[trivial] ";
            return;
      }
   }

   void visitLoadInst(LoadInst *LI) {
      printLoadOwnershipQualifier(LI->getOwnershipQualifier());
      *this << getIDAndType(LI->getOperand());
   }

   void visitLoadBorrowInst(LoadBorrowInst *LBI) {
      *this << getIDAndType(LBI->getOperand());
   }

   void visitBeginBorrowInst(BeginBorrowInst *LBI) {
      *this << getIDAndType(LBI->getOperand());
   }

   void printStoreOwnershipQualifier(StoreOwnershipQualifier Qualifier) {
      switch (Qualifier) {
         case StoreOwnershipQualifier::Unqualified:
            return;
         case StoreOwnershipQualifier::Init:
            *this << "[init] ";
            return;
         case StoreOwnershipQualifier::Assign:
            *this << "[assign] ";
            return;
         case StoreOwnershipQualifier::Trivial:
            *this << "[trivial] ";
            return;
      }
   }

   void printAssignOwnershipQualifier(AssignOwnershipQualifier Qualifier) {
      switch (Qualifier) {
         case AssignOwnershipQualifier::Unknown:
            return;
         case AssignOwnershipQualifier::Init:
            *this << "[init] ";
            return;
         case AssignOwnershipQualifier::Reassign:
            *this << "[reassign] ";
            return;
         case AssignOwnershipQualifier::Reinit:
            *this << "[reinit] ";
            return;
      }
   }

   void visitStoreInst(StoreInst *SI) {
      *this << Ctx.getID(SI->getSrc()) << " to ";
      printStoreOwnershipQualifier(SI->getOwnershipQualifier());
      *this << getIDAndType(SI->getDest());
   }

   void visitStoreBorrowInst(StoreBorrowInst *SI) {
      *this << Ctx.getID(SI->getSrc()) << " to ";
      *this << getIDAndType(SI->getDest());
   }

   void visitEndBorrowInst(EndBorrowInst *EBI) {
      *this << getIDAndType(EBI->getOperand());
   }

   void visitAssignInst(AssignInst *AI) {
      *this << Ctx.getID(AI->getSrc()) << " to ";
      printAssignOwnershipQualifier(AI->getOwnershipQualifier());
      *this << getIDAndType(AI->getDest());
   }

   void visitAssignByWrapperInst(AssignByWrapperInst *AI) {
      *this << getIDAndType(AI->getSrc()) << " to ";
      printAssignOwnershipQualifier(AI->getOwnershipQualifier());
      *this << getIDAndType(AI->getDest())
            << ", init " << getIDAndType(AI->getInitializer())
            << ", set " << getIDAndType(AI->getSetter());
   }

   void visitMarkUninitializedInst(MarkUninitializedInst *MU) {
      switch (MU->getKind()) {
         case MarkUninitializedInst::Var: *this << "[var] "; break;
         case MarkUninitializedInst::RootSelf:  *this << "[rootself] "; break;
         case MarkUninitializedInst::CrossModuleRootSelf:
            *this << "[crossmodulerootself] ";
            break;
         case MarkUninitializedInst::DerivedSelf:  *this << "[derivedself] "; break;
         case MarkUninitializedInst::DerivedSelfOnly:
            *this << "[derivedselfonly] ";
            break;
         case MarkUninitializedInst::DelegatingSelf: *this << "[delegatingself] ";break;
         case MarkUninitializedInst::DelegatingSelfAllocated:
            *this << "[delegatingselfallocated] ";
            break;
      }

      *this << getIDAndType(MU->getOperand());
   }

   void visitMarkFunctionEscapeInst(MarkFunctionEscapeInst *MFE) {
      interleave(MFE->getElements(),
                 [&](PILValue Var) { *this << getIDAndType(Var); },
                 [&] { *this << ", "; });
   }

   void visitDebugValueInst(DebugValueInst *DVI) {
      *this << getIDAndType(DVI->getOperand());
      printDebugVar(DVI->getVarInfo());
   }

   void visitDebugValueAddrInst(DebugValueAddrInst *DVAI) {
      *this << getIDAndType(DVAI->getOperand());
      printDebugVar(DVAI->getVarInfo());
   }

#define NEVER_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
  void visitLoad##Name##Inst(Load##Name##Inst *LI) { \
    if (LI->isTake()) \
      *this << "[take] "; \
    *this << getIDAndType(LI->getOperand()); \
  } \
  void visitStore##Name##Inst(Store##Name##Inst *SI) { \
    *this << Ctx.getID(SI->getSrc()) << " to "; \
    if (SI->isInitializationOfDest()) \
      *this << "[initialization] "; \
    *this << getIDAndType(SI->getDest()); \
  }
#include "polarphp/ast/ReferenceStorageDef.h"

   void visitCopyAddrInst(CopyAddrInst *CI) {
      if (CI->isTakeOfSrc())
         *this << "[take] ";
      *this << Ctx.getID(CI->getSrc()) << " to ";
      if (CI->isInitializationOfDest())
         *this << "[initialization] ";
      *this << getIDAndType(CI->getDest());
   }

   void visitBindMemoryInst(BindMemoryInst *BI) {
      *this << getIDAndType(BI->getBase()) << ", ";
      *this << getIDAndType(BI->getIndex()) << " to ";
      *this << BI->getBoundType();
   }

   void visitUnconditionalCheckedCastInst(UnconditionalCheckedCastInst *CI) {
      *this << getIDAndType(CI->getOperand()) << " to " << CI->getTargetFormalType();
   }

   void visitCheckedCastBranchInst(CheckedCastBranchInst *CI) {
      if (CI->isExact())
         *this << "[exact] ";
      *this << getIDAndType(CI->getOperand()) << " to " << CI->getTargetFormalType()
            << ", " << Ctx.getID(CI->getSuccessBB()) << ", "
            << Ctx.getID(CI->getFailureBB());
      if (CI->getTrueBBCount())
         *this << " !true_count(" << CI->getTrueBBCount().getValue() << ")";
      if (CI->getFalseBBCount())
         *this << " !false_count(" << CI->getFalseBBCount().getValue() << ")";
   }

   void visitCheckedCastValueBranchInst(CheckedCastValueBranchInst *CI) {
      *this << CI->getSourceFormalType() << " in "
            << getIDAndType(CI->getOperand()) << " to " << CI->getTargetFormalType()
            << ", " << Ctx.getID(CI->getSuccessBB()) << ", "
            << Ctx.getID(CI->getFailureBB());
   }

   void visitUnconditionalCheckedCastAddrInst(UnconditionalCheckedCastAddrInst *CI) {
      *this << CI->getSourceFormalType() << " in " << getIDAndType(CI->getSrc())
            << " to " << CI->getTargetFormalType() << " in "
            << getIDAndType(CI->getDest());
   }

   void visitUnconditionalCheckedCastValueInst(
      UnconditionalCheckedCastValueInst *CI) {
      *this << CI->getSourceFormalType() << " in " << getIDAndType(CI->getOperand())
            << " to " << CI->getTargetFormalType();
   }

   void visitCheckedCastAddrBranchInst(CheckedCastAddrBranchInst *CI) {
      *this << getCastConsumptionKindName(CI->getConsumptionKind()) << ' '
            << CI->getSourceFormalType() << " in " << getIDAndType(CI->getSrc())
            << " to " << CI->getTargetFormalType() << " in "
            << getIDAndType(CI->getDest()) << ", "
            << Ctx.getID(CI->getSuccessBB()) << ", "
            << Ctx.getID(CI->getFailureBB());
      if (CI->getTrueBBCount())
         *this << " !true_count(" << CI->getTrueBBCount().getValue() << ")";
      if (CI->getFalseBBCount())
         *this << " !false_count(" << CI->getFalseBBCount().getValue() << ")";
   }

   void printUncheckedConversionInst(ConversionInst *CI, PILValue operand) {
      *this << getIDAndType(operand) << " to " << CI->getType();
   }

   void visitUncheckedOwnershipConversionInst(
      UncheckedOwnershipConversionInst *UOCI) {
      *this << getIDAndType(UOCI->getOperand()) << ", "
            << "@" << UOCI->getOperand().getOwnershipKind() << " to "
            << "@" << UOCI->getConversionOwnershipKind();
   }

   void visitConvertFunctionInst(ConvertFunctionInst *CI) {
      *this << getIDAndType(CI->getOperand()) << " to ";
      if (CI->withoutActuallyEscaping())
         *this << "[without_actually_escaping] ";
      *this << CI->getType();
   }
   void visitConvertEscapeToNoEscapeInst(ConvertEscapeToNoEscapeInst *CI) {
      *this << (CI->isLifetimeGuaranteed() ? "" : "[not_guaranteed] ")
            << getIDAndType(CI->getOperand()) << " to " << CI->getType();
   }
   void visitThinFunctionToPointerInst(ThinFunctionToPointerInst *CI) {
      printUncheckedConversionInst(CI, CI->getOperand());
   }
   void visitPointerToThinFunctionInst(PointerToThinFunctionInst *CI) {
      printUncheckedConversionInst(CI, CI->getOperand());
   }
   void visitUpcastInst(UpcastInst *CI) {
      printUncheckedConversionInst(CI, CI->getOperand());
   }
   void visitAddressToPointerInst(AddressToPointerInst *CI) {
      printUncheckedConversionInst(CI, CI->getOperand());
   }
   void visitPointerToAddressInst(PointerToAddressInst *CI) {
      *this << getIDAndType(CI->getOperand()) << " to ";
      if (CI->isStrict())
         *this << "[strict] ";
      if (CI->isInvariant())
         *this << "[invariant] ";
      *this << CI->getType();
   }
   void visitUncheckedRefCastInst(UncheckedRefCastInst *CI) {
      printUncheckedConversionInst(CI, CI->getOperand());
   }
   void visitUncheckedRefCastAddrInst(UncheckedRefCastAddrInst *CI) {
      *this << ' ' << CI->getSourceFormalType() << " in " << getIDAndType(CI->getSrc())
            << " to " << CI->getTargetFormalType() << " in "
            << getIDAndType(CI->getDest());
   }
   void visitUncheckedAddrCastInst(UncheckedAddrCastInst *CI) {
      printUncheckedConversionInst(CI, CI->getOperand());
   }
   void visitUncheckedTrivialBitCastInst(UncheckedTrivialBitCastInst *CI) {
      printUncheckedConversionInst(CI, CI->getOperand());
   }
   void visitUncheckedBitwiseCastInst(UncheckedBitwiseCastInst *CI) {
      printUncheckedConversionInst(CI, CI->getOperand());
   }
   void visitRefToRawPointerInst(RefToRawPointerInst *CI) {
      printUncheckedConversionInst(CI, CI->getOperand());
   }
   void visitRawPointerToRefInst(RawPointerToRefInst *CI) {
      printUncheckedConversionInst(CI, CI->getOperand());
   }

#define LOADABLE_REF_STORAGE(Name, ...) \
  void visitRefTo##Name##Inst(RefTo##Name##Inst *CI) { \
    printUncheckedConversionInst(CI, CI->getOperand()); \
  } \
  void visit##Name##ToRefInst(Name##ToRefInst *CI) { \
    printUncheckedConversionInst(CI, CI->getOperand()); \
  }
#include "polarphp/ast/ReferenceStorageDef.h"
   void visitThinToThickFunctionInst(ThinToThickFunctionInst *CI) {
      printUncheckedConversionInst(CI, CI->getOperand());
   }
   // @todo
//   void visitThickToObjCMetatypeInst(ThickToObjCMetatypeInst *CI) {
//      printUncheckedConversionInst(CI, CI->getOperand());
//   }
//   void visitObjCToThickMetatypeInst(ObjCToThickMetatypeInst *CI) {
//      printUncheckedConversionInst(CI, CI->getOperand());
//   }
//   void visitObjCMetatypeToObjectInst(ObjCMetatypeToObjectInst *CI) {
//      printUncheckedConversionInst(CI, CI->getOperand());
//   }
//
//   void visitObjCExistentialMetatypeToObjectInst(
//      ObjCExistentialMetatypeToObjectInst *CI) {
//      printUncheckedConversionInst(CI, CI->getOperand());
//   }
//   void visitObjCInterfaceInst(ObjCInterfaceInst *CI) {
//      *this << "#" << CI->getInterface()->getName() << " : " << CI->getType();
//   }

   void visitRefToBridgeObjectInst(RefToBridgeObjectInst *I) {
      *this << getIDAndType(I->getConverted()) << ", "
            << getIDAndType(I->getBitsOperand());
   }

   void visitBridgeObjectToRefInst(BridgeObjectToRefInst *I) {
      printUncheckedConversionInst(I, I->getOperand());
   }
   void visitBridgeObjectToWordInst(BridgeObjectToWordInst *I) {
      printUncheckedConversionInst(I, I->getOperand());
   }

   void visitCopyValueInst(CopyValueInst *I) {
      *this << getIDAndType(I->getOperand());
   }

#define UNCHECKED_REF_STORAGE(Name, ...)                                       \
  void visitStrongCopy##Name##ValueInst(StrongCopy##Name##ValueInst *I) {      \
    *this << getIDAndType(I->getOperand());                                    \
  }

#define ALWAYS_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...)            \
  void visitStrongCopy##Name##ValueInst(StrongCopy##Name##ValueInst *I) {      \
    *this << getIDAndType(I->getOperand());                                    \
  }
#include "polarphp/ast/ReferenceStorageDef.h"

   void visitDestroyValueInst(DestroyValueInst *I) {
      *this << getIDAndType(I->getOperand());
   }

   void visitStructInst(StructInst *SI) {
      *this << SI->getType() << " (";
      interleave(SI->getElements(),
                 [&](const PILValue &V) { *this << getIDAndType(V); },
                 [&] { *this << ", "; });
      *this << ')';
   }

   void visitObjectInst(ObjectInst *OI) {
      *this << OI->getType() << " (";
      interleave(OI->getBaseElements(),
                 [&](const PILValue &V) { *this << getIDAndType(V); },
                 [&] { *this << ", "; });
      if (!OI->getTailElements().empty()) {
         *this << ", [tail_elems] ";
         interleave(OI->getTailElements(),
                    [&](const PILValue &V) { *this << getIDAndType(V); },
                    [&] { *this << ", "; });
      }
      *this << ')';
   }

   void visitTupleInst(TupleInst *TI) {

      // Check to see if the type of the tuple can be inferred accurately from the
      // elements.
      bool SimpleType = true;
      for (auto &Elt : TI->getType().castTo<TupleType>()->getElements()) {
         if (Elt.hasName() || Elt.isVararg()) {
            SimpleType = false;
            break;
         }
      }

      // If the type is simple, just print the tuple elements.
      if (SimpleType) {
         *this << '(';
         interleave(TI->getElements(),
                    [&](const PILValue &V){ *this << getIDAndType(V); },
                    [&] { *this << ", "; });
         *this << ')';
      } else {
         // Otherwise, print the type, then each value.
         *this << TI->getType() << " (";
         interleave(TI->getElements(),
                    [&](const PILValue &V) { *this << Ctx.getID(V); },
                    [&] { *this << ", "; });
         *this << ')';
      }
   }

   void visitEnumInst(EnumInst *UI) {
      *this << UI->getType() << ", "
            << PILDeclRef(UI->getElement(), PILDeclRef::Kind::EnumElement);
      if (UI->hasOperand()) {
         *this << ", " << getIDAndType(UI->getOperand());
      }
   }

   void visitInitEnumDataAddrInst(InitEnumDataAddrInst *UDAI) {
      *this << getIDAndType(UDAI->getOperand()) << ", "
            << PILDeclRef(UDAI->getElement(), PILDeclRef::Kind::EnumElement);
   }

   void visitUncheckedEnumDataInst(UncheckedEnumDataInst *UDAI) {
      *this << getIDAndType(UDAI->getOperand()) << ", "
            << PILDeclRef(UDAI->getElement(), PILDeclRef::Kind::EnumElement);
   }

   void visitUncheckedTakeEnumDataAddrInst(UncheckedTakeEnumDataAddrInst *UDAI) {
      *this << getIDAndType(UDAI->getOperand()) << ", "
            << PILDeclRef(UDAI->getElement(), PILDeclRef::Kind::EnumElement);
   }

   void visitInjectEnumAddrInst(InjectEnumAddrInst *IUAI) {
      *this << getIDAndType(IUAI->getOperand()) << ", "
            << PILDeclRef(IUAI->getElement(), PILDeclRef::Kind::EnumElement);
   }

   void visitTupleExtractInst(TupleExtractInst *EI) {
      *this << getIDAndType(EI->getOperand()) << ", " << EI->getFieldNo();
   }

   void visitTupleElementAddrInst(TupleElementAddrInst *EI) {
      *this << getIDAndType(EI->getOperand()) << ", " << EI->getFieldNo();
   }
   void visitStructExtractInst(StructExtractInst *EI) {
      *this << getIDAndType(EI->getOperand()) << ", #";
      printFullContext(EI->getField()->getDeclContext(), PrintState.OS);
      *this << EI->getField()->getName().get();
   }
   void visitStructElementAddrInst(StructElementAddrInst *EI) {
      *this << getIDAndType(EI->getOperand()) << ", #";
      printFullContext(EI->getField()->getDeclContext(), PrintState.OS);
      *this << EI->getField()->getName().get();
   }
   void visitRefElementAddrInst(RefElementAddrInst *EI) {
      *this << getIDAndType(EI->getOperand()) << ", #";
      printFullContext(EI->getField()->getDeclContext(), PrintState.OS);
      *this << EI->getField()->getName().get();
   }

   void visitRefTailAddrInst(RefTailAddrInst *RTAI) {
      *this << getIDAndType(RTAI->getOperand()) << ", " << RTAI->getTailType();
   }

   void visitDestructureStructInst(DestructureStructInst *DSI) {
      *this << getIDAndType(DSI->getOperand());
   }

   void visitDestructureTupleInst(DestructureTupleInst *DTI) {
      *this << getIDAndType(DTI->getOperand());
   }

   void printMethodInst(MethodInst *I, PILValue Operand) {
      *this << getIDAndType(Operand) << ", " << I->getMember();
   }

   void visitClassMethodInst(ClassMethodInst *AMI) {
      printMethodInst(AMI, AMI->getOperand());
      *this << " : " << AMI->getMember().getDecl()->getInterfaceType();
      *this << ", ";
      *this << AMI->getType();
   }
   void visitSuperMethodInst(SuperMethodInst *AMI) {
      printMethodInst(AMI, AMI->getOperand());
      *this << " : " << AMI->getMember().getDecl()->getInterfaceType();
      *this << ", ";
      *this << AMI->getType();
   }
   void visitObjCMethodInst(ObjCMethodInst *AMI) {
      printMethodInst(AMI, AMI->getOperand());
      *this << " : " << AMI->getMember().getDecl()->getInterfaceType();
      *this << ", ";
      *this << AMI->getType();
   }
   void visitObjCSuperMethodInst(ObjCSuperMethodInst *AMI) {
      printMethodInst(AMI, AMI->getOperand());
      *this << " : " << AMI->getMember().getDecl()->getInterfaceType();
      *this << ", ";
      *this << AMI->getType();
   }
   void visitWitnessMethodInst(WitnessMethodInst *WMI) {
      PrintOptions QualifiedPILTypeOptions =
         PrintOptions::printQualifiedPILType();
      QualifiedPILTypeOptions.CurrentModule = WMI->getModule().getTypePHPModule();
      *this << "$" << WMI->getLookupType() << ", " << WMI->getMember() << " : ";
      WMI->getMember().getDecl()->getInterfaceType().print(
         PrintState.OS, QualifiedPILTypeOptions);
      if (!WMI->getTypeDependentOperands().empty()) {
         *this << ", ";
         *this << getIDAndType(WMI->getTypeDependentOperands()[0].get());
      }
      *this << " : " << WMI->getType();
   }
   void visitOpenExistentialAddrInst(OpenExistentialAddrInst *OI) {
      if (OI->getAccessKind() == OpenedExistentialAccess::Immutable)
         *this << "immutable_access ";
      else
         *this << "mutable_access ";
      *this << getIDAndType(OI->getOperand()) << " to " << OI->getType();
   }
   void visitOpenExistentialRefInst(OpenExistentialRefInst *OI) {
      *this << getIDAndType(OI->getOperand()) << " to " << OI->getType();
   }
   void visitOpenExistentialMetatypeInst(OpenExistentialMetatypeInst *OI) {
      *this << getIDAndType(OI->getOperand()) << " to " << OI->getType();
   }
   void visitOpenExistentialBoxInst(OpenExistentialBoxInst *OI) {
      *this << getIDAndType(OI->getOperand()) << " to " << OI->getType();
   }
   void visitOpenExistentialBoxValueInst(OpenExistentialBoxValueInst *OI) {
      *this << getIDAndType(OI->getOperand()) << " to " << OI->getType();
   }
   void visitOpenExistentialValueInst(OpenExistentialValueInst *OI) {
      *this << getIDAndType(OI->getOperand()) << " to " << OI->getType();
   }
   void visitInitExistentialAddrInst(InitExistentialAddrInst *AEI) {
      *this << getIDAndType(AEI->getOperand()) << ", $"
            << AEI->getFormalConcreteType();
   }
   void visitInitExistentialValueInst(InitExistentialValueInst *AEI) {
      *this << getIDAndType(AEI->getOperand()) << ", $"
            << AEI->getFormalConcreteType() << ", " << AEI->getType();
   }
   void visitInitExistentialRefInst(InitExistentialRefInst *AEI) {
      *this << getIDAndType(AEI->getOperand()) << " : $"
            << AEI->getFormalConcreteType() << ", " << AEI->getType();
   }
   void visitInitExistentialMetatypeInst(InitExistentialMetatypeInst *AEI) {
      *this << getIDAndType(AEI->getOperand()) << ", " << AEI->getType();
   }
   void visitAllocExistentialBoxInst(AllocExistentialBoxInst *AEBI) {
      *this << AEBI->getExistentialType() << ", $"
            << AEBI->getFormalConcreteType();
   }
   void visitDeinitExistentialAddrInst(DeinitExistentialAddrInst *DEI) {
      *this << getIDAndType(DEI->getOperand());
   }
   void visitDeinitExistentialValueInst(DeinitExistentialValueInst *DEI) {
      *this << getIDAndType(DEI->getOperand());
   }
   void visitDeallocExistentialBoxInst(DeallocExistentialBoxInst *DEI) {
      *this << getIDAndType(DEI->getOperand()) << ", $" << DEI->getConcreteType();
   }
   void visitProjectBlockStorageInst(ProjectBlockStorageInst *PBSI) {
      *this << getIDAndType(PBSI->getOperand());
   }
   void visitInitBlockStorageHeaderInst(InitBlockStorageHeaderInst *IBSHI) {
      *this << getIDAndType(IBSHI->getBlockStorage()) << ", invoke "
            << Ctx.getID(IBSHI->getInvokeFunction());
      printSubstitutions(IBSHI->getSubstitutions());
      *this << " : " << IBSHI->getInvokeFunction()->getType()
            << ", type " << IBSHI->getType();
   }
   void visitValueMetatypeInst(ValueMetatypeInst *MI) {
      *this << MI->getType() << ", " << getIDAndType(MI->getOperand());
   }
   void visitExistentialMetatypeInst(ExistentialMetatypeInst *MI) {
      *this << MI->getType() << ", " << getIDAndType(MI->getOperand());
   }
   void visitMetatypeInst(MetatypeInst *MI) { *this << MI->getType(); }

   void visitFixLifetimeInst(FixLifetimeInst *RI) {
      *this << getIDAndType(RI->getOperand());
   }

   void visitEndLifetimeInst(EndLifetimeInst *ELI) {
      *this << getIDAndType(ELI->getOperand());
   }
   void visitValueToBridgeObjectInst(ValueToBridgeObjectInst *VBOI) {
      *this << getIDAndType(VBOI->getOperand());
   }
   void visitClassifyBridgeObjectInst(ClassifyBridgeObjectInst *CBOI) {
      *this << getIDAndType(CBOI->getOperand());
   }
   void visitMarkDependenceInst(MarkDependenceInst *MDI) {
      *this << getIDAndType(MDI->getValue()) << " on "
            << getIDAndType(MDI->getBase());
   }
   void visitCopyBlockInst(CopyBlockInst *RI) {
      *this << getIDAndType(RI->getOperand());
   }
   void visitCopyBlockWithoutEscapingInst(CopyBlockWithoutEscapingInst *RI) {
      *this << getIDAndType(RI->getBlock()) << " withoutEscaping "
            << getIDAndType(RI->getClosure());
   }
   void visitRefCountingInst(RefCountingInst *I) {
      if (I->isNonAtomic())
         *this << "[nonatomic] ";
      *this << getIDAndType(I->getOperand(0));
   }
   void visitIsUniqueInst(IsUniqueInst *CUI) {
      *this << getIDAndType(CUI->getOperand());
   }
   void visitIsEscapingClosureInst(IsEscapingClosureInst *CUI) {
      if (CUI->getVerificationType())
         *this << "[objc] ";
      *this << getIDAndType(CUI->getOperand());
   }
   void visitDeallocStackInst(DeallocStackInst *DI) {
      *this << getIDAndType(DI->getOperand());
   }
   void visitDeallocRefInst(DeallocRefInst *DI) {
      if (DI->canAllocOnStack())
         *this << "[stack] ";
      *this << getIDAndType(DI->getOperand());
   }
   void visitDeallocPartialRefInst(DeallocPartialRefInst *DPI) {
      *this << getIDAndType(DPI->getInstance());
      *this << ", ";
      *this << getIDAndType(DPI->getMetatype());
   }
   void visitDeallocValueBufferInst(DeallocValueBufferInst *DVBI) {
      *this << DVBI->getValueType() << " in " << getIDAndType(DVBI->getOperand());
   }
   void visitDeallocBoxInst(DeallocBoxInst *DI) {
      *this << getIDAndType(DI->getOperand());
   }
   void visitDestroyAddrInst(DestroyAddrInst *DI) {
      *this << getIDAndType(DI->getOperand());
   }
   void visitProjectValueBufferInst(ProjectValueBufferInst *PVBI) {
      *this << PVBI->getValueType() << " in " << getIDAndType(PVBI->getOperand());
   }
   void visitProjectBoxInst(ProjectBoxInst *PBI) {
      *this << getIDAndType(PBI->getOperand()) << ", " << PBI->getFieldIndex();
   }
   void visitProjectExistentialBoxInst(ProjectExistentialBoxInst *PEBI) {
      *this << PEBI->getType().getObjectType()
            << " in " << getIDAndType(PEBI->getOperand());
   }
   void visitBeginAccessInst(BeginAccessInst *BAI) {
      *this << '[' << getPILAccessKindName(BAI->getAccessKind()) << "] ["
            << getPILAccessEnforcementName(BAI->getEnforcement()) << "] "
            << (BAI->hasNoNestedConflict() ? "[no_nested_conflict] " : "")
            << (BAI->isFromBuiltin() ? "[builtin] " : "")
            << getIDAndType(BAI->getOperand());
   }
   void visitEndAccessInst(EndAccessInst *EAI) {
      *this << (EAI->isAborting() ? "[abort] " : "")
            << getIDAndType(EAI->getOperand());
   }
   void visitBeginUnpairedAccessInst(BeginUnpairedAccessInst *BAI) {
      *this << '[' << getPILAccessKindName(BAI->getAccessKind()) << "] ["
            << getPILAccessEnforcementName(BAI->getEnforcement()) << "] "
            << (BAI->hasNoNestedConflict() ? "[no_nested_conflict] " : "")
            << (BAI->isFromBuiltin() ? "[builtin] " : "")
            << getIDAndType(BAI->getSource()) << ", "
            << getIDAndType(BAI->getBuffer());
   }
   void visitEndUnpairedAccessInst(EndUnpairedAccessInst *EAI) {
      *this << (EAI->isAborting() ? "[abort] " : "") << '['
            << getPILAccessEnforcementName(EAI->getEnforcement()) << "] "
            << (EAI->isFromBuiltin() ? "[builtin] " : "")
            << getIDAndType(EAI->getOperand());
   }

   void visitCondFailInst(CondFailInst *FI) {
      *this << getIDAndType(FI->getOperand()) << ", "
            << QuotedString(FI->getMessage());
   }

   void visitIndexAddrInst(IndexAddrInst *IAI) {
      *this << getIDAndType(IAI->getBase()) << ", "
            << getIDAndType(IAI->getIndex());
   }

   void visitTailAddrInst(TailAddrInst *TAI) {
      *this << getIDAndType(TAI->getBase()) << ", "
            << getIDAndType(TAI->getIndex()) << ", " << TAI->getTailType();
   }

   void visitIndexRawPointerInst(IndexRawPointerInst *IAI) {
      *this << getIDAndType(IAI->getBase()) << ", "
            << getIDAndType(IAI->getIndex());
   }

   void visitUnreachableInst(UnreachableInst *UI) {}

   void visitReturnInst(ReturnInst *RI) {
      *this << getIDAndType(RI->getOperand());
   }

   void visitThrowInst(ThrowInst *TI) {
      *this << getIDAndType(TI->getOperand());
   }

   void visitUnwindInst(UnwindInst *UI) {
      // no operands
   }

   void visitYieldInst(YieldInst *YI) {
      auto values = YI->getYieldedValues();
      if (values.size() != 1) *this << '(';
      interleave(values,
                 [&](PILValue value) { *this << getIDAndType(value); },
                 [&] { *this << ", "; });
      if (values.size() != 1) *this << ')';
      *this << ", resume " << Ctx.getID(YI->getResumeBB())
            << ", unwind " << Ctx.getID(YI->getUnwindBB());
   }

   void visitSwitchValueInst(SwitchValueInst *SII) {
      *this << getIDAndType(SII->getOperand());
      for (unsigned i = 0, e = SII->getNumCases(); i < e; ++i) {
         PILValue value;
         PILBasicBlock *dest;
         std::tie(value, dest) = SII->getCase(i);
         *this << ", case " << Ctx.getID(value) << ": " << Ctx.getID(dest);
      }
      if (SII->hasDefault())
         *this << ", default " << Ctx.getID(SII->getDefaultBB());
   }

   void printSwitchEnumInst(SwitchEnumInstBase *SOI) {
      *this << getIDAndType(SOI->getOperand());
      for (unsigned i = 0, e = SOI->getNumCases(); i < e; ++i) {
         EnumElementDecl *elt;
         PILBasicBlock *dest;
         std::tie(elt, dest) = SOI->getCase(i);
         *this << ", case " << PILDeclRef(elt, PILDeclRef::Kind::EnumElement)
               << ": " << Ctx.getID(dest);
         if (SOI->getCaseCount(i)) {
            *this << " !case_count(" << SOI->getCaseCount(i).getValue() << ")";
         }
      }
      if (SOI->hasDefault()) {
         *this << ", default " << Ctx.getID(SOI->getDefaultBB());
         if (SOI->getDefaultCount()) {
            *this << " !default_count(" << SOI->getDefaultCount().getValue() << ")";
         }
      }
   }

   void visitSwitchEnumInst(SwitchEnumInst *SOI) {
      printSwitchEnumInst(SOI);
   }
   void visitSwitchEnumAddrInst(SwitchEnumAddrInst *SOI) {
      printSwitchEnumInst(SOI);
   }

   void printSelectEnumInst(SelectEnumInstBase *SEI) {
      *this << getIDAndType(SEI->getEnumOperand());

      for (unsigned i = 0, e = SEI->getNumCases(); i < e; ++i) {
         EnumElementDecl *elt;
         PILValue result;
         std::tie(elt, result) = SEI->getCase(i);
         *this << ", case " << PILDeclRef(elt, PILDeclRef::Kind::EnumElement)
               << ": " << Ctx.getID(result);
      }
      if (SEI->hasDefault())
         *this << ", default " << Ctx.getID(SEI->getDefaultResult());

      *this << " : " << SEI->getType();
   }

   void visitSelectEnumInst(SelectEnumInst *SEI) {
      printSelectEnumInst(SEI);
   }
   void visitSelectEnumAddrInst(SelectEnumAddrInst *SEI) {
      printSelectEnumInst(SEI);
   }

   void visitSelectValueInst(SelectValueInst *SVI) {
      *this << getIDAndType(SVI->getOperand());

      for (unsigned i = 0, e = SVI->getNumCases(); i < e; ++i) {
         PILValue casevalue;
         PILValue result;
         std::tie(casevalue, result) = SVI->getCase(i);
         *this << ", case " << Ctx.getID(casevalue) << ": " << Ctx.getID(result);
      }
      if (SVI->hasDefault())
         *this << ", default " << Ctx.getID(SVI->getDefaultResult());

      *this << " : " << SVI->getType();
   }

   void visitDynamicMethodBranchInst(DynamicMethodBranchInst *DMBI) {
      *this << getIDAndType(DMBI->getOperand()) << ", " << DMBI->getMember()
            << ", " << Ctx.getID(DMBI->getHasMethodBB()) << ", "
            << Ctx.getID(DMBI->getNoMethodBB());
   }

   void printBranchArgs(OperandValueArrayRef args) {
      if (args.empty()) return;

      *this << '(';
      interleave(args,
                 [&](PILValue v) { *this << getIDAndType(v); },
                 [&] { *this << ", "; });
      *this << ')';
   }

   void visitBranchInst(BranchInst *UBI) {
      *this << Ctx.getID(UBI->getDestBB());
      printBranchArgs(UBI->getArgs());
   }

   void visitCondBranchInst(CondBranchInst *CBI) {
      *this << Ctx.getID(CBI->getCondition()) << ", "
            << Ctx.getID(CBI->getTrueBB());
      printBranchArgs(CBI->getTrueArgs());
      *this << ", " << Ctx.getID(CBI->getFalseBB());
      printBranchArgs(CBI->getFalseArgs());
      if (CBI->getTrueBBCount())
         *this << " !true_count(" << CBI->getTrueBBCount().getValue() << ")";
      if (CBI->getFalseBBCount())
         *this << " !false_count(" << CBI->getFalseBBCount().getValue() << ")";
   }

   void visitKeyPathInst(KeyPathInst *KPI) {
      *this << KPI->getType() << ", ";

      auto pattern = KPI->getPattern();

      if (pattern->getGenericSignature()) {
         pattern->getGenericSignature()->print(PrintState.OS);
         *this << ' ';
      }

      *this << "(";

      if (!pattern->getObjCString().empty())
         *this << "objc \"" << pattern->getObjCString() << "\"; ";

      *this << "root $" << KPI->getPattern()->getRootType();

      for (auto &component : pattern->getComponents()) {
         *this << "; ";

         printKeyPathPatternComponent(component);
      }

      *this << ')';
      if (!KPI->getSubstitutions().empty()) {
         *this << ' ';
         printSubstitutions(KPI->getSubstitutions());
      }
      if (!KPI->getAllOperands().empty()) {
         *this << " (";

         interleave(KPI->getAllOperands(),
                    [&](const Operand &operand) {
                       *this << Ctx.getID(operand.get());
                    }, [&]{
               *this << ", ";
            });

         *this << ")";
      }
   }

   void
   printKeyPathPatternComponent(const KeyPathPatternComponent &component) {
      auto printComponentIndices =
         [&](ArrayRef<KeyPathPatternComponent::Index> indices) {
            *this << '[';
            interleave(indices,
                       [&](const KeyPathPatternComponent::Index &i) {
                          *this << "%$" << i.Operand << " : $"
                                << i.FormalType << " : "
                                << i.LoweredType;
                       }, [&]{
                  *this << ", ";
               });
            *this << ']';
         };

      switch (auto kind = component.getKind()) {
         case KeyPathPatternComponent::Kind::StoredProperty: {
            auto prop = component.getStoredPropertyDecl();
            *this << "stored_property #";
            printValueDecl(prop, PrintState.OS);
            *this << " : $" << component.getComponentType();
            break;
         }
         case KeyPathPatternComponent::Kind::GettableProperty:
         case KeyPathPatternComponent::Kind::SettableProperty: {
            *this << (kind == KeyPathPatternComponent::Kind::GettableProperty
                      ? "gettable_property $" : "settable_property $")
                  << component.getComponentType() << ", "
                  << " id ";
            auto id = component.getComputedPropertyId();
            switch (id.getKind()) {
               case KeyPathPatternComponent::ComputedPropertyId::DeclRef: {
                  auto declRef = id.getDeclRef();
                  *this << declRef << " : "
                        << declRef.getDecl()->getInterfaceType();
                  break;
               }
               case KeyPathPatternComponent::ComputedPropertyId::Function: {
                  id.getFunction()->printName(PrintState.OS);
                  *this << " : " << id.getFunction()->getLoweredType();
                  break;
               }
               case KeyPathPatternComponent::ComputedPropertyId::Property: {
                  *this << "##";
                  printValueDecl(id.getProperty(), PrintState.OS);
                  break;
               }
            }
            *this << ", getter ";
            component.getComputedPropertyGetter()->printName(PrintState.OS);
            *this << " : "
                  << component.getComputedPropertyGetter()->getLoweredType();
            if (kind == KeyPathPatternComponent::Kind::SettableProperty) {
               *this << ", setter ";
               component.getComputedPropertySetter()->printName(PrintState.OS);
               *this << " : "
                     << component.getComputedPropertySetter()->getLoweredType();
            }

            if (!component.getSubscriptIndices().empty()) {
               *this << ", indices ";
               printComponentIndices(component.getSubscriptIndices());
               *this << ", indices_equals ";
               component.getSubscriptIndexEquals()->printName(PrintState.OS);
               *this << " : "
                     << component.getSubscriptIndexEquals()->getLoweredType();
               *this << ", indices_hash ";
               component.getSubscriptIndexHash()->printName(PrintState.OS);
               *this << " : "
                     << component.getSubscriptIndexHash()->getLoweredType();
            }

            if (auto external = component.getExternalDecl()) {
               *this << ", external #";
               printValueDecl(external, PrintState.OS);
               auto subs = component.getExternalSubstitutions();
               if (!subs.empty()) {
                  printSubstitutions(subs);
               }
            }

            break;
         }
         case KeyPathPatternComponent::Kind::OptionalWrap:
         case KeyPathPatternComponent::Kind::OptionalChain:
         case KeyPathPatternComponent::Kind::OptionalForce: {
            switch (kind) {
               case KeyPathPatternComponent::Kind::OptionalWrap:
                  *this << "optional_wrap : $";
                  break;
               case KeyPathPatternComponent::Kind::OptionalChain:
                  *this << "optional_chain : $";
                  break;
               case KeyPathPatternComponent::Kind::OptionalForce:
                  *this << "optional_force : $";
                  break;
               default:
                  llvm_unreachable("out of sync");
            }
            *this << component.getComponentType();
            break;
         }
         case KeyPathPatternComponent::Kind::TupleElement: {
            *this << "tuple_element #" << component.getTupleIndex();
            *this << " : $" << component.getComponentType();
            break;
         }
      }
   }
};
} // end anonymous namespace

static void printBlockID(raw_ostream &OS, PILBasicBlock *bb) {
   PILPrintContext Ctx(OS);
   OS << Ctx.getID(bb);
}

void PILBasicBlock::printAsOperand(raw_ostream &OS, bool PrintType) {
   printBlockID(OS, this);
}

//===----------------------------------------------------------------------===//
// Printing for PILInstruction, PILBasicBlock, PILFunction, and PILModule
//===----------------------------------------------------------------------===//

void PILNode::dump() const {
   print(llvm::errs());
}

void PILNode::print(raw_ostream &OS) const {
   PILPrintContext Ctx(OS);
   PILPrinter(Ctx).print(this);
}

void PILInstruction::dump() const {
   print(llvm::errs());
}

void SingleValueInstruction::dump() const {
   PILInstruction::dump();
}

void PILInstruction::print(raw_ostream &OS) const {
   PILPrintContext Ctx(OS);
   PILPrinter(Ctx).print(this);
}

/// Pretty-print the PILBasicBlock to errs.
void PILBasicBlock::dump() const {
   print(llvm::errs());
}

/// Pretty-print the PILBasicBlock to the designated stream.
void PILBasicBlock::print(raw_ostream &OS) const {
   PILPrintContext Ctx(OS);

   // Print the debug scope (and compute if we didn't do it already).
   auto &SM = this->getParent()->getModule().getAstContext().SourceMgr;
   for (auto &I : *this) {
      PILPrinter P(Ctx);
      P.printDebugScope(I.getDebugScope(), SM);
   }

   PILPrinter(Ctx).print(this);
}

void PILBasicBlock::print(raw_ostream &OS, PILPrintContext &Ctx) const {
   PILPrinter(Ctx).print(this);
}

/// Pretty-print the PILFunction to errs.
void PILFunction::dump(bool Verbose) const {
   PILPrintContext Ctx(llvm::errs(), Verbose);
   print(Ctx);
}

// This is out of line so the debugger can find it.
void PILFunction::dump() const {
   dump(false);
}

void PILFunction::dump(const char *FileName) const {
   std::error_code EC;
   llvm::raw_fd_ostream os(FileName, EC, llvm::sys::fs::OpenFlags::F_None);
   print(os);
}

static StringRef getLinkageString(PILLinkage linkage) {
   switch (linkage) {
      case PILLinkage::Public: return "public ";
      case PILLinkage::PublicNonABI: return "non_abi ";
      case PILLinkage::Hidden: return "hidden ";
      case PILLinkage::Shared: return "shared ";
      case PILLinkage::Private: return "private ";
      case PILLinkage::PublicExternal: return "public_external ";
      case PILLinkage::HiddenExternal: return "hidden_external ";
      case PILLinkage::SharedExternal: return "shared_external ";
      case PILLinkage::PrivateExternal: return "private_external ";
   }
   llvm_unreachable("bad linkage");
}

static void printLinkage(llvm::raw_ostream &OS, PILLinkage linkage,
                         bool isDefinition) {
   if ((isDefinition && linkage == PILLinkage::DefaultForDefinition) ||
       (!isDefinition && linkage == PILLinkage::DefaultForDeclaration))
      return;

   OS << getLinkageString(linkage);
}

/// Pretty-print the PILFunction to the designated stream.
void PILFunction::print(PILPrintContext &PrintCtx) const {
   llvm::raw_ostream &OS = PrintCtx.OS();
   if (PrintCtx.printDebugInfo()) {
      auto &SM = getModule().getAstContext().SourceMgr;
      for (auto &BB : *this)
         for (auto &I : BB) {
            PILPrinter P(PrintCtx);
            P.printDebugScope(I.getDebugScope(), SM);
         }
      OS << "\n";
   }

   if (PILPrintGenericSpecializationInfo) {
      if (isSpecialization()) {
         printGenericSpecializationInfo(OS, "function", getName(),
                                        getSpecializationInfo());
      }
   }

   OS << "// " << demangleSymbol(getName()) << '\n';
   OS << "pil ";
   printLinkage(OS, getLinkage(), isDefinition());

   if (isTransparent())
      OS << "[transparent] ";

   switch (isSerialized()) {
      case IsNotSerialized: break;
      case IsSerializable: OS << "[serializable] "; break;
      case IsSerialized: OS << "[serialized] "; break;
   }

   switch (isThunk()) {
      case IsNotThunk: break;
      case IsThunk: OS << "[thunk] "; break;
      case IsSignatureOptimizedThunk:
         OS << "[signature_optimized_thunk] ";
         break;
      case IsReabstractionThunk: OS << "[reabstraction_thunk] "; break;
   }
   if (isDynamicallyReplaceable()) {
      OS << "[dynamically_replacable] ";
   }
   if (isExactSelfClass()) {
      OS << "[exact_self_class] ";
   }
   if (isWithoutActuallyEscapingThunk())
      OS << "[without_actually_escaping] ";

   if (isGlobalInit())
      OS << "[global_init] ";
   if (isAlwaysWeakImported())
      OS << "[weak_imported] ";
   auto availability = getAvailabilityForLinkage();
   if (!availability.isAlwaysAvailable()) {
      auto version = availability.getOSVersion().getLowerEndpoint();
      OS << "[available " << version.getAsString() << "] ";
   }

   switch (getInlineStrategy()) {
      case NoInline: OS << "[noinline] "; break;
      case AlwaysInline: OS << "[always_inline] "; break;
      case InlineDefault: break;
   }

   switch (getOptimizationMode()) {
      case OptimizationMode::NoOptimization: OS << "[Onone] "; break;
      case OptimizationMode::ForSpeed: OS << "[Ospeed] "; break;
      case OptimizationMode::ForSize: OS << "[Osize] "; break;
      default: break;
   }

   if (getEffectsKind() == EffectsKind::ReadOnly)
      OS << "[readonly] ";
   else if (getEffectsKind() == EffectsKind::ReadNone)
      OS << "[readnone] ";
   else if (getEffectsKind() == EffectsKind::ReadWrite)
      OS << "[readwrite] ";
   else if (getEffectsKind() == EffectsKind::ReleaseNone)
      OS << "[releasenone] ";

   if (auto *replacedFun = getDynamicallyReplacedFunction()) {
      OS << "[dynamic_replacement_for \"";
      OS << replacedFun->getName();
      OS << "\"] ";
   }

   if (hasObjCReplacement()) {
      OS << "[objc_replacement_for \"";
      OS << getObjCReplacement().str();
      OS << "\"] ";
   }

   for (auto &Attr : getSemanticsAttrs())
      OS << "[_semantics \"" << Attr << "\"] ";

   for (auto *Attr : getSpecializeAttrs()) {
      OS << "[_specialize "; Attr->print(OS); OS << "] ";
   }

   // TODO: Handle clang node owners which don't have a name.
   if (hasClangNode() && getClangNodeOwner()->hasName()) {
      OS << "[clang ";
      printValueDecl(getClangNodeOwner(), OS);
      OS << "] ";
   }

   // Handle functions that are deserialized from canonical PIL. Normally, we
   // should emit PIL with the correct PIL stage, so preserving this attribute
   // won't be necessary. But consider serializing raw PIL (either textual PIL or
   // SIB) after importing canonical PIL from another module. If the imported
   // functions are reserialized (e.g. shared linkage), then we must preserve
   // this attribute.
   if (WasDeserializedCanonical && getModule().getStage() == PILStage::Raw)
      OS << "[canonical] ";

   // If this function is not an external declaration /and/ is in ownership ssa
   // form, print [ossa].
   if (!isExternalDeclaration() && hasOwnership())
      OS << "[ossa] ";

   printName(OS);
   OS << " : $";

   // Print the type by substituting our context parameter names for the dependent
   // parameters. In PIL, we may end up with multiple generic parameters that
   // have the same name from different contexts, for instance, a generic
   // protocol requirement with a generic method parameter <T>, which is
   // witnessed by a generic type that has a generic type parameter also named
   // <T>, so we may need to introduce disambiguating aliases.
   llvm::DenseMap<CanType, Identifier> Aliases;
   llvm::DenseSet<Identifier> UsedNames;

   auto sig = getLoweredFunctionType()->getSubstGenericSignature();
   auto *env = getGenericEnvironment();
   if (sig && env) {
      llvm::SmallString<16> disambiguatedNameBuf;
      unsigned disambiguatedNameCounter = 1;
      for (auto *paramTy : sig->getGenericParams()) {
         auto sugaredTy = env->getSugaredType(paramTy);
         Identifier name = sugaredTy->getName();
         while (!UsedNames.insert(name).second) {
            disambiguatedNameBuf.clear();
            {
               llvm::raw_svector_ostream names(disambiguatedNameBuf);
               names << sugaredTy->getName() << disambiguatedNameCounter++;
            }
            name = getAstContext().getIdentifier(disambiguatedNameBuf);
         }
         if (name != sugaredTy->getName()) {
            Aliases[paramTy->getCanonicalType()] = name;

            // Also for the archetype
            auto archetypeTy = env->mapTypeIntoContext(paramTy)
               ->getAs<ArchetypeType>();
            if (archetypeTy)
               Aliases[archetypeTy->getCanonicalType()] = name;
         }
      }
   }

   {
      PrintOptions withGenericEnvironment = PrintOptions::printPIL();
      withGenericEnvironment.GenericEnv = env;
      withGenericEnvironment.AlternativeTypeNames =
         Aliases.empty() ? nullptr : &Aliases;
      LoweredType->print(OS, withGenericEnvironment);
   }

   if (!isExternalDeclaration()) {
      if (auto eCount = getEntryCount()) {
         OS << " !function_entry_count(" << eCount.getValue() << ")";
      }
      OS << " {\n";

      PILPrinter(PrintCtx, (Aliases.empty() ? nullptr : &Aliases))
         .print(this);
      OS << "} // end pil function '" << getName() << '\'';
   }

   OS << "\n\n";
}

/// Pretty-print the PILFunction's name using PIL syntax,
/// '@function_mangled_name'.
void PILFunction::printName(raw_ostream &OS) const {
   OS << "@" << Name;
}

/// Pretty-print a global variable to the designated stream.
void PILGlobalVariable::print(llvm::raw_ostream &OS, bool Verbose) const {
   OS << "// " << demangleSymbol(getName()) << '\n';

   OS << "pil_global ";
   printLinkage(OS, getLinkage(), isDefinition());

   if (isSerialized())
      OS << "[serialized] ";

   if (isLet())
      OS << "[let] ";

   printName(OS);
   OS << " : " << LoweredType;

   if (!StaticInitializerBlock.empty()) {
      OS << " = {\n";
      {
         PILPrintContext Ctx(OS);
         PILPrinter Printer(Ctx);
         for (const PILInstruction &I : StaticInitializerBlock) {
            Printer.print(&I);
         }
      }
      OS << "}\n";
   }

   OS << "\n\n";
}

void PILGlobalVariable::dump(bool Verbose) const {
   print(llvm::errs(), Verbose);
}
void PILGlobalVariable::dump() const {
   dump(false);
}

void PILGlobalVariable::printName(raw_ostream &OS) const {
   OS << "@" << Name;
}

/// Pretty-print the PILModule to errs.
void PILModule::dump(bool Verbose) const {
   PILPrintContext Ctx(llvm::errs(), Verbose);
   print(Ctx);
}

void PILModule::dump(const char *FileName, bool Verbose,
                     bool PrintASTDecls) const {
   std::error_code EC;
   llvm::raw_fd_ostream os(FileName, EC, llvm::sys::fs::OpenFlags::F_None);
   PILPrintContext Ctx(os, Verbose);
   print(Ctx, getTypePHPModule(), PrintASTDecls);
}

static void printPILGlobals(PILPrintContext &Ctx,
                            const PILModule::GlobalListType &Globals) {
   if (!Ctx.sortPIL()) {
      for (const PILGlobalVariable &g : Globals)
         g.print(Ctx.OS(), Ctx.printVerbose());
      return;
   }

   std::vector<const PILGlobalVariable *> globals;
   globals.reserve(Globals.size());
   for (const PILGlobalVariable &g : Globals)
      globals.push_back(&g);
   std::sort(globals.begin(), globals.end(),
             [] (const PILGlobalVariable *g1, const PILGlobalVariable *g2) -> bool {
                return g1->getName().compare(g2->getName()) == -1;
             }
   );
   for (const PILGlobalVariable *g : globals)
      g->print(Ctx.OS(), Ctx.printVerbose());
}

static void printPILFunctions(PILPrintContext &Ctx,
                              const PILModule::FunctionListType &Functions) {
   if (!Ctx.sortPIL()) {
      for (const PILFunction &f : Functions)
         f.print(Ctx);
      return;
   }

   std::vector<const PILFunction *> functions;
   functions.reserve(Functions.size());
   for (const PILFunction &f : Functions)
      functions.push_back(&f);
   std::sort(functions.begin(), functions.end(),
             [] (const PILFunction *f1, const PILFunction *f2) -> bool {
                return f1->getName().compare(f2->getName()) == -1;
             }
   );
   for (const PILFunction *f : functions)
      f->print(Ctx);
}

static void printPILVTables(PILPrintContext &Ctx,
                            const PILModule::VTableListType &VTables) {
   if (!Ctx.sortPIL()) {
      for (const PILVTable &vt : VTables)
         vt.print(Ctx.OS(), Ctx.printVerbose());
      return;
   }

   std::vector<const PILVTable *> vtables;
   vtables.reserve(VTables.size());
   for (const PILVTable &vt : VTables)
      vtables.push_back(&vt);
   std::sort(vtables.begin(), vtables.end(),
             [] (const PILVTable *v1, const PILVTable *v2) -> bool {
                StringRef Name1 = v1->getClass()->getName().str();
                StringRef Name2 = v2->getClass()->getName().str();
                return Name1.compare(Name2) == -1;
             }
   );
   for (const PILVTable *vt : vtables)
      vt->print(Ctx.OS(), Ctx.printVerbose());
}

static void
printPILWitnessTables(PILPrintContext &Ctx,
                      const PILModule::WitnessTableListType &WTables) {
   if (!Ctx.sortPIL()) {
      for (const PILWitnessTable &wt : WTables)
         wt.print(Ctx.OS(), Ctx.printVerbose());
      return;
   }

   std::vector<const PILWitnessTable *> witnesstables;
   witnesstables.reserve(WTables.size());
   for (const PILWitnessTable &wt : WTables)
      witnesstables.push_back(&wt);
   std::sort(witnesstables.begin(), witnesstables.end(),
             [] (const PILWitnessTable *w1, const PILWitnessTable *w2) -> bool {
                return w1->getName().compare(w2->getName()) == -1;
             }
   );
   for (const PILWitnessTable *wt : witnesstables)
      wt->print(Ctx.OS(), Ctx.printVerbose());
}

static void
printPILDefaultWitnessTables(PILPrintContext &Ctx,
                             const PILModule::DefaultWitnessTableListType &WTables) {
   if (!Ctx.sortPIL()) {
      for (const PILDefaultWitnessTable &wt : WTables)
         wt.print(Ctx.OS(), Ctx.printVerbose());
      return;
   }

   std::vector<const PILDefaultWitnessTable *> witnesstables;
   witnesstables.reserve(WTables.size());
   for (const PILDefaultWitnessTable &wt : WTables)
      witnesstables.push_back(&wt);
   std::sort(witnesstables.begin(), witnesstables.end(),
             [] (const PILDefaultWitnessTable *w1,
                 const PILDefaultWitnessTable *w2) -> bool {
                return w1->getInterface()->getName()
                          .compare(w2->getInterface()->getName()) == -1;
             }
   );
   for (const PILDefaultWitnessTable *wt : witnesstables)
      wt->print(Ctx.OS(), Ctx.printVerbose());
}

static void
printPILCoverageMaps(PILPrintContext &Ctx,
                     const PILModule::CoverageMapCollectionType &CoverageMaps) {
   if (!Ctx.sortPIL()) {
      for (const auto &M : CoverageMaps)
         M.second->print(Ctx);
      return;
   }

   std::vector<const PILCoverageMap *> Maps;
   Maps.reserve(CoverageMaps.size());
   for (const auto &M : CoverageMaps)
      Maps.push_back(M.second);
   std::sort(Maps.begin(), Maps.end(),
             [](const PILCoverageMap *LHS, const PILCoverageMap *RHS) -> bool {
                return LHS->getName().compare(RHS->getName()) == -1;
             });
   for (const PILCoverageMap *M : Maps)
      M->print(Ctx);
}

void PILProperty::print(PILPrintContext &Ctx) const {
   PrintOptions Options = PrintOptions::printPIL();

   auto &OS = Ctx.OS();
   OS << "pil_property ";
   if (isSerialized())
      OS << "[serialized] ";

   OS << '#';
   printValueDecl(getDecl(), OS);
   if (auto sig = getDecl()->getInnermostDeclContext()
      ->getGenericSignatureOfContext()) {
      sig->getCanonicalSignature()->print(OS, Options);
   }
   OS << " (";
   if (auto component = getComponent())
      PILPrinter(Ctx).printKeyPathPatternComponent(*component);
   OS << ")\n";
}

void PILProperty::dump() const {
   PILPrintContext context(llvm::errs());
   print(context);
}

static void printPILProperties(PILPrintContext &Ctx,
                               const PILModule::PropertyListType &Properties) {
   for (const PILProperty &P : Properties) {
      P.print(Ctx);
   }
}

static void printExternallyVisibleDecls(PILPrintContext &Ctx,
                                        ArrayRef<ValueDecl *> decls) {
   if (decls.empty())
      return;
   Ctx.OS() << "/* externally visible decls: \n";
   for (ValueDecl *decl : decls) {
      printValueDecl(decl, Ctx.OS());
      Ctx.OS() << '\n';
   }
   Ctx.OS() << "*/\n";
}

/// Pretty-print the PILModule to the designated stream.
void PILModule::print(PILPrintContext &PrintCtx, ModuleDecl *M,
                      bool PrintASTDecls) const {
   llvm::raw_ostream &OS = PrintCtx.OS();
   OS << "pil_stage ";
   switch (Stage) {
      case PILStage::Raw:
         OS << "raw";
         break;
      case PILStage::Canonical:
         OS << "canonical";
         break;
      case PILStage::Lowered:
         OS << "lowered";
         break;
   }

   OS << "\n\nimport " << BUILTIN_NAME
      << "\nimport " << STDLIB_NAME
      << "\nimport " << POLAR_SHIMS_NAME << "\n\n";

   // Print the declarations and types from the associated context (origin module or
   // current file).
   if (M && PrintASTDecls) {
      PrintOptions Options = PrintOptions::printPIL();
      Options.TypeDefinitions = true;
      Options.VarInitializers = true;
      // FIXME: ExplodePatternBindingDecls is incompatible with VarInitializers!
      Options.ExplodePatternBindingDecls = true;
      Options.SkipImplicit = false;
      Options.PrintGetSetOnRWProperties = true;
      Options.PrintInPILBody = false;
      bool WholeModuleMode = (M == AssociatedDeclContext);

      SmallVector<Decl *, 32> topLevelDecls;
      M->getTopLevelDecls(topLevelDecls);
      for (const Decl *D : topLevelDecls) {
         if (!WholeModuleMode && !(D->getDeclContext() == AssociatedDeclContext))
            continue;
         if ((isa<ValueDecl>(D) || isa<OperatorDecl>(D) ||
              isa<ExtensionDecl>(D) || isa<ImportDecl>(D)) &&
             !D->isImplicit()) {
            if (isa<AccessorDecl>(D))
               continue;

            // skip to visit ASTPrinter to avoid pil-opt prints duplicated import declarations
            if (auto importDecl = dyn_cast<ImportDecl>(D)) {
               StringRef importName = importDecl->getModule()->getName().str();
               if (importName == BUILTIN_NAME ||
                   importName == STDLIB_NAME ||
                   importName == POLAR_SHIMS_NAME)
                  continue;
            }
            D->print(OS, Options);
            OS << "\n\n";
         }
      }
   }

   printPILGlobals(PrintCtx, getPILGlobalList());
   printPILFunctions(PrintCtx, getFunctionList());
   printPILVTables(PrintCtx, getVTableList());
   printPILWitnessTables(PrintCtx, getWitnessTableList());
   printPILDefaultWitnessTables(PrintCtx, getDefaultWitnessTableList());
   printPILCoverageMaps(PrintCtx, getCoverageMaps());
   printPILProperties(PrintCtx, getPropertyList());
   printExternallyVisibleDecls(PrintCtx, externallyVisible.getArrayRef());

   OS << "\n\n";
}

void PILNode::dumpInContext() const {
   printInContext(llvm::errs());
}
void PILNode::printInContext(llvm::raw_ostream &OS) const {
   PILPrintContext Ctx(OS);
   PILPrinter(Ctx).printInContext(this);
}

void PILInstruction::dumpInContext() const {
   printInContext(llvm::errs());
}
void PILInstruction::printInContext(llvm::raw_ostream &OS) const {
   PILPrintContext Ctx(OS);
   PILPrinter(Ctx).printInContext(this);
}

void PILVTable::print(llvm::raw_ostream &OS, bool Verbose) const {
   OS << "pil_vtable ";
   if (isSerialized())
      OS << "[serialized] ";
   OS << getClass()->getName() << " {\n";

   PrintOptions QualifiedPILTypeOptions = PrintOptions::printQualifiedPILType();
   for (auto &entry : getEntries()) {
      OS << "  ";
      entry.Method.print(OS);
      OS << ": ";

      bool HasSingleImplementation = false;
      switch (entry.Method.kind) {
         default:
            break;
         case PILDeclRef::Kind::IVarDestroyer:
         case PILDeclRef::Kind::Destroyer:
         case PILDeclRef::Kind::Deallocator:
            HasSingleImplementation = true;
      }
      // No need to emit the signature for methods that may have only
      // single implementation, e.g. for destructors.
      if (!HasSingleImplementation) {
         QualifiedPILTypeOptions.CurrentModule =
            entry.Method.getDecl()->getDeclContext()->getParentModule();
         entry.Method.getDecl()->getInterfaceType().print(OS,
                                                          QualifiedPILTypeOptions);
         OS << " : ";
      }
      OS << '@' << entry.Implementation->getName();
      switch (entry.TheKind) {
         case PILVTable::Entry::Kind::Normal:
            break;
         case PILVTable::Entry::Kind::Inherited:
            OS << " [inherited]";
            break;
         case PILVTable::Entry::Kind::Override:
            OS << " [override]";
            break;
      }
      OS << "\t// " << demangleSymbol(entry.Implementation->getName());
      OS << "\n";
   }
   OS << "}\n\n";
}

void PILVTable::dump() const {
   print(llvm::errs());
}

/// Returns true if anything was printed.
static bool printAssociatedTypePath(llvm::raw_ostream &OS, CanType path) {
   if (auto memberType = dyn_cast<DependentMemberType>(path)) {
      if (printAssociatedTypePath(OS, memberType.getBase()))
         OS << '.';
      OS << memberType->getName().str();
      return true;
   } else {
      assert(isa<GenericTypeParamType>(path));
      return false;
   }
}

void PILWitnessTable::Entry::print(llvm::raw_ostream &out, bool verbose,
                                   const PrintOptions &options) const {
   PrintOptions QualifiedPILTypeOptions = PrintOptions::printQualifiedPILType();
   out << "  ";
   switch (getKind()) {
      case WitnessKind::Invalid:
         out << "no_default";
         break;
      case WitnessKind::Method: {
         // method #declref: @function
         auto &methodWitness = getMethodWitness();
         out << "method ";
         methodWitness.Requirement.print(out);
         out << ": ";
         QualifiedPILTypeOptions.CurrentModule =
            methodWitness.Requirement.getDecl()
               ->getDeclContext()
               ->getParentModule();
         methodWitness.Requirement.getDecl()->getInterfaceType().print(
            out, QualifiedPILTypeOptions);
         out << " : ";
         if (methodWitness.Witness) {
            methodWitness.Witness->printName(out);
            out << "\t// "
                << demangleSymbol(methodWitness.Witness->getName());
         } else {
            out << "nil";
         }
         break;
      }
      case WitnessKind::AssociatedType: {
         // associated_type AssociatedTypeName: ConformingType
         auto &assocWitness = getAssociatedTypeWitness();
         out << "associated_type ";
         out << assocWitness.Requirement->getName() << ": ";
         assocWitness.Witness->print(out, options);
         break;
      }
      case WitnessKind::AssociatedTypeInterface: {
         // associated_type_protocol (AssociatedTypeName: Interface): <conformance>
         auto &assocProtoWitness = getAssociatedTypeInterfaceWitness();
         out << "associated_type_interface (";
         (void) printAssociatedTypePath(out, assocProtoWitness.Requirement);
         out << ": " << assocProtoWitness.Interface->getName() << "): ";
         if (assocProtoWitness.Witness.isConcrete())
            assocProtoWitness.Witness.getConcrete()->printName(out, options);
         else
            out << "dependent";
         break;
      }
      case WitnessKind::BaseInterface: {
         // base_protocol Interface: <conformance>
         auto &baseProtoWitness = getBaseInterfaceWitness();
         out << "base_protocol "
             << baseProtoWitness.Requirement->getName() << ": ";
         baseProtoWitness.Witness->printName(out, options);
         break;
      }
   }
   out << '\n';
}

void PILWitnessTable::print(llvm::raw_ostream &OS, bool Verbose) const {
   PrintOptions Options = PrintOptions::printPIL();
   PrintOptions QualifiedPILTypeOptions = PrintOptions::printQualifiedPILType();
   OS << "pil_witness_table ";
   printLinkage(OS, getLinkage(), /*isDefinition*/ isDefinition());
   if (isSerialized())
      OS << "[serialized] ";

   getConformance()->printName(OS, Options);
   Options.GenericEnv =
      getConformance()->getDeclContext()->getGenericEnvironmentOfContext();

   if (isDeclaration()) {
      OS << "\n\n";
      return;
   }

   OS << " {\n";

   for (auto &witness : getEntries()) {
      witness.print(OS, Verbose, Options);
   }

   for (auto conditionalConformance : getConditionalConformances()) {
      // conditional_conformance (TypeName: Interface):
      // <conformance>
      OS << "  conditional_conformance (";
      conditionalConformance.Requirement.print(OS, Options);
      OS << ": " << conditionalConformance.Conformance.getRequirement()->getName()
         << "): ";
      if (conditionalConformance.Conformance.isConcrete())
         conditionalConformance.Conformance.getConcrete()->printName(OS, Options);
      else
         OS << "dependent";

      OS << '\n';
   }

   OS << "}\n\n";
}

void PILWitnessTable::dump() const {
   print(llvm::errs());
}

void PILDefaultWitnessTable::print(llvm::raw_ostream &OS, bool Verbose) const {
   // pil_default_witness_table [<Linkage>] <Interface> <MinSize>
   PrintOptions QualifiedPILTypeOptions = PrintOptions::printQualifiedPILType();
   OS << "pil_default_witness_table ";
   printLinkage(OS, getLinkage(), ForDefinition);
   OS << getInterface()->getName() << " {\n";

   PrintOptions options = PrintOptions::printPIL();
   options.GenericEnv = Interface->getGenericEnvironmentOfContext();

   for (auto &witness : getEntries()) {
      witness.print(OS, Verbose, options);
   }

   OS << "}\n\n";
}

void PILDefaultWitnessTable::dump() const {
   print(llvm::errs());
}

void PILCoverageMap::print(PILPrintContext &PrintCtx) const {
   llvm::raw_ostream &OS = PrintCtx.OS();
   OS << "pil_coverage_map " << QuotedString(getFile()) << " "
      << QuotedString(getName()) << " " << QuotedString(getPGOFuncName()) << " "
      << getHash() << " {\t// " << demangleSymbol(getName()) << "\n";
   if (PrintCtx.sortPIL())
      std::sort(MappedRegions.begin(), MappedRegions.end(),
                [](const MappedRegion &LHS, const MappedRegion &RHS) {
                   return std::tie(LHS.StartLine, LHS.StartCol, LHS.EndLine, LHS.EndCol) <
                          std::tie(RHS.StartLine, RHS.StartCol, RHS.EndLine, RHS.EndCol);
                });
   for (auto &MR : getMappedRegions()) {
      OS << "  " << MR.StartLine << ":" << MR.StartCol << " -> " << MR.EndLine
         << ":" << MR.EndCol << " : ";
      printCounter(OS, MR.Counter);
      OS << "\n";
   }
   OS << "}\n\n";
}

void PILCoverageMap::dump() const {
   print(llvm::errs());
}

#ifndef NDEBUG
// Disable the "for use only in debugger" warning.
#if SWIFT_COMPILER_IS_MSVC
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

void PILDebugScope::dump(SourceManager &SM, llvm::raw_ostream &OS,
                         unsigned Indent) const {
   OS << "{\n";
   OS.indent(Indent);
   if (Loc.isAstNode())
      Loc.getSourceLoc().print(OS, SM);
   OS << "\n";
   OS.indent(Indent + 2);
   OS << " parent: ";
   if (auto *P = Parent.dyn_cast<const PILDebugScope *>()) {
      P->dump(SM, OS, Indent + 2);
      OS.indent(Indent + 2);
   }
   else if (auto *F = Parent.dyn_cast<PILFunction *>())
      OS << "@" << F->getName();
   else
      OS << "nullptr";

   OS << "\n";
   OS.indent(Indent + 2);
   if (auto *CS = InlinedCallSite) {
      OS << "inlinedCallSite: ";
      CS->dump(SM, OS, Indent + 2);
      OS.indent(Indent + 2);
   }
   OS << "}\n";
}

void PILDebugScope::dump(PILModule &Mod) const {
   // We just use the default indent and llvm::errs().
   dump(Mod.getAstContext().SourceMgr);
}

#if SWIFT_COMPILER_IS_MSVC
#pragma warning(pop)
#endif

#endif

void PILSpecializeAttr::print(llvm::raw_ostream &OS) const {
   PILPrintContext Ctx(OS);
   // Print other types as their Swift representation.
   PrintOptions SubPrinter = PrintOptions::printPIL();
   auto exported = isExported() ? "true" : "false";
   auto kind = isPartialSpecialization() ? "partial" : "full";

   OS << "exported: " << exported << ", ";
   OS << "kind: " << kind << ", ";

   ArrayRef<Requirement> requirements;
   SmallVector<Requirement, 4> requirementsScratch;
   if (auto specializedSig = getSpecializedSignature()) {
      if (auto env = getFunction()->getGenericEnvironment()) {
         requirementsScratch = specializedSig->requirementsNotSatisfiedBy(
            env->getGenericSignature());
         requirements = requirementsScratch;
      } else {
         requirements = specializedSig->getRequirements();
      }
   }
   if (!requirements.empty()) {
      OS << "where ";
      PILFunction *F = getFunction();
      assert(F);
      auto GenericEnv = F->getGenericEnvironment();
      interleave(requirements,
                 [&](Requirement req) {
                    if (!GenericEnv) {
                       req.print(OS, SubPrinter);
                       return;
                    }
                    // Use GenericEnvironment to produce user-friendly
                    // names instead of something like t_0_0.
                    auto FirstTy = GenericEnv->getSugaredType(req.getFirstType());
                    if (req.getKind() != RequirementKind::Layout) {
                       auto SecondTy =
                          GenericEnv->getSugaredType(req.getSecondType());
                       Requirement ReqWithDecls(req.getKind(), FirstTy, SecondTy);
                       ReqWithDecls.print(OS, SubPrinter);
                    } else {
                       Requirement ReqWithDecls(req.getKind(), FirstTy,
                                                req.getLayoutConstraint());
                       ReqWithDecls.print(OS, SubPrinter);
                    }
                 },
                 [&] { OS << ", "; });
   }
}

//===----------------------------------------------------------------------===//
// PILPrintContext members
//===----------------------------------------------------------------------===//

PILPrintContext::PILPrintContext(llvm::raw_ostream &OS, bool Verbose,
                                 bool SortedPIL) :
   OutStream(OS), Verbose(Verbose), SortedPIL(SortedPIL),
   DebugInfo(PILPrintDebugInfo) { }

PILPrintContext::PILPrintContext(llvm::raw_ostream &OS, bool Verbose,
                                 bool SortedPIL, bool DebugInfo) :
   OutStream(OS), Verbose(Verbose), SortedPIL(SortedPIL),
   DebugInfo(DebugInfo) { }

void PILPrintContext::setContext(const void *FunctionOrBlock) {
   if (FunctionOrBlock != ContextFunctionOrBlock) {
      BlocksToIDMap.clear();
      ValueToIDMap.clear();
      ContextFunctionOrBlock = FunctionOrBlock;
   }
}

PILPrintContext::~PILPrintContext() {
}

void PILPrintContext::printInstructionCallBack(const PILInstruction *I) {
}

void PILPrintContext::initBlockIDs(ArrayRef<const PILBasicBlock *> Blocks) {
   if (Blocks.empty())
      return;

   setContext(Blocks[0]->getParent());

   // Initialize IDs so our IDs are in RPOT as well. This is a hack.
   for (unsigned Index : indices(Blocks))
      BlocksToIDMap[Blocks[Index]] = Index;
}

ID PILPrintContext::getID(const PILBasicBlock *Block) {
   setContext(Block->getParent());

   // Lazily initialize the Blocks-to-IDs mapping.
   // If we are asked to emit sorted PIL, print out our BBs in RPOT order.
   if (BlocksToIDMap.empty()) {
      if (sortPIL()) {
         std::vector<PILBasicBlock *> RPOT;
         auto *UnsafeF = const_cast<PILFunction *>(Block->getParent());
         std::copy(po_begin(UnsafeF), po_end(UnsafeF), std::back_inserter(RPOT));
         std::reverse(RPOT.begin(), RPOT.end());
         // Initialize IDs so our IDs are in RPOT as well. This is a hack.
         for (unsigned Index : indices(RPOT))
            BlocksToIDMap[RPOT[Index]] = Index;
      } else {
         unsigned idx = 0;
         for (const PILBasicBlock &B : *Block->getParent())
            BlocksToIDMap[&B] = idx++;
      }
   }
   ID R = {ID::PILBasicBlock, BlocksToIDMap[Block]};
   return R;
}

ID PILPrintContext::getID(const PILNode *node) {
   if (node == nullptr)
      return {ID::Null, ~0U};

   if (isa<PILUndef>(node))
      return {ID::PILUndef, 0};

   PILBasicBlock *BB = node->getParentBlock();
   if (PILFunction *F = BB->getParent()) {
      setContext(F);
      // Lazily initialize the instruction -> ID mapping.
      if (ValueToIDMap.empty())
         F->numberValues(ValueToIDMap);
      ID R = {ID::SSAValue, ValueToIDMap[node]};
      return R;
   }

   setContext(BB);

   // Check if we have initialized our ValueToIDMap yet. If we have, just use
   // that.
   if (!ValueToIDMap.empty()) {
      ID R = {ID::SSAValue, ValueToIDMap[node]};
      return R;
   }

   // Otherwise, initialize the instruction -> ID mapping cache.
   unsigned idx = 0;
   for (auto &I : *BB) {
      // Give the instruction itself the next ID.
      ValueToIDMap[&I] = idx;

      // If there are no results, make sure we don't reuse that ID.
      auto results = I.getResults();
      if (results.empty()) {
         idx++;
         continue;
      }

      // Otherwise, assign all of the results an index.  Note that
      // we'll assign the same ID to both the instruction and the
      // first result.
      for (auto result : results) {
         ValueToIDMap[result] = idx++;
      }
   }

   ID R = {ID::SSAValue, ValueToIDMap[node]};
   return R;
}
