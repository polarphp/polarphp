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

#ifndef POLARPHP_DRIVER_JOB_H
#define POLARPHP_DRIVER_JOB_H

#include "polarphp/basic/FileTypes.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/OutputFileMap.h"
#include "polarphp/driver/Action.h"
#include "polarphp/driver/Utils.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Option/Option.h"
#include "llvm/Support/Chrono.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>

namespace polar::driver {

using polar::basic::OutputFileMap;

class Job;
class JobAction;

/// \file Job.h
///
///Some terminology for the following sections (and especially Driver.cpp):
///
/// baseInput: a filename provided by the user, upstream of the entire Job
///            graph, usually denoted by an InputAction. Every Job has access,
///            during construction, to a set of BaseInputs that are upstream of
///            its inputs and input jobs in the job graph, and from which it can
///            derive primaryInput names for itself.
///
/// BaseOutput: a filename that is a non-temporary, output at the bottom of a
///             Job graph, and often (though not always) directly specified by
///             the user in the form of a -o or -emit-foo-path name, or an entry
///             in a user-provided OutputFileMap. May also be an auxiliary,
///             derived from a baseInput and a type.
///
/// primaryInput: one of the distinguished inputs-to-act-on (as opposed to
///               merely informative additional inputs) to a Job. May be a
///               baseInput but may also be a temporary that doesn't live beyond
///               the execution of the Job graph.
///
/// PrimaryOutput: an output file matched 1:1 with a specific
///                primaryInput. Auxiliary outputs may also be produced. A
///                PrimaryOutput may be a BaseOutput, but may also be a
///                temporary that doesn't live beyond the execution of the Job
///                graph (that is: it exists in order to be the primaryInput
///                for a subsequent Job).
///
/// The user-provided OutputFileMap lists BaseInputs and BaseOutputs, but doesn't
/// describe the temporaries inside the Job graph.
///
/// The Compilation's DerivedOutputFileMap (shared by all CommandOutputs) lists
/// PrimaryInputs and maps them to PrimaryOutputs, including all the
/// temporaries. This means that in a multi-stage Job graph, the baseInput =>
/// BaseOutput entries provided by the user are split in two (or more) steps,
/// one baseInput => SomeTemporary and one SomeTemporary => BaseOutput.
///
/// To try to keep this as simple as possible (it's already awful) we associate
/// every primaryInput 1:1 with a specific baseInput from which it was derived;
/// this way a CommandOutput will have a vector of _pairs_ of
/// {base,primary}m_inputs rather than a pair of separate vectors. This arrangement
/// appears to cover all the graph topologies we encounter in practice.


struct CommandInputPair
{
   /// A filename provided from the user, either on the command line or in an
   /// input file map. Feeds into a Job graph, from InputActions, and is
   /// _associated_ with a primaryInput for a given Job, but may be upstream of
   /// the Job (and its primaryInput) and thus not necessarily passed as a
   /// filename to the job. Used as a key into the user-provided OutputFileMap
   /// (of BaseInputs and BaseOutputs), and used to derive downstream names --
   /// both temporaries and auxiliaries -- but _not_ used as a key into the
   /// DerivedOutputFileMap.
   StringRef base;

   /// A filename that _will be passed_ to the command as a designated primary
   /// input. Typically either equal to baseInput or a temporary with a name
   /// derived from the baseInput it is related to. Also used as a key into
   /// the DerivedOutputFileMap.
   StringRef primary;

   /// Construct a CommandInputPair from a base input and, optionally, a primary;
   /// if the primary is empty, use the base value for it.
   explicit CommandInputPair(StringRef baseInput, StringRef primaryInput)
      : base(baseInput),
        primary(primaryInput.empty() ? baseInput : primaryInput)
   {}
};

class CommandOutput
{
public:
   CommandOutput(filetypes::FileTypeId primaryOutputType, OutputFileMap &derived);

   /// Return the primary output type for this CommandOutput.
   filetypes::FileTypeId getPrimaryOutputType() const;

   /// Associate a new \p primaryOutputFile (of type \c getPrimaryOutputType())
   /// with the provided \p input pair of base and primary inputs.
   void addPrimaryOutput(CommandInputPair input, StringRef primaryOutputFile);

   /// Return true iff the set of additional output types in \c this is
   /// identical to the set of additional output types in \p other.
   bool hasSameAdditionalOutputTypes(CommandOutput const &other) const;

