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

#ifndef POLARPHP_RUNTIME_PHPDEFS_H
#define POLARPHP_RUNTIME_PHPDEFS_H

#include "polarphp/global/CompilerFeature.h"
#include "polarphp/global/SystemDetection.h"
#include "polarphp/global/Config.h"
#include "polarphp/global/PolarVersion.h"
#include "polarphp/runtime/internal/ZendMacros.h"

POLAR_BEGIN_DISABLE_ZENDVM_WARNING()
#include "polarphp/vm/zend/zend_portability.h"
POLAR_END_DISABLE_ZENDVM_WARNING()

#define PHP_DEFAULT_CHARSET "UTF-8"
/* PHP's DEBUG value must match Zend's ZEND_DEBUG value */
#undef PHP_DEBUG
#define PHP_DEBUG ZEND_DEBUG

#ifdef POLAR_OS_WIN32
#	define PHP_DIR_SEPARATOR '\\'
#	define PHP_EOL "\r\n"
#else
#	define PHP_DIR_SEPARATOR '/'
#	define PHP_EOL "\n"
#endif

#define PHP_MT_RAND_MAX ((zend_long) (0x7FFFFFFF)) /* (1<<31) - 1 */
/* System Rand functions */
#ifndef RAND_MAX
#define RAND_MAX PHP_MT_RAND_MAX
#endif

#define PHP_RAND_MAX PHP_MT_RAND_MAX

/* Windows specific defines */
#ifdef POLAR_OS_WIN32
# define HAVE_DECLARED_TIMEZONE
# define WIN32_LEAN_AND_MEAN
# define NOOPENFILE

# include <io.h>
# include <malloc.h>
# include <direct.h>
# include <stdlib.h>
# include <stdio.h>
# include <stdarg.h>
# include <sys/types.h>
# include <process.h>

typedef int uid_t;
typedef int gid_t;
typedef char * caddr_t;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef int pid_t;

# ifndef PHP_DEBUG
#  ifdef inline
#   undef inline
#  endif
#  define inline		__inline
# endif

# define M_TWOPI        (M_PI * 2.0)
# define off_t			_off_t

# define lstat(x, y)	php_sys_lstat(x, y)
# define chdir(path)	_chdir(path)
# define mkdir(a, b)	_mkdir(a)
# define rmdir(a)		_rmdir(a)
# define getpid			_getpid
# define php_sleep(t)	SleepEx(t*1000, TRUE)

# ifndef getcwd
#  define getcwd(a, b)	_getcwd(a, b)
# endif
#endif

#if HAVE_ASSERT_H
#if PHP_DEBUG
#undef NDEBUG
#else
#ifndef NDEBUG
#define NDEBUG
#endif
#endif
#include <assert.h>
#else /* HAVE_ASSERT_H */
#define assert(expr) ((void) (0))
#endif /* HAVE_ASSERT_H */

#if HAVE_UNIX_H
#include <unix.h>
#endif

#if HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include <stdlib.h>
#include <ctype.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#else
# if HAVE_SYS_VARARGS_H
# include <sys/varargs.h>
# endif
#endif

#if HAVE_PWD_H
# ifdef POLAR_OS_WIN32
#include "win32/param.h"
# else
#include <pwd.h>
#include <sys/param.h>
# endif
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif

#ifndef LONG_MAX
#define LONG_MAX 2147483647L
#endif

#ifndef LONG_MIN
#define LONG_MIN (- LONG_MAX - 1)
#endif

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

#ifndef INT_MIN
#define INT_MIN (- INT_MAX - 1)
#endif

/* double limits */
#include <float.h>
#if defined(DBL_MANT_DIG) && defined(DBL_MIN_EXP)
#define PHP_DOUBLE_MAX_LENGTH (3 + DBL_MANT_DIG - DBL_MIN_EXP)
#else
#define PHP_DOUBLE_MAX_LENGTH 1080
#endif

#define PHP_GCC_VERSION ZEND_GCC_VERSION
#define PHP_ATTRIBUTE_MALLOC ZEND_ATTRIBUTE_MALLOC
#define PHP_ATTRIBUTE_FORMAT ZEND_ATTRIBUTE_FORMAT

#ifndef MAXPATHLEN
# ifdef PHP_WIN32
#  include "win32/ioutil.h"
#  define MAXPATHLEN PHP_WIN32_IOUTIL_MAXPATHLEN
# elif PATH_MAX
#  define MAXPATHLEN PATH_MAX
# elif defined(MAX_PATH)
#  define MAXPATHLEN MAX_PATH
# else
#  define MAXPATHLEN 256    /* Should be safe for any weird systems that do not define it */
# endif
#endif

#ifndef HAVE_STRLCPY
namespace polar {
POLAR_DECL_EXPORT size_t stringlcpy(char *dst, const char *src, size_t siz);
} // polar
#undef strlcpy
#define strlcpy ::polar::stringlcpy
#define HAVE_STRLCPY 1
#define USE_STRLCPY_PHP_IMPL 1
#endif

