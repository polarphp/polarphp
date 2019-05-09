# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
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

re2c_target(NAME PolarLangLexer
   OUTPUT ${POLAR_GENERATED_LEX_IMPL_FILE}
   INPUT ${POLAR_PARSER_INCLUDE_DIR}/LexicalRule.l
   OPTIONS --no-generation-date --case-inverted -cbdFt ${POLAR_GENERATED_LEX_HEADER_FILE})

list(APPEND POLAR_COMPILER_SOURCES
   ${POLAR_GENERATED_LEX_IMPL_FILE})
