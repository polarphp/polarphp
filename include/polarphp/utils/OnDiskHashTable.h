// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/01.

#ifndef POLARPHP_UTILS_DISK_HASH_TABLE_H
#define POLARPHP_UTILS_DISK_HASH_TABLE_H

#include "polarphp/utils/Allocator.h"
#include "polarphp/global/DataTypes.h"
#include "polarphp/utils/EndianStream.h"
#include "polarphp/utils/Host.h"
#include "polarphp/utils/MathExtras.h"
#include "polarphp/utils/RawOutStream.h"
#include <cassert>
#include <cstdlib>

namespace polar {
namespace utils {

using polar::basic::IteratorRange;
using polar::basic::make_range;

/// \brief Generates an on disk hash table.
///
/// This needs an \c Info that handles storing values into the hash table's
/// payload and computes the hash for a given key. This should provide the
/// following interface:
///
/// \code
/// class ExampleInfo {
/// public:
///   typedef ExampleKey key_type;   // Must be copy constructible
///   typedef ExampleKey &key_type_ref;
///   typedef ExampleData data_type; // Must be copy constructible
///   typedef ExampleData &data_type_ref;
///   typedef uint32_t hash_value_type; // The type the hash function returns.
///   typedef uint32_t OffsetType; // The type for offsets into the table.
///
///   /// Calculate the hash for Key
///   static hash_value_type computeHash(key_type_ref Key);
///   /// Return the lengths, in bytes, of the given Key/Data pair.
///   static std::pair<OffsetType, OffsetType>
///   emitKeyDataLength(RawOutStream &outstream, key_type_ref Key, data_type_ref Data);
///   /// Write Key to outstream.  KeyLen is the length from emitKeyDataLength.
///   static void emitKey(RawOutStream &outstream, key_type_ref Key,
///                       OffsetType KeyLen);
///   /// Write Data to outstream.  DataLen is the length from emitKeyDataLength.
///   static void emitData(RawOutStream &outstream, key_type_ref Key,
///                        data_type_ref Data, OffsetType DataLen);
///   /// Determine if two keys are equal. Optional, only needed by contains.
///   static bool EqualKey(key_type_ref Key1, key_type_ref Key2);
/// };
/// \endcode
template <typename Info>
class OnDiskChainedHashTableGenerator
{
   /// \brief A single item in the hash table.
   class Item
   {
   public:
      typename Info::key_type m_key;
      typename Info::data_type m_data;
      Item *m_next;
      const typename Info::hash_value_type m_hash;

      Item(typename Info::key_type_ref key, typename Info::data_type_ref data,
           Info &infoObj)
         : m_key(key), m_data(data), m_next(nullptr), m_hash(infoObj.computeHash(key))
      {}
   };

   typedef typename Info::OffsetType OffsetType;
   OffsetType m_numBuckets;
   OffsetType m_numEntries;
   polar::utils::SpecificBumpPtrAllocator<Item> m_bumpPtrAllocator;

   /// \brief A linked list of values in a particular hash bucket.
   struct Bucket
   {
      OffsetType m_offset;
      unsigned m_length;
      Item *m_head;
   };

   Bucket *m_buckets;

private:
   /// \brief Insert an item into the appropriate hash bucket.
   void insert(Bucket *buckets, size_t size, Item *item)
   {
      Bucket &bucket = buckets[item->m_hash & (size - 1)];
      item->m_next = bucket.m_head;
      ++bucket.m_length;
      bucket.m_head = item;
   }

   /// \brief Resize the hash table, moving the old entries into the new buckets.
   void resize(size_t newSize)
   {
      Bucket *newBuckets = (Bucket *)std::calloc(newSize, sizeof(Bucket));
      // Populate newBuckets with the old entries.
      for (size_t i = 0; i < m_numBuckets; ++i) {
         for (Item *item = m_buckets[i].m_head; item;) {
            Item *next = item->m_next;
            item->m_next = nullptr;
            insert(newBuckets, newSize, item);
            item = next;
         }
      }
      free(m_buckets);
      m_numBuckets = newSize;
      m_buckets = newBuckets;
   }

public:
   /// \brief Insert an entry into the table.
   void insert(typename Info::key_type_ref key,
               typename Info::data_type_ref data)
   {
      Info infoObj;
      insert(key, data, infoObj);
   }

