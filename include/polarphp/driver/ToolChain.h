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

#ifndef POLARPHP_DRIVER_TOOLCHAIN_H
#define POLARPHP_DRIVER_TOOLCHAIN_H

#include "polarphp/basic/FileTypes.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/driver/Action.h"
#include "polarphp/driver/Job.h"
#include "polarphp/option/Options.h"
#include "llvm/ADT/Triple.h"

#include <memory>

namespace polar::driver {

class CommandOutput;
class Compilation;
class Driver;
class Job;
class OutputInfo;

/// A ToolChain is responsible for turning abstract Actions into concrete,
/// runnable jobs.
///
/// The primary purpose of a ToolChain is built around the
/// \c constructInvocation family of methods. This is a set of callbacks
/// following the Visitor pattern for the various JobAction subclasses, which
/// returns an executable name and arguments for the Job to be run. The base
/// ToolChain knows how to perform most operations, but some (like linking)
/// require platform-specific knowledge, provided in subclasses.
class ToolChain
{
protected:
   ToolChain(const Driver &driver, const llvm::Triple &triple)
      : m_driver(driver),
        m_triple(triple)
   {}

   /// A special name used to identify the Swift executable itself.
   constexpr static const char *const POLARPHP_EXECUTABLE_NAME = "polarphp";

   /// Packs together the supplementary information about the job being created.
   class JobContext
   {
   private:
      Compilation &m_compilation;

   public:
      ArrayRef<const Job *> inputs;
      ArrayRef<const Action *> inputActions;
      const CommandOutput &output;
      const OutputInfo &outputInfo;

      /// The arguments to the driver. Can also be used to create new strings with
      /// the same lifetime.
      ///
      /// This just caches C.getArgs().
      const llvm::opt::ArgList &args;

   public:
      JobContext(Compilation &C, ArrayRef<const Job *> inputs,
                 ArrayRef<const Action *> inputActions,
                 const CommandOutput &output, const OutputInfo &outputInfo);

      /// Forwards to Compilation::getInputFiles.
      ArrayRef<InputPair> getTopLevelInputFiles() const;

      /// Forwards to Compilation::getAllSourcesPath.
      const char *getAllSourcesPath() const;

      /// Creates a new temporary file for use by a job.
      ///
      /// The returned string already has its lifetime extended to match other
      /// arguments.
      const char *getTemporaryFilePath(const llvm::Twine &name,
                                       StringRef suffix = "") const;

      /// For frontend, merge-module, and link invocations.
      bool shouldUseInputFileList() const;

      bool shouldUsePrimaryInputFileListInFrontendInvocation() const;

      bool shouldUseMainOutputFileListInFrontendInvocation() const;

      bool shouldUseSupplementaryOutputFileMapInFrontendInvocation() const;

      /// Reify the existing behavior that SingleCompile compile actions do not
      /// filter, but batch-mode and single-file compilations do. Some clients are
      /// relying on this (i.e., they pass inputs that don't have ".swift" as an
      /// extension.) It would be nice to eliminate this distinction someday.
      bool shouldFilterFrontendInputsByType() const;

      const char *computeFrontendModeForCompile() const;

      void addFrontendInputAndOutputArguments(
            llvm::opt::ArgStringList &arguments,
            std::vector<FilelistInfo> &filelistInfos) const;

   private:
      void addFrontendCommandLineInputArguments(
            bool mayHavePrimaryInputs, bool useFileList, bool usePrimaryFileList,
            bool filterByType, llvm::opt::ArgStringList &arguments) const;
      void addFrontendSupplementaryOutputArguments(
            llvm::opt::ArgStringList &arguments) const;
   };

   /// Packs together information chosen by toolchains to create jobs.
   struct InvocationInfo
   {
      const char *executableName;
      llvm::opt::ArgStringList arguments;
      std::vector<std::pair<const char *, const char *>> extraEnvironment;
      std::vector<FilelistInfo> filelistInfos;

      // Not all platforms and jobs support the use of response files, so assume
      // "false" by default. If the executable specified in the InvocationInfo
      // constructor supports response files, this can be overridden and set to
      // "true".
      bool allowsResponseFiles = false;

