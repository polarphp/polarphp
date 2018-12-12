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

add_subdirectory(Inputs/discovery)
add_subdirectory(Inputs/max-failures)
add_subdirectory(Inputs/progress-bar)
add_subdirectory(Inputs/shtest-env)
add_subdirectory(Inputs/shtest-output-printing)
add_subdirectory(Inputs/shtest-run-at-line)
add_subdirectory(Inputs/shtest-shell)
add_subdirectory(Inputs/shtest-format)
add_subdirectory(Inputs/shtest-timeout)
add_subdirectory(Inputs/googletest-format)
add_subdirectory(Inputs/googletest-timeout)
add_subdirectory(Inputs/googletest-upstream-format)
add_subdirectory(Inputs/testrunner-custom-parsers)
add_subdirectory(Inputs/test-data)
add_subdirectory(Inputs/test-data-micro)
add_subdirectory(Inputs/xunit-output)

polar_setup_lit_cfg_setters(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}
   OUTPUT_NAME littests
   SKIP_DIRS "Inputs")

set(DEFS "")

list(APPEND DEFS "LIT_TEST_BIN=\"${POLAR_RUNTIME_OUTPUT_INTDIR}${DIR_SEPARATOR}lit\"")
list(APPEND DEFS "POLAR_RUNTIME_OUTPUT_INTDIR=\"${POLAR_RUNTIME_OUTPUT_INTDIR}\"")

set_target_properties(littests
   PROPERTIES
   COMPILE_DEFINITIONS "${DEFS}")
