//===--- LoopUnroll.cpp - Loop unrolling ----------------------------------===//
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

#define DEBUG_TYPE "pil-loopunroll"

#include "llvm/ADT/DepthFirstIterator.h"

#include "polarphp/pil/lang/PatternMatch.h"
#include "polarphp/pil/lang/PILCloner.h"
#include "polarphp/pil/optimizer/Analysis/LoopAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/PerformanceInlinerUtils.h"
#include "polarphp/pil/optimizer/utils/PILInliner.h"
#include "polarphp/pil/optimizer/utils/PILSSAUpdater.h"

using namespace polar;
using namespace polar::patternmatch;

using llvm::DenseMap;
using llvm::MapVector;

namespace {

/// Clone the basic blocks in a loop.
///
/// Currently invalidates the DomTree.
class LoopCloner : public PILCloner<LoopCloner> {
  PILLoop *Loop;

  friend class PILInstructionVisitor<LoopCloner>;
  friend class PILCloner<LoopCloner>;

public:
  LoopCloner(PILLoop *Loop)
      : PILCloner<LoopCloner>(*Loop->getHeader()->getParent()), Loop(Loop) {}

  /// Clone the basic blocks in the loop.
  void cloneLoop();

  // Update SSA helper.
  void collectLoopLiveOutValues(
      DenseMap<PILValue, SmallVector<PILValue, 8>> &LoopLiveOutValues);

protected:
  // PILCloner CRTP override.
  PILValue getMappedValue(PILValue V) {
    if (auto *BB = V->getParentBlock()) {
      if (!Loop->contains(BB))
        return V;
    }
    return PILCloner<LoopCloner>::getMappedValue(V);
  }
  // PILCloner CRTP override.
  void postProcess(PILInstruction *Orig, PILInstruction *Cloned) {
    PILCloner<LoopCloner>::postProcess(Orig, Cloned);
  }
};

} // end anonymous namespace

void LoopCloner::cloneLoop() {
  SmallVector<PILBasicBlock *, 16> ExitBlocks;
  Loop->getExitBlocks(ExitBlocks);

  // Clone the entire loop.
  cloneReachableBlocks(Loop->getHeader(), ExitBlocks);
}

/// Determine the number of iterations the loop is at most executed. The loop
/// might contain early exits so this is the maximum if no early exits are
/// taken.
static Optional<uint64_t> getMaxLoopTripCount(PILLoop *Loop,
                                              PILBasicBlock *Preheader,
                                              PILBasicBlock *Header,
                                              PILBasicBlock *Latch) {

  // Skip a split backedge.
  PILBasicBlock *OrigLatch = Latch;
  if (!Loop->isLoopExiting(Latch) &&
      !(Latch = Latch->getSinglePredecessorBlock()))
    return None;
  if (!Loop->isLoopExiting(Latch))
    return None;

  // Get the loop exit condition.
  auto *CondBr = dyn_cast<CondBranchInst>(Latch->getTerminator());
  if (!CondBr)
    return None;

  // Match an add 1 recurrence.

  auto *Cmp = dyn_cast<BuiltinInst>(CondBr->getCondition());
  if (!Cmp)
    return None;

  unsigned Adjust = 0;
  PILBasicBlock *Exit = CondBr->getTrueBB();

  switch (Cmp->getBuiltinInfo().ID) {
    case BuiltinValueKind::ICMP_EQ:
    case BuiltinValueKind::ICMP_SGE:
      break;
    case BuiltinValueKind::ICMP_SGT:
      Adjust = 1;
      break;
    case BuiltinValueKind::ICMP_SLE:
      Exit = CondBr->getFalseBB();
      Adjust = 1;
      break;
    case BuiltinValueKind::ICMP_NE:
    case BuiltinValueKind::ICMP_SLT:
      Exit = CondBr->getFalseBB();
      break;
    default:
      return None;
  }

  if (Loop->contains(Exit))
    return None;

  auto *End = dyn_cast<IntegerLiteralInst>(Cmp->getArguments()[1]);
  if (!End)
    return None;

  PILValue RecNext = Cmp->getArguments()[0];
  PILPhiArgument *RecArg;
  if (!match(RecNext, m_TupleExtractOperation(
                          m_ApplyInst(BuiltinValueKind::SAddOver,
                                      m_PILPhiArgument(RecArg), m_One()),
                          0)))
    return None;

  if (RecArg->getParent() != Header)
    return None;

  auto *Start = dyn_cast_or_null<IntegerLiteralInst>(
      RecArg->getIncomingPhiValue(Preheader));
  if (!Start)
    return None;

  if (RecNext != RecArg->getIncomingPhiValue(OrigLatch))
    return None;

  auto StartVal = Start->getValue();
  auto EndVal = End->getValue();
  if (StartVal.sgt(EndVal))
    return None;

  auto Dist = EndVal - StartVal;
  if (Dist.getBitWidth() > 64)
    return None;

  if (Dist == 0)
    return None;

  return Dist.getZExtValue() + Adjust;
}

