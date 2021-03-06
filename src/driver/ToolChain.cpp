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

using namespace polar;
using namespace polar::driver;
using namespace llvm::opt;

ToolChain::JobContext::JobContext(Compilation &C, ArrayRef<const Job *> Inputs,
                                  ArrayRef<const Action *> InputActions,
                                  const CommandOutput &Output,
                                  const OutputInfo &OI)
   : C(C), Inputs(Inputs), InputActions(InputActions), Output(Output), OI(OI),
     Args(C.getArgs()) {}

ArrayRef<InputPair> ToolChain::JobContext::getTopLevelInputFiles() const {
   return C.getInputFiles();
}
const char *ToolChain::JobContext::getAllSourcesPath() const {
   return C.getAllSourcesPath();
}

const char *
ToolChain::JobContext::getTemporaryFilePath(const llvm::Twine &name,
                                            StringRef suffix) const {
   SmallString<128> buffer;
   std::error_code EC = llvm::sys::fs::createTemporaryFile(name, suffix, buffer);
   if (EC) {
      // Use the constructor that prints both the error code and the description.
      // FIXME: This should not take down the entire process.
      auto error = llvm::make_error<llvm::StringError>(
         EC,
         "- unable to create temporary file for " + name + "." + suffix);
      llvm::report_fatal_error(std::move(error));
   }

   C.addTemporaryFile(buffer.str(), PreserveOnSignal::Yes);
   // We can't just reference the data in the TemporaryFiles vector because
   // that could theoretically get copied to a new address.
   return C.getArgs().MakeArgString(buffer.str());
}

Optional<Job::ResponseFileInfo>
ToolChain::getResponseFileInfo(const Compilation &C, const char *executablePath,
                               const ToolChain::InvocationInfo &invocationInfo,
                               const ToolChain::JobContext &context) const {
   const bool forceResponseFiles =
      C.getArgs().hasArg(options::OPT_driver_force_response_files);
   assert((invocationInfo.allowsResponseFiles || !forceResponseFiles) &&
          "Cannot force response file if platform does not allow it");

   if (forceResponseFiles || (invocationInfo.allowsResponseFiles &&
                              !llvm::sys::commandLineFitsWithinSystemLimits(
                                 executablePath, invocationInfo.Arguments))) {
      const char *responseFilePath =
         context.getTemporaryFilePath("arguments", "resp");
      const char *responseFileArg =
         C.getArgs().MakeArgString(Twine("@") + responseFilePath);
      return {{responseFilePath, responseFileArg}};
   }
   return None;
}

