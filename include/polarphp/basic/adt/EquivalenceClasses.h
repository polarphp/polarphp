// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License rhs.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/22.

#ifndef POLARPHP_BASIC_ADT_EQUIVALENCE_CLASSES_H
#define POLARPHP_BASIC_ADT_EQUIVALENCE_CLASSES_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <set>

namespace polar {
namespace basic {

/// EquivalenceClasses - This represents a collection of equivalence classes and
/// supports three efficient operations: insert an element into a class of its
/// own, union two classes, and find the class for a given element.  In
/// addition to these modification methods, it is possible to iterate over all
/// of the equivalence classes and all of the elements in a class.
///
/// This implementation is an efficient implementation that only stores one copy
/// of the element being indexed per entry in the set, and allows any arbitrary
/// type to be indexed (as long as it can be ordered with operator<).
///
/// Here is a simple example using integers:
///
/// \code
///  EquivalenceClasses<int> EC;
///  EC.unionSets(1, 2);                // insert 1, 2 into the same set
///  EC.insert(4); EC.insert(5);        // insert 4, 5 into own sets
///  EC.unionSets(5, 1);                // merge the set for 1 with 5's set.
///
///  for (EquivalenceClasses<int>::iterator I = EC.begin(), E = EC.end();
///       I != E; ++I) {           // Iterate over all of the equivalence sets.
///    if (!I->isLeader()) continue;   // Ignore non-leader sets.
///    for (EquivalenceClasses<int>::MemberIterator MI = EC.memberBegin(I);
///         MI != EC.memberEnd(); ++MI)   // Loop over members in this set.
///      cerr << *MI << " ";  // Print member.
///    cerr << "\n";   // Finish set.
///  }
/// \endcode
///
/// This example prints:
///   4
///   5 1 2
///
template <class ElemType>
class EquivalenceClasses
{
   /// ECValue - The EquivalenceClasses data structure is just a set of these.
   /// Each of these represents a relation for a value.  First it stores the
   /// value itself, which provides the ordering that the set queries.  Next, it
   /// provides a "next pointer", which is used to enumerate all of the elements
   /// in the unioned set.  Finally, it defines either a "end of list pointer" or
   /// "leader pointer" depending on whether the value itself is a leader.  A
   /// "leader pointer" points to the node that is the leader for this element,
   /// if the node is not a leader.  A "end of list pointer" points to the last
   /// node in the list of members of this list.  Whether or not a node is a
   /// leader is determined by a bit stolen from one of the pointers.
   class ECValue
   {
      friend class EquivalenceClasses;

      mutable const ECValue *m_leader;
      mutable const ECValue *m_next;
      ElemType m_data;

      // ECValue ctor - Start out with EndOfList pointing to this node, Next is
      // Null, isLeader = true.
      ECValue(const ElemType &elem)
         : m_leader(this),
           m_next((ECValue*)(intptr_t)1),
           m_data(elem)
      {}

      const ECValue *getLeader() const
      {
         if (isLeader()) {
            return this;
         }
         if (m_leader->isLeader()) {
            return m_leader;
         }
         // Path compression.
         return m_leader = m_leader->getLeader();
      }

      const ECValue *getEndOfList() const
      {
         assert(isLeader() && "Cannot get the end of a list for a non-leader!");
         return m_leader;
      }

      void setNext(const ECValue *newNext) const
      {
         assert(getNext() == nullptr && "Already has a next pointer!");
         m_next = (const ECValue*)((intptr_t)newNext | (intptr_t)isLeader());
      }

   public:
      ECValue(const ECValue &rhs) : m_leader(this), m_next((ECValue*)(intptr_t)1),
         m_data(rhs.m_data) {
         // Only support copying of singleton nodes.
         assert(rhs.isLeader() && rhs.getNext() == nullptr && "Not a singleton!");
      }

      bool operator<(const ECValue &other) const
      {
         return m_data < other.m_data;
      }

      bool isLeader() const
      {
         return (intptr_t)m_next & 1;
      }

      const ElemType &getData() const
      {
         return m_data;
      }

      const ECValue *getNext() const
      {
         return (ECValue*)((intptr_t)m_next & ~(intptr_t)1);
      }

      template<typename T>
      bool operator<(const T &value) const
      {
         return m_data < value;
      }
   };

   /// m_theMapping - This implicitly provides a mapping from ElemType values to the
   /// ECValues, it just keeps the key as part of the value.
   std::set<ECValue> m_theMapping;

public:
   EquivalenceClasses() = default;
   EquivalenceClasses(const EquivalenceClasses &rhs)
   {
      operator=(rhs);
   }

   const EquivalenceClasses &operator=(const EquivalenceClasses &rhs)
   {
      m_theMapping.clear();
      for (iterator iter = rhs.begin(), end = rhs.end(); iter != end; ++iter) {
         if (iter->isLeader()) {
            MemberIterator miter = rhs.memberBegin(iter);
            MemberIterator leaderIt = memberBegin(insert(*miter));
            for (++miter; miter != memberEnd(); ++miter) {
               unionSets(leaderIt, memberBegin(insert(*miter)));
            }
         }
      }
      return *this;
   }

   //===--------------------------------------------------------------------===//
   // Inspection methods
   //

   /// iterator* - Provides a way to iterate over all values in the set.
   using iterator = typename std::set<ECValue>::const_iterator;

   iterator begin() const { return m_theMapping.begin(); }
   iterator end() const { return m_theMapping.end(); }

