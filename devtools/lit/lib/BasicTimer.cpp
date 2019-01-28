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

#include "BasicTimer.h"
#include <cassert>
#include <condition_variable>

namespace polar {
namespace lit {

BasicTimer::BasicTimer()
   : m_isSingleShot(true),
     m_running(false),
     m_startupMultithread(true),
     m_interval(0),
     m_timeoutHandler(nullptr)
{}

BasicTimer::BasicTimer(const TimeoutFunc &handler)
   : m_isSingleShot(true),
     m_running(false),
     m_startupMultithread(true),
     m_interval(0),
     m_timeoutHandler(handler)
{}

BasicTimer::BasicTimer(const BasicTimer::TimeoutFunc &handler,
                       const BasicTimer::Interval &interval,
                       bool singleShot)
   : m_isSingleShot(singleShot),
     m_running(false),
     m_startupMultithread(true),
     m_interval(interval),
     m_timeoutHandler(handler)
{
}

BasicTimer::~BasicTimer()
{
   m_interupted.notify_one();
   if (m_startupMultithread && m_running) {
      m_thread.join();
   }
}

void BasicTimer::start(bool multiThread)
{
   assert(m_timeoutHandler != nullptr && "the timeout handler of timer can not be null.");
   if (this->running() == true) {
      return;
   }
   m_running = true;
   m_startupMultithread = multiThread;
   if (multiThread == true) {
      m_thread = std::thread(
               &BasicTimer::getTemporize, this);
   } else{
      this->getTemporize();
   }
}

void BasicTimer::stop()
{
   m_interupted.notify_one();
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

const BasicTimer::Interval &BasicTimer::getInterval() const
{
   return m_interval;
}

void BasicTimer::setTimeoutHandler(const TimeoutFunc &handler)
{
   if (this->running() == true) {
      return;
   }
   m_timeoutHandler = handler;
}

const BasicTimer::TimeoutFunc &BasicTimer::getTimeoutHandler() const
{
   return m_timeoutHandler;
}

void BasicTimer::getTemporize()
{
   if (m_isSingleShot == true) {
      this->sleepThenTimeout();
   }
   else {
      while (this->running()) {
         this->sleepThenTimeout();
      }
   }
}

void BasicTimer::sleepThenTimeout()
{
   std::unique_lock<std::mutex> locker(m_mutex);
   std::cv_status status = m_interupted.wait_for(locker, m_interval);
   if (status == std::cv_status::timeout) {
      // timeout we need invoke handler
      this->getTimeoutHandler()();
   }
}

} // lit
} // polar
