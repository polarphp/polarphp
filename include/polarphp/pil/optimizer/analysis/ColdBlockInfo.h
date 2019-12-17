//===--- ColdBlockInfo.h - Fast/slow path analysis for PIL CFG --*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_ANALYSIS_COLDBLOCKS_H
#define POLARPHP_PIL_OPTIMIZER_ANALYSIS_COLDBLOCKS_H

#include "llvm/ADT/DenseMap.h"
#include "polarphp/pil/lang/PILValue.h"

namespace polar {
class DominanceAnalysis;
class PILBasicBlock;

/// Cache a set of basic blocks that have been determined to be cold or hot.
///
/// This does not inherit from PILAnalysis because it is not worth preserving
/// across passes.
class ColdBlockInfo {
  DominanceAnalysis *DA;

  /// Each block in this map has been determined to be either cold or hot.
  llvm::DenseMap<const PILBasicBlock*, bool> ColdBlockMap;

  // This is a cache and shouldn't be copied around.
  ColdBlockInfo(const ColdBlockInfo &) = delete;
  ColdBlockInfo &operator=(const ColdBlockInfo &) = delete;

  /// Tri-value return code for checking branch hints.
  enum BranchHint : unsigned {
    None,
    LikelyTrue,
    LikelyFalse
  };

  enum {
    RecursionDepthLimit = 3
  };

  BranchHint getBranchHint(PILValue Cond, int recursionDepth);

  bool isSlowPath(const PILBasicBlock *FromBB, const PILBasicBlock *ToBB,
                  int recursionDepth);

  bool isCold(const PILBasicBlock *BB,
              int recursionDepth);

public:
  ColdBlockInfo(DominanceAnalysis *DA): DA(DA) {}

  bool isSlowPath(const PILBasicBlock *FromBB, const PILBasicBlock *ToBB) {
    return isSlowPath(FromBB, ToBB, 0);
  }

  bool isCold(const PILBasicBlock *BB) { return isCold(BB, 0); }
};

} // end namespace polar

#endif // POLARPHP_PIL_OPTIMIZER_ANALYSIS_COLDBLOCKS_H
