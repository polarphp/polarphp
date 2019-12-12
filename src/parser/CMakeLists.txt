# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/09/04.

# setup compiler sources

set(POLAR_PARSER_SOURCES)

polar_collect_files(
   TYPE_BOTH
   DIR ${POLAR_SOURCE_ROOT_DIR}/ast
   OUTPUT_VAR POLAR_AST_SOURCES)
polar_merge_list(POLAR_PARSER_SOURCES POLAR_AST_SOURCES)

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLAR_PARSER_SOURCES)

polar_merge_list(POLAR_PARSER_SOURCES POLAR_PARSER_SOURCES)
polar_merge_list(POLAR_PARSER_SOURCES POLAR_HEADERS)

# generate yyparser source files

#set(POLAR_PARSER_INCLUDE_DIR ${POLAR_MAIN_INCLUDE_DIR}/polarphp/parser)
#set(POLAR_PARSER_BINARY_INCLUDE_DIR ${POLAR_INCLUDE_DIR}/polarphp/parser)
#set(POLAR_PARSER_SRC_DIR ${POLAR_SOURCE_DIR}/src/parser)
#
#set(POLAR_GENERATED_LEX_IMPL_FILE ${POLAR_PARSER_SRC_DIR}/internal/YYLexer.cpp)
#set(POLAR_GENERATED_LEX_HEADER_FILE ${POLAR_PARSER_INCLUDE_DIR}/internal/YYLexerConditionDefs.h)
#
#set(POLAR_GENERATED_PARSER_IMPL_FILE ${POLAR_PARSER_SRC_DIR}/internal/YYParser.cpp)
#set(POLAR_GENERATED_PARSER_HEADER_FILE ${POLAR_PARSER_INCLUDE_DIR}/internal/YYParserDefs.h)
#set(POLAR_GENERATED_PARSER_LOC_HEADER_FILE ${POLAR_PARSER_INCLUDE_DIR}/internal/YYLocation.h)
#set(POLAR_GRAMMER_FILE ${POLAR_PARSER_INCLUDE_DIR}/LangGrammer.y)
#
#re2c_target(NAME PolarRe2cLangLexer
#   OUTPUT ${POLAR_GENERATED_LEX_IMPL_FILE}
#   INPUT ${POLAR_PARSER_INCLUDE_DIR}/LexicalRule.l
#   OPTIONS --no-generation-date --case-inverted -Wundefined-control-flow -cbdFt ${POLAR_GENERATED_LEX_HEADER_FILE})
#
#add_custom_command(OUTPUT ${POLAR_GENERATED_PARSER_IMPL_FILE}
#   COMMAND ${BISON_EXECUTABLE}
#   "-d" ${POLAR_GRAMMER_FILE}
#   "-o" ${POLAR_GENERATED_PARSER_IMPL_FILE}
#   "--defines=${POLAR_GENERATED_PARSER_HEADER_FILE}"
#   DEPENDS ${POLAR_GRAMMER_FILE} polar-parser-generated-headers
#   )
#
#list(APPEND POLAR_PARSER_SOURCES
#   ${POLAR_GENERATED_LEX_IMPL_FILE}
#   ${POLAR_GENERATED_PARSER_IMPL_FILE}
#   ${POLAR_GENERATED_PARSER_HEADER_FILE}
#   )
#
#list(APPEND POLAR_HEADERS
#   ${POLAR_GENERATED_PARSER_HEADER_FILE})
#
#polar_add_library(PolarParser SHARED
#   ${POLAR_PARSER_SOURCES}
#   LINK_LIBS PolarUtils PolarGlobal PolarKernel PolarSyntax
#   clangAST clangBasic clangSema clangIndex PolarDemangling)
#
## for YYParserDefs.h
#target_include_directories(PolarParser PRIVATE ${POLAR_MAIN_INCLUDE_DIR}/polarphp/parser/internal)
#set_target_properties(PolarParser
#   PROPERTIES
#   INTERFACE_LINK_LIBRARIES "PolarSyntax;PolarUtils;PolarDemangling;clangAST;clangBasic;clangSema;clangIndex")
#
#set(POLAR_HEADERS ${POLAR_HEADERS} PARENT_SCOPE)
