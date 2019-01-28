// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/09.

#ifndef POLARPHP_GLOBAL_COMPILER_FEATURE_H
#define POLARPHP_GLOBAL_COMPILER_FEATURE_H

#include "polarphp/global/CompilerDetection.h"
#include <new>
#include <stddef.h>

#if defined(_MSC_VER)
#include <sal.h>
#endif

#ifndef __has_feature
# define __has_feature(x) 0
#endif

#ifndef __has_extension
# define __has_extension(x) 0
#endif

#ifndef __has_attribute
# define __has_attribute(x) 0
#endif

#ifndef __has_cpp_attribute
# define __has_cpp_attribute(x) 0
#endif

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif

/// \macro POLAR_GNUC_PREREQ
/// Extend the default __GNUC_PREREQ even if glibc's features.h isn't
/// available.
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

/// \macro POLAR_MSC_PREREQ
/// Is the compiler MSVC of at least the specified version?
/// The common \param version values to check for are:
///  * 1900: Microsoft Visual Studio 2015 / 14.0
#ifdef _MSC_VER
#define POLAR_MSC_PREREQ(version) (_MSC_VER >= (version))

// We require at least MSVC 2015.
#if !POLAR_MSC_PREREQ(1900)
#error POLAR requires at least MSVC 2015.
#endif

#else
#define POLAR_MSC_PREREQ(version) 0
#endif

/// Does the compiler support ref-qualifiers for *this?
///
/// Sadly, this is separate from just rvalue reference support because GCC
/// and MSVC implemented this later than everything else.
#if __has_feature(cxx_rvalue_references) || POLAR_GNUC_PREREQ(4, 8, 1)
#define POLAR_HAS_RVALUE_REFERENCE_THIS 1
#else
#define POLAR_HAS_RVALUE_REFERENCE_THIS 0
#endif

/// Expands to '&' if ref-qualifiers for *this are supported.
///
/// This can be used to provide lvalue/rvalue overrides of member functions.
/// The rvalue override should be guarded by POLAR_HAS_RVALUE_REFERENCE_THIS
#if POLAR_HAS_RVALUE_REFERENCE_THIS
#define POLAR_LVALUE_FUNCTION &
#else
#define POLAR_LVALUE_FUNCTION
#endif

/// POLAR_LIBRARY_VISIBILITY - If a class marked with this attribute is linked
/// into a shared library, then the class should be private to the library and
/// not accessible from outside it.  Can also be used to mark variables and
/// functions, making them private to any shared library they are linked into.
/// On PE/COFF targets, library visibility is the default, so this isn't needed.
#if (__has_attribute(visibility) || POLAR_GNUC_PREREQ(4, 0, 0)) &&              \
   !defined(__MINGW32__) && !defined(__CYGWIN__) && !defined(_WIN32)
#define POLAR_LIBRARY_VISIBILITY __attribute__ ((visibility("hidden")))
#else
#define POLAR_LIBRARY_VISIBILITY
#endif

#if defined(__GNUC__)
#define POLAR_PREFETCH(addr, rw, locality) __builtin_prefetch(addr, rw, locality)
#else
#define POLAR_PREFETCH(addr, rw, locality)
#endif

#if __has_attribute(used) || POLAR_GNUC_PREREQ(3, 1, 0)
#define POLAR_ATTRIBUTE_USED __attribute__((__used__))
#else
#define POLAR_ATTRIBUTE_USED
#endif

/// POLAR_NODISCARD - Warn if a type or return value is discarded.
#if __cplusplus > 201402L && __has_cpp_attribute(nodiscard)
#define POLAR_NODISCARD [[nodiscard]]
#elif !__cplusplus
// Workaround for llvm.org/PR23435, since clang 3.6 and below emit a spurious
// error when __has_cpp_attribute is given a scoped attribute in C mode.
#define POLAR_NODISCARD
#elif __has_cpp_attribute(clang::warn_unused_result)
#define POLAR_NODISCARD [[clang::warn_unused_result]]
#else
#define POLAR_NODISCARD
#endif


