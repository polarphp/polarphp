// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/09.

//===----------------------------------------------------------------------===//
//
// This file defines the SmallPtrSet class.  See the doxygen comment for
// SmallPtrSetImplBase for more details on the algorithm used.
//
//===----------------------------------------------------------------------===//
#ifndef POLARPHP_BASIC_ADT_SMALL_PTR_SET_H
#define POLARPHP_BASIC_ADT_SMALL_PTR_SET_H

#include "polarphp/basic/adt/EpochTracker.h"
#include "polarphp/utils/ReverseIteration.h"
#include "polarphp/utils/TypeTraits.h"
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <utility>

namespace polar {
namespace basic {

using polar::utils::should_reverse_iterate;
using polar::utils::PointerLikeTypeTraits;

/// SmallPtrSetImplBase - This is the common code shared among all the
/// SmallPtrSet<>'s, which is almost everything.  SmallPtrSet has two modes, one
/// for small and one for large sets.
///
/// Small sets use an array of pointers allocated in the SmallPtrSet object,
/// which is treated as a simple array of pointers.  When a pointer is added to
/// the set, the array is scanned to see if the element already exists, if not
/// the element is 'pushed back' onto the array.  If we run out of space in the
/// array, we grow into the 'large set' case.  SmallSet should be used when the
/// sets are often small.  In this case, no memory allocation is used, and only
/// light-weight and cache-efficient scanning is used.
///
/// Large sets use a classic exponentially-probed hash table.  Empty buckets are
/// represented with an illegal pointer value (-1) to allow null pointers to be
/// inserted.  Tombstones are represented with another illegal pointer value
/// (-2), to allow deletion.  The hash table is resized when the table is 3/4 or
/// more.  When this happens, the table is doubled in size.
///
class SmallPtrSetImplBase : public DebugEpochBase
{
   friend class SmallPtrSetIteratorImpl;

protected:
   /// m_smallArray - Points to a fixed size set of buckets, used in 'small mode'.
   const void **m_smallArray;
   /// m_curArray - This is the current set of buckets.  If equal to m_smallArray,
   /// then the set is in 'small mode'.
   const void **m_curArray;
   /// m_curArraySize - The allocated size of m_curArray, always a power of two.
   unsigned m_curArraySize;

   /// Number of elements in m_curArray that contain a value or are a tombstone.
   /// If small, all these elements are at the beginning of m_curArray and the rest
   /// is uninitialized.
   unsigned m_numNonEmpty;
   /// Number of tombstones in m_curArray.
   unsigned m_numTombstones;

   // Helpers to copy and move construct a SmallPtrSet.
   SmallPtrSetImplBase(const void **smallStorage,
                       const SmallPtrSetImplBase &that);
   SmallPtrSetImplBase(const void **smallStorage, unsigned smallSize,
                       SmallPtrSetImplBase &&that);

   explicit SmallPtrSetImplBase(const void **smallStorage, unsigned smallSize)
      : m_smallArray(smallStorage), m_curArray(smallStorage),
        m_curArraySize(smallSize), m_numNonEmpty(0), m_numTombstones(0)
   {
      assert(smallSize && (smallSize & (smallSize-1)) == 0 &&
             "Initial size must be a power of two!");
   }

   ~SmallPtrSetImplBase()
   {
      if (!isSmall()) {
         free(m_curArray);
      }
   }

public:
   using size_type = unsigned;

   SmallPtrSetImplBase &operator=(const SmallPtrSetImplBase &) = delete;

   POLAR_NODISCARD bool empty() const
   {
      return getSize() == 0;
   }
   size_type getSize() const
   {
      return m_numNonEmpty - m_numTombstones;
   }

   void clear()
   {
      incrementEpoch();
      // If the capacity of the array is huge, and the # elements used is small,
      // shrink the array.
      if (!isSmall()) {
         if (getSize() * 4 < m_curArraySize && m_curArraySize > 32) {
            return shrinkAndClear();
         }
         // Fill the array with empty markers.
         memset(m_curArray, -1, m_curArraySize * sizeof(void *));
      }
      m_numNonEmpty = 0;
      m_numTombstones = 0;
   }

protected:
   static void *getTombstoneMarker()
   {
      return reinterpret_cast<void*>(-2);
   }

   static void *getEmptyMarker()
   {
      // Note that -1 is chosen to make clear() efficiently implementable with
      // memset and because it's not a valid pointer value.
      return reinterpret_cast<void*>(-1);
   }

   const void **endPointer() const
   {
      return isSmall() ? m_curArray + m_numNonEmpty : m_curArray + m_curArraySize;
   }

