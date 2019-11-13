# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/06/14.

polar_add_unittest(PolarCompilerTests ParserLexerTest
   ../TestEntry.cpp
   LexerTest.cpp
   TokenJsonSerializationTest.cpp)
target_link_libraries(ParserLexerTest PRIVATE PolarParser)

add_library(AbstractParserSupport SHARED
   AbstractParserTestCase.h
   AbstractParserTestCase.cpp)

if(POLAR_FOUND_NATIVE_GTEST)
   set(POLAR_TEMP_GTEST_LIBS googletest::gtest googletest::gtest_main
      googletest::gmock googletest::gmock_main)
else()
   include_directories(${POLAR_THIRDPARTY_DIR}/unittest/googletest/include)
   include_directories(${POLAR_THIRDPARTY_DIR}/unittest/googlemock/include)
   set(POLAR_TEMP_GTEST_LIBS gtest_main gtest gmock gmock_main)
endif()

target_include_directories(AbstractParserSupport PRIVATE ${POLAR_THIRDPARTY_DIR}/googletest/googletest/include)
target_link_libraries(AbstractParserSupport PUBLIC PolarParser PolarUtils ${POLAR_TEMP_GTEST_LIBS})

add_subdirectory(commonsyntaxnode)
add_subdirectory(declsyntaxnode)
add_subdirectory(exprsyntaxnode)
add_subdirectory(stmtsyntaxnode)
