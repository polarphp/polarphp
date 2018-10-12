# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/10/08.

#polar_collect_files(
#   TYPE_BOTH
#   DIR .
#   OUTPUT_VAR POLAR_DEVTOOLS_UTILS_LIB_SOURCES)

#if (UNIX)
#   list(APPEND POLAR_DEVTOOLS_UTILS_LIB_SOURCES _platform/ProcessUtilsUnix.cpp)
#   list(APPEND POLAR_DEVTOOLS_UTILS_LIB_SOURCES _platform/SignalsUnix.cpp)
#endif()

#configure_file(Config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/UtilsConfig.h @ONLY)
##configure_file(ABiBreaking.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/AbiBreaking.h @ONLY)

#list(APPEND POLAR_DEVTOOLS_UTILS_LIB_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/UtilsConfig.h)

#polar_add_library(devtoolsutils SHARED BUILDTREE_ONLY
#   ${POLAR_DEVTOOLS_UTILS_LIB_SOURCES})

#set_target_properties(
#   devtoolsutils
#   PROPERTIES
#   INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR};"
#   )

#target_link_libraries(devtoolsutils INTERFACE
#   nlohmann_json::nlohmann_json)
