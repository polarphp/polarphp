# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/10/12.

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLARPHP_BASIC_ADT_SOURCES)

polar_merge_list(POLARPHP_BASIC_ADT_SOURCES POLAR_HEADERS)

add_library(PolarBasic OBJECT ${POLARPHP_BASIC_ADT_SOURCES})
set_target_properties(
   PolarBasic
   PROPERTIES
   INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR};"
   INTERFACE_LINK_LIBRARIES "PolarGlobal")

target_link_libraries(PolarBasic PolarGlobal)
