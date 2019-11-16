# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/12/20.

#polar_collect_files(
#   TYPE_BOTH
#   DIR lib
#   OUTPUT_VAR POLAR_MAIN_LIB_SOURCES)

#string(TIMESTAMP _buildDate "%Y-%m-%d %H:%M")

#polar_add_executable(polar main.cpp ${POLAR_MAIN_LIB_SOURCES})
#set_target_properties(polar PROPERTIES COMPILE_DEFINITIONS "BUILD_TIME=\"${_buildDate}\"")
#target_link_libraries(polar PUBLIC CLI11::CLI11)
#install(TARGETS polar RUNTIME
#   DESTINATION bin
#   COMPONENT corebins)
