# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/12/17.

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLAR_SERIALIZATION_SOURCES)
polar_merge_list(POLAR_SERIALIZATION_SOURCES POLAR_HEADERS)

polar_add_host_library(PolarSerialization STATIC
   ${POLAR_SERIALIZATION_SOURCES}
   LLVM_LINK_COMPONENTS
      BitstreamReader
   )

target_link_libraries(PolarSerialization PRIVATE PolarClangImporter)