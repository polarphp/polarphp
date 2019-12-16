//===--- Compilation.h - Compilation Task Data Structure --------*- C++ -*-===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/26.

#ifndef POLARPHP_DRIVER_COMPILER_H
#define POLARPHP_DRIVER_COMPILER_H

#include "polarphp/basic/ArrayRefView.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/OutputFileMap.h"
#include "polarphp/basic/Statistic.h"
#include "polarphp/driver/Driver.h"
#include "polarphp/driver/Job.h"
#include "polarphp/driver/Utils.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Chrono.h"

#include <memory>
#include <vector>

namespace llvm::opt {
class InputArgList;
class DerivedArgList;
} // llvm::opt

namespace polar::ast {
class DiagnosticEngine;
} // polar::ast

namespace polar::sys {
class TaskQueue;
} // polar::sys

namespace polar::driver {

using polar::ast::DiagnosticEngine;
using llvm::opt::InputArgList;
using llvm::opt::DerivedArgList;
using polar::sys::TaskQueue;
using polar::ArrayRefView;
using polar::UnifiedStatsReporter;

class Driver;
class ToolChain;
class OutputInfo;
class PerformJobsState;

/// An enum providing different levels of output which should be produced
/// by a Compilation.
enum class OutputLevel
{
   /// Indicates that normal output should be produced.
   Normal,

   /// Indicates that only jobs should be printed and not run. (-###)
   PrintJobs,

   /// Indicates that verbose output should be produced. (-v)
   Verbose,

   /// Indicates that parseable output should be produced.
   Parseable,
};

/// Indicates whether a temporary file should always be preserved if a part of
/// the compilation crashes.
enum class PreserveOnSignal : bool
{
   No,
   Yes
};

class Compilation
{
private:
   template <typename T>
   static T *unwrap(const std::unique_ptr<T> &p)
   {
      return p.get();
   }

   template <typename T>
   using UnwrappedArrayView =
   ArrayRefView<std::unique_ptr<T>, T *, Compilation::unwrap<T>>;
public:
   // clang-format off
   Compilation(DiagnosticEngine &m_diags, const ToolChain &TC,
               OutputInfo const &OI,
               OutputLevel m_level,
               std::unique_ptr<llvm::opt::InputArgList> InputArgs,
               std::unique_ptr<llvm::opt::DerivedArgList> m_translatedArgs,
               InputFileList InputsWithTypes,
               std::string m_compilationRecordPath,
               bool m_outputCompilationRecordForModuleOnlyBuild,
               StringRef m_argsHash, llvm::sys::TimePoint<> StartTime,
               llvm::sys::TimePoint<> m_lastBuildTime,
               size_t m_filelistThreshold,
               bool m_enableIncrementalBuild = false,
               bool m_enableBatchMode = false,
               unsigned m_batchSeed = 0,
               Optional<unsigned> m_batchCount = None,
               Optional<unsigned> m_batchSizeLimit = None,
               bool m_saveTemps = false,
               bool m_showDriverTimeCompilation = false,
               std::unique_ptr<UnifiedStatsReporter> m_stats = nullptr,
               bool m_enableExperimentalDependencies = false,
               bool m_verifyExperimentalDependencyGraphAfterEveryImport = false,
               bool m_emitExperimentalDependencyDotFileAfterEveryImport = false,
               bool m_experimentalDependenciesIncludeIntrafileOnes = false);
   // clang-format on
   ~Compilation();

   ToolChain const &getToolChain() const {
      return m_theToolChain;
   }

   OutputInfo const &getOutputInfo() const {
      return m_theOutputInfo;
   }

   DiagnosticEngine &getDiags() const {
      return m_diags;
   }

   UnwrappedArrayView<const Action> getActions() const
   {
      return llvm::makeArrayRef(m_actions);
   }

   template <typename SpecificAction, typename... Args>
   SpecificAction *createAction(Args &&...args)
   {
      auto newAction = new SpecificAction(std::forward<Args>(args)...);
      m_actions.emplace_back(newAction);
      return newAction;
   }

   UnwrappedArrayView<const Job> getJobs() const
   {
      return llvm::makeArrayRef(m_jobs);
   }

   Job *addJob(std::unique_ptr<Job> job);

   void addTemporaryFile(StringRef file,
                         PreserveOnSignal preserve = PreserveOnSignal::No)
   {
      m_tempFilePaths[file] = preserve;
   }

   bool isTemporaryFile(StringRef file)
   {
      return m_tempFilePaths.count(file);
   }

   const llvm::opt::DerivedArgList &getArgs() const
   {
      return *m_translatedArgs;
   }

   ArrayRef<InputPair> getInputFiles() const
   {
      return m_inputFilesWithTypes;
   }

   OutputFileMap &getDerivedOutputFileMap()
   {
      return m_derivedOutputFileMap;
   }

   const OutputFileMap &getDerivedOutputFileMap() const
   {
      return m_derivedOutputFileMap;
   }

   bool getIncrementalBuildEnabled() const
   {
      return m_enableIncrementalBuild;
   }