// Indicate that a non-static, non-const C++ member function reinitializes
// the entire object to a known state, independent of the previous state of
// the object.
//
// The clang-tidy check bugprone-use-after-move recognizes this attribute as a
// marker that a moved-from object has left the indeterminate state and can be
// reused.
#if __has_cpp_attribute(clang::reinitializes)
#define POLAR_ATTRIBUTE_REINITIALIZES [[clang::reinitializes]]
#else
#define POLAR_ATTRIBUTE_REINITIALIZES
#endif

// Some compilers warn about unused functions. When a function is sometimes
// used or not depending on build settings (e.g. a function only called from
// within "assert"), this attribute can be used to suppress such warnings.
//
// However, it shouldn't be used for unused *variables*, as those have a much
// more portable solution:
//   (void)unused_var_name;
// Prefer cast-to-void wherever it is sufficient.
#if __has_attribute(unused) || POLAR_GNUC_PREREQ(3, 1, 0)
#define POLAR_ATTRIBUTE_UNUSED __attribute__((__unused__))
#else
#define POLAR_ATTRIBUTE_UNUSED
#endif

// FIXME: Provide this for PE/COFF targets.
#if (__has_attribute(weak) || POLAR_GNUC_PREREQ(4, 0, 0)) &&                    \
   (!defined(__MINGW32__) && !defined(__CYGWIN__) && !defined(_WIN32))
#define POLAR_ATTRIBUTE_WEAK __attribute__((__weak__))
#else
#define POLAR_ATTRIBUTE_WEAK
#endif

// Prior to clang 3.2, clang did not accept any spelling of
// __has_attribute(const), so assume it is supported.
#if defined(__clang__) || defined(__GNUC__)
// aka 'CONST' but following POLAR Conventions.
#define POLAR_READNONE __attribute__((__const__))
#else
#define POLAR_READNONE
#endif

#if __has_attribute(pure) || defined(__GNUC__)
// aka 'PURE' but following POLAR Conventions.
#define POLAR_READONLY __attribute__((__pure__))
#else
#define POLAR_READONLY
#endif

/// POLAR_ATTRIBUTE_NOINLINE - On compilers where we have a directive to do so,
/// mark a method "not for inlining".
#if __has_attribute(noinline) || POLAR_GNUC_PREREQ(3, 4, 0)
#define POLAR_ATTRIBUTE_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define POLAR_ATTRIBUTE_NOINLINE __declspec(noinline)
#else
#define POLAR_ATTRIBUTE_NOINLINE
#endif

/// POLAR_ATTRIBUTE_ALWAYS_INLINE - On compilers where we have a directive to do
/// so, mark a method "always inline" because it is performance sensitive. GCC
/// 3.4 supported this but is buggy in various cases and produces unimplemented
/// errors, just use it in GCC 4.0 and later.
#if __has_attribute(always_inline) || POLAR_GNUC_PREREQ(4, 0, 0)
#define POLAR_ATTRIBUTE_ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
#define POLAR_ATTRIBUTE_ALWAYS_INLINE __forceinline
#else
#define POLAR_ATTRIBUTE_ALWAYS_INLINE
#endif

#ifdef __GNUC__
#define POLAR_ATTRIBUTE_NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define POLAR_ATTRIBUTE_NORETURN __declspec(noreturn)
#else
#define POLAR_ATTRIBUTE_NORETURN
#endif

#if __has_attribute(returns_nonnull) || POLAR_GNUC_PREREQ(4, 9, 0)
#define POLAR_ATTRIBUTE_RETURNS_NONNULL __attribute__((returns_nonnull))
#elif defined(_MSC_VER)
#define POLAR_ATTRIBUTE_RETURNS_NONNULL _Ret_notnull_
#else
#define POLAR_ATTRIBUTE_RETURNS_NONNULL
#endif

/// \macro POLAR_ATTRIBUTE_RETURNS_NOALIAS Used to mark a function as returning a
/// pointer that does not alias any other valid pointer.
#ifdef __GNUC__
#define POLAR_ATTRIBUTE_RETURNS_NOALIAS __attribute__((__malloc__))
#elif defined(_MSC_VER)
#define POLAR_ATTRIBUTE_RETURNS_NOALIAS __declspec(restrict)
#else
#define POLAR_ATTRIBUTE_RETURNS_NOALIAS
#endif

