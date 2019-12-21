//===--- CFGOptUtils.cpp - PIL CFG edge utilities -------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "polarphp/pil/optimizer/utils/CFGOptUtils.h"
#include "polarphp/demangling/ManglingMacros.h"
#include "polarphp/pil/lang/BasicBlockUtils.h"
#include "polarphp/pil/lang/Dominance.h"
#include "polarphp/pil/lang/LoopInfo.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "llvm/ADT/TinyPtrVector.h"

using namespace polar;

/// Adds a new argument to an edge between a branch and a destination
/// block.
///
/// \param branch The terminator to add the argument to.
/// \param dest The destination block of the edge.
/// \param val The value to the arguments of the branch.
/// \return The created branch. The old branch is deleted.
/// The argument is appended at the end of the argument tuple.
TermInst *polar::addNewEdgeValueToBranch(TermInst *branch, PILBasicBlock *dest,
                                         PILValue val) {
   PILBuilderWithScope builder(branch);
   TermInst *newBr = nullptr;

   if (auto *cbi = dyn_cast<CondBranchInst>(branch)) {
      SmallVector<PILValue, 8> trueArgs;
      SmallVector<PILValue, 8> falseArgs;

      for (auto arg : cbi->getTrueArgs())
         trueArgs.push_back(arg);

      for (auto arg : cbi->getFalseArgs())
         falseArgs.push_back(arg);

      if (dest == cbi->getTrueBB()) {
         trueArgs.push_back(val);
         assert(trueArgs.size() == dest->getNumArguments());
      }
      if (dest == cbi->getFalseBB()) {
         falseArgs.push_back(val);
         assert(falseArgs.size() == dest->getNumArguments());
      }

      newBr = builder.createCondBranch(
         cbi->getLoc(), cbi->getCondition(), cbi->getTrueBB(), trueArgs,
         cbi->getFalseBB(), falseArgs, cbi->getTrueBBCount(),
         cbi->getFalseBBCount());
   } else if (auto *bi = dyn_cast<BranchInst>(branch)) {
      SmallVector<PILValue, 8> args;

      for (auto arg : bi->getArgs())
         args.push_back(arg);

      args.push_back(val);
      assert(args.size() == dest->getNumArguments());
      newBr = builder.createBranch(bi->getLoc(), bi->getDestBB(), args);
   } else {
      // At the moment we can only add arguments to br and cond_br.
      llvm_unreachable("Can't add argument to terminator");
   }

   branch->dropAllReferences();
   branch->eraseFromParent();

   return newBr;
}

static void
deleteTriviallyDeadOperandsOfDeadArgument(MutableArrayRef<Operand> termOperands,
                                          unsigned deadArgIndex) {
   Operand &op = termOperands[deadArgIndex];
   auto *i = op.get()->getDefiningInstruction();
   if (!i)
      return;
   op.set(PILUndef::get(op.get()->getType(), *i->getFunction()));
   recursivelyDeleteTriviallyDeadInstructions(i);
}

// Our implementation assumes that our caller is attempting to remove a dead
// PILPhiArgument from a PILBasicBlock and has already RAUWed the argument.
TermInst *polar::deleteEdgeValue(TermInst *branch, PILBasicBlock *destBlock,
                                 size_t argIndex) {
   if (auto *cbi = dyn_cast<CondBranchInst>(branch)) {
      SmallVector<PILValue, 8> trueArgs;
      SmallVector<PILValue, 8> falseArgs;

      llvm::copy(cbi->getTrueArgs(), std::back_inserter(trueArgs));
      llvm::copy(cbi->getFalseArgs(), std::back_inserter(falseArgs));

      if (destBlock == cbi->getTrueBB()) {
         deleteTriviallyDeadOperandsOfDeadArgument(cbi->getTrueOperands(),
                                                   argIndex);
         trueArgs.erase(trueArgs.begin() + argIndex);
      }

      if (destBlock == cbi->getFalseBB()) {
         deleteTriviallyDeadOperandsOfDeadArgument(cbi->getFalseOperands(),
                                                   argIndex);
         falseArgs.erase(falseArgs.begin() + argIndex);
      }

      PILBuilderWithScope builder(cbi);
      auto *result = builder.createCondBranch(
         cbi->getLoc(), cbi->getCondition(), cbi->getTrueBB(), trueArgs,
         cbi->getFalseBB(), falseArgs, cbi->getTrueBBCount(),
         cbi->getFalseBBCount());
      branch->eraseFromParent();
      return result;
   }

   if (auto *bi = dyn_cast<BranchInst>(branch)) {
      SmallVector<PILValue, 8> args;
      llvm::copy(bi->getArgs(), std::back_inserter(args));

      deleteTriviallyDeadOperandsOfDeadArgument(bi->getAllOperands(), argIndex);
      args.erase(args.begin() + argIndex);
      auto *result = PILBuilderWithScope(bi).createBranch(bi->getLoc(),
                                                          bi->getDestBB(), args);
      branch->eraseFromParent();
      return result;
   }

   llvm_unreachable("unsupported terminator");
}

