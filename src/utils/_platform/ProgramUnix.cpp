// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/09.

#include "polarphp/utils/unix/Unix.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/global/Config.h"
#include "polarphp/utils/ErrorCode.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/Path.h"
#include "polarphp/utils/RawOutStream.h"
#include "polarphp/utils/Program.h"
#include "polarphp/utils/StringSaver.h"
#include "polarphp/utils/ErrorNumber.h"

#ifdef POLAR_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#ifdef POLAR_HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef POLAR_HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef POLAR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_POSIX_SPAWN
#include <spawn.h>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
#define USE_NSGETENVIRON 1
#else
#define USE_NSGETENVIRON 0
#endif

#if !USE_NSGETENVIRON
extern char **environ;
#else
#include <crt_externs.h> // _NSGetEnviron
#endif
#endif

namespace polar {
namespace sys {

using polar::basic::SmallVector;
using polar::basic::SmallString;
using polar::utils::ErrorCode;
using polar::utils::StringSaver;
using polar::utils::BumpPtrAllocator;

ProcessInfo::ProcessInfo()
   : m_pid(0),
     m_returnCode(0)
{}

ProcessIdType ProcessInfo::getPid() const
{
   return m_pid;
}

ProcessIdType ProcessInfo::getProcess() const
{
   return m_process;
}

int ProcessInfo::getReturnCode() const
{
   return m_returnCode;
}

std::optional<StringRef> ProcessInfo::getStdinFilename() const
{
   if (m_stdinFilename.has_value()) {
      return m_stdinFilename.value();
   }
   return std::nullopt;
}

std::optional<StringRef> ProcessInfo::getStdoutFilename() const
{
   if (m_stdoutFilename.has_value()) {
      return m_stdoutFilename.value();
   }
   return std::nullopt;
}

std::optional<StringRef> ProcessInfo::getStderrFilename() const
{
   if (m_stderrFilename.has_value()) {
      return m_stderrFilename.value();
   }
   return std::nullopt;
}

OptionalError<std::string> find_program_by_name(StringRef name,
                                                ArrayRef<StringRef> paths) {
   assert(!name.empty() && "Must have a name!");
   // Use the given path verbatim if it contains any slashes; this matches
   // the behavior of sh(1) and friends.
   if (name.find('/') != StringRef::npos) {
      return std::string(name);
   }
   SmallVector<StringRef, 16> environmentPaths;
   if (paths.empty()) {
      if (const char *pathEnv = std::getenv("PATH")) {
         polar::basic::split_string(pathEnv, environmentPaths, ":");
         paths = environmentPaths;
      }
   }
   for (auto path : paths) {
      if (path.empty()) {
         continue;
      }
      // Check to see if this first directory contains the executable...
      SmallString<128> filePath(path);
      fs::path::append(filePath, name);
      if (fs::can_execute(filePath.getCStr())) {
         return std::string(filePath.getStr()); // Found the executable!
      }
   }
   return ErrorCode::no_such_file_or_directory;
}

OptionalError<std::string>
find_program_by_name(StringRef name, const std::list<std::string> &paths)
{
   SmallVector<StringRef, 16> pathsRef;
   for (const std::string &path: paths) {
      pathsRef.pushBack(path);
   }
   return find_program_by_name(name, pathsRef);
}

namespace {

bool redirect_io(std::optional<StringRef> path, int fd, std::optional<int> mode, std::string* errMsg)
{
   if (!path) {// Noop
      return false;
   }
   std::string file;
   if (path->empty()) {
      // Redirect empty paths to /dev/null
      file = "/dev/null";
   } else {
      file = *path;
   }
   if (!mode) {
      mode = fd == 0 ? O_RDONLY : O_WRONLY|O_CREAT;
   }

   // Open the file
   int inFD = open(file.c_str(), mode.value(), 0666);
   if (inFD == -1) {
      make_error_msg(errMsg, "Cannot open file '" + file + "' for "
                     + (fd == 0 ? "input" : "output"));
      return true;
   }

   // Install it as the requested FD
   if (dup2(inFD, fd) == -1) {
      make_error_msg(errMsg, "Cannot dup2");
      close(inFD);
      return true;
   }
   close(inFD);      // Close the original FD
   return false;
}

#ifdef HAVE_POSIX_SPAWN
bool redirect_io_ps(const std::string *path, int fd, std::optional<int> mode, std::string *errMsg,
                    posix_spawn_file_actions_t *fileActions)
{
   if (!path) {// Noop
      return false;
   }
   const char *file;
   if (path->empty()) {
      // Redirect empty paths to /dev/null
      file = "/dev/null";
   } else {
      file = path->c_str();
   }
   if (!mode) {
      mode = fd == 0 ? O_RDONLY : O_WRONLY|O_CREAT;
   }
   if (int error = posix_spawn_file_actions_addopen(
          fileActions, fd, file,
          mode.value(), 0666)) {
      return make_error_msg(errMsg, "Cannot dup2", error);
   }
   return false;
}

#endif

void timeout_handler(int sig)
{
}

void set_memory_limits(unsigned size)
{
#if defined(HAVE_SYS_RESOURCE_H) && defined(POLAR_HAVE_GETRLIMIT) && defined(POLAR_HAVE_SETRLIMIT)
   struct rlimit r;
   __typeof__ (r.rlim_cur) limit = (__typeof__ (r.rlim_cur)) (size) * 1048576;

   // Heap size
   getrlimit (RLIMIT_DATA, &r);
   r.rlim_cur = limit;
   setrlimit (RLIMIT_DATA, &r);
#ifdef RLIMIT_RSS
   // Resident set size.
   getrlimit (RLIMIT_RSS, &r);
   r.rlim_cur = limit;
   setrlimit (RLIMIT_RSS, &r);
#endif
#endif
}

std::vector<const char *>
to_null_terminated_cstring_array(ArrayRef<StringRef> strings, StringSaver &saver)
{
   std::vector<const char *> result;
   for (StringRef str : strings) {
      result.push_back(saver.save(str).getData());
   }
   result.push_back(nullptr);
   return result;
}

bool setup_process_cwd(std::optional<StringRef> cwd, std::string &errorMsg)
{
   // if we chdir, wether need detect target directory exists?
   if (cwd.has_value()) {
      const std::string &cwdStr = cwd.value();
      if (!fs::exists(cwdStr)) {
         errorMsg = "chdir error: Can't redirect stderr to stdout";
         return false;
      }
      if (-1 == chdir(cwdStr.c_str())) {
         make_error_msg(&errorMsg, "Can't redirect stderr to stdout", errno);
         return false;
      }
   }
   return true;
}

} // anonymous namespace

bool execute(ProcessInfo &processInfo, StringRef program,
             ArrayRef<StringRef>args, std::optional<StringRef> cwd,
             std::optional<ArrayRef<StringRef>> env,
             ArrayRef<std::optional<StringRef>> redirects,
             ArrayRef<std::optional<int>> redirectsOpenModes,
             unsigned memoryLimit, std::string *errMsg)
{
   if (!fs::exists(program)) {
      if (errMsg) {
         *errMsg = std::string("Executable \"") + program.getStr() +
               std::string("\" doesn't exist!");
      }
      return false;
   }

   BumpPtrAllocator allocator;
   StringSaver saver(allocator);
   std::vector<const char *> argVector, envVector;
   const char **argv = nullptr;
   const char **envp = nullptr;
   argVector = to_null_terminated_cstring_array(args, saver);
   argv = argVector.data();
   if (env) {
      envVector = to_null_terminated_cstring_array(*env, saver);
      envp = envVector.data();
   }

   // If this OS has posix_spawn and there is no memory limit being implied, use
   // posix_spawn.  It is more efficient than fork/exec.
#ifdef HAVE_POSIX_SPAWN
   if (!cwd) {
      if (memoryLimit == 0) {
         posix_spawn_file_actions_t fileActionsStore;
         posix_spawn_file_actions_t *fileActions = nullptr;

         // If we call posix_spawn_file_actions_addopen we have to make sure the
         // c strings we pass to it stay alive until the call to posix_spawn,
         // so we copy any StringRefs into this variable.
         std::string redirectsStorage[3];

         if (!redirects.empty()) {
            assert(redirects.getSize() == 3);
            assert(redirectsOpenModes.getSize() == 3);
            std::string *redirectsStr[3] = {nullptr, nullptr, nullptr};
            for (int index = 0; index < 3; ++index) {
               if (redirects[index]) {
                  redirectsStorage[index] = *redirects[index];
                  redirectsStr[index] = &redirectsStorage[index];
               }
            }
            if (redirectsStr[0] != nullptr) {
               processInfo.m_stdinFilename = *redirectsStr[0];
            }
            if (redirectsStr[1] != nullptr) {
               processInfo.m_stdoutFilename = *redirectsStr[1];
            }
            if (redirectsStr[2] != nullptr) {
               processInfo.m_stderrFilename = *redirectsStr[2];
            }
            fileActions = &fileActionsStore;
            posix_spawn_file_actions_init(fileActions);

            // Redirect stdin/stdout.
            if (redirect_io_ps(redirectsStr[0], 0, redirectsOpenModes[0], errMsg, fileActions) ||
                redirect_io_ps(redirectsStr[1], 1, redirectsOpenModes[1], errMsg, fileActions)) {
               return false;
            }

            if (!redirects[1] || !redirects[2] || *redirects[1] != *redirects[2]) {
               // Just redirect stderr
               if (redirect_io_ps(redirectsStr[2], 2, redirectsOpenModes[2], errMsg, fileActions)) {
                  return false;
               }

            } else {
               // If stdout and stderr should go to the same place, redirect stderr
               // to the FD already open for stdout.
               if (int error = posix_spawn_file_actions_adddup2(fileActions, 1, 2)) {
                  return !make_error_msg(errMsg, "Can't redirect stderr to stdout", error);
               }
            }
         }
         if (!envp) {
#if !USE_NSGETENVIRON
            envp = const_cast<const char **>(environ);
#else
            // environ is missing in dylibs.
            envp = const_cast<const char **>(*_NSGetEnviron());
#endif
         }

         // Explicitly initialized to prevent what appears to be a valgrind false
         // positive.
         pid_t pid = 0;
         int error = posix_spawn(&pid, program.getStr().c_str(), fileActions,
                                 /*attrp*/nullptr, const_cast<char **>(argv),
                                 const_cast<char **>(envp));

         if (fileActions) {
            posix_spawn_file_actions_destroy(fileActions);
         }

         if (error) {
            return !make_error_msg(errMsg, "posix_spawn failed", error);
         }
         processInfo.m_pid = pid;
         return true;
      }
   }

#endif

   // Create a child process.
   int child = fork();
   switch (child) {
   // An error occurred:  Return to the caller.
   case -1:
      make_error_msg(errMsg, "Couldn't fork");
      return false;

      // Child process: Execute the program.
   case 0: {
      // Redirect file descriptors...
      if (!redirects.empty()) {
         // Redirect stdin
         if (redirect_io(redirects[0], 0, redirectsOpenModes[0], errMsg)) { return false; }
         // Redirect stdout
         if (redirect_io(redirects[1], 1, redirectsOpenModes[1], errMsg)) { return false; }
         if (redirects[1] && redirects[2] && *redirects[1] == *redirects[2]) {
            // If stdout and stderr should go to the same place, redirect stderr
            // to the FD already open for stdout.
            if (-1 == dup2(1,2)) {
               make_error_msg(errMsg, "Can't redirect stderr to stdout");
               return false;
            }
         } else {
            // Just redirect stderr
            if (redirect_io(redirects[2], 2, redirectsOpenModes[2], errMsg)) {
               return false;
            }
         }
      }

      // Set memory limits
      if (memoryLimit != 0) {
         set_memory_limits(memoryLimit);
      }
      if (cwd && !setup_process_cwd(cwd, *errMsg)) {
         return false;
      }
      // Execute!
      std::string pathStr = program;
      if (envp != nullptr) {
         execve(pathStr.c_str(),
                const_cast<char **>(argv),
                const_cast<char **>(envp));
      } else {
         execv(pathStr.c_str(),
               const_cast<char **>(argv));
      }

      // If the execve() failed, we should exit. Follow Unix protocol and
      // return 127 if the executable was not found, and 126 otherwise.
      // Use _exit rather than exit so that atexit functions and static
      // object destructors cloned from the parent process aren't
      // redundantly run, and so that any data buffered in stdio buffers
      // cloned from the parent aren't redundantly written out.
      _exit(errno == ENOENT ? 127 : 126);
   }
      // Parent process: Break out of the switch to do our processing.
   default:
      break;
   }
   if (!redirects.empty()) {
      if (redirects[0]) {
         if (redirects[0]->empty()) {
            processInfo.m_stdinFilename = "/dev/null";
         } else {
            processInfo.m_stdinFilename = redirects[0].value();
         }
      }
      if (redirects[1]) {
         if (redirects[1]->empty()) {
            processInfo.m_stdoutFilename = "/dev/null";
         } else {
            processInfo.m_stdoutFilename = redirects[1].value();
         }
      }
      if (redirects[2]) {
         if (redirects[2]->empty()) {
            processInfo.m_stderrFilename = "/dev/null";
         } else {
            processInfo.m_stderrFilename = redirects[2].value();
         }
      }
   }
   processInfo.m_pid = child;
   processInfo.m_process = child;
   return true;
}

ProcessInfo wait(const ProcessInfo &processInfo, unsigned secondsToWait,
                 bool waitUntilTerminates, std::string *errMsg)
{
   struct sigaction act, old;
   assert(processInfo.m_pid && "invalid pid to wait on, process not started?");

   int waitPidOptions = 0;
   pid_t childPid = processInfo.m_pid;
   if (waitUntilTerminates) {
      secondsToWait = 0;
   } else if (secondsToWait) {
      // Install a timeout handler.  The handler itself does nothing, but the
      // simple fact of having a handler at all causes the wait below to return
      // with EINTR, unlike if we used SIG_IGN.
      memset(&act, 0, sizeof(act));
      act.sa_handler = timeout_handler;
      sigemptyset(&act.sa_mask);
      sigaction(SIGALRM, &act, &old);
      alarm(secondsToWait);
   } else if (secondsToWait == 0) {
      waitPidOptions = WNOHANG;
   }

   // Parent process: Wait for the child process to terminate.
   int status;
   ProcessInfo waitResult;

   do {
      waitResult.m_pid = waitpid(childPid, &status, waitPidOptions);
   } while (waitUntilTerminates && waitResult.m_pid == -1 && errno == EINTR);

   if (waitResult.m_pid != processInfo.m_pid) {
      if (waitResult.m_pid == 0) {
         // Non-blocking wait.
         return waitResult;
      } else {
         if (secondsToWait && errno == EINTR) {
            // Kill the child.
            kill(processInfo.m_pid, SIGKILL);

            // Turn off the alarm and restore the signal handler
            alarm(0);
            sigaction(SIGALRM, &old, nullptr);

            // Wait for child to die
            if (::wait(&status) != childPid) {
               make_error_msg(errMsg, "Child timed out but wouldn't die");
            } else {
               make_error_msg(errMsg, "Child timed out", 0);
            }
            waitResult.m_returnCode = -2; // Timeout detected
            return waitResult;
         } else if (errno != EINTR) {
            make_error_msg(errMsg, "Error waiting for child process");
            waitResult.m_returnCode = -1;
            return waitResult;
         }
      }
   }

   // We exited normally without timeout, so turn off the timer.
   if (secondsToWait && !waitUntilTerminates) {
      alarm(0);
      sigaction(SIGALRM, &old, nullptr);
   }

   // Return the proper exit status. Detect error conditions
   // so we can return -1 for them and set ErrMsg informatively.
   int result = 0;
   if (WIFEXITED(status)) {
      result = WEXITSTATUS(status);
      waitResult.m_returnCode = result;

      if (result == 127) {
         if (errMsg) {
            *errMsg = polar::utils::get_str_error(ENOENT);
         }
         waitResult.m_returnCode = -1;
         return waitResult;
      }
      if (result == 126) {
         if (errMsg) {
            *errMsg = "Program could not be executed";
         }
         waitResult.m_returnCode = -1;
         return waitResult;
      }
   } else if (WIFSIGNALED(status)) {
      if (errMsg) {
         *errMsg = strsignal(WTERMSIG(status));
#ifdef WCOREDUMP
         if (WCOREDUMP(status)) {
            *errMsg += " (core dumped)";
         }
#endif
      }
      // Return a special value to indicate that the process received an unhandled
      // signal during execution as opposed to failing to execute.
      waitResult.m_returnCode = -2;
   }
   return waitResult;
}

std::error_code change_stdin_to_binary()
{
   // Do nothing, as Unix doesn't differentiate between text and binary.
   return std::error_code();
}

std::error_code change_stdout_to_binary()
{
   // Do nothing, as Unix doesn't differentiate between text and binary.
   return std::error_code();
}

std::error_code
write_file_with_encoding(StringRef fileName, StringRef contents,
                         WindowsEncodingMethod encoding /*unused*/)
{
   std::error_code errorCode;
   polar::utils::RawFdOutStream outStream(fileName, errorCode, fs::OpenFlags::F_Text);

   if (errorCode) {
      return errorCode;
   }
   outStream << contents;
   if (outStream.hasError()) {
      return make_error_code(ErrorCode::io_error);
   }
   return errorCode;
}

bool commandline_fits_within_system_limits(StringRef program,
                                           ArrayRef<StringRef> args)
{
   static long argMax = sysconf(_SC_ARG_MAX);
   // POSIX requires that _POSIX_ARG_MAX is 4096, which is the lowest possible
   // value for ARG_MAX on a POSIX compliant system.
   static long argMin = _POSIX_ARG_MAX;

   // This the same baseline used by xargs.
   long effectiveArgMax = 128 * 1024;

   if (effectiveArgMax > argMax) {
      effectiveArgMax = argMax;
   } else if (effectiveArgMax < argMin) {
      effectiveArgMax = argMin;
   }

   // System says no practical limit.
   if (argMax == -1) {
      return true;
   }
   // Conservatively account for space required by environment variables.
   long halfArgMax = effectiveArgMax / 2;

   size_t argLength = program.getSize() + 1;
   for (StringRef arg : args) {
      // Ensure that we do not exceed the MAX_ARG_STRLEN constant on Linux, which
      // does not have a constant unlike what the man pages would have you
      // believe. Since this limit is pretty high, perform the check
      // unconditionally rather than trying to be aggressive and limiting it to
      // Linux only.
      if (arg.size() >= (32 * 4096)) {
         return false;
      }
      argLength += arg.size() + 1;
      if (argLength > size_t(halfArgMax)) {
         return false;
      }
   }
   return true;
}

} // sys
} // polar
