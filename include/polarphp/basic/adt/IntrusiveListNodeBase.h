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

#ifndef POLARPHP_BASIC_ADT_INTRUSIVE_LIST_NODE_BASE_H
#define POLARPHP_BASIC_ADT_INTRUSIVE_LIST_NODE_BASE_H

#include "polarphp/basic/adt/PointerIntPair.h"

namespace polar {
namespace basic {

/// Base class for ilist nodes.
///
/// Optionally tracks whether this node is the sentinel.
template <bool EnableSentinelTracking>
class IntrusiveListNodeBase;

template <>
class IntrusiveListNodeBase<false>
{
   IntrusiveListNodeBase *m_prev = nullptr;
   IntrusiveListNodeBase *m_next = nullptr;

public:
   void setPrev(IntrusiveListNodeBase *prev)
   {
      this->m_prev = prev;
   }

   void setNext(IntrusiveListNodeBase *next)
   {
      this->m_next = next;
   }

   IntrusiveListNodeBase *getPrev() const
   {
      return m_prev;
   }

   IntrusiveListNodeBase *getNext() const
   {
      return m_next;
   }

   bool isKnownSentinel() const
   {
      return false;
   }

   void initializeSentinel()
   {}
};

template <>
class IntrusiveListNodeBase<true>
{
   PointerIntPair<IntrusiveListNodeBase *, 1> m_prevAndSentineleBase;
   IntrusiveListNodeBase *m_next = nullptr;

public:
   void setPrev(IntrusiveListNodeBase *prev)
   {
      m_prevAndSentineleBase.setPointer(prev);
   }

   void setNext(IntrusiveListNodeBase *next)
   {
      this->m_next = next;
   }

   IntrusiveListNodeBase *getPrev() const
   {
      return m_prevAndSentineleBase.getPointer();
   }

   IntrusiveListNodeBase *getNext() const
   {
      return m_next;
   }

   bool isSentinel() const
   {
      return m_prevAndSentineleBase.getInt();
   }

   bool isKnownSentinel() const
   {
      return isSentinel();
   }

   void initializeSentinel()
   {
      m_prevAndSentineleBase.setInt(true);
   }
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_INTRUSIVE_LIST_NODE_BASE_H
