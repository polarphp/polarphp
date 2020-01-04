# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/12/24.

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLARPHP_IMMEDIATE_SOURCES)

polar_merge_list(POLARPHP_IMMEDIATE_SOURCES POLAR_HEADERS)

polar_add_host_library(PolarImmediate STATIC
   ${POLARPHP_IMMEDIATE_SOURCES}
   LLVM_LINK_COMPONENTS
   executionengine
   linker
   mcjit
   transformutils
   #   ExecutionEngine
   #   Linker
   #   TransformUtils
   #   MCJIT
   )

target_link_libraries(PolarImmediate
   PRIVATE
   PolarBasic
   PolarUtils
   PolarIRGen
   PolarPILGen
   PolarIDE
   PolarFrontend
   PolarPILOptimizer)

if(HAVE_UNICODE_LIBEDIT)
   target_link_libraries(PolarImmediate PRIVATE edit)
endif()
