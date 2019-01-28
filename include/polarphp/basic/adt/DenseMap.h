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


#ifndef POLARPHP_BASIC_ADT_DENSE_MAP_H
#define POLARPHP_BASIC_ADT_DENSE_MAP_H

#include "polarphp/basic/adt/DenseMapInfo.h"
#include "polarphp/basic/adt/EpochTracker.h"
#include "polarphp/utils/AlignOf.h"
#include "polarphp/utils/MathExtras.h"
#include "polarphp/utils/ReverseIteration.h"
#include "polarphp/utils/TypeTraits.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <new>
#include <type_traits>
#include <utility>
#include <initializer_list>

namespace polar {
namespace basic {

using polar::utils::should_reverse_iterate;
using polar::utils::AlignedCharArrayUnion;
using polar::utils::IsPodLike;

namespace internal {

// We extend a pair to allow users to override the bucket type with their own
// implementation without requiring two members.
template <typename KeyType, typename ValueType>
struct DenseMapPair : public std::pair<KeyType, ValueType>
{
   // FIXME: Switch to inheriting constructors when we drop support for older
   //        clang versions.
   // NOTE: This default constructor is declared with '{}' rather than
   //       '= default' to work around a separate bug in clang-3.8. This can
   //       also go when we switch to inheriting constructors.
   DenseMapPair() {}

   DenseMapPair(const KeyType &Key, const ValueType &Value)
      : std::pair<KeyType, ValueType>(Key, Value) {}

   DenseMapPair(KeyType &&Key, ValueType &&Value)
      : std::pair<KeyType, ValueType>(std::move(Key), std::move(Value))
   {}

   template <typename AltKeyT, typename AltValueT>
   DenseMapPair(AltKeyT &&AltKey, AltValueT &&AltValue,
                typename std::enable_if<
                std::is_convertible<AltKeyT, KeyType>::value &&
                std::is_convertible<AltValueT, ValueType>::value>::type * = 0)
      : std::pair<KeyType, ValueType>(std::forward<AltKeyT>(AltKey),
                                      std::forward<AltValueT>(AltValue)) {}

   template <typename AltPairT>
   DenseMapPair(AltPairT &&AltPair,
                typename std::enable_if<std::is_convertible<
                AltPairT, std::pair<KeyType, ValueType>>::value>::type * = 0)
      : std::pair<KeyType, ValueType>(std::forward<AltPairT>(AltPair))
   {}

   KeyType &getFirst()
   {
      return std::pair<KeyType, ValueType>::first;
   }

   const KeyType &getFirst() const
   {
      return std::pair<KeyType, ValueType>::first;
   }

   ValueType &getSecond()
   {
      return std::pair<KeyType, ValueType>::second;
   }
   const ValueType &getSecond() const
   {
      return std::pair<KeyType, ValueType>::second;
   }
};

} // end namespace internal

template <
      typename KeyType, typename ValueType,
      typename KeyInfoType = DenseMapInfo<KeyType>,
      typename Bucket = polar::basic::internal::DenseMapPair<KeyType, ValueType>,
      bool IsConst = false>
class DenseMapIterator;

template <typename DerivedType, typename KeyType, typename ValueType, typename KeyInfoType,
          typename BucketType>
class DenseMapBase : public DebugEpochBase
{
   template <typename T>
   using ConstArgType = typename polar::utils::const_pointer_or_const_ref<T>::type;

public:
   using size_type = unsigned;
   using key_type = KeyType;
   using mapped_type = ValueType;
   using value_type = BucketType;

   using iterator = DenseMapIterator<KeyType, ValueType, KeyInfoType, BucketType>;
   using const_iterator =
   DenseMapIterator<KeyType, ValueType, KeyInfoType, BucketType, true>;

   inline iterator begin()
   {
      // When the map is empty, avoid the overhead of advancing/retreating past
      // empty buckets.
      if (empty()) {
         return end();
      }
      if (should_reverse_iterate<KeyType>()) {
         return makeIterator(getBucketsEnd() - 1, getBuckets(), *this);
      }
      return makeIterator(getBuckets(), getBucketsEnd(), *this);
   }

   inline iterator end()
   {
      return makeIterator(getBucketsEnd(), getBucketsEnd(), *this, true);
   }

   inline const_iterator begin() const
   {
      if (empty()) {
         return end();
      }
      if (should_reverse_iterate<KeyType>()) {
         return makeConstIterator(getBucketsEnd() - 1, getBuckets(), *this);
      }
      return makeConstIterator(getBuckets(), getBucketsEnd(), *this);
   }

   inline const_iterator end() const
   {
      return makeConstIterator(getBucketsEnd(), getBucketsEnd(), *this, true);
   }

   POLAR_NODISCARD bool empty() const
   {
      return getNumEntries() == 0;
   }

   unsigned getSize() const
   {
      return getNumEntries();
   }

   inline unsigned size() const
   {
      return getSize();
   }

   /// Grow the densemap so that it can contain at least \p NumEntries items
   /// before resizing again.
   void reserve(size_type numEntries)
   {
      auto numBuckets = getMinBucketTypeoReserveForEntries(numEntries);
      incrementEpoch();
      if (numBuckets > getNumBuckets()) {
         grow(numBuckets);
      }
   }

   void clear()
   {
      incrementEpoch();
      if (getNumEntries() == 0 && getNumTombstones() == 0) {
         return;
      }

      // If the capacity of the array is huge, and the # elements used is small,
      // shrink the array.
      if (getNumEntries() * 4 < getNumBuckets() && getNumBuckets() > 64) {
         shrinkAndClear();
         return;
      }
      const KeyType emptyKey = getEmptyKey(), tombstoneKey = getTombstoneKey();
      if (IsPodLike<KeyType>::value && IsPodLike<ValueType>::value) {
         // Use a simpler loop when these are trivial types.
         for (BucketType *ptr = getBuckets(), *end = getBucketsEnd(); ptr != end; ++ptr) {
            ptr->getFirst() = emptyKey;
         }
      } else {
         unsigned numEntries = getNumEntries();
         for (BucketType *ptr = getBuckets(), *end = getBucketsEnd(); ptr != end; ++ptr) {
            if (!KeyInfoType::isEqual(ptr->getFirst(), emptyKey)) {
               if (!KeyInfoType::isEqual(ptr->getFirst(), tombstoneKey)) {
                  ptr->getSecond().~ValueType();
                  --numEntries;
               }
               ptr->getFirst() = emptyKey;
            }
         }
         assert(numEntries == 0 && "Node count imbalance!");
      }
      setNumEntries(0);
      setNumTombstones(0);
   }

