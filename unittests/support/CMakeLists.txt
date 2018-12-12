# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/10/18.

polar_collect_files(
   TYPE_BOTH
   RELATIVE
   DIR ${CMAKE_CURRENT_SOURCE_DIR}
   OUTPUT_VAR POLAR_TEST_SUPPORT_SOURCES)

if(POLAR_FOUND_NATIVE_GTEST)
   set(POLAR_TEMP_GTEST_LIBS googletest::gtest googletest::gtest_main
      googletest::gmock googletest::gmock_main)
else()
   include_directories(${POLAR_THIRDPARTY_DIR}/unittest/googletest/include)
   include_directories(${POLAR_THIRDPARTY_DIR}/unittest/googlemock/include)
   set(POLAR_TEMP_GTEST_LIBS gtest_main gtest gmock gmock_main)
endif()

add_library(TestSupport STATIC ${POLAR_TEST_SUPPORT_SOURCES})
target_link_libraries(TestSupport PolarUtils ${POLAR_TEMP_GTEST_LIBS})
