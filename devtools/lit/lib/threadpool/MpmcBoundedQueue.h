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

// Copyright (c) 2010-2011 Dmitry Vyukov. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided
// that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice,
//   this list of
//      conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright
//   notice, this list
//      of conditions and the following disclaimer in the documentation and/or
//      other materials
//      provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY DMITRY VYUKOV "AS IS" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT
// SHALL DMITRY VYUKOV OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
// OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
// The views and conclusions contained in the software and documentation are
// those of the authors and
// should not be interpreted as representing official policies, either expressed
// or implied, of Dmitry Vyukov.

#ifndef POLAR_DEVLTOOLS_LIT_THREAD_POOL_MPMC_BOUNDED_QUEUE_H
#define POLAR_DEVLTOOLS_LIT_THREAD_POOL_MPMC_BOUNDED_QUEUE_H

#include <atomic>
#include <type_traits>
#include <vector>
#include <stdexcept>

namespace polar {
namespace lit {
namespace threadpool {

/**
 * @brief The MPMCBoundedQueue class implements bounded
 * multi-producers/multi-consumers lock-free queue.
 * Doesn't accept non-movable types as T.
 * Inspired by Dmitry Vyukov's mpmc queue.
 * http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
 */
template <typename T>
class MPMCBoundedQueue
{
   static_assert(
         std::is_move_constructible<T>::value, "Should be of movable type");

public:
   /**
     * @brief MPMCBoundedQueue Constructor.
     * @param size Power of 2 number - queue length.
     * @throws std::invalid_argument if size is bad.
     */
   explicit MPMCBoundedQueue(size_t size);

   /**
     * @brief Move ctor implementation.
     */
   MPMCBoundedQueue(MPMCBoundedQueue&& rhs) noexcept;

   /**
     * @brief Move assignment implementaion.
     */
   MPMCBoundedQueue& operator=(MPMCBoundedQueue&& rhs) noexcept;

   /**
     * @brief push Push data to queue.
     * @param data Data to be pushed.
     * @return true on success.
     */
   template <typename U>
   bool push(U&& data);

   /**
     * @brief pop Pop data from queue.
     * @param data Place to store popped data.
     * @return true on sucess.
     */
   bool pop(T& data);

private:
   struct Cell
   {
      std::atomic<size_t> m_sequence;
      T data;

      Cell() = default;

      Cell(const Cell&) = delete;
      Cell& operator=(const Cell&) = delete;

      Cell(Cell&& rhs)
         : m_sequence(rhs.m_sequence.load()), data(std::move(rhs.data))
      {
      }

      Cell& operator=(Cell&& rhs)
      {
         m_sequence = rhs.m_sequence.load();
         data = std::move(rhs.data);

         return *this;
      }
   };

private:
   typedef char Cacheline[64];

   Cacheline pad0;
   std::vector<Cell> m_buffer;
   /* const */ size_t m_bufferMask;
   Cacheline pad1;
   std::atomic<size_t> m_enqueuePos;
   Cacheline pad2;
   std::atomic<size_t> m_dequeuePos;
   Cacheline pad3;
};


/// Implementation

template <typename T>
inline MPMCBoundedQueue<T>::MPMCBoundedQueue(size_t size)
   : m_buffer(size), m_bufferMask(size - 1), m_enqueuePos(0),
     m_dequeuePos(0)
{
   bool sizeIsPowerOf2 = (size >= 2) && ((size & (size - 1)) == 0);
   if(!sizeIsPowerOf2)
   {
      throw std::invalid_argument("buffer size should be a power of 2");
   }

   for(size_t i = 0; i < size; ++i)
   {
      m_buffer[i].m_sequence = i;
   }
}

template <typename T>
inline MPMCBoundedQueue<T>::MPMCBoundedQueue(MPMCBoundedQueue&& rhs) noexcept
{
   *this = rhs;
}

template <typename T>
inline MPMCBoundedQueue<T>& MPMCBoundedQueue<T>::operator=(MPMCBoundedQueue&& rhs) noexcept
{
   if (this != &rhs)
   {
      m_buffer = std::move(rhs.m_buffer);
      m_bufferMask = std::move(rhs.m_bufferMask);
      m_enqueuePos = rhs.m_enqueuePos.load();
      m_dequeuePos = rhs.m_dequeuePos.load();
   }
   return *this;
}

template <typename T>
template <typename U>
inline bool MPMCBoundedQueue<T>::push(U&& data)
{
   Cell* cell;
   size_t pos = m_enqueuePos.load(std::memory_order_relaxed);
   for(;;)
   {
      cell = &m_buffer[pos & m_bufferMask];
      size_t seq = cell->m_sequence.load(std::memory_order_acquire);
      intptr_t dif = (intptr_t)seq - (intptr_t)pos;
      if(dif == 0)
      {
         if(m_enqueuePos.compare_exchange_weak(
                  pos, pos + 1, std::memory_order_relaxed))
         {
            break;
         }
      }
      else if(dif < 0)
      {
         return false;
      }
      else
      {
         pos = m_enqueuePos.load(std::memory_order_relaxed);
      }
   }

   cell->data = std::forward<U>(data);

   cell->m_sequence.store(pos + 1, std::memory_order_release);

   return true;
}

template <typename T>
inline bool MPMCBoundedQueue<T>::pop(T& data)
{
   Cell* cell;
   size_t pos = m_dequeuePos.load(std::memory_order_relaxed);
   for(;;)
   {
      cell = &m_buffer[pos & m_bufferMask];
      size_t seq = cell->m_sequence.load(std::memory_order_acquire);
      intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
      if(dif == 0)
      {
         if(m_dequeuePos.compare_exchange_weak(
                  pos, pos + 1, std::memory_order_relaxed))
         {
            break;
         }
      }
      else if(dif < 0)
      {
         return false;
      }
      else
      {
         pos = m_dequeuePos.load(std::memory_order_relaxed);
      }
   }

   data = std::move(cell->data);

   cell->m_sequence.store(
            pos + m_bufferMask + 1, std::memory_order_release);

   return true;
}

} // threadpool
} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_THREAD_POOL_MPMC_BOUNDED_QUEUE_H
