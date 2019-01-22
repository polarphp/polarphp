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

#include "polarphp/runtime/Reentrancy.h"
#include "polarphp/runtime/RtDefs.h"

#include <sys/types.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

enum
{
   LOCALTIME_R,
   CTIME_R,
   ASCTIME_R,
   GMTIME_R,
   READDIR_R,
   NUMBER_OF_LOCKS
};

#if defined(PHP_NEED_REENTRANCY)

#include <TSRM.h>

static MUTEX_T reentrant_locks[NUMBER_OF_LOCKS];

#define local_lock(x) tsrm_mutex_lock(reentrant_locks[x])
#define local_unlock(x) tsrm_mutex_unlock(reentrant_locks[x])

#else

#define local_lock(x)
#define local_unlock(x)

#endif

#if defined(PHP_IRIX_TIME_R)

#define HAVE_CTIME_R 1
#define HAVE_ASCTIME_R 1

char *polar_ctime_r(const time_t *clock, char *buf)
{
   if (ctime_r(clock, buf) == buf) {
      return buf;
   }
   return nullptr;
}

char *polar_asctime_r(const struct tm *tm, char *buf)
{
   if (asctime_r(tm, buf) == buf) {
      return buf;
   }
   return nullptr;
}

#endif

#if defined(PHP_HPUX_TIME_R)

#define HAVE_LOCALTIME_R 1
#define HAVE_CTIME_R 1
#define HAVE_ASCTIME_R 1
#define HAVE_GMTIME_R 1

struct tm *polar_localtime_r(const time_t *const timep, struct tm *p_tm)
{
   if (localtime_r(timep, p_tm) == 0) {
      return (p_tm);
   }
   return nullptr;
}

POLAR_DECL_EXPORT char *polar_ctime_r(const time_t *clock, char *buf)
{
   if (ctime_r(clock, buf, 26) != -1) {
      return buf;
   }
   return nullptr;
}

char *polar_asctime_r(const struct tm *tm, char *buf)
{
   if (asctime_r(tm, buf, 26) != -1) {
      return buf;
   }
   return nullptr;
}

struct tm *polar_gmtime_r(const time_t *const timep, struct tm *p_tm)
{
   if (gmtime_r(timep, p_tm) == 0) {
      return (p_tm);
   }
   return nullptr;
}

#endif

#if !defined(HAVE_POSIX_READDIR_R)

int polar_readdir_r(DIR *dirp, struct dirent *entry,
                    struct dirent **result)
{
#if defined(HAVE_OLD_READDIR_R)
   int ret = 0;

   /* We cannot rely on the return value of readdir_r
      as it differs between various platforms
      (HPUX returns 0 on success whereas Solaris returns non-zero)
    */
   entry->d_name[0] = '\0';
   readdir_r(dirp, entry);
   if (entry->d_name[0] == '\0') {
      *result = nullptr;
      ret = errno;
   } else {
      *result = entry;
   }
   return ret;
#else
   struct dirent *ptr;
   int ret = 0;
   local_lock(READDIR_R);
   errno = 0;
   ptr = readdir(dirp);
   if (!ptr && errno != 0) {
      ret = errno;
   }
   if (ptr) {
      memcpy(entry, ptr, sizeof(*ptr));
   }
   *result = ptr;
   local_unlock(READDIR_R);
   return ret;
#endif
}

#endif

#if !defined(HAVE_LOCALTIME_R) && defined(HAVE_LOCALTIME)

struct tm *polar_localtime_r(const time_t *const timep, struct tm *p_tm)
{
   struct tm *tmp;
   local_lock(LOCALTIME_R);
   tmp = localtime(timep);
   if (tmp) {
      memcpy(p_tm, tmp, sizeof(struct tm));
      tmp = p_tm;
   }
   local_unlock(LOCALTIME_R);
   return tmp;
}

#endif

#if !defined(HAVE_CTIME_R) && defined(HAVE_CTIME)

char *polar_ctime_r(const time_t *clock, char *buf)
{
   char *tmp;
   local_lock(CTIME_R);
   tmp = ctime(clock);
   strcpy(buf, tmp);
   local_unlock(CTIME_R);
   return buf;
}

#endif

#if !defined(HAVE_ASCTIME_R) && defined(HAVE_ASCTIME)

char *polar_asctime_r(const struct tm *tm, char *buf)
{
   char *tmp;
   local_lock(ASCTIME_R);
   tmp = asctime(tm);
   strcpy(buf, tmp);
   local_unlock(ASCTIME_R);
   return buf;
}

#endif

#if !defined(HAVE_GMTIME_R) && defined(HAVE_GMTIME)

struct tm *polar_gmtime_r(const time_t *const timep, struct tm *p_tm)
{
   struct tm *tmp;

   local_lock(GMTIME_R);

   tmp = gmtime(timep);
   if (tmp) {
      memcpy(p_tm, tmp, sizeof(struct tm));
      tmp = p_tm;
   }

   local_unlock(GMTIME_R);

   return tmp;
}

#endif

#if defined(PHP_NEED_REENTRANCY)

void polar_reentrancy_startup()
{
   for (int i = 0; i < NUMBER_OF_LOCKS; i++) {
      reentrant_locks[i] = tsrm_mutex_alloc();
   }
}

void polar_reentrancy_shutdown()
{
   for (int i = 0; i < NUMBER_OF_LOCKS; i++) {
      tsrm_mutex_free(reentrant_locks[i]);
   }
}

#endif

#ifndef HAVE_RAND_R

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Posix rand_r function added May 1999 by Wes Peters <wes@softweyr.com>.
 */

#include <sys/types.h>
#include <stdlib.h>

static int
do_rand(unsigned long *ctx)
{
   return ((*ctx = *ctx * 1103515245 + 12345) % ((u_long)PHP_RAND_MAX + 1));
}

POLAR_DECL_EXPORT int polar_rand_r(unsigned int *ctx)
{
   u_long val = (u_long) *ctx;
   *ctx = do_rand(&val);
   return (int) *ctx;
}

#endif

#ifndef HAVE_STRTOK_R

/*
 * Copyright (c) 1998 Softweyr LLC.  All rights reserved.
 *
 * strtok_r, from Berkeley strtok
 * Oct 13, 1998 by Wes Peters <wes@softweyr.com>
 *
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notices, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notices, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *
 *	This product includes software developed by Softweyr LLC, the
 *      University of California, Berkeley, and its contributors.
 *
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SOFTWEYR LLC, THE REGENTS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL SOFTWEYR LLC, THE
 * REGENTS, OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stddef.h>

POLAR_DECL_EXPORT char *polar_strtok_r(char *s, const char *delim, char **last)
{
   char *spanp;
   int c, sc;
   char *tok;
   if (s == nullptr && (s = *last) == nullptr) {
      return nullptr;
   }
   /*
     * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
     */
cont:
   c = *s++;
   for (spanp = (char *)delim; (sc = *spanp++) != 0; ) {
      if (c == sc) {
         goto cont;
      }
   }

   if (c == 0)	{
      /* no non-delimiter characters */
      *last = nullptr;
      return nullptr;
   }
   tok = s - 1;

   /*
     * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
     * Note that delim must have one NUL; we stop if we see that, too.
     */
   for (;;) {
      c = *s++;
      spanp = (char *)delim;
      do {
         if ((sc = *spanp++) == c) {
            if (c == 0) {
               s = nullptr;
            } else {
               char *w = s - 1;
               *w = '\0';
            }
            *last = s;
            return tok;
         }
      }
      while (sc != 0);
   }
   /* NOTREACHED */
}

#endif
