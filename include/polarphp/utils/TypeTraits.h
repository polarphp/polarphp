// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/12.

#ifndef POLARPHP_UTILS_TYPE_TRAITS_H
#define POLARPHP_UTILS_TYPE_TRAITS_H

#include "polarphp/global/CompilerFeature.h"
#include <type_traits>
#include <utility>

#ifndef __has_feature
#define POLAR_DEFINED_HAS_FEATURE
#define __has_feature(x) 0
#endif

namespace polar::utils {


/// Metafunction that determines whether the given type is either an
/// integral type or an enumeration type, including enum classes.
///
/// Note that this accepts potentially more integral types than is_integral
/// because it is based on being implicitly convertible to an integral type.
/// Also note that enum classes aren't implicitly convertible to integral types,
/// the value may therefore need to be explicitly converted before being used.
template <typename T>
class is_integral_or_enum
{
   using UnderlyingT = typename std::remove_reference<T>::type;

public:
   static const bool value =
         !std::is_class<UnderlyingT>::value && // Filter conversion operators.
         !std::is_pointer<UnderlyingT>::value &&
         !std::is_floating_point<UnderlyingT>::value &&
         (std::is_enum<UnderlyingT>::value ||
          std::is_convertible<UnderlyingT, unsigned long long>::value);
};

/// If T is a pointer, just return it. If it is not, return T&.
template<typename T, typename Enable = void>
struct add_lvalue_reference_if_not_pointer
{ using type = T &; };

template <typename T>
struct add_lvalue_reference_if_not_pointer<
      T, typename std::enable_if<std::is_pointer<T>::value>::type>
{
   using type = T;
};

/// If T is a pointer to X, return a pointer to const X. If it is not,
/// return const T.
template<typename T, typename Enable = void>
struct add_const_past_pointer
{
   using type = const T;
};

template <typename T>
struct add_const_past_pointer<
      T, typename std::enable_if<std::is_pointer<T>::value>::type>
{
   using type = const typename std::remove_pointer<T>::type *;
};

template <typename T, typename Enable = void>
struct const_pointer_or_const_ref
{
   using type = const T &;
};
template <typename T>
struct const_pointer_or_const_ref<
      T, typename std::enable_if<std::is_pointer<T>::value>::type>
{
   using type = typename add_const_past_pointer<T>::type;
};

namespace internal {
/// Internal utility to detect trivial copy construction.
template<typename T>
union copy_construction_triviality_helper
{
   T t;
   copy_construction_triviality_helper() = default;
   copy_construction_triviality_helper(const copy_construction_triviality_helper&) = default;
   ~copy_construction_triviality_helper() = default;
};
/// Internal utility to detect trivial move construction.
template<typename T>
union move_construction_triviality_helper
{
   T t;
   move_construction_triviality_helper() = default;
   move_construction_triviality_helper(move_construction_triviality_helper&&) = default;
   ~move_construction_triviality_helper() = default;
};

template<class T>
union trivial_helper
{
   T t;
};

} // end namespace detail

/// An implementation of `std::is_trivially_copy_constructible` since we have
/// users with STLs that don't yet include it.
template <typename T>
struct is_trivially_copy_constructible
      : std::is_copy_constructible<
      polar::utils::internal::copy_construction_triviality_helper<T>> {};
template <typename T>
struct is_trivially_copy_constructible<T &> : std::true_type {};
template <typename T>
struct is_trivially_copy_constructible<T &&> : std::false_type {};

/// An implementation of `std::is_trivially_move_constructible` since we have
/// users with STLs that don't yet include it.
template <typename T>
struct is_trivially_move_constructible
      : std::is_move_constructible<
      polar::utils::internal::move_construction_triviality_helper<T>> {};
template <typename T>
struct is_trivially_move_constructible<T &> : std::true_type {};
template <typename T>
struct is_trivially_move_constructible<T &&> : std::true_type {};


template <typename T>
struct is_copy_assignable
{
   template<class F>
   static auto get(F*) -> decltype(std::declval<F &>() = std::declval<const F &>(), std::true_type{});
   static std::false_type get(...);
   static constexpr bool value = decltype(get((T*)nullptr))::value;
};

template <typename T>
struct is_move_assignable {
   template<class F>
   static auto get(F*) -> decltype(std::declval<F &>() = std::declval<F &&>(), std::true_type{});
   static std::false_type get(...);
   static constexpr bool value = decltype(get((T*)nullptr))::value;
};


// An implementation of `std::is_trivially_copyable` since STL version
// is not equally supported by all compilers, especially GCC 4.9.
// Uniform implementation of this trait is important for ABI compatibility
// as it has an impact on SmallVector's ABI (among others).
template <typename T>
class is_trivially_copyable
{

   // copy constructors
   static constexpr bool has_trivial_copy_constructor =
         std::is_copy_constructible<internal::trivial_helper<T>>::value;
   static constexpr bool has_deleted_copy_constructor =
         !std::is_copy_constructible<T>::value;

   // move constructors
   static constexpr bool has_trivial_move_constructor =
         std::is_move_constructible<internal::trivial_helper<T>>::value;
   static constexpr bool has_deleted_move_constructor =
         !std::is_move_constructible<T>::value;

   // copy assign
   static constexpr bool has_trivial_copy_assign =
         is_copy_assignable<internal::trivial_helper<T>>::value;
   static constexpr bool has_deleted_copy_assign =
         !is_copy_assignable<T>::value;

   // move assign
   static constexpr bool has_trivial_move_assign =
         is_move_assignable<internal::trivial_helper<T>>::value;
   static constexpr bool has_deleted_move_assign =
         !is_move_assignable<T>::value;

   // destructor
   static constexpr bool has_trivial_destructor =
         std::is_destructible<internal::trivial_helper<T>>::value;

public:

   static constexpr bool value =
         has_trivial_destructor &&
         (has_deleted_move_assign || has_trivial_move_assign) &&
         (has_deleted_move_constructor || has_trivial_move_constructor) &&
         (has_deleted_copy_assign || has_trivial_copy_assign) &&
         (has_deleted_copy_constructor || has_trivial_copy_constructor);

#ifdef HAVE_STD_IS_TRIVIALLY_COPYABLE
   static_assert(value == std::is_trivially_copyable<T>::value,
                 "inconsistent behavior between llvm:: and std:: implementation of is_trivially_copyable");
#endif
};
template <typename T>
class is_trivially_copyable<T*> : public std::true_type {
};

} // polar::utils

// If the compiler supports detecting whether a class is final, define
// an POLAR_IS_FINAL macro. If it cannot be defined properly, this
// macro will be left undefined.
#if __cplusplus >= 201402L || defined(_MSC_VER)
#define POLAR_IS_FINAL(Ty) std::is_final<Ty>()
#elif __has_feature(is_final) || POLAR_GNUC_PREREQ(4, 7, 0)
#define POLAR_IS_FINAL(Ty) __is_final(Ty)
#endif

#ifdef POLAR_DEFINED_HAS_FEATURE
#undef __has_feature
#endif

#endif // POLARPHP_UTILS_TYPE_TRAITS_H
