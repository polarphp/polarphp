// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/06/26.

#ifndef POLARPHP_BASIC_ADT_SPARSE_SET_H
#define POLARPHP_BASIC_ADT_SPARSE_SET_H

#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/utils/Allocator.h"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <utility>

namespace polar {
namespace basic {

/// SparseSetValTraits - Objects in a SparseSet are identified by keys that can
/// be uniquely converted to a small integer less than the set's universe. This
/// class allows the set to hold values that differ from the set's key type as
/// long as an index can still be derived from the value. SparseSet never
/// directly compares ValueT, only their indices, so it can map keys to
/// arbitrary values. SparseSetValTraits computes the index from the value
/// object. To compute the index from a key, SparseSet uses a separate
/// KeyFunctorT template argument.
///
/// A simple type declaration, SparseSet<Type>, handles these cases:
/// - unsigned key, identity index, identity value
/// - unsigned key, identity index, fat value providing getSparseSetIndex()
///
/// The type declaration SparseSet<Type, UnaryFunction> handles:
/// - unsigned key, remapped index, identity value (virtual registers)
/// - pointer key, pointer-derived index, identity value (node+ID)
/// - pointer key, pointer-derived index, fat value with getSparseSetIndex()
///
/// Only other, unexpected cases require specializing SparseSetValTraits.
///
/// For best results, ValueT should not require a destructor.
///
template<typename ValueT>
struct SparseSetValTraits
{
   static unsigned getValIndex(const ValueT &value)
   {
      return value.getSparseSetIndex();
   }
};

/// SparseSetValFunctor - Helper class for selecting SparseSetValTraits. The
/// generic implementation handles ValueT classes which either provide
/// getSparseSetIndex() or specialize SparseSetValTraits<>.
///
template<typename KeyT, typename ValueT, typename KeyFunctorT>
struct SparseSetValFunctor
{
   unsigned operator()(const ValueT &value) const
   {
      return SparseSetValTraits<ValueT>::getValIndex(value);
   }
};

/// SparseSetValFunctor<KeyT, KeyT> - Helper class for the common case of
/// identity key/value sets.
template<typename KeyT, typename KeyFunctorT>
struct SparseSetValFunctor<KeyT, KeyT, KeyFunctorT>
{
   unsigned operator()(const KeyT &key) const
   {
      return KeyFunctorT()(key);
   }
};

/// SparseSet - Fast set implmentation for objects that can be identified by
/// small unsigned keys.
///
/// SparseSet allocates memory proportional to the size of the key universe, so
/// it is not recommended for building composite data structures.  It is useful
/// for algorithms that require a single set with fast operations.
///
/// Compared to DenseSet and DenseMap, SparseSet provides constant-time fast
/// clear() and iteration as fast as a vector.  The find(), insert(), and
/// erase() operations are all constant time, and typically faster than a hash
/// table.  The iteration order doesn't depend on numerical key values, it only
/// depends on the order of insert() and erase() operations.  When no elements
/// have been erased, the iteration order is the insertion order.
///
/// Compared to BitVector, SparseSet<unsigned> uses 8x-40x more memory, but
/// offers constant-time clear() and size() operations as well as fast
/// iteration independent on the size of the universe.
///
/// SparseSet contains a dense vector holding all the objects and a sparse
/// array holding indexes into the dense vector.  Most of the memory is used by
/// the sparse array which is the size of the key universe.  The SparseT
/// template parameter provides a space/speed tradeoff for sets holding many
/// elements.
///
/// When SparseT is uint32_t, find() only touches 2 cache lines, but the sparse
/// array uses 4 x Universe bytes.
///
/// When SparseT is uint8_t (the default), find() touches up to 2+[N/256] cache
/// lines, but the sparse array is 4x smaller.  N is the number of elements in
/// the set.
///
/// For sets that may grow to thousands of elements, SparseT should be set to
/// uint16_t or uint32_t.
///
/// @tparam ValueT      The type of objects in the set.
/// @tparam KeyFunctorT A functor that computes an unsigned index from KeyT.
/// @tparam SparseT     An unsigned integer type. See above.
///
template<typename ValueT,
         typename KeyFunctorT = Identity<unsigned>,
         typename SparseT = uint8_t>
class SparseSet
{
   static_assert(std::numeric_limits<SparseT>::is_integer &&
                 !std::numeric_limits<SparseT>::is_signed,
                 "SparseT must be an unsigned integer type");

   using KeyT = typename KeyFunctorT::ArgumentType;
   using DenseT = SmallVector<ValueT, 8>;
   using size_type = unsigned;
   DenseT m_dense;
   SparseT *m_sparse = nullptr;
   unsigned m_universe = 0;
   KeyFunctorT m_keyIndexOf;
   SparseSetValFunctor<KeyT, ValueT, KeyFunctorT> m_valIndexOf;

public:
   using value_type = ValueT;
   using reference = ValueT &;
   using const_reference = const ValueT &;
   using pointer = ValueT *;
   using const_pointer = const ValueT *;

   SparseSet() = default;
   SparseSet(const SparseSet &) = delete;
   SparseSet &operator=(const SparseSet &) = delete;
   ~SparseSet()
   {
      free(m_sparse);
   }

