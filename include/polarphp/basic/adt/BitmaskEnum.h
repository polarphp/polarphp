// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/22.

#ifndef POLARPHP_BASIC_ADT_BITMASK_ENUM_H
#define POLARPHP_BASIC_ADT_BITMASK_ENUM_H

#include <cassert>
#include <type_traits>
#include <utility>

#include "polarphp/utils/MathExtras.h"

/// POLAR_MARK_AS_BITMASK_ENUM lets you opt in an individual enum type so you can
/// perform bitwise operations on it without putting static_cast everywhere.
///
/// \code
///   enum MyEnum {
///     E1 = 1, E2 = 2, E3 = 4, E4 = 8,
///     POLAR_MARK_AS_BITMASK_ENUM(/* largestValue = */ E4)
///   };
///
///   void Foo() {
///     MyEnum A = (E1 | E2) & E3 ^ ~E4; // Look, ma: No static_cast!
///   }
/// \endcode
///
/// Normally when you do a bitwise operation on an enum value, you get back an
/// instance of the underlying type (e.g. int).  But using this macro, bitwise
/// ops on your enum will return you back instances of the enum.  This is
/// particularly useful for enums which represent a combination of flags.
///
/// The parameter to POLAR_MARK_AS_BITMASK_ENUM should be the largest individual
/// value in your enum.
///
/// All of the enum's values must be non-negative.
#define POLAR_MARK_AS_BITMASK_ENUM(largestValue)                                \
   POLAR_BITMASK_LARGEST_ENUMERATOR = largestValue

/// POLAR_ENABLE_BITMASK_ENUMS_IN_NAMESPACE() pulls the operator overloads used
/// by POLAR_MARK_AS_BITMASK_ENUM into the current namespace.
///
/// Suppose you have an enum foo::bar::MyEnum.  Before using
/// POLAR_MARK_AS_BITMASK_ENUM on MyEnum, you must put
/// POLAR_ENABLE_BITMASK_ENUMS_IN_NAMESPACE() somewhere inside namespace foo or
/// namespace foo::bar.  This allows the relevant operator overloads to be found
/// by ADL.
///
/// You don't need to use this macro in namespace llvm; it's done at the bottom
/// of this file.
#define POLAR_ENABLE_BITMASK_ENUMS_IN_NAMESPACE()                               \
   using ::polar::basic::bitmaskenumdetail::operator~;                                  \
   using ::polar::basic::bitmaskenumdetail::operator|;                                  \
   using ::polar::basic::bitmaskenumdetail::operator&;                                  \
   using ::polar::basic::bitmaskenumdetail::operator^;                                  \
   using ::polar::basic::bitmaskenumdetail::operator|=;                                 \
   using ::polar::basic::bitmaskenumdetail::operator&=;                                 \
   /* Force a semicolon at the end of this macro. */                            \
   using ::polar::basic::bitmaskenumdetail::operator^=

namespace polar {
namespace basic {

/// Traits class to determine whether an enum has a
/// POLAR_BITMASK_LARGEST_ENUMERATOR enumerator.
template <typename E, typename Enable = void>
struct is_bitmask_enum : std::false_type {};

template <typename E>
struct is_bitmask_enum<
      E, typename std::enable_if<sizeof(E::POLAR_BITMASK_LARGEST_ENUMERATOR) >=
0>::type> : std::true_type {};
namespace bitmaskenumdetail
{

/// Get a bitmask with 1s in all places up to the high-order bit of E's largest
/// value.
template <typename E>
typename std::underlying_type<E>::type mask()
{
   // On overflow, NextPowerOf2 returns zero with the type uint64_t, so
   // subtracting 1 gives us the mask with all bits set, like we want.
   return polar::utils::next_power_of_two(static_cast<typename std::underlying_type<E>::type>(
                                             E::POLAR_BITMASK_LARGEST_ENUMERATOR)) -
         1;
}

/// Check that value is in range for E, and return value cast to E's underlying
/// type.
template <typename E> typename std::underlying_type<E>::type underlying(E value)
{
   auto U = static_cast<typename std::underlying_type<E>::type>(value);
   assert(U >= 0 && "Negative enum values are not allowed.");
   assert(U <= mask<E>() && "Enum value too large (or largest val too small?)");
   return U;
}

template <typename E,
          typename = typename std::enable_if<is_bitmask_enum<E>::value>::type>
E operator~(E value)
{
   return static_cast<E>(~underlying(value) & mask<E>());
}

template <typename E,
          typename = typename std::enable_if<is_bitmask_enum<E>::value>::type>
E operator|(E lhs, E rhs)
{
   return static_cast<E>(underlying(lhs) | underlying(rhs));
}

template <typename E,
          typename = typename std::enable_if<is_bitmask_enum<E>::value>::type>
E operator&(E lhs, E rhs)
{
   return static_cast<E>(underlying(lhs) & underlying(rhs));
}

template <typename E,
          typename = typename std::enable_if<is_bitmask_enum<E>::value>::type>
E operator^(E lhs, E rhs)
{
   return static_cast<E>(underlying(lhs) ^ underlying(rhs));
}

// |=, &=, and ^= return a reference to lhs, to match the behavior of the
// operators on builtin types.

template <typename E,
          typename = typename std::enable_if<is_bitmask_enum<E>::value>::type>
E &operator|=(E &lhs, E rhs)
{
   lhs = lhs | rhs;
   return lhs;
}

template <typename E,
          typename = typename std::enable_if<is_bitmask_enum<E>::value>::type>
E &operator&=(E &lhs, E rhs)
{
   lhs = lhs & rhs;
   return lhs;
}

template <typename E,
          typename = typename std::enable_if<is_bitmask_enum<E>::value>::type>
E &operator^=(E &lhs, E rhs)
{
   lhs = lhs ^ rhs;
   return lhs;
}

} // namespace bitmaskenumdetail

// Enable bitmask enums in namespace ::polar and all nested namespaces.
POLAR_ENABLE_BITMASK_ENUMS_IN_NAMESPACE();

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_BITMASK_ENUM_H