   /// \brief Insert an entry into the table.
   ///
   /// Uses the provided Info instead of a stack allocated one.
   void insert(typename Info::key_type_ref key,
               typename Info::data_type_ref data, Info &infoObj)
   {
      ++m_numEntries;
      if (4 * m_numEntries >= 3 * m_numBuckets) {
         resize(m_numBuckets * 2);
      }
      insert(buckets, m_numBuckets, new (m_bumpPtrAllocator.allocate()) Item(key, data, infoObj));
   }

   /// \brief Determine whether an entry has been inserted.
   bool contains(typename Info::key_type_ref key, Info &infoObj)
   {
      unsigned hash = infoObj.computeHash(key);
      for (Item *item = buckets[hash & (m_numBuckets - 1)].m_head; item; item = item->m_next) {
         if (item->m_hash == hash && infoObj.equalKey(item->m_key, key)) {
            return true;
         }
      }
      return false;
   }

   /// \brief emit the table to outstream, which must not be at offset 0.
   OffsetType emit(RawOutStream &outstreamstream)
   {
      Info infoObj;
      return emit(outstream, infoObj);
   }

   /// \brief emit the table to outstream, which must not be at offset 0.
   ///
   /// Uses the provided Info instead of a stack allocated one.
   OffsetType emit(RawOutStream &outstream, Info &infoObj)
   {
      endian::Writer<Endianness::Little> le(outstream);

      // Now we're done adding entries, resize the bucket list if it's
      // significantly too large. (This only happens if the number of
      // entries is small and we're within our initial allocation of
      // 64 buckets.) We aim for an occupancy ratio in [3/8, 3/4).
      //
      // As a special case, if there are two or fewer entries, just
      // form a single bucket. A linear scan is fine in that case, and
      // this is very common in C++ class lookup tables. This also
      // guarantees we produce at least one bucket for an empty table.
      //
      // FIXME: Try computing a perfect hash function at this point.
      unsigned targetNumBuckets =
            m_numEntries <= 2 ? 1 : next_power_of_two(m_numEntries * 4 / 3);
      if (targetNumBuckets != m_numBuckets) {
         resize(targetNumBuckets);
      }

      // emit the payload of the table.
      for (OffsetType i = 0; i < m_numBuckets; ++i) {
         Bucket &bucket = m_buckets[i];
         if (!bucket.m_head) {
            continue;
         }

         // Store the offset for the data of this bucket.
         bucket.m_offset = outstream.tell();
         assert(bucket.m_offset && "Cannot write a bucket at offset 0. Please add padding.");

         // Write outstream the number of items in the bucket.
         le.write<uint16_t>(bucket.m_length);
         assert(bucket.m_length != 0 && "Bucket has a head but zero length?");

         // Write outstream the entries in the bucket.
         for (Item *item = bucket.m_head; item; item = item->m_next) {
            le.write<typename Info::hash_value_type>(item->m_hash);
            const std::pair<OffsetType, OffsetType> &length =
                  infoObj.emitKeyDataLength(outstream, item->m_key, item->m_data);
#ifdef NDEBUG
            infoObj.emitKey(outstream, item->m_key, length.first);
            infoObj.emitData(outstream, item->m_key, item->m_data, length.second);
#else
            // In asserts mode, check that the users length matches the data they
            // wrote.
            uint64_t keyStart = outstream.tell();
            infoObj.emitKey(outstream, item->m_key, Len.first);
            uint64_t dataStart = outstream.tell();
            infoObj.emitData(outstream, item->m_key, item->m_data, length.second);
            uint64_t end = outstream.tell();
            assert(OffsetType(dataStart - keyStart) == length.first &&
                   "key length does not match bytes written");
            assert(OffsetType(end - dataStart) == length.second &&
                   "data length does not match bytes written");
#endif
         }
      }

      // Pad with zeros so that we can start the hashtable at an aligned address.
      OffsetType tableOff = outstream.tell();
      uint64_t N = offset_to_alignment(tableOff, alignof(OffsetType));
      tableOff += N;
      while (N--) {
         le.write<uint8_t>(0);
      }
      // emit the hashtable itself.
      le.write<OffsetType>(m_numBuckets);
      le.write<OffsetType>(m_numEntries);
      for (OffsetType i = 0; i < m_numBuckets; ++i) {
         le.write<OffsetType>(m_buckets[i].m_offset);
      }
      return tableOff;
   }

