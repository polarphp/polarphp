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

#ifndef POLARPHP_RUNTIME_REENTRANCY_H
#define POLARPHP_RUNTIME_REENTRANCY_H

#include "polarphp/runtime/internal/DepsZendVmHeaders.h"

#include <sys/types.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <time.h>

/* currently, PHP does not check for these functions, but assumes
   that they are available on all systems. */

#define HAVE_LOCALTIME 1
#define HAVE_GMTIME 1
#define HAVE_ASCTIME 1
#define HAVE_CTIME 1

#if defined(PHP_IRIX_TIME_R)
#undef HAVE_ASCTIME_R
#undef HAVE_CTIME_R
#endif

#if defined(PHP_HPUX_TIME_R)
#undef HAVE_LOCALTIME_R
#undef HAVE_ASCTIME_R
#undef HAVE_CTIME_R
#undef HAVE_GMTIME_R
#endif

#ifdef HAVE_POSIX_READDIR_R
#define polar_readdir_r ::readdir_r
#else
POLAR_DECL_EXPORT int polar_readdir_r(DIR *dirp, struct dirent *entry,
                                struct dirent **result);
#endif

#if !defined(HAVE_LOCALTIME_R) && defined(HAVE_LOCALTIME)
#define PHP_NEED_REENTRANCY 1
POLAR_DECL_EXPORT struct tm *polar_localtime_r(const time_t *const timep, struct tm *p_tm);
#else
#define php_localtime_r ::localtime_r
#ifdef MISSING_LOCALTIME_R_DECL
struct tm *polar_localtime_r(const time_t *const timep, struct tm *p_tm);
#endif
#endif

#if !defined(HAVE_CTIME_R) && defined(HAVE_CTIME)
#define PHP_NEED_REENTRANCY 1
POLAR_DECL_EXPORT char *polar_ctime_r(const time_t *clock, char *buf);
#else
#define php_ctime_r ::ctime_r
#ifdef MISSING_CTIME_R_DECL
char *polar_ctime_r(const time_t *clock, char *buf);
#endif
#endif


#if !defined(HAVE_ASCTIME_R) && defined(HAVE_ASCTIME)
#define PHP_NEED_REENTRANCY 1
POLAR_DECL_EXPORT char *polar_asctime_r(const struct tm *tm, char *buf);
#else
#define polar_asctime_r ::asctime_r
#ifdef MISSING_ASCTIME_R_DECL
char *polar_asctime_r(const struct tm *tm, char *buf);
#endif
#endif

#if !defined(HAVE_GMTIME_R) && defined(HAVE_GMTIME)
#define PHP_NEED_REENTRANCY 1
POLAR_DECL_EXPORT struct tm *polar_gmtime_r(const time_t *const timep, struct tm *p_tm);
#else
#define polar_gmtime_r ::gmtime_r
#ifdef MISSING_GMTIME_R_DECL
struct tm *polar_gmtime_r(const time_t *const timep, struct tm *p_tm);
#endif
#endif

#if !defined(HAVE_STRTOK_R)
POLAR_DECL_EXPORT char *polar_strtok_r(char *s, const char *delim, char **last);
#else
#define polar_strtok_r strtok_r
#ifdef MISSING_STRTOK_R_DECL
char *polar_strtok_r(char *s, const char *delim, char **last);
#endif
#endif

#if !defined(HAVE_RAND_R)
POLAR_DECL_EXPORT int polar_rand_r(unsigned int *seed);
#else
#define polar_rand_r ::rand_r
#endif

#if !defined(ZTS)
#undef PHP_NEED_REENTRANCY
#endif

#if defined(PHP_NEED_REENTRANCY)
void polar_reentrancy_startup();
void polar_reentrancy_shutdown();
#else
#define polar_reentrancy_startup()
#define polar_reentrancy_shutdown()
#endif

#endif // POLARPHP_RUNTIME_REENTRANCY_H
