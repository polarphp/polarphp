//===--- Compilation.cpp - Compilation Task Data Structure ----------------===//
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
// Created by polarboy on 2019/12/02.

#include "polarphp/driver/Compilation.h"

#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/ast/DiagnosticsDriver.h"
#include "polarphp/ast/ExperimentalDependencies.h"
#include "polarphp/driver/internal/CompilationRecord.h"
#include "polarphp/basic/OutputFileMap.h"
#include "polarphp/basic/Program.h"
#include "polarphp/basic/StlExtras.h"
#include "polarphp/basic/Statistic.h"
#include "polarphp/basic/TaskQueue.h"
#include "polarphp/kernel/Version.h"
#include "polarphp/driver/Action.h"
#include "polarphp/driver/DependencyGraph.h"
#include "polarphp/driver/Driver.h"
#include "polarphp/driver/ExperimentalDependencyDriverGraph.h"
#include "polarphp/driver/Job.h"
#include "polarphp/driver/ParseableOutput.h"
#include "polarphp/driver/ToolChain.h"
#include "polarphp/option/Options.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/Option/Arg.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/Chrono.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/YAMLParser.h"
#include "llvm/Support/raw_ostream.h"

#include <signal.h>
#include <type_traits>
// batch-mode has a sub-mode for testing that randomizes batch partitions,
// by user-provided seed. That is the only thing randomized here.
#include <random>

#define DEBUG_TYPE "batch-mode"

namespace polar::driver {

using namespace llvm::opt;
using namespace polar::ast;
using namespace polar::sys;
using namespace polar::basic;

struct LogJob
{
   const Job *job;
   LogJob(const Job *job)
      : job(job)
   {}
};

struct LogJobArray
{
   const ArrayRef<const Job *> jobs;
   LogJobArray(const ArrayRef<const Job *> jobs)
      : jobs(jobs)
   {}
};

struct LogJobSet
{
   const SmallPtrSetImpl<const Job*> &jobs;
   LogJobSet(const SmallPtrSetImpl<const Job*> &jobs)
      : jobs(jobs) {}
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &ostream, const LogJob &logJob)
{
   logJob.job->printSummary(ostream);
   return ostream;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &ostream, const LogJobArray &logJob)
{
   ostream << "[";
   interleave(logJob.jobs,
              [&](Job const *job) { ostream << LogJob(job); },
   [&]() { ostream << ' '; });
   ostream << "]";
   return ostream;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &ostream, const LogJobSet &logJobSet)
{
   ostream << "{";
   interleave(logJobSet.jobs,
              [&](Job const *job) { ostream << LogJob(job); },
   [&]() { ostream << ' '; });
   ostream << "}";
   return ostream;
}

// clang-format off
Compilation::Compilation(DiagnosticEngine &diags,
                         const ToolChain &toolchain,
                         OutputInfo const &outputInfo,
                         OutputLevel level,
                         std::unique_ptr<InputArgList> inputArgs,
                         std::unique_ptr<DerivedArgList> translatedArgs,
                         InputFileList inputsWithTypes,
                         std::string compilationRecordPath,
                         bool outputCompilationRecordForModuleOnlyBuild,
                         StringRef argsHash,
                         llvm::sys::TimePoint<> startTime,
                         llvm::sys::TimePoint<> lastBuildTime,
                         size_t filelistThreshold,
                         bool enableIncrementalBuild,
                         bool enableBatchMode,
                         unsigned batchSeed,
                         Optional<unsigned> batchCount,
                         Optional<unsigned> batchSizeLimit,
                         bool saveTemps,
                         bool showDriverTimeCompilation,
                         std::unique_ptr<UnifiedStatsReporter> statsReporter,
                         bool enableExperimentalDependencies,
                         bool verifyExperimentalDependencyGraphAfterEveryImport,
                         bool emitExperimentalDependencyDotFileAfterEveryImport,
                         bool experimentalDependenciesIncludeIntrafileOnes)
   : m_diags(diags),
     m_theToolChain(toolchain),
     m_theOutputInfo(outputInfo),
     m_level(level),
     m_rawInputArgs(std::move(inputArgs)),
     m_translatedArgs(std::move(translatedArgs)),
     m_inputFilesWithTypes(std::move(inputsWithTypes)),
     m_compilationRecordPath(compilationRecordPath),
     m_argsHash(argsHash),
     m_buildStartTime(startTime),
     m_lastBuildTime(lastBuildTime),
     m_enableIncrementalBuild(enableIncrementalBuild),
     m_outputCompilationRecordForModuleOnlyBuild(
        outputCompilationRecordForModuleOnlyBuild),
     m_enableBatchMode(enableBatchMode),
     m_batchSeed(batchSeed),
     m_batchCount(batchCount),
     m_batchSizeLimit(batchSizeLimit),
     m_saveTemps(saveTemps),
     m_showDriverTimeCompilation(showDriverTimeCompilation),
     m_stats(std::move(statsReporter)),
     m_filelistThreshold(filelistThreshold),
     m_enableExperimentalDependencies(enableExperimentalDependencies),
     m_verifyExperimentalDependencyGraphAfterEveryImport(
        verifyExperimentalDependencyGraphAfterEveryImport),
     m_emitExperimentalDependencyDotFileAfterEveryImport(
        emitExperimentalDependencyDotFileAfterEveryImport),
     m_experimentalDependenciesIncludeIntrafileOnes(
        experimentalDependenciesIncludeIntrafileOnes)
{}

// clang-format on

static bool write_filelist_if_necessary(const Job *job, const ArgList &args,
                                        DiagnosticEngine &diags);

using CommandSet = llvm::SmallPtrSet<const Job *, 16>;
using CommandSetVector = llvm::SetVector<const Job*>;
using BatchPartition = std::vector<std::vector<const Job*>>;

using InputInfoMap = llvm::SmallMapVector<const llvm::opt::Arg *,
CompileJobAction::InputInfo, 16>;


class PerformJobsState
{
public:
   /// Dependency graph for deciding which jobs are dirty (need running)
   /// or clean (can be skipped).
   using JobDependencyGraph = DependencyGraph<const Job *>;
   JobDependencyGraph m_standardDepGraph;

   /// Experimental Dependency graph for finer-grained dependencies
   Optional<experimentaldependencies::ModuleDepGraph> m_expDepGraph;

private:

   void noteBuilding(const Job *cmd, StringRef reason)
   {
      if (!m_compilation.getShowIncrementalBuildDecisions()) {
         return;
      }
      if (m_scheduledCommands.count(cmd)) {
         return;
      }
      llvm::outs() << "Queuing " << reason << ": " << LogJob(cmd) << "\n";
      if (m_compilation.getEnableExperimentalDependencies()) {
         m_expDepGraph.getValue().printPath(llvm::outs(), cmd);
      } else {
         m_incrementalTracer->printPath(
                  llvm::outs(), cmd, [](raw_ostream &out, const Job *base) {
            out << llvm::sys::path::filename(base->getOutput().getBaseInput(0));
         });
      }
   }

   const Job *findUnfinishedJob(ArrayRef<const Job *> jobs)
   {
      for (const Job *cmd : jobs) {
         if (!m_finishedCommands.count(cmd)) {
            return cmd;
         }
      }
      return nullptr;
   }

   /// Schedule the given Job if it has not been scheduled and if all of
   /// its inputs are in m_finishedCommands.
   void scheduleCommandIfNecessaryAndPossible(const Job *cmd)
   {
      if (m_scheduledCommands.count(cmd)) {
         if (m_compilation.getShowJobLifecycle()) {
            llvm::outs() << "Already scheduled: " << LogJob(cmd) << "\n";
         }
         return;
      }

      if (auto Blocking = findUnfinishedJob(cmd->getInputs())) {
         m_blockingCommands[Blocking].push_back(cmd);
         if (m_compilation.getShowJobLifecycle()) {
            llvm::outs() << "blocked by: " << LogJob(Blocking)
                         << ", now blocking jobs: "
                         << LogJobArray(m_blockingCommands[Blocking]) << "\n";
         }
         return;
      }

      // Adding to scheduled means we've committed to its completion (not
      // distinguished from skipping). We never remove it once inserted.
      m_scheduledCommands.insert(cmd);

      // Adding to pending means it should be in the next round of additions to
      // the task queue (either batched or singularly); we remove Jobs from
      // m_pendingExecution once we hand them over to the taskQueue.
      m_pendingExecution.insert(cmd);
   }

