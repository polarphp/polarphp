# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/12/28.

polar_collect_files(
   TYPE_BOTH
   RELATIVE
   DIR ${CMAKE_CURRENT_SOURCE_DIR}
   OUTPUT_VAR POLAR_UNITTEST_VM_DS_SOURCES)

polar_add_unittest(ZendApiTests ZendApiDsTest
   ${POLAR_UNITTEST_VM_DS_SOURCES})

target_link_libraries(ZendApiDsTest PRIVATE PolarEmbed)
