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

#include "BasicTimer.h"

namespace polar {
namespace lit {

BasicTimer::BasicTimer(const Timeout &timeout)
   : m_timeout(timeout)
{
}

BasicTimer::BasicTimer(const BasicTimer::Timeout &timeout,
                       const BasicTimer::Interval &interval,
                       bool singleShot)
   : m_isSingleShot(singleShot),
     m_interval(interval),
     m_timeout(timeout)
{
}

void BasicTimer::start(bool multiThread)
{
   if (this->running() == true) {
      return;
   }
   m_running = true;
   if (multiThread == true) {
      m_thread = std::thread(
               &BasicTimer::temporize, this);
   }
   else{
      this->temporize();
   }
}

void BasicTimer::stop()
{
   m_running = false;
   m_thread.join();
}

bool BasicTimer::running() const
{
   return m_running;
}

void BasicTimer::setSingleShot(bool singleShot)
{
   if (this->running() == true) {
      return;
   }
   m_isSingleShot = singleShot;
}

bool BasicTimer::isSingleShot() const
{
   return m_isSingleShot;
}

void BasicTimer::setInterval(const BasicTimer::Interval &interval)
{
   if (this->running() == true) {
       return;
   }
   m_interval = interval;
}

const BasicTimer::Interval &BasicTimer::interval() const
{
   return m_interval;
}

void BasicTimer::setTimeout(const Timeout &timeout)
{
   if (this->running() == true)
      return;

   m_timeout = timeout;
}

const BasicTimer::Timeout &BasicTimer::timeout() const
{
   return m_timeout;
}

void BasicTimer::temporize()
{
   if (m_isSingleShot == true) {
      this->sleepThenTimeout();
   }
   else {
      while (this->running() == true) {
         this->sleepThenTimeout();
      }
   }
}

void BasicTimer::sleepThenTimeout()
{
   std::this_thread::sleep_for(m_interval);

   if (this->running() == true) {
      this->timeout()();
   }
}

} // lit
} // polar
