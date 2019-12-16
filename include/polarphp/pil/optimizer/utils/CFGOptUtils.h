//===--- CFGOptUtils.h - PIL CFG edge utilities -----------------*- C++ -*-===//
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
///
/// APIs used by the PILOptimizer for low-level branch and CFG edge analysis
/// and operations. These may merge blocks, split blocks, or create empty
/// blocks, but don't duplicate whole blocks.
///
/// Essential CFG utilities are in PIL/CFG.h.
///
/// Whole block-level transformations are in BasicBlockOptUtils.h.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_OPTIMIZER_UTILS_CFGOPTUTILS_H
#define POLARPHP_PIL_OPTIMIZER_UTILS_CFGOPTUTILS_H

#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/pil/lang/PILInstruction.h"

namespace llvm {
template <typename T> class TinyPtrVector;
}

namespace polar {

class DominanceInfo;
class PILLoop;
class PILLoopInfo;

/// Adds a new argument to an edge between a branch and a destination
/// block.
///
/// \param branch The terminator to add the argument to.
/// \param dest The destination block of the edge.
/// \param val The value to the arguments of the branch.
/// \return The created branch. The old branch is deleted.
/// The argument is appended at the end of the argument tuple.
TermInst *addNewEdgeValueToBranch(TermInst *branch, PILBasicBlock *dest,
                                  PILValue val);

/// Changes the edge value between a branch and destination basic block
/// at the specified index. Changes all edges from \p Branch to \p Dest to carry
/// the value.
///
/// \param branch The branch to modify.
/// \param dest The destination of the edge.
/// \param idx The index of the argument to modify.
/// \param val The new value to use.
/// \return The new branch. Deletes the old one.
TermInst *changeEdgeValue(TermInst *branch, PILBasicBlock *dest, size_t idx,
                          PILValue val);

/// Deletes the edge value between a branch and a destination basic block at the
/// specified index. Asserts internally that the argument along the edge does
/// not have uses.
TermInst *deleteEdgeValue(TermInst *branch, PILBasicBlock *destBlock,
                          size_t argIndex);

/// Erase the \p argIndex phi argument from \p block. Asserts that the argument
/// is a /real/ phi argument. Removes all incoming values for the argument from
/// predecessor terminators. Asserts internally that it only ever is given
/// "true" phi argument.
void erasePhiArgument(PILBasicBlock *block, unsigned argIndex);

/// Replace a branch target.
///
/// \param t The terminating instruction to modify.
/// \param oldDest The successor block that will be replaced.
/// \param newDest The new target block.
/// \param preserveArgs If set, preserve arguments on the replaced edge.
void replaceBranchTarget(TermInst *t, PILBasicBlock *oldDest,
                         PILBasicBlock *newDest, bool preserveArgs);

/// Check if the edge from the terminator is critical.
bool isCriticalEdge(TermInst *t, unsigned edgeIdx);

/// Splits the edge from terminator if it is critical.
///
/// Updates dominance information and loop information if not null.
/// Returns the newly created basic block on success or nullptr otherwise (if
/// the edge was not critical).
PILBasicBlock *splitCriticalEdge(TermInst *, unsigned edgeIdx,
                                 DominanceInfo *domInfo = nullptr,
                                 PILLoopInfo *loopInfo = nullptr);

/// Splits the critical edges between from and to. This code assumes there is
/// exactly one edge between the two basic blocks. It will return the wrong
/// result if there are multiple edges and will assert if there are no edges in
/// between the two blocks.
///
/// Updates dominance information and loop information if not null.
PILBasicBlock *splitIfCriticalEdge(PILBasicBlock *from, PILBasicBlock *to,
                                   DominanceInfo *domInfo = nullptr,
                                   PILLoopInfo *loopInfo = nullptr);

/// Splits all critical edges originating from `fromBB`.
bool splitCriticalEdgesFrom(PILBasicBlock *fromBB,
                            DominanceInfo *domInfo = nullptr,
                            PILLoopInfo *loopInfo = nullptr);

/// Splits the edges between two basic blocks.
///
/// Updates dominance information and loop information if not null.
void splitEdgesFromTo(PILBasicBlock *from, PILBasicBlock *to,
                      DominanceInfo *domInfo = nullptr,
                      PILLoopInfo *loopInfo = nullptr);

/// Splits the basic block before the instruction with an unconditional branch
/// and updates the dominator tree and loop info. Returns the new, branched to
/// block that contains the end of \p SplitBeforeInst's block.
PILBasicBlock *splitBasicBlockAndBranch(PILBuilder &builder,
                                        PILInstruction *splitBeforeInst,
                                        DominanceInfo *domInfo,
                                        PILLoopInfo *loopInfo);

/// Return true if the function has a critical edge, false otherwise.
bool hasCriticalEdges(PILFunction &f, bool onlyNonCondBr);

/// Split all critical edges in the given function, updating the
/// dominator tree and loop information if they are provided.
///
/// FIXME: This should never be called! Fix passes that create critical edges.
bool splitAllCriticalEdges(PILFunction &F, DominanceInfo *domInfo,
                           PILLoopInfo *loopInfo);

/// Split all cond_br critical edges with non-trivial arguments in the
/// function updating the dominator tree and loop information (if they are not
/// set to null).
///
/// A current invariant of Ownership PIL is that cond_br can only have critical
/// edges with non-trivial arguments. This simplifies computation.
bool splitAllCondBrCriticalEdgesWithNonTrivialArgs(PILFunction &fn,
                                                   DominanceInfo *domInfo,
                                                   PILLoopInfo *loopInfo);

/// Merge a basic block ending in a branch with its successor
/// if possible. If dominance information or loop info is non null update it.
/// Return true if block was merged.
bool mergeBasicBlockWithSuccessor(PILBasicBlock *bb, DominanceInfo *domInfo,
                                  PILLoopInfo *loopInfo);

/// Merge basic blocks in the given function by eliminating all unconditional
/// branches to single-predecessor branch targets.
///
/// During optimization, SimplifyCFG also handles this, but this is a basic
/// canonicalization after any pass that splits blocks, such as inlining. This
/// is not done on-the-fly after splitting blocks because merging is linear in
/// the number of instructions, so interleaved merging and splitting is
/// quadratic.
bool mergeBasicBlocks(PILFunction *f);

/// Return true if we conservatively find all bb's that are non-failure exit
/// basic blocks and place them in \p bbs. If we find something we don't
/// understand, bail.
///
/// A non-failure exit BB is defined as a BB that:
///
/// 1. Has a return terminator.
/// 2. unreachable + noreturn terminator sequence.
/// 3. has a throw terminator.
///
/// If we just have an unreachable without a noreturn call before it, we must
/// have a failure BB.
///
/// We use a TinyPtrVector since in most cases this will only return one
/// PILBasicBlock since non-failure noreturn functions should not occur often
/// implying in most cases this will be one element.
///
/// TODO:
bool findAllNonFailureExitBBs(PILFunction *f,
                              llvm::TinyPtrVector<PILBasicBlock *> &bbs);

} // end namespace polar

#endif // POLARPHP_PIL_OPTIMIZER_UTILS_CFGOPTUTILS_H
