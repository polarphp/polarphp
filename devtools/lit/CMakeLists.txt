# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/08/27.

polar_collect_files(
   TYPE_BOTH
   DIR lib
   OUTPUT_VAR POLAR_DEVTOOLS_LIT_LIB_SOURCES)

if (UNIX)
   list(APPEND POLAR_DEVTOOLS_LIT_LIB_SOURCES lib/_platform/ProcessUtilsUnix.cpp)
endif()

polar_add_library(litkernel SHARED BUILDTREE_ONLY
   ${POLAR_DEVTOOLS_LIT_LIB_SOURCES})
set_target_properties(
   litkernel
   PROPERTIES
   INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/lib;${CMAKE_CURRENT_BINARY_DIR}"
   )

configure_file(Config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/Config.h @ONLY)

list(APPEND POLAR_DEVTOOLS_LIT_LIB_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/Config.h)

polar_add_executable(lit main.cpp)

target_link_libraries(lit PRIVATE litkernel CLI11::CLI11)

add_subdirectory(unittests)
