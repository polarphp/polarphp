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

# process language grammer files
include(ProcessLangGrammer)

polar_add_library(PolarSyntax SHARED
   ${POLAR_SYNTAX_SOURCES}
   LINK_LIBS PolarUtils PolarKernel
   DEPENDS PolarRe2cLangLexer)

# for YYParserDefs.h
target_include_directories(PolarSyntax PRIVATE ${POLAR_MAIN_INCLUDE_DIR}/polarphp/syntax/internal)
