# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/10/12.

polar_collect_files(
   TYPE_BOTH
   DIR ${CMAKE_CURRENT_SOURCE_DIR}
   OUTPUT_VAR POLAR_TOOLS_AST_DUMPER)

polar_add_executable(
   polar-ast-dumper ${POLAR_TOOLS_AST_DUMPER}
   LINK_LIBS PolarSyntax PolarParser
   )
