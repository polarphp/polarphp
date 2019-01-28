// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/04.

#ifndef POLARPHP_BASIC_ADT_STRING_MAP_H
#define POLARPHP_BASIC_ADT_STRING_MAP_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Iterator.h"
#include "polarphp/basic/adt/IteratorRange.h"
#include "polarphp/utils/Allocator.h"
#include "polarphp/utils/PointerLikeTypeTraits.h"
#include "polarphp/utils/ErrorHandling.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <utility>

namespace polar {
namespace basic {

template<typename ValueType>
class StringMapConstIterator;

template<typename ValueType>
class StringMapIterator;

template<typename ValueType>
class StringMapKeyIterator;

using polar::utils::MallocAllocator;
using polar::utils::PointerLikeTypeTraits;

/// StringMapEntryBase - Shared base class of StringMapEntry instances.
class StringMapEntryBase
{
   size_t m_strLength;
public:
   explicit StringMapEntryBase(size_t length)
      : m_strLength(length)
   {}

   size_t getKeyLength() const
   {
      return m_strLength;
   }
};

/// StringMapImpl - This is the base class of StringMap that is shared among
/// all of its instantiations.
class StringMapImpl
{
protected:
   // Array of m_numBuckets pointers to entries, null pointers are holes.
   // m_theTable[m_numBuckets] contains a sentinel value for easy iteration. Followed
   // by an array of the actual hash values as unsigned integers.
   StringMapEntryBase **m_theTable = nullptr;
   unsigned m_numBuckets = 0;
   unsigned m_numItems = 0;
   unsigned m_numTombstones = 0;
   unsigned m_itemSize;

protected:
   explicit StringMapImpl(unsigned itemSize)
      : m_itemSize(itemSize)
   {}
   StringMapImpl(StringMapImpl &&rhs)
      : m_theTable(rhs.m_theTable), m_numBuckets(rhs.m_numBuckets),
        m_numItems(rhs.m_numItems), m_numTombstones(rhs.m_numTombstones),
        m_itemSize(rhs.m_itemSize)
   {
      rhs.m_theTable = nullptr;
      rhs.m_numBuckets = 0;
      rhs.m_numItems = 0;
      rhs.m_numTombstones = 0;
   }

   StringMapImpl(unsigned initSize, unsigned itemSize);
   unsigned rehashTable(unsigned bucketNo = 0);

   /// LookupBucketFor - Look up the bucket that the specified string should end
   /// up in.  If it already exists as a key in the map, the Item pointer for the
   /// specified bucket will be non-null.  Otherwise, it will be null.  In either
   /// case, the FullHashValue field of the bucket will be set to the hash value
   /// of the string.
   unsigned lookupBucketFor(StringRef key);

   /// FindKey - Look up the bucket that contains the specified key. If it exists
   /// in the map, return the bucket number of the key.  Otherwise return -1.
   /// This does not modify the map.
   int findKey(StringRef key) const;

   /// RemoveKey - Remove the specified StringMapEntry from the table, but do not
   /// delete it.  This aborts if the value isn't in the table.
   void removeKey(StringMapEntryBase *value);

   /// RemoveKey - Remove the StringMapEntry for the specified key from the
   /// table, returning it.  If the key is not in the table, this returns null.
   StringMapEntryBase *removeKey(StringRef key);

   /// Allocate the table with the specified number of buckets and otherwise
   /// setup the map as empty.
   void init(unsigned size);

public:
   static StringMapEntryBase *getTombstoneValue()
   {
      uintptr_t value = static_cast<uintptr_t>(-1);
      value <<= PointerLikeTypeTraits<StringMapEntryBase *>::NumLowBitsAvailable;
      return reinterpret_cast<StringMapEntryBase *>(value);
   }

   unsigned getNumBuckets() const
   {
      return m_numBuckets;
   }

   unsigned getNumItems() const
   {
      return m_numItems;
   }

   bool empty() const
   {
      return m_numItems == 0;
   }

   unsigned getSize() const
   {
      return m_numItems;
   }

