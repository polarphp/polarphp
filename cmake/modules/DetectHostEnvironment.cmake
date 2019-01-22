# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

include(CheckIncludeFiles)
include(CheckTypeSize)
include(CMakeDetermineSystem)
include(CheckFunctionExists)
include(CheckStdCHeaders)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCSourceCompiles)
include(TestBigEndian)
include(CheckLibraryExists)
include(CheckTypeSize)

# get system uname -a
execute_process(COMMAND uname -a
   RESULT_VARIABLE _retCode
   OUTPUT_VARIABLE _output
   ERROR_VARIABLE _errorMsg)
if (NOT _retCode EQUAL 0)
   message(FATAL_ERROR "run shell command uname -a error: ${_errorMsg}")
endif()
string(STRIP "${_output}" PHP_UNAME)

# Platform-specific compile settings.
string(TOLOWER ${CMAKE_HOST_SYSTEM_PROCESSOR} _hostProcessor)
if (${_hostProcessor} MATCHES alpha.*)
   if (POLAR_CC_GCC)
      polar_append_flag(-mieee CMAKE_C_FLAGS)
   else()
      polar_append_flag(-ieee CMAKE_C_FLAGS)
   endif()
elseif(${_hostProcessor} MATCHES sparc.*)
   if (POLAR_CC_NORMAL_NAME STREQUAL "suncc")
      polar_append_flag(-xmemalign=8s CMAKE_C_FLAGS)
   endif()
endif()

if(WIN32 AND NOT CYGWIN )
   # We consider Cygwin as another Unix
   set(PURE_WINDOWS 1)
endif()

# Mark symbols hidden by default if the compiler (for example, gcc >= 4)
# supports it. This can help reduce the binary size and startup time.
# use it just for compile zendVM
check_c_compiler_flag("-Werror -fvisibility=hidden" "C_SUPPORTS_VISIBILITY_HIDDEN")
polar_append_flag_if("C_SUPPORTS_VISIBILITY_HIDDEN" "-fvisibility=hidden" CMAKE_C_FLAGS)

if (CMAKE_HOST_SOLARIS)
   set(_POSIX_PTHREAD_SEMANTICS ON)
   if (POLAR_ENABLE_LIBGCC AND POLAR_CC_GCC)
      set(POLAR_ENABLE_LIBGCC ON)
   else()
      set(POLAR_ENABLE_LIBGCC OFF)
   endif()
elseif(POLAR_SYSTEM_NORMAL_NAME MATCHES ".*dgux.*")
   set(_BSD_TIMEOFDAY_FLAVOR ON)
elseif(POLAR_SYSTEM_NORMAL_NAME STREQUAL "darwin" OR POLAR_SYSTEM_NORMAL_NAME MATCHES ".*rhapsody.*")
   if (POLAR_CC_GCC)
      check_cxx_compiler_flag(-no-cpp-precomp HAVE_NO_CPP_PRECOMP)
      if (HAVE_NO_CPP_PRECOMP)
         polar_append_flag(-no-cpp-precomp CMAKE_CXX_FLAGS)
      endif()
   endif()
   set(POLAR_MULTIPLE_SHLIB_VERSION_OK ON)
elseif(POLAR_SYSTEM_NORMAL_NAME MATCHES ".*beos.*")
   set(POLAR_BEOS_THREADS ON)
   polar_append_flag(-lbe -lroot CMAKE_EXE_LINKER_FLAGS)
elseif(POLAR_SYSTEM_NORMAL_NAME MATCHES ".*mips.*")
   set(_XPG_IV ON)
elseif(POLAR_SYSTEM_NORMAL_NAME MATCHES ".*hpux.*")
   if (POLAR_CC_GCC)
      set(_XOPEN_SOURCE_EXTENDED ON)
   endif()
endif()

# Include Zend and TSRM configurations.

polar_pthreads_check()

# Starting system checks.
# -------------------------------------------------------------------------
# Check whether the system uses EBCDIC (not ASCII) as its native codeset
polar_check_ebcdic()

