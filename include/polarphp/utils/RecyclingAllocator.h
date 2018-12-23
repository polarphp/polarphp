// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/24.

#ifndef POLARPHP_UTILS_RECYCLINGALLOCATOR_H
#define POLARPHP_UTILS_RECYCLINGALLOCATOR_H

#include "polarphp/utils/Recycler.h"

namespace polar {
namespace utils {

/// RecyclingAllocator - This class wraps an Allocator, adding the
/// functionality of recycling deleted objects.
///
template <class AllocatorType, class T, size_t Size = sizeof(T),
          size_t Align = alignof(T)>
class RecyclingAllocator
{
private:
   /// Base - Implementation details.
   ///
   Recycler<T, Size, Align> m_base;

   /// Allocator - The wrapped allocator.
   ///
   AllocatorType m_allocator;

public:
   ~RecyclingAllocator()
   {
      m_base.clear(m_allocator);
   }

   /// Allocate - Return a pointer to storage for an object of type
   /// SubClass. The storage may be either newly allocated or recycled.
   ///
   template<class SubClass>
   SubClass *allocate()
   {
      return m_base.template allocate<SubClass>(m_allocator);
   }

   T *allocate()
   {
      return m_base.Allocate(m_allocator);
   }

   /// Deallocate - Release storage for the pointed-to object. The
   /// storage will be kept track of and may be recycled.
   ///
   template<class SubClass>
   void deallocate(SubClass *subObject)
   {
      return m_base.deallocate(m_allocator, subObject);
   }

   void printStats()
   {
      m_allocator.printStats();
      m_base.printStats();
   }
};

} // utils
} // polar

using polar::utils::RecyclingAllocator;

template<class AllocatorType, class T, size_t Size, size_t Align>
inline void *operator new(size_t size,
                          RecyclingAllocator<AllocatorType,
                          T, Size, Align> &allocator)
{
   assert(size <= Size && "allocation size exceeded");
   return allocator.allocate();
}

template<class AllocatorType, class T, size_t Size, size_t Align>
inline void operator delete(void *subObject,
                            RecyclingAllocator<AllocatorType,
                            T, Size, Align> &allocator)
{
   allocator.deallocate(subObject);
}

#endif // POLARPHP_UTILS_RECYCLINGALLOCATOR_H