   void swap(StringMapImpl &other)
   {
      std::swap(m_theTable, other.m_theTable);
      std::swap(m_numBuckets, other.m_numBuckets);
      std::swap(m_numItems, other.m_numItems);
      std::swap(m_numTombstones, other.m_numTombstones);
   }
};

/// StringMapEntry - This is used to represent one value that is inserted into
/// a StringMap.  It contains the Value itself and the key: the string length
/// and data.
template<typename ValueType>
class StringMapEntry : public StringMapEntryBase
{
public:
   ValueType m_second;

   explicit StringMapEntry(size_t strLen)
      : StringMapEntryBase(strLen), m_second()
   {}

   template <typename... InitType>
   StringMapEntry(size_t strLen, InitType &&... initVals)
      : StringMapEntryBase(strLen), m_second(std::forward<InitType>(initVals)...)
   {}

   StringMapEntry(StringMapEntry &entry) = delete;

   StringRef getKey() const
   {
      return StringRef(getKeyData(), getKeyLength());
   }

   const ValueType &getValue() const
   {
      return m_second;
   }

   ValueType &getValue()
   {
      return m_second;
   }

   void setValue(const ValueType &value)
   {
      m_second = value;
   }

   /// getKeyData - Return the start of the string data that is the key for this
   /// value.  The string data is always stored immediately after the
   /// StringMapEntry object.
   const char *getKeyData() const
   {
      return reinterpret_cast<const char*>(this + 1);
   }

   StringRef getFirst() const
   {
      return StringRef(getKeyData(), getKeyLength());
   }

   /// Create a StringMapEntry for the specified key construct the value using
   /// \p InitiVals.
   template <typename AllocatorType, typename... InitType>
   static StringMapEntry *create(StringRef key, AllocatorType &allocator,
                                 InitType &&... initVals)
   {
      unsigned keyLength = key.getSize();

      // Allocate a new item with space for the string at the end and a null
      // terminator.
      unsigned allocSize = static_cast<unsigned>(sizeof(StringMapEntry))+
            keyLength+1;
      unsigned alignment = alignof(StringMapEntry);

      StringMapEntry *newItem =
            static_cast<StringMapEntry*>(allocator.allocate(allocSize, alignment));

      if (newItem == nullptr) {
         polar::utils::report_bad_alloc_error("Allocation of StringMap entry failed.");
      }
      // Construct the value.
      new (newItem) StringMapEntry(keyLength, std::forward<InitType>(initVals)...);

      // Copy the string information.
      char *strBuffer = const_cast<char*>(newItem->getKeyData());
      if (keyLength > 0) {
         memcpy(strBuffer, key.getData(), keyLength);
      }
      strBuffer[keyLength] = 0;  // Null terminate for convenience of clients.
      return newItem;
   }

   /// Create - Create a StringMapEntry with normal malloc/free.
   template <typename... InitTypepe>
   static StringMapEntry *create(StringRef key, InitTypepe &&... initVal)
   {
      MallocAllocator allocator;
      return create(key, allocator, std::forward<InitTypepe>(initVal)...);
   }

   static StringMapEntry *create(StringRef key)
   {
      return create(key, ValueType());
   }

   /// GetStringMapEntryFromKeyData - Given key data that is known to be embedded
   /// into a StringMapEntry, return the StringMapEntry itself.
   static StringMapEntry &getStringMapEntryFromKeyData(const char *keyData)
   {
      char *ptr = const_cast<char*>(keyData) - sizeof(StringMapEntry<ValueType>);
      return *reinterpret_cast<StringMapEntry*>(ptr);
   }

   /// Destroy - Destroy this StringMapEntry, releasing memory back to the
   /// specified allocator.
   template<typename AllocatorType>
   void destroy(AllocatorType &allocator)
   {
      // Free memory referenced by the item.
      unsigned allocSize =
            static_cast<unsigned>(sizeof(StringMapEntry)) + getKeyLength() + 1;
      this->~StringMapEntry();
      allocator.deallocate(static_cast<void *>(this), allocSize);
   }

