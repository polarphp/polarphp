// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/30.

#ifndef POLARPHP_BASIC_INTERNAL_PLATFORM_TASK_QUEUE_IMPL_UNIX_H
#define POLARPHP_BASIC_INTERNAL_PLATFORM_TASK_QUEUE_IMPL_UNIX_H

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/ArrayRef.h"

namespace polar {
class UnifiedStatsReporter;
}

namespace polar::sys {

using llvm::ArrayRef;
using llvm::StringRef;

class Task
{
   /// The path to the executable which this Task will execute.
   const char *m_execPath;

   /// Any arguments which should be passed during execution.
   ArrayRef<const char *> m_args;

   /// The environment which will be used during execution. If empty, uses
   /// this process's environment.
   ArrayRef<const char *> m_env;

   /// m_context which should be associated with this task.
   void *m_context;

   /// True if the errors of the Task should be stored in m_errors instead of m_output.
   bool m_separateErrors;

   /// The pid of this Task when executing.
   pid_t m_pid;

   /// A m_pipe for reading output from the child process.
   int m_pipe;

   /// A m_pipe for reading errors from the child prcess, if m_separateErrors is true.
   int m_errorPipe;

   /// The current m_state of the Task.
   enum class TaskState { Preparing, Executing, Finished } m_state;

   /// Once the Task has finished, this contains the buffered output of the Task.
   std::string m_output;

   /// Once the Task has finished, if m_separateErrors is true, this contains the
   /// errors from the Task.
   std::string m_errors;

   /// Optional place to count I/O and subprocess events.
   UnifiedStatsReporter *m_stats;

public:
   Task(const char *execPath, ArrayRef<const char *> args,
        ArrayRef<const char *> env, void *context, bool separateErrors,
        UnifiedStatsReporter *usr)
      : m_execPath(execPath), m_args(args), m_env(env), m_context(context),
        m_separateErrors(separateErrors), m_pid(-1), m_pipe(-1), m_errorPipe(-1),
        m_state(TaskState::Preparing), m_stats(usr)
   {
      assert((m_env.empty() || m_env.back() == nullptr) &&
             "m_env must either be empty or null-terminated!");
   }

   const char *getExecPath() const
   {
      return m_execPath;
   }

   ArrayRef<const char *> getArgs() const
   {
      return m_args;
   }

   StringRef getOutput() const
   {
      return m_output;
   }

   StringRef getErrors() const
   {
      return m_errors;
   }

   void *getContext() const
   {
      return m_context;
   }

   pid_t getPid() const
   {
      return m_pid;
   }

   int getPipe() const
   {
      return m_pipe;
   }

   int getErrorPipe() const
   {
      return m_errorPipe;
   }

   /// Begins execution of this Task.
   /// \returns true on error.
   bool execute();

   /// Reads data from the pipes, if any is available.
   ///
   /// If \p untilEnd is true, reads until the end of the stream; otherwise reads
   /// once (possibly with a retry on EINTR), and returns.
   /// \returns true on error.
   bool readFromPipes(bool untilEnd);

   /// Performs any post-execution work for this Task, such as reading
   /// piped output and closing the m_pipe.
   void finishExecution();
};

} // polar::sys

#endif // POLARPHP_BASIC_INTERNAL_PLATFORM_TASK_QUEUE_IMPL_UNIX_H
