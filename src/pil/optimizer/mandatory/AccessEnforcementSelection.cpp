//===--- AccessEnforcementSelection.cpp - Select access enforcement -------===//
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
/// This pass eliminates 'unknown' access enforcement by selecting either
/// static or dynamic enforcement.
///
/// TODO: This is currently a module transform so that it can process closures
/// after analyzing their parent scope. This isn't a big problem now because
/// AccessMarkerElimination is also a module pass that follows this pass, so all
/// markers will still be present when this pass runs. However, we would like to
/// mostly eliminate module transforms. This could be done by changing the
/// PassManager to follow ClosureScopeAnalysis. A new ClosureTransform type
/// would be pipelined just like FunctionTransform, but would have an entry
/// point that handled a parent closure scope and all its children in one
/// invocation. For function pipelining to be upheld, we would need to verify
/// that BasicCalleeAnalysis never conflicts with ClosureScopeAnalysis. i.e. we
/// could never create a caller->callee edge when the callee is passed as a
/// function argument. Normal FunctionTransforms would then be called on each
/// closure function and its parent scope before calling the ClosureTransform.
///
/// FIXME: handle boxes used by copy_value when neither copy is captured.
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "access-enforcement-selection"

#include "polarphp/pil/lang/ApplySite.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILUndef.h"
#include "polarphp/pil/lang/InstructionUtils.h"
#include "polarphp/pil/optimizer/analysis/ClosureScope.h"
#include "polarphp/pil/optimizer/analysis/PostOrderAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"

using namespace polar;

static void setStaticEnforcement(BeginAccessInst *access) {
   // TODO: delete if we're not using static enforcement?
   access->setEnforcement(PILAccessEnforcement::Static);

   LLVM_DEBUG(llvm::dbgs() << "Static Access: " << *access);
}

static void setDynamicEnforcement(BeginAccessInst *access) {
   // TODO: delete if we're not using dynamic enforcement?
   access->setEnforcement(PILAccessEnforcement::Dynamic);

   LLVM_DEBUG(llvm::dbgs() << "Dynamic Access: " << *access);
}

namespace {
// Information about an address-type closure capture.
// This is only valid for inout_aliasable parameters.
//
// TODO: Verify somewhere that we properly handle any non-inout_aliasable
// partial apply captures or that they never happen. Eventually @inout_aliasable
// should be simply replaced by @in or @out, once we don't have special aliasing
// rules.
struct AddressCapture {
   ApplySite site;
   unsigned calleeArgIdx;

   AddressCapture(Operand &oper)
      : site(oper.getUser()), calleeArgIdx(site.getCalleeArgIndex(oper)) {
      if (site.getOrigCalleeConv().getPILArgumentConvention(calleeArgIdx)
          != PILArgumentConvention::Indirect_InoutAliasable) {
         site = ApplySite();
         calleeArgIdx = ~0U;
         return;
      }
      assert(oper.get()->getType().isAddress());
   }

   bool isValid() const { return bool(site); }
};

LLVM_ATTRIBUTE_UNUSED
raw_ostream &operator<<(raw_ostream &os, const AddressCapture &capture) {
   os << *capture.site.getInstruction() << " captures Arg #"
      << capture.calleeArgIdx;
   auto *F = capture.site.getCalleeFunction();
   if (F)
      os << " of " << F->getName();
   os << '\n';
   return os;
}

// For each non-escaping closure, record the indices of arguments that
// require dynamic enforcement.
class DynamicCaptures {
   llvm::DenseMap<PILFunction *, SmallVector<unsigned, 4>> dynamicCaptureMap;

   DynamicCaptures(DynamicCaptures &) = delete;

public:
   DynamicCaptures() = default;

   void recordCapture(AddressCapture capture) {
      LLVM_DEBUG(llvm::dbgs() << "Dynamic Capture: " << capture);

      // *NOTE* For dynamically replaceable local functions, getCalleeFunction()
      // returns nullptr. This assert verifies the assumption that a captured
      // local variable can never be promoted to capture-by-address for
      // dynamically replaceable local functions.
      auto callee = capture.site.getCalleeFunction();
      assert(callee && "cannot locate function ref for nonescaping closure");

      auto &dynamicArgs = dynamicCaptureMap[callee];
      if (!llvm::is_contained(dynamicArgs, capture.calleeArgIdx))
         dynamicArgs.push_back(capture.calleeArgIdx);
   }

