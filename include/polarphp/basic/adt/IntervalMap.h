// This source file is part of the polarphpath.org open source project
//
// Copyright (c) 2017 - 2018  polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphpath.org/LICENSE.txt for license information
// See http://polarphpath.org/CONTRIBUTORS.txt for the list of  polarphp project authors
//
// Created by polarboy on 2018/06/24.

//===----------------------------------------------------------------------===//
//
// This file implements a coalescing interval map for small objects.
//
// KeyT objects are mapped to ValT objects. Intervals of keys that map to the
// same value are represented in a compressed form.
//
// Iterators provide ordered access to the compressed intervals rather than the
// individual keys, and insert and erase operations use key intervals as well.
//
// Like SmallVector, IntervalMap will store the first N intervals in the map
// object itself without any allocations. When space is exhausted it switches to
// a B+-tree representation with very small overhead for small key and value
// objects.
//
// A Traits class specifies how keys are compared. It also allows IntervalMap to
// work with both closed and half-open intervals.
//
// Keys and values are not stored next to each other in a std::pair, so we don't
// provide such a value_type. Dereferencing iterators only returns the mapped
// value. The interval bounds are accessible through the start() and stop()
// iterator methods.
//
// IntervalMap is optimized for small key and value objects, 4 or 8 bytes each
// is the optimal size. For large objects use std::map instead.
//
//===----------------------------------------------------------------------===//
//
// Synopsis:
//
// template <typename KeyT, typename ValT, unsigned N, typename Traits>
// class IntervalMap {
// public:
//   typedef KeyT key_type;
//   typedef ValT mapped_type;
//   typedef RecyclingAllocator<...> Allocator;
//   class iterator;
//   class const_iterator;
//
//   explicit IntervalMap(Allocator&);
//   ~IntervalMap():
//
//   bool empty() const;
//   KeyT start() const;
//   KeyT stop() const;
//   ValT lookup(KeyT x, Value NotFound = Value()) const;
//
//   const_iterator begin() const;
//   const_iterator end() const;
//   iterator begin();
//   iterator end();
//   const_iterator find(KeyT x) const;
//   iterator find(KeyT x);
//
//   void insert(KeyT a, KeyT b, ValT y);
//   void clear();
// };
//
// template <typename KeyT, typename ValT, unsigned N, typename Traits>
// class IntervalMap::const_iterator :
//   public std::iterator<std::bidirectional_iterator_tag, ValT> {
// public:
//   bool operator==(const const_iterator &) const;
//   bool operator!=(const const_iterator &) const;
//   bool valid() const;
//
//   const KeyT &start() const;
//   const KeyT &stop() const;
//   const ValT &value() const;
//   const ValT &operator*() const;
//   const ValT *operator->() const;
//
//   const_iterator &operator++();
//   const_iterator &operator++(int);
//   const_iterator &operator--();
//   const_iterator &operator--(int);
//   void goToBegin();
//   void goToEnd();
//   void find(KeyT x);
//   void advanceTo(KeyT x);
// };
//
// template <typename KeyT, typename ValT, unsigned N, typename Traits>
// class IntervalMap::iterator : public const_iterator {
// public:
//   void insert(KeyT a, KeyT b, Value y);
//   void erase();
// };
//
//===----------------------------------------------------------------------===//

//
#ifndef  POLARPHP_BASIC_ADT_INTERVAL_MAP_H
#define  POLARPHP_BASIC_ADT_INTERVAL_MAP_H

#include "polarphp/basic/adt/PointerIntPair.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/utils/AlignOf.h"
#include "polarphp/utils/Allocator.h"
#include "polarphp/utils/RecyclingAllocator.h"
#include "polarphp/basic/adt/Bit.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <new>
#include <utility>

namespace polar {
namespace basic {

using polar::utils::RecyclingAllocator;
using polar::utils::BumpPtrAllocator;
using polar::utils::AlignedCharArrayUnion;

//===----------------------------------------------------------------------===//
//---                              Key traits                              ---//
//===----------------------------------------------------------------------===//
//
// The IntervalMap works with closed or half-open intervals.
// Adjacent intervals that map to the same value are coalesced.
//
// The IntervalMapInfo traits class is used to determine if a key is contained
// in an interval, and if two intervals are adjacent so they can be coalesced.
// The provided implementation works for closed integer intervals, other keys
// probably need a specialized version.
//
// The point x is contained in [a;b] when !startLess(x, a) && !stopLess(b, x).
//
// It is assumed that (a;b] half-open intervals are not used, only [a;b) is
// allowed. This is so that stopLess(a, b) can be used to determine if two
// intervals overlapath.
//
//===----------------------------------------------------------------------===//

template <typename T>
struct IntervalMapInfo
{
   /// startLess - Return true if x is not in [a;b].
   /// This is x < a both for closed intervals and for [a;b) half-open intervals.
   static inline bool startLess(const T &x, const T &a)
   {
      return x < a;
   }

   /// stopLess - Return true if x is not in [a;b].
   /// This is b < x for a closed interval, b <= x for [a;b) half-open intervals.
   static inline bool stopLess(const T &b, const T &x)
   {
      return b < x;
   }

   /// adjacent - Return true when the intervals [x;a] and [b;y] can coalesce.
   /// This is a+1 == b for closed intervals, a == b for half-open intervals.
   static inline bool adjacent(const T &a, const T &b)
   {
      return a + 1 == b;
   }

   /// nonEmpty - Return true if [a;b] is non-empty.
   /// This is a <= b for a closed interval, a < b for [a;b) half-open intervals.
   static inline bool nonEmpty(const T &a, const T &b)
   {
      return a <= b;
   }
};

template <typename T>
struct IntervalMapHalfOpenInfo
{
   /// startLess - Return true if x is not in [a;b).
   static inline bool startLess(const T &x, const T &a)
   {
      return x < a;
   }

   /// stopLess - Return true if x is not in [a;b).
   static inline bool stopLess(const T &b, const T &x)
   {
      return b <= x;
   }

   /// adjacent - Return true when the intervals [x;a) and [b;y) can coalesce.
   static inline bool adjacent(const T &a, const T &b)
   {
      return a == b;
   }

   /// nonEmpty - Return true if [a;b) is non-empty.
   static inline bool nonEmpty(const T &a, const T &b)
   {
      return a < b;
   }
};

/// intervalmapimpl - Namespace used for IntervalMap implementation details.
/// It should be considered private to the implementation.
namespace intervalmapimpl
{

using IdxPair = std::pair<unsigned,unsigned>;

//===----------------------------------------------------------------------===//
//---                    intervalmapimpl::NodeBase                         ---//
//===----------------------------------------------------------------------===//
//
// Both leaf and branch nodes store vectors of pairs.
// Leaves store ((KeyType, KeyType), ValueType) pairs, branches use (NodeRef, KeyType).
//
// Keys and values are stored in separate arrays to avoid padding caused by
// different object alignments. This also helps improve locality of reference
// when searching the keys.
//
// The nodes don't know how many elements they contain - that information is
// stored elsewhere. Omitting the size field prevents padding and allows a node
// to fill the allocated cache lines completely.
//
// These are typical key and value sizes, the node branching factor (N), and
// wasted space when nodes are sized to fit in three cache lines (192 bytes):
//
//   T1  T2   N Waste  Used by
//    4   4  24   0    Branch<4> (32-bit pointers)
//    8   4  16   0    Leaf<4,4>, Branch<4>
//    8   8  12   0    Leaf<4,8>, Branch<8>
//   16   4   9  12    Leaf<8,4>
//   16   8   8   0    Leaf<8,8>
//
//===----------------------------------------------------------------------===//

template <typename T1, typename T2, unsigned N>
class NodeBase
{
public:
   enum { Capacity = N };

   T1 m_first[N];
   T2 m_second[N];

   /// copy - Copy elements from another node.
   /// @param Other Node elements are copied from.
   /// @param i     Beginning of the source range in other.
   /// @param j     Beginning of the destination range in this.
   /// @param count Number of elements to copy.
   template <unsigned M>
   void copy(const NodeBase<T1, T2, M> &other, unsigned i,
             unsigned j, unsigned count)
   {
      assert(i + count <= M && "Invalid source range");
      assert(j + count <= N && "Invalid dest range");
      for (unsigned e = i + count; i != e; ++i, ++j) {
         m_first[j]  = other.m_first[i];
         m_second[j] = other.m_second[i];
      }
   }

   /// moveLeft - Move elements to the left.
   /// @param i     Beginning of the source range.
   /// @param j     Beginning of the destination range.
   /// @param count Number of elements to copy.
   void moveLeft(unsigned i, unsigned j, unsigned count)
   {
      assert(j <= i && "Use moveRight shift elements right");
      copy(*this, i, j, count);
   }

   /// moveRight - Move elements to the right.
   /// @param i     Beginning of the source range.
   /// @param j     Beginning of the destination range.
   /// @param count Number of elements to copy.
   void moveRight(unsigned i, unsigned j, unsigned count)
   {
      assert(i <= j && "Use moveLeft shift elements left");
      assert(j + count <= N && "Invalid range");
      while (count--) {
         m_first[j + count]  = m_first[i + count];
         m_second[j + count] = m_second[i + count];
      }
   }

   /// erase - Erase elements [i;j).
   /// @param i    Beginning of the range to erase.
   /// @param j    End of the range. (Exclusive).
   /// @param Size Number of elements in node.
   void erase(unsigned i, unsigned j, unsigned size)
   {
      moveLeft(j, i, size - j);
   }

   /// erase - Erase element at i.
   /// @param i    Index of element to erase.
   /// @param Size Number of elements in node.
   void erase(unsigned i, unsigned size)
   {
      erase(i, i+1, size);
   }

   /// shift - Shift elements [i;size) 1 position to the right.
   /// @param i    Beginning of the range to move.
   /// @param Size Number of elements in node.
   void shift(unsigned i, unsigned size)
   {
      moveRight(i, i + 1, size - i);
   }

