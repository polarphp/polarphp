// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/05/27.

#ifndef POLARPHP_GLOBAL_COMPILERDETECTION_H
#define POLARPHP_GLOBAL_COMPILERDETECTION_H

/*
   The compiler, must be one of: (POLAR_CC_x)
     SYM      - Digital Mars C/C++ (used to be Symantec C++)
     MSVC     - Microsoft Visual C/C++, Intel C++ for Windows
     BOR      - Borland/Turbo C++
     WAT      - Watcom C++
     GNU      - GNU C++
     COMEAU   - Comeau C++
     EDG      - Edison Design Group C++
     OC       - CenterLine C++
     SUN      - Forte Developer, or Sun Studio C++
     MIPS     - MIPSpro C++
     DEC      - DEC C++
     HPACC    - HP aC++
     USLC     - SCO OUDK and UDK
     CDS      - Reliant C++
     KAI      - KAI C++
     INTEL    - Intel C++ for Linux, Intel C++ for Windows
     HIGHC    - MetaWare High C/C++
     PGI      - Portland Group C++
     GHS      - Green Hills Optimizing C++ Compilers
     RVCT     - ARM Realview Compiler Suite
     CLANG    - C++ front-end for the LLVM compiler
   Should be sorted most to least authoritative.
*/

/* Symantec C++ is now Digital Mars */
#if defined(__DMC__) || defined(__SC__)
#   define POLAR_CC_SYM
/* "explicit" semantics implemented in 8.1e but keyword recognized since 7.5 */
#   if defined(__SC__) && __SC__ < 0x750
#      error "Compiler not supported"
#   endif
#elif defined(_MSC_VER)
#   ifdef __clang__
#      define POLAR_CC_CLANG ((__clang_major__ * 100) + __clang_minor__)
#   endif
#   define POLAR_CC_MSVC (_MSC_VER)
#   define POLAR_CC_MSVC_NET
#   define POLAR_OUTOFLINE_TEMPLATE inline
#   if _MSC_VER < 1600
#      define POLAR_NO_TEMPLATE_FRIENDS
#   endif
#   define POLAR_COMPILER_MANGLES_RETURN_TYPE
#   define POLAR_FUNC_INFO __FUNCSIG__
#   define POLAR_ALIGNOF(type) __alignof(type)
#   define POLAR_DECL_ALIGN(n) __declspec(align(n))
#   define POLAR_ASSUME_IMPL() __assume(0)
#   define POLAR_NORETURN __declspec(noreturn)
#   define POLAR_DECL_DEPRECATED __declspec(deprecated)
#   ifndef POLAR_CC_CLANG
#      define POLAR_DECL_DEPRECATED_X(text) __declspec(deprecated(text))
#   endif
#   define POLAR_DECL_EXPORT __declspec(dllexport)
#   define POLAR_DECL_IMPORT __declspec(dllimport)
#   if _MSC_VER >= 1800
#      define POLAR_MAKE_UNCHECKED_ARRAY_ITERATOR(x) stdext::make_unchecked_array_iterator(x)
#   endif
#   if _MSC_VER >= 1500
#      define POLAR_MAKE_CHECKED_ARRAY_ITERATOR(x, N) stdext::make_checked_array_iterator(x, size_t(N))
#   endif
/* Intel C++ disguising as Visual C++: the `using' keyword avoids warnings */
#   if defined(__INTEL_COMPILER)
#      define POLAR_DECL_VARIABLE_DEPRECATED
#      define POLAR_CC_INTEL __INTEL_COMPILER
#   endif

/* only defined for MSVC since that's the only compiler that actually optimizes for this */
/* might get overridden further down when POLAR_COMPILER_NOEXCEPT is detected */
#   ifdef __cplusplus
#      define POLAR_DECL_NOTHROW  throw()
#   endif
#elif defined(__BORLANDC__) || defined(__TURBOC__)
#   define POLAR_CC_BORLAND
#   define POLAR_INLINE_TEMPLATE
#   if __BORLANDC__ < 0x502
#      error "Compiler not supported"
#   endif
#elif defined(__WATCOMC__)
#   define POLAR_CC_WAT

/* ARM Realview Compiler Suite
   RVCT compiler also defines __EDG__ and __GNUC__ (if --gnu flag is given),
   so check for it before that */
#elif defined(__ARMCC__) || defined(__CC_ARM)
#   define POLAR_CC_RVCT
/* work-around for missing compiler intrinsics */
#   define __is_empty(x) false
#   define __is_pod(x) false
#   define POLAR_DECL_DEPRECATED __attribute__ ((__deprecated__))
#   ifdef POLAR_OS_LINUX
#      define POLAR_DECL_EXPORT __attribute__((visibility("default")))
#      define POLAR_DECL_IMPORT __attribute__((visibility("default")))
#      define POLAR_DECL_HIDDEN __attribute__((visibility("hidden")))
#   else
#      define POLAR_DECL_EXPORT __declspec(dllexport)
#      define POLAR_DECL_EXPORT __declspec(dllimport)
#   endif
#elif defined(__GNUC__)
#   define POLAR_CC_GNU (__GNUC__ * 100 + __GNUC_MINOR__)
#   if defined(__MINGW32__)
#      define POLAR_CC_MINGW
#   endif
#   if defined(__INTEL_COMPILER)
/* Intel C++ also masquerades as GCC */
#      define POLAR_CC_INTEL (__INTEL_COMPILER)
#      ifdef __clang__
#         define POLAR_CC_CLANG 305
#      endif
#      define POLAR_ASSUME_IMPL(expr) __assume(expr)
#      define POLAR_UNREACHABLE_IMPL() __builtin_unreachable()
#      if __INTEL_COMPILER >= 1300 && !defined(__APPLE__)
#         define POLAR_DECL_DEPRECATED_X(text) __attribute__ ((__deprecated__(text)))
#      endif
#   elif defined(__clang__)
/* Clang also masquerades as GCC */
#      if defined(__appple_build_version__)
/* http://en.wikipedia.org/wiki/Xcode#Toolchain_Versions */
#         if __appple_build_version__ >= 7000053
#            define POLAR_CC_CLANG 306
#         elif __appple_build_version__ >= 6000051
#            define POLAR_CC_CLANG 305
#         elif __appple_build_version__ >= 5030038
#            define POLAR_CC_CLANG 304
#         elif __appple_build_version__ >= 5000275
#            define POLAR_CC_CLANG 303
#         elif __appple_build_version__ >= 4250024
#            define POLAR_CC_CLANG 302
#         elif __appple_build_version__ >= 3180045
#            define POLAR_CC_CLANG 301
#         elif __appple_build_version__ >= 2111001
#            define POLAR_CC_CLANG 300
#         else
#            error "Unknown Apple Clang version"
#         endif
#      else
#         define POLAR_CC_CLANG ((__clang_major__ * 100) + __clang_minor__)
#      endif
#      if __has_builtin(__builtin_assume)
#         define POLAR_ASSUME_IMPL(expr) __builtin_assume(expr)
#      else
#         define POLAR_ASSUME_IMPL(expr) if (expr){} else __builtin_unreachable()
#      endif
#      define POLAR_UNREACHABLE_IMPL() __builtin_unreachable()
#      if !defined(__has_extension)
#         /* Compatibility with older Clang versions */
#         define __has_extension __has_feature
#      endif
#      if defined(__APPLE__)
/* Apple/clang specific features */
#         define POLAR_DECL_CF_RETURNS_RETAINED __attribute__((cf_returns_retained))
#         ifdef __OBJC__
#            define POLAR_DECL_NS_RETURNS_AUTORELEASED __attribute__((ns_returns_autoreleased))
#         endif
#      endif
#   else
/* Plain GCC */
#      if POLAR_CC_GNU >= 405
#         define POLAR_ASSUME_IMPL(expr)  if (expr){} else __builtin_unreachable()
#         define POLAR_UNREACHABLE_IMPL() __builtin_unreachable()
#         define POLAR_DECL_DEPRECATED_X(text) __attribute__ ((__deprecated__(text)))
#      endif
#   endif
#   ifdef POLAR_OS_WIN
#      define POLAR_DECL_EXPORT __declspec(dllexport)
#      define POLAR_DECL_IMPORT __declspec(dllimport)
#   elif defined(POLAR_VISIBILITY_AVAILABLE)
#      define POLAR_DECL_EXPORT __attribute__((visibility("default")))
#      define POLAR_DECL_IMPORT __attribute__((visibility("default")))
#      define POLAR_DECL_HIDDEN __attribute__((visibility("hidden")))
#   endif
#   define POLAR_FUNC_INFO __PRETTY_FUNCTION__
#   define POLAR_ALIGNOF(type) __alignof__(type)
#   define POLAR_TYPEOF(expr) __typeof(expr)
#   define POLAR_DECL_DEPRECATED __attribute__ ((__deprecated__))
#   define POLAR_DECL_ALIGN(n) __attribute__((__aligned__(n)))
#   define POLAR_DECL_UNUSED __attribute__((__unused__))
#   define POLAR_LIKELY(expr) __builtin_expect(!!(expr), true)
#   define POLAR_UNLIKELY(expr)  __builtin_expect(!!(expr), false)
#   define POLAR_NORETURN __attribute__((__noreturn__))
#   define POLAR_REQUIRED_RESULT __attribute__ ((__warn_unused_result__))
#   define POLAR_DECL_PURE_FUNCTION __attribute__((pure))
#   define POLAR_DECL_CONST_FUNCTION __attribute__((const))
#   define POLAR_PACKED __attribute__ ((__packed__))
#   ifndef __ARM_EABI__
#      define POLAR_NO_ARM_EABI
#   endif
#   if POLAR_CC_GNU >= 403 && !defined(POLAR_CC_CLANG)
#      define POLAR_ALLOC_SIZE(x) __attribute__((alloc_size(x)))
#   endif
/* IBM compiler versions are a bit messy. There are actually two products:
   the C product, and the C++ product. The C++ compiler is always packaged
   with the latest version of the C compiler. Version numbers do not always
   match. This little table (I'm not sure it's accurate) should be helpful:
   C++ product                C product
   C Set 3.1                  C Compiler 3.0
   ...                        ...
   C++ Compiler 3.6.6         C Compiler 4.3
   ...                        ...
   Visual Age C++ 4.0         ...
   ...                        ...
   Visual Age C++ 5.0         C Compiler 5.0
   ...                        ...
   Visual Age C++ 6.0         C Compiler 6.0
   Now:
   __xlC__    is the version of the C compiler in hexadecimal notation
              is only an approximation of the C++ compiler version
   __IBMCPP__ is the version of the C++ compiler in decimal notation
              but it is not defined on older compilers like C Set 3.1 */