void polar::erasePhiArgument(PILBasicBlock *block, unsigned argIndex) {
   assert(block->getArgument(argIndex)->isPhiArgument()
          && "Only should be used on phi arguments");
   block->eraseArgument(argIndex);

   // Determine the set of predecessors in case any predecessor has
   // two edges to this block (e.g. a conditional branch where both
   // sides reach this block).
   //
   // NOTE: This needs to be a SmallSetVector since we need both uniqueness /and/
   // insertion order. Otherwise non-determinism can result.
   SmallSetVector<PILBasicBlock *, 8> predBlocks;

   for (auto *pred : block->getPredecessorBlocks())
      predBlocks.insert(pred);

   for (auto *pred : predBlocks)
      deleteEdgeValue(pred->getTerminator(), block, argIndex);
}

/// Changes the edge value between a branch and destination basic block
/// at the specified index. Changes all edges from \p branch to \p dest to carry
/// the value.
///
/// \param branch The branch to modify.
/// \param dest The destination of the edge.
/// \param idx The index of the argument to modify.
/// \param Val The new value to use.
/// \return The new branch. Deletes the old one.
/// Changes the edge value between a branch and destination basic block at the
/// specified index.
TermInst *polar::changeEdgeValue(TermInst *branch, PILBasicBlock *dest,
                                 size_t idx, PILValue Val) {
   PILBuilderWithScope builder(branch);

   if (auto *cbi = dyn_cast<CondBranchInst>(branch)) {
      SmallVector<PILValue, 8> trueArgs;
      SmallVector<PILValue, 8> falseArgs;

      OperandValueArrayRef oldTrueArgs = cbi->getTrueArgs();
      bool branchOnTrue = cbi->getTrueBB() == dest;
      assert((!branchOnTrue || idx < oldTrueArgs.size()) && "Not enough edges");

      // Copy the edge values overwriting the edge at idx.
      for (unsigned i = 0, e = oldTrueArgs.size(); i != e; ++i) {
         if (branchOnTrue && idx == i)
            trueArgs.push_back(Val);
         else
            trueArgs.push_back(oldTrueArgs[i]);
      }
      assert(trueArgs.size() == cbi->getTrueBB()->getNumArguments()
             && "Destination block's number of arguments must match");

      OperandValueArrayRef oldFalseArgs = cbi->getFalseArgs();
      bool branchOnFalse = cbi->getFalseBB() == dest;
      assert((!branchOnFalse || idx < oldFalseArgs.size()) && "Not enough edges");

      // Copy the edge values overwriting the edge at idx.
      for (unsigned i = 0, e = oldFalseArgs.size(); i != e; ++i) {
         if (branchOnFalse && idx == i)
            falseArgs.push_back(Val);
         else
            falseArgs.push_back(oldFalseArgs[i]);
      }
      assert(falseArgs.size() == cbi->getFalseBB()->getNumArguments()
             && "Destination block's number of arguments must match");

      cbi = builder.createCondBranch(
         cbi->getLoc(), cbi->getCondition(), cbi->getTrueBB(), trueArgs,
         cbi->getFalseBB(), falseArgs, cbi->getTrueBBCount(),
         cbi->getFalseBBCount());
      branch->dropAllReferences();
      branch->eraseFromParent();
      return cbi;
   }

   if (auto *bi = dyn_cast<BranchInst>(branch)) {
      SmallVector<PILValue, 8> args;

      assert(idx < bi->getNumArgs() && "Not enough edges");
      OperandValueArrayRef oldArgs = bi->getArgs();

      // Copy the edge values overwriting the edge at idx.
      for (unsigned i = 0, e = oldArgs.size(); i != e; ++i) {
         if (idx == i)
            args.push_back(Val);
         else
            args.push_back(oldArgs[i]);
      }
      assert(args.size() == dest->getNumArguments());

      bi = builder.createBranch(bi->getLoc(), bi->getDestBB(), args);
      branch->dropAllReferences();
      branch->eraseFromParent();
      return bi;
   }

   llvm_unreachable("Unhandled terminator leading to merge block");
}

