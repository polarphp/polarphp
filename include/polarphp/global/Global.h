// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/05/27.

#ifndef POLARPHP_GLOBAL_GLOBAL_H
#define POLARPHP_GLOBAL_GLOBAL_H

#ifdef __cplusplus
#  include <type_traits>
#  include <cstddef>
#  include <utility>
#  include <algorithm>
#  include <memory>
#endif
#ifndef __ASSEMBLER__
#  include <stddef.h>
#endif

#include "polarphp/global/Config.h"
#include "polarphp/global/DataTypes.h"

#define POLAR_STRINGIFY2(x) #x
#define POLAR_STRINGIFY(x) POLAR_STRINGIFY2(x)

#include "polarphp/global/SystemDetection.h"
#include "polarphp/global/ProcessorDetection.h"
#include "polarphp/global/CompilerDetection.h"

#if defined(__ELF__)
#   define POLAR_OF_ELF
#endif

#if defined(__MACH__) && defined(__APPLE__)
#   define POLAR_OF_MACH_O
#endif

#if defined(POLAR_SHARED) || !defined(POLAR_STATIC)
#  ifdef POLAR_STATIC
#     error "Both POLAR_SHARED and POLAR_STATIC defined, pelase check up your cmake options!"
#  endif
#  ifndef POLAR_SHARED
#     define POLAR_SHARED
#  endif
#  if defined(POLAR_BUILD_SELF)
#     define POLAR_CORE_EXPORT POLAR_DECL_IMPORT
#  else
#     define POLAR_CORE_EXPORT POLAR_DECL_EXPORT
#  endif
#else
#  define POLAR_CORE_EXPORT
#endif

#define POLAR_CONFIG(feature) (1/POLAR_FEATURE_##feature == 1)
#define POLAR_REQUIRE_CONFIG(feature) POLAR_STATIC_ASSERT_X(POLAR_FEATURE_##feature == 1, "Required feature " #feature " for file " __FILE__ " not available.")

#define POLAR_DISABLE_COPY(Class)\
   Class(const Class &) = delete;\
   Class &operator=(const Class &) = delete

#if defined(__i386__) || defined(_WIN32)
#  if defined(POLAR_CC_GNU)
#     define POLAR_FASTCALL __attribute__((regparm(3)))
#  elif defined(POLAR_CC_MSVC)
#     define POLAR_FASTCALL __fastcall
#  else
#     define POLAR_FASTCALL
#  endif
#else
#  define POLAR_FASTCALL
#endif
/*
   Avoid "unused parameter" warnings
*/
#define POLAR_UNUSED(x) (void)x

#define POLAR_TERMINATE_ON_EXCEPTION(expr) do { expr; } while (false)

#if !defined(POLAR_NO_DEBUG) && !defined(POLAR_DEBUG_MODE)
#  define POLAR_DEBUG_MODE
#endif

#if defined(POLAR_CC_GNU) && !defined(__INSURE__)
#  if defined(POLAR_CC_MINGW) && !defined(POLAR_CC_CLANG)
#     define POLAR_ATTRIBUTE_FORMAT_PRINTF(A, B) \
   __attribute__((format(gnu_printf, (A), (B))))
#  else
#     define POLAR_ATTRIBUTE_FORMAT_PRINTF(A, B) \
   __attribute__((format(printf, (A), (B))))
#  endif
#else
#  define POLAR_ATTRIBUTE_FORMAT_PRINTF(A, B)
#endif

#ifndef __ASSEMBLER__
#  ifdef __cplusplus
namespace polar
{

using longlong = std::int64_t;
using ulonglong = std::uint64_t;

inline void polar_noop(void) {}

} // POLAR

#  endif // __cplusplus

using uchar = unsigned char ;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;

