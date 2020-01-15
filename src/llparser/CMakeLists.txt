# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/12/11.

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLAR_LLPARSER_SOURCES)

polar_merge_list(POLAR_LLPARSER_SOURCES POLAR_HEADERS)

polar_add_host_library(PolarLLParser STATIC
   ${POLAR_LLPARSER_SOURCES}
   )

target_link_libraries(PolarLLParser PolarAST LLVMBitstreamReader)