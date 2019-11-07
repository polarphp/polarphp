# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/11/07.

set(PHPLIT_DIR ${POLAR_DEVTOOLS_DIR}/phplit)
set(GYB_DIR ${POLAR_DEVTOOLS_DIR}/gyb)

message("initializing phplit")
execute_process(COMMAND ${POLAR_COMPOSER_EXECUTABLE} install
   WORKING_DIRECTORY ${POLAR_DEVTOOLS_DIR}/phplit
   RESULT_VARIABLE exitCode
   ERROR_VARIABLE errorMsg)
if (NOT exitCode EQUAL 0)
   message(FATAL_ERROR "initialize phplit error: ${errorMsg}")
endif()

message("initializing gyb")
execute_process(COMMAND ${POLAR_COMPOSER_EXECUTABLE} install
   WORKING_DIRECTORY ${POLAR_DEVTOOLS_DIR}/gyb
   RESULT_VARIABLE exitCode
   ERROR_VARIABLE errorMsg)
if (NOT exitCode EQUAL 0)
   message(FATAL_ERROR "initialize gyb error: ${errorMsg}")
endif()
