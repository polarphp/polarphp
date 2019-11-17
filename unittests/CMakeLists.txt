# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/10/12.

add_custom_target(PolarBaseLibTests)
set_target_properties(PolarBaseLibTests PROPERTIES FOLDER "PolarBaseLibTests")
add_custom_target(PolarCompilerTests)
set_target_properties(PolarCompilerTests PROPERTIES FOLDER "PolarCompilerTests")

add_subdirectory(support)
add_subdirectory(utils)

if (POLAR_DEV_BUILD_POLARPHP_UNITTEST)
   add_subdirectory(syntax)
   add_subdirectory(parser)
endif()
