// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/30.

#include "../Utils.h"
#include "../ProcessUtils.h"
#include <sys/types.h>
#include <signal.h>

namespace polar {
namespace lit {

void kill_process_and_children(pid_t pid) noexcept
{
   std::tuple<std::list<pid_t>, bool> result = retrieve_children_pids(pid, true);
   if (std::get<1>(result)) {
      for (pid_t cpid : std::get<0>(result)) {
         kill(cpid, SIGKILL);
      }
   }
}

} // lit
} // polar
