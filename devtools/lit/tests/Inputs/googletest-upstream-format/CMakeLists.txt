# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/11/15.

polar_setup_lit_cfg_setters(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}
   OUTPUT_NAME littestsgtestupstreamformat)

polar_add_lit_extra_test_executable(LittestUpStreamGoogleTest
   LINK_LIBS "PolarUtils"
   dummysubdir/UpStreamGoogleTest.cpp)

# simulate google test binary
polar_set_output_directory(LittestUpStreamGoogleTest BINARY_DIR ${POLAR_REGRESSION_TEST_BINARY_DIR})

set(DEFS "")
list(APPEND DEFS "UPSTREAM_FORMAT_GTEST_BIN=\"${POLAR_REGRESSION_TEST_BINARY_DIR}${DIR_SEPARATOR}LittestUpStreamGoogleTest\"")

set_target_properties(littestsgtestupstreamformat
   PROPERTIES
   COMPILE_DEFINITIONS "${DEFS}")
