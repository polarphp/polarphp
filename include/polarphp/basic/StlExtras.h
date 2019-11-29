//===--- STLExtras.h - additions to the STL ---------------------*- C++ -*-===//
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
// Created by polarboy on 2019/11/28.
//===----------------------------------------------------------------------===//
//
/// \file Provides STL-style algorithms for convenience.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_STLEXTRAS_H
#define POLARPHP_BASIC_STLEXTRAS_H

#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Casting.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <numeric>
#include <type_traits>

namespace polar::basic {

//===----------------------------------------------------------------------===//
//                              Function Traits
//===----------------------------------------------------------------------===//

template <class T>
struct function_traits : function_traits<decltype(&T::operator())> {};

// function
template <class R, class... Args> struct function_traits<R(Args...)>
{
   using result_type = R;
   using argument_types = std::tuple<Args...>;
};

// function pointer
template <class R, class... Args> struct function_traits<R (*)(Args...)>
{
   using result_type = R;
   using argument_types = std::tuple<Args...>;
};

// std::function
template <class R, class... Args>
struct function_traits<std::function<R(Args...)>>
{
   using result_type = R;
   using argument_types = std::tuple<Args...>;
};

// pointer-to-member-function (i.e., operator()'s)
template <class T, class R, class... Args>
struct function_traits<R (T::*)(Args...)>
{
   using result_type = R;
   using argument_types = std::tuple<Args...>;
};

template <class T, class R, class... Args>
struct function_traits<R (T::*)(Args...) const>
{
   using result_type = R;
   using argument_types = std::tuple<Args...>;
};

/// @{

/// An STL-style algorithm similar to std::for_each that applies a second
/// functor between every pair of elements.
///
/// This provides the control flow logic to, for example, print a
/// comma-separated list:
/// \code
///   interleave(names.begin(), names.end(),
///              [&](StringRef name) { OS << name; },
///              [&] { OS << ", "; });
/// \endcode
template <typename ForwardIterator, typename UnaryFunctor,
          typename NullaryFunctor>
inline void interleave(ForwardIterator begin, ForwardIterator end,
                       UnaryFunctor each_fn,
                       NullaryFunctor between_fn)
{
   if (begin == end)
      return;
   each_fn(*begin);
   ++begin;
   for (; begin != end; ++begin) {
      between_fn();
      each_fn(*begin);
   }
}

template <typename Container, typename UnaryFunctor, typename NullaryFunctor>
inline void interleave(const Container &c, UnaryFunctor each_fn,
                       NullaryFunctor between_fn)
{
   interleave(c.begin(), c.end(), each_fn, between_fn);
}

/// @}
/// @{

/// The equivalent of std::for_each, but for two lists at once.
template <typename InputIt1, typename InputIt2, typename BinaryFunction>
inline void for_each(InputIt1 I1, InputIt1 E1, InputIt2 I2, BinaryFunction f)
{
   while (I1 != E1) {
      f(*I1, *I2);
      ++I1; ++I2;
   }
}

template <typename Container1, typename Container2, typename BinaryFunction>
inline void for_each(const Container1 &c1, const Container2 &c2,
                     BinaryFunction f)
{
   assert(c1.size() == c2.size());
   for_each(c1.begin(), c1.end(), c2.begin(), f);
}

/// The equivalent of std::for_each, but for three lists at once.
template <typename InputIt1, typename InputIt2, typename InputIt3,
          typename TernaryFunction>
inline void for_each3(InputIt1 I1, InputIt1 E1, InputIt2 I2, InputIt3 I3,
                      TernaryFunction f)
{
   while (I1 != E1) {
      f(*I1, *I2, *I3);
      ++I1; ++I2; ++I3;
   }
}

template <typename Container1, typename Container2, typename Container3,
          typename TernaryFunction>
inline void for_each3(const Container1 &c1, const Container2 &c2,
                      const Container3 &c3,
                      TernaryFunction f)
{
   assert(c1.size() == c2.size());
   assert(c2.size() == c3.size());
   for_each3(c1.begin(), c1.end(), c2.begin(), c3.begin(), f);
}

/// The equivalent of std::for_each, but visits the set union of two sorted
/// lists without allocating additional memory.
///
/// This has the following requirements:
///
/// 1. The ranges must be sorted.
/// 2. The elements must have the same type.
/// 3. There are no duplicate elements.
/// 4. All elements must be comparable with std::less.
template <typename InputIt1, typename InputIt2, typename BinaryFunction>
inline void set_union_for_each(InputIt1 I1, InputIt1 E1, InputIt2 I2,
                               InputIt2 E2, BinaryFunction f)
{
   static_assert(
            std::is_same<
            typename std::iterator_traits<InputIt1>::value_type,
            typename std::iterator_traits<InputIt2>::value_type
            >::value,
            "Expected both iterator types to have the same underlying value type");

   using RefTy = typename std::iterator_traits<InputIt1>::reference;

   while (true) {
      // If we have reached the end of either list, visit the rest of the other
      // list, We do not need to worry about duplicates since each array we know
      // is unique.
      if (I1 == E1) {
         std::for_each(I2, E2, f);
         return;
      }

      if (I2 == E2) {
         std::for_each(I1, E1, f);
         return;
      }

      // If I1 < I2, then visit I1 and continue.
      if (std::less<RefTy>()(*I1, *I2)) {
         f(*I1);
         ++I1;
         continue;
      }

      // If I2 < I1, visit I2 and continue.
      if (std::less<RefTy>()(*I2, *I1)) {
         f(*I2);
         ++I2;
         continue;
      }

      // Otherwise, we know that I1 and I2 equal. We know that we can only have
      // one of each element in each list, so we can just visit I1 and continue.
      f(*I1);
      ++I1;
      ++I2;
   }
}

/// A container adapter for set_union_for_each.
///
/// To see the requirements upon the containers, please see the iterator based
/// set_union_for_each.
template <typename Container1, typename Container2, typename UnaryFunction>
inline void set_union_for_each(const Container1 &C1, const Container2 &C2,
                               UnaryFunction f)
{
   // Make sure that our iterators have the same value type.
   static_assert(
            std::is_same<
            typename std::iterator_traits<
            typename Container1::iterator
            >::value_type,
            typename std::iterator_traits<
            typename Container2::iterator
            >::value_type
            >::value,
            "Expected both containers to have the same iterator value type");
   set_union_for_each(C1.begin(), C1.end(), C2.begin(), C2.end(), f);
}

/// If \p it is equal to \p end, then return \p defaultIter. Otherwise, return
/// std::next(\p it).
template <typename Iterator>
inline Iterator next_or_default(Iterator it, Iterator end,
                                Iterator defaultIter)
{
   return (it == end) ? defaultIter : std::next(it);
}

/// If \p it is equal to \p begin, then return \p defaultIter. Otherwise, return
/// std::prev(\p it).
template <typename Iterator>
inline Iterator prev_or_default(Iterator it, Iterator begin,
                                Iterator defaultIter)
{
   return (it == begin) ? defaultIter : std::prev(it);
}

/// Takes an iterator and an iterator pointing to the end of the iterator range.
/// If the iterator already points to the end of its range, simply return it,
/// otherwise return the next element.
template <typename Iterator>
inline Iterator next_or_end(Iterator it, Iterator end)
{
   return next_or_default(it, end, end);
}

template <typename Iterator>
inline Iterator prev_or_begin(Iterator it, Iterator begin)
{
   return prev_or_default(it, begin, begin);
}

/// @}

/// A range of iterators.
/// TODO: Add `llvm::iterator_range::empty()`, then remove this helper, along
/// with the superfluous FilterIterator and TransformIterator.
template<typename Iterator>
class IteratorRange {
   Iterator m_first;
   Iterator m_last;

public:
   using iterator = Iterator;