   bool isDynamic(PILFunctionArgument *arg) const {
      auto pos = dynamicCaptureMap.find(arg->getFunction());
      if (pos == dynamicCaptureMap.end())
         return false;

      auto &dynamicArgs = pos->second;
      return llvm::is_contained(dynamicArgs, arg->getIndex());
   }
};
} // anonymous namespace

namespace {
class SelectEnforcement {
   // Reference back to the known dynamically enforced non-escaping closure
   // arguments in this module. Parent scopes are processed before the closures
   // they reference.
   DynamicCaptures &dynamicCaptures;

   AllocBoxInst *Box;

   /// A state for tracking escape information about a variable.
   /// StateMap only has entries for blocks for which the variable
   /// has potentially escaped at exit.
   struct State {
      bool IsInWorklist = false;

      // At least one of the following must be true.
      bool HasEscape = false;
      bool HasPotentiallyEscapedAtEntry = false;

      // In a more advanced problem, this could easily be passed a State.
      bool adjustForEscapeInPredecessor() {
         bool updateSuccessors = false;

         if (!HasPotentiallyEscapedAtEntry) {
            HasPotentiallyEscapedAtEntry = true;
            updateSuccessors = !HasEscape;
         }

         return updateSuccessors;
      }
   };
   llvm::DenseMap<PILBasicBlock*, State> StateMap;

   /// All the accesses of Box in the function.
   SmallVector<BeginAccessInst*, 8> Accesses;

   /// All the non-escaping closure captures of the Boxed value in this function.
   SmallVector<AddressCapture, 8> Captures;

   /// All the escapes in the function.
   SmallPtrSet<PILInstruction*, 8> Escapes;

   /// A worklist we use for various purposes.
   SmallVector<PILBasicBlock*, 8> Worklist;

public:
   SelectEnforcement(DynamicCaptures &dc, AllocBoxInst *box)
      : dynamicCaptures(dc), Box(box) {}

   void run();

private:
   void analyzeUsesOfBox(SingleValueInstruction *source);
   void analyzeProjection(ProjectBoxInst *projection);

   /// Note that the given instruction is a use of the box (or a use of
   /// a projection from it) in which the address escapes.
   void noteEscapingUse(PILInstruction *inst);

   void propagateEscapes();
   void propagateEscapesFrom(PILBasicBlock *bb);

   bool hasPotentiallyEscapedAt(PILInstruction *inst);

   typedef llvm::SmallSetVector<PILBasicBlock*, 8> BlockSetVector;
   void findBlocksAccessedAcross(EndAccessInst *endAccess,
                                 BlockSetVector &blocksAccessedAcross);
   bool hasPotentiallyEscapedAtAnyReachableBlock(
      BeginAccessInst *access, BlockSetVector &blocksAccessedAcross);

   void updateAccesses();
   void updateAccess(BeginAccessInst *access);
   void updateCapture(AddressCapture capture);
};
} // end anonymous namespace

void SelectEnforcement::run() {
   LLVM_DEBUG(llvm::dbgs() << "  Box: " << *Box);

   // Set up the data-flow problem.
   analyzeUsesOfBox(Box);

   // Run the data-flow problem.
   propagateEscapes();

   // Update all the accesses.
   updateAccesses();
}