#  ifdef __cplusplus
namespace polar
{

//#ifndef POLAR_CC_MSVC
//POLAR_NORETURN
//#endif
POLAR_CORE_EXPORT void polar_assert(const char *assertion, const char *file,
                                    int line) noexcept;

#if !defined(POLAR_ASSERT)
#  if defined(POLAR_NO_DEBUG) && !defined(POLAR_FORCE_ASSERTS)
#     define POLAR_ASSERT(cond) static_cast<void>(false && (cond))
#  else
#     define POLAR_ASSERT(cond) ((cond) ? static_cast<void>(0) : ::polar::polar_assert(#cond,__FILE__,__LINE__))
#  endif
#endif

/*
  uintptr and ptrdiff is guaranteed to be the same size as a pointer, i.e.
      sizeof(void *) == sizeof(uintptr)
      && sizeof(void *) == sizeof(ptrdiff)
*/
template <int> struct IntegerForSize;

template <>
struct IntegerForSize<1>
{
   using Unsigned = std::uint8_t;
   using Signed = std::int8_t;
};

template <>
struct IntegerForSize<2>
{
   using Unsigned = std::uint16_t;
   using Signed = std::int16_t;
};

template <>
struct IntegerForSize<4>
{
   using Unsigned = std::uint32_t;
   using Signed = std::int32_t;
};

template <>
struct IntegerForSize<8>
{
   using Unsigned = std::uint64_t;
   using Signed = std::int64_t;
};

#if defined(POLAR_CC_GNU) && defined(__SIZEOF_INT128__)
template <>
struct IntegerForSize<16>
{
   __extension__ typedef unsigned __int128 Unsigned;
   __extension__ typedef __int128 Signed;
};
#endif

template <typename T>
struct IntegerForSizeof : IntegerForSize<sizeof(T)>
{};

using registerint = typename IntegerForSize<POLAR_PROCESSOR_WORDSIZE>::Signed;
using registeruint = typename IntegerForSize<POLAR_PROCESSOR_WORDSIZE>::Unsigned;
using uintptr = typename IntegerForSizeof<void *>::Unsigned;
using intptr = typename IntegerForSizeof<void *>::Signed;
using ptrdiff = intptr;
using sizetype = typename IntegerForSizeof<std::size_t>::Signed;

template <typename T>
static inline T *get_ptr_helper(T *ptr)
{
   return ptr;
}

template <typename T>
static inline typename T::pointer get_ptr_helper(const T &p)
{
   return p.get();
}

template <typename T>
static inline T *get_ptr_helper(const std::shared_ptr<T> &p)
{
   return p.get();
}

template <typename T>
static inline T *get_ptr_helper(const std::unique_ptr<T> &p)
{
   return p.get();
}

#define POLAR_DECLARE_PRIVATE(Class)\
   inline Class##Private* getImplPtr()\
   {\
      return reinterpret_cast<Class##Private *>(polar::get_ptr_helper(m_implPtr));\
   }\
   inline const Class##Private* getImplPtr() const\
   {\
      return reinterpret_cast<const Class##Private *>(polar::get_ptr_helper(m_implPtr));\
   }\
   friend class Class##Private

#define POLAR_DECLARE_PUBLIC(Class)\
   inline Class* getApiPtr()\
   {\
      return static_cast<Class *>(m_apiPtr);\
   }\
   inline const Class* getApiPtr() const\
   {\
      return static_cast<const Class *>(m_apiPtr);\
   }\
   friend class Class

#define POLAR_D(Class) Class##Private * const implPtr = getImplPtr()
#define POLAR_Q(Class) Class * const apiPtr = getApiPtr()

//#ifndef POLAR_CC_MSVC
//POLAR_NORETURN
//#endif
POLAR_CORE_EXPORT void polar_assert_x(const char *where, const char *what,
                                      const char *file, int line) noexcept;

#if !defined(POLAR_ASSERT_X)
#  if defined(POLAR_NO_DEBUG) && !defined(POLAR_FORCE_ASSERTS)
#     define POLAR_ASSERT_X(cond, where, what) do {} while ((false) && (cond))
#  else
#     define POLAR_ASSERT_X(cond, where, what) ((!(cond)) ? polar::polar_assert_x(where, what,__FILE__,__LINE__) : polar::polar_noop())
#  endif
#endif

#define POLAR_STATIC_ASSERT(Condition) static_assert(bool(Condition), #Condition)
#define POLAR_STATIC_ASSERT_X(Condition, Message) static_assert(bool(Condition), Message)

using NoImplicitBoolCast = int;

#define POLAR_CHECK_ALLOC_PTR(ptr) do { if (!(ptr)) throw std::bad_alloc(); } while (false)

template <typename T>
constexpr const T &bound(const T &min, const T &value, const T &max)
{
   return std::max(min, std::min(value, max));
}

// just as std::as_const
template <typename T>
constexpr typename std::add_const<T>::type &as_const(T &value) noexcept
{
   return value;
}

template <typename T>
void as_const(const T &&) = delete;

#  endif // __cplusplus
#endif // __ASSEMBLER__

template <typename Enumeration>
constexpr auto as_integer(Enumeration const value)
-> typename std::underlying_type<Enumeration>::type
{
   return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

} // polar

#include "CompilerFeature.h"

#define POLAR_CLI11_PARSE(app, argc, argv) CLI11_PARSE(app, argc, argv) (void) 0

#endif // POLARPHP_GLOBAL_GLOBAL_H
