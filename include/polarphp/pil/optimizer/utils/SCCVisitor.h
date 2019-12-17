//===--- SCCVisitor.h - PIL SCC Visitor -------------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_UTILS_SCCVISITOR_H
#define POLARPHP_PIL_OPTIMIZER_UTILS_SCCVISITOR_H

#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "polarphp/pil/lang/PILBasicBlockCFG.h"
#include "polarphp/pil/lang/PILFunctionCFG.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILBasicBlock.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILValue.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallVector.h"
#include <algorithm>
#include <tuple>

namespace polar {

/// A visitor class for visiting the instructions and basic block
/// arguments of a PIL function one strongly connected component at a
/// time in reverse post-order.
///
/// Inherit from this in the usual CRTP fashion and define a visit()
/// function. This function will get called with a vector of
/// pointer-to-ValueBase which are the elements of the SCC.
template <typename ImplClass>
class SCCVisitor {
public:
   SCCVisitor(PILFunction &F) : F(F) {}
   ~SCCVisitor() {
      cleanup();
   }
   ImplClass &asImpl() {  return static_cast<ImplClass &>(*this); }

   void visit(llvm::SmallVectorImpl<PILNode *> &SCC) { }

   void run() {
      llvm::ReversePostOrderTraversal<PILFunction *> RPOT(&F);

      for (auto Iter = RPOT.begin(), E = RPOT.end(); Iter != E; ++Iter) {
         auto *BB = *Iter;
         for (auto &I : *BB)
            maybeDFS(&I);
      }

      cleanup();
   }

private:
   struct DFSInfo {
      PILNode *Node;
      int DFSNum;
      int LowNum;

      DFSInfo(PILNode *node, int num) : Node(node), DFSNum(num), LowNum(num) {}
   };

   PILFunction &F;
   int CurrentNum = 0;

   llvm::DenseSet<PILNode *> Visited;
   llvm::SetVector<PILNode *> DFSStack;
   typedef llvm::DenseMap<PILNode *, std::unique_ptr<DFSInfo>> ValueInfoMapType;
   ValueInfoMapType ValueInfoMap;

   void cleanup() {
      Visited.clear();
      DFSStack.clear();
      ValueInfoMap.clear();
      CurrentNum = 0;
   }

   DFSInfo &addDFSInfo(PILNode *node) {
      assert(node->isRepresentativePILNodeInObject());

      auto insertion = ValueInfoMap.try_emplace(node,
                                                new DFSInfo(node, CurrentNum++));
      assert(insertion.second && "Cannot add DFS info more than once!");
      return *insertion.first->second;
   }

   DFSInfo &getDFSInfo(PILNode *node) {
      assert(node->isRepresentativePILNodeInObject());
      auto it = ValueInfoMap.find(node);
      assert(it != ValueInfoMap.end() &&
             "Expected to find value in DFS info map!");

      return *it->second;
   }

   void getArgsForTerminator(TermInst *Term, PILBasicBlock *SuccBB, int Index,
                             llvm::SmallVectorImpl<PILValue> &Operands) {
      switch (Term->getTermKind()) {
         case TermKind::BranchInst:
            return Operands.push_back(cast<BranchInst>(Term)->getArg(Index));

         case TermKind::CondBranchInst: {
            auto *CBI = cast<CondBranchInst>(Term);
            if (SuccBB == CBI->getTrueBB())
               return Operands.push_back(CBI->getTrueArgs()[Index]);
            assert(SuccBB == CBI->getFalseBB() &&
                   "Block is not a successor of terminator!");
            Operands.push_back(CBI->getFalseArgs()[Index]);
            return;
         }

         case TermKind::SwitchEnumInst:
         case TermKind::SwitchEnumAddrInst:
         case TermKind::CheckedCastBranchInst:
         case TermKind::CheckedCastValueBranchInst:
         case TermKind::CheckedCastAddrBranchInst:
         case TermKind::DynamicMethodBranchInst:
            assert(Index == 0 && "Expected argument index to always be zero!");
            return Operands.push_back(Term->getOperand(0));

         case TermKind::UnreachableInst:
         case TermKind::ReturnInst:
         case TermKind::SwitchValueInst:
         case TermKind::ThrowInst:
         case TermKind::UnwindInst:
            llvm_unreachable("Did not expect terminator that does not have args!");

         case TermKind::YieldInst:
            for (auto &O : cast<YieldInst>(Term)->getAllOperands())
               Operands.push_back(O.get());
            return;

         case TermKind::TryApplyInst:
            for (auto &O : cast<TryApplyInst>(Term)->getAllOperands())
               Operands.push_back(O.get());
            return;
      }
   }

   void collectOperandsForUser(PILNode *node,
                               llvm::SmallVectorImpl<PILValue> &Operands) {
      if (auto *I = dyn_cast<PILInstruction>(node)) {
         for (auto &O : I->getAllOperands())
            Operands.push_back(O.get());
         return;
      }

      if (auto *A = dyn_cast<PILArgument>(node)) {
         auto *BB = A->getParent();
         auto Index = A->getIndex();

         for (auto *Pred : BB->getPredecessorBlocks())
            getArgsForTerminator(Pred->getTerminator(), BB, Index, Operands);
         return;
      }
   }

   void maybeDFS(PILInstruction *inst) {
      (void) maybeDFSCanonicalNode(inst->getRepresentativePILNodeInObject());
   }

   /// Continue a DFS from the given node, finding the strongly
   /// component that User is a part of, calling visit() with that SCC,
   /// and returning the DFSInfo for the node.
   /// But if we've already visited the node, just return null.
   DFSInfo *maybeDFSCanonicalNode(PILNode *node) {
      assert(node->isRepresentativePILNodeInObject() &&
             "should already be canonical");

      if (!Visited.insert(node).second)
         return nullptr;

      DFSStack.insert(node);

      auto &nodeInfo = addDFSInfo(node);

      llvm::SmallVector<PILValue, 4> operands;
      collectOperandsForUser(node, operands);

      // Visit each unvisited operand, updating the lowest DFS number we've seen
      // reachable in User's SCC.
      for (PILValue operandValue : operands) {
         PILNode *operandNode = operandValue->getRepresentativePILNodeInObject();
         if (auto operandNodeInfo = maybeDFSCanonicalNode(operandNode)) {
            nodeInfo.LowNum = std::min(nodeInfo.LowNum, operandNodeInfo->LowNum);
         } else if (DFSStack.count(operandNode)) {
            auto operandNodeInfo = &getDFSInfo(operandNode);
            nodeInfo.LowNum = std::min(nodeInfo.LowNum, operandNodeInfo->DFSNum);
         }
      }

      // If User is the head of its own SCC, pop that SCC off the DFS stack.
      if (nodeInfo.DFSNum == nodeInfo.LowNum) {
         llvm::SmallVector<PILNode *, 4> SCC;
         PILNode *poppedNode;
         do {
            poppedNode = DFSStack.pop_back_val();
            SCC.push_back(poppedNode);
         } while (poppedNode != node);

         asImpl().visit(SCC);
      }

      return &nodeInfo;
   }
};

} // polar

#endif // POLARPHP_PIL_OPTIMIZER_UTILS_SCCVISITOR_H
