# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/08/29.

add_custom_target(DevToolLitUnittests)
set_target_properties(DevToolLitUnittests PROPERTIES FOLDER "DevToolLitUnittests")

set(DEVL_TOOLS_LIT_TEST_SRCS)
polar_add_files(DEVL_TOOLS_LIT_TEST_SRCS
   ProcessUtilsTest.cpp
   BooleanExpressionTest.cpp
   UtilsTest.cpp
   ShellLexerTest.cpp
   ShellParseTest.cpp
   )

polar_add_unittest(DevToolLitUnittests LitlibTest ${DEVL_TOOLS_LIT_TEST_SRCS})

target_link_libraries(LitlibTest PRIVATE litkernel boost_regex)

set(LIT_LIB_TEST_DEFS
   "UNITTEST_TEMP_DIR=\"${CMAKE_CURRENT_BINARY_DIR}/temp\"")
list(APPEND LIT_LIB_TEST_DEFS "POLAR_BUILD_BINARY_DIR=\"${POLAR_RUNTIME_OUTPUT_INTDIR}\"")
list(APPEND LIT_LIB_TEST_DEFS "UNITTEST_CURRENT_BUILD_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\"")
list(APPEND LIT_LIB_TEST_DEFS "UNITTEST_LIT_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/data\"")

set_target_properties(
   LitlibTest
   PROPERTIES
   COMPILE_DEFINITIONS "${LIT_LIB_TEST_DEFS}"
   )

