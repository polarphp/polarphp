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

#ifndef POLARPHP_BASIC_ADT_SIMPLE_INTRUSIVE_LIST_H
#define POLARPHP_BASIC_ADT_SIMPLE_INTRUSIVE_LIST_H

#include "polarphp/global/Global.h"
#include "polarphp/basic/adt/IntrusiveListBase.h"
#include "polarphp/basic/adt/IntrusiveListIterator.h"
#include "polarphp/basic/adt/IntrusiveListNode.h"
#include "polarphp/basic/adt/IntrusiveListNodeOptions.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>

namespace polar {
namespace basic {

/// A simple intrusive list implementation.
///
/// This is a simple intrusive list for a \c T that inherits from \c
/// ilist_node<T>.  The list never takes ownership of anything inserted in it.
///
/// Unlike \a iplist<T> and \a ilist<T>, \a SimpleIntrusiveList<T> never allocates or
/// deletes values, and has no callback traits.
///
/// The API for adding nodes include \a push_front(), \a push_back(), and \a
/// insert().  These all take values by reference (not by pointer), except for
/// the range version of \a insert().
///
/// There are three sets of API for discarding nodes from the list: \a
/// remove(), which takes a reference to the node to remove, \a erase(), which
/// takes an iterator or iterator range and returns the next one, and \a
/// clear(), which empties out the container.  All three are constant time
/// operations.  None of these deletes any nodes; in particular, if there is a
/// single node in the list, then these have identical semantics:
/// \li \c L.remove(L.front());
/// \li \c L.erase(L.begin());
/// \li \c L.clear();
///
/// As a convenience for callers, there are parallel APIs that take a \c
/// Disposer (such as \c std::default_delete<T>): \a removeAndDispose(), \a
/// eraseAndDispose(), and \a clearAndDispose().  These have different names
/// because the extra semantic is otherwise non-obvious.  They are equivalent
/// to calling \a std::for_each() on the range to be discarded.
///
/// The currently available \p Options customize the nodes in the list.  The
/// same options must be specified in the \a ilist_node instantation for
/// compatibility (although the order is irrelevant).
/// \li Use \a ilist_tag to designate which ilist_node for a given \p T this
/// list should use.  This is useful if a type \p T is part of multiple,
/// independent lists simultaneously.
/// \li Use \a IntrusiveListSentinel_tracking to always (or never) track whether a
/// node is a sentinel.  Specifying \c true enables the \a
/// ilist_node::isSentinel() API.  Unlike \a ilist_node::isKnownSentinel(),
/// which is only appropriate for assertions, \a ilist_node::isSentinel() is
/// appropriate for real logic.
///
/// Here are examples of \p Options usage:
/// \li \c SimpleIntrusiveList<T> gives the defaults.  \li \c
/// SimpleIntrusiveList<T,IntrusiveListSentinel_tracking<true>> enables the \a
/// ilist_node::isSentinel() API.
/// \li \c SimpleIntrusiveList<T,ilist_tag<A>,IntrusiveListSentinel_tracking<false>>
/// specifies a tag of A and that tracking should be off (even when
/// POLAR_ENABLE_ABI_BREAKING_CHECKS are enabled).
/// \li \c SimpleIntrusiveList<T,IntrusiveListSentinel_tracking<false>,ilist_tag<A>> is
/// equivalent to the last.
///
/// See \a is_valid_option for steps on adding a new option.
template <typename T, typename... Options>
class SimpleIntrusiveList
      : ilist_internal::ComputeNodeOptions<T, Options...>::type::ListBaseType,
      ilist_internal::SpecificNodeAccess<
      typename ilist_internal::ComputeNodeOptions<T, Options...>::type>
{
   static_assert(ilist_internal::CheckOptions<Options...>::value,
                 "Unrecognized node option!");
   using OptionsType =
   typename ilist_internal::ComputeNodeOptions<T, Options...>::type;
   using ListBaseType = typename OptionsType::ListBaseType;
   IntrusiveListSentinel<OptionsType> m_sentinel;

public:
   using value_type = typename OptionsType::value_type;
   using pointer = typename OptionsType::pointer;
   using reference = typename OptionsType::reference;
   using const_pointer = typename OptionsType::const_pointer;
   using const_reference = typename OptionsType::const_reference;
   using iterator = IntrusiveListIterator<OptionsType, false, false>;
   using Iterator = iterator;
   using const_iterator = IntrusiveListIterator<OptionsType, false, true>;
   using ConstIterator = const_iterator;
   using reverse_iterator = IntrusiveListIterator<OptionsType, true, false>;
   using ReverseIterator = reverse_iterator;
   using const_reverse_iterator = IntrusiveListIterator<OptionsType, true, true>;
   using ConstReverseIterator = const_reverse_iterator;
   using size_type = size_t;
   using difference_type = ptrdiff_t;

   SimpleIntrusiveList() = default;
   ~SimpleIntrusiveList() = default;

   // No copy constructors.
   SimpleIntrusiveList(const SimpleIntrusiveList &) = delete;
   SimpleIntrusiveList &operator=(const SimpleIntrusiveList &) = delete;

   // Move constructors.
   SimpleIntrusiveList(SimpleIntrusiveList &&other)
   {
      splice(end(), other);
   }

   SimpleIntrusiveList &operator=(SimpleIntrusiveList &&other)
   {
      clear();
      splice(end(), other);
      return *this;
   }

   iterator begin()
   {
      return ++iterator(m_sentinel);
   }

   const_iterator begin() const
   {
      return ++const_iterator(m_sentinel);
   }

   iterator end()
   {
      return iterator(m_sentinel);
   }

   const_iterator end() const
   {
      return const_iterator(m_sentinel);
   }

   reverse_iterator rbegin()
   {
      return ++reverse_iterator(m_sentinel);
   }

   const_reverse_iterator rbegin() const
   {
      return ++const_reverse_iterator(m_sentinel);
   }

   reverse_iterator rend()
   {
      return reverse_iterator(m_sentinel);
   }

   const_reverse_iterator rend() const
   {
      return const_reverse_iterator(m_sentinel);
   }

   /// Check if the list is empty in constant time.
   POLAR_NODISCARD bool empty() const
   {
      return m_sentinel.empty();
   }

   /// Calculate the size of the list in linear time.
   POLAR_NODISCARD size_type size() const
   {
      return std::distance(begin(), end());
   }

   reference front()
   {
      return *begin();
   }

   const_reference front() const
   {
      return *begin();
   }

   reference back()
   {
      return *rbegin();
   }

   const_reference back() const
   {
      return *rbegin();
   }

   /// Insert a node at the front; never copies.
   void push_front(reference node)
   {
      insert(begin(), node);
   }

   /// Insert a node at the back; never copies.
   void push_back(reference node)
   {
      insert(end(), node);
   }

   /// Remove the node at the front; never deletes.
   void pop_front()
   {
      erase(begin());
   }

   /// Remove the node at the back; never deletes.
   void pop_back()
   {
      erase(--end());
   }

   /// Swap with another list in place using std::swap.
   void swap(SimpleIntrusiveList &other)
   {
      std::swap(*this, other);
   }

   /// Insert a node by reference; never copies.
   iterator insert(iterator iter, reference node)
   {
      ListBaseType::insertBefore(*iter.getNodePtr(), *this->getNodePtr(&node));
      return iterator(&node);
   }

   /// Insert a range of nodes; never copies.
   template <typename Iterator>
   void insert(iterator iter, Iterator first, Iterator last)
   {
      for (; first != last; ++first) {
         insert(iter, *first);
      }
   }

   /// Clone another list.
   template <typename Cloner, class Disposer>
   void cloneFrom(const SimpleIntrusiveList &list, Cloner clone, Disposer dispose)
   {
      clearAndDispose(dispose);
      for (const_reference item : list) {
         push_back(*clone(item));
      }
   }

   /// Remove a node by reference; never deletes.
   ///
   /// \see \a erase() for removing by iterator.
   /// \see \a removeAndDispose() if the node should be deleted.
   void remove(reference node)
   {
      ListBaseType::remove(*this->getNodePtr(&node));
   }

   /// Remove a node by reference and dispose of it.
   template <typename Disposer>
   void removeAndDispose(reference node, Disposer dispose)
   {
      remove(node);
      dispose(&node);
   }

   /// Remove a node by iterator; never deletes.
   ///
   /// \see \a remove() for removing by reference.
   /// \see \a eraseAndDispose() it the node should be deleted.
   iterator erase(iterator iter)
   {
      assert(iter != end() && "Cannot remove end of list!");
      remove(*iter++);
      return iter;
   }

   /// Remove a range of nodes; never deletes.
   ///
   /// \see \a eraseAndDispose() if the nodes should be deleted.
   iterator erase(iterator first, iterator last)
   {
      ListBaseType::removeRange(*first.getNodePtr(), *last.getNodePtr());
      return last;
   }

   /// Remove a node by iterator and dispose of it.
   template <typename Disposer>
   iterator eraseAndDispose(iterator iter, Disposer dispose)
   {
      auto next = std::next(iter);
      erase(iter);
      dispose(&*iter);
      return next;
   }

   /// Remove a range of nodes and dispose of them.
   template <typename Disposer>
   iterator eraseAndDispose(iterator first, iterator last, Disposer dispose)
   {
      while (first != last) {
         first = eraseAndDispose(first, dispose);
      }
      return last;
   }

   /// Clear the list; never deletes.
   ///
   /// \see \a clearAndDispose() if the nodes should be deleted.
   void clear()
   {
      m_sentinel.reset();
   }

   /// Clear the list and dispose of the nodes.
   template <typename Disposer> void clearAndDispose(Disposer dispose)
   {
      eraseAndDispose(begin(), end(), dispose);
   }

   /// Splice in another list.
   void splice(iterator iter, SimpleIntrusiveList &list)
   {
      splice(iter, list, list.begin(), list.end());
   }

   /// Splice in a node from another list.
   void splice(iterator iter, SimpleIntrusiveList &list, iterator node)
   {
      splice(iter, list, node, std::next(node));
   }

   /// Splice in a range of nodes from another list.
   void splice(iterator iter, SimpleIntrusiveList &, iterator first, iterator last)
   {
      ListBaseType::transferBefore(*iter.getNodePtr(), *first.getNodePtr(),
                                   *last.getNodePtr());
   }

   /// Merge in another list.
   ///
   /// \pre \c this and \p RHS are sorted.
   ///@{
   void merge(SimpleIntrusiveList &other)
   {
      merge(other, std::less<T>());
   }

   template <typename Compare>
   void merge(SimpleIntrusiveList &other, Compare comp);
   ///@}

   /// Sort the list.
   ///@{
   void sort()
   {
      sort(std::less<T>());
   }

   template <typename Compare>
   void sort(Compare comp);
   ///@}
};

template <typename T, typename... Options>
template <typename Compare>
void SimpleIntrusiveList<T, Options...>::merge(SimpleIntrusiveList &other, Compare comp)
{
   if (this == &other || other.empty()) {
      return;
   }
   iterator leftIter = begin(), leftIterEnd = end();
   iterator rightIter = other.begin(), rightIterEnd = other.end();
   while (leftIter != leftIterEnd) {
      if (comp(*rightIter, *leftIter)) {
         // Transfer a run of at least size 1 from RHS to LHS.
         iterator runStart = rightIter++;
         rightIter = std::find_if(rightIter, rightIterEnd, [&](reference refvalue) { return !comp(refvalue, *leftIter); });
         splice(leftIter, other, runStart, rightIter);
         if (rightIter == rightIterEnd) {
            return;
         }
      }
      ++leftIter;
   }
   // Transfer the remaining RHS nodes once LHS is finished.
   splice(leftIterEnd, other, rightIter, rightIterEnd);
}

template <typename T, class... Options>
template <typename Compare>
void SimpleIntrusiveList<T, Options...>::sort(Compare comp)
{
   // Vacuously sorted.
   if (empty() || std::next(begin()) == end()) {
      return;
   }
   // Split the list in the middle.
   iterator center = begin(), endMark = begin();
   while (endMark != end() && ++endMark != end()) {
      ++center;
      ++endMark;
   }
   SimpleIntrusiveList rhs;
   rhs.splice(rhs.end(), *this, center, end());

   // Sort the sublists and merge back together.
   sort(comp);
   rhs.sort(comp);
   merge(rhs, comp);
}

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_SIMPLE_INTRUSIVE_LIST_H