   OnDiskChainedHashTableGenerator()
   {
      m_numEntries = 0;
      m_numBuckets = 64;
      // Note that we do not need to run the constructors of the individual
      // Bucket objects since 'calloc' returns bytes that are all 0.
      m_buckets = (Bucket *)std::calloc(m_numBuckets, sizeof(Bucket));
   }

   ~OnDiskChainedHashTableGenerator()
   {
      std::free(m_buckets);
   }
};

/// \brief Provides lookup on an on disk hash table.
///
/// This needs an \c Info that handles reading values from the hash table's
/// payload and computes the hash for a given key. This should provide the
/// following interface:
///
/// \code
/// class ExampleLookupInfo {
/// public:
///   typedef ExampleData data_type;
///   typedef ExampleInternalKey internal_key_type; // The stored key type.
///   typedef ExampleKey external_key_type; // The type to pass to find().
///   typedef uint32_t hash_value_type; // The type the hash function returns.
///   typedef uint32_t OffsetType; // The type for offsets into the table.
///
///   /// Compare two keys for equality.
///   static bool EqualKey(internal_key_type &Key1, internal_key_type &Key2);
///   /// Calculate the hash for the given key.
///   static hash_value_type computeHash(internal_key_type &ikey);
///   /// Translate from the semantic type of a key in the hash table to the
///   /// type that is actually stored and used for hashing and comparisons.
///   /// The internal and external types are often the same, in which case this
///   /// can simply return the passed in value.
///   static const internal_key_type &getInternalKey(external_key_type &ekey);
///   /// Read the key and data length from Buffer, leaving it pointing at the
///   /// following byte.
///   static std::pair<OffsetType, OffsetType>
///   ReadKeyDataLength(const unsigned char *&Buffer);
///   /// Read the key from Buffer, given the KeyLen as reported from
///   /// ReadKeyDataLength.
///   const internal_key_type &ReadKey(const unsigned char *Buffer,
///                                    OffsetType KeyLen);
///   /// Read the data for Key from Buffer, given the DataLen as reported from
///   /// ReadKeyDataLength.
///   data_type ReadData(StringRef Key, const unsigned char *Buffer,
///                      OffsetType DataLen);
/// };
/// \endcode
template <typename Info>
class OnDiskChainedHashTable
{
   const typename Info::OffsetType m_numBuckets;
   const typename Info::OffsetType m_numEntries;
   const unsigned char *const m_buckets;
   const unsigned char *const m_base;
   Info m_infoObj;

public:
   typedef Info InfoType;
   typedef typename Info::internal_key_type internal_key_type;
   typedef typename Info::external_key_type external_key_type;
   typedef typename Info::data_type data_type;
   typedef typename Info::hash_value_type hash_value_type;
   typedef typename Info::OffsetType OffsetType;

   OnDiskChainedHashTable(OffsetType numBuckets, OffsetType numEntries,
                        const unsigned char *buckets,
                        const unsigned char *base,
                        const Info &infoObj = Info())
      : m_numBuckets(numBuckets), m_numEntries(numEntries), m_buckets(buckets),
        m_base(base), m_infoObj(infoObj)
   {
      assert((reinterpret_cast<uintptr_t>(m_buckets) & 0x3) == 0 &&
             "'buckets' must have a 4-byte alignment");
   }