   /// transferToleftSib - Transfer elements to a left sibling node.
   /// @param Size  Number of elements in this.
   /// @param Sib   Left sibling node.
   /// @param ssize Number of elements in sib.
   /// @param count Number of elements to transfer.
   void transferToleftSib(unsigned size, NodeBase &sib, unsigned ssize,
                          unsigned count)
   {
      sib.copy(*this, 0, ssize, count);
      erase(0, count, size);
   }

   /// transferTorightSib - Transfer elements to a right sibling node.
   /// @param Size  Number of elements in this.
   /// @param Sib   Right sibling node.
   /// @param ssize Number of elements in sib.
   /// @param count Number of elements to transfer.
   void transferTorightSib(unsigned size, NodeBase &sib, unsigned ssize,
                           unsigned count)
   {
      sib.moveRight(0, count, ssize);
      sib.copy(*this, size - count, 0, count);
   }

   /// adjustFromleftSib - Adjust the number if elements in this node by moving
   /// elements to or from a left sibling node.
   /// @param Size  Number of elements in this.
   /// @param Sib   Right sibling node.
   /// @param ssize Number of elements in sib.
   /// @param Add   The number of elements to add to this node, possibly < 0.
   /// @return      Number of elements added to this node, possibly negative.
   int adjustFromleftSib(unsigned size, NodeBase &sib, unsigned ssize, int add)
   {
      if (add > 0) {
         // We want to grow, copy from sib.
         unsigned count = std::min(std::min(unsigned(add), ssize), N - size);
         sib.transferTorightSib(ssize, *this, size, count);
         return count;
      } else {
         // We want to shrink, copy to sib.
         unsigned count = std::min(std::min(unsigned(-add), size), N - ssize);
         transferToleftSib(size, sib, ssize, count);
         return -count;
      }
   }
};

/// intervalmapimpl::adjust_sibling_sizes - Move elements between sibling nodes.
/// @param Node  Array of pointers to sibling nodes.
/// @param nodes Number of nodes.
/// @param curSize Array of current node sizes, will be overwritten.
/// @param newSize Array of desired node sizes.
template <typename NodeType>
void adjust_sibling_sizes(NodeType *node[], unsigned nodes,
                          unsigned curSize[], const unsigned newSize[])
{
   // Move elements right.
   for (int n = nodes - 1; n; --n) {
      if (curSize[n] == newSize[n]) {
         continue;
      }
      for (int m = n - 1; m != -1; --m) {
         int d = node[n]->adjustFromleftSib(curSize[n], *node[m], curSize[m],
                                            newSize[n] - curSize[n]);
         curSize[m] -= d;
         curSize[n] += d;
         // Keep going if the current node was exhausted.
         if (curSize[n] >= newSize[n]) {
            break;
         }
      }
   }

   if (nodes == 0) {
      return;
   }
   // Move elements left.
   for (unsigned n = 0; n != nodes - 1; ++n) {
      if (curSize[n] == newSize[n]) {
         continue;
      }
      for (unsigned m = n + 1; m != nodes; ++m) {
         int d = node[m]->adjustFromleftSib(curSize[m], *node[n], curSize[n],
                                            curSize[n] -  newSize[n]);
         curSize[m] += d;
         curSize[n] -= d;
         // Keep going if the current node was exhausted.
         if (curSize[n] >= newSize[n]) {
            break;
         }
      }
   }

#ifndef NDEBUG
   for (unsigned n = 0; n != nodes; n++) {
      assert(curSize[n] == newSize[n] && "Insufficient element shuffle");
   }
#endif
}

/// intervalmapimpl::distribute - Compute a new distribution of node elements
/// after an overflow or underflow. Reserve space for a new element at position,
/// and compute the node that will hold position after redistributing node
/// elements.
///
/// It is required that
///
///   elements == sum(curSize), and
///   elements + Grow <= nodes * Capacity.
///
/// newSize[] will be filled in such that:
///
///   sum(newSize) == elements, and
///   newSize[i] <= Capacity.
///
/// The returned index is the node where position will go, so:
///
///   sum(newSize[0..idx-1]) <= position
///   sum(newSize[0..idx])   >= position
///
/// The last equality, sum(newSize[0..idx]) == position, can only happen when
/// Grow is set and newSize[idx] == Capacity-1. The index points to the node
/// before the one holding the position'th element where there is room for an
/// insertion.
///
/// @param nodes    The number of nodes.
/// @param elements Total elements in all nodes.
/// @param Capacity The capacity of each node.
/// @param curSize  Array[nodes] of current node sizes, or NULL.
/// @param newSize  Array[nodes] to receive the new node sizes.
/// @param position Insert position.
/// @param Grow     Reserve space for a new element at position.
/// @return         (node, offset) for position.
IdxPair distribute(unsigned nodes, unsigned elements, unsigned capacity,
                   const unsigned *curSize, unsigned newSize[],
                   unsigned position, bool grow);

//===----------------------------------------------------------------------===//
//---                   intervalmapimpl:: NodeSizer                         ---//
//===----------------------------------------------------------------------===//
//
// Compute node sizes from key and value types.
//
// The branching factors are chosen to make nodes fit in three cache lines.
// This may not be possible if keys or values are very large. Such large objects
// are handled correctly, but a std::map would probably give better performance.
//
//===----------------------------------------------------------------------===//

enum {
   // Cache line size. Most architectures have 32 or 64 byte cache lines.
   // We use 64 bytes here because it provides good branching factors.
   Log2CacheLine = 6,
   CacheLineBytes = 1 << Log2CacheLine,
   DesiredNodeBytes = 3 * CacheLineBytes
};

template <typename KeyType, typename ValueType>
struct  NodeSizer
{
   enum {
      // Compute the leaf node branching factor that makes a node fit in three
      // cache lines. The branching factor must be at least 3, or some B+-tree
      // balancing algorithms won't work.
      // LeafSize can't be larger than CacheLineBytes. This is required by the
      // PointerIntPair used by NodeRef.
      DesiredLeafSize = DesiredNodeBytes /
      static_cast<unsigned>(2*sizeof(KeyType) + sizeof(ValueType)),
      MinLeafSize = 3,
      LeafSize = DesiredLeafSize > MinLeafSize ? DesiredLeafSize : MinLeafSize
   };

   using LeafBase = NodeBase<std::pair<KeyType, KeyType>, ValueType, LeafSize>;

   enum {
      // Now that we have the leaf branching factor, compute the actual allocation
      // unit size by rounding up to a whole number of cache lines.
      AllocBytes = (sizeof(LeafBase) + CacheLineBytes-1) & ~(CacheLineBytes-1),

      // Determine the branching factor for branch nodes.
      BranchSize = AllocBytes /
      static_cast<unsigned>(sizeof(KeyType) + sizeof(void*))
   };

   /// Allocator - The recycling allocator used for both branch and leaf nodes.
   /// This typedef is very likely to be identical for all IntervalMaps with
   /// reasonably sized entries, so the same allocator can be shared among
   /// different kinds of maps.
   using Allocator =
   RecyclingAllocator<BumpPtrAllocator, char, AllocBytes, CacheLineBytes>;
};

//===----------------------------------------------------------------------===//
//---                     intervalmapimpl::NodeRef                         ---//
//===----------------------------------------------------------------------===//
//
// B+-tree nodes can be leaves or branches, so we need a polymorphic node
// pointer that can point to both kinds.
//
// All nodes are cache line aligned and the low 6 bits of a node pointer are
// always 0. These bits are used to store the number of elements in the
// referenced node. Besides saving space, placing node sizes in the parents
// allow tree balancing algorithms to run without faulting cache lines for nodes
// that may not need to be modified.
//
// A NodeRef doesn't know whether it references a leaf node or a branch node.
// It is the responsibility of the caller to use the correct types.
//
// nodes are never supposed to be empty, and it is invalid to store a node size
// of 0 in a NodeRef. The valid range of sizes is 1-64.
//
//===----------------------------------------------------------------------===//

class NodeRef
{
   struct CacheAlignedPointerTraits
   {
      static inline void *getAsVoidPointer(void *ptr)
      {
         return ptr;
      }

      static inline void *getFromVoidPointer(void *ptr)
      {
         return ptr;
      }

      enum { NumLowBitsAvailable = Log2CacheLine };
   };
   PointerIntPair<void*, Log2CacheLine, unsigned, CacheAlignedPointerTraits> m_pointerIntPair;

public:
   /// NodeRef - Create a null ref.
   NodeRef() = default;

   /// operator bool - Detect a null ref.
   explicit operator bool() const
   {
      return m_pointerIntPair.getOpaqueValue();
   }

   /// NodeRef - Create a reference to the node p with n elements.
   template <typename NodeType>
   NodeRef(NodeType *p, unsigned n) : m_pointerIntPair(p, n - 1)
   {
      assert(n <= NodeType::Capacity && "Size too big for node");
   }

   /// size - Return the number of elements in the referenced node.
   unsigned size() const
   {
      return m_pointerIntPair.getInt() + 1;
   }

   unsigned getSize() const
   {
      return size();
   }

   /// setSize - Update the node size.
   void setSize(unsigned n)
   {
      m_pointerIntPair.setInt(n - 1);
   }

   /// subtree - Access the i'th subtree reference in a branch node.
   /// This depends on branch nodes storing the NodeRef array as their first
   /// member.
   NodeRef &subtree(unsigned i) const
   {
      return reinterpret_cast<NodeRef*>(m_pointerIntPair.getPointer())[i];
   }

   /// get - Dereference as a NodeType reference.
   template <typename NodeType>
   NodeType &get() const
   {
      return *reinterpret_cast<NodeType*>(m_pointerIntPair.getPointer());
   }

   bool operator==(const NodeRef &other) const
   {
      if (m_pointerIntPair == other.m_pointerIntPair) {
         return true;
      }
      assert(m_pointerIntPair.getPointer() != other.m_pointerIntPair.getPointer() && "Inconsistent NodeRefs");
      return false;
   }