template <class SwitchEnumTy, class SwitchEnumCaseTy>
PILBasicBlock *replaceSwitchDest(SwitchEnumTy *sTy,
                                 SmallVectorImpl<SwitchEnumCaseTy> &cases,
                                 unsigned edgeIdx, PILBasicBlock *newDest) {
   auto *defaultBB = sTy->hasDefault() ? sTy->getDefaultBB() : nullptr;
   for (unsigned i = 0, e = sTy->getNumCases(); i != e; ++i)
      if (edgeIdx != i)
         cases.push_back(sTy->getCase(i));
      else
         cases.push_back(std::make_pair(sTy->getCase(i).first, newDest));
   if (edgeIdx == sTy->getNumCases())
      defaultBB = newDest;
   return defaultBB;
}

template <class SwitchEnumTy, class SwitchEnumCaseTy>
PILBasicBlock *
replaceSwitchDest(SwitchEnumTy *sTy, SmallVectorImpl<SwitchEnumCaseTy> &cases,
                  PILBasicBlock *oldDest, PILBasicBlock *newDest) {
   auto *defaultBB = sTy->hasDefault() ? sTy->getDefaultBB() : nullptr;
   for (unsigned i = 0, e = sTy->getNumCases(); i != e; ++i)
      if (sTy->getCase(i).second != oldDest)
         cases.push_back(sTy->getCase(i));
      else
         cases.push_back(std::make_pair(sTy->getCase(i).first, newDest));
   if (oldDest == defaultBB)
      defaultBB = newDest;
   return defaultBB;
}