#elif defined(__xlC__)
#   define POLAR_CC_XLC
#   define POLAR_FULL_TEMPLATE_INSTANTIATION
#   if __xlC__ < 0x400
#      error "Compiler not supported"
#   elif __xlC__ >= 0x0600
#      define POLAR_ALGINOF(type) __alignof__(type)
#      define POLAR_TYPEOF __typeof__(expr)
#      define POLAR_DECL_ALIGN(n) __attribute__((__aligned__(n)))
#      define POLAR_PACKED __attribute__((__packed__))
#   endif

/* Older versions of DEC C++ do not define __EDG__ or __EDG - observed
   on DEC C++ V5.5-004. New versions do define  __EDG__ - observed on
   Compaq C++ V6.3-002.
   This compiler is different enough from other EDG compilers to handle
   it separately anyway. */
#elif defined(__DECCXX) || defined(__DECC)
#   define POLAR_CC_DEC
/* Compaq C++ V6 compilers are EDG-based but I'm not sure about older
   DEC C++ V5 compilers. */
#   if defined(__EDG__)
#      define POLAR_CC_EDG
#   endif
/* Compaq has disabled EDG's _BOOL macro and uses _BOOL_EXISTS instead
   - observed on Compaq C++ V6.3-002.
   In any case versions prior to Compaq C++ V6.0-005 do not have bool. */
#   if !defined(_BOOL_EXISTS)
#      error "Compiler not supported"
#   endif
/* Spurious (?) error messages observed on Compaq C++ V6.5-014. */
/* Apply to all versions prior to Compaq C++ V6.0-000 - observed on
   DEC C++ V5.5-004. */
#   if __DECCXX_VER < 60060000
#      define POLAR_BROKEN_TEMPLATE_SPECIALIZATION
#   endif
/* avoid undefined symbol problems with out-of-line template members */
#   define POLAR_OUTOFLINE_TEMPLATE inline

/* The Portland Group C++ compiler is based on EDG and does define __EDG__
   but the C compiler does not */
#elif defined(__PGI)
#   define POLAR_CC_PGI
#   if defined(__EDG__)
#      define POLAR_CC_EDG
#   endif

/* Compilers with EDG front end are similar. To detect them we test:
   __EDG documented by SGI, observed on MIPSpro 7.3.1.1 and KAI C++ 4.0b
   __EDG__ documented in EDG online docs, observed on Compaq C++ V6.3-002
   and PGI C++ 5.2-4 */
#elif !defined(POLAR_OS_HPUX) && (defined(__EDG) || defined(__EDG__))
#   define POLAR_CC_EDG
/* From the EDG documentation (does not seem to apply to Compaq C++ or GHS C):
   _BOOL
        Defined in C++ mode when bool is a keyword. The name of this
        predefined macro is specified by a configuration flag. _BOOL
        is the default.
   __BOOL_DEFINED
        Defined in Microsoft C++ mode when bool is a keyword. */
#   if !defined(_BOOL) && !defined(__BOOL_DEFINED) && !defined(__ghs)
#      error "Compiler not supported"
#   endif

/* The Comeau compiler is based on EDG and does define __EDG__ */
#   if defined(__COMO__)
#      define POLAR_CC_COMEAU
/* The `using' keyword was introduced to avoid KAI C++ warnings
   but it's now causing KAI C++ errors instead. The standard is
   unclear about the use of this keyword, and in practice every
   compiler is using its own set of rules. Forget it. */
#   elif defined(__KCC)
#      define POLAR_CC_KAI
/* Using the `using' keyword avoids Intel C++ for Linux warnings */
#   elif defined(__INTEL_COMPILER)
#      define POLAR_CC_INTEL (__INTEL_COMPILER)
/* Uses CFront, make sure to read the manual how to tweak templates. */
#   elif defined(__ghs)
#      define POLAR_CC_GHS
#      define POLAR_DECL_DEPRECATED __attribute__((__deprecated__))
#      define POLAR_FUNC_INFO  __PRETTY_FUNCTION__
#      define POLAR_TYPEOF(expr) __typeof__(expr)
#      define POLAR_ALIGNOF(type) __alignof__(type)
#      define POLAR_UNREACHABLE_IMPL()
#      if defined(__cplusplus)
#         define POLAR_COMPILER_AUTO_TYPE
#         define POLAR_COMPILER_STATIC_ASSERT
#         define POLAR_COMPILER_RANGE_FOR
#         if __GHS_VERSION_NUMBER >= 201505
#            define POLAR_COMPILER_ALIGNAS
#            define POLAR_COMPILER_ALIGNOF
#            define POLAR_COMPILER_ATOMICS
#            define POLAR_COMPILER_ATTRIBUTES
#            define POLAR_COMPILER_AUTO_FUNCTION
#            define POLAR_COMPILER_CLASS_ENUM
#            define POLAR_COMPILER_CONSTEXPR
#            define POLAR_COMPILER_DECLTYPE
#            define POLAR_COMPILER_DEFAULT_MEMBERS
#            define POLAR_COMPILER_DELETE_MEMBERS
#            define POLAR_COMPILER_DELEGATING_CONSTRUCTORS
#            define POLAR_COMPILER_EXPLICIT_CONVERSIONS
#            define POLAR_COMPILER_EXPLICIT_OVERRIDES
#            define POLAR_COMPILER_EXTERN_TEMPLATES
#            define POLAR_COMPILER_INHERITING_CONSTRUCTORS
#            define POLAR_COMPILER_INITIALIZER_LISTS
#            define POLAR_COMPILER_LAMBDA
#            define POLAR_COMPILER_NONSTATIC_MEMBER_INIT
#            define POLAR_COMPILER_NOEXCEPT
#            define POLAR_COMPILER_NULLPTR
#            define POLAR_COMPILER_RANGE_FOR
#            define POLAR_COMPILER_RAW_STRINGS
#            define POLAR_COMPILER_REF_QUALIFIERS
#            define POLAR_COMPILER_RVALUE_REFS
#            define POLAR_COMPILER_STATIC_ASSERT
#            define POLAR_COMPILER_TEMPLATE_ALIAS
#            define POLAR_COMPILER_THREAD_LOCAL
#            define POLAR_COMPILER_THREADSAFE_STATICS
#            define POLAR_COMPILER_UDL
#            define POLAR_COMPILER_UNICODE_STRINGS
#            define POLAR_COMPILER_UNIFORM_INIT
#            define POLAR_COMPILER_UNRESTRICTED_UNIONS
#            define POLAR_COMPILER_VARIADIC_MACROS
#            define POLAR_COMPILER_VARIADIC_TEMPLATES
#         endif
#      endif //__cplusplus
#   elseif defined(__DCC__)
#      define POLAR_CC_DIAB
#      if !defined(__bool)
#         error "Compiler not supported"
#      endif
/* The UnixWare 7 UDK compiler is based on EDG and does define __EDG__ */
#   elif defined(__USLC__) && defined(__SCO_VERSION__)
#      define POLAR_CC_USLC
/* The latest UDK 7.1.1b does not need this, but previous versions do */
#      if !defined(__SCO_VERSION__) || (__SCO_VERSION__ < 302200010)
#         define POLAR_OUTOFLINE_TEMPLATE inline
#      endif
/* Never tested! */
#   elif defined(CENTERLINE_CLPP) || defined(OBJECTCENTER)
#      define POLAR_CC_OC
/* CDS++ defines __EDG__ although this is not documented in the Reliant
   documentation. It also follows conventions like _BOOL and this documented */
