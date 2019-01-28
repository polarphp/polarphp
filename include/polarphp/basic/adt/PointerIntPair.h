// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/05.

#ifndef POLARPHP_BASIC_ADT_POINTER_INT_PAIR_H
#define POLARPHP_BASIC_ADT_POINTER_INT_PAIR_H

#include "polarphp/utils/PointerLikeTypeTraits.h"
#include <cassert>
#include <cstdint>
#include <limits>

namespace polar {
namespace basic {

template <typename T> struct DenseMapInfo;
template <typename PointerType, unsigned IntBits, typename PtrTraits>
struct PointerIntPairInfo;

/// PointerIntPair - This class implements a pair of a pointer and small
/// integer.  It is designed to represent this in the space required by one
/// pointer by bitmangling the integer into the low part of the pointer.  This
/// can only be done for small integers: typically up to 3 bits, but it depends
/// on the number of bits available according to PointerLikeTypeTraits for the
/// type.
///
/// Note that PointerIntPair always puts the intVal part in the highest bits
/// possible.  For example, PointerIntPair<void*, 1, bool> will put the bit for
/// the bool into bit #2, not bit #0, which allows the low two bits to be used
/// for something else.  For example, this allows:
///   PointerIntPair<PointerIntPair<void*, 1, bool>, 1, bool>
/// ... and the two bools will land in different bits.
template <typename PointerTypeype, unsigned IntBits, typename IntType = unsigned,
          typename PtrTraits = polar::utils::PointerLikeTypeTraits<PointerTypeype>,
          typename Info = PointerIntPairInfo<PointerTypeype, IntBits, PtrTraits>>
class PointerIntPair
{
   intptr_t m_value = 0;

public:
   constexpr PointerIntPair() = default;

   PointerIntPair(PointerTypeype ptrVal, IntType intVal)
   {
      setPointerAndInt(ptrVal, intVal);
   }

   explicit PointerIntPair(PointerTypeype ptrVal)
   {
      initWithPointer(ptrVal);
   }

   PointerTypeype getPointer() const
   {
      return Info::getPointer(m_value);
   }

   IntType getInt() const
   {
      return (IntType)Info::getInt(m_value);
   }

   void setPointer(PointerTypeype ptrVal)
   {
      m_value = Info::updatePointer(m_value, ptrVal);
   }

   void setInt(IntType intVal)
   {
      m_value = Info::updateInt(m_value, static_cast<intptr_t>(intVal));
   }

   void initWithPointer(PointerTypeype ptrVal)
   {
      m_value = Info::updatePointer(0, ptrVal);
   }

   void setPointerAndInt(PointerTypeype ptrVal, IntType intVal)
   {
      m_value = Info::updateInt(Info::updatePointer(0, ptrVal),
                                static_cast<intptr_t>(intVal));
   }

   PointerTypeype const *getAddrOfPointer() const
   {
      return const_cast<PointerIntPair *>(this)->getAddrOfPointer();
   }

   PointerTypeype *getAddrOfPointer()
   {
      assert(m_value == reinterpret_cast<intptr_t>(getPointer()) &&
             "Can only return the address if IntBits is cleared and "
             "PtrTraits doesn't change the pointer");
      return reinterpret_cast<PointerTypeype *>(&m_value);
   }

   void *getOpaqueValue() const
   {
      return reinterpret_cast<void *>(m_value);
   }

   void setFromOpaqueValue(void *value)
   {
      m_value = reinterpret_cast<intptr_t>(value);
   }

   static PointerIntPair getFromOpaqueValue(void *value)
   {
      PointerIntPair pair;
      pair.setFromOpaqueValue(value);
      return pair;
   }

   // Allow PointerIntPairs to be created from const void * if and only if the
   // pointer type could be created from a const void *.
   static PointerIntPair getFromOpaqueValue(const void *value)
   {
      (void)PtrTraits::getFromVoidPointer(value);
      return getFromOpaqueValue(const_cast<void *>(value));
   }

   bool operator==(const PointerIntPair &rhs) const
   {
      return m_value == rhs.m_value;
   }

   bool operator!=(const PointerIntPair &rhs) const
   {
      return m_value != rhs.m_value;
   }

   bool operator<(const PointerIntPair &rhs) const
   {
      return m_value < rhs.m_value;
   }

   bool operator>(const PointerIntPair &rhs) const
   {
      return m_value > rhs.m_value;
   }

   bool operator<=(const PointerIntPair &rhs) const
   {
      return m_value <= rhs.m_value;
   }

