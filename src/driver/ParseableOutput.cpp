//===--- ParseableOutput.cpp - Helpers for parseable output ---------------===//
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
// Created by polarboy on 2019/12/02.

#include "polarphp/driver/ParseableOutput.h"
#include "polarphp/driver/Action.h"
#include "polarphp/driver/Job.h"
#include "polarphp/basic/FileTypes.h"
#include "polarphp/basic/JSONSerialization.h"
#include "polarphp/basic/TaskQueue.h"
#include "llvm/Option/Arg.h"
#include "llvm/Support/raw_ostream.h"

using namespace polar::driver::parseableoutput;
using namespace polar::driver;
using namespace polar::sys;
using namespace polar;

namespace {
struct CommandInput
{
   std::string path;
   CommandInput() {}
   CommandInput(StringRef path)
      : path(path)
   {}
};

using OutputPair = std::pair<filetypes::FileTypeId, std::string>;
} // end anonymous namespace

namespace polar::json {
template<>
struct ScalarTraits<CommandInput>
{
   static void output(const CommandInput &value, llvm::raw_ostream &ostream)
   {
      ostream << value.path;
   }

   static bool mustQuote(StringRef)
   {
      return true;
   }
};

template <>
struct ScalarEnumerationTraits<filetypes::FileTypeId>
{
   static void enumeration(Output &out, filetypes::FileTypeId &value)
   {
      filetypes::for_all_types([&](filetypes::FileTypeId ty)
      {
         std::string typeName = filetypes::get_type_name(ty);
         out.enumCase(value, typeName.c_str(), ty);
      });
   }
};

template <>
struct ObjectTraits<std::pair<filetypes::FileTypeId, std::string>>
{
   static void mapping(Output &out,
                       std::pair<filetypes::FileTypeId, std::string> &value)
   {
      out.mapRequired("type", value.first);
      out.mapRequired("path", value.second);
   }
};

template<typename T, unsigned N>
struct ArrayTraits<SmallVector<T, N>>
{
   static size_t size(Output &out, SmallVector<T, N> &seq)
   {
      return seq.size();
   }

   static T &element(Output &out, SmallVector<T, N> &seq, size_t index)
   {
      if (index >= seq.size()) {
         seq.resize(index+1);
      }
      return seq[index];
   }
};
} // polar::json

namespace {

class Message
{
   std::string m_kind;
   std::string m_name;
public:
   Message(StringRef kind, StringRef name)
      : m_kind(kind),
        m_name(name)
   {}
   virtual ~Message() = default;

   virtual void provideMapping(polar::json::Output &out)
   {
      out.mapRequired("kind", m_kind);
      out.mapRequired("name", m_name);
   }
};

class CommandBasedMessage : public Message
{
public:
   CommandBasedMessage(StringRef kind, const Job &cmd) :
      Message(kind, cmd.getSource().getClassName())
   {}
};

class DetailedCommandBasedMessage : public CommandBasedMessage
{
   std::string m_executable;
   SmallVector<std::string, 16> m_arguments;
   std::string m_commandLine;
   SmallVector<CommandInput, 4> m_inputs;
   SmallVector<OutputPair, 8> m_outputs;
public:
   DetailedCommandBasedMessage(StringRef kind, const Job &cmd)
      : CommandBasedMessage(kind, cmd)
   {
      m_executable = cmd.getExecutable();
      for (const auto &action : cmd.getArguments()) {
         m_arguments.push_back(action);
      }
      llvm::raw_string_ostream wrapper(m_commandLine);
      cmd.printCommandLine(wrapper, "");
      wrapper.flush();
      for (const Action *action : cmd.getSource().getInputs()) {
         if (const auto *inputAction = dyn_cast<InputAction>(action)) {
            m_inputs.push_back(CommandInput(inputAction->getInputArg().getValue()));
         }
      }

      for (const Job *job : cmd.getInputs()) {
         auto outFiles = job->getOutput().getPrimaryOutputFilenames();
         if (const auto *BJAction = dyn_cast<BackendJobAction>(&cmd.getSource())) {
            m_inputs.push_back(CommandInput(outFiles[BJAction->getInputIndex()]));
         } else {
            for (const std::string &filename : outFiles) {
               m_inputs.push_back(CommandInput(filename));
            }
         }
      }

      // TODO: set up m_outputs appropriately.
      filetypes::FileTypeId primaryOutputType = cmd.getOutput().getPrimaryOutputType();
      if (primaryOutputType != filetypes::TY_Nothing) {
         for (const std::string &outputFileName : cmd.getOutput().getPrimaryOutputFilenames()) {
            m_outputs.push_back(OutputPair(primaryOutputType, outputFileName));
         }
      }
      filetypes::for_all_types([&](filetypes::FileTypeId type) {
         for (auto Output : cmd.getOutput().getAdditionalOutputsForType(type)) {
            m_outputs.push_back(OutputPair(type, Output));
         }
      });
   }

