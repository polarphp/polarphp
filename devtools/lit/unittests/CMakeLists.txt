# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/08/29.

add_custom_target(DevToolLitUnittests)
set_target_properties(DevToolLitUnittests PROPERTIES FOLDER "DevToolLitUnittests")

set(DEVL_TOOLS_LIT_TEST_SRCS)
polar_add_files(DEVL_TOOLS_LIT_TEST_SRCS
   ProcessUtilsTest.cpp
   )

polar_add_unittest(DevToolLitUnittests LitlibTest ${DEVL_TOOLS_LIT_TEST_SRCS})