/// Replace a branch target.
///
/// \param t The terminating instruction to modify.
/// \param oldDest The successor block that will be replaced.
/// \param newDest The new target block.
/// \param preserveArgs If set, preserve arguments on the replaced edge.
void polar::replaceBranchTarget(TermInst *t, PILBasicBlock *oldDest,
                                PILBasicBlock *newDest, bool preserveArgs) {
   PILBuilderWithScope builder(t);

   switch (t->getTermKind()) {
      // Only Branch and CondBranch may have arguments.
      case TermKind::BranchInst: {
         auto br = cast<BranchInst>(t);
         assert(oldDest == br->getDestBB() && "wrong branch target");
         SmallVector<PILValue, 8> args;
         if (preserveArgs) {
            for (auto arg : br->getArgs())
               args.push_back(arg);
         }
         builder.createBranch(t->getLoc(), newDest, args);
         br->dropAllReferences();
         br->eraseFromParent();
         return;
      }

      case TermKind::CondBranchInst: {
         auto condBr = cast<CondBranchInst>(t);
         SmallVector<PILValue, 8> trueArgs;
         if (oldDest == condBr->getFalseBB() || preserveArgs) {
            for (auto Arg : condBr->getTrueArgs())
               trueArgs.push_back(Arg);
         }
         SmallVector<PILValue, 8> falseArgs;
         if (oldDest == condBr->getTrueBB() || preserveArgs) {
            for (auto arg : condBr->getFalseArgs())
               falseArgs.push_back(arg);
         }
         PILBasicBlock *trueDest = condBr->getTrueBB();
         PILBasicBlock *falseDest = condBr->getFalseBB();
         if (oldDest == condBr->getTrueBB()) {
            trueDest = newDest;
         } else {
            assert(oldDest == condBr->getFalseBB() && "wrong cond_br target");
            falseDest = newDest;
         }

         builder.createCondBranch(
            condBr->getLoc(), condBr->getCondition(), trueDest, trueArgs, falseDest,
            falseArgs, condBr->getTrueBBCount(), condBr->getFalseBBCount());
         condBr->dropAllReferences();
         condBr->eraseFromParent();
         return;
      }

      case TermKind::SwitchValueInst: {
         auto sii = cast<SwitchValueInst>(t);
         SmallVector<std::pair<PILValue, PILBasicBlock *>, 8> cases;
         auto *defaultBB = replaceSwitchDest(sii, cases, oldDest, newDest);
         builder.createSwitchValue(sii->getLoc(), sii->getOperand(), defaultBB,
                                   cases);
         sii->eraseFromParent();
         return;
      }

      case TermKind::SwitchEnumInst: {
         auto sei = cast<SwitchEnumInst>(t);
         SmallVector<std::pair<EnumElementDecl *, PILBasicBlock *>, 8> cases;
         auto *defaultBB = replaceSwitchDest(sei, cases, oldDest, newDest);
         builder.createSwitchEnum(sei->getLoc(), sei->getOperand(), defaultBB,
                                  cases);
         sei->eraseFromParent();
         return;
      }

      case TermKind::SwitchEnumAddrInst: {
         auto sei = cast<SwitchEnumAddrInst>(t);
         SmallVector<std::pair<EnumElementDecl *, PILBasicBlock *>, 8> cases;
         auto *defaultBB = replaceSwitchDest(sei, cases, oldDest, newDest);
         builder.createSwitchEnumAddr(sei->getLoc(), sei->getOperand(), defaultBB,
                                      cases);
         sei->eraseFromParent();
         return;
      }

      case TermKind::DynamicMethodBranchInst: {
         auto dmbi = cast<DynamicMethodBranchInst>(t);
         assert((oldDest == dmbi->getHasMethodBB()
                         || oldDest == dmbi->getNoMethodBB()) && "Invalid edge index");
         auto hasMethodBB =
            oldDest == dmbi->getHasMethodBB() ? newDest : dmbi->getHasMethodBB();
         auto noMethodBB =
            oldDest == dmbi->getNoMethodBB() ? newDest : dmbi->getNoMethodBB();
         builder.createDynamicMethodBranch(dmbi->getLoc(), dmbi->getOperand(),
                                           dmbi->getMember(), hasMethodBB,
                                           noMethodBB);
         dmbi->eraseFromParent();
         return;
      }

      case TermKind::CheckedCastBranchInst: {
         auto cbi = cast<CheckedCastBranchInst>(t);
         assert((oldDest == cbi->getSuccessBB()
                         || oldDest == cbi->getFailureBB()) && "Invalid edge index");
         auto successBB =
            oldDest == cbi->getSuccessBB() ? newDest : cbi->getSuccessBB();
         auto failureBB =
            oldDest == cbi->getFailureBB() ? newDest : cbi->getFailureBB();
         builder.createCheckedCastBranch(
            cbi->getLoc(), cbi->isExact(), cbi->getOperand(),
            cbi->getTargetLoweredType(), cbi->getTargetFormalType(),
            successBB, failureBB, cbi->getTrueBBCount(), cbi->getFalseBBCount());
         cbi->eraseFromParent();
         return;
      }

      case TermKind::CheckedCastValueBranchInst: {
         auto cbi = cast<CheckedCastValueBranchInst>(t);
         assert((oldDest == cbi->getSuccessBB()
                         || oldDest == cbi->getFailureBB()) && "Invalid edge index");
         auto successBB =
            oldDest == cbi->getSuccessBB() ? newDest : cbi->getSuccessBB();
         auto failureBB =
            oldDest == cbi->getFailureBB() ? newDest : cbi->getFailureBB();
         builder.createCheckedCastValueBranch(
            cbi->getLoc(), cbi->getOperand(), cbi->getSourceFormalType(),
            cbi->getTargetLoweredType(), cbi->getTargetFormalType(),
            successBB, failureBB);
         cbi->eraseFromParent();
         return;
      }

      case TermKind::CheckedCastAddrBranchInst: {
         auto cbi = cast<CheckedCastAddrBranchInst>(t);
         assert((oldDest == cbi->getSuccessBB()
                         || oldDest == cbi->getFailureBB()) && "Invalid edge index");
         auto successBB =
            oldDest == cbi->getSuccessBB() ? newDest : cbi->getSuccessBB();
         auto failureBB =
            oldDest == cbi->getFailureBB() ? newDest : cbi->getFailureBB();
         auto trueCount = cbi->getTrueBBCount();
         auto falseCount = cbi->getFalseBBCount();
         builder.createCheckedCastAddrBranch(
            cbi->getLoc(), cbi->getConsumptionKind(),
            cbi->getSrc(), cbi->getSourceFormalType(),
            cbi->getDest(), cbi->getTargetFormalType(),
            successBB, failureBB, trueCount, falseCount);
         cbi->eraseFromParent();
         return;
      }

      case TermKind::ReturnInst:
      case TermKind::ThrowInst:
      case TermKind::TryApplyInst:
      case TermKind::UnreachableInst:
      case TermKind::UnwindInst:
      case TermKind::YieldInst:
         llvm_unreachable(
            "Branch target cannot be replaced for this terminator instruction!");
   }
   llvm_unreachable("Not yet implemented!");
}

