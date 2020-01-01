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

class Task {
   /// The path to the executable which this Task will execute.
   const char *ExecPath;

   /// Any arguments which should be passed during execution.
   ArrayRef<const char *> Args;

   /// The environment which will be used during execution. If empty, uses
   /// this process's environment.
   ArrayRef<const char *> Env;

   /// Context which should be associated with this task.
   void *Context;

   /// True if the errors of the Task should be stored in Errors instead of Output.
   bool SeparateErrors;

   /// The pid of this Task when executing.
   pid_t Pid;

   /// A pipe for reading output from the child process.
   int Pipe;

   /// A pipe for reading errors from the child prcess, if SeparateErrors is true.
   int ErrorPipe;

   /// The current state of the Task.
   enum class TaskState { Preparing, Executing, Finished } State;

   /// Once the Task has finished, this contains the buffered output of the Task.
   std::string Output;

   /// Once the Task has finished, if SeparateErrors is true, this contains the
   /// errors from the Task.
   std::string Errors;

   /// Optional place to count I/O and subprocess events.
   UnifiedStatsReporter *Stats;

public:
   Task(const char *ExecPath, ArrayRef<const char *> Args,
        ArrayRef<const char *> Env, void *Context, bool SeparateErrors,
        UnifiedStatsReporter *USR)
      : ExecPath(ExecPath), Args(Args), Env(Env), Context(Context),
        SeparateErrors(SeparateErrors), Pid(-1), Pipe(-1), ErrorPipe(-1),
        State(TaskState::Preparing), Stats(USR) {
      assert((Env.empty() || Env.back() == nullptr) &&
             "Env must either be empty or null-terminated!");
   }

   const char *getExecPath() const { return ExecPath; }
   ArrayRef<const char *> getArgs() const { return Args; }
   StringRef getOutput() const { return Output; }
   StringRef getErrors() const { return Errors; }
   void *getContext() const { return Context; }
   pid_t getPid() const { return Pid; }
   int getPipe() const { return Pipe; }
   int getErrorPipe() const { return ErrorPipe; }

   /// Begins execution of this Task.
   /// \returns true on error.
   bool execute();

   /// Reads data from the pipes, if any is available.
   ///
   /// If \p UntilEnd is true, reads until the end of the stream; otherwise reads
   /// once (possibly with a retry on EINTR), and returns.
   /// \returns true on error.
   bool readFromPipes(bool UntilEnd);

   /// Performs any post-execution work for this Task, such as reading
   /// piped output and closing the pipe.
   void finishExecution();
};

} // polar::sys

#endif // POLARPHP_BASIC_INTERNAL_PLATFORM_TASK_QUEUE_IMPL_UNIX_H