#   elif defined(sinix)
#      define POLAR_CC_CDS
/* The MIPSpro compiler defines __EDG */
#   elif defined(__sgi)
#      define POLAR_CC_MIPS
#      define POLAR_NO_TEMPLATE_FRIENDS
#      if defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 740)
#         define POLAR_OUTOFLINE_TEMPLATE inline
#         pragma set woff 3624,3625,3649 /* turn off some harmless warnings */
#      endif
#   endif

/* VxWorks' DIAB toolchain has an additional EDG type C++ compiler
   (see __DCC__ above). This one is for C mode files (__EDG is not defined) */
#elif defined(_DIAB_TOOL)
#   define POLAR_CC_DIAB
#   define POLAR_FUNC_INFO __PRETTY_FUNCTION__

/* Never tested! */
#elif defined(__HIGHC__)
#   define POLAR_CC_HIGHC

#elif defined(__SUNPRO_CC) || defined(__SUNPRO_C)
#   define POLAR_CC_SUN
#   define POLAR_COMPILER_MANGLES_RETURN_TYPE
/* 5.0 compiler or better
    'bool' is enabled by default but can be disabled using -features=nobool
    in which case _BOOL is not defined
        this is the default in 4.2 compatibility mode triggered by -compat=4 */
#   if __SUNPRO_CC >= 0x500
#      define POLAR_NO_TEMPLATE_TEMPLATE_PARAMETERS
/* see http://developers.sun.com/sunstudio/support/Ccompare.html */
#      if __SUNPRO_CC >= 0x590
#         define POLAR_ALIGNOF(type) __alignof__(type)
#         define POLAR_TYPEOF(expr) __typeof__(expr)
#         define POLAR_DECL_ALIGN(n) __attribute__((__aligned__(n)))
#      endif
#      if __SUNPRO_CC >= 0x550
#         define POLAR_DECL_EXPORT __global
#      endif
#      if __SUNPRO_CC < 0x5a0
#         define POLAR_NO_TEMPLATE_FRIENDS
#      endif
#      if !defined(_BOOL)
#         error "Compiler not supported"
#      endif
/* 4.2 compiler or older */
#   else
#      error "Compiler not supported"
#   endif

/* CDS++ does not seem to define __EDG__ or __EDG according to Reliant
   documentation but nevertheless uses EDG conventions like _BOOL */
#elif defined(sinix)
#   define POLAR_CC_EDG
#   define POLAR_CC_CDS
#   if !defined(_BOOL)
#      error "Compiler not supported"
#   endif
#   define POLAR_BROKEN_TEMPLATE_SPECIALIZATION

#elif defined(POLAR_OS_HPUX)
/* __HP_aCC was not defined in first aCC releases */
#   if defined(__HP_aCC) || __cplusplus >= 199707L
#      define POLAR_NO_TEMPLATE_FRIENDS
#      define POLAR_CC_HPACC
#      define POLAR_FUNC_INFO __PRETTY_FUNCTION__
#      if __HP_aCC-0 < 060000
#         define POLAR_NO_TEMPLATE_TEMPLATE_PARAMETERS
#         define POLAR_DECL_EXPORT __declspec(dllexport)
#         define POLAR_DECL_IMPORT __declspec(dllimport)
#      endif
#      if __HP_aCC-0 >= 061200
#         define POLAR_DECL_ALIGN(n) __attribute__((aligned(n)))
#      endif
#      if __HP_aCC-0 >= 062000
#         define POLAR_DECL_EXPORT __attribute__((visibility("default")))
#         define POLAR_DECL_HIDDEN __attribute__((visibility("hidden")))
#         define POLAR_DECL_IMPORT POLAR_DECL_EXPORT
#      endif
#   else
#      error "Compiler not supported"
#   endif
#else
#   error "libPOLAR has not been tested with this compiler - see http://www.libPOLAR.org/"
#endif

/*
 * C++11 support
 *
 *  Paper           Macro                               SD-6 macro
 *  N2341           POLAR_COMPILER_ALIGNAS
 *  N2341           POLAR_COMPILER_ALIGNOF
 *  N2427           POLAR_COMPILER_ATOMICS
 *  N2761           POLAR_COMPILER_ATTRIBUTES               __cpp_attributes = 200809
 *  N2541           POLAR_COMPILER_AUTO_FUNCTION
 *  N1984 N2546     POLAR_COMPILER_AUTO_TYPE
 *  N2437           POLAR_COMPILER_CLASS_ENUM
 *  N2235           POLAR_COMPILER_CONSTEXPR                __cpp_constexpr = 200704
 *  N2343 N3276     POLAR_COMPILER_DECLTYPE                 __cpp_decltype = 200707
 *  N2346           POLAR_COMPILER_DEFAULT_MEMBERS
 *  N2346           POLAR_COMPILER_DELETE_MEMBERS
 *  N1986           POLAR_COMPILER_DELEGATING_CONSTRUCTORS
 *  N2437           POLAR_COMPILER_EXPLICIT_CONVERSIONS
 *  N3206 N3272     POLAR_COMPILER_EXPLICIT_OVERRIDES
 *  N1987           POLAR_COMPILER_EXTERN_TEMPLATES
 *  N2540           POLAR_COMPILER_INHERITING_CONSTRUCTORS
 *  N2672           POLAR_COMPILER_INITIALIZER_LISTS
 *  N2658 N2927     POLAR_COMPILER_LAMBDA                   __cpp_lambdas = 200907
 *  N2756           POLAR_COMPILER_NONSTATIC_MEMBER_INIT
 *  N2855 N3050     POLAR_COMPILER_NOEXCEPT
 *  N2431           POLAR_COMPILER_NULLPTR
 *  N2930           POLAR_COMPILER_RANGE_FOR
 *  N2442           POLAR_COMPILER_RAW_STRINGS              __cpp_raw_strings = 200710
 *  N2439           POLAR_COMPILER_REF_QUALIFIERS
 *  N2118 N2844 N3053 POLAR_COMPILER_RVALUE_REFS            __cpp_rvalue_references = 200610
 *  N1720           POLAR_COMPILER_STATIC_ASSERT            __cpp_static_assert = 200410
 *  N2258           POLAR_COMPILER_TEMPLATE_ALIAS
 *  N2659           POLAR_COMPILER_THREAD_LOCAL
 *  N2660           POLAR_COMPILER_THREADSAFE_STATICS
 *  N2765           POLAR_COMPILER_UDL                      __cpp_user_defined_literals = 200809
 *  N2442           POLAR_COMPILER_UNICODE_STRINGS          __cpp_unicode_literals = 200710
 *  N2640           POLAR_COMPILER_UNIFORM_INIT
 *  N2544           POLAR_COMPILER_UNRESTRICTED_UNIONS
 *  N1653           POLAR_COMPILER_VARIADIC_MACROS
 *  N2242 N2555     POLAR_COMPILER_VARIADIC_TEMPLATES       __cpp_variadic_templates = 200704
 *
 * For any future version of the C++ standard, we use only the SD-6 macro.
 * For full listing, see
 *  http://isocpp.org/std/standing-documents/sd-6-sg10-feature-test-recommendations
 *
 * C++ extensions:
 *  POLAR_COMPILER_RESTRICTED_VLA       variable-length arrays, prior to __cpp_runtime_arrays
 */

