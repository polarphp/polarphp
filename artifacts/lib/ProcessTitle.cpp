// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/13.

#include "ProcessTitle.h"
#include "polarphp/global/SystemDetection.h"
#include "polarphp/global/CompilerFeature.h"

#ifdef POLAR_OS_WIN32
#include "config.w32.h"
#include <windows.h>
#include <process.h>
#include "win32/codepage.h"
#else
POLAR_BEGIN_DISABLE_ZENDVM_WARNING()
#include "polarphp/global/php_config.h"
POLAR_END_DISABLE_ZENDVM_WARNING()
extern "C" {
extern char **environ;
}
#endif

#include <cstdio>

#ifdef HAVE_UNISTD_H
#include <sys/types.h>
#include <unistd.h>
#endif

#include <cstring>
#include <cstdlib>

#if HAVE_SYS_PSTAT_H
#include <sys/pstat.h> /* for HP-UX */
#endif
#if HAVE_PS_STRINGS
#include <machine/vmparam.h> /* for old BSD */
#include <sys/exec.h>
#endif
#if defined(DARWIN)
#include <crt_externs.h>
#endif

/*
 * Ways of updating ps display:
 *
 * PS_USE_SETPROCTITLE
 *         use the function setproctitle(const char *, ...)
 *         (newer BSD systems)
 * PS_USE_PSTAT
 *         use the pstat(PSTAT_SETCMD, )
 *         (HPUX)
 * PS_USE_PS_STRINGS
 *         assign PS_STRINGS->ps_argvstr = "string"
 *         (some BSD systems)
 * PS_USE_CHANGE_ARGV
 *         assign argv[0] = "string"
 *         (some other BSD systems)
 * PS_USE_CLOBBER_ARGV
 *         write over the argv and environment area
 *         (Linux and most SysV-like systems)
 * PS_USE_WIN32
 *         push the string out as the name of a Windows event
 * PS_USE_NONE
 *         don't update ps display
 *         (This is the default, as it is safest.)
 */
#if defined(HAVE_SETPROCTITLE)
#define PS_USE_SETPROCTITLE
#elif defined(HAVE_SYS_PSTAT_H) && defined(PSTAT_SETCMD)
#define PS_USE_PSTAT
#elif defined(HAVE_PS_STRINGS)
#define PS_USE_PS_STRINGS
#elif defined(BSD) && !defined(DARWIN)
#define PS_USE_CHANGE_ARGV
#elif defined(__linux__) || defined(_AIX) || defined(__sgi) || (defined(sun) && !defined(BSD)) || defined(ultrix) || defined(__osf__) || defined(DARWIN)
#define PS_USE_CLOBBER_ARGV
#elif defined(POLAR_OS_WIN32)
#define PS_USE_WIN32
#else
#define PS_USE_NONE
#endif

/* Different systems want the buffer padded differently */
#if defined(_AIX) || defined(__linux__) || defined(DARWIN)
#define PS_PADDING '\0'
#else
#define PS_PADDING ' '
#endif

