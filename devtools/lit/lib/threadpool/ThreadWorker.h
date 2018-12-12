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

#ifndef POLAR_DEVLTOOLS_LIT_THREAD_POOL_THREAD_WORKER_H
#define POLAR_DEVLTOOLS_LIT_THREAD_POOL_THREAD_WORKER_H

#include <atomic>
#include <thread>

namespace polar {
namespace lit {
namespace threadpool {

/**
 * @brief The Worker class owns task queue and executing thread.
 * In thread it tries to pop task from queue. If queue is empty then it tries
 * to steal task from the sibling worker. If steal was unsuccessful then spins
 * with one millisecond delay.
 */
template <typename Task, template<typename> class Queue>
class Worker
{
public:
   /**
     * @brief Worker Constructor.
     * @param queueSize Length of undelaying task queue.
     */
   explicit Worker(size_t queueSize);

   /**
     * @brief Move ctor implementation.
     */
   Worker(Worker&& rhs) noexcept;

   /**
     * @brief Move assignment implementaion.
     */
   Worker& operator=(Worker&& rhs) noexcept;

   /**
     * @brief start Create the executing thread and start tasks execution.
     * @param id Worker ID.
     * @param stealDonor Sibling worker to steal task from it.
     */
   void start(size_t id, Worker* stealDonor);

   /**
     * @brief check wether worker is stopped
     */
   bool isStopped();

   /**
     * @brief stop Stop all worker's thread and stealing activity.
     * Waits until the executing thread became finished.
     */
   void stop();

   /**
     * @brief post Post task to queue.
     * @param handler Handler to be executed in executing thread.
     * @return true on success.
     */
   template <typename Handler>
   bool post(Handler&& handler);

   /**
     * @brief steal Steal one task from this worker queue.
     * @param task Place for stealed task to be stored.
     * @return true on success.
     */
   bool steal(Task& task);

   /**
     * @brief getWorkerIdForCurrentThread Return worker ID associated with
     * current thread if exists.
     * @return Worker ID.
     */
   static size_t getWorkerIdForCurrentThread();

private:
   /**
     * @brief threadFunc Executing thread function.
     * @param id Worker ID to be associated with this thread.
     * @param stealDonor Sibling worker to steal task from it.
     */
   void threadFunc(size_t id, Worker* stealDonor);

   Queue<Task> m_queue;
   std::atomic<bool> m_runningFlag;
   std::thread m_thread;
};


/// Implementation

namespace internal
{
inline size_t* retrieve_thread_id()
{
   static thread_local size_t tss_id = -1u;
   return &tss_id;
}
}

template <typename Task, template<typename> class Queue>
inline Worker<Task, Queue>::Worker(size_t queueSize)
   : m_queue(queueSize)
   , m_runningFlag(true)
{
}

template <typename Task, template<typename> class Queue>
inline Worker<Task, Queue>::Worker(Worker&& rhs) noexcept
{
   *this = rhs;
}

template <typename Task, template<typename> class Queue>
inline Worker<Task, Queue>& Worker<Task, Queue>::operator=(Worker&& rhs) noexcept
{
   if (this != &rhs) {
      m_queue = std::move(rhs.m_queue);
      m_runningFlag = rhs.m_runningFlag.load();
      m_thread = std::move(rhs.m_thread);
   }
   return *this;
}

template <typename Task, template<typename> class Queue>
inline void Worker<Task, Queue>::stop()
{
   m_runningFlag.store(false, std::memory_order_relaxed);
   m_thread.join();
}

template <typename Task, template<typename> class Queue>
inline bool Worker<Task, Queue>::isStopped()
{
   return !m_runningFlag.load(std::memory_order_relaxed);
}

template <typename Task, template<typename> class Queue>
inline void Worker<Task, Queue>::start(size_t id, Worker* stealDonor)
{
   m_thread = std::thread(&Worker<Task, Queue>::threadFunc, this, id, stealDonor);
}

template <typename Task, template<typename> class Queue>
inline size_t Worker<Task, Queue>::getWorkerIdForCurrentThread()
{
   return *internal::retrieve_thread_id();
}

template <typename Task, template<typename> class Queue>
template <typename Handler>
inline bool Worker<Task, Queue>::post(Handler&& handler)
{
   return m_queue.push(std::forward<Handler>(handler));
}

template <typename Task, template<typename> class Queue>
inline bool Worker<Task, Queue>::steal(Task& task)
{
   return m_queue.pop(task);
}

template <typename Task, template<typename> class Queue>
inline void Worker<Task, Queue>::threadFunc(size_t id, Worker* stealDonor)
{
   *internal::retrieve_thread_id() = id;
   Task handler;
   while (m_runningFlag.load(std::memory_order_relaxed)) {
      if (m_queue.pop(handler) || stealDonor->steal(handler)) {
         try {
            handler();
         } catch(...) {
            // suppress all exceptions
         }
      } else {
         std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
   }
}

} // threadpool
} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_THREAD_POOL_THREAD_WORKER_H
