// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/08.

//===----------------------------------------------------------------------===//
//=== WARNING: Implementation here must contain only TRULY operating system
//===          independent code.
//===----------------------------------------------------------------------===//

#include "polarphp/utils/Program.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/global/Config.h"
#include <system_error>

namespace polar {
namespace sys {

using polar::basic::SmallVector;
using polar::basic::ArrayRef;

bool execute(ProcessInfo &processInfo, StringRef program, ArrayRef<StringRef> args,
             std::optional<StringRef> cwd, std::optional<ArrayRef<StringRef>> envp,
             ArrayRef<std::optional<StringRef>> redirects,
             ArrayRef<std::optional<int>> redirectsOpenModes,
             unsigned memoryLimit, std::string *errMsg);

int execute_and_wait(StringRef program, ArrayRef<StringRef> args,
                     std::optional<StringRef> cwd,
                     std::optional<ArrayRef<StringRef>> envp,
                     ArrayRef<std::optional<StringRef>> redirects,
                     unsigned secondsToWait, unsigned memoryLimit,
                     std::string *errMsg, bool *executionFailed)
{
   ArrayRef<std::optional<int>> openModes{
      std::nullopt,
            std::nullopt,
            std::nullopt
   };
   return execute_and_wait(program, args, cwd, envp, redirects, openModes, secondsToWait,
                           memoryLimit, errMsg, executionFailed);
}

int execute_and_wait(StringRef program, ArrayRef<StringRef> args,
                     std::optional<StringRef> cwd,
                     std::optional<ArrayRef<StringRef>> envp,
                     ArrayRef<std::optional<StringRef>> redirects,
                     ArrayRef<std::optional<int>> redirectsOpenModes,
                     unsigned secondsToWait, unsigned memoryLimit,
                     std::string *errMsg, bool *executionFailed)
{
   assert(redirects.empty() || redirects.getSize() == 3);
   ProcessInfo processInfo;
   if (execute(processInfo, program, args, cwd, envp, redirects, redirectsOpenModes,
               memoryLimit, errMsg)) {
      if (executionFailed) {
         *executionFailed = false;
      }
      ProcessInfo result = wait(
               processInfo, secondsToWait, /*WaitUntilTerminates=*/secondsToWait == 0, errMsg);
      return result.m_returnCode;
   }

   if (executionFailed) {
      *executionFailed = true;
   }
   return -1;
}

ProcessInfo execute_no_wait(StringRef program, ArrayRef<StringRef> args,
                            std::optional<StringRef> cwd,
                            std::optional<ArrayRef<StringRef>> envp,
                            ArrayRef<std::optional<StringRef>> redirects,
                            unsigned memoryLimit, std::string *errMsg,
                            bool *executionFailed)
{
   ArrayRef<std::optional<int>> openModes{
      std::nullopt,
            std::nullopt,
            std::nullopt
   };
   return execute_no_wait(program, args, cwd, envp, redirects, openModes,
                          memoryLimit, errMsg, executionFailed);
}

ProcessInfo execute_no_wait(StringRef program, ArrayRef<StringRef> args,
                            std::optional<StringRef> cwd,
                            std::optional<ArrayRef<StringRef>> envp,
                            ArrayRef<std::optional<StringRef>> redirects,
                            ArrayRef<std::optional<int>> redirectsOpenModes,
                            unsigned memoryLimit, std::string *errMsg,
                            bool *executionFailed)
{
   assert(redirects.empty() || redirects.getSize() == 3);
   ProcessInfo processInfo;
   if (executionFailed) {
      *executionFailed = false;
   }
   if (!execute(processInfo, program, args, cwd, envp, redirects, redirectsOpenModes,
                memoryLimit, errMsg)) {
      if (executionFailed) {
         *executionFailed = true;
      }
   }
   return processInfo;
}

bool commandline_fits_within_system_limits(StringRef program,
                                           ArrayRef<const char *> args)
{
   SmallVector<StringRef, 8> stringRefArgs;
   stringRefArgs.reserve(args.getSize());
   for (const char *arg : args) {
      stringRefArgs.emplace_back(arg);
   }
   return commandline_fits_within_system_limits(program, stringRefArgs);
}

} // sys
} // polar