/// Check if the edge from the terminator is critical.
bool polar::isCriticalEdge(TermInst *t, unsigned edgeIdx) {
   assert(t->getSuccessors().size() > edgeIdx && "Not enough successors");

   auto srcSuccs = t->getSuccessors();

   if (srcSuccs.size() <= 1 &&
       // Also consider non-branch instructions with a single successor for
       // critical edges, for example: a switch_enum of a single-case enum.
       (isa<BranchInst>(t) || isa<CondBranchInst>(t)))
      return false;

   PILBasicBlock *destBB = srcSuccs[edgeIdx];
   assert(!destBB->pred_empty() && "There should be a predecessor");
   if (destBB->getSinglePredecessorBlock())
      return false;

   return true;
}

/// Splits the basic block at the iterator with an unconditional branch and
/// updates the dominator tree and loop info.
PILBasicBlock *polar::splitBasicBlockAndBranch(PILBuilder &builder,
                                               PILInstruction *splitBeforeInst,
                                               DominanceInfo *domInfo,
                                               PILLoopInfo *loopInfo) {
   auto *origBB = splitBeforeInst->getParent();
   auto *newBB = origBB->split(splitBeforeInst->getIterator());
   builder.setInsertionPoint(origBB);
   builder.createBranch(splitBeforeInst->getLoc(), newBB);

   // Update the dominator tree.
   if (domInfo) {
      auto origBBDTNode = domInfo->getNode(origBB);
      if (origBBDTNode) {
         // Change the immediate dominators of the children of the block we
         // splitted to the splitted block.
         SmallVector<DominanceInfoNode *, 16> Adoptees(origBBDTNode->begin(),
                                                       origBBDTNode->end());

         auto newBBDTNode = domInfo->addNewBlock(newBB, origBB);
         for (auto *adoptee : Adoptees)
            domInfo->changeImmediateDominator(adoptee, newBBDTNode);
      }
   }

   // Update loop info.
   if (loopInfo)
      if (auto *origBBLoop = loopInfo->getLoopFor(origBB)) {
         origBBLoop->addBasicBlockToLoop(newBB, loopInfo->getBase());
      }

   return newBB;
}

/// Split every edge between two basic blocks.
void polar::splitEdgesFromTo(PILBasicBlock *From, PILBasicBlock *To,
                             DominanceInfo *domInfo, PILLoopInfo *loopInfo) {
   for (unsigned edgeIndex = 0, E = From->getSuccessors().size(); edgeIndex != E;
        ++edgeIndex) {
      PILBasicBlock *succBB = From->getSuccessors()[edgeIndex];
      if (succBB != To)
         continue;
      splitEdge(From->getTerminator(), edgeIndex, domInfo, loopInfo);
   }
}

/// Splits the n-th critical edge from the terminator and updates dominance and
/// loop info if set.
/// Returns the newly created basic block on success or nullptr otherwise (if
/// the edge was not critical.
PILBasicBlock *polar::splitCriticalEdge(TermInst *t, unsigned edgeIdx,
                                        DominanceInfo *domInfo,
                                        PILLoopInfo *loopInfo) {
   if (!isCriticalEdge(t, edgeIdx))
      return nullptr;

   return splitEdge(t, edgeIdx, domInfo, loopInfo);
}

