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

#ifndef POLAR_BASIC_ADT_POINTER_SUM_TYPE_H
#define POLAR_BASIC_ADT_POINTER_SUM_TYPE_H

#include "polarphp/basic/adt/DenseMapInfo.h"
#include "polarphp/basic/adt/Bit.h"
#include "polarphp/utils/PointerLikeTypeTraits.h"

#include <cassert>
#include <cstdint>
#include <type_traits>

namespace polar {
namespace basic {

/// A compile time pair of an integer tag and the pointer-like type which it
/// indexes within a sum type. Also allows the user to specify a particular
/// traits class for pointer types with custom behavior such as over-aligned
/// allocation.
template <uintptr_t N, typename PointerArgT,
          typename TraitsArgT = PointerLikeTypeTraits<PointerArgT>>
struct PointerSumTypeMember
{
   enum { Tag = N };
   using PointerT = PointerArgT;
   using TraitsT = TraitsArgT;
};

namespace internal {

template <typename TagT, typename... MemberTs>
struct PointerSumTypeHelper;

} // end namespace internal

/// A sum type over pointer-like types.
///
/// This is a normal tagged union across pointer-like types that uses the low
/// bits of the pointers to store the tag.
///
/// Each member of the sum type is specified by passing a \c
/// PointerSumTypeMember specialization in the variadic member argument list.
/// This allows the user to control the particular tag value associated with
/// a particular type, use the same type for multiple different tags, and
/// customize the pointer-like traits used for a particular member. Note that
/// these *must* be specializations of \c PointerSumTypeMember, no other type
/// will suffice, even if it provides a compatible interface.
///
/// This type implements all of the comparison operators and even hash table
/// support by comparing the underlying storage of the pointer values. It
/// doesn't support delegating to particular members for comparisons.
///
/// It also default constructs to a zero tag with a null pointer, whatever that
/// would be. This means that the zero value for the tag type is significant
/// and may be desirable to set to a state that is particularly desirable to
/// default construct.
///
/// Having a supported zero-valued tag also enables getting the address of a
/// pointer stored with that tag provided it is stored in its natural bit
/// representation. This works because in the case of a zero-valued tag, the
/// pointer's value is directly stored into this object and we can expose the
/// address of that internal storage. This is especially useful when building an
/// `ArrayRef` of a single pointer stored in a sum type.
///
/// There is no support for constructing or accessing with a dynamic tag as
/// that would fundamentally violate the type safety provided by the sum type.
template <typename TagT, typename... MemberTs>
class PointerSumType
{
   using HelperT = internal::PointerSumTypeHelper<TagT, MemberTs...>;
   // We keep both the raw value and the min tag value's pointer in a union. When
   // the minimum tag value is zero, this allows code below to cleanly expose the
   // address of the zero-tag pointer instead of just the zero-tag pointer
   // itself. This is especially useful when building `ArrayRef`s out of a single
   // pointer. However, we have to carefully access the union due to the active
   // member potentially changing. When we *store* a new value, we directly
   // access the union to allow us to store using the obvious types. However,
   // when we *read* a value, we copy the underlying storage out to avoid relying
   // on one member or the other being active.
   union StorageT {
      // Ensure we get a null default constructed value. We don't use a member
      // initializer because some compilers seem to not implement those correctly
      // for a union.
      StorageT() : m_value(0) {}

      uintptr_t m_value;

      typename HelperT::template Lookup<HelperT::MinTag>::PointerT m_minTagPointer;
   };

   StorageT m_storage;
public:
   constexpr PointerSumType() = default;
   /// A typed setter to a given tagged member of the sum type.
   template <TagT N>
   void set(typename HelperT::template Lookup<N>::PointerT pointer)
   {
      void *value = HelperT::template Lookup<N>::TraitsT::getAsVoidPointer(pointer);
      assert((reinterpret_cast<uintptr_t>(value) & HelperT::TagMask) == 0 &&
             "pointer is insufficiently aligned to store the discriminant!");
      m_storage.m_value = reinterpret_cast<uintptr_t>(value) | N;
   }