   void addPendingJobToTaskQueue(const Job *cmd)
   {
      // FIXME: Failing here should not take down the whole process.
      bool success =
            write_filelist_if_necessary(cmd, m_compilation.getArgs(), m_compilation.getDiags());
      assert(success && "failed to write filelist");
      (void)success;

      assert(cmd->getExtraEnvironment().empty() &&
             "not implemented for compilations with multiple jobs");
      if (m_compilation.getShowJobLifecycle()) {
         llvm::outs() << "Added to taskQueue: " << LogJob(cmd) << "\n";
      }
      m_taskQueue->addTask(cmd->getExecutable(), cmd->getArgumentsForTaskExecution(),
                           llvm::None, reinterpret_cast<void *>(const_cast<Job *>(cmd)));
   }

   /// When a task finishes, check other Jobs that may be blocked.
   void markFinished(const Job *cmd, bool Skipped=false)
   {
      if (m_compilation.getShowJobLifecycle()) {
         llvm::outs() << "Job "
                      << (Skipped ? "skipped" : "finished")
                      << ": " << LogJob(cmd) << "\n";
      }
      m_finishedCommands.insert(cmd);
      if (auto *Stats = m_compilation.getStatsReporter()) {
         auto &counter = Stats->getDriverCounters();
         if (Skipped) {
            counter.NumDriverJobsSkipped++;
         } else {
            counter.NumDriverJobsRun++;
         }
      }
      auto blockedIter = m_blockingCommands.find(cmd);
      if (blockedIter != m_blockingCommands.end()) {
         auto allBlocked = std::move(blockedIter->second);
         if (m_compilation.getShowJobLifecycle()) {
            llvm::outs() << "Scheduling maybe-unblocked jobs: "
                         << LogJobArray(allBlocked) << "\n";
         }
         m_blockingCommands.erase(blockedIter);
         for (auto *blocked : allBlocked) {
            scheduleCommandIfNecessaryAndPossible(blocked);
         }
      }
   }

   bool isBatchJob(const Job *maybeBatchJob) const
   {
      return m_batchJobs.count(maybeBatchJob) != 0;
   }

   /// Callback which will be called immediately after a task has started. This
   /// callback may be used to provide output indicating that the task began.
   void taskBegan(ProcessId pid, void *context)
   {
      // TODO: properly handle task began.
      const Job *beganCmd = reinterpret_cast<const Job *>(context);
      if (m_compilation.getShowDriverTimeCompilation()) {
         llvm::SmallString<128> TimerName;
         llvm::raw_svector_ostream ostream(TimerName);
         ostream << LogJob(beganCmd);
         m_driverTimers.insert({
                                  beganCmd,
                                  std::unique_ptr<llvm::Timer>(
                                  new llvm::Timer("task", ostream.str(), m_driverTimerGroup))
                               });
         m_driverTimers[beganCmd]->startTimer();
      }

      switch (m_compilation.getOutputLevel()) {
      case OutputLevel::Normal:
         break;
         // For command line or verbose output, print out each command as it
         // begins execution.
      case OutputLevel::PrintJobs:
         beganCmd->printCommandLineAndEnvironment(llvm::outs());
         break;
      case OutputLevel::Verbose:
         beganCmd->printCommandLine(llvm::errs());
         break;
      case OutputLevel::Parseable:
         beganCmd->forEachContainedJobAndPID(pid, [&](const Job *job, Job::PID pid) {
            parseableoutput::emit_began_message(llvm::errs(), *job, pid,
                                                TaskProcessInformation(pid));
         });
         break;
      }
   }

   /// Note that a .swiftdeps file failed to load and take corrective actions:
   /// disable incremental logic and schedule all existing deferred commands.
   void
   dependencyLoadFailed(StringRef dependenciesFile, bool warn=true)
   {
      if (warn && m_compilation.getShowIncrementalBuildDecisions()) {
         m_compilation.getDiags().diagnose(SourceLoc(),
                                           diag::warn_unable_to_load_dependencies,
                                           dependenciesFile);
      }
      m_compilation.disableIncrementalBuild();
      for (const Job *cmd : m_deferredCommands) {
         scheduleCommandIfNecessaryAndPossible(cmd);
      }
      m_deferredCommands.clear();
   }

   /// Helper that attempts to reload a job's .swiftdeps file after the job
   /// exits, and re-run transitive marking to ensure everything is properly
   /// invalidated by any new dependency edges introduced by it. If reloading
   /// fails, this can cause deferred jobs to be immediately scheduled.
   template <unsigned N, typename DependencyGraphT>
   void reloadAndRemarkDeps(const Job *finishedCmd,
                            int returnCode,
                            SmallVector<const Job *, N> &dependents,
                            DependencyGraphT &depGraph)
   {

      const CommandOutput &output = finishedCmd->getOutput();
      StringRef dependenciesFile =
            output.getAdditionalOutputForType(filetypes::TY_PolarDeps);

      if (dependenciesFile.empty()) {
         // If this job doesn't track dependencies, it must always be run.
         // Note: In theory CheckDependencies makes sense as well (for a leaf
         // node in the dependency graph), and maybe even NewlyAdded (for very
         // coarse dependencies that always affect downstream nodes), but we're
         // not using either of those right now, and this logic should probably
         // be revisited when we are.
         assert(finishedCmd->getCondition() == Job::Condition::Always);
      } else {
         // If we have a dependency file /and/ the frontend task exited normally,
         // we can be discerning about what downstream files to rebuild.
         if (returnCode == EXIT_SUCCESS || returnCode == EXIT_FAILURE) {
            // "Marked" means that everything provided by this node (i.e. Job) is
            // dirty. Thus any file using any of these provides must be
            // recompiled. (Only non-private entities are output as provides.) In
            // other words, this Job "cascades"; the need to recompile it causes
            // other recompilations. It is possible that the current code marks
            // things that do not need to be marked. Unecessary compilation would
            // result if that were the case.
            bool wasCascading = depGraph.isMarked(finishedCmd);
            switch (depGraph.loadFromPath(finishedCmd, dependenciesFile,
                                          m_compilation.getDiags())) {
            case DependencyGraphImpl::LoadResult::HadError:
               if (returnCode == EXIT_SUCCESS) {
                  dependencyLoadFailed(dependenciesFile);
                  dependents.clear();
               } // else, let the next build handle it.
               break;
            case DependencyGraphImpl::LoadResult::UpToDate:
               if (!wasCascading)
                  break;
               LLVM_FALLTHROUGH;
            case DependencyGraphImpl::LoadResult::AffectsDownstream:
               depGraph.markTransitive(dependents, finishedCmd,
                                       m_incrementalTracer);
               break;
            }
         } else {
            // If there's an abnormal exit (a crash), assume the worst.
            switch (finishedCmd->getCondition()) {
            case Job::Condition::NewlyAdded:
               // The job won't be treated as newly added next time. Conservatively
               // mark it as affecting other jobs, because some of them may have
               // completed already.
               depGraph.markTransitive(dependents, finishedCmd,
                                       m_incrementalTracer);
               break;
            case Job::Condition::Always:
               // Any incremental task that shows up here has already been marked;
               // we didn't need to wait for it to finish to start downstream
               // tasks.
               assert(depGraph.isMarked(finishedCmd));
               break;
            case Job::Condition::RunWithoutCascading:
               // If this file changed, it might have been a non-cascading change
               // and it might not. Unfortunately, the interface hash has been
               // updated or compromised, so we don't actually know anymore; we
               // have to conservatively assume the changes could affect other
               // files.
               depGraph.markTransitive(dependents, finishedCmd,
                                       m_incrementalTracer);
               break;
            case Job::Condition::CheckDependencies:
               // If the only reason we're running this is because something else
               // changed, then we can trust the dependency graph as to whether
               // it's a cascading or non-cascading change. That is, if whatever
               // /caused/ the error isn't supposed to affect other files, and
               // whatever /fixes/ the error isn't supposed to affect other files,
               // then there's no need to recompile any other inputs. If either of
               // those are false, we /do/ need to recompile other inputs.
               break;
            }
         }
      }
   }

