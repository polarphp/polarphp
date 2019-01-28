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

//===----------------------------------------------------------------------===//
//
// This file defines classes to implement an intrusive doubly linked list class
// (i.e. each node of the list must contain a next and previous field for the
// list.
//
// The ilist class itself should be a plug in replacement for list.  This list
// replacement does not provide a constant time size() method, so be careful to
// use empty() when you really want to know if it's empty.
//
// The ilist class is implemented as a circular list.  The list itself contains
// a sentinel node, whose Next points at begin() and whose Prev points at
// rbegin().  The sentinel node itself serves as end() and rend().
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_ADT_INTRUSIVE_LIST_H
#define POLARPHP_BASIC_ADT_INTRUSIVE_LIST_H

#include "polarphp/basic/adt/SimpleIntrusiveList.h"
#include <cassert>
#include <cstddef>
#include <iterator>

namespace polar {
namespace basic {

/// Use delete by default for PurelyIntrusiveList and ilist.
///
/// Specialize this to get different behaviour for ownership-related API.  (If
/// you really want ownership semantics, consider using std::list or building
/// something like \a BumpPtrList.)
///
/// \see IntrusiveListNoAllocTraits
template <typename NodeTypeype>
struct IntrusiveListAllocTraits
{
   static void deleteNode(NodeTypeype *node)
   {
      delete node;
   }
};

/// Custom traits to do nothing on deletion.
///
/// Specialize IntrusiveListAllocTraits to inherit from this to disable the
/// non-intrusive deletion in PurelyIntrusiveList (which implies ownership).
///
/// If you want purely intrusive semantics with no callbacks, consider using \a
/// SimpleIntrusiveList instead.
///
/// \code
/// template <>
/// struct IntrusiveListAllocTraits<MyType> : IntrusiveListNoAllocTraits<MyType> {};
/// \endcode
template <typename NodeTypeype>
struct IntrusiveListNoAllocTraits
{
   static void deleteNode(NodeTypeype *node)
   {}
};

/// Callbacks do nothing by default in PurelyIntrusiveList and ilist.
///
/// Specialize this for to use callbacks for when nodes change their list
/// membership.
template <typename NodeTypeype>
struct IntrusiveListCallbackTraits
{
   void addNodeToList(NodeTypeype *) {}
   void removeNodeFromList(NodeTypeype *) {}

   /// Callback before transferring nodes to this list.
   ///
   /// \pre \c this!=&oldList
   template <typename Iterator>
   void transferNodesFromList(IntrusiveListCallbackTraits &oldList, Iterator /*first*/,
                              Iterator /*last*/)
   {
      (void)oldList;
   }
};

/// A fragment for template traits for intrusive list that provides default
/// node related operations.
///
/// TODO: Remove this layer of indirection.  It's not necessary.
template <typename NodeTypeype>
struct IntrusiveListNodeTyperaits : IntrusiveListAllocTraits<NodeTypeype>,
      IntrusiveListCallbackTraits<NodeTypeype>
{};

/// Default template traits for intrusive list.
///
/// By inheriting from this, you can easily use default implementations for all
/// common operations.
///
/// TODO: Remove this customization point.  Specializing IntrusiveListTyperaits is
/// already fully general.
template <typename NodeTypeype>
struct IntrusiveListDefaultTraits : public IntrusiveListNodeTyperaits<NodeTypeype>
{};

/// Template traits for intrusive list.
///
/// Customize callbacks and allocation semantics.
template <typename NodeTypeype>
struct IntrusiveListTyperaits : public IntrusiveListDefaultTraits<NodeTypeype>
{};

/// Const traits should never be instantiated.
template <typename Type>
struct IntrusiveListTyperaits<const Type>
{};

namespace ilist_internal {

template <typename T> T &make();

/// Type trait to check for a traits class that has a getNext member (as a
/// canary for any of the ilist_nextprev_traits API).
template <typename TraitsType, typename NodeType>
struct HasGetNext
{
   typedef char Yes[1];
   typedef char No[2];

