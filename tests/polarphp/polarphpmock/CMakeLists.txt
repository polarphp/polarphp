# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/02/07.

polar_collect_files(
   TYPE_BOTH
   DIR ${CMAKE_CURRENT_SOURCE_DIR}/libpdkmock
   OUTPUT_VAR POLAR_TEST_LIBPDK_MOCK_SOURCES)

add_library(stdlibmock ${POLAR_TEST_LIBPDK_MOCK_SOURCES})

target_link_libraries(stdlibmock ZendApi)

polar_add_lit_extra_test_executable(polarmock
   LINK_LIBS "ZendApi stdlibmock CLI11::CLI11"
   main.cpp)

set_target_properties(polarmock PROPERTIES
   BUILD_WITH_INSTALL_RPATH ON
   BUILD_RPATH "${POLAR_LIBRARY_DIR};${POLAR_REGRESSION_LIB_DIR}"
   INSTALL_RPATH "${POLAR_LIBRARY_DIR};${POLAR_REGRESSION_LIB_DIR}")

polar_set_output_directory(polarmock BINARY_DIR ${POLAR_REGRESSION_TEST_BINARY_DIR})
polar_set_output_directory(stdlibmock LIBRARY_DIR ${POLAR_REGRESSION_LIB_DIR})
