// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/05/27.

#ifndef POLARPHP_BASIC_ADT_ITERATOR_RANGE_H
#define POLARPHP_BASIC_ADT_ITERATOR_RANGE_H

#include <iterator>
#include <utility>

namespace polar {
namespace basic {

/// \brief A range adaptor for a pair of iterators.
///
/// This just wraps two iterators into a range-compatible interface. Nothing
/// fancy at all.
template <typename IteratorType>
class IteratorRange
{
   IteratorType m_beginIterator;
   IteratorType m_endIterator;

public:
   //TODO: Add SFINAE to test that the Container's iterators match the range's
   //      iterators.
   template <typename Container>
   IteratorRange(Container &&c)
   //TODO: Consider ADL/non-member begin/end calls.
      : m_beginIterator(c.begin()),
        m_endIterator(c.end())
   {}

   IteratorRange(IteratorType beginIterator, IteratorType endIterator)
      : m_beginIterator(std::move(beginIterator)),
        m_endIterator(std::move(endIterator))
   {}

   IteratorType begin() const
   {
      return m_beginIterator;
   }

   IteratorType end() const
   {
      return m_endIterator;
   }
};

/// \brief Convenience function for iterating over sub-ranges.
///
/// This provides a bit of syntactic sugar to make using sub-ranges
/// in for loops a bit easier. Analogous to std::make_pair().
template <class T>
IteratorRange<T> make_range(T x, T y)
{
   return IteratorRange<T>(std::move(x), std::move(y));
}

template <typename T>
IteratorRange<T> make_range(std::pair<T, T> p)
{
   return IteratorRange<T>(std::move(p.first), std::move(p.second));
}

template<typename T>
IteratorRange<decltype(adl_begin(std::declval<T>()))> drop_begin(T &&t, int n)
{
   return make_range(std::next(adl_begin(t), n), adl_end(t));
}

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_ITERATOR_RANGE_H