/// POLAR_EXTENSION - Support compilers where we have a keyword to suppress
/// pedantic diagnostics.
#ifdef __GNUC__
#define POLAR_EXTENSION __extension__
#else
#define POLAR_EXTENSION
#endif

// POLAR_ATTRIBUTE_DEPRECATED(decl, "message")
#if __has_feature(attribute_deprecated_with_message)
# define POLAR_ATTRIBUTE_DEPRECATED(decl, message) \
   decl __attribute__((deprecated(message)))
#elif defined(__GNUC__)
# define POLAR_ATTRIBUTE_DEPRECATED(decl, message) \
   decl __attribute__((deprecated))
#elif defined(_MSC_VER)
# define POLAR_ATTRIBUTE_DEPRECATED(decl, message) \
   __declspec(deprecated(message)) decl
#else
# define POLAR_ATTRIBUTE_DEPRECATED(decl, message) \
   decl
#endif

/// POLAR_BUILTIN_UNREACHABLE - On compilers which support it, expands
/// to an expression which states that it is undefined behavior for the
/// compiler to reach this point.  Otherwise is not defined.
#if __has_builtin(__builtin_unreachable) || POLAR_GNUC_PREREQ(4, 5, 0)
# define POLAR_BUILTIN_UNREACHABLE __builtin_unreachable()
#elif defined(_MSC_VER)
# define POLAR_BUILTIN_UNREACHABLE __assume(false)
#endif

/// POLAR_BUILTIN_TRAP - On compilers which support it, expands to an expression
/// which causes the program to exit abnormally.
#if __has_builtin(__builtin_trap) || POLAR_GNUC_PREREQ(4, 3, 0)
# define POLAR_BUILTIN_TRAP __builtin_trap()
#elif defined(_MSC_VER)
// The __debugbreak intrinsic is supported by MSVC, does not require forward
// declarations involving platform-specific typedefs (unlike RaiseException),
// results in a call to vectored exception handlers, and encodes to a short
// instruction that still causes the trapping behavior we want.
# define POLAR_BUILTIN_TRAP __debugbreak()
#else
# define POLAR_BUILTIN_TRAP *(volatile int*)0x11 = 0
#endif

/// POLAR_BUILTIN_DEBUGTRAP - On compilers which support it, expands to
/// an expression which causes the program to break while running
/// under a debugger.
#if __has_builtin(__builtin_debugtrap)
# define POLAR_BUILTIN_DEBUGTRAP __builtin_debugtrap()
#elif defined(_MSC_VER)
// The __debugbreak intrinsic is supported by MSVC and breaks while
// running under the debugger, and also supports invoking a debugger
// when the OS is configured appropriately.
# define POLAR_BUILTIN_DEBUGTRAP __debugbreak()
#else
// Just continue execution when built with compilers that have no
// support. This is a debugging aid and not intended to force the
// program to abort if encountered.
# define POLAR_BUILTIN_DEBUGTRAP
#endif

/// \macro POLAR_ASSUME_ALIGNED
/// Returns a pointer with an assumed alignment.
#if __has_builtin(__builtin_assume_aligned) || POLAR_GNUC_PREREQ(4, 7, 0)
# define POLAR_ASSUME_ALIGNED(p, a) __builtin_assume_aligned(p, a)
#elif defined(POLAR_BUILTIN_UNREACHABLE)
// As of today, clang does not support __builtin_assume_aligned.
# define POLAR_ASSUME_ALIGNED(p, a) \
   (((uintptr_t(p) % (a)) == 0) ? (p) : (POLAR_BUILTIN_UNREACHABLE, (p)))
#else
# define POLAR_ASSUME_ALIGNED(p, a) (p)
#endif

/// \macro POLAR_ALIGNAS
/// Used to specify a minimum alignment for a structure or variable.
#if __GNUC__ && !__has_feature(cxx_alignas) && !POLAR_GNUC_PREREQ(4, 8, 1)
# define POLAR_ALIGNAS(x) __attribute__((aligned(x)))
#else
# define POLAR_ALIGNAS(x) alignas(x)
#endif