/// Check whether we can duplicate the instructions in the loop and use a
/// heuristic that looks at the trip count and the cost of the instructions in
/// the loop to determine whether we should unroll this loop.
static bool canAndShouldUnrollLoop(PILLoop *Loop, uint64_t TripCount) {
  assert(Loop->getSubLoops().empty() && "Expect innermost loops");
  if (TripCount > 32)
    return false;

  // We can unroll a loop if we can duplicate the instructions it holds.
  uint64_t Cost = 0;
  // Average number of instructions per basic block.
  // It is used to estimate the cost of the callee
  // inside a loop.
  const uint64_t InsnsPerBB = 4;
  // Use command-line threshold for unrolling.
  const uint64_t PILLoopUnrollThreshold = Loop->getBlocks().empty() ? 0 :
    (Loop->getBlocks())[0]->getParent()->getModule().getOptions().UnrollThreshold;
  for (auto *BB : Loop->getBlocks()) {
    for (auto &Inst : *BB) {
      if (!Loop->canDuplicate(&Inst))
        return false;
      if (instructionInlineCost(Inst) != InlineCost::Free)
        ++Cost;
      if (auto AI = FullApplySite::isa(&Inst)) {
        auto Callee = AI.getCalleeFunction();
        if (Callee && getEligibleFunction(AI, InlineSelection::Everything)) {
          // If callee is rather big and potentialy inlinable, it may be better
          // not to unroll, so that the body of the calle can be inlined later.
          Cost += Callee->size() * InsnsPerBB;
        }
      }
      if (Cost * TripCount > PILLoopUnrollThreshold)
        return false;
  }
  }
  return true;
}

/// Redirect the terminator of the current loop iteration's latch to the next
/// iterations header or if this is the last iteration remove the backedge to
/// the header.
static void redirectTerminator(PILBasicBlock *Latch, unsigned CurLoopIter,
                               unsigned LastLoopIter, PILBasicBlock *CurrentHeader,
                               PILBasicBlock *NextIterationsHeader) {

  auto *CurrentTerminator = Latch->getTerminator();

  // We can either have a split backedge as our latch terminator.
  //   HeaderBlock:
  //     ...
  //     cond_br %cond, ExitBlock, BackedgeBlock
  //
  //   BackedgeBlock:
  //     br HeaderBlock:
  //
  // Or a conditional branch back to the header.
  //   HeaderBlock:
  //     ...
  //     cond_br %cond, ExitBlock, HeaderBlock
  //
  // Redirect the HeaderBlock target to the unrolled successor. In the
  // unrolled block of the last iteration unconditionally jump to the
  // ExitBlock instead.

  // Handle the split backedge case.
  if (auto *Br = dyn_cast<BranchInst>(CurrentTerminator)) {
    // On the last iteration change the conditional exit to an unconditional
    // one.
    if (CurLoopIter == LastLoopIter) {
      auto *CondBr = cast<CondBranchInst>(
          Latch->getSinglePredecessorBlock()->getTerminator());
      if (CondBr->getTrueBB() != Latch)
        PILBuilderWithScope(CondBr).createBranch(
            CondBr->getLoc(), CondBr->getTrueBB(), CondBr->getTrueArgs());
      else
        PILBuilderWithScope(CondBr).createBranch(
            CondBr->getLoc(), CondBr->getFalseBB(), CondBr->getFalseArgs());
      CondBr->eraseFromParent();
      return;
    }

    // Otherwise, branch to the next iteration's header.
    PILBuilderWithScope(Br).createBranch(Br->getLoc(), NextIterationsHeader,
                                         Br->getArgs());
    Br->eraseFromParent();
    return;
  }

  // Otherwise, we have a conditional branch to the header.
  auto *CondBr = cast<CondBranchInst>(CurrentTerminator);
  // On the last iteration change the conditional exit to an unconditional
  // one.
  if (CurLoopIter == LastLoopIter) {
    if (CondBr->getTrueBB() == CurrentHeader) {
      PILBuilderWithScope(CondBr).createBranch(
          CondBr->getLoc(), CondBr->getFalseBB(), CondBr->getFalseArgs());
    } else {
      assert(CondBr->getFalseBB() == CurrentHeader);
      PILBuilderWithScope(CondBr).createBranch(
          CondBr->getLoc(), CondBr->getTrueBB(), CondBr->getTrueArgs());
    }
    CondBr->eraseFromParent();
    return;
  }

  // Otherwise, branch to the next iteration's header.
  if (CondBr->getTrueBB() == CurrentHeader) {
    PILBuilderWithScope(CondBr).createCondBranch(
        CondBr->getLoc(), CondBr->getCondition(), NextIterationsHeader,
        CondBr->getTrueArgs(), CondBr->getFalseBB(), CondBr->getFalseArgs());
  } else {
    assert(CondBr->getFalseBB() == CurrentHeader);
    PILBuilderWithScope(CondBr).createCondBranch(
        CondBr->getLoc(), CondBr->getCondition(), CondBr->getTrueBB(),
        CondBr->getTrueArgs(), NextIterationsHeader, CondBr->getFalseArgs());
  }
  CondBr->eraseFromParent();
}

