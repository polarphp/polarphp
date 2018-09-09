// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/09.

#ifndef POLAR_DEVLTOOLS_LIT_BASIC_TIMER_H
#define POLAR_DEVLTOOLS_LIT_BASIC_TIMER_H

#include <chrono>
#include <thread>
#include <functional>

namespace polar {
namespace lit {

class BasicTimer
{
public:
   using Interval = std::chrono::milliseconds ;
   using Timeout = std::function<void(void)> ;

   BasicTimer(const Timeout &timeout);
   BasicTimer(const Timeout &timeout,
              const Interval &interval,
              bool singleShot = true);

   void start(bool multiThread = false);
   void stop();

   bool running() const;

   void setSingleShot(bool singleShot);
   bool isSingleShot() const;

   void setInterval(const Interval &interval);
   const Interval &interval() const;

   void setTimeout(const Timeout &timeout);
   const Timeout &timeout() const;
private:
   void temporize();
   void sleepThenTimeout();
private:
   std::thread m_thread;
   bool m_running = false;
   bool m_isSingleShot = true;
   Interval m_interval = Interval(0);
   Timeout m_timeout = nullptr;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_BASIC_TIMER_H
