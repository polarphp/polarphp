//===--- PILInliner.cpp - Inlines PIL functions ---------------------------===//
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

#define DEBUG_TYPE "pil-inliner"

#include "polarphp/pil/optimizer/utils/PILInliner.h"
#include "polarphp/pil/lang/PrettyStackTrace.h"
#include "polarphp/pil/lang/PILDebugScope.h"
#include "polarphp/pil/lang/TypeSubstCloner.h"
#include "polarphp/pil/optimizer/utils/CFGOptUtils.h"
#include "polarphp/pil/optimizer/utils/PILOptFunctionBuilder.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Debug.h"

using namespace polar;

static bool canInlineBeginApply(BeginApplyInst *BA) {
   // Don't inline if we have multiple resumption sites (i.e. end_apply or
   // abort_apply instructions).  The current implementation clones a single
   // copy of the end_apply and abort_apply paths, so it can't handle values
   // that might be live in the caller across different resumption sites.  To
   // handle this in general, we'd need to separately clone the resume/unwind
   // paths into each end/abort.
   bool hasEndApply = false, hasAbortApply = false;
   for (auto tokenUse : BA->getTokenResult()->getUses()) {
      auto user = tokenUse->getUser();
      if (isa<EndApplyInst>(user)) {
         if (hasEndApply) return false;
         hasEndApply = true;
      } else {
         assert(isa<AbortApplyInst>(user));
         if (hasAbortApply) return false;
         hasAbortApply = true;
      }
   }

   // Don't inline a coroutine with multiple yields.  The current
   // implementation doesn't clone code from the caller, so it can't handle
   // values that might be live in the callee across different yields.
   // To handle this in general, we'd need to clone code in the caller,
   // both between the begin_apply and the resumption site and then
   // potentially after the resumption site when there are un-mergeable
   // values alive across it.
   bool hasYield = false;
   for (auto &B : BA->getReferencedFunctionOrNull()->getBlocks()) {
      if (isa<YieldInst>(B.getTerminator())) {
         if (hasYield) return false;
         hasYield = true;
      }
   }
   // Note that zero yields is fine; it just means the begin_apply is
   // basically noreturn.

   return true;
}

bool PILInliner::canInlineApplySite(FullApplySite apply) {
   if (!apply.canOptimize())
      return false;

   if (auto BA = dyn_cast<BeginApplyInst>(apply))
      return canInlineBeginApply(BA);

   return true;
}

namespace {

/// Utility class for rewiring control-flow of inlined begin_apply functions.
class BeginApplySite {
   PILLocation Loc;
   PILBuilder *Builder;
   BeginApplyInst *BeginApply;
   bool HasYield = false;

   EndApplyInst *EndApply = nullptr;
   PILBasicBlock *EndApplyBB = nullptr;
   PILBasicBlock *EndApplyReturnBB = nullptr;

   AbortApplyInst *AbortApply = nullptr;
   PILBasicBlock *AbortApplyBB = nullptr;
   PILBasicBlock *AbortApplyReturnBB = nullptr;

public:
   BeginApplySite(BeginApplyInst *BeginApply, PILLocation Loc,
                  PILBuilder *Builder)
      : Loc(Loc), Builder(Builder), BeginApply(BeginApply) {}

   static Optional<BeginApplySite> get(FullApplySite AI, PILLocation Loc,
                                       PILBuilder *Builder) {
      auto *BeginApply = dyn_cast<BeginApplyInst>(AI);
      if (!BeginApply)
         return None;
      return BeginApplySite(BeginApply, Loc, Builder);
   }

   void preprocess(PILBasicBlock *returnToBB,
                   SmallVectorImpl<PILInstruction *> &endBorrowInsertPts) {
      SmallVector<EndApplyInst *, 1> endApplyInsts;
      SmallVector<AbortApplyInst *, 1> abortApplyInsts;
      BeginApply->getCoroutineEndPoints(endApplyInsts, abortApplyInsts);
      while (!endApplyInsts.empty()) {
         auto *endApply = endApplyInsts.pop_back_val();
         collectEndApply(endApply);
         endBorrowInsertPts.push_back(&*std::next(endApply->getIterator()));
      }
      while (!abortApplyInsts.empty()) {
         auto *abortApply = abortApplyInsts.pop_back_val();
         collectAbortApply(abortApply);
         endBorrowInsertPts.push_back(&*std::next(abortApply->getIterator()));
      }
   }

   // Split the basic block before the end/abort_apply. We will insert code
   // to jump to the resume/unwind blocks depending on the integer token
   // later. And the inlined resume/unwind return blocks will jump back to
   // the merge blocks.
   void collectEndApply(EndApplyInst *End) {
      assert(!EndApply);
      EndApply = End;
      EndApplyBB = EndApply->getParent();
      EndApplyReturnBB = EndApplyBB->split(PILBasicBlock::iterator(EndApply));
   }
   void collectAbortApply(AbortApplyInst *Abort) {
      assert(!AbortApply);
      AbortApply = Abort;
      AbortApplyBB = AbortApply->getParent();
      AbortApplyReturnBB = AbortApplyBB->split(PILBasicBlock::iterator(Abort));
   }