   /// Check to see if a job produced a zero-length serialized diagnostics
   /// file, which is used to indicate batch-constituents that were batched
   /// together with a failing constituent but did not, themselves, produce any
   /// errors.
   bool jobWasBatchedWithFailingJobs(const Job *job) const
   {
      auto diaPath =
            job->getOutput().getAnyOutputForType(filetypes::TY_SerializedDiagnostics);
      if (diaPath.empty()) {
         return false;
      }
      if (!llvm::sys::fs::is_regular_file(diaPath)) {
         return false;
      }
      uint64_t Size;
      auto errorCode = llvm::sys::fs::file_size(diaPath, Size);
      if (errorCode)
         return false;
      return Size == 0;
   }

   /// If a batch-constituent job happens to be batched together with a job
   /// that exits with an error, the batch-constituent may be considered
   /// "cancelled".
   bool jobIsCancelledBatchConstituent(int returnCode,
                                       const Job *containerJob,
                                       const Job *constituentJob)
   {
      return returnCode != 0 &&
            isBatchJob(containerJob) &&
            jobWasBatchedWithFailingJobs(constituentJob);
   }

   /// Unpack a \c BatchJob that has finished into its constituent \c Job
   /// members, and call \c taskFinished on each, propagating any \c
   /// TaskFinishedResponse other than \c
   /// TaskFinishedResponse::ContinueExecution from any of the constituent
   /// calls.
   TaskFinishedResponse
   unpackAndFinishBatch(int returnCode, StringRef output,
                        StringRef errors, const BatchJob *batchJob)
   {
      if (m_compilation.getShowJobLifecycle()) {
         llvm::outs() << "batch job finished: " << LogJob(batchJob) << "\n";
      }
      auto res = TaskFinishedResponse::ContinueExecution;
      for (const Job *job : batchJob->getCombinedJobs()) {
         if (m_compilation.getShowJobLifecycle()) {
            llvm::outs() << "  ==> Unpacked batch constituent finished: "
                         << LogJob(job) << "\n";
         }
         auto r = taskFinished(
                  llvm::sys::ProcessInfo::InvalidPid, returnCode, output, errors,
                  TaskProcessInformation(llvm::sys::ProcessInfo::InvalidPid),
                  reinterpret_cast<void *>(const_cast<Job *>(job)));
         if (r != TaskFinishedResponse::ContinueExecution) {
            res = r;
         }
      }
      return res;
   }

   void
   emitParseableOutputForEachFinishedJob(ProcessId pid, int returnCode,
                                         StringRef output,
                                         const Job *finishedCmd,
                                         TaskProcessInformation procInfo)
   {
      finishedCmd->forEachContainedJobAndPID(pid, [&](const Job *job,
                                             Job::PID pid) {
         if (jobIsCancelledBatchConstituent(returnCode, finishedCmd, job)) {
            // Simulate SIGINT-interruption to parseable-output consumer for any
            // constituent of a failing batch job that produced no errors of its
            // own.
            parseableoutput::emit_signalled_message(llvm::errs(), *job, pid,
                                                    "cancelled batch constituent",
                                                    "", SIGINT, procInfo);
         } else {
            parseableoutput::emit_finished_message(llvm::errs(), *job, pid, returnCode,
                                                   output, procInfo);
         }
      });
   }

   /// Callback which will be called immediately after a task has finished
   /// execution. Determines if execution should continue, and also schedule
   /// any additional Jobs which we now know we need to run.
   TaskFinishedResponse taskFinished(ProcessId pid, int returnCode,
                                     StringRef output, StringRef errors,
                                     TaskProcessInformation procInfo,
                                     void *context)
   {
      const Job *finishedCmd = reinterpret_cast<const Job *>(context);

      if (pid != llvm::sys::ProcessInfo::InvalidPid) {

         if (m_compilation.getShowDriverTimeCompilation()) {
            m_driverTimers[finishedCmd]->stopTimer();
         }

         switch (m_compilation.getOutputLevel()) {
         case OutputLevel::PrintJobs:
            // Only print the jobs, not the outputs
            break;
         case OutputLevel::Normal:
         case OutputLevel::Verbose:
            // Send the buffered output to stderr, though only if we
            // support getting buffered output.
            if (TaskQueue::supportsBufferingOutput())
               llvm::errs() << output;
            break;
         case OutputLevel::Parseable:
            emitParseableOutputForEachFinishedJob(pid, returnCode, output,
                                                  finishedCmd, procInfo);
            break;
         }
      }

      if (isBatchJob(finishedCmd)) {
         return unpackAndFinishBatch(returnCode, output, errors,
                                     static_cast<const BatchJob *>(finishedCmd));
      }

      // In order to handle both old dependencies that have disappeared and new
      // dependencies that have arisen, we need to reload the dependency file.
      // Do this whether or not the build succeeded.
      SmallVector<const Job *, 16> dependents;
      if (m_compilation.getIncrementalBuildEnabled()) {
         if (m_compilation.getEnableExperimentalDependencies()) {
            reloadAndRemarkDeps(finishedCmd, returnCode, dependents,
                                m_expDepGraph.getValue());
         } else {
            reloadAndRemarkDeps(finishedCmd, returnCode, dependents,
                                m_standardDepGraph);
         }
      }

      if (returnCode != EXIT_SUCCESS) {
         // The task failed, so return true without performing any further
         // dependency analysis.

         // Store this task's returnCode as our m_result if we haven't stored
         // anything yet.
         if (m_result == EXIT_SUCCESS) {
            m_result = returnCode;
         }

         if (!isa<CompileJobAction>(finishedCmd->getSource()) ||
             returnCode != EXIT_FAILURE) {
            m_compilation.getDiags().diagnose(SourceLoc(), diag::error_command_failed,
                                              finishedCmd->getSource().getClassName(),
                                              returnCode);
         }

         // See how ContinueBuildingAfterErrors gets set up in Driver.cpp for
         // more info.
         assert((m_compilation.getContinueBuildingAfterErrors() ||
                 !m_compilation.getBatchModeEnabled()) &&
                "batch mode diagnostics require ContinueBuildingAfterErrors");

         return m_compilation.getContinueBuildingAfterErrors() ?
                  TaskFinishedResponse::ContinueExecution :
                  TaskFinishedResponse::StopExecution;
      }

      // When a task finishes, we need to reevaluate the other commands that
      // might have been blocked.
      markFinished(finishedCmd);

      for (const Job *cmd : dependents) {
         m_deferredCommands.erase(cmd);
         noteBuilding(cmd, "because of dependencies discovered later");
         scheduleCommandIfNecessaryAndPossible(cmd);
      }

      return TaskFinishedResponse::ContinueExecution;
   }