   void disableIncrementalBuild()
   {
      m_enableIncrementalBuild = false;
   }

   bool getEnableExperimentalDependencies() const
   {
      return m_enableExperimentalDependencies;
   }

   bool getVerifyExperimentalDependencyGraphAfterEveryImport() const
   {
      return m_verifyExperimentalDependencyGraphAfterEveryImport;
   }

   bool getEmitExperimentalDependencyDotFileAfterEveryImport() const
   {
      return m_emitExperimentalDependencyDotFileAfterEveryImport;
   }

   bool getExperimentalDependenciesIncludeIntrafileOnes() const
   {
      return m_experimentalDependenciesIncludeIntrafileOnes;
   }

   bool getBatchModeEnabled() const
   {
      return m_enableBatchMode;
   }

   bool getContinueBuildingAfterErrors() const
   {
      return m_continueBuildingAfterErrors;
   }

   void setContinueBuildingAfterErrors(bool Value = true)
   {
      m_continueBuildingAfterErrors = Value;
   }

   bool getShowIncrementalBuildDecisions() const
   {
      return m_showIncrementalBuildDecisions;
   }

   void setShowIncrementalBuildDecisions(bool value = true)
   {
      m_showIncrementalBuildDecisions = value;
   }

   bool getShowJobLifecycle() const
   {
      return m_showJobLifecycle;
   }

   void setShowJobLifecycle(bool value = true)
   {
      m_showJobLifecycle = value;
   }

   bool getShowDriverTimeCompilation() const
   {
      return m_showDriverTimeCompilation;
   }

   size_t getFilelistThreshold() const
   {
      return m_filelistThreshold;
   }

   UnifiedStatsReporter *getStatsReporter() const
   {
      return m_stats.get();
   }

   /// True if extra work has to be done when tracing through the dependency
   /// graph, either in order to print dependencies or to collect statistics.
   bool getTraceDependencies() const
   {
      return getShowIncrementalBuildDecisions() || getStatsReporter();
   }

   OutputLevel getOutputLevel() const
   {
      return m_level;
   }

   unsigned getBatchSeed() const
   {
      return m_batchSeed;
   }

   llvm::sys::TimePoint<> getLastBuildTime() const
   {
      return m_lastBuildTime;
   }

   Optional<unsigned> getBatchCount() const
   {
      return m_batchCount;
   }

   Optional<unsigned> getBatchSizeLimit() const
   {
      return m_batchSizeLimit;
   }

   /// Requests the path to a file containing all input source files. This can
   /// be shared across jobs.
   ///
   /// If this is never called, the Compilation does not bother generating such
   /// a file.
   ///
   /// \sa types::isPartOfSwiftCompilation
   const char *getAllSourcesPath() const;

   /// Asks the Compilation to perform the m_jobs which it knows about.
   ///
   /// \param TQ The TaskQueue used to schedule jobs for execution.
   ///
   /// \returns result code for the Compilation's m_jobs; 0 indicates success and
   /// -2 indicates that one of the Compilation's m_jobs crashed during execution
   int performJobs(std::unique_ptr<sys::TaskQueue> &&TQ);

   /// Returns whether the callee is permitted to pass -emit-loaded-module-trace
   /// to a frontend job.
   ///
   /// This only returns true once, because only one job should pass that
   /// argument.
   bool requestPermissionForFrontendToEmitLoadedModuleTrace()
   {
      if (m_passedEmitLoadedModuleTraceToFrontendJob) {
         // Someone else has already done it!
         return false;
      } else {
         // We're the first and only (to execute this path).
         m_passedEmitLoadedModuleTraceToFrontendJob = true;
         return true;
      }
   }

private:
   /// Perform all jobs.
   ///
   /// \param[out] abnormalExit Set to true if any job exits abnormally (i.e.
   /// crashes).
   /// \param TQ The task queue on which jobs will be scheduled.
   ///
   /// \returns exit code of the first failed Job, or 0 on success. If a Job
   /// crashes during execution, a negative value will be returned.
   int performJobsImpl(bool &abnormalExit, std::unique_ptr<sys::TaskQueue> &&TQ);

   /// Performs a single Job by executing in place, if possible.
   ///
   /// \param Cmd the Job which should be performed.
   ///
   /// \returns Typically, this function will not return, as the current process
   /// will no longer exist, or it will call exit() if the program was
   /// successfully executed. In the event of an error, this function will return
   /// a negative value indicating a failure to execute.
   int performSingleCommand(const Job *Cmd);

public:
   /// The filelist threshold value to pass to ensure file lists are never used
   static const size_t NEVER_USE_FILELIST = SIZE_MAX;

private:
   /// The DiagnosticEngine to which this Compilation should emit diagnostics.
   DiagnosticEngine &m_diags;

   /// The ToolChain this Compilation was built with, that it may reuse to build
   /// subsequent BatchJobs.
   const ToolChain &m_theToolChain;

   /// The OutputInfo, which the Compilation stores a copy of upon
   /// construction, and which it may use to build subsequent batch
   /// jobs itself.
   OutputInfo m_theOutputInfo;