   /// Copy all the input pairs from \p other to \c this. Assumes (and asserts)
   /// that \p other shares output file map and m_primaryOutputType with \c this
   /// already, as well as m_additionalOutputTypes if \c this has any.
   void addOutputs(CommandOutput const &other);

   /// Assuming (and asserting) that there is only one input pair, return the
   /// primary output file associated with it. Note that the returned StringRef
   /// may be invalidated by subsequent mutations to the \c CommandOutput.
   StringRef getPrimaryOutputFilename() const;

   /// Return a all of the outputs of type \c getPrimaryOutputType() associated
   /// with a primary input. The return value will contain one \c StringRef per
   /// primary input, _even if_ the primary output type is TY_Nothing, and the
   /// primary output filenames are therefore all empty strings.
   ///
   /// FIXME: This is not really ideal behaviour -- it would be better to return
   /// only nonempty strings in all cases, and have the callers differentiate
   /// contexts with absent primary outputs another way -- but this is currently
   /// assumed at several call sites.
   SmallVector<StringRef, 16> getPrimaryOutputFilenames() const;

   /// Assuming (and asserting) that there are one or more input pairs, associate
   /// an additional output named \p outputFilename of type \p type with the
   /// first primary input. If the provided \p type is the primary output type,
   /// overwrite the existing entry assocaited with the first primary input.
   void setAdditionalOutputForType(filetypes::FileTypeId type,
                                   StringRef outputFilename);

   /// Assuming (and asserting) that there are one or more input pairs, return
   /// the _additional_ (not primary) output of type \p type associated with the
   /// first primary input.
   StringRef getAdditionalOutputForType(filetypes::FileTypeId type) const;

   /// Return a vector of additional (not primary) outputs of type \p type
   /// associated with the primary inputs.
   ///
   /// In contrast to \c getPrimaryOutputFilenames, this method does _not_ return
   /// any empty strings or ensure the return vector is matched in size with the
   /// set of primary inputs; however it _does_ assert that the return vector's
   /// length is _either_ zero, one, or equal to the size of the set of inputs,
   /// as these are the only valid arity relationships between primary and
   /// additional outputs.
   SmallVector<StringRef, 16>
   getAdditionalOutputsForType(filetypes::FileTypeId type) const;

   /// Assuming (and asserting) that there is only one input pair, return any
   /// output -- primary or additional -- of type \p type associated with that
   /// the sole primary input.
   StringRef getAnyOutputForType(filetypes::FileTypeId type) const;

   /// Return the whole derived output map.
   const OutputFileMap &getDerivedOutputMap() const;

   /// Return the baseInput numbered by \p Index.
   StringRef getBaseInput(size_t Index) const;

   /// Write a file map naming the outputs for each primary input.
   void writeOutputFileMap(llvm::raw_ostream &out) const;

   void print(raw_ostream &Stream) const;
   void dump() const LLVM_ATTRIBUTE_USED;

   /// For use in assertions: check the CommandOutput's state is consistent with
   /// its invariants.
   void checkInvariants() const;

private:
   // If there is an entry in the m_derivedOutputMap for a given (\p
   // primaryInputFile, \p type) pair, return a nonempty StringRef, otherwise
   // return an empty StringRef.
   StringRef getOutputForInputAndType(StringRef primaryInputFile,
                                      filetypes::FileTypeId type) const;

   /// Add an entry to the \c m_derivedOutputMap if it doesn't exist. If an entry
   /// already exists for \p primaryInputFile of type \p type, then either
   /// overwrite the entry (if \p overwrite is \c true) or assert that it has
   /// the same value as \p outputFile.
   void ensureEntry(StringRef primaryInputFile, filetypes::FileTypeId type,
                    StringRef outputFile, bool overwrite);

private:
   /// A CommandOutput designates one type of output as primary, though there
   /// may be multiple outputs of that type.
   filetypes::FileTypeId m_primaryOutputType;

   /// A CommandOutput also restricts its attention regarding additional-outputs
   /// to a subset of the PrimaryOutputs associated with its PrimaryInputs;
   /// sometimes multiple commands operate on the same primaryInput, in different
   /// phases (eg. autolink-extract and link both operate on the same .o file),
   /// so Jobs cannot _just_ rely on the presence of a primary output in the
   /// DerivedOutputFileMap.
   llvm::SmallSet<filetypes::FileTypeId, 4> m_additionalOutputTypes;

