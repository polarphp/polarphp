// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/25.

#ifndef POLARPHP_BASIC_ADT_MAP_VECTOR_H
#define POLARPHP_BASIC_ADT_MAP_VECTOR_H

#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/SmallVector.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace polar {
namespace basic {

/// This class implements a map that also provides access to all stored values
/// in a deterministic order. The values are kept in a std::vector and the
/// mapping is done with DenseMap from Keys to indexes in that m_vector.
template<typename KeyT, typename ValueT,
         typename MapType = DenseMap<KeyT, unsigned>,
         typename VectorType = std::vector<std::pair<KeyT, ValueT>>>
class MapVector
{
   MapType m_map;
   VectorType m_vector;
   static_assert(
         std::is_integral<typename MapType::mapped_type>::value,
         "The mapped_type of the specified Map must be an integral type");
public:
   using value_type = typename VectorType::value_type;
   using size_type = typename VectorType::size_type;
   using iterator = typename VectorType::iterator;
   using const_iterator = typename VectorType::const_iterator;
   using reverse_iterator = typename VectorType::reverse_iterator;
   using const_reverse_iterator = typename VectorType::const_reverse_iterator;

   /// Clear the MapVector and return the underlying m_vector.
   VectorType takeVector()
   {
      m_map.clear();
      return std::move(m_vector);
   }

   size_type size() const
   {
      return m_vector.size();
   }

   /// Grow the MapVector so that it can contain at least \p numEntries items
   /// before resizing again.
   void reserve(size_type numEntries)
   {
      m_map.reserve(numEntries);
      m_vector.reserve(numEntries);
   }

   iterator begin()
   {
      return m_vector.begin();
   }

   const_iterator begin() const
   {
      return m_vector.begin();
   }

   iterator end()
   {
      return m_vector.end();
   }

   const_iterator end() const
   {
      return m_vector.end();
   }

   reverse_iterator rbegin()
   {
      return m_vector.rbegin();
   }

   const_reverse_iterator rbegin() const
   {
      return m_vector.rbegin();
   }

   reverse_iterator rend()
   {
      return m_vector.rend();
   }

   const_reverse_iterator rend() const
   {
      return m_vector.rend();
   }

   bool empty() const
   {
      return m_vector.empty();
   }

   std::pair<KeyT, ValueT> &front()
   {
      return m_vector.front();
   }

   const std::pair<KeyT, ValueT> &front() const
   {
      return m_vector.front();
   }

   std::pair<KeyT, ValueT> &back()
   {
      return m_vector.back();
   }

   const std::pair<KeyT, ValueT> &back() const
   {
      return m_vector.back();
   }

   void clear()
   {
      m_map.clear();
      m_vector.clear();
   }

   void swap(MapVector &other)
   {
      std::swap(m_map, other.m_map);
      std::swap(m_vector, other.m_vector);
   }

   ValueT &operator[](const KeyT &key)
   {
      std::pair<KeyT, unsigned> pair = std::make_pair(key, 0);
      std::pair<typename MapType::iterator, bool> result = m_map.insert(pair);
      unsigned &iter = result.first->second;
      if (result.second) {
         m_vector.push_back(std::make_pair(key, ValueT()));
         iter = m_vector.size() - 1;
      }
      return m_vector[iter].second;
   }

   // Returns a copy of the value.  Only allowed if ValueT is copyable.
   ValueT lookup(const KeyT &key) const
   {
      static_assert(std::is_copy_constructible<ValueT>::value,
                    "Cannot call lookup() if ValueT is not copyable.");
      typename MapType::const_iterator pos = m_map.find(key);
      return pos == m_map.end()? ValueT() : m_vector[pos->second].second;
   }

   std::pair<iterator, bool> insert(const std::pair<KeyT, ValueT> &keyValue)
   {
      std::pair<KeyT, typename MapType::mapped_type> pair = std::make_pair(keyValue.first, 0);
      std::pair<typename MapType::iterator, bool> result = m_map.insert(pair);
      auto &iter = result.first->second;
      if (result.second) {
         m_vector.push_back(std::make_pair(keyValue.first, keyValue.second));
         iter = m_vector.size() - 1;
         return std::make_pair(std::prev(end()), true);
      }
      return std::make_pair(begin() + iter, false);
   }