   /// Return 1 if the specified key is in the map, 0 otherwise.
   size_type count(ConstArgType<KeyType> value) const
   {
      const BucketType *theBucket;
      return lookupBucketFor(value, theBucket) ? 1 : 0;
   }

   iterator find(ConstArgType<KeyType> value)
   {
      BucketType *theBucket;
      if (lookupBucketFor(value, theBucket))
         return makeIterator(theBucket, getBucketsEnd(), *this, true);
      return end();
   }

   const_iterator find(ConstArgType<KeyType> value) const
   {
      const BucketType *theBucket;
      if (lookupBucketFor(value, theBucket)) {
         return makeConstIterator(theBucket, getBucketsEnd(), *this, true);
      }
      return end();
   }

   /// Alternate version of find() which allows a different, and possibly
   /// less expensive, key type.
   /// The DenseMapInfo is responsible for supplying methods
   /// getHashValue(LookupKeyType) and isEqual(LookupKeyType, KeyType) for each key
   /// type used.
   template<typename LookupKeyType>
   iterator findAs(const LookupKeyType &value)
   {
      BucketType *theBucket;
      if (lookupBucketFor(value, theBucket)) {
         return makeIterator(theBucket, getBucketsEnd(), *this, true);
      }
      return end();
   }

   template<typename LookupKeyType>
   const_iterator findAs(const LookupKeyType &value) const
   {
      const BucketType *theBucket;
      if (lookupBucketFor(value, theBucket)) {
         return makeConstIterator(theBucket, getBucketsEnd(), *this, true);
      }
      return end();
   }

   /// lookup - Return the entry for the specified key, or a default
   /// constructed value if no such entry exists.
   ValueType lookup(ConstArgType<KeyType> value) const
   {
      const BucketType *theBucket;
      if (lookupBucketFor(value, theBucket)) {
         return theBucket->getSecond();
      }
      return ValueType();
   }

   // Inserts key,value pair into the map if the key isn't already in the map.
   // If the key is already in the map, it returns false and doesn't update the
   // value.
   std::pair<iterator, bool> insert(const std::pair<KeyType, ValueType> &pair)
   {
      return tryEmplace(pair.first, pair.second);
   }

   // Inserts key,value pair into the map if the key isn't already in the map.
   // If the key is already in the map, it returns false and doesn't update the
   // value.
   std::pair<iterator, bool> insert(std::pair<KeyType, ValueType> &&pair)
   {
      return tryEmplace(std::move(pair.first), std::move(pair.second));
   }

   // Inserts key,value pair into the map if the key isn't already in the map.
   // The value is constructed in-place if the key is not in the map, otherwise
   // it is not moved.
   template <typename... Ts>
   std::pair<iterator, bool> tryEmplace(KeyType &&key, Ts &&... args)
   {
      BucketType *theBucket;
      if (lookupBucketFor(key, theBucket))
         return std::make_pair(
                  makeIterator(theBucket, getBucketsEnd(), *this, true),
                  false); // Already in map.

      // Otherwise, insert the new element.
      theBucket =
            insertIntoBucket(theBucket, std::move(key), std::forward<Ts>(args)...);
      return std::make_pair(
               makeIterator(theBucket, getBucketsEnd(), *this, true),
               true);
   }

   // Inserts key,value pair into the map if the key isn't already in the map.
   // The value is constructed in-place if the key is not in the map, otherwise
   // it is not moved.
   template <typename... Ts>
   std::pair<iterator, bool> tryEmplace(const KeyType &key, Ts &&... args)
   {
      BucketType *theBucket;
      if (lookupBucketFor(key, theBucket))
         return std::make_pair(
                  makeIterator(theBucket, getBucketsEnd(), *this, true),
                  false); // Already in map.

      // Otherwise, insert the new element.
      theBucket = insertIntoBucket(theBucket, key, std::forward<Ts>(args)...);
      return std::make_pair(
               makeIterator(theBucket, getBucketsEnd(), *this, true),
               true);
   }

   /// Alternate version of insert() which allows a different, and possibly
   /// less expensive, key type.
   /// The DenseMapInfo is responsible for supplying methods
   /// getHashValue(LookupKeyType) and isEqual(LookupKeyType, KeyType) for each key
   /// type used.
   template <typename LookupKeyType>
   std::pair<iterator, bool> insertAs(std::pair<KeyType, ValueType> &&pair,
                                      const LookupKeyType &value)
   {
      BucketType *theBucket;
      if (lookupBucketFor(value, theBucket))
         return std::make_pair(
                  makeIterator(theBucket, getBucketsEnd(), *this, true),
                  false); // Already in map.

      // Otherwise, insert the new element.
      theBucket = insertIntoBucketWithLookup(theBucket, std::move(pair.first),
                                             std::move(pair.second), value);
      return std::make_pair(
               makeIterator(theBucket, getBucketsEnd(), *this, true),
               true);
   }

   /// insert - Range insertion of pairs.
   template<typename InputIterType>
   void insert(InputIterType iter, InputIterType end)
   {
      for (; iter != end; ++iter) {
         insert(*iter);
      }
   }

   bool erase(const KeyType &value)
   {
      BucketType *theBucket;
      if (!lookupBucketFor(value, theBucket)) {
         return false; // not in map.
      }

      theBucket->getSecond().~ValueType();
      theBucket->getFirst() = getTombstoneKey();
      decrementNumEntries();
      incrementNumTombstones();
      return true;
   }

   void erase(iterator iter)
   {
      BucketType *theBucket = &*iter;
      theBucket->getSecond().~ValueType();
      theBucket->getFirst() = getTombstoneKey();
      decrementNumEntries();
      incrementNumTombstones();
   }