   TaskFinishedResponse taskSignalled(ProcessId pid, StringRef errorMsg,
                                      StringRef output, StringRef errors,
                                      void *context, Optional<int> signal,
                                      TaskProcessInformation procInfo)
   {
      const Job *signalledCmd = reinterpret_cast<const Job *>(context);

      if (m_compilation.getShowDriverTimeCompilation()) {
         m_driverTimers[signalledCmd]->stopTimer();
      }

      if (m_compilation.getOutputLevel() == OutputLevel::Parseable) {
         // Parseable output was requested.
         signalledCmd->forEachContainedJobAndPID(pid, [&](const Job *job,
                                                 Job::PID pid) {
            parseableoutput::emit_signalled_message(llvm::errs(), *job, pid, errorMsg,
                                                    output, signal, procInfo);
         });
      } else {
         // Otherwise, send the buffered output to stderr, though only if we
         // support getting buffered output.
         if (TaskQueue::supportsBufferingOutput()) {
            llvm::errs() << output;
         }
      }
      if (!errorMsg.empty()) {
         m_compilation.getDiags().diagnose(SourceLoc(),
                                           diag::error_unable_to_execute_command,
                                           errorMsg);
      }


      if (signal.hasValue()) {
         m_compilation.getDiags().diagnose(SourceLoc(), diag::error_command_signalled,
                                           signalledCmd->getSource().getClassName(),
                                           signal.getValue());
      } else {
         m_compilation.getDiags()
               .diagnose(SourceLoc(),
                         diag::error_command_signalled_without_signal_number,
                         signalledCmd->getSource().getClassName());
      }

      // Since the task signalled, unconditionally set result to -2.
      m_result = -2;
      m_anyAbnormalExit = true;
      return TaskFinishedResponse::StopExecution;
   }

public:
   PerformJobsState(Compilation &compilation, std::unique_ptr<TaskQueue> &&taskQueue)
      : m_compilation(compilation), m_actualIncrementalTracer(m_compilation.getStatsReporter()),
        m_taskQueue(std::move(taskQueue))
   {
      if (m_compilation.getEnableExperimentalDependencies()) {
         m_expDepGraph.emplace(
                  m_compilation.getVerifyExperimentalDependencyGraphAfterEveryImport(),
                  m_compilation.getEmitExperimentalDependencyDotFileAfterEveryImport(),
                  m_compilation.getTraceDependencies(),
                  m_compilation.getStatsReporter()
                  );
      } else if (m_compilation.getTraceDependencies()) {
         m_incrementalTracer = &m_actualIncrementalTracer;
      }
   }

   /// Schedule and run initial, additional, and batch jobs.
   template <typename DependencyGraphT>
   void runJobs(DependencyGraphT &depGraph)
   {
      scheduleInitialJobs(depGraph);
      scheduleAdditionalJobs(depGraph);
      formBatchJobsAndAddPendingJobsToTaskQueue();
      runTaskQueueToCompletion();
      checkUnfinishedJobs(depGraph);
   }

private:
   /// Schedule all jobs we can from the initial list provided by Compilation.
   template <typename DependencyGraphT>
   void scheduleInitialJobs(DependencyGraphT &depGraph)
   {
      for (const Job *cmd : m_compilation.getJobs()) {
         if (!m_compilation.getIncrementalBuildEnabled()) {
            scheduleCommandIfNecessaryAndPossible(cmd);
            continue;
         }

         // Try to load the dependencies file for this job. If there isn't one, we
         // always have to run the job, but it doesn't affect any other jobs. If
         // there should be one but it's not present or can't be loaded, we have to
         // run all the jobs.
         // FIXME: We can probably do better here!
         Job::Condition Condition = Job::Condition::Always;
         StringRef dependenciesFile =
               cmd->getOutput().getAdditionalOutputForType(
                  filetypes::TY_PolarDeps);
         if (!dependenciesFile.empty()) {
            if (cmd->getCondition() == Job::Condition::NewlyAdded) {
               depGraph.addIndependentNode(cmd);
            } else {
               switch (
                       depGraph.loadFromPath(cmd, dependenciesFile, m_compilation.getDiags())) {
               case DependencyGraphImpl::LoadResult::HadError:
                  dependencyLoadFailed(dependenciesFile, /*warn=*/false);
                  break;
               case DependencyGraphImpl::LoadResult::UpToDate:
                  Condition = cmd->getCondition();
                  break;
               case DependencyGraphImpl::LoadResult::AffectsDownstream:
                  if (m_compilation.getEnableExperimentalDependencies()) {
                     // The experimental graph reports a change, since it lumps new
                     // files together with new "Provides".
                     Condition = cmd->getCondition();
                     break;
                  }
                  llvm_unreachable("we haven't marked anything in this graph yet");
               }
            }
         }

         switch (Condition) {
         case Job::Condition::Always:
            if (m_compilation.getIncrementalBuildEnabled() && !dependenciesFile.empty()) {
               // Ensure dependents will get recompiled.
               m_initialCascadingCommands.push_back(cmd);
               // Mark this job as cascading.
               //
               // It would probably be safe and simpler to markTransitive on the
               // start nodes in the "Always" condition from the start instead of
               // using markIntransitive and having later functions call
               // markTransitive. That way markIntransitive would be an
               // implementation detail of DependencyGraph.
               depGraph.markIntransitive(cmd);
            }
            LLVM_FALLTHROUGH;
         case Job::Condition::RunWithoutCascading:
            noteBuilding(cmd, "(initial)");
            scheduleCommandIfNecessaryAndPossible(cmd);
            break;
         case Job::Condition::CheckDependencies:
            m_deferredCommands.insert(cmd);
            break;
         case Job::Condition::NewlyAdded:
            llvm_unreachable("handled above");
         }
      }
      if (m_expDepGraph.hasValue()) {
         assert(m_expDepGraph.getValue().emitDotFileAndVerify(m_compilation.getDiags()));
      }
   }

   /// Schedule transitive closure of initial jobs, and external jobs.
   template <typename DependencyGraphT>
   void scheduleAdditionalJobs(DependencyGraphT &depGraph)
   {
      if (m_compilation.getIncrementalBuildEnabled()) {
         SmallVector<const Job *, 16> AdditionalOutOfDateCommands;

         // We scheduled all of the files that have actually changed. Now add the
         // files that haven't changed, so that they'll get built in parallel if
         // possible and after the first set of files if it's not.
         for (auto *cmd : m_initialCascadingCommands) {
            depGraph.markTransitive(AdditionalOutOfDateCommands, cmd,
                                    m_incrementalTracer);
         }

         for (auto *transitiveCmd : AdditionalOutOfDateCommands)
            noteBuilding(transitiveCmd, "because of the initial set");
         size_t firstSize = AdditionalOutOfDateCommands.size();

         // Check all cross-module dependencies as well.
         for (StringRef dependency : depGraph.getExternalDependencies()) {
            llvm::sys::fs::file_status depStatus;
            if (!llvm::sys::fs::status(dependency, depStatus)) {
               if (depStatus.getLastModificationTime() < m_compilation.getLastBuildTime()) {
                  continue;
               }
            }
            // If the dependency has been modified since the oldest built file,
            // or if we can't stat it for some reason (perhaps it's been deleted?),
            // trigger rebuilds through the dependency graph.
            depGraph.markExternal(AdditionalOutOfDateCommands, dependency);
         }

         for (auto *externalCmd :
              llvm::makeArrayRef(AdditionalOutOfDateCommands).slice(firstSize)) {
            noteBuilding(externalCmd, "because of external dependencies");
         }

         for (auto *AdditionalCmd : AdditionalOutOfDateCommands) {
            if (!m_deferredCommands.count(AdditionalCmd)) {
               continue;
            }
            scheduleCommandIfNecessaryAndPossible(AdditionalCmd);
            m_deferredCommands.erase(AdditionalCmd);
         }
      }
   }

   /// Insert all jobs in \p cmds (of descriptive name \p kind) to the \c
   /// taskQueue, and clear \p cmds.
   template <typename Container>
   void transferJobsToTaskQueue(Container &cmds, StringRef kind)
   {
      for (const Job *cmd : cmds) {
         if (m_compilation.getShowJobLifecycle()) {
            llvm::outs() << "Adding " << kind
                         << " job to task queue: "
                         << LogJob(cmd) << "\n";
         }
         addPendingJobToTaskQueue(cmd);
      }
      cmds.clear();
   }

