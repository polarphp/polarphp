# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/12/04

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLAR_SYNTAX_SOURCES)
polar_merge_list(POLAR_SYNTAX_SOURCES POLAR_HEADERS)

set(POLAR_PARSER_INCLUDE_DIR ${POLAR_MAIN_INCLUDE_DIR}/polarphp/parser)
set(POLAR_SYNTAX_INCLUDE_DIR ${POLAR_MAIN_INCLUDE_DIR}/polarphp/syntax)
set(POLAR_TOKEN_ENUM_DEF_FILE ${POLAR_SYNTAX_INCLUDE_DIR}/internal/TokenEnumDefs.h)
set(POLAR_TOKEN_DESC_MAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/internal/TokenDescMap.cpp)
set(POLAR_GENERATE_TOKEN_DESC_MAP_SCRIPT ${POLAR_CMAKE_SCRIPTS_DIR}/GenerateTokenDescMap.php)
set(POLAR_GENERATE_TOKEN_ENUM_DEF_SCRIPT ${POLAR_CMAKE_SCRIPTS_DIR}/GenerateTokenEnumDefs.php)
set(POLAR_GENERATED_PARSER_HEADER_FILE ${POLAR_PARSER_INCLUDE_DIR}/internal/YYParserDefs.h)
set(POLAR_GRAMMER_FILE ${POLAR_PARSER_INCLUDE_DIR}/LangGrammer.y)

add_custom_command(OUTPUT ${POLAR_TOKEN_DESC_MAP_FILE}
   COMMAND ${PHP_EXECUTABLE} ${POLAR_GENERATE_TOKEN_DESC_MAP_SCRIPT}
   DEPENDS ${POLAR_GRAMMER_FILE}
   WORKING_DIRECTORY ${POLAR_SOURCE_DIR})

add_custom_command(OUTPUT ${POLAR_TOKEN_ENUM_DEF_FILE}
   COMMAND ${PHP_EXECUTABLE} ${POLAR_GENERATE_TOKEN_ENUM_DEF_SCRIPT}
   DEPENDS ${POLAR_GENERATED_PARSER_HEADER_FILE}
   WORKING_DIRECTORY ${POLAR_SOURCE_DIR})

list(APPEND POLAR_SYNTAX_SOURCES
   ${POLAR_TOKEN_DESC_MAP_FILE}
   ${POLAR_TOKEN_ENUM_DEF_FILE}
   )

polar_add_library(PolarSyntax SHARED
   ${POLAR_SYNTAX_SOURCES}
   LINK_LIBS PolarUtils PolarKernel
   DEPENDS PolarRe2cLangLexer)

