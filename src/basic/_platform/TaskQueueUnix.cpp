//===--- TaskQueue.inc - Unix-specific TaskQueue ----------------*- C++ -*-===//
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

#include "polarphp/basic/TaskQueue.h"
#include "polarphp/basic/internal/_platform/TaskQueueImplUnix.h"
#include "polarphp/basic/Statistic.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/ErrorHandling.h"
#include "polarphp/global/Config.h"

#include <string>
#include <cerrno>

#ifdef HAVE_POSIX_SPAWN
#include <spawn.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(HAVE_GETRUSAGE) && !defined(__HAIKU__)
#include <sys/resource.h>
#endif

#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>

#if !defined(__APPLE__)
extern char **environ;
#else
extern "C" {
// _NSGetEnviron is from crt_externs.h which is missing in the iOS SDK.
extern char ***_NSGetEnviron(void);
}
#endif

namespace polar::sys {

using polar::basic::UnifiedStatsReporter;

#if defined(HAVE_GETRUSAGE) && !defined(__HAIKU__)
TaskProcessInformation::TaskProcessInformation(ProcessId pid, struct rusage Usage)
   : TaskProcessInformation(pid,
                            uint64_t(Usage.ru_utime.tv_sec) * 1000000 +
                            uint64_t(Usage.ru_utime.tv_usec),
                            uint64_t(Usage.ru_stime.tv_sec) * 1000000 +
                            uint64_t(Usage.ru_stime.tv_usec),
                            Usage.ru_maxrss) {
#ifndef __APPLE__
   // Apple systems report bytes; everything else appears to report KB.
   this->ProcessUsage.getValue().Maxrss <<= 10;
#endif // __APPLE__
}
#endif // defined(HAVE_GETRUSAGE) && !defined(__HAIKU__)

bool Task::execute()
{
   assert(m_state < TaskState::Executing && "This Task cannot be executed twice!");
   m_state = TaskState::Executing;

   // Construct argv.
   SmallVector<const char *, 128> m_argv;
   m_argv.push_back(m_execPath);
   m_argv.append(m_args.begin(), m_args.end());
   m_argv.push_back(0); // argv is expected to be null-terminated.

   // Set up the pipe.
   int fullPipe[2];
   pipe(fullPipe);
   m_pipe = fullPipe[0];

   int FullErrorPipe[2];
   if (m_separateErrors) {
      pipe(FullErrorPipe);
      m_errorPipe = FullErrorPipe[0];
   }

   // Get the environment to pass down to the subtask.
   const char *const *envp = m_env.empty() ? nullptr : m_env.data();
   if (!envp) {
#if __APPLE__
      envp = *_NSGetEnviron();
#else
      envp = environ;
#endif
   }

   const char **argvp = m_argv.data();

#ifdef HAVE_POSIX_SPAWN
   posix_spawn_file_actions_t fileActions;
   posix_spawn_file_actions_init(&fileActions);
   posix_spawn_file_actions_adddup2(&fileActions, fullPipe[1], STDOUT_FILENO);
   if (m_separateErrors) {
      posix_spawn_file_actions_adddup2(&fileActions, FullErrorPipe[1],
            STDERR_FILENO);
   } else {
      posix_spawn_file_actions_adddup2(&fileActions, STDOUT_FILENO,
                                       STDERR_FILENO);
   }

   posix_spawn_file_actions_addclose(&fileActions, fullPipe[0]);
   if (m_separateErrors) {
      posix_spawn_file_actions_addclose(&fileActions, FullErrorPipe[0]);
   }

   // Spawn the subtask.
   int spawnErr =
         posix_spawn(&m_pid, m_execPath, &fileActions, nullptr,
                     const_cast<char **>(argvp), const_cast<char **>(envp));

   posix_spawn_file_actions_destroy(&fileActions);
   close(fullPipe[1]);
   if (m_separateErrors) {
      close(FullErrorPipe[1]);
   }

   if (spawnErr != 0 || m_pid == 0) {
      close(fullPipe[0]);
      if (m_separateErrors) {
         close(FullErrorPipe[0]);
      }
      m_state = TaskState::Finished;
      return true;
   }
#else
   m_pid = fork();
   switch (m_pid) {
   case -1: {
      close(fullPipe[0]);
      if (m_separateErrors) {
         close(FullErrorPipe[0]);
      }
      m_state = TaskState::Finished;
      m_pid = 0;
      break;
   }
   case 0: {
      // Child process: Execute the program.
      dup2(fullPipe[1], STDOUT_FILENO);
      if (m_separateErrors) {
         dup2(FullErrorPipe[1], STDERR_FILENO);
      } else {
         dup2(STDOUT_FILENO, STDERR_FILENO);
      }
      close(fullPipe[0]);
      if (m_separateErrors) {
         close(FullErrorPipe[0]);
      }
      execve(m_execPath, const_cast<char **>(argvp), const_cast<char **>(envp));

      // If the execve() failed, we should exit. Follow Unix protocol and
      // return 127 if the executable was not found, and 126 otherwise.
      // Use _exit rather than exit so that atexit functions and static
      // object destructors cloned from the parent process aren't
      // redundantly run, and so that any data buffered in stdio buffers
      // cloned from the parent aren't redundantly written out.
      _exit(errno == ENOENT ? 127 : 126);
   }
   default:
      // Parent process: Break out of the switch to do our processing.
      break;
   }

   close(fullPipe[1]);
   if (m_separateErrors) {
      close(FullErrorPipe[1]);
   }

   if (m_pid == 0)
      return true;
#endif

   return false;
}

/// Read the data in \p m_pipe, and append it to \p output.
/// \p m_pipe must be in blocking mode, and must contain unread data.
/// If \p untilEnd is true, keep reading, and possibly blocking, till the pipe
/// is closed. If \p untilEnd is false, just read once. Return true if error
static bool readFromAPipe(std::string &output, int pipe,
                          UnifiedStatsReporter *m_stats, bool untilEnd)
{
   char outputBuffer[1024];
   ssize_t readBytes = 0;
   while ((readBytes = read(pipe, outputBuffer, sizeof(outputBuffer))) != 0) {
      if (readBytes < 0) {
         if (errno == EINTR)
            // read() was interrupted, so try again.
            // Q: Why isn't there a counter to break out of this loop if there are
            //    more than some number of EINTRs?
            // A: EINTR on a blocking read means only one thing: the syscall was
            //    interrupted and the program should retry. So there is no need to
            //    stop retrying after any particular number of interruptions (any
            //    more than the program would stop reading after a particular number
            //    of bytes or whatever).
            continue;
         return true;
      }
      output.append(outputBuffer, readBytes);
      if (m_stats)
         m_stats->getDriverCounters().NumDriverPipeReads++;
      if (!untilEnd)
         break;
   }
   return false;
}

bool Task::readFromPipes(bool untilEnd)
{
   bool ret = readFromAPipe(m_output, m_pipe, m_stats, untilEnd);
   if (m_separateErrors) {
      ret |= readFromAPipe(m_errors, m_errorPipe, m_stats, untilEnd);
   }
   return ret;
}

void Task::finishExecution()
{
   assert(m_state == TaskState::Executing &&
          "This Task must be executing to finish execution!");

   m_state = TaskState::Finished;

   // Read the output of the command, so we can use it later.
   readFromPipes(/*untilEnd*/ false);

   close(m_pipe);
   if (m_separateErrors) {
      close(m_errorPipe);
   }
}

bool TaskQueue::supportsBufferingOutput()
{
   // The Unix implementation supports buffering output.
   return true;
}

bool TaskQueue::supportsParallelExecution()
{
   // The Unix implementation supports parallel execution.
   return true;
}

unsigned TaskQueue::getNumberOfParallelTasks() const
{
   // TODO: add support for choosing a better default value for
   // MaxNumberOfParallelTasks if m_numberOfParallelTasks is 0. (Optimally, this
   // should choose a value > 1 tailored to the current system.)
   return m_numberOfParallelTasks > 0 ? m_numberOfParallelTasks : 1;
}

void TaskQueue::addTask(const char *m_execPath, ArrayRef<const char *> m_args,
                        ArrayRef<const char *> m_env, void *context,
                        bool m_separateErrors)
{
   std::unique_ptr<Task> task(
            new Task(m_execPath, m_args, m_env, context, m_separateErrors, m_stats));
   m_queuedTasks.push(std::move(task));
}

/// Owns Tasks, handles correspondence between Tasks, file descriptors, and
/// process IDs.
/// FIXME: only handles stdout pipes, ignores stderr pipes.
class TaskMap
{
   using PidToTaskMap = llvm::DenseMap<pid_t, std::unique_ptr<Task>>;
   PidToTaskMap m_tasksByPid;

public:
   TaskMap() = default;

