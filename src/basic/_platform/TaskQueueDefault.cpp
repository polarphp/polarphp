//===--- TaskQueue.inc - Default serial TaskQueue ---------------*- C++ -*-===//
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
// Created by polarboy on 2019/11/30.
///
/// \file
/// This file contains a platform-agnostic implementation of TaskQueue
/// using the functions from llvm/Support/Program.h.
///
/// \note The default implementation of TaskQueue does not support parallel
/// execution, nor does it support output buffering. As a result,
/// platform-specific implementations should be preferred.
///
//===----------------------------------------------------------------------===//

#include "polarphp/basic/TaskQueue.h"
#include "polarphp/basic/internal/_platform/TaskQueueImplDefault.h"
#include "polarphp/basic/LLVM.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Signals.h"

namespace polar::sys {

using namespace llvm::sys;

bool TaskQueue::supportsBufferingOutput()
{
   // The default implementation supports buffering output.
   return true;
}

bool TaskQueue::supportsParallelExecution()
{
   // The default implementation does not support parallel execution.
   return false;
}

unsigned TaskQueue::getNumberOfParallelTasks() const
{
   // The default implementation does not support parallel execution.
   return 1;
}

void TaskQueue::addTask(const char *execPath, ArrayRef<const char *> args,
                        ArrayRef<const char *> env, void *context,
                        bool separateErrors)
{
   std::unique_ptr<Task> T(new Task(execPath, args, env, context, separateErrors));
   m_queuedTasks.push(std::move(T));
}

bool TaskQueue::execute(TaskBeganCallback began, TaskFinishedCallback finished,
                        TaskSignalledCallback signalled)
{
   bool continueExecution = true;

   // This implementation of TaskQueue doesn't support parallel execution.
   // We need to reference m_numberOfParallelTasks to avoid warnings, though.
   (void)m_numberOfParallelTasks;

   while (!m_queuedTasks.empty() && continueExecution) {
      std::unique_ptr<Task> T(m_queuedTasks.front().release());
      m_queuedTasks.pop();

      SmallVector<const char *, 128> argv;
      argv.push_back(T->execPath);
      argv.append(T->args.begin(), T->args.end());
      argv.push_back(nullptr);

      llvm::Optional<llvm::ArrayRef<llvm::StringRef>> Envp =
            T->env.empty() ? decltype(Envp)(None)
                           : decltype(Envp)(llvm::toStringRefArray(T->env.data()));

      llvm::SmallString<64> stdoutPath;
      if (fs::createTemporaryFile("stdout", "tmp",  stdoutPath)) {
         return true;
      }

      llvm::sys::RemoveFileOnSignal(stdoutPath);

      llvm::SmallString<64> stderrPath;
      if (T->separateErrors) {
         if (fs::createTemporaryFile("stderr", "tmp", stdoutPath)) {
            return true;
         }
         llvm::sys::RemoveFileOnSignal(stderrPath);
      }

      Optional<StringRef> redirects[] = {None, {stdoutPath}, {T->separateErrors ? stderrPath : stdoutPath}};

      bool executionFailed = false;
      ProcessInfo processInfo = ExecuteNoWait(T->execPath,
                                              llvm::toStringRefArray(argv.data()), Envp,
                                              /*redirects*/redirects, /*memoryLimit*/0,
                                              /*errMsg*/nullptr, &executionFailed);
      if (executionFailed) {
         return true;
      }

      if (began) {
         began(processInfo.Pid, T->context);
      }

      std::string errMsg;
      processInfo = Wait(processInfo, 0, true, &errMsg);
      int ReturnCode = processInfo.ReturnCode;

      auto stdoutBuffer = llvm::MemoryBuffer::getFile(stdoutPath);
      StringRef stdoutContents = stdoutBuffer.get()->getBuffer();

      StringRef stderrContents;
      if (T->separateErrors) {
         auto stderrBuffer = llvm::MemoryBuffer::getFile(stderrPath);
         stderrContents = stderrBuffer.get()->getBuffer();
      }

#if defined(_WIN32)
      // Wait() sets the upper two bits of the return code to indicate warnings
      // (10) and errors (11).
      //
      // This isn't a true signal on Windows, but we'll treat it as such so that
      // we clean up after it properly
      bool crashed = ReturnCode & 0xC0000000;
#else
      // Wait() returning a return code of -2 indicates the process received
      // a signal during execution.
      bool crashed = ReturnCode == -2;
#endif
      if (crashed) {
         if (signalled) {
            TaskFinishedResponse Response =
                  signalled(processInfo.Pid, errMsg, stdoutContents, stderrContents, T->context, ReturnCode, TaskProcessInformation(processInfo.Pid));
            continueExecution = Response != TaskFinishedResponse::StopExecution;
         } else {
            // If we don't have a signalled callback, unconditionally stop.
            continueExecution = false;
         }
      } else {
         // Wait() returned a normal return code, so just indicate that the task
         // finished.
         if (finished) {
            TaskFinishedResponse Response = finished(processInfo.Pid, processInfo.ReturnCode,
                                                     stdoutContents, stderrContents, TaskProcessInformation(processInfo.Pid), T->context);
            continueExecution = Response != TaskFinishedResponse::StopExecution;
         } else if (processInfo.ReturnCode != 0) {
            continueExecution = false;
         }
      }
      llvm::sys::fs::remove(stdoutPath);
      if (T->separateErrors) {
         llvm::sys::fs::remove(stderrPath);
      }
   }
   return !continueExecution;
}

} // polar::sys
