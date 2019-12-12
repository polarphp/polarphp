# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/12/12.

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLAR_AST_SOURCES)

polar_merge_list(POLAR_AST_SOURCES POLAR_HEADERS)

add_library(PolarAST STATIC ${POLAR_AST_SOURCES})

target_link_libraries(PolarAST INTERFACE
   PolarKernel
    clangCodeGen
    clangIndex
    clangFormat
    clangToolingCore
    clangFrontendTool
    clangFrontend
    clangDriver
    clangSerialization
    clangParse
    clangSema
    clangAnalysis
    clangEdit
    clangRewriteFrontend
    clangRewrite
    clangAST
    clangLex
    clangBasic)