      InvocationInfo(const char *name, llvm::opt::ArgStringList args = {},
                     decltype(extraEnvironment) extraEnv = {})
         : executableName(name), arguments(std::move(args)),
           extraEnvironment(std::move(extraEnv)) {}
   };

   virtual InvocationInfo constructInvocation(const CompileJobAction &job,
                                              const JobContext &context) const;
   virtual InvocationInfo constructInvocation(const InterpretJobAction &job,
                                              const JobContext &context) const;
   virtual InvocationInfo constructInvocation(const BackendJobAction &job,
                                              const JobContext &context) const;
   virtual InvocationInfo constructInvocation(const MergeModuleJobAction &job,
                                              const JobContext &context) const;
   virtual InvocationInfo constructInvocation(const ModuleWrapJobAction &job,
                                              const JobContext &context) const;

   virtual InvocationInfo constructInvocation(const REPLJobAction &job,
                                              const JobContext &context) const;

   virtual InvocationInfo constructInvocation(const GenerateDSYMJobAction &job,
                                              const JobContext &context) const;
   virtual InvocationInfo
   constructInvocation(const VerifyDebugInfoJobAction &job,
                       const JobContext &context) const;
   virtual InvocationInfo constructInvocation(const GeneratePCHJobAction &job,
                                              const JobContext &context) const;
   virtual InvocationInfo
   constructInvocation(const AutolinkExtractJobAction &job,
                       const JobContext &context) const;
   virtual InvocationInfo constructInvocation(const DynamicLinkJobAction &job,
                                              const JobContext &context) const;

   virtual InvocationInfo constructInvocation(const StaticLinkJobAction &job,
                                              const JobContext &context) const;

   /// Searches for the given executable in appropriate paths relative to the
   /// Swift binary.
   ///
   /// This method caches its results.
   ///
   /// \sa findProgramRelativeToSwiftImpl
   std::string findProgramRelativeToSwift(StringRef name) const;

   /// An override point for platform-specific subclasses to customize how to
   /// do relative searches for programs.
   ///
   /// This method is invoked by findProgramRelativeToPolarphp().
   virtual std::string findProgramRelativeToPolarphpImpl(StringRef name) const;

   void addInputsOfType(llvm::opt::ArgStringList &arguments,
                        ArrayRef<const Action *> inputs,
                        filetypes::FileTypeId inputType,
                        const char *prefixArgument = nullptr) const;

   void addInputsOfType(llvm::opt::ArgStringList &arguments,
                        ArrayRef<const Job *> jobs,
                        const llvm::opt::ArgList &args, filetypes::FileTypeId inputType,
                        const char *prefixArgument = nullptr) const;

   void addPrimaryInputsOfType(llvm::opt::ArgStringList &arguments,
                               ArrayRef<const Job *> jobs,
                               const llvm::opt::ArgList &args,
                               filetypes::FileTypeId inputType,
                               const char *prefixArgument = nullptr) const;

   /// Get the resource dir link path, which is platform-specific and found
   /// relative to the compiler.
   void getResourceDirPath(SmallVectorImpl<char> &runtimeLibPath,
                           const llvm::opt::ArgList &args, bool shared) const;

   /// Get the runtime library link paths, which typically include the resource
   /// dir path and the SDK.
   void getRuntimeLibraryPaths(SmallVectorImpl<std::string> &runtimeLibPaths,
                               const llvm::opt::ArgList &args,
                               StringRef sdkPath, bool shared) const;

   void addPathEnvironmentVariableIfNeeded(Job::EnvironmentVector &env,
                                           const char *name,
                                           const char *separator,
                                           options::ID optionID,
                                           const llvm::opt::ArgList &args,
                                           ArrayRef<std::string> extraEntries = {}) const;

   /// Specific toolchains should override this to provide additional conditions
   /// under which the compiler invocation should be written into debug info. For
   /// example, Darwin does this if the RC_DEBUG_OPTIONS environment variable is
   /// set to match the behavior of Clang.
   virtual bool shouldStoreInvocationInDebugInfo() const { return false; }