   bool operator!=(const NodeRef &other) const
   {
      return !operator==(other);
   }
};

//===----------------------------------------------------------------------===//
//---                      intervalmapimpl::LeafNode                       ---//
//===----------------------------------------------------------------------===//
//
// Leaf nodes store up to N disjoint intervals with corresponding values.
//
// The intervals are kept sorted and fully coalesced so there are no adjacent
// intervals mapping to the same value.
//
// These constraints are always satisfied:
//
// - Traits::stopLess(start(i), stop(i))    - Non-empty, sane intervals.
//
// - Traits::stopLess(stop(i), start(i + 1) - Sorted.
//
// - value(i) != value(i + 1) || !Traits::adjacent(stop(i), start(i + 1))
//                                          - Fully coalesced.
//
//===----------------------------------------------------------------------===//

template <typename KeyType, typename ValueType, unsigned N, typename Traits>
class LeafNode : public NodeBase<std::pair<KeyType, KeyType>, ValueType, N>
{
public:
   const KeyType &start(unsigned i) const
   {
      return this->m_first[i].first;
   }

   const KeyType &stop(unsigned i) const
   {
      return this->m_first[i].second;
   }

   const ValueType &value(unsigned i) const
   {
      return this->m_second[i];
   }

   KeyType &start(unsigned i)
   {
      return this->m_first[i].first;
   }

   KeyType &stop(unsigned i)
   {
      return this->m_first[i].second;
   }

   ValueType &value(unsigned i)
   {
      return this->m_second[i];
   }

   /// findFrom - Find the first interval after i that may contain x.
   /// @param i    Starting index for the search.
   /// @param Size Number of elements in node.
   /// @param x    Key to search for.
   /// @return     First index with !stopLess(key[i].stop, x), or size.
   ///             This is the first interval that can possibly contain x.
   unsigned findFrom(unsigned i, unsigned size, KeyType x) const
   {
      assert(i <= size && size <= N && "Bad indices");
      assert((i == 0 || Traits::stopLess(stop(i - 1), x)) &&
             "Index is past the needed point");
      while (i != size && Traits::stopLess(stop(i), x)) ++i;
      return i;
   }

   /// safeFind - Find an interval that is known to exist. This is the same as
   /// findFrom except is it assumed that x is at least within range of the last
   /// interval.
   /// @param i Starting index for the search.
   /// @param x Key to search for.
   /// @return  First index with !stopLess(key[i].stop, x), never size.
   ///          This is the first interval that can possibly contain x.
   unsigned safeFind(unsigned i, KeyType x) const
   {
      assert(i < N && "Bad index");
      assert((i == 0 || Traits::stopLess(stop(i - 1), x)) &&
             "Index is past the needed point");
      while (Traits::stopLess(stop(i), x)) ++i;
      assert(i < N && "Unsafe intervals");
      return i;
   }

   /// safeLookup - Lookup mapped value for a safe key.
   /// It is assumed that x is within range of the last entry.
   /// @param x        Key to search for.
   /// @param notFound Value to return if x is not in any interval.
   /// @return         The mapped value at x or notFound.
   ValueType safeLookup(KeyType x, ValueType notFound) const
   {
      unsigned i = safeFind(0, x);
      return Traits::startLess(x, start(i)) ? notFound : value(i);
   }

   unsigned insertFrom(unsigned &pos, unsigned size, KeyType a, KeyType b, ValueType y);
};

/// insertFrom - Add mapping of [a;b] to y if possible, coalescing as much as
/// possible. This may cause the node to grow by 1, or it may cause the node
/// to shrink because of coalescing.
/// @param pos  Starting index = insertFrom(0, size, a)
/// @param Size Number of elements in node.
/// @param a    Interval start.
/// @param b    Interval stopath.
/// @param y    Value be mapped.
/// @return     (insert position, new size), or (i, Capacity+1) on overflow.
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
unsigned LeafNode<KeyType, ValueType, N, Traits>::
insertFrom(unsigned &pos, unsigned size, KeyType a, KeyType b, ValueType y)
{
   unsigned i = pos;
   assert(i <= size && size <= N && "Invalid index");
   assert(!Traits::stopLess(b, a) && "Invalid interval");

   // Verify the findFrom invariant.
   assert((i == 0 || Traits::stopLess(stop(i - 1), a)));
   assert((i == size || !Traits::stopLess(stop(i), a)));
   assert((i == size || Traits::stopLess(b, start(i))) && "Overlapping insert");

   // Coalesce with previous interval.
   if (i && value(i - 1) == y && Traits::adjacent(stop(i - 1), a)) {
      pos = i - 1;
      // Also coalesce with next interval?
      if (i != size && value(i) == y && Traits::adjacent(b, start(i))) {
         stop(i - 1) = stop(i);
         this->erase(i, size);
         return size - 1;
      }
      stop(i - 1) = b;
      return size;
   }

   // Detect overflow.
   if (i == N) {
      return N + 1;
   }
   // Add new interval at end.
   if (i == size) {
      start(i) = a;
      stop(i) = b;
      value(i) = y;
      return size + 1;
   }

   // Try to coalesce with following interval.
   if (value(i) == y && Traits::adjacent(b, start(i))) {
      start(i) = a;
      return size;
   }

   // We must insert before i. Detect overflow.
   if (size == N) {
      return N + 1;
   }
   // Insert before i.
   this->shift(i, size);
   start(i) = a;
   stop(i) = b;
   value(i) = y;
   return size + 1;
}

//===----------------------------------------------------------------------===//
//---                   intervalmapimpl::BranchNode                        ---//
//===----------------------------------------------------------------------===//
//
// A branch node stores references to 1--N subtrees all of the same height.
//
// The key array in a branch node holds the rightmost stop key of each subtree.
// It is redundant to store the last stop key since it can be found in the
// parent node, but doing so makes tree balancing a lot simpler.
//
// It is unusual for a branch node to only have one subtree, but it can happen
// in the root node if it is smaller than the normal nodes.
//
// When all of the leaf nodes from all the subtrees are concatenated, they must
// satisfy the same constraints as a single leaf node. They must be sorted,
// sane, and fully coalesced.
//
//===----------------------------------------------------------------------===//

template <typename KeyType, typename ValueType, unsigned N, typename Traits>
class BranchNode : public NodeBase<NodeRef, KeyType, N>
{
public:
   const KeyType &stop(unsigned i) const
   {
      return this->m_second[i];
   }

   const NodeRef &subtree(unsigned i) const
   {
      return this->m_first[i];
   }

   KeyType &stop(unsigned i)
   {
      return this->m_second[i];
   }

   NodeRef &subtree(unsigned i)
   {
      return this->m_first[i];
   }

   /// findFrom - Find the first subtree after i that may contain x.
   /// @param i    Starting index for the search.
   /// @param Size Number of elements in node.
   /// @param x    Key to search for.
   /// @return     First index with !stopLess(key[i], x), or size.
   ///             This is the first subtree that can possibly contain x.
   unsigned findFrom(unsigned i, unsigned size, KeyType x) const
   {
      assert(i <= size && size <= N && "Bad indices");
      assert((i == 0 || Traits::stopLess(stop(i - 1), x)) &&
             "Index to findFrom is past the needed point");
      while (i != size && Traits::stopLess(stop(i), x)) ++i;
      return i;
   }

   /// safeFind - Find a subtree that is known to exist. This is the same as
   /// findFrom except is it assumed that x is in range.
   /// @param i Starting index for the search.
   /// @param x Key to search for.
   /// @return  First index with !stopLess(key[i], x), never size.
   ///          This is the first subtree that can possibly contain x.
   unsigned safeFind(unsigned i, KeyType x) const
   {
      assert(i < N && "Bad index");
      assert((i == 0 || Traits::stopLess(stop(i - 1), x)) &&
             "Index is past the needed point");
      while (Traits::stopLess(stop(i), x)) ++i;
      assert(i < N && "Unsafe intervals");
      return i;
   }

   /// safeLookup - Get the subtree containing x, Assuming that x is in range.
   /// @param x Key to search for.
   /// @return  Subtree containing x
   NodeRef safeLookup(KeyType x) const
   {
      return subtree(safeFind(0, x));
   }

   /// insert - Insert a new (subtree, stop) pair.
   /// @param i    Insert position, following entries will be shifted.
   /// @param Size Number of elements in node.
   /// @param Node Subtree to insert.
   /// @param Stop Last key in subtree.
   void insert(unsigned i, unsigned size, NodeRef node, KeyType stopValue)
   {
      assert(size < N && "branch node overflow");
      assert(i <= size && "Bad insert position");
      this->shift(i, size);
      subtree(i) = node;
      stop(i) = stopValue;
   }
};

//===----------------------------------------------------------------------===//
//---                         intervalmapimpl::Path                        ---//
//===----------------------------------------------------------------------===//
//
// A Path is used by Iterators to represent a position in a B+-tree, and the
// path to get there from the root.
//
// The Path class also contains the tree navigation code that doesn't have to
// be templatized.
//
//===----------------------------------------------------------------------===//

class Path
{
   /// Entry - Each step in the path is a node pointer and an offset into that
   /// node.
   struct Entry
   {
      void *m_node;
      unsigned m_size;
      unsigned m_offset;

      Entry(void *node, unsigned size, unsigned offset)
         : m_node(node), m_size(size), m_offset(offset)
      {}

      Entry(NodeRef node, unsigned offset)
         : m_node(&node.subtree(0)), m_size(node.getSize()), m_offset(offset)
      {}

      NodeRef &subtree(unsigned i) const
      {
         return reinterpret_cast<NodeRef*>(m_node)[i];
      }
   };

   /// path - The path entries, m_path[0] is the root node, m_path.back() is a leaf.
   SmallVector<Entry, 4> m_path;

public:
   // Node accessors.
   template <typename NodeType> NodeType &node(unsigned level) const
   {
      return *reinterpret_cast<NodeType*>(m_path[level].m_node);
   }
   unsigned size(unsigned level) const
   {
      return m_path[level].m_size;
   }

   unsigned offset(unsigned level) const
   {
      return m_path[level].m_offset;
   }