   IteratorRange(Iterator first, Iterator last)
      : m_first(first),
        m_last(last) {}

   iterator begin() const
   {
      return m_first;
   }

   iterator end() const
   {
      return m_last;
   }

   bool empty() const
   {
      return m_first == m_last;
   }

   typename std::iterator_traits<iterator>::value_type front() const {
      assert(!empty() && "Front of empty range");
      return *begin();
   }
};

/// Create a new iterator range.
template<typename Iterator>
inline IteratorRange<Iterator>
make_iterator_range(Iterator first, Iterator last)
{
   return IteratorRange<Iterator>(first, last);
}

/// An iterator that filters the results of an underlying forward
/// iterator, only passing through those values that satisfy a predicate.
///
/// \tparam Iterator the underlying iterator.
///
/// \tparam Predicate A predicate that determines whether a value of the
/// underlying iterator is available in the resulting sequence.
template<typename Iterator, typename Predicate>
class FilterIterator
{
   Iterator m_current;
   Iterator m_end;

   /// FIXME: Could optimize away this storage with EBCO tricks.
   Predicate m_pred;

   /// Skip any non-matching elements.
   void skipNonMatching()
   {
      while (m_current != m_end && !m_pred(*m_current)) {
         ++m_current;
      }
   }

public:
   /// Used to indicate when the current iterator has already been
   /// "primed", meaning that it's at the end or points to a value that
   /// satisfies the predicate.
   enum PrimedT { Primed };

