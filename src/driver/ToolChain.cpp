//===--- ToolChain.cpp - Collections of tools for one platform ------------===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/03.
//
/// \file This file defines the base implementation of the ToolChain class.
/// The platform-specific subclasses are implemented in ToolChains.cpp.
/// For organizational purposes, the platform-independent logic for
/// constructing job invocations is also located in ToolChains.cpp.
//
//===----------------------------------------------------------------------===//

#include "polarphp/driver/ToolChain.h"
#include "polarphp/driver/Compilation.h"
#include "polarphp/driver/Driver.h"
#include "polarphp/driver/Job.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"

namespace polar::driver {

using namespace llvm::opt;

ToolChain::JobContext::JobContext(Compilation &compilation, ArrayRef<const Job *> inputs,
                                  ArrayRef<const Action *> inputActions,
                                  const CommandOutput &output,
                                  const OutputInfo &outputInfo)
   : m_compilation(compilation),
     inputs(inputs),
     inputActions(inputActions),
     output(output),
     outputInfo(outputInfo),
     args(compilation.getArgs())
{}

ArrayRef<InputPair> ToolChain::JobContext::getTopLevelInputFiles() const
{
   return m_compilation.getInputFiles();
}

const char *ToolChain::JobContext::getAllSourcesPath() const
{
   return m_compilation.getAllSourcesPath();
}

const char *
ToolChain::JobContext::getTemporaryFilePath(const llvm::Twine &name,
                                            StringRef suffix) const
{
   SmallString<128> buffer;
   std::error_code errorCode = llvm::sys::fs::createTemporaryFile(name, suffix, buffer);
   if (errorCode) {
      // FIXME: This should not take down the entire process.
      llvm::report_fatal_error("unable to create temporary file for filelist");
   }
   m_compilation.addTemporaryFile(buffer.str(), PreserveOnSignal::Yes);
   // We can't just reference the data in the TemporaryFiles vector because
   // that could theoretically get copied to a new address.
   return m_compilation.getArgs().MakeArgString(buffer.str());
}

Optional<Job::ResponseFileInfo>
ToolChain::getResponseFileInfo(const Compilation &compilation, const char *executablePath,
                               const ToolChain::InvocationInfo &invocationInfo,
                               const ToolChain::JobContext &context) const
{
   const bool forceResponseFiles =
         compilation.getArgs().hasArg(options::OPT_driver_force_response_files);
   assert((invocationInfo.allowsResponseFiles || !forceResponseFiles) &&
          "Cannot force response file if platform does not allow it");

   if (forceResponseFiles || (invocationInfo.allowsResponseFiles &&
                              !llvm::sys::commandLineFitsWithinSystemLimits(
                                 executablePath, invocationInfo.arguments))) {
      const char *responseFilePath =
            context.getTemporaryFilePath("arguments", "resp");
      const char *responseFileArg =
            compilation.getArgs().MakeArgString(Twine("@") + responseFilePath);
      return {{responseFilePath, responseFileArg}};
   }
   return None;
}

std::unique_ptr<Job> ToolChain::constructJob(
      const JobAction &jobAction, Compilation &compilation, SmallVectorImpl<const Job *> &&inputs,
      ArrayRef<const Action *> inputActions,
      std::unique_ptr<CommandOutput> output, const OutputInfo &outputInfo) const
{
   JobContext context{compilation, inputs, inputActions, *output, outputInfo};
   auto invocationInfo = [&]() -> InvocationInfo {
         switch (jobAction.getKind()) {
      #define CASE(K)                                                                \
         case Action::Kind::K:                                                        \
            return constructInvocation(cast<K##Action>(jobAction), context);
            CASE(CompileJob)
            CASE(InterpretJob)
            CASE(BackendJob)
            CASE(MergeModuleJob)
            CASE(ModuleWrapJob)
            CASE(DynamicLinkJob)
            CASE(StaticLinkJob)
            CASE(GenerateDSYMJob)
            CASE(VerifyDebugInfoJob)
            CASE(GeneratePCHJob)
            CASE(AutolinkExtractJob)
            CASE(REPLJob)
         #undef CASE
         case Action::Kind::Input:
          llvm_unreachable("not a JobAction");
         }
         // Work around MSVC warning: not all control paths return a value
         llvm_unreachable("All switch cases are covered");
   }();

   // Special-case the Swift frontend.
   const char *executablePath = nullptr;
   if (StringRef(POLARPHP_EXECUTABLE_NAME) == invocationInfo.executableName) {
      executablePath = getDriver().getPolarphpProgramPath().c_str();
   } else {
      std::string relativePath =
            findProgramRelativeToSwift(invocationInfo.executableName);
      if (!relativePath.empty()) {
         executablePath = compilation.getArgs().MakeArgString(relativePath);
      } else {
         auto systemPath =
               llvm::sys::findProgramByName(invocationInfo.executableName);
         if (systemPath) {
            executablePath = compilation.getArgs().MakeArgString(systemPath.get());
         } else {
            // For debugging purposes.
            executablePath = invocationInfo.executableName;
         }
      }
   }

   // Determine if the argument list is so long that it needs to be written into
   // a response file.
   auto responseFileInfo =
         getResponseFileInfo(compilation, executablePath, invocationInfo, context);

   return std::make_unique<Job>(
            jobAction, std::move(inputs), std::move(output), executablePath,
            std::move(invocationInfo.arguments),
            std::move(invocationInfo.extraEnvironment),
            std::move(invocationInfo.filelistInfos), responseFileInfo);
}

std::string
ToolChain::findProgramRelativeToSwift(StringRef executableName) const
{
   auto insertionResult =
         m_programLookupCache.insert(std::make_pair(executableName, ""));
   if (insertionResult.second) {
      std::string path = findProgramRelativeToPolarphpImpl(executableName);
      insertionResult.first->setValue(std::move(path));
   }
   return insertionResult.first->getValue();
}

std::string
ToolChain::findProgramRelativeToPolarphpImpl(StringRef executableName) const
{
   StringRef polarphpPath = getDriver().getPolarphpProgramPath();
   StringRef polarphpBinDir = llvm::sys::path::parent_path(polarphpPath);
   auto result = llvm::sys::findProgramByName(executableName, {polarphpBinDir});
   if (result) {
      return result.get();
   }
   return {};
}

filetypes::FileTypeId ToolChain::lookupTypeForExtension(StringRef ext) const
{
   return filetypes::lookup_type_for_extension(ext);
}

/// Return a _single_ TY_Swift InputAction, if one exists;
/// if 0 or >1 such inputs exist, return nullptr.
static const InputAction *find_single_polarphp_input(const CompileJobAction *cja)
{
   auto inputs = cja->getInputs();
   const InputAction *inputAction = nullptr;
   for (auto const *input : inputs) {
      if (auto const *S = dyn_cast<InputAction>(input)) {
         if (S->getType() == filetypes::TY_Polar) {
            if (inputAction == nullptr) {
               inputAction = S;
            } else {
               // Already found one, two is too many.
               return nullptr;
            }
         }
      }
   }
   return inputAction;
}

static bool jobs_have_same_executable_names(const Job *lhs, const Job *rhs)
{
   // Jobs that get here (that are derived from CompileJobActions) should always
   // have the same executable name -- it should always be SWIFT_EXECUTABLE_NAME
   // -- but we check here just to be sure / fail gracefully in non-assert
   // builds.
   assert(strcmp(lhs->getExecutable(), rhs->getExecutable()) == 0);
   if (strcmp(lhs->getExecutable(), rhs->getExecutable()) != 0) {
      return false;
   }
   return true;
}

static bool jobs_have_same_output_types(const Job *lhs, const Job *rhs)
{
   if (lhs->getOutput().getPrimaryOutputType() !=
       rhs->getOutput().getPrimaryOutputType()) {
      return false;
   }
   return lhs->getOutput().hasSameAdditionalOutputTypes(rhs->getOutput());
}

static bool jobs_have_same_environment(const Job *lhs, const Job *rhs)
{
   auto lhsEnv = lhs->getExtraEnvironment();
   auto rhsEnv = rhs->getExtraEnvironment();
   if (lhsEnv.size() != rhsEnv.size()) {
      return false;
   }
   for (size_t i = 0; i < lhsEnv.size(); ++i) {
      if (strcmp(lhsEnv[i].first, rhsEnv[i].first) != 0) {
         return false;
      }
      if (strcmp(lhsEnv[i].second, rhsEnv[i].second) != 0) {
         return false;
      }
   }
   return true;
}

bool ToolChain::jobIsBatchable(const Compilation &compilation, const Job *job) const
{
   // FIXME: There might be a tighter criterion to use here?
   if (compilation.getOutputInfo().compilerMode != OutputInfo::Mode::StandardCompile) {
      return false;
   }
   auto const *cjActA = dyn_cast<const CompileJobAction>(&job->getSource());
   if (!cjActA) {
      return false;
   }
   return find_single_polarphp_input(cjActA) != nullptr;
}

bool ToolChain::jobsAreBatchCombinable(const Compilation &compilation, const Job *lhs,
                                       const Job *rhs) const
{
   assert(jobIsBatchable(compilation, lhs));
   assert(jobIsBatchable(compilation, rhs));
   return (jobs_have_same_executable_names(lhs, rhs) && jobs_have_same_output_types(lhs, rhs) &&
           jobs_have_same_environment(lhs, rhs));
}

/// Form a synthetic \c CommandOutput for a \c BatchJob by merging together the
/// \c CommandOutputs of all the jobs passed.
static std::unique_ptr<CommandOutput>
make_batch_command_output(ArrayRef<const Job *> jobs, Compilation &compilation,
                       filetypes::FileTypeId outputType)
{
   auto output =
         std::make_unique<CommandOutput>(outputType, compilation.getDerivedOutputFileMap());
   for (auto const *job : jobs) {
      output->addOutputs(job->getOutput());
   }
   return output;
}

/// Set-union the \c inputs and \c inputActions from each \c Job in \p jobs into
/// the provided \p inputJobs and \p inputActions vectors, further adding all \c
/// Actions in the \p jobs -- inputActions or otherwise -- to \p batchCJA. Do
/// set-union rather than concatenation here to avoid mentioning the same input
/// multiple times.
static bool
merge_batch_inputs(ArrayRef<const Job *> jobs,
                   llvm::SmallSetVector<const Job *, 16> &inputJobs,
                   llvm::SmallSetVector<const Action *, 16> &inputActions,
                   CompileJobAction *batchCJA)
{

   llvm::SmallSetVector<const Action *, 16> allActions;

   for (auto const *job : jobs) {
      for (auto const *input : job->getInputs()) {
         inputJobs.insert(input);
      }
      auto const *cja = dyn_cast<CompileJobAction>(&job->getSource());
      if (!cja) {
         return true;
      }
      for (auto const *input : cja->getInputs()) {
         // Capture _all_ input actions -- whether or not they are inputActions --
         // in allActions, to set as the inputs for batchCJA below.
         allActions.insert(input);
         // Only collect input actions that _are inputActions_ in the inputActions
         // array, to load into the JobContext in our caller.
         if (auto const *inputAction = dyn_cast<InputAction>(input)) {
            inputActions.insert(inputAction);
         }
      }
   }

   for (auto const *input : allActions) {
      batchCJA->addInput(input);
   }
   return false;
}

/// Unfortunately the success or failure of a Swift compilation is currently
/// sensitive to the order in which files are processed, at least in terms of
/// the order of processing extensions (and likely other ways we haven't
/// discovered yet). So long as this is true, we need to make sure any batch job
/// we build names its inputs in an order that's a subsequence of the sequence
/// of inputs the driver was initially invoked with.
static void
sort_jobs_to_match_compilation_inputs(ArrayRef<const Job *> unsortedJobs,
                                      SmallVectorImpl<const Job *> &sortedJobs,
                                      Compilation &compilation)
{
   llvm::DenseMap<StringRef, const Job *> jobsByInput;
   for (const Job *job : unsortedJobs) {
      const CompileJobAction *cja = cast<CompileJobAction>(&job->getSource());
      const InputAction *inputAction = find_single_polarphp_input(cja);
      auto R =
            jobsByInput.insert(std::make_pair(inputAction->getInputArg().getValue(), job));
      assert(R.second);
      (void)R;
   }
   for (const InputPair &P : compilation.getInputFiles()) {
      auto input = jobsByInput.find(P.second->getValue());
      if (input != jobsByInput.end()) {
         sortedJobs.push_back(input->second);
      }
   }
}

/// Construct a \c BatchJob by merging the constituent \p jobs' CommandOutput,
/// input \c Job and \c Action members. Call through to \c constructInvocation
/// on \p BatchJob, to build the \c InvocationInfo.
std::unique_ptr<Job>
ToolChain::constructBatchJob(ArrayRef<const Job *> unsortedJobs,
                             Job::PID &nextQuasiPID,
                             Compilation &compilation) const {
   if (unsortedJobs.empty())
      return nullptr;

   llvm::SmallVector<const Job *, 16> sortedJobs;
   sort_jobs_to_match_compilation_inputs(unsortedJobs, sortedJobs, compilation);

   // Synthetic OutputInfo is a slightly-modified version of the initial
   // compilation's outputInfo.
   auto outputInfo = compilation.getOutputInfo();
   outputInfo.compilerMode = OutputInfo::Mode::BatchModeCompile;

   auto const *executablePath = sortedJobs[0]->getExecutable();
   auto outputType = sortedJobs[0]->getOutput().getPrimaryOutputType();
   auto output = make_batch_command_output(sortedJobs, compilation, outputType);

   llvm::SmallSetVector<const Job *, 16> inputJobs;
   llvm::SmallSetVector<const Action *, 16> inputActions;
   auto *batchCJA = compilation.createAction<CompileJobAction>(outputType);
   if (merge_batch_inputs(sortedJobs, inputJobs, inputActions, batchCJA)) {
      return nullptr;
   }

   JobContext context{compilation, inputJobs.getArrayRef(), inputActions.getArrayRef(),
            *output, outputInfo};
   auto invocationInfo = constructInvocation(*batchCJA, context);
   // Batch mode can produce quite long command lines; in almost every case these
   // will trigger use of supplementary output file maps. However, if the driver
   // command line is long for reasons unrelated to the number of input files,
   // such as passing a large number of flags, then the individual batch jobs are
   // also likely to overflow. We have to check for that explicitly here, because
   // the BatchJob created here does not go through the same code path in
   // constructJob above.
   //
   // The `allowsResponseFiles` flag on the `invocationInfo` we have here exists
   // only to model external tools that don't know about response files, such as
   // platform linkers; when talking to the frontend (which we control!) it
   // should always be true. But double check with an assert here in case someone
   // failed to set it in `constructInvocation`.
   assert(invocationInfo.allowsResponseFiles);
   auto responseFileInfo =
         getResponseFileInfo(compilation, executablePath, invocationInfo, context);

   return std::make_unique<BatchJob>(
            *batchCJA, inputJobs.takeVector(), std::move(output), executablePath,
            std::move(invocationInfo.arguments),
            std::move(invocationInfo.extraEnvironment),
            std::move(invocationInfo.filelistInfos), sortedJobs, nextQuasiPID,
            responseFileInfo);
}

} // polar::basic