   unsigned &offset(unsigned level)
   {
      return m_path[level].m_offset;
   }

   // Leaf accessors.
   template <typename NodeType>
   NodeType &getLeaf() const
   {
      return *reinterpret_cast<NodeType*>(m_path.getBack().m_node);
   }

   unsigned getLeafSize() const
   {
      return m_path.getBack().m_size;
   }

   unsigned getLeafOffset() const
   {
      return m_path.getBack().m_offset;
   }

   unsigned &getLeafOffset()
   {
      return m_path.getBack().m_offset;
   }

   /// valid - Return true if path is at a valid node, not at end().
   bool valid() const
   {
      return !m_path.empty() && m_path.getFront().m_offset < m_path.getFront().m_size;
   }

   /// height - Return the height of the tree corresponding to this m_path.
   /// This matches m_map->height in a full m_path.
   unsigned getHeight() const
   {
      return m_path.getSize() - 1;
   }

   /// subtree - Get the subtree referenced from level. When the path is
   /// consistent, node(level + 1) == subtree(level).
   /// @param level 0..height-1. The leaves have no subtrees.
   NodeRef &subtree(unsigned level) const
   {
      return m_path[level].subtree(m_path[level].m_offset);
   }

   /// reset - Reset cached information about node(level) from subtree(level -1).
   /// @param level 1..height. THe node to update after parent node changed.
   void reset(unsigned level)
   {
      m_path[level] = Entry(subtree(level - 1), offset(level));
   }

   /// push - Add entry to m_path.
   /// @param Node Node to add, should be subtree(m_path.size()-1).
   /// @param offset offset into Node.
   void push(NodeRef node, unsigned offset)
   {
      m_path.push_back(Entry(node, offset));
   }

   /// pop - Remove the last path entry.
   void pop()
   {
      m_path.pop_back();
   }

   /// setSize - Set the size of a node both in the path and in the tree.
   /// @param level 0..height. Note that setting the root size won't change
   ///              m_map->m_rootSize.
   /// @param Size New node size.
   void setSize(unsigned level, unsigned size)
   {
      m_path[level].m_size = size;
      if (level) {
         subtree(level - 1).setSize(size);
      }
   }

   /// setRoot - Clear the path and set a new root node.
   /// @param Node New root node.
   /// @param Size New root size.
   /// @param offset offset into root node.
   void setRoot(void *node, unsigned size, unsigned offset)
   {
      m_path.clear();
      m_path.push_back(Entry(node, size, offset));
   }

   /// replaceRoot - Replace the current root node with two new entries after the
   /// tree height has increased.
   /// @param Root The new root node.
   /// @param Size Number of entries in the new root.
   /// @param offsets offsets into the root and first branch nodes.
   void replaceRoot(void *root, unsigned size, IdxPair offsets);

   /// getLeftSibling - Get the left sibling node at level, or a null NodeRef.
   /// @param level Get the sibling to node(level).
   /// @return Left sibling, or NodeRef().
   NodeRef getLeftSibling(unsigned level) const;

   /// moveLeft - Move path to the left sibling at level. Leave nodes below level
   /// unaltered.
   /// @param level Move node(level).
   void moveLeft(unsigned level);

   /// fillLeft - Grow path to Height by taking leftmost branches.
   /// @param Height The target height.
   void fillLeft(unsigned height)
   {
      while (getHeight() < height) {
         push(subtree(getHeight()), 0);
      }
   }

   /// getLeftSibling - Get the left sibling node at level, or a null NodeRef.
   /// @param level Get the sinbling to node(level).
   /// @return Left sibling, or NodeRef().
   NodeRef getRightSibling(unsigned level) const;

   /// moveRight - Move path to the left sibling at level. Leave nodes below
   /// level unaltered.
   /// @param level Move node(level).
   void moveRight(unsigned level);

   /// atBegin - Return true if path is at begin().
   bool atBegin() const
   {
      for (unsigned i = 0, e = m_path.getSize(); i != e; ++i) {
         if (m_path[i].m_offset != 0) {
            return false;
         }
      }
      return true;
   }

   /// atLastEntry - Return true if the path is at the last entry of the node at
   /// level.
   /// @param level Node to examine.
   bool atLastEntry(unsigned level) const
   {
      return m_path[level].m_offset == m_path[level].m_size - 1;
   }

   /// legalizeForInsert - Prepare the path for an insertion at level. When the
   /// path is at end(), node(level) may not be a legal node. legalizeForInsert
   /// ensures that node(level) is real by moving back to the last node at level,
   /// and setting offset(level) to size(level) if required.
   /// @param level The level where an insertion is about to take place.
   void legalizeForInsert(unsigned level)
   {
      if (valid()) {
         return;
      }
      moveLeft(level);
      ++m_path[level].m_offset;
   }
};

} // end namespace intervalmapimpl

//===----------------------------------------------------------------------===//
//---                          IntervalMap                                ----//
//===----------------------------------------------------------------------===//

template <typename KeyT, typename ValueT,
          unsigned N = intervalmapimpl:: NodeSizer<KeyT, ValueT>::LeafSize,
          typename Traits = IntervalMapInfo<KeyT>>
class IntervalMap
{
   using Sizer = intervalmapimpl:: NodeSizer<KeyT, ValueT>;
   using Leaf = intervalmapimpl::LeafNode<KeyT, ValueT, Sizer::LeafSize, Traits>;
   using Branch =
   intervalmapimpl::BranchNode<KeyT, ValueT, Sizer::BranchSize, Traits>;
   using RootLeaf = intervalmapimpl::LeafNode<KeyT, ValueT, N, Traits>;
   using IdxPair = intervalmapimpl::IdxPair;

   // The RootLeaf capacity is given as a template parameter. We must compute the
   // corresponding RootBranch capacity.
   enum {
      DesiredRootBranchCap = (sizeof(RootLeaf) - sizeof(KeyT)) /
      (sizeof(KeyT) + sizeof(intervalmapimpl::NodeRef)),
      RootBranchCap = DesiredRootBranchCap ? DesiredRootBranchCap : 1
   };

   using RootBranch =
   intervalmapimpl::BranchNode<KeyT, ValueT, RootBranchCap, Traits>;

   // When branched, we store a global start key as well as the branch node.
   struct RootBranchData
   {
      KeyT m_start;
      RootBranch m_node;
   };

public:
   using Allocator = typename Sizer::Allocator;
   using KeyType = KeyT;
   using ValueType = ValueT;
   using KeyTyperaits = Traits;

private:
   // The root data is either a RootLeaf or a RootBranchData instance.
   POLAR_ALIGNAS(RootLeaf) POLAR_ALIGNAS(RootBranchData)
   AlignedCharArrayUnion<RootLeaf, RootBranchData> m_data;

   // Tree height.
   // 0: Leaves in root.
   // 1: Root points to leaf.
   // 2: root->branch->leaf ...
   unsigned m_height;

   // Number of entries in the root node.
   unsigned m_rootSize;

   // Allocator used for creating external nodes.
   Allocator &m_allocator;

    /// Represent data as a node type without breaking aliasing rules.
   template <typename T>
   T &getDataAs() const
   {
      return *bit_cast<T *>(const_cast<char *>(m_data.m_buffer));
   }

   const RootLeaf &getRootLeaf() const
   {
      assert(!branched() && "Cannot acces leaf data in branched root");
      return getDataAs<RootLeaf>();
   }

   RootLeaf &getRootLeaf()
   {
      assert(!branched() && "Cannot acces leaf data in branched root");
      return getDataAs<RootLeaf>();
   }

   RootBranchData &getRootBranchData() const
   {
      assert(branched() && "Cannot access branch data in non-branched root");
      return getDataAs<RootBranchData>();
   }

   RootBranchData &getRootBranchData()
   {
      assert(branched() && "Cannot access branch data in non-branched root");
      return getDataAs<RootBranchData>();
   }

   const RootBranch &getRootBranch() const
   {
      return getRootBranchData().m_node;
   }

   RootBranch &getRootBranch()
   {
      return getRootBranchData().m_node;
   }

   KeyType getRootBranchStart() const
   {
      return getRootBranchData().m_start;
   }

   KeyType &getRootBranchStart()
   {
      return getRootBranchData().m_start;
   }

   template <typename NodeType>
   NodeType *newNode()
   {
      return new(m_allocator.template allocate<NodeType>()) NodeType();
   }

   template <typename NodeType>
   void deleteNode(NodeType *node)
   {
      node->~NodeType();
      m_allocator.deallocate(node);
   }

   IdxPair branchRoot(unsigned position);
   IdxPair splitRoot(unsigned position);

   void switchRootToBranch()
   {
      getRootLeaf().~RootLeaf();
      m_height = 1;
      new (&getRootBranchData()) RootBranchData();
   }

   void switchRootToLeaf()
   {
      getRootBranchData().~RootBranchData();
      m_height = 0;
      new(&getRootLeaf()) RootLeaf();
   }

   bool branched() const
   {
      return m_height > 0;
   }

   ValueType treeSafeLookup(KeyType value, ValueType notFound) const;
   void visitNodes(void (IntervalMap::*f)(intervalmapimpl::NodeRef,
                                          unsigned level));
   void deleteNode(intervalmapimpl::NodeRef node, unsigned level);

public:
   explicit IntervalMap(Allocator &allocator)
      : m_height(0), m_rootSize(0), m_allocator(allocator)
   {
      assert((uintptr_t(m_data.m_buffer) & (alignof(RootLeaf) - 1)) == 0 &&
             "Insufficient alignment");
      new(&getRootLeaf()) RootLeaf();
   }

   ~IntervalMap()
   {
      clear();
      getRootLeaf().~RootLeaf();
   }

   /// empty -  Return true when no intervals are mapped.
   bool empty() const
   {
      return m_rootSize == 0;
   }

   /// start - Return the smallest mapped key in a non-empty mapath.
   KeyType start() const
   {
      assert(!empty() && "Empty IntervalMap has no start");
      return !branched() ? getRootLeaf().start(0) : getRootBranchStart();
   }

