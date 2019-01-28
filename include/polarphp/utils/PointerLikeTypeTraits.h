// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/05/31.

#ifndef POLARPHP_UTILS_POINTER_LIKE_TYPE_TRAITS_H
#define POLARPHP_UTILS_POINTER_LIKE_TYPE_TRAITS_H

#include "polarphp/global/DataTypes.h"
#include <type_traits>

namespace polar {
namespace utils {

/// A traits type that is used to handle pointer types and things that are just
/// wrappers for pointers as a uniform entity.
template <typename T>
struct PointerLikeTypeTraits;

namespace internal
{
/// A tiny meta function to compute the log2 of a compile time constant.
template <size_t N>
struct ConstantLog2
      : std::integral_constant<size_t, ConstantLog2<N / 2>::value + 1> {};
template <> struct ConstantLog2<1> : std::integral_constant<size_t, 0> {};

// Provide a trait to check if T is pointer-like.
template <typename T, typename U = void>
struct HasPointerLikeTypeTraits
{
   static const bool value = false;
};

// sizeof(T) is valid only for a complete T.
template <typename T>
struct HasPointerLikeTypeTraits<T, decltype((sizeof(PointerLikeTypeTraits<T>) + sizeof(T)), void())>
{
   static const bool value = true;
};

template <typename T>
struct IsPointerLike
{
   static const bool value = HasPointerLikeTypeTraits<T>::value;
};

template <typename T> struct IsPointerLike<T *>
{
   static const bool value = true;
};
} // namespace internal

// Provide PointerLikeTypeTraits for non-cvr pointers.
template <typename T>
struct PointerLikeTypeTraits<T *>
{
   static inline void *getAsVoidPointer(T *pointer)
   {
      return pointer;
   }

   static inline T *getFromVoidPointer(void *pointer)
   {
      return static_cast<T *>(pointer);
   }

   enum
   {
      NumLowBitsAvailable = internal::ConstantLog2<alignof(T)>::value
   };
};

template <>
struct PointerLikeTypeTraits<void *>
{
   static inline void *getAsVoidPointer(void *pointer)
   {
      return pointer;
   }

   static inline void *getFromVoidPointer(void *pointer)
   {
      return pointer;
   }

   /// Note, we assume here that void* is related to raw malloc'ed memory and
   /// that malloc returns objects at least 4-byte aligned. However, this may be
   /// wrong, or pointers may be from something other than malloc. In this case,
   /// you should specify a real typed pointer or avoid this template.
   ///
   /// All clients should use assertions to do a run-time check to ensure that
   /// this is actually true.
   enum {
      NumLowBitsAvailable = 2
   };
};

// Provide PointerLikeTypeTraits for const things.
template <typename T> struct PointerLikeTypeTraits<const T>
{
   using NonConst = PointerLikeTypeTraits<T>;
   static inline const void *getAsVoidPointer(const T pointer)
   {
      return NonConst::getAsVoidPointer(pointer);
   }

   static inline const T getFromVoidPointer(const void *pointer)
   {
      return NonConst::getFromVoidPointer(const_cast<void *>(pointer));
   }

   enum {
      NumLowBitsAvailable = NonConst::NumLowBitsAvailable
   };
};

// Provide PointerLikeTypeTraits for const pointers.
template <typename T>
struct PointerLikeTypeTraits<const T *>
{
   using NonConst = PointerLikeTypeTraits<T *>;

   static inline const void *getAsVoidPointer(const T *pointer)
   {
      return NonConst::getAsVoidPointer(const_cast<T *>(pointer));
   }

   static inline const T *getFromVoidPointer(const void *pointer)
   {
      return NonConst::getFromVoidPointer(const_cast<void *>(pointer));
   }
   enum {
      NumLowBitsAvailable = NonConst::NumLowBitsAvailable
   };
};

// Provide PointerLikeTypeTraits for uintptr_t.
template <>
struct PointerLikeTypeTraits<uintptr_t>
{
   static inline void *getAsVoidPointer(uintptr_t pointer)
   {
      return reinterpret_cast<void *>(pointer);
   }

   static inline uintptr_t getFromVoidPointer(void *pointer)
   {
      return reinterpret_cast<uintptr_t>(pointer);
   }
   // No bits are available!
   enum {
      NumLowBitsAvailable = 0
   };
};


/// Provide suitable custom traits struct for function pointers.
///
/// Function pointers can't be directly given these traits as functions can't
/// have their alignment computed with `alignof` and we need different casting.
///
/// To rely on higher alignment for a specialized use, you can provide a
/// customized form of this template explicitly with higher alignment, and
/// potentially use alignment attributes on functions to satisfy that.
template <int Alignment, typename FunctionPointerT>
struct FunctionPointerLikeTypeTraits
{
  enum { NumLowBitsAvailable = internal::ConstantLog2<Alignment>::value };
  static inline void *getAsVoidPointer(FunctionPointerT ptr)
  {
    assert((reinterpret_cast<uintptr_t>(ptr) &
            ~((uintptr_t)-1 << NumLowBitsAvailable)) == 0 &&
           "Alignment not satisfied for an actual function pointer!");
    return reinterpret_cast<void *>(ptr);
  }

  static inline FunctionPointerT getFromVoidPointer(void *ptr)
  {
    return reinterpret_cast<FunctionPointerT>(ptr);
  }
};

/// Provide a default specialization for function pointers that assumes 4-byte
/// alignment.
///
/// We assume here that functions used with this are always at least 4-byte
/// aligned. This means that, for example, thumb functions won't work or systems
/// with weird unaligned function pointers won't work. But all practical systems
/// we support satisfy this requirement.
template <typename ReturnT, typename... ParamTs>
struct PointerLikeTypeTraits<ReturnT (*)(ParamTs...)>
    : FunctionPointerLikeTypeTraits<4, ReturnT (*)(ParamTs...)>
{};

} // utils
} // polar

#endif // POLARPHP_UTILS_POINTER_LIKE_TYPE_TRAITS_H
