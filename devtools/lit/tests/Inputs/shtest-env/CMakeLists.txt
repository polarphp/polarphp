# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/09/19.

polar_setup_lit_cfg_setters(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}
   OUTPUT_NAME littestsshtestenv)

# need add depended ?

polar_add_lit_extra_test_executable(LittestPrintEnvironment PrintEnvironment.cpp)

set_target_properties(littestsshtestenv
   PROPERTIES
   COMPILE_DEFINITIONS "LIT_TEST_PRINT_ENVIRONMENT_BIN=\"${POLAR_LIT_TEST_BIN_DIR}${DIR_SEPARATOR}LittestPrintEnvironment\"")