   /// Perform special processing for the given terminator if necessary.
   ///
   /// \return false to use the normal inlining logic
   bool processTerminator(
      TermInst *terminator, PILBasicBlock *returnToBB,
      llvm::function_ref<PILBasicBlock *(PILBasicBlock *)> remapBlock,
      llvm::function_ref<PILValue(PILValue)> getMappedValue) {
      // A yield branches to the begin_apply return block passing the yielded
      // results as branch arguments. Collect the yields target block for
      // resuming later. Pass an integer token to the begin_apply return block
      // to mark the yield we came from.
      if (auto *yield = dyn_cast<YieldInst>(terminator)) {
         assert(!HasYield);
         HasYield = true;

         // Pairwise replace the yielded values of the BeginApply with the
         // values that were yielded.
         auto calleeYields = yield->getYieldedValues();
         auto callerYields = BeginApply->getYieldedValues();
         assert(calleeYields.size() == callerYields.size());
         for (auto i : indices(calleeYields)) {
            auto remappedYield = getMappedValue(calleeYields[i]);
            callerYields[i]->replaceAllUsesWith(remappedYield);
         }
         Builder->createBranch(Loc, returnToBB);

         // Add branches at the resumption sites to the resume/unwind block.
         if (EndApply) {
            SavedInsertionPointRAII savedIP(*Builder, EndApplyBB);
            auto resumeBB = remapBlock(yield->getResumeBB());
            Builder->createBranch(EndApply->getLoc(), resumeBB);
         }
         if (AbortApply) {
            SavedInsertionPointRAII savedIP(*Builder, AbortApplyBB);
            auto unwindBB = remapBlock(yield->getUnwindBB());
            Builder->createBranch(AbortApply->getLoc(), unwindBB);
         }
         return true;
      }

      // 'return' and 'unwind' instructions turn into branches to the
      // end_apply/abort_apply return blocks, respectively.  If those blocks
      // are null, it's because there weren't any of the corresponding
      // instructions in the caller.  That means this entire path is
      // unreachable.
      if (isa<ReturnInst>(terminator) || isa<UnwindInst>(terminator)) {
         bool isNormal = isa<ReturnInst>(terminator);
         auto returnBB = isNormal ? EndApplyReturnBB : AbortApplyReturnBB;
         if (returnBB) {
            Builder->createBranch(Loc, returnBB);
         } else {
            Builder->createUnreachable(Loc);
         }
         return true;
      }

      assert(!isa<ThrowInst>(terminator) &&
             "Unexpected throw instruction in yield_once function");

      // Otherwise, we just map the instruction normally.
      return false;
   }

   /// Complete the begin_apply-specific inlining work. Delete vestiges of the
   /// apply site except the callee value. Return a valid iterator after the
   /// original begin_apply.
   void complete() {
      // If there was no yield in the coroutine, then control never reaches
      // the end of the begin_apply, so all the downstream code is unreachable.
      // Make sure the function is well-formed, since we otherwise rely on
      // having visited a yield instruction.
      if (!HasYield) {
         // Make sure the split resumption blocks have terminators.
         if (EndApplyBB) {
            SavedInsertionPointRAII savedIP(*Builder, EndApplyBB);
            Builder->createUnreachable(Loc);
         }
         if (AbortApplyBB) {
            SavedInsertionPointRAII savedIP(*Builder, AbortApplyBB);
            Builder->createUnreachable(Loc);
         }

         // Replace all the yielded values in the callee with undef.
         for (auto calleeYield : BeginApply->getYieldedValues()) {
            calleeYield->replaceAllUsesWith(
               PILUndef::get(calleeYield->getType(), Builder->getFunction()));
         }
      }

      // Remove the resumption sites.
      if (EndApply)
         EndApply->eraseFromParent();
      if (AbortApply)
         AbortApply->eraseFromParent();

      assert(!BeginApply->hasUsesOfAnyResult());
   }
};

} // end anonymous namespace

