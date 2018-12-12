# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

macro(polar_compile_env_summary_output)
   message(STATUS "--------------------------------------------------------------------------------------")
   message(STATUS "Thank for using polarphp project, have a lot of fun!")
   message(STATUS "--------------------------------------------------------------------------------------")
   message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
   message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
   message(STATUS "PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")
   message(STATUS "PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
   message(STATUS "CMAKE_ROOT: ${CMAKE_ROOT}")
   message(STATUS "CMAKE_SYSTEM: ${CMAKE_SYSTEM}")
   message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
   message(STATUS "CMAKE_SKIP_RPATH: ${CMAKE_SKIP_RPATH}")
   message(STATUS "CMAKE_VERBOSE_MAKEFILE: ${CMAKE_VERBOSE_MAKEFILE}")
   message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
   message(STATUS "CMAKE_C_COMPILER_VERSION: ${CMAKE_C_COMPILER_VERSION}")
   message(STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
   message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
   message(STATUS "CMAKE_CXX_COMPILER_VERSION: ${CMAKE_CXX_COMPILER_VERSION}")
   message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
   message(STATUS "--------------------------------------------------------------------------------------")
endmacro()