   /// The list of inputs for this \c CommandOutput. Each input in the list has
   /// two names (often but not always the same), of which the second (\c
   /// CommandInputPair::primary) acts as a key into \c m_derivedOutputMap.  Each
   /// input thus designates an associated _set_ of outputs, one of which (the
   /// one of type \c m_primaryOutputType) is considered the "primary output" for
   /// the input.
   SmallVector<CommandInputPair, 1> m_inputs;

   /// All CommandOutputs in a Compilation share the same \c
   /// m_derivedOutputMap. This is computed both from any user-provided input file
   /// map, and any inference steps.
   OutputFileMap &m_derivedOutputMap;
};

class Job
{
public:
   enum class Condition
   {
      // There was no information about the previous build (i.e., an input map),
      // or the map marked this Job as dirty or needing a cascading build.
      // Be maximally conservative with dependencies.
      Always,
      // The input changed, or this job was scheduled as non-cascading in the last
      // build but didn't get to run.
      RunWithoutCascading,
      // The best case: input didn't change, output exists.
      // Only run if it depends on some other thing that changed.
      CheckDependencies,
      // Run no matter what (but may or may not cascade).
      NewlyAdded
   };

   /// Packs together information about response file usage for a job.
   ///
   /// The strings in this struct must be kept alive as long as the Job is alive
   /// (e.g., by calling MakeArgString on the arg list associated with the
   /// Compilation).
   struct ResponseFileInfo
   {
      /// The path to the response file that a job should use.
      const char *path;

      /// The '@'-prefixed argument string that should be passed to the tool to
      /// use the response file.
      const char *argString;
   };

   using EnvironmentVector = std::vector<std::pair<const char *, const char *>>;

   /// If positive, contains llvm::ProcessID for a real Job on the host OS. If
   /// negative, contains a quasi-PID, which identifies a Job that's a member of
   /// a BatchJob _without_ denoting an operating system process.
   using PID = int64_t;

public:
   Job(const JobAction &source, SmallVectorImpl<const Job *> &&inputs,
       std::unique_ptr<CommandOutput> output, const char *executable,
       llvm::opt::ArgStringList arguments,
       EnvironmentVector extraEnvironment = {},
       std::vector<FilelistInfo> infos = {},
       Optional<ResponseFileInfo> responseFile = None)
      : m_sourceAndCondition(&source, Condition::Always),
        m_inputs(std::move(inputs)), m_output(std::move(output)),
        executable(executable), m_arguments(std::move(arguments)),
        m_extraEnvironment(std::move(extraEnvironment)),
        m_filelistFileInfos(std::move(infos)), m_responseFile(responseFile)
   {}

   virtual ~Job();

   const JobAction &getSource() const
   {
      return *m_sourceAndCondition.getPointer();
   }

   const char *getExecutable() const
   {
      return executable;
   }

   const llvm::opt::ArgStringList &getArguments() const
   {
      return m_arguments;
   }

   ArrayRef<const char *> getResponseFileArg() const
   {
      assert(hasResponseFile());
      return m_responseFile->argString;
   }

   ArrayRef<FilelistInfo> getFilelistInfos() const
   {
      return m_filelistFileInfos;
   }

   ArrayRef<const char *> getArgumentsForTaskExecution() const;

   ArrayRef<const Job *> getInputs() const
   {
      return m_inputs;
   }

   const CommandOutput &getOutput() const
   {
      return *m_output;
   }

   Condition getCondition() const
   {
      return m_sourceAndCondition.getInt();
   }

   void setCondition(Condition cond)
   {
      m_sourceAndCondition.setInt(cond);
   }

   void setInputModTime(llvm::sys::TimePoint<> time)
   {
      m_inputModTime = time;
   }

   llvm::sys::TimePoint<> getInputModTime() const
   {
      return m_inputModTime;
   }

   ArrayRef<std::pair<const char *, const char *>> getExtraEnvironment() const
   {
      return m_extraEnvironment;
   }

   /// Print the command line for this Job to the given \p stream,
   /// terminating output with the given \p terminator.
   void printCommandLine(raw_ostream &Stream, StringRef Terminator = "\n") const;

