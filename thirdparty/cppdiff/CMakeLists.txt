# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/10/30.
# head only libraray just add into include directories

polar_collect_files(
   TYPE_BOTH
   DIR dtl
   OUTPUT_VAR POLAR_THIRDPARTY_CPPDIFF_SOURCES)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(cppdiff OBJECT ${POLAR_THIRDPARTY_CPPDIFF_SOURCES})
set_target_properties(
   cppdiff
   PROPERTIES
   INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")

