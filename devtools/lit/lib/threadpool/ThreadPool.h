// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/10.

#ifndef POLAR_DEVLTOOLS_LIT_THREAD_POOL_THREAD_POOOL_H
#define POLAR_DEVLTOOLS_LIT_THREAD_POOL_THREAD_POOOL_H

#include "FixedFunction.h"
#include "MpmcBoundedQueue.h"
#include "ThreadPoolOptions.h"
#include "ThreadWorker.h"

#include <atomic>
#include <memory>
#include <stdexcept>
#include <vector>

namespace polar {
namespace lit {
namespace threadpool {

template <typename Task, template<typename> class Queue>
class ThreadPoolImpl;
using ThreadPool = ThreadPoolImpl<FixedFunction<void(), 128>,
MPMCBoundedQueue>;

/**
 * @brief The ThreadPool class implements thread pool pattern.
 * It is highly scalable and fast.
 * It is header only.
 * It implements both work-stealing and work-distribution balancing
 * startegies.
 * It implements cooperative scheduling strategy for tasks.
 */
template <typename Task, template<typename> class Queue>
class ThreadPoolImpl
{
public:
   /**
     * @brief ThreadPool Construct and start new thread pool.
     * @param options Creation options.
     */
   explicit ThreadPoolImpl(
         const ThreadPoolOptions &options = ThreadPoolOptions());

   /**
     * @brief Move ctor implementation.
     */
   ThreadPoolImpl(ThreadPoolImpl &&rhs) noexcept;
   /**
    * @brief terminate terminate
    */
   void terminate();
   /**
     * @brief ~ThreadPool Stop all workers and destroy thread pool.
     */
   ~ThreadPoolImpl();

   /**
     * @brief Move assignment implementaion.
     */
   ThreadPoolImpl &operator=(ThreadPoolImpl &&rhs) noexcept;

   /**
     * @brief post Try post job to thread pool.
     * @param handler Handler to be called from thread pool worker. It has
     * to be callable as 'handler()'.
     * @return 'true' on success, false otherwise.
     * @note All exceptions thrown by handler will be suppressed.
     */
   template <typename Handler>
   bool tryPost(Handler &&handler);

   /**
     * @brief post Post job to thread pool.
     * @param handler Handler to be called from thread pool worker. It has
     * to be callable as 'handler()'.
     * @throw std::overflow_error if worker's queue is full.
     * @note All exceptions thrown by handler will be suppressed.
     */
   template <typename Handler>
   void post(Handler &&handler);

private:
   Worker<Task, Queue> &getWorker();

   std::vector<std::unique_ptr<Worker<Task, Queue>>> m_workers;
   std::atomic<size_t> m_nextWorker;
};


/// Implementation

template <typename Task, template<typename> class Queue>
inline ThreadPoolImpl<Task, Queue>::ThreadPoolImpl(
      const ThreadPoolOptions &options)
   : m_workers(options.getThreadCount())
   , m_nextWorker(0)
{
   for(auto & workerPtr : m_workers) {
      workerPtr.reset(new Worker<Task, Queue>(options.getQueueSize()));
   }

   for(size_t i = 0; i < m_workers.size(); ++i) {
      Worker<Task, Queue>* steal_donor =
            m_workers[(i + 1) % m_workers.size()].get();
      m_workers[i]->start(i, steal_donor);
   }
}

template <typename Task, template<typename> class Queue>
inline ThreadPoolImpl<Task, Queue>::ThreadPoolImpl(ThreadPoolImpl<Task, Queue> &&rhs) noexcept
{
   *this = rhs;
}

template <typename Task, template<typename> class Queue>
inline void ThreadPoolImpl<Task, Queue>::terminate()
{
   for (auto &workerPtr : m_workers) {
      workerPtr->stop();
   }
}

template <typename Task, template<typename> class Queue>
inline ThreadPoolImpl<Task, Queue>::~ThreadPoolImpl()
{
   for (auto &workerPtr : m_workers) {
      if (!workerPtr->isStopped()) {
         workerPtr->stop();
      }
   }
}

template <typename Task, template<typename> class Queue>
inline ThreadPoolImpl<Task, Queue>&
ThreadPoolImpl<Task, Queue>::operator=(ThreadPoolImpl<Task, Queue> &&rhs) noexcept
{
   if (this != &rhs) {
      m_workers = std::move(rhs.m_workers);
      m_nextWorker = rhs.m_nextWorker.load();
   }
   return *this;
}

template <typename Task, template<typename> class Queue>
template <typename Handler>
inline bool ThreadPoolImpl<Task, Queue>::tryPost(Handler &&handler)
{
   return getWorker().post(std::forward<Handler>(handler));
}

template <typename Task, template<typename> class Queue>
template <typename Handler>
inline void ThreadPoolImpl<Task, Queue>::post(Handler &&handler)
{
   const auto ok = tryPost(std::forward<Handler>(handler));
   if (!ok) {
      throw std::runtime_error("thread pool queue is full");
   }
}

template <typename Task, template<typename> class Queue>
inline Worker<Task, Queue> &ThreadPoolImpl<Task, Queue>::getWorker()
{
   auto id = Worker<Task, Queue>::getWorkerIdForCurrentThread();

   if (id > m_workers.size()) {
      id = m_nextWorker.fetch_add(1, std::memory_order_relaxed) %
            m_workers.size();
   }
   return *m_workers[id];
}

} // threadpool
} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_THREAD_POOL_THREAD_POOOL_H
