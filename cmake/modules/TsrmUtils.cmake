# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

# =================================================================================
#  http://www.gnu.org/software/autoconf-archive/ax_func_which_gethostbyname_r.html
# =================================================================================
#
# SYNOPSIS
#
#   AX_FUNC_WHICH_GETHOSTBYNAME_R
#
# DESCRIPTION
#
#   Determines which historical variant of the gethostbyname_r() call
#   (taking three, five, or six arguments) is available on the system and
#   defines one of the following macros accordingly:
#
#     HAVE_FUNC_GETHOSTBYNAME_R_6
#     HAVE_FUNC_GETHOSTBYNAME_R_5
#     HAVE_FUNC_GETHOSTBYNAME_R_3
#
#   as well as
#
#     HAVE_GETHOSTBYNAME_R
#
#   If used in conjunction with gethostname.c, the API demonstrated in
#   test.c can be used regardless of which gethostbyname_r() is available.
#   These example files can be found at
#   http://www.csn.ul.ie/~caolan/publink/gethostbyname_r
#
#   based on David Arnold's autoconf suggestion in the threads faq
#
#   Originally named "AC_caolan_FUNC_WHICH_GETHOSTBYNAME_R". Rewritten for
#   Autoconf 2.5x, and updated for 2.68 by Daniel Richard G.
#
# LICENSE
#
#   Copyright (c) 2008 Caolan McNamara <caolan@skynet.ie>
#   Copyright (c) 2008 Daniel Richard G. <skunk@iskunk.org>
#
#   This program is free software; you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation; either version 2 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <http://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Archive. When you make and distribute a
#   modified version of the Autoconf Macro, you may extend this special
#   exception to the GPL to apply to your modified version as well.

#serial 7

