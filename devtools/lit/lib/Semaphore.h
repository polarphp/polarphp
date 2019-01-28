// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/02.

#ifndef POLAR_DEVLTOOLS_SEMAPHORE_H
#define POLAR_DEVLTOOLS_SEMAPHORE_H

#include <mutex>
#include <condition_variable>

namespace polar {
namespace lit {

class Semaphore
{
private:
   std::mutex m_mutex;
   std::condition_variable m_condition;
   unsigned long m_count = 0; // Initialized as locked.

public:
   Semaphore(unsigned long value)
      : m_count(value)
   {}
   Semaphore(const Semaphore &other) = delete;
   Semaphore &operator=(const Semaphore &other) = delete;
   void notify()
   {
      std::lock_guard<decltype(m_mutex)> lock(m_mutex);
      ++m_count;
      m_condition.notify_one();
   }

   void wait()
   {
      std::unique_lock<decltype(m_mutex)> lock(m_mutex);
      // Handle spurious wake-ups.
      while(!m_count) {
         m_condition.wait(lock);
      }
      --m_count;
   }

   bool try_wait()
   {
      std::lock_guard<decltype(m_mutex)> lock(m_mutex);
      if(m_count) {
         --m_count;
         return true;
      }
      return false;
   }
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_SEMAPHORE_H
