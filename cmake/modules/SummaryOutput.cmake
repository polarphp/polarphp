# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

message(STATUS "--------------------------------------------------------------------------------------")
message(STATUS "Thank for using polarphp project, have a lot of fun!")
message(STATUS "--------------------------------------------------------------------------------------")

if(POLAR_INCLUDE_TOOLS)
   message(STATUS "Building host polarphp tools for ${POLAR_HOST_VARIANT_SDK} ${POLAR_HOST_VARIANT_ARCH}")
   message(STATUS "  Build type:     ${CMAKE_BUILD_TYPE}")
   message(STATUS "  Assertions:     ${LLVM_ENABLE_ASSERTIONS}")
   message(STATUS "  LTO:            ${POLAR_TOOLS_ENABLE_LTO}")
   message(STATUS "")
else()
   message(STATUS "Not building host polarphp tools")
   message(STATUS "")
endif()

if(POLAR_BUILD_STDLIB OR POLAR_BUILD_SDK_OVERLAY)
   message(STATUS "Building polarphp standard library and overlays for SDKs: ${POLAR_SDKS}")
   message(STATUS "  Build type:       ${POLAR_STDLIB_BUILD_TYPE}")
   message(STATUS "  Assertions:       ${POLAR_STDLIB_ASSERTIONS}")
   message(STATUS "")

   message(STATUS "Building polarphp runtime with:")
   message(STATUS "  Leak Detection Checker Entrypoints: ${POLAR_RUNTIME_ENABLE_LEAK_CHECKER}")
   message(STATUS "")
else()
   message(STATUS "Not building polarphp standard library, SDK overlays, and runtime")
   message(STATUS "")
endif()

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
message(STATUS "CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "--------------------------------------------------------------------------------------")

