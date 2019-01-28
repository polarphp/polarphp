// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/03.

#include "polarphp/utils/Parallel.h"

#include <atomic>
#include <stack>
#include <thread>

namespace polar {
namespace utils {
namespace parallel {
namespace {

/// \brief An abstract class that takes closures and runs them asynchronously.
class Executor
{
public:
   virtual ~Executor() = default;
   virtual void add(std::function<void()> func) = 0;

   static Executor *getDefaultExecutor();
};

#if defined(_MSC_VER)
/// \brief An Executor that runs tasks via ConcRT.
class ConcRTExecutor : public Executor
{
   struct Taskish
   {
      Taskish(std::function<void()> task) : m_task(task) {}

      std::function<void()> m_task;

      static void run(void *runnable)
      {
         Taskish *self = static_cast<Taskish *>(runnable);
         self->m_task();
         concurrency::Free(self);
      }
   };

public:
   virtual void add(std::function<void()> func)
   {
      Concurrency::CurrentScheduler::ScheduleTask(
               Taskish::run, new (concurrency::Alloc(sizeof(Taskish))) Taskish(func));
   }
};

Executor *Executor::getDefaultExecutor()
{
   static ConcRTExecutor exec;
   return &exec;
}

#else
/// \brief An implementation of an Executor that runs closures on a thread pool
///   in filo order.
class ThreadPoolExecutor : public Executor
{
public:
   explicit ThreadPoolExecutor(unsigned threadCount = std::thread::hardware_concurrency())
      : m_done(threadCount) {
      // Spawn all but one of the threads in another thread as spawning threads
      // can take a while.
      std::thread([&, threadCount] {
         for (size_t i = 1; i < threadCount; ++i) {
            std::thread([=] { work(); }).detach();
         }
         work();
      }).detach();
   }

   ~ThreadPoolExecutor() override
   {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_stop = true;
      lock.unlock();
      m_cond.notify_all();
      // Wait for ~Latch.
   }

   void add(std::function<void()> func) override
   {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_workStack.push(func);
      lock.unlock();
      m_cond.notify_one();
   }

private:
   void work()
   {
      while (true) {
         std::unique_lock<std::mutex> lock(m_mutex);
         m_cond.wait(lock, [&] { return m_stop || !m_workStack.empty(); });
         if (m_stop) {
            break;
         }
         auto m_task = m_workStack.top();
         m_workStack.pop();
         lock.unlock();
         m_task();
      }
      m_done.dec();
   }

   std::atomic<bool> m_stop{false};
   std::stack<std::function<void()>> m_workStack;
   std::mutex m_mutex;
   std::condition_variable m_cond;
   parallel::internal::Latch m_done;
};

Executor *Executor::getDefaultExecutor()
{
   static ThreadPoolExecutor exec;
   return &exec;
}
#endif
}

void parallel::internal::TaskGroup::spawn(std::function<void()> func)
{
   m_latch.inc();
   Executor::getDefaultExecutor()->add([&, func] {
      func();
      m_latch.dec();
   });
}

} // parallel
} // utils
} // polar