   /// Read the number of buckets and the number of entries from a hash table
   /// produced by OnDiskHashTableGenerator::emit, and advance the buckets
   /// pointer past them.
   static std::pair<OffsetType, OffsetType>
   readNumBucketsAndEntries(const unsigned char *&buckets)
   {
      assert((reinterpret_cast<uintptr_t>(buckets) & 0x3) == 0 &&
             "buckets should be 4-byte aligned.");
      OffsetType m_numBuckets =
            endian::read_next<OffsetType, Endianness::Little, ALIGNED>(buckets);
      OffsetType m_numEntries =
            endian::read_next<OffsetType, Endianness::Little, ALIGNED>(buckets);
      return std::make_pair(m_numBuckets, m_numEntries);
   }

   OffsetType getNumBuckets() const
   {
      return m_numBuckets;
   }

   OffsetType getNumEntries() const
   {
      return m_numEntries;
   }

   const unsigned char *getBase() const
   {
      return m_base;
   }

   const unsigned char *getBuckets() const
   {
      return m_buckets;
   }

   bool isEmpty() const
   {
      return m_numEntries == 0;
   }

   class iterator
   {
      internal_key_type m_key;
      const unsigned char *const m_data;
      const OffsetType m_length;
      Info *m_infoObj;

   public:
      iterator() : m_key(), m_data(nullptr), m_length(0), m_infoObj(nullptr)
      {}

      iterator(const internal_key_type key, const unsigned char *data, OffsetType length,
               Info *infoObj)
         : m_key(key), m_data(data), m_length(length), m_infoObj(infoObj)
      {}

      data_type operator*() const
      {
         return m_infoObj->readData(m_key, m_data, m_length);
      }

      const unsigned char *getDataPtr() const
      {
         return m_data;
      }

      OffsetType getDataLen() const
      {
         return m_length;
      }

      bool operator==(const iterator &other) const
      {
         return other.m_data == m_data;
      }

      bool operator!=(const iterator &other) const
      {
         return other.m_data != m_data;
      }
   };

   /// \brief Look up the stored data for a particular key.
   iterator find(const external_key_type &ekey, Info *infoPtr = nullptr)
   {
      const internal_key_type &ikey = m_infoObj.getInternalKey(ekey);
      hash_value_type keyHash = m_infoObj.computeHash(ikey);
      return findHashed(ikey, keyHash, infoPtr);
   }

   /// \brief Look up the stored data for a particular key with a known hash.
   iterator findHashed(const internal_key_type &ikey, hash_value_type keyHash,
                       Info *infoPtr = nullptr)
   {
      if (!infoPtr) {
         infoPtr = &m_infoObj;
      }
      // Each bucket is just an offset into the hash table file.
      OffsetType idx = keyHash & (m_numBuckets - 1);
      const unsigned char *bucket = m_buckets + sizeof(OffsetType) * idx;

      OffsetType offset = endian::read_next<OffsetType, Endianness::Little, ALIGNED>(bucket);
      if (offset == 0) {
         return iterator(); // Empty bucket.
      }
      const unsigned char *items = m_base + offset;

      // 'items' starts with a 16-bit unsigned integer representing the
      // number of items in this bucket.
      unsigned Len = endian::read_next<uint16_t, Endianness::Little, UNALIGNED>(items);

      for (unsigned i = 0; i < Len; ++i) {
         // Read the hash.
         hash_value_type itemHash =
               endian::read_next<hash_value_type, Endianness::Little, UNALIGNED>(items);

         // Determine the length of the key and the data.
         const std::pair<OffsetType, OffsetType> &length =
               Info::readKeyDataLength(items);
         OffsetType itemLen = length.first + length.second;

         // Compare the hashes.  If they are not the same, skip the entry entirely.
         if (itemHash != keyHash) {
            items += itemLen;
            continue;
         }

         // Read the key.
         const internal_key_type &item =
               infoPtr->readKey((const unsigned char *const)items, length.first);

         // If the key doesn't match just skip reading the value.
         if (!infoPtr->equalKey(item, ikey)) {
            items += itemLen;
            continue;
         }

         // The key matches!
         return iterator(item, items + length.first, length.second, infoPtr);
      }

      return iterator();
   }

