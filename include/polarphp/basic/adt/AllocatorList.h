// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/05.

#ifndef POLARPHP_BASIC_ADT_ALLOCATOR_LIST_H
#define POLARPHP_BASIC_ADT_ALLOCATOR_LIST_H

#include "polarphp/basic/adt/IntrusiveListNode.h"
#include "polarphp/basic/adt/Iterator.h"
#include "polarphp/basic/adt/SimpleIntrusiveList.h"
#include "polarphp/utils/Allocator.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace polar {
namespace basic {

using polar::utils::BumpPtrAllocator;

/// A linked-list with a custom, local allocator.
///
/// Expose a std::list-like interface that owns and uses a custom LLVM-style
/// allocator (e.g., BumpPtrAllocator), leveraging \a simple_ilist for the
/// implementation details.
///
/// Because this list owns the allocator, calling \a splice() with a different
/// list isn't generally safe.  As such, \a splice has been left out of the
/// interface entirely.
template <typename T, typename AllocatorT>
class AllocatorList : AllocatorT
{
   struct Node : IntrusiveListNode<Node>
   {
      Node(Node &&) = delete;
      Node(const Node &) = delete;
      Node &operator=(Node &&) = delete;
      Node &operator=(const Node &) = delete;

      Node(T &&other) : m_value(std::move(other))
      {}

      Node(const T &value) : m_value(value)
      {}

      template <typename... Ts> Node(Ts &&... values) : m_value(std::forward<Ts>(values)...)
      {}

      T m_value;
   };

   using ListType = SimpleIntrusiveList<Node>;

   ListType m_list;

   AllocatorT &getAlloc()
   {
      return *this;
   }

   const AllocatorT &getAlloc() const
   {
      return *this;
   }

   template <typename... ArgTs> Node *create(ArgTs &&... args)
   {
      return new (getAlloc()) Node(std::forward<ArgTs>(args)...);
   }

   struct Cloner
   {
      AllocatorList &m_allocatorList;

      Cloner(AllocatorList &m_allocatorList) : m_allocatorList(m_allocatorList)
      {}

      Node *operator()(const Node &node) const
      {
         return m_allocatorList.create(node.m_value);
      }
   };

   struct Disposer
   {
      AllocatorList &m_allocatorList;

      Disposer(AllocatorList &allocatorList) : m_allocatorList(allocatorList)
      {}

      void operator()(Node *node) const {
         node->~Node();
         m_allocatorList.getAlloc().deallocate(node);
      }
   };

public:
   using value_type = T;
   using pointer = T *;
   using reference = T &;
   using const_pointer = const T *;
   using const_reference = const T &;
   using size_type = typename ListType::size_type;
   using difference_type = typename ListType::difference_type;

private:
   template <typename ValueT, class IteratorBase>
   class IteratorImpl
         : public IteratorAdaptorBase<IteratorImpl<ValueT, IteratorBase>,
         IteratorBase,
         std::bidirectional_iterator_tag, ValueT> {
      template <typename OtherValueT, class OtherIteratorBase>
      friend class IteratorImpl;
      friend class AllocatorList;

      using BaseType =
      IteratorAdaptorBase<IteratorImpl<ValueT, IteratorBase>, IteratorBase,
      std::bidirectional_iterator_tag, ValueT>;

   public:
      using value_type = ValueT;
      using pointer = ValueT *;
      using reference = ValueT &;

      IteratorImpl() = default;
      IteratorImpl(const IteratorImpl &) = default;
      IteratorImpl &operator=(const IteratorImpl &) = default;

      explicit IteratorImpl(const IteratorBase &iter) : BaseType(iter)
      {}

      template <typename OtherValueT, class OtherIteratorBase>
      IteratorImpl(const IteratorImpl<OtherValueT, OtherIteratorBase> &other,
                   typename std::enable_if<std::is_convertible<
                   OtherIteratorBase, IteratorBase>::value>::type * = nullptr)
         : BaseType(other.wrapped())
      {}

      ~IteratorImpl() = default;

      reference operator*() const
      {
         return BaseType::getWrapped()->m_value;
      }

      pointer operator->() const
      {
         return &operator*();
      }

      friend bool operator==(const IteratorImpl &lhs, const IteratorImpl &rhs)
      {
         return lhs.getWrapped() == rhs.getWrapped();
      }

      friend bool operator!=(const IteratorImpl &lhs, const IteratorImpl &rhs)
      {
         return !(lhs == rhs);
      }
   };

public:
   using Iterator = IteratorImpl<T, typename ListType::Iterator>;
   using reverse_iterator =
   IteratorImpl<T, typename ListType::reverse_iterator>;
   using const_iterator =
   IteratorImpl<const T, typename ListType::const_iterator>;
   using const_reverse_iterator =
   IteratorImpl<const T, typename ListType::const_reverse_iterator>;

   AllocatorList() = default;
   AllocatorList(AllocatorList &&other)
      : AllocatorT(std::move(other.getAlloc())), m_list(std::move(other.m_list))
   {}

