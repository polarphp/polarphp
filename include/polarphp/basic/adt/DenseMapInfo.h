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

#ifndef POLARPHP_BASIC_ADT_DENSE_MAP_INFO_H
#define POLARPHP_BASIC_ADT_DENSE_MAP_INFO_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/Hashing.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/PointerLikeTypeTraits.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace polar {
namespace basic {

using polar::utils::PointerLikeTypeTraits;
using polar::basic::StringRef;

template<typename T>
struct DenseMapInfo
{
   //static inline T getEmptyKey();
   //static inline T getTombstoneKey();
   //static unsigned getHashValue(const T &value);
   //static bool isEqual(const T &lhs, const T &rhs);
};

// Provide DenseMapInfo for all pointers.
template<typename T>
struct DenseMapInfo<T*>
{
   static inline T* getEmptyKey()
   {
      uintptr_t value = static_cast<uintptr_t>(-1);
      value <<= PointerLikeTypeTraits<T*>::NumLowBitsAvailable;
      return reinterpret_cast<T*>(value);
   }

   static inline T* getTombstoneKey()
   {
      uintptr_t value = static_cast<uintptr_t>(-2);
      value <<= PointerLikeTypeTraits<T*>::NumLowBitsAvailable;
      return reinterpret_cast<T*>(value);
   }

   static unsigned getHashValue(const T *ptrValue)
   {
      return (unsigned((uintptr_t)ptrValue) >> 4) ^
            (unsigned((uintptr_t)ptrValue) >> 9);
   }

   static bool isEqual(const T *lhs, const T *rhs)
   {
      return lhs == rhs;
   }
};

// Provide DenseMapInfo for chars.
template<>
struct DenseMapInfo<char>
{
   static inline char getEmptyKey()
   {
      return ~0;
   }

   static inline char getTombstoneKey()
   {
      return ~0 - 1;
   }

   static unsigned getHashValue(const char &value)
   {
      return value * 37U;
   }

   static bool isEqual(const char &lhs, const char &rhs)
   {
      return lhs == rhs;
   }
};

// Provide DenseMapInfo for unsigned shorts.
template <>
struct DenseMapInfo<unsigned short>
{
   static inline unsigned short getEmptyKey()
   {
      return 0xFFFF;
   }

   static inline unsigned short getTombstoneKey()
   {
      return 0xFFFF - 1;
   }

   static unsigned getHashValue(const unsigned short &value)
   {
      return value * 37U;
   }

   static bool isEqual(const unsigned short &lhs, const unsigned short &rhs)
   {
      return lhs == rhs;
   }
};

// Provide DenseMapInfo for unsigned ints.
template<> struct DenseMapInfo<unsigned>
{
   static inline unsigned getEmptyKey()
   {
      return ~0U;
   }
   static inline unsigned getTombstoneKey()
   {
      return ~0U - 1;
   }

   static unsigned getHashValue(const unsigned &value)
   {
      return value * 37U;
   }

   static bool isEqual(const unsigned& lhs, const unsigned& rhs)
   {
      return lhs == rhs;
   }
};

// Provide DenseMapInfo for unsigned longs.
template<> struct DenseMapInfo<unsigned long>
{
   static inline unsigned long getEmptyKey()
   {
      return ~0UL;
   }

   static inline unsigned long getTombstoneKey()
   {
      return ~0UL - 1L;
   }

   static unsigned getHashValue(const unsigned long &value)
   {
      return (unsigned)(value * 37UL);
   }

   static bool isEqual(const unsigned long &lhs, const unsigned long &rhs)
   {
      return lhs == rhs;
   }
};

// Provide DenseMapInfo for unsigned long longs.
template<> struct DenseMapInfo<unsigned long long>
{
   static inline unsigned long long getEmptyKey()
   {
      return ~0ULL;
   }

   static inline unsigned long long getTombstoneKey()
   {
      return ~0ULL - 1ULL;
   }

   static unsigned getHashValue(const unsigned long long &value)
   {
      return (unsigned)(value * 37ULL);
   }

   static bool isEqual(const unsigned long long& lhs,
                       const unsigned long long& rhs)
   {
      return lhs == rhs;
   }
};

// Provide DenseMapInfo for shorts.
template <> struct DenseMapInfo<short>
{
   static inline short getEmptyKey()
   {
      return 0x7FFF;
   }

   static inline short getTombstoneKey()
   {
      return -0x7FFF - 1;
   }

   static unsigned getHashValue(const short &value)
   {
      return value * 37U;
   }

   static bool isEqual(const short &lhs, const short &rhs)
   {
      return lhs == rhs;
   }
};

// Provide DenseMapInfo for ints.
template<>
struct DenseMapInfo<int>
{
   static inline int getEmptyKey()
   {
      return 0x7fffffff;
   }

   static inline int getTombstoneKey()
   {
      return -0x7fffffff - 1;
   }

   static unsigned getHashValue(const int& value)
   {
      return (unsigned)(value * 37U);
   }

   static bool isEqual(const int& lhs, const int& rhs)
   {
      return lhs == rhs;
   }
};

// Provide DenseMapInfo for longs.
template<>
struct DenseMapInfo<long>
{
   static inline long getEmptyKey()
   {
      return (1UL << (sizeof(long) * 8 - 1)) - 1UL;
   }