   bool operator>=(const PointerIntPair &rhs) const
   {
      return m_value >= rhs.m_value;
   }
};

template <typename PointerType, unsigned IntBits, typename PtrTraits>
struct PointerIntPairInfo
{
   static_assert(PtrTraits::NumLowBitsAvailable <
                 std::numeric_limits<uintptr_t>::digits,
                 "cannot use a pointer type that has all bits free");
   static_assert(IntBits <= PtrTraits::NumLowBitsAvailable,
                 "PointerIntPair with integer size too large for pointer");
   enum : uintptr_t {
      /// PointerBitMask - The bits that come from the pointer.
      PointerBitMask =
      ~(uintptr_t)(((intptr_t)1 << PtrTraits::NumLowBitsAvailable) - 1),

      /// IntShift - The number of low bits that we reserve for other uses, and
      /// keep zero.
      IntShift = (uintptr_t)PtrTraits::NumLowBitsAvailable - IntBits,

      /// IntMask - This is the unshifted mask for valid bits of the int type.
      IntMask = (uintptr_t)(((intptr_t)1 << IntBits) - 1),

      // ShiftedIntMask - This is the bits for the integer shifted in place.
      ShiftedIntMask = (uintptr_t)(IntMask << IntShift)
   };

   static PointerType getPointer(intptr_t value)
   {
      return PtrTraits::getFromVoidPointer(
               reinterpret_cast<void *>(value & PointerBitMask));
   }

   static intptr_t getInt(intptr_t value)
   {
      return (value >> IntShift) & IntMask;
   }

   static intptr_t updatePointer(intptr_t origValue, PointerType ptr)
   {
      intptr_t ptrWord =
            reinterpret_cast<intptr_t>(PtrTraits::getAsVoidPointer(ptr));
      assert((ptrWord & ~PointerBitMask) == 0 &&
             "Pointer is not sufficiently aligned");
      // Preserve all low bits, just update the pointer.
      return ptrWord | (origValue & ~PointerBitMask);
   }

   static intptr_t updateInt(intptr_t origValue, intptr_t intvalue)
   {
      intptr_t intWord = static_cast<intptr_t>(intvalue);
      assert((intWord & ~IntMask) == 0 && "Integer too large for field");

      // Preserve all bits other than the ones we are updating.
      return (origValue & ~ShiftedIntMask) | intWord << IntShift;
   }
};


// Provide specialization of DenseMapInfo for PointerIntPair.
template <typename PointerTypeype, unsigned IntBits, typename IntType>
struct DenseMapInfo<PointerIntPair<PointerTypeype, IntBits, IntType>>
{
   using Ty = PointerIntPair<PointerTypeype, IntBits, IntType>;

   static Ty getEmptyKey()
   {
      uintptr_t value = static_cast<uintptr_t>(-1);
      value <<= polar::utils::PointerLikeTypeTraits<Ty>::NumLowBitsAvailable;
      return Ty::getFromOpaqueValue(reinterpret_cast<void *>(value));
   }

   static Ty getTombstoneKey()
   {
      uintptr_t value = static_cast<uintptr_t>(-2);
      value <<= polar::utils::PointerLikeTypeTraits<PointerTypeype>::NumLowBitsAvailable;
      return Ty::getFromOpaqueValue(reinterpret_cast<void *>(value));
   }

   static unsigned getHashValue(Ty value)
   {
      uintptr_t intValue = reinterpret_cast<uintptr_t>(value.getOpaqueValue());
      return unsigned(intValue) ^ unsigned(intValue >> 9);
   }

   static bool isEqual(const Ty &lhs, const Ty &rhs)
   {
      return lhs == rhs;
   }
};

} // basic

namespace utils {

using polar::basic::PointerIntPair;

template <typename T> struct IsPodLike;
template <typename PointerTypeype, unsigned IntBits, typename IntType>
struct IsPodLike<PointerIntPair<PointerTypeype, IntBits, IntType>>
{
   static const bool value = true;
};


// Teach SmallPtrSet that PointerIntPair is "basically a pointer".
template <typename PointerTypeype, unsigned IntBits, typename IntType,
          typename PtrTraits>
struct PointerLikeTypeTraits<
      PointerIntPair<PointerTypeype, IntBits, IntType, PtrTraits>>
{
   static inline void *
   getAsVoidPointer(const PointerIntPair<PointerTypeype, IntBits, IntType> &pointer)
   {
      return pointer.getOpaqueValue();
   }

   static inline PointerIntPair<PointerTypeype, IntBits, IntType>
   getFromVoidPointer(void *pointer)
   {
      return PointerIntPair<PointerTypeype, IntBits, IntType>::getFromOpaqueValue(pointer);
   }

   static inline PointerIntPair<PointerTypeype, IntBits, IntType>
   getFromVoidPointer(const void *pointer)
   {
      return PointerIntPair<PointerTypeype, IntBits, IntType>::getFromOpaqueValue(pointer);
   }

   enum { NumLowBitsAvailable = PtrTraits::NumLowBitsAvailable - IntBits };
};

} // utils

} // polar

#endif // POLARPHP_BASIC_ADT_POINTER_INT_PAIR_H
