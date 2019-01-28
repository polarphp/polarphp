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
      # TODO here we may detect clang version
      check_cxx_compiler_flag(-lc++fs HAVE_FS_LIB)
      if (HAVE_FS_LIB)
         polar_add_rt_require_lib(c++fs)
      else()
         check_cxx_compiler_flag(-lc++fs HAVE_EXPERIMENTAL_FS_LIB)
         if (HAVE_EXPERIMENTAL_FS_LIB)
            polar_add_rt_require_lib(c++experimental)
         endif()
      endif()
   elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      check_cxx_compiler_flag(-lstdc++fs HAVE_STDCXX_FS_LIB)
      if (HAVE_STDCXX_FS_LIB)
         polar_add_rt_require_lib(stdc++fs)
      endif()
   endif()
endif()