   /// setUniverse - Set the universe size which determines the largest key the
   /// set can hold.  The universe must be sized before any elements can be
   /// added.
   ///
   /// @param U Universe size. All object keys must be less than U.
   ///
   void setUniverse(unsigned universe)
   {
      // It's not hard to resize the universe on a non-empty set, but it doesn't
      // seem like a likely use case, so we can add that code when we need it.
      assert(empty() && "Can only resize universe on an empty map");
      // Hysteresis prevents needless reallocations.
      if (universe >= m_universe/4 && universe <= m_universe)
         return;
      free(m_sparse);
      // The Sparse array doesn't actually need to be initialized, so malloc
      // would be enough here, but that will cause tools like valgrind to
      // complain about branching on uninitialized data.
      m_sparse = static_cast<SparseT*>(polar::utils::safe_calloc(universe, sizeof(SparseT)));
      m_universe = universe;
   }

   // Import trivial vector stuff from DenseT.
   using iterator = typename DenseT::iterator;
   using const_iterator = typename DenseT::const_iterator;

   const_iterator begin() const
   {
      return m_dense.begin();
   }

   const_iterator end() const
   {
      return m_dense.end();
   }

   iterator begin()
   {
      return m_dense.begin();
   }

   iterator end()
   {
      return m_dense.end();
   }

   /// empty - Returns true if the set is empty.
   ///
   /// This is not the same as BitVector::empty().
   ///
   bool empty() const
   {
      return m_dense.empty();
   }

   /// size - Returns the number of elements in the set.
   ///
   /// This is not the same as BitVector::size() which returns the size of the
   /// universe.
   ///
   size_type size() const
   {
      return m_dense.getSize();
   }

   size_type getSize() const
   {
      return size();
   }

   /// clear - Clears the set.  This is a very fast constant time operation.
   ///
   void clear()
   {
      // Sparse does not need to be cleared, see find().
      m_dense.clear();
   }

   /// findIndex - Find an element by its index.
   ///
   /// @param   idx A valid index to find.
   /// @returns An iterator to the element identified by key, or end().
   ///
   iterator findIndex(unsigned idx)
   {
      assert(idx < m_universe && "Key out of range");
      const unsigned stride = std::numeric_limits<SparseT>::max() + 1u;
      for (unsigned i = m_sparse[idx], e = size(); i < e; i += stride) {
         const unsigned foundidx = m_valIndexOf(m_dense[i]);
         assert(foundidx < m_universe && "Invalid key in set. Did object mutate?");
         if (idx == foundidx) {
            return begin() + i;
         }
         // Stride is 0 when SparseT >= unsigned.  We don't need to loop.
         if (!stride) {
            break;
         }
      }
      return end();
   }

   /// find - Find an element by its key.
   ///
   /// @param   Key A valid key to find.
   /// @returns An iterator to the element identified by key, or end().
   ///
   iterator find(const KeyT &key)
   {
      return findIndex(m_keyIndexOf(key));
   }

   const_iterator find(const KeyT &Key) const
   {
      return const_cast<SparseSet*>(this)->findIndex(m_keyIndexOf(Key));
   }

   /// count - Returns 1 if this set contains an element identified by Key,
   /// 0 otherwise.
   ///
   size_type count(const KeyT &key) const
   {
      return find(key) == end() ? 0 : 1;
   }

   /// insert - Attempts to insert a new element.
   ///
   /// If Val is successfully inserted, return (I, true), where I is an iterator
   /// pointing to the newly inserted element.
   ///
   /// If the set already contains an element with the same key as Val, return
   /// (I, false), where I is an iterator pointing to the existing element.
   ///
   /// Insertion invalidates all iterators.
   ///
   std::pair<iterator, bool> insert(const ValueT &value)
   {
      unsigned idx = m_valIndexOf(value);
      iterator iter = findIndex(idx);
      if (iter != end()) {
         return std::make_pair(iter, false);
      }
      m_sparse[idx] = size();
      m_dense.push_back(value);
      return std::make_pair(end() - 1, true);
   }

   /// array subscript - If an element already exists with this key, return it.
   /// Otherwise, automatically construct a new value from Key, insert it,
   /// and return the newly inserted element.
   ValueT &operator[](const KeyT &key)
   {
      return *insert(ValueT(key)).first;
   }

   ValueT popBackValue()
   {
      // Sparse does not need to be cleared, see find().
      return m_dense.popBackValue();
   }

   /// erase - Erases an existing element identified by a valid iterator.
   ///
   /// This invalidates all iterators, but erase() returns an iterator pointing
   /// to the next element.  This makes it possible to erase selected elements
   /// while iterating over the set:
   ///
   ///   for (SparseSet::iterator I = Set.begin(); I != Set.end();)
   ///     if (test(*I))
   ///       I = Set.erase(I);
   ///     else
   ///       ++I;
   ///
   /// Note that end() changes when elements are erased, unlike std::list.
   ///
   iterator erase(iterator iter)
   {
      assert(unsigned(iter - begin()) < size() && "Invalid iterator");
      if (iter != end() - 1) {
         *iter = m_dense.back();
         unsigned backIdx = m_valIndexOf(m_dense.back());
         assert(backIdx < m_universe && "Invalid key in set. Did object mutate?");
         m_sparse[backIdx] = iter - begin();
      }
      // This depends on SmallVector::pop_back() not invalidating iterators.
      // std::vector::pop_back() doesn't give that guarantee.
      m_dense.pop_back();
      return iter;
   }

   /// erase - Erases an element identified by Key, if it exists.
   ///
   /// @param   Key The key identifying the element to erase.
   /// @returns True when an element was erased, false if no element was found.
   ///
   bool erase(const KeyT &key)
   {
      iterator iter = find(key);
      if (iter == end()) {
         return false;
      }
      erase(iter);
      return true;
   }
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_SPARSE_SET_H