# Check whether the system byte ordering is bigendian
test_big_endian(WORDS_BIGENDIAN)
if (WORDS_BIGENDIAN)
   set(POLAR_PROCESSOR_BIGENDIAN)
   set(PHP_PROCESSOR_BIGENDIAN)
else()
   set(PHP_PROCESSOR_LITTLEENDIAN)
endif()

# Check whether writing to stdout works
polar_check_write_stdout()

# Check for /usr/pkg/{lib,include} which is where NetBSD puts binary
# and source packages.  This should be harmless on other OSs.
if (EXISTS /usr/pkg/include AND EXISTS /usr/pkg/lib)
   include_directories(SYSTEM /usr/pkg/include)
   link_directories(/usr/pkg/lib)
endif()

if(EXISTS /usr/ucblib)
   link_directories(/usr/ucblib)
endif()

# First, library checks.
# -------------------------------------------------------------------------

# Some systems (OpenServer 5) dislike -lsocket -lnsl, so we try
# to avoid -lnsl checks, if we already have the functions which
# are usually in libnsl
# Also, uClibc will bark at linking with glibc's libnsl.
polar_check_library_exists(socket socket "" HAVE_SOCKET)
polar_check_library_exists(socket socketpair "" HAVE_SOCKETPAIR)
polar_check_library_exists(socket htonl "" HAVE_HTONL)
polar_check_library_exists(nsl gethostname "" HAVE_GETHOSTNAME)
polar_check_library_exists(nsl gethostbyaddr "" HAVE_GETHOSTBYADDR)
polar_check_library_exists(nsl yp_get_default_domain "" HAVE_YP_GET_DEFAULT_DOMAIN)
polar_check_library_exists(dl dlopen "" HAVE_DLOPEN)

if(POLAR_ENABLE_ZLIB)
   foreach(library z zlib_static zlib)
      string(TOUPPER ${library} library_suffix)
      polar_check_library_exists(${library} compress2 "" HAVE_LIBZ_${library_suffix})
      if(HAVE_LIBZ_${library_suffix})
         set(HAVE_LIBZ 1)
         set(ZLIB_LIBRARIES "${library}")
         break()
      endif()
   endforeach()
endif()

# library checks
if(NOT PURE_WINDOWS)
   polar_check_library_exists(pthread pthread_create "" HAVE_LIBPTHREAD)
   if (HAVE_LIBPTHREAD)
      polar_check_library_exists(pthread pthread_getspecific "" HAVE_PTHREAD_GETSPECIFIC)
      polar_check_library_exists(pthread pthread_rwlock_init "" HAVE_PTHREAD_RWLOCK_INIT)
      polar_check_library_exists(pthread pthread_mutex_lock "" HAVE_PTHREAD_MUTEX_LOCK)
   else()
      # this could be Android
      polar_check_library_exists(c pthread_create "" PTHREAD_IN_LIBC)
      if (PTHREAD_IN_LIBC)
         polar_check_library_exists(c pthread_getspecific "" HAVE_PTHREAD_GETSPECIFIC)
         polar_check_library_exists(c pthread_rwlock_init "" HAVE_PTHREAD_RWLOCK_INIT)
         polar_check_library_exists(c pthread_mutex_lock "" HAVE_PTHREAD_MUTEX_LOCK)
      endif()
   endif()
   polar_check_library_exists(dl dlopen "" HAVE_LIBDL)
   polar_check_library_exists(rt clock_gettime "" HAVE_LIBRT)
   # we require dl library
   polar_add_rt_require_lib(dl)
endif()

polar_check_library_exists(m sin "" HAVE_LIBM)
if (NOT HAVE_LIBM)
   message(FATAL_ERROR "m library is require by polarphp")
endif()

polar_add_rt_require_lib(m)

polar_check_library_exists(c inet_aton "" HAVE_INET_ATON)
if (NOT HAVE_INET_ATON)
   unset(HAVE_INET_ATON CACHE)
