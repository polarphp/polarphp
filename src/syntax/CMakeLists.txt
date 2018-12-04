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
   OUTPUT_VAR POLAR_SYNTAX_SOURCES)
polar_merge_list(POLAR_SYNTAX_SOURCES POLAR_HEADERS)

polar_add_library(PolarSyntax SHARED BUILDTREE_ONLY
   ${POLAR_SYNTAX_SOURCES}
   LINK_LIBS PolarUtils PolarBasic)

set_target_properties(
   PolarAst
   PROPERTIES
   INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR};"
   )
