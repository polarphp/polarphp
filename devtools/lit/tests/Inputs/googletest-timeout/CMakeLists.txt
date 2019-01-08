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
   OUTPUT_NAME littestsgtesttimeout)

polar_add_lit_extra_test_executable(LittestTimeoutGoogleTest
   LINK_LIBS "PolarUtils"
   dummysubdir/TimeoutGoogleTest.cpp)

# simulate google test binary
polar_set_output_directory(LittestTimeoutGoogleTest BINARY_DIR ${POLAR_REGRESSION_TEST_BINARY_DIR})

set(DEFS "")
list(APPEND DEFS "TIMEOUT_GTEST_BIN=\"${POLAR_REGRESSION_TEST_BINARY_DIR}${DIR_SEPARATOR}LittestTimeoutGoogleTest\"")

set_target_properties(littestsgtesttimeout
   PROPERTIES
   COMPILE_DEFINITIONS "${DEFS}")