endif()
polar_check_library_exists(resolv inet_aton "" HAVE_INET_ATON)
if (NOT HAVE_INET_ATON)
   unset(HAVE_INET_ATON CACHE)
endif()
polar_check_library_exists(bind inet_aton "" HAVE_INET_ATON)

# Then headers.
# -------------------------------------------------------------------------

## Check include files
polar_check_stdc_headers()
polar_check_dirent_headers()

# QNX requires unix.h to allow functions in libunix to work properly
polar_check_headers(
   inttypes.h
   stdint.h
   dirent.h
   link.h
   ApplicationServices/ApplicationServices.h
   netinet/in.h
   alloca.h
   arpa/inet.h
   arpa/nameser.h
   assert.h
   crypt.h
   dns.h
   fcntl.h
   grp.h
   ieeefp.h
   langinfo.h
   limits.h
   locale.h
   monetary.h
   netdb.h
   poll.h
   pwd.h
   resolv.h
   signal.h
   stdarg.h
   stdlib.h
   string.h
   strings.h
   syslog.h
   sysexits.h
   sys/param.h
   sys/types.h
   sys/time.h
   sys/ioctl.h
   sys/file.h
   sys/mman.h
   sys/mount.h
   sys/poll.h
   sys/resource.h
   sys/select.h
   sys/socket.h
   sys/stat.h
   sys/statfs.h
   sys/statvfs.h
   sys/vfs.h
   sys/sysexits.h
   sys/varargs.h
   sys/wait.h
   sys/loadavg.h
   sys/utsname.h
   sys/ipc.h
   termios.h
   unistd.h
   unix.h
   utime.h
   valgrind/valgrind.h
   dlfcn.h
   assert.h
   malloc.h
   malloc/malloc.h
   errno.h
   mach/mach.h
   zlib.h)

polar_check_c_const()
polar_check_fopen_cookie()
polar_check_broken_getcwd()
polar_check_broken_glic_fopen_append()

# Checks for typedefs, structures, and compiler characteristics.
# -------------------------------------------------------------------------

polar_check_type_struct_tm()
polar_check_type_struct_timezone()
polar_check_missing_time_r_decl()
polar_check_missing_fclose_decl()
polar_check_tm_gmtoff()
polar_check_struct_flock()
polar_check_socklen_type()

check_type_size(size_t SIZEOF_SIZE_T)
check_type_size("long long" SIZEOF_LONG_LONG)
check_type_size("long long int" SIZEOF_LONG_LONG_INT)
check_type_size(long SIZEOF_LONG)
check_type_size(int SIZEOF_INT)
set(SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P})

# These are defined elsewhere than stdio.h
check_type_size(intmax_t SIZEOF_INTMAX_T)
check_type_size(ssize_t SIZEOF_SSIZE_T)
check_type_size(off_t SIZEOF_OFF_T)
check_type_size(ptrdiff_t SIZEOF_PTRDIFF_T)

# Check stdint types (must be after header check)
polar_check_stdint_types()
polar_check_builtin_expect()
polar_check_builtin_clz()
polar_check_builtin_ctzl()
polar_check_builtin_ctzll()
polar_check_builtin_smull_overflow()
polar_check_builtin_smulll_overflow()
polar_check_builtin_saddl_overflow()
polar_check_builtin_saddll_overflow()
polar_check_builtin_ssubl_overflow()
polar_check_builtin_ssubll_overflow()

