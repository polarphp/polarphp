// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/11/22.

#ifndef POLAR_DEVLTOOLS_LIT_TIMEOUT_HELPER_H
#define POLAR_DEVLTOOLS_LIT_TIMEOUT_HELPER_H

#include "BasicTimer.h"
#include <optional>
#include <list>
#include <mutex>

namespace polar {
namespace lit {

class TimeoutHelper
{
public:
   TimeoutHelper(int timeout);
   void cancel();
   bool active();
   void addProcess(pid_t process);
   void startTimer();
   bool timeoutReached();

private:
   void handleTimeoutReached();
   void kill();
protected:
   int m_timeout;
   std::list<pid_t> m_procs;
   bool m_timeoutReached;
   bool m_doneKillPass;
   std::mutex m_lock;
   std::optional<BasicTimer> m_timer;
};


} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_TIMEOUT_HELPER_H
