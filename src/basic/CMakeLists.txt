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

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLARPHP_BASIC_SOURCES)

set(platformDir ${CMAKE_CURRENT_SOURCE_DIR}/_platform)

# On non-Darwin require UUID.
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
   set(UUID_INCLUDE "")
   set(UUID_LIBRARIES "")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
   set(UUID_INCLUDE "")
   set(UUID_LIBRARIES "rpcrt4.lib")
else()
   find_package(UUID REQUIRED)
   set(UUID_INCLUDE "${UUID_INCLUDE_DIRS}")
endif()

# Figure out if we can track VC revisions.
# FIXME: Copied from Clang.
function(find_first_existing_file out_var)
   foreach(file ${ARGN})
      if(EXISTS "${file}")
         set(${out_var} "${file}" PARENT_SCOPE)
         return()
      endif()
   endforeach()
endfunction()

polar_merge_list(POLARPHP_BASIC_SOURCES POLAR_HEADERS)

if (UNIX)
   # TODO #if LLVM_ON_UNIX && !defined(__CYGWIN__) && !defined(__HAIKU__)
   list(APPEND POLARPHP_BASIC_SOURCES ${platformDir}/TaskQueueUnix.cpp)
else()
   list(APPEND POLARPHP_BASIC_SOURCES ${platformDir}/TaskQueueDefault.cpp)
endif()

if (APPLE)
   list(APPEND POLARPHP_BASIC_SOURCES ${platformDir}/CacheDarwin.cpp)
else()
   list(APPEND POLARPHP_BASIC_SOURCES ${platformDir}/CacheDefault.cpp)
endif()

polar_add_host_library(PolarBasic STATIC
   ${POLARPHP_BASIC_SOURCES}
   )

target_include_directories(PolarBasic PRIVATE
  ${UUID_INCLUDE})

target_link_libraries(PolarBasic INTERFACE
   PolarGlobal
   ${UUID_LIBRARIES}
   nlohmann_json::nlohmann_json
   LLVMSupport
   LLVMCore)

set_target_properties(
   PolarBasic
   PROPERTIES
   INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR};"
   INTERFACE_LINK_LIBRARIES "PolarGlobal;nlohmann_json::nlohmann_json;LLVMSupport;LLVMCore")

add_dependencies(PolarBasic
   llvm-project
   clang-project
   thirdparty_cli11
)