   void provideMapping(polar::json::Output &out) override
   {
      Message::provideMapping(out);
      out.mapRequired("command", m_commandLine); // Deprecated, do not document
      out.mapRequired("command_executable", m_executable);
      out.mapRequired("command_arguments", m_arguments);
      out.mapOptional("inputs", m_inputs);
      out.mapOptional("outputs", m_outputs);
   }
};

class TaskBasedMessage : public CommandBasedMessage
{
   int64_t m_pid;
public:
   TaskBasedMessage(StringRef kind, const Job &cmd, int64_t pid) :
      CommandBasedMessage(kind, cmd), m_pid(pid) {}

   void provideMapping(polar::json::Output &out) override
   {
      CommandBasedMessage::provideMapping(out);
      out.mapRequired("pid", m_pid);
   }
};

class BeganMessage : public DetailedCommandBasedMessage
{
   int64_t m_pid;
   TaskProcessInformation m_procInfo;

public:
   BeganMessage(const Job &cmd, int64_t pid, TaskProcessInformation procInfo)
      : DetailedCommandBasedMessage("began", cmd), m_pid(pid),
        m_procInfo(procInfo) {}

   void provideMapping(polar::json::Output &out) override
   {
      DetailedCommandBasedMessage::provideMapping(out);
      out.mapRequired("pid", m_pid);
      out.mapRequired("process", m_procInfo);
   }
};

class TaskOutputMessage : public TaskBasedMessage
{
   std::string m_output;
   TaskProcessInformation m_procInfo;

public:
   TaskOutputMessage(StringRef kind, const Job &cmd, int64_t pid,
                     StringRef output, TaskProcessInformation procInfo)
      : TaskBasedMessage(kind, cmd, pid), m_output(output), m_procInfo(procInfo)
   {}

   void provideMapping(polar::json::Output &out) override
   {
      TaskBasedMessage::provideMapping(out);
      out.mapOptional("output", m_output, std::string());
      out.mapRequired("process", m_procInfo);
   }
};

class FinishedMessage : public TaskOutputMessage
{
   int m_exitStatus;
public:
   FinishedMessage(const Job &cmd, int64_t pid, StringRef output,
                   TaskProcessInformation procInfo, int exitStatus)
      : TaskOutputMessage("finished", cmd, pid, output, procInfo),
        m_exitStatus(exitStatus)
   {}

   void provideMapping(polar::json::Output &out) override
   {
      TaskOutputMessage::provideMapping(out);
      out.mapRequired("exit-status", m_exitStatus);
   }
};

class SignalledMessage : public TaskOutputMessage
{
   std::string m_errorMsg;
   Optional<int> m_signal;
public:
   SignalledMessage(const Job &cmd, int64_t pid, StringRef output,
                    StringRef errorMsg, Optional<int> signal,
                    TaskProcessInformation procInfo)
      : TaskOutputMessage("signalled", cmd, pid, output, procInfo),
        m_errorMsg(errorMsg),
        m_signal(signal)
   {}

   void provideMapping(polar::json::Output &out) override
   {
      TaskOutputMessage::provideMapping(out);
      out.mapOptional("error-message", m_errorMsg, std::string());
      out.mapOptional("signal", m_signal);
   }
};

class SkippedMessage : public DetailedCommandBasedMessage
{
public:
   SkippedMessage(const Job &cmd) :
      DetailedCommandBasedMessage("skipped", cmd)
   {}
};

} // end anonymous namespace

namespace polar::json {

template<>
struct ObjectTraits<Message>
{
   static void mapping(Output &out, Message &msg)
   {
      msg.provideMapping(out);
   }
};

} // polar::json

static void emitMessage(raw_ostream &ostream, Message &msg)
{
   std::string JSONString;
   llvm::raw_string_ostream BufferStream(JSONString);
   json::Output yout(BufferStream);
   yout << msg;
   BufferStream.flush();
   ostream << JSONString.length() << '\n';
   ostream << JSONString << '\n';
}

void parseableoutput::emit_began_message(raw_ostream &ostream, const Job &cmd,
                                         int64_t pid,
                                         TaskProcessInformation procInfo)
{
   BeganMessage msg(cmd, pid, procInfo);
   emitMessage(ostream, msg);
}

void parseableoutput::emit_finished_message(raw_ostream &ostream, const Job &cmd,
                                            int64_t pid, int exitStatus,
                                            StringRef output,
                                            TaskProcessInformation procInfo)
{
   FinishedMessage msg(cmd, pid, output, procInfo, exitStatus);
   emitMessage(ostream, msg);
}

void parseableoutput::emit_signalled_message(raw_ostream &ostream, const Job &cmd,
                                             int64_t pid, StringRef errorMsg,
                                             StringRef output,
                                             Optional<int> signal,
                                             TaskProcessInformation procInfo)
{
   SignalledMessage msg(cmd, pid, output, errorMsg, signal, procInfo);
   emitMessage(ostream, msg);
}

void parseableoutput::emit_skipped_message(raw_ostream &ostream, const Job &cmd)
{
   SkippedMessage msg(cmd);
   emitMessage(ostream, msg);
}
