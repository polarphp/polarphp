//===--- Job.cpp - Command to Execute -------------------------------------===//
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
// Created by polarboy on 2019/12/01.

#include "polarphp/basic/StlExtras.h"
#include "polarphp/driver/Job.h"
#include "polarphp/driver/PrettyStackTrace.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Option/Arg.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"

namespace polar::driver {

using polar::interleave;

StringRef CommandOutput::getOutputForInputAndType(StringRef primaryInputFile,
                                                  filetypes::FileTypeId type) const
{
   if (type == filetypes::TY_Nothing) {
      return StringRef();
   }
   auto const *map = m_derivedOutputMap.getOutputMapForInput(primaryInputFile);
   if (!map) {
      return StringRef();
   }
   auto const out = map->find(type);
   if (out == map->end()) {
      return StringRef();
   }
   assert(!out->second.empty());
   return StringRef(out->second);
}

struct CommandOutputInvariantChecker
{
   CommandOutput const &out;
   CommandOutputInvariantChecker(CommandOutput const &commandOut)
      : out(commandOut)
   {
#ifndef NDEBUG
      out.checkInvariants();
#endif
   }

   ~CommandOutputInvariantChecker()
   {
#ifndef NDEBUG
      out.checkInvariants();
#endif
   }
};

void CommandOutput::ensureEntry(StringRef primaryInputFile,
                                filetypes::FileTypeId type,
                                StringRef outputFile,
                                bool overwrite)
{
   assert(!primaryInputFile.empty());
   assert(!outputFile.empty());
   assert(type != filetypes::TY_Nothing);
   auto &map = m_derivedOutputMap.getOrCreateOutputMapForInput(primaryInputFile);
   if (overwrite) {
      map[type] = outputFile;
   } else {
      auto res = map.insert(std::make_pair(type, outputFile));
      if (res.second) {
         // New entry, no need to compare.
      } else {
         // Existing entry, check equality with request.
         assert(res.first->getSecond() == outputFile);
      }
   }
}

void CommandOutput::checkInvariants() const
{
   filetypes::for_all_types([&]( filetypes::FileTypeId type) {
      size_t numOutputsOfType = 0;
      for (auto const &input : m_inputs) {
         // FIXME: At the moment, empty primary input names correspond to
         // corner cases in the driver where it is doing TY_Nothing work
         // and isn't even given a primary input; but at some point we
         // ought to enable storing derived OFM entries under the empty
         // name in general, for "whole build" additional outputs. They
         // are presently (arbitrarily and wrongly) stored in entries
         // associated with the first primary input of the CommandOutput
         // that they were derived from.
         assert(m_primaryOutputType == filetypes::TY_Nothing || !input.primary.empty());
         auto const *map = m_derivedOutputMap.getOutputMapForInput(input.primary);
         if (!map) {
            continue;
         }
         auto const out = map->find(type);
         if (out == map->end()) {
            continue;
         }
         assert(!out->second.empty());
         ++numOutputsOfType;
      }
      assert(numOutputsOfType == 0 ||
             numOutputsOfType == 1 ||
             numOutputsOfType == m_inputs.size());
   });
   assert(m_additionalOutputTypes.count(m_primaryOutputType) == 0);
}

bool CommandOutput::hasSameAdditionalOutputTypes(
      CommandOutput const &other) const
{
   bool sameAdditionalOutputTypes = true;
   filetypes::for_all_types([&](filetypes::FileTypeId type) {
      bool a = m_additionalOutputTypes.count(type) == 0;
      bool b = other.m_additionalOutputTypes.count(type) == 0;
      if (a != b)
         sameAdditionalOutputTypes = false;
   });
   return sameAdditionalOutputTypes;
}

void CommandOutput::addOutputs(CommandOutput const &other)
{
   CommandOutputInvariantChecker Check(*this);
   assert(m_primaryOutputType == other.m_primaryOutputType);
   assert(&m_derivedOutputMap == &other.m_derivedOutputMap);
   m_inputs.append(other.m_inputs.begin(),
                   other.m_inputs.end());
   // Should only be called with an empty m_additionalOutputTypes
   // or one populated with the same types as other.
   if (m_additionalOutputTypes.empty()) {
      m_additionalOutputTypes = other.m_additionalOutputTypes;
   } else {
      assert(hasSameAdditionalOutputTypes(other));
   }
}

CommandOutput::CommandOutput( filetypes::FileTypeId primaryOutputType,
                              OutputFileMap &derived)
   : m_primaryOutputType(primaryOutputType),
     m_derivedOutputMap(derived)
{
   CommandOutputInvariantChecker Check(*this);
}

filetypes::FileTypeId CommandOutput::getPrimaryOutputType() const
{
   return m_primaryOutputType;
}

void CommandOutput::addPrimaryOutput(CommandInputPair input,
                                     StringRef primaryOutputFile)
{
   PrettyStackTraceDriverCommandOutputAddition CrashInfo(
            "primary", this, input.primary, m_primaryOutputType, primaryOutputFile);
   if (m_primaryOutputType == filetypes::TY_Nothing) {
      // For TY_Nothing, we accumulate the inputs but do not add any outputs.
      // The invariant holds on either side of this action because all primary
      // outputs for this command will be absent (so the length == 0 case in the
      // invariant holds).
      CommandOutputInvariantChecker Check(*this);
      m_inputs.push_back(input);
      return;
   }
   // The invariant holds in the non-TY_Nothing case before an input is added and
   // _after the corresponding output is added_, but not inbetween. Don't try to
   // merge these two cases, they're different.
   CommandOutputInvariantChecker Check(*this);
   m_inputs.push_back(input);
   assert(!primaryOutputFile.empty());
   assert(m_additionalOutputTypes.count(m_primaryOutputType) == 0);
   ensureEntry(input.primary, m_primaryOutputType, primaryOutputFile, false);
}

StringRef CommandOutput::getPrimaryOutputFilename() const
{
   // FIXME: ideally this shouldn't exist, or should at least assert size() == 1,
   // and callers should handle cases with multiple primaries explicitly.
   assert(m_inputs.size() >= 1);
   return getOutputForInputAndType(m_inputs[0].primary, m_primaryOutputType);
}

SmallVector<StringRef, 16> CommandOutput::getPrimaryOutputFilenames() const
{
   SmallVector<StringRef, 16> vector;
   size_t nonEmpty = 0;
   for (auto const &input : m_inputs) {
      auto out = getOutputForInputAndType(input.primary, m_primaryOutputType);
      vector.push_back(out);
      if (!out.empty()) {
         ++nonEmpty;
      }
      assert(!out.empty() || m_primaryOutputType == filetypes::TY_Nothing);
   }
   assert(nonEmpty == 0 || nonEmpty == m_inputs.size());
   return vector;
}

void CommandOutput::setAdditionalOutputForType( filetypes::FileTypeId type,
                                                StringRef outputFilename)
{
   PrettyStackTraceDriverCommandOutputAddition CrashInfo(
            "additional", this, m_inputs[0].primary, type, outputFilename);
   CommandOutputInvariantChecker Check(*this);
   assert(m_inputs.size() >= 1);
   assert(!outputFilename.empty());
   assert(type != filetypes::TY_Nothing);

   // If we're given an "additional" output with the same type as the primary,
   // and we've not yet had such an additional type added, we treat it as a
   // request to overwrite the primary choice (which happens early and is
   // sometimes just inferred) with a refined value (eg. -emit-module-path).
   bool overwrite = type == m_primaryOutputType;
   if (overwrite) {
      assert(m_additionalOutputTypes.count(type) == 0);
   } else {
      m_additionalOutputTypes.insert(type);
   }
   ensureEntry(m_inputs[0].primary, type, outputFilename, overwrite);
}

StringRef CommandOutput::getAdditionalOutputForType( filetypes::FileTypeId type) const
{
   if (m_additionalOutputTypes.count(type) == 0) {
      return StringRef();
   }
   assert(m_inputs.size() >= 1);
   // FIXME: ideally this shouldn't associate the additional output with the
   // first primary, but with a specific primary (and/or possibly the primary "",
   // for build-wide outputs) specified by the caller.
   assert(m_inputs.size() >= 1);
   return getOutputForInputAndType(m_inputs[0].primary, type);
}

SmallVector<StringRef, 16>
CommandOutput::getAdditionalOutputsForType( filetypes::FileTypeId type) const
{
   SmallVector<StringRef, 16> vector;
   if (m_additionalOutputTypes.count(type) != 0) {
      for (auto const &I : m_inputs) {
         auto out = getOutputForInputAndType(I.primary, type);
         // FIXME: In theory this should always be non-empty -- and vector.size() would
         // always be either 0 or N like with primary outputs -- but in practice
         // WMO currently associates additional outputs with the _first primary_ in
         // a multi-primary job, which means that the 2nd..Nth primaries will have
         // an empty result from getOutputForInputAndType, and vector.size() will be 1.
         if (!out.empty()) {
            vector.push_back(out);
         }
      }
   }
   assert(vector.empty() || vector.size() == 1 || vector.size() == m_inputs.size());
   return vector;
}

StringRef CommandOutput::getAnyOutputForType( filetypes::FileTypeId type) const
{
   if (m_primaryOutputType == type) {
      return getPrimaryOutputFilename();
   }
   return getAdditionalOutputForType(type);
}

const OutputFileMap &CommandOutput::getDerivedOutputMap() const
{
   return m_derivedOutputMap;
}

StringRef CommandOutput::getBaseInput(size_t index) const
{
   assert(index < m_inputs.size());
   return m_inputs[index].base;
}

static void escapeAndPrintString(llvm::raw_ostream &ostream, StringRef str)
{
   if (str.empty()) {
      // Special-case the empty string.
      ostream << "\"\"";
      return;
   }

   bool needsEscape = str.find_first_of(" \"\\$") != StringRef::npos;

   if (!needsEscape) {
      // This string doesn't have anything we need to escape, so print it directly
      ostream << str;
      return;
   }

   // Quote and escape. This isn't really complete, but is good enough, and
   // matches how Clang's Command handles escaping arguments.
   ostream << '"';
   for (const char c : str) {
      switch (c) {
      case '"':
      case '\\':
      case '$':
         // These characters need to be escaped.
         ostream << '\\';
         // Fall-through to the default case, since we still need to print the
         // character.
         LLVM_FALLTHROUGH;
      default:
         ostream << c;
      }
   }
   ostream << '"';
}

void
CommandOutput::print(raw_ostream &out) const
{
   out
         << "{\n"
         << "    m_primaryOutputType = " << filetypes::get_type_name(m_primaryOutputType)
         << ";\n"
         << "    m_inputs = [\n";
   interleave(m_inputs,
              [&](CommandInputPair const &inputPair) {
      out << "        CommandInputPair {\n"
          << "            Base = ";
      escapeAndPrintString(out, inputPair.base);
      out << ", \n"
          << "            primary = ";
      escapeAndPrintString(out, inputPair.primary);
      out << "\n        }";
   },
   [&] { out << ",\n"; });
   out << "];\n"
       << "    DerivedOutputFileMap = {\n";
   m_derivedOutputMap.dump(out, true);
   out << "\n    };\n}";
}

void
CommandOutput::dump() const
{
   print(llvm::errs());
   llvm::errs() << '\n';
}

void CommandOutput::writeOutputFileMap(llvm::raw_ostream &out) const {
   SmallVector<StringRef, 4> inputs;
   for (const CommandInputPair &input : m_inputs) {
      assert(input.base == input.primary && !input.base.empty() &&
             "output file maps won't work if these differ");
      inputs.push_back(input.primary);
   }
   getDerivedOutputMap().write(out, inputs);
}

Job::~Job() = default;

void Job::printArguments(raw_ostream &ostream,
                         const llvm::opt::ArgStringList &args)
{
   interleave(args,
              [&](const char *Arg) { escapeAndPrintString(ostream, Arg); },
   [&] { ostream << ' '; });
}

void Job::dump() const
{
   printCommandLineAndEnvironment(llvm::errs());
}

ArrayRef<const char *> Job::getArgumentsForTaskExecution() const
{
   if (hasResponseFile()) {
      writeArgsToResponseFile();
      return getResponseFileArg();
   } else {
      return getArguments();
   }
}

void Job::printCommandLineAndEnvironment(raw_ostream &stream,
                                         StringRef terminator) const
{
   printCommandLine(stream, /*terminator=*/"");
   if (!m_extraEnvironment.empty()) {
      stream << "  #";
      for (auto &pair : m_extraEnvironment) {
         stream << " " << pair.first << "=" << pair.second;
      }
   }
   stream << "\n";
}

void Job::printCommandLine(raw_ostream &ostream, StringRef terminator) const
{
   escapeAndPrintString(ostream, m_executable);
   ostream << ' ';
   if (hasResponseFile()) {
      printArguments(ostream, {m_responseFile->argString});
      ostream << " # ";
   }
   printArguments(ostream, m_arguments);
   ostream << terminator;
}

void Job::printSummary(raw_ostream &ostream) const
{
   // Deciding how to describe our inputs is a bit subtle; if we are a Job built
   // from a JobAction that itself has InputActions sources, then we collect
   // those up. Otherwise it's more correct to talk about our inputs as the
   // outputs of our input-jobs.
   SmallVector<StringRef, 4> inputs;
   SmallVector<StringRef, 4> outputs = getOutput().getPrimaryOutputFilenames();

   for (const Action *action : getSource().getInputs()) {
      if (const auto *inputAction = dyn_cast<InputAction>(action)) {
         inputs.push_back(inputAction->getInputArg().getValue());
      }
   }
   for (const Job *job : getInputs()) {
      for (StringRef f : job->getOutput().getPrimaryOutputFilenames()) {
         inputs.push_back(f);
      }
   }

   size_t limit = 3;
   size_t actualIn = inputs.size();
   size_t actualOut = outputs.size();
   if (actualIn > limit) {
      inputs.erase(inputs.begin() + limit, inputs.end());
   }
   if (actualOut > limit) {
      outputs.erase(outputs.begin() + limit, outputs.end());
   }

   ostream << "{" << getSource().getClassName() << ": ";
   interleave(outputs,
              [&](const std::string &Arg) {
      ostream << llvm::sys::path::filename(Arg);
   },
   [&] { ostream << ' '; });
   if (actualOut > limit) {
      ostream << " ... " << (actualOut-limit) << " more";
   }
   ostream << " <= ";
   interleave(inputs,
              [&](const std::string &Arg) {
      ostream << llvm::sys::path::filename(Arg);
   },
   [&] { ostream << ' '; });
   if (actualIn > limit) {
      ostream << " ... " << (actualIn-limit) << " more";
   }
   ostream << "}";
}

bool Job::writeArgsToResponseFile() const
{
   assert(hasResponseFile());
   std::error_code errorCode;
   llvm::raw_fd_ostream ostream(m_responseFile->path, errorCode, llvm::sys::fs::F_None);
   if (errorCode) {
      return true;
   }
   for (const char *arg : m_arguments) {
      escapeAndPrintString(ostream, arg);
      ostream << " ";
   }
   ostream.flush();
   return false;
}

BatchJob::BatchJob(const JobAction &source,
                   SmallVectorImpl<const Job *> &&inputs,
                   std::unique_ptr<CommandOutput> output,
                   const char *executable, llvm::opt::ArgStringList arguments,
                   EnvironmentVector extraEnvironment,
                   std::vector<FilelistInfo> infos,
                   ArrayRef<const Job *> combined, int64_t &nextQuasiPID,
                   Optional<ResponseFileInfo> responseFile)
   : Job(source, std::move(inputs), std::move(output), executable, arguments,
         extraEnvironment, infos, responseFile),
     m_combinedJobs(combined.begin(), combined.end()),
     m_quasiPIDBase(nextQuasiPID)
{

   assert(m_quasiPIDBase < 0);
   nextQuasiPID -= m_combinedJobs.size();
   assert(nextQuasiPID < 0);
}

} // polar::driver