namespace polar {
class PILInlineCloner
   : public TypeSubstCloner<PILInlineCloner, PILOptFunctionBuilder> {
   friend class PILInstructionVisitor<PILInlineCloner>;
   friend class PILCloner<PILInlineCloner>;
   using SuperTy = TypeSubstCloner<PILInlineCloner, PILOptFunctionBuilder>;
   using InlineKind = PILInliner::InlineKind;

   PILOptFunctionBuilder &FuncBuilder;
   InlineKind IKind;

   // The original, noninlined apply site. These become invalid after fixUp,
   // which is called as the last step in PILCloner::cloneFunctionBody.
   FullApplySite Apply;
   Optional<BeginApplySite> BeginApply;

   PILInliner::DeletionFuncTy DeletionCallback;

   /// The location representing the inlined instructions.
   ///
   /// This location wraps the call site AST node that is being inlined.
   /// Alternatively, it can be the PIL file location of the call site (in case
   /// of PIL-to-PIL transformations).
   Optional<PILLocation> Loc;
   const PILDebugScope *CallSiteScope = nullptr;
   llvm::SmallDenseMap<const PILDebugScope *, const PILDebugScope *, 8>
      InlinedScopeCache;

   // Block in the original caller serving as the successor of the inlined
   // control path.
   PILBasicBlock *ReturnToBB = nullptr;

   // Keep track of the next instruction after inlining the call.
   PILBasicBlock::iterator NextIter;

public:
   PILInlineCloner(PILFunction *CalleeFunction, FullApplySite Apply,
                   PILOptFunctionBuilder &FuncBuilder, InlineKind IKind,
                   SubstitutionMap ApplySubs,
                   PILOpenedArchetypesTracker &OpenedArchetypesTracker,
                   PILInliner::DeletionFuncTy deletionCallback);

   PILFunction *getCalleeFunction() const { return &Original; }

   PILBasicBlock::iterator cloneInline(ArrayRef<PILValue> AppliedArgs);

protected:
   PILValue borrowFunctionArgument(PILValue callArg, FullApplySite AI);

   void visitDebugValueInst(DebugValueInst *Inst);
   void visitDebugValueAddrInst(DebugValueAddrInst *Inst);

   void visitTerminator(PILBasicBlock *BB);

   /// This hook is called after either of the top-level visitors:
   /// cloneReachableBlocks or clonePILFunction.
   ///
   /// After fixUp, the PIL must be valid and semantically equivalent to the PIL
   /// before cloning.
   void fixUp(PILFunction *calleeFunction);

   const PILDebugScope *getOrCreateInlineScope(const PILDebugScope *DS);

   void postProcess(PILInstruction *Orig, PILInstruction *Cloned) {
      // We just updated the debug scope information. Intentionally
      // don't call PILClonerWithScopes<PILInlineCloner>::postProcess().
      PILCloner<PILInlineCloner>::postProcess(Orig, Cloned);
   }

   PILLocation remapLocation(PILLocation InLoc) {
      // For performance inlining return the original location.
      if (IKind == InlineKind::PerformanceInline)
         return InLoc;
      // Inlined location wraps the call site that is being inlined, regardless
      // of the input location.
      return Loc.hasValue()
             ? Loc.getValue()
             : MandatoryInlinedLocation::getMandatoryInlinedLocation(
            (Decl *)nullptr);
   }

   const PILDebugScope *remapScope(const PILDebugScope *DS) {
      if (IKind == InlineKind::MandatoryInline)
         // Transparent functions are absorbed into the call
         // site. No soup, err, debugging for you!
         return CallSiteScope;
      else
         // Create an inlined version of the scope.
         return getOrCreateInlineScope(DS);
   }
};
} // namespace polar

std::pair<PILBasicBlock::iterator, PILBasicBlock *>
PILInliner::inlineFunction(PILFunction *calleeFunction, FullApplySite apply,
                           ArrayRef<PILValue> appliedArgs) {
   PrettyStackTracePILFunction calleeTraceRAII("inlining", calleeFunction);
   PrettyStackTracePILFunction callerTraceRAII("...into", apply.getFunction());
   assert(canInlineApplySite(apply)
          && "Asked to inline function that is unable to be inlined?!");

   PILInlineCloner cloner(calleeFunction, apply, FuncBuilder, IKind, ApplySubs,
                          OpenedArchetypesTracker, DeletionCallback);
   auto nextI = cloner.cloneInline(appliedArgs);
   return std::make_pair(nextI, cloner.getLastClonedBB());
}

std::pair<PILBasicBlock::iterator, PILBasicBlock *>
PILInliner::inlineFullApply(FullApplySite apply,
                            PILInliner::InlineKind inlineKind,
                            PILOptFunctionBuilder &funcBuilder) {
   assert(apply.canOptimize());
   SmallVector<PILValue, 8> appliedArgs;
   for (const auto &arg : apply.getArguments())
      appliedArgs.push_back(arg);

   PILFunction *caller = apply.getFunction();
   PILOpenedArchetypesTracker OpenedArchetypesTracker(caller);
   caller->getModule().registerDeleteNotificationHandler(
      &OpenedArchetypesTracker);
   // The callee only needs to know about opened archetypes used in
   // the substitution list.
   OpenedArchetypesTracker.registerUsedOpenedArchetypes(apply.getInstruction());

   PILInliner Inliner(funcBuilder, inlineKind, apply.getSubstitutionMap(),
                      OpenedArchetypesTracker);
   return Inliner.inlineFunction(apply.getReferencedFunctionOrNull(), apply,
                                 appliedArgs);
}