#ifdef __cplusplus
#   if __cplusplus < 201103L && !(defined(POLAR_CC_MSVC) && POLAR_CC_MSVC >= 1800)
#      error libPOLAR requires a C++11 compiler and yours does not seem to be that.
#   endif
#endif

#ifdef POLAR_CC_INTEL
#   define POLAR_COMPILER_RESTRICTED_VLA
#   define POLAR_COMPILER_VARIADIC_MACROS  // C++11 feature supported as an extension in other modes, too
#   define POLAR_COMPILER_THREADSAFE_STATICS
#   if __INTEL_COMPILER < 1200
#      define POLAR_NO_TEMPLATE_FRIENDS
#   endif
#   if __INTEL_COMPILER >= 1310 && !defined(_WIN32)
//    ICC supports C++14 binary literals in C, C++98, and C++11 modes
//    at least since 13.1, but I can't test further back
#      define POLAR_COMPILER_BINARY_LITERALS
#   endif
#   if __cplusplus >= 201103L || defined(__INTEL_CXX11_MODE__)
#      if __INTEL_COMPILER >= 1200
#         define POLAR_COMPILER_AUTO_TYPE
#         define POLAR_COMPILER_CLASS_ENUM
#         define POLAR_COMPILER_DECLTYPE
#         define POLAR_COMPILER_DEFAULT_MEMBERS
#         define POLAR_COMPILER_DELETE_MEMBERS
#         define POLAR_COMPILER_EXTERN_TEMPLATES
#         define POLAR_COMPILER_LAMBDA
#         define POLAR_COMPILER_RVALUE_REFS
#         define POLAR_COMPILER_STATIC_ASSERT
#         define POLAR_COMPILER_VARIADIC_MACROS
#      endif
#      if __INTEL_COMPILER >= 1210
#         define POLAR_COMPILER_ATTRIBUTES
#         define POLAR_COMPILER_AUTO_FUNCTION
#         define POLAR_COMPILER_NULLPTR
#         define POLAR_COMPILER_TEMPLATE_ALIAS
#         ifndef _CHAR16T      // MSVC headers
#            define POLAR_COMPILER_UNICODE_STRINGS
#         endif
#         define POLAR_COMPILER_VARIADIC_TEMPLATES
#      endif
#      if __INTEL_COMPILER >= 1300
#         define POLAR_COMPILER_ATOMICS
//        constexpr support is only partial
//#       define POLAR_COMPILER_CONSTEXPR
#         define POLAR_COMPILER_INITIALIZER_LISTS
#         define POLAR_COMPILER_UNIFORM_INIT
#         define POLAR_COMPILER_NOEXCEPT
#      endif
#      if __INTEL_COMPILER >= 1400
//       causes issues with QArrayData and POLAR_Private::RefCount - Intel issue ID 6000056211, bug DPD200534796
//#       define POLAR_COMPILER_CONSTEXPR
#         define POLAR_COMPILER_DELEGATING_CONSTRUCTORS
#         define POLAR_COMPILER_EXPLICIT_CONVERSIONS
#         define POLAR_COMPILER_EXPLICIT_OVERRIDES
#         define POLAR_COMPILER_NONSTATIC_MEMBER_INIT
#         define POLAR_COMPILER_RANGE_FOR
#         define POLAR_COMPILER_RAW_STRINGS
#         define POLAR_COMPILER_REF_QUALIFIERS
#         define POLAR_COMPILER_UNICODE_STRINGS
#         define POLAR_COMPILER_UNRESTRICTED_UNIONS
#      endif
#      if __INTEL_COMPILER >= 1500
#         if __INTEL_COMPILER * 100 + __INTEL_COMPILER_UPDATE >= 150001
//        the bug mentioned above is fixed in 15.0.1
#            define POLAR_COMPILER_CONSTEXPR
#         endif
#         define POLAR_COMPILER_ALIGNAS
#         define POLAR_COMPILER_ALIGNOF
#         define POLAR_COMPILER_INHERITING_CONSTRUCTORS
#         ifndef POLAR_OS_OSX
//        C++11 thread_local is broken on OS X (Clang doesn't support it either)
#            define POLAR_COMPILER_THREAD_LOCAL
#         endif
#         define POLAR_COMPILER_UDL
#      endif
#      ifdef _MSC_VER
#         if _MSC_VER == 1700
//       <initializer_list> is missing with MSVC 2012 (it's present in 2010, 2013 and up)
#            undef POLAR_COMPILER_INITIALIZER_LISTS
#         endif
#         if _MSC_VER < 1900
//        ICC disables unicode string support when compatibility mode with MSVC 2013 or lower is active
#            undef POLAR_COMPILER_UNICODE_STRINGS
//        Even though ICC knows about ref-qualified members, MSVC 2013 or lower doesn't, so
//        certain member functions (like QString::toUpper) may be missing from the DLLs.
#            undef POLAR_COMPILER_REF_QUALIFIERS
//       Disable constexpr unless the MS headers have constexpr in all the right places too
//       (like std::numeric_limits<T>::max())
#            undef POLAR_COMPILER_CONSTEXPR
#         endif
#      endif
#   endif
#endif

#if defined(POLAR_CC_CLANG) && !defined(POLAR_CC_INTEL)
/* General C++ features */
#   define POLAR_COMPILER_RESTRICTED_VLA
#   define POLAR_COMPILER_THREADSAFE_STATICS
#   if __has_feature(attribute_deprecated_with_message)
#      define POLAR_DECL_DEPRECATED_X(text) __attribute__ ((__deprecated__(text)))
#   endif

// Clang supports binary literals in C, C++98 and C++11 modes
// It's been supported "since the dawn of time itself" (cf. commit 179883)
#   if __has_extension(cxx_binary_literals)
#      define POLAR_COMPILER_BINARY_LITERALS
#   endif

// Variadic macros are supported for gnu++98, c++11, c99 ... since 2.9
#   if POLAR_CC_CLANG >= 209
#      if !defined(__STRICT_ANSI__) || defined(__GXX_EXPERIMENTAL_CXX0X__) \
   || (defined(__cplusplus) && (__cplusplus >= 201103L)) \
   || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
#         define POLAR_COMPILER_VARIADIC_MACROS
#      endif
#   endif

