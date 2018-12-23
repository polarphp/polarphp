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

#ifndef POLARPHP_UTILS_THREAD_POOL_H
#define POLARPHP_UTILS_THREAD_POOL_H

#include "polarphp/global/Config.h"

#include <future>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>

namespace polar {
namespace utils {

/// A ThreadPool for asynchronous parallel execution on a defined number of
/// threads.
///
/// The pool keeps a vector of threads alive, waiting on a condition variable
/// for some work to become available.
class ThreadPool
{
public:
   using TaskTy = std::function<void()>;
   using PackagedTaskTy = std::packaged_task<void()>;

   /// Construct a pool with the number of threads found by
   /// hardware_concurrency().
   ThreadPool();

   /// Construct a pool of \p ThreadCount threads
   ThreadPool(unsigned threadCount);

   /// Blocking destructor: the pool will wait for all the threads to complete.
   ~ThreadPool();

   /// Asynchronous submission of a task to the pool. The returned future can be
   /// used to wait for the task to finish and is *non-blocking* on destruction.
   template <typename Function, typename... Args>
   inline std::shared_future<void> async(Function &&func, Args &&... argList)
   {
      auto task =
            std::bind(std::forward<Function>(func), std::forward<Args>(argList)...);
      return asyncImpl(std::move(task));
   }

   /// Asynchronous submission of a task to the pool. The returned future can be
   /// used to wait for the task to finish and is *non-blocking* on destruction.
   template <typename Function>
   inline std::shared_future<void> async(Function &&func)
   {
      return asyncImpl(std::forward<Function>(func));
   }

   /// Blocking wait for all the threads to complete and the queue to be empty.
   /// It is an error to try to add new tasks while blocking on this call.
   void wait();

private:
   /// Asynchronous submission of a task to the pool. The returned future can be
   /// used to wait for the task to finish and is *non-blocking* on destruction.
   std::shared_future<void> asyncImpl(TaskTy func);

   /// Threads in flight
   std::vector<std::thread> m_threads;

   /// Tasks waiting for execution in the pool.
   std::queue<PackagedTaskTy> m_tasks;

   /// Locking and signaling for accessing the Tasks queue.
   std::mutex m_queueLock;
   std::condition_variable m_queueCondition;

   /// Locking and signaling for job completion
   std::mutex m_completionLock;
   std::condition_variable m_completionCondition;

   /// Keep track of the number of thread actually busy
   std::atomic<unsigned> m_activeThreads;
   /// Signal for the destruction of the pool, asking thread to exit.
   bool m_enableFlag;
};

} // utils
} // polar

#endif // POLARPHP_UTILS_THREAD_POOL_H
