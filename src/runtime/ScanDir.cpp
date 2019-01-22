// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/20.

#include "polarphp/runtime/ScanDir.h"
#include "polarphp/runtime/Reentrancy.h"
#include "polarphp/runtime/RtDefs.h"

namespace polar {

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#ifndef HAVE_SCANDIR

#ifdef POLAR_OS_WIN32
#include "win32/param.h"
#include "win32/readdir.h"
#endif

#include <stdlib.h>
#include <search.h>

#endif /* HAVE_SCANDIR */

#ifndef HAVE_ALPHASORT

#ifdef HAVE_STRING_H
#include <string.h>
#endif

int php_alphasort(const struct dirent **a, const struct dirent **b)
{
   return strcoll((*a)->d_name,(*b)->d_name);
}
#endif /* HAVE_ALPHASORT */

#ifndef HAVE_SCANDIR
int php_scandir(const char *dirname, struct dirent **namelist[], int (*selector) (const struct dirent *entry), int (*compare) (const struct dirent **a, const struct dirent **b))
{
   DIR *dirp = nullptr;
   struct dirent **vector = nullptr;
   int vector_size = 0;
   int nfiles = 0;
   char entry[sizeof(struct dirent)+MAXPATHLEN];
   struct dirent *dp = (struct dirent *)&entry;

   if (namelist == nullptr) {
      return -1;
   }
   if (!(dirp = opendir(dirname))) {
      return -1;
   }
   while (!polar_readdir_r(dirp, (struct dirent *)entry, &dp) && dp) {
      size_t dsize = 0;
      struct dirent *newdp = nullptr;
      if (selector && (*selector)(dp) == 0) {
         continue;
      }
      if (nfiles == vector_size) {
         struct dirent **newv;
         if (vector_size == 0) {
            vector_size = 10;
         } else {
            vector_size *= 2;
         }
         newv = (struct dirent **) realloc (vector, vector_size * sizeof (struct dirent *));
         if (!newv) {
            return -1;
         }
         vector = newv;
      }
      dsize = sizeof (struct dirent) + ((strlen(dp->d_name) + 1) * sizeof(char));
      newdp = (struct dirent *) malloc(dsize);
      if (newdp == nullptr) {
         goto fail;
      }
      vector[nfiles++] = (struct dirent *) memcpy(newdp, dp, dsize);
   }
   closedir(dirp);
   *namelist = vector;
   if (compare) {
      qsort (*namelist, nfiles, sizeof(struct dirent *), (int (*) (const void *, const void *)) compare);
   }

   return nfiles;

fail:
   while (nfiles-- > 0) {
      free(vector[nfiles]);
   }
   free(vector);
   return -1;
}
#endif

} // polar