/* C++11 features, see http://clang.llvm.org/cxx_status.html */
#   if __cplusplus >= 201103L  || defined(__GXX_EXPERIMENTAL_CXX0X__)
/* Detect C++ features using __has_feature(), see http://clang.llvm.org/docs/LanguageExtensions.html#cxx11 */
#      if __has_feature(cxx_alignas)
#         define POLAR_COMPILER_ALIGNAS
#         define POLAR_COMPILER_ALIGNOF
#      endif
#      if __has_feature(cxx_atomic) && __has_include(<atomic>)
#         define POLAR_COMPILER_ATOMICS
#      endif
#      if __has_feature(cxx_attributes)
#         define POLAR_COMPILER_ATTRIBUTES
#      endif
#      if __has_feature(cxx_auto_type)
#         define POLAR_COMPILER_AUTO_FUNCTION
#         define POLAR_COMPILER_AUTO_TYPE
#      endif
#      if __has_feature(cxx_strong_enums)
#         define POLAR_COMPILER_CLASS_ENUM
#      endif
#      if __has_feature(cxx_constexpr) && POLAR_CC_CLANG > 302 /* CLANG 3.2 has bad/partial support */
#         define POLAR_COMPILER_CONSTEXPR
#      endif
#      if __has_feature(cxx_decltype) /* && __has_feature(cxx_decltype_incomplete_return_types) */
#         define POLAR_COMPILER_DECLTYPE
#      endif
#      if __has_feature(cxx_defaulted_functions)
#         define POLAR_COMPILER_DEFAULT_MEMBERS
#      endif
#      if __has_feature(cxx_deleted_functions)
#         define POLAR_COMPILER_DELETE_MEMBERS
#      endif
#      if __has_feature(cxx_delegating_constructors)
#         define POLAR_COMPILER_DELEGATING_CONSTRUCTORS
#      endif
#      if __has_feature(cxx_explicit_conversions)
#         define POLAR_COMPILER_EXPLICIT_CONVERSIONS
#      endif
#      if __has_feature(cxx_override_control)
#         define POLAR_COMPILER_EXPLICIT_OVERRIDES
#      endif
#      if __has_feature(cxx_inheriting_constructors)
#         define POLAR_COMPILER_INHERITING_CONSTRUCTORS
#      endif
#      if __has_feature(cxx_generalized_initializers)
#         define POLAR_COMPILER_INITIALIZER_LISTS
#         define POLAR_COMPILER_UNIFORM_INIT /* both covered by this feature macro, according to docs */
#      endif
#      if __has_feature(cxx_lambdas)
#         define POLAR_COMPILER_LAMBDA
#      endif
#      if __has_feature(cxx_noexcept)
#         define POLAR_COMPILER_NOEXCEPT
#      endif
#      if __has_feature(cxx_nonstatic_member_init)
#         define POLAR_COMPILER_NONSTATIC_MEMBER_INIT
#      endif
#      if __has_feature(cxx_nullptr)
#         define POLAR_COMPILER_NULLPTR
#      endif
#      if __has_feature(cxx_range_for)
#         define POLAR_COMPILER_RANGE_FOR
#      endif
#      if __has_feature(cxx_raw_string_literals)
#         define POLAR_COMPILER_RAW_STRINGS
#      endif
#      if __has_feature(cxx_reference_qualified_functions)
#         define POLAR_COMPILER_REF_QUALIFIERS
#      endif
#      if __has_feature(cxx_rvalue_references)
#         define POLAR_COMPILER_RVALUE_REFS
#      endif
#      if __has_feature(cxx_static_assert)
#         define POLAR_COMPILER_STATIC_ASSERT
#      endif
#      if __has_feature(cxx_alias_templates)
#         define POLAR_COMPILER_TEMPLATE_ALIAS
#      endif
#      if __has_feature(cxx_thread_local)
#         if !defined(__FreeBSD__) /* FreeBSD clang fails on __cxa_thread_atexit */
#            define POLAR_COMPILER_THREAD_LOCAL
#         endif
#      endif
#      if __has_feature(cxx_user_literals)
#         define POLAR_COMPILER_UDL
#      endif
#      if __has_feature(cxx_unicode_literals)
#         define POLAR_COMPILER_UNICODE_STRINGS
#      endif
#      if __has_feature(cxx_unrestricted_unions)
#         define POLAR_COMPILER_UNRESTRICTED_UNIONS
#      endif
#      if __has_feature(cxx_variadic_templates)
#         define POLAR_COMPILER_VARIADIC_TEMPLATES
#      endif
/* Features that have no __has_feature() check */
#      if POLAR_CC_CLANG >= 209 /* since clang 2.9 */
#         define POLAR_COMPILER_EXTERN_TEMPLATES
#      endif
#   endif
/* C++1y features, deprecated macros. Do not update this list. */
#   if __cplusplus > 201103L
#      if __has_feature(cxx_generic_lambda)
#         define POLAR_COMPILER_GENERIC_LAMBDA
#      endif
#      if __has_feature(cxx_init_capture)
#         define POLAR_COMPILER_LAMBDA_CAPTURES
#      endif
#      if __has_feature(cxx_relaxed_constexpr)
#         define POLAR_COMPILER_RELAXED_CONSTEXPR_FUNCTIONS
#      endif
#      if __has_feature(cxx_decltype_auto) && __has_feature(cxx_return_type_deduction)
#         define POLAR_COMPILER_RETURN_TYPE_DEDUCTION
#      endif
#      if __has_feature(cxx_variable_templates)
#         define POLAR_COMPILER_VARIABLE_TEMPLATES
#      endif
#      if __has_feature(cxx_runtime_array)
#         define POLAR_COMPILER_VLA
#      endif
#   endif

#   if defined(__has_warning)
#      if __has_warning("-Wunused-private-field")
#         define POLAR_DECL_UNUSED_MEMBER POLAR_DECL_UNUSED
#      endif
#   endif
#endif // POLAR_CC_CLANG

#if defined(POLAR_CC_GNU) && !defined(POLAR_CC_INTEL) && !defined(POLAR_CC_CLANG)
#   define POLAR_COMPILER_RESTRICTED_VLA
#   define POLAR_COMPILER_THREADSAFE_STATICS
#   if POLAR_CC_GNU >= 403
//   GCC supports binary literals in C, C++98 and C++11 modes
#      define POLAR_COMPILER_BINARY_LITERALS
#   endif
#   if !defined(__STRICT_ANSI__) || defined(__GXX_EXPERIMENTAL_CXX0X__) \
   || (defined(__cplusplus) && (__cplusplus >= 201103L)) \
   || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
// Variadic macros are supported for gnu++98, c++11, C99 ... since forever (gcc 2.97)
#      define POLAR_COMPILER_VARIADIC_MACROS
#   endif
#   if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#      if POLAR_CC_GNU >= 403
/* C++11 features supported in GCC 4.3: */
#         define POLAR_COMPILER_DECLTYPE
#         define POLAR_COMPILER_RVALUE_REFS
#         define POLAR_COMPILER_STATIC_ASSERT
#      endif
#      if POLAR_CC_GNU >= 404
#         /* C++11 features supported in GCC 4.4: */
#         define POLAR_COMPILER_AUTO_FUNCTION
#         define POLAR_COMPILER_AUTO_TYPE
#         define POLAR_COMPILER_EXTERN_TEMPLATES
#         define POLAR_COMPILER_UNIFORM_INIT
#         define POLAR_COMPILER_UNICODE_STRINGS
#         define POLAR_COMPILER_VARIADIC_TEMPLATES
#      endif
#      if POLAR_CC_GNU >= 405
#         /* C++11 features supported in GCC 4.5: */
#         define POLAR_COMPILER_EXPLICIT_CONVERSIONS
/* GCC 4.4 implements initializer_list but does not define typedefs required by the standard. */
#         define POLAR_COMPILER_INITIALIZER_LISTS
#         define POLAR_COMPILER_LAMBDA
#         define POLAR_COMPILER_RAW_STRINGS
#         define POLAR_COMPILER_CLASS_ENUM
#      endif
#      if POLAR_CC_GNU >= 406
/* Pre-4.6 compilers implement a non-final snapshot of N2346, hence default and delete
        * functions are supported only if they are public. Starting from 4.6, GCC handles
        * final version - the access modifier is not relevant. */
#         define POLAR_COMPILER_DEFAULT_MEMBERS
#         define POLAR_COMPILER_DELETE_MEMBERS
/* C++11 features supported in GCC 4.6: */
#         define POLAR_COMPILER_CONSTEXPR
#         define POLAR_COMPILER_NULLPTR
#         define POLAR_COMPILER_UNRESTRICTED_UNIONS
#         define POLAR_COMPILER_RANGE_FOR
#      endif
#      if POLAR_CC_GNU >= 407
/* GCC 4.4 implemented <atomic> and std::atomic using its old intrinsics.
 * However, the implementation is incomplete for most platforms until GCC 4.7:
 * instead, std::atomic would use an external lock. Since we need an std::atomic
 * that is behavior-compatible with QBasicAtomic, we only enable it here */
#         define POLAR_COMPILER_ATOMICS
/* GCC 4.6.x has problems dealing with noexcept expressions,
        * so turn the feature on for 4.7 and above, only */
#         define POLAR_COMPILER_NOEXCEPT
/* C++11 features supported in GCC 4.7: */
#         define POLAR_COMPILER_NONSTATIC_MEMBER_INIT
#         define POLAR_COMPILER_DELEGATING_CONSTRUCTORS
#         define POLAR_COMPILER_EXPLICIT_OVERRIDES
#         define POLAR_COMPILER_TEMPLATE_ALIAS
#         define POLAR_COMPILER_UDL
#      endif
#      if POLAR_CC_GNU >= 408
#         define POLAR_COMPILER_ATTRIBUTES
#         define POLAR_COMPILER_ALIGNAS
#         define POLAR_COMPILER_ALIGNOF
#         define POLAR_COMPILER_INHERITING_CONSTRUCTORS
#         define POLAR_COMPILER_THREAD_LOCAL
#         if POLAR_CC_GNU > 408 || __GNUC_PATCHLEVEL__ >= 1
#            define POLAR_COMPILER_REF_QUALIFIERS
#         endif
#      endif
/* C++11 features are complete as of GCC 4.8.1 */
#   endif
#   if __cplusplus > 201103L
#      if POLAR_CC_GNU >= 409
#         define POLAR_COMPILER_LAMBDA_CAPTURES
#         define POLAR_COMPILER_RETURN_TYPE_DEDUCTION
#      endif
#   endif
#endif

