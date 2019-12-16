//===--- TaskQueue.h - Task Execution Work Queue ----------------*- C++ -*-===//
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
// Created by polarboy on 2019/11/27.

#ifndef POLARPHP_BASIC_TASK_QUEUE_H
#define POLARPHP_BASIC_TASK_QUEUE_H

#include "polarphp/basic/JSONSerialization.h"
#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/Program.h"
#include "polarphp/global/Config.h"

#include <functional>
#include <memory>
#include <queue>

#if defined(HAVE_GETRUSAGE) && !defined(__HAIKU__)
struct rusage;
#endif

namespace polar {
class UnifiedStatsReporter;
}

namespace polar::sys {

class Task; // forward declared to allow for platform-specific implementations

using polar::UnifiedStatsReporter;
using ProcessId = llvm::sys::procid_t;

/// Indicates how a TaskQueue should respond to the task finished event.
enum class TaskFinishedResponse
{
   /// Indicates that execution should continue.
   ContinueExecution,
   /// Indicates that execution should stop (no new tasks will begin execution,
   /// but tasks which are currently executing will be allowed to finish).
   StopExecution,
};

/// TaskProcessInformation is bound to a task and contains information about the
/// process that ran this task. This is especially useful to find out which
/// tasks ran in the same process (in multifile-mode or when WMO is activated
/// e.g.). If available, it also contains information about the usage of
/// resources like CPU time or memory the process used in the system. How ever,
/// this could differ from platform to platform and is therefore optional.

/// One process could handle multiple tasks in some modes of the Swift compiler
/// (multifile, WMO). To not break existing tools, the driver does use unique
/// identifiers for the tasks that are not the process identifier. To still be
/// able to reason about tasks that ran in the same process the
/// TaskProcessInformation struct contains information about the actual process
/// of the operating system. The m_osPid is the actual process identifier and is
/// therefore not guaranteed to be unique over all tasks. The m_processUsage
/// contains optional usage information about the operating system process. It
/// could be used by tools that take those information as input for analyzing
/// the Swift compiler on a process-level. It will be `None` if the execution
/// has been skipped or one of the following symbols are not available on the
/// system: `rusage`, `wait4`.
///
struct TaskProcessInformation
{

   struct ResourceUsage
   {
      // user time in µs
      uint64_t utime;
      // system time in µs
      uint64_t stime;
      // maximum resident set size in Bytes
      uint64_t maxrss;

      ResourceUsage(uint64_t utime, uint64_t stime, uint64_t maxrss)
         : utime(utime),
           stime(stime),
           maxrss(maxrss)
      {}

      virtual ~ResourceUsage() = default;
      virtual void provideMapping(json::Output &out);
   };

public:
   TaskProcessInformation(ProcessId pid, uint64_t utime, uint64_t stime,
                          uint64_t maxrss)
      : m_osPid(pid),
        m_processUsage(ResourceUsage(utime, stime, maxrss))
   {}

   TaskProcessInformation(ProcessId pid)
      : m_osPid(pid),
        m_processUsage(None)
   {}

#if defined(HAVE_GETRUSAGE) && !defined(__HAIKU__)
   TaskProcessInformation(ProcessId pid, struct rusage Usage);
#endif // defined(HAVE_GETRUSAGE) && !defined(__HAIKU__)
   virtual ~TaskProcessInformation() = default;
   virtual void provideMapping(json::Output &out);

private:
   // the process identifier of the operating system
   ProcessId m_osPid;
   // usage information about the process, if available
   Optional<ResourceUsage> m_processUsage;
};

/// A class encapsulating the execution of multiple tasks in parallel.
class TaskQueue
{
public:
   /// Create a new TaskQueue instance.
   ///
   /// \param m_numberOfParallelTasks indicates the number of tasks which should
   /// be run in parallel. If 0, the TaskQueue will choose the most appropriate
   /// number of parallel tasks for the current system.
   /// \param USR Optional stats reporter to count I/O and subprocess events.
   TaskQueue(unsigned m_numberOfParallelTasks = 0,
             UnifiedStatsReporter *USR = nullptr);
   virtual ~TaskQueue();

   // TODO: remove once -Wdocumentation stops warning for \param, \returns on
   // std::function (<rdar://problem/15665132>).
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
   /// A callback which will be executed when each task begins execution
   ///
   /// \param pid the ProcessId of the task which just began execution.
   /// \param context the context which was passed when the task was added
   using TaskBeganCallback = std::function<void(ProcessId pid, void *context)>;

   /// A callback which will be executed after each task finishes
   /// execution.
   ///
   /// \param pid the ProcessId of the task which finished execution.
   /// \param ReturnCode the return code of the task which finished execution.
   /// \param Output the output from the task which finished execution,
   /// if available. (This may not be available on all platforms.)
   /// \param Errors the errors from the task which finished execution, if
   /// available and separateErrors was true.  (This may not be available on all
   /// platforms.)
   /// \param ProcInfo contains information like the operating process identifier
   /// and resource usage if available
   /// \param context the context which was passed when the task was added
   ///
   /// \returns true if further execution of tasks should stop,
   /// false if execution should continue
   using TaskFinishedCallback = std::function<TaskFinishedResponse(
   ProcessId pid, int ReturnCode, StringRef Output, StringRef Errors,
   TaskProcessInformation ProcInfo, void *context)>;