PILInlineCloner::PILInlineCloner(
   PILFunction *calleeFunction, FullApplySite apply,
   PILOptFunctionBuilder &funcBuilder, InlineKind inlineKind,
   SubstitutionMap applySubs,
   PILOpenedArchetypesTracker &openedArchetypesTracker,
   PILInliner::DeletionFuncTy deletionCallback)
   : SuperTy(*apply.getFunction(), *calleeFunction, applySubs,
             openedArchetypesTracker, /*Inlining=*/true),
     FuncBuilder(funcBuilder), IKind(inlineKind), Apply(apply),
     DeletionCallback(deletionCallback) {

   PILFunction &F = getBuilder().getFunction();
   assert(apply.getFunction() && apply.getFunction() == &F
          && "Inliner called on apply instruction in wrong function?");
   assert(((calleeFunction->getRepresentation()
            != PILFunctionTypeRepresentation::ObjCMethod
            && calleeFunction->getRepresentation()
               != PILFunctionTypeRepresentation::CFunctionPointer)
           || IKind == InlineKind::PerformanceInline)
          && "Cannot inline Objective-C methods or C functions in mandatory "
             "inlining");

   // Compute the PILLocation which should be used by all the inlined
   // instructions.
   if (IKind == InlineKind::PerformanceInline)
      Loc = InlinedLocation::getInlinedLocation(apply.getLoc());
   else {
      assert(IKind == InlineKind::MandatoryInline && "Unknown InlineKind.");
      Loc = MandatoryInlinedLocation::getMandatoryInlinedLocation(apply.getLoc());
   }

   auto applyScope = apply.getDebugScope();
   // FIXME: Turn this into an assertion instead.
   if (!applyScope)
      applyScope = apply.getFunction()->getDebugScope();

   if (IKind == InlineKind::MandatoryInline) {
      // Mandatory inlining: every instruction inherits scope/location
      // from the call site.
      CallSiteScope = applyScope;
   } else {
      // Performance inlining. Construct a proper inline scope pointing
      // back to the call site.
      CallSiteScope = new (F.getModule()) PILDebugScope(
         apply.getLoc(), nullptr, applyScope, applyScope->InlinedCallSite);
   }
   assert(CallSiteScope && "call site has no scope");
   assert(CallSiteScope->getParentFunction() == &F);

   // Set up the coroutine-specific inliner if applicable.
   BeginApply = BeginApplySite::get(apply, Loc.getValue(), &getBuilder());
}

// Clone the entire callee function into the caller function at the apply site.
// Delete the original apply and all dead arguments except the callee. Return an
// iterator the the first instruction after the original apply.
PILBasicBlock::iterator
PILInlineCloner::cloneInline(ArrayRef<PILValue> AppliedArgs) {
   assert(getCalleeFunction()->getArguments().size() == AppliedArgs.size()
          && "Unexpected number of callee arguments.");

   getBuilder().setInsertionPoint(Apply.getInstruction());

   SmallVector<PILValue, 4> entryArgs;
   entryArgs.reserve(AppliedArgs.size());
   SmallBitVector borrowedArgs(AppliedArgs.size());

   auto calleeConv = getCalleeFunction()->getConventions();
   for (auto p : llvm::enumerate(AppliedArgs)) {
      PILValue callArg = p.value();
      unsigned idx = p.index();
      // Insert begin/end borrow for guaranteed arguments.
      if (idx >= calleeConv.getPILArgIndexOfFirstParam() &&
          calleeConv.getParamInfoForPILArg(idx).isGuaranteed()) {
         if (PILValue newValue = borrowFunctionArgument(callArg, Apply)) {
            callArg = newValue;
            borrowedArgs[idx] = true;
         }
      }
      entryArgs.push_back(callArg);
   }

   // Create the return block and set ReturnToBB for use in visitTerminator
   // callbacks.
   PILBasicBlock *callerBlock = Apply.getParent();
   PILBasicBlock *throwBlock = nullptr;
   SmallVector<PILInstruction *, 1> endBorrowInsertPts;

   switch (Apply.getKind()) {
      case FullApplySiteKind::ApplyInst: {
         auto *AI = dyn_cast<ApplyInst>(Apply);

         // Split the BB and do NOT create a branch between the old and new
         // BBs; we will create the appropriate terminator manually later.
         ReturnToBB =
            callerBlock->split(std::next(Apply.getInstruction()->getIterator()));
         endBorrowInsertPts.push_back(&*ReturnToBB->begin());

         // Create an argument on the return-to BB representing the returned value.
         auto *retArg =
            ReturnToBB->createPhiArgument(AI->getType(), ValueOwnershipKind::Owned);
         // Replace all uses of the ApplyInst with the new argument.
         AI->replaceAllUsesWith(retArg);
         break;
      }
      case FullApplySiteKind::BeginApplyInst: {
         ReturnToBB =
            callerBlock->split(std::next(Apply.getInstruction()->getIterator()));
         // For begin_apply, we insert the end_borrow in the end_apply, abort_apply
         // blocks to ensure that our borrowed values live over both the body and
         // resume block of our coroutine.
         BeginApply->preprocess(ReturnToBB, endBorrowInsertPts);
         break;
      }
      case FullApplySiteKind::TryApplyInst: {
         auto *tai = cast<TryApplyInst>(Apply);
         ReturnToBB = tai->getNormalBB();
         endBorrowInsertPts.push_back(&*ReturnToBB->begin());
         throwBlock = tai->getErrorBB();
         break;
      }
   }

   // Then insert end_borrow in our end borrow block and in the throw
   // block if we have one.
   if (borrowedArgs.any()) {
      for (unsigned i : indices(AppliedArgs)) {
         if (!borrowedArgs.test(i)) {
            continue;
         }

         for (auto *insertPt : endBorrowInsertPts) {
            PILBuilderWithScope returnBuilder(insertPt, getBuilder());
            returnBuilder.createEndBorrow(Apply.getLoc(), entryArgs[i]);
         }

         if (throwBlock) {
            PILBuilderWithScope throwBuilder(throwBlock->begin(), getBuilder());
            throwBuilder.createEndBorrow(Apply.getLoc(), entryArgs[i]);
         }
      }
   }

   // Visit original BBs in depth-first preorder, starting with the
   // entry block, cloning all instructions and terminators.
   //
   // NextIter is initialized during `fixUp`.
   cloneFunctionBody(getCalleeFunction(), callerBlock, entryArgs);

   // For non-throwing applies, the inlined body now unconditionally branches to
   // the returned-to-code, which was previously part of the call site's basic
   // block. We could trivially merge these blocks now, however, this would be
   // quadratic: O(num-calls-in-block * num-instructions-in-block). Also,
   // guaranteeing that caller instructions following the inlined call are in a
   // separate block gives the inliner control over revisiting only the inlined
   // instructions.
   //
   // Once all calls in a function are inlined, unconditional branches are
   // eliminated by mergeBlocks.
   return NextIter;
}

