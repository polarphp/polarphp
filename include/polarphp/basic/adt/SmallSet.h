// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/20.

#ifndef POLARPHP_BASIC_ADT_SMALL_SET_H
#define POLARPHP_BASIC_ADT_SMALL_SET_H

#include "polarphp/basic/adt/SmallPtrSet.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/Iterator.h"
#include <cstddef>
#include <functional>
#include <set>
#include <utility>

namespace polar {
namespace basic {


/// SmallSetIterator - This class implements a const_iterator for SmallSet by
/// delegating to the underlying SmallVector or Set iterators.
template <typename T, unsigned N, typename C>
class SmallSetIterator
      : public IteratorFacadeBase<SmallSetIterator<T, N, C>,
      std::forward_iterator_tag, T> {
private:
   using SetIterType = typename std::set<T, C>::const_iterator;
   using VecIterType = typename SmallVector<T, N>::const_iterator;
   using SelfTy = SmallSetIterator<T, N, C>;

   /// Iterators to the parts of the SmallSet containing the data. They are set
   /// depending on m_isSmall.
   union {
      SetIterType m_setIter;
      VecIterType m_vecIter;
   };

   bool m_isSmall;

public:
   SmallSetIterator(SetIterType m_setIter)
      : m_setIter(m_setIter), m_isSmall(false)
   {}
   SmallSetIterator(VecIterType m_vecIter)
      : m_vecIter(m_vecIter), m_isSmall(true)
   {}

   // Spell out destructor, copy/move constructor and assignment operators for
   // MSVC STL, where set<T>::const_iterator is not trivially copy constructible.
   ~SmallSetIterator()
   {
      if (m_isSmall) {
         m_vecIter.~VecIterType();
      } else {
         m_setIter.~SetIterType();
      }
   }

   SmallSetIterator(const SmallSetIterator &other)
      : m_isSmall(other.m_isSmall)
   {
      if (m_isSmall) {
         m_vecIter = other.m_vecIter;
      } else {
         // Use placement new, to make sure m_setIter is properly constructed, even
         // if it is not trivially copy-able (e.g. in MSVC).
         new (&m_setIter) SetIterType(other.m_setIter);
      }

   }

   SmallSetIterator(SmallSetIterator &&other)
      : m_isSmall(other.m_isSmall)
   {
      if (m_isSmall) {
         m_vecIter = std::move(other.m_vecIter);
      } else {
         // Use placement new, to make sure m_setIter is properly constructed, even
         // if it is not trivially copy-able (e.g. in MSVC).
         new (&m_setIter) SetIterType(std::move(other.m_setIter));
      }
   }

   SmallSetIterator& operator=(const SmallSetIterator &other) {
      // Call destructor for m_setIter, so it gets properly destroyed if it is
      // not trivially destructible in case we are setting m_vecIter.
      if (!m_isSmall) {
         m_setIter.~SetIterType();
      }
      m_isSmall = other.m_isSmall;
      if (m_isSmall) {
         m_vecIter = other.m_vecIter;
      } else {
         new (&m_setIter) SetIterType(other.m_setIter);
      }
      return *this;
   }

   SmallSetIterator& operator=(SmallSetIterator &&other)
   {
      // Call destructor for m_setIter, so it gets properly destroyed if it is
      // not trivially destructible in case we are setting m_vecIter.
      if (!m_isSmall) {
         m_setIter.~SetIterType();
      }
      m_isSmall = other.m_isSmall;
      if (m_isSmall) {
         m_vecIter = std::move(other.m_vecIter);
      } else {
         new (&m_setIter) SetIterType(std::move(other.m_setIter));
      }
      return *this;
   }

   bool operator==(const SmallSetIterator &rhs) const
   {
      if (m_isSmall != rhs.m_isSmall) {
         return false;
      }
      if (m_isSmall) {
         return m_vecIter == rhs.m_vecIter;
      }
      return m_setIter == rhs.m_setIter;
   }

   SmallSetIterator &operator++()
   { // Preincrement
      if (m_isSmall) {
         m_vecIter++;
      } else {
         m_setIter++;
      }
      return *this;
   }

   const T &operator*() const
   {
      return m_isSmall ? *m_vecIter : *m_setIter;
   }
};


/// SmallSet - This maintains a set of unique values, optimizing for the case
/// when the set is small (less than N).  In this case, the set can be
/// maintained with no mallocs.  If the set gets large, we expand to using an
/// std::set to maintain reasonable lookup times.
///
/// Note that this set does not provide a way to iterate over members in the
/// set.
template <typename T, unsigned N, typename C = std::less<T>>
class SmallSet
{
   /// Use a SmallVector to hold the elements here (even though it will never
   /// reach its 'large' stage) to avoid calling the default ctors of elements
   /// we will never use.
   SmallVector<T, N> m_vector;
   std::set<T, C> m_set;