   /// A callback which will be executed if a task exited abnormally due
   /// to a signal.
   ///
   /// \param pid the ProcessId of the task which exited abnormally.
   /// \param ErrorMsg a string describing why the task exited abnormally. If
   /// no reason could be deduced, this may be empty.
   /// \param Output the output from the task which exited abnormally, if
   /// available. (This may not be available on all platforms.)
   /// \param Errors the errors from the task which exited abnormally, if
   /// available and separateErrors was true.  (This may not be available on all
   /// platforms.)
   /// \param ProcInfo contains information like the operating process identifier
   /// and resource usage if available
   /// \param context the context which was passed when the task was added
   /// \param Signal the terminating signal number, if available. This may not be
   /// available on all platforms. If it is ever provided, it should not be
   /// removed in future versions of the compiler.
   ///
   /// \returns a TaskFinishedResponse indicating whether or not execution
   /// should proceed
   using TaskSignalledCallback = std::function<TaskFinishedResponse(
   ProcessId pid, StringRef ErrorMsg, StringRef Output, StringRef Errors,
   void *context, Optional<int> Signal, TaskProcessInformation ProcInfo)>;
#pragma clang diagnostic pop

   /// Indicates whether TaskQueue supports buffering output on the
   /// current system.
   ///
   /// \note If this returns false, the TaskFinishedCallback passed
   /// to \ref execute will always receive an empty StringRef for output, even
   /// if the task actually generated output.
   static bool supportsBufferingOutput();

   /// Indicates whether TaskQueue supports parallel execution on the
   /// current system.
   static bool supportsParallelExecution();

   /// \returns the maximum number of tasks which this TaskQueue will execute in
   /// parallel
   unsigned getNumberOfParallelTasks() const;

   /// Adds a task to the TaskQueue.
   ///
   /// \param execPath the path to the executable which the task should execute
   /// \param args the arguments which should be passed to the task
   /// \param env the environment which should be used for the task;
   /// must be null-terminated. If empty, inherits the parent's environment.
   /// \param context an optional context which will be associated with the task
   /// \param separateErrors Controls whether error output is reported separately
   virtual void addTask(const char *execPath, ArrayRef<const char *> args,
                        ArrayRef<const char *> env = llvm::None,
                        void *context = nullptr, bool separateErrors = false);

   /// Synchronously executes the tasks in the TaskQueue.
   ///
   /// \param began a callback which will be called when a task begins
   /// \param finished a callback which will be called when a task finishes
   /// \param signalled a callback which will be called if a task exited
   /// abnormally due to a signal
   ///
   /// \returns true if all tasks did not execute successfully
   virtual bool
   execute(TaskBeganCallback began = TaskBeganCallback(),
           TaskFinishedCallback finished = TaskFinishedCallback(),
           TaskSignalledCallback signalled = TaskSignalledCallback());

   /// Returns true if there are any tasks that have been queued but have not
   /// yet been executed.
   virtual bool hasRemainingTasks()
   {
      return !m_queuedTasks.empty();
   }

private:
   /// Tasks which have not begun execution.
   std::queue<std::unique_ptr<Task>> m_queuedTasks;

   /// The number of tasks to execute in parallel.
   unsigned m_numberOfParallelTasks;

   /// Optional place to count I/O and subprocess events.
   UnifiedStatsReporter *m_stats;
};

/// A class which simulates execution of tasks with behavior similar to
/// TaskQueue.
class DummyTaskQueue : public TaskQueue
{
   class DummyTask
   {
   public:
      const char *execPath;
      ArrayRef<const char *> args;
      ArrayRef<const char *> env;
      void *context;
      bool separateErrors;

      DummyTask(const char *execPath, ArrayRef<const char *> args,
                ArrayRef<const char *> env = llvm::None, void *context = nullptr,
                bool separateErrors = false)
         : execPath(execPath), args(args), env(env), context(context),
           separateErrors(separateErrors) {}
   };

public:
   /// Create a new DummyTaskQueue instance.
   DummyTaskQueue(unsigned m_numberOfParallelTasks = 0);
   virtual ~DummyTaskQueue() override;

   void addTask(const char *execPath, ArrayRef<const char *> args,
                ArrayRef<const char *> env = llvm::None,
                void *context = nullptr, bool separateErrors = false) override;

   bool
   execute(TaskBeganCallback began = TaskBeganCallback(),
           TaskFinishedCallback finished = TaskFinishedCallback(),
           TaskSignalledCallback signalled = TaskSignalledCallback()) override;

   bool hasRemainingTasks() override
   {
      // Need to override here because m_queuedTasks is redeclared.
      return !m_queuedTasks.empty();
   }

private:
   std::queue<std::unique_ptr<DummyTask>> m_queuedTasks;
};

} // polar::sys

namespace polar::json {
template <>
struct ObjectTraits<sys::TaskProcessInformation>
{
   static void mapping(Output &out, sys::TaskProcessInformation &value)
   {
      value.provideMapping(out);
   }
};

template <>
struct ObjectTraits<sys::TaskProcessInformation::ResourceUsage>
{
   static void mapping(Output &out,
                       sys::TaskProcessInformation::ResourceUsage &value)
   {
      value.provideMapping(out);
   }
};
} // polar::json

#endif // POLARPHP_BASIC_TASK_QUEUE_H
