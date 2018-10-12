// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/10.

#ifndef POLARPHP_UTILS_STLEXTRAS_H
#define POLARPHP_UTILS_STLEXTRAS_H

#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/global/AbiBreaking.h"

#include <optional>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#ifdef EXPENSIVE_CHECKS
#include <random> // for std::mt19937
#endif

namespace polar {
namespace utils {

template <typename T, T> struct SameType;

/// An efficient, type-erasing, non-owning reference to a callable. This is
/// intended for use as the type of a function parameter that is not used
/// after the function in question returns.
///
/// This class does not own the callable, so it is not in general safe to store
/// a FunctionRef.
template<typename Fn> class FunctionRef;

template<typename Ret, typename ...Params>
class FunctionRef<Ret(Params...)>
{
   Ret (*m_callback)(intptr_t callable, Params ...params) = nullptr;
   intptr_t m_callable;

   template<typename Callable>
   static Ret callbackWrapper(intptr_t callable, Params ...params)
   {
      return (*reinterpret_cast<Callable*>(callable))(
               std::forward<Params>(params)...);
   }

   public:
   FunctionRef() = default;
   FunctionRef(std::nullptr_t)
   {}

   template <typename Callable>
   FunctionRef(Callable &&callable,
               typename std::enable_if<
               !std::is_same<typename std::remove_reference<Callable>::type,
               FunctionRef>::value>::type * = nullptr)
      : m_callback(callbackWrapper<typename std::remove_reference<Callable>::type>),
        m_callable(reinterpret_cast<intptr_t>(&callable))
   {}

   Ret operator()(Params ...params) const
   {
      return m_callback(m_callable, std::forward<Params>(params)...);
   }

   operator bool() const
   {
      return m_callback;
   }
};

// deleter - Very very very simple method that is used to invoke operator
// delete on something.  It is used like this:
//
//   for_each(V.begin(), B.end(), deleter<Interval>);
template <class T>
inline void deleter(T *ptr)
{
   delete ptr;
}

/// traits class for checking whether type T is one of any of the given
/// types in the variadic list.
template <typename T, typename... Ts> struct is_one_of
{
   static const bool value = false;
};

template <typename T, typename U, typename... Ts>
struct is_one_of<T, U, Ts...>
{
   static const bool value =
         std::is_same<T, U>::value || is_one_of<T, Ts...>::value;
};

/// traits class for checking whether type T is a base class for all
///  the given types in the variadic list.
template <typename T, typename... Ts> struct are_base_of
{
   static const bool value = true;
};

template <typename T, typename U, typename... Ts>
struct are_base_of<T, U, Ts...>
{
   static const bool value =
         std::is_base_of<T, U>::value && are_base_of<T, Ts...>::value;
};

//===----------------------------------------------------------------------===//
//     Extra additions for arrays
//===----------------------------------------------------------------------===//

/// Find the length of an array.
template <class T, std::size_t N>
constexpr inline size_t array_lengthof(T (&)[N])
{
   return N;
}

} // utils
} // polar

#endif // POLARPHP_UTILS_STLEXTRAS_H