// FIXME: This should cover a superset of AllocBoxToStack's findUnexpectedBoxUse
// to avoid perturbing codegen. They should be sharing the same analysis.
void SelectEnforcement::analyzeUsesOfBox(SingleValueInstruction *source) {
   // Collect accesses rooted off of projections.
   for (auto use : source->getUses()) {
      auto user = use->getUser();

      if (auto MUI = dyn_cast<MarkUninitializedInst>(user)) {
         analyzeUsesOfBox(MUI);
         continue;
      }

      if (auto projection = dyn_cast<ProjectBoxInst>(user)) {
         analyzeProjection(projection);
         continue;
      }

      // Ignore certain other uses that do not capture the value.
      if (isa<StrongRetainInst>(user) ||
          isa<StrongReleaseInst>(user) ||
          isa<DestroyValueInst>(user) ||
          isa<DeallocBoxInst>(user))
         continue;

      // Treat everything else as an escape.
      // A Box typically escapes via copy_value.
      noteEscapingUse(user);
   }
   // Accesses may still be empty if the user of the Box is a partial apply
   // capture and, for some reason, the closure is dead.
}

// Verify that accesses are not nested before mandatory inlining.
// Closure captures should also not be nested within an access.
static void checkUsesOfAccess(BeginAccessInst *access) {
#ifndef NDEBUG
   // These conditions are only true prior to mandatory inlining.
   assert(!access->getFunction()->wasDeserializedCanonical());
   for (auto *use : access->getUses()) {
      auto user = use->getUser();
      assert(!isa<BeginAccessInst>(user));
      assert(!isa<PartialApplyInst>(user) ||
             onlyUsedByAssignByWrapper(cast<PartialApplyInst>(user)));
   }
#endif
}

void SelectEnforcement::analyzeProjection(ProjectBoxInst *projection) {
   for (auto *use : projection->getUses()) {
      auto user = use->getUser();

      // Collect accesses.
      if (auto *access = dyn_cast<BeginAccessInst>(user)) {
         if (access->getEnforcement() == PILAccessEnforcement::Unknown)
            Accesses.push_back(access);

         checkUsesOfAccess(access);

         continue;
      }
      // Handle both partial applies and directly applied non-escaping closures.
      if (ApplySite::isa(user)) {
         AddressCapture capture(*use);
         if (capture.isValid())
            Captures.emplace_back(capture);
         else
            // Only full apply sites can have non-inout_aliasable address arguments,
            // but those aren't actually captures.
            assert(FullApplySite::isa(user));
      }
   }
}

void SelectEnforcement::noteEscapingUse(PILInstruction *inst) {
   LLVM_DEBUG(llvm::dbgs() << "    Escape: " << *inst);

   // Add it to the escapes set.
   Escapes.insert(inst);

   // Record this point as escaping.
   auto userBB = inst->getParent();
   auto &state = StateMap[userBB];
   if (!state.IsInWorklist) {
      state.HasEscape = true;
      state.IsInWorklist = true;
      Worklist.push_back(userBB);
   }
   assert(state.HasEscape);
   assert(state.IsInWorklist);
}

void SelectEnforcement::propagateEscapes() {
   while (!Worklist.empty()) {
      auto bb = Worklist.pop_back_val();
      auto it = StateMap.find(bb);
      assert(it != StateMap.end() &&
             "block was in worklist but doesn't have a tracking state");
      auto &state = it->second;
      assert(state.HasEscape || state.HasPotentiallyEscapedAtEntry);
      state.IsInWorklist = false;
      propagateEscapesFrom(bb);
   }
}

/// Given that the box potentially escaped before we exited the
/// given block, propagate that information to all of its successors.
void SelectEnforcement::propagateEscapesFrom(PILBasicBlock *bb) {
   assert(StateMap.count(bb));

   // Iterate over the successors of the block.
   for (PILBasicBlock *succ : bb->getSuccessors()) {
      auto &succState = StateMap[succ];

      // If updating the successor changes it in a way that will
      // require us to update its successors, add it to the worklist.
      if (succState.adjustForEscapeInPredecessor()) {
         if (!succState.IsInWorklist) {
            succState.IsInWorklist = true;
            Worklist.push_back(succ);
         }
      }
   }
}