void PILInlineCloner::visitTerminator(PILBasicBlock *BB) {
   // Coroutine terminators need special handling.
   if (BeginApply) {
      if (BeginApply->processTerminator(
         BB->getTerminator(), ReturnToBB,
         [=](PILBasicBlock *Block) -> PILBasicBlock * {
            return this->remapBasicBlock(Block);
         },
         [=](PILValue Val) -> PILValue { return this->getMappedValue(Val); }))
         return;
   }

   // Modify return terminators to branch to the return-to BB, rather than
   // trying to clone the ReturnInst.
   if (auto *RI = dyn_cast<ReturnInst>(BB->getTerminator())) {
      auto returnedValue = getMappedValue(RI->getOperand());
      getBuilder().createBranch(Loc.getValue(), ReturnToBB, returnedValue);
      return;
   }

   // Modify throw terminators to branch to the error-return BB, rather than
   // trying to clone the ThrowInst.
   if (auto *TI = dyn_cast<ThrowInst>(BB->getTerminator())) {
      switch (Apply.getKind()) {
         case FullApplySiteKind::ApplyInst:
            assert(cast<ApplyInst>(Apply)->isNonThrowing()
                   && "apply of a function with error result must be non-throwing");
            getBuilder().createUnreachable(Loc.getValue());
            return;
         case FullApplySiteKind::BeginApplyInst:
            assert(cast<BeginApplyInst>(Apply)->isNonThrowing()
                   && "apply of a function with error result must be non-throwing");
            getBuilder().createUnreachable(Loc.getValue());
            return;
         case FullApplySiteKind::TryApplyInst:
            auto tryAI = cast<TryApplyInst>(Apply);
            auto returnedValue = getMappedValue(TI->getOperand());
            getBuilder().createBranch(Loc.getValue(), tryAI->getErrorBB(),
                                      returnedValue);
            return;
      }
   }
   // Otherwise use normal visitor, which clones the existing instruction
   // but remaps basic blocks and values.
   visit(BB->getTerminator());
}

void PILInlineCloner::fixUp(PILFunction *calleeFunction) {
   // "Completing" the BeginApply only fixes the end of the apply scope. The
   // begin_apply itself lingers.
   if (BeginApply)
      BeginApply->complete();

   NextIter = std::next(Apply.getInstruction()->getIterator());

   assert(!Apply.getInstruction()->hasUsesOfAnyResult());

   auto deleteCallback = [this](PILInstruction *deletedI) {
      if (NextIter == deletedI->getIterator())
         ++NextIter;
      if (DeletionCallback)
         DeletionCallback(deletedI);
   };
   recursivelyDeleteTriviallyDeadInstructions(Apply.getInstruction(), true,
                                              deleteCallback);
}

PILValue PILInlineCloner::borrowFunctionArgument(PILValue callArg,
                                                 FullApplySite AI) {
   if (!AI.getFunction()->hasOwnership()
       || callArg.getOwnershipKind() != ValueOwnershipKind::Owned) {
      return PILValue();
   }

   PILBuilderWithScope beginBuilder(AI.getInstruction(), getBuilder());
   return beginBuilder.createBeginBorrow(AI.getLoc(), callArg);
}

