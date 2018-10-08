# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/10/08.

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLAR_DEVTOOLS_UTILS_LIB_SOURCES)

polar_add_library(devtoolsutils SHARED BUILDTREE_ONLY
   ${POLAR_DEVTOOLS_UTILS_LIB_SOURCES})

set_target_properties(
   devtoolsutils
   PROPERTIES
   INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR};"
   )

target_link_libraries(devtoolsutils INTERFACE
   nlohmann_json::nlohmann_json)
