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
set(POLAR_GENERATED_PARSER_HEADER_FILE ${POLAR_PARSER_INCLUDE_DIR}/internal/YYParserDefs.h)
set(POLAR_GRAMMER_FILE ${POLAR_PARSER_INCLUDE_DIR}/LangGrammer.y)


list(APPEND POLAR_SYNTAX_SOURCES
   SyntaxNodes.cpp.gyb
   SyntaxNodeBuilders.cpp.gyb
   SyntaxKind.cpp.gyb
   TokenKind.cpp.gyb
   SyntaxNodeFactory.cpp.gyb
   SyntaxNodeVisitor.cpp.gyb
   Trivia.cpp.gyb
   )

polar_add_library(PolarSyntax SHARED
   ${POLAR_SYNTAX_SOURCES}
   LINK_LIBS PolarUtils PolarKernel
   DEPENDS PolarRe2cLangLexer polar-syntax-generated-headers)

set_target_properties(
   PolarSyntax
   PROPERTIES
   INTERFACE_LINK_LIBRARIES "PolarBasic;PolarGlobal;"
   )

