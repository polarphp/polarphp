# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

polar_collect_files(
   TYPE_BOTH
   DIR lib
   OUTPUT_VAR POLAR_MAIN_LIB_SOURCES)

configure_file(
   ${CMAKE_CURRENT_SOURCE_DIR}/lib/PolarVersion.h.cmake
   ${CMAKE_CURRENT_BINARY_DIR}/lib/PolarVersion.h
   @ONLY
   NEWLINE_STYLE UNIX)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

polar_add_executable(polar main.cpp ${POLAR_MAIN_LIB_SOURCES})

target_link_libraries(polar PRIVATE ZendVM PolarUtils CLI11::CLI11)