   /// stop - Return the largest mapped key in a non-empty mapath.
   KeyType stop() const
   {
      assert(!empty() && "Empty IntervalMap has no stop");
      return !branched() ? getRootLeaf().stop(m_rootSize - 1) :
                           getRootBranch().stop(m_rootSize - 1);
   }

   /// lookup - Return the mapped value at x or notFound.
   ValueType lookup(KeyType value, ValueType notFound = ValueType()) const
   {
      if (empty() || Traits::startLess(value, start()) || Traits::stopLess(stop(), value)) {
         return notFound;
      }

      return branched() ? treeSafeLookup(value, notFound) :
                          getRootLeaf().safeLookup(value, notFound);
   }

   /// insert - Add a mapping of [a;b] to y, coalesce with adjacent intervals.
   /// It is assumed that no key in the interval is mapped to another value, but
   /// overlapping intervals already mapped to y will be coalesced.
   void insert(KeyType a, KeyType b, ValueType y)
   {
      if (branched() || m_rootSize == RootLeaf::Capacity) {
         return find(a).insert(a, b, y);
      }
      // Easy insert into root leaf.
      unsigned p = getRootLeaf().findFrom(0, m_rootSize, a);
      m_rootSize = getRootLeaf().insertFrom(p, m_rootSize, a, b, y);
   }

   /// clear - Remove all entries.
   void clear();

   class ConstIterator;
   class Iterator;
   friend class ConstIterator;
   friend class Iterator;

   ConstIterator begin() const
   {
      ConstIterator iter(*this);
      iter.goToBegin();
      return iter;
   }

   Iterator begin()
   {
      Iterator iter(*this);
      iter.goToBegin();
      return iter;
   }

   ConstIterator end() const
   {
      ConstIterator iter(*this);
      iter.goToEnd();
      return iter;
   }

   Iterator end()
   {
      Iterator iter(*this);
      iter.goToEnd();
      return iter;
   }

   /// find - Return an Iterator pointing to the first interval ending at or
   /// after x, or end().
   ConstIterator find(KeyType value) const
   {
      ConstIterator iter(*this);
      iter.find(value);
      return iter;
   }

   Iterator find(KeyType value)
   {
      Iterator iter(*this);
      iter.find(value);
      return iter;
   }
};

/// treeSafeLookup - Return the mapped value at x or notFound, assuming a
/// branched root.
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
ValueType IntervalMap<KeyType, ValueType, N, Traits>::
treeSafeLookup(KeyType value, ValueType notFound) const
{
   assert(branched() && "treeLookup assumes a branched root");

   intervalmapimpl::NodeRef nodeRef = getRootBranch().safeLookup(value);
   for (unsigned h = m_height - 1; h; --h) {
      nodeRef = nodeRef.get<Branch>().safeLookup(value);
   }
   return nodeRef.get<Leaf>().safeLookup(value, notFound);
}

// branchRoot - Switch from a leaf root to a branched root.
// Return the new (root offset, node offset) corresponding to position.
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
intervalmapimpl::IdxPair IntervalMap<KeyType, ValueType, N, Traits>::
branchRoot(unsigned position)
{
   using namespace intervalmapimpl;
   // How many external leaf nodes to hold RootLeaf+1?
   const unsigned nodes = RootLeaf::Capacity / Leaf::Capacity + 1;

   // Compute element distribution among new nodes.
   unsigned size[nodes];
   IdxPair newOffset(0, position);

   // Is is very common for the root node to be smaller than external nodes.
   if (nodes == 1) {
      size[0] = m_rootSize;
   } else {
       newOffset = distribute(nodes, m_rootSize, Leaf::Capacity,  nullptr, size,
                             position, true);
   }

   // Allocate new nodes.
   unsigned pos = 0;
   NodeRef node[nodes];
   for (unsigned n = 0; n != nodes; ++n) {
      Leaf *leaf = newNode<Leaf>();
      leaf->copy(getRootLeaf(), pos, 0, size[n]);
      node[n] = NodeRef(leaf, size[n]);
      pos += size[n];
   }

   // Destroy the old leaf node, construct branch node instead.
   switchRootToBranch();
   for (unsigned n = 0; n != nodes; ++n) {
      getRootBranch().stop(n) = node[n].template get<Leaf>().stop(size[n]-1);
      getRootBranch().subtree(n) = node[n];
   }
   getRootBranchStart() = node[0].template get<Leaf>().start(0);
   m_rootSize = nodes;
   return newOffset;
}

// splitRoot - Split the current BranchRoot into multiple Branch nodes.
// Return the new (root offset, node offset) corresponding to position.
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
intervalmapimpl::IdxPair IntervalMap<KeyType, ValueType, N, Traits>::
splitRoot(unsigned position)
{
   using namespace intervalmapimpl;
   // How many external leaf nodes to hold RootBranch+1?
   const unsigned nodes = RootBranch::Capacity / Branch::Capacity + 1;

   // Compute element distribution among new nodes.
   unsigned size[nodes];
   IdxPair newOffset(0, position);

   // Is is very common for the root node to be smaller than external nodes.
   if (nodes == 1) {
      size[0] = m_rootSize;
   } else {
       newOffset = distribute(nodes, m_rootSize, Leaf::Capacity,  nullptr, size,
                             position, true);
   }
   // Allocate new nodes.
   unsigned pos = 0;
   NodeRef node[nodes];
   for (unsigned n = 0; n != nodes; ++n) {
      Branch *branch = newNode<Branch>();
      branch->copy(getRootBranch(), pos, 0, size[n]);
      node[n] = NodeRef(branch, size[n]);
      pos += size[n];
   }

   for (unsigned n = 0; n != nodes; ++n) {
      getRootBranch().stop(n) = node[n].template get<Branch>().stop(size[n]-1);
      getRootBranch().subtree(n) = node[n];
   }
   m_rootSize = nodes;
   ++m_height;
   return  newOffset;
}

/// visitNodes - Visit each external node.
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
void IntervalMap<KeyType, ValueType, N, Traits>::
visitNodes(void (IntervalMap::*f)(intervalmapimpl::NodeRef, unsigned height))
{
   if (!branched()) {
      return;
   }
   SmallVector<intervalmapimpl::NodeRef, 4> refs, nextRefs;

   // Collect level 0 nodes from the root.
   for (unsigned i = 0; i != m_rootSize; ++i) {
      refs.push_back(getRootBranch().subtree(i));
   }
   // Visit all branch nodes.
   for (unsigned h = m_height - 1; h; --h) {
      for (unsigned i = 0, e = refs.getSize(); i != e; ++i) {
         for (unsigned j = 0, s = refs[i].size(); j != s; ++j) {
            nextRefs.push_back(refs[i].subtree(j));
         }
         (this->*f)(refs[i], h);
      }
      refs.clear();
      refs.swap(nextRefs);
   }

   // Visit all leaf nodes.
   for (unsigned i = 0, e = refs.getSize(); i != e; ++i) {
      (this->*f)(refs[i], 0);
   }
}

template <typename KeyType, typename ValueType, unsigned N, typename Traits>
void IntervalMap<KeyType, ValueType, N, Traits>::
deleteNode(intervalmapimpl::NodeRef node, unsigned level)
{
   if (level) {
      deleteNode(&node.get<Branch>());
   } else {
      deleteNode(&node.get<Leaf>());
   }
}

template <typename KeyType, typename ValueType, unsigned N, typename Traits>
void IntervalMap<KeyType, ValueType, N, Traits>::
clear()
{
   if (branched()) {
      visitNodes(&IntervalMap::deleteNode);
       switchRootToLeaf();
   }
   m_rootSize = 0;
}

//===----------------------------------------------------------------------===//
//---                   IntervalMap::ConstIterator                       ----//
//===----------------------------------------------------------------------===//

template <typename KeyType, typename ValueType, unsigned N, typename Traits>
class IntervalMap<KeyType, ValueType, N, Traits>::ConstIterator :
      public std::iterator<std::bidirectional_iterator_tag, ValueType>
{

protected:
   friend class IntervalMap;

   // The map referred to.
   IntervalMap *m_map = nullptr;

   // We store a full path from the root to the current position.
   // The path may be partially filled, but never between Iterator calls.
   intervalmapimpl::Path m_path;

   explicit ConstIterator(const IntervalMap &map) :
      m_map(const_cast<IntervalMap*>(&map))
   {}

   bool branched() const
   {
      assert(m_map && "Invalid Iterator");
      return m_map->branched();
   }

   void setRoot(unsigned offset) {
      if (branched()) {
         m_path.setRoot(&m_map->getRootBranch(), m_map->m_rootSize, offset);
      } else {
         m_path.setRoot(&m_map->getRootLeaf(), m_map->m_rootSize, offset);
      }
   }

   void pathFillFind(KeyType value);
   void treeFind(KeyType value);
   void treeAdvanceTo(KeyType value);

   /// unsafeStart - Writable access to start() for Iterator.
   KeyType &unsafeStart() const
   {
      assert(valid() && "Cannot access invalid Iterator");
      return branched() ? m_path.getLeaf<Leaf>().start(m_path.getLeafOffset()) :
                          m_path.getLeaf<RootLeaf>().start(m_path.getLeafOffset());
   }

   /// unsafeStop - Writable access to stop() for Iterator.
   KeyType &unsafeStop() const
   {
      assert(valid() && "Cannot access invalid Iterator");
      return branched() ? m_path.getLeaf<Leaf>().stop(m_path.getLeafOffset()) :
                          m_path.getLeaf<RootLeaf>().stop(m_path.getLeafOffset());
   }

   /// unsafeValue - Writable access to value() for Iterator.
   ValueType &unsafeValue() const {
      assert(valid() && "Cannot access invalid Iterator");
      return branched() ? m_path.getLeaf<Leaf>().value(m_path.getLeafOffset()) :
                          m_path.getLeaf<RootLeaf>().value(m_path.getLeafOffset());
   }

public:
   /// ConstIterator - Create an Iterator that isn't pointing anywhere.
   ConstIterator() = default;

   /// setMap - Change the map iterated over. This call must be followed by a
   /// call to goToBegin(), goToEnd(), or find()
   void setMap(const IntervalMap &map)
   {
      m_map = const_cast<IntervalMap*>(&map);
   }

   /// valid - Return true if the current position is valid, false for end().
   bool valid() const
   {
      return m_path.valid();
   }

   /// atBegin - Return true if the current position is the first map entry.
   bool atBegin() const
   {
      return m_path.atBegin();
   }

   /// start - Return the beginning of the current interval.
   const KeyType &start() const
   {
      return unsafeStart();
   }

   /// stop - Return the end of the current interval.
   const KeyType &stop() const
   {
      return unsafeStop();
   }

   /// value - Return the mapped value at the current interval.
   const ValueType &value() const
   {
      return unsafeValue();
   }

   const ValueType &operator*() const
   {
      return value();
   }

   bool operator==(const ConstIterator &other) const
   {
      assert(m_map == other.m_map && "Cannot compare Iterators from different maps");
      if (!valid()) {
         return !other.valid();
      }
      if (m_path.getLeafOffset() != other.m_path.getLeafOffset()) {
         return false;
      }
      return &m_path.template getLeaf<Leaf>() == &other.m_path.template getLeaf<Leaf>();
   }

   bool operator!=(const ConstIterator &other) const
   {
      return !operator==(other);
   }

   /// goToBegin - Move to the first interval in mapath.
   void goToBegin()
   {
      setRoot(0);
      if (branched()) {
         m_path.fillLeft(m_map->m_height);
      }
   }

   /// goToEnd - Move beyond the last interval in mapath.
   void goToEnd()
   {
      setRoot(m_map->m_rootSize);
   }

   /// preincrement - move to the next interval.
   ConstIterator &operator++()
   {
      assert(valid() && "Cannot increment end()");
      if (++m_path.getLeafOffset() == m_path.getLeafSize() && branched()) {
         m_path.moveRight(m_map->m_height);
      }
      return *this;
   }

   /// postincrement - Dont do that!
   ConstIterator operator++(int)
   {
      ConstIterator tmp = *this;
      operator++();
      return tmp;
   }

   /// predecrement - move to the previous interval.
   ConstIterator &operator--()
   {
      if (m_path.getLeafOffset() && (valid() || !branched())) {
         --m_path.getLeafOffset();
      } else {
         m_path.moveLeft(m_map->m_height);
      }
      return *this;
   }

   /// postdecrement - Dont do that!
   ConstIterator operator--(int)
   {
      ConstIterator tmp = *this;
      operator--();
      return tmp;
   }

   /// find - Move to the first interval with stop >= x, or end().
   /// This is a full search from the root, the current position is ignored.
   void find(KeyType value)
   {
      if (branched()) {
         treeFind(value);
      } else {
         setRoot(m_map->getRootLeaf().findFrom(0, m_map->m_rootSize, value));
      }
   }

   /// advanceTo - Move to the first interval with stop >= x, or end().
   /// The search is started from the current position, and no earlier positions
   /// can be found. This is much faster than find() for small moves.
   void advanceTo(KeyType value)
   {
      if (!valid()) {
         return;
      }
      if (branched()) {
         treeAdvanceTo(value);
      } else {
         m_path.getLeafOffset() =
               m_map->getRootLeaf().findFrom(m_path.getLeafOffset(), m_map->m_rootSize, value);
      }
   }
};

/// pathFillFind - Complete path by searching for x.
/// @param x Key to search for.
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
void IntervalMap<KeyType, ValueType, N, Traits>::
ConstIterator::pathFillFind(KeyType value)
{
   intervalmapimpl::NodeRef nodeRef = m_path.subtree(m_path.getHeight());
   for (unsigned i = m_map->m_height - m_path.getHeight() - 1; i; --i) {
      unsigned p = nodeRef.get<Branch>().safeFind(0, value);
      m_path.push(nodeRef, p);
      nodeRef = nodeRef.subtree(p);
   }
   m_path.push(nodeRef, nodeRef.get<Leaf>().safeFind(0, value));
}

/// treeFind - Find in a branched tree.
/// @param x Key to search for.
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
void IntervalMap<KeyType, ValueType, N, Traits>::
ConstIterator::treeFind(KeyType value)
{
   setRoot(m_map->getRootBranch().findFrom(0, m_map->m_rootSize, value));
   if (valid()) {
      pathFillFind(value);
   }
}

/// treeAdvanceTo - Find position after the current one.
/// @param x Key to search for.
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
void IntervalMap<KeyType, ValueType, N, Traits>::
ConstIterator::treeAdvanceTo(KeyType value)
{
   // Can we stay on the same leaf node?
   if (!Traits::stopLess(m_path.getLeaf<Leaf>().stop(m_path.getLeafSize() - 1), value)) {
      m_path.getLeafOffset() = m_path.getLeaf<Leaf>().safeFind(m_path.getLeafOffset(), value);
      return;
   }
   // Drop the current leaf.
   m_path.pop();
   // Search towards the root for a usable subtree.
   if (m_path.getHeight()) {
      for (unsigned l = m_path.getHeight() - 1; l; --l) {
         if (!Traits::stopLess(m_path.node<Branch>(l).stop(m_path.offset(l)), value)) {
            // The branch node at l+1 is usable
            m_path.offset(l + 1) =
                  m_path.node<Branch>(l + 1).safeFind(m_path.offset(l + 1), value);
            return pathFillFind(value);
         }
         m_path.pop();
      }
      // Is the level-1 Branch usable?
      if (!Traits::stopLess(m_map->getRootBranch().stop(m_path.offset(0)), value)) {
         m_path.offset(1) = m_path.node<Branch>(1).safeFind(m_path.offset(1), value);
         return pathFillFind(value);
      }
   }

   // We reached the root.
   setRoot(m_map->getRootBranch().findFrom(m_path.offset(0), m_map->m_rootSize, value));
   if (valid()) {
      pathFillFind(value);
   }
}

//===----------------------------------------------------------------------===//
//---                       IntervalMap::Iterator                         ----//
//===----------------------------------------------------------------------===//

template <typename KeyType, typename ValueType, unsigned N, typename Traits>
class IntervalMap<KeyType, ValueType, N, Traits>::Iterator : public ConstIterator
{
   friend class IntervalMap;
   using IdxPair = intervalmapimpl::IdxPair;
   explicit Iterator(IntervalMap &map) : ConstIterator(map)
   {}

