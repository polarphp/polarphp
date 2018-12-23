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

#ifndef POLARPHP_BASIC_ADT_SPARSE_MULTI_SET_H
#define POLARPHP_BASIC_ADT_SPARSE_MULTI_SET_H

#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/SparseSet.h"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <utility>

namespace polar {
namespace basic {

/// Fast multiset implementation for objects that can be identified by small
/// unsigned keys.
///
/// SparseMultiSet allocates memory proportional to the size of the key
/// universe, so it is not recommended for building composite data structures.
/// It is useful for algorithms that require a single set with fast operations.
///
/// Compared to DenseSet and DenseMap, SparseMultiSet provides constant-time
/// fast clear() as fast as a vector.  The find(), insert(), and erase()
/// operations are all constant time, and typically faster than a hash table.
/// The iteration order doesn't depend on numerical key values, it only depends
/// on the order of insert() and erase() operations.  Iteration order is the
/// insertion order. Iteration is only provided over elements of equivalent
/// keys, but iterators are bidirectional.
///
/// Compared to BitVector, SparseMultiSet<unsigned> uses 8x-40x more memory, but
/// offers constant-time clear() and size() operations as well as fast iteration
/// independent on the size of the universe.
///
/// SparseMultiSet contains a dense vector holding all the objects and a sparse
/// array holding indexes into the dense vector.  Most of the memory is used by
/// the sparse array which is the size of the key universe. The SparseT template
/// parameter provides a space/speed tradeoff for sets holding many elements.
///
/// When SparseT is uint32_t, find() only touches up to 3 cache lines, but the
/// sparse array uses 4 x Universe bytes.
///
/// When SparseT is uint8_t (the default), find() touches up to 3+[N/256] cache
/// lines, but the sparse array is 4x smaller.  N is the number of elements in
/// the set.
///
/// For sets that may grow to thousands of elements, SparseT should be set to
/// uint16_t or uint32_t.
///
/// Multiset behavior is provided by providing doubly linked lists for values
/// that are inlined in the dense vector. SparseMultiSet is a good choice when
/// one desires a growable number of entries per key, as it will retain the
/// SparseSet algorithmic properties despite being growable. Thus, it is often a
/// better choice than a SparseSet of growable containers or a vector of
/// vectors. SparseMultiSet also keeps iterators valid after erasure (provided
/// the iterators don't point to the element erased), allowing for more
/// intuitive and fast removal.
///
/// @tparam ValueT      The type of objects in the set.
/// @tparam KeyFunctorT A functor that computes an unsigned index from KeyT.
/// @tparam SparseT     An unsigned integer type. See above.
///
template<typename ValueT,
         typename KeyFunctorT = Identity<unsigned>,
         typename SparseT = uint8_t>
class SparseMultiSet
{
   static_assert(std::numeric_limits<SparseT>::is_integer &&
                 !std::numeric_limits<SparseT>::is_signed,
                 "SparseT must be an unsigned integer type");

   /// The actual data that's stored, as a doubly-linked list implemented via
   /// indices into the DenseVector.  The doubly linked list is implemented
   /// circular in Prev indices, and INVALID-terminated in Next indices. This
   /// provides efficient access to list tails. These nodes can also be
   /// tombstones, in which case they are actually nodes in a single-linked
   /// freelist of recyclable slots.
   struct SMSNode
   {
      static const unsigned INVALID = ~0U;

      ValueT m_data;
      unsigned m_prev;
      unsigned m_next;

      SMSNode(ValueT data, unsigned pred, unsigned next) : m_data(data), m_prev(pred), m_next(next)
      {}

      /// List tails have invalid Nexts.
      bool isTail() const
      {
         return m_next == INVALID;
      }

      /// Whether this node is a tombstone node, and thus is in our freelist.
      bool isTombstone() const
      {
         return m_prev == INVALID;
      }

      /// Since the list is circular in Prev, all non-tombstone nodes have a valid
      /// Prev.
      bool isValid() const
      {
         return m_prev != INVALID;
      }
   };

   using KeyT = typename KeyFunctorT::ArgumentType;
   using DenseT = SmallVector<SMSNode, 8>;
   DenseT m_dense;
   SparseT *m_sparse = nullptr;
   unsigned m_universe = 0;
   KeyFunctorT m_keyIndexOf;
   SparseSetValFunctor<KeyT, ValueT, KeyFunctorT> m_valIndexOf;

   /// We have a built-in recycler for reusing tombstone slots. This recycler
   /// puts a singly-linked free list into tombstone slots, allowing us quick
   /// erasure, iterator preservation, and dense size.
   unsigned m_freelistIdx = SMSNode::INVALID;
   unsigned m_numFree = 0;

