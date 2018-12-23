// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/09.

#ifndef POLARPHP_BASIC_ADT_DENSE_SET_H
#define POLARPHP_BASIC_ADT_DENSE_SET_H

#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/DenseMapInfo.h"
#include "polarphp/utils/MathExtras.h"
#include "polarphp/utils/TypeTraits.h"
#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <utility>

namespace polar {
namespace basic {

namespace internal {

struct DenseSetEmpty {};

// Use the empty base class trick so we can create a DenseMap where the buckets
// contain only a single item.
template <typename KeyType>
class DenseSetPair : public DenseSetEmpty
{
   KeyType m_key;

public:
   KeyType &getFirst()
   {
      return m_key;
   }

   const KeyType &getFirst() const
   {
      return m_key;
   }

   KeyType &first()
   {
      return m_key;
   }

   const KeyType &first() const
   {
      return m_key;
   }

   DenseSetEmpty &getSecond()
   {
      return *this;
   }

   const DenseSetEmpty &getSecond() const
   {
      return *this;
   }

   DenseSetEmpty &second()
   {
      return *this;
   }

   const DenseSetEmpty &second() const
   {
      return *this;
   }
};

/// Base class for DenseSet and DenseSmallSet.
///
/// MapType should be either
///
///   DenseMap<ValueType, internal::DenseSetEmpty, ValueInfoType,
///            internal::DenseSetPair<ValueType>>
///
/// or the equivalent SmallDenseMap type.  ValueInfoType must implement the
/// DenseMapInfo "concept".
template <typename ValueType, typename MapType, typename ValueInfoType>
class DenseSetImpl
{
   static_assert(sizeof(typename MapType::value_type) == sizeof(ValueType),
                 "DenseMap buckets unexpectedly large!");
   MapType m_theMap;

   template <typename T>
   using const_arg_type_t = typename polar::utils::const_pointer_or_const_ref<T>::type;

public:
   using key_type = ValueType;
   using value_type = ValueType;
   using size_type = unsigned;

   explicit DenseSetImpl(unsigned initialReserve = 0) : m_theMap(initialReserve)
   {}

   DenseSetImpl(std::initializer_list<ValueType> elems)
      : DenseSetImpl(polar::utils::power_of_two_ceil(elems.size()))
   {
      insert(elems.begin(), elems.end());
   }

   bool empty() const
   {
      return m_theMap.empty();
   }

   size_type getSize() const
   {
      return m_theMap.getSize();
   }

   size_type size() const
   {
      return getSize();
   }

   size_t getMemorySize() const
   {
      return m_theMap.getMemorySize();
   }

   /// Grow the DenseSet so that it has at least Size buckets. Will not shrink
   /// the Size of the set.
   void resize(size_t size)
   {
      m_theMap.resize(size);
   }

   /// Grow the DenseSet so that it can contain at least \p NumEntries items
   /// before resizing again.
   void reserve(size_t size)
   {
      m_theMap.reserve(size);
   }

   void clear()
   {
      m_theMap.clear();
   }

   /// Return 1 if the specified key is in the set, 0 otherwise.
   size_type count(const_arg_type_t<ValueType> value) const
   {
      return m_theMap.count(value);
   }

   bool erase(const ValueType &value)
   {
      return m_theMap.erase(value);
   }

   void swap(DenseSetImpl &rhs)
   {
      m_theMap.swap(rhs.m_theMap);
   }

   // Iterators.

   class ConstIterator;

   class Iterator
   {
      typename MapType::iterator m_iter;
      friend class DenseSetImpl;
      friend class ConstIterator;

   public:
      using difference_type = typename MapType::iterator::difference_type;
      using value_type = ValueType;
      using pointer = value_type *;
      using reference = value_type &;
      using iterator_category = std::forward_iterator_tag;

      Iterator() = default;
      Iterator(const typename MapType::iterator &i) : m_iter(i) {}

      ValueType &operator*()
      {
         return m_iter->getFirst();
      }

      const ValueType &operator*() const
      {
         return m_iter->getFirst();
      }

      ValueType *operator->()
      {
         return &m_iter->getFirst();
      }

      const ValueType *operator->() const
      {
         return &m_iter->getFirst();
      }

      Iterator& operator++()
      {
         ++m_iter;
         return *this;
      }
      Iterator operator++(int)
      {
         auto temp = *this;
         ++m_iter;
         return m_iter;
      }

      bool operator==(const ConstIterator& other) const
      {
         return m_iter == other.m_iter;
      }
      bool operator!=(const ConstIterator& other) const
      {
         return m_iter != other.m_iter;
      }
   };

   class ConstIterator
   {
      typename MapType::const_iterator m_iter;
      friend class DenseSet;
      friend class Iterator;

   public:
      using difference_type = typename MapType::const_iterator::difference_type;
      using value_type = ValueType;
      using pointer = const value_type *;
      using reference = const value_type &;
      using iterator_category = std::forward_iterator_tag;

      ConstIterator() = default;
      ConstIterator(const Iterator &other) : m_iter(other.m_iter)
      {}

      ConstIterator(const typename MapType::const_iterator &iter) : m_iter(iter)
      {}

      const ValueType &operator*() const
      {
         return m_iter->getFirst();
      }

      const ValueType *operator->() const
      {
         return &m_iter->getFirst();
      }

      ConstIterator& operator++()
      {
         ++m_iter;
         return *this;
      }

      ConstIterator operator++(int)
      {
         auto temp = *this;
         ++m_iter;
         return temp;
      }