/// Collect all the loop live out values in the map that maps original live out
/// value to live out value in the cloned loop.
void LoopCloner::collectLoopLiveOutValues(
    DenseMap<PILValue, SmallVector<PILValue, 8>> &LoopLiveOutValues) {
  for (auto *Block : Loop->getBlocks()) {
    // Look at block arguments.
    for (auto *Arg : Block->getArguments()) {
      for (auto *Op : Arg->getUses()) {
        // Is this use outside the loop?
        if (!Loop->contains(Op->getUser())) {
          auto ArgumentValue = PILValue(Arg);
          if (!LoopLiveOutValues.count(ArgumentValue))
            LoopLiveOutValues[ArgumentValue].push_back(
                getMappedValue(ArgumentValue));
        }
      }
    }
    // And the instructions.
    for (auto &Inst : *Block) {
      for (PILValue result : Inst.getResults()) {
        for (auto *Op : result->getUses()) {
          // Ignore uses inside the loop.
          if (Loop->contains(Op->getUser()))
            continue;

          auto UsedValue = Op->get();
          assert(UsedValue == result && "Instructions must match");

          if (!LoopLiveOutValues.count(UsedValue))
            LoopLiveOutValues[UsedValue].push_back(getMappedValue(result));
        }
      }
    }
  }
}

static void
updateSSA(PILModule &M, PILLoop *Loop,
          DenseMap<PILValue, SmallVector<PILValue, 8>> &LoopLiveOutValues) {
  PILSSAUpdater SSAUp;
  for (auto &MapEntry : LoopLiveOutValues) {
    // Collect out of loop uses of this value.
    auto OrigValue = MapEntry.first;
    SmallVector<UseWrapper, 16> UseList;
    for (auto Use : OrigValue->getUses())
      if (!Loop->contains(Use->getUser()->getParent()))
        UseList.push_back(UseWrapper(Use));
    // Update SSA of use with the available values.
    SSAUp.Initialize(OrigValue->getType());
    SSAUp.AddAvailableValue(OrigValue->getParentBlock(), OrigValue);
    for (auto NewValue : MapEntry.second)
      SSAUp.AddAvailableValue(NewValue->getParentBlock(), NewValue);
    for (auto U : UseList) {
      Operand *Use = U;
      SSAUp.RewriteUse(*Use);
    }
  }
}