   template <size_t N>
   struct SFINAE {};

   template <typename U>
   static Yes &test(U *iter, decltype(iter->getNext(&make<NodeType>())) * = 0);
   template <typename> static No &test(...);

public:
   static const bool value = sizeof(test<TraitsType>(nullptr)) == sizeof(Yes);
};

/// Type trait to check for a traits class that has a createSentinel member (as
/// a canary for any of the ilist_sentinel_traits API).
template <typename TraitsType>
struct HasCreateSentinel
{
   typedef char Yes[1];
   typedef char No[2];

   template <typename U>
   static Yes &test(U *iter, decltype(iter->createSentinel()) * = 0);
   template <typename> static No &test(...);

public:
   static const bool value = sizeof(test<TraitsType>(nullptr)) == sizeof(Yes);
};

/// Type trait to check for a traits class that has a createNode member.
/// Allocation should be managed in a wrapper class, instead of in
/// IntrusiveListTyperaits.
template <typename TraitsType, typename NodeType>
struct HasCreateNode
{
   typedef char Yes[1];
   typedef char No[2];
   template <size_t N> struct SFINAE {};

   template <typename U>
   static Yes &test(U *iter, decltype(iter->createNode(make<NodeType>())) * = 0);
   template <typename> static No &test(...);

public:
   static const bool value = sizeof(test<TraitsType>(nullptr)) == sizeof(Yes);
};

template <typename TraitsType, typename NodeType>
struct HasObsoleteCustomization
{
   static const bool value = HasGetNext<TraitsType, NodeType>::value ||
   HasCreateSentinel<TraitsType>::value ||
   HasCreateNode<TraitsType, NodeType>::value;
};

} // end namespace ilist_internal

//===----------------------------------------------------------------------===//
//
/// A wrapper around an intrusive list with callbacks and non-intrusive
/// ownership.
///
/// This wraps a purely intrusive list (like SimpleIntrusiveList) with a configurable
/// traits class.  The traits can implement callbacks and customize the
/// ownership semantics.
///
/// This is a subset of ilist functionality that can safely be used on nodes of
/// polymorphic types, i.e. a heterogeneous list with a common base class that
/// holds the next/prev pointers.  The only state of the list itself is an
/// ilist_sentinel, which holds pointers to the first and last nodes in the
/// list.
template <typename IntrusiveListType, typename TraitsType>
class PurelyIntrusiveListImpl : public TraitsType, IntrusiveListType
{
   using BaseListType = IntrusiveListType;

protected:
   typedef PurelyIntrusiveListImpl PurelyIntrusiveListImplType;

public:
   typedef typename BaseListType::pointer pointer;
   typedef typename BaseListType::const_pointer const_pointer;
   typedef typename BaseListType::reference reference;
   typedef typename BaseListType::const_reference const_reference;
   typedef typename BaseListType::value_type value_type;
   typedef typename BaseListType::size_type size_type;
   typedef typename BaseListType::difference_type difference_type;
   typedef typename BaseListType::iterator iterator;
   typedef typename BaseListType::const_iterator const_iterator;
   typedef typename BaseListType::reverse_iterator reverse_iterator;
   typedef
   typename BaseListType::const_reverse_iterator const_reverse_iterator;

private:
   // TODO: Drop this assertion and the transitive type traits anytime after
   // v4.0 is branched (i.e,. keep them for one release to help out-of-tree code
   // update).
   static_assert(
         !ilist_internal::HasObsoleteCustomization<TraitsType, value_type>::value,
         "ilist customization points have changed!");

   static bool opLess(const_reference lhs, const_reference rhs)
   {
      return lhs < rhs;
   }

   static bool opEqual(const_reference lhs, const_reference rhs)
   {
      return lhs == rhs;
   }

public:
   PurelyIntrusiveListImpl() = default;

   PurelyIntrusiveListImpl(const PurelyIntrusiveListImpl &) = delete;
   PurelyIntrusiveListImpl &operator=(const PurelyIntrusiveListImpl &) = delete;