   value_type &findAndConstruct(const KeyType &key)
   {
      BucketType *theBucket;
      if (lookupBucketFor(key, theBucket)) {
         return *theBucket;
      }
      return *insertIntoBucket(theBucket, key);
   }

   ValueType &operator[](const KeyType &key)
   {
      return findAndConstruct(key).second;
   }

   value_type& findAndConstruct(KeyType &&key)
   {
      BucketType *theBucket;
      if (lookupBucketFor(key, theBucket)) {
         return *theBucket;
      }
      return *insertIntoBucket(theBucket, std::move(key));
   }

   ValueType &operator[](KeyType &&key)
   {
      return findAndConstruct(std::move(key)).second;
   }

   /// isPointerIntoBucketsArray - Return true if the specified pointer points
   /// somewhere into the DenseMap's array of buckets (i.e. either to a key or
   /// value in the DenseMap).
   bool isPointerIntoBucketsArray(const void *ptr) const
   {
      return ptr >= getBuckets() && ptr < getBucketsEnd();
   }

   /// getPointerIntoBucketsArray() - Return an opaque pointer into the buckets
   /// array.  In conjunction with the previous method, this can be used to
   /// determine whether an insertion caused the DenseMap to reallocate.
   const void *getPointerIntoBucketsArray() const
   {
      return getBuckets();
   }

protected:
   DenseMapBase() = default;

   void destroyAll()
   {
      if (getNumBuckets() == 0) {// Nothing to do.
         return;
      }
      const KeyType emptyKey = getEmptyKey(), tombstoneKey = getTombstoneKey();
      for (BucketType *ptr = getBuckets(), *end = getBucketsEnd(); ptr != end; ++ptr) {
         if (!KeyInfoType::isEqual(ptr->getFirst(), emptyKey) &&
             !KeyInfoType::isEqual(ptr->getFirst(), tombstoneKey)) {
            ptr->getSecond().~ValueType();
         }
         ptr->getFirst().~KeyType();
      }
   }

   void initEmpty()
   {
      setNumEntries(0);
      setNumTombstones(0);

      assert((getNumBuckets() & (getNumBuckets()-1)) == 0 &&
             "# initial buckets must be a power of two!");
      const KeyType emptyKey = getEmptyKey();
      for (BucketType *bucketIter = getBuckets(), *end = getBucketsEnd(); bucketIter != end; ++bucketIter) {
         ::new (&bucketIter->getFirst()) KeyType(emptyKey);
      }
   }

   /// Returns the number of buckets to allocate to ensure that the DenseMap can
   /// accommodate \p NumEntries without need to grow().
   unsigned getMinBucketTypeoReserveForEntries(unsigned numEntries)
   {
      // Ensure that "numEntries * 4 < numEntries * 3"
      if (numEntries == 0) {
         return 0;
      }
      // +1 is required because of the strict equality.
      // For example if NumEntries is 48, we need to return 401.
      return polar::utils::next_power_of_two(numEntries * 4 / 3 + 1);
   }

   void moveFromOldBuckets(BucketType *oldBucketsBegin, BucketType *oldBucketsEnd)
   {
      initEmpty();
      // Insert all the old elements.
      const KeyType emptyKey = getEmptyKey();
      const KeyType tombstoneKey = getTombstoneKey();
      for (BucketType *bucketIter = oldBucketsBegin, *end = oldBucketsEnd; bucketIter != end; ++bucketIter) {
         if (!KeyInfoType::isEqual(bucketIter->getFirst(), emptyKey) &&
             !KeyInfoType::isEqual(bucketIter->getFirst(), tombstoneKey)) {
            // Insert the key/value into the new table.
            BucketType *destBucket;
            bool foundVal = lookupBucketFor(bucketIter->getFirst(), destBucket);
            (void)foundVal; // silence warning.
            assert(!foundVal && "Key already in new map?");
            destBucket->getFirst() = std::move(bucketIter->getFirst());
            ::new (&destBucket->getSecond()) ValueType(std::move(bucketIter->getSecond()));
            incrementNumEntries();

            // Free the value.
            bucketIter->getSecond().~ValueType();
         }
         bucketIter->getFirst().~KeyType();
      }
   }

   template <typename OtherBaseType>
   void copyFrom(
         const DenseMapBase<OtherBaseType, KeyType, ValueType, KeyInfoType, BucketType> &other)
   {
      assert(&other != this);
      assert(getNumBuckets() == other.getNumBuckets());

      setNumEntries(other.getNumEntries());
      setNumTombstones(other.getNumTombstones());

      if (IsPodLike<KeyType>::value && IsPodLike<ValueType>::value) {
         memcpy(reinterpret_cast<void *>(getBuckets()), other.getBuckets(),
                getNumBuckets() * sizeof(BucketType));
      } else {
         for (size_t i = 0; i < getNumBuckets(); ++i) {
            ::new (&getBuckets()[i].getFirst())
                  KeyType(other.getBuckets()[i].getFirst());
            if (!KeyInfoType::isEqual(getBuckets()[i].getFirst(), getEmptyKey()) &&
                !KeyInfoType::isEqual(getBuckets()[i].getFirst(), getTombstoneKey())) {
               ::new (&getBuckets()[i].getSecond())
                     ValueType(other.getBuckets()[i].getSecond());
            }
         }
      }
   }

   static unsigned getHashValue(const KeyType &value)
   {
      return KeyInfoType::getHashValue(value);
   }

   template<typename LookupKeyType>
   static unsigned getHashValue(const LookupKeyType &value)
   {
      return KeyInfoType::getHashValue(value);
   }

   static const KeyType getEmptyKey()
   {
      static_assert(std::is_base_of<DenseMapBase, DerivedType>::value,
                    "Must pass the derived type to this template!");
      return KeyInfoType::getEmptyKey();
   }

   static const KeyType getTombstoneKey()
   {
      return KeyInfoType::getTombstoneKey();
   }

private:
   iterator makeIterator(BucketType *bucketIter, BucketType *end,
                         DebugEpochBase &epoch,
                         bool noAdvance=false)
   {
      if (should_reverse_iterate<KeyType>()) {
         BucketType *bucket = bucketIter == getBucketsEnd() ? getBuckets() : bucketIter + 1;
         return iterator(bucket, end, epoch, noAdvance);
      }
      return iterator(bucketIter, end, epoch, noAdvance);
   }