bool polar::splitCriticalEdgesFrom(PILBasicBlock *fromBB,
                                   DominanceInfo *domInfo,
                                   PILLoopInfo *loopInfo) {
   bool changed = false;
   for (unsigned idx = 0, e = fromBB->getSuccessors().size(); idx != e; ++idx) {
      auto *newBB =
         splitCriticalEdge(fromBB->getTerminator(), idx, domInfo, loopInfo);
      changed |= (newBB != nullptr);
   }
   return changed;
}

bool polar::hasCriticalEdges(PILFunction &f, bool onlyNonCondBr) {
   for (PILBasicBlock &bb : f) {
      // Only consider critical edges for terminators that don't support block
      // arguments.
      if (onlyNonCondBr && isa<CondBranchInst>(bb.getTerminator()))
         continue;

      if (isa<BranchInst>(bb.getTerminator()))
         continue;

      for (unsigned idx = 0, e = bb.getSuccessors().size(); idx != e; ++idx)
         if (isCriticalEdge(bb.getTerminator(), idx))
            return true;
   }
   return false;
}

/// Split all critical edges in the function updating the dominator tree and
/// loop information (if they are not set to null).
bool polar::splitAllCriticalEdges(PILFunction &f, DominanceInfo *domInfo,
                                  PILLoopInfo *loopInfo) {
   bool changed = false;

   for (PILBasicBlock &bb : f) {
      if (isa<BranchInst>(bb.getTerminator()))
         continue;

      for (unsigned idx = 0, e = bb.getSuccessors().size(); idx != e; ++idx) {
         auto *newBB =
            splitCriticalEdge(bb.getTerminator(), idx, domInfo, loopInfo);
         assert((!newBB
                         || isa<CondBranchInst>(bb.getTerminator()))
                   && "Only cond_br may have a critical edge.");
         changed |= (newBB != nullptr);
      }
   }
   return changed;
}

/// Merge the basic block with its successor if possible. If dominance
/// information or loop info is non null update it. Return true if block was
/// merged.
bool polar::mergeBasicBlockWithSuccessor(PILBasicBlock *bb,
                                         DominanceInfo *domInfo,
                                         PILLoopInfo *loopInfo) {
   auto *branch = dyn_cast<BranchInst>(bb->getTerminator());
   if (!branch)
      return false;

   auto *succBB = branch->getDestBB();
   if (bb == succBB || !succBB->getSinglePredecessorBlock())
      return false;

   if (domInfo)
      if (auto *succBBNode = domInfo->getNode(succBB)) {
         // Change the immediate dominator for children of the successor to be the
         // current block.
         auto *bbNode = domInfo->getNode(bb);
         SmallVector<DominanceInfoNode *, 8> Children(succBBNode->begin(),
                                                      succBBNode->end());
         for (auto *ChildNode : Children)
            domInfo->changeImmediateDominator(ChildNode, bbNode);

         domInfo->eraseNode(succBB);
      }

   if (loopInfo)
      loopInfo->removeBlock(succBB);

   mergeBasicBlockWithSingleSuccessor(bb, succBB);

   return true;
}

bool polar::mergeBasicBlocks(PILFunction *f) {
   bool merged = false;
   for (auto bbIter = f->begin(); bbIter != f->end();) {
      if (mergeBasicBlockWithSuccessor(&*bbIter, /*domInfo*/ nullptr,
         /*loopInfo*/ nullptr)) {
         merged = true;
         // Continue to merge the current block without advancing.
         continue;
      }
      ++bbIter;
   }
   return merged;
}

/// Splits the critical edges between from and to. This code assumes there is
/// only one edge between the two basic blocks.
PILBasicBlock *polar::splitIfCriticalEdge(PILBasicBlock *from,
                                          PILBasicBlock *to,
                                          DominanceInfo *domInfo,
                                          PILLoopInfo *loopInfo) {
   auto *t = from->getTerminator();
   for (unsigned i = 0, e = t->getSuccessors().size(); i != e; ++i) {
      if (t->getSuccessors()[i] == to)
         return splitCriticalEdge(t, i, domInfo, loopInfo);
   }
   llvm_unreachable("Destination block not found");
}

