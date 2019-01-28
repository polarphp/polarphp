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

#ifndef POLARPHP_BASIC_ADT_PRIORITY_WORKLIST_H
#define POLARPHP_BASIC_ADT_PRIORITY_WORKLIST_H

#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallVector.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <vector>

namespace polar {
namespace basic {

/// A FILO worklist that prioritizes on re-insertion without duplication.
///
/// This is very similar to a \c SetVector with the primary difference that
/// while re-insertion does not create a duplicate, it does adjust the
/// visitation order to respect the last insertion point. This can be useful
/// when the visit order needs to be prioritized based on insertion point
/// without actually having duplicate visits.
///
/// Note that this doesn't prevent re-insertion of elements which have been
/// visited -- if you need to break cycles, a set will still be necessary.
///
/// The type \c T must be default constructable to a null value that will be
/// ignored. It is an error to insert such a value, and popping elements will
/// never produce such a value. It is expected to be used with common nullable
/// types like pointers or optionals.
///
/// Internally this uses a vector to store the worklist and a map to identify
/// existing elements in the worklist. Both of these may be customized, but the
/// map must support the basic DenseMap API for mapping from a T to an integer
/// index into the vector.
///
/// A partial specialization is provided to automatically select a SmallVector
/// and a SmallDenseMap if custom data structures are not provided.
template <typename T, typename VectorType = std::vector<T>,
          typename MapType = DenseMap<T, ptrdiff_t>>
class PriorityWorklist
{
public:
   using value_type = T;
   using key_type = T;
   using reference = T&;
   using const_reference = const T&;
   using size_type = typename MapType::size_type;

   /// Construct an empty PriorityWorklist
   PriorityWorklist() = default;

   /// Determine if the PriorityWorklist is empty or not.
   bool empty() const
   {
      return m_vector.empty();
   }

   /// Returns the number of elements in the worklist.
   size_type size() const
   {
      return m_map.getSize();
   }

   /// Count the number of elements of a given key in the PriorityWorklist.
   /// \returns 0 if the element is not in the PriorityWorklist, 1 if it is.
   size_type count(const key_type &key) const

   {
      return m_map.count(key);
   }

   /// Return the last element of the PriorityWorklist.
   const T &back() const
   {
      assert(!empty() && "Cannot call back() on empty PriorityWorklist!");
      return m_vector.back();
   }

   /// Insert a new element into the PriorityWorklist.
   /// \returns true if the element was inserted into the PriorityWorklist.
   bool insert(const T &item)
   {
      assert(item != T() && "Cannot insert a null (default constructed) value!");
      auto insertResult = m_map.insert({item, m_vector.size()});
      if (insertResult.second) {
         // Fresh value, just append it to the vector.
         m_vector.push_back(item);
         return true;
      }

      auto &index = insertResult.first->second;
      assert(m_vector[index] == item && "Value not actually at index in map!");
      if (index != (ptrdiff_t)(m_vector.size() - 1)) {
         // If the element isn't at the back, null it out and append a fresh one.
         m_vector[index] = T();
         index = (ptrdiff_t)m_vector.size();
         m_vector.push_back(item);
      }
      return false;
   }

   /// Insert a sequence of new elements into the PriorityWorklist.
   template <typename SequenceT>
   typename std::enable_if<!std::is_convertible<SequenceT, T>::value>::type
   insert(SequenceT &&input)
   {
      if (std::begin(input) == std::end(input)) {
         // Nothing to do for an empty input sequence.
         return;
      }
      // First pull the input sequence into the vector as a bulk append
      // operation.
      ptrdiff_t startIndex = m_vector.size();
      m_vector.insert(m_vector.end(), std::begin(input), std::end(input));
      // Now walk backwards fixing up the index map and deleting any duplicates.
      for (ptrdiff_t i = m_vector.size() - 1; i >= startIndex; --i) {
         auto insertResult = m_map.insert({m_vector[i], i});
         if (insertResult.second) {
            continue;
         }
         // If the existing index is before this insert's start, nuke that one and
         // move it up.
         ptrdiff_t &index = insertResult.first->second;
         if (index < startIndex) {
            m_vector[index] = T();
            index = i;
            continue;
         }
         // Otherwise the existing one comes first so just clear out the value in
         // this slot.
         m_vector[i] = T();
      }
   }