   const_iterator makeConstIterator(const BucketType *bucketIter, const BucketType *end,
                                    const DebugEpochBase &epoch,
                                    const bool noAdvance=false) const
   {
      if (should_reverse_iterate<KeyType>()) {
         const BucketType *bucket = bucketIter == getBucketsEnd() ? getBuckets() : bucketIter + 1;
         return const_iterator(bucket, end, epoch, noAdvance);
      }
      return const_iterator(bucketIter, end, epoch, noAdvance);
   }

   unsigned getNumEntries() const
   {
      return static_cast<const DerivedType *>(this)->getNumEntries();
   }

   void setNumEntries(unsigned num)
   {
      static_cast<DerivedType *>(this)->setNumEntries(num);
   }

   void incrementNumEntries()
   {
      setNumEntries(getNumEntries() + 1);
   }

   void decrementNumEntries()
   {
      setNumEntries(getNumEntries() - 1);
   }

   unsigned getNumTombstones() const
   {
      return static_cast<const DerivedType *>(this)->getNumTombstones();
   }

   void setNumTombstones(unsigned num)
   {
      static_cast<DerivedType *>(this)->setNumTombstones(num);
   }

   void incrementNumTombstones()
   {
      setNumTombstones(getNumTombstones() + 1);
   }

   void decrementNumTombstones()
   {
      setNumTombstones(getNumTombstones() - 1);
   }

   const BucketType *getBuckets() const
   {
      return static_cast<const DerivedType *>(this)->getBuckets();
   }

   BucketType *getBuckets()
   {
      return static_cast<DerivedType *>(this)->getBuckets();
   }

   unsigned getNumBuckets() const
   {
      return static_cast<const DerivedType *>(this)->getNumBuckets();
   }

   BucketType *getBucketsEnd()
   {
      return getBuckets() + getNumBuckets();
   }

   const BucketType *getBucketsEnd() const {
      return getBuckets() + getNumBuckets();
   }

   void grow(unsigned atLeast)
   {
      static_cast<DerivedType *>(this)->grow(atLeast);
   }

   void shrinkAndClear()
   {
      static_cast<DerivedType *>(this)->shrinkAndClear();
   }

   template <typename KeyArg, typename... ValueArgs>
   BucketType *insertIntoBucket(BucketType *theBucket, KeyArg &&key,
                                ValueArgs &&... values)
   {
      theBucket = insertIntoBucketImpl(key, key, theBucket);
      theBucket->getFirst() = std::forward<KeyArg>(key);
      ::new (&theBucket->getSecond()) ValueType(std::forward<ValueArgs>(values)...);
      return theBucket;
   }

   template <typename LookupKeyType>
   BucketType *insertIntoBucketWithLookup(BucketType *theBucket, KeyType &&key,
                                          ValueType &&value, LookupKeyType &lookup)
   {
      theBucket = insertIntoBucketImpl(key, lookup, theBucket);
      theBucket->getFirst() = std::move(key);
      ::new (&theBucket->getSecond()) ValueType(std::move(value));
      return theBucket;
   }

   template <typename LookupKeyType>
   BucketType *insertIntoBucketImpl(const KeyType &key, const LookupKeyType &lookup,
                                    BucketType *theBucket)
   {
      incrementEpoch();

      // If the load of the hash table is more than 3/4, or if fewer than 1/8 of
      // the buckets are empty (meaning that many are filled with tombstones),
      // grow the table.
      //
      // The later case is tricky.  For example, if we had one empty bucket with
      // tons of tombstones, failing lookups (e.g. for insertion) would have to
      // probe almost the entire table until it found the empty bucket.  If the
      // table completely filled with tombstones, no lookup would ever succeed,
      // causing infinite loops in lookup.
      unsigned newNumEntries = getNumEntries() + 1;
      unsigned numBuckets = getNumBuckets();
      if (POLAR_UNLIKELY(newNumEntries * 4 >= numBuckets * 3)) {
         this->grow(numBuckets * 2);
         lookupBucketFor(lookup, theBucket);
         numBuckets = getNumBuckets();
      } else if (POLAR_UNLIKELY(numBuckets - (newNumEntries + getNumTombstones()) <=
                                numBuckets / 8)) {
         this->grow(numBuckets);
         lookupBucketFor(lookup, theBucket);
      }
      assert(theBucket);

      // Only update the state after we've grown our bucket space appropriately
      // so that when growing buckets we have self-consistent entry count.
      incrementNumEntries();

      // If we are writing over a tombstone, remember this.
      const KeyType emptyKey = getEmptyKey();
      if (!KeyInfoType::isEqual(theBucket->getFirst(), emptyKey)) {
         decrementNumTombstones();
      }
      return theBucket;
   }

   /// lookupBucketFor - Lookup the appropriate bucket for Val, returning it in
   /// FoundBucket.  If the bucket contains the key and a value, this returns
   /// true, otherwise it returns a bucket with an empty marker or tombstone and
   /// returns false.
   template<typename LookupKeyType>
   bool lookupBucketFor(const LookupKeyType &value,
                        const BucketType *&foundBucket) const
   {
      const BucketType *bucketsPtr = getBuckets();
      const unsigned numBuckets = getNumBuckets();

      if (numBuckets == 0) {
         foundBucket = nullptr;
         return false;
      }

      // FoundTombstone - Keep track of whether we find a tombstone while probing.
      const BucketType *foundTombstone = nullptr;
      const KeyType emptyKey = getEmptyKey();
      const KeyType tombstoneKey = getTombstoneKey();
      assert(!KeyInfoType::isEqual(value, emptyKey) &&
             !KeyInfoType::isEqual(value, tombstoneKey) &&
             "Empty/Tombstone value shouldn't be inserted into map!");

      unsigned bucketNo = getHashValue(value) & (numBuckets-1);
      unsigned probeAmt = 1;
      while (true) {
         const BucketType *thisBucket = bucketsPtr + bucketNo;
         // Found Val's bucket?  If so, return it.
         if (POLAR_LIKELY(KeyInfoType::isEqual(value, thisBucket->getFirst()))) {
            foundBucket = thisBucket;
            return true;
         }

         // If we found an empty bucket, the key doesn't exist in the set.
         // Insert it and return the default value.
         if (POLAR_LIKELY(KeyInfoType::isEqual(thisBucket->getFirst(), emptyKey))) {
            // If we've already seen a tombstone while probing, fill it in instead
            // of the empty bucket we eventually probed to.
            foundBucket = foundTombstone ? foundTombstone : thisBucket;
            return false;
         }

         // If this is a tombstone, remember it.  If Val ends up not in the map, we
         // prefer to return it than something that would require more probing.
         if (KeyInfoType::isEqual(thisBucket->getFirst(), tombstoneKey) &&
             !foundTombstone) {
            foundTombstone = thisBucket;  // Remember the first tombstone found.
         }
         // Otherwise, it's a hash collision or a tombstone, continue quadratic
         // probing.
         bucketNo += probeAmt++;
         bucketNo &= (numBuckets - 1);
      }
   }

