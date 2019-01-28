# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
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
   TSRM.c
   tsrm_strtok_r.h
   tsrm_strtok_r.c)

polar_merge_list(TSRM_SRCS POLAR_ZENDVM_ZEND_HEADERS)

set_source_files_properties(TSRM.c
   tsrm_strtok_r.c
   PROPERTIES
   COMPILE_DEFINITIONS ZEND_ENABLE_STATIC_TSRMLS_CACHE=1)

polar_collect_files(
   TYPE_HEADER
   DIR ${CMAKE_CURRENT_SOURCE_DIR}
   OUTPUT_VAR POLAR_ZRNDVM_TSRM_HEADERS)

# turn off warning, zendVM team ensure the code correctness
if (POLAR_ENABLE_WARNINGS AND (POLAR_COMPILER_IS_GCC_COMPATIBLE OR CLANG_CL))
   string(REPLACE "-Wall" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
   string(REPLACE "-pedantic" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
   polar_append_flag("-Wno-implicit-fallthrough" CMAKE_C_FLAGS)
   polar_append_flag("-Wno-discarded-qualifiers" CMAKE_C_FLAGS)
   polar_append_flag("-Wno-cast-function-type" CMAKE_C_FLAGS)
   polar_append_flag("-Wno-sign-compare" CMAKE_C_FLAGS)
   polar_append_flag("-Wno-format" CMAKE_C_FLAGS)
endif (POLAR_ENABLE_WARNINGS AND (POLAR_COMPILER_IS_GCC_COMPATIBLE OR CLANG_CL))

set(CMAKE_INCLUDE_CURRENT_DIR ON)

polar_add_library_internal(tsrm STATIC ${TSRM_SRCS})

get_target_property(tsrmIncludeDirs tsrm INCLUDE_DIRECTORIES)
polar_merge_list(tsrmIncludeDirs POLAR_ZENDVM_INCLUDE_DIRS)

target_include_directories(tsrm PUBLIC "${tsrmIncludeDirs}")

install(FILES ${POLAR_ZRNDVM_TSRM_HEADERS} DESTINATION ${POLAR_HEADER_INSTALL_DIR}/vm/TSRM
   COMPONENT ZendVMHeaders)