   /// Remove the last element of the PriorityWorklist.
   void popBack()
   {
      assert(!empty() && "Cannot remove an element when empty!");
      assert(back() != T() && "Cannot have a null element at the back!");
      m_map.erase(back());
      do {
         m_vector.pop_back();
      } while (!m_vector.empty() && m_vector.back() == T());
   }

   void pop_back()
   {
      popBack();
   }

   POLAR_NODISCARD T popBackValue()
   {
      T ret = back();
      pop_back();
      return ret;
   }

   /// Erase an item from the worklist.
   ///
   /// Note that this is constant time due to the nature of the worklist implementation.
   bool erase(const T &value)
   {
      auto iter = m_map.find(value);
      if (iter == m_map.end()) {
         return false;
      }
      assert(m_vector[iter->second] == value && "Value not actually at index in map!");
      if (iter->second == (ptrdiff_t)(m_vector.size() - 1)) {
         do {
            m_vector.pop_back();
         } while (!m_vector.empty() && m_vector.back() == T());
      } else {
         m_vector[iter->second] = T();
      }
      m_map.erase(iter);
      return true;
   }

   /// Erase items from the set vector based on a predicate function.
   ///
   /// This is intended to be equivalent to the following code, if we could
   /// write it:
   ///
   /// \code
   ///   m_vector.erase(remove_if(V, P), m_vector.end());
   /// \endcode
   ///
   /// However, PriorityWorklist doesn't expose non-const iterators, making any
   /// algorithm like remove_if impossible to use.
   ///
   /// \returns true if any element is removed.
   template <typename UnaryPredicate>
   bool eraseIf(UnaryPredicate pred)
   {
      typename VectorType::iterator end =
            remove_if(m_vector, TestAndEraseFromMap<UnaryPredicate>(pred, m_map));
      if (end == m_vector.end()) {
         return false;
      }

      for (auto iter = m_vector.begin(); iter != end; ++iter) {
         if (*iter != T()) {
            m_map[*iter] = iter - m_vector.begin();
         }
      }
      m_vector.erase(end, m_vector.end());
      return true;
   }

   /// Reverse the items in the PriorityWorklist.
   ///
   /// This does an in-place reversal. Other kinds of reverse aren't easy to
   /// support in the face of the worklist semantics.

   /// Completely clear the PriorityWorklist
   void clear()
   {
      m_map.clear();
      m_vector.clear();
   }

private:
   /// A wrapper predicate designed for use with std::remove_if.
   ///
   /// This predicate wraps a predicate suitable for use with std::remove_if to
   /// call m_map.erase(x) on each element which is slated for removal. This just
   /// allows the predicate to be move only which we can't do with lambdas
   /// today.
   template <typename UnaryPredicateT>
   class TestAndEraseFromMap
   {
      UnaryPredicateT m_pred;
      MapType &m_map;

   public:
      TestAndEraseFromMap(UnaryPredicateT pred, MapType &map)
         : m_pred(std::move(pred)), m_map(map)
      {}

      bool operator()(const T &arg)
      {
         if (arg == T()) {
            // Skip null values in the PriorityWorklist.
            return false;
         }
         if (m_pred(arg)) {
            m_map.erase(arg);
            return true;
         }
         return false;
      }
   };

   /// The map from value to index in the vector.
   MapType m_map;

   /// The vector of elements in insertion order.
   VectorType m_vector;
};

/// A version of \c PriorityWorklist that selects small size optimized data
/// structures for the vector and map.
template <typename T, unsigned N>
class SmallPriorityWorkList
      : public PriorityWorklist<T, SmallVector<T, N>,
      SmallDenseMap<T, ptrdiff_t>>
{
public:
   SmallPriorityWorkList() = default;
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_PRIORITY_WORKLIST_H