   bool empty() const
   {
      return m_tasksByPid.empty();
   }

   unsigned size() const
   {
      return m_tasksByPid.size();
   }

   void add(std::unique_ptr<Task> task)
   {
      m_tasksByPid[task->getPid()] = std::move(task);
   }

   Task &findTaskForFd(const int fd)
   {
      auto predicate = [&fd](PidToTaskMap::value_type &value) -> bool {
         return value.second->getPipe() == fd;
      };
      auto iter = std::find_if(m_tasksByPid.begin(), m_tasksByPid.end(), predicate);
      assert(iter != m_tasksByPid.end() &&
            "All outstanding fds must be associated with a  Task");
      return *iter->second;
   }

   void destroyTask(Task &task) { m_tasksByPid.erase(task.getPid()); }
};

/// Concurrently execute the tasks in the TaskQueue, collecting the outputs from
/// each task.
/// Maintain invarients connecting tasks to execute, tasks currently executing,
/// and fds being polled. These invarients include:
/// A task is not in both TasksToBeExecuted and TasksBeingExecuted,
/// A task is executing iff it is in TasksBeingExecuted,
/// A task is executing iff any of its fds being polled are in fdsBeingPolled
/// (These should be all of its output fds, but today is only stdout.)
/// When a task has finished executing, wait for it to die, takes
/// action appropriate to the cause of death, then reclaim its
/// storage.
class TaskMonitor
{
   std::queue<std::unique_ptr<Task>> &TasksToBeExecuted;
   TaskMap TasksBeingExecuted;