   /// insertImpl - This returns true if the pointer was new to the set, false if
   /// it was already in the set.  This is hidden from the client so that the
   /// derived class can check that the right type of pointer is passed in.
   std::pair<const void *const *, bool> insertImpl(const void *ptr)
   {
      if (isSmall()) {
         // Check to see if it is already in the set.
         const void **lastTombstone = nullptr;
         for (const void **aptr = m_smallArray, **end = m_smallArray + m_numNonEmpty;
              aptr != end; ++aptr) {
            const void *value = *aptr;
            if (value == ptr)
               return std::make_pair(aptr, false);
            if (value == getTombstoneMarker()) {
               lastTombstone = aptr;
            }
         }

         // Did we find any tombstone marker?
         if (lastTombstone != nullptr) {
            *lastTombstone = ptr;
            --m_numTombstones;
            incrementEpoch();
            return std::make_pair(lastTombstone, true);
         }
         // Nope, there isn't.  If we stay small, just 'pushback' now.
         if (m_numNonEmpty < m_curArraySize) {
            m_smallArray[m_numNonEmpty++] = ptr;
            incrementEpoch();
            return std::make_pair(m_smallArray + (m_numNonEmpty - 1), true);
         }
         // Otherwise, hit the big set case, which will call grow.
      }
      return insertImplBig(ptr);
   }

   /// eraseImpl - If the set contains the specified pointer, remove it and
   /// return true, otherwise return false.  This is hidden from the client so
   /// that the derived class can check that the right type of pointer is passed
   /// in.
   bool eraseImpl(const void * ptr)
   {
      const void *const *p = findImpl(ptr);
      if (p == endPointer()) {
         return false;
      }
      const void **loc = const_cast<const void **>(p);
      assert(*loc == ptr && "broken find!");
      *loc = getTombstoneMarker();
      m_numTombstones++;
      return true;
   }

   /// Returns the raw pointer needed to construct an iterator.  If element not
   /// found, this will be endPointer.  Otherwise, it will be a pointer to the
   /// slot which stores Ptr;
   const void *const * findImpl(const void * ptr) const
   {
      if (isSmall()) {
         // Linear search for the item.
         for (const void *const *aptr = m_smallArray,
              *const *end = m_smallArray + m_numNonEmpty; aptr != end; ++aptr) {
            if (*aptr == ptr) {
               return aptr;
            }
         }
         return endPointer();
      }

      // Big set case.
      auto *bucket = findBucketFor(ptr);
      if (*bucket == ptr) {
         return bucket;
      }
      return endPointer();
   }

private:
   bool isSmall() const
   {
      return m_curArray == m_smallArray;
   }

   std::pair<const void *const *, bool> insertImplBig(const void *ptr);

   const void * const *findBucketFor(const void *ptr) const;
   void shrinkAndClear();

   /// Grow - Allocate a larger backing store for the buckets and move it over.
   void grow(unsigned newSize);

protected:
   /// swap - Swaps the elements of two sets.
   /// Note: This method assumes that both sets have the same small size.
   void swap(SmallPtrSetImplBase &other);

   void copyFrom(const SmallPtrSetImplBase &other);
   void moveFrom(unsigned smallSize, SmallPtrSetImplBase &&other);

private:
   /// Code shared by MoveFrom() and move constructor.
   void moveHelper(unsigned smallSize, SmallPtrSetImplBase &&other);
   /// Code shared by CopyFrom() and copy constructor.
   void copyHelper(const SmallPtrSetImplBase &other);
};

/// SmallPtrSetIteratorImpl - This is the common base class shared between all
/// instances of SmallPtrSetIterator.
class SmallPtrSetIteratorImpl
{
protected:
   const void *const *m_bucket;
   const void *const *m_end;

public:
   explicit SmallPtrSetIteratorImpl(const void *const *bucketPtr, const void*const *end)
      : m_bucket(bucketPtr), m_end(end) {
      if (should_reverse_iterate()) {
         retreatIfNotValid();
         return;
      }
      advanceIfNotValid();
   }

   bool operator==(const SmallPtrSetIteratorImpl &other) const
   {
      return m_bucket == other.m_bucket;
   }

   bool operator!=(const SmallPtrSetIteratorImpl &other) const
   {
      return m_bucket != other.m_bucket;
   }

protected:
   /// AdvanceIfNotValid - If the current m_bucket isn't valid, advance to a m_bucket
   /// that is.   This is guaranteed to stop because the m_end() m_bucket is marked
   /// valid.
   void advanceIfNotValid()
   {
      assert(m_bucket <= m_end);
      while (m_bucket != m_end &&
             (*m_bucket == SmallPtrSetImplBase::getEmptyMarker() ||
              *m_bucket == SmallPtrSetImplBase::getTombstoneMarker())) {
         ++m_bucket;
      }
   }