   template <typename LookupKeyType>
   bool lookupBucketFor(const LookupKeyType &value, BucketType *&foundBucket)
   {
      const BucketType *constFoundBucket;
      bool result = const_cast<const DenseMapBase *>(this)
            ->lookupBucketFor(value, constFoundBucket);
      foundBucket = const_cast<BucketType *>(constFoundBucket);
      return result;
   }

public:
   /// Return the approximate size (in bytes) of the actual map.
   /// This is just the raw memory used by DenseMap.
   /// If entries are pointers to objects, the size of the referenced objects
   /// are not included.
   size_t getMemorySize() const
   {
      return getNumBuckets() * sizeof(BucketType);
   }
};

/// Equality comparison for DenseMap.
///
/// Iterates over elements of lhs confirming that each (key, value) pair in lhs
/// is also in rhs, and that no additional pairs are in rhs.
/// Equivalent to N calls to rhs.find and N value comparisons. Amortized
/// complexity is linear, worst case is O(N^2) (if every hash collides).
template <typename DerivedT, typename KeyT, typename ValueT, typename KeyInfoT,
          typename BucketT>
bool operator==(
      const DenseMapBase<DerivedT, KeyT, ValueT, KeyInfoT, BucketT> &lhs,
      const DenseMapBase<DerivedT, KeyT, ValueT, KeyInfoT, BucketT> &rhs)
{
   if (lhs.size() != rhs.size()) {
      return false;
   }

   for (auto &kv : lhs) {
      auto iter = rhs.find(kv.first);
      if (iter == rhs.end() || iter->second != kv.second) {
         return false;
      }
   }

   return true;
}

/// Inequality comparison for DenseMap.
///
/// Equivalent to !(lhs == rhs). See operator== for performance notes.
template <typename DerivedT, typename KeyT, typename ValueT, typename KeyInfoT,
          typename BucketT>
bool operator!=(
      const DenseMapBase<DerivedT, KeyT, ValueT, KeyInfoT, BucketT> &lhs,
      const DenseMapBase<DerivedT, KeyT, ValueT, KeyInfoT, BucketT> &rhs)
{
   return !(lhs == rhs);
}


template <typename KeyType, typename ValueType,
          typename KeyInfoType = DenseMapInfo<KeyType>,
          typename BucketType = polar::basic::internal::DenseMapPair<KeyType, ValueType>>
class DenseMap : public DenseMapBase<DenseMap<KeyType, ValueType, KeyInfoType, BucketType>,
      KeyType, ValueType, KeyInfoType, BucketType>
{
   friend class DenseMapBase<DenseMap, KeyType, ValueType, KeyInfoType, BucketType>;

   // Lift some types from the dependent base class into this class for
   // simplicity of referring to them.
   using BaseType = DenseMapBase<DenseMap, KeyType, ValueType, KeyInfoType, BucketType>;

   BucketType *m_buckets;
   unsigned m_numEntries;
   unsigned m_numTombstones;
   unsigned m_numBuckets;

public:
   /// Create a DenseMap wth an optional \p InitialReserve that guarantee that
   /// this number of elements can be inserted in the map without grow()
   explicit DenseMap(unsigned initialReserve = 0)
   {
      init(initialReserve);
   }

   DenseMap(const DenseMap &other) : BaseType()
   {
      init(0);
      copyFrom(other);
   }

   DenseMap(DenseMap &&other) : BaseType()
   {
      init(0);
      swap(other);
   }

   template<typename InputIterType>
   DenseMap(const InputIterType &iter, const InputIterType &end)
   {
      init(std::distance(iter, end));
      this->insert(iter, end);
   }

   DenseMap(std::initializer_list<typename BaseType::value_type> values)
   {
      init(values.size());
      this->insert(values.begin(), values.end());
   }

   ~DenseMap()
   {
      this->destroyAll();
      operator delete(m_buckets);
   }

   void swap(DenseMap &other)
   {
      this->incrementEpoch();
      other.incrementEpoch();
      std::swap(m_buckets, other.m_buckets);
      std::swap(m_numEntries, other.m_numEntries);
      std::swap(m_numTombstones, other.m_numTombstones);
      std::swap(m_numBuckets, other.m_numBuckets);
   }

   DenseMap& operator=(const DenseMap& other)
   {
      if (&other != this) {
         copyFrom(other);
      }
      return *this;
   }

   DenseMap& operator=(DenseMap &&other)
   {
      this->destroyAll();
      operator delete(m_buckets);
      init(0);
      swap(other);
      return *this;
   }

   void copyFrom(const DenseMap& other)
   {
      this->destroyAll();
      operator delete(m_buckets);
      if (allocateBuckets(other.m_numBuckets)) {
         this->BaseType::copyFrom(other);
      } else {
         m_numEntries = 0;
         m_numTombstones = 0;
      }
   }

   void init(unsigned initNumEntries)
   {
      auto initBuckets = BaseType::getMinBucketTypeoReserveForEntries(initNumEntries);
      if (allocateBuckets(initBuckets)) {
         this->BaseType::initEmpty();
      } else {
         m_numEntries = 0;
         m_numTombstones = 0;
      }
   }

   void grow(unsigned atLeast)
   {
      unsigned oldNumBuckets = m_numBuckets;
      BucketType *oldBuckets = m_buckets;

      allocateBuckets(std::max<unsigned>(64, static_cast<unsigned>(polar::utils::next_power_of_two(atLeast - 1))));
      assert(m_buckets);
      if (!oldBuckets) {
         this->BaseType::initEmpty();
         return;
      }

      this->moveFromOldBuckets(oldBuckets, oldBuckets + oldNumBuckets);

      // Free the old table.
      operator delete(oldBuckets);
   }

   void shrinkAndClear()
   {
      unsigned oldNumEntries = m_numEntries;
      this->destroyAll();

      // Reduce the number of buckets.
      unsigned newNumBuckets = 0;
      if (oldNumEntries) {
         newNumBuckets = std::max(64, 1 << (polar::utils::log2_ceil_32(oldNumEntries) + 1));
      }
      if (newNumBuckets == m_numBuckets) {
         this->BaseType::initEmpty();
         return;
      }

      operator delete(m_buckets);
      init(newNumBuckets);
   }

private:
   unsigned getNumEntries() const
   {
      return m_numEntries;
   }

   void setNumEntries(unsigned num)
   {
      m_numEntries = num;
   }

   unsigned getNumTombstones() const
   {
      return m_numTombstones;
   }

   void setNumTombstones(unsigned num)
   {
      m_numTombstones = num;
   }

   BucketType *getBuckets() const
   {
      return m_buckets;
   }

   unsigned getNumBuckets() const
   {
      return m_numBuckets;
   }

   bool allocateBuckets(unsigned num)
   {
      m_numBuckets = num;
      if (m_numBuckets == 0) {
         m_buckets = nullptr;
         return false;
      }

      m_buckets = static_cast<BucketType*>(operator new(sizeof(BucketType) * m_numBuckets));
      return true;
   }
};

template <typename KeyType, typename ValueType, unsigned inlineBuckets = 4,
          typename KeyInfoType = DenseMapInfo<KeyType>,
          typename BucketType = polar::basic::internal::DenseMapPair<KeyType, ValueType>>
class SmallDenseMap
      : public DenseMapBase<
      SmallDenseMap<KeyType, ValueType, inlineBuckets, KeyInfoType, BucketType>, KeyType,
      ValueType, KeyInfoType, BucketType>
{
   friend class DenseMapBase<SmallDenseMap, KeyType, ValueType, KeyInfoType, BucketType>;

   // Lift some types from the dependent base class into this class for
   // simplicity of referring to them.
   using BaseType = DenseMapBase<SmallDenseMap, KeyType, ValueType, KeyInfoType, BucketType>;

   static_assert(polar::utils::is_power_of_two64(inlineBuckets),
                 "InlineBuckets must be a power of 2.");

   unsigned m_small : 1;
   unsigned m_numEntries : 31;
   unsigned m_numTombstones;

   struct LargeRep
   {
      BucketType *m_buckets;
      unsigned m_numBuckets;
   };

   /// A "union" of an inline bucket array and the struct representing
   /// a large bucket. This union will be discriminated by the 'Small' bit.
   AlignedCharArrayUnion<BucketType[inlineBuckets], LargeRep> m_storage;

public:
   explicit SmallDenseMap(unsigned numInitBuckets = 0)
   {
      init(numInitBuckets);
   }

   SmallDenseMap(const SmallDenseMap &other) : BaseType()
   {
      init(0);
      copyFrom(other);
   }

   SmallDenseMap(SmallDenseMap &&other) : BaseType()
   {
      init(0);
      swap(other);
   }

   template<typename InputIterType>
   SmallDenseMap(const InputIterType &iter, const InputIterType &end)
   {
      init(polar::utils::next_power_of_two(std::distance(iter, end)));
      this->insert(iter, end);
   }

   ~SmallDenseMap()
   {
      this->destroyAll();
      deallocateBuckets();
   }

   void swap(SmallDenseMap& other)
   {
      unsigned tmpNumEntries = other.m_numEntries;
      other.m_numEntries = m_numEntries;
      m_numEntries = tmpNumEntries;
      std::swap(m_numTombstones, other.m_numTombstones);

      const KeyType emptyKey = this->getEmptyKey();
      const KeyType tombstoneKey = this->getTombstoneKey();
      if (m_small && other.m_small) {
         // If we're swapping inline bucket arrays, we have to cope with some of
         // the tricky bits of DenseMap's storage system: the buckets are not
         // fully initialized. Thus we swap every key, but we may have
         // a one-directional move of the value.
         for (unsigned index = 0, end = inlineBuckets; index != end; ++index) {
            BucketType *lhsb = &getInlineBuckets()[index],
                  *rhsb = &other.getInlineBuckets()[index];
            bool hasLHSValue = (!KeyInfoType::isEqual(lhsb->getFirst(), emptyKey) &&
                                !KeyInfoType::isEqual(lhsb->getFirst(), tombstoneKey));
            bool hasRHSValue = (!KeyInfoType::isEqual(rhsb->getFirst(), emptyKey) &&
                                !KeyInfoType::isEqual(rhsb->getFirst(), tombstoneKey));
            if (hasLHSValue && hasRHSValue) {
               // Swap together if we can...
               std::swap(*lhsb, *rhsb);
               continue;
            }
            // Swap separately and handle any assymetry.
            std::swap(lhsb->getFirst(), rhsb->getFirst());
            if (hasLHSValue) {
               ::new (&rhsb->getSecond()) ValueType(std::move(lhsb->getSecond()));
               lhsb->getSecond().~ValueType();
            } else if (hasRHSValue) {
               ::new (&lhsb->getSecond()) ValueType(std::move(rhsb->getSecond()));
               rhsb->getSecond().~ValueType();
            }
         }
         return;
      }
      if (!m_small && !other.m_small) {
         std::swap(getLargeRep()->m_buckets, other.getLargeRep()->m_buckets);
         std::swap(getLargeRep()->m_numBuckets, other.getLargeRep()->m_numBuckets);
         return;
      }

      SmallDenseMap &smallSide = m_small ? *this : other;
      SmallDenseMap &largeSide = m_small ? other : *this;

      // First stash the large side's rep and move the small side across.
      LargeRep tmpRep = std::move(*largeSide.getLargeRep());
      largeSide.getLargeRep()->~LargeRep();
      largeSide.m_small = true;
      // This is similar to the standard move-from-old-buckets, but the bucket
      // count hasn't actually rotated in this case. So we have to carefully
      // move construct the keys and values into their new locations, but there
      // is no need to re-hash things.
      for (unsigned index = 0, end = inlineBuckets; index != end; ++index) {
         BucketType *newB = &largeSide.getInlineBuckets()[index],
               *oldB = &smallSide.getInlineBuckets()[index];
         ::new (&newB->getFirst()) KeyType(std::move(oldB->getFirst()));
         oldB->getFirst().~KeyType();
         if (!KeyInfoType::isEqual(newB->getFirst(), emptyKey) &&
             !KeyInfoType::isEqual(newB->getFirst(), tombstoneKey)) {
            ::new (&newB->getSecond()) ValueType(std::move(oldB->getSecond()));
            oldB->getSecond().~ValueType();
         }
      }

      // The hard part of moving the small buckets across is done, just move
      // the TmpRep into its new home.
      smallSide.m_small = false;
      new (smallSide.getLargeRep()) LargeRep(std::move(tmpRep));
   }

   SmallDenseMap& operator=(const SmallDenseMap& other)
   {
      if (&other != this) {
         copyFrom(other);
      }
      return *this;
   }

   SmallDenseMap& operator=(SmallDenseMap &&other)
   {
      this->destroyAll();
      deallocateBuckets();
      init(0);
      swap(other);
      return *this;
   }

   void copyFrom(const SmallDenseMap& other)
   {
      this->destroyAll();
      deallocateBuckets();
      m_small = true;
      if (other.getNumBuckets() > inlineBuckets) {
         m_small = false;
         new (getLargeRep()) LargeRep(allocateBuckets(other.getNumBuckets()));
      }
      this->BaseType::copyFrom(other);
   }

   void init(unsigned initBuckets)
   {
      m_small = true;
      if (initBuckets > inlineBuckets) {
         m_small = false;
         new (getLargeRep()) LargeRep(allocateBuckets(initBuckets));
      }
      this->BaseType::initEmpty();
   }

   void grow(unsigned atLeast)
   {
      if (atLeast >= inlineBuckets) {
         atLeast = std::max<unsigned>(64, polar::utils::next_power_of_two(atLeast-1));
      }
      if (m_small) {
         if (atLeast < inlineBuckets) {
            return; // Nothing to do.
         }
         // First move the inline buckets into a temporary storage.
         AlignedCharArrayUnion<BucketType[inlineBuckets]> tmpStorage;
         BucketType *tmpBegin = reinterpret_cast<BucketType *>(tmpStorage.m_buffer);
         BucketType *tmpEnd = tmpBegin;

         // Loop over the buckets, moving non-empty, non-tombstones into the
         // temporary storage. Have the loop move the TmpEnd forward as it goes.
         const KeyType emptyKey = this->getEmptyKey();
         const KeyType tombstoneKey = this->getTombstoneKey();
         for (BucketType *iter = getBuckets(), *end = iter + inlineBuckets; iter != end; ++iter) {
            if (!KeyInfoType::isEqual(iter->getFirst(), emptyKey) &&
                !KeyInfoType::isEqual(iter->getFirst(), tombstoneKey)) {
               assert(size_t(tmpEnd - tmpBegin) < inlineBuckets &&
                      "Too many inline buckets!");
               ::new (&tmpEnd->getFirst()) KeyType(std::move(iter->getFirst()));
               ::new (&tmpEnd->getSecond()) ValueType(std::move(iter->getSecond()));
               ++tmpEnd;
               iter->getSecond().~ValueType();
            }
            iter->getFirst().~KeyType();
         }

         // Now make this map use the large rep, and move all the entries back
         // into it.
         m_small = false;
         new (getLargeRep()) LargeRep(allocateBuckets(atLeast));
         this->moveFromOldBuckets(tmpBegin, tmpEnd);
         return;
      }

      LargeRep oldRep = std::move(*getLargeRep());
      getLargeRep()->~LargeRep();
      if (atLeast <= inlineBuckets) {
         m_small = true;
      } else {
         new (getLargeRep()) LargeRep(allocateBuckets(atLeast));
      }
      this->moveFromOldBuckets(oldRep.m_buckets, oldRep.m_buckets + oldRep.m_numBuckets);
      // Free the old table.
      operator delete(oldRep.m_buckets);
   }

   void shrinkAndClear()
   {
      unsigned oldSize = this->getSize();
      this->destroyAll();

      // Reduce the number of buckets.
      unsigned newNumBuckets = 0;
      if (oldSize) {
         newNumBuckets = 1 << (polar::utils::log2_ceil_32(oldSize) + 1);
         if (newNumBuckets > inlineBuckets && newNumBuckets < 64u) {
            newNumBuckets = 64;
         }
      }
      if ((m_small && newNumBuckets <= inlineBuckets) ||
          (!m_small && newNumBuckets == getLargeRep()->m_numBuckets)) {
         this->BaseType::initEmpty();
         return;
      }

      deallocateBuckets();
      init(newNumBuckets);
   }

private:
   unsigned getNumEntries() const
   {
      return m_numEntries;
   }

   void setNumEntries(unsigned num)
   {
      // NumEntries is hardcoded to be 31 bits wide.
      assert(num < (1U << 31) && "Cannot support more than 1<<31 entries");
      m_numEntries = num;
   }

   unsigned getNumTombstones() const
   {
      return m_numTombstones;
   }

   void setNumTombstones(unsigned num)
   {
      m_numTombstones = num;
   }

   const BucketType *getInlineBuckets() const
   {
      assert(m_small);
      // Note that this cast does not violate aliasing rules as we assert that
      // the memory's dynamic type is the small, inline bucket buffer, and the
      // 'storage.buffer' static type is 'char *'.
      return reinterpret_cast<const BucketType *>(m_storage.m_buffer);
   }

   BucketType *getInlineBuckets()
   {
      return const_cast<BucketType *>(
               const_cast<const SmallDenseMap *>(this)->getInlineBuckets());
   }

   const LargeRep *getLargeRep() const
   {
      assert(!m_small);
      // Note, same rule about aliasing as with getInlineBuckets.
      return reinterpret_cast<const LargeRep *>(m_storage.m_buffer);
   }

   LargeRep *getLargeRep()
   {
      return const_cast<LargeRep *>(
               const_cast<const SmallDenseMap *>(this)->getLargeRep());
   }

   const BucketType *getBuckets() const
   {
      return m_small ? getInlineBuckets() : getLargeRep()->m_buckets;
   }

   BucketType *getBuckets()
   {
      return const_cast<BucketType *>(
               const_cast<const SmallDenseMap *>(this)->getBuckets());
   }

   unsigned getNumBuckets() const
   {
      return m_small ? inlineBuckets : getLargeRep()->m_numBuckets;
   }

   void deallocateBuckets()
   {
      if (m_small) {
         return;
      }
      operator delete(getLargeRep()->m_buckets);
      getLargeRep()->~LargeRep();
   }

   LargeRep allocateBuckets(unsigned num)
   {
      assert(num > inlineBuckets && "Must allocate more buckets than are inline");
      LargeRep rep = {
         static_cast<BucketType*>(operator new(sizeof(BucketType) * num)), num
      };
      return rep;
   }
};

template <typename KeyType, typename ValueType, typename KeyInfoType, typename Bucket,
          bool IsConst>
class DenseMapIterator : DebugEpochBase::HandleBase
{
   friend class DenseMapIterator<KeyType, ValueType, KeyInfoType, Bucket, true>;
   friend class DenseMapIterator<KeyType, ValueType, KeyInfoType, Bucket, false>;