   /// partition the jobs in \c m_pendingExecution into those that are \p
   /// batchable and those that are \p nonBatchable, clearing \p
   /// m_pendingExecution.
   void getPendingBatchableJobs(CommandSetVector &batchable,
                                CommandSetVector &nonBatchable)
   {
      for (const Job *cmd : m_pendingExecution) {
         if (m_compilation.getToolChain().jobIsBatchable(m_compilation, cmd)) {
            if (m_compilation.getShowJobLifecycle()) {
               llvm::outs() << "batchable: " << LogJob(cmd) << "\n";
            }
            batchable.insert(cmd);
         } else {
            if (m_compilation.getShowJobLifecycle()) {
               llvm::outs() << "Not batchable: " << LogJob(cmd) << "\n";
            }
            nonBatchable.insert(cmd);
         }
      }
   }

   /// If \p batch is nonempty, construct a new \c BatchJob from its
   /// contents by calling \p ToolChain::constructBatchJob, then insert the
   /// new \c BatchJob into \p batches.
   void
   formBatchJobFromPartitionBatch(std::vector<const Job *> &batches,
                                  std::vector<const Job *> const &batch)
   {
      if (batch.empty()) {
         return;
      }
      if (m_compilation.getShowJobLifecycle()) {
         llvm::outs() << "Forming batch job from "
                      << batch.size() << " constituents\n";
      }
      auto const &toolchain = m_compilation.getToolChain();
      auto job = toolchain.constructBatchJob(batch, m_nextBatchQuasiPID, m_compilation);
      if (job) {
         batches.push_back(m_compilation.addJob(std::move(job)));
      }
   }

   /// Build a vector of partition indices, one per Job: the i'th index says
   /// which batch of the partition the i'th Job will be assigned to. If we are
   /// shuffling due to -driver-batch-seed, the returned indices will not be
   /// arranged in contiguous runs. We shuffle partition-indices here, not
   /// elements themselves, to preserve the invariant that each batch is a
   /// subsequence of the full set of inputs, not just a subset.
   std::vector<size_t>
   assignJobsToPartitions(size_t partitionSize,
                          size_t numJobs)
   {
      size_t Remainder = numJobs % partitionSize;
      size_t TargetSize = numJobs / partitionSize;
      std::vector<size_t> partitionIndex;
      partitionIndex.reserve(numJobs);
      for (size_t pid = 0; pid < partitionSize; ++pid) {
         // Spread remainder evenly across partitions by adding 1 to the target
         // size of the first Remainder of them.
         size_t FillCount = TargetSize + ((pid < Remainder) ? 1 : 0);
         std::fill_n(std::back_inserter(partitionIndex), FillCount, pid);
      }
      if (m_compilation.getBatchSeed() != 0) {
         std::minstd_rand gen(m_compilation.getBatchSeed());
         std::shuffle(partitionIndex.begin(), partitionIndex.end(), gen);
      }
      assert(partitionIndex.size() == numJobs);
      return partitionIndex;
   }

   /// Create \c NumberOfParallelCommands batches and assign each job to a
   /// batch either filling each partition in order or, if seeded with a
   /// nonzero value, pseudo-randomly (but determinstically and nearly-evenly).
   void partitionIntoBatches(std::vector<const Job *> batchable,
                             BatchPartition &partition)
   {
      if (m_compilation.getShowJobLifecycle()) {
         llvm::outs() << "Found " << batchable.size() << " batchable jobs\n";
         llvm::outs() << "Forming into " << partition.size() << " batches\n";
      }

      assert(!partition.empty());
      auto partitionIndex = assignJobsToPartitions(partition.size(),
                                                   batchable.size());
      assert(partitionIndex.size() == batchable.size());
      auto const &toolchain = m_compilation.getToolChain();
      for_each(batchable, partitionIndex, [&](const Job *cmd, size_t Idx) {
         assert(Idx < partition.size());
         std::vector<const Job*> &pid = partition[Idx];
         if (pid.empty() || toolchain.jobsAreBatchCombinable(m_compilation, pid[0], cmd)) {
            if (m_compilation.getShowJobLifecycle()) {
               llvm::outs() << "Adding " << LogJob(cmd)
                            << " to batch " << Idx << '\n';
            }
            pid.push_back(cmd);
         } else {
            // Strange but theoretically possible that we have a batchable job
            // that's not combinable with others; tack a new batch on for it.
            if (m_compilation.getShowJobLifecycle())
               llvm::outs() << "Adding " << LogJob(cmd)
                            << " to new batch " << partition.size() << '\n';
            partition.push_back(std::vector<const Job*>());
            partition.back().push_back(cmd);
         }
      });
   }

   // Selects the number of partitions based on the user-provided batch
   // count and/or the number of parallel tasks we can run, subject to a
   // fixed per-batch safety cap, to avoid overcommitting memory.
   size_t pickNumberOfPartitions()
   {

      // If the user asked for something, use that.
      if (m_compilation.getBatchCount().hasValue()) {
         return m_compilation.getBatchCount().getValue();
      }

      // This is a long comment to justify a simple calculation.
      //
      // Because there is a secondary "outer" build system potentially also
      // scheduling multiple drivers in parallel on separate build targets
      // -- while we, the driver, schedule our own subprocesses -- we might
      // be creating up to $NCPU^2 worth of _memory pressure_.
      //
      // Oversubscribing CPU is typically no problem these days, but
      // oversubscribing memory can lead to paging, which on modern systems
      // is quite bad.
      //
      // In practice, $NCPU^2 processes doesn't _quite_ happen: as core
      // count rises, it usually exceeds the number of large targets
      // without any dependencies between them (which are the only thing we
      // have to worry about): you might have (say) 2 large independent
      // modules * 2 architectures, but that's only an $NTARGET value of 4,
      // which is much less than $NCPU if you're on a 24 or 36-way machine.
      //
      //  So the actual number of concurrent processes is:
      //
      //     NCONCUR := $NCPU * min($NCPU, $NTARGET)
      //
      // Empirically, a frontend uses about 512kb RAM per non-primary file
      // and about 10mb per primary. The number of non-primaries per
      // process is a constant in a given module, but the number of
      // primaries -- the "batch size" -- is inversely proportional to the
      // batch count (default: $NCPU). As a result, the memory pressure
      // we can expect is:
      //
      //  $NCONCUR * (($NONPRIMARYMEM * $NFILE) +
      //              ($PRIMARYMEM * ($NFILE/$NCPU)))
      //
      // If we tabulate this across some plausible values, we see
      // unfortunate memory-pressure results:
      //
      //                          $NFILE
      //                  +---------------------
      //  $NTARGET $NCPU  |  100    500    1000
      //  ----------------+---------------------
      //     2        2   |  2gb   11gb    22gb
      //     4        4   |  4gb   24gb    48gb
      //     4        8   |  5gb   28gb    56gb
      //     4       16   |  7gb   36gb    72gb
      //     4       36   | 11gb   56gb   112gb
      //
      // As it happens, the lower parts of the table are dominated by
      // number of processes rather than the files-per-batch (the batches
      // are already quite small due to the high core count) and the left
      // side of the table is dealing with modules too small to worry
      // about. But the middle and upper-right quadrant is problematic: 4
      // and 8 core machines do not typically have 24-48gb of RAM, it'd be
      // nice not to page on them when building a 4-target project with
      // 500-file modules.
      //
      // Turns we can do that if we just cap the batch size statically at,
      // say, 25 files per batch, we get a better formula:
      //
      //  $NCONCUR * (($NONPRIMARYMEM * $NFILE) +
      //              ($PRIMARYMEM * min(25, ($NFILE/$NCPU))))
      //
      //                          $NFILE
      //                  +---------------------
      //  $NTARGET $NCPU  |  100    500    1000
      //  ----------------+---------------------
      //     2        2   |  1gb    2gb     3gb
      //     4        4   |  4gb    8gb    12gb
      //     4        8   |  5gb   16gb    24gb
      //     4       16   |  7gb   32gb    48gb
      //     4       36   | 11gb   56gb   108gb
      //
      // This means that the "performance win" of batch mode diminishes
      // slightly: the batching factor in the equation drops from
      // ($NFILE/$NCPU) to min(25, $NFILE/$NCPU). In practice this seems to
      // not cost too much: the additional factor in number of subprocesses
      // run is the following:
      //
      //                          $NFILE
      //                  +---------------------
      //  $NTARGET $NCPU  |  100    500    1000
      //  ----------------+---------------------
      //     2        2   |  2x    10x      20x
      //     4        4   |   -     5x      10x
      //     4        8   |   -   2.5x       5x
      //     4       16   |   -  1.25x     2.5x
      //     4       36   |   -      -     1.1x
      //
      // Where - means "no difference" because the batches were already
      // smaller than 25.
      //
      // Even in the worst case here, the 1000-file module on 2-core
      // machine is being built with only 40 subprocesses, rather than the
      // pre-batch-mode 1000. I.e. it's still running 96% fewer
      // subprocesses than before. And significantly: it's doing so while
      // not exceeding the RAM of a typical 2-core laptop.

      size_t defaultSizeLimit = 25;
      size_t numTasks = m_taskQueue->getNumberOfParallelTasks();
      size_t numFiles = m_pendingExecution.size();
      size_t sizeLimit = m_compilation.getBatchSizeLimit().getValueOr(defaultSizeLimit);
      return std::max(numTasks, numFiles / sizeLimit);
   }

