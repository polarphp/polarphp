# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/10/12.

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLAR_UTILS_SOURCES)

if (UNIX)
   list(APPEND POLAR_UTILS_SOURCES _platform/ProcessUtilsUnix.cpp)
   list(APPEND POLAR_UTILS_SOURCES _platform/SignalsUnix.cpp)
   list(APPEND POLAR_UTILS_SOURCES _platform/PathUnix.cpp)
   list(APPEND POLAR_UTILS_SOURCES _platform/ProcessUnix.cpp)
endif()

polar_merge_list(POLAR_UTILS_SOURCES POLAR_HEADERS)

polar_add_library(PolarUtils SHARED BUILDTREE_ONLY
   ${POLAR_UTILS_SOURCES})

set_target_properties(
   PolarUtils
   PROPERTIES
   INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR};"
   )

target_link_libraries(PolarUtils INTERFACE
   PolarBasic
   nlohmann_json::nlohmann_json)