#ifndef HAVE_STRLCAT
namespace polar {
POLAR_DECL_EXPORT size_t stringlcat(char *dst, const char *src, size_t siz);
}
#undef strlcat
#define strlcat ::polar::stringlcat
#define HAVE_STRLCAT 1
#define USE_STRLCAT_PHP_IMPL 1
#endif

#undef ZEND_IGNORE_VALUE
#if defined(__GNUC__) && __GNUC__ >= 4
# define ZEND_IGNORE_VALUE(x) { __typeof__ (x) __x = (x); (void) __x; }
#else
# define ZEND_IGNORE_VALUE(x) ((void) (x))
#endif
#define php_ignore_value(x) ZEND_IGNORE_VALUE(x)

/* global variables */
#if !defined(POLAR_OS_WIN32)
#define PHP_SLEEP_NON_VOID
#define php_sleep sleep
extern char **environ;
#endif	/* !defined(POLAR_OS_WIN32) */

#define php_error zend_error
#define error_handling_t zend_error_handling_t

namespace polar {
namespace runtime {

using VmExtensionInitFuncType = bool(*)();
extern VmExtensionInitFuncType sg_vmExtensionInitHook;

POLAR_DECL_EXPORT ZEND_COLD void php_verror(const char *docref, const char *params, int type, const char *format, va_list args) PHP_ATTRIBUTE_FORMAT(printf, 4, 0);

/* POLAR_DECL_EXPORT void php_error(int type, const char *format, ...); */
POLAR_DECL_EXPORT ZEND_COLD void php_error_docref0(const char *docref, int type, const char *format, ...)
PHP_ATTRIBUTE_FORMAT(printf, 3, 4);
POLAR_DECL_EXPORT ZEND_COLD void php_error_docref1(const char *docref, const char *param1, int type, const char *format, ...)
PHP_ATTRIBUTE_FORMAT(printf, 4, 5);
POLAR_DECL_EXPORT ZEND_COLD void php_error_docref2(const char *docref, const char *param1, const char *param2, int type, const char *format, ...)
PHP_ATTRIBUTE_FORMAT(printf, 5, 6);
#ifdef POLAR_OS_WIN32
POLAR_DECL_EXPORT ZEND_COLD void php_win32_docref2_from_error(DWORD error, const char *param1, const char *param2);
#endif

#define php_error_docref ::polar::runtime::php_error_docref0
#define zenderror phperror
#define zendlex phplex
#define phpparse zendparse
#define phprestart zendrestart
#define phpin zendin
#define php_memnstr zend_memnstr

POLAR_DECL_EXPORT bool php_register_internal_extensions();
POLAR_DECL_EXPORT void php_register_pre_request_shutdown(void (*func)(void *), void *userdata);
POLAR_DECL_EXPORT void php_com_initialize();
POLAR_DECL_EXPORT char *php_get_current_user();

/* emit warning and suggestion for unsafe select(2) usage */
namespace internal {
void emit_fd_setsize_warning(int maxFd);
} // internal
} // runtime
} // polar

/* PHP-named Zend macro wrappers */
#define PHP_FN					ZEND_FN
#define PHP_MN					ZEND_MN
#define PHP_NAMED_FUNCTION		ZEND_NAMED_FUNCTION
#define PHP_FUNCTION			ZEND_FUNCTION
#define PHP_METHOD  			ZEND_METHOD

#define PHP_RAW_NAMED_FE ZEND_RAW_NAMED_FE
#define PHP_NAMED_FE	ZEND_NAMED_FE
#define PHP_FE			ZEND_FE
#define PHP_DEP_FE      ZEND_DEP_FE
#define PHP_FALIAS		ZEND_FALIAS
#define PHP_DEP_FALIAS	ZEND_DEP_FALIAS
#define PHP_ME          ZEND_ME
#define PHP_MALIAS      ZEND_MALIAS
#define PHP_ABSTRACT_ME ZEND_ABSTRACT_ME
#define PHP_ME_MAPPING  ZEND_ME_MAPPING
#define PHP_FE_END      ZEND_FE_END

#define PHP_MODULE_STARTUP_N	ZEND_MODULE_STARTUP_N
#define PHP_MODULE_SHUTDOWN_N	ZEND_MODULE_SHUTDOWN_N
#define PHP_MODULE_ACTIVATE_N	ZEND_MODULE_ACTIVATE_N
#define PHP_MODULE_DEACTIVATE_N	ZEND_MODULE_DEACTIVATE_N
#define PHP_MODULE_INFO_N		ZEND_MODULE_INFO_N

#define PHP_MODULE_STARTUP_D	ZEND_MODULE_STARTUP_D
#define PHP_MODULE_SHUTDOWN_D	ZEND_MODULE_SHUTDOWN_D
#define PHP_MODULE_ACTIVATE_D	ZEND_MODULE_ACTIVATE_D
#define PHP_MODULE_DEACTIVATE_D	ZEND_MODULE_DEACTIVATE_D
#define PHP_MODULE_INFO_D		ZEND_MODULE_INFO_D

