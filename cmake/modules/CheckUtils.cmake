# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

include(CheckCSourceRuns)
include(CheckFunctionExists)
include(CheckCSourceCompiles)
include(CheckStructHasMember)
include(CheckVariableExists)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckPrototypeDefinition)

macro(polar_include_default includes)
   set(${includes} "
      #include <stdio.h>
      #ifdef HAVE_SYS_TYPES_H
      # include <sys/types.h>
      #endif
      #ifdef HAVE_SYS_STAT_H
      # include <sys/stat.h>
      #endif
      #ifdef STDC_HEADERS
      # include <stdlib.h>
      # include <stddef.h>
      #else
      # ifdef HAVE_STDLIB_H
      #  include <stdlib.h>
      # endif
      #endif
      #ifdef HAVE_STRING_H
      # if !defined STDC_HEADERS && defined HAVE_MEMORY_H
      #  include <memory.h>
      # endif
      # include <string.h>
      #endif
      #ifdef HAVE_STRINGS_H
      # include <strings.h>
      #endif
      #ifdef HAVE_INTTYPES_H
      # include <inttypes.h>
      #endif
      #ifdef HAVE_STDINT_H
      # include <stdint.h>
      #endif
      #ifdef HAVE_UNISTD_H
      # include <unistd.h>
      #endif
      #include <sys/types.h>
      #include <sys/stat.h>
      #include <stdlib.h>
      #include <string.h>
      #include <memory.h>
      #include <strings.h>
      ")

endmacro()

macro(polar_check_ebcdic)
   check_c_source_runs("int main(void) {
      return (unsigned char)'A' != (unsigned char)0xC1;
      }" CheckingEBCDIC)
   if (CheckingEBCDIC)
      set(POLAR_CHARSET_EBCDIC ON)
   endif()
endmacro()

macro(polar_check_prog_awk)
   find_program(POLOAR_PROGRAM_AWK awk NAMES gawk nawk mawk
      PATHS /usr/xpg4/bin/)
   if (NOT POLOAR_PROGRAM_AWK)
      message(FATAL_ERROR "Could not find awk; Install GNU awk")
   else()
      if(POLOAR_PROGRAM_AWK MATCHES ".*mawk")
         message(WARNING "mawk is known to have problems on some systems. You should install GNU awk")
      else()
         message(STATUS "checking wether ${POLOAR_PROGRAM_AWK} is broken")
         execute_process(COMMAND ${POLOAR_PROGRAM_AWK} "function foo() {}" ">/dev/null 2>&1"
            RESULT_VARIABLE _awkExecRet)
         if (_awkExecRet)
            message(FATAL_ERROR "You should install GNU awk")
            unset(POLOAR_PROGRAM_AWK)
         else()
            message("${POLOAR_PROGRAM_AWK} is works")
         endif()
      endif()
   endif()
endmacro()

macro(polar_check_write_stdout)
   check_c_source_runs("
      #ifdef HAVE_UNISTD_H
      #include <unistd.h>
      #endif

      #define TEXT \"This is the test message -- \"

      main()
      {
      int n;

      n = write(1, TEXT, sizeof(TEXT)-1);
      return (!(n == sizeof(TEXT)-1));
      }" CheckingWriteStdout)
   if (CheckingWriteStdout)
      set(POLAR_WRITE_STDOUT ON)
      set(PHP_WRITE_STDOUT ON)
   endif()
endmacro()

macro(polar_check_headers)
   foreach(_filename ${ARGV})
      polar_generate_header_guard(${_filename} _guardName)
      check_include_file(${_filename} ${_guardName})
      if (${${_guardName}})
         set(POLAR_${_guardName} ON)
      endif()
   endforeach()
endmacro()

macro(polar_check_c_const)
   file(READ ${POLAR_CMAKE_TEST_CODE_DIR}/test_const_code.c _testcode)
   check_c_source_runs("${_testcode}" checkConstSupport)
   if (NOT checkConstSupport)
      set(const "")
   endif()
endmacro()

macro(polar_check_fopen_cookie)
   message("checking fopen cookie")
   check_function_exists(fopencookie HAVE_FOPENCOOKIE)
   # this comes in two flavors:
   # newer glibcs (since 2.1.2 ? )
   # have a type called cookie_io_functions_t
   if (HAVE_FOPENCOOKIE)
      check_c_source_compiles("
         #define _GNU_SOURCE
         #include <stdio.h>
         int main() {
         cookie_io_functions_t cookie;
         }" CheckCookieIoFunctionType)
      if (CheckCookieIoFunctionType)
         set(HAVE_COOKIE_IO_FUNCTIONS_T)
         set(POLAR_COOKIE_IO_FUNCTIONS_T "cookie_io_functions_t")
         set(HAVE_FOPEN_COOKIE ON)
         # even newer glibcs have a different seeker definition...
         check_c_source_runs(
            "
            #define _GNU_SOURCE
            #include <stdio.h>

            struct cookiedata {
            __off64_t pos;
            };

            __ssize_t reader(void *cookie, char *buffer, size_t size)
            { return size; }
            __ssize_t writer(void *cookie, const char *buffer, size_t size)
            { return size; }
            int closer(void *cookie)
            { return 0; }
            int seeker(void *cookie, __off64_t *position, int whence)
            { ((struct cookiedata*)cookie)->pos = *position; return 0; }

            cookie_io_functions_t funcs = {reader, writer, seeker, closer};

            main() {
            struct cookiedata g = { 0 };
            FILE *fp = fopencookie(&g, \"r\", funcs);

            if (fp && fseek(fp, 8192, SEEK_SET) == 0 && g.pos == 8192)
               exit(0);
               exit(1);
               }" checkCookieIoFunctionsUseOff64Type)
         if (checkCookieIoFunctionsUseOff64Type)
            set(POLAR_COOKIE_IO_FUNCTIONS_USE_OFFSET_64_T ON)
            set(COOKIE_IO_FUNCTIONS_USE_OFFSET_64_T ON)
         endif()
      endif()
   else()
      # older glibc versions (up to 2.1.2 ?)
      # call it _IO_cookie_io_functions_t
      check_c_source_compiles("
         #define _GNU_SOURCE
         #include <stdio.h>
         int main() {
         _IO_cookie_io_functions_t cookie;
         }" CheckIoCookieIoFunctionType)
      if (CheckIoCookieIoFunctionType)
         set(HAVE_IO_COOKIE_IO_FUNCTIONS_T)
         set(POLAR_COOKIE_IO_FUNCTIONS_T "_IO_cookie_io_functions_t")
         set(HAVE_FOPEN_COOKIE ON)
      endif()
   endif()
   if (HAVE_FOPEN_COOKIE)
      set(HAVE_FOPEN_COOKIE ${HAVE_FOPEN_COOKIE})
      set(COOKIE_IO_FUNCTIONS_T ${POLAR_COOKIE_IO_FUNCTIONS_T})
      if (POLAR_COOKIE_IO_FUNCTIONS_USE_OFFSET_64_T)
         set(POLAR_COOKIE_IO_FUNCTIONS_USE_OFFSET_64_T ${POLAR_COOKIE_IO_FUNCTIONS_USE_OFFSET_64_T})
         set(COOKIE_IO_FUNCTIONS_USE_OFFSET_64_T ${POLAR_COOKIE_IO_FUNCTIONS_USE_OFFSET_64_T})
      endif()
      message("fopen cookie type : ${COOKIE_IO_FUNCTIONS_T}")
   else()
      message("fopen cookie not found")
   endif()
endmacro()

macro(polar_check_broken_getcwd)
   message("checking wether getcwd broken")
   if (POLAR_SYSTEM_NORMAL_NAME MATCHES ".*sunos.*")
      set(HAVE_BROKEN_GETCWD ON)
      message("getcwd is broken")
   else()
       message("getcwd is works")
   endif()
endmacro()

macro(polar_check_broken_glic_fopen_append)
   message("check wether broken libc stdio")
   file(READ ${POLAR_CMAKE_TEST_CODE_DIR}/test_broken_glibc_fopen_append_code.c _testcode)
   check_c_source_runs("${_testcode}" checkGlibcFopenAppendRun)
   if (checkGlibcFopenAppendRun)
      set(haveBrokenGlibcFopenAppend OFF)
   else()
      set(haveBrokenGlibcFopenAppend ON)
   endif()
   check_c_source_compiles("
      #include <features.h>
      int main() {
      #if !__GLIBC_PREREQ(2,2)
      choke me
      #endif
      }" checkGlibcFopenAppendCompile)
   if (checkGlibcFopenAppendCompile)
      set(haveBrokenGlibcFopenAppend ON)
   else()
      set(haveBrokenGlibcFopenAppend OFF)
   endif()
   if(haveBrokenGlibcFopenAppend)
      set(HAVE_BROKEN_GLIBC_FOPEN_APPEND ON)
   else()
      message("libc stdio fopen append works")
   endif()
endmacro()

macro(polar_check_broken_sprintf)
   check_c_source_runs("main() {char buf[20];int code = sprintf(buf,\"testing 123\")!=11 ? 0 : 1;exit(code); }"
      checkBrokenSprintf)
   if (checkBrokenSprintf)
      set(ZEND_BROKEN_SPRINTF ON)
   endif()
endmacro()

macro(polar_check_type_struct_tm)
   message("whether struct tm is in sys/time.h or time.h")
   check_c_source_runs(
      "#include <sys/types.h>
      #include <time.h>
      int main(){
         struct tm tm;
         int *p = &tm.tm_sec;
         return !p;
      }" CheckStructTm)
   if (CheckStructTm)
      set(POLAR_TYPE_STRUCT_TM_TYPE "time.h")
   else()
      set(POLAR_TYPE_STRUCT_TM_TYPE "sys/time.h")
   endif()
   if (POLAR_TYPE_STRUCT_TM_TYPE STREQUAL "sys/time.h")
      set(TM_IN_SYS_TIME ON)
   endif()
   message("struct tm in header ${POLAR_TYPE_STRUCT_TM_TYPE}")
endmacro()

# Figure out how to get the current timezone.  If `struct tm' has a
# `tm_zone' member, define `HAVE_TM_ZONE'.  Otherwise, if the
# external array `tzname' is found, define `HAVE_TZNAME'.
macro(polar_check_type_struct_timezone)
   message("checking strcut timezone")
   check_struct_has_member("struct tm" tm_zone "sys/types.h;${POLAR_TYPE_STRUCT_TM_TYPE}" CheckStructTimeZone LANGUAGE C)
   if (CheckStructTimeZone)
      set(HAVE_STRUCT_TM_TM_ZONE ON)
      message("strcut timezone found")
   else()
      check_variable_exists(tzname haveTzName)
      if (haveTzName)
         check_c_source_runs(
            "#include <time.h>
            #if !HAVE_DECL_TZNAME
            extern char *tzname[];
            #endif
            int main(){
               tzname[0][0];
               return 0;
            }" CheckTzName)
         if (CheckTzName)
            set(HAVE_TZNAME ON)
         endif()
      endif()
   endif()
endmacro()

macro(polar_check_missing_time_r_decl)
   message("checking missing declarations of reentrant functions")
   check_c_source_compiles("
      #include <time.h>
      int main(){struct tm *(*func)() = localtime_r;return 0;}"
      checkLocalTimeR)
   if (NOT checkLocalTimeR)
      set(MISSING_LOCALTIME_R_DECL ON)
   endif()

   check_c_source_compiles("
      #include <time.h>
      int main(){struct tm *(*func)() = gmtime_r;return 0;}"
      checkGmTimeR)
   if (NOT checkGmTimeR)
      set(MISSING_GMTIME_R_DECL ON)
   endif()

   check_c_source_compiles("
      #include <time.h>
      int main(){struct tm *(*func)() = asctime_r;return 0;}"
      checkAscTimeR)
   if (NOT checkAscTimeR)
      set(MISSING_ASCTIME_R_DECL ON)
   endif()

   check_c_source_compiles("
      #include <time.h>
      int main(){struct tm *(*func)() = ctime_r;return 0;}"
      checkcTimeR)
   if (NOT checkcTimeR)
      set(MISSING_CTIME_R_DECL ON)
   endif()

   check_c_source_compiles("
      #include <time.h>
      int main(){struct tm *(*func)() = strtok_r;return 0;}"
      checkStrTokR)
   if (NOT checkStrTokR)
      set(MISSING_STRTOK_R_DECL ON)
   endif()
endmacro()

# See if we have broken header files like SunOS has.

macro(polar_check_missing_fclose_decl)
   message("checking fclose declaration")
   check_c_source_compiles("
      #include <stdio.h>
      int main(){int (*func)() = fclose;return 0;}"
      checkFClose)
   if (checkFClose)
      set(MISSING_FCLOSE_DECL OFF)
   else()
      set(MISSING_FCLOSE_DECL ON)
      message("fclose declaration found")
   endif()
endmacro()

macro(polar_check_tm_gmtoff)
   message("checking tm_gmtoff in struct tm")
   check_c_source_compiles("
      #include <sys/types.h>
      #include <${POLAR_TYPE_STRUCT_TM_TYPE}>
      int main(){
         struct tm tm;
         tm.tm_gmtoff;
         return 0;
      }"
      checkTmGmtOff)
   if(checkTmGmtOff)
      set(HAVE_TM_GMTOFF ON)
   endif()
endmacro()

macro(polar_check_struct_flock)
   check_c_source_compiles("
      #include <unistd.h>
      #include <fcntl.h>
      int main(){
         struct flock x;
         return 0;
      }"
      checkStructFlock)
   if (checkStructFlock)
      set(HAVE_STRUCT_FLOCK ON)
   endif()
endmacro()

macro(polar_check_socklen_type)
   check_c_source_compiles("
      #include <sys/types.h>
      #include <sys/socket.h>
      int main(){
         socklen_t x;
         return 0;
      }"
      checkSockLength)
   if(checkSockLength)
      set(HAVE_SOCKLEN_T ON)
   endif()
endmacro()

function(polar_check_type type var headers)
   set(includes "")
   foreach(header ${headers})
      set(includes "${includes}\n#include <${header}>")
   endforeach()
   if (HAVE_STDINT_H)
      set(includes "${includes}\n#include <stdint.h>")
   endif()
   if(HAVE_SYS_TYPES_H)
      set(includes "${includes}\n#include <sys/types.h>")
   endif()
   check_c_source_compiles("
      ${includes}
      int main(){
         ${type} value;
         return 0;
      }"
      ${var})
endfunction()

macro(polar_check_stdint_types)
   check_type_size(short SIZEOF_LONG)
   check_type_size(int SIZEOF_INT)
   check_type_size(long SIZEOF_LONG)
   check_type_size("long long" SIZEOF_LONG_LONG)

   polar_check_type(int8 HAVE_INT8 "")
   polar_check_type(int16 HAVE_INT16 "")
   polar_check_type(int32 HAVE_INT32 "")
   polar_check_type(int64 HAVE_INT64 "")

   polar_check_type(int8_t HAVE_INT8_T "")
   polar_check_type(int16_t HAVE_INT16_T "")
   polar_check_type(int32_t HAVE_INT32_T "")
   polar_check_type(int64_t HAVE_INT64_T "")

   polar_check_type(uint8 HAVE_UINT8 "")
   polar_check_type(uint16 HAVE_UINT16 "")
   polar_check_type(uint32 HAVE_UINT32 "")
   polar_check_type(uint64 HAVE_UINT64 "")

   polar_check_type(uint8_t HAVE_UINT8_T "")
   polar_check_type(uint16_t HAVE_UINT16_T "")
   polar_check_type(uint32_t HAVE_UINT32_T "")
   polar_check_type(uint64_t HAVE_UINT64_T "")

   polar_check_type(u_int8_t HAVE_U_INT8_T "")
   polar_check_type(u_int16_t HAVE_U_INT16_T "")
   polar_check_type(u_int32_t HAVE_U_INT32_T "")
   polar_check_type(u_int64_t HAVE_U_INT64_T "")

   if (HAVE_INT8 OR HAVE_INT8 OR HAVE_INT8_T OR HAVE_UINT8 OR HAVE_UINT8_T OR HAVE_U_INT8_T)
      set(PHP_HAVE_STDINT_TYPES ON)
      set(POLAR_HAVE_STDINT_TYPES ON)
   endif()
endmacro()

macro(polar_check_builtin_expect)
   check_c_source_runs("
      int main(){int value = __builtin_expect(1,1) ? 1 : 0;return 0;}"
      checkBuiltinExpect)
   if (checkBuiltinExpect)
      set(PHP_HAVE_BUILTIN_EXPECT ON)
      set(POLAR_HAVE_BUILTIN_EXPECT ON)
   endif()
endmacro()

macro(polar_check_builtin_clz)
   check_c_source_runs("
      int main(){int value = __builtin_clz(1) ? 1 : 0;;return 0;}"
      checkBuiltinClz)
   if (checkBuiltinClz)
      set(PHP_HAVE_BUILTIN_CLZ ON)
      set(POLAR_HAVE_BUILTIN_CLZ ON)
   endif()
endmacro()

macro(polar_check_builtin_ctzl)
   check_c_source_runs("
      int main(){int value = __builtin_ctzl(2L) ? 1 : 0;return 0;}"
      checkBuiltinCtzl)
   if (checkBuiltinCtzl)
      set(PHP_HAVE_BUILTIN_CTZL ON)
      set(POLAR_HAVE_BUILTIN_CTZL ON)
   endif()
endmacro()

macro(polar_check_builtin_ctzll)
   check_c_source_runs("
      int main(){int value = __builtin_ctzll(2LL) ? 1 : 0;return 0;}"
      checkBuiltinCtzll)
   if (checkBuiltinCtzll)
      set(PHP_HAVE_BUILTIN_CTZLL ON)
      set(POLAR_HAVE_BUILTIN_CTZLL ON)
   endif()
endmacro()

macro(polar_check_builtin_smull_overflow)
   check_c_source_runs("
      int main(){
         long tmpvar;
         __builtin_smull_overflow(3, 7, &tmpvar);
         return 0;
      }"
      checkBuiltinSmullOverflow)
   if (checkBuiltinSmullOverflow)
      set(PHP_HAVE_BUILTIN_SMULL_OVERFLOW ON)
      set(POLAR_HAVE_BUILTIN_SMULL_OVERFLOW ON)
   endif()
endmacro()

macro(polar_check_builtin_smulll_overflow)
   check_c_source_runs("
      int main(){
         long long tmpvar;
         __builtin_smulll_overflow(3, 7, &tmpvar);
         return 0;
      }"
      checkBuiltinSmulllOverflow)
   if (checkBuiltinSmulllOverflow)
      set(PHP_HAVE_BUILTIN_SMULLL_OVERFLOW ON)
      set(POLAR_HAVE_BUILTIN_SMULLL_OVERFLOW ON)
   endif()
endmacro()

macro(polar_check_builtin_saddl_overflow)
   check_c_source_runs("
      int main(){
         long tmpvar;
         __builtin_saddl_overflow(3, 7, &tmpvar);
         return 0;
      }"
      checkBuiltinSaddlverflow)
   if (checkBuiltinSaddlverflow)
      set(PHP_HAVE_BUILTIN_SADDL_OVERFLOW ON)
      set(POLAR_HAVE_BUILTIN_SADDL_OVERFLOW ON)
   endif()
endmacro()

macro(polar_check_builtin_saddll_overflow)
   check_c_source_runs("
      int main(){
         long tmpvar;
         __builtin_saddll_overflow(3, 7, &tmpvar);
         return 0;
      }"
      checkBuiltinSaddllverflow)
   if (checkBuiltinSaddllverflow)
      set(PHP_HAVE_BUILTIN_SADDLL_OVERFLOW ON)
      set(POLAR_HAVE_BUILTIN_SADDLL_OVERFLOW ON)
   endif()
endmacro()

macro(polar_check_builtin_ssubl_overflow)
   check_c_source_runs("
      int main(){
         long tmpvar;
         __builtin_ssubl_overflow(3, 7, &tmpvar);
         return 0;
      }"
      checkBuiltinSSublverflow)
   if (checkBuiltinSSublverflow)
      set(PHP_HAVE_BUILTIN_SSUBL_OVERFLOW ON)
      set(POLAR_HAVE_BUILTIN_SSUBL_OVERFLOW ON)
   endif()
endmacro()

macro(polar_check_builtin_ssubll_overflow)
   check_c_source_runs("
      int main(){
         long tmpvar;
         __builtin_ssubll_overflow(3, 7, &tmpvar);
         return 0;
      }"
      checkBuiltinSSubllverflow)
   if (checkBuiltinSSubllverflow)
      set(PHP_HAVE_BUILTIN_SSUBLL_OVERFLOW ON)
      set(POLAR_HAVE_BUILTIN_SSUBLL_OVERFLOW ON)
   endif()
endmacro()

macro(polar_check_type_uid_type)
   check_type_size(gid_t GID_T LANGUAGE C)
   check_type_size(uid_t UID_T LANGUAGE C)
   if (NOT HAVE_UID_T)
      set(uid_t int)
   endif()
   if (NOT HAVE_GID_T)
      set(gid_t int)
   endif()
endmacro()

macro(polar_check_type_size_t)
   check_type_size(int SIZE_T LANGUAGE C)
   if (NOT HAVE_SIZE_T)
      set(size_t "unsigned int")
   endif()
endmacro()

# Check for struct sockaddr_storage exists
macro(polar_check_sockaddr)
   check_c_source_runs("
      #include <sys/types.h>
      #include <sys/socket.h>
      int main(){
         struct sockaddr_storage s;s;
         return 0;
      }"
      checkSockAddrStorage)
   if (checkSockAddrStorage)
      set(HAVE_SOCKADDR_STORAGE ON)
   endif()
   # Check if field sa_len exists in struct sockaddr
   check_c_source_runs("
      #include <sys/types.h>
      #include <sys/socket.h>
      int main(){
         static struct sockaddr sa;
         int n = (int) sa.sa_len;
         return 0;
      }"
      checkSockaddrSaLen)
   if(checkSockaddrSaLen)
      set(HAVE_SOCKADDR_SA_LEN ON)
   endif()
endmacro()

macro(polar_check_have_ipv6_support)
   check_c_source_runs("
      #include <sys/types.h>
      #include <sys/socket.h>
      #include <netinet/in.h>
      int main(){
         struct sockaddr_in6 s;
         struct in6_addr t=in6addr_any;
         int i=AF_INET6;
         s;
         t.s6_addr[0] = 0;
         return 0;
      }"
      checkHaveIpV6Support)
   if(checkHaveIpV6Support)
      set(HAVE_IPV6_SUPPORT ON)
   endif()
endmacro()

macro(polar_check_func_vprintf)
   check_function_exists(vprintf HAVE_VPRINTF)
   set(POLAR_HAVE_VPRINTF ${HAVE_VPRINTF})
   if (NOT HAVE_VPRINTF)
      check_function_exists(_doprnt HAVE_DOPRNT)
   endif()
endmacro()

macro(polar_check_funcs)
   foreach(_func ${ARGV})
      string(TOUPPER ${_func} upcase)
      check_function_exists(${_func} HAVE_${upcase})
      if (${HAVE_${upcase}})
         set(POLAR_HAVE_${upcase} ON)
      endif()
   endforeach()
endmacro()

macro(polar_check_func_utime_null)
   polar_include_default(headers)
   set(_tempFilename ${CMAKE_CURRENT_BINARY_DIR}/conftest.data)
   file(TOUCH ${_tempFilename})
   check_c_source_runs(
      "${headers}
      #ifdef HAVE_UTIME_H
         # include <utime.h>
      #endif
      int main(){
      struct stat s, t;
        return ! (stat (\"${_tempFilename}\", &s) == 0
                && utime (\"${_tempFilename}\", 0) == 0
                && stat (\"${_tempFilename}\", &t) == 0
                && t.st_mtime >= s.st_mtime
                && t.st_mtime - s.st_mtime < 120);
      }" checkHaveUtimeNull)
   file(REMOVE ${_tempFilename})
   if (checkHaveUtimeNull)
      set(HAVE_UTIME_NULL ON)
   endif()
endmacro()

macro(polar_check_func_alloca)
   # The Ultrix 4.2 mips builtin alloca declared by alloca.h only works
   # for constant arguments.  Useless!
   check_c_source_runs(
      "#include <alloca.h>
      int main(){
         char *p = (char *) alloca (2 * sizeof (int));
         if (p) return 0;
      }" checkWorkingAllocaH)
   if (checkWorkingAllocaH)
      set(HAVE_ALLOCA_H ON)
   endif()
   check_c_source_runs(
      "#ifdef __GNUC__
      # define alloca __builtin_alloca
      #else
      # ifdef _MSC_VER
      #  include <malloc.h>
      #  define alloca _alloca
      # else
      #  ifdef HAVE_ALLOCA_H
      #   include <alloca.h>
      #  else
      #   ifdef _AIX
       #pragma alloca
      #   else
      #    ifndef alloca /* predefined by HP cc +Olibcalls */
      void *alloca (size_t);
      #    endif
      #   endif
      #  endif
      # endif
      #endif
      int main(){
         char *p = (char *) alloca (1);
         if (p) return 0;
      }" checkFuncAllocaWorks)
   if (checkFuncAllocaWorks)
      set(HAVE_ALLOCA ON)
   else()
      message(FATAL_ERROR "missing alloca function")
   endif()
endmacro()

macro(polar_check_declare_timezone)
   check_c_source_runs(
      "#include <sys/types.h>
       #include <time.h>
       #ifdef HAVE_SYS_TIME_H
         #include <sys/time.h>
       #endif
       int main() {
         time_t foo = (time_t) timezone;
         return 0;
       }
      " checkDeclaredTimeZone)
   if (checkDeclaredTimeZone)
      set(HAVE_DECLARED_TIMEZONE ON)
   endif()
endmacro()

# Check type of reentrant time-related functions
# Type can be: irix, hpux or POSIX
macro(polar_check_time_r_type)
   set(_timeRType posix)
   check_c_source_runs(
      "#include <time.h>
      main() {
         char buf[27];
         struct tm t;
         time_t old = 0;
         int r, s;

         s = gmtime_r(&old, &t);
         r = (int) asctime_r(&t, buf, 26);
         if (r == s && s == 0) return (0);
         return (1);
      }
      " checkTimeRTypeHpux)
   if (checkTimeRTypeHpux)
      set(_timeRType hpux)
   else()
      check_c_source_runs(
         "#include <time.h>
         main() {
           struct tm t, *s;
           time_t old = 0;
           char buf[27], *p;

           s = gmtime_r(&old, &t);
           p = asctime_r(&t, buf, 26);
           if (p == buf && s == &t) return (0);
           return (1);
         }" checkTimeRTypeIrix)
      if (checkTimeRTypeIrix)
         set(_timeRType irix)
      endif()
   endif()
   if (_timeRType STREQUAL "hpux")
      set(PHP_HPUX_TIME_R ON)
   elseif(_timeRType STREQUAL "irix")
      set(PHP_IRIX_TIME_R ON)
   else()
      set(PHP_POSIX_TIME_R ON)
   endif()
endmacro()

macro(polar_check_readdir_r_type)
   # HAVE_READDIR_R is also defined by libmysql
   check_function_exists(readdir_r checkFuncReadDirR)
   set(_whatReadDirR no)
   if (checkFuncReadDirR)
      check_c_source_runs(
         "#define _REENTRANT
         #include <sys/types.h>
         #include <dirent.h>
         #ifndef PATH_MAX
         #define PATH_MAX 1024
         #endif
         main() {
           DIR *dir;
           char entry[sizeof(struct dirent)+PATH_MAX];
           struct dirent *pentry = (struct dirent *) &entry;
           dir = opendir(\"/\");
           if (!dir)
             exit(1);
           if (readdir_r(dir, (struct dirent *) entry, &pentry) == 0) {
             close(dir);
             exit(0);
           }
           close(dir);
           exit(1);
         }" checkFuncReadDirRPosix)
      if (checkFuncReadDirRPosix)
         set(_whatReadDirR posix)
      else()
         check_cxx_source_runs("
            #define _REENTRANT
            #include <sys/types.h>
            #include <dirent.h>
            int readdir_r(DIR *, struct dirent *);
            int main(){return 0;}
            " checkFuncReadDirROldStyle)
         if (checkFuncReadDirROldStyle)
            set(_whatReadDirR oldstyle)
         endif()
      endif()
   endif()
   if (_whatReadDirR STREQUAL "posix")
      set(HAVE_POSIX_READDIR_R ON)
   elseif(if (_whatReadDirR STREQUAL "oldstyle"))
      set(HAVE_OLD_READDIR_R ON)
   endif()
endmacro()

macro(polar_check_in_addr_t)
   # AIX keeps in_addr_t in /usr/include/netinet/in.h
   if(HAVE_NETINET_IN_H)
      list(APPEND _headers netinet/in.h)
   endif()
   if (HAVE_ARPA_INET_H)
      list(APPEND _headers arpa/inet.h)
   endif()
   polar_check_type(in_addr_t HAVE_IN_ADDR_T "${_headers}")
   if (NOT HAVE_IN_ADDR_T)
      set(in_addr_t "unsigned int")
   endif()
endmacro()

# detect the style of crypt_r() is any is available
# see APR_CHECK_CRYPT_R_STYLE() for original version
macro(polar_crypt_r_style)
   set(_cryptRStyle "none")
   check_c_source_runs("
      #define _REENTRANT 1
      #include <crypt.h>
      int main() {
         CRYPTD buffer;
         crypt_r(\"passwd\", \"hash\", &buffer);
         return 0;
      }
      " checkCryptRStyleCryptd)
   if (checkCryptRStyleCryptd)
      set(_cryptRStyle "cryptd")
   endif()
   if (_cryptRStyle STREQUAL "none")
      check_c_source_runs("
         #define _REENTRANT 1
         #include <crypt.h>
         int main() {
            struct crypt_data buffer;
            crypt_r(\"passwd\", \"hash\", &buffer);
            return 0;
         }
         " checkCryptRStyleStructCryptData)
      if (checkCryptRStyleStructCryptData)
         set(_cryptRStyle "struct_crypt_data")
      endif()
   endif()
   if (_cryptRStyle STREQUAL "none")
      check_c_source_runs("
         #define _REENTRANT 1
         #define _GNU_SOURCE
         #include <crypt.h>
         int main() {
            struct crypt_data buffer;
            crypt_r(\"passwd\", \"hash\", &buffer);
            return 0;
         }
         " checkCryptRStyleStructCryptDataGnuStyle)
      if (checkCryptRStyleStructCryptDataGnuStyle)
         set(_cryptRStyle "struct_crypt_data_gnu_source")
      endif()
   endif()
   if (_cryptRStyle STREQUAL "cryptd")
      set(CRYPT_R_CRYPTD ON)
   endif()
   if(_cryptRStyle STREQUAL "struct_crypt_data" OR _cryptRStyle STREQUAL "struct_crypt_data_gnu_source")
      set(CRYPT_R_STRUCT_CRYPT_DATA ON)
   endif()
   if (_cryptRStyle STREQUAL "struct_crypt_data_gnu_source")
      set(CRYPT_R_GNU_SOURCE ON)
   endif()
   if (_cryptRStyle STREQUAL "none")
      message(FATAL_ERROR "Unable to detect data struct used by crypt_r")
   endif()
endmacro()

# Note that identifiers starting with SIG are reserved by ANSI C.
# C89 requires signal handlers to return void; only K&R returned int;
# modern code does not need to worry about using this macro (not to
# mention that sigaction is better than signal).
# your code may safely assume C89 semantics that RETSIGTYPE is void.
# TODO maybe something wrong here
macro(polar_check_signal)

   check_c_source_runs("
      #include <sys/types.h>
      #include <signal.h>
      #include <crypt.h>
      int main() {
         if (*(signal (0, 0)) (0) == 1) {
            return 0;
         }
         return 1;
      }
      " checkSignalType)
   set(signalType "")
   if (checkSignalType)
      set(signalType "int")
   else()
      set(signalType "void")
   endif()
   set(RETSIGTYPE ${signalType})
endmacro()

macro(polar_check_func_memcmp)
   file(READ ${POLAR_CMAKE_TEST_CODE_DIR}/test_func_cmp_code.c _testcode)
   check_c_source_runs("${_testcode}"
      checkFuncMemCmp)
   if (checkFuncMemCmp)
      set(HAVE_MEMCMP ON)
   else()
      message(FATAL_ERROR "memcmp is not working")
   endif()
endmacro()

macro(polar_check_inline)
   # Do nothing if the compiler accepts the inline keyword.
   # Otherwise define inline to __inline__ or __inline if one of those work,
   # otherwise define inline to be empty.

   # HP C version B.11.11.04 doesn't allow a typedef as the return value for an
   # inline function, only builtin types.
   set(POLAR_FEATURE_INLINE "")
   foreach(inlineKeyword inline __inline__ __inline)
      check_c_source_compiles("
         #ifndef __cplusplus
            typedef int foo_t;
            static ${inlineKeyword} foo_t static_foo () {return 0; }
            ${inlineKeyword} foo_t foo () {return 0; }
         #endif
         int main(){
            return 0;
         }" checkInlineType${inlineKeyword})
      if (checkInlineType${inlineKeyword})
         set(POLAR_FEATURE_INLINE ${inlineKeyword})
         break()
      endif()
   endforeach()
endmacro()
