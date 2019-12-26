//===--- Visibility.h - Visibility macros for runtime exports ---*- C++ -*-===//
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
//
//  These macros are used to declare symbols that should be exported from the
//  Polarphp runtime.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_ABI_SHIMS_VISIBILITY_H
#define POLARPHP_ABI_SHIMS_VISIBILITY_H

#if !defined(__has_feature)
#define __has_feature(x) 0
#endif

#if !defined(__has_attribute)
#define __has_attribute(x) 0
#endif

#if !defined(__has_builtin)
#define __has_builtin(builtin) 0
#endif

#if __has_feature(nullability)
// Provide macros to temporarily suppress warning about the use of
// _Nullable and _Nonnull.
# define POLAR_BEGIN_NULLABILITY_ANNOTATIONS                        \
  _Pragma("clang diagnostic push")                                  \
  _Pragma("clang diagnostic ignored \"-Wnullability-extension\"")
# define POLAR_END_NULLABILITY_ANNOTATIONS                          \
  _Pragma("clang diagnostic pop")

#else
// #define _Nullable and _Nonnull to nothing if we're not being built
// with a compiler that supports them.
# define _Nullable
# define _Nonnull
# define POLAR_BEGIN_NULLABILITY_ANNOTATIONS
# define POLAR_END_NULLABILITY_ANNOTATIONS
#endif

#if __has_attribute(pure)
#define POLAR_READONLY __attribute__((__pure__))
#else
#define POLAR_READONLY
#endif

#if __has_attribute(const)
#define POLAR_READNONE __attribute__((__const__))
#else
#define POLAR_READNONE
#endif

#if __has_attribute(always_inline)
#define POLAR_ALWAYS_INLINE __attribute__((always_inline))
#else
#define POLAR_ALWAYS_INLINE
#endif

#if __has_attribute(unavailable)
#define POLAR_ATTRIBUTE_UNAVAILABLE __attribute__((__unavailable__))
#else
#define POLAR_ATTRIBUTE_UNAVAILABLE
#endif

// TODO: support using shims headers in overlays by parameterizing
// POLAR_RUNTIME_EXPORT on the library it's exported from.

/// Attribute used to export symbols from the runtime.
#if defined(__MACH__)

# define POLAR_EXPORT_ATTRIBUTE __attribute__((__visibility__("default")))

#elif defined(__ELF__)

// We make assumptions that the runtime and standard library can refer to each
// other's symbols as DSO-local, which means we can't allow the dynamic linker
// to relocate these symbols. We must give them protected visibility while
// building the standard library and runtime.
# if defined(swiftCore_EXPORTS)
#  define POLAR_EXPORT_ATTRIBUTE __attribute__((__visibility__("protected")))
# else
#  define POLAR_EXPORT_ATTRIBUTE __attribute__((__visibility__("default")))
# endif

// FIXME: this #else should be some sort of #elif Windows
#else // !__MACH__ && !__ELF__

# if defined(__CYGWIN__)
#  define POLAR_EXPORT_ATTRIBUTE
# else

#  if defined(swiftCore_EXPORTS)
#   define POLAR_EXPORT_ATTRIBUTE __declspec(dllexport)
#  else
#   define POLAR_EXPORT_ATTRIBUTE __declspec(dllimport)
#  endif

# endif

#endif

#if defined(__cplusplus)
#define POLAR_RUNTIME_EXPORT extern "C" POLAR_EXPORT_ATTRIBUTE
#else
#define POLAR_RUNTIME_EXPORT POLAR_EXPORT_ATTRIBUTE
#endif


#ifndef POLAR_GNUC_PREREQ
# if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#  define POLAR_GNUC_PREREQ(maj, min, patch) \
    ((__GNUC__ << 20) + (__GNUC_MINOR__ << 10) + __GNUC_PATCHLEVEL__ >= \
     ((maj) << 20) + ((min) << 10) + (patch))
# elif defined(__GNUC__) && defined(__GNUC_MINOR__)
#  define POLAR_GNUC_PREREQ(maj, min, patch) \
    ((__GNUC__ << 20) + (__GNUC_MINOR__ << 10) >= ((maj) << 20) + ((min) << 10))
# else
#  define POLAR_GNUC_PREREQ(maj, min, patch) 0
# endif
#endif

/// Attributes for runtime-stdlib interfaces.
/// Use these for C implementations that are imported into Polarphp via PolarphpShims
/// and for C implementations of Polarphp @_silgen_name declarations
/// Note that @_silgen_name implementations must also be marked POLAR_CC(swift).
///
/// POLAR_RUNTIME_STDLIB_API functions are called by compiler-generated code
/// or by @inlinable Polarphp code.
/// Such functions must be exported and must be supported forever as API.
/// The function name should be prefixed with `swift_`.
///
/// POLAR_RUNTIME_STDLIB_SPI functions are called by overlay code.
/// Such functions must be exported, but are still SPI
/// and may be changed at any time.
/// The function name should be prefixed with `_polarphp_`.
///
/// POLAR_RUNTIME_STDLIB_INTERNAL functions are called only by the stdlib.
/// Such functions are internal and are not exported.
#define POLAR_RUNTIME_STDLIB_API       POLAR_RUNTIME_EXPORT
#define POLAR_RUNTIME_STDLIB_SPI       POLAR_RUNTIME_EXPORT

// Match the definition of LLVM_LIBRARY_VISIBILITY from LLVM's
// Compiler.h. That header requires C++ and this needs to work in C.
#if (__has_attribute(visibility) || POLAR_GNUC_PREREQ(4, 0, 0)) &&              \
    !defined(__MINGW32__) && !defined(__CYGWIN__) && !defined(_WIN32)
#define POLAR_LIBRARY_VISIBILITY __attribute__ ((visibility("hidden")))
#else
#define POLAR_LIBRARY_VISIBILITY
#endif

#if defined(__cplusplus)
#define POLAR_RUNTIME_STDLIB_INTERNAL extern "C" POLAR_LIBRARY_VISIBILITY
#else
#define POLAR_RUNTIME_STDLIB_INTERNAL POLAR_LIBRARY_VISIBILITY
#endif

#if __has_builtin(__builtin_expect)
#  ifndef POLAR_LIKELY
#define POLAR_LIKELY(expression) (__builtin_expect(!!(expression), 1))
#  endif
#ifndef POLAR_UNLIKELY
#  define POLAR_UNLIKELY(expression) (__builtin_expect(!!(expression), 0))
#endif
#else
#  ifndef POLAR_LIKELY
#define POLAR_LIKELY(expression) ((expression))
#  endif
#  ifndef POLAR_UNLIKELY
#define POLAR_UNLIKELY(expression) ((expression))
#  endif
#endif

// POLARPHP_ABI_SHIMS_VISIBILITY_H
#endif
