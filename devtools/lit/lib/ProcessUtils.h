// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/29.

#ifndef POLAR_DEVLTOOLS_LIT_PROCESS_UTILS_H
#define POLAR_DEVLTOOLS_LIT_PROCESS_UTILS_H

#include <string>
#include <optional>
#include <tuple>
#include <list>
#include <map>
#include "LitGlobal.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/utils/Program.h"

namespace polar {
namespace lit {

using polar::sys::ProcessInfo;
using polar::basic::StringRef;
using polar::basic::ArrayRef;

std::tuple<std::list<pid_t>, bool> retrieve_children_pids(pid_t pid, bool recursive = false) noexcept;
std::tuple<std::list<pid_t>, bool> call_pgrep_command(pid_t pid) noexcept;

/// This function executes the program using the arguments provided.  The
/// invoked program will inherit the stdin, stdout, and stderr file
/// descriptors, the environment and other configuration settings of the
/// invoking program.
/// This function waits for the program to finish, so should be avoided in
/// library functions that aren't expected to block. Consider using
/// ExecuteNoWait() instead.
/// \returns an integer result code indicating the status of the program.
/// A zero or positive value indicates the result code of the program.
/// -1 indicates failure to execute
/// -2 indicates a crash during execution or timeout
int execute_and_wait(
      StringRef program, ///< Path of the program to be executed. It is
      ///< presumed this is the result of the findProgramByName method.
      ArrayRef<StringRef> args, ///< A vector of strings that are passed to the
      ///< program.  The first element should be the name of the program.
      ///< The list *must* be terminated by a null char* entry.
      std::optional<StringRef> cwd,
      std::optional<ArrayRef<StringRef>> env = std::nullopt, ///< An optional vector of strings to use for
      ///< the program's environment. If not provided, the current program's
      ///< environment will be used.
      ArrayRef<std::optional<StringRef>> redirects = {}, ///<
      ///< An array of optional paths. Should have a size of zero or three.
      ///< If the array is empty, no redirections are performed.
      ///< Otherwise, the inferior process's stdin(0), stdout(1), and stderr(2)
      ///< will be redirected to the corresponding paths, if the optional path
      ///< is present (not \c polar::basic::None).
      ///< When an empty path is passed in, the corresponding file descriptor
      ///< will be disconnected (ie, /dev/null'd) in a portable way.
      unsigned secondsToWait = 0, ///< If non-zero, this specifies the amount
      ///< of time to wait for the child process to exit. If the time
      ///< expires, the child is killed and this call returns. If zero,
      ///< this function will wait until the child finishes or forever if
      ///< it doesn't.
      unsigned memoryLimit = 0, ///< If non-zero, this specifies max. amount
      ///< of memory can be allocated by process. If memory usage will be
      ///< higher limit, the child is killed and this call returns. If zero
      ///< - no memory limit.
      std::string *errMsg = nullptr, ///< If non-zero, provides a pointer to a
      ///< string instance in which error messages will be returned. If the
      ///< string is non-empty upon return an error occurred while invoking the
      ///< program.
      bool *executionFailed = nullptr);

int execute_and_wait(
      StringRef program, ///< Path of the program to be executed. It is
      ArrayRef<StringRef> args, ///< A vector of strings that are passed to the
      std::optional<StringRef> cwd,
      std::optional<ArrayRef<StringRef>> env, ///< An optional vector of strings to use for
      ArrayRef<std::optional<StringRef>> redirects, ///<
      ArrayRef<std::optional<int>> redirectsOpenModes,
      unsigned secondsToWait = 0, ///< If non-zero, this specifies the amount
      unsigned memoryLimit = 0, ///< If non-zero, this specifies max. amount
      std::string *errMsg = nullptr, ///< If non-zero, provides a pointer to a
      bool *executionFailed = nullptr);

RunCmdResponse execute_and_wait(
      StringRef program,
      ArrayRef<StringRef> args,
      std::optional<StringRef> cwd = std::nullopt,
      std::optional<ArrayRef<StringRef>> env = std::nullopt,
      unsigned secondsToWait = 0,
      unsigned memoryLimit = 0,
      std::string *errMsg = nullptr,
      bool *executionFailed = nullptr);

ProcessInfo wait_with_timer(
      const ProcessInfo &processInfo, ///< The child process that should be waited on.
      unsigned secondsToWait, ///< If non-zero, this specifies the amount of
      ///< time to wait for the child process to exit. If the time expires, the
      ///< child is killed and this function returns. If zero, this function
      ///< will perform a non-blocking wait on the child process.
      bool waitUntilTerminates, ///< If true, ignores \p SecondsToWait and waits
      ///< until child has terminated.
      std::string *errMsg = nullptr ///< If non-zero, provides a pointer to a
      ///< string instance in which error messages will be returned. If the
      ///< string is non-empty upon return an error occurred while invoking the
      ///< program.
      );

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_PROCESS_UTILS_H