/// Try to fully unroll the loop if we can determine the trip count and the trip
/// count lis below a threshold.
static bool tryToUnrollLoop(PILLoop *Loop) {
  assert(Loop->getSubLoops().empty() && "Expecting innermost loops");

  auto *Preheader = Loop->getLoopPreheader();
  if (!Preheader)
    return false;
  PILModule &M = Preheader->getParent()->getModule();

  auto *Latch = Loop->getLoopLatch();
  if (!Latch)
    return false;

  auto *Header = Loop->getHeader();

  Optional<uint64_t> MaxTripCount =
      getMaxLoopTripCount(Loop, Preheader, Header, Latch);
  if (!MaxTripCount)
    return false;

  if (!canAndShouldUnrollLoop(Loop, MaxTripCount.getValue()))
    return false;

  // TODO: We need to split edges from non-condbr exits for the SSA updater. For
  // now just don't handle loops containing such exits.
  SmallVector<PILBasicBlock *, 16> ExitingBlocks;
  Loop->getExitingBlocks(ExitingBlocks);
  for (auto &Exit : ExitingBlocks)
    if (!isa<CondBranchInst>(Exit->getTerminator()))
      return false;

  LLVM_DEBUG(llvm::dbgs() << "Unrolling loop in "
                          << Header->getParent()->getName()
                          << " " << *Loop << "\n");

  SmallVector<PILBasicBlock *, 16> Headers;
  Headers.push_back(Header);

  SmallVector<PILBasicBlock *, 16> Latches;
  Latches.push_back(Latch);

  DenseMap<PILValue, SmallVector<PILValue, 8>> LoopLiveOutValues;

  // Copy the body MaxTripCount-1 times.
  for (uint64_t Cnt = 1; Cnt < *MaxTripCount; ++Cnt) {
    // Clone the blocks in the loop.
    LoopCloner cloner(Loop);
    cloner.cloneLoop();
    Headers.push_back(cloner.getOpBasicBlock(Header));
    Latches.push_back(cloner.getOpBasicBlock(Latch));

    // Collect values defined in the loop but used outside. On the first
    // iteration we populate the map from original loop to cloned loop. On
    // subsequent iterations we only need to update this map with the values
    // from the new iteration's clone.
    if (Cnt == 1)
      cloner.collectLoopLiveOutValues(LoopLiveOutValues);
    else {
      for (auto &MapEntry : LoopLiveOutValues) {
        // Look it up in the value map.
        PILValue MappedValue = cloner.getOpValue(MapEntry.first);
        MapEntry.second.push_back(MappedValue);
        assert(MapEntry.second.size() == Cnt);
      }
    }
  }

  // Thread the loop clones by redirecting the loop latches to the successor
  // iteration's header.
  for (unsigned Iteration = 0, End = Latches.size(); Iteration != End;
       ++Iteration) {
    auto *CurrentLatch = Latches[Iteration];
    auto LastIteration = End - 1;
    auto *CurrentHeader = Headers[Iteration];
    auto *NextIterationsHeader =
        Iteration == LastIteration ? nullptr : Headers[Iteration + 1];

    redirectTerminator(CurrentLatch, Iteration, LastIteration, CurrentHeader,
                       NextIterationsHeader);
  }

  // Fixup SSA form for loop values used outside the loop.
  updateSSA(M, Loop, LoopLiveOutValues);
  return true;
}

// =============================================================================
//                                 Driver
// =============================================================================

namespace {

class LoopUnrolling : public PILFunctionTransform {

  void run() override {
    bool Changed = false;

    auto *Fun = getFunction();
    PILLoopInfo *LoopInfo = PM->getAnalysis<PILLoopAnalysis>()->get(Fun);

    // Collect innermost loops.
    SmallVector<PILLoop *, 16> InnermostLoops;
    for (auto *Loop : *LoopInfo) {
      SmallVector<PILLoop *, 8> Worklist;
      Worklist.push_back(Loop);

      for (unsigned i = 0; i < Worklist.size(); ++i) {
        auto *L = Worklist[i];
        for (auto *SubLoop : *L)
          Worklist.push_back(SubLoop);
        if (L->getSubLoops().empty())
          InnermostLoops.push_back(L);
      }
    }

    // Try to unroll innermost loops.
    for (auto *Loop : InnermostLoops)
      Changed |= tryToUnrollLoop(Loop);

    if (Changed) {
      invalidateAnalysis(PILAnalysis::InvalidationKind::FunctionBody);
    }
  }
};

} // end anonymous namespace

PILTransform *polar::createLoopUnroll() {
  return new LoopUnrolling();
}
