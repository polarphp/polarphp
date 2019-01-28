# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/10/28.

polar_collect_files(
   TYPE_BOTH
   DIR libs/regex/src
   OUTPUT_VAR POLAR_THIRDPARTY_BOOST_SOURCES)

polar_add_library(boost_regex SHARED
   ${POLAR_THIRDPARTY_BOOST_SOURCES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(boost_regex PUBLIC
   ${ICU_LIBRARIES})

set_target_properties(
   boost_regex
   PROPERTIES
   INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
   COMPILE_DEFINITIONS "BOOST_HAS_ICU"
   )