   /// A typed constructor for a specific tagged member of the sum type.
   template <TagT N>
   static PointerSumType
   create(typename HelperT::template Lookup<N>::PointerT pointer)
   {
      PointerSumType result;
      result.set<N>(pointer);
      return result;
   }

   /// Clear the value to null with the min tag type.
   void clear()
   {
      set<HelperT::MinTag>(nullptr);
   }

   TagT getTag() const
   {
      return static_cast<TagT>(getOpaqueValue() & HelperT::TagMask);
   }

   template <TagT N> bool is() const
   {
      return N == getTag();
   }

   template <TagT N>
   typename HelperT::template Lookup<N>::PointerT get() const
   {
      void *ptr = is<N>() ? getVoidPtr() : nullptr;
      return HelperT::template Lookup<N>::TraitsT::getFromVoidPointer(ptr);
   }

   template <TagT N>
   typename HelperT::template Lookup<N>::PointerT cast() const
   {
      assert(is<N>() && "This instance has a different active member.");
      return HelperT::template Lookup<N>::TraitsT::getFromVoidPointer(getVoidPtr());
   }

   /// If the tag is zero and the pointer's value isn't changed when being
   /// stored, get the address of the stored value type-punned to the zero-tag's
   /// pointer type.
   typename HelperT::template Lookup<HelperT::MinTag>::PointerT const *
   getAddrOfZeroTagPointer() const
   {
      return const_cast<PointerSumType *>(this)->getAddrOfZeroTagPointer();
   }

   /// If the tag is zero and the pointer's value isn't changed when being
   /// stored, get the address of the stored value type-punned to the zero-tag's
   /// pointer type.
   typename HelperT::template Lookup<HelperT::MinTag>::PointerT *
   getAddrOfZeroTagPointer()
   {
      static_assert(HelperT::MinTag == 0, "Non-zero minimum tag value!");
      assert(is<HelperT::MinTag>() && "The active tag is not zero!");
      // Store the initial value of the pointer when read out of our storage.
      auto initialPtr = get<HelperT::MinTag>();
      // Now update the active member of the union to be the actual pointer-typed
      // member so that accessing it indirectly through the returned address is
      // valid.
      m_storage.m_minTagPointer = initialPtr;
      // Finally, validate that this was a no-op as expected by reading it back
      // out using the same underlying-storage read as above.
      assert(initialPtr == get<HelperT::MinTag>() &&
             "Switching to typed storage changed the pointer returned!");
      // Now we can correctly return an address to typed storage.
      return &m_storage.m_minTagPointer;
   }

   explicit operator bool() const
   {
      return getOpaqueValue() & HelperT::PointerMask;
   }

   bool operator==(const PointerSumType &other) const
   {
      return getOpaqueValue() == other.getOpaqueValue();
   }

   bool operator!=(const PointerSumType &other) const
   {
      return getOpaqueValue() != other.getOpaqueValue();
   }

   bool operator<(const PointerSumType &other) const
   {
      return getOpaqueValue() < other.getOpaqueValue();
   }

   bool operator>(const PointerSumType &other) const
   {
      return getOpaqueValue() > other.getOpaqueValue();
   }

   bool operator<=(const PointerSumType &other) const
   {
      return getOpaqueValue() <= other.getOpaqueValue();
   }

   bool operator>=(const PointerSumType &other) const
   {
      return getOpaqueValue() >= other.getOpaqueValue();
   }

