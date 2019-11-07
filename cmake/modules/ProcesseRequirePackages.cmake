# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/08/22.

# find dependence package
find_package(Re2c)
if (${RE2C_FOUND})
   message("using re2c version: ${RE2C_VERSION}")
endif()

set(RE2C_FLAGS "")
if (POLAR_ENABLE_RE2C_CGOTO)
   message("checking whether re2c -g works")
   check_c_source_compiles("
      int main(int argc, const char **argv)
      {
      argc = argc;
      argv = argv;
      label1:
      label2:
      static void *adr[] = { &&label1, &&label2};
      goto *adr[0];
      return 0;
      }" checkRe2cGoto)
   if (checkRe2cGoto)
      set(RE2C_FLAGS "-g")
   endif()
endif()

find_package(BISON)
if (BISON_FOUND)
   message("using bison version: ${BISON_VERSION}")
else()
   message(FATAL_ERROR "bison is required to build polarphp")
endif()

find_package(PHP)
if (PHP_FOUND)
   message("using php interpreter version: ${PHP_VERSION_STRING}")
else()
   message(FATAL_ERROR "php interpreter is required to build polarphp")
endif()

find_program(POLAR_COMPOSER_EXECUTABLE composer)
if (NOT POLAR_COMPOSER_EXECUTABLE)
   message(FATAL_ERROR "composer is required to build polarphp")
else()
   message("found ${POLAR_COMPOSER_EXECUTABLE}")
endif()

polar_check_prog_awk()
find_program(POLAR_PROG_SED sed)
if (NOT POLAR_PROG_SED)
   message(FATAL_ERROR "sed required by polarphp")
endif()

find_package(UUID)
if (NOT UUID_FOUND)
   message(FATAL_ERROR "uuid required by polarphp")
endif()

if (POLAR_BUILD_TESTS)
   find_package(Curses REQUIRED)
endif()

find_package(Backtrace)
set(HAVE_BACKTRACE ${Backtrace_FOUND})
set(BACKTRACE_HEADER ${Backtrace_HEADER})

# Don't look for these libraries on Windows.
if (NOT PURE_WINDOWS)
   if(POLAR_ENABLE_TERMINFO)
      set(HAVE_TERMINFO 0)
      foreach(library tinfo terminfo curses ncurses ncursesw)
         string(TOUPPER ${library} library_suffix)
         polar_check_library_exists(${library} setupterm "" HAVE_TERMINFO_${library_suffix})
         if(HAVE_TERMINFO_${library_suffix})
            set(HAVE_TERMINFO 1)
            set(TERMINFO_LIBS "${library}")
            break()
         endif()
      endforeach()
   else()
      set(HAVE_TERMINFO 0)
   endif()
endif()

# find icu package for unicode
# for macos install icu by brew
if (APPLE)
   list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/icu4c)
endif()
find_package(ICU COMPONENTS data i18n io tu uc REQUIRED)
message("found icu version: ${ICU_VERSION}")
include_directories(${ICU_INCLUDE_DIRS})
