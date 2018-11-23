// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/29.

#include "ProcessUtils.h"
#include "Utils.h"
#include <stack>

namespace polar {
namespace lit {

std::tuple<std::list<pid_t>, bool> retrieve_children_pids(pid_t pid, bool recursive) noexcept
{
   if (!recursive) {
      return call_pgrep_command(pid);
   }
   std::list<pid_t> resultList;
   std::stack<pid_t> workList;
   std::tuple<std::list<pid_t>, bool> selfPids = call_pgrep_command(pid);
   if (std::get<1>(selfPids)) {
      // init pids
      for (pid_t curpid : std::get<0>(selfPids)) {
         workList.push(curpid);
      }
      while (!workList.empty()) {
         pid_t topPid = workList.top();
         resultList.push_back(topPid);
         workList.pop();
         std::tuple<std::list<pid_t>, bool> selfPids = call_pgrep_command(topPid);
         if (std::get<1>(selfPids)) {
            for (pid_t curpid : std::get<0>(selfPids)) {
               workList.push(curpid);
            }
         }
      }
   } else {
      return selfPids;
   }
   return std::make_tuple(resultList, true);
}

std::tuple<std::list<pid_t>, bool> call_pgrep_command(pid_t pid) noexcept
{
   RunCmdResponse result = run_program("pgrep", std::nullopt, std::nullopt, std::nullopt,
                                       "-P " + std::to_string(pid));
   if (!std::get<0>(result)) {
      return std::make_tuple(std::list<pid_t>{}, false);
   }
   std::list<int32_t> pids;
   for (const std::string &item : split_string(std::get<1>(result), '\n')) {
      pids.push_back(std::stoi(item));
   }
   return std::make_tuple(pids, true);
}

int execute_and_wait(
      StringRef program,
      ArrayRef<StringRef> args,
      std::optional<StringRef> cwd,
      std::optional<ArrayRef<StringRef>> env,
      ArrayRef<std::optional<StringRef>> redirects,
      unsigned secondsToWait,
      unsigned memoryLimit,
      std::string *errMsg,
      bool *executionFailed)
{
   ArrayRef<std::optional<int>> openModes{
      std::nullopt,
            std::nullopt,
            std::nullopt
   };
   return execute_and_wait(program, args, cwd, env, redirects, openModes, secondsToWait,
                           memoryLimit, errMsg, executionFailed);
}

} // lit
} // polar