   unsigned sparseIndex(const ValueT &value) const
   {
      assert(m_valIndexOf(value) < m_universe &&
             "Invalid key in set. Did object mutate?");
      return m_valIndexOf(value);
   }

   unsigned sparseIndex(const SMSNode &node) const
   {
      return sparseIndex(node.m_data);
   }

   /// Whether the given entry is the head of the list. List heads's previous
   /// pointers are to the tail of the list, allowing for efficient access to the
   /// list tail. D must be a valid entry node.
   bool isHead(const SMSNode &node) const
   {
      assert(node.isValid() && "Invalid node for head");
      return m_dense[node.m_prev].isTail();
   }

   /// Whether the given entry is a singleton entry, i.e. the only entry with
   /// that key.
   bool isSingleton(const SMSNode &node) const
   {
      assert(node.isValid() && "Invalid node for singleton");
      // Is N its own predecessor?
      return &m_dense[node.m_prev] == &node;
   }

   /// Add in the given SMSNode. Uses a free entry in our freelist if
   /// available. Returns the index of the added node.
   unsigned addValue(const ValueT& value, unsigned prev, unsigned next)
   {
      if (m_numFree == 0) {
         m_dense.push_back(SMSNode(value, prev, next));
         return m_dense.size() - 1;
      }

      // Peel off a free slot
      unsigned idx = m_freelistIdx;
      unsigned nextFree = m_dense[idx].m_next;
      assert(m_dense[idx].isTombstone() && "Non-tombstone free?");

      m_dense[idx] = SMSNode(value, prev, next);
      m_freelistIdx = nextFree;
      --m_numFree;
      return idx;
   }

   /// Make the current index a new tombstone. Pushes it onto the freelist.
   void makeTombstone(unsigned idx)
   {
      m_dense[idx].m_prev = SMSNode::INVALID;
      m_dense[idx].m_next = m_freelistIdx;
      m_freelistIdx = idx;
      ++m_numFree;
   }

public:
   using value_type = ValueT;
   using reference = ValueT &;
   using const_reference = const ValueT &;
   using pointer = ValueT *;
   using const_pointer = const ValueT *;
   using size_type = unsigned;

   SparseMultiSet() = default;
   SparseMultiSet(const SparseMultiSet &) = delete;
   SparseMultiSet &operator=(const SparseMultiSet &) = delete;
   ~SparseMultiSet()
   {
      free(m_sparse);
   }

   /// Set the universe size which determines the largest key the set can hold.
   /// The universe must be sized before any elements can be added.
   ///
   /// @param U Universe size. All object keys must be less than U.
   ///
   void setUniverse(unsigned universe)
   {
      // It's not hard to resize the universe on a non-empty set, but it doesn't
      // seem like a likely use case, so we can add that code when we need it.
      assert(empty() && "Can only resize universe on an empty map");
      // Hysteresis prevents needless reallocations.
      if (universe >= m_universe/4 && universe <= m_universe) {
         return;
      }
      free(m_sparse);
      // The Sparse array doesn't actually need to be initialized, so malloc
      // would be enough here, but that will cause tools like valgrind to
      // complain about branching on uninitialized data.
      m_sparse = static_cast<SparseT*>(polar::utils::safe_calloc(universe, sizeof(SparseT)));
      m_universe = universe;
   }

