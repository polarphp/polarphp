# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/11/26.

set(POLAR_FRONTEND_TOOL_SOURCES)

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLAR_FRONTEND_TOOL_SOURCES)

polar_merge_list(POLAR_FRONTEND_TOOL_SOURCES POLAR_HEADERS)

polar_add_host_library(PolarFrontendTool STATIC
   ${POLAR_FRONTEND_TOOL_SOURCES})

target_link_libraries(PolarFrontendTool PRIVATE
#   PolarClangImporter
   PolarDemangling
   PolarFrontend
#   PolarIDE
#   PolarImmediate
#   PolarIndex
#   PolarIRGen
   PolarOption
#   PolarSerialization
#   PolarPIL
#   PolarPILGen
#   PolarPILOptimizer
#   PolarTBDGen
   )

target_link_libraries(PolarFrontendTool PUBLIC
   LLVMTarget
   LLVMAArch64AsmParser)