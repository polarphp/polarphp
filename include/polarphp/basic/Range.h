//===--- Range.h - Classes for conveniently working with ranges -*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/29.
//===----------------------------------------------------------------------===//
///
///  \file
///
///  This file provides classes and functions for conveniently working
///  with ranges,
///
///  reversed returns an iterator_range out of the reverse iterators of a type.
///
///  map creates an iterator_range which applies a function to all the elements
///  in another iterator_range.
///
///  IntRange is a template class for iterating over a range of
///  integers.
///
///  indices returns the range of indices from [0..size()) on a
///  subscriptable type.
///
///  Note that this is kept in Swift because it's really only useful in
///  C++11, and there aren't any major open-source subprojects of LLVM
///  that can use C++11 yet.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_RANGE_H
#define POLARPHP_BASIC_RANGE_H

#include <algorithm>
#include <type_traits>
#include <utility>

#include "llvm/ADT/iterator_range.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"

namespace polar::basic {

using llvm::make_range;
using llvm::iterator_range;

template<typename T>
inline auto reversed(T &&container)
-> decltype(llvm::make_range(container.rbegin(), container.rend()))
{
   return llvm::make_range(container.rbegin(), container.rend());
}

// Wrapper for std::transform that creates a new back-insertable container
// and transforms a range into it.
template<typename T, typename InputRange, typename MapFn>
T map(InputRange &&range, MapFn &&mapFn)
{
   T result;
   std::transform(std::begin(range), std::end(range),
                  std::back_inserter(result),
                  std::forward<MapFn>(mapFn));
   return result;
}

template <class T, bool IsEnum = std::is_enum<T>::value>
struct IntRangeTraits;

template <class T>
struct IntRangeTraits<T, /*is enum*/ false>
{
   static_assert(std::is_integral<T>::value,
                 "argument type of IntRange is either an integer nor an enum");
   using int_type = T;
   using difference_type = typename std::make_signed<int_type>::type;

   static T addOffset(T value, difference_type quantity)
   {
      return T(difference_type(value) + quantity);
   }

   static difference_type distance(T begin, T end)
   {
      return difference_type(end) - difference_type(begin);
   }
};

template <class T>
struct IntRangeTraits<T, /*is enum*/ true>
{
   using int_type = typename std::underlying_type<T>::type;
   using difference_type = typename std::make_signed<int_type>::type;

   static T addOffset(T value, difference_type quantity)
   {
      return T(difference_type(value) + quantity);
   }

   static difference_type distance(T begin, T end)
   {
      return difference_type(end) - difference_type(begin);
   }
};

/// A range of integers or enum values.  This type behaves roughly
/// like an ArrayRef.
template <class T = unsigned, class Traits = IntRangeTraits<T>>
class IntRange
{
   using int_type = typename Traits::int_type;
   using difference_type = typename Traits::difference_type;

public:
   IntRange() : m_begin(0), m_end(0) {}
   IntRange(T end) : m_begin(0), m_end(end) {}
   IntRange(T begin, T end) : m_begin(begin), m_end(end) {
      assert(begin <= end);
   }

   class iterator
   {
      friend class IntRange<T>;
      iterator(T value)
         : m_value(value) {}
   public:
      using value_type = T;
      using reference = T;
      using pointer = void;
      using difference_type = typename std::make_signed<T>::type;
      using iterator_category = std::random_access_iterator_tag;

      T operator*() const
      {
         return m_value;
      }

      iterator &operator++()
      {
         return *this += 1;
      }

      iterator operator++(int)
      {
         auto copy = *this;
         *this += 1;
         return copy;
      }

      iterator &operator--()
      {
         return *this -= 1;
      }

      iterator operator--(int) {
         auto copy = *this;
         *this -= 1;
         return copy;
      }

      bool operator==(iterator rhs)
      {
         return m_value == rhs.m_value;
      }

      bool operator!=(iterator rhs)
      {
         return m_value != rhs.m_value;
      }

      iterator &operator+=(difference_type i)
      {
         m_value = Traits::addOffset(m_value, i);
         return *this;
      }

      iterator operator+(difference_type i) const
      {
         return iterator(Traits::adddOfset(m_value, i));
      }

      friend iterator operator+(difference_type i, iterator base)
      {
         return iterator(Traits::addOffset(base.m_value, i));
      }

      iterator &operator-=(difference_type i)
      {
         m_value = Traits::addOffset(m_value, -i);
         return *this;
      }

      iterator operator-(difference_type i) const
      {
         return iterator(Traits::addOffset(m_value, -i));
      }

      difference_type operator-(iterator rhs) const
      {
         return Traits::distance(rhs.m_value, m_value);
      }

      T operator[](difference_type i) const
      {
         return Traits::addOffset(m_value, i);
      }

      bool operator<(iterator rhs) const
      {
         return m_value <  rhs.m_value;
      }

      bool operator<=(iterator rhs) const
      {
         return m_value <= rhs.m_value;
      }

      bool operator>(iterator rhs) const
      {
         return m_value >  rhs.m_value;
      }

      bool operator>=(iterator rhs) const
      {
         return m_value >= rhs.m_value;
      }

   private:
      T m_value;
   };

   iterator begin() const
   {
      return iterator(m_begin);
   }

   iterator end() const
   {
      return iterator(m_end);
   }

   std::reverse_iterator<iterator> rbegin() const
   {
      return std::reverse_iterator<iterator>(end());
   }

   std::reverse_iterator<iterator> rend() const
   {
      return std::reverse_iterator<iterator>(begin());
   }

   bool empty() const
   {
      return m_begin == m_end;
   }

