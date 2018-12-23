// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/23.

#include "polarphp/utils/ThreadPool.h"
#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace utils {


// Default to hardware_concurrency
ThreadPool::ThreadPool()
   : ThreadPool(std::thread::hardware_concurrency())
{}

ThreadPool::ThreadPool(unsigned threadCount)
   : m_activeThreads(0),
     m_enableFlag(true)
{
   // Create threadCount threads that will loop forever, wait on m_queueCondition
   // for tasks to be queued or the Pool to be destroyed.
   m_threads.reserve(threadCount);
   for (unsigned threadID = 0; threadID < threadCount; ++threadID) {
      m_threads.emplace_back([&] {
         while (true) {
            PackagedTaskTy task;
            {
               std::unique_lock<std::mutex> lockGuard(m_queueLock);
               // Wait for tasks to be pushed in the queue
               m_queueCondition.wait(lockGuard,
                                   [&] { return !m_enableFlag || !m_tasks.empty(); });
               // Exit condition
               if (!m_enableFlag && m_tasks.empty())
                  return;
               // Yeah, we have a task, grab it and release the lock on the queue

               // We first need to signal that we are active before popping the queue
               // in order for wait() to properly detect that even if the queue is
               // empty, there is still a task in flight.
               {
                  std::unique_lock<std::mutex> lockGuard(m_completionLock);
                  ++m_activeThreads;
               }
               task = std::move(m_tasks.front());
               m_tasks.pop();
            }
            // Run the task we just grabbed
            task();

            {
               // Adjust `m_activeThreads`, in case someone waits on ThreadPool::wait()
               std::unique_lock<std::mutex> lockGuard(m_completionLock);
               --m_activeThreads;
            }

            // Notify task completion, in case someone waits on ThreadPool::wait()
            m_completionCondition.notify_all();
         }
      });
   }
}

void ThreadPool::wait()
{
   // Wait for all threads to complete and the queue to be empty
   std::unique_lock<std::mutex> lockGuard(m_completionLock);
   // The order of the checks for m_activeThreads and m_tasks.empty() matters because
   // any active threads might be modifying the m_tasks queue, and this would be a
   // race.
   m_completionCondition.wait(lockGuard,
                            [&] { return !m_activeThreads && m_tasks.empty(); });
}

std::shared_future<void> ThreadPool::asyncImpl(TaskTy task)
{
   /// Wrap the task in a packaged_task to return a future object.
   PackagedTaskTy packagedTask(std::move(task));
   auto Future = packagedTask.get_future();
   {
      // Lock the queue and push the new task
      std::unique_lock<std::mutex> lockGuard(m_queueLock);

      // Don't allow enqueueing after disabling the pool
      assert(m_enableFlag && "Queuing a thread during ThreadPool destruction");

      m_tasks.push(std::move(packagedTask));
   }
   m_queueCondition.notify_one();
   return Future.share();
}

// The destructor joins all threads, waiting for completion.
ThreadPool::~ThreadPool()
{
   {
      std::unique_lock<std::mutex> lockGuard(m_queueLock);
      m_enableFlag = false;
   }
   m_queueCondition.notify_all();
   for (auto &worker : m_threads) {
      worker.join();
   }
}

} // utils
} // polar