/// \macro POLAR_PTR_SIZE
/// A constant integer equivalent to the value of sizeof(void*).
/// Generally used in combination with POLAR_ALIGNAS or when doing computation in
/// the preprocessor.
#ifdef __SIZEOF_POINTER__
# define POLAR_PTR_SIZE __SIZEOF_POINTER__
#elif defined(_WIN64)
# define POLAR_PTR_SIZE 8
#elif defined(_WIN32)
# define POLAR_PTR_SIZE 4
#elif defined(_MSC_VER)
# error "could not determine POLAR_PTR_SIZE as a constant int for MSVC"
#else
# define POLAR_PTR_SIZE sizeof(void *)
#endif

/// \macro POLAR_MEMORY_SANITIZER_BUILD
/// Whether POLAR itself is built with MemorySanitizer instrumentation.
#if __has_feature(memory_sanitizer)
# define POLAR_MEMORY_SANITIZER_BUILD 1
# include <sanitizer/msan_interface.h>
#else
# define POLAR_MEMORY_SANITIZER_BUILD 0
# define __msan_allocated_memory(p, size)
# define __msan_unpoison(p, size)
#endif

/// \macro POLAR_ADDRESS_SANITIZER_BUILD
/// Whether POLAR itself is built with AddressSanitizer instrumentation.
#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
# define POLAR_ADDRESS_SANITIZER_BUILD 1
# include <sanitizer/asan_interface.h>
#else
# define POLAR_ADDRESS_SANITIZER_BUILD 0
# define __asan_poison_memory_region(p, size)
# define __asan_unpoison_memory_region(p, size)
#endif

/// \macro POLAR_THREAD_SANITIZER_BUILD
/// Whether POLAR itself is built with ThreadSanitizer instrumentation.
#if __has_feature(thread_sanitizer) || defined(__SANITIZE_THREAD__)
# define POLAR_THREAD_SANITIZER_BUILD 1
#else
# define POLAR_THREAD_SANITIZER_BUILD 0
#endif

#if POLAR_THREAD_SANITIZER_BUILD
// Thread Sanitizer is a tool that finds races in code.
// See http://code.google.com/p/data-race-test/wiki/DynamicAnnotations .
// tsan detects these exact functions by name.
#ifdef __cplusplus
extern "C" {
#endif
void AnnotateHappensAfter(const char *file, int line, const volatile void *cv);
void AnnotateHappensBefore(const char *file, int line, const volatile void *cv);
void AnnotateIgnoreWritesBegin(const char *file, int line);
void AnnotateIgnoreWritesEnd(const char *file, int line);
#ifdef __cplusplus
}
#endif

// This marker is used to define a happens-before arc. The race detector will
// infer an arc from the begin to the end when they share the same pointer
// argument.
# define TsanHappensBefore(cv) AnnotateHappensBefore(__FILE__, __LINE__, cv)

// This marker defines the destination of a happens-before arc.
# define TsanHappensAfter(cv) AnnotateHappensAfter(__FILE__, __LINE__, cv)

// Ignore any races on writes between here and the next TsanIgnoreWritesEnd.
# define TsanIgnoreWritesBegin() AnnotateIgnoreWritesBegin(__FILE__, __LINE__)

// Resume checking for racy writes.
# define TsanIgnoreWritesEnd() AnnotateIgnoreWritesEnd(__FILE__, __LINE__)
#else
# define TsanHappensBefore(cv)
# define TsanHappensAfter(cv)
# define TsanIgnoreWritesBegin()
# define TsanIgnoreWritesEnd()
#endif

/// \macro POLAR_NO_SANITIZE
/// Disable a particular sanitizer for a function.
#if __has_attribute(no_sanitize)
#define POLAR_NO_SANITIZE(KIND) __attribute__((no_sanitize(KIND)))
#else
#define POLAR_NO_SANITIZE(KIND)
#endif

/// Mark debug helper function definitions like dump() that should not be
/// stripped from debug builds.
/// Note that you should also surround dump() functions with
/// `#if !defined(NDEBUG) || defined(POLAR_ENABLE_DUMP)` so they do always
/// get stripped in release builds.
// FIXME: Move this to a private config.h as it's not usable in public headers.
#if !defined(NDEBUG) || defined(POLAR_ENABLE_DUMP)
#define POLAR_DUMP_METHOD POLAR_ATTRIBUTE_NOINLINE POLAR_ATTRIBUTE_USED
#else
#define POLAR_DUMP_METHOD POLAR_ATTRIBUTE_NOINLINE
#endif

