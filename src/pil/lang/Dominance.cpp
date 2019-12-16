//===--- Dominance.cpp - PIL basic block dominance analysis ---------------===//
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

#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILBasicBlock.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/Dominance.h"
#include "polarphp/pil/lang/PILFunctionCFG.h"
#include "llvm/Support/GenericDomTreeConstruction.h"

using namespace polar;

template class llvm::DominatorTreeBase<PILBasicBlock, false>;
template class llvm::DominatorTreeBase<PILBasicBlock, true>;
template class llvm::DomTreeNodeBase<PILBasicBlock>;

namespace llvm {
namespace DomTreeBuilder {
template void Calculate<PILDomTree>(PILDomTree &DT);
template void Calculate<PILPostDomTree>(PILPostDomTree &DT);
} // namespace DomTreeBuilder
} // namespace llvm

/// Compute the immediate-dominators map.
DominanceInfo::DominanceInfo(PILFunction *F)
   : DominatorTreeBase() {
   assert(!F->isExternalDeclaration() &&
          "Make sure the function is a definition and not a declaration.");
   recalculate(*F);
}

bool DominanceInfo::properlyDominates(PILInstruction *a, PILInstruction *b) {
   auto aBlock = a->getParent(), bBlock = b->getParent();

   // If the blocks are different, it's as easy as whether A's block
   // dominates B's block.
   if (aBlock != bBlock)
      return properlyDominates(a->getParent(), b->getParent());

   // Otherwise, they're in the same block, and we just need to check
   // whether B comes after A.  This is a non-strict computation.
   auto aIter = a->getIterator();
   auto bIter = b->getIterator();
   auto fIter = aBlock->begin();
   while (bIter != fIter) {
      --bIter;
      if (aIter == bIter)
         return true;
   }

   return false;
}

/// Does value A properly dominate instruction B?
bool DominanceInfo::properlyDominates(PILValue a, PILInstruction *b) {
   if (auto *Inst = a->getDefiningInstruction()) {
      return properlyDominates(Inst, b);
   }
   if (auto *Arg = dyn_cast<PILArgument>(a)) {
      return dominates(Arg->getParent(), b->getParent());
   }
   return false;
}

void DominanceInfo::verify() const {
   // Recompute.
   auto *F = getRoot()->getParent();
   DominanceInfo OtherDT(F);

   // And compare.
   if (errorOccurredOnComparison(OtherDT)) {
      llvm::errs() << "DominatorTree is not up to date!\nComputed:\n";
      print(llvm::errs());
      llvm::errs() << "\nActual:\n";
      OtherDT.print(llvm::errs());
      abort();
   }
}

/// Compute the immediate-post-dominators map.
PostDominanceInfo::PostDominanceInfo(PILFunction *F)
   : PostDominatorTreeBase() {
   assert(!F->isExternalDeclaration() &&
          "Cannot construct a post dominator tree for a declaration");
   recalculate(*F);
}

bool
PostDominanceInfo::
properlyDominates(PILInstruction *I1, PILInstruction *I2) {
   PILBasicBlock *BB1 = I1->getParent(), *BB2 = I2->getParent();

   // If the blocks are different, it's as easy as whether BB1 post dominates
   // BB2.
   if (BB1 != BB2)
      return properlyDominates(BB1, BB2);

   // Otherwise, they're in the same block, and we just need to check
   // whether A comes after B.
   for (auto II = I1->getIterator(), IE = BB1->end(); II != IE; ++II) {
      if (&*II == I2) {
         return false;
      }
   }

   return true;
}

void PostDominanceInfo::verify() const {
   // Recompute.
   //
   // Even though at the PIL level we have "one" return function, we can have
   // multiple exits provided by no-return functions.
   auto *F = getRoots()[0]->getParent();
   PostDominanceInfo OtherDT(F);

   // And compare.
   if (errorOccurredOnComparison(OtherDT)) {
      llvm::errs() << "PostDominatorTree is not up to date!\nComputed:\n";
      print(llvm::errs());
      llvm::errs() << "\nActual:\n";
      OtherDT.print(llvm::errs());
      abort();
   }
}