   std::vector<struct pollfd> fdsBeingPolled;

   const unsigned MaxNumberOfParallelTasks;

public:
   struct Callbacks
   {
      const TaskQueue::TaskBeganCallback taskBegan;
      const TaskQueue::TaskFinishedCallback taskFinished;
      const TaskQueue::TaskSignalledCallback taskSignalled;
      const std::function<void()> polledAnFd;
   };

private:
   Callbacks m_callbacks;

public:
   TaskMonitor(std::queue<std::unique_ptr<Task>> &TasksToBeExecuted,
               const unsigned m_numberOfParallelTasks, const Callbacks &callbacks)
      : TasksToBeExecuted(TasksToBeExecuted),
        MaxNumberOfParallelTasks(
           m_numberOfParallelTasks == 0 ? 1 : m_numberOfParallelTasks),
        m_callbacks(callbacks)
   {}

   /// Run the tasks to be executed.
   /// \return true on error.
   bool executeTasks();

private:
   bool isFinishedExecutingTasks() const
   {
      return TasksBeingExecuted.empty() && TasksToBeExecuted.empty();
   }

   /// Start up tasks if we aren't already at the parallel limit, and no earlier
   /// subtasks have failed.
   /// \return true on error.
   bool startUpSomeTasks();

   /// \return true on error.
   bool beginExecutingATask(Task &task);

   /// Enter the task and its outputs in this TaskMonitor's data structures so
   /// it can be polled.
   void startPollingFdsOfTask(const Task &task);

   void stopPolling(ArrayRef<int> finishedFds);

   enum class PollResult
   {
      HardError,
      SoftError,
      NoError
   };
   PollResult pollTheFds();

   /// \return None on error.
   Optional<std::vector<int>> readFromReadyFdsReturningFinishedOnes();

   /// Ensure that events bits returned from polling are what's expected.
   void verifyEvents(short events) const;

   void readDataIfAvailable(short events, int fd, Task &task) const;