#if defined(POLAR_CC_MSVC) && !defined(POLAR_CC_INTEL)
#   if defined(__cplusplus)
#      if _MSC_VER >= 1400
/* C++11 features supported in VC8 = VC2005: */
#         define POLAR_COMPILER_VARIADIC_MACROS
#         ifndef __cplusplus_cli
/* 2005 supports the override and final contextual keywords, in
           the same positions as the C++11 variants, but 'final' is
           called 'sealed' instead:
           http://msdn.microsoft.com/en-us/library/0w2w91tf%28v=vs.80%29.aspx
           The behavior is slightly different in C++/CLI, which requires the
           "virtual" keyword to be present too, so don't define for that.
           So don't define POLAR_COMPILER_EXPLICIT_OVERRIDES (since it's not
           the same as the C++11 version), but define the POLAR_DECL_* flags
           accordingly: */
#            define POLAR_DECL_OVERRIDE override
#            define POLAR_DECL_FINAL sealed
#         endif
#      endif
#      if _MSC_VER >= 1600
/* C++11 features supported in VC10 = VC2010: */
#         define POLAR_COMPILER_AUTO_FUNCTION
#         define POLAR_COMPILER_AUTO_TYPE
#         define POLAR_COMPILER_DECLTYPE
#         define POLAR_COMPILER_EXTERN_TEMPLATES
#         define POLAR_COMPILER_LAMBDA
#         define POLAR_COMPILER_NULLPTR
#         define POLAR_COMPILER_RVALUE_REFS
#         define POLAR_COMPILER_STATIC_ASSERT
//  MSVC's library has std::initializer_list, but the compiler does not support the braces initialization
//#       define POLAR_COMPILER_INITIALIZER_LISTS
//#       define POLAR_COMPILER_UNIFORM_INIT
#      endif
#      if _MSC_VER >= 1700
/* C++11 features supported in VC11 = VC2012: */
#         undef POLAR_DECL_OVERRIDE               /* undo 2005/2008 settings... */
#         undef POLAR_DECL_FINAL                  /* undo 2005/2008 settings... */
#         define POLAR_COMPILER_EXPLICIT_OVERRIDES /* ...and use std C++11 now   */
#         define POLAR_COMPILER_CLASS_ENUM
#         define POLAR_COMPILER_ATOMICS
#      endif /* VC 11 */
#      if _MSC_VER >= 1800
/* C++11 features in VC12 = VC2013 */
/* Implemented, but can't be used on move special members */
/* #      define POLAR_COMPILER_DEFAULT_MEMBERS */
#         define POLAR_COMPILER_DELETE_MEMBERS
#         define POLAR_COMPILER_DELEGATING_CONSTRUCTORS
#         define POLAR_COMPILER_EXPLICIT_CONVERSIONS
#         define POLAR_COMPILER_NONSTATIC_MEMBER_INIT
// implemented, but nested initialization fails (eg tst_qvector): http://connect.microsoft.com/VisualStudio/feedback/details/800364/initializer-list-calls-object-destructor-twice
//       #define POLAR_COMPILER_INITIALIZER_LISTS
// implemented in principle, but has a bug that makes it unusable: http://connect.microsoft.com/VisualStudio/feedback/details/802058/c-11-unified-initialization-fails-with-c-style-arrays
//       #define POLAR_COMPILER_UNIFORM_INIT
#         define POLAR_COMPILER_RAW_STRINGS
#         define POLAR_COMPILER_TEMPLATE_ALIAS
#         define POLAR_COMPILER_VARIADIC_TEMPLATES
#      endif /* VC 12 */
#      if _MSC_FULL_VER >= 180030324 // VC 12 SP 2 RC
#         define POLAR_COMPILER_INITIALIZER_LISTS
#      endif /* VC 12 SP 2 RC */
#      if _MSC_VER >= 1900
/* C++11 features in VC14 = VC2015 */
#         define POLAR_COMPILER_DEFAULT_MEMBERS
#         define POLAR_COMPILER_ALIGNAS
#         define POLAR_COMPILER_ALIGNOF
// Partial support, insufficient for POLAR_
//#       define POLAR_COMPILER_CONSTEXPR
#         define POLAR_COMPILER_INHERITING_CONSTRUCTORS
#         define POLAR_COMPILER_NOEXCEPT
#         define POLAR_COMPILER_RANGE_FOR
#         define POLAR_COMPILER_REF_QUALIFIERS
#         define POLAR_COMPILER_THREAD_LOCAL
// Broken, see POLAR_BUG-47224 and https://connect.microsoft.com/VisualStudio/feedback/details/1549785
//#       define POLAR_COMPILER_THREADSAFE_STATICS
#         define POLAR_COMPILER_UDL
#         define POLAR_COMPILER_UNICODE_STRINGS
// Uniform initialization is not working yet -- build errors with QUuid
//#       define POLAR_COMPILER_UNIFORM_INIT
#         define POLAR_COMPILER_UNRESTRICTED_UNIONS
#      endif
#      if _MSC_FULL_VER >= 190023419
#         define POLAR_COMPILER_ATTRIBUTES
// Almost working, see https://connect.microsoft.com/VisualStudio/feedback/details/2011648
//#       define POLAR_COMPILER_CONSTEXPR
#         define POLAR_COMPILER_THREADSAFE_STATICS
#         define POLAR_COMPILER_UNIFORM_INIT
#      endif
#      if _MSC_VER >= 1910
#         define POLAR_COMPILER_CONSTEXPR
#      endif
#   endif /* __cplusplus */
#endif  /* POLAR_CC_MSVC */

#ifdef __cplusplus
#   include <utility>
// By default, QNX 7.0 uses libc++ (from LLVM) and
// QNX 6.X uses Dinkumware's libcpp. In all versions,
// it is also possible to use GNU libstdc++.

// For Dinkumware, some features must be disabled
// (mostly because of library problems).
// Dinkumware is assumed when __GLIBCXX__ (GNU libstdc++)
// and _LIBCPP_VERSION (LLVM libc++) are both absent.
#   if defined(POLAR_OS_QNX)
// By default, QNX 7.0 uses libc++ (from LLVM) and
// QNX 6.X uses Dinkumware's libcpp. In all versions,
// it is also possible to use GNU libstdc++.

// For Dinkumware, some features must be disabled
// (mostly because of library problems).
// Dinkumware is assumed when __GLIBCXX__ (GNU libstdc++)
// and _LIBCPP_VERSION (LLVM libc++) are both absent.
#      if !defined(__GLIBCXX__) && !defined(_LIBCPP_VERSION)
// Older versions of libcpp (QNX 650) do not support C++11 features
// _HAS_* macros are set to 1 by toolchains that actually include
// Dinkum C++11 libcpp.
#         if !defined(_HAS_CPP0X) || !_HAS_CPP0X
#            undef POLAR_COMPILER_INITIALIZER_LISTS
#            undef POLAR_COMPILER_RVALUE_REFS
#            undef POLAR_COMPILER_REF_QUALIFIERS
#            undef POLAR_COMPILER_UNICODE_STRINGS
#            undef POLAR_COMPILER_NOEXCEPT
#         endif // !_HAS_CPP0X
#         if !defined(_HAS_NULLPTR_T) || !_HAS_NULLPTR_T
#            undef POLAR_COMPILER_NULLPTR
#         endif
#         if !defined(_HAS_CONSTEXPR) || !_HAS_CONSTEXPR
// The libcpp is missing constexpr keywords on important functions like std::numeric_limits<>::min()
// Disable constexpr support on QNX even if the compiler supports it
#            undef POLAR_COMPILER_CONSTEXPR
#         endif // !_HAS_CONSTEXPR
#      endif // !__GLIBCXX__ && !_LIBCPP_VERSION
#   endif // POLAR_OS_QNX
#   if (defined(POLAR_CC_CLANG) || defined(POLAR_CC_INTEL)) && defined(POLAR_OS_MAC) && defined(__GNUC_LIBSTD__) \
   && ((__GNUC_LIBSTD__-0) * 100 + __GNUC_LIBSTD_MINOR__-0 <= 402)