   /// Destroy this object, releasing memory back to the malloc allocator.
   void destroy()
   {
      MallocAllocator allocator;
      destroy(allocator);
   }
};

/// StringMap - This is an unconventional map that is specialized for handling
/// keys that are "strings", which are basically ranges of bytes. This does some
/// funky memory allocation and hashing things to make it extremely efficient,
/// storing the string data *after* the value in the map.
template<typename ValueType, typename AllocatorType = MallocAllocator>
class StringMap : public StringMapImpl
{
   AllocatorType m_allocator;

public:
   using MapEntryType = StringMapEntry<ValueType>;

   StringMap() : StringMapImpl(static_cast<unsigned>(sizeof(MapEntryType)))
   {}

   explicit StringMap(unsigned initialSize)
      : StringMapImpl(initialSize, static_cast<unsigned>(sizeof(MapEntryType)))
   {}

   explicit StringMap(AllocatorType allocator)
      : StringMapImpl(static_cast<unsigned>(sizeof(MapEntryType))), m_allocator(allocator)
   {}

   StringMap(unsigned initialSize, AllocatorType allocator)
      : StringMapImpl(initialSize, static_cast<unsigned>(sizeof(MapEntryType))),
        m_allocator(allocator)
   {}

   StringMap(std::initializer_list<std::pair<StringRef, ValueType>> list)
      : StringMapImpl(list.size(), static_cast<unsigned>(sizeof(MapEntryType)))
   {
      for (const auto &item : list) {
         insert(item);
      }
   }

   StringMap(StringMap &&rhs)
      : StringMapImpl(std::move(rhs)), m_allocator(std::move(rhs.m_allocator))
   {}

   StringMap(const StringMap &other) :
      StringMapImpl(static_cast<unsigned>(sizeof(MapEntryType))),
      m_allocator(other.m_allocator)
   {
      if (other.empty()) {
         return;
      }
      // Allocate m_theTable of the same size as RHS's m_theTable, and set the
      // sentinel appropriately (and m_numBuckets).
      init(other.m_numBuckets);
      unsigned *hashTable = (unsigned *)(m_theTable + m_numBuckets + 1),
            *rhsHashTable = (unsigned *)(other.m_theTable + m_numBuckets + 1);

      m_numItems = other.m_numItems;
      m_numTombstones = other.m_numTombstones;
      for (unsigned index = 0, end = m_numBuckets; index != end; ++index) {
         StringMapEntryBase *bucket = other.m_theTable[index];
         if (!bucket || bucket == getTombstoneValue()) {
            m_theTable[index] = bucket;
            continue;
         }

         m_theTable[index] = MapEntryType::create(
                  static_cast<MapEntryType *>(bucket)->getKey(), m_allocator,
                  static_cast<MapEntryType *>(bucket)->getValue());
         hashTable[index] = rhsHashTable[index];
      }

      // Note that here we've copied everything from the RHS into this object,
      // tombstones included. We could, instead, have re-probed for each key to
      // instantiate this new object without any tombstone buckets. The
      // assumption here is that items are rarely deleted from most StringMaps,
      // and so tombstones are rare, so the cost of re-probing for all inputs is
      // not worthwhile.
   }

   StringMap &operator=(StringMap rhs)
   {
      StringMapImpl::swap(rhs);
      std::swap(m_allocator, rhs.m_allocator);
      return *this;
   }

   ~StringMap()
   {
      // Delete all the elements in the map, but don't reset the elements
      // to default values.  This is a copy of clear(), but avoids unnecessary
      // work not required in the destructor.
      if (!empty()) {
         for (unsigned index = 0, end = m_numBuckets; index != end; ++index) {
            StringMapEntryBase *bucket = m_theTable[index];
            if (bucket && bucket != getTombstoneValue()) {
               static_cast<MapEntryType*>(bucket)->destroy(m_allocator);
            }
         }
      }
      free(m_theTable);
   }

   AllocatorType &getAllocator()
   {
      return m_allocator;
   }

   const AllocatorType &getAllocator() const
   {
      return m_allocator;
   }

   using key_type = const char*;
   using mapped_type = ValueType;
   using value_type = StringMapEntry<ValueType>;
   using size_type = size_t;

   using const_iterator = StringMapConstIterator<ValueType>;
   using iterator = StringMapIterator<ValueType>;

   iterator begin()
   {
      return iterator(m_theTable, m_numBuckets == 0);
   }

