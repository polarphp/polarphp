// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/05/30.

#ifndef POLARPHP_BASIC_ADT_ITERATOR_H
#define POLARPHP_BASIC_ADT_ITERATOR_H

#include "polarphp/basic/adt/IteratorRange.h"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace polar {
namespace basic {

/// CRTP base class which implements the entire standard iterator facade
/// in terms of a minimal subset of the interface.
///
/// Use this when it is reasonable to implement most of the iterator
/// functionality in terms of a core subset. If you need special behavior or
/// there are performance implications for this, you may want to override the
/// relevant members instead.
///
/// Note, one abstraction that this does *not* provide is implementing
/// subtraction in terms of addition by negating the difference. Negation isn't
/// always information preserving, and I can see very reasonable iterator
/// designs where this doesn't work well. It doesn't really force much added
/// boilerplate anyways.
///
/// Another abstraction that this doesn't provide is implementing increment in
/// terms of addition of one. These aren't equivalent for all iterator
/// categories, and respecting that adds a lot of complexity for little gain.
///
/// Classes wishing to use `IteratorFacadeBase` should implement the following
/// methods:
///
/// Forward Iterators:
///   (All of the following methods)
///   - DerivedT &operator=(const DerivedT &R);
///   - bool operator==(const DerivedT &R) const;
///   - const T &operator*() const;
///   - T &operator*();
///   - DerivedT &operator++();
///
/// Bidirectional Iterators:
///   (All methods of forward iterators, plus the following)
///   - DerivedT &operator--();
///
/// Random-access Iterators:
///   (All methods of bidirectional iterators excluding the following)
///   - DerivedT &operator++();
///   - DerivedT &operator--();
///   (and plus the following)
///   - bool operator<(const DerivedT &RHS) const;
///   - DifferenceTypeT operator-(const DerivedT &R) const;
///   - DerivedT &operator+=(DifferenceTypeT N);
///   - DerivedT &operator-=(DifferenceTypeT N);
///
template <typename DerivedT, typename IteratorCategoryT, typename T,
          typename DifferenceTypeT = std::ptrdiff_t, typename PointerT = T *,
          typename ReferenceT = T &>
class IteratorFacadeBase
      : public std::iterator<IteratorCategoryT, T, DifferenceTypeT, PointerT,
      ReferenceT>
{
protected:
   enum {
      IsRandomAccess = std::is_base_of<std::random_access_iterator_tag,
      IteratorCategoryT>::value,
      IsBidirectional = std::is_base_of<std::bidirectional_iterator_tag,
      IteratorCategoryT>::value,
   };

   /// A proxy object for computing a reference via indirecting a copy of an
   /// iterator. This is used in APIs which need to produce a reference via
   /// indirection but for which the iterator object might be a temporary. The
   /// proxy preserves the iterator internally and exposes the indirected
   /// reference via a conversion operator.
   class ReferenceProxy
   {
      friend class IteratorFacadeBase;

      DerivedT m_iter;

      ReferenceProxy(DerivedT iter)
         : m_iter(std::move(iter))
      {}

   public:
      operator ReferenceT() const
      {
         return *m_iter;
      }
   };

public:
   DerivedT operator+(DifferenceTypeT n) const
   {
      static_assert(std::is_base_of<IteratorFacadeBase, DerivedT>::value,
                    "Must pass the derived type to this template!");
      static_assert(
               IsRandomAccess,
               "The '+' operator is only defined for random access iterators.");
      DerivedT tmp = *static_cast<const DerivedT *>(this);
      tmp += n;
      return tmp;
   }

   friend DerivedT operator+(DifferenceTypeT n, const DerivedT &i)
   {
      static_assert(
               IsRandomAccess,
               "The '+' operator is only defined for random access iterators.");
      return i + n;
   }

   DerivedT operator-(DifferenceTypeT n) const
   {
      static_assert(
               IsRandomAccess,
               "The '-' operator is only defined for random access iterators.");
      DerivedT tmp = *static_cast<const DerivedT *>(this);
      tmp -= n;
      return tmp;
   }

   DerivedT &operator++()
   {
      static_assert(std::is_base_of<IteratorFacadeBase, DerivedT>::value,
                    "Must pass the derived type to this template!");
      return static_cast<DerivedT *>(this)->operator+=(1);
   }

   DerivedT operator++(int)
   {
      DerivedT tmp = *static_cast<DerivedT *>(this);
      ++*static_cast<DerivedT *>(this);
      return tmp;
   }

   DerivedT &operator--()
   {
      static_assert(
               IsBidirectional,
               "The decrement operator is only defined for bidirectional iterators.");
      return static_cast<DerivedT *>(this)->operator-=(1);
   }

   DerivedT operator--(int)
   {
      static_assert(
               IsBidirectional,
               "The decrement operator is only defined for bidirectional iterators.");
      DerivedT tmp = *static_cast<DerivedT *>(this);
      --*static_cast<DerivedT *>(this);
      return tmp;
   }

   bool operator!=(const DerivedT &rhs) const
   {
      return !static_cast<const DerivedT *>(this)->operator==(rhs);
   }

   bool operator>(const DerivedT &rhs) const
   {
      static_assert(
               IsRandomAccess,
               "Relational operators are only defined for random access iterators.");
      return !static_cast<const DerivedT *>(this)->operator<(rhs) &&
            !static_cast<const DerivedT *>(this)->operator==(rhs);
   }

   bool operator<=(const DerivedT &rhs) const
   {
      static_assert(
               IsRandomAccess,
               "Relational operators are only defined for random access iterators.");
      return !static_cast<const DerivedT *>(this)->operator>(rhs);
   }

   bool operator>=(const DerivedT &rhs) const
   {
      static_assert(
               IsRandomAccess,
               "Relational operators are only defined for random access iterators.");
      return !static_cast<const DerivedT *>(this)->operator<(rhs);
   }

   PointerT operator->()
   {
      return &static_cast<DerivedT *>(this)->operator*();
   }

   PointerT operator->() const
   {
      return &static_cast<const DerivedT *>(this)->operator*();
   }

   ReferenceProxy operator[](DifferenceTypeT n)
   {
      static_assert(IsRandomAccess,
                    "Subscripting is only defined for random access iterators.");
      return ReferenceProxy(static_cast<DerivedT *>(this)->operator+(n));
   }

   ReferenceProxy operator[](DifferenceTypeT n) const
   {
      static_assert(IsRandomAccess,
                    "Subscripting is only defined for random access iterators.");
      return ReferenceProxy(static_cast<const DerivedT *>(this)->operator+(n));
   }
};

/// CRTP base class for adapting an iterator to a different type.
///
/// This class can be used through CRTP to adapt one iterator into another.
/// Typically this is done through providing in the derived class a custom \c
/// operator* implementation. Other methods can be overridden as well.
template <
      typename DerivedT, typename WrappedIteratorT,
      typename IteratorCategoryT =
      typename std::iterator_traits<WrappedIteratorT>::iterator_category,
      typename T = typename std::iterator_traits<WrappedIteratorT>::value_type,
      typename DifferenceTypeT =
      typename std::iterator_traits<WrappedIteratorT>::difference_type,
      typename PointerT = typename std::conditional<
         std::is_same<T, typename std::iterator_traits<
                         WrappedIteratorT>::value_type>::value,
         typename std::iterator_traits<WrappedIteratorT>::pointer, T *>::type,
      typename ReferenceT = typename std::conditional<
         std::is_same<T, typename std::iterator_traits<
                         WrappedIteratorT>::value_type>::value,
         typename std::iterator_traits<WrappedIteratorT>::reference, T &>::type>
class IteratorAdaptorBase
      : public IteratorFacadeBase<DerivedT, IteratorCategoryT, T,
      DifferenceTypeT, PointerT, ReferenceT>
{
   using BaseT = typename IteratorAdaptorBase::IteratorFacadeBase;

protected:
   WrappedIteratorT m_iter;

   IteratorAdaptorBase() = default;

   explicit IteratorAdaptorBase(WrappedIteratorT u)
      : m_iter(std::move(u))
   {
      static_assert(std::is_base_of<IteratorAdaptorBase, DerivedT>::value,
                    "Must pass the derived type to this template!");
   }

   const WrappedIteratorT &getWrapped() const
   {
      return m_iter;
   }

public:
   using difference_type = DifferenceTypeT;

   DerivedT &operator+=(difference_type n)
   {
      static_assert(
               BaseT::IsRandomAccess,
               "The '+=' operator is only defined for random access iterators.");
      m_iter += n;
      return *static_cast<DerivedT *>(this);
   }

   DerivedT &operator-=(difference_type n)
   {
      static_assert(
               BaseT::IsRandomAccess,
               "The '-=' operator is only defined for random access iterators.");
      m_iter -= n;
      return *static_cast<DerivedT *>(this);
   }

   using BaseT::operator-;
   difference_type operator-(const DerivedT &rhs) const {
      static_assert(
               BaseT::IsRandomAccess,
               "The '-' operator is only defined for random access iterators.");
      return m_iter - rhs.m_iter;
   }

   // We have to explicitly provide ++ and -- rather than letting the facade
   // forward to += because WrappedIteratorT might not support +=.
   using BaseT::operator++;
   DerivedT &operator++()
   {
      ++m_iter;
      return *static_cast<DerivedT *>(this);
   }

   using BaseT::operator--;
   DerivedT &operator--()
   {
      static_assert(
               BaseT::IsBidirectional,
               "The decrement operator is only defined for bidirectional iterators.");
      --m_iter;
      return *static_cast<DerivedT *>(this);
   }

   bool operator==(const DerivedT &rhs) const
   {
      return m_iter == rhs.m_iter;
   }

   bool operator<(const DerivedT &rhs) const
   {
      static_assert(
               BaseT::IsRandomAccess,
               "Relational operators are only defined for random access iterators.");
      return m_iter < rhs.m_iter;
   }

   ReferenceT operator*() const
   {
      return *m_iter;
   }
};

/// An iterator type that allows iterating over the pointees via some
/// other iterator.
///
/// The typical usage of this is to expose a type that iterates over Ts, but
/// which is implemented with some iterator over T*s:
///
/// \code
///   using iterator = PointeeIterator<SmallVectorImpl<T *>::iterator>;
/// \endcode
template <typename WrappedIteratorT,
          typename T = typename std::remove_reference<
             decltype(**std::declval<WrappedIteratorT>())>::type>
struct PointeeIterator
      : IteratorAdaptorBase<
      PointeeIterator<WrappedIteratorT, T>, WrappedIteratorT,
      typename std::iterator_traits<WrappedIteratorT>::iterator_category,
      T> {
   PointeeIterator() = default;
   template <typename U>
   PointeeIterator(U &&u)
      : PointeeIterator::IteratorAdaptorBase(std::forward<U &&>(u))
   {}

   T &operator*() const
   {
      return **this->m_iter;
   }
};

template <typename RangeType, typename WrappedIteratorT =
          decltype(std::begin(std::declval<RangeType>()))>
IteratorRange<PointeeIterator<WrappedIteratorT>>
make_pointee_range(RangeType &&range)
{
   using PointeeIteratorT = PointeeIterator<WrappedIteratorT>;
   return make_range(PointeeIteratorT(std::begin(std::forward<RangeType>(range))),
                     PointeeIteratorT(std::end(std::forward<RangeType>(range))));
}

template <typename WrappedIteratorT,
          typename T = decltype(&*std::declval<WrappedIteratorT>())>
class PointerIterator
      : public IteratorAdaptorBase<
      PointerIterator<WrappedIteratorT, T>, WrappedIteratorT,
      typename std::iterator_traits<WrappedIteratorT>::iterator_category,
      T>
{
   mutable T m_ptr;

public:
   PointerIterator() = default;

   explicit PointerIterator(WrappedIteratorT u)
      : PointerIterator::IteratorAdaptorBase(std::move(u))
   {}

   T &operator*()
   {
      return m_ptr = &*this->m_iter;
   }

   const T &operator*() const
   {
      return m_ptr = &*this->m_iter;
   }
};

template <typename RangeType, typename WrappedIteratorT =
          decltype(std::begin(std::declval<RangeType>()))>
IteratorRange<PointerIterator<WrappedIteratorT>>
make_pointer_range(RangeType &&range)
{
   using PointerIteratorT = PointerIterator<WrappedIteratorT>;
   return make_range(PointerIteratorT(std::begin(std::forward<RangeType>(range))),
                     PointerIteratorT(std::end(std::forward<RangeType>(range))));
}

// Wrapper iterator over iterator ItType, adding DataRef to the type of ItType,
// to create NodeRef = std::pair<InnerTypeOfItType, DataRef>.
template <typename ItType, typename NodeRef, typename DataRef>
class WrappedPairNodeDataIterator
      : public IteratorAdaptorBase<
      WrappedPairNodeDataIterator<ItType, NodeRef, DataRef>, ItType,
      typename std::iterator_traits<ItType>::iterator_category, NodeRef,
      std::ptrdiff_t, NodeRef *, NodeRef &>
{
   using BaseT = IteratorAdaptorBase<
   WrappedPairNodeDataIterator, ItType,
   typename std::iterator_traits<ItType>::iterator_category, NodeRef,
   std::ptrdiff_t, NodeRef *, NodeRef &>;

   const DataRef m_dataRef;
   mutable NodeRef m_nodeRef;

public:
   WrappedPairNodeDataIterator(ItType begin, const DataRef dataRef)
      : BaseT(begin), m_dataRef(dataRef)
   {
      m_nodeRef.first = m_dataRef;
   }

   NodeRef &operator*() const
   {
      m_nodeRef.second = *this->m_iter;
      return m_nodeRef;
   }
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_ITERATOR_H