   using iterator_category = std::forward_iterator_tag;
   using value_type = typename std::iterator_traits<Iterator>::value_type;
   using reference = typename std::iterator_traits<Iterator>::reference;
   using pointer = typename std::iterator_traits<Iterator>::pointer;
   using difference_type =
   typename std::iterator_traits<Iterator>::difference_type;

   /// Construct a new filtering iterator for the given iterator range
   /// and predicate.
   FilterIterator(Iterator current, Iterator end, Predicate pred)
      : m_current(current),
        m_end(end),
        m_pred(pred)
   {
      // Prime the iterator.
      skipNonMatching();
   }

   /// Construct a new filtering iterator for the given iterator range
   /// and predicate, where the iterator range has already been
   /// "primed" by ensuring that it is empty or the current iterator
   /// points to something that matches the predicate.
   FilterIterator(Iterator current, Iterator end, Predicate pred, PrimedT)
      : m_current(current),
        m_end(end),
        m_pred(pred)
   {
      // Assert that the iterators have already been primed.
      assert(m_current == m_end || m_pred(*m_current) && "Not primed!");
   }

   reference operator*() const
   {
      return *m_current;
   }

   pointer operator->() const
   {
      return m_current.operator->();
   }

   FilterIterator &operator++()
   {
      ++m_current;
      skipNonMatching();
      return *this;
   }

   FilterIterator operator++(int)
   {
      FilterIterator old = *this;
      ++*this;
      return old;
   }

   friend bool operator==(FilterIterator lhs, FilterIterator rhs)
   {
      return lhs.m_current == rhs.m_current;
   }

   friend bool operator!=(FilterIterator lhs, FilterIterator rhs)
   {
      return !(lhs == rhs);
   }
};

/// Create a new filter iterator.
template<typename Iterator, typename Predicate>
inline FilterIterator<Iterator, Predicate>
make_filter_iterator(Iterator current, Iterator end, Predicate pred)
{
   return FilterIterator<Iterator, Predicate>(current, end, pred);
}

/// A range filtered by a specific predicate.
template<typename Range, typename Predicate>
class FilterRange
{
   using Iterator = typename Range::iterator;

   Iterator m_first;
   Iterator m_last;
   Predicate m_pred;

public:
   using iterator = FilterIterator<Iterator, Predicate>;

   FilterRange(Range range, Predicate pred)
      : m_first(range.begin()),
        m_last(range.end()),
        m_pred(pred)
   {
      // Prime the sequence.
      while (m_first != m_last && !m_pred(*m_first)) {
         ++m_first;
      }
   }

   iterator begin() const
   {
      return iterator(m_first, m_last, m_pred, iterator::Primed);
   }

   iterator end() const
   {
      return iterator(m_last, m_last, m_pred, iterator::Primed);
   }

   bool empty() const
   {
      return m_first == m_last;
   }

   typename std::iterator_traits<iterator>::value_type front() const
   {
      assert(!empty() && "Front of empty range");
      return *begin();
   }
};

/// Create a new filter range.
template<typename Range, typename Predicate>
inline FilterRange<Range, Predicate>
make_filter_range(Range range, Predicate pred)
{
   return FilterRange<Range, Predicate>(range, pred);
}

/// An iterator that transforms the result of an underlying bidirectional
/// iterator with a given operation.
///
/// \tparam Iterator the underlying iterator.
///
/// \tparam Operation A function object that transforms the underlying
/// sequence's values into the new sequence's values.
template<typename Iterator, typename Operation>
class TransformIterator
{
   Iterator m_current;

   /// FIXME: Could optimize away this storage with EBCO tricks.
   Operation m_op;

