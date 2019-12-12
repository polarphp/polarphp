# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/11/19.

if (POLAR_DEV_BUILD_DEVTOOLS_TESTS)
   polar_add_lit_testsuite(check-devtools "devtools lit tests"
      ${CMAKE_CURRENT_SOURCE_DIR}/devtools/filechecker
      ARGS -j ${POLAR_PROCESSOR_COUNT}
      )
endif()

if (POLAR_DEV_BUILD_POLARPHP_TESTS)
#   polar_add_lit_testsuite(check-parser "polarphp parser tests"
#      ${CMAKE_CURRENT_SOURCE_DIR}/parser
#      DEPENDS PolarSyntax PolarParser
#      ARGS -j ${POLAR_PROCESSOR_COUNT}
#      )
endif()