   AllocatorList(const AllocatorList &other)
   {
      m_list.cloneFrom(other.m_list, Cloner(*this), Disposer(*this));
   }

   AllocatorList &operator=(AllocatorList &&other)
   {
      clear(); // Dispose of current nodes explicitly.
      m_list = std::move(other.m_list);
      getAlloc() = std::move(other.getAlloc());
      return *this;
   }

   AllocatorList &operator=(const AllocatorList &other)
   {
      m_list.cloneFrom(other.m_list, Cloner(*this), Disposer(*this));
      return *this;
   }

   ~AllocatorList()
   {
      clear();
   }

   void swap(AllocatorList &other)
   {
      m_list.swap(other.m_list);
      std::swap(getAlloc(), other.getAlloc());
   }

   bool empty()
   {
      return m_list.empty();
   }

   size_t size()
   {
      return m_list.size();
   }

   Iterator begin()
   {
      return Iterator(m_list.begin());
   }

   Iterator end()
   {
      return Iterator(m_list.end());
   }

   const_iterator begin() const
   {
      return const_iterator(m_list.begin());
   }

   const_iterator end() const
   {
      return const_iterator(m_list.end());
   }

   reverse_iterator rbegin()
   {
      return reverse_iterator(m_list.rbegin());
   }

   reverse_iterator rend()
   {
      return reverse_iterator(m_list.rend());
   }

   const_reverse_iterator rbegin() const
   {
      return const_reverse_iterator(m_list.rbegin());
   }

   const_reverse_iterator rend() const
   {
      return const_reverse_iterator(m_list.rend());
   }

   T &back()
   {
      return m_list.back().m_value;
   }

   T &front()
   {
      return m_list.front().m_value;
   }

   const T &back() const
   {
      return m_list.back().m_value;
   }

   const T &front() const
   {
      return m_list.front().m_value;
   }

   template <typename... Ts> Iterator emplace(Iterator iter, Ts &&... values)
   {
      return Iterator(m_list.insert(iter.getWrapped(), *create(std::forward<Ts>(values)...)));
   }

   Iterator insert(Iterator iter, T &&value)
   {
      return Iterator(m_list.insert(iter.getWrapped(), *create(std::move(value))));
   }

   Iterator insert(Iterator iter, const T &value)
   {
      return Iterator(m_list.insert(iter.getWrapped(), *create(value)));
   }

   template <typename OtherIterType>
   void insert(Iterator iter, OtherIterType first, OtherIterType last)
   {
      for (; first != last; ++first) {
         m_list.insert(iter.getWrapped(), *create(*first));
      }
   }

   Iterator erase(Iterator iter)
   {
      return Iterator(m_list.eraseAndDispose(iter.getWrapped(), Disposer(*this)));
   }

   Iterator erase(Iterator first, Iterator last)
   {
      return Iterator(
               m_list.eraseAndDispose(first.getWrapped(), last.getWrapped(), Disposer(*this)));
   }

   void clear()
   {
      m_list.clearAndDispose(Disposer(*this));
   }

   void popBack()
   {
      pop_back();
   }

   void pop_back()
   {
      m_list.eraseAndDispose(--m_list.end(), Disposer(*this));
   }

   void popFront()
   {
      pop_front();
   }

   void pop_front()
   {
      m_list.eraseAndDispose(m_list.begin(), Disposer(*this));
   }

   void pushBack(T &&value)
   {
      insert(end(), std::move(value));
   }

   void push_back(T &&value)
   {
      insert(end(), std::move(value));
   }

   void push_front(T &&value)
   {
      insert(begin(), std::move(value));
   }

   void pushFront(T &&value)
   {
      insert(begin(), std::move(value));
   }

   void push_back(const T &value)
   {
      insert(end(), value);
   }

   void pushBack(const T &value)
   {
      insert(end(), value);
   }

   void push_front(const T &value)
   {
      insert(begin(), value);
   }

   void pushFront(const T &value)
   {
      insert(begin(), value);
   }

   template <typename... Ts>
   void emplace_back(Ts &&... values)
   {
      emplace(end(), std::forward<Ts>(values)...);
   }

   template <typename... Ts>
   void emplaceBack(Ts &&... values)
   {
      emplace(end(), std::forward<Ts>(values)...);
   }

   template <typename... Ts>
   void emplace_front(Ts &&... values)
   {
      emplace(begin(), std::forward<Ts>(values)...);
   }

   template <typename... Ts>
   void emplaceFront(Ts &&... values)
   {
      emplace(begin(), std::forward<Ts>(values)...);
   }

   /// Reset the underlying allocator.
   ///
   /// \pre \c empty()
   void resetAlloc()
   {
      assert(empty() && "Cannot reset allocator if not empty");
      getAlloc().reset();
   }
};

template <typename T>
using BumpPtrList = AllocatorList<T, BumpPtrAllocator>;

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_ALLOCATOR_LIST_H