bool SelectEnforcement::hasPotentiallyEscapedAt(PILInstruction *point) {
   auto bb = point->getParent();

   // If we're not tracking anything for the whole block containing
   // the instruction, we're done; it hasn't escaped here.
   auto it = StateMap.find(bb);
   if (it == StateMap.end())
      return false;

   // If the tracking information says there are escapes before entry,
   // we're done; it has potentially escaped.
   const auto &state = it->second;
   if (state.HasPotentiallyEscapedAtEntry)
      return true;

   // Okay, there must be an escape within this block.
   assert(state.HasEscape);
   for (auto ii = point->getIterator(), ie = bb->begin(); ii != ie; ) {
      auto inst = &*--ii;

      // Maybe just record the first escape in the block and see if we
      // come after it?
      if (Escapes.count(inst))
         return true;
   }

   return false;
}

/// Add all blocks to `Worklist` between the given `endAccess` and its
/// `begin_access` in which the access is active at the end of the block.
void SelectEnforcement::findBlocksAccessedAcross(
   EndAccessInst *endAccess, BlockSetVector &blocksAccessedAcross) {

   // Fast path: we're not tracking any escapes.  (But the box should
   // probably have been promoted to the stack in this case.)
   if (StateMap.empty())
      return;

   PILBasicBlock *beginBB = endAccess->getBeginAccess()->getParent();
   if (endAccess->getParent() == beginBB)
      return;

   assert(Worklist.empty());
   Worklist.push_back(endAccess->getParent());
   while (!Worklist.empty()) {
      PILBasicBlock *bb = Worklist.pop_back_val();
      for (auto *predBB : bb->getPredecessorBlocks()) {
         if (!blocksAccessedAcross.insert(predBB)) continue;
         if (predBB == beginBB) continue;
         Worklist.push_back(predBB);
      }
   }
}

bool SelectEnforcement::hasPotentiallyEscapedAtAnyReachableBlock(
   BeginAccessInst *access, BlockSetVector &blocksAccessedAcross) {

   assert(Worklist.empty());
   SmallPtrSet<PILBasicBlock*, 8> visited;

   // Don't follow any paths that lead to an end_access.
   for (auto endAccess : access->getEndAccesses())
      visited.insert(endAccess->getParent());

   /// Initialize the worklist with all blocks that exit the access path.
   for (PILBasicBlock *bb : blocksAccessedAcross) {
      for (PILBasicBlock *succBB : bb->getSuccessorBlocks()) {
         if (blocksAccessedAcross.count(succBB)) continue;
         if (visited.insert(succBB).second)
            Worklist.push_back(succBB);
      }
   }

   while (!Worklist.empty()) {
      PILBasicBlock *bb = Worklist.pop_back_val();
      assert(visited.count(bb));

      // If we're tracking information for this block, there's an escape.
      if (StateMap.count(bb))
         return true;

      // Add all reachable successors.
      for (PILBasicBlock *succ : bb->getSuccessors()) {
         if (visited.insert(succ).second)
            Worklist.push_back(succ);
      }
   }

   // No reachable block has an escape.
   return false;
}

void SelectEnforcement::updateAccesses() {
   for (auto *access : Accesses) {
      LLVM_DEBUG(llvm::dbgs() << "    Access: " << *access);
      updateAccess(access);
   }
   for (AddressCapture &capture : Captures) {
      LLVM_DEBUG(llvm::dbgs() << "    Capture: " << capture);
      updateCapture(capture);
   }
}

void SelectEnforcement::updateAccess(BeginAccessInst *access) {
   assert(access->getEnforcement() == PILAccessEnforcement::Unknown);

   // Check whether the variable escaped before any of the end_accesses.
   BlockSetVector blocksAccessedAcross;
   for (auto endAccess : access->getEndAccesses()) {
      if (hasPotentiallyEscapedAt(endAccess))
         return setDynamicEnforcement(access);

      // Add all blocks to blocksAccessedAcross between begin_access and this
      // end_access.
      findBlocksAccessedAcross(endAccess, blocksAccessedAcross);
   }
   assert(blocksAccessedAcross.empty()
          || blocksAccessedAcross.count(access->getParent()));

   // For every path through this access that doesn't reach an end_access, check
   // if any block reachable from that path can see an escaped value.
   if (hasPotentiallyEscapedAtAnyReachableBlock(access, blocksAccessedAcross)) {
      setDynamicEnforcement(access);
      return;
   }
   // Otherwise, use static enforcement.
   setStaticEnforcement(access);
}

