# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarPHP software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
#
# Created by polarboy on 2018/08/22.
include(DetermineGCCCompatible)

if(NOT DEFINED POLAR_STDLIB_HANDLED)
   set(POLAR_STDLIB_HANDLED ON)
   include(CheckCXXCompilerFlag)
   if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      check_cxx_compiler_flag("-stdlib=libc++" CXX_SUPPORTS_STDLIB)
      if(CXX_SUPPORTS_STDLIB)
         polar_append_flag("-stdlib=libc++"
            CMAKE_CXX_FLAGS CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS
            CMAKE_MODULE_LINKER_FLAGS)
      else()
         message(WARNING "Can't specify libc++ with '-stdlib='")
      endif()
   endif()
endif()