   size_t size() const
   {
      return size_t(Traits::distance(m_begin, m_end));
   }

   T operator[](size_t i) const
   {
      assert(i < size());
      return Traits::addOffset(m_begin, i);
   }

   T front() const
   {
      assert(!empty()); return m_begin;
   }

   T back() const
   {
      assert(!empty());
      return Traits::addOffset(m_end, -1);
   }

   IntRange drop_back(size_t length = 1) const
   {
      assert(length <= size());
      return IntRange(m_begin, Traits::addOffset(m_end, -length));
   }

   IntRange slice(size_t start) const
   {
      assert(start <= size());
      return IntRange(Traits::addOffset(m_begin, start), m_end);
   }

   IntRange slice(size_t start, size_t length) const
   {
      assert(start <= size());
      auto newBegin = Traits::addOffset(m_begin, start);
      auto newSize = std::min(length, size_t(Traits::distance(newBegin, m_end)));
      return IntRange(newBegin, Traits::addOffset(newBegin, newSize));
   }

   bool operator==(IntRange other) const
   {
      return m_begin == other.m_begin && m_end == other.m_end;
   }

   bool operator!=(IntRange other) const
   {
      return !(operator==(other));
   }
private:
   T m_begin;
   T m_end;
};

/// indices - Given a type that's subscriptable with integers, return
/// an IntRange consisting of the valid subscripts.
template <class T>
typename std::enable_if<sizeof(std::declval<T>()[size_t(1)]) != 0,
IntRange<decltype(std::declval<T>().size())>>::type
indices(const T &collection)
{
   return IntRange<decltype(std::declval<T>().size())>(0, collection.size());
}

/// Returns an Int range [start, end).
static inline IntRange<unsigned> range(unsigned start, unsigned end)
{
   assert(start <= end && "Invalid integral range");
   return IntRange<unsigned>(start, end);
}

/// Returns an Int range [0, end).
static inline IntRange<unsigned> range(unsigned end)
{
   return range(0, end);
}

/// Returns a reverse Int range (start, end].
static inline auto reverse_range(unsigned start, unsigned end) ->
decltype(reversed(range(start+1, end+1)))
{
   assert(start <= end && "Invalid integral range");
   return reversed(range(start+1, end+1));
}

/// A random access range that provides iterators that can be used to iterate
/// over the (element, index) pairs of a collection.
template <typename IterTy> class EnumeratorRange
{
public:
   using IterTraitsTy = typename std::iterator_traits<IterTy>;
   static_assert(std::is_same<typename IterTraitsTy::iterator_category,
                 std::random_access_iterator_tag>::value,
                 "Expected a random access iterator");

   EnumeratorRange(IterTy begin, IterTy end) : m_begin(begin), m_end(end) {}

   class iterator
   {
      friend class EnumeratorRange;
      IterTy m_begin;
      IterTy m_iter;

      iterator(IterTy begin, IterTy iter)
         : m_begin(begin), m_iter(iter) {}

   public:
      using value_type =
      std::pair<typename std::iterator_traits<IterTy>::value_type, int>;
      using reference =
      std::pair<typename std::iterator_traits<IterTy>::value_type, int>;
      using pointer = void;
      using iterator_category = std::random_access_iterator_tag;
      using difference_type = int;

      value_type operator*() const
      {
         return {*m_iter, std::distance(m_begin, m_iter)};
      }

      iterator &operator++()
      {
         m_iter++;
         return *this;
      }

      iterator operator++(int)
      {
         return iterator(m_begin, m_iter++);
      }

      iterator &operator--()
      {
         m_iter--;
         return *this;
      }

      iterator operator--(int)
      {
         return iterator(m_begin, m_iter--);
      }

      bool operator==(iterator rhs)
      {
         return m_iter == rhs.m_iter;
      }

      bool operator!=(iterator rhs)
      {
         return !(*this == rhs);
      }

      iterator &operator+=(difference_type i)
      {
         std::advance(m_iter, i);
         return *this;
      }

      iterator operator+(difference_type i) const
      {
         auto IterCopy = m_iter;
         std::advance(IterCopy, i);
         return iterator(m_begin, IterCopy);
      }

      friend iterator operator+(difference_type i, iterator base)
      {
         std::advance(base.m_iter, i);
         return base;
      }

      iterator &operator-=(difference_type i)
      {
         *this += -i;
         return *this;
      }

      iterator operator-(difference_type i) const
      {
         auto newIter = *this;
         return newIter -= i;
      }

      difference_type operator-(iterator rhs) const
      {
         return difference_type(std::distance(m_iter, rhs.m_iter));
      }

   };

   iterator begin() const
   {
      return iterator(m_begin, m_begin);
   }

   iterator end() const
   {
      return iterator(m_begin, m_end);
   }

   using reverse_iterator = std::reverse_iterator<iterator>;
   reverse_iterator rbegin() const
   {
      return reverse_iterator(end());
   }

   reverse_iterator rend() const
   {
      return reverse_iterator(begin());
   }

private:
private:
   IterTy m_begin;
   IterTy m_end;
};

/// enumerate - Given a type that's subscriptable with integers, return an
/// IntEnumerateRange consisting of the valid subscripts.
template <class T>
EnumeratorRange<typename T::iterator> enumerate(T &collection)
{
   return EnumeratorRange<typename T::iterator>(collection.begin(),
                                                collection.end());
}

template <class T> EnumeratorRange<T> enumerate(T begin, T end)
{
   return EnumeratorRange<T>(begin, end);
}

} // polar::basic

#endif // POLARPHP_BASIC_RANGE_H
