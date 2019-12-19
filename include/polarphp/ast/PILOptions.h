//===--- PILOptions.h - Swift Language PILGen and PIL options ---*- C++ -*-===//
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
// This file defines the options which control the generation, processing,
// and optimization of PIL.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_PIL_OPTIONS_H
#define POLARPHP_AST_PIL_OPTIONS_H

#include "polarphp/basic/Sanitizers.h"
#include "polarphp/basic/OptionSet.h"
#include "polarphp/basic/OptimizationMode.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/StringRef.h"
#include <string>
#include <climits>

namespace polar {

using polar::OptimizationMode;
using polar::OptionSet;
using polar::SanitizerKind;

class PILOptions {
public:
  /// Controls the aggressiveness of the performance inliner.
  int InlineThreshold = -1;

  /// Controls the aggressiveness of the performance inliner for Osize.
  int CallerBaseBenefitReductionFactor = 2;

  /// Controls the aggressiveness of the loop unroller.
  int UnrollThreshold = 250;

  /// The number of threads for multi-threaded code generation.
  int NumThreads = 0;

  /// Controls whether to pull in PIL from partial modules during the
  /// merge modules step. Could perhaps be merged with the link mode
  /// above but the interactions between all the flags are tricky.
  bool MergePartialModules = false;

  /// Remove all runtime assertions during optimizations.
  bool RemoveRuntimeAsserts = false;

  /// Controls whether the PIL ARC optimizations are run.
  bool EnableARCOptimizations = true;

  /// Controls whether specific OSSA optimizations are run. For benchmarking
  /// purposes.
  bool EnableOSSAOptimizations = true;

  /// Should we run any PIL performance optimizations
  ///
  /// Useful when you want to enable -O LLVM opts but not -O PIL opts.
  bool DisablePILPerfOptimizations = false;

  /// Controls whether cross module optimization is enabled.
  bool CrossModuleOptimization = false;

  /// Controls whether or not paranoid verification checks are run.
  bool VerifyAll = false;

  /// Are we debugging sil serialization.
  bool DebugSerialization = false;

  /// Whether to dump verbose PIL with scope and location information.
  bool EmitVerbosePIL = false;

  /// Whether to stop the optimization pipeline after serializing PIL.
  bool StopOptimizationAfterSerialization = false;

  /// Whether to skip emitting non-inlinable function bodies.
  bool SkipNonInlinableFunctionBodies = false;

  /// Optimization mode being used.
  OptimizationMode OptMode = OptimizationMode::NotSet;

  enum AssertConfiguration: unsigned {
    // Used by standard library code to distinguish between a debug and release
    // build.
    Debug = 0,     // Enables all asserts.
    Release = 1,   // Disables asserts.
    Unchecked = 2, // Disables asserts, preconditions, and runtime checks.

    // Leave the assert_configuration instruction around.
    DisableReplacement = UINT_MAX
  };

  /// The assert configuration controls how assertions behave.
  unsigned AssertConfig = Debug;

  /// Should we print out instruction counts if -print-stats is passed in?
  bool PrintInstCounts = false;

  /// Instrument code to generate profiling information.
  bool GenerateProfile = false;

  /// Path to the profdata file to be used for PGO, or the empty string.
  std::string UseProfile = "";

  /// Emit a mapping of profile counters for use in coverage.
  bool EmitProfileCoverageMapping = false;

  /// Should we use a pass pipeline passed in via a json file? Null by default.
  llvm::StringRef ExternalPassPipelineFilename;

  /// Don't generate code using partial_apply in PIL generation.
  bool DisablePILPartialApply = false;

  /// The name of the PIL outputfile if compiled with PIL debugging (-gsil).
  std::string PILOutputFileNameForDebugging;

  /// If set to true, compile with the PIL Ownership Model enabled.
  bool VerifyPILOwnership = true;

  /// Assume that code will be executed in a single-threaded environment.
  bool AssumeSingleThreaded = false;

  /// Indicates which sanitizer is turned on.
  OptionSet<SanitizerKind> Sanitizers;

  /// Emit compile-time diagnostics when the law of exclusivity is violated.
  bool EnforceExclusivityStatic = true;

  /// Emit checks to trap at run time when the law of exclusivity is violated.
  bool EnforceExclusivityDynamic = true;

  /// Emit extra exclusvity markers for memory access and verify coverage.
  bool VerifyExclusivity = false;

  /// Calls to the replaced method inside of the replacement method will call
  /// the previous implementation.
  ///
  /// @_dynamicReplacement(for: original())
  /// func replacement() {
  ///   if (...)
  ///     original() // calls original() implementation if true
  /// }
  bool EnableDynamicReplacementCanCallPreviousImplementation = true;

  /// Enable large loadable types IRGen pass.
  bool EnableLargeLoadableTypes = true;

  /// Should the default pass pipelines strip ownership during the diagnostic
  /// pipeline or after serialization.
  bool StripOwnershipAfterSerialization = true;

  /// The name of the file to which the backend should save YAML optimization
  /// records.
  std::string OptRecordFile;

  PILOptions() {}

  /// Return a hash code of any components from these options that should
  /// contribute to a Swift Bridging PCH hash.
  llvm::hash_code getPCHHashComponents() const {
    return llvm::hash_value(0);
  }

  bool shouldOptimize() const {
    return OptMode > OptimizationMode::NoOptimization;
  }

  bool hasMultipleIRGenThreads() const { return NumThreads > 1; }
  bool shouldPerformIRGenerationInParallel() const { return NumThreads != 0; }
  bool hasMultipleIGMs() const { return hasMultipleIRGenThreads(); }
};

} // end namespace polar

#endif // POLARPHP_AST_PIL_OPTIONS_H
