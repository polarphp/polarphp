# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/11/07.

polar_setup_lit_cfg_setters(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}
   OUTPUT_NAME littestsshtestshell)

polar_add_lit_extra_test_executable(LittestCheckArgs CheckArgs.cpp)
polar_add_lit_extra_test_executable(LittestCheckPath CheckPath.cpp)
polar_add_lit_extra_test_executable(LittestWriteToStderr WriteToStderr.cpp)
polar_add_lit_extra_test_executable(LittestWriteToStdoutAndStderr WriteToStdoutAndStderr.cpp)

set(DEFS "")

if (WIN32)
   set(DEFS "POLAR_OS_WIN32")
endif()
list(APPEND DEFS "LIT_TEST_CHECKARGS_BIN=\"${POLAR_LIT_TEST_BIN_DIR}${DIR_SEPARATOR}LittestCheckArgs\"")
list(APPEND DEFS "LIT_TEST_CHECKPATH_BIN=\"${POLAR_LIT_TEST_BIN_DIR}${DIR_SEPARATOR}LittestCheckPath\"")
list(APPEND DEFS "LIT_TEST_WRITE_TO_STDERR_BIN=\"${POLAR_LIT_TEST_BIN_DIR}${DIR_SEPARATOR}LittestWriteToStderr\"")
list(APPEND DEFS "LIT_TEST_WRITE_TO_STDOUT_AND_STDERR_BIN=\"${POLAR_LIT_TEST_BIN_DIR}${DIR_SEPARATOR}LittestWriteToStdoutAndStderr\"")

set_target_properties(LittestCheckArgs
   PROPERTIES
   COMPILE_DEFINITIONS "${DEFS}")

list(APPEND DEFS "LIT_TEST_BIN=\"${POLAR_RUNTIME_OUTPUT_INTDIR}${DIR_SEPARATOR}lit\"")
list(APPEND DEFS "POLAR_RUNTIME_OUTPUT_INTDIR=\"${POLAR_RUNTIME_OUTPUT_INTDIR}\"")

set_target_properties(littestsshtestshell
   PROPERTIES
   COMPILE_DEFINITIONS "${DEFS}")
