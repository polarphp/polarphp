//===--- PassManagerVerifierAnalysis.h ------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_OPTIMIZER_ANALYSIS_PASSMANAGERVERIFIERANALYSIS_H
#define POLARPHP_PIL_OPTIMIZER_ANALYSIS_PASSMANAGERVERIFIERANALYSIS_H

#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringSet.h"

namespace polar {

/// An analysis that validates that the pass manager properly sends add/delete
/// messages as functions are added/deleted from the module.
///
/// All methods are no-ops when asserts are disabled.
class PassManagerVerifierAnalysis : public PILAnalysis {
  /// The module that we are processing.
  LLVM_ATTRIBUTE_UNUSED
  PILModule &mod;

  /// The set of "live" functions that we are tracking. We store the names of
  /// the functions so that if a function is deleted we do not need to touch its
  /// memory to get its name.
  ///
  /// All functions in mod must be in liveFunctions and vis-a-versa.
  llvm::StringSet<> liveFunctionNames;

public:
  PassManagerVerifierAnalysis(PILModule *mod);

  static bool classof(const PILAnalysis *analysis) {
    return analysis->getKind() == PILAnalysisKind::PassManagerVerifier;
  }

  /// Validate that the analysis is able to look up all functions and that those
  /// functions are live.
  void invalidate() override final;

  /// Validate that the analysis is able to look up the given function.
  void invalidate(PILFunction *f, InvalidationKind k) override final;

  /// If a function has not yet been seen start tracking it.
  void notifyAddedOrModifiedFunction(PILFunction *f) override final;

  /// Stop tracking a function.
  void notifyWillDeleteFunction(PILFunction *f) override final;

  /// Make sure that when we invalidate a function table, make sure we can find
  /// all functions for all witness tables.
  void invalidateFunctionTables() override final;

  /// Run the entire verification.
  void verifyFull() const override final;
};

} // namespace polar

#endif // POLARPHP_PIL_OPTIMIZER_ANALYSIS_PASSMANAGERVERIFIERANALYSIS_H
