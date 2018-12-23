// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/14.

#include "polarphp/basic/adt/StringMap.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/utils/MathExtras.h"
#include <cassert>

namespace polar {
namespace basic {

/// Returns the number of buckets to allocate to ensure that the DenseMap can
/// accommodate \p NumEntries without need to grow().
namespace {
unsigned get_min_bucket_to_reserve_for_entries(unsigned numEntries) {
   // Ensure that "NumEntries * 4 < numBuckets * 3"
   if (numEntries == 0) {
      return 0;
   }
   // +1 is required because of the strict equality.
   // For example if NumEntries is 48, we need to return 401.
   return polar::utils::next_power_of_two(numEntries * 4 / 3 + 1);
}
} // anonympous namespace

StringMapImpl::StringMapImpl(unsigned initSize, unsigned itemSize)
{
   m_itemSize = itemSize;
   // If a size is specified, initialize the table with that many buckets.
   if (initSize) {
      // The table will grow when the number of entries reach 3/4 of the number of
      // buckets. To guarantee that "initSize" number of entries can be inserted
      // in the table without growing, we allocate just what is needed here.
      init(get_min_bucket_to_reserve_for_entries(initSize));
      return;
   }
   // Otherwise, initialize it with zero buckets to avoid the allocation.
   m_theTable = nullptr;
   m_numBuckets = 0;
   m_numItems = 0;
   m_numTombstones = 0;
}

void StringMapImpl::init(unsigned initSize)
{
   assert((initSize & (initSize-1)) == 0 &&
          "Init Size must be a power of 2 or zero!");
   unsigned newnumBuckets = initSize ? initSize : 16;
   m_numItems = 0;
   m_numTombstones = 0;
   m_theTable = (StringMapEntryBase **)calloc(newnumBuckets+1,
                                              sizeof(StringMapEntryBase **) +
                                              sizeof(unsigned));

   if (m_theTable == nullptr) {
      polar::utils::report_bad_alloc_error("Allocation of StringMap table failed.");
   }
   // Set the member only if TheTable was successfully allocated
   m_numBuckets = newnumBuckets;
   // Allocate one extra bucket, set it to look filled so the iterators stop at
   // end.
   m_theTable[m_numBuckets] = (StringMapEntryBase*)2;
}

/// LookupBucketFor - Look up the bucket that the specified string should end
/// up in.  If it already exists as a key in the map, the Item pointer for the
/// specified bucket will be non-null.  Otherwise, it will be null.  In either
/// case, the fullHashValue field of the bucket will be set to the hash value
/// of the string.
unsigned StringMapImpl::lookupBucketFor(StringRef name)
{
   unsigned htSize = m_numBuckets;
   if (htSize == 0) {  // Hash table unallocated so far?
      init(16);
      htSize = m_numBuckets;
   }
   unsigned fullHashValue = hash_string(name);
   unsigned bucketNo = fullHashValue & (htSize-1);
   unsigned *hashTable = (unsigned *)(m_theTable + m_numBuckets + 1);
   unsigned probeAmt = 1;
   int firstTombstone = -1;
   while (true) {
      StringMapEntryBase *bucketItem = m_theTable[bucketNo];
      // If we found an empty bucket, this key isn't in the table yet, return it.
      if (POLAR_LIKELY(!bucketItem)) {
         // If we found a tombstone, we want to reuse the tombstone instead of an
         // empty bucket.  This reduces probing.
         if (firstTombstone != -1) {
            hashTable[firstTombstone] = fullHashValue;
            return firstTombstone;
         }
         hashTable[bucketNo] = fullHashValue;
         return bucketNo;
      }
      if (bucketItem == getTombstoneValue()) {
         // Skip over tombstones.  However, remember the first one we see.
         if (firstTombstone == -1) {
            firstTombstone = bucketNo;
         }
      } else if (POLAR_LIKELY(hashTable[bucketNo] == fullHashValue)) {
         // If the full hash value matches, check deeply for a match.  The common
         // case here is that we are only looking at the buckets (for item info
         // being non-null and for the full hash value) not at the items.  This
         // is important for cache locality.
         // Do the comparison like this because Name isn't necessarily
         // null-terminated!
         char *itemStr = (char*)bucketItem+m_itemSize;
         if (name == StringRef(itemStr, bucketItem->getKeyLength())) {
            // We found a match!
            return bucketNo;
         }
      }
      // Okay, we didn't find the item.  Probe to the next bucket.
      bucketNo = (bucketNo+probeAmt) & (htSize-1);
      // Use quadratic probing, it has fewer clumping artifacts than linear
      // probing and has good cache behavior in the common case.
      ++probeAmt;
   }
}

/// FindKey - Look up the bucket that contains the specified key. If it exists
/// in the map, return the bucket number of the key.  Otherwise return -1.
/// This does not modify the map.
int StringMapImpl::findKey(StringRef key) const
{
   unsigned htSize = m_numBuckets;
   if (htSize == 0) {
      return -1;  // Really empty table?
   }
   unsigned fullHashValue = hash_string(key);
   unsigned bucketNo = fullHashValue & (htSize-1);
   unsigned *hashTable = (unsigned *)(m_theTable + m_numBuckets + 1);
   unsigned probeAmt = 1;
   while (true) {
      StringMapEntryBase *bucketItem = m_theTable[bucketNo];
      // If we found an empty bucket, this key isn't in the table yet, return.
      if (POLAR_LIKELY(!bucketItem)) {
         return -1;
      }
      if (bucketItem == getTombstoneValue()) {
         // Ignore tombstones.
      } else if (POLAR_LIKELY(hashTable[bucketNo] == fullHashValue)) {
         // If the full hash value matches, check deeply for a match.  The common
         // case here is that we are only looking at the buckets (for item info
         // being non-null and for the full hash value) not at the items.  This
         // is important for cache locality.
         // Do the comparison like this because NameStart isn't necessarily
         // null-terminated!
         char *itemStr = (char*)bucketItem + m_itemSize;
         if (key == StringRef(itemStr, bucketItem->getKeyLength())) {
            // We found a match!
            return bucketNo;
         }
      }
      // Okay, we didn't find the item.  Probe to the next bucket.
      bucketNo = (bucketNo+probeAmt) & (htSize-1);
      // Use quadratic probing, it has fewer clumping artifacts than linear
      // probing and has good cache behavior in the common case.
      ++probeAmt;
   }
}

/// RemoveKey - Remove the specified StringMapEntry from the table, but do not
/// delete it.  This aborts if the value isn't in the table.
void StringMapImpl::removeKey(StringMapEntryBase *value)
{
   const char *vstr = (char*)value + m_itemSize;
   StringMapEntryBase *v2 = removeKey(StringRef(vstr, value->getKeyLength()));
   (void)v2;
   assert(value == v2 && "Didn't find key?");
}

/// RemoveKey - Remove the StringMapEntry for the specified key from the
/// table, returning it.  If the key is not in the table, this returns null.
StringMapEntryBase *StringMapImpl::removeKey(StringRef key)
{
   int bucket = findKey(key);
   if (bucket == -1) {
      return nullptr;
   }
   StringMapEntryBase *result = m_theTable[bucket];
   m_theTable[bucket] = getTombstoneValue();
   --m_numItems;
   ++m_numTombstones;
   assert(m_numItems + m_numTombstones <= m_numBuckets);
   return result;
}

/// RehashTable - Grow the table, redistributing values into the buckets with
/// the appropriate mod-of-hashTable-size.
unsigned StringMapImpl::rehashTable(unsigned bucketNo)
{
   unsigned newSize;
   unsigned *hashTable = (unsigned *)(m_theTable + m_numBuckets + 1);

   // If the hash table is now more than 3/4 full, or if fewer than 1/8 of
   // the buckets are empty (meaning that many are filled with tombstones),
   // grow/rehash the table.
   if (POLAR_UNLIKELY(m_numItems * 4 > m_numBuckets * 3)) {
      newSize = m_numBuckets*2;
   } else if (POLAR_UNLIKELY(m_numBuckets - (m_numItems + m_numTombstones) <=
                             m_numBuckets / 8)) {
      newSize = m_numBuckets;
   } else {
      return bucketNo;
   }
   unsigned newbucketNo = bucketNo;
   // Allocate one extra bucket which will always be non-empty.  This allows the
   // iterators to stop at end.
   StringMapEntryBase **newTableArray =
         (StringMapEntryBase **)calloc(newSize+1, sizeof(StringMapEntryBase *) +
                                       sizeof(unsigned));
   if (newTableArray == nullptr) {
      polar::utils::report_bad_alloc_error("Allocation of StringMap hash table failed.");
   }
   unsigned *newHashArray = (unsigned *)(newTableArray + newSize + 1);
   newTableArray[newSize] = (StringMapEntryBase*)2;

   // Rehash all the items into their new buckets.  Luckily :) we already have
   // the hash values available, so we don't have to rehash any strings.
   for (unsigned index = 0, end = m_numBuckets; index != end; ++index) {
      StringMapEntryBase *bucket = m_theTable[index];
      if (bucket && bucket != getTombstoneValue()) {
         // Fast case, bucket available.
         unsigned fullHash = hashTable[index];
         unsigned newBucket = fullHash & (newSize-1);
         if (!newTableArray[newBucket]) {
            newTableArray[fullHash & (newSize-1)] = bucket;
            newHashArray[fullHash & (newSize-1)] = fullHash;
            if (index == bucketNo) {
               newbucketNo = newBucket;
            }
            continue;
         }
         // Otherwise probe for a spot.
         unsigned probeSize = 1;
         do {
            newBucket = (newBucket + probeSize++) & (newSize-1);
         } while (newTableArray[newBucket]);

         // Finally found a slot.  Fill it in.
         newTableArray[newBucket] = bucket;
         newHashArray[newBucket] = fullHash;
         if (index == bucketNo) {
            newbucketNo = newBucket;
         }
      }
   }
   free(m_theTable);
   m_theTable = newTableArray;
   m_numBuckets = newSize;
   m_numTombstones = 0;
   return newbucketNo;
}

} // basic
} // polar