   /// The underlying reference type, which will be passed to the
   /// operation.
   using OpTraits = function_traits<Operation>;

public:
   using iterator_category = std::bidirectional_iterator_tag;
   using value_type = typename OpTraits::result_type;
   using reference = value_type;
   using pointer = void; // FIXME: Should provide a pointer proxy.
   using difference_type =
   typename std::iterator_traits<Iterator>::difference_type;

   /// Construct a new transforming iterator for the given iterator
   /// and operation.
   TransformIterator(Iterator current, Operation op)
      : m_current(current), m_op(op)
   {}

   reference operator*() const
   {
      return m_op(*m_current);
   }

   TransformIterator &operator++()
   {
      ++m_current;
      return *this;
   }

   TransformIterator operator++(int)
   {
      TransformIterator old = *this;
      ++*this;
      return old;
   }

   TransformIterator &operator--()
   {
      --m_current;
      return *this;
   }

   TransformIterator operator--(int)
   {
      TransformIterator old = *this;
      --*this;
      return old;
   }

   friend bool operator==(TransformIterator lhs, TransformIterator rhs)
   {
      return lhs.m_current == rhs.m_current;
   }

   friend bool operator!=(TransformIterator lhs, TransformIterator rhs)
   {
      return !(lhs == rhs);
   }
};

/// Create a new transform iterator.
template<typename Iterator, typename Operation>
inline TransformIterator<Iterator, Operation>
make_transform_iterator(Iterator current, Operation op)
{
   return TransformIterator<Iterator, Operation>(current, op);
}

/// A range transformed by a specific predicate.
template<typename Range, typename Operation>
class TransformRange
{
   Range m_range;
   Operation m_op;

public:
   using iterator = TransformIterator<typename Range::iterator, Operation>;

   TransformRange(Range range, Operation op)
      : m_range(range), m_op(op) { }

   iterator begin() const
   {
      return iterator(m_range.begin(), m_op);
   }

   iterator end() const
   {
      return iterator(m_range.end(), m_op);

   }
   bool empty() const
   {
      return begin() == end();
   }

   typename std::iterator_traits<iterator>::value_type front() const
   {
      assert(!empty() && "Front of empty range");
      return *begin();
   }
};

/// Create a new transform range.
template<typename Range, typename Operation>
inline TransformRange<Range, Operation>
make_transform_range(Range range, Operation op)
{
   return TransformRange<Range, Operation>(range, op);
}

/// An iterator that filters and transforms the results of an
/// underlying forward iterator based on a transformation from the underlying
/// value type to an optional result type.
///
/// \tparam Iterator the underlying iterator.
///
/// \tparam OptionalTransform A function object that maps a value of
/// the underlying iterator type to an optional containing a value of
/// the resulting sequence, or an empty optional if this item should
/// be skipped.
template<typename Iterator, typename OptionalTransform>
class OptionalTransformIterator
{
   Iterator m_current;
   Iterator m_end;

   /// FIXME: Could optimize away this storage with EBCO tricks.
   OptionalTransform m_op;

   /// Skip any non-matching elements.
   void skipNonMatching()
   {
      while (m_current != m_end && !m_op(*m_current)) {
         ++m_current;
      }
   }

   using UnderlyingReference =
   typename std::iterator_traits<Iterator>::reference;

   using ResultReference =
   typename std::result_of<OptionalTransform(UnderlyingReference)>::type;

public:
   /// Used to indicate when the current iterator has already been
   /// "primed", meaning that it's at the end or points to a value that
   /// satisfies the transform.
   enum PrimedT { Primed };

   using iterator_category = std::forward_iterator_tag;
   using reference = typename ResultReference::value_type;
   using value_type = typename ResultReference::value_type;
   using pointer = void; // FIXME: should add a proxy here.
   using difference_type =
   typename std::iterator_traits<Iterator>::difference_type;

   /// Construct a new optional transform iterator for the given
   /// iterator range and operation.
   OptionalTransformIterator(Iterator current, Iterator end,
                             OptionalTransform op)
      : m_current(current), m_end(end), m_op(op)
   {
      // Prime the iterator.
      skipNonMatching();
   }

   /// Construct a new optional transform iterator for the given iterator range
   /// and operation, where the iterator range has already been
   /// "primed" by ensuring that it is empty or the current iterator
   /// points to something that matches the operation.
   OptionalTransformIterator(Iterator current, Iterator end,
                             OptionalTransform op, PrimedT)
      : m_current(current),
        m_end(end),
        m_op(op)
   {
      // Assert that the iterators have already been primed.
      assert((m_current == m_end || m_op(*m_current)) && "Not primed!");
   }

