// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/09.

#include "TimeoutHelper.h"
#include "ProcessUtils.h"
#include "Utils.h"

namespace polar {
namespace lit {

TimeoutHelper::TimeoutHelper(int timeout)
   : m_timeout(timeout),
     m_timeoutReached(false),
     m_doneKillPass(false),
     m_timer(std::nullopt)
{
}

void TimeoutHelper::cancel()
{
   if (m_timer) {
      if (active()) {
         m_timer.value().stop();
      }
   }
}

bool TimeoutHelper::active()
{
   return m_timeout > 0;
}

void TimeoutHelper::addProcess(pid_t process)
{
   if (!active()) {
      return;
   }
   bool needToRunKill = false;
   {
      std::lock_guard lock(m_lock);
      m_procs.push_back(process);
      // Avoid re-entering the lock by finding out if kill needs to be run
      // again here but call it if necessary once we have left the lock.
      // We could use a reentrant lock here instead but this code seems
      // clearer to me.
      needToRunKill = m_doneKillPass;
   }
   // The initial call to _kill() from the timer thread already happened so
   // we need to call it again from this thread, otherwise this process
   // will be left to run even though the timeout was already hit
   if (needToRunKill) {
      assert(timeoutReached());
      kill();
   }
}

void TimeoutHelper::startTimer()
{
   if (!active()) {
      return;
   }
   // Do some late initialisation that's only needed
   // if there is a timeout set
   m_timer.emplace([](){
      //handleTimeoutReached();
   }, std::chrono::milliseconds(m_timeout), true);
   BasicTimer &timer = m_timer.value();
   timer.start();
}

void TimeoutHelper::handleTimeoutReached()
{
   m_timeoutReached = true;
   kill();
}

bool TimeoutHelper::timeoutReached()
{
   return m_timeoutReached;
}

void TimeoutHelper::kill()
{
   std::lock_guard locker(m_lock);
   for (pid_t pid : m_procs) {
      kill_process_and_children(pid);
   }
   m_procs.clear();
   m_doneKillPass = true;
}

} //lit
} // polar
