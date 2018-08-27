# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/08/22.

# find dependence package
find_package(RE2C)
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

if (NOT EXISTS ${POLAR_SOURCE_DIR}/Zend/zend_language_parser.h OR NOT EXISTS ${POLAR_SOURCE_DIR}/Zend/zend_language_parser.c)
   if (NOT RE2C_FOUND)
      message(FATAL_ERROR "re2c is required to build PHP/Zend when building a GIT checkout!")
   endif()
endif()

find_package(BISON)
if (BISON_FOUND)
   message("using bison version: ${BISON_VERSION}")
else()
   if (NOT EXISTS ${POLAR_SOURCE_DIR}/Zend/zend_language_parser.h OR
         NOT EXISTS ${POLAR_SOURCE_DIR}/Zend/zend_language_parser.c)
      message(FATAL_ERROR "bison is required to build PHP/Zend when building a GIT checkout!")
   endif()
endif()

polar_check_prog_awk()
find_program(POLAR_PROG_SED sed)
if (NOT POLAR_PROG_SED)
   message(FATAL_ERROR "sed required by php building")
endif()

find_package(UUID)