   PurelyIntrusiveListImpl(PurelyIntrusiveListImpl &&other)
      : TraitsType(std::move(other)), IntrusiveListType(std::move(other))
   {}

   PurelyIntrusiveListImpl &operator=(PurelyIntrusiveListImpl &&other)
   {
      *static_cast<TraitsType *>(this) = std::move(other);
      *static_cast<IntrusiveListType *>(this) = std::move(other);
      return *this;
   }

   ~PurelyIntrusiveListImpl()
   {
      clear();
   }

   // Miscellaneous inspection routines.
   size_type getMaxSize() const
   {
      return size_type(-1);
   }

   using BaseListType::begin;
   using BaseListType::end;
   using BaseListType::rbegin;
   using BaseListType::rend;
   using BaseListType::empty;
   using BaseListType::front;
   using BaseListType::back;

   void swap(PurelyIntrusiveListImpl &rhs)
   {
      assert(0 && "Swap does not use list traits callback correctly yet!");
      BaseListType::swap(rhs);
   }

   iterator insert(iterator where, pointer newValue)
   {
      this->addNodeToList(newValue); // Notify traits that we added a node...
      return BaseListType::insert(where, *newValue);
   }

   iterator insert(iterator where, const_reference newValue)
   {
      return this->insert(where, new value_type(newValue));
   }

   iterator insertAfter(iterator where, pointer newValue)
   {
      if (empty()) {
         return insert(begin(), newValue);
      } else {
         return insert(++where, newValue);
      }
   }

   /// Clone another list.
   template <typename Cloner> void cloneFrom(const PurelyIntrusiveListImpl &list, Cloner clone)
   {
      clear();
      for (const_reference item : list) {
         push_back(clone(item));
      }
   }

   pointer remove(iterator &iter)
   {
      pointer node = &*iter++;
      this->removeNodeFromList(node); // Notify traits that we removed a node...
      BaseListType::remove(*node);
      return node;
   }

   pointer remove(const iterator &iter)
   {
      iterator mutIt = iter;
      return remove(mutIt);
   }

   pointer remove(pointer iter)
   {
      return remove(iterator(iter));
   }

   pointer remove(reference iter)
   {
      return remove(iterator(iter));
   }

   // erase - remove a node from the controlled sequence... and delete it.
   iterator erase(iterator where)
   {
      this->deleteNode(remove(where));
      return where;
   }

   iterator erase(pointer iter)
   {
      return erase(iterator(iter));
   }

   iterator erase(reference iter)
   {
      return erase(iterator(iter));
   }

   /// Remove all nodes from the list like clear(), but do not call
   /// removeNodeFromList() or deleteNode().
   ///
   /// This should only be used immediately before freeing nodes in bulk to
   /// avoid traversing the list and bringing all the nodes into cache.
   void clearAndLeakNodesUnsafely()
   {
      BaseListType::clear();
   }

private:
   // transfer - The heart of the splice function.  Move linked list nodes from
   // [first, last) into position.
   //
   void transfer(iterator position, PurelyIntrusiveListImpl &list, iterator first, iterator last) {
      if (position == last) {
         return;
      }
      if (this != &list) {
         // Notify traits we moved the nodes...
         this->transferNodesFromList(list, first, last);
      }
      BaseListType::splice(position, list, first, last);
   }

public:
   //===----------------------------------------------------------------------===
   // Functionality derived from other functions defined above...
   //

   using BaseListType::size;

   iterator erase(iterator first, iterator last)
   {
      while (first != last) {
         first = erase(first);
      }
      return last;
   }

   void clear()
   {
      erase(begin(), end());
   }

   // Front and back inserters...
   void push_front(pointer value)
   {
      insert(begin(), value);
   }

   void push_back(pointer value)
   {
      insert(end(), value);
   }

   void pop_front()
   {
      assert(!empty() && "pop_front() on empty list!");
      erase(begin());
   }

   void pop_back()
   {
      assert(!empty() && "pop_back() on empty list!");
      iterator t = end(); erase(--t);
   }