   std::pair<iterator, bool> insert(std::pair<KeyT, ValueT> &&keyValue)
   {
      // Copy KV.first into the map, then move it into the m_vector.
      std::pair<KeyT, typename MapType::mapped_type> pair = std::make_pair(keyValue.first, 0);
      std::pair<typename MapType::iterator, bool> result = m_map.insert(pair);
      auto &iter = result.first->second;
      if (result.second) {
         m_vector.push_back(std::move(keyValue));
         iter = m_vector.size() - 1;
         return std::make_pair(std::prev(end()), true);
      }
      return std::make_pair(begin() + iter, false);
   }

   size_type count(const KeyT &key) const
   {
      typename MapType::const_iterator pos = m_map.find(key);
      return pos == m_map.end()? 0 : 1;
   }

   iterator find(const KeyT &key)
   {
      typename MapType::const_iterator pos = m_map.find(key);
      return pos == m_map.end()? m_vector.end() :
                                 (m_vector.begin() + pos->second);
   }

   const_iterator find(const KeyT &key) const
   {
      typename MapType::const_iterator pos = m_map.find(key);
      return pos == m_map.end()? m_vector.end() :
                                 (m_vector.begin() + pos->second);
   }

   /// Remove the last element from the m_vector.
   void pop_back()
   {
      typename MapType::iterator pos = m_map.find(m_vector.back().first);
      m_map.erase(pos);
      m_vector.pop_back();
   }

   void popBack()
   {
      pop_back();
   }

   /// Remove the element given by Iterator.
   ///
   /// Returns an iterator to the element following the one which was removed,
   /// which may be end().
   ///
   /// \note This is a deceivingly expensive operation (linear time).  It's
   /// usually better to use \a removeIf() if possible.
   typename VectorType::iterator erase(typename VectorType::iterator iterator)
   {
      m_map.erase(iterator->first);
      auto next = m_vector.erase(iterator);
      if (next == m_vector.end()) {
         return next;
      }
      // Update indices in the m_map.
      size_t index = next - m_vector.begin();
      for (auto &iter : m_map) {
         assert(iter.second != index && "Index was already erased!");
         if (iter.second > index) {
            --iter.second;
         }
      }
      return next;
   }

   /// Remove all elements with the key value Key.
   ///
   /// Returns the number of elements removed.
   size_type erase(const KeyT &key)
   {
      auto iterator = find(key);
      if (iterator == end()) {
         return 0;
      }
      erase(iterator);
      return 1;
   }

   /// Remove the elements that match the predicate.
   ///
   /// Erase all elements that match \c Pred in a single pass.  Takes linear
   /// time.
   template <typename Predicate> void removeIf(Predicate pred);
};

template <typename KeyT, typename ValueT, typename MapType, typename VectorType>
template <typename Function>
void MapVector<KeyT, ValueT, MapType, VectorType>::removeIf(Function pred)
{
   auto begin = m_vector.begin();
   for (auto iter = begin, end = m_vector.end(); iter != end; ++iter) {
      if (pred(*iter)) {
         // Erase from the m_map.
         m_map.erase(iter->first);
         continue;
      }

      if (iter != begin) {
         // Move the value and update the index in the m_map.
         *begin = std::move(*iter);
         m_map[begin->first] = begin - m_vector.begin();
      }
      ++begin;
   }
   // Erase trailing entries in the m_vector.
   m_vector.erase(begin, m_vector.end());
}

/// A MapVector that performs no allocations if smaller than a certain
/// size.
template <typename KeyT, typename ValueT, unsigned N>
struct SmallMapVector
      : MapVector<KeyT, ValueT, SmallDenseMap<KeyT, unsigned, N>,
      SmallVector<std::pair<KeyT, ValueT>, N>>
{
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_MAP_VECTOR_H
