// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/26.

#ifndef POLARPHP_BASIC_ADT_SEQUENCE_H
#define POLARPHP_BASIC_ADT_SEQUENCE_H

#include "polarphp/basic/adt/Iterator.h"
#include "polarphp/basic/adt/IteratorRange.h"
#include <algorithm>
#include <iterator>
#include <utility>

namespace polar {
namespace basic {

namespace internal {

template <typename ValueT>
class ValueSequenceIterator
      : public IteratorFacadeBase<ValueSequenceIterator<ValueT>,
      std::random_access_iterator_tag,
      const ValueT>
{
   using BaseT = typename ValueSequenceIterator::IteratorFacadeBase;

   ValueT m_value;

public:
   using difference_type = typename BaseT::difference_type;
   using reference = typename BaseT::reference;

   ValueSequenceIterator() = default;
   ValueSequenceIterator(const ValueSequenceIterator &) = default;
   ValueSequenceIterator(ValueSequenceIterator &&arg)
      : m_value(std::move(arg.m_value)) {}

   template <typename U, typename Enabler = decltype(ValueT(std::declval<U>()))>
   ValueSequenceIterator(U &&value) : m_value(std::forward<U>(value))
   {}

   ValueSequenceIterator &operator+=(difference_type N)
   {
      m_value += N;
      return *this;
   }

   ValueSequenceIterator &operator-=(difference_type N)
   {
      m_value -= N;
      return *this;
   }

   using BaseT::operator-;

   difference_type operator-(const ValueSequenceIterator &other) const
   {
      return m_value - other.m_value;
   }

   bool operator==(const ValueSequenceIterator &other) const
   {
      return m_value == other.m_value;
   }
   bool operator<(const ValueSequenceIterator &other) const
   {
      return m_value < other.m_value;
   }

   reference operator*() const
   {
      return m_value;
   }
};

} // end namespace internal

template <typename ValueT>
IteratorRange<internal::ValueSequenceIterator<ValueT>> seq(ValueT begin,
                                                           ValueT end)
{
   return make_range(internal::ValueSequenceIterator<ValueT>(begin),
                     internal::ValueSequenceIterator<ValueT>(end));
}

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_SEQUENCE_H