void SelectEnforcement::updateCapture(AddressCapture capture) {
   auto captureIfEscaped = [&](PILInstruction *user) {
      if (hasPotentiallyEscapedAt(user))
         dynamicCaptures.recordCapture(capture);
   };
   SingleValueInstruction *PAIUser = dyn_cast<PartialApplyInst>(capture.site);
   if (!PAIUser) {
      // This is a full apply site. Immediately record the capture and return.
      captureIfEscaped(capture.site.getInstruction());
      return;
   }
   // For partial applies, check all use points of the closure.
   llvm::SmallSetVector<SingleValueInstruction *, 8> worklist;
   auto visitUse = [&](Operand *oper) {
      auto *user = oper->getUser();
      if (FullApplySite::isa(user)) {
         // A call is considered a closure access regardless of whether it calls
         // the closure or accepts the closure as an argument.
         captureIfEscaped(user);
         return;
      }
      switch (user->getKind()) {
         case PILInstructionKind::ConvertEscapeToNoEscapeInst:
         case PILInstructionKind::MarkDependenceInst:
         case PILInstructionKind::ConvertFunctionInst:
         case PILInstructionKind::BeginBorrowInst:
         case PILInstructionKind::CopyValueInst:
         case PILInstructionKind::EnumInst:
         case PILInstructionKind::StructInst:
         case PILInstructionKind::TupleInst:
         case PILInstructionKind::PartialApplyInst:
            // Propagate the closure.
            worklist.insert(cast<SingleValueInstruction>(user));
            return;
         case PILInstructionKind::StrongRetainInst:
         case PILInstructionKind::StrongReleaseInst:
         case PILInstructionKind::DebugValueInst:
         case PILInstructionKind::DestroyValueInst:
         case PILInstructionKind::RetainValueInst:
         case PILInstructionKind::ReleaseValueInst:
         case PILInstructionKind::EndBorrowInst:
            // partial_apply [stack] is matched with dealloc_stack.
         case PILInstructionKind::DeallocStackInst:
            // Benign use.
            return;
         case PILInstructionKind::TupleExtractInst:
         case PILInstructionKind::StructExtractInst:
         case PILInstructionKind::AssignInst:
         case PILInstructionKind::BranchInst:
         case PILInstructionKind::CondBranchInst:
         case PILInstructionKind::ReturnInst:
         case PILInstructionKind::StoreInst:
            // These are all valid partial_apply users, however we don't expect them
            // to occur with non-escaping closures. Handle them conservatively just in
            // case they occur.
            LLVM_FALLTHROUGH;
         default:
            LLVM_DEBUG(llvm::dbgs() << "    Unrecognized partial_apply user: "
                                    << *user);

            // Handle unknown uses conservatively by assuming a capture.
            captureIfEscaped(user);
      }
   };
   while (true) {
      for (auto *oper : PAIUser->getUses())
         visitUse(oper);
      if (worklist.empty())
         break;
      PAIUser = worklist.pop_back_val();
   }
}

namespace {

// Model the kind of access needed based on analyzing the access's source.
// This is either determined to be static or dynamic, or requires further
// analysis of a boxed variable.
struct SourceAccess {
   enum { StaticAccess, DynamicAccess, BoxAccess } kind;
   AllocBoxInst *allocBox;

   static SourceAccess getStaticAccess() { return {StaticAccess, nullptr}; }
   static SourceAccess getDynamicAccess() { return {DynamicAccess, nullptr}; }