void PILInlineCloner::visitDebugValueInst(DebugValueInst *Inst) {
   // The mandatory inliner drops debug_value instructions when inlining, as if
   // it were a "nodebug" function in C.
   if (IKind == InlineKind::MandatoryInline) return;

   return PILCloner<PILInlineCloner>::visitDebugValueInst(Inst);
}
void PILInlineCloner::visitDebugValueAddrInst(DebugValueAddrInst *Inst) {
   // The mandatory inliner drops debug_value_addr instructions when inlining, as
   // if it were a "nodebug" function in C.
   if (IKind == InlineKind::MandatoryInline) return;

   return PILCloner<PILInlineCloner>::visitDebugValueAddrInst(Inst);
}

const PILDebugScope *
PILInlineCloner::getOrCreateInlineScope(const PILDebugScope *CalleeScope) {
   if (!CalleeScope)
      return CallSiteScope;
   auto it = InlinedScopeCache.find(CalleeScope);
   if (it != InlinedScopeCache.end())
      return it->second;

   auto &M = getBuilder().getModule();
   auto InlinedAt =
      getOrCreateInlineScope(CalleeScope->InlinedCallSite);

   auto *ParentFunction = CalleeScope->Parent.dyn_cast<PILFunction *>();
   if (ParentFunction)
      ParentFunction = remapParentFunction(
         FuncBuilder, M, ParentFunction, SubsMap,
         getCalleeFunction()->getLoweredFunctionType()
            ->getInvocationGenericSignature(),
         ForInlining);

   auto *ParentScope = CalleeScope->Parent.dyn_cast<const PILDebugScope *>();
   auto *InlinedScope = new (M) PILDebugScope(
      CalleeScope->Loc, ParentFunction,
      ParentScope ? getOrCreateInlineScope(ParentScope) : nullptr, InlinedAt);
   InlinedScopeCache.insert({CalleeScope, InlinedScope});
   return InlinedScope;
}

//===----------------------------------------------------------------------===//
//                                 Cost Model
//===----------------------------------------------------------------------===//

static InlineCost getEnforcementCost(PILAccessEnforcement enforcement) {
   switch (enforcement) {
      case PILAccessEnforcement::Unknown:
         llvm_unreachable("evaluating cost of access with unknown enforcement?");
      case PILAccessEnforcement::Dynamic:
         return InlineCost::Expensive;
      case PILAccessEnforcement::Static:
      case PILAccessEnforcement::Unsafe:
         return InlineCost::Free;
   }
   llvm_unreachable("bad enforcement");
}

