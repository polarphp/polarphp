# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarPHP software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
# Check if the host compiler is new enough. POLAR requires at least GCC 4.8,
# MSVC 2015 (Update 3), or Clang 3.1.
# Created by polarboy on 2018/08/17.

polar_check_headers(
   utime.h
   dirent.h
   stdarg.h
   alloca.h
   unistd.h
   limits.h)

set(TSRM_SRCS
   TSRM.h
   TSRM.c
   readdir.h
   tsrm_config_common.h
   tsrm_strtok_r.h
   tsrm_strtok_r.c)

set_source_files_properties(TSRM.c
   tsrm_strtok_r.c
   PROPERTIES
   COMPILE_DEFINITIONS ZEND_ENABLE_STATIC_TSRMLS_CACHE=1)

add_library(tsrm SHARED ${TSRM_SRCS})