   /// Print a short summary of this Job to the given \p Stream.
   void printSummary(raw_ostream &Stream) const;

   /// Print the command line for this Job to the given \p stream,
   /// and include any extra environment variables that will be set.
   ///
   /// \sa printCommandLine
   void printCommandLineAndEnvironment(raw_ostream &Stream,
                                       StringRef Terminator = "\n") const;

   /// Call the provided callback with any Jobs (and their possibly-quasi-PIDs)
   /// contained within this Job; if this job is not a BatchJob, just pass \c
   /// this and the provided \p osPid back to the callback.
   virtual void forEachContainedJobAndPID(
         llvm::sys::procid_t osPid,
         llvm::function_ref<void(const Job *, Job::PID)> callback) const
   {
      callback(this, static_cast<Job::PID>(osPid));
   }

   void dump() const LLVM_ATTRIBUTE_USED;

   static void printArguments(raw_ostream &Stream,
                              const llvm::opt::ArgStringList &Args);

   bool hasResponseFile() const
   {
      return m_responseFile.hasValue();
   }

   bool writeArgsToResponseFile() const;

private:
   /// The action which caused the creation of this Job, and the conditions
   /// under which it must be run.
   llvm::PointerIntPair<const JobAction *, 2, Condition> m_sourceAndCondition;

   /// The list of other Jobs which are inputs to this Job.
   SmallVector<const Job *, 4> m_inputs;

   /// The output of this command.
   std::unique_ptr<CommandOutput> m_output;

   /// The executable to run.
   const char *executable;

   /// The list of program arguments (not including the implicit first argument,
   /// which will be the executable).
   ///
   /// These argument strings must be kept alive as long as the Job is alive.
   llvm::opt::ArgStringList m_arguments;

   /// Additional variables to set in the process environment when running.
   ///
   /// These strings must be kept alive as long as the Job is alive.
   EnvironmentVector m_extraEnvironment;

   /// Whether the job wants a list of input or output files created.
   std::vector<FilelistInfo> m_filelistFileInfos;

   /// The path and argument string to use for the response file if the job's
   /// arguments should be passed using one.
   Optional<ResponseFileInfo> m_responseFile;

   /// The modification time of the main input file, if any.
   llvm::sys::TimePoint<> m_inputModTime = llvm::sys::TimePoint<>::max();
};

/// A BatchJob comprises a _set_ of jobs, each of which is sufficiently similar
/// to the others that the whole set can be combined into a single subprocess
/// (and thus run potentially more-efficiently than running each Job in the set
/// individually).
///
/// Not all Jobs can be combined into a BatchJob: at present, only those Jobs
/// that come from CompileJobActions, and which otherwise have the exact same
/// input file list and arguments as one another, aside from their primary-file.
/// See ToolChain::jobsAreBatchCombinable for details.

class BatchJob : public Job
{
public:
   BatchJob(const JobAction &source, SmallVectorImpl<const Job *> &&inputs,
            std::unique_ptr<CommandOutput> output, const char *executable,
            llvm::opt::ArgStringList arguments,
            EnvironmentVector extraEnvironment, std::vector<FilelistInfo> infos,
            ArrayRef<const Job *> Combined, Job::PID &nextQuasiPID,
            Optional<ResponseFileInfo> responseFile = None);

   ArrayRef<const Job*> getCombinedJobs() const
   {
      return m_combinedJobs;
   }

   /// Call the provided callback for each Job in the batch, passing the
   /// corresponding quasi-PID with each Job.
   void forEachContainedJobAndPID(
         llvm::sys::procid_t osPid,
         llvm::function_ref<void(const Job *, Job::PID)> callback) const override
   {
      Job::PID qpid = m_quasiPIDBase;
      assert(qpid < 0);
      for (auto const *J : m_combinedJobs) {
         assert(qpid != std::numeric_limits<Job::PID>::min());
         callback(J, qpid--);
      }
   }
private:
   /// The set of constituents making up the batch.
   const SmallVector<const Job *, 4> m_combinedJobs;

   /// A negative number to use as the base value for assigning quasi-PID to Jobs
   /// in the \c m_combinedJobs array. Quasi-PIDs count _down_ from this value.
   const Job::PID m_quasiPIDBase;
};


} // polar::driver

#endif // POLARPHP_DRIVER_JOB_H
