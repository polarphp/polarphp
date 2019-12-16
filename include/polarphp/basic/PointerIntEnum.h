//===--- PointerIntEnum.h ---------------------------------------*- C++ -*-===//
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

#ifndef POLARPHP_BASIC_POINTERINTENUM_H
#define POLARPHP_BASIC_POINTERINTENUM_H

#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/Optional.h"
#include "llvm/Support/PointerLikeTypeTraits.h"
#include <cassert>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <utility>

namespace polar {

/// A tiny meta function to compute the log2 of a compile time constant.
///
/// *NOTE* This will be in an updated version of LLVM so this should be removed
/// at that point in time.
template <size_t N>
struct ConstantLog2
      : std::integral_constant<size_t, ConstantLog2<N / 2>::value + 1>
{};

template <>
struct ConstantLog2<1> : std::integral_constant<size_t, 0>
{};

/// A meta function for computing at compile time cleanly the value for an index
/// kind's value without using cpp macros.
template <unsigned Value, typename EnumTy>
struct PointerIntEnumIndexKindValue
      : std::integral_constant<unsigned,
      (Value << unsigned(ConstantLog2<
                         size_t(EnumTy::FirstIndexKind) + 1>::value)) |
      unsigned(EnumTy::FirstIndexKind)> {};

/// A pointer sized ADT that is able to compactly represent a Swift like enum
/// that can contain both Integer and Pointer payloads. It attempts to optimize
/// for the case of being able to represent as many pointer cases as possible
/// while allowing for indices to be stored as well. Without any loss of
/// generality assume that T* is our stored pointer. Then this is done as
/// follows:
///
/// 1. A PointerIntEnum for which bits [0, (num_tagged_bits(T*)-1)] are not all
/// set to 1 represent an enum with a pointer case. This means that one can have
/// at most ((1 << num_tagged_bits(T*)) - 2) enum cases associated with
/// pointers.
///
/// 2. A PointerIntEnum for which bits [0, (num_tagged_bits(T*)-1)] are all set
/// is either an invalid PointerIntEnum or an index.
///
/// 3. A PointerIntEnum with all bits set is an invalid PointerIntEnum.
///
/// 4. A PointerIntEnum for which bits [0, (num_tagged_bits(T*)-1)] are all set
/// but for which the upper bits are not all set is an index enum. The case bits
/// for the index PointerIntEnum are stored in bits [num_tagged_bits(T*),
/// num_tagged_bits(T*) + num_index_case_bits]. Then the actual index is stored
/// in the remaining top bits. For the case in which this is used in swift
/// currently, we use 3 index bits meaning that on a 32 bit system we have 26
/// bits for representing indices meaning we can represent indices up to
/// 67_108_862. Any index larger than that will result in an invalid
/// PointerIntEnum. On 64 bit we have many more bits than that.
///
/// By using this representation, we can make PointerIntEnum a true value type
/// that is trivially constructible and destructible without needing to malloc
/// memory.
///
/// In order for all of this to work, the user of this needs to construct an
/// enum with the appropriate case structure that allows the data structure to
/// determine what cases are pointer and which are indices. For instance the one
/// used by Projection in swift is:
///
///    enum class ProjectionKind : unsigned {
///      // PointerProjectionKinds
///      Upcast = 0,
///      RefCast = 1,
///      BitwiseCast = 2,
///      FirstPointerKind = Upcast,
///      LastPointerKind = BitwiseCast,
///
///
///      // This needs to be set to ((1 << num_tagged_bits(T*)) - 1). It
///      // represents the first NonPointerKind.
///      FirstIndexKind = 7,
///
///      // Index Projection Kinds
///      Struct = PointerIntEnumIndexKindValue<0, EnumTy>::value,
///      Tuple = PointerIntEnumIndexKindValue<1, EnumTy>::value,
///      Index = PointerIntEnumIndexKindValue<2, EnumTy>::value,
///      Class = PointerIntEnumIndexKindValue<3, EnumTy>::value,
///      Enum = PointerIntEnumIndexKindValue<4, EnumTy>::value,
///      LastIndexKind = Enum,
///    };
///
template <typename EnumTy, typename PointerTy, unsigned NumPointerKindBits,
          unsigned NumIndexKindBits,
          typename PtrTraits = llvm::PointerLikeTypeTraits<PointerTy>>
class PointerIntEnum
{

   // Make sure that the enum fits our requirements.
   static_assert(unsigned(EnumTy::FirstIndexKind) ==
                 ((1U << NumPointerKindBits) - 1U),
                 "Invalid Enum");
   static_assert(unsigned(EnumTy::FirstIndexKind) <=
                 unsigned(EnumTy::LastIndexKind),
                 "Invalid Enum");
   static_assert(unsigned(EnumTy::FirstPointerKind) <=
                 unsigned(EnumTy::LastPointerKind),
                 "Invalid Enum");
   static_assert(unsigned(EnumTy::LastPointerKind) <
                 unsigned(EnumTy::FirstIndexKind),
                 "Invalid Enum");