   /// Select jobs that are batch-combinable from \c m_pendingExecution, combine
   /// them together into \p BatchJob instances (also inserted into \p
   /// m_batchJobs), and enqueue all \c m_pendingExecution jobs (whether batched or
   /// not) into the \c taskQueue for execution.
   void formBatchJobsAndAddPendingJobsToTaskQueue()
   {

      // If batch mode is not enabled, just transfer the set of pending jobs to
      // the task queue, as-is.
      if (!m_compilation.getBatchModeEnabled()) {
         transferJobsToTaskQueue(m_pendingExecution, "standard");
         return;
      }

      size_t NumPartitions = pickNumberOfPartitions();
      CommandSetVector batchable, nonBatchable;
      std::vector<const Job *> batches;

      // Split the batchable from non-batchable pending jobs.
      getPendingBatchableJobs(batchable, nonBatchable);

      // partition the batchable jobs into sets.
      BatchPartition partition(NumPartitions);
      partitionIntoBatches(batchable.takeVector(), partition);

      // Construct a BatchJob from each batch in the partition.
      for (auto const &batch : partition) {
         formBatchJobFromPartitionBatch(batches, batch);
      }

      m_pendingExecution.clear();

      // Save batches so we can locate and decompose them on task-exit.
      for (const Job *cmd : batches) {
         m_batchJobs.insert(cmd);
      }
      // Enqueue the resulting jobs, batched and non-batched alike.
      transferJobsToTaskQueue(batches, "batch");
      transferJobsToTaskQueue(nonBatchable, "non-batch");
   }

   void runTaskQueueToCompletion()
   {
      do {
         using namespace std::placeholders;
         // Ask the taskQueue to execute.
         if (m_taskQueue->execute(std::bind(&PerformJobsState::taskBegan, this, _1, _2),
                                  std::bind(&PerformJobsState::taskFinished, this, _1, _2,
                                            _3, _4, _5, _6),
                                  std::bind(&PerformJobsState::taskSignalled, this, _1,
                                            _2, _3, _4, _5, _6, _7))) {
            if (m_result == EXIT_SUCCESS) {
               // FIXME: Error from task queue while m_result == EXIT_SUCCESS most
               // likely means some fork/exec or posix_spawn failed; taskQueue saw
               // "an error" at some stage before even calling us with a process
               // exit / signal (or else a poll failed); unfortunately the task
               // causing it was dropped on the floor and we have no way to recover
               // it here, so we report a very poor, generic error.
               m_compilation.getDiags().diagnose(SourceLoc(),
                                                 diag::error_unable_to_execute_command,
                                                 "<unknown>");
               m_result = -2;
               m_anyAbnormalExit = true;
               return;
            }
         }

         // Returning without error from taskQueue::execute should mean either an
         // empty taskQueue or a failed subprocess.
         assert(!(m_result == 0 && m_taskQueue->hasRemainingTasks()));

         // Task-exit callbacks from taskQueue::execute may have unblocked jobs,
         // which means there might be m_pendingExecution jobs to enqueue here. If
         // there are, we need to continue trying to make progress on the
         // taskQueue before we start marking deferred jobs as skipped, below.
         if (!m_pendingExecution.empty() && m_result == 0) {
            formBatchJobsAndAddPendingJobsToTaskQueue();
            continue;
         }

         // If we got here, all the queued and pending work we know about is
         // done; mark anything still in deferred state as skipped.
         for (const Job *cmd : m_deferredCommands) {
            if (m_compilation.getOutputLevel() == OutputLevel::Parseable) {
               // Provide output indicating this command was skipped if parseable
               // output was requested.
               parseableoutput::emit_skipped_message(llvm::errs(), *cmd);
            }
            m_scheduledCommands.insert(cmd);
            markFinished(cmd, /*Skipped=*/true);
         }
         m_deferredCommands.clear();

         // It's possible that by marking some jobs as skipped, we unblocked
         // some jobs and thus have entries in m_pendingExecution again; push
         // those through to the taskQueue.
         formBatchJobsAndAddPendingJobsToTaskQueue();

         // If we added jobs to the taskQueue, and we are not in an error state,
         // we want to give the taskQueue another run.
      } while (m_result == 0 && m_taskQueue->hasRemainingTasks());
   }

   template <typename DependencyGraphT>
   void checkUnfinishedJobs(DependencyGraphT &depGraph)
   {
      if (m_result == 0) {
         assert(m_blockingCommands.empty() &&
                "some blocking commands never finished properly");
      } else {
         // Make sure we record any files that still need to be rebuilt.
         for (const Job *cmd : m_compilation.getJobs()) {
            // Skip files that don't use dependency analysis.
            StringRef dependenciesFile =
                  cmd->getOutput().getAdditionalOutputForType(
                     filetypes::TY_PolarDeps);
            if (dependenciesFile.empty()) {
               continue;
            }
            // Don't worry about commands that finished or weren't going to run.
            if (m_finishedCommands.count(cmd)) {
               continue;
            }
            if (!m_scheduledCommands.count(cmd)) {
               continue;
            }
            bool isCascading = true;
            if (m_compilation.getIncrementalBuildEnabled()) {
               isCascading = depGraph.isMarked(cmd);
            }
            m_unfinishedCommands.insert({cmd, isCascading});
         }
      }
   }

public:
   void populateInputInfoMap(InputInfoMap &inputs) const
   {
      for (auto &entry : m_unfinishedCommands) {
         for (auto *action : entry.first->getSource().getInputs()) {
            auto inputFile = dyn_cast<InputAction>(action);
            if (!inputFile) {
               continue;
            }
            CompileJobAction::InputInfo info;
            info.previousModTime = entry.first->getInputModTime();
            info.status = entry.second ?
                     CompileJobAction::InputInfo::NeedsCascadingBuild :
                     CompileJobAction::InputInfo::NeedsNonCascadingBuild;
            inputs[&inputFile->getInputArg()] = info;
         }
      }

      for (const Job *entry : m_finishedCommands) {
         const auto *compileAction = dyn_cast<CompileJobAction>(&entry->getSource());
         if (!compileAction) {
            continue;
         }
         for (auto *action : compileAction->getInputs()) {
            auto inputFile = dyn_cast<InputAction>(action);
            if (!inputFile) {
               continue;
            }
            CompileJobAction::InputInfo info;
            info.previousModTime = entry->getInputModTime();
            info.status = CompileJobAction::InputInfo::UpToDate;
            inputs[&inputFile->getInputArg()] = info;
         }
      }

      // Sort the entries by input order.
      static_assert(std::is_trivially_copyable<CompileJobAction::InputInfo>::value,
                    "llvm::array_pod_sort relies on trivially-copyable data");
      using InputInfoEntry = std::decay<decltype(inputs.front())>::type;
      llvm::array_pod_sort(inputs.begin(), inputs.end(),
                           [](const InputInfoEntry *lhs,
                           const InputInfoEntry *rhs) -> int {
         auto lhsIndex = lhs->first->getIndex();
         auto rhsIndex = rhs->first->getIndex();
         return (lhsIndex < rhsIndex) ? -1 : (lhsIndex > rhsIndex) ? 1 : 0;
      });
   }

