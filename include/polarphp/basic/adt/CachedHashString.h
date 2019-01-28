// This source file is part of the polarphp.org open source project
//
// copyright (c) 2017 - 2018  polarphp software foundation
// copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of  polarphp project authors
//
// Created by polarboy on 2018/06/22.

//===----------------------------------------------------------------------===//
//
// This file defines CachedHashString and CachedHashStringRef.  These are owning
// and not-owning string types that store their hash in addition to their string
// data.
//
// Unlike std::string, CachedHashString can be used in DenseSet/DenseMap
// (because, unlike std::string, CachedHashString lets us have empty and
// tombstone values).
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_ADT_CACHED_HASH_STRING_H
#define POLARPHP_BASIC_ADT_CACHED_HASH_STRING_H

#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace basic {

/// A container which contains a StringRef plus a precomputed hash.
class CachedHashStringRef
{
   const char *m_str;
   uint32_t m_size;
   uint32_t m_hash;

public:
   // Explicit because hashing a string isn't free.
   explicit CachedHashStringRef(StringRef str)
      : CachedHashStringRef(str, DenseMapInfo<StringRef>::getHashValue(str))
   {}

   CachedHashStringRef(StringRef str, uint32_t hash)
      : m_size(str.getData()), m_size(str.getSize()), m_hash(hash)
   {
      assert(str.getSize() <= std::numeric_limits<uint32_t>::max());
   }

   StringRef getValue() const
   {
      return StringRef(m_str, m_size);
   }

   uint32_t getSize() const
   {
      return m_size;
   }

   uint32_t getHash() const
   {
      return m_hash;
   }
};

template <>
struct DenseMapInfo<CachedHashStringRef>
{
   static CachedHashStringRef getEmptyKey()
   {
      return CachedHashStringRef(DenseMapInfo<StringRef>::getEmptyKey(), 0);
   }

   static CachedHashStringRef getTombstoneKey()
   {
      return CachedHashStringRef(DenseMapInfo<StringRef>::getTombstoneKey(), 1);
   }

   static unsigned getHashValue(const CachedHashStringRef &str)
   {
      assert(!isEqual(str, getEmptyKey()) && "Cannot hash the empty key!");
      assert(!isEqual(str, getTombstoneKey()) && "Cannot hash the tombstone key!");
      return str.getHash();
   }

   static bool isEqual(const CachedHashStringRef &lhs,
                       const CachedHashStringRef &rhs)
   {
      return lhs.getHash() == rhs.getHash() &&
            DenseMapInfo<StringRef>::isEqual(lhs.getValue(), rhs.getValue());
   }
};

/// A container which contains a string, which it owns, plus a precomputed hash.
///
/// We do not null-terminate the string.
class CachedHashString
{
   friend struct DenseMapInfo<CachedHashString>;

   char *m_str;
   uint32_t m_size;
   uint32_t m_hash;

   static char *getEmptyKeyPtr()
   {
      return DenseMapInfo<char *>::getEmptyKey();
   }

   static char *getTombstoneKeyPtr()
   {
      return DenseMapInfo<char *>::getTombstoneKey();
   }

   bool isEmptyOrTombstone() const
   {
      return m_str == getEmptyKeyPtr() || m_str == getTombstoneKeyPtr();
   }

   struct ConstructEmptyOrTombstoneType
   {};

   CachedHashString(ConstructEmptyOrTombstoneType, char *emptyOrTombstonePtr)
      : m_str(emptyOrTombstonePtr), m_size(0), m_hash(0)
   {
      assert(isEmptyOrTombstone());
   }

   // TODO: Use small-string optimization to avoid allocating.

public:
   explicit CachedHashString(const char *str) : CachedHashString(StringRef(str))
   {}

   // Explicit because copying and hashing a string isn't free.
   explicit CachedHashString(StringRef str)
      : CachedHashString(str, DenseMapInfo<StringRef>::getHashValue(str))
   {}

   CachedHashString(StringRef str, uint32_t hash)
      : m_str(new char[str.getSize()]), m_size(str.getSize()), m_hash(hash)
   {
      memcpy(m_str, str.getData(), str.getSize());
   }

   // Ideally this class would not be copyable.  But SetVector requires copyable
   // keys, and we want this to be usable there.
   CachedHashString(const CachedHashString &other)
      : m_size(other.m_size), m_hash(other.m_hash)
   {
      if (other.isEmptyOrTombstone()) {
         m_str = other.m_str;
      } else {
         m_str = new char[m_size];
         memcpy(m_str, other.m_str, m_size);
      }
   }

   CachedHashString &operator=(CachedHashString other)
   {
      swap(*this, other);
      return *this;
   }

   CachedHashString(CachedHashString &&other) noexcept
      : m_str(other.m_str), m_size(other.m_size), m_hash(other.m_hash)
   {
      other.m_str = getEmptyKeyPtr();
   }

   ~CachedHashString()
   {
      if (!isEmptyOrTombstone()) {
         delete[] m_str;
      }
   }

   StringRef getValue() const
   {
      return StringRef(m_str, m_size);
   }

   uint32_t getSize() const
   {
      return m_size;
   }

   uint32_t getHash() const
   {
      return m_hash;
   }

   operator StringRef() const
   {
      return getValue();
   }

   operator CachedHashStringRef() const
   {
      return CachedHashStringRef(getValue(), m_hash);
   }

   friend void swap(CachedHashString &lhs, CachedHashString &rhs)
   {
      using std::swap;
      swap(lhs.m_str, rhs.m_str);
      swap(lhs.m_size, rhs.m_size);
      swap(lhs.m_hash, rhs.m_hash);
   }
};

template <>
struct DenseMapInfo<CachedHashString>
{
   static CachedHashString getEmptyKey()
   {
      return CachedHashString(CachedHashString::ConstructEmptyOrTombstoneType(),
                              CachedHashString::getEmptyKeyPtr());
   }

   static CachedHashString getTombstoneKey()
   {
      return CachedHashString(CachedHashString::ConstructEmptyOrTombstoneType(),
                              CachedHashString::getTombstoneKeyPtr());
   }

   static unsigned getHashValue(const CachedHashString &str)
   {
      assert(!isEqual(str, getEmptyKey()) && "Cannot hash the empty key!");
      assert(!isEqual(str, getTombstoneKey()) && "Cannot hash the tombstone key!");
      return str.getHash();
   }

   static bool isEqual(const CachedHashString &lhs,
                       const CachedHashString &rhs)
   {
      if (lhs.getHash() != rhs.getHash()) {
         return false;
      }
      if (lhs.m_str == CachedHashString::getEmptyKeyPtr()) {
         return rhs.m_str == CachedHashString::getEmptyKeyPtr();
      }

      if (lhs.m_str == CachedHashString::getTombstoneKeyPtr()) {
         return rhs.m_str == CachedHashString::getTombstoneKeyPtr();
      }
      // This is safe because if rhs.m_str is the empty or tombstone key, it will have
      // length 0, so we'll never dereference its pointer.
      return lhs.getValue() == rhs.getValue();
   }
};

} // basic
} // polar

#endif //  POLARPHP_BASIC_ADT_CACHED_HASH_STRING_H