      bool operator==(const ConstIterator &other) const
      {
         return m_iter == other.m_iter;
      }
      bool operator!=(const ConstIterator &other) const
      {
         return m_iter != other.m_iter;
      }
   };

   using iterator = Iterator;
   using const_iterator = ConstIterator;

   iterator begin()
   {
      return Iterator(m_theMap.begin());
   }

   iterator end()
   {
      return Iterator(m_theMap.end());
   }

   const_iterator begin() const
   {
      return ConstIterator(m_theMap.begin());
   }

   const_iterator end() const
   {
      return ConstIterator(m_theMap.end());
   }

   iterator find(const_arg_type_t<ValueType> value)
   {
      return Iterator(m_theMap.find(value));
   }

   const_iterator find(const_arg_type_t<ValueType> value) const
   {
      return ConstIterator(m_theMap.find(value));
   }

   /// Alternative version of find() which allows a different, and possibly less
   /// expensive, key type.
   /// The DenseMapInfo is responsible for supplying methods
   /// getHashValue(LookupKeyType) and isEqual(LookupKeyType, KeyType) for each key type
   /// used.
   template <class LookupKeyType>
   iterator findAs(const LookupKeyType &value)
   {
      return Iterator(m_theMap.findAs(value));
   }

   template <class LookupKeyType>
   const_iterator findAs(const LookupKeyType &value) const
   {
      return ConstIterator(m_theMap.findAs(value));
   }

   void erase(Iterator iter)
   {
      return m_theMap.erase(iter.m_iter);
   }

   void erase(ConstIterator citer)
   {
      return m_theMap.erase(citer.m_iter);
   }

   std::pair<iterator, bool> insert(const ValueType &value)
   {
      internal::DenseSetEmpty empty;
      return m_theMap.tryEmplace(value, empty);
   }

   std::pair<iterator, bool> insert(ValueType &&value)
   {
      internal::DenseSetEmpty empty;
      return m_theMap.tryEmplace(std::move(value), empty);
   }

   /// Alternative version of insert that uses a different (and possibly less
   /// expensive) key type.
   template <typename LookupKeyType>
   std::pair<iterator, bool> insertAs(const ValueType &value,
                                      const LookupKeyType &lookupKey)
   {
      return m_theMap.insertAs({value, internal::DenseSetEmpty()}, lookupKey);
   }

   template <typename LookupKeyType>
   std::pair<iterator, bool> insertAs(ValueType &&value, const LookupKeyType &lookupKey)
   {
      return m_theMap.insertAs({std::move(value), internal::DenseSetEmpty()}, lookupKey);
   }

   // Range insertion of values.
   template<typename InputIt>
   void insert(InputIt iter, InputIt end) {
      for (; iter != end; ++iter) {
         insert(*iter);
      }
   }
};

/// Equality comparison for DenseSet.
///
/// Iterates over elements of lhs confirming that each element is also a member
/// of rhs, and that rhs contains no additional values.
/// Equivalent to N calls to rhs.count. Amortized complexity is linear, worst
/// case is O(N^2) (if every hash collides).
template <typename ValueT, typename MapTy, typename ValueInfoT>
bool operator==(const DenseSetImpl<ValueT, MapTy, ValueInfoT> &lhs,
                const DenseSetImpl<ValueT, MapTy, ValueInfoT> &rhs)
{
   if (lhs.size() != rhs.size()) {
      return false;
   }
   for (auto &e : lhs) {
      if (!rhs.count(e)) {
         return false;
      }
   }
   return true;
}

/// Inequality comparison for DenseSet.
///
/// Equivalent to !(lhs == rhs). See operator== for performance notes.
template <typename ValueT, typename MapTy, typename ValueInfoT>
bool operator!=(const DenseSetImpl<ValueT, MapTy, ValueInfoT> &lhs,
                const DenseSetImpl<ValueT, MapTy, ValueInfoT> &rhs)
{
   return !(lhs == rhs);
}

} // end namespace internal

/// Implements a dense probed hash-table based set.
template <typename ValueType, typename ValueInfoType = DenseMapInfo<ValueType>>
class DenseSet : public internal::DenseSetImpl<
      ValueType, DenseMap<ValueType, internal::DenseSetEmpty, ValueInfoType,
      internal::DenseSetPair<ValueType>>,
      ValueInfoType> {
   using BaseType =
   internal::DenseSetImpl<ValueType,
   DenseMap<ValueType, internal::DenseSetEmpty, ValueInfoType,
   internal::DenseSetPair<ValueType>>,
   ValueInfoType>;

public:
   using BaseType::BaseType;
};

/// Implements a dense probed hash-table based set with some number of buckets
/// stored inline.
template <typename ValueType, unsigned InlineBuckets = 4,
          typename ValueInfoType = DenseMapInfo<ValueType>>
class SmallDenseSet
      : public internal::DenseSetImpl<
      ValueType, SmallDenseMap<ValueType, internal::DenseSetEmpty, InlineBuckets,
      ValueInfoType, internal::DenseSetPair<ValueType>>,
      ValueInfoType>
{
   using BaseType = internal::DenseSetImpl<
   ValueType, SmallDenseMap<ValueType, internal::DenseSetEmpty, InlineBuckets,
   ValueInfoType, internal::DenseSetPair<ValueType>>,
   ValueInfoType>;

public:
   using BaseType::BaseType;
};

} // basic
} // poalr

#endif // POLARPHP_BASIC_ADT_DENSE_SET_H
