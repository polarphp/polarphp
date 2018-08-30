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

} // lit
} // polar
