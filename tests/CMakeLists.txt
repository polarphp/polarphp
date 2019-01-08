# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/11/19.

set(REGRESSION_TEST_DEFS "")

list(APPEND REGRESSION_TEST_DEFS "LIT_TEST_BIN=\"${POLAR_RUNTIME_OUTPUT_INTDIR}${DIR_SEPARATOR}lit\"")
list(APPEND REGRESSION_TEST_DEFS "POLAR_RUNTIME_OUTPUT_INTDIR=\"${POLAR_RUNTIME_OUTPUT_INTDIR}\"")

if (POLAR_DEV_BUILD_DEVTOOLS_TESTS)
   add_subdirectory(devtools)
endif()

if (POLAR_DEV_BUILD_POLARPHP_TESTS)
   add_subdirectory(polarphp/polarphpmock)
   add_subdirectory(polarphp/vmapi)
endif()