   void setNodeStop(unsigned level, KeyType stop);
   bool insertNode(unsigned level, intervalmapimpl::NodeRef node, KeyType stop);
   template <typename NodeType> bool overflow(unsigned level);
   void treeInsert(KeyType a, KeyType b, ValueType y);
   void eraseNode(unsigned level);
   void treeErase(bool updateRoot = true);
   bool canCoalesceLeft(KeyType start, ValueType x);
   bool canCoalesceRight(KeyType stop, ValueType x);

public:
   /// Iterator - Create null Iterator.
   Iterator() = default;

   /// setStart - Move the start of the current interval.
   /// This may cause coalescing with the previous interval.
   /// @param a New start key, must not overlap the previous interval.
   void setStart(KeyType a);

   /// setStop - Move the end of the current interval.
   /// This may cause coalescing with the following interval.
   /// @param b New stop key, must not overlap the following interval.
   void setStop(KeyType b);

   /// setValue - Change the mapped value of the current interval.
   /// This may cause coalescing with the previous and following intervals.
   /// @param x New value.
   void setValue(ValueType x);

   /// setStartUnchecked - Move the start of the current interval without
   /// checking for coalescing or overlaps.
   /// This should only be used when it is known that coalescing is not required.
   /// @param a New start key.
   void setStartUnchecked(KeyType a)
   {
      this->unsafeStart() = a;
   }

   /// setStopUnchecked - Move the end of the current interval without checking
   /// for coalescing or overlaps.
   /// This should only be used when it is known that coalescing is not required.
   /// @param b New stop key.
   void setStopUnchecked(KeyType b)
   {
      this->unsafeStop() = b;
      // Update keys in branch nodes as well.
      if (this->m_path.atLastEntry(this->m_path.getHeight())) {
         setNodeStop(this->m_path.getHeight(), b);
      }
   }

   /// setValueUnchecked - Change the mapped value of the current interval
   /// without checking for coalescing.
   /// @param x New value.
   void setValueUnchecked(ValueType value)
   {
      this->unsafeValue() = value;
   }

   /// insert - Insert mapping [a;b] -> y before the current position.
   void insert(KeyType a, KeyType b, ValueType y);

   /// erase - Erase the current interval.
   void erase();

   Iterator &operator++()
   {
      ConstIterator::operator++();
      return *this;
   }

   Iterator operator++(int)
   {
      Iterator tmp = *this;
      operator++();
      return tmp;
   }

   Iterator &operator--()
   {
      ConstIterator::operator--();
      return *this;
   }