   bool empty() const { return m_theMapping.empty(); }

   /// member_* Iterate over the members of an equivalence class.
   class MemberIterator;
   MemberIterator memberBegin(iterator I) const {
      // Only leaders provide anything to iterate over.
      return MemberIterator(I->isLeader() ? &*I : nullptr);
   }
   MemberIterator memberEnd() const
   {
      return MemberIterator(nullptr);
   }

   /// findValue - Return an iterator to the specified value.  If it does not
   /// exist, end() is returned.
   iterator findValue(const ElemType &value) const
   {
      return m_theMapping.find(value);
   }

   /// getLeaderValue - Return the leader for the specified value that is in the
   /// set.  It is an error to call this method for a value that is not yet in
   /// the set.  For that, call getOrInsertLeaderValue(V).
   const ElemType &getLeaderValue(const ElemType &value) const
   {
      MemberIterator miter = findLeader(value);
      assert(miter != memberEnd() && "Value is not in the set!");
      return *miter;
   }

   /// getOrInsertLeaderValue - Return the leader for the specified value that is
   /// in the set.  If the member is not in the set, it is inserted, then
   /// returned.
   const ElemType &getOrInsertLeaderValue(const ElemType &value)
   {
      MemberIterator miter = findLeader(insert(value));
      assert(miter != memberEnd() && "Value is not in the set!");
      return *miter;
   }

   /// getNumClasses - Return the number of equivalence classes in this set.
   /// Note that this is a linear time operation.
   unsigned getNumClasses() const
   {
      unsigned nc = 0;
      for (iterator iter = begin(), end = end(); iter != end; ++iter) {
         if (iter->isLeader()) {
            ++nc;
         }
      }

      return nc;
   }

   //===--------------------------------------------------------------------===//
   // Mutation methods

   /// insert - Insert a new value into the union/find set, ignoring the request
   /// if the value already exists.
   iterator insert(const ElemType &data)
   {
      return m_theMapping.insert(ECValue(data)).first;
   }

   /// findLeader - Given a value in the set, return a member iterator for the
   /// equivalence class it is in.  This does the path-compression part that
   /// makes union-find "union findy".  This returns an end iterator if the value
   /// is not in the equivalence class.
   MemberIterator findLeader(iterator iter) const
   {
      if (iter == m_theMapping.end()) {
         return memberEnd();
      }
      return MemberIterator(iter->getLeader());
   }

   MemberIterator findLeader(const ElemType &value) const
   {
      return findLeader(m_theMapping.find(value));
   }

   /// union - Merge the two equivalence sets for the specified values, inserting
   /// them if they do not already exist in the equivalence set.
   MemberIterator unionSets(const ElemType &lhs, const ElemType &rhs)
   {
      iterator lhsIter = insert(lhs), rhsIter = insert(rhs);
      return unionSets(findLeader(lhsIter), findLeader(rhsIter));
   }

   MemberIterator unionSets(MemberIterator lhs, MemberIterator rhs)
   {
      assert(lhs != memberEnd() && rhs != memberEnd() && "Illegal inputs!");
      if (lhs == rhs) {
         return lhs;   // Unifying the same two sets, noop.
      }

      // Otherwise, this is a real union operation.  Set the end of the lhs list to
      // point to the rhs leader node.
      const ECValue &lhsLV = *lhs.m_node, &rhsLV = *rhs.m_node;
      lhsLV.getEndOfList()->setNext(&rhsLV);

      // Update lhsLV's end of list pointer.
      lhsLV.m_leader = rhsLV.getEndOfList();

      // Clear rhs's leader flag:
      rhsLV.m_next = rhsLV.getNext();

      // rhs's leader is now lhs.
      rhsLV.m_leader = &lhsLV;
      return lhs;
   }

   // isEquivalent - Return true if lhs is equivalent to rhs. This can happen if
   // lhs is equal to rhs or if they belong to one equivalence class.
   bool isEquivalent(const ElemType &lhs, const ElemType &rhs) const
   {
      // Fast path: any element is equivalent to itself.
      if (lhs == rhs) {
         return true;
      }
      auto iter = findLeader(lhs);
      return iter != memberEnd() && iter == findLeader(rhs);
   }

   class MemberIterator : public std::iterator<std::forward_iterator_tag,
         const ElemType, ptrdiff_t>
   {
      friend class EquivalenceClasses;

      using super = std::iterator<std::forward_iterator_tag,
      const ElemType, ptrdiff_t>;

      const ECValue *m_node;

   public:
      using size_type = size_t;
      using pointer = typename super::pointer;
      using reference = typename super::reference;

      explicit MemberIterator() = default;
      explicit MemberIterator(const ECValue *node) : m_node(node)
      {}

      reference operator*() const
      {
         assert(m_node != nullptr && "Dereferencing end()!");
         return m_node->getData();
      }
      pointer operator->() const { return &operator*(); }

      MemberIterator &operator++()
      {
         assert(m_node != nullptr && "++'d off the end of the list!");
         m_node = m_node->getNext();
         return *this;
      }

      MemberIterator operator++(int)
      {    // postincrement operators.
         MemberIterator tmp = *this;
         ++*this;
         return tmp;
      }

      bool operator==(const MemberIterator &rhs) const
      {
         return m_node == rhs.m_node;
      }

      bool operator!=(const MemberIterator &rhs) const
      {
         return m_node != rhs.m_node;
      }
   };
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_EQUIVALENCE_CLASSES_H
