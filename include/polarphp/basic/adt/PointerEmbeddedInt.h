// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/25.

#ifndef POLARPHP_BASIC_ADT_POINTER_EMBEDDED_INT_H
#define POLARPHP_BASIC_ADT_POINTER_EMBEDDED_INT_H

#include "polarphp/basic/adt/DenseMapInfo.h"
#include "polarphp/utils/MathExtras.h"
#include "polarphp/utils/PointerLikeTypeTraits.h"
#include <cassert>
#include <climits>
#include <cstdint>
#include <type_traits>

namespace polar {
namespace basic {

/// Utility to embed an integer into a pointer-like type. This is specifically
/// intended to allow embedding integers where fewer bits are required than
/// exist in a pointer, and the integer can participate in abstractions along
/// side other pointer-like types. For example it can be placed into a \c
/// PointerSumType or \c PointerUnion.
///
/// Note that much like pointers, an integer value of zero has special utility
/// due to boolean conversions. For example, a non-null value can be tested for
/// in the above abstractions without testing the particular active member.
/// Also, the default constructed value zero initializes the integer.
template <typename IntType, int Bits = sizeof(IntType) * CHAR_BIT>
class PointerEmbeddedInt
{
   uintptr_t m_value = 0;

   // Note: This '<' is correct; using '<=' would result in some shifts
   // overflowing their storage types.
   static_assert(Bits < sizeof(uintptr_t) * CHAR_BIT,
                 "Cannot embed more bits than we have in a pointer!");

   enum : uintptr_t {
      // We shift as many zeros into the value as we can while preserving the
      // number of bits desired for the integer.
      Shift = sizeof(uintptr_t) * CHAR_BIT - Bits,

      // We also want to be able to mask out the preserved bits for asserts.
      Mask = static_cast<uintptr_t>(-1) << Bits
   };

   struct RawValueTag
   {
      explicit RawValueTag() = default;
   };

   friend struct PointerLikeTypeTraits<PointerEmbeddedInt>;
   explicit PointerEmbeddedInt(uintptr_t value, RawValueTag) : m_value(value)
   {}

public:
   PointerEmbeddedInt() = default;

   PointerEmbeddedInt(IntType value)
   {
      *this = value;
   }

   PointerEmbeddedInt &operator=(IntType value)
   {
      assert((std::is_signed<IntType>::value ? polar::utils::is_int<Bits>(value) : polar::utils::is_uint<Bits>(value)) &&
             "Integer has bits outside those preserved!");
      m_value = static_cast<uintptr_t>(value) << Shift;
      return *this;
   }

   // Note that this implicit conversion additionally allows all of the basic
   // comparison operators to work transparently, etc.
   operator IntType() const
   {
      if (std::is_signed<IntType>::value) {
         return static_cast<IntType>(static_cast<intptr_t>(m_value) >> Shift);
      }
      return static_cast<IntType>(m_value >> Shift);
   }
};

} // basic

namespace utils {

using polar::basic::PointerEmbeddedInt;

// Provide pointer like traits to support use with pointer unions and sum
// types.
template <typename IntType, int Bits>
struct PointerLikeTypeTraits<PointerEmbeddedInt<IntType, Bits>>
{
   using T = PointerEmbeddedInt<IntType, Bits>;

   static inline void *getAsVoidPointer(const T &ptr)
   {
      return reinterpret_cast<void *>(ptr.m_value);
   }

   static inline T getFromVoidPointer(void *ptr)
   {
      return T(reinterpret_cast<uintptr_t>(ptr), typename T::RawValueTag());
   }

   static inline T getFromVoidPointer(const void *ptr)
   {
      return T(reinterpret_cast<uintptr_t>(ptr), typename T::RawValueTag());
   }

   enum { NumLowBitsAvailable = T::Shift };
};

} // utils

namespace basic {

// Teach DenseMap how to use PointerEmbeddedInt objects as keys if the Int type
// itself can be a key.
template <typename IntType, int Bits>
struct DenseMapInfo<PointerEmbeddedInt<IntType, Bits>>
{
   using T = PointerEmbeddedInt<IntType, Bits>;
   using IntInfo = DenseMapInfo<IntType>;

   static inline T getEmptyKey()
   {
      return IntInfo::getEmptyKey();
   }

   static inline T getTombstoneKey()
   {
      return IntInfo::getTombstoneKey();
   }

   static unsigned getHashValue(const T &arg)
   {
      return IntInfo::getHashValue(arg);
   }

   static bool isEqual(const T &lhs, const T &rhs)
   {
      return lhs == rhs;
   }
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_POINTER_EMBEDDED_INT_H