/// For now just assume that every PIL instruction is one to one with an LLVM
/// instruction. This is of course very much so not true.
InlineCost polar::instructionInlineCost(PILInstruction &I) {
   switch (I.getKind()) {
      case PILInstructionKind::IntegerLiteralInst:
      case PILInstructionKind::FloatLiteralInst:
      case PILInstructionKind::DebugValueInst:
      case PILInstructionKind::DebugValueAddrInst:
      case PILInstructionKind::StringLiteralInst:
      case PILInstructionKind::FixLifetimeInst:
      case PILInstructionKind::EndBorrowInst:
      case PILInstructionKind::BeginBorrowInst:
      case PILInstructionKind::MarkDependenceInst:
      case PILInstructionKind::PreviousDynamicFunctionRefInst:
      case PILInstructionKind::DynamicFunctionRefInst:
      case PILInstructionKind::FunctionRefInst:
      case PILInstructionKind::AllocGlobalInst:
      case PILInstructionKind::GlobalAddrInst:
      case PILInstructionKind::EndLifetimeInst:
      case PILInstructionKind::UncheckedOwnershipConversionInst:
         return InlineCost::Free;

         // Typed GEPs are free.
      case PILInstructionKind::TupleElementAddrInst:
      case PILInstructionKind::StructElementAddrInst:
      case PILInstructionKind::ProjectBlockStorageInst:
         return InlineCost::Free;

         // Aggregates are exploded at the IR level; these are effectively no-ops.
      case PILInstructionKind::TupleInst:
      case PILInstructionKind::StructInst:
      case PILInstructionKind::StructExtractInst:
      case PILInstructionKind::TupleExtractInst:
      case PILInstructionKind::DestructureStructInst:
      case PILInstructionKind::DestructureTupleInst:
         return InlineCost::Free;

         // Unchecked casts are free.
      case PILInstructionKind::AddressToPointerInst:
      case PILInstructionKind::PointerToAddressInst:

      case PILInstructionKind::UncheckedRefCastInst:
      case PILInstructionKind::UncheckedRefCastAddrInst:
      case PILInstructionKind::UncheckedAddrCastInst:
      case PILInstructionKind::UncheckedTrivialBitCastInst:
      case PILInstructionKind::UncheckedBitwiseCastInst:

      case PILInstructionKind::RawPointerToRefInst:
      case PILInstructionKind::RefToRawPointerInst:

      case PILInstructionKind::UpcastInst:

      case PILInstructionKind::ThinToThickFunctionInst:
      case PILInstructionKind::ThinFunctionToPointerInst:
      case PILInstructionKind::PointerToThinFunctionInst:
      case PILInstructionKind::ConvertFunctionInst:
      case PILInstructionKind::ConvertEscapeToNoEscapeInst:

      case PILInstructionKind::BridgeObjectToWordInst:
         return InlineCost::Free;

         // Access instructions are free unless we're dynamically enforcing them.
      case PILInstructionKind::BeginAccessInst:
         return getEnforcementCost(cast<BeginAccessInst>(I).getEnforcement());
      case PILInstructionKind::EndAccessInst:
         return getEnforcementCost(cast<EndAccessInst>(I).getBeginAccess()
                                      ->getEnforcement());
      case PILInstructionKind::BeginUnpairedAccessInst:
         return getEnforcementCost(cast<BeginUnpairedAccessInst>(I)
                                      .getEnforcement());
      case PILInstructionKind::EndUnpairedAccessInst:
         return getEnforcementCost(cast<EndUnpairedAccessInst>(I)
                                      .getEnforcement());

         // TODO: These are free if the metatype is for a Swift class.
//      case PILInstructionKind::ThickToObjCMetatypeInst:
//      case PILInstructionKind::ObjCToThickMetatypeInst:
//         return InlineCost::Expensive;

         // TODO: Bridge object conversions imply a masking operation that should be
         // "hella cheap" but not really expensive.
      case PILInstructionKind::BridgeObjectToRefInst:
      case PILInstructionKind::RefToBridgeObjectInst:
      case PILInstructionKind::ClassifyBridgeObjectInst:
      case PILInstructionKind::ValueToBridgeObjectInst:
         return InlineCost::Expensive;

      case PILInstructionKind::MetatypeInst:
         // Thin metatypes are always free.
         if (cast<MetatypeInst>(I).getType().castTo<MetatypeType>()
                ->getRepresentation() == MetatypeRepresentation::Thin)
            return InlineCost::Free;
         // TODO: Thick metatypes are free if they don't require generic or lazy
         // instantiation.
         return InlineCost::Expensive;

         // Protocol descriptor references are free.
//      case PILInstructionKind::ObjCProtocolInst:
//         return InlineCost::Free;

         // Metatype-to-object conversions are free.
//      case PILInstructionKind::ObjCExistentialMetatypeToObjectInst:
//      case PILInstructionKind::ObjCMetatypeToObjectInst:
//         return InlineCost::Free;

         // Return and unreachable are free.
      case PILInstructionKind::UnreachableInst:
      case PILInstructionKind::ReturnInst:
      case PILInstructionKind::ThrowInst:
      case PILInstructionKind::UnwindInst:
      case PILInstructionKind::YieldInst:
         return InlineCost::Free;

      case PILInstructionKind::AbortApplyInst:
      case PILInstructionKind::ApplyInst:
      case PILInstructionKind::TryApplyInst:
      case PILInstructionKind::AllocBoxInst:
      case PILInstructionKind::AllocExistentialBoxInst:
      case PILInstructionKind::AllocRefInst:
      case PILInstructionKind::AllocRefDynamicInst:
      case PILInstructionKind::AllocStackInst:
      case PILInstructionKind::AllocValueBufferInst:
      case PILInstructionKind::BindMemoryInst:
      case PILInstructionKind::BeginApplyInst:
      case PILInstructionKind::ValueMetatypeInst:
      case PILInstructionKind::WitnessMethodInst:
      case PILInstructionKind::AssignInst:
      case PILInstructionKind::AssignByWrapperInst:
      case PILInstructionKind::BranchInst:
      case PILInstructionKind::CheckedCastBranchInst:
      case PILInstructionKind::CheckedCastValueBranchInst:
      case PILInstructionKind::CheckedCastAddrBranchInst:
      case PILInstructionKind::ClassMethodInst:
//      case PILInstructionKind::ObjCMethodInst:
      case PILInstructionKind::CondBranchInst:
      case PILInstructionKind::CondFailInst:
      case PILInstructionKind::CopyBlockInst:
      case PILInstructionKind::CopyBlockWithoutEscapingInst:
      case PILInstructionKind::CopyAddrInst:
      case PILInstructionKind::RetainValueInst:
      case PILInstructionKind::RetainValueAddrInst:
      case PILInstructionKind::UnmanagedRetainValueInst:
      case PILInstructionKind::CopyValueInst:
      case PILInstructionKind::DeallocBoxInst:
      case PILInstructionKind::DeallocExistentialBoxInst:
      case PILInstructionKind::DeallocRefInst:
      case PILInstructionKind::DeallocPartialRefInst:
      case PILInstructionKind::DeallocStackInst:
      case PILInstructionKind::DeallocValueBufferInst:
      case PILInstructionKind::DeinitExistentialAddrInst:
      case PILInstructionKind::DeinitExistentialValueInst:
      case PILInstructionKind::DestroyAddrInst:
      case PILInstructionKind::EndApplyInst:
      case PILInstructionKind::ProjectValueBufferInst:
      case PILInstructionKind::ProjectBoxInst:
      case PILInstructionKind::ProjectExistentialBoxInst:
      case PILInstructionKind::ReleaseValueInst:
      case PILInstructionKind::ReleaseValueAddrInst:
      case PILInstructionKind::UnmanagedReleaseValueInst:
      case PILInstructionKind::DestroyValueInst:
      case PILInstructionKind::AutoreleaseValueInst:
      case PILInstructionKind::UnmanagedAutoreleaseValueInst:
      case PILInstructionKind::DynamicMethodBranchInst:
      case PILInstructionKind::EnumInst:
      case PILInstructionKind::IndexAddrInst:
      case PILInstructionKind::TailAddrInst:
      case PILInstructionKind::IndexRawPointerInst:
      case PILInstructionKind::InitEnumDataAddrInst:
      case PILInstructionKind::InitExistentialAddrInst:
      case PILInstructionKind::InitExistentialValueInst:
      case PILInstructionKind::InitExistentialMetatypeInst:
      case PILInstructionKind::InitExistentialRefInst:
      case PILInstructionKind::InjectEnumAddrInst:
      case PILInstructionKind::LoadInst:
      case PILInstructionKind::LoadBorrowInst:
      case PILInstructionKind::OpenExistentialAddrInst:
      case PILInstructionKind::OpenExistentialBoxInst:
      case PILInstructionKind::OpenExistentialBoxValueInst:
      case PILInstructionKind::OpenExistentialMetatypeInst:
      case PILInstructionKind::OpenExistentialRefInst:
      case PILInstructionKind::OpenExistentialValueInst:
      case PILInstructionKind::PartialApplyInst:
      case PILInstructionKind::ExistentialMetatypeInst:
      case PILInstructionKind::RefElementAddrInst:
      case PILInstructionKind::RefTailAddrInst:
      case PILInstructionKind::StoreInst:
      case PILInstructionKind::StoreBorrowInst:
      case PILInstructionKind::StrongReleaseInst:
      case PILInstructionKind::SetDeallocatingInst:
      case PILInstructionKind::StrongRetainInst:
      case PILInstructionKind::SuperMethodInst:
      case PILInstructionKind::ObjCSuperMethodInst:
      case PILInstructionKind::SwitchEnumAddrInst:
      case PILInstructionKind::SwitchEnumInst:
      case PILInstructionKind::SwitchValueInst:
      case PILInstructionKind::UncheckedEnumDataInst:
      case PILInstructionKind::UncheckedTakeEnumDataAddrInst:
      case PILInstructionKind::UnconditionalCheckedCastInst:
      case PILInstructionKind::UnconditionalCheckedCastAddrInst:
      case PILInstructionKind::UnconditionalCheckedCastValueInst:
      case PILInstructionKind::IsEscapingClosureInst:
      case PILInstructionKind::IsUniqueInst:
      case PILInstructionKind::InitBlockStorageHeaderInst:
      case PILInstructionKind::SelectEnumAddrInst:
      case PILInstructionKind::SelectEnumInst:
      case PILInstructionKind::SelectValueInst:
      case PILInstructionKind::KeyPathInst:
      case PILInstructionKind::GlobalValueInst:
#define COMMON_ALWAYS_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name)          \
  case PILInstructionKind::Name##ToRefInst:                                    \
  case PILInstructionKind::RefTo##Name##Inst:                                  \
  case PILInstructionKind::StrongCopy##Name##ValueInst:
#define NEVER_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
  case PILInstructionKind::Load##Name##Inst: \
  case PILInstructionKind::Store##Name##Inst:
#define ALWAYS_LOADABLE_CHECKED_REF_STORAGE(Name, ...)                         \
  COMMON_ALWAYS_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name)                \
  case PILInstructionKind::Name##RetainInst:                                   \
  case PILInstructionKind::Name##ReleaseInst:                                  \
  case PILInstructionKind::StrongRetain##Name##Inst:
#define SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
  NEVER_LOADABLE_CHECKED_REF_STORAGE(Name, "...") \
  ALWAYS_LOADABLE_CHECKED_REF_STORAGE(Name, "...")
#define UNCHECKED_REF_STORAGE(Name, ...) \
  COMMON_ALWAYS_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name)
#include "polarphp/ast/ReferenceStorageDef.h"
#undef COMMON_ALWAYS_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE
         return InlineCost::Expensive;

      case PILInstructionKind::BuiltinInst: {
         auto *BI = cast<BuiltinInst>(&I);
         // Expect intrinsics are 'free' instructions.
         if (BI->getIntrinsicInfo().ID == llvm::Intrinsic::expect)
            return InlineCost::Free;
         if (BI->getBuiltinInfo().ID == BuiltinValueKind::OnFastPath)
            return InlineCost::Free;

         return InlineCost::Expensive;
      }
      case PILInstructionKind::MarkFunctionEscapeInst:
      case PILInstructionKind::MarkUninitializedInst:
         llvm_unreachable("not valid in canonical sil");
      case PILInstructionKind::ObjectInst:
         llvm_unreachable("not valid in a function");
   }

   llvm_unreachable("Unhandled ValueKind in switch.");
}