   using ConstIterator = DenseMapIterator<KeyType, ValueType, KeyInfoType, Bucket, true>;

public:
   using difference_type = ptrdiff_t;
   using value_type =
   typename std::conditional<IsConst, const Bucket, Bucket>::type;
   using pointer = value_type *;
   using reference = value_type &;
   using iterator_category = std::forward_iterator_tag;

private:
   pointer m_ptr = nullptr;
   pointer m_end = nullptr;

public:
   DenseMapIterator() = default;

   DenseMapIterator(pointer pos, pointer end, const DebugEpochBase &epoch,
                    bool noAdvance = false)
      : DebugEpochBase::HandleBase(&epoch), m_ptr(pos), m_end(end)
   {
      assert(isHandleInSync() && "invalid construction!");
      if (noAdvance) {
         return;
      }
      if (should_reverse_iterate<KeyType>()) {
         retreatPastEmptyBuckets();
         return;
      }
      advancePastEmptyBuckets();
   }

   // Converting ctor from non-const iterators to const iterators. SFINAE'd out
   // for const iterator destinations so it doesn't end up as a user defined copy
   // constructor.
   template <bool IsConstSrc,
             typename = typename std::enable_if<!IsConstSrc && IsConst>::type>
   DenseMapIterator(
         const DenseMapIterator<KeyType, ValueType, KeyInfoType, Bucket, IsConstSrc> &other)
      : DebugEpochBase::HandleBase(other), m_ptr(other.m_ptr), m_end(other.m_end)
   {}