   int getResult()
   {
      if (m_result == 0) {
         m_result = m_compilation.getDiags().hadAnyError();
      }
      return m_result;
   }

   bool hadAnyAbnormalExit()
   {
      return m_anyAbnormalExit;
   }

private:
   /// The containing Compilation object.
   Compilation &m_compilation;

   /// All jobs which have been scheduled for execution (whether or not
   /// they've finished execution), or which have been determined that they
   /// don't need to run.
   CommandSet m_scheduledCommands;

   /// A temporary buffer to hold commands that were scheduled but haven't been
   /// added to the Task Queue yet, because we might try batching them together
   /// first.
   CommandSetVector m_pendingExecution;

   /// Set of synthetic m_batchJobs that serve to cluster subsets of jobs waiting
   /// in m_pendingExecution. Also used to identify (then unpack) m_batchJobs back
   /// to their underlying non-batch Jobs, when running a callback from
   /// taskQueue.
   CommandSet m_batchJobs;

   /// Persistent counter for allocating quasi-PIDs to Jobs combined into
   /// m_batchJobs. Quasi-PIDs are _negative_ PID-like unique keys used to
   /// masquerade BatchJob constituents as (quasi)processes, when writing
   /// parseable output to consumers that don't understand the idea of a batch
   /// job. They are negative in order to avoid possibly colliding with real
   /// PIDs (which are always positive). We start at -1000 here as a crude but
   /// harmless hedge against colliding with an errno value that might slip
   /// into the stream of real PIDs (say, due to a taskQueue bug).
   int64_t m_nextBatchQuasiPID = -1000;

   /// All jobs which have finished execution or which have been determined
   /// that they don't need to run.
   CommandSet m_finishedCommands;

   /// A map from a Job to the commands it is known to be blocking.
   ///
   /// The blocked jobs should be scheduled as soon as possible.
   llvm::SmallDenseMap<const Job *, TinyPtrVector<const Job *>, 16>
   m_blockingCommands;

   /// A map from commands that didn't get to run to whether or not they affect
   /// downstream commands.
   ///
   /// Only intended for source files.
   llvm::SmallDenseMap<const Job *, bool, 16> m_unfinishedCommands;

   /// Jobs that incremental-mode has decided it can skip.
   CommandSet m_deferredCommands;

   /// Jobs in the initial set with Condition::Always, and having an existing
   /// .swiftdeps files.
   /// Set by scheduleInitialJobs and used only by scheduleAdditionalJobs.
   SmallVector<const Job *, 16> m_initialCascadingCommands;

   /// Helper for tracing the propagation of marks in the graph.
   JobDependencyGraph::MarkTracer m_actualIncrementalTracer;
   JobDependencyGraph::MarkTracer *m_incrementalTracer = nullptr;

   /// taskQueue for execution.
   std::unique_ptr<TaskQueue> m_taskQueue;

   /// Cumulative result of PerformJobs(), accumulated from subprocesses.
   int m_result = EXIT_SUCCESS;

   /// True if any Job crashed.
   bool m_anyAbnormalExit = false;

