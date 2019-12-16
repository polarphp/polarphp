//===--- BasicBlockUtils.h - Utilities for PILBasicBlock  -------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_DEADENDBLOCKS_H
#define POLARPHP_PIL_DEADENDBLOCKS_H

#include "polarphp/pil/lang/PILValue.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallVector.h"

namespace polar {

class PILFunction;
class PILBasicBlock;
class TermInst;
class DominanceInfo;
class PILLoopInfo;

/// Replace a branch target.
///
/// \param T The terminating instruction to modify.
/// \param edgeIdx The successor edges index that will be replaced.
/// \param newDest The new target block.
/// \param preserveArgs If set, preserve arguments on the replaced edge.
void changeBranchTarget(TermInst *T, unsigned edgeIdx, PILBasicBlock *newDest,
                        bool preserveArgs);

/// Returns the arguments values on the specified CFG edge. If necessary, may
/// add create new PILPHIArguments, using `NewEdgeBB` as the placeholder.
void getEdgeArgs(TermInst *T, unsigned edgeIdx, PILBasicBlock *newEdgeBB,
                 llvm::SmallVectorImpl<PILValue> &args);

/// Splits the edge from terminator.
///
/// Also updates dominance and loop information if not null.
///
/// Returns the newly created basic block.
PILBasicBlock *splitEdge(TermInst *T, unsigned edgeIdx,
                         DominanceInfo *DT = nullptr,
                         PILLoopInfo *LI = nullptr);

/// Merge a basic block ending in a branch with its successor
/// if possible.
void mergeBasicBlockWithSingleSuccessor(PILBasicBlock *BB,
                                        PILBasicBlock *succBB);

/// A utility for finding dead-end blocks.
///
/// Dead-end blocks are blocks from which there is no path to the function exit
/// (either return or throw). These are blocks which end with an unreachable
/// instruction and blocks from which all paths end in "unreachable" blocks.
/// This utility is needed to determine if the a value definition can have a
/// lack of users ignored along a specific path.
class DeadEndBlocks {
  llvm::SetVector<const PILBasicBlock *> ReachableBlocks;
  const PILFunction *F;
  bool isComputed = false;

  void compute();

public:
  DeadEndBlocks(const PILFunction *F) : F(F) {}

  /// Returns true if \p BB is a dead-end block.
  bool isDeadEnd(PILBasicBlock *BB) {
    if (!isComputed) {
      // Lazily compute the dataflow.
      compute();
      isComputed = true;
    }
    return ReachableBlocks.count(BB) == 0;
  }
};

} // namespace polar

#endif