   static SourceAccess getBoxedAccess(AllocBoxInst *inst) {
      return {BoxAccess, inst};
   }
};

/// The pass.
///
/// This can't be a PILFunctionTransform because DynamicCaptures need to be
/// recorded while analyzing a closure's parent scopes before processing the
/// closures.
///
/// TODO: Make this a "ClosureTransform". See the file-level comments above.
class AccessEnforcementSelection : public PILModuleTransform {
   // Reference back to the known dynamically enforced non-escaping closure
   // arguments in this module. Parent scopes are processed before the closures
   // they reference.
   DynamicCaptures dynamicCaptures;

#ifndef NDEBUG
   // Per-function book-keeping to verify that a box is processed before all of
   // its accesses and captures are seen.
   llvm::DenseSet<AllocBoxInst *> handledBoxes;
   llvm::DenseSet<PILFunction *> visited;
#endif

public:
   void run() override;

protected:
   void processFunction(PILFunction *F);
   SourceAccess getAccessKindForBox(ProjectBoxInst *projection);
   SourceAccess getSourceAccess(PILValue address);
   void handleApply(ApplySite apply);
   void handleAccess(BeginAccessInst *access);
};

void AccessEnforcementSelection::run() {
   auto *CSA = getAnalysis<ClosureScopeAnalysis>();
   TopDownClosureFunctionOrder closureOrder(CSA);
   closureOrder.visitFunctions(
      [this](PILFunction *F) { this->processFunction(F); });
}

void AccessEnforcementSelection::processFunction(PILFunction *F) {
   if (F->isExternalDeclaration())
      return;

   LLVM_DEBUG(llvm::dbgs() << "Access Enforcement Selection in " << F->getName()
                           << "\n");

   // This ModuleTransform needs to analyze closures and their parent scopes in
   // the same pass, and the parent needs to be analyzed before the closure.
#ifndef NDEBUG
   auto *CSA = getAnalysis<ClosureScopeAnalysis>();
   if (isNonEscapingClosure(F->getLoweredFunctionType())) {
      for (auto *scopeF : CSA->getClosureScopes(F)) {
         LLVM_DEBUG(llvm::dbgs() << "  Parent scope: " << scopeF->getName()
                                 << "\n");
         assert(visited.count(scopeF));
         // Closures must be defined in the same module as their parent scope.
         assert(scopeF->wasDeserializedCanonical()
                == F->wasDeserializedCanonical());
      }
   }
   visited.insert(F);
#endif

   // Deserialized functions, which have been mandatory inlined, no longer meet
   // the structural requirements on access markers required by this pass.
   if (F->wasDeserializedCanonical())
      return;

   // Perform an RPO walk so that boxes are always processed before their access.
   auto *PO = getAnalysis<PostOrderAnalysis>()->get(F);
   for (PILBasicBlock *bb : PO->getReversePostOrder()) {
      for (auto ii = bb->begin(), ie = bb->end(); ii != ie;) {
         PILInstruction *inst = &*ii;
         ++ii;

         // Analyze all boxes. Even if they aren't accessed in this function, they
         // may still have captures that require dynamic enforcement because the
         // box has escaped prior to the capture.
         if (auto box = dyn_cast<AllocBoxInst>(inst)) {
            SelectEnforcement(dynamicCaptures, box).run();
            assert(handledBoxes.insert(box).second);

         } else if (auto access = dyn_cast<BeginAccessInst>(inst))
            handleAccess(access);

         else if (auto access = dyn_cast<BeginUnpairedAccessInst>(inst))
            assert(access->getEnforcement() == PILAccessEnforcement::Dynamic);

            // Check for unboxed captures in both partial_applies and direct
            // applications of non-escaping closures.
         else if (auto apply = ApplySite::isa(inst))
            handleApply(apply);
      }
   }
   invalidateAnalysis(F, PILAnalysis::InvalidationKind::Instructions);
#ifndef NDEBUG
   // There's no need to track handled boxes across functions.
   handledBoxes.clear();
#endif
}

SourceAccess
AccessEnforcementSelection::getAccessKindForBox(ProjectBoxInst *projection) {
   PILValue source = projection->getOperand();
   if (auto *MUI = dyn_cast<MarkUninitializedInst>(source))
      source = MUI->getOperand();

   // If we didn't allocate the box, assume that we need to use
   // dynamic enforcement.
   // TODO: use static enforcement in certain provable cases.
   auto box = dyn_cast<AllocBoxInst>(source);
   if (!box)
      return SourceAccess::getDynamicAccess();

   return SourceAccess::getBoxedAccess(box);
}

SourceAccess AccessEnforcementSelection::getSourceAccess(PILValue address) {
   // Recurse through MarkUninitializedInst.
   if (auto *MUI = dyn_cast<MarkUninitializedInst>(address))
      return getSourceAccess(MUI->getOperand());

   if (auto box = dyn_cast<ProjectBoxInst>(address))
      return getAccessKindForBox(box);

   if (auto arg = dyn_cast<PILFunctionArgument>(address)) {
      switch (arg->getArgumentConvention()) {
         case PILArgumentConvention::Indirect_Inout:
            // `inout` arguments are checked on the caller side, either statically
            // or dynamically if necessary. The @inout does not alias and cannot
            // escape within the callee, so static enforcement is always sufficient.
            return SourceAccess::getStaticAccess();

         case PILArgumentConvention::Indirect_InoutAliasable:
            if (dynamicCaptures.isDynamic(arg))
               return SourceAccess::getDynamicAccess();

            return SourceAccess::getStaticAccess();
         case PILArgumentConvention::Indirect_In:
         case PILArgumentConvention::Indirect_In_Guaranteed:
            // @in/@in_guaranteed cannot be mutably accessed, mutably captured, or
            // passed as inout. @in/@in_guaranteed may be captured @inout_aliasable.
            // (This is fairly horrible, but presumably Sema/PILGen made sure a copy
            // wasn't needed?)
            //
            // FIXME: When we have borrowed arguments, a "read" needs to be enforced
            // on the caller side.
            return SourceAccess::getStaticAccess();

         case PILArgumentConvention::Indirect_Out:
            // We use an initialized 'out' argument as a parameter.
            return SourceAccess::getStaticAccess();

         default:
            llvm_unreachable("Expecting an inout argument.");
      }
   }
   // If we're not accessing a box or argument, we must've lowered to a stack
   // element. Other sources of access are either outright dynamic (GlobalAddr,
   // RefElementAddr), or only exposed after mandatory inlining (nested
   // dependent BeginAccess).
   //
   // Running before diagnostic constant propagation requires handling 'undef'.
   assert(isa<AllocStackInst>(address) || isa<PILUndef>(address));
   return SourceAccess::getStaticAccess();
}

void AccessEnforcementSelection::handleApply(ApplySite apply) {
   auto calleeTy = apply.getOrigCalleeType();
   PILFunctionConventions calleeConv(calleeTy, *getModule());

   for (Operand &oper : apply.getArgumentOperands()) {
      AddressCapture capture(oper);
      if (!capture.isValid())
         continue;

      // This is a non-escaping closure argument. If the argument requires dynamic
      // access, record that in dynamicCaptures.
      auto sourceAccess = getSourceAccess(oper.get());
      switch (sourceAccess.kind) {
         case SourceAccess::StaticAccess:
            // If the captured variable does not require dynamic enforcement, then
            // there's no need to track it.
            break;
         case SourceAccess::DynamicAccess: {
            dynamicCaptures.recordCapture(capture);
            break;
         }
         case SourceAccess::BoxAccess:
            // Captures of box projections are handled during SelectEnforcement, which
            // determines the access enforcement for all users of a box. Within
            // SelectEnforcement, we know whether the box has escaped before the
            // capture. Here there's just nothing to do.
            assert(handledBoxes.count(sourceAccess.allocBox));
            break;
      }
   }
}

void AccessEnforcementSelection::handleAccess(BeginAccessInst *access) {
   if (access->getEnforcement() != PILAccessEnforcement::Unknown)
      return;

   auto sourceAccess = getSourceAccess(access->getOperand());
   switch (sourceAccess.kind) {
      case SourceAccess::StaticAccess:
         setStaticEnforcement(access);
         break;
      case SourceAccess::DynamicAccess:
         setDynamicEnforcement(access);
         break;
      case SourceAccess::BoxAccess:
         llvm_unreachable("All boxes must have already been selected.");
   }
}

} // end anonymous namespace

PILTransform *polar::createAccessEnforcementSelection() {
   return new AccessEnforcementSelection();
}