   iterator end()
   {
      return iterator(m_theTable + m_numBuckets, true);
   }

   const_iterator begin() const
   {
      return const_iterator(m_theTable, m_numBuckets == 0);
   }

   const_iterator end() const
   {
      return const_iterator(m_theTable + m_numBuckets, true);
   }

   IteratorRange<StringMapKeyIterator<ValueType>> getKeys() const
   {
      return make_range(StringMapKeyIterator<ValueType>(begin()),
                        StringMapKeyIterator<ValueType>(end()));
   }

   iterator find(StringRef key)
   {
      int bucket = findKey(key);
      if (bucket == -1) {
         return end();
      }
      return iterator(m_theTable + bucket, true);
   }

   const_iterator find(StringRef key) const
   {
      int bucket = findKey(key);
      if (bucket == -1) {
         return end();
      }
      return const_iterator(m_theTable + bucket, true);
   }

   /// lookup - Return the entry for the specified key, or a default
   /// constructed value if no such entry exists.
   ValueType lookup(StringRef key) const
   {
      const_iterator iter = find(key);
      if (iter != end()) {
         return iter->m_second;
      }
      return ValueType();
   }

   /// Lookup the ValueType for the \p Key, or create a default constructed value
   /// if the key is not in the map.
   ValueType &operator[](StringRef key)
   {
      return tryEmplace(key).first->m_second;
   }

   /// count - Return 1 if the element is in the map, 0 otherwise.
   size_type count(StringRef key) const
   {
      return find(key) == end() ? 0 : 1;
   }

   /// insert - Insert the specified key/value pair into the map.  If the key
   /// already exists in the map, return false and ignore the request, otherwise
   /// insert it and return true.
   bool insert(MapEntryType *keyValue)
   {
      unsigned bucketNo = lookupBucketFor(keyValue->getKey());
      StringMapEntryBase *&bucket = m_theTable[bucketNo];
      if (bucket && bucket != getTombstoneValue()) {
         return false;  // Already exists in map.
      }
      if (bucket == getTombstoneValue()) {
         --m_numTombstones;
      }
      bucket = keyValue;
      ++m_numItems;
      assert(m_numItems + m_numTombstones <= m_numBuckets);

      rehashTable();
      return true;
   }

   /// insert - Inserts the specified key/value pair into the map if the key
   /// isn't already in the map. The bool component of the returned pair is true
   /// if and only if the insertion takes place, and the iterator component of
   /// the pair points to the element with key equivalent to the key of the pair.
   std::pair<iterator, bool> insert(std::pair<StringRef, ValueType> item)
   {
      return tryEmplace(item.first, std::move(item.second));
   }

   /// Emplace a new element for the specified key into the map if the key isn't
   /// already in the map. The bool component of the returned pair is true
   /// if and only if the insertion takes place, and the iterator component of
   /// the pair points to the element with key equivalent to the key of the pair.
   template <typename... ArgsType>
   std::pair<iterator, bool> tryEmplace(StringRef key, ArgsType &&... args)
   {
      unsigned bucketNo = lookupBucketFor(key);
      StringMapEntryBase *&bucket = m_theTable[bucketNo];
      if (bucket && bucket != getTombstoneValue())
         return std::make_pair(iterator(m_theTable + bucketNo, false),
                               false); // Already exists in map.

      if (bucket == getTombstoneValue()) {
         --m_numTombstones;
      }
      bucket = MapEntryType::create(key, m_allocator, std::forward<ArgsType>(args)...);
      ++m_numItems;
      assert(m_numItems + m_numTombstones <= m_numBuckets);

      bucketNo = rehashTable(bucketNo);
      return std::make_pair(iterator(m_theTable + bucketNo, false), true);
   }

   // clear - Empties out the StringMap
   void clear()
   {
      if (empty()) {
         return;
      }
      // Zap all values, resetting the keys back to non-present (not tombstone),
      // which is safe because we're removing all elements.
      for (unsigned index = 0, end = m_numBuckets; index != end; ++index) {
         StringMapEntryBase *&bucket = m_theTable[index];
         if (bucket && bucket != getTombstoneValue()) {
            static_cast<MapEntryType*>(bucket)->destroy(m_allocator);
         }
         bucket = nullptr;
      }
      m_numItems = 0;
      m_numTombstones = 0;
   }

