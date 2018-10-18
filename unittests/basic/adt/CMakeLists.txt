# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/10/18.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

polar_add_unittest(PolarBaseLibTests BasicAdtTest
   ../../TestEntry.cpp
   ApIntTest.cpp
   StringRefTest.cpp
   TwineTest.cpp
   )

target_link_libraries(BasicAdtTest PRIVATE TestSupport)