   /// Our iterators are iterators over the collection of objects that share a
   /// key.
   template<typename SMSPtrTy>
   class IteratorBase : public std::iterator<std::bidirectional_iterator_tag,
         ValueT>
   {
      friend class SparseMultiSet;

      SMSPtrTy m_sms;
      unsigned m_idx;
      unsigned m_sparseIdx;

      IteratorBase(SMSPtrTy ptr, unsigned idx, unsigned sidx)
         : m_sms(ptr), m_idx(idx), m_sparseIdx(sidx)
      {}

      /// Whether our iterator has fallen outside our dense vector.
      bool isEnd() const
      {
         if (m_idx == SMSNode::INVALID) {
            return true;
         }
         assert(m_idx < m_sms->m_dense.size() && "Out of range, non-INVALID Idx?");
         return false;
      }

      /// Whether our iterator is properly keyed, i.e. the SparseIdx is valid
      bool isKeyed() const
      {
         return m_sparseIdx < m_sms->m_universe;
      }

      unsigned getPrev() const
      {
         return m_sms->m_dense[m_idx].m_prev;
      }

      unsigned getNext() const
      {
         return m_sms->m_dense[m_idx].m_next;
      }

      void setPrev(unsigned prev)
      {
         m_sms->m_dense[m_idx].m_prev = prev;
      }

      void setNext(unsigned next)
      {
         m_sms->m_dense[m_idx].m_next = next;
      }

   public:
      using super = std::iterator<std::bidirectional_iterator_tag, ValueT>;
      using value_type = typename super::value_type;
      using difference_type = typename super::difference_type;
      using pointer = typename super::pointer;
      using reference = typename super::reference;

      reference operator*() const
      {
         assert(isKeyed() && m_sms->sparseIndex(m_sms->m_dense[m_idx].m_data) == m_sparseIdx &&
                "Dereferencing iterator of invalid key or index");

         return m_sms->m_dense[m_idx].m_data;
      }

      pointer operator->() const
      {
         return &operator*();
      }

      /// Comparison operators
      bool operator==(const IteratorBase &other) const
      {
         // end compares equal
         if (m_sms == other.m_sms && m_idx == other.m_idx) {
            assert((isEnd() || m_sparseIdx == other.m_sparseIdx) &&
                   "Same dense entry, but different keys?");
            return true;
         }

         return false;
      }

      bool operator!=(const IteratorBase &other) const
      {
         return !operator==(other);
      }

      /// Increment and decrement operators
      IteratorBase &operator--()
      { // predecrement - Back up
         assert(isKeyed() && "Decrementing an invalid iterator");
         assert((isEnd() || !m_sms->isHead(m_sms->m_dense[m_idx])) &&
                "Decrementing head of list");

         // If we're at the end, then issue a new find()
         if (isEnd()) {
            m_idx = m_sms->findIndex(m_sparseIdx).getPrev();
         } else {
            m_idx = getPrev();
         }
         return *this;
      }

      IteratorBase &operator++()
      { // preincrement - Advance
         assert(!isEnd() && isKeyed() && "Incrementing an invalid/end iterator");
         m_idx = getNext();
         return *this;
      }

      IteratorBase operator--(int)
      { // postdecrement
         IteratorBase iter(*this);
         --*this;
         return iter;
      }

      IteratorBase operator++(int)
      { // postincrement
         IteratorBase iter(*this);
         ++*this;
         return iter;
      }
   };

   using iterator = IteratorBase<SparseMultiSet *>;
   using const_iterator = IteratorBase<const SparseMultiSet *>;

   // Convenience types
   using RangePair = std::pair<iterator, iterator>;

   /// Returns an iterator past this container. Note that such an iterator cannot
   /// be decremented, but will compare equal to other end iterators.
   iterator end()
   {
      return iterator(this, SMSNode::INVALID, SMSNode::INVALID);
   }

   const_iterator end() const
   {
      return const_iterator(this, SMSNode::INVALID, SMSNode::INVALID);
   }

   /// Returns true if the set is empty.
   ///
   /// This is not the same as BitVector::empty().
   ///
   bool empty() const
   {
      return size() == 0;
   }

   /// Returns the number of elements in the set.
   ///
   /// This is not the same as BitVector::size() which returns the size of the
   /// universe.
   ///
   size_type size() const
   {
      assert(m_numFree <= m_dense.getSize() && "Out-of-bounds free entries");
      return m_dense.size() - m_numFree;
   }

   /// Clears the set.  This is a very fast constant time operation.
   ///
   void clear()
   {
      // Sparse does not need to be cleared, see find().
      m_dense.clear();
      m_numFree = 0;
      m_freelistIdx = SMSNode::INVALID;
   }

   /// Find an element by its index.
   ///
   /// @param   Idx A valid index to find.
   /// @returns An iterator to the element identified by key, or end().
   ///
   iterator findIndex(unsigned idx)
   {
      assert(idx < m_universe && "Key out of range");
      const unsigned stride = std::numeric_limits<SparseT>::max() + 1u;
      for (unsigned i = m_sparse[idx], e = m_dense.getSize(); i < e; i += stride) {
         const unsigned foundIdx = sparseIndex(m_dense[i]);
         // Check that we're pointing at the correct entry and that it is the head
         // of a valid list.
         if (idx == foundIdx && m_dense[i].isValid() && isHead(m_dense[i])) {
            return iterator(this, i, idx);
         }
         // stride is 0 when SparseT >= unsigned.  We don't need to loop.
         if (!stride) {
            break;
         }
      }
      return end();
   }

   /// Find an element by its key.
   ///
   /// @param   Key A valid key to find.
   /// @returns An iterator to the element identified by key, or end().
   ///
   iterator find(const KeyT &key)
   {
      return findIndex(m_keyIndexOf(key));
   }

   const_iterator find(const KeyT &key) const
   {
      iterator iter = const_cast<SparseMultiSet*>(this)->findIndex(m_keyIndexOf(key));
      return const_iterator(iter.m_sms, iter.m_idx, m_keyIndexOf(key));
   }

   /// Returns the number of elements identified by Key. This will be linear in
   /// the number of elements of that key.
   size_type count(const KeyT &key) const
   {
      unsigned ret = 0;
      for (const_iterator iter = find(key); iter != end(); ++iter) {
         ++ret;
      }
      return ret;
   }