bool polar::splitAllCondBrCriticalEdgesWithNonTrivialArgs(
   PILFunction &fn, DominanceInfo *domInfo, PILLoopInfo *loopInfo) {
   // Find our targets.
   llvm::SmallVector<std::pair<PILBasicBlock *, unsigned>, 8> targets;
   for (auto &block : fn) {
      auto *cbi = dyn_cast<CondBranchInst>(block.getTerminator());
      if (!cbi)
         continue;

      // See if our true index is a critical edge. If so, add block to the list
      // and continue. If the false edge is also critical, we will handle it at
      // the same time.
      if (isCriticalEdge(cbi, CondBranchInst::TrueIdx)) {
         targets.emplace_back(&block, CondBranchInst::TrueIdx);
      }

      if (!isCriticalEdge(cbi, CondBranchInst::FalseIdx)) {
         continue;
      }

      targets.emplace_back(&block, CondBranchInst::FalseIdx);
   }

   if (targets.empty())
      return false;

   for (auto p : targets) {
      PILBasicBlock *block = p.first;
      unsigned index = p.second;
      auto *result =
         splitCriticalEdge(block->getTerminator(), index, domInfo, loopInfo);
      (void)result;
      assert(result);
   }

   return true;
}

static bool isSafeNonExitTerminator(TermInst *ti) {
   switch (ti->getTermKind()) {
      case TermKind::BranchInst:
      case TermKind::CondBranchInst:
      case TermKind::SwitchValueInst:
      case TermKind::SwitchEnumInst:
      case TermKind::SwitchEnumAddrInst:
      case TermKind::DynamicMethodBranchInst:
      case TermKind::CheckedCastBranchInst:
      case TermKind::CheckedCastValueBranchInst:
      case TermKind::CheckedCastAddrBranchInst:
         return true;
      case TermKind::UnreachableInst:
      case TermKind::ReturnInst:
      case TermKind::ThrowInst:
      case TermKind::UnwindInst:
         return false;
         // yield is special because it can do arbitrary,
         // potentially-process-terminating things.
      case TermKind::YieldInst:
         return false;
      case TermKind::TryApplyInst:
         return true;
   }

   llvm_unreachable("Unhandled TermKind in switch.");
}

static bool isTrapNoReturnFunction(ApplyInst *ai) {
   const char *fatalName = MANGLE_AS_STRING(
      MANGLE_SYM(s18_fatalErrorMessageyys12StaticStringV_AcCSutF));
   auto *fn = ai->getReferencedFunctionOrNull();

   // We use endswith here since if we specialize fatal error we will always
   // prepend the specialization records to fatalName.
   if (!fn || !fn->getName().endswith(fatalName))
      return false;

   return true;
}

bool polar::findAllNonFailureExitBBs(
   PILFunction *f, llvm::TinyPtrVector<PILBasicBlock *> &bbs) {
   for (PILBasicBlock &bb : *f) {
      TermInst *ti = bb.getTerminator();

      // If we know that this terminator is not an exit terminator, continue.
      if (isSafeNonExitTerminator(ti))
         continue;

      // A return inst is always a non-failure exit bb.
      if (ti->isFunctionExiting()) {
         bbs.push_back(&bb);
         continue;
      }

      // If we don't have an unreachable inst at this point, this is a terminator
      // we don't understand. Be conservative and return false.
      if (!isa<UnreachableInst>(ti))
         return false;

      // Ok, at this point we know we have a terminator. If it is the only
      // instruction in our bb, it is a failure bb. continue...
      if (ti == &*bb.begin())
         continue;

      // If the unreachable is preceded by a no-return apply inst, then it is a
      // non-failure exit bb. Add it to our list and continue.
      auto prevIter = std::prev(PILBasicBlock::iterator(ti));
      if (auto *ai = dyn_cast<ApplyInst>(&*prevIter)) {
         if (ai->isCalleeNoReturn() && !isTrapNoReturnFunction(ai)) {
            bbs.push_back(&bb);
            continue;
         }
      }

      // Otherwise, it must be a failure bb where we leak, continue.
      continue;
   }

   // We understood all terminators, return true.
   return true;
}
