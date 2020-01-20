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

#set(POLAR_FRONTEND_SOURCES)

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLAR_FRONTEND_SOURCES)

polar_merge_list(POLAR_FRONTEND_SOURCES POLAR_HEADERS)

polar_add_host_library(PolarFrontend STATIC
   ${POLAR_FRONTEND_SOURCES})

target_link_libraries(PolarFrontend PRIVATE
#   PolarPIL
   PolarOption
#   PolarPILParser
#   PolarPILGen
#   PolarPILOptimizer
#   PolarSema
#   PolarSerialization
   )
