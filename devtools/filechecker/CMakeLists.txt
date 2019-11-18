# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/09/19.

polar_collect_files(
   TYPE_BOTH
   DIR lib
   OUTPUT_VAR POLAR_DEVTOOLS_FILECHECKER_LIB_SOURCES)

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

configure_file(Config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/FileCheckerConfig.h @ONLY)

list(APPEND POLAR_DEVTOOLS_FILECHECKER_LIB_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/FileCheckerConfig.h)

polar_add_library(filecheckerkernel SHARED BUILDTREE_ONLY
   ${POLAR_DEVTOOLS_FILECHECKER_LIB_SOURCES})
set_target_properties(
   filecheckerkernel
   PROPERTIES
   INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/lib;${CMAKE_CURRENT_BINARY_DIR};${CURSES_INCLUDE_DIRS}"
   )

target_link_libraries(filecheckerkernel PUBLIC
   nlohmann_json::nlohmann_json
   ${CURSES_LIBRARIES}
   PolarUtils)

polar_add_executable(filechecker main.cpp)

target_link_libraries(filechecker PRIVATE filecheckerkernel CLI11::CLI11)
