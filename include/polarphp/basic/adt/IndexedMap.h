// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018  polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of  polarphp project authors
//
// Created by polarboy on 2018/06/24.

//===----------------------------------------------------------------------===//
//
// This file implements an indexed map. The index map template takes two
// types. The first is the mapped type and the second is a functor
// that maps its argument to a size_t. On instantiation a "null" value
// can be provided to be used as a "does not exist" indicator in the
// map. A member function grow() is provided that given the value of
// the maximally indexed key (the argument of the functor) makes sure
// the map has enough space for it.
//
//===----------------------------------------------------------------------===//

#ifndef  POLARPHP_BASIC_ADT_IMMUTABLE_INDEXED_MAP_H
#define  POLARPHP_BASIC_ADT_IMMUTABLE_INDEXED_MAP_H

#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StlExtras.h"
#include <cassert>

namespace polar {
namespace basic {

template <typename T, typename ToIndexType = Identity<unsigned>>
class IndexedMap
{
   using IndexType = typename ToIndexType::argument_type;
   // Prefer SmallVector with zero inline storage over std::vector. IndexedMaps
   // can grow very large and SmallVector grows more efficiently as long as T
   // is trivially copyable.
   using StorageType = SmallVector<T, 0>;

   StorageType m_storage;
   T m_nullValue;
   ToIndexType m_toIndex;

public:
   IndexedMap() : m_nullValue(T())
   {}

   explicit IndexedMap(const T &value) : m_nullValue(value)
   {}

   typename StorageType::reference operator[](IndexType n)
   {
      assert(m_toIndex(n) < m_storage.getSize() && "index out of bounds!");
      return m_storage[m_toIndex(n)];
   }

   typename StorageType::const_reference operator[](IndexType n) const
   {
      assert(m_toIndex(n) < m_storage.getSize() && "index out of bounds!");
      return m_storage[m_toIndex(n)];
   }

   void reserve(typename StorageType::size_type size)
   {
      m_storage.reserve(size);
   }

   void resize(typename StorageType::size_type size)
   {
      m_storage.resize(size, m_nullValue);
   }

   void clear()
   {
      m_storage.clear();
   }

   void grow(IndexType n)
   {
      unsigned newSize = m_toIndex(n) + 1;
      if (newSize > m_storage.size()) {
         resize(newSize);
      }
   }

   bool inBounds(IndexType n) const
   {
      return m_toIndex(n) < m_storage.getSize();
   }

   typename StorageType::size_type size() const
   {
      return m_storage.size();
   }

   typename StorageType::size_type getSize() const
   {
      return size();
   }
};

} // basic
} // polar

#endif //  POLARPHP_BASIC_ADT_IMMUTABLE_INDEXED_MAP_H
