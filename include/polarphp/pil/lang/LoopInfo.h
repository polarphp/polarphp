//===--- LoopInfo.h - PIL Loop Analysis -------------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_LOOPINFO_H
#define POLARPHP_PIL_LOOPINFO_H

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/ADT/iterator_range.h"

namespace polar {
class DominanceInfo;
class PILLoop;
class PILPassManager;
class PILBasicBlock;
class PILInstruction;
class PILFunction;
}

// Implementation in LoopInfoImpl.h
#ifdef __GNUC__
__extension__ extern template
class llvm::LoopBase<polar::PILBasicBlock, polar::PILLoop>;
__extension__ extern template
class llvm::LoopInfoBase<polar::PILBasicBlock, polar::PILLoop>;
#endif

namespace polar {
/// Information about a single natural loop.
class PILLoop : public llvm::LoopBase<PILBasicBlock, PILLoop> {
public:
   PILLoop() {}
   void dump() const;

   llvm::iterator_range<iterator> getSubLoopRange() const {
      return make_range(begin(), end());
   }

   /// Check whether it is safe to duplicate this instruction when duplicating
   /// this loop by unrolling or versioning.
   bool canDuplicate(PILInstruction *Inst) const;

private:
   friend class llvm::LoopInfoBase<PILBasicBlock, PILLoop>;

   explicit PILLoop(PILBasicBlock *BB)
      : llvm::LoopBase<PILBasicBlock, PILLoop>(BB) {}
};

/// Information about loops in a function.
class PILLoopInfo {
   friend class llvm::LoopBase<PILBasicBlock, PILLoop>;
   using PILLoopInfoBase = llvm::LoopInfoBase<PILBasicBlock, PILLoop>;

   PILLoopInfoBase LI;
   DominanceInfo *Dominance;

   void operator=(const PILLoopInfo &) = delete;
   PILLoopInfo(const PILLoopInfo &) = delete;

public:
   PILLoopInfo(PILFunction *F, DominanceInfo *DT);

   PILLoopInfoBase &getBase() { return LI; }

   /// Verify loop information. This is very expensive.
   void verify() const;

   /// The iterator interface to the top-level loops in the current
   /// function.
   using iterator = PILLoopInfoBase::iterator;
   iterator begin() const { return LI.begin(); }
   iterator end() const { return LI.end(); }
   bool empty() const { return LI.empty(); }
   llvm::iterator_range<iterator> getTopLevelLoops() const {
      return make_range(begin(), end());
   }

   /// Return the inner most loop that BB lives in.  If a basic block is in no
   /// loop (for example the entry node), null is returned.
   PILLoop *getLoopFor(const PILBasicBlock *BB) const {
      return LI.getLoopFor(BB);
   }

   /// Return the inner most loop that BB lives in.  If a basic block is in no
   /// loop (for example the entry node), null is returned.
   const PILLoop *operator[](const PILBasicBlock *BB) const {
      return LI.getLoopFor(BB);
   }

   /// Return the loop nesting level of the specified block.
   unsigned getLoopDepth(const PILBasicBlock *BB) const {
      return LI.getLoopDepth(BB);
   }

   /// True if the block is a loop header node.
   bool isLoopHeader(PILBasicBlock *BB) const {
      return LI.isLoopHeader(BB);
   }

   /// This removes the specified top-level loop from this loop info object. The
   /// loop is not deleted, as it will presumably be inserted into another loop.
   PILLoop *removeLoop(iterator I) { return LI.removeLoop(I); }

   /// Change the top-level loop that contains BB to the specified loop.  This
   /// should be used by transformations that restructure the loop hierarchy
   /// tree.
   void changeLoopFor(PILBasicBlock *BB, PILLoop *L) { LI.changeLoopFor(BB, L); }

   /// Replace the specified loop in the top-level loops list with the indicated
   /// loop.
   void changeTopLevelLoop(PILLoop *OldLoop, PILLoop *NewLoop) {
      LI.changeTopLevelLoop(OldLoop, NewLoop);
   }

   ///  This adds the specified loop to the collection of top-level loops.
   void addTopLevelLoop(PILLoop *New) {
      LI.addTopLevelLoop(New);
   }

   /// This method completely removes BB from all data structures, including all
   /// of the Loop objects it is nested in and our mapping from PILBasicBlocks to
   /// loops.
   void removeBlock(PILBasicBlock *BB) {
      LI.removeBlock(BB);
   }
};

} // end namespace polar

#endif