   /// Gets the response file path and command line argument for an invocation
   /// if the tool supports response files and if the command line length would
   /// exceed system limits.
   Optional<Job::ResponseFileInfo>
   getResponseFileInfo(const Compilation &C, const char *executablePath,
                       const InvocationInfo &invocationInfo,
                       const JobContext &context) const;

public:
   virtual ~ToolChain() = default;

   const Driver &getDriver() const
   {
      return m_driver;
   }

   const llvm::Triple &getTriple() const
   {
      return m_triple;
   }

   /// Construct a Job for the action \p JA, taking the given information into
   /// account.
   ///
   /// This method dispatches to the various \c constructInvocation methods,
   /// which may be overridden by platform-specific subclasses.
   std::unique_ptr<Job> constructJob(const JobAction &JA, Compilation &C,
                                     SmallVectorImpl<const Job *> &&inputs,
                                     ArrayRef<const Action *> inputActions,
                                     std::unique_ptr<CommandOutput> output,
                                     const OutputInfo &outputInfo) const;

   /// Return true iff the input \c Job \p A is an acceptable candidate for
   /// batching together into a BatchJob, via a call to \c
   /// constructBatchJob. This is true when the \c Job is a built from a \c
   /// CompileJobAction in a \c Compilation \p C running in \c
   /// OutputInfo::Mode::StandardCompile output mode, with a single \c TY_Swift
   /// \c InputAction.
   bool jobIsBatchable(const Compilation &comilation, const Job *action) const;

   /// Equivalence relation that holds iff the two input jobs \p A and \p B are
   /// acceptable candidates for combining together into a \c BatchJob, via a
   /// call to \c constructBatchJob. This is true when each job independently
   /// satisfies \c jobIsBatchable, and the two jobs have identical executables,
   /// output types and environments (i.e. they are identical aside from their
   /// inputs).
   bool jobsAreBatchCombinable(const Compilation &comilation, const Job *action,
                               const Job *job) const;

   /// Construct a \c BatchJob that subsumes the work of a set of jobs. Any pair
   /// of elements in \p jobs are assumed to satisfy the equivalence relation \c
   /// jobsAreBatchCombinable, i.e. they should all be "the same" job in in all
   /// ways other than their choices of inputs. The provided \p nextQuasiPID
   /// should be a negative number that persists between calls; this method will
   /// decrement it to assign quasi-PIDs to each of the \p jobs passed.
   std::unique_ptr<Job> constructBatchJob(ArrayRef<const Job *> jobs,
                                          int64_t &nextQuasiPID,
                                          Compilation &compilation) const;

   /// Return the default language type to use for the given extension.
   /// If the extension is empty or is otherwise not recognized, return
   /// the invalid type \c TY_INVALID.
   filetypes::FileTypeId lookupTypeForExtension(StringRef ext) const;

   /// Copies the path for the directory clang libraries would be stored in on
   /// the current toolchain.
   void getClangLibraryPath(const llvm::opt::ArgList &args,
                            SmallString<128> &libPath) const;

   /// Returns the name the clang library for a given sanitizer would have on
   /// the current toolchain.
   ///
   /// \param sanitizer sanitizer name.
   /// \param shared Whether the library is shared
   virtual std::string sanitizerRuntimeLibName(StringRef sanitizer,
                                               bool shared = true) const = 0;

   /// Returns whether a given sanitizer exists for the current toolchain.
   ///
   /// \param sanitizer sanitizer name.
   /// \param shared Whether the library is shared
   bool sanitizerRuntimeLibExists(const llvm::opt::ArgList &args,
                                  StringRef sanitizer, bool shared = true) const;

   /// Adds a runtime library to the arguments list for linking.
   ///
   /// \param libName The library name
   /// \param arguments The arguments list to append to
   void addLinkRuntimeLib(const llvm::opt::ArgList &args,
                          llvm::opt::ArgStringList &arguments,
                          StringRef libName) const;

private:
   const Driver &m_driver;
   const llvm::Triple m_triple;
   mutable llvm::StringMap<std::string> m_programLookupCache;
};

} // polar::driver

#endif // POLARPHP_DRIVER_TOOLCHAIN_H
