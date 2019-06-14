# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/05/09.

set(POLAR_PARSER_INCLUDE_DIR ${POLAR_MAIN_INCLUDE_DIR}/polarphp/parser)
set(POLAR_SYNTAX_INCLUDE_DIR ${POLAR_MAIN_INCLUDE_DIR}/polarphp/syntax)
set(POLAR_PARSER_SRC_DIR ${POLAR_SOURCE_DIR}/src/parser)
set(POLAR_SYNTAX_SRC_DIR ${POLAR_SOURCE_DIR}/src/syntax)

set(POLAR_GENERATED_LEX_IMPL_FILE ${POLAR_PARSER_SRC_DIR}/impl/YYLexer.cpp)
set(POLAR_GENERATED_LEX_HEADER_FILE ${POLAR_PARSER_INCLUDE_DIR}/internal/YYLexerDefs.h)

set(POLAR_GENERATED_PARSER_IMPL_FILE ${POLAR_PARSER_SRC_DIR}/impl/YYParser.cpp)
set(POLAR_GENERATED_PARSER_HEADER_FILE ${POLAR_PARSER_INCLUDE_DIR}/internal/YYParserDefs.h)
set(POLAR_GENERATED_PARSER_LOC_HEADER_FILE ${POLAR_PARSER_INCLUDE_DIR}/internal/YYLocation.h)
set(POLAR_GRAMMER_FILE ${POLAR_PARSER_INCLUDE_DIR}/LangGrammer.y)

re2c_target(NAME PolarRe2cLangLexer
   OUTPUT ${POLAR_GENERATED_LEX_IMPL_FILE}
   INPUT ${POLAR_PARSER_INCLUDE_DIR}/LexicalRule.l
   OPTIONS --no-generation-date --case-inverted -cbdFt ${POLAR_GENERATED_LEX_HEADER_FILE})

file(MD5 ${POLAR_GRAMMER_FILE} grammerFileHash)

if ((NOT EXISTS ${POLAR_GENERATED_PARSER_IMPL_FILE} OR
      NOT EXISTS ${POLAR_GENERATED_PARSER_HEADER_FILE} OR
      NOT EXISTS ${POLAR_GENERATED_PARSER_LOC_HEADER_FILE})
      OR (NOT (POLAR_GRAMMER_FILE_MD5 AND POLAR_GRAMMER_FILE_MD5 STREQUAL grammerFileHash)))
   execute_process(COMMAND ${BISON_EXECUTABLE}
      "-d" ${POLAR_PARSER_INCLUDE_DIR}/LangGrammer.y
      "-o" ${POLAR_GENERATED_PARSER_IMPL_FILE}
      "--defines=${POLAR_GENERATED_PARSER_HEADER_FILE}"
      RESULT_VARIABLE _bisonResult
      ERROR_VARIABLE _bisonError)
   if (NOT _bisonResult EQUAL 0)
      message(FATAL_ERROR ${_bisonError})
   endif()
   set(POLAR_GRAMMER_FILE_MD5 ${grammerFileHash} CACHE STRING "language grammer file md5 value" FORCE)
   mark_as_advanced(POLAR_GRAMMER_FILE_MD5)
endif()

set(POLAR_TOKEN_ENUM_DEF_FILE ${POLAR_SYNTAX_INCLUDE_DIR}/TokenEnumDefs.h)
set(POLAR_TOKEN_DESC_MAP_FILE ${POLAR_SYNTAX_SRC_DIR}/TokenDescMap.cpp)
set(POLAR_GENERATE_TOKEN_DESC_MAP_SCRIPT ${POLAR_CMAKE_SCRIPTS_DIR}/GenerateTokenDescMap.php)
set(POLAR_GENERATE_TOKEN_ENUM_DEF_SCRIPT ${POLAR_CMAKE_SCRIPTS_DIR}/GenerateTokenEnumDefs.php)

if ((NOT EXISTS ${POLAR_TOKEN_DESC_MAP_FILE} OR NOT EXISTS ${POLAR_TOKEN_ENUM_DEF_FILE}) OR
      (NOT (POLAR_GRAMMER_FILE_MD5_FOR_TOKEN_DESC_MAP AND POLAR_GRAMMER_FILE_MD5_FOR_TOKEN_DESC_MAP STREQUAL grammerFileHash)))
   # generate token desc map
   execute_process(COMMAND ${PHP_EXECUTABLE}
      ${POLAR_GENERATE_TOKEN_ENUM_DEF_SCRIPT}
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
      ERROR_VARIABLE _error
      WORKING_DIRECTORY ${POLAR_SOURCE_DIR})
   if (NOT _result EQUAL 0)
      message(FATAL_ERROR "${_output}")
   else()
      message("${_output}")
   endif()

   # generate token enum defs
   execute_process(COMMAND ${PHP_EXECUTABLE}
      ${POLAR_GENERATE_TOKEN_DESC_MAP_SCRIPT}
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
      ERROR_VARIABLE _error
      WORKING_DIRECTORY ${POLAR_SOURCE_DIR})
   if (NOT _result EQUAL 0)
      message(FATAL_ERROR ${_output})
   else()
      message("${_output}")
   endif()
   set(POLAR_GRAMMER_FILE_MD5_FOR_TOKEN_DESC_MAP ${grammerFileHash} CACHE STRING "language grammer file md5 value" FORCE)
   mark_as_advanced(POLAR_GRAMMER_FILE_MD5_FOR_TOKEN_DESC_MAP)
endif()




list(APPEND POLAR_PARSER_SOURCES
   ${POLAR_GENERATED_LEX_IMPL_FILE}
   ${POLAR_GENERATED_PARSER_IMPL_FILE}
   ${POLAR_GENERATED_PARSER_HEADER_FILE}
   ${POLAR_TOKEN_DESC_MAP_FILE}
   )

list(APPEND POLAR_HEADERS
   ${POLAR_GENERATED_PARSER_HEADER_FILE})