   Iterator operator--(int)
   {
      Iterator tmp = *this;
      operator--();
      return tmp;
   }
};

/// canCoalesceLeft - Can the current interval coalesce to the left after
/// changing start or value?
/// @param Start New start of current interval.
/// @param Value New value for current interval.
/// @return True when updating the current interval would enable coalescing.
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
bool IntervalMap<KeyType, ValueType, N, Traits>::
Iterator::canCoalesceLeft(KeyType start, ValueType value)
{
   using namespace intervalmapimpl;
   Path &path = this->m_path;
   if (!this->branched()) {
      unsigned i = path.getLeafOffset();
      RootLeaf &node = path.getLeaf<RootLeaf>();
      return i && node.value(i-1) == value &&
            Traits::adjacent(node.stop(i-1), start);
   }
   // Branched.
   if (unsigned i = path.getLeafOffset()) {
      Leaf &node = path.getLeaf<Leaf>();
      return node.value(i-1) == value && Traits::adjacent(node.stop(i-1), start);
   } else if (NodeRef nodeRef = path.getLeftSibling(path.getHeight())) {
      unsigned i = nodeRef.size() - 1;
      Leaf &node = nodeRef.get<Leaf>();
      return node.value(i) == value && Traits::adjacent(node.stop(i), start);
   }
   return false;
}

/// canCoalesceRight - Can the current interval coalesce to the right after
/// changing stop or value?
/// @param Stop New stop of current interval.
/// @param Value New value for current interval.
/// @return True when updating the current interval would enable coalescing.
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
bool IntervalMap<KeyType, ValueType, N, Traits>::
Iterator::canCoalesceRight(KeyType stop, ValueType value)
{
   using namespace intervalmapimpl;
   Path &path = this->m_path;
   unsigned i = path.getLeafOffset() + 1;
   if (!this->branched()) {
      if (i >= path.getLeafSize())
         return false;
      RootLeaf &node = path.getLeaf<RootLeaf>();
      return node.value(i) == value && Traits::adjacent(stop, node.start(i));
   }
   // Branched.
   if (i < path.getLeafSize()) {
      Leaf &node = path.getLeaf<Leaf>();
      return node.value(i) == value && Traits::adjacent(stop, node.start(i));
   } else if (NodeRef nodeRef = path.getRightSibling(path.getHeight())) {
      Leaf &node = nodeRef.get<Leaf>();
      return node.value(0) == value && Traits::adjacent(stop, node.start(0));
   }
   return false;
}

/// setNodeStop - Update the stop key of the current node at level and above.
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
void IntervalMap<KeyType, ValueType, N, Traits>::
Iterator::setNodeStop(unsigned level, KeyType stop)
{
   // There are no references to the root node, so nothing to update.
   if (!level) {
      return;
   }
   intervalmapimpl::Path &path = this->m_path;
   // Update nodes pointing to the current node.
   while (--level) {
      path.node<Branch>(level).stop(path.offset(level)) = stop;
      if (!path.atLastEntry(level)) {
         return;
      }
   }
   // Update root separately since it has a different layout.
   path.node<RootBranch>(level).stop(path.offset(level)) = stop;
}

template <typename KeyType, typename ValueType, unsigned N, typename Traits>
void IntervalMap<KeyType, ValueType, N, Traits>::
Iterator::setStart(KeyType a)
{
   assert(Traits::nonEmpty(a, this->stop()) && "Cannot move start beyond stop");
   KeyType &curStart = this->unsafeStart();
   if (!Traits::startLess(a, curStart) || !canCoalesceLeft(a, this->value())) {
      curStart = a;
      return;
   }
   // Coalesce with the interval to the left.
   --*this;
   a = this->start();
   erase();
   setStartUnchecked(a);
}

template <typename KeyType, typename ValueType, unsigned N, typename Traits>
void IntervalMap<KeyType, ValueType, N, Traits>::
Iterator::setStop(KeyType b)
{
   assert(Traits::nonEmpty(this->start(), b) && "Cannot move stop beyond start");
   if (Traits::startLess(b, this->stop()) ||
       !canCoalesceRight(b, this->value())) {
      setStopUnchecked(b);
      return;
   }
   // Coalesce with interval to the right.
   KeyType a = this->start();
   erase();
   setStartUnchecked(a);
}

template <typename KeyType, typename ValueType, unsigned N, typename Traits>
void IntervalMap<KeyType, ValueType, N, Traits>::
Iterator::setValue(ValueType x) {

   setValueUnchecked(x);
   if (canCoalesceRight(this->stop(), x)) {
      KeyType a = this->start();
      erase();
      setStartUnchecked(a);
   }
   if (canCoalesceLeft(this->start(), x)) {
      --*this;
      KeyType a = this->start();
      erase();
      setStartUnchecked(a);
   }
}

/// insertNode - insert a node before the current path at level.
/// Leave the current path pointing at the new node.
/// @param level path index of the node to be inserted.
/// @param Node The node to be inserted.
/// @param Stop The last index in the new node.
/// @return True if the tree height was increased.
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
bool IntervalMap<KeyType, ValueType, N, Traits>::
Iterator::insertNode(unsigned level, intervalmapimpl::NodeRef node, KeyType stop)
{
   assert(level && "Cannot insert next to the root");
   bool splitRoot = false;
   IntervalMap &im = *this->m_map;
   intervalmapimpl::Path &path = this->m_path;

   if (level == 1) {
      // Insert into the root branch node.
      if (im.m_rootSize < RootBranch::Capacity) {
         im.getRootBranch().insert(path.offset(0), im.m_rootSize, node, stop);
         path.setSize(0, ++im.m_rootSize);
         path.reset(level);
         return splitRoot;
      }

      // We need to split the root while keeping our position.
      splitRoot = true;
      IdxPair offset = im.splitRoot(path.offset(0));
      path.replaceRoot(&im.getRootBranch(), im.m_rootSize, offset);

      // Fall through to insert at the new higher level.
      ++level;
   }
   // When inserting before end(), make sure we have a valid m_path.
   path.legalizeForInsert(--level);

   // Insert into the branch node at level-1.
   if (path.size(level) == Branch::Capacity) {
      // Branch node is full, handle handle the overflow.
      assert(!splitRoot && "Cannot overflow after splitting the root");
      splitRoot = overflow<Branch>(level);
      level += splitRoot;
   }
   path.node<Branch>(level).insert(path.offset(level), path.size(level), node, stop);
   path.setSize(level, path.size(level) + 1);
   if (path.atLastEntry(level)) {
      setNodeStop(level, stop);
   }
   path.reset(level + 1);
   return splitRoot;
}

// insert
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
void IntervalMap<KeyType, ValueType, N, Traits>::
Iterator::insert(KeyType a, KeyType b, ValueType y)
{
   if (this->branched()) {
      return treeInsert(a, b, y);
   }
   IntervalMap &im = *this->m_map;
   intervalmapimpl::Path &path = this->m_path;

   // Try simple root leaf insert.
   unsigned size = im.getRootLeaf().insertFrom(path.getLeafOffset(), im.m_rootSize, a, b, y);

   // Was the root node insert successful?
   if (size <= RootLeaf::Capacity) {
      path.setSize(0, im.m_rootSize = size);
      return;
   }

   // Root leaf node is full, we must branch.
   IdxPair offset = im.branchRoot(path.getLeafOffset());
   path.replaceRoot(&im.getRootBranch(), im.m_rootSize, offset);

   // Now it fits in the new leaf.
   treeInsert(a, b, y);
}

template <typename KeyType, typename ValueType, unsigned N, typename Traits>
void IntervalMap<KeyType, ValueType, N, Traits>::
Iterator::treeInsert(KeyType a, KeyType b, ValueType y)
{
   using namespace intervalmapimpl;
   Path &path = this->m_path;

   if (!path.valid())
      path.legalizeForInsert(this->m_map->m_height);

   // Check if this insertion will extend the node to the left.
   if (path.getLeafOffset() == 0 && Traits::startLess(a, path.getLeaf<Leaf>().start(0))) {
      // Node is growing to the left, will it affect a left sibling node?
      if (NodeRef sib = path.getLeftSibling(path.getHeight())) {
         Leaf &sibLeaf = sib.get<Leaf>();
         unsigned sibOfs = sib.size() - 1;
         if (sibLeaf.value(sibOfs) == y &&
             Traits::adjacent(sibLeaf.stop(sibOfs), a)) {
            // This insertion will coalesce with the last entry in sibLeaf. We can
            // handle it in two ways:
            //  1. Extend sibLeaf.stop to b and be done, or
            //  2. Extend a to sibLeaf, erase the sibLeaf entry and continue.
            // We prefer 1., but need 2 when coalescing to the right as well.
            Leaf &curLeaf = path.getLeaf<Leaf>();
            path.moveLeft(path.getHeight());
            if (Traits::stopLess(b, curLeaf.start(0)) &&
                (y != curLeaf.value(0) || !Traits::adjacent(b, curLeaf.start(0)))) {
               // Easy, just extend sibLeaf and we're done.
               setNodeStop(path.getHeight(), sibLeaf.stop(sibOfs) = b);
               return;
            } else {
               // We have both left and right coalescing. Erase the old sibLeaf entry
               // and continue inserting the larger interval.
               a = sibLeaf.start(sibOfs);
               treeErase(/* UpdateRoot= */false);
            }
         }
      } else {
         // No left sibling means we are at begin(). Update cached bound.
         this->m_map->getRootBranchStart() = a;
      }
   }

   // When we are inserting at the end of a leaf node, we must update stops.
   unsigned size = path.getLeafSize();
   bool grow = path.getLeafOffset() == size;
   size = path.getLeaf<Leaf>().insertFrom(path.getLeafOffset(), size, a, b, y);

   // Leaf insertion unsuccessful? Overflow and try again.
   if (size > Leaf::Capacity) {
      overflow<Leaf>(path.getHeight());
      grow = path.getLeafOffset() == path.getLeafSize();
      size = path.getLeaf<Leaf>().insertFrom(path.getLeafOffset(), path.getLeafSize(), a, b, y);
      assert(size <= Leaf::Capacity && "overflow() didn't make room");
   }

   // Inserted, update offset and leaf size.
   path.setSize(path.getHeight(), size);

   // Insert was the last node entry, update stops.
   if (grow) {
      setNodeStop(path.getHeight(), b);
   }
}

/// erase - erase the current interval and move to the next position.
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
void IntervalMap<KeyType, ValueType, N, Traits>::
Iterator::erase()
{
   IntervalMap &im = *this->m_map;
   intervalmapimpl::Path &path = this->m_path;
   assert(path.valid() && "Cannot erase end()");
   if (this->branched()) {
      return treeErase();
   }
   im.getRootLeaf().erase(path.getLeafOffset(), im.m_rootSize);
   path.setSize(0, --im.m_rootSize);
}

/// treeErase - erase() for a branched tree.
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
void IntervalMap<KeyType, ValueType, N, Traits>::
Iterator::treeErase(bool updateRoot)
{
   IntervalMap &im = *this->m_map;
   intervalmapimpl::Path &path = this->m_path;
   Leaf &node = path.getLeaf<Leaf>();

   // nodes are not allowed to become empty.
   if (path.getLeafSize() == 1) {
      im.deleteNode(&node);
      eraseNode(im.m_height);
      // Update rootBranchStart if we erased begin().
      if (updateRoot && im.branched() && path.valid() && path.atBegin()) {
         im.getRootBranchStart() = path.getLeaf<Leaf>().start(0);
      }
      return;
   }

   // Erase current entry.
   node.erase(path.getLeafOffset(), path.getLeafSize());
   unsigned newSize = path.getLeafSize() - 1;
   path.setSize(im.m_height, newSize);
   // When we erase the last entry, update stop and move to a legal position.
   if (path.getLeafOffset() == newSize) {
      setNodeStop(im.m_height, node.stop(newSize - 1));
      path.moveRight(im.m_height);
   } else if (updateRoot && path.atBegin()) {
      im.getRootBranchStart() = path.getLeaf<Leaf>().start(0);
   }
}

/// eraseNode - Erase the current node at level from its parent and move path to
/// the first entry of the next sibling node.
/// The node must be deallocated by the caller.
/// @param level 1..height, the root node cannot be erased.
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
void IntervalMap<KeyType, ValueType, N, Traits>::
Iterator::eraseNode(unsigned level)
{
   assert(level && "Cannot erase root node");
   IntervalMap &im = *this->m_map;
   intervalmapimpl::Path &path = this->m_path;

   if (--level == 0) {
      im.getRootBranch().erase(path.offset(0), im.m_rootSize);
      path.setSize(0, --im.m_rootSize);
      // If this cleared the root, switch to height=0.
      if (im.empty()) {
         im. switchRootToLeaf();
         this->setRoot(0);
         return;
      }
   } else {
      // Remove node ref from branch node at level.
      Branch &parent = path.node<Branch>(level);
      if (path.size(level) == 1) {
         // Branch node became empty, remove it recursively.
         im.deleteNode(&parent);
         eraseNode(level);
      } else {
         // Branch node won't become empty.
         parent.erase(path.offset(level), path.size(level));
         unsigned newSize = path.size(level) - 1;
         path.setSize(level, newSize);
         // If we removed the last branch, update stop and move to a legal pos.
         if (path.offset(level) == newSize) {
            setNodeStop(level, parent.stop(newSize - 1));
            path.moveRight(level);
         }
      }
   }
   // Update path cache for the new right sibling position.
   if (path.valid()) {
      path.reset(level + 1);
      path.offset(level + 1) = 0;
   }
}

/// overflow - Distribute entries of the current node evenly among
/// its siblings and ensure that the current node is not full.
/// This may require allocating a new node.
/// @tparam NodeType The type of node at level (Leaf or Branch).
/// @param level path index of the overflowing node.
/// @return True when the tree height was changed.
template <typename KeyType, typename ValueType, unsigned N, typename Traits>
template <typename NodeType>
bool IntervalMap<KeyType, ValueType, N, Traits>::
Iterator::overflow(unsigned level)
{
   using namespace intervalmapimpl;
   Path &path = this->m_path;
   unsigned curSize[4];
   NodeType *node[4];
   unsigned nodes = 0;
   unsigned elements = 0;
   unsigned offset = path.offset(level);

   // Do we have a left sibling?
   NodeRef leftSib = path.getLeftSibling(level);
   if (leftSib) {
      offset += elements = curSize[nodes] = leftSib.size();
      node[nodes++] = &leftSib.get<NodeType>();
   }

   // Current node.
   elements += curSize[nodes] = path.size(level);
   node[nodes++] = &path.node<NodeType>(level);

   // Do we have a right sibling?
   NodeRef rightSib = path.getRightSibling(level);
   if (rightSib) {
      elements += curSize[nodes] = rightSib.size();
      node[nodes++] = &rightSib.get<NodeType>();
   }

   // Do we need to allocate a new node?
   unsigned newNode = 0;
   if (elements + 1 > nodes * NodeType::Capacity) {
      // Insert newNode at the penultimate position, or after a single node.
      newNode = nodes == 1 ? 1 : nodes - 1;
      curSize[nodes] = curSize[newNode];
      node[nodes] = node[newNode];
      curSize[newNode] = 0;
      node[newNode] = this->m_map->template newNode<NodeType>();
      ++nodes;
   }

   // Compute the new element distribution.
   unsigned newSize[4];
   IdxPair newOffset = distribute(nodes, elements, NodeType::Capacity,
                                  curSize, newSize, offset, true);
   adjust_sibling_sizes(node, nodes, curSize, newSize);

   // Move current location to the leftmost node.
   if (leftSib)
      path.moveLeft(level);

   // elements have been rearranged, now update node sizes and stops.
   bool splitRoot = false;
   unsigned pos = 0;
   while (true) {
      KeyType stop = node[pos]->stop(newSize[pos]-1);
      if (newNode && pos == newNode) {
         splitRoot = insertNode(level, NodeRef(node[pos], newSize[pos]), stop);
         level += splitRoot;
      } else {
         path.setSize(level, newSize[pos]);
         setNodeStop(level, stop);
      }
      if (pos + 1 == nodes) {
         break;
      }
      path.moveRight(level);
      ++pos;
   }

   // Where was I? Find  newOffset.
   while(pos !=  newOffset.first) {
      path.moveLeft(level);
      --pos;
   }
   path.offset(level) =  newOffset.second;
   return splitRoot;
}

//===----------------------------------------------------------------------===//
//---                       IntervalMapOverlaps                           ----//
//===----------------------------------------------------------------------===//

/// IntervalMapOverlaps - Iterate over the overlaps of mapped intervals in two
/// IntervalMaps. The maps may be different, but the KeyType and Traits types
/// should be the same.
///
/// Typical uses:
///
/// 1. Test for overlap:
///    bool overlap = IntervalMapOverlaps(a, b).valid();
///
/// 2. Enumerate overlaps:
///    for (IntervalMapOverlaps I(a, b); I.valid() ; ++I) { ... }
///
template <typename MapA, typename MapB>
class IntervalMapOverlaps
{
   using KeyType = typename MapA::KeyType;
   using Traits = typename MapA::KeyTyperaits;

