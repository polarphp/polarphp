//===--- FlaggedPointer.h - Explicit pointer tagging container --*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/02.
//===----------------------------------------------------------------------===//
// This file defines the FlaggedPointer class.
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_FLAGGEDPOINTER_H
#define POLARPHP_BASIC_FLAGGEDPOINTER_H

#include <cassert>
#include "llvm/Support/Compiler.h"
#include "llvm/Support/PointerLikeTypeTraits.h"

namespace polar {

/// This class implements a pair of a pointer and boolean flag.
/// Like PointerIntPair, it represents this by mangling a bit into the low part
/// of the pointer, taking advantage of pointer alignment. Unlike
/// PointerIntPair, you must specify the bit position explicitly, instead of
/// automatically placing an integer into the highest bits possible.
///
/// Composing this with `PointerIntPair` is not allowed.
template <typename PointerTy,
          unsigned BitPosition,
          typename PtrTraits = llvm::PointerLikeTypeTraits<PointerTy>>
class FlaggedPointer
{
   intptr_t m_value;
   static_assert(PtrTraits::NumLowBitsAvailable > 0,
                 "Not enough bits to store flag at this position");
   enum : uintptr_t
   {
      FlagMask = static_cast<uintptr_t>(1) << BitPosition,
      PointerBitMask = ~FlagMask
   };
public:
   FlaggedPointer()
      : m_value(0)
   {}

   FlaggedPointer(PointerTy ptrVal, bool flagVal)
   {
      setPointerAndFlag(ptrVal, flagVal);
   }
   explicit FlaggedPointer(PointerTy ptrVal)
   {
      initWithPointer(ptrVal);
   }

   /// Returns the underlying pointer with the flag bit masked out.
   PointerTy getPointer() const
   {
      return PtrTraits::getFromVoidPointer(
               reinterpret_cast<void*>(m_value & PointerBitMask));
   }

   void setPointer(PointerTy ptrVal)
   {
      intptr_t ptrWord = reinterpret_cast<intptr_t>(
               PtrTraits::getAsVoidPointer(ptrVal));
      assert((ptrWord & ~PointerBitMask) == 0 &&
             "Pointer is not sufficiently aligned");
      m_value = ptrWord | (m_value & ~PointerBitMask);
   }

   bool getFlag() const
   {
      return static_cast<bool>((m_value & FlagMask));
   }

   void setFlag(bool flagVal)
   {
      intptr_t flagWord = static_cast<intptr_t>(flagVal);
      m_value &= ~FlagMask;
      m_value |= flagWord << BitPosition;
   }

   /// Set the pointer value and assert if it overlaps with
   /// the flag's bit position.
   void initWithPointer(PointerTy ptrVal)
   {
      intptr_t ptrWord = reinterpret_cast<intptr_t>(
               PtrTraits::getAsVoidPointer(ptrVal));
      assert((ptrWord & ~PointerBitMask) == 0 &&
             "Pointer is not sufficiently aligned");
      m_value = ptrWord;
   }

   /// Set the pointer value, set the flag, and assert
   /// if the pointer's value would overlap with the flag's
   /// bit position.
   void setPointerAndFlag(PointerTy ptrVal, bool flagVal)
   {
      intptr_t ptrWord = reinterpret_cast<intptr_t>(
               PtrTraits::getAsVoidPointer(ptrVal));
      assert((ptrWord & ~PointerBitMask) == 0 &&
             "Pointer is not sufficiently aligned");
      intptr_t flagWord = static_cast<intptr_t>(flagVal);

      m_value = ptrWord | (flagWord << BitPosition);
   }

   PointerTy const *getAddrOfPointer() const
   {
      return const_cast<FlaggedPointer *>(this)->getAddrOfPointer();
   }

   PointerTy *getAddrOfPointer()
   {
      assert(m_value == reinterpret_cast<intptr_t>(getPointer()) &&
             "Can only return the address if IntBits is cleared and "
             "PtrTraits doesn't change the pointer");
      return reinterpret_cast<PointerTy *>(&m_value);
   }

   /// Get the raw pointer value for the underlying pointer
   /// including its flag value.
   void *getOpaqueValue() const
   {
      return reinterpret_cast<void*>(m_value);
   }

   void setFromOpaqueValue(void *value)
   {
      m_value = reinterpret_cast<intptr_t>(value);
   }

   static FlaggedPointer getFromOpaqueValue(const void *value)
   {
      FlaggedPointer pointer;
      pointer.setFromOpaqueValue(const_cast<void *>(value));
      return pointer;
   }

   bool operator==(const FlaggedPointer &other) const
   {
      return m_value == other.m_value;
   }

   bool operator!=(const FlaggedPointer &other) const
   {
      return m_value != other.m_value;
   }

   bool operator<(const FlaggedPointer &other) const
   {
      return m_value < other.m_value;
   }

   bool operator>(const FlaggedPointer &other) const
   {
      return m_value > other.m_value;
   }

   bool operator<=(const FlaggedPointer &other) const
   {
      return m_value <= other.m_value;
   }

   bool operator>=(const FlaggedPointer &other) const
   {
      return m_value >= other.m_value;
   }
};

} // polar

// Teach SmallPtrSet that FlaggedPointer is "basically a pointer".
template <typename PointerTy, unsigned BitPosition, typename PtrTraits>
struct llvm::PointerLikeTypeTraits<
      polar::FlaggedPointer<PointerTy, BitPosition, PtrTraits>>
{
public:
   static inline void *
   getAsVoidPointer(const polar::FlaggedPointer<PointerTy, BitPosition> &pointer)
   {
      return pointer.getOpaqueValue();
   }

   static inline polar::FlaggedPointer<PointerTy, BitPosition>
   getFromVoidPointer(void *pointer)
   {
      return polar::FlaggedPointer<PointerTy, BitPosition>::getFromOpaqueValue(pointer);
   }

   static inline polar::FlaggedPointer<PointerTy, BitPosition>
   getFromVoidPointer(const void *pointer)
   {
      return polar::FlaggedPointer<PointerTy, BitPosition>::getFromOpaqueValue(pointer);
   }

   enum {
      NumLowBitsAvailable = (BitPosition >= PtrTraits::NumLowBitsAvailable)
      ? PtrTraits::NumLowBitsAvailable
      : (std::min(int(BitPosition + 1),
      int(PtrTraits::NumLowBitsAvailable)) - 1)
   };
};

#endif // POLARPHP_BASIC_FLAGGEDPOINTER_H
