// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/05.

#ifndef POLARPHP_BASIC_ADT_INTRUSIVE_LIST_ITERATOR_H
#define POLARPHP_BASIC_ADT_INTRUSIVE_LIST_ITERATOR_H

#include "polarphp/basic/adt/IntrusiveListNode.h"

#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace polar {
namespace basic {

namespace ilist_internal {

/// Find const-correct node types.
template <typename OptionsType, bool IsConst>
struct IteratorTraits;

template <typename OptionsType>
struct IteratorTraits<OptionsType, false>
{
   using value_type = typename OptionsType::value_type;
   using pointer = typename OptionsType::pointer;
   using reference = typename OptionsType::reference;
   using node_pointer = IntrusiveListNodeImpl<OptionsType> *;
   using node_reference = IntrusiveListNodeImpl<OptionsType> &;
};

template <typename OptionsType>
struct IteratorTraits<OptionsType, true> {
   using value_type = const typename OptionsType::value_type;
   using pointer = typename OptionsType::const_pointer;
   using reference = typename OptionsType::const_reference;
   using node_pointer = const IntrusiveListNodeImpl<OptionsType> *;
   using node_reference = const IntrusiveListNodeImpl<OptionsType> &;
};

template <bool IsReverse>
struct IteratorHelper;

template <>
struct IteratorHelper<false> : ilist_internal::NodeAccess
{
   using Access = ilist_internal::NodeAccess;
   template <typename T> static void increment(T *&iter)
   {
      iter = Access::getNext(*iter);
   }

   template <typename T> static void decrement(T *&iter)
   {
      iter = Access::getPrev(*iter);
   }
};

template <>
struct IteratorHelper<true> : ilist_internal::NodeAccess
{
   using Access = ilist_internal::NodeAccess;

   template <typename T> static void increment(T *&iter)
   {
      iter = Access::getPrev(*iter);
   }

   template <typename T> static void decrement(T *&iter)
   {
      iter = Access::getNext(*iter);
   }
};

} // end namespace ilist_internal

/// Iterator for intrusive lists  based on ilist_node.
template <typename OptionsType, bool IsReverse, bool IsConst>
class IntrusiveListIterator : ilist_internal::SpecificNodeAccess<OptionsType>
{
   friend class IntrusiveListIterator<OptionsType, IsReverse, !IsConst>;
   friend class IntrusiveListIterator<OptionsType, !IsReverse, IsConst>;
   friend class IntrusiveListIterator<OptionsType, !IsReverse, !IsConst>;

   using Traits = ilist_internal::IteratorTraits<OptionsType, IsConst>;
   using Access = ilist_internal::SpecificNodeAccess<OptionsType>;

public:
   using value_type = typename Traits::value_type;
   using pointer = typename Traits::pointer;
   using reference = typename Traits::reference;
   using difference_type = ptrdiff_t;
   using iterator_category = std::bidirectional_iterator_tag;
   using const_pointer = typename OptionsType::const_pointer;
   using const_reference = typename OptionsType::const_reference;

private:
   using node_pointer = typename Traits::node_pointer;
   using node_reference = typename Traits::node_reference;

   node_pointer m_nodePtr = nullptr;

public:
   /// Create from an ilist_node.
   explicit IntrusiveListIterator(node_reference node)
      : m_nodePtr(&node)
   {}

   explicit IntrusiveListIterator(pointer nodePtr)
      : m_nodePtr(Access::getNodePtr(nodePtr))
   {}

   explicit IntrusiveListIterator(reference nodeRef)
      : m_nodePtr(Access::getNodePtr(&nodeRef))
   {}

   IntrusiveListIterator() = default;

   // This is templated so that we can allow constructing a const iterator from
   // a nonconst iterator...
   template <bool RHSIsConst>
   IntrusiveListIterator(
         const IntrusiveListIterator<OptionsType, IsReverse, RHSIsConst> &rhs,
         typename std::enable_if<IsConst || !RHSIsConst, void *>::type = nullptr)
      : m_nodePtr(rhs.m_nodePtr)
   {}

   // This is templated so that we can allow assigning to a const iterator from
   // a nonconst iterator...
   template <bool RHSIsConst>
   typename std::enable_if<IsConst || !RHSIsConst, IntrusiveListIterator &>::type
   operator=(const IntrusiveListIterator<OptionsType, IsReverse, RHSIsConst> &rhs)
   {
      m_nodePtr = rhs.m_nodePtr;
      return *this;
   }