   /// The offset in bits where an index would be stored.
   static constexpr unsigned sm_indexShiftOffset =
         NumIndexKindBits + NumPointerKindBits;

   /// The number of bits in a PointerIntEnum that can be used to store indices.
   static constexpr unsigned sm_numIndexBits =
         sizeof(uintptr_t) * CHAR_BIT - sm_indexShiftOffset;

   /// The maximum index that can be stored for an index PointerIntEnum case.
   static constexpr uintptr_t sm_maxIndex = (uintptr_t(1) << sm_numIndexBits) - 2;

   /// The bit representation of an Invalid PointerIntEnum's storage.
   static constexpr uintptr_t sm_invalidStorage = uintptr_t(0) - 1;

   /// The pointer sized type used for the actual storage.
   uintptr_t m_storage;

public:
   PointerIntEnum()
      : m_storage(sm_invalidStorage)
   {}

   /// Initialize this PointerIntEnum with the kind \p kind and the Pointer \p
   /// ptr.
   PointerIntEnum(EnumTy kind, uintptr_t newIndex)
   {
      // If we can not represent this index, make the PointerIntEnum invalid.
      if (newIndex > sm_maxIndex) {
         m_storage = sm_invalidStorage;
         return;
      }

      m_storage = uintptr_t(kind) | (newIndex << sm_indexShiftOffset);
   }

   /// Initialize this PointerIntEnum with the kind \p kind and the Pointer \p
   /// ptr.
   PointerIntEnum(EnumTy kind, PointerTy ptr)
   {
      void *voidPtr = PtrTraits::getAsVoidPointer(ptr);

      // Make sure the pointer is at least aligned to NumPointerKindBits.
      assert((uintptr_t(voidPtr) & ((1 << NumPointerKindBits) - 1)) == 0);
      // Make sure that kind is a PointerKind.
      assert(unsigned(kind) >= unsigned(EnumTy::FirstPointerKind));
      assert(unsigned(kind) <= unsigned(EnumTy::LastPointerKind));

      m_storage = uintptr_t(voidPtr) | uintptr_t(kind);
   }

   PointerIntEnum(PointerIntEnum &&ptr) = default;
   PointerIntEnum(const PointerIntEnum &ptr) = default;
   ~PointerIntEnum() = default;
   PointerIntEnum &operator=(const PointerIntEnum &ptr) = default;
   PointerIntEnum &operator=(PointerIntEnum &&ptr) = default;

   bool isValid() const
   {
      return m_storage != sm_invalidStorage;
   }

   bool operator==(const PointerIntEnum &other) const
   {
      // Just compare the raw storage.
      return other.m_storage == m_storage;
   }

   bool operator!=(const PointerIntEnum &other) const
   {
      return !(*this == other);
   }

   /// \returns the kind of the enum if the enum is valid. Returns None if the
   /// enum is invalid.
   Optional<EnumTy> getKind() const
   {
      if (!isValid()) {
         return None;
      }
      // Check if the bottom pointer bits are all not set. If that is true then we
      // know that we have a pointer kind.
      unsigned pointerBits = m_storage & uintptr_t(EnumTy::FirstIndexKind);
      if (pointerBits != unsigned(EnumTy::FirstIndexKind)) {
         return EnumTy(pointerBits);
      }

      // Otherwise, we have an index kind. Just mask off the actual index bits and
      // return the kind.
      unsigned mask = (1 << sm_indexShiftOffset) - 1;
      unsigned maskedStorage = m_storage & mask;
      return EnumTy(maskedStorage);
   }

   /// \returns the index stored in the enum if the enum has an index
   /// payload. Asserts if the PointerIntEnum is invalid or has a pointer
   /// payload.
   uintptr_t getIndex() const
   {
      assert(isValid());
      assert(unsigned(*getKind()) >= unsigned(EnumTy::FirstIndexKind));
      return m_storage >> sm_indexShiftOffset;
   }

   /// \returns the pointer stored in the enum if the enum has a pointer
   /// payload. Asserts if the PointerIntEnum is invalid or has an index payload.
   PointerTy getPointer() const
   {
      assert(isValid());
      assert(unsigned(*getKind()) <= unsigned(EnumTy::LastPointerKind));
      uintptr_t Value = m_storage & ~(uintptr_t(EnumTy::FirstIndexKind));
      return PtrTraits::getFromVoidPointer((void *)(Value));
   }

   /// Return the raw storage of the type. Used for testing purposes.
   uintptr_t getStorage() const
   {
      return m_storage;
   }
};

} // polar

#endif // POLARPHP_BASIC_POINTERINTENUM_H
