// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/19.

//===----------------------------------------------------------------------===//
//
// This file implements the SmallPtrSet class.  See SmallPtrSet.h for an
// overview of the algorithm.
//
//===----------------------------------------------------------------------===//

#include "polarphp/basic/adt/SmallPtrSet.h"
#include "polarphp/basic/adt/DenseMapInfo.h"
#include "polarphp/utils/MathExtras.h"
#include "polarphp/utils/ErrorHandling.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>

namespace polar {
namespace basic {

using polar::utils::log2_ceil_32;
using polar::utils::log2_ceil_64;
using polar::utils::report_bad_alloc_error;

void SmallPtrSetImplBase::shrinkAndClear()
{
   assert(!isSmall() && "Can't shrink a small set!");
   free(m_curArray);

   // Reduce the number of buckets.
   unsigned size = getSize();
   m_curArraySize = size > 16 ? 1 << (log2_ceil_32(size) + 1) : 32;
   m_numNonEmpty = m_numTombstones = 0;

   // Install the new array.  Clear all the buckets to empty.
   m_curArray = (const void**)malloc(sizeof(void*) * m_curArraySize);
   if (m_curArray == nullptr) {
      report_bad_alloc_error("Allocation of SmallPtrSet bucket array failed.");
   }
   memset(m_curArray, -1, m_curArraySize*sizeof(void*));
}

std::pair<const void *const *, bool>
SmallPtrSetImplBase::insertImplBig(const void *ptr)
{
   if (POLAR_UNLIKELY(getSize() * 4 >= m_curArraySize * 3)) {
      // If more than 3/4 of the array is full, grow.
      grow(m_curArraySize < 64 ? 128 : m_curArraySize * 2);
   } else if (POLAR_UNLIKELY(m_curArraySize - m_numNonEmpty < m_curArraySize / 8)) {
      // If fewer of 1/8 of the array is empty (meaning that many are filled with
      // tombstones), rehash.
      grow(m_curArraySize);
   }

   // Okay, we know we have space.  Find a hash bucket.
   const void **bucket = const_cast<const void**>(findBucketFor(ptr));
   if (*bucket == ptr) {
      return std::make_pair(bucket, false); // Already inserted, good.
   }
   // Otherwise, insert it!
   if (*bucket == getTombstoneMarker()) {
      --m_numTombstones;
   } else {
      ++m_numNonEmpty; // Track density.
   }
   *bucket = ptr;
   incrementEpoch();
   return std::make_pair(bucket, true);
}

const void * const *SmallPtrSetImplBase::findBucketFor(const void *ptr) const
{
   unsigned bucket = DenseMapInfo<void *>::getHashValue(ptr) & (m_curArraySize-1);
   unsigned arraySize = m_curArraySize;
   unsigned probeAmt = 1;
   const void *const *array = m_curArray;
   const void *const *tombstone = nullptr;
   while (true) {
      // If we found an empty bucket, the pointer doesn't exist in the set.
      // Return a tombstone if we've seen one so far, or the empty bucket if
      // not.
      if (POLAR_LIKELY(array[bucket] == getEmptyMarker())) {
         return tombstone ? tombstone : array + bucket;
      }
      // Found Ptr's bucket?
      if (POLAR_LIKELY(array[bucket] == ptr)) {
         return array + bucket;
      }
      // If this is a tombstone, remember it.  If Ptr ends up not in the set, we
      // prefer to return it than something that would require more probing.
      if (array[bucket] == getTombstoneMarker() && !tombstone) {
         tombstone = array + bucket;  // Remember the first tombstone found.
      }
      // It's a hash collision or a tombstone. Reprobe.
      bucket = (bucket + probeAmt++) & (arraySize-1);
   }
}

/// Grow - Allocate a larger backing store for the buckets and move it over.
///
void SmallPtrSetImplBase::grow(unsigned newSize)
{
   const void **oldBuckets = m_curArray;
   const void **oldEnd = endPointer();
   bool wasSmall = isSmall();

   // Install the new array.  Clear all the buckets to empty.
   const void **newBuckets = (const void**) malloc(sizeof(void*) * newSize);
   if (newBuckets == nullptr) {
      report_bad_alloc_error("Allocation of SmallPtrSet bucket array failed.");
   }
   // Reset member only if memory was allocated successfully
   m_curArray = newBuckets;
   m_curArraySize = newSize;
   memset(m_curArray, -1, newSize*sizeof(void*));

   // Copy over all valid entries.
   for (const void **bucketPtr = oldBuckets; bucketPtr != oldEnd; ++bucketPtr) {
      // Copy over the element if it is valid.
      const void *elt = *bucketPtr;
      if (elt != getTombstoneMarker() && elt != getEmptyMarker()) {
         *const_cast<void**>(findBucketFor(elt)) = const_cast<void*>(elt);
      }
   }
   if (!wasSmall) {
      free(oldBuckets);
   }
   m_numNonEmpty -= m_numTombstones;
   m_numTombstones = 0;
}

SmallPtrSetImplBase::SmallPtrSetImplBase(const void **smallStorage,
                                         const SmallPtrSetImplBase &that)
{
   m_smallArray = smallStorage;

   // If we're becoming small, prepare to insert into our stack space
   if (that.isSmall()) {
      m_curArray = m_smallArray;
      // Otherwise, allocate new heap space (unless we were the same size)
   } else {
      m_curArray = (const void**)malloc(sizeof(void*) * that.m_curArraySize);
      if (m_curArray == nullptr) {
         report_bad_alloc_error("Allocation of SmallPtrSet bucket array failed.");
      }
   }
   // Copy over the that array.
   copyHelper(that);
}

SmallPtrSetImplBase::SmallPtrSetImplBase(const void **smallStorage,
                                         unsigned smallSize,
                                         SmallPtrSetImplBase &&that)
{
   m_smallArray = smallStorage;
   moveHelper(smallSize, std::move(that));
}

void SmallPtrSetImplBase::copyFrom(const SmallPtrSetImplBase &other)
{
   assert(&other != this && "Self-copy should be handled by the caller.");

   if (isSmall() && other.isSmall()) {
      assert(m_curArraySize == other.m_curArraySize &&
             "Cannot assign sets with different small sizes");
   }
   // If we're becoming small, prepare to insert into our stack space
   if (other.isSmall()) {
      if (!isSmall()) {
         free(m_curArray);
      }
      m_curArray = m_smallArray;
      // Otherwise, allocate new heap space (unless we were the same size)
   } else if (m_curArraySize != other.m_curArraySize) {
      if (isSmall()) {
         m_curArray = (const void**)malloc(sizeof(void*) * other.m_curArraySize);
      } else {
         const void **temp = (const void**)realloc(m_curArray,
                                                   sizeof(void*) * other.m_curArraySize);
         if (!temp) {
            free(m_curArray);
         }
         m_curArray = temp;
      }
      if (m_curArray == nullptr) {
         report_bad_alloc_error("Allocation of SmallPtrSet bucket array failed.");
      }
   }
   copyHelper(other);
}

void SmallPtrSetImplBase::copyHelper(const SmallPtrSetImplBase &other)
{
   // Copy over the new array size
   m_curArraySize = other.m_curArraySize;
   // Copy over the contents from the other set
   std::copy(other.m_curArray, other.endPointer(), m_curArray);
   m_numNonEmpty = other.m_numNonEmpty;
   m_numTombstones = other.m_numTombstones;
}

void SmallPtrSetImplBase::moveFrom(unsigned smallSize,
                                   SmallPtrSetImplBase &&other)
{
   if (!isSmall()) {
      free(m_curArray);
   }
   moveHelper(smallSize, std::move(other));
}

void SmallPtrSetImplBase::moveHelper(unsigned smallSize,
                                     SmallPtrSetImplBase &&other)
{
   assert(&other != this && "Self-move should be handled by the caller.");
   if (other.isSmall()) {
      // Copy a small other rather than moving.
      m_curArray = m_smallArray;
      std::copy(other.m_curArray, other.m_curArray + other.m_numNonEmpty, m_curArray);
   } else {
      m_curArray = other.m_curArray;
      other.m_curArray = other.m_smallArray;
   }

   // Copy the rest of the trivial members.
   m_curArraySize = other.m_curArraySize;
   m_numNonEmpty = other.m_numNonEmpty;
   m_numTombstones = other.m_numTombstones;

   // Make the other small and empty.
   other.m_curArraySize = smallSize;
   assert(other.m_curArray == other.m_smallArray);
   other.m_numNonEmpty = 0;
   other.m_numTombstones = 0;
}

void SmallPtrSetImplBase::swap(SmallPtrSetImplBase &other)
{
   if (this == &other) {
      return;
   }
   // We can only avoid copying elements if neither set is small.
   if (!this->isSmall() && !other.isSmall()) {
      std::swap(this->m_curArray, other.m_curArray);
      std::swap(this->m_curArraySize, other.m_curArraySize);
      std::swap(this->m_numNonEmpty, other.m_numNonEmpty);
      std::swap(this->m_numTombstones, other.m_numTombstones);
      return;
   }

   // FIXME: From here on we assume that both sets have the same small size.

   // If only other is small, copy the small elements into other and move the pointer
   // from other to other.
   if (!this->isSmall() && other.isSmall()) {
      assert(other.m_curArray == other.m_smallArray);
      std::copy(other.m_curArray, other.m_curArray + other.m_numNonEmpty, this->m_smallArray);
      std::swap(other.m_curArraySize, this->m_curArraySize);
      std::swap(this->m_numNonEmpty, other.m_numNonEmpty);
      std::swap(this->m_numTombstones, other.m_numTombstones);
      other.m_curArray = this->m_curArray;
      this->m_curArray = this->m_smallArray;
      return;
   }

   // If only other is small, copy the small elements into other and move the pointer
   // from other to other.
   if (this->isSmall() && !other.isSmall()) {
      assert(this->m_curArray == this->m_smallArray);
      std::copy(this->m_curArray, this->m_curArray + this->m_numNonEmpty,
                other.m_smallArray);
      std::swap(other.m_curArraySize, this->m_curArraySize);
      std::swap(other.m_numNonEmpty, this->m_numNonEmpty);
      std::swap(other.m_numTombstones, this->m_numTombstones);
      this->m_curArray = other.m_curArray;
      other.m_curArray = other.m_smallArray;
      return;
   }

   // Both a small, just swap the small elements.
   assert(this->isSmall() && other.isSmall());
   unsigned minNonEmpty = std::min(this->m_numNonEmpty, other.m_numNonEmpty);
   std::swap_ranges(this->m_smallArray, this->m_smallArray + minNonEmpty,
                    other.m_smallArray);
   if (this->m_numNonEmpty > minNonEmpty) {
      std::copy(this->m_smallArray + minNonEmpty,
                this->m_smallArray + this->m_numNonEmpty,
                other.m_smallArray + minNonEmpty);
   } else {
      std::copy(other.m_smallArray + minNonEmpty, other.m_smallArray + other.m_numNonEmpty,
                this->m_smallArray + minNonEmpty);
   }
   assert(this->m_curArraySize == other.m_curArraySize);
   std::swap(this->m_numNonEmpty, other.m_numNonEmpty);
   std::swap(this->m_numTombstones, other.m_numTombstones);
}

} // basic
} // polar
