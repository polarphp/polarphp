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

#ifndef POLAR_DEVLTOOLS_LIT_BASIC_TIMER_H
#define POLAR_DEVLTOOLS_LIT_BASIC_TIMER_H

#include <chrono>
#include <thread>
#include <functional>
#include <condition_variable>
#include <mutex>

namespace polar {
namespace lit {

class BasicTimer
{
public:
   using Interval = std::chrono::milliseconds;
   using TimeoutFunc = std::function<void(void)> ;

   BasicTimer();
   BasicTimer(const TimeoutFunc &timeoutHandler);
   BasicTimer(const TimeoutFunc &timeoutHandler,
              const Interval &interval,
              bool singleShot = true);
   ~BasicTimer();

   void start(bool multiThread = false);
   void stop();

   bool running() const;

   void setSingleShot(bool singleShot);
   bool isSingleShot() const;

   void setInterval(const Interval &interval);
   const Interval &getInterval() const;

   void setTimeoutHandler(const TimeoutFunc &handler);
   const TimeoutFunc &getTimeoutHandler() const;
private:
   void getTemporize();
   void sleepThenTimeout();
private:
   bool m_isSingleShot;
   bool m_running;
   bool m_startupMultithread;
   Interval m_interval;
   std::condition_variable m_interupted;
   std::mutex m_mutex;
   TimeoutFunc m_timeoutHandler;
   std::thread m_thread;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_BASIC_TIMER_H