   /// remove - Remove the specified key/value pair from the map, but do not
   /// erase it.  This aborts if the key is not in the map.
   void remove(MapEntryType *keyValue)
   {
      removeKey(keyValue);
   }

   void erase(iterator iter)
   {
      MapEntryType &value = *iter;
      remove(&value);
      value.destroy(m_allocator);
   }

   bool erase(StringRef key)
   {
      iterator iter = find(key);
      if (iter == end()) {
         return false;
      }
      erase(iter);
      return true;
   }
};

template <typename DerivedType, typename ValueType>
class StringMapIterBase
      : public IteratorFacadeBase<DerivedType, std::forward_iterator_tag,
      ValueType>
{
protected:
   StringMapEntryBase **m_ptr = nullptr;

public:
   StringMapIterBase() = default;

   explicit StringMapIterBase(StringMapEntryBase **bucket,
                              bool noAdvance = false)
      : m_ptr(bucket)
   {
      if (!noAdvance) {
         AdvancePastEmptyBuckets();
      }
   }

   DerivedType &operator=(const DerivedType &other)
   {
      m_ptr = other.m_ptr;
      return static_cast<DerivedType &>(*this);
   }

   bool operator==(const DerivedType &rhs) const
   {
      return m_ptr == rhs.m_ptr;
   }

   DerivedType &operator++()
   { // Preincrement
      ++m_ptr;
      AdvancePastEmptyBuckets();
      return static_cast<DerivedType &>(*this);
   }

   DerivedType operator++(int)
   { // Post-increment
      DerivedType temp(m_ptr);
      ++*this;
      return temp;
   }

private:
   void AdvancePastEmptyBuckets()
   {
      while (*m_ptr == nullptr || *m_ptr == StringMapImpl::getTombstoneValue()) {
         ++m_ptr;
      }
   }
};

template <typename ValueType>
class StringMapConstIterator
      : public StringMapIterBase<StringMapConstIterator<ValueType>,
      const StringMapEntry<ValueType>>
{
   using base = StringMapIterBase<StringMapConstIterator<ValueType>,
   const StringMapEntry<ValueType>>;

public:
   StringMapConstIterator() = default;
   explicit StringMapConstIterator(StringMapEntryBase **bucket,
                                   bool noAdvance = false)
      : base(bucket, noAdvance)
   {}

   const StringMapEntry<ValueType> &operator*() const
   {
      return *static_cast<const StringMapEntry<ValueType> *>(*this->m_ptr);
   }
};

template <typename ValueType>
class StringMapIterator : public StringMapIterBase<StringMapIterator<ValueType>,
      StringMapEntry<ValueType>>
{
   using base =
   StringMapIterBase<StringMapIterator<ValueType>, StringMapEntry<ValueType>>;

public:
   StringMapIterator() = default;
   explicit StringMapIterator(StringMapEntryBase **bucket,
                              bool noAdvance = false)
      : base(bucket, noAdvance)
   {}

   StringMapEntry<ValueType> &operator*() const
   {
      return *static_cast<StringMapEntry<ValueType> *>(*this->m_ptr);
   }

   operator StringMapConstIterator<ValueType>() const
   {
      return StringMapConstIterator<ValueType>(this->m_ptr, true);
   }
};

template <typename ValueType>
class StringMapKeyIterator
      : public IteratorAdaptorBase<StringMapKeyIterator<ValueType>,
      StringMapConstIterator<ValueType>,
      std::forward_iterator_tag, StringRef>
{
   using base = IteratorAdaptorBase<StringMapKeyIterator<ValueType>,
   StringMapConstIterator<ValueType>,
   std::forward_iterator_tag, StringRef>;

public:
   StringMapKeyIterator() = default;
   explicit StringMapKeyIterator(StringMapConstIterator<ValueType> iter)
      : base(std::move(iter))
   {}

   StringRef &operator*()
   {
      m_key = this->getWrapped()->getKey();
      return m_key;
   }

private:
   StringRef m_key;
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_STRING_MAP_H