   /// Explicit conversion between forward/reverse iterators.
   ///
   /// Translate between forward and reverse iterators without changing range
   /// boundaries.  The resulting iterator will dereference (and have a handle)
   /// to the previous node, which is somewhat unexpected; but converting the
   /// two endpoints in a range will give the same range in reverse.
   ///
   /// This matches std::reverse_iterator conversions.
   explicit IntrusiveListIterator(
         const IntrusiveListIterator<OptionsType, !IsReverse, IsConst> &rhs)
      : IntrusiveListIterator(++rhs.getReverse())
   {}

   /// Get a reverse iterator to the same node.
   ///
   /// Gives a reverse iterator that will dereference (and have a handle) to the
   /// same node.  Converting the endpoint iterators in a range will give a
   /// different range; for range operations, use the explicit conversions.
   IntrusiveListIterator<OptionsType, !IsReverse, IsConst> getReverse() const
   {
      if (m_nodePtr) {
         return IntrusiveListIterator<OptionsType, !IsReverse, IsConst>(*m_nodePtr);
      }
      return IntrusiveListIterator<OptionsType, !IsReverse, IsConst>();
   }

   /// Const-cast.
   IntrusiveListIterator<OptionsType, IsReverse, false> getNonConst() const
   {
      if (m_nodePtr) {
         return IntrusiveListIterator<OptionsType, IsReverse, false>(
                  const_cast<typename IntrusiveListIterator<OptionsType, IsReverse,
                  false>::node_reference>(*m_nodePtr));
      }
      return IntrusiveListIterator<OptionsType, IsReverse, false>();
   }

   // Accessors...
   reference operator*() const
   {
      assert(!m_nodePtr->isKnownSentinel());
      return *Access::getValuePtr(m_nodePtr);
   }

   pointer operator->() const
   {
      return &operator*();
   }

   // Comparison operators
   friend bool operator==(const IntrusiveListIterator &lhs, const IntrusiveListIterator &rhs)
   {
      return lhs.m_nodePtr == rhs.m_nodePtr;
   }

   friend bool operator!=(const IntrusiveListIterator &lhs, const IntrusiveListIterator &rhs)
   {
      return lhs.m_nodePtr != rhs.m_nodePtr;
   }

   // Increment and decrement operators...
   IntrusiveListIterator &operator--()
   {
      m_nodePtr = IsReverse ? m_nodePtr->getNext() : m_nodePtr->getPrev();
      return *this;
   }

   IntrusiveListIterator &operator++()
   {
      m_nodePtr = IsReverse ? m_nodePtr->getPrev() : m_nodePtr->getNext();
      return *this;
   }

   IntrusiveListIterator operator--(int)
   {
      IntrusiveListIterator temp = *this;
      --*this;
      return temp;
   }

   IntrusiveListIterator operator++(int)
   {
      IntrusiveListIterator temp = *this;
      ++*this;
      return temp;
   }

   /// Get the underlying ilist_node.
   node_pointer getNodePtr() const
   {
      return static_cast<node_pointer>(m_nodePtr);
   }

   /// Check for end.  Only valid if ilist_sentinel_tracking<true>.
   bool isEnd() const
   {
      return m_nodePtr ? m_nodePtr->isSentinel() : false;
   }
};

template <typename From> struct SimplifyType;

/// Allow IntrusiveListIterators to convert into pointers to a node automatically when
/// used by the dyn_cast, cast, isa mechanisms...
///
/// FIXME: remove this, since there is no implicit conversion to NodeTy.
template <typename OptionsType, bool IsConst>
struct SimplifyType<IntrusiveListIterator<OptionsType, false, IsConst>> {
   using iterator = IntrusiveListIterator<OptionsType, false, IsConst>;
   using SimpleType = typename iterator::pointer;

   static SimpleType getSimplifiedValue(const iterator &node)
   {
      return &*node;
   }
};

template <typename OptionsType, bool IsConst>
struct SimplifyType<const IntrusiveListIterator<OptionsType, false, IsConst>>
      : SimplifyType<IntrusiveListIterator<OptionsType, false, IsConst>>
{};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_INTRUSIVE_LIST_ITERATOR_H