   typename MapA::ConstIterator m_posA;
   typename MapB::ConstIterator m_posB;

   /// advance - Move m_posA and m_posB forward until reaching an overlap, or until
   /// either meets end.
   /// Don't move the Iterators if they are already overlapping.
   void advance()
   {
      if (!valid()) {
         return;
      }
      if (Traits::stopLess(m_posA.stop(), m_posB.start())) {
         // A ends before B begins. Catch upath.
         m_posA.advanceTo(m_posB.start());
         if (!m_posA.valid() || !Traits::stopLess(m_posB.stop(), m_posA.start()))
            return;
      } else if (Traits::stopLess(m_posB.stop(), m_posA.start())) {
         // B ends before A begins. Catch upath.
         m_posB.advanceTo(m_posA.start());
         if (!m_posB.valid() || !Traits::stopLess(m_posA.stop(), m_posB.start())) {
            return;
         }
      } else {
         // Already overlapping.
         return;
      }
      while (true) {
         // Make a.end > b.m_start.
         m_posA.advanceTo(m_posB.start());
         if (!m_posA.valid() || !Traits::stopLess(m_posB.stop(), m_posA.start())) {
            return;
         }
         // Make b.end > a.m_start.
         m_posB.advanceTo(m_posA.start());
         if (!m_posB.valid() || !Traits::stopLess(m_posA.stop(), m_posB.start())) {
            return;
         }
      }
   }

public:
   /// IntervalMapOverlaps - Create an Iterator for the overlaps of a and b.
   IntervalMapOverlaps(const MapA &a, const MapB &b)
      : m_posA(b.empty() ? a.end() : a.find(b.start())),
        m_posB(m_posA.valid() ? b.find(m_posA.start()) : b.end())
   {
      advance();
   }

   /// valid - Return true if Iterator is at an overlapath.
   bool valid() const
   {
      return m_posA.valid() && m_posB.valid();
   }

   /// a - access the left hand side in the overlapath.
   const typename MapA::ConstIterator &a() const
   {
      return m_posA;
   }

   /// b - access the right hand side in the overlapath.
   const typename MapB::ConstIterator &b() const
   {
      return m_posB;
   }

   /// start - Beginning of the overlapping interval.
   KeyType start() const
   {
      KeyType ak = a().start();
      KeyType bk = b().start();
      return Traits::startLess(ak, bk) ? bk : ak;
   }

   /// stop - End of the overlapping interval.
   KeyType stop() const
   {
      KeyType ak = a().stop();
      KeyType bk = b().stop();
      return Traits::startLess(ak, bk) ? ak : bk;
   }

   /// skipA - Move to the next overlap that doesn't involve a().
   void skipA()
   {
      ++m_posA;
      advance();
   }

   /// skipB - Move to the next overlap that doesn't involve b().
   void skipB()
   {
      ++m_posB;
      advance();
   }

   /// Preincrement - Move to the next overlapath.
   IntervalMapOverlaps &operator++()
   {
      // Bump the Iterator that ends first. The other one may have more overlaps.
      if (Traits::startLess(m_posB.stop(), m_posA.stop())) {
         skipB();
      } else {
         skipA();
      }
      return *this;
   }

   /// advanceTo - Move to the first overlapping interval with
   /// stopLess(x, stop()).
   void advanceTo(KeyType x)
   {
      if (!valid()) {
         return;
      }
      // Make sure advanceTo sees monotonic keys.
      if (Traits::stopLess(m_posA.stop(), x)) {
         m_posA.advanceTo(x);
      }

      if (Traits::stopLess(m_posB.stop(), x)) {
         m_posB.advanceTo(x);
      }
      advance();
   }
};

} // basic
} // polar

#endif //  POLARPHP_BASIC_ADT_INTERVAL_MAP_H
