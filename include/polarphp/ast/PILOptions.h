// This source file is part of the polarphp.org open source project
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/26.
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

namespace polar::ast {

using polar::basic::OptimizationMode;
using polar::basic::OptionSet;
using polar::basic::SanitizerKind;

class PILOptions
{
public:
   /// Controls the aggressiveness of the performance inliner.
   int inlineThreshold = -1;

   /// Controls the aggressiveness of the performance inliner for Osize.
   int callerBaseBenefitReductionFactor = 2;

   /// Controls the aggressiveness of the loop unroller.
   int unrollThreshold = 250;

   /// The number of threads for multi-threaded code generation.
   int numThreads = 0;

   /// Controls whether to pull in SIL from partial modules during the
   /// merge modules step. Could perhaps be merged with the link mode
   /// above but the interactions between all the flags are tricky.
   bool mergePartialModules = false;

   /// Remove all runtime assertions during optimizations.
   bool removeRuntimeAsserts = false;

   /// Enable existential specializer optimization.
   bool existentialSpecializer = false;

   /// Controls whether the SIL ARC optimizations are run.
   bool enableARCOptimizations = true;

   /// Should we run any SIL performance optimizations
   ///
   /// Useful when you want to enable -O LLVM opts but not -O SIL opts.
   bool disableSILPerfOptimizations = false;

   /// Controls whether or not paranoid verification checks are run.
   bool verifyAll = false;

   /// Are we debugging sil serialization.
   bool debugSerialization = false;

   /// Whether to dump verbose SIL with scope and location information.
   bool emitVerbosePIL = false;

   /// Whether to stop the optimization pipeline after serializing SIL.
   bool stopOptimizationAfterSerialization = false;

   /// Optimization mode being used.
   OptimizationMode optMode = OptimizationMode::NotSet;

   enum AssertConfiguration: unsigned
   {
      // Used by standard library code to distinguish between a debug and release
      // build.
      Debug = 0,     // Enables all asserts.
      Release = 1,   // Disables asserts.
      Unchecked = 2, // Disables asserts, preconditions, and runtime checks.

      // Leave the assert_configuration instruction around.
      DisableReplacement = UINT_MAX
   };

   /// The assert configuration controls how assertions behave.
   unsigned assertConfig = Debug;

   /// Should we print out instruction counts if -print-stats is passed in?
   bool printInstCounts = false;

   /// Instrument code to generate profiling information.
   bool generateProfile = false;

   /// Path to the profdata file to be used for PGO, or the empty string.
   std::string useProfile = "";

   /// Emit a mapping of profile counters for use in coverage.
   bool emitProfileCoverageMapping = false;

   /// Should we use a pass pipeline passed in via a json file? Null by default.
   llvm::StringRef externalPassPipelineFilename;

   /// Don't generate code using partial_apply in SIL generation.
   bool disableSILPartialApply = false;

   /// The name of the SIL outputfile if compiled with SIL debugging (-gsil).
   std::string PILOutputFileNameForDebugging;

   /// If set to true, compile with the SIL Ownership Model enabled.
   bool verifyPILOwnership = false;

   /// Assume that code will be executed in a single-threaded environment.
   bool assumeSingleThreaded = false;

   /// Indicates which sanitizer is turned on.
   OptionSet<SanitizerKind> sanitizers;

   /// Emit compile-time diagnostics when the law of exclusivity is violated.
   bool enforceExclusivityStatic = true;

   /// Emit checks to trap at run time when the law of exclusivity is violated.
   bool enforceExclusivityDynamic = true;

   /// Emit extra exclusvity markers for memory access and verify coverage.
   bool verifyExclusivity = false;

   /// Enable the mandatory semantic arc optimizer.
   bool enableMandatorySemanticARCOpts = false;

   /// Calls to the replaced method inside of the replacement method will call
   /// the previous implementation.
   ///
   /// @_dynamicReplacement(for: original())
   /// func replacement() {
   ///   if (...)
   ///     original() // calls original() implementation if true
   /// }
   bool enableDynamicReplacementCanCallPreviousImplementation = true;

   /// Enable large loadable types IRGen pass.
   bool enableLargeLoadableTypes = true;

   /// Should the default pass pipelines strip ownership during the diagnostic
   /// pipeline.
   bool stripOwnershipDuringDiagnosticsPipeline = true;

   /// The name of the file to which the backend should save YAML optimization
   /// records.
   std::string optRecordFile;

   PILOptions() {}

   /// Return a hash code of any components from these options that should
   /// contribute to a Swift Bridging PCH hash.
   llvm::hash_code getPCHHashComponents() const
   {
      return llvm::hash_value(0);
   }

   bool shouldOptimize() const
   {
      return optMode > OptimizationMode::NoOptimization;
   }

   bool hasMultipleIRGenThreads() const
   {
      return numThreads > 1;
   }

   bool shouldPerformIRGenerationInParallel() const
   {
      return numThreads != 0;
   }

   bool hasMultipleIGMs() const
   {
      return hasMultipleIRGenThreads();
   }
};

} // polar::ast

#endif // POLARPHP_AST_PIL_OPTIONS_H
