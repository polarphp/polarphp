# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

polar_collect_files(
   TYPE_BOTH
   DIR lib
   OUTPUT_VAR POLAR_MAIN_LIB_SOURCES)

configure_file(
   ${CMAKE_CURRENT_SOURCE_DIR}/lib/PolarVersion.h.cmake
   ${CMAKE_CURRENT_BINARY_DIR}/lib/PolarVersion.h
   @ONLY
   NEWLINE_STYLE UNIX)

string(TIMESTAMP _buildDate "%Y-%m-%d %H:%M")

polar_add_executable(polar main.cpp ${POLAR_MAIN_LIB_SOURCES})

target_include_directories(polar PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(polar PROPERTIES COMPILE_DEFINITIONS "BUILD_TIME=\"${_buildDate}\"")

target_link_libraries(polar PUBLIC ZendVM PolarUtils CLI11::CLI11)