namespace polar {


#ifdef PS_USE_WIN32
static char windows_error_details[64];
static char sg_psBuffer[MAX_PATH];
static const size_t sg_psBufferSize = MAX_PATH;
#elif defined(PS_USE_CLOBBER_ARGV)
static char *sg_psBuffer;         /* will point to argv area */
static size_t sg_psBufferSize;   /* space determined at run time */
static char *sg_emptyEnviron[] = {0}; /* empty environment */
#else
#define PS_BUFFER_SIZE 256
static char sg_psBuffer[PS_BUFFER_SIZE];
static const size_t sg_psBufferSize = PS_BUFFER_SIZE;
#endif

static size_t sg_psBufferCurLen; /* actual string length in sg_psBuffer */

/* save the original argv[] location here */
static int sg_saveArgc;
static char** sg_saveArgv;

/*
 * This holds the 'locally' allocated environ from the save_ps_args method.
 * This is subsequently free'd at exit.
 */
#if defined(PS_USE_CLOBBER_ARGV)
static char** sg_frozenEnviron, **sg_newEnviron;
#endif

/*
 * Call this method early, before any code has used the original argv passed in
 * from main().
 * If needed, this code will make deep copies of argv and environ and return
 * these to the caller for further use. The original argv is then 'clobbered'
 * to store the process title.
 */
char** save_ps_args(int argc, char** argv)
{
   sg_saveArgc = argc;
   sg_saveArgv = argv;

#if defined(PS_USE_CLOBBER_ARGV)
   /*
     * If we're going to overwrite the argv area, count the available space.
     * Also move the environment to make additional room.
     */
   {
      char* end_of_area = nullptr;
      int non_contiguous_area = 0;
      int i;

      /*
         * check for contiguous argv strings
         */
      for (i = 0; (non_contiguous_area == 0) && (i < argc); i++)
      {
         if (i != 0 && end_of_area + 1 != argv[i])
            non_contiguous_area = 1;
         end_of_area = argv[i] + strlen(argv[i]);
      }

      /*
         * check for contiguous environ strings following argv
         */
      for (i = 0; (non_contiguous_area == 0) && (environ[i] != nullptr); i++)
      {
         if (end_of_area + 1 != environ[i])
            non_contiguous_area = 1;
         end_of_area = environ[i] + strlen(environ[i]);
      }

      if (non_contiguous_area != 0)
         goto clobber_error;

      sg_psBuffer = argv[0];
      sg_psBufferSize = end_of_area - argv[0];

      /*
         * move the environment out of the way
         */
      sg_newEnviron = (char **) malloc((i + 1) * sizeof(char *));
      sg_frozenEnviron = (char **) malloc((i + 1) * sizeof(char *));
      if (!sg_newEnviron || !sg_frozenEnviron)
         goto clobber_error;
      for (i = 0; environ[i] != nullptr; i++)
      {
         sg_newEnviron[i] = strdup(environ[i]);
         if (!sg_newEnviron[i])
            goto clobber_error;
      }
      sg_newEnviron[i] = nullptr;
      environ = sg_newEnviron;
      memcpy((char *)sg_frozenEnviron, (char *)sg_newEnviron, sizeof(char *) * (i + 1));

   }
#endif /* PS_USE_CLOBBER_ARGV */

#if defined(PS_USE_CHANGE_ARGV) || defined(PS_USE_CLOBBER_ARGV)
   /*
     * If we're going to change the original argv[] then make a copy for
     * argument parsing purposes.
     *
     * (NB: do NOT think to remove the copying of argv[]!
     * On some platforms, getopt() keeps pointers into the argv array, and
     * will get horribly confused when it is re-called to analyze a subprocess'
     * argument string if the argv storage has been clobbered meanwhile.
     * Other platforms have other dependencies on argv[].)
     */
   {
      char** new_argv;
      int i;

      new_argv = (char **) malloc((argc + 1) * sizeof(char *));
      if (!new_argv)
         goto clobber_error;
      for (i = 0; i < argc; i++)
      {
         new_argv[i] = strdup(argv[i]);
         if (!new_argv[i]) {
            free(new_argv);
            goto clobber_error;
         }
      }
      new_argv[argc] = nullptr;

#if defined(DARWIN)
      /*
         * Darwin (and perhaps other NeXT-derived platforms?) has a static
         * copy of the argv pointer, which we may fix like so:
         */
      *_NSGetArgv() = new_argv;
#endif

      argv = new_argv;

   }
#endif /* PS_USE_CHANGE_ARGV or PS_USE_CLOBBER_ARGV */

#if defined(PS_USE_CLOBBER_ARGV)
   {
      /* make extra argv slots point at end_of_area (a NUL) */
      int i;
      for (i = 1; i < sg_saveArgc; i++)
         sg_saveArgv[i] = sg_psBuffer + sg_psBufferSize;
   }
#endif /* PS_USE_CLOBBER_ARGV */

#ifdef PS_USE_CHANGE_ARGV
   sg_saveArgv[0] = sg_psBuffer; /* sg_psBuffer here is a static const array of size PS_BUFFER_SIZE */
   sg_saveArgv[1] = nullptr;
#endif /* PS_USE_CHANGE_ARGV */

   return argv;

#if defined(PS_USE_CHANGE_ARGV) || defined(PS_USE_CLOBBER_ARGV)
clobber_error:
   /* probably can't happen?!
     * if we ever get here, argv still points to originally passed
     * in argument
     */
   sg_saveArgv = nullptr;
   sg_saveArgc = 0;
   sg_psBuffer = nullptr;
   sg_psBufferSize = 0;
   return argv;
#endif /* PS_USE_CHANGE_ARGV or PS_USE_CLOBBER_ARGV */
}

/*
 * Returns PS_TITLE_SUCCESS if the OS supports this functionality
 * and the init function was called.
 * Otherwise returns NOT_AVAILABLE or NOT_INITIALIZED
 */
int is_ps_title_available()
{
#ifdef PS_USE_NONE
   return PS_TITLE_NOT_AVAILABLE; /* disabled functionality */
#endif

   if (!sg_saveArgv)
      return PS_TITLE_NOT_INITIALIZED;

#ifdef PS_USE_CLOBBER_ARGV
   if (!sg_psBuffer)
      return PS_TITLE_BUFFER_NOT_AVAILABLE;
#endif /* PS_USE_CLOBBER_ARGV */

   return PS_TITLE_SUCCESS;
}

/*
 * Convert error codes into error strings
 */
const char* ps_title_errno(int rc)
{
   switch(rc)
   {
   case PS_TITLE_SUCCESS:
      return "Success";

   case PS_TITLE_NOT_AVAILABLE:
      return "Not available on this OS";

   case PS_TITLE_NOT_INITIALIZED:
      return "Not initialized correctly";

   case PS_TITLE_BUFFER_NOT_AVAILABLE:
      return "Buffer not contiguous";

#ifdef PS_USE_WIN32
   case PS_TITLE_WINDOWS_ERROR:
      sprintf(windows_error_details, "Windows error code: %lu", GetLastError());
      return windows_error_details;
#endif
   }

   return "Unknown error code";
}

/*
 * Set a new process title.
 * Returns the appropriate error code if if there's an error
 * (like the functionality is compile time disabled, or the
 * save_ps_args() was not called.
 * Else returns 0 on success.
 */
int set_ps_title(const char* title)
{
   int rc = is_ps_title_available();
   if (rc != PS_TITLE_SUCCESS) {
      return rc;
   }
   strncpy(sg_psBuffer, title, sg_psBufferSize);
   sg_psBuffer[sg_psBufferSize - 1] = '\0';
   sg_psBufferCurLen = strlen(sg_psBuffer);

#ifdef PS_USE_SETPROCTITLE
   setproctitle("%s", sg_psBuffer);
#endif

#ifdef PS_USE_PSTAT
   {
      union pstun pst;

      pst.pst_command = sg_psBuffer;
      pstat(PSTAT_SETCMD, pst, sg_psBufferCurLen, 0, 0);
   }
#endif /* PS_USE_PSTAT */

#ifdef PS_USE_PS_STRINGS
   PS_STRINGS->ps_nargvstr = 1;
   PS_STRINGS->ps_argvstr = sg_psBuffer;
#endif /* PS_USE_PS_STRINGS */

#ifdef PS_USE_CLOBBER_ARGV
   /* pad unused memory */
   if (sg_psBufferCurLen < sg_psBufferSize)
   {
      memset(sg_psBuffer + sg_psBufferCurLen, PS_PADDING,
             sg_psBufferSize - sg_psBufferCurLen);
   }
#endif /* PS_USE_CLOBBER_ARGV */

#ifdef PS_USE_WIN32
   {
      wchar_t *sg_psBuffer_w = php_win32_cp_any_to_w(sg_psBuffer);

      if (!sg_psBuffer_w || !SetConsoleTitleW(sg_psBuffer_w)) {
         return PS_TITLE_WINDOWS_ERROR;
      }

      free(sg_psBuffer_w);
   }
#endif /* PS_USE_WIN32 */

   return PS_TITLE_SUCCESS;
}

/*
 * Returns the current sg_psBuffer value into string.  On some platforms
 * the string will not be null-terminated, so return the effective
 * length into *displen.
 * The return code indicates the error.
 */
int get_ps_title(int *displen, const char** string)
{
   int rc = is_ps_title_available();
   if (rc != PS_TITLE_SUCCESS) {
      return rc;
   }
#ifdef PS_USE_WIN32
   {
      wchar_t sg_psBuffer_w[MAX_PATH];
      char *tmp;

      if (!(sg_psBufferCurLen = GetConsoleTitleW(sg_psBuffer_w, (DWORD)sizeof(sg_psBuffer_w)))) {
         return PS_TITLE_WINDOWS_ERROR;
      }

      tmp = php_win32_cp_conv_w_to_any(sg_psBuffer_w, PHP_WIN32_CP_IGNORE_LEN, &sg_psBufferCurLen);
      if (!tmp) {
         return PS_TITLE_WINDOWS_ERROR;
      }

      sg_psBufferCurLen = sg_psBufferCurLen > sizeof(sg_psBuffer)-1 ? sizeof(sg_psBuffer)-1 : sg_psBufferCurLen;

      memmove(sg_psBuffer, tmp, sg_psBufferCurLen);
      free(tmp);
   }
#endif
   *displen = (int)sg_psBufferCurLen;
   *string = sg_psBuffer;
   return PS_TITLE_SUCCESS;
}

/*
 * Clean up the allocated argv and environ if applicable. Only call
 * this right before exiting.
 * This isn't needed per-se because the OS will clean-up anyway, but
 * having and calling this will ensure Valgrind doesn't output 'false
 * positives'.
 */
void cleanup_ps_args(char **argv)
{
#ifndef PS_USE_NONE
   if (sg_saveArgv) {
      sg_saveArgv = nullptr;
      sg_saveArgc = 0;

#ifdef PS_USE_CLOBBER_ARGV
      {
         for (int i = 0; sg_frozenEnviron[i] != nullptr; i++) {
            free(sg_frozenEnviron[i]);
         }
         free(sg_frozenEnviron);
         free(sg_newEnviron);
         /* leave a sane environment behind since some atexit() handlers
                call getenv(). */
         environ = sg_emptyEnviron;
      }
#endif /* PS_USE_CLOBBER_ARGV */

#if defined(PS_USE_CHANGE_ARGV) || defined(PS_USE_CLOBBER_ARGV)
      {
         for (int i=0; argv[i] != nullptr; i++) {
            free(argv[i]);
         }
         free(argv);
      }
#endif /* PS_USE_CHANGE_ARGV or PS_USE_CLOBBER_ARGV */
   }
#endif /* PS_USE_NONE */
   return;
}

} // polar
