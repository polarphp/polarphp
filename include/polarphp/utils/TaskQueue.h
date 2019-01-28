// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/24.

#ifndef POLARPHP_UTILS_TASK_QUEUE_H
#define POLARPHP_UTILS_TASK_QUEUE_H

#include "polarphp/global/Config.h"
#include "polarphp/utils/ThreadPool.h"

#include <thread>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <utility>

namespace polar {
namespace utils {

/// TaskQueue executes serialized work on a user-defined Thread Pool.  It
/// guarantees that if task B is enqueued after task A, task B begins after
/// task A completes and there is no overlap between the two.
class TaskQueue
{
   // Because we don't have init capture to use move-only local variables that
   // are captured into a lambda, we create the promise inside an explicit
   // callable struct. We want to do as much of the wrapping in the
   // type-specialized domain (before type erasure) and then erase this into a
   // std::function.
   template <typename Callable>
   struct Task
   {
      using ResultTy = typename std::result_of<Callable()>::type;
      explicit Task(Callable callable, TaskQueue &parent)
         : m_callable(std::move(callable)),
           m_promise(std::make_shared<std::promise<ResultTy>>()),
           m_parent(&parent)
      {}

      template<typename T>
      void invokeCallbackAndSetPromise(T*)
      {
         m_promise->set_value(m_callable());
      }

      void invokeCallbackAndSetPromise(void*)
      {
         m_callable();
         m_promise->set_value();
      }

      void operator()() noexcept
      {
         ResultTy *dummy = nullptr;
         invokeCallbackAndSetPromise(dummy);
         m_parent->completeTask();
      }

      Callable m_callable;
      std::shared_ptr<std::promise<ResultTy>> m_promise;
      TaskQueue *m_parent;
   };

public:
   /// Construct a task queue with no work.
   TaskQueue(ThreadPool &scheduler)
      : m_scheduler(scheduler)
   {
      (void)m_scheduler;
   }

   /// Blocking destructor: the queue will wait for all work to complete.
   ~TaskQueue()
   {
      m_scheduler.wait();
      assert(m_tasks.empty());
   }

   /// Asynchronous submission of a task to the queue. The returned future can be
   /// used to wait for the task (and all previous tasks that have not yet
   /// completed) to finish.
   template <typename Callable>
   std::future<typename std::result_of<Callable()>::type> async(Callable &&callable)
   {
      Task<Callable> task{std::move(callable), *this};
      using ResultTy = typename std::result_of<Callable()>::type;
      std::future<ResultTy> future = task.m_promise->get_future();
      {
         std::lock_guard<std::mutex> locker(m_queueLock);
         // If there's already a task in flight, just queue this one up.  If
         // there is not a task in flight, bypass the queue and schedule this
         // task immediately.
         if (m_isTaskInFlight)
            m_tasks.push_back(std::move(task));
         else {
            m_scheduler.async(std::move(task));
            m_isTaskInFlight = true;
         }
      }
      return std::move(future);
   }

private:
   void completeTask()
   {
      // We just completed a task.  If there are no more tasks in the queue,
      // update IsTaskInFlight to false and stop doing work.  Otherwise
      // schedule the next task (while not holding the lock).
      std::function<void()> continuation;
      {
         std::lock_guard<std::mutex> locker(m_queueLock);
         if (m_tasks.empty()) {
            m_isTaskInFlight = false;
            return;
         }

         continuation = std::move(m_tasks.front());
         m_tasks.pop_front();
      }
      m_scheduler.async(std::move(continuation));
   }

   /// The thread pool on which to run the work.
   ThreadPool &m_scheduler;

   /// State which indicates whether the queue currently is currently processing
   /// any work.
   bool m_isTaskInFlight = false;

   /// Mutex for synchronizing access to the Tasks array.
   std::mutex m_queueLock;

   /// Tasks waiting for execution in the queue.
   std::deque<std::function<void()>> m_tasks;
};

} // utils
} // polar

#endif // POLARPHP_UTILS_TASK_QUEUE_H