   using VIterator = typename SmallVector<T, N>::const_iterator;
   using mutable_iterator = typename SmallVector<T, N>::iterator;

   // In small mode SmallPtrSet uses linear search for the elements, so it is
   // not a good idea to choose this value too high. You may consider using a
   // DenseSet<> instead if you expect many elements in the set.
   static_assert(N <= 32, "N should be small");

public:
   using size_type = size_t;
   using const_iterator = SmallSetIterator<T, N, C>;

   SmallSet() = default;

   POLAR_NODISCARD bool empty() const
   {
      return m_vector.empty() && m_set.empty();
   }

   size_type size() const
   {
      return isSmall() ? m_vector.getSize() : m_set.size();
   }

   size_type getSize() const
   {
      return size();
   }

   /// count - Return 1 if the element is in the set, 0 otherwise.
   size_type count(const T &value) const
   {
      if (isSmall()) {
         // Since the collection is small, just do a linear search.
         return vfind(value) == m_vector.end() ? 0 : 1;
      } else {
         return m_set.count(value);
      }
   }

   /// insert - Insert an element into the set if it isn't already there.
   /// Returns true if the element is inserted (it was not in the set before).
   /// The first value of the returned pair is unused and provided for
   /// partial compatibility with the standard library self-associative container
   /// concept.
   // FIXME: Add iterators that abstract over the small and large form, and then
   // return those here.
   std::pair<std::nullopt_t, bool> insert(const T &value)
   {
      if (!isSmall()) {
         return std::make_pair(std::nullopt, m_set.insert(value).second);
      }
      VIterator iter = vfind(value);
      if (iter != m_vector.end()) {    // Don't reinsert if it already exists.
         return std::make_pair(std::nullopt, false);
      }
      if (m_vector.getSize() < N) {
         m_vector.push_back(value);
         return std::make_pair(std::nullopt, true);
      }
      // Otherwise, grow from m_vector to set.
      while (!m_vector.empty()) {
         m_set.insert(m_vector.getBack());
         m_vector.pop_back();
      }
      m_set.insert(value);
      return std::make_pair(std::nullopt, true);
   }

   template <typename IterType>
   void insert(IterType iter, IterType end)
   {
      for (; iter != end; ++iter) {
         insert(*iter);
      }
   }

   bool erase(const T &value)
   {
      if (!isSmall()) {
         return m_set.erase(value);
      }
      for (mutable_iterator iter = m_vector.begin(), end = m_vector.end(); iter != end; ++iter) {
         if (*iter == value) {
            m_vector.erase(iter);
            return true;
         }
      }
      return false;
   }

   void clear()
   {
      m_vector.clear();
      m_set.clear();
   }

   const_iterator begin() const
   {
      if (isSmall()) {
         return {m_vector.begin()};
      }
      return {m_set.begin()};
   }

   const_iterator end() const
   {
      if (isSmall()) {
         return {m_vector.end()};
      }
      return {m_set.end()};
   }

private:
   bool isSmall() const
   {
      return m_set.empty();
   }

   VIterator vfind(const T &value) const
   {
      for (VIterator iter = m_vector.begin(), end = m_vector.end(); iter != end; ++iter) {
         if (*iter == value) {
            return iter;
         }
      }
      return m_vector.end();
   }
};

/// If this set is of pointer values, transparently switch over to using
/// SmallPtrSet for performance.
template <typename PointeeType, unsigned N>
class SmallSet<PointeeType*, N> : public SmallPtrSet<PointeeType*, N>
{};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_SMALL_SET_H