   /// Timers for monitoring execution time of subprocesses.
   llvm::TimerGroup m_driverTimerGroup {"driver", "Driver Compilation Time"};
   llvm::SmallDenseMap<const Job *, std::unique_ptr<llvm::Timer>, 16>
   m_driverTimers;
};

Compilation::~Compilation() = default;

Job *Compilation::addJob(std::unique_ptr<Job> job)
{
   Job *result = job.get();
   m_jobs.emplace_back(std::move(job));
   return result;
}

static void checkForOutOfDateInputs(DiagnosticEngine &diags,
                                    const InputInfoMap &inputs)
{
   for (const auto &inputPair : inputs) {
      auto recordedModTime = inputPair.second.previousModTime;
      if (recordedModTime == llvm::sys::TimePoint<>::max()) {
         continue;
      }
      const char *input = inputPair.first->getValue();
      llvm::sys::fs::file_status inputStatus;
      if (auto statError = llvm::sys::fs::status(input, inputStatus)) {
         diags.diagnose(SourceLoc(), diag::warn_cannot_stat_input,
                        llvm::sys::path::filename(input), statError.message());
         continue;
      }
      if (recordedModTime != inputStatus.getLastModificationTime()) {
         diags.diagnose(SourceLoc(), diag::error_input_changed_during_build,
                        llvm::sys::path::filename(input));
      }
   }
}

static void writeCompilationRecord(StringRef path, StringRef argsHash,
                                   llvm::sys::TimePoint<> buildTime,
                                   const InputInfoMap &inputs)
{
   // Before writing to the dependencies file path, preserve any previous file
   // that may have been there. No error handling -- this is just a nicety, it
   // doesn't matter if it fails.
   llvm::sys::fs::rename(path, path + "~");

   std::error_code error;
   llvm::raw_fd_ostream out(path, error, llvm::sys::fs::F_None);
   if (out.has_error()) {
      // FIXME: How should we report this error?
      out.clear_error();
      return;
   }

   auto writeTimeValue = [](llvm::raw_ostream &out,
         llvm::sys::TimePoint<> time) {
      using namespace std::chrono;
      auto secs = time_point_cast<seconds>(time);
      time -= secs.time_since_epoch(); // remainder in nanoseconds
      out << "[" << secs.time_since_epoch().count()
          << ", " << time.time_since_epoch().count() << "]";
   };

   using compilationrecord::TopLevelKey;
   // NB: We calculate effective version from getCurrentLanguageVersion()
   // here because any -swift-version argument is handled in the
   // argsHash that follows.
   out << compilationrecord::get_name(TopLevelKey::Version) << ": \""
       << llvm::yaml::escape(version::retrieve_polarphp_full_version(
                                polar::version::Version::getCurrentLanguageVersion()))
       << "\"\n";
   out << compilationrecord::get_name(TopLevelKey::Options) << ": \""
       << llvm::yaml::escape(argsHash) << "\"\n";
   out << compilationrecord::get_name(TopLevelKey::BuildTime) << ": ";
   writeTimeValue(out, buildTime);
   out << "\n";
   out << compilationrecord::get_name(TopLevelKey::Inputs) << ":\n";

   for (auto &entry : inputs) {
      out << "  \"" << llvm::yaml::escape(entry.first->getValue()) << "\": ";

      using compilationrecord::get_identifier_for_input_info_status;
      auto name = get_identifier_for_input_info_status(entry.second.status);
      if (!name.empty()) {
         out << name << " ";
      }

      writeTimeValue(out, entry.second.previousModTime);
      out << "\n";
   }
}

static bool write_filelist_if_necessary(const Job *job, const ArgList &args,
                                        DiagnosticEngine &diags)
{
   bool ok = true;
   for (const FilelistInfo &filelistInfo : job->getFilelistInfos()) {
      if (filelistInfo.path.empty()) {
         return true;
      }


      std::error_code error;
      llvm::raw_fd_ostream out(filelistInfo.path, error, llvm::sys::fs::F_None);
      if (out.has_error()) {
         out.clear_error();
         diags.diagnose(SourceLoc(), diag::error_unable_to_make_temporary_file,
                        error.message());
         ok = false;
         continue;
      }

      switch (filelistInfo.whichFiles) {
      case FilelistInfo::WhichFiles::Input:
         // FIXME: Duplicated from ToolChains.cpp.
         for (const Job *input : job->getInputs()) {
            const CommandOutput &outputInfo = input->getOutput();
            if (outputInfo.getPrimaryOutputType() == filelistInfo.type) {
               for (auto &output : outputInfo.getPrimaryOutputFilenames()) {
                  out << output << "\n";
               }
            } else {
               auto output = outputInfo.getAnyOutputForType(filelistInfo.type);
               if (!output.empty()) {
                  out << output << "\n";
               }
            }
         }
         break;
      case FilelistInfo::WhichFiles::PrimaryInputs:
         // Ensure that -index-file-path works in conjunction with
         // -driver-use-filelists. It needs to be the only primary.
         if (Arg *A = args.getLastArg(options::OPT_index_file_path)) {
            out << A->getValue() << "\n";
         } else {
            // The normal case for non-single-compile jobs.
            for (const Action *A : job->getSource().getInputs()) {
               // A could be a GeneratePCHJobAction
               if (!isa<InputAction>(A))
                  continue;
               const auto *IA = cast<InputAction>(A);
               out << IA->getInputArg().getValue() << "\n";
            }
         }
         break;
      case FilelistInfo::WhichFiles::Output: {
         const CommandOutput &outputInfo = job->getOutput();
         assert(outputInfo.getPrimaryOutputType() == filelistInfo.type);
         for (auto &output : outputInfo.getPrimaryOutputFilenames())
            out << output << "\n";
         break;
      }
      case FilelistInfo::WhichFiles::SupplementaryOutput:
         job->getOutput().writeOutputFileMap(out);
         break;
      }
   }
   return ok;
}

int Compilation::performJobsImpl(bool &abnormalExit,
                                 std::unique_ptr<TaskQueue> &&taskQueue)
{
   PerformJobsState state(*this, std::move(taskQueue));
   if (getEnableExperimentalDependencies()) {
      state.runJobs(state.m_expDepGraph.getValue());
   } else {
      state.runJobs(state.m_standardDepGraph);
   }
   if (!m_compilationRecordPath.empty()) {
      InputInfoMap inputInfo;
      state.populateInputInfoMap(inputInfo);
      checkForOutOfDateInputs(m_diags, inputInfo);
      writeCompilationRecord(m_compilationRecordPath, m_argsHash, m_buildStartTime,
                             inputInfo);

      if (m_outputCompilationRecordForModuleOnlyBuild) {
         // TODO: Optimize with clonefile(2) ?
         llvm::sys::fs::copy_file(m_compilationRecordPath,
                                  m_compilationRecordPath + "~moduleonly");
      }
   }
   if (state.m_expDepGraph.hasValue()) {
      assert(state.m_expDepGraph.getValue().emitDotFileAndVerify(getDiags()));
   }
   abnormalExit = state.hadAnyAbnormalExit();
   return state.getResult();
}

int Compilation::performSingleCommand(const Job *cmd)
{
   assert(cmd->getInputs().empty() &&
          "This can only be used to run a single command with no inputs");

   switch (cmd->getCondition()) {
   case Job::Condition::CheckDependencies:
      return 0;
   case Job::Condition::RunWithoutCascading:
   case Job::Condition::Always:
   case Job::Condition::NewlyAdded:
      break;
   }

   if (!write_filelist_if_necessary(cmd, *m_translatedArgs.get(), m_diags))
      return 1;

   switch (m_level) {
   case OutputLevel::Normal:
   case OutputLevel::Parseable:
      break;
   case OutputLevel::PrintJobs:
      cmd->printCommandLineAndEnvironment(llvm::outs());
      return 0;
   case OutputLevel::Verbose:
      cmd->printCommandLine(llvm::errs());
      break;
   }

   SmallVector<const char *, 128> argv;
   argv.push_back(cmd->getExecutable());
   argv.append(cmd->getArguments().begin(), cmd->getArguments().end());
   argv.push_back(nullptr);

   const char *execPath = cmd->getExecutable();

   for (auto &envPair : cmd->getExtraEnvironment()) {
#if defined(_MSC_VER)
      int envResult =_putenv_s(envPair.first, envPair.second);
#else
      int envResult = setenv(envPair.first, envPair.second, /*replacing=*/true);
#endif
      assert(envResult == 0 &&
             "expected environment variable to be set successfully");
      // Bail out early in release builds.
      if (envResult != 0) {
         return envResult;
      }
   }

   return execute_in_place(execPath, argv.data());
}

static bool write_all_sources_file(DiagnosticEngine &diags, StringRef path,
                                   ArrayRef<InputPair> inputFiles)
{
   std::error_code error;
   llvm::raw_fd_ostream out(path, error, llvm::sys::fs::F_None);
   if (out.has_error()) {
      out.clear_error();
      diags.diagnose(SourceLoc(), diag::error_unable_to_make_temporary_file,
                     error.message());
      return false;
   }

   for (auto inputPair : inputFiles) {
      if (!filetypes::is_part_of_polarphp_compilation(inputPair.first)) {
         continue;
      }
      out << inputPair.second->getValue() << "\n";
   }

   return true;
}

int Compilation::performJobs(std::unique_ptr<TaskQueue> &&taskQueue)
{
   if (m_allSourceFilesPath) {
      if (!write_all_sources_file(m_diags, m_allSourceFilesPath, getInputFiles())) {
         return EXIT_FAILURE;
      }
   }

   // If we don't have to do any cleanup work, just exec the subprocess.
   if (m_level < OutputLevel::Parseable &&
       !m_showDriverTimeCompilation &&
       (m_saveTemps || m_tempFilePaths.empty()) &&
       m_compilationRecordPath.empty() &&
       m_jobs.size() == 1) {
      return performSingleCommand(m_jobs.front().get());
   }

   if (!TaskQueue::supportsParallelExecution() && taskQueue->getNumberOfParallelTasks() > 1) {
      m_diags.diagnose(SourceLoc(), diag::warning_parallel_execution_not_supported);
   }

   bool abnormalExit;
   int result = performJobsImpl(abnormalExit, std::move(taskQueue));

   if (!m_saveTemps) {
      for (const auto &pathPair : m_tempFilePaths) {
         if (!abnormalExit || pathPair.getValue() == PreserveOnSignal::No) {
            (void)llvm::sys::fs::remove(pathPair.getKey());
         }
      }
   }
   if (m_stats) {
      m_stats->noteCurrentProcessExitStatus(result);
   }
   return result;
}

const char *Compilation::getAllSourcesPath() const
{
   if (!m_allSourceFilesPath) {
      SmallString<128> Buffer;
      std::error_code errorCode =
            llvm::sys::fs::createTemporaryFile("sources", "", Buffer);
      if (errorCode) {
         m_diags.diagnose(SourceLoc(),
                          diag::error_unable_to_make_temporary_file,
                          errorCode.message());
         // FIXME: This should not take down the entire process.
         llvm::report_fatal_error("unable to create list of input sources");
      }
      auto *mutableThis = const_cast<Compilation *>(this);
      mutableThis->addTemporaryFile(Buffer.str(), PreserveOnSignal::Yes);
      mutableThis->m_allSourceFilesPath = getArgs().MakeArgString(Buffer);
   }
   return m_allSourceFilesPath;
}

} // polar::driver
