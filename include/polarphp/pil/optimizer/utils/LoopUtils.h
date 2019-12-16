//===--- LoopUtils.h --------------------------------------------*- C++ -*-===//
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
/// This header file declares utility functions for simplifying and
/// canonicalizing loops.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_OPTIMIZER_UTILS_LOOPUTILS_H
#define POLARPHP_PIL_OPTIMIZER_UTILS_LOOPUTILS_H

#include "llvm/ADT/SmallVector.h"

namespace polar {

class PILFunction;
class PILBasicBlock;
class PILLoop;
class DominanceInfo;
class PILLoopInfo;

/// Canonicalize the loop for rotation and downstream passes.
///
/// Create a single preheader and single latch block.
bool canonicalizeLoop(PILLoop *L, DominanceInfo *DT, PILLoopInfo *LI);

/// Canonicalize all loops in the function F for which \p LI contains loop
/// information. We update loop info and dominance info while we do this.
bool canonicalizeAllLoops(DominanceInfo *DT, PILLoopInfo *LI);

/// A visitor that visits loops in a function in a bottom up order. It only
/// performs the visit.
class PILLoopVisitor {
  PILFunction *F;
  PILLoopInfo *LI;

public:
  PILLoopVisitor(PILFunction *Func, PILLoopInfo *LInfo) : F(Func), LI(LInfo) {}
  virtual ~PILLoopVisitor() {}

  void run();

  PILFunction *getFunction() const { return F; }

  virtual void runOnLoop(PILLoop *L) = 0;
  virtual void runOnFunction(PILFunction *F) = 0;
};

/// A group of sil loop visitors, run in sequence on a function.
class PILLoopVisitorGroup : public PILLoopVisitor {
  /// The list of visitors to run.
  ///
  /// This is set to 3, since currently the only place this is used will have at
  /// most 3 such visitors.
  llvm::SmallVector<PILLoopVisitor *, 3> Visitors;

public:
  PILLoopVisitorGroup(PILFunction *Func, PILLoopInfo *LInfo)
      : PILLoopVisitor(Func, LInfo) {}
  virtual ~PILLoopVisitorGroup() {}

  void addVisitor(PILLoopVisitor *V) {
    Visitors.push_back(V);
  }

  void runOnLoop(PILLoop *L) override {
    for (auto *V : Visitors) {
      V->runOnLoop(L);
    }
  }

  void runOnFunction(PILFunction *F) override {
    for (auto *V : Visitors) {
      V->runOnFunction(F);
    }
  }
};

} // end polar namespace

#endif // POLARPHP_PIL_OPTIMIZER_UTILS_LOOPUTILS_H