std::unique_ptr<Job> ToolChain::constructJob(
   const JobAction &JA, Compilation &C, SmallVectorImpl<const Job *> &&inputs,
   ArrayRef<const Action *> inputActions,
   std::unique_ptr<CommandOutput> output, const OutputInfo &OI) const {
   JobContext context{C, inputs, inputActions, *output, OI};

   auto invocationInfo = [&]() -> InvocationInfo {
      switch (JA.getKind()) {
#define CASE(K)                                                                \
  case Action::Kind::K:                                                        \
    return constructInvocation(cast<K##Action>(JA), context);
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
   if (StringRef(POLARPHP_EXECUTABLE_NAME) == invocationInfo.ExecutableName) {
      executablePath = getDriver().getPHPProgramPath().c_str();
   } else {
      std::string relativePath =
         findProgramRelativeToPHP(invocationInfo.ExecutableName);
      if (!relativePath.empty()) {
         executablePath = C.getArgs().MakeArgString(relativePath);
      } else {
         auto systemPath =
            llvm::sys::findProgramByName(invocationInfo.ExecutableName);
         if (systemPath) {
            executablePath = C.getArgs().MakeArgString(systemPath.get());
         } else {
            // For debugging purposes.
            executablePath = invocationInfo.ExecutableName;
         }
      }
   }

   // Determine if the argument list is so long that it needs to be written into
   // a response file.
   auto responseFileInfo =
      getResponseFileInfo(C, executablePath, invocationInfo, context);

   return std::make_unique<Job>(
      JA, std::move(inputs), std::move(output), executablePath,
      std::move(invocationInfo.Arguments),
      std::move(invocationInfo.ExtraEnvironment),
      std::move(invocationInfo.FilelistInfos), responseFileInfo);
}

std::string
ToolChain::findProgramRelativeToPHP(StringRef executableName) const {
   auto insertionResult =
      ProgramLookupCache.insert(std::make_pair(executableName, ""));
   if (insertionResult.second) {
      std::string path = findProgramRelativeToPHPImpl(executableName);
      insertionResult.first->setValue(std::move(path));
   }
   return insertionResult.first->getValue();
}

std::string
ToolChain::findProgramRelativeToPHPImpl(StringRef executableName) const {
   StringRef swiftPath = getDriver().getPHPProgramPath();
   StringRef swiftBinDir = llvm::sys::path::parent_path(swiftPath);

   auto result = llvm::sys::findProgramByName(executableName, {swiftBinDir});
   if (result)
      return result.get();
   return {};
}

filetypes::FileTypeId ToolChain::lookupTypeForExtension(StringRef Ext) const {
   return filetypes::lookup_type_for_extension(Ext);
}

/// Return a _single_ TY_PHP InputAction, if one exists;
/// if 0 or >1 such inputs exist, return nullptr.
static const InputAction *findSinglePHPInput(const CompileJobAction *CJA) {
   auto Inputs = CJA->getInputs();
   const InputAction *IA = nullptr;
   for (auto const *I : Inputs) {
      if (auto const *S = dyn_cast<InputAction>(I)) {
         if (S->getType() == filetypes::TY_PHP) {
            if (IA == nullptr) {
               IA = S;
            } else {
               // Already found one, two is too many.
               return nullptr;
            }
         }
      }
   }
   return IA;
}

static bool jobsHaveSameExecutableNames(const Job *A, const Job *B) {
   // Jobs that get here (that are derived from CompileJobActions) should always
   // have the same executable name -- it should always be SWIFT_EXECUTABLE_NAME
   // -- but we check here just to be sure / fail gracefully in non-assert
   // builds.
   assert(strcmp(A->getExecutable(), B->getExecutable()) == 0);
   if (strcmp(A->getExecutable(), B->getExecutable()) != 0) {
      return false;
   }
   return true;
}

static bool jobsHaveSameOutputTypes(const Job *A, const Job *B) {
   if (A->getOutput().getPrimaryOutputType() !=
       B->getOutput().getPrimaryOutputType())
      return false;
   return A->getOutput().hasSameAdditionalOutputTypes(B->getOutput());
}

static bool jobsHaveSameEnvironment(const Job *A, const Job *B) {
   auto AEnv = A->getExtraEnvironment();
   auto BEnv = B->getExtraEnvironment();
   if (AEnv.size() != BEnv.size())
      return false;
   for (size_t i = 0; i < AEnv.size(); ++i) {
      if (strcmp(AEnv[i].first, BEnv[i].first) != 0)
         return false;
      if (strcmp(AEnv[i].second, BEnv[i].second) != 0)
         return false;
   }
   return true;
}

bool ToolChain::jobIsBatchable(const Compilation &C, const Job *A) const {
   // FIXME: There might be a tighter criterion to use here?
   if (C.getOutputInfo().CompilerMode != OutputInfo::Mode::StandardCompile)
      return false;
   auto const *CJActA = dyn_cast<const CompileJobAction>(&A->getSource());
   if (!CJActA)
      return false;
   return findSinglePHPInput(CJActA) != nullptr;
}

bool ToolChain::jobsAreBatchCombinable(const Compilation &C, const Job *A,
                                       const Job *B) const {
   assert(jobIsBatchable(C, A));
   assert(jobIsBatchable(C, B));
   return (jobsHaveSameExecutableNames(A, B) && jobsHaveSameOutputTypes(A, B) &&
           jobsHaveSameEnvironment(A, B));
}

/// Form a synthetic \c CommandOutput for a \c BatchJob by merging together the
/// \c CommandOutputs of all the jobs passed.
static std::unique_ptr<CommandOutput>
makeBatchCommandOutput(ArrayRef<const Job *> jobs, Compilation &C,
                       filetypes::FileTypeId outputType) {
   auto output =
      std::make_unique<CommandOutput>(outputType, C.getDerivedOutputFileMap());
   for (auto const *J : jobs) {
      output->addOutputs(J->getOutput());
   }
   return output;
}

/// Set-union the \c Inputs and \c InputActions from each \c Job in \p jobs into
/// the provided \p inputJobs and \p inputActions vectors, further adding all \c
/// Actions in the \p jobs -- InputActions or otherwise -- to \p batchCJA. Do
/// set-union rather than concatenation here to avoid mentioning the same input
/// multiple times.
static bool
mergeBatchInputs(ArrayRef<const Job *> jobs,
                 llvm::SmallSetVector<const Job *, 16> &inputJobs,
                 llvm::SmallSetVector<const Action *, 16> &inputActions,
                 CompileJobAction *batchCJA) {

   llvm::SmallSetVector<const Action *, 16> allActions;

   for (auto const *J : jobs) {
      for (auto const *I : J->getInputs()) {
         inputJobs.insert(I);
      }
      auto const *CJA = dyn_cast<CompileJobAction>(&J->getSource());
      if (!CJA)
         return true;
      for (auto const *I : CJA->getInputs()) {
         // Capture _all_ input actions -- whether or not they are InputActions --
         // in allActions, to set as the inputs for batchCJA below.
         allActions.insert(I);
         // Only collect input actions that _are InputActions_ in the inputActions
         // array, to load into the JobContext in our caller.
         if (auto const *IA = dyn_cast<InputAction>(I)) {
            inputActions.insert(IA);
         }
      }
   }

   for (auto const *I : allActions) {
      batchCJA->addInput(I);
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
sortJobsToMatchCompilationInputs(ArrayRef<const Job *> unsortedJobs,
                                 SmallVectorImpl<const Job *> &sortedJobs,
                                 Compilation &C) {
   llvm::DenseMap<StringRef, const Job *> jobsByInput;
   for (const Job *J : unsortedJobs) {
      const CompileJobAction *CJA = cast<CompileJobAction>(&J->getSource());
      const InputAction *IA = findSinglePHPInput(CJA);
      auto R =
         jobsByInput.insert(std::make_pair(IA->getInputArg().getValue(), J));
      assert(R.second);
      (void)R;
   }
   for (const InputPair &P : C.getInputFiles()) {
      auto I = jobsByInput.find(P.second->getValue());
      if (I != jobsByInput.end()) {
         sortedJobs.push_back(I->second);
      }
   }
}

/// Construct a \c BatchJob by merging the constituent \p jobs' CommandOutput,
/// input \c Job and \c Action members. Call through to \c constructInvocation
/// on \p BatchJob, to build the \c InvocationInfo.
std::unique_ptr<Job>
ToolChain::constructBatchJob(ArrayRef<const Job *> unsortedJobs,
                             Job::PID &NextQuasiPID,
                             Compilation &C) const {
   if (unsortedJobs.empty())
      return nullptr;

   llvm::SmallVector<const Job *, 16> sortedJobs;
   sortJobsToMatchCompilationInputs(unsortedJobs, sortedJobs, C);

   // Synthetic OutputInfo is a slightly-modified version of the initial
   // compilation's OI.
   auto OI = C.getOutputInfo();
   OI.CompilerMode = OutputInfo::Mode::BatchModeCompile;

   auto const *executablePath = sortedJobs[0]->getExecutable();
   auto outputType = sortedJobs[0]->getOutput().getPrimaryOutputType();
   auto output = makeBatchCommandOutput(sortedJobs, C, outputType);

   llvm::SmallSetVector<const Job *, 16> inputJobs;
   llvm::SmallSetVector<const Action *, 16> inputActions;
   auto *batchCJA = C.createAction<CompileJobAction>(outputType);
   if (mergeBatchInputs(sortedJobs, inputJobs, inputActions, batchCJA))
      return nullptr;

   JobContext context{C, inputJobs.getArrayRef(), inputActions.getArrayRef(),
                      *output, OI};
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
      getResponseFileInfo(C, executablePath, invocationInfo, context);

   return std::make_unique<BatchJob>(
      *batchCJA, inputJobs.takeVector(), std::move(output), executablePath,
      std::move(invocationInfo.Arguments),
      std::move(invocationInfo.ExtraEnvironment),
      std::move(invocationInfo.FilelistInfos), sortedJobs, NextQuasiPID,
      responseFileInfo);
}
