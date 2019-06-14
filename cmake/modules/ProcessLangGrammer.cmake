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
set(POLAR_PARSER_SRC_DIR ${POLAR_SOURCE_DIR}/src/parser)

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

include(GenerateTokenDescMap)

list(APPEND POLAR_PARSER_SOURCES
   ${POLAR_GENERATED_LEX_IMPL_FILE}
   ${POLAR_GENERATED_PARSER_IMPL_FILE}
   ${POLAR_GENERATED_PARSER_HEADER_FILE}
   )

list(APPEND POLAR_HEADERS
   ${POLAR_GENERATED_PARSER_HEADER_FILE})