   /// Returns true if this set contains an element identified by Key.
   bool contains(const KeyT &key) const
   {
      return find(key) != end();
   }

   /// Return the head and tail of the subset's list, otherwise returns end().
   iterator getHead(const KeyT &key)
   {
      return find(key);
   }

   iterator getTail(const KeyT &key)
   {
      iterator iter = find(key);
      if (iter != end()) {
         iter = iterator(this, iter.getPrev(), m_keyIndexOf(key));
      }
      return iter;
   }

   /// The bounds of the range of items sharing Key K. First member is the head
   /// of the list, and the second member is a decrementable end iterator for
   /// that key.
   RangePair equalRange(const KeyT &key)
   {
      iterator begin = find(key);
      iterator end = iterator(this, SMSNode::INVALID, begin.m_sparseIdx);
      return std::make_pair(begin, end);
   }

   /// Insert a new element at the tail of the subset list. Returns an iterator
   /// to the newly added entry.
   iterator insert(const ValueT &value)
   {
      unsigned idx = sparseIndex(value);
      iterator iter = findIndex(idx);
      unsigned nodeIdx = addValue(value, SMSNode::INVALID, SMSNode::INVALID);

      if (iter == end()) {
         // Make a singleton list
         m_sparse[idx] = nodeIdx;
         m_dense[nodeIdx].m_prev = nodeIdx;
         return iterator(this, nodeIdx, idx);
      }

      // Stick it at the end.
      unsigned headIdx = iter.m_idx;
      unsigned tailIdx = iter.getPrev();
      m_dense[tailIdx].m_next = nodeIdx;
      m_dense[headIdx].m_prev = nodeIdx;
      m_dense[nodeIdx].m_prev = tailIdx;
      return iterator(this, nodeIdx, idx);
   }

   /// Erases an existing element identified by a valid iterator.
   ///
   /// This invalidates iterators pointing at the same entry, but erase() returns
   /// an iterator pointing to the next element in the subset's list. This makes
   /// it possible to erase selected elements while iterating over the subset:
   ///
   ///   tie(I, E) = Set.equalRange(Key);
   ///   while (I != E)
   ///     if (test(*I))
   ///       I = Set.erase(I);
   ///     else
   ///       ++I;
   ///
   /// Note that if the last element in the subset list is erased, this will
   /// return an end iterator which can be decremented to get the new tail (if it
   /// exists):
   ///
   ///  tie(B, I) = Set.equalRange(Key);
   ///  for (bool isBegin = B == I; !isBegin; /* empty */) {
   ///    isBegin = (--I) == B;
   ///    if (test(I))
   ///      break;
   ///    I = erase(I);
   ///  }
   iterator erase(iterator iter)
   {
      assert(iter.isKeyed() && !iter.isEnd() && !m_dense[iter.m_idx].isTombstone() &&
            "erasing invalid/end/tombstone iterator");
      // First, unlink the node from its list. Then swap the node out with the
      // dense vector's last entry
      iterator nextIter = unlink(m_dense[iter.m_idx]);
      // Put in a tombstone.
      makeTombstone(iter.m_idx);
      return nextIter;
   }

   /// Erase all elements with the given key. This invalidates all
   /// iterators of that key.
   void eraseAll(const KeyT &key)
   {
      for (iterator iter = find(key); iter != end(); /* empty */) {
         iter = erase(iter);
      }
   }

private:
   /// Unlink the node from its list. Returns the next node in the list.
   iterator unlink(const SMSNode &node)
   {
      if (isSingleton(node)) {
         // Singleton is already unlinked
         assert(node.m_next == SMSNode::INVALID && "Singleton has next?");
         return iterator(this, SMSNode::INVALID, m_valIndexOf(node.m_data));
      }
      if (isHead(node)) {
         // If we're the head, then update the sparse array and our next.
         m_sparse[sparseIndex(node)] = node.m_next;
         m_dense[node.m_next].m_prev = node.m_prev;
         return iterator(this, node.m_next, m_valIndexOf(node.m_data));
      }

      if (node.isTail()) {
         // If we're the tail, then update our head and our previous.
         findIndex(sparseIndex(node)).setPrev(node.m_prev);
         m_dense[node.m_prev].m_next = node.m_next;
         // Give back an end iterator that can be decremented
         iterator iter(this, node.m_prev, m_valIndexOf(node.m_data));
         return ++iter;
      }

      // Otherwise, just drop us
      m_dense[node.m_next].m_prev = node.m_prev;
      m_dense[node.m_prev].m_next = node.m_next;
      return iterator(this, node.m_next, m_valIndexOf(node.m_data));
   }
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_SPARSE_MULTI_SET_H
