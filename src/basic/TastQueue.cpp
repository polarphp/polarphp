//===--- TaskQueue.cpp - Task Execution Work Queue ------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2019/04/25.
//===----------------------------------------------------------------------===//
///
/// \file
/// This file includes the appropriate platform-specific TaskQueue
/// implementation (or the default serial fallback if one is not available),
/// as well as any platform-agnostic TaskQueue functionality.
///
//===----------------------------------------------------------------------===//

#include "polarphp/basic/TaskQueue.h"
// Include the correct TaskQueue implementation.
#if LLVM_ON_UNIX && !defined(__CYGWIN__) && !defined(__HAIKU__)
#include "polarphp/basic/internal/_platform/TaskQueueImplUnix.h"
#else
#include "polarphp/basic/internal/_platform/TaskQueueImplDefault.h"
#endif

namespace polar::sys {

using polar::json::Output;

void TaskProcessInformation::ResourceUsage::provideMapping(Output &out)
{
   out.mapRequired("utime", utime);
   out.mapRequired("stime", stime);
   out.mapRequired("maxrss", maxrss);
}

void TaskProcessInformation::provideMapping(Output &out)
{
   out.mapRequired("real_pid", m_osPid);
   if (m_processUsage.hasValue()) {
      out.mapRequired("usage", m_processUsage.getValue());
   }
}

TaskQueue::TaskQueue(unsigned numberOfParallelTasks,
                     UnifiedStatsReporter *usr)
   : m_numberOfParallelTasks(numberOfParallelTasks),
     m_stats(usr)
{}

TaskQueue::~TaskQueue() = default;

// DummyTaskQueue implementation

DummyTaskQueue::DummyTaskQueue(unsigned numberOfParallelTasks)
   : TaskQueue(numberOfParallelTasks)
{}

DummyTaskQueue::~DummyTaskQueue() = default;

void DummyTaskQueue::addTask(const char *execPath, ArrayRef<const char *> args,
                             ArrayRef<const char *> env, void *context,
                             bool separateErrors)
{
   m_queuedTasks.emplace(std::unique_ptr<DummyTask>(
                            new DummyTask(execPath, args, env, context, separateErrors)));
}

bool DummyTaskQueue::execute(TaskQueue::TaskBeganCallback began,
                             TaskQueue::TaskFinishedCallback finished,
                             TaskQueue::TaskSignalledCallback signalled)
{
   using PidTaskPair = std::pair<ProcessId, std::unique_ptr<DummyTask>>;
   std::queue<PidTaskPair> executingTasks;

   bool subtaskFailed = false;

   static ProcessId pid = 0;

   unsigned maxNumberOfParallelTasks = getNumberOfParallelTasks();

   while ((!m_queuedTasks.empty() && !subtaskFailed) ||
          !executingTasks.empty()) {
      // Enqueue additional tasks if we have additional tasks, we aren't already
      // at the parallel limit, and no earlier subtasks have failed.
      while (!subtaskFailed && !m_queuedTasks.empty() &&
             executingTasks.size() < maxNumberOfParallelTasks) {
         std::unique_ptr<DummyTask> T(m_queuedTasks.front().release());
         m_queuedTasks.pop();

         if (began) {
            began(++pid, T->context);
         }
         executingTasks.push(PidTaskPair(pid, std::move(T)));
      }

      // Finish the first scheduled task.
      PidTaskPair taskPair = std::move(executingTasks.front());
      executingTasks.pop();

      if (finished) {
         std::string output = "output placeholder\n";
         std::string errors =
               taskPair.second->separateErrors ? "Error placeholder\n" : "";
         if (finished(taskPair.first, 0, output, errors, TaskProcessInformation(pid),
                      taskPair.second->context) == TaskFinishedResponse::StopExecution) {
            subtaskFailed = true;
         }
      }
   }
   return false;
}

} // polar::sys