// Apple has not updated libstdc++ since 2007, which means it does not have
// <initializer_list> or std::move. Let's disable these features
#      undef POLAR_COMPILER_INITIALIZER_LISTS
#      undef POLAR_COMPILER_RVALUE_REFS
#      undef POLAR_COMPILER_REF_QUALIFIERS
// Also disable <atomic>, since it's clearly not there
#      undef POLAR_COMPILER_ATOMICS
#   endif
#   if defined(POLAR_CC_CLANG) && defined(POLAR_CC_INTEL) && POLAR_CC_INTEL >= 1500
// ICC 15.x and 16.0 have their own implementation of std::atomic, which is activated when in Clang mode
// (probably because libc++'s <atomic> on OS X failed to compile), but they're missing some
// critical definitions. (Reported as Intel Issue ID 6000117277)
#      define __USE_CONSTEXPR 1
#      define __USE_NOEXCEPT 1
#   endif
#   if defined(POLAR_CC_MSVC) && defined(POLAR_CC_CLANG)
// Clang and the Intel compiler support more C++ features than the Microsoft compiler
// so make sure we don't enable them if the MS headers aren't properly adapted.
#      ifndef _HAS_CONSTEXPR
#         undef POLAR_COMPILER_CONSTEXPR
#      endif
#      ifndef _HAS_DECLTYPE
#         undef POLAR_COMPILER_DECLTYPE
#      endif
#      ifndef _HAS_INITIALIZER_LISTS
#         undef POLAR_COMPILER_INITIALIZER_LISTS
#      endif
#      ifndef _HAS_NULLPTR_T
#         undef POLAR_COMPILER_NULLPTR
#      endif
#      ifndef _HAS_RVALUE_REFERENCES
#         undef POLAR_COMPILER_RVALUE_REFS
#      endif
#      ifndef _HAS_SCOPED_ENUM
#         undef POLAR_COMPILER_CLASS_ENUM
#      endif
#      ifndef _HAS_TEMPLATE_ALIAS
#         undef POLAR_COMPILER_TEMPLATE_ALIAS
#      endif
#      ifndef _HAS_VARIADIC_TEMPLATES
#         undef POLAR_COMPILER_VARIADIC_TEMPLATES
#      endif
#   endif
#   if defined(POLAR_COMPILER_THREADSAFE_STATICS) && defined(POLAR_OS_MAC)
// Apple's low-level implementation of the C++ support library
// (libc++abi.dylib, shared between libstdc++ and libc++) has deadlocks. The
// C++11 standard requires the deadlocks to be removed, so this will eventually
// be fixed; for now, let's disable this.
#      undef POLAR_COMPILER_THREADSAFE_STATICS
#   endif
#endif

/*
 * C++11 keywords and expressions
 */
#ifdef POLAR_COMPILER_NULLPTR
#   define POLAR_NULLPTR nullptr
#else
#   define POLAR_NULLPTR NULL
#endif

#ifdef POLAR_COMPILER_DEFAULT_MEMBERS
#   define POLAR_DECL_EPOLAR_DEFAULT = default
#else
#   define POLAR_DECL_EPOLAR_DEFAULT
#endif

#ifdef POLAR_COMPILER_DELETE_MEMBERS
#   define POLAR_DECL_EPOLAR_DELETE = delete
#else
#   define POLAR_DECL_EPOLAR_DELETE
#endif

// Don't break code that is already using POLAR_COMPILER_DEFAULT_DELETE_MEMBERS
#if defined(POLAR_COMPILER_DEFAULT_MEMBERS) && defined(POLAR_COMPILER_DELETE_MEMBERS)
#   define POLAR_COMPILER_DEFAULT_DELETE_MEMBERS
#endif

#if defined(POLAR_COMPILER_CONSTEXPR)
#   if defined(__cpp_constexpr) && __cpp_constexpr-0 >= 201304
#      define POLAR_DECL_CONSTEXPR constexpr
#      define POLAR_DECL_RELAXED_CONSTEXPR constexpr
#      define POLAR_CONSTEXPR constexpr
#      define POLAR_RELAXED_CONSTEXPR constexpr
#   else
#      define POLAR_DECL_CONSTEXPR constexpr
#      define POLAR_DECL_RELAXED_CONSTEXPR
#      define POLAR_CONSTEXPR constexpr
#      define POLAR_RELAXED_CONSTEXPR const
#   endif
#else
#   define POLAR_DECL_CONSTEXPR
#   define POLAR_DECL_RELAXED_CONSTEXPR
#   define POLAR_CONSTEXPR const
#   define POLAR_RELAXED_CONSTEXPR const
#endif

#ifdef POLAR_COMPILER_EXPLICIT_OVERRIDES
#   define POLAR_DECL_OVERRIDE override
#   define POLAR_DECL_FINAL final
#else
#   ifndef POLAR_DECL_OVERRIDE
#      define POLAR_DECL_OVERRIDE
#   endif
#   ifndef POLAR_DECL_FINAL
#      define POLAR_DECL_FINAL
#   endif
#endif

#ifdef POLAR_COMPILER_NOEXCEPT
#   define POLAR_DECL_NOEXCEPT noexcept
#   define POLAR_DECL_NOEXCEPT_EXPR(x) noexcept(x)
#   ifdef POLAR_DECL_NOTHROW
#      undef POLAR_DECL_NOTHROW /* override with C++11 noexcept if available */
#   endif
#else
#   define POLAR_DECL_NOEXCEPT
#   define POLAR_DECL_NOEXCEPT_EXPR(x)
#endif

#ifndef POLAR_DECL_NOTHROW
#   define POLAR_DECL_NOTHROW POLAR_DECL_NOEXCEPT
#endif

#if defined(POLAR_COMPILER_ALIGNOF)
#   undef POLAR_ALIGNOF
#   define POLAR_ALIGNOF(x) alignof(x)
#endif

#if defined(POLAR_COMPILER_ALIGNAS)
#   undef POLAR_DECL_ALIGN
#   define POLAR_DECL_ALIGN(n) alignas(n)
#endif

/*
 * Fallback macros to certain compiler features
 */
#ifndef POLAR_NORETURN
#   define POLAR_NORETURN
#endif

#ifndef POLAR_LIKELY
#   define POLAR_LIKELY(x) (x)
#endif

#ifndef  POLAR_UNLIKELY
#   define POLAR_UNLIKELY(x) (x)
#endif

#ifndef POLAR_ASSUME_IMPL
#   define POLAR_ASSUME_IMPL(expr) POLAR_noop()
#endif

#ifndef POLAR_UNREACHABLE_IMPL
#   define POLAR_UNREACHABLE_IMPL() POLAR_noop()
#endif

#ifndef POLAR_ALLOC_SIZE
#   define POLAR_ALLOC_SIZE(x)
#endif

#ifndef POLAR_REQUIRED_RESULT
#   define POLAR_REQUIRED_RESULT
#endif

#ifndef POLAR_DECL_DEPRECATED
#   define POLAR_DECL_DEPRECATED
#endif

#ifndef POLAR_DECL_VARIABLE_DEPRECATED
#   define POLAR_DECL_VARIABLE_DEPRECATED POLAR_DECL_DEPRECATED
#endif

#ifndef POLAR_DECL_DEPRECATED_X
#   define POLAR_DECL_DEPRECATED_X(text) POLAR_DECL_DEPRECATED
#endif

#ifndef POLAR_DECL_EXPORT
#   define POLAR_DECL_EXPORT
#endif

#ifndef POLAR_DECL_IMPORT
#   define POLAR_DECL_IMPORT
#endif

#ifndef POLAR_DECL_HIDDEN
#   define POLAR_DECL_HIDDEN
#endif

#ifndef POLAR_DECL_UNUSED
#   define POLAR_DECL_UNUSED
#endif

#ifndef POLAR_DECL_UNUSED_MEMBER
#   define POLAR_DECL_UNUSED_MEMBER
#endif

#ifndef POLAR_FUNC_INFO
#   if defined(POLAR_OS_SOLARIS) || defined(POLAR_CC_XLC)
#      define POLAR_FUNC_INFO __FILE__ "(line number unavailable)"
#   else
#      define POLAR_FUNC_INFO __FILE__ ":" POLAR_STRINGIFY(__LINE__)
#   endif
#endif

#ifndef POLAR_DECL_CF_RETURNS_RETAINED
#   define POLAR_DECL_CF_RETURNS_RETAINED
#endif

#ifndef POLAR_DECL_NS_RETURNS_AUTORELEASED
#   define POLAR_DECL_NS_RETURNS_AUTORELEASED
#endif

#ifndef POLAR_DECL_PURE_FUNCTION
#   define POLAR_DECL_PURE_FUNCTION
#endif

#ifndef POLAR_DECL_CONST_FUNCTION
#   define POLAR_DECL_CONST_FUNCTION POLAR_DECL_PURE_FUNCTION
#endif

#ifndef POLAR_MAKE_UNCHECKED_ARRAY_ITERATOR
#   define POLAR_MAKE_UNCHECKED_ARRAY_ITERATOR(x) (x)
#endif