   reference operator*() const
   {
      return *m_op(*m_current);
   }

   OptionalTransformIterator &operator++()
   {
      ++m_current;
      skipNonMatching();
      return *this;
   }

   OptionalTransformIterator operator++(int)
   {
      OptionalTransformIterator old = *this;
      ++*this;
      return old;
   }

   friend bool operator==(OptionalTransformIterator lhs,
                          OptionalTransformIterator rhs)
   {
      return lhs.m_current == rhs.m_current;
   }

   friend bool operator!=(OptionalTransformIterator lhs,
                          OptionalTransformIterator rhs)
   {
      return !(lhs == rhs);
   }
};

/// Create a new filter iterator.
template<typename Iterator, typename OptionalTransform>
inline OptionalTransformIterator<Iterator, OptionalTransform>
make_optional_transform_iterator(Iterator current, Iterator end,
                                 OptionalTransform op)
{
   return OptionalTransformIterator<Iterator, OptionalTransform>(current, end,
                                                                 op);
}

/// A range filtered and transformed by the optional transform.
template <typename Range, typename OptionalTransform,
          typename Iterator = typename Range::iterator>
class OptionalTransformRange
{
public:
   using iterator = OptionalTransformIterator<Iterator, OptionalTransform>;

   OptionalTransformRange(Range range, OptionalTransform op)
      : m_first(range.begin()),
        m_last(range.end()), m_op(op)
   {
      // Prime the sequence.
      while (m_first != m_last && !m_op(*m_first)) {
         ++m_first;
      }
   }

   iterator begin() const
   {
      return iterator(m_first, m_last, m_op, iterator::Primed);
   }

   iterator end() const
   {
      return iterator(m_last, m_last, m_op, iterator::Primed);
   }

   bool empty() const
   {
      return m_first == m_last;
   }

   typename std::iterator_traits<iterator>::value_type front() const
   {
      assert(!empty() && "Front of empty range");
      return *begin();
   }
private:
   Iterator m_first;
   Iterator m_last;
   OptionalTransform m_op;
};

/// Create a new filter range.
template<typename Range, typename OptionalTransform>
inline OptionalTransformRange<Range, OptionalTransform>
make_optional_transform_range(Range range, OptionalTransform op)
{
   return OptionalTransformRange<Range, OptionalTransform>(range, op);
}

/// Function object that attempts a downcast to a subclass, wrapping
/// the result in an optional to indicate success or failure.
template<typename Subclass>
struct DowncastAsOptional
{
   template<typename Superclass>
   auto operator()(Superclass &value) const
   -> Optional<decltype(llvm::cast<Subclass>(value))>
   {
      if (auto result = llvm::dyn_cast<Subclass>(value)) {
         return result;
      }
      return None;
   }

