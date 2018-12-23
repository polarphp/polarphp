// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/03.

#ifndef POLARPHP_UTILS_RECYCLER_H
#define POLARPHP_UTILS_RECYCLER_H

#include "polarphp/basic/adt/IntrusiveList.h"
#include "polarphp/utils/Allocator.h"
#include "polarphp/utils/ErrorHandling.h"
#include <cassert>

namespace polar {
namespace utils {


/// PrintRecyclingAllocatorStats - Helper for RecyclingAllocator for
/// printing statistics.
///
void print_recycler_stats(size_t size, size_t align, size_t freeListSize);

/// Recycler - This class manages a linked-list of deallocated nodes
/// and facilitates reusing deallocated memory in place of allocating
/// new memory.
///
template <typename T, size_t Size = sizeof(T), size_t Align = alignof(T)>
class Recycler
{
   struct FreeNode
   {
      FreeNode *m_next;
   };

   /// List of nodes that have deleted contents and are not in active use.
   FreeNode *m_freeList = nullptr;

   FreeNode *popValue()
   {
      auto *value = m_freeList;
      __asan_unpoison_memory_region(value, Size);
      m_freeList = m_freeList->m_next;
      __msan_allocated_memory(value, Size);
      return value;
   }

   void push(FreeNode *node)
   {
      node->m_next = m_freeList;
      m_freeList = node;
      __asan_poison_memory_region(node, Size);
   }

public:
   ~Recycler()
   {
      // If this fails, either the callee has lost track of some allocation,
      // or the callee isn't tracking allocations and should just call
      // clear() before deleting the Recycler.
      assert(!m_freeList && "Non-empty recycler deleted!");
   }

   /// clear - Release all the tracked allocations to the allocator. The
   /// recycler must be free of any tracked allocations before being
   /// deleted; calling clear is one way to ensure this.
   template<class AllocatorType>
   void clear(AllocatorType &allocator)
   {
      while (m_freeList) {
         T *temp = reinterpret_cast<T *>(popValue());
         allocator.Deallocate(temp);
      }
   }

   /// Special case for BumpPtrAllocator which has an empty Deallocate()
   /// function.
   ///
   /// There is no need to traverse the free list, pulling all the objects into
   /// cache.
   void clear(BumpPtrAllocator &)
   {
      m_freeList = nullptr;
   }

   template<class SubClass, class AllocatorType>
   SubClass *allocate(AllocatorType &allocator)
   {
      static_assert(alignof(SubClass) <= Align,
                    "Recycler allocation alignment is less than object align!");
      static_assert(sizeof(SubClass) <= Size,
                    "Recycler allocation size is less than object size!");
      return m_freeList ? reinterpret_cast<SubClass *>(popValue())
                        : static_cast<SubClass *>(allocator.allocate(Size, Align));
   }

   template<class AllocatorType>
   T *allocate(AllocatorType &allocator)
   {
      return allocate<T>(allocator);
   }

   template<class SubClass, class AllocatorType>
   void deallocate(AllocatorType & /*Allocator*/, SubClass* element)
   {
      push(reinterpret_cast<FreeNode *>(element));
   }

   void printStats();
};

template <class T, size_t Size, size_t Align>
void Recycler<T, Size, Align>::printStats()
{
   size_t size = 0;
   for (auto *iter = m_freeList; iter; iter = iter->m_next) {
      ++size;
   }
   print_recycler_stats(Size, Align, size);
}

} // utils
} // polar

#endif // POLARPHP_UTILS_RECYCLER_H