   iterator end() const
   {
      return iterator();
   }

   Info &getInfoObj()
   {
      return m_infoObj;
   }

   /// \brief Create the hash table.
   ///
   /// \param buckets is the beginning of the hash table itself, which follows
   /// the payload of entire structure. This is the value returned by
   /// OnDiskHashTableGenerator::emit.
   ///
   /// \param Base is the point from which all offsets into the structure are
   /// based. This is offset 0 in the stream that was used when emitting the
   /// table.
   static OnDiskChainedHashTable *create(const unsigned char *buckets,
                                       const unsigned char *const base,
                                       const Info &infoObj = Info())
   {
      assert(buckets > base);
      auto numBucketsAndEntries = readNumBucketsAndEntries(buckets);
      return new OnDiskChainedHashTable<Info>(numBucketsAndEntries.first,
                                            numBucketsAndEntries.second,
                                            buckets, base, infoObj);
   }
};

/// \brief Provides lookup and iteration over an on disk hash table.
///
/// \copydetails llvm::OnDiskChainedHashTable
template <typename Info>
class OnDiskIterableChainedHashTable : public OnDiskChainedHashTable<Info>
{
   const unsigned char *m_payload;

public:
   typedef OnDiskChainedHashTable<Info>          base_type;
   typedef typename base_type::internal_key_type internal_key_type;
   typedef typename base_type::external_key_type external_key_type;
   typedef typename base_type::data_type         data_type;
   typedef typename base_type::hash_value_type   hash_value_type;
   typedef typename base_type::OffsetType       OffsetType;

private:
   /// \brief Iterates over all of the keys in the table.
   class IteratorBase {
      const unsigned char *m_ptr;
      OffsetType m_numItemsInBucketLeft;
      OffsetType m_numEntriesLeft;

   public:
      typedef external_key_type value_type;

      IteratorBase(const unsigned char *const ptr, OffsetType numEntries)
         : m_ptr(ptr), m_numItemsInBucketLeft(0), m_numEntriesLeft(numEntries)
      {}

      IteratorBase()
         : m_ptr(nullptr), m_numItemsInBucketLeft(0), m_numEntriesLeft(0)
      {}

      friend bool operator==(const IteratorBase &lhs, const IteratorBase &rhs)
      {
         return lhs.m_numEntriesLeft == rhs.m_numEntriesLeft;
      }

      friend bool operator!=(const IteratorBase &lhs, const IteratorBase &rhs)
      {
         return lhs.m_numEntriesLeft != rhs.m_numEntriesLeft;
      }

      /// Move to the next item.
      void advance()
      {
         if (!m_numItemsInBucketLeft) {
            // 'items' starts with a 16-bit unsigned integer representing the
            // number of items in this bucket.
            m_numItemsInBucketLeft =
                  endian::read_next<uint16_t, Endianness, UNALIGNED>(m_ptr);
         }
         m_ptr += sizeof(hash_value_type); // Skip the hash.
         // Determine the length of the key and the data.
         const std::pair<OffsetType, OffsetType> &length =
               Info::readKeyDataLength(m_ptr);
         m_ptr += length.first + length.second;
         assert(m_numItemsInBucketLeft);
         --m_numItemsInBucketLeft;
         assert(m_numEntriesLeft);
         --m_numEntriesLeft;
      }

      /// Get the start of the item as written by the trait (after the hash and
      /// immediately before the key and value length).
      const unsigned char *getItem() const
      {
         return m_ptr + (m_numItemsInBucketLeft ? 0 : 2) + sizeof(hash_value_type);
      }
   };

public:
   OnDiskIterableChainedHashTable(OffsetType numBuckets, OffsetType numEntries,
                                  const unsigned char *buckets,
                                  const unsigned char *payload,
                                  const unsigned char *base,
                                  const Info &infoObj = Info())
      : base_type(numBuckets, numEntries, buckets, base, infoObj),
        m_payload(payload)
   {}