   template<typename Superclass>
   auto operator()(const Superclass &value) const
   -> Optional<decltype(llvm::cast<Subclass>(value))>
   {
      if (auto result = llvm::dyn_cast<Subclass>(value)) {
         return result;
      }
      return None;
   }
};

template<typename Subclass, typename Iterator>
using DowncastFilterIterator
= OptionalTransformIterator<Iterator, DowncastAsOptional<Subclass>>;

template<typename Subclass, typename Iterator>
inline DowncastFilterIterator<Subclass, Iterator>
make_downcast_filter_iterator(Iterator current, Iterator end)
{
   DowncastAsOptional<Subclass> op;
   return DowncastFilterIterator<Subclass, Iterator>(current, end, op);
}

template<typename Subclass, typename Range>
class DowncastFilterRange
      : public OptionalTransformRange<Range, DowncastAsOptional<Subclass>>
{

   using Inherited = OptionalTransformRange<Range, DowncastAsOptional<Subclass>>;

public:
   DowncastFilterRange(Range range)
      : Inherited(range, DowncastAsOptional<Subclass>())
   {}
};

template<typename Subclass, typename Range>
DowncastFilterRange<Subclass, Range>
make_downcast_filter_range(Range range)
{
   return DowncastFilterRange<Subclass, Range>(range);
}

/// Sorts and then uniques a container with random access iterators and an erase
/// method that removes a range specified by random access iterators.
template <typename Container>
void sort_unique(
      Container &C,
      typename std::enable_if<
      std::is_same<typename std::iterator_traits<
      typename Container::iterator>::iterator_category,
      std::random_access_iterator_tag>::value,
      void>::type * = nullptr)
{
   std::sort(C.begin(), C.end());
   C.erase(std::unique(C.begin(), C.end()), C.end());
}

/// Sorts and then uniques a container with random access iterators and an erase
/// method that removes a range specified by random access iterators.
template <typename Container, typename Comparator>
void sort_unique(
      Container &C,
      Comparator Cmp,
      typename std::enable_if<
      std::is_same<typename std::iterator_traits<
      typename Container::iterator>::iterator_category,
      std::random_access_iterator_tag>::value,
      void>::type * = nullptr)
{
   std::sort(C.begin(), C.end(), Cmp);
   C.erase(std::unique(C.begin(), C.end()), C.end());
}

/// Returns true if [II, IE) is a sorted and uniqued array. Returns false
/// otherwise.
template <typename IterTy>
inline bool is_sorted_and_uniqued(IterTy II, IterTy IE)
{
   using RefTy = typename std::iterator_traits<IterTy>::reference;

   // The empty list is always sorted and uniqued.
   if (II == IE) {
      return true;
   }

   // The list of one element is always sorted and uniqued.
   auto lastI = II;
   ++II;
   if (II == IE) {
      return true;
   }
   // Otherwise, until we reach the end of the list...
   while (II != IE) {
      // If lastI is greater than II then we know that our array is not sorted. If
      // lastI equals II, then we know that our array is not unique. If both of
      // those are conditions are false, then visit the next iterator element.
      if (std::greater_equal<RefTy>()(*lastI, *II)) {
         // Return false otherwise.
         return false;
      }
      lastI = II;
      ++II;
   }

   // Success!
   return true;
}

template <typename Container>
inline bool is_sorted_and_uniqued(const Container &C)
{
   return is_sorted_and_uniqued(C.begin(), C.end());
}

template <typename Container, typename OutputIterator>
inline void copy(const Container &C, OutputIterator iter)
{
   std::copy(C.begin(), C.end(), iter);
}

template <typename Container, typename OutputIterator, typename Predicate>
inline void copy_if(const Container &C, OutputIterator result, Predicate pred)
{
   std::copy_if(C.begin(), C.end(), result, pred);
}

template <typename Container, typename OutputIterator, typename UnaryOperation>
inline OutputIterator transform(const Container &C, OutputIterator result,
                                UnaryOperation op)
{
   return std::transform(C.begin(), C.end(), result, op);
}

template <typename Container, typename T, typename BinaryOperation>
inline T accumulate(const Container &C, T init, BinaryOperation op)
{
   return std::accumulate(C.begin(), C.end(), init, op);
}

/// Returns true if the range defined by \p mainBegin ..< \p mainEnd starts with
/// the same elements as the range defined by \p prefixBegin ..< \p prefixEnd.
///
/// This includes cases where the prefix range is empty, as well as when the two
/// ranges are the same length and contain the same elements.
template <typename MainInputIterator, typename PrefixInputIterator>
inline bool hasPrefix(MainInputIterator mainBegin,
                      const MainInputIterator mainEnd,
                      PrefixInputIterator prefixBegin,
                      const PrefixInputIterator prefixEnd)
{
   while (prefixBegin != prefixEnd) {
      // If "main" is shorter than "prefix", it does not start with "prefix".
      if (mainBegin == mainEnd) {
         return false;
      }
      // If there's a mismatch, "main" does not start with "prefix".
      if (*mainBegin != *prefixBegin) {
         return false;
      }
      ++prefixBegin;
      ++mainBegin;
   }
   // If we checked every element of "prefix", "main" does start with "prefix".
   return true;
}

/// Provides default implementations of !=, <=, >, and >= based on == and <.
template <typename T>
class RelationalOperationsBase
{
public:
   friend bool operator>(const T &left, const T &right)
   {
      return right < left;
   }

   friend bool operator>=(const T &left, const T &right)
   {
      return !(left < right);
   }

   friend bool operator<=(const T &left, const T &right)
   {
      return !(right < left);
   }

   friend bool operator!=(const T &left, const T &right)
   {
      return !(left == right);
   }
};

} // polar::basic

#endif // POLARPHP_BASIC_STLEXTRAS_H
