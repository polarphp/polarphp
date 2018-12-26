# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/12/10.

set(POLARPHP_STDLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

include_directories(BEFORE ${POLARPHP_STDLIB_INCLUDE_DIR})
add_subdirectory(include)
add_subdirectory(src)