   static inline long getTombstoneKey()
   {
      return getEmptyKey() - 1L;
   }

   static unsigned getHashValue(const long& value)
   {
      return (unsigned)(value * 37UL);
   }

   static bool isEqual(const long& lhs, const long& rhs)
   {
      return lhs == rhs;
   }
};

// Provide DenseMapInfo for long longs.
template<>
struct DenseMapInfo<long long>
{
   static inline long long getEmptyKey()
   {
      return 0x7fffffffffffffffLL;
   }

   static inline long long getTombstoneKey()
   {
      return -0x7fffffffffffffffLL-1;
   }

   static unsigned getHashValue(const long long& value)
   {
      return (unsigned)(value * 37ULL);
   }

   static bool isEqual(const long long& lhs, const long long& rhs)
   {
      return lhs == rhs;
   }
};

// Provide DenseMapInfo for all pairs whose members have info.
template<typename T, typename U>
struct DenseMapInfo<std::pair<T, U>>
{
   using Pair = std::pair<T, U>;
   using FirstInfo = DenseMapInfo<T>;
   using SecondInfo = DenseMapInfo<U>;

   static inline Pair getEmptyKey()
   {
      return std::make_pair(FirstInfo::getEmptyKey(),
                            SecondInfo::getEmptyKey());
   }

   static inline Pair getTombstoneKey()
   {
      return std::make_pair(FirstInfo::getTombstoneKey(),
                            SecondInfo::getTombstoneKey());
   }

   static unsigned getHashValue(const Pair &pairVal)
   {
      uint64_t key = (uint64_t)FirstInfo::getHashValue(pairVal.first) << 32
                                                                         | (uint64_t)SecondInfo::getHashValue(pairVal.second);
      key += ~(key << 32);
      key ^= (key >> 22);
      key += ~(key << 13);
      key ^= (key >> 8);
      key += (key << 3);
      key ^= (key >> 15);
      key += ~(key << 27);
      key ^= (key >> 31);
      return (unsigned)key;
   }

   static bool isEqual(const Pair &lhs, const Pair &rhs)
   {
      return FirstInfo::isEqual(lhs.first, rhs.first) &&
            SecondInfo::isEqual(lhs.second, rhs.second);
   }
};

// Provide DenseMapInfo for StringRefs.
template <>
struct DenseMapInfo<StringRef>
{
   static inline StringRef getEmptyKey()
   {
      return StringRef(reinterpret_cast<const char *>(~static_cast<uintptr_t>(0)),
                       0);
   }

   static inline StringRef getTombstoneKey()
   {
      return StringRef(reinterpret_cast<const char *>(~static_cast<uintptr_t>(1)),
                       0);
   }

   static unsigned getHashValue(StringRef value)
   {
      assert(value.getData() != getEmptyKey().getData() && "Cannot hash the empty key!");
      assert(value.getData() != getTombstoneKey().getData() &&
            "Cannot hash the tombstone key!");
      return (unsigned)(hash_value(value));
   }

   static bool isEqual(StringRef lhs, StringRef rhs)
   {
      if (rhs.getData() == getEmptyKey().getData()) {
         return lhs.getData() == getEmptyKey().getData();
      }
      if (rhs.getData() == getTombstoneKey().getData()) {
         return lhs.getData() == getTombstoneKey().getData();
      }
      return lhs == rhs;
   }
};

// Provide DenseMapInfo for ArrayRefs.
template <typename T>
struct DenseMapInfo<ArrayRef<T>>
{
   static inline ArrayRef<T> getEmptyKey()
   {
      return ArrayRef<T>(reinterpret_cast<const T *>(~static_cast<uintptr_t>(0)),
                         size_t(0));
   }

   static inline ArrayRef<T> getTombstoneKey()
   {
      return ArrayRef<T>(reinterpret_cast<const T *>(~static_cast<uintptr_t>(1)),
                         size_t(0));
   }

   static unsigned getHashValue(ArrayRef<T> value)
   {
      assert(value.getData() != getEmptyKey().getData() && "Cannot hash the empty key!");
      assert(value.getData() != getTombstoneKey().getData() &&
            "Cannot hash the tombstone key!");
      return (unsigned)(hash_value(value));
   }

   static bool isEqual(ArrayRef<T> lhs, ArrayRef<T> rhs)
   {
      if (rhs.getData() == getEmptyKey().getData()) {
         return lhs.getData() == getEmptyKey().getData();
      }
      if (rhs.getData() == getTombstoneKey().getData()) {
         return lhs.getData() == getTombstoneKey().getData();
      }
      return lhs == rhs;
   }
};

template <>
struct DenseMapInfo<HashCode>
{
   static inline HashCode getEmptyKey()
   {
      return HashCode(-1);
   }

   static inline HashCode getTombstoneKey()
   {
      return HashCode(-2);
   }

   static unsigned getHashValue(HashCode val)
   {
      return val;
   }

   static bool isEqual(HashCode lhs, HashCode rhs)
   {
      return lhs == rhs;
   }
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_DENSE_MAP_INFO_H