# Check for members of the stat structure
check_struct_has_member("struct stat" st_blksize "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_BLKSIZE LANGUAGE C)
if (HAVE_STRUCT_STAT_ST_BLKSIZE)
   set(HAVE_ST_BLKSIZE ON)
endif()

# AC_STRUCT_ST_BLOCKS will screw QNX because fileblocks.o does not exists
# The WARNING_LEVEL required because cc in QNX hates -w option without an argument
if (NOT POLAR_SYSTEM_NORMAL_NAME STREQUAL "qnx")
   # If `struct stat' contains an `st_blocks' member, define
   # HAVE_STRUCT_STAT_ST_BLOCKS.  Otherwise, add `fileblocks.o' to the
   # output variable LIBOBJS.  We still define HAVE_ST_BLOCKS for backward
   # compatibility.  In the future, we will activate specializations for
   # this macro, so don't obsolete it right now.
   check_struct_has_member("struct stat" st_blocks "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_BLOCKS LANGUAGE C)
   if (HAVE_STRUCT_STAT_ST_BLOCKS)
      set(HAVE_ST_BLOCKS ON)
   endif()
else()
   set(POLAR_WARNING_LEVEL 0)
   message(WARNING "warnings level for cc set to 0")
endif()

check_struct_has_member("struct stat" st_rdev "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_RDEV LANGUAGE C)
if (HAVE_STRUCT_STAT_ST_RDEV)
   set(HAVE_ST_RDEV ON)
endif()

# Checks for types
polar_check_type_size_t()
polar_check_type_uid_type()

# Checks for sockaddr_storage and sockaddr.sa_len
polar_check_sockaddr()

# Check for IPv6 support
polar_check_have_ipv6_support()

# Checks for library functions.
# -------------------------------------------------------------------------
polar_check_func_vprintf()

polar_check_funcs(
   alphasort
   asctime_r
   chroot
   ctime_r
   cuserid
   crypt
   flock
   ftok
   funopen
   gai_strerror
   gcvt
   getloadavg
   getlogin
   getprotobyname
   getprotobynumber
   getservbyname
   getservbyport
   gethostname
   gettimeofday
   gmtime_r
   getpwnam_r
   getgrnam_r
   getpwuid_r
   grantpt
   inet_ntoa
   inet_ntop
   inet_pton
   isascii
   link
   localtime_r
   lockf
   lchown
   lrand48
   memcpy
   memmove
   mkstemp
   mmap
   nl_langinfo
   perror
   poll
   ptsname
   putenv
   realpath
   random
   rand_r
   scandir
   setitimer
   setlocale
   localeconv
   setenv
   setpgid
   setsockopt
   setvbuf
   shutdown
   sin
   snprintf
   srand48
   srandom
   statfs
   statvfs
   std_syslog
   strcasecmp
   strcoll
   strdup
   strerror
   strftime
   strnlen
   strptime
   strstr
   strtok_r
   symlink
   tempnam
   tzset
   unlockpt
   unsetenv
   usleep
   utime
   vsnprintf
   vasprintf
   asprintf
   nanosleep
   setproctitle
   setproctitle_fast)

# TODO use polar_check_symbol_exists or polar_check_funcs ?
polar_check_symbol_exists(arc4random "stdlib.h" HAVE_DECL_ARC4RANDOM)
polar_check_symbol_exists(_Unwind_Backtrace "unwind.h" HAVE__UNWIND_BACKTRACE)
polar_check_symbol_exists(getpagesize unistd.h HAVE_GETPAGESIZE)
polar_check_symbol_exists(sysconf unistd.h HAVE_SYSCONF)
polar_check_symbol_exists(getrusage sys/resource.h HAVE_GETRUSAGE)
polar_check_symbol_exists(setrlimit sys/resource.h HAVE_SETRLIMIT)
polar_check_symbol_exists(isatty unistd.h HAVE_ISATTY)
polar_check_symbol_exists(futimens sys/stat.h HAVE_FUTIMENS)
polar_check_symbol_exists(futimes sys/time.h HAVE_FUTIMES)
polar_check_symbol_exists(posix_fallocate fcntl.h HAVE_POSIX_FALLOCATE)
polar_check_symbol_exists(malloc_zone_statistics malloc/malloc.h
   HAVE_MALLOC_ZONE_STATISTICS)
polar_check_symbol_exists(posix_spawn spawn.h HAVE_POSIX_SPAWN)
polar_check_symbol_exists(pread unistd.h HAVE_PREAD)
polar_check_symbol_exists(realpath stdlib.h HAVE_REALPATH)
polar_check_symbol_exists(getrlimit "sys/types.h;sys/time.h;sys/resource.h" HAVE_GETRLIMIT)
polar_check_symbol_exists(sbrk unistd.h HAVE_SBRK)
polar_check_symbol_exists(strerror_r string.h HAVE_STRERROR_R)
polar_check_symbol_exists(strerror_s string.h HAVE_DECL_STRERROR_S)
# AddressSanitizer conflicts with lib/Support/Unix/Signals.inc
# Avoid sigaltstack on Apple platforms, where backtrace() cannot handle it
# (rdar://7089625) and _Unwind_Backtrace is unusable because it cannot unwind
# past the signal handler after an assertion failure (rdar://29866587).
if(HAVE_SIGNAL_H AND NOT APPLE)
   polar_check_symbol_exists(sigaltstack signal.h HAVE_SIGALTSTACK)
endif()
set(CMAKE_REQUIRED_DEFINITIONS "-D_LARGEFILE64_SOURCE")
polar_check_symbol_exists(lseek64 "sys/types.h;unistd.h" HAVE_LSEEK64)
set(CMAKE_REQUIRED_DEFINITIONS "")
polar_check_symbol_exists(mallctl malloc_np.h HAVE_MALLCTL)
polar_check_symbol_exists(mallinfo malloc.h HAVE_MALLINFO)
polar_check_symbol_exists(malloc_zone_statistics malloc/malloc.h
   HAVE_MALLOC_ZONE_STATISTICS)

if(HAVE_DLFCN_H)
   if(HAVE_LIBDL)
      list(APPEND CMAKE_REQUIRED_LIBRARIES dl)
   endif()
   polar_check_symbol_exists(dlopen dlfcn.h HAVE_DLOPEN)
   polar_check_symbol_exists(dladdr dlfcn.h HAVE_DLADDR)
   if(HAVE_LIBDL)
      list(REMOVE_ITEM CMAKE_REQUIRED_LIBRARIES dl)
   endif()
endif()

CHECK_STRUCT_HAS_MEMBER("struct stat" st_mtimespec.tv_nsec
    "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC)
CHECK_STRUCT_HAS_MEMBER("struct stat" st_mtim.tv_nsec
    "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC)

polar_check_symbol_exists(__GLIBC__ stdio.h POLAR_USING_GLIBC)
if(POLAR_USING_GLIBC)
   add_definitions(-D_GNU_SOURCE)
   list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
endif()
# This check requires _GNU_SOURCE
polar_check_symbol_exists(sched_getaffinity sched.h HAVE_SCHED_GETAFFINITY)
polar_check_symbol_exists(CPU_COUNT sched.h HAVE_CPU_COUNT)

# Some systems (like OpenSolaris) do not have nanosleep in libc

polar_check_library_exists(rt nanosleep "" HAVE_RT)
if (POLAR_HAVE_RT)
   set(HAVE_RT ON)
   set(HAVE_NANOSLEEP ON)
   set(POLAR_HAVE_NANOSLEEP ON)
   polar_append_flag(-lrt CMAKE_EXE_LINKER_FLAGS)
   set(HAVE_RT ON)
   set(HAVE_NANOSLEEP ON)
endif()

# Check for getaddrinfo, should be a better way, but...
# Also check for working getaddrinfo
check_c_source_runs(
   "#include <netdb.h>
   int main(){
   struct addrinfo *g,h;g=&h;getaddrinfo(\"\",\"\",g,&g);
   return 0;
   }" checkLinkGetAddrInfo)

if(checkLinkGetAddrInfo)
   check_c_source_runs(
      "#include <netdb.h>
      #include <sys/types.h>
      #ifndef AF_INET
      # include <sys/socket.h>
      #endif
      int main(){
      struct addrinfo *ai, *pai, hints;

      memset(&hints, 0, sizeof(hints));
      hints.ai_flags = AI_NUMERICHOST;

      if (getaddrinfo(\"127.0.0.1\", 0, &hints, &ai) < 0) {
         exit(1);
         }

         if (ai == 0) {
            exit(1);
            }

            pai = ai;

            while (pai) {
               if (pai->ai_family != AF_INET) {
                  /* 127.0.0.1/NUMERICHOST should only resolve ONE way */
                  exit(1);
                  }
                  if (pai->ai_addr->sa_family != AF_INET) {
                     /* 127.0.0.1/NUMERICHOST should only resolve ONE way */
                     exit(1);
                     }
                     pai = pai->ai_next;
                     }
                     freeaddrinfo(ai);
                     exit(0);
                     }"
                     checkHaveGetAddrInfo)
   if (checkHaveGetAddrInfo)
      set(HAVE_GETADDRINFO ON)
   endif()
endif()

# Check for the __sync_fetch_and_add builtin
check_c_source_runs(
   "#include <netdb.h>
   int main(){
   int x;__sync_fetch_and_add(&x,1);
   return 0;
   }" checkSyncFetchAndAdd)

if (checkSyncFetchAndAdd)
   set(HAVE_SYNC_FETCH_AND_ADD ON)
endif()

# todo
# AC_REPLACE_FUNCS(strlcat strlcpy explicit_bzero getopt)
polar_check_func_utime_null()
polar_check_func_alloca()
polar_check_declare_timezone()
polar_check_time_r_type()
polar_check_readdir_r_type()
polar_check_in_addr_t()

check_function_exists(crypt_r checkFuncCryptR)

if (checkFuncCryptR)
   polar_crypt_r_style()
endif()

if (POLAR_WITH_VALGRIND)
   message("checking for vargrind support")
   set(_searchPath "")
   set(_valgrindDir)
   if (POLAR_VALGRIND_DIR)
      set(_searchPath ${POLAR_VALGRIND_DIR})
   else()
      set(_searchPath "/usr/local /usr")
   endif()
   set(_searchFor "/include/valgrind/valgrind.h")
   foreach(_path ${_searchPath})
      set(_valgrindFile ${_path}${_searchFor})
      if (EXISTS ${_valgrindFile})
         set(_valgrindDir ${_path})
      endif()
   endforeach()
   if(NOT _valgrindDir)
      message("valgrind not found")
   else()
      message("found valgrind in ${_valgrindDir}")
      set(HAVE_VALGRIND ON)
   endif()
endif()

if(POLAR_ENABLE_GCOV)
   if (NOT POLAR_CC_GCC)
      message(FATAL_ERROR "GCC is required for enable gcov")
   endif()
   # min: 1.5 (i.e. 105, major * 100 + minor for easier comparison)
   set(POLAR_LTP_VERSION_MIN 105)
   # non-working versions, e.g. "1.8 1.18";
   # remove "none" when introducing the first incompatible LTP version and
   # separate any following additions by spaces
   set(POLAR_LTP_EXCLUDE "1.8")
   find_program(POLAR_PROG_LTP lcov)
   find_program(POLAR_PROG_LTP_GENHTML genhtml)
   if (POLAR_PROG_LTP)
      set(_tempLtpVersion "invalid")
      execute_process(COMMAND ${POLAR_PROG_LTP} -v 2>/dev/null
         RESULT_VARIABLE _retCode
         OUTPUT_VARIABLE _output
         ERROR_VARIABLE _errorMsg)
      if (NOT _retCode EQUAL 0)
         message(FATAL_ERROR "run shell command ${POLAR_PROG_LTP} -v 2>/dev/null error: ${_errorMsg}")
      endif()
      string(REGEX MATCH "([0-9]+)\.([0-9]+)"
         matchResult ${_output})
      set(POLAR_LTP_VERSION ${matchResult})
      string(REPLACE "." ";" matchResult "${matchResult}")
      list(GET matchResult 0 _ltpMajorVersion)
      list(GET matchResult 1 _ltpMinorVersion)
      math(EXPR POLAR_LTP_VERSION_NUM "${_ltpMajorVersion} * 100 + ${_ltpMinorVersion}")
      if (POLAR_LTP_VERSION_NUM GREATER POLAR_LTP_VERSION_MIN)
         set(_tempLtpVersion "${POLAR_LTP_VERSION} (ok)")
         foreach(_checkLtpVersion ${POLAR_LTP_EXCLUDE})
            if (POLAR_LTP_VERSION STREQUAL _checkLtpVersion)
               set(_tempLtpVersion "invalid")
               break()
            endif()
         endforeach()
      endif()
   else()
      message(FATAL_ERROR "To enable code coverage reporting you must have LTP installed")
   endif()
   if (NOT POLAR_LTP_VERSION OR POLAR_LTP_VERSION STREQUAL "invalid")
      message(FATAL_ERROR "This LTP version is not supported (found: ${POLAR_LTP_VERSION}, min: ${POLAR_LTP_VERSION_MIN}, excluded: ${POLAR_LTP_EXCLUDE}).")
      set(POLAR_PROG_LTP "exit 0;")
   endif()
   if (NOT POLAR_PROG_LTP_GENHTML)
      message(FATAL_ERROR "Could not find genhtml from the LTP package")
   endif()
   # Remove all optimization flags from CFLAGS

   string(REGEX REPLACE "-O[0-9s]*" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
   string(REGEX REPLACE "-O[0-9s]*" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
   # Add the special gcc flags
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -fprofile-arcs -ftest-coverage")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -fprofile-arcs -ftest-coverage")
endif()

if(POLARPHP_CONFIG_FILE_PATH STREQUAL "Default")
   # if config path is default, we install php.ini in install_prefix/etc directory
   # wether we need a global variable ?
   set(PHP_CONFIG_FILE_PATH ${CMAKE_INSTALL_PREFIX}/etc)
endif()

message("checking where to scan for configuration files")
if (POLARPHP_CONFIG_FILE_SCAN_DIR STREQUAL "Default")
   set(POLARPHP_CONFIG_FILE_SCAN_DIR "")
endif()
if (POLARPHP_CONFIG_FILE_SCAN_DIR)
   message("using directory ${POLARPHP_CONFIG_FILE_SCAN_DIR} for scan configuration files")
endif()

if (POLAR_ENABLE_SIGCHILD)
   set(PHP_SIGCHILD ON)
   set(POLAR_SIGCHILD ON)
endif()

if(POLAR_ENABLE_LIBGCC)
   polar_libgcc_path(POLAR_LIBGCC_LIBPATH)
   if (NOT POLAR_LIBGCC_LIBPATH)
      message(FATAL_ERROR "Cannot locate libgcc. Make sure that gcc is in your path")
   endif()
   link_directories(${POLAR_LIBGCC_LIBPATH})
   polar_append_flag(-lgcc CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
endif()

# we disable short open tag
set(DEFAULT_SHORT_OPEN_TAG OFF)
if (POLAR_ENABLE_DMALLOC)
   check_library_exists(dmalloc dmalloc_error "" checkDMallocExist)
   if(checkDMallocExist)
      polar_append_flag(-ldmalloc CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
      set(DMALLOC_FUNC_CHECK ON)
   else()
      message(FATAL_ERROR "Problem with enabling dmalloc. ")
   endif()
endif()

if (POLAR_ENABLE_IPV6 AND HAVE_IPV6_SUPPORT)
   set(HAVE_IPV6_SUPPORT ON)
   set(HAVE_IPV6 ON)
endif()

# TODO I don't understand very well about this
# I need read some article about dtrace
if (POLAR_ENABLE_DTRACE)
   check_include_files(sys/sdt.h HAVE_SYS_SDT_H LANGUAGE C)
   if (HAVE_SYS_SDT_H)
   else()
      message(FATAL_ERROR "Cannot find sys/sdt.h which is required for DTrace support")
   endif()
endif()

if (POLAR_ENABLE_FD_SETSIZE)
   if (POLAR_FD_SETSIZE GREATER 0)
      polar_append_flag(-DFD_SETSIZE="${POLAR_FD_SETSIZE}" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
   else()
      message(FATAL_ERROR "Invalid value passed to POLAR_FD_SETSIZE!")
   endif()
else()
   message("using system default fd set limit")
endif()

# By default, we target the host, but this can be overridden at CMake
# invocation time.
polar_get_host_triple(POLAR_INFERRED_HOST_TRIPLE)
set(POLAR_HOST_TRIPLE "${POLAR_INFERRED_HOST_TRIPLE}" CACHE STRING
   "Host on which polarphp binaries will run")

string(REGEX MATCH "^[^-]*" POLAR_NATIVE_ARCH ${POLAR_HOST_TRIPLE})

if (POLAR_NATIVE_ARCH MATCHES "i[2-6]86")
   set(POLAR_NATIVE_ARCH X86)
elseif (POLAR_NATIVE_ARCH STREQUAL "x86")
   set(POLAR_NATIVE_ARCH X86)
elseif (POLAR_NATIVE_ARCH STREQUAL "amd64")
   set(POLAR_NATIVE_ARCH X86)
elseif (POLAR_NATIVE_ARCH STREQUAL "x86_64")
   set(POLAR_NATIVE_ARCH X86)
elseif (POLAR_NATIVE_ARCH MATCHES "sparc")
   set(POLAR_NATIVE_ARCH Sparc)
elseif (POLAR_NATIVE_ARCH MATCHES "powerpc")
   set(POLAR_NATIVE_ARCH PowerPC)
elseif (POLAR_NATIVE_ARCH MATCHES "aarch64")
   set(POLAR_NATIVE_ARCH AArch64)
elseif (POLAR_NATIVE_ARCH MATCHES "arm64")
   set(POLAR_NATIVE_ARCH AArch64)
elseif (POLAR_NATIVE_ARCH MATCHES "arm")
   set(POLAR_NATIVE_ARCH ARM)
elseif (POLAR_NATIVE_ARCH MATCHES "mips")
   set(POLAR_NATIVE_ARCH Mips)
elseif (POLAR_NATIVE_ARCH MATCHES "xcore")
   set(POLAR_NATIVE_ARCH XCore)
elseif (POLAR_NATIVE_ARCH MATCHES "msp430")
   set(POLAR_NATIVE_ARCH MSP430)
elseif (POLAR_NATIVE_ARCH MATCHES "hexagon")
   set(POLAR_NATIVE_ARCH Hexagon)
elseif (POLAR_NATIVE_ARCH MATCHES "s390x")
   set(POLAR_NATIVE_ARCH SystemZ)
elseif (POLAR_NATIVE_ARCH MATCHES "wasm32")
   set(POLAR_NATIVE_ARCH WebAssembly)
elseif (POLAR_NATIVE_ARCH MATCHES "wasm64")
   set(POLAR_NATIVE_ARCH WebAssembly)
elseif (POLAR_NATIVE_ARCH MATCHES "riscv32")
   set(POLAR_NATIVE_ARCH RISCV)
elseif (POLAR_NATIVE_ARCH MATCHES "riscv64")
   set(POLAR_NATIVE_ARCH RISCV)
else ()
   message(FATAL_ERROR "Unknown architecture ${POLAR_NATIVE_ARCH}")
endif ()

# check asm_goto
try_run(retCode compileResult
   ${CMAKE_CURRENT_BINARY_DIR} ${POLAR_CMAKE_TEST_CODE_DIR}/test_asm_goto.c
   COMPILE_DEFINITIONS ${POLAR_COMPILE_DEFINITIONS}
   COMPILE_OUTPUT_VARIABLE compileOutput
   RUN_OUTPUT_VARIABLE runOutput)

if (retCode EQUAL 0)
   set(HAVE_ASM_GOTO ON)
endif()
