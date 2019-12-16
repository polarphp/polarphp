//===--- PILProfiler.h - Instrumentation based profiling ===-----*- C++ -*-===//
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
// This file defines PILProfiler, which contains the profiling state for one
// function. It's used to drive PGO and generate code coverage reports.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PROFILER_H
#define POLARPHP_PIL_PROFILER_H

#include "polarphp/ast/AstNode.h"
#include "polarphp/basic/ProfileCounter.h"
#include "polarphp/pil/lang/PILAllocated.h"
#include "polarphp/pil/lang/PILDeclRef.h"
#include "llvm/ADT/DenseMap.h"

namespace polar {

class AbstractFunctionDecl;
class PILCoverageMap;
class PILFunction;
class PILModule;

/// Returns whether the given AST node requires profiling instrumentation.
bool doesAstRequireProfiling(PILModule &M, AstNode N);

/// PILProfiler - Maps AST nodes to profile counters.
class PILProfiler : public PILAllocated<PILProfiler> {
private:
  PILModule &M;

  AstNode Root;

  PILDeclRef forDecl;

  bool EmitCoverageMapping;

  PILCoverageMap *CovMap = nullptr;

  StringRef CurrentFileName;

  std::string PGOFuncName;

  uint64_t PGOFuncHash = 0;

  unsigned NumRegionCounters = 0;

  llvm::DenseMap<AstNode, unsigned> RegionCounterMap;

  llvm::DenseMap<AstNode, ProfileCounter> RegionLoadedCounterMap;

  llvm::DenseMap<AstNode, AstNode> RegionCondToParentMap;

  std::vector<std::tuple<std::string, uint64_t, std::string>> CoverageData;

  PILProfiler(PILModule &M, AstNode Root, PILDeclRef forDecl,
              bool EmitCoverageMapping)
      : M(M), Root(Root), forDecl(forDecl),
        EmitCoverageMapping(EmitCoverageMapping) {}

public:
  static PILProfiler *create(PILModule &M, ForDefinition_t forDefinition,
                             AstNode N, PILDeclRef forDecl);

  /// Check if the function is set up for profiling.
  bool hasRegionCounters() const { return NumRegionCounters != 0; }

  /// Get the execution count corresponding to \p Node from a profile, if one
  /// is available.
  ProfileCounter getExecutionCount(AstNode Node);

  /// Get the node's parent AstNode (e.g to get the parent IfStmt or IfCond of
  /// a condition), if one is available.PILSuccessor.h
  Optional<AstNode> getPGOParent(AstNode Node);

  /// Get the function name mangled for use with PGO.
  StringRef getPGOFuncName() const { return PGOFuncName; }

  /// Get the function hash.
  uint64_t getPGOFuncHash() const { return PGOFuncHash; }

  /// Get the number of region counters.
  unsigned getNumRegionCounters() const { return NumRegionCounters; }

  /// Get the mapping from \c AstNode to its corresponding profile counter.
  const llvm::DenseMap<AstNode, unsigned> &getRegionCounterMap() const {
    return RegionCounterMap;
  }

private:
  /// Map counters to AstNodes and set them up for profiling the function.
  void assignRegionCounters();
};

} // end namespace polar

#endif // POLARPHP_PIL_PROFILER_H