macro(polar_check_func_which_gethostbyname_r)
   set(_funcWhichGethostbynameR "unknown")
   #
   # ONE ARGUMENT (sanity check)
   #

   # This should fail, as there is no variant of gethostbyname_r() that takes
   # a single argument. If it actually compiles, then we can assume that
   # netdb.h is not declaring the function, and the compiler is thereby
   # assuming an implicit prototype. In which case, we're out of luck.
   #
   check_c_source_compiles("
      #include <netdb.h>
      int main() {
      char *name = \"www.gnu.org\";
      (void)gethostbyname_r(name);
      return 0;
      }" CheckGetHostByNameOneArg)
   if (CheckGetHostByNameOneArg)
      set(_funcWhichGethostbynameR OFF)
   endif()

   #
   # SIX ARGUMENTS
   # (e.g. Linux)
   #
   if (_funcWhichGethostbynameR STREQUAL "unknown")
      check_c_source_compiles("
         #include <netdb.h>
         int main() {
         char *name = \"www.gnu.org\";
         struct hostent ret, *retp;
         char buf[1024];
         int buflen = 1024;
         int my_h_errno;
         (void)gethostbyname_r(name, &ret, buf, buflen, &retp, &my_h_errno);
         return 0;
         }" CheckGetHostByNameSixArgs)
   endif()
   if (CheckGetHostByNameSixArgs)
      set(_funcWhichGethostbynameR "six")
   endif()

   #
   # FIVE ARGUMENTS
   # (e.g. Solaris)
   #
   if (_funcWhichGethostbynameR STREQUAL "unknown")
      check_c_source_compiles("
         #include <netdb.h>
         int main() {
         char *name = \"www.gnu.org\";
         struct hostent ret;
         char buf[1024];
         int buflen = 1024;
         int my_h_errno;
         (void)gethostbyname_r(name, &ret, buf, buflen, &my_h_errno);
         return 0;
         }" CheckGetHostByNameFiveArgs)
   endif()
   if (CheckGetHostByNameFiveArgs)
      set(_funcWhichGethostbynameR "five")
   endif()

   #
   # THREE ARGUMENTS
   # (e.g. AIX, HP-UX, Tru64)
   #
   if (_funcWhichGethostbynameR STREQUAL "unknown")
      check_c_source_compiles("
         #include <netdb.h>
         int main() {
         char *name = \"www.gnu.org\";
         struct hostent ret;
         struct hostent_data data;
         (void)gethostbyname_r(name, &ret, &data) /* ; */
         return 0;
         }" CheckGetHostByNameThreeArgs)
   endif()
   if (CheckGetHostByNameThreeArgs)
      set(_funcWhichGethostbynameR "three")
   endif()
   ################################################################
   if (_funcWhichGethostbynameR STREQUAL "three" OR _funcWhichGethostbynameR STREQUAL "five" OR _funcWhichGethostbynameR STREQUAL "six")
      set(HAVE_GETHOSTBYNAME_R ON)
   endif()
   if (_funcWhichGethostbynameR STREQUAL "three")
      set(HAVE_FUNC_GETHOSTBYNAME_R_3 ON)
   elseif (_funcWhichGethostbynameR STREQUAL "five")
      set(HAVE_FUNC_GETHOSTBYNAME_R_5 ON)
   elseif (_funcWhichGethostbynameR STREQUAL "six")
      set(HAVE_FUNC_GETHOSTBYNAME_R_6 ON)
   elseif(_funcWhichGethostbynameR STREQUAL "no")
      message("cannot find function declaration in netdb.h")
   elseif(_funcWhichGethostbynameR STREQUAL "unkonw")
      message("can't tell")
   endif()
endmacro()

macro(polar_tsrm_basic_check)
   polar_check_headers(stdarg.h)
   check_function_exists(sigprocmask HAVE_SIGPROCMASK)
   polar_check_func_which_gethostbyname_r()
endmacro()

macro(polar_tsrm_check_threads)
   if (POLAR_BEOS_THREADS)
      # Whether to use native BeOS threads
      set(BETHREADS ON)
   else()
      if (NOT POLAR_THREADS_WORKING)
         message(FATAL_ERROR "Your system seems to lack POSIX threads.")
      endif()
      # Whether to use Pthreads
      set(PTHREADS ON)
      message("using pthread implementation")
   endif()
endmacro()

macro(polar_tsrm_check_pth)
   message("checking GNU Pth library")
   find_program(_havePthCfg "${POLAR_WITH_TSRM_PTH_CONFIG}")
   if (NOT _havePthCfg)
      message(FATAL_ERROR "${POLAR_WITH_TSRM_PTH_CONFIG} is not found, Please check your Pth installation")
   endif()
   execute_process(COMMAND ${POLAR_WITH_TSRM_PTH_CONFIG} --prefix
      RESULT_VARIABLE _retCode
      OUTPUT_VARIABLE _output
      ERROR_VARIABLE _errorMsg)
   if (NOT _retCode EQUAL 0)
      message(FATAL_ERROR "run ${POLAR_WITH_TSRM_PTH_CONFIG} --prefix error: ${_errorMsg}")
   endif()
   execute_process(COMMAND ${POLAR_WITH_TSRM_PTH_CONFIG} --cflags
      RESULT_VARIABLE _retCode
      OUTPUT_VARIABLE _output
      ERROR_VARIABLE _errorMsg)
   if (NOT _retCode EQUAL 0)
      message(FATAL_ERROR "run ${POLAR_WITH_TSRM_PTH_CONFIG} --cflags error: ${_errorMsg}")
   endif()
   polar_append_flag(${_output} CMAKE_CXX_FLAGS)
   execute_process(COMMAND ${POLAR_WITH_TSRM_PTH_CONFIG} --ldflags
      RESULT_VARIABLE _retCode
      OUTPUT_VARIABLE _output
      ERROR_VARIABLE _errorMsg)
   if (NOT _retCode EQUAL 0)
      message(FATAL_ERROR "run ${POLAR_WITH_TSRM_PTH_CONFIG} --ldflags error: ${_errorMsg}")
   endif()
   link_directories(${_output})
   set(GNUPTH ON)
endmacro()

macro(polar_tsrm_threads_checks)
   # For the thread implementations, we always use --with-*
   # to maintain consistency
   if(POLAR_TSRM_USE_PTHREADS)
      polar_tsrm_check_threads()
   elseif (POLAR_TSRM_USE_PTH)
      polar_tsrm_check_pth()
   endif()
endmacro()
