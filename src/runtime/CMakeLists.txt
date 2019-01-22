# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/12/25.

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLARPHP_RUNTIME_SOURCES)

polar_merge_list(POLARPHP_RUNTIME_SOURCES POLAR_HEADERS)

if (NOT HAVE_STRLCAT)
   list(APPEND POLARPHP_RUNTIME_SOURCES _platform/Strlcat.cpp)
endif()

if (NOT HAVE_STRLCPY)
   list(APPEND POLARPHP_RUNTIME_SOURCES _platform/Strlcopy.cpp)
endif()

polar_add_library(PolarRuntime SHARED
   ${POLARPHP_RUNTIME_SOURCES}
   LINK_LIBS PolarUtils ZendVM)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

set_target_properties(
   PolarRuntime
   PROPERTIES
   INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR};"
   INTERFACE_LINK_LIBRARIES "PolarUtils;ZendVM"
   COMPILE_DEFINITIONS "ZEND_ENABLE_STATIC_TSRMLS_CACHE=1"
   )