   void retreatIfNotValid()
   {
      assert(m_bucket >= m_end);
      while (m_bucket != m_end &&
             (m_bucket[-1] == SmallPtrSetImplBase::getEmptyMarker() ||
              m_bucket[-1] == SmallPtrSetImplBase::getTombstoneMarker())) {
         --m_bucket;
      }
   }
};

/// SmallPtrSetIterator - This implements a const_iterator for SmallPtrSet.
template <typename PtrType>
class SmallPtrSetIterator : public SmallPtrSetIteratorImpl,
      DebugEpochBase::HandleBase
{
   using PtrTraits = PointerLikeTypeTraits<PtrType>;

public:
   using value_type = PtrType;
   using reference = PtrType;
   using pointer = PtrType;
   using difference_type = std::ptrdiff_t;
   using iterator_category = std::forward_iterator_tag;

   explicit SmallPtrSetIterator(const void *const *bucketPtr, const void *const *end,
                                const DebugEpochBase &epoch)
      : SmallPtrSetIteratorImpl(bucketPtr, end), DebugEpochBase::HandleBase(&epoch)
   {}

   // Most methods provided by baseclass.

   const PtrType operator*() const
   {
      assert(isHandleInSync() && "invalid iterator access!");
      if (should_reverse_iterate()) {
         assert(m_bucket > m_end);
         return PtrTraits::getFromVoidPointer(const_cast<void *>(m_bucket[-1]));
      }
      assert(m_bucket < m_end);
      return PtrTraits::getFromVoidPointer(const_cast<void*>(*m_bucket));
   }

   inline SmallPtrSetIterator& operator++()
   {          // Preincrement
      assert(isHandleInSync() && "invalid iterator access!");
      if (should_reverse_iterate()) {
         --m_bucket;
         retreatIfNotValid();
         return *this;
      }
      ++m_bucket;
      advanceIfNotValid();
      return *this;
   }

   SmallPtrSetIterator operator++(int)
   {        // Postincrement
      SmallPtrSetIterator tmp = *this;
      ++*this;
      return tmp;
   }
};

/// RoundUpToPowerOfTwo - This is a helper template that rounds N up to the next
/// power of two (which means N itself if N is already a power of two).
template<unsigned N>
struct RoundUpToPowerOfTwo;

/// RoundUpToPowerOfTwoH - If N is not a power of two, increase it.  This is a
/// helper template used to implement RoundUpToPowerOfTwo.
template<unsigned N, bool isPowerTwo>
struct RoundUpToPowerOfTwoH
{
   enum { Val = N };
};

template<unsigned N>
struct RoundUpToPowerOfTwoH<N, false>
{
   enum {
      // We could just use NextVal = N+1, but this converges faster.  N|(N-1) sets
      // the right-most zero bits to one all at once, e.g. 0b0011000 -> 0b0011111.
      Val = RoundUpToPowerOfTwo<(N|(N-1)) + 1>::Val
   };
};

template<unsigned N>
struct RoundUpToPowerOfTwo
{
   enum { Val = RoundUpToPowerOfTwoH<N, (N&(N-1)) == 0>::Val };
};

/// A templated base class for \c SmallPtrSet which provides the
/// typesafe interface that is common across all small sizes.
///
/// This is particularly useful for passing around between interface boundaries
/// to avoid encoding a particular small size in the interface boundary.
template <typename PtrTypepe>
class SmallPtrSetImpl : public SmallPtrSetImplBase
{
   using ConstPtrTypepe = typename polar::utils::add_const_past_pointer<PtrTypepe>::type;
   using PtrTraits = PointerLikeTypeTraits<PtrTypepe>;
   using ConstPtrTraits = PointerLikeTypeTraits<ConstPtrTypepe>;

protected:
   // Constructors that forward to the base.
   SmallPtrSetImpl(const void **smallStorage, const SmallPtrSetImpl &that)
      : SmallPtrSetImplBase(smallStorage, that)
   {}
   SmallPtrSetImpl(const void **smallStorage, unsigned smallSize,
                   SmallPtrSetImpl &&that)
      : SmallPtrSetImplBase(smallStorage, smallSize, std::move(that))
   {}
   explicit SmallPtrSetImpl(const void **smallStorage, unsigned smallSize)
      : SmallPtrSetImplBase(smallStorage, smallSize)
   {}

public:
   using iterator = SmallPtrSetIterator<PtrTypepe>;
   using const_iterator = SmallPtrSetIterator<PtrTypepe>;
   using key_type = ConstPtrTypepe;
   using value_type = PtrTypepe;

   SmallPtrSetImpl(const SmallPtrSetImpl &) = delete;

