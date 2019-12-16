//===--- Dominance.h - PIL dominance analysis -------------------*- C++ -*-===//
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
// This file provides interfaces for computing and working with
// control-flow dominance in PIL.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_DOMINANCE_H
#define POLARPHP_PIL_DOMINANCE_H

#include "llvm/Support/GenericDomTree.h"
#include "polarphp/pil/lang/PILBasicBlockCFG.h"
#include "polarphp/pil/lang/PILFunction.h"

extern template class llvm::DominatorTreeBase<polar::PILBasicBlock, false>;
extern template class llvm::DominatorTreeBase<polar::PILBasicBlock, true>;
extern template class llvm::DomTreeNodeBase<polar::PILBasicBlock>;

namespace llvm {
namespace DomTreeBuilder {
using PILDomTree = llvm::DomTreeBase<polar::PILBasicBlock>;
using PILPostDomTree = llvm::PostDomTreeBase<polar::PILBasicBlock>;

extern template void Calculate<PILDomTree>(PILDomTree &DT);
extern template void Calculate<PILPostDomTree>(PILPostDomTree &DT);
} // namespace DomTreeBuilder
} // namespace llvm

namespace polar {

using DominatorTreeBase = llvm::DominatorTreeBase<polar::PILBasicBlock, false>;
using PostDominatorTreeBase = llvm::DominatorTreeBase<polar::PILBasicBlock, true>;
using DominanceInfoNode = llvm::DomTreeNodeBase<PILBasicBlock>;

/// A class for computing basic dominance information.
class DominanceInfo : public DominatorTreeBase {
   using super = DominatorTreeBase;
public:
   DominanceInfo(PILFunction *F);

   /// Does instruction A properly dominate instruction B?
   bool properlyDominates(PILInstruction *a, PILInstruction *b);

   /// Does instruction A dominate instruction B?
   bool dominates(PILInstruction *a, PILInstruction *b) {
      return a == b || properlyDominates(a, b);
   }

   /// Does value A properly dominate instruction B?
   bool properlyDominates(PILValue a, PILInstruction *b);

   void verify() const;

   /// Return true if the other dominator tree does not match this dominator
   /// tree.
   inline bool errorOccurredOnComparison(const DominanceInfo &Other) const {
      const auto *R = getRootNode();
      const auto *OtherR = Other.getRootNode();

      if (!R || !OtherR || R->getBlock() != OtherR->getBlock())
         return true;

      // Returns *false* if they match.
      if (compare(Other))
         return true;

      return false;
   }

   using DominatorTreeBase::properlyDominates;
   using DominatorTreeBase::dominates;

   bool isValid(PILFunction *F) const {
//      return getNode(&F->front()) != nullptr;
   }
   void reset() {
      super::reset();
   }
};

/// Helper class for visiting basic blocks in dominance order, based on a
/// worklist algorithm. Example usage:
/// \code
///   DominanceOrder DomOrder(Function->front(), DominanceInfo);
///   while (PILBasicBlock *block = DomOrder.getNext()) {
///     doSomething(block);
///     domOrder.pushChildren(block);
///   }
/// \endcode
class DominanceOrder {

   SmallVector<PILBasicBlock *, 16> buffer;
   DominanceInfo *DT;
   size_t srcIdx = 0;

public:

   /// Constructor.
   /// \p entry The root of the dominator (sub-)tree.
   /// \p DT The dominance info of the function.
   /// \p capacity Should be the number of basic blocks in the dominator tree to
   ///             reduce memory allocation.
   DominanceOrder(PILBasicBlock *root, DominanceInfo *DT, int capacity = 0) :
      DT(DT) {
      buffer.reserve(capacity);
      buffer.push_back(root);
   }

   /// Gets the next block from the worklist.
   ///
   PILBasicBlock *getNext() {
      if (srcIdx == buffer.size())
         return nullptr;
      return buffer[srcIdx++];
   }

   /// Pushes the dominator children of a block onto the worklist.
   void pushChildren(PILBasicBlock *block) {
      pushChildrenIf(block, [] (PILBasicBlock *) { return true; });
   }

   /// Conditionally pushes the dominator children of a block onto the worklist.
   /// \p pred Takes a block (= a dominator child) as argument and returns true
   ///         if it should be added to the worklist.
   ///
   template <typename Pred> void pushChildrenIf(PILBasicBlock *block, Pred pred) {
      DominanceInfoNode *DINode = DT->getNode(block);
      for (auto *DIChild : *DINode) {
         PILBasicBlock *child = DIChild->getBlock();
         if (pred(child))
            buffer.push_back(DIChild->getBlock());
      }
   }
};

/// A class for computing basic post-dominance information.
class PostDominanceInfo : public PostDominatorTreeBase {
   using super = PostDominatorTreeBase;
public:
   PostDominanceInfo(PILFunction *F);

   bool properlyDominates(PILInstruction *A, PILInstruction *B);

   void verify() const;

   /// Return true if the other dominator tree does not match this dominator
   /// tree.
   inline bool errorOccurredOnComparison(const PostDominanceInfo &Other) const {
      const auto *R = getRootNode();
      const auto *OtherR = Other.getRootNode();

      if (!R || !OtherR || R->getBlock() != OtherR->getBlock())
         return true;

      if (!R->getBlock()) {
         // The post dom-tree has multiple roots. The compare() function can not
         // cope with multiple roots if at least one of the roots is caused by
         // an infinite loop in the CFG (it crashes because no nodes are allocated
         // for the blocks in the infinite loop).
         // So we return a conservative false in this case.
         // TODO: eventually fix the DominatorTreeBase::compare() function.
         return false;
      }

      // Returns *false* if they match.
      if (compare(Other))
         return true;

      return false;
   }

   bool isValid(PILFunction *F) const { return getNode(&F->front()) != nullptr; }

   using super::properlyDominates;
};

} // end namespace polar

namespace llvm {

/// DominatorTree GraphTraits specialization so the DominatorTree can be
/// iterable by generic graph iterators.
template <> struct GraphTraits<polar::DominanceInfoNode *> {
   using ChildIteratorType = polar::DominanceInfoNode::iterator;
   using NodeRef = polar::DominanceInfoNode *;

   static NodeRef getEntryNode(NodeRef N) { return N; }
   static inline ChildIteratorType child_begin(NodeRef N) { return N->begin(); }
   static inline ChildIteratorType child_end(NodeRef N) { return N->end(); }
};

template <> struct GraphTraits<const polar::DominanceInfoNode *> {
   using ChildIteratorType = polar::DominanceInfoNode::const_iterator;
   using NodeRef = const polar::DominanceInfoNode *;

   static NodeRef getEntryNode(NodeRef N) { return N; }
   static inline ChildIteratorType child_begin(NodeRef N) { return N->begin(); }
   static inline ChildIteratorType child_end(NodeRef N) { return N->end(); }
};

} // end namespace llvm
#endif