   /// The OutputLevel at which this Compilation should generate output.
   OutputLevel m_level;

   /// The OutputFileMap describing the Compilation's outputs, populated both by
   /// the user-provided output file map (if it exists) and inference rules that
   /// derive otherwise-unspecified output filenames from context.
   OutputFileMap m_derivedOutputFileMap;

   /// The m_actions which were used to build the m_jobs.
   ///
   /// This is mostly only here for lifetime management.
   SmallVector<std::unique_ptr<const Action>, 32> m_actions;

   /// The m_jobs which will be performed by this compilation.
   SmallVector<std::unique_ptr<const Job>, 32> m_jobs;

   /// The original (untranslated) input argument list.
   ///
   /// This is only here for lifetime management. Any inspection of
   /// command-line arguments should use #getArgs().
   std::unique_ptr<llvm::opt::InputArgList> m_rawInputArgs;

   /// The translated input arg list.
   std::unique_ptr<llvm::opt::DerivedArgList> m_translatedArgs;

   /// A list of input files and their associated types.
   InputFileList m_inputFilesWithTypes;

   /// When non-null, a temporary file containing all input .swift files.
   /// Used for large compilations to avoid overflowing argv.
   const char *m_allSourceFilesPath = nullptr;

   /// Temporary files that should be cleaned up after the compilation finishes.
   ///
   /// These apply whether the compilation succeeds or fails. If the
   llvm::StringMap<PreserveOnSignal> m_tempFilePaths;

   /// Write information about this compilation to this file.
   ///
   /// This is used for incremental builds.
   std::string m_compilationRecordPath;

   /// A hash representing all the arguments that could trigger a full rebuild.
   std::string m_argsHash;

   /// When the build was started.
   ///
   /// This should be as close as possible to when the driver was invoked, since
   /// it's used as a lower bound.
   llvm::sys::TimePoint<> m_buildStartTime;

   /// The time of the last build.
   ///
   /// If unknown, this will be some time in the past.
   llvm::sys::TimePoint<> m_lastBuildTime = llvm::sys::TimePoint<>::min();

   /// Indicates whether this Compilation should continue execution of subtasks
   /// even if they returned an error status.
   bool m_continueBuildingAfterErrors = false;

   /// Indicates whether tasks should only be executed if their output is out
   /// of date.
   bool m_enableIncrementalBuild;

   /// When true, emit duplicated compilation record file whose filename is
   /// suffixed with '~moduleonly'.
   ///
   /// This compilation record is used by '-emit-module'-only incremental builds
   /// so that module-only builds do not affect compilation record file for
   /// normal builds, while module-only incremental builds are able to use
   /// artifacts of normal builds if they are already up to date.
   bool m_outputCompilationRecordForModuleOnlyBuild = false;

   /// Indicates whether groups of parallel frontend jobs should be merged
   /// together and run in composite "batch jobs" when possible, to reduce
   /// redundant work.
   const bool m_enableBatchMode;

   /// Provides a randomization seed to batch-mode partitioning, for debugging.
   const unsigned m_batchSeed;

   /// Overrides parallelism level and \c m_batchSizeLimit, sets exact
   /// count of batches, if in batch-mode.
   const Optional<unsigned> m_batchCount;

   /// Overrides maximum batch size, if in batch-mode and not overridden
   /// by \c m_batchCount.
   const Optional<unsigned> m_batchSizeLimit;

   /// True if temporary files should not be deleted.
   const bool m_saveTemps;

   /// When true, dumps information on how long each compilation task took to
   /// execute.
   const bool m_showDriverTimeCompilation;

   /// When non-null, record various high-level counters to this.
   std::unique_ptr<UnifiedStatsReporter> m_stats;

   /// When true, dumps information about why files are being scheduled to be
   /// rebuilt.
   bool m_showIncrementalBuildDecisions = false;

   /// When true, traces the lifecycle of each driver job. Provides finer
   /// detail than m_showIncrementalBuildDecisions.
   bool m_showJobLifecycle = false;

   /// When true, some frontend job has requested permission to pass
   /// -emit-loaded-module-trace, so no other job needs to do it.
   bool m_passedEmitLoadedModuleTraceToFrontendJob = false;

   /// The limit for the number of files to pass on the command line. Beyond this
   /// limit filelists will be used.
   size_t m_filelistThreshold;

   /// Scaffolding to permit experimentation with finer-grained dependencies and
   /// faster rebuilds.
   const bool m_enableExperimentalDependencies;

   /// Helpful for debugging, but slows down the driver. So, only turn on when
   /// needed.
   const bool m_verifyExperimentalDependencyGraphAfterEveryImport;
   /// Helpful for debugging, but slows down the driver. So, only turn on when
   /// needed.
   const bool m_emitExperimentalDependencyDotFileAfterEveryImport;

   /// Experiment with inter-file dependencies
   const bool m_experimentalDependenciesIncludeIntrafileOnes;
};

} // polar::driver

#endif // POLARPHP_DRIVER_COMPILER_H