   reference operator*() const
   {
      assert(isHandleInSync() && "invalid iterator access!");
      if (should_reverse_iterate<KeyType>()) {
         return m_ptr[-1];
      }
      return *m_ptr;
   }
   pointer operator->() const
   {
      assert(isHandleInSync() && "invalid iterator access!");
      if (should_reverse_iterate<KeyType>()) {
         return &(m_ptr[-1]);
      }
      return m_ptr;
   }

   bool operator==(const ConstIterator &other) const
   {
      assert((!m_ptr || isHandleInSync()) && "handle not in sync!");
      assert((!other.m_ptr || other.isHandleInSync()) && "handle not in sync!");
      assert(getEpochAddress() == other.getEpochAddress() &&
             "comparing incomparable iterators!");
      return m_ptr == other.m_ptr;
   }

   bool operator!=(const ConstIterator &other) const
   {
      assert((!m_ptr || isHandleInSync()) && "handle not in sync!");
      assert((!other.m_ptr || other.isHandleInSync()) && "handle not in sync!");
      assert(getEpochAddress() == other.getEpochAddress() &&
             "comparing incomparable iterators!");
      return m_ptr != other.m_ptr;
   }

   inline DenseMapIterator &operator++()
   {  // Preincrement
      assert(isHandleInSync() && "invalid iterator access!");
      if (should_reverse_iterate<KeyType>()) {
         --m_ptr;
         retreatPastEmptyBuckets();
         return *this;
      }
      ++m_ptr;
      advancePastEmptyBuckets();
      return *this;
   }