   /// \brief Iterates over all of the keys in the table.
   class KeyIterator : public IteratorBase
   {
      Info *m_infoObj;

   public:
      typedef external_key_type value_type;

      KeyIterator(const unsigned char *const ptr, OffsetType numEntries,
                  Info *infoObj)
         : IteratorBase(m_ptr, m_numEntries), m_infoObj(infoObj)
      {}

      KeyIterator() : IteratorBase(), m_infoObj()
      {}

      KeyIterator &operator++()
      {
         this->advance();
         return *this;
      }

      KeyIterator operator++(int)
      { // Postincrement
         KeyIterator tmp = *this;
         ++*this;
         return tmp;
      }

      internal_key_type getInternalKey() const
      {
         auto *localPtr = this->getItem();

         // Determine the length of the key and the data.
         auto length = Info::readKeyDataLength(localPtr);

         // Read the key.
         return infoObj->readKey(localPtr, length.first);
      }

      value_type operator*() const
      {
         return infoObj->GetExternalKey(getInternalKey());
      }
   };

   KeyIterator keyBegin()
   {
      return KeyIterator(m_payload, this->getNumEntries(), &this->getInfoObj());
   }

   KeyIterator keyEnd()
   {
      return KeyIterator();
   }

   IteratorRange<KeyIterator> getKeys()
   {
      return make_range(keyBegin(), keyEnd());
   }

   /// \brief Iterates over all the entries in the table, returning the data.
   class DataIterator : public IteratorBase
   {
      Info *m_infoObj;

   public:
      typedef data_type value_type;

      DataIterator(const unsigned char *const ptr, OffsetType numEntries,
                   Info *infoObj)
         : IteratorBase(m_ptr, m_numEntries), m_infoObj(infoObj)
      {}

      DataIterator() : IteratorBase(), m_infoObj()
      {}

      DataIterator &operator++()
      { // Preincrement
         this->advance();
         return *this;
      }

      DataIterator operator++(int)
      { // Postincrement
         DataIterator tmp = *this;
         ++*this;
         return tmp;
      }

      value_type operator*() const
      {
         auto *localPtr = this->getItem();

         // Determine the length of the key and the data.
         auto length = Info::readKeyDataLength(localPtr);

         // Read the key.
         const internal_key_type &key = infoObj->readKey(localPtr, length.first);
         return infoObj->ReadData(key, localPtr + length.first, length.second);
      }
   };

   DataIterator dataBegin()
   {
      return DataIterator(m_payload, this->getNumEntries(), &this->getInfoObj());
   }
   DataIterator dataEnd()
   {
      return DataIterator();
   }

   IteratorRange<DataIterator> getData()
   {
      return make_range(dataBegin(), dataEnd());
   }

   /// \brief Create the hash table.
   ///
   /// \param buckets is the beginning of the hash table itself, which follows
   /// the payload of entire structure. This is the value returned by
   /// OnDiskHashTableGenerator::emit.
   ///
   /// \param m_payload is the beginning of the data contained in the table.  This
   /// is Base plus any padding or header data that was stored, ie, the offset
   /// that the stream was at when calling emit.
   ///
   /// \param Base is the point from which all offsets into the structure are
   /// based. This is offset 0 in the stream that was used when emitting the
   /// table.
   static OnDiskIterableChainedHashTable *
   create(const unsigned char *buckets, const unsigned char *const payload,
          const unsigned char *const base, const Info &infoObj = Info())
   {
      assert(buckets > base);
      auto numBucketsAndEntries =
            OnDiskIterableChainedHashTable<Info>::readNumBucketsAndEntries(buckets);
      return new OnDiskIterableChainedHashTable<Info>(
               numBucketsAndEntries.first, numBucketsAndEntries.second,
               buckets, payload, base, infoObj);
   }
};

} // utils
} // polar

#endif // POLARPHP_UTILS_DISK_HASH_TABLE_H