/* Compatibility macros */
#define PHP_MINIT		ZEND_MODULE_STARTUP_N
#define PHP_MSHUTDOWN	ZEND_MODULE_SHUTDOWN_N
#define PHP_RINIT		ZEND_MODULE_ACTIVATE_N
#define PHP_RSHUTDOWN	ZEND_MODULE_DEACTIVATE_N
#define PHP_MINFO		ZEND_MODULE_INFO_N
#define PHP_GINIT		ZEND_GINIT
#define PHP_GSHUTDOWN	ZEND_GSHUTDOWN

#define PHP_MINIT_FUNCTION		ZEND_MODULE_STARTUP_D
#define PHP_MSHUTDOWN_FUNCTION	ZEND_MODULE_SHUTDOWN_D
#define PHP_RINIT_FUNCTION		ZEND_MODULE_ACTIVATE_D
#define PHP_RSHUTDOWN_FUNCTION	ZEND_MODULE_DEACTIVATE_D
#define PHP_MINFO_FUNCTION		ZEND_MODULE_INFO_D
#define PHP_GINIT_FUNCTION		ZEND_GINIT_FUNCTION
#define PHP_GSHUTDOWN_FUNCTION	ZEND_GSHUTDOWN_FUNCTION

#define PHP_MODULE_GLOBALS		ZEND_MODULE_GLOBALS

/* Error display modes */
#define PHP_DISPLAY_ERRORS_STDOUT	1
#define PHP_DISPLAY_ERRORS_STDERR	2

/* macros */
#define PHP_STR_PRINT(str)	((str)?(str):"")

#ifdef POLAR_OS_WIN32
/* it is safe to FD_SET too many fd's under win32; the macro will simply ignore
 * descriptors that go beyond the default FD_SETSIZE */
# define PHP_SAFE_FD_SET(fd, set)	FD_SET(fd, set)
# define PHP_SAFE_FD_CLR(fd, set)	FD_CLR(fd, set)
# define PHP_SAFE_FD_ISSET(fd, set)	FD_ISSET(fd, set)
# define PHP_SAFE_MAX_FD(m, n)		do { if (n + 1 >= FD_SETSIZE) { polar::internal::emit_fd_setsize_warning(n); }} while(0)
#else
# define PHP_SAFE_FD_SET(fd, set)	do { if (fd < FD_SETSIZE) FD_SET(fd, set); } while(0)
# define PHP_SAFE_FD_CLR(fd, set)	do { if (fd < FD_SETSIZE) FD_CLR(fd, set); } while(0)
# define PHP_SAFE_FD_ISSET(fd, set)	((fd < FD_SETSIZE) && FD_ISSET(fd, set))
# define PHP_SAFE_MAX_FD(m, n)		do { if (m >= FD_SETSIZE) { polar::internal::emit_fd_setsize_warning(m); m = FD_SETSIZE - 1; }} while(0)
#endif

#define PHP_EMPTY_STR const_cast<char *>("")

/* Syslog filters */
#define PHP_SYSLOG_FILTER_ALL		0
#define PHP_SYSLOG_FILTER_NO_CTRL	1
#define PHP_SYSLOG_FILTER_ASCII		2

#define polar_try zend_try
#define polar_catch zend_catch
#define polar_first_try zend_first_try
#define polar_end_try zend_end_try()

#ifdef POLAR_OS_WIN32
typedef SOCKET php_socket_t;
#else
typedef int php_socket_t;
#endif

#include "polarphp/runtime/Reentrancy.h"

#define POLAR_INI_DEFAULT(name, value)\
   ZVAL_NEW_STR(&tmp, zend_string_init(value, sizeof(value) - 1, 1));\
   zend_hash_str_update(configuration_hash, name, sizeof(name) - 1, &tmp);\

void cli_ini_defaults(HashTable *configuration_hash);

#ifndef XtOffset
#if defined(CRAY) || (defined(__arm) && !(defined(LINUX) || defined(__riscos__)))
#ifdef __STDC__
#define XtOffset(p_type, field) _Offsetof(p_type, field)
#else
#ifdef CRAY2
#define XtOffset(p_type, field) \
   (sizeof(int)*((unsigned int)&(((p_type)NULL)->field)))

#else /* !CRAY2 */

#define XtOffset(p_type, field) ((unsigned int)&(((p_type)NULL)->field))

#endif /* !CRAY2 */
#endif /* __STDC__ */
#else /* ! (CRAY || __arm) */

#define XtOffset(p_type, field) \
   ((zend_long) (((char *) (&(((p_type)NULL)->field))) - ((char *) NULL)))

#endif /* !CRAY */
#endif /* ! XtOffset */

#ifndef XtOffsetOf
#ifdef offsetof
#define XtOffsetOf(s_type, field) offsetof(s_type, field)
#else
#define XtOffsetOf(s_type, field) XtOffset(s_type*, field)
#endif
#endif /* !XtOffsetOf */

#define POLAR_LITERAL_STR(str) const_cast<char *>((str))
#define PHP_STDIN_FILENAME_MARK const_cast<char *>("Standard input code")

#endif // POLARPHP_RUNTIME_PHPDEFS_H
