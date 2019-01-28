// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/21.

#ifndef POLARPHP_UTILS_ARRAY_RECYCLER_H
#define POLARPHP_UTILS_ARRAY_RECYCLER_H

#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/utils/Allocator.h"
#include "polarphp/utils/MathExtras.h"

namespace polar {
namespace utils {

/// Recycle small arrays allocated from a BumpPtrAllocator.
///
/// Arrays are allocated in a small number of fixed sizes. For each supported
/// array size, the ArrayRecycler keeps a free list of available arrays.
///
template <typename T, size_t Align = alignof(T)>
class ArrayRecycler
{
   // The free list for a given array size is a simple singly linked list.
   // We can't use iplist or Recycler here since those classes can't be copied.
   struct FreeList
   {
      FreeList *m_next;
   };

   static_assert(Align >= alignof(FreeList), "Object underaligned");
   static_assert(sizeof(T) >= sizeof(FreeList), "Objects are too small");

   // Keep a free list for each array size.
   SmallVector<FreeList*, 8> m_bucket;

   // Remove an entry from the free list in m_bucket[Idx] and return it.
   // Return NULL if no entries are available.
   T *pop(unsigned index)
   {
      if (index >= m_bucket.getSize()) {
         return nullptr;
      }
      FreeList *entry = m_bucket[index];
      if (!entry) {
         return nullptr;
      }
      __asan_unpoison_memory_region(entry, Capacity::get(index).getSize());
      m_bucket[index] = entry->m_next;
      __msan_allocated_memory(entry, Capacity::get(index).getSize());
      return reinterpret_cast<T*>(entry);
   }

   // Add an entry to the free list at m_bucket[Idx].
   void push(unsigned index, T *ptr)
   {
      assert(ptr && "Cannot recycle NULL pointer");
      FreeList *entry = reinterpret_cast<FreeList*>(ptr);
      if (index >= m_bucket.getSize()) {
         m_bucket.resize(size_t(index) + 1);
      }
      entry->m_next = m_bucket[index];
      m_bucket[index] = entry;
      __asan_poison_memory_region(ptr, Capacity::get(index).getSize());
   }

public:
   /// The size of an allocated array is represented by a Capacity instance.
   ///
   /// This class is much smaller than a size_t, and it provides methods to work
   /// with the set of legal array capacities.
   class Capacity
   {
      uint8_t m_index;
      explicit Capacity(uint8_t idx) : m_index(idx)
      {}

   public:
      Capacity() : m_index(0)
      {}

      /// Get the capacity of an array that can hold at least N elements.
      static Capacity get(size_t N)
      {
         return Capacity(N ? log2_ceil_64(static_cast<uint64_t>(N)) : 0);
      }

      /// Get the number of elements in an array with this capacity.
      size_t getSize() const
      {
         return size_t(1u) << m_index;
      }

      /// Get the bucket number for this capacity.
      unsigned getBucket() const
      {
         return m_index;
      }

      /// Get the next larger capacity. Large capacities grow exponentially, so
      /// this function can be used to reallocate incrementally growing vectors
      /// in amortized linear time.
      Capacity getNext() const
      {
         return Capacity(m_index + 1);
      }
   };

   ~ArrayRecycler()
   {
      // The client should always call clear() so recycled arrays can be returned
      // to the allocator.
      assert(m_bucket.empty() && "Non-empty ArrayRecycler deleted!");
   }

   /// Release all the tracked allocations to the allocator. The recycler must
   /// be free of any tracked allocations before being deleted.
   template<class AllocatorType>
   void clear(AllocatorType &allocator)
   {
      for (; !m_bucket.empty(); m_bucket.pop_back()) {
         while (T *Ptr = pop(m_bucket.getSize() - 1)) {
            allocator.deallocate(Ptr);
         }
      }
   }

   /// Special case for BumpPtrAllocator which has an empty Deallocate()
   /// function.
   ///
   /// There is no need to traverse the free lists, pulling all the objects into
   /// cache.
   void clear(BumpPtrAllocator&)
   {
      m_bucket.clear();
   }

   /// Allocate an array of at least the requested capacity.
   ///
   /// Return an existing recycled array, or allocate one from Allocator if
   /// none are available for recycling.
   ///
   template<typename AllocatorType>
   T *allocate(Capacity capacity, AllocatorType &allocator)
   {
      // Try to recycle an existing array.
      if (T *ptr = pop(capacity.getBucket())) {
         return ptr;
      }
      // Nope, get more memory.
      return static_cast<T*>(allocator.allocate(sizeof(T)*capacity.getSize(), Align));
   }

   /// Deallocate an array with the specified Capacity.
   ///
   /// Cap must be the same capacity that was given to allocate().
   ///
   void deallocate(Capacity capacity, T *ptr)
   {
      push(capacity.getBucket(), ptr);
   }
};

} // utils
} // polar

#endif // POLARPHP_UTILS_ARRAY_RECYCLER_H