   DenseMapIterator operator++(int)
   {  // Postincrement
      assert(isHandleInSync() && "invalid iterator access!");
      DenseMapIterator tmp = *this;
      ++*this;
      return tmp;
   }

private:
   void advancePastEmptyBuckets()
   {
      assert(m_ptr <= m_end);
      const KeyType empty = KeyInfoType::getEmptyKey();
      const KeyType tombstone = KeyInfoType::getTombstoneKey();

      while (m_ptr != m_end && (KeyInfoType::isEqual(m_ptr->getFirst(), empty) ||
                                KeyInfoType::isEqual(m_ptr->getFirst(), tombstone))) {
         ++m_ptr;
      }
   }

   void retreatPastEmptyBuckets()
   {
      assert(m_ptr >= m_end);
      const KeyType empty = KeyInfoType::getEmptyKey();
      const KeyType tombstone = KeyInfoType::getTombstoneKey();

      while (m_ptr != m_end && (KeyInfoType::isEqual(m_ptr[-1].getFirst(), empty) ||
                                KeyInfoType::isEqual(m_ptr[-1].getFirst(), tombstone))) {
         --m_ptr;
      }
   }
};

template <typename KeyType, typename ValueType, typename KeyInfoType>
inline size_t capacity_in_bytes(const DenseMap<KeyType, ValueType, KeyInfoType> &value)
{
   return value.getMemorySize();
}

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_DENSE_MAP_H
