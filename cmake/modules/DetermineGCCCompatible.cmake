# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/08/22.

# Determine if the compiler has GCC-compatible command-line syntax.

if(NOT DEFINED POLAR_COMPILER_IS_GCC_COMPATIBLE)
   if(CMAKE_COMPILER_IS_GNUCXX)
      set(POLAR_COMPILER_IS_GCC_COMPATIBLE ON)
   elseif(MSVC)
      set(POLAR_COMPILER_IS_GCC_COMPATIBLE OFF)
   elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
      set(POLAR_COMPILER_IS_GCC_COMPATIBLE ON)
   elseif( "${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
      set(POLAR_COMPILER_IS_GCC_COMPATIBLE ON)
   endif()
endif()
