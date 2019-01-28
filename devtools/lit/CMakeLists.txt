# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/08/27.

polar_collect_files(
   TYPE_BOTH
   DIR lib
   OUTPUT_VAR POLAR_DEVTOOLS_LIT_LIB_SOURCES)

if (UNIX)
   list(APPEND POLAR_DEVTOOLS_LIT_LIB_SOURCES lib/_platform/ProcessUtilsUnix.cpp)
   list(APPEND POLAR_DEVTOOLS_LIT_LIB_SOURCES lib/_platform/UtilsUnix.cpp)
   list(APPEND POLAR_DEVTOOLS_LIT_LIB_SOURCES lib/_platform/CfgSetterPluginLoaderUnix.cpp)
endif()

if (APPLE)
   set(POLAR_OS_DARWIN ON)
   set(POLAR_OS_MACOS ON)
elseif(WIN32)
   set(POLAR_OS_WIN32 ON)
elseif(UNIX)
   set(POLAR_OS_UNIX ON)
endif()

if (CYGWIN)
   set(POLAR_OS_CYGWIN ON)
endif()

if (MINGW)
   set(POLAR_OS_MINGW ON)
endif()

set(POLAR_LIT_VERSION "0.0.1")

configure_file(Config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/LitConfigDef.h @ONLY)

list(APPEND POLAR_DEVTOOLS_LIT_LIB_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/LitConfigDef.h)

polar_add_library(litkernel SHARED BUILDTREE_ONLY
   ${POLAR_DEVTOOLS_LIT_LIB_SOURCES})

set(DEFS "")
list(APPEND DEFS "FILECHECKER_BIN=\"${POLAR_RUNTIME_OUTPUT_INTDIR}${DIR_SEPARATOR}filechecker\"")
list(APPEND DEFS "CAT_BIN=\"${POLAR_LIT_TEST_BIN_DIR}${DIR_SEPARATOR}LitBuiltinCat\"")
list(APPEND DEFS "UNITEST_BIN_DIR=\"${POLAR_REGRESSION_TEST_BINARY_DIR}\"")

set_target_properties(
   litkernel
   PROPERTIES
   INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/lib;${CMAKE_CURRENT_BINARY_DIR};${CURSES_INCLUDE_DIRS}"
   COMPILE_DEFINITIONS "${DEFS}"
   )

target_link_libraries(litkernel PUBLIC
   nlohmann_json::nlohmann_json
   cppdiff
   boost_regex
   PolarUtils
   ${ICU_LIBRARIES}
   ${CURSES_LIBRARIES})

polar_add_executable(lit main.cpp)

target_link_libraries(lit PRIVATE litkernel CLI11::CLI11)

if (POLAR_DEV_BUILD_DEVTOOLS_TESTS)
   add_subdirectory(tests)
   add_subdirectory(unittests)
endif()

add_subdirectory(builtincommands)