   // Special forms of insert...
   template<typename InIterator>
   void insert(iterator where, InIterator first, InIterator last)
   {
      for (; first != last; ++first) {
         insert(where, *first);
      }
   }

   // Splice members - defined in terms of transfer...
   void splice(iterator where, PurelyIntrusiveListImpl &list)
   {
      if (!list.empty()) {
         transfer(where, list, list.begin(), list.end());
      }
   }

   void splice(iterator where, PurelyIntrusiveListImpl &list, iterator first)
   {
      iterator last = first; ++last;
      if (where == first || where == last) {
         return; // No change
      }
      transfer(where, list, first, last);
   }
   void splice(iterator where, PurelyIntrusiveListImpl &list, iterator first, iterator last)
   {
      if (first != last) {
         transfer(where, list, first, last);
      }
   }

   void splice(iterator where, PurelyIntrusiveListImpl &list, reference node)
   {
      splice(where, list, iterator(node));
   }

   void splice(iterator where, PurelyIntrusiveListImpl &list, pointer node)
   {
      splice(where, list, iterator(node));
   }

   template <typename Compare>
   void merge(PurelyIntrusiveListImpl &other, Compare comp)
   {
      if (this == &other) {
         return;
      }
      this->transferNodesFromList(other, other.begin(), other.end());
      BaseListType::merge(other, comp);
   }

   void merge(PurelyIntrusiveListImpl &other)
   {
      return merge(other, opLess);
   }

   using BaseListType::sort;

   /// \brief Get the previous node, or \c nullptr for the list head.
   pointer getPrevNode(reference node) const
   {
      auto iter = node.getIterator();
      if (iter == begin()) {
         return nullptr;
      }
      return &*std::prev(iter);
   }

   /// \brief Get the previous node, or \c nullptr for the list head.
   const_pointer getPrevNode(const_reference node) const
   {
      return getPrevNode(const_cast<reference >(node));
   }

   /// \brief Get the next node, or \c nullptr for the list tail.
   pointer getNextNode(reference node) const
   {
      auto next = std::next(node.getIterator());
      if (next == end()) {
         return nullptr;
      }
      return &*next;
   }

   /// \brief Get the next node, or \c nullptr for the list tail.
   const_pointer getNextNode(const_reference node) const
   {
      return getNextNode(const_cast<reference >(node));
   }
};

/// An intrusive list with ownership and callbacks specified/controlled by
/// IntrusiveListTyperaits, only with API safe for polymorphic types.
///
/// The \p Options parameters are the same as those for \a SimpleIntrusiveList.  See
/// there for a description of what's available.
template <typename T, class... Options>
class PurelyIntrusiveList
      : public PurelyIntrusiveListImpl<SimpleIntrusiveList<T, Options...>, IntrusiveListTyperaits<T>>
{
   using PurelyIntrusiveListImplType = typename PurelyIntrusiveList::PurelyIntrusiveListImplType;

public:
   PurelyIntrusiveList() = default;

   PurelyIntrusiveList(const PurelyIntrusiveList &other) = delete;
   PurelyIntrusiveList &operator=(const PurelyIntrusiveList &other) = delete;

   PurelyIntrusiveList(PurelyIntrusiveList &&other) : PurelyIntrusiveListImplType(std::move(other))
   {}

   PurelyIntrusiveList &operator=(PurelyIntrusiveList &&other)
   {
      *static_cast<PurelyIntrusiveListImplType *>(this) = std::move(other);
      return *this;
   }
};

template <typename T, class... Options>
using IntrusiveList = PurelyIntrusiveList<T, Options...>;

} // basic
} // polar

namespace std {

// Ensure that swap uses the fast list swap...
template<typename Type>
void swap(polar::basic::PurelyIntrusiveList<Type> &left, polar::basic::PurelyIntrusiveList<Type> &right)
{
   left.swap(right);
}

} // end namespace std

#endif // POLARPHP_BASIC_ADT_INTRUSIVE_LIST_H
