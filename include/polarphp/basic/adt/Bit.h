// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/04.

#ifndef POLARPHP_BASIC_ADT_BIT_H
#define POLARPHP_BASIC_ADT_BIT_H

#include "polarphp/global/Global.h"
#include <cstring>
#include <type_traits>

namespace polar {
namespace basic {

// This implementation of bit_cast is different from the C++17 one in two ways:
//  - It isn't constexpr because that requires compiler support.
//  - It requires trivially-constructible To, to avoid UB in the implementation.
template <typename To, typename From
          , typename = typename std::enable_if<sizeof(To) == sizeof(From)>::type
#if (__has_feature(is_trivially_constructible) && defined(_LIBCPP_VERSION)) || \
    (defined(__GNUC__) && __GNUC__ >= 5)
          , typename = typename std::is_trivially_constructible<To>::type
#elif __has_feature(is_trivially_constructible)
          , typename = typename std::enable_if<__is_trivially_constructible(To)>::type
#else
  // See comment below.
#endif
#if (__has_feature(is_trivially_copyable) && defined(_LIBCPP_VERSION)) || \
    (defined(__GNUC__) && __GNUC__ >= 5)
          , typename = typename std::enable_if<std::is_trivially_copyable<To>::value>::type
          , typename = typename std::enable_if<std::is_trivially_copyable<From>::value>::type
#elif __has_feature(is_trivially_copyable)
          , typename = typename std::enable_if<__is_trivially_copyable(To)>::type
          , typename = typename std::enable_if<__is_trivially_copyable(From)>::type
#else
  // This case is GCC 4.x. clang with libc++ or libstdc++ never get here. Unlike
  // polar/utils/TypeTraits.h's isPodLike we don't want to provide a
  // good-enough answer here: developers in that configuration will hit
  // compilation failures on the bots instead of locally. That's acceptable
  // because it's very few developers, and only until we move past C++11.
#endif
>
inline To bit_cast(const From &from) noexcept
{
  To to;
  std::memcpy(&to, &from, sizeof(To));
  return to;
}

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_BIT_H