   uintptr_t getOpaqueValue() const
   {
      return bit_cast<uintptr_t>(m_storage);
   }

protected:
   void *getVoidPtr() const
   {
      return reinterpret_cast<void *>(getOpaqueValue() & HelperT::PointerMask);
   }
};

namespace internal {

/// A helper template for implementing \c PointerSumType. It provides fast
/// compile-time lookup of the member from a particular tag value, along with
/// useful constants and compile time checking infrastructure..
template <typename TagT, typename... MemberTs>
struct PointerSumTypeHelper : MemberTs...
{
   // First we use a trick to allow quickly looking up information about
   // a particular member of the sum type. This works because we arranged to
   // have this type derive from all of the member type templates. We can select
   // the matching member for a tag using type deduction during overload
   // resolution.
   template <TagT N, typename PointerT, typename TraitsT>
   static PointerSumTypeMember<N, PointerT, TraitsT>
   lookupOverload(PointerSumTypeMember<N, PointerT, TraitsT> *);
   template <TagT N> static void lookupOverload(...);
   template <TagT N> struct Lookup
   {
      // Compute a particular member type by resolving the lookup helper ovorload.
      using MemberT = decltype(
      lookupOverload<N>(static_cast<PointerSumTypeHelper *>(nullptr)));

      /// The Nth member's pointer type.
      using PointerT = typename MemberT::PointerT;

      /// The Nth member's traits type.
      using TraitsT = typename MemberT::TraitsT;
   };

   // Next we need to compute the number of bits available for the discriminant
   // by taking the min of the bits available for each member. Much of this
   // would be amazingly easier with good constexpr support.
   template <uintptr_t V, uintptr_t... Vs>
   struct Min : std::integral_constant<
         uintptr_t, (V < Min<Vs...>::value ? V : Min<Vs...>::value)>
   {
   };
   template <uintptr_t V>
   struct Min<V> : std::integral_constant<uintptr_t, V> {};
   enum { NumTagBits = Min<MemberTs::TraitsT::NumLowBitsAvailable...>::value };

   // Also compute the smallest discriminant and various masks for convenience.
   constexpr static TagT MinTag =
         static_cast<TagT>(Min<MemberTs::Tag...>::value);
   enum : uint64_t {
      PointerMask = static_cast<uint64_t>(-1) << NumTagBits,
      TagMask = ~PointerMask
   };

   // Finally we need a recursive template to do static checks of each
   // member.
   template <typename MemberT, typename... InnerMemberTs>
   struct Checker : Checker<InnerMemberTs...> {
      static_assert(MemberT::Tag < (1 << NumTagBits),
                    "This discriminant value requires too many bits!");
   };
   template <typename MemberT> struct Checker<MemberT> : std::true_type {
      static_assert(MemberT::Tag < (1 << NumTagBits),
                    "This discriminant value requires too many bits!");
   };
   static_assert(Checker<MemberTs...>::value,
                 "Each member must pass the checker.");
};

} // end namespace internal

// Teach DenseMap how to use PointerSumTypes as keys.
template <typename TagT, typename... MemberTs>
struct DenseMapInfo<PointerSumType<TagT, MemberTs...>> {
   using SumType = PointerSumType<TagT, MemberTs...>;
   using HelperT = internal::PointerSumTypeHelper<TagT, MemberTs...>;
   enum { SomeTag = HelperT::MinTag };
   using SomePointerT =
   typename HelperT::template Lookup<HelperT::MinTag>::PointerT;
   using SomePointerInfo = DenseMapInfo<SomePointerT>;

   static inline SumType getEmptyKey()
   {
      return SumType::create<SomeTag>(SomePointerInfo::getEmptyKey());
   }

   static inline SumType getTombstoneKey()
   {
      return SumType::create<SomeTag>(SomePointerInfo::getTombstoneKey());
   }

   static unsigned getHashValue(const SumType &arg)
   {
      uintptr_t opaqueValue = arg.getOpaqueValue();
      return DenseMapInfo<uintptr_t>::getHashValue(opaqueValue);
   }

   static bool isEqual(const SumType &lhs, const SumType &rhs)
   {
      return lhs == rhs;
   }
};

} // basic
} // polar

#endif // POLAR_BASIC_ADT_POINTER_SUM_TYPE_H
