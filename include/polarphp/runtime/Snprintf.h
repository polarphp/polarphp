// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/25.

#ifndef POLARPHP_RUNTIME_PHP_SNPRINTF_H
#define POLARPHP_RUNTIME_PHP_SNPRINTF_H

#include "polarphp/runtime/RtDefs.h"

// forward declare
struct lconv;

namespace polar {
namespace runtime {

typedef int bool_int;

typedef enum {
   NO = 0, YES = 1
} boolean_e;

POLAR_DECL_EXPORT int ap_php_slprintf(char *buf, size_t len, const char *format,...) ZEND_ATTRIBUTE_FORMAT(printf, 3, 4);
POLAR_DECL_EXPORT int ap_php_vslprintf(char *buf, size_t len, const char *format, va_list ap);
POLAR_DECL_EXPORT int ap_php_snprintf(char *, size_t, const char *, ...) ZEND_ATTRIBUTE_FORMAT(printf, 3, 4);
POLAR_DECL_EXPORT int ap_php_vsnprintf(char *, size_t, const char *, va_list ap);
POLAR_DECL_EXPORT int ap_php_vasprintf(char **buf, const char *format, va_list ap);
POLAR_DECL_EXPORT int ap_php_asprintf(char **buf, const char *format, ...) ZEND_ATTRIBUTE_FORMAT(printf, 2, 3);
POLAR_DECL_EXPORT int php_sprintf (char* s, const char* format, ...) PHP_ATTRIBUTE_FORMAT(printf, 2, 3);
POLAR_DECL_EXPORT char * php_gcvt(double value, int ndigit, char dec_point, char exponent, char *buf);
POLAR_DECL_EXPORT char * php_0cvt(double value, int ndigit, char dec_point, char exponent, char *buf);
POLAR_DECL_EXPORT char * php_conv_fp(char format, double num,
                                     boolean_e add_dp, int precision, char dec_point, bool_int * is_negative, char *buf, size_t *len);

#ifdef slprintf
#undef slprintf
#endif
#define slprintf ap_php_slprintf

#ifdef vslprintf
#undef vslprintf
#endif
#define vslprintf ap_php_vslprintf

#ifdef snprintf
#undef snprintf
#endif
#define snprintf ap_php_snprintf

#ifdef vsnprintf
#undef vsnprintf
#endif
#define vsnprintf ap_php_vsnprintf

#ifndef HAVE_VASPRINTF
#define vasprintf ap_php_vasprintf
#endif

#ifndef HAVE_ASPRINTF
#define asprintf ap_php_asprintf
#endif

#ifdef sprintf
#undef sprintf
#endif
#define sprintf php_sprintf

typedef enum {
   LM_STD = 0,
#if SIZEOF_INTMAX_T
   LM_INTMAX_T,
#endif
#if SIZEOF_PTRDIFF_T
   LM_PTRDIFF_T,
#endif
#if SIZEOF_LONG_LONG
   LM_LONG_LONG,
#endif
   LM_SIZE_T,
   LM_LONG,
   LM_LONG_DOUBLE,
   LM_PHP_INT_T
} length_modifier_e;

/// TODO here size maybe something wrong
#ifdef POLAR_OS_WIN32
# define WIDE_INT		__int64
#elif SIZEOF_LONG_LONG_INT
# define WIDE_INT		long long int
#elif SIZEOF_LONG_LONG
# define WIDE_INT		long long
#else
# define WIDE_INT		long
#endif
typedef WIDE_INT wide_int;
typedef unsigned WIDE_INT u_wide_int;

POLAR_DECL_EXPORT char * ap_php_conv_10(wide_int num, bool_int is_unsigned,
                                        bool_int * is_negative, char *buf_end, size_t *len);

POLAR_DECL_EXPORT char * ap_php_conv_p2(u_wide_int num, int nbits,
                                        char format, char *buf_end, size_t *len);

#ifdef HAVE_LOCALECONV
/* {{{ localeconv_r
 * glibc's localeconv is not reentrant, so lets make it so ... sorta */
POLAR_DECL_EXPORT lconv *localeconv_r(lconv *out);
#endif

/* The maximum precision that's allowed for float conversion. Does not include
 * decimal separator, exponent, sign, terminator. Currently does not affect
 * the modes e/f, only g/k/H, as those have a different limit enforced at
 * another level (see NDIG in php_conv_fp()).
 * Applies to the formatting functions of both spprintf.c and snprintf.c, which
 * use equally sized buffers of MAX_BUF_SIZE = 512 to hold the result of the
 * call to php_gcvt().
 * This should be reasonably smaller than MAX_BUF_SIZE (I think MAX_BUF_SIZE - 9
 * should be enough, but let's give some more space) */
#define FORMAT_CONV_MAX_PRECISION 500

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_PHP_SNPRINTF_H