   /// Inserts Ptr if and only if there is no element in the container equal to
   /// Ptr. The bool component of the returned pair is true if and only if the
   /// insertion takes place, and the iterator component of the pair points to
   /// the element equal to Ptr.
   std::pair<iterator, bool> insert(PtrTypepe ptr)
   {
      auto p = insertImpl(PtrTraits::getAsVoidPointer(ptr));
      return std::make_pair(makeIterator(p.first), p.second);
   }

   /// erase - If the set contains the specified pointer, remove it and return
   /// true, otherwise return false.
   bool erase(PtrTypepe ptr)
   {
      return eraseImpl(PtrTraits::getAsVoidPointer(ptr));
   }
   /// count - Return 1 if the specified pointer is in the set, 0 otherwise.
   size_type count(ConstPtrTypepe ptr) const
   {
      return find(ptr) != end() ? 1 : 0;
   }

   iterator find(ConstPtrTypepe ptr) const
   {
      return makeIterator(findImpl(ConstPtrTraits::getAsVoidPointer(ptr)));
   }

   template <typename IterType>
   void insert(IterType iter, IterType end) {
      for (; iter != end; ++iter) {
         insert(*iter);
      }
   }

   void insert(std::initializer_list<PtrTypepe> list)
   {
      insert(list.begin(), list.end());
   }

   iterator begin() const
   {
      if (should_reverse_iterate()) {
         return makeIterator(endPointer() - 1);
      }
      return makeIterator(m_curArray);
   }

   iterator end() const
   {
      return makeIterator(endPointer());
   }

private:
   /// Create an iterator that dereferences to same place as the given pointer.
   iterator makeIterator(const void *const *ptr) const
   {
      if (should_reverse_iterate()) {
         return iterator(ptr == endPointer() ? m_curArray : ptr + 1, m_curArray, *this);
      }
      return iterator(ptr, endPointer(), *this);
   }
};

/// SmallPtrSet - This class implements a set which is optimized for holding
/// smallSize or less elements.  This internally rounds up smallSize to the next
/// power of two if it is not already a power of two.  See the comments above
/// SmallPtrSetImplBase for details of the algorithm.
template<typename PtrTypepe, unsigned smallSize>
class SmallPtrSet : public SmallPtrSetImpl<PtrTypepe>
{
   // In small mode SmallPtrSet uses linear search for the elements, so it is
   // not a good idea to choose this value too high. You may consider using a
   // DenseSet<> instead if you expect many elements in the set.
   static_assert(smallSize <= 32, "smallSize should be small");

   using BaseType = SmallPtrSetImpl<PtrTypepe>;

   // Make sure that smallSize is a power of two, round up if not.
   enum { smallSizePowTwo = RoundUpToPowerOfTwo<smallSize>::Val };
   /// smallStorage - Fixed size storage used in 'small mode'.
   const void *smallStorage[smallSizePowTwo];

public:
   SmallPtrSet() : BaseType(smallStorage, smallSizePowTwo)
   {}

   SmallPtrSet(const SmallPtrSet &that) : BaseType(smallStorage, that)
   {}

   SmallPtrSet(SmallPtrSet &&that)
      : BaseType(smallStorage, smallSizePowTwo, std::move(that))
   {}

   template<typename IterType>
   SmallPtrSet(IterType iter, IterType end) : BaseType(smallStorage, smallSizePowTwo)
   {
      this->insert(iter, end);
   }

   SmallPtrSet(std::initializer_list<PtrTypepe> list)
      : BaseType(smallStorage, smallSizePowTwo)
   {
      this->insert(list.begin(), list.end());
   }

   SmallPtrSet<PtrTypepe, smallSize> &
   operator=(const SmallPtrSet<PtrTypepe, smallSize> &other) {
      if (&other != this) {
         this->copyFrom(other);
      }
      return *this;
   }

   SmallPtrSet<PtrTypepe, smallSize> &
   operator=(SmallPtrSet<PtrTypepe, smallSize> &&other)
   {
      if (&other != this) {
         this->moveFrom(smallSizePowTwo, std::move(other));
      }
      return *this;
   }

   SmallPtrSet<PtrTypepe, smallSize> &
   operator=(std::initializer_list<PtrTypepe> list)
   {
      this->clear();
      this->insert(list.begin(), list.end());
      return *this;
   }

   /// swap - Swaps the elements of two sets.
   void swap(SmallPtrSet<PtrTypepe, smallSize> &other)
   {
      SmallPtrSetImplBase::swap(other);
   }
};

} // basic
} // polar

namespace std {

/// Implement std::swap in terms of SmallPtrSet swap.
template<typename T, unsigned N>
inline void swap(polar::basic::SmallPtrSet<T, N> &lhs, polar::basic::SmallPtrSet<T, N> &rhs)
{
   lhs.swap(rhs);
}

} // end namespace std

#endif // POLARPHP_BASIC_ADT_SMALL_PTR_SET_H