   bool didTaskHangup(short events) const;
};

bool TaskMonitor::executeTasks()
{
   while (!isFinishedExecutingTasks()) {
      if (startUpSomeTasks()) {
         return true;
      }

      switch (pollTheFds()) {
      case PollResult::HardError:
         return true;
      case PollResult::SoftError:
         continue;
      case PollResult::NoError:
         break;
      }
      Optional<std::vector<int>> finishedFds =
            readFromReadyFdsReturningFinishedOnes();
      if (!finishedFds) {
         return true;
      }
      stopPolling(*finishedFds);
   }
   return false;
}

bool TaskMonitor::startUpSomeTasks()
{
   while (!TasksToBeExecuted.empty() &&
          TasksBeingExecuted.size() < MaxNumberOfParallelTasks) {
      std::unique_ptr<Task> task(TasksToBeExecuted.front().release());
      TasksToBeExecuted.pop();
      if (beginExecutingATask(*task))
         return true;
      startPollingFdsOfTask(*task);
      TasksBeingExecuted.add(std::move(task));
   }
   return false;
}

void TaskMonitor::startPollingFdsOfTask(const Task &task)
{
   fdsBeingPolled.push_back({task.getPipe(), POLLIN | POLLPRI | POLLHUP, 0});
   // We should also poll task->getErrorPipe(), but this introduces timing
   // issues with shutting down the task after reading getPipe().
}

TaskMonitor::PollResult TaskMonitor::pollTheFds()
{
   assert(!fdsBeingPolled.empty() &&
          "We should only call poll() if we have fds to watch!");
   int ReadyFdCount = poll(fdsBeingPolled.data(), fdsBeingPolled.size(), -1);
   if (m_callbacks.polledAnFd)
      m_callbacks.polledAnFd();
   if (ReadyFdCount != -1)
      return PollResult::NoError;
   return errno == EAGAIN || errno == EINTR ? PollResult::SoftError
                                            : PollResult::HardError;
}

bool TaskMonitor::beginExecutingATask(Task &task)
{
   if (task.execute())
      return true;
   if (m_callbacks.taskBegan)
      m_callbacks.taskBegan(task.getPid(), task.getContext());
   return false;
}

static bool
cleanup_ahungup_task(Task &task,
                     const TaskQueue::TaskFinishedCallback finishedCallback,
                     TaskQueue::TaskSignalledCallback signalledCallback);

/**
 Wait for the process with a given pid to finish.

 @param pidToWaitFor the pid of the process to wait for
 @return status information of the wait call and information about process
 */
static std::pair<Optional<int>, TaskProcessInformation> waitForPid(const pid_t pidToWaitFor);
static bool
cleanup_after_signal(int status, const Task &task, TaskProcessInformation procInfo,
                     const TaskQueue::TaskSignalledCallback signalledCallback);
static bool
cleanup_after_exit(int status, const Task &task, TaskProcessInformation procInfo,
                   const TaskQueue::TaskFinishedCallback finishedCallback);

Optional<std::vector<int>>
TaskMonitor::readFromReadyFdsReturningFinishedOnes()
{
   std::vector<int> finishedFds;
   for (struct pollfd &fd : fdsBeingPolled) {
      const int fileDes = fd.fd;
      const short receivedEvents = fd.revents;
      fd.revents = 0;
      verifyEvents(receivedEvents);
      Task &task = TasksBeingExecuted.findTaskForFd(fileDes);
      readDataIfAvailable(receivedEvents, fileDes, task);
      if (!didTaskHangup(receivedEvents)) {
         continue;
      }
      finishedFds.push_back(fileDes);
      const bool hadError =
            cleanup_ahungup_task(task, m_callbacks.taskFinished, m_callbacks.taskSignalled);
      TasksBeingExecuted.destroyTask(task);
      if (hadError) {
         return None;
      }
   }
   return finishedFds;
}

void TaskMonitor::verifyEvents(const short events) const
{
   // We passed an invalid fd; this should never happen,
   // since we always mark fds as finished after calling
   // Task::finishExecution() (which closes the Task's fd).
   assert((events & POLLNVAL) == 0 && "Asked poll() to watch a closed fd");
   const short expectedEvents = POLLIN | POLLPRI | POLLHUP | POLLERR;
   assert((events & ~expectedEvents) == 0 && "Received unexpected event");
   (void)expectedEvents;
}

void TaskMonitor::readDataIfAvailable(const short events, const int fd,
                                      Task &task) const
{
   if (events & (POLLIN | POLLPRI)) {
      // There's data available to read. Read _some_ of it here, but not
      // necessarily _all_, since the pipe is in blocking mode and we might
      // have other input pending (or soon -- before this subprocess is done
      // writing) from other subprocesses.
      //
      // FIXME: longer term, this should probably either be restructured to
      // use O_NONBLOCK, or at very least poll the stderr file descriptor as
      // well; the whole loop here is a bit of a mess.
      task.readFromPipes(/*untilEnd*/ false);
   }
}

bool TaskMonitor::didTaskHangup(const short events) const {
   return (events & (POLLHUP | POLLERR)) != 0;
}

static bool
cleanup_ahungup_task(Task &task,
                     const TaskQueue::TaskFinishedCallback finishedCallback,
                     const TaskQueue::TaskSignalledCallback signalledCallback)
{
   const auto statusAndProcessInformation = waitForPid(task.getPid());
   if (!statusAndProcessInformation.first)
      return true;

   task.finishExecution();
   int status = *(statusAndProcessInformation.first);
   TaskProcessInformation procInfo = statusAndProcessInformation.second;
   return WIFEXITED(status)
         ? cleanup_after_exit(status, task, procInfo, finishedCallback)
         : WIFSIGNALED(status)
           ? cleanup_after_signal(status, task, procInfo, signalledCallback)
           : false /* Can this case ever happen? */;
}

static std::pair<Optional<int>, TaskProcessInformation> waitForPid(const pid_t pidToWaitFor)
{
   for (;;) {
      int status = 0;

#if defined(HAVE_GETRUSAGE) && !defined(__HAIKU__) && defined(HAVE_WAIT4)
      struct rusage Usage;
      const pid_t pidFromWait = wait4(pidToWaitFor, &status, 0, &Usage);
      TaskProcessInformation procInfo(pidToWaitFor, Usage);
#else
      const pid_t pidFromWait = waitpid(pidToWaitFor, &status, 0);
      TaskProcessInformation procInfo(pidToWaitFor);
#endif
      if (pidFromWait == pidToWaitFor) {
         return std::make_pair(status, procInfo);
      }
      assert(pidFromWait == -1 &&
             "Did not pass WNOHANG, should only get pidToWaitFor or -1");
      if (errno == ECHILD || errno == EINVAL) {
         return std::make_pair(None, TaskProcessInformation(pidToWaitFor));
      }
   }
}

static bool
cleanup_after_exit(int status, const Task &task, TaskProcessInformation procInfo,
                   const TaskQueue::TaskFinishedCallback finishedCallback)
{
   const int Result = WEXITSTATUS(status);
   if (!finishedCallback) {
      // Since we don't have a TaskFinishedCallback, treat a subtask
      // which returned a nonzero exit code as having failed.
      return Result != 0;
   }
   // If we have a TaskFinishedCallback, only have an error if the callback
   // returns StopExecution.
   return TaskFinishedResponse::StopExecution ==
         finishedCallback(task.getPid(), Result, task.getOutput(), task.getErrors(), procInfo,
                          task.getContext());
}

static bool
cleanup_after_signal(int status, const Task &task, TaskProcessInformation procInfo,
                     const TaskQueue::TaskSignalledCallback signalledCallback)
{
   // The process exited due to a signal.
   const int signal = WTERMSIG(status);
   StringRef errorMsg = strsignal(signal);

   if (!signalledCallback) {
      // Since we don't have a TaskCrashedCallback, treat a crashing
      // subtask as having failed.
      return true;
   }
   // If we have a TaskCrashedCallback, only return an error if the callback
   // returns StopExecution.
   return TaskFinishedResponse::StopExecution ==
         signalledCallback(task.getPid(), errorMsg, task.getOutput(), task.getErrors(),
                           task.getContext(), signal, procInfo);
}

void TaskMonitor::stopPolling(ArrayRef<int> finishedFds)
{
   // Remove any fds which we've closed from fdsBeingPolled.
   for (int fd : finishedFds) {
      auto predicate = [&fd](struct pollfd &i) { return i.fd == fd; };
      auto iter =
            std::find_if(fdsBeingPolled.begin(), fdsBeingPolled.end(), predicate);
      assert(iter != fdsBeingPolled.end() &&
            "The finished fd must be in fdsBeingPolled!");
      fdsBeingPolled.erase(iter);
   }
}

bool TaskQueue::execute(TaskBeganCallback beganCallback,
                        TaskFinishedCallback finishedCallback,
                        TaskSignalledCallback signalledCallback)
{
   TaskMonitor::Callbacks m_callbacks{
      beganCallback, finishedCallback, signalledCallback, [&] {
         if (m_stats)
            ++m_stats->getDriverCounters().NumDriverPipePolls;
      }};

   TaskMonitor taskMonitor(m_queuedTasks, getNumberOfParallelTasks(), m_callbacks);
   return taskMonitor.executeTasks();
}

} // polar::sys