#ifndef POLAR_MAKE_CHECKED_ARRAY_ITERATOR
#   define POLAR_MAKE_CHECKED_ARRAY_ITERATOR(x, N) (x)
#endif

/*
 * SG10's SD-6 feature detection and some useful extensions from Clang and GCC
 * https://isocpp.org/std/standing-documents/sd-6-sg10-feature-test-recommendations
 * http://clang.llvm.org/docs/LanguageExtensions.html#feature-checking-macros
 */
#ifdef __has_builtin
#   define POLAR_HAS_BUILTIN(x) __has_builtin(x)
#else
#   define POLAR_HAS_BUILTIN(x) 0
#endif

#ifdef __has_attribute
#   define POLAR_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
#   define POLAR_HAS_ATTRIBUTE(x) 0
#endif

#ifdef __has_cpp_attribute
#   define POLAR_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#   define POLAR_HAS_CPP_ATTRIBUTE(x) 0
#endif

#ifdef __has_include
#   define POLAR_HAS_INCLUDE(x) __has_include(x)
#else
#   define POLAR_HAS_INCLUDE(x) 0
#endif

#ifdef __has_include_next
#   define POLAR_HAS_INCLUDE_NEXT(x) __has_include_next(x)
#else
#   define POLAR_HAS_INCLUDE_NEXT(x) 0
#endif

/*
 * Warning/diagnostic handling
 */
#define POLAR_DO_PRAGMA(text) _Pragma(#text)
#if defined(POLAR_CC_INTEL) && defined(POLAR_CC_MSVC)
/* icl.exe: Intel compiler on Windows */
#   undef POLAR_DO_PRAGMA                           /* not needed */
#   define POLAR_WARNING_PUSH                       __pragma(warning(push))
#   define POLAR_WARNING_POP                        __pragma(warning(pop))
#   define POLAR_WARNING_DISABLE_MSVC(number)
#   define POLAR_WARNING_DISABLE_INTEL(number)      __pragma(warning(disable: number))
#   define POLAR_WARNING_DISABLE_CLANG(text)
#   define POLAR_WARNING_DISABLE_GCC(text)
#   define POLAR_WARNING_DISABLE_DEPRECATED         POLAR_WARNING_DISABLE_INTEL(1478 1786)
#elif defined(POLAR_CC_INTEL)
/* icc: Intel compiler on Linux or OS X */
#   define POLAR_WARNING_PUSH                       POLAR_DO_PRAGMA(warning(push))
#   define POLAR_WARNING_POP                        POLAR_DO_PRAGMA(warning(pop))
#   define POLAR_WARNING_DISABLE_INTEL(number)      POLAR_DO_PRAGMA(warning(disable: number))
#   define POLAR_WARNING_DISABLE_MSVC(number)
#   define POLAR_WARNING_DISABLE_CLANG(text)
#   define POLAR_WARNING_DISABLE_GCC(text)
#   define POLAR_WARNING_DISABLE_DEPRECATED         POLAR_WARNING_DISABLE_INTEL(1478 1786)
#elif defined(POLAR_CC_MSVC) && _MSC_VER >= 1500 && !defined(POLAR_CC_CLANG)
#   undef POLAR_DO_PRAGMA                           /* not needed */
#   define POLAR_WARNING_PUSH                       __pragma(warning(push))
#   define POLAR_WARNING_POP                        __pragma(warning(pop))
#   define POLAR_WARNING_DISABLE_MSVC(number)       __pragma(warning(disable: number))
#   define POLAR_WARNING_DISABLE_INTEL(number)
#   define POLAR_WARNING_DISABLE_CLANG(text)
#   define POLAR_WARNING_DISABLE_GCC(text)
#   define POLAR_WARNING_DISABLE_DEPRECATED         POLAR_WARNING_DISABLE_MSVC(4996)
#elif defined(POLAR_CC_CLANG)
#   define POLAR_WARNING_PUSH                       POLAR_DO_PRAGMA(clang diagnostic push)
#   define POLAR_WARNING_POP                        POLAR_DO_PRAGMA(clang diagnostic pop)
#   define POLAR_WARNING_DISABLE_CLANG(text)        POLAR_DO_PRAGMA(clang diagnostic ignored text)
#   define POLAR_WARNING_DISABLE_GCC(text)
#   define POLAR_WARNING_DISABLE_INTEL(number)
#   define POLAR_WARNING_DISABLE_MSVC(number)
#   define POLAR_WARNING_DISABLE_DEPRECATED         POLAR_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")
#elif defined(POLAR_CC_GNU) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 406)
#   define POLAR_WARNING_PUSH                       POLAR_DO_PRAGMA(GCC diagnostic push)
#   define POLAR_WARNING_POP                        POLAR_DO_PRAGMA(GCC diagnostic pop)
#   define POLAR_WARNING_DISABLE_GCC(text)          POLAR_DO_PRAGMA(GCC diagnostic ignored text)
#   define POLAR_WARNING_DISABLE_CLANG(text)
#   define POLAR_WARNING_DISABLE_INTEL(number)
#   define POLAR_WARNING_DISABLE_MSVC(number)
#   define POLAR_WARNING_DISABLE_DEPRECATED         POLAR_WARNING_DISABLE_GCC("-Wdeprecated-declarations")
#else       // All other compilers, GCC < 4.6 and MSVC < 2008
#   define POLAR_WARNING_DISABLE_GCC(text)
#   define POLAR_WARNING_PUSH
#   define POLAR_WARNING_POP
#   define POLAR_WARNING_DISABLE_INTEL(number)
#   define POLAR_WARNING_DISABLE_MSVC(number)
#   define POLAR_WARNING_DISABLE_CLANG(text)
#   define POLAR_WARNING_DISABLE_GCC(text)
#   define POLAR_WARNING_DISABLE_DEPRECATED
#endif

//#define POLAR_UNREACHABLE() \
//   do {\
//   POLAR_ASSERT_X(false, "POLAR_UNREACHABLE()", "POLAR_UNREACHABLE was reached");\
//   POLAR_UNREACHABLE_IMPL();\
//   } while (0)

#define POLAR_ASSUME(Expr) \
   do {\
   const bool valueOfExpression = Expr;\
   POLAR_ASSERT_X(valueOfExpression, "POLAR_ASSUME()", "Assumption in POLAR_ASSUME(\"" #Expr "\") was not correct");\
   POLAR_ASSUME_IMPL(valueOfExpression);\
   } while (0)

#if POLAR_HAS_CPP_ATTRIBUTE(fallthrough)
#   define POLAR_FALLTHROUGH [[fallthrough]]
#elif defined(__cplusplus)
/* Clang can not parse namespaced attributes in C mode, but defines __has_cpp_attribute */
#   if POLAR_HAS_CPP_ATTRIBUTE(clang::fallthrough)
#      define POLAR_FALLTHROUGH [[clang::fallthrough]]
#   elif POLAR_HAS_CPP_ATTRIBUTE(gnu::fallthrough)
#      define POLAR_FALLTHROUGH [[gnu::fallthrough]]
#   endif
#endif

#ifndef POLAR_FALLTHROUGH
#   if defined(POLAR_CC_GNU) && POLAR_CC_GNU >= 700
#      define POLAR_FALLTHROUGH __attribute__((fallthrough))
#   else
#      define POLAR_FALLTHROUGH (void)0
#   endif
#endif

/*
    Sanitize compiler feature availability
*/
#if !defined(POLAR_PROCESSOR_X86)
#   undef POLAR_COMPILER_SUPPORTS_SSE2
#   undef POLAR_COMPILER_SUPPORTS_SSE3
#   undef POLAR_COMPILER_SUPPORTS_SSSE3
#   undef POLAR_COMPILER_SUPPORTS_SSE4_1
#   undef POLAR_COMPILER_SUPPORTS_SSE4_2
#   undef POLAR_COMPILER_SUPPORTS_AVX
#   undef POLAR_COMPILER_SUPPORTS_AVX2
#endif

#if !defined(POLAR_PROCESSOR_ARM)
#   undef POLAR_COMPILER_SUPPORTS_NEON
#endif

#if !defined(POLAR_PROCESSOR_MIPS)
#   undef POLAR_COMPILER_SUPPORTS_MIPS_DSP
#   undef POLAR_COMPILER_SUPPORTS_MIPS_DSPR2
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

#endif // POLARPHP_GLOBAL_COMPILERDETECTION_H
