# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/12/04.

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLAR_PARSER_SOURCES)
polar_merge_list(POLAR_PARSER_SOURCES POLAR_HEADERS)

polar_add_library(PolarParser SHARED BUILDTREE_ONLY
   ${POLAR_PARSER_SOURCES}
   LINK_LIBS PolarUtils PolarBasic PolarAst PolarSyntax)

set_target_properties(
   PolarParser
   PROPERTIES
   INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR};"
   )