/// \macro POLAR_PRETTY_FUNCTION
/// Gets a user-friendly looking function signature for the current scope
/// using the best available method on each platform.  The exact format of the
/// resulting string is implementation specific and non-portable, so this should
/// only be used, for example, for logging or diagnostics.
#if defined(_MSC_VER)
#define POLAR_PRETTY_FUNCTION __FUNCSIG__
#elif defined(__GNUC__) || defined(__clang__)
#define POLAR_PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
#define POLAR_PRETTY_FUNCTION __func__
#endif

/// \macro POLAR_THREAD_LOCAL
/// A thread-local storage specifier which can be used with globals,
/// extern globals, and static globals.
///
/// This is essentially an extremely restricted analog to C++11's thread_local
/// support, and uses that when available. However, it falls back on
/// platform-specific or vendor-provided extensions when necessary. These
/// extensions don't support many of the C++11 thread_local's features. You
/// should only use this for PODs that you can statically initialize to
/// some constant value. In almost all circumstances this is most appropriate
/// for use with a pointer, integer, or small aggregation of pointers and
/// integers.
#if POLAR_ENABLE_THREADS
#if __has_feature(cxx_thread_local)
#define POLAR_THREAD_LOCAL thread_local
#elif defined(_MSC_VER)
// MSVC supports this with a __declspec.
#define POLAR_THREAD_LOCAL __declspec(thread)
#else
// Clang, GCC, and other compatible compilers used __thread prior to C++11 and
// we only need the restricted functionality that provides.
#define POLAR_THREAD_LOCAL __thread
#endif
#else // !POLAR_ENABLE_THREADS
// If threading is disabled entirely, this compiles to nothing and you get
// a normal global variable.
#define POLAR_THREAD_LOCAL
#endif

/// \macro POLAR_ENABLE_EXCEPTIONS
/// Whether POLAR is built with exception support.
#if __has_feature(cxx_exceptions)
#define POLAR_ENABLE_EXCEPTIONS 1
#elif defined(__GNUC__) && defined(__EXCEPTIONS)
#define POLAR_ENABLE_EXCEPTIONS 1
#elif defined(_MSC_VER) && defined(_CPPUNWIND)
#define POLAR_ENABLE_EXCEPTIONS 1
#endif

namespace polar {

/// Allocate a buffer of memory with the given size and alignment.
///
/// When the compiler supports aligned operator new, this will use it to to
/// handle even over-aligned allocations.
///
/// However, this doesn't make any attempt to leverage the fancier techniques
/// like posix_memalign due to portability. It is mostly intended to allow
/// compatibility with platforms that, after aligned allocation was added, use
/// reduced default alignment.
inline void *allocate_buffer(size_t size, size_t alignment) {
   return ::operator new(size
                      #ifdef __cpp_aligned_new
                         ,
                         std::align_val_t(alignment)
                      #endif
                         );
}

/// Deallocate a buffer of memory with the given size and alignment.
///
/// If supported, this will used the sized delete operator. Also if supported,
/// this will pass the alignment to the delete operator.
///
/// The pointer must have been allocated with the corresponding new operator,
/// most likely using the above helper.
inline void deallocate_buffer(void *ptr, size_t size, size_t alignment) {
   ::operator delete(ptr
      #ifdef __cpp_sized_deallocation
         ,
         size
      #endif
      #if __cpp_aligned_new
         ,
         std::align_val_t(alignment)
      #endif
         );
}

} // polar

#ifdef POLAR_CC_GNU
#define POLAR_BEGIN_DISABLE_ZENDVM_WARNING() \
   POLAR_WARNING_PUSH\
   _Pragma("GCC diagnostic ignored \"-Wignored-qualifiers\"")\
   _Pragma("GCC diagnostic ignored \"-Wpedantic\"")

#define POLAR_END_DISABLE_ZENDVM_WARNING() POLAR_WARNING_POP
#endif

#endif // POLAR_DEVLTOOLS_UTILS_COMPILER_FEATURE_H
