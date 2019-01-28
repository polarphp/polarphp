# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
# Check if the host compiler is new enough. POLAR requires at least GCC 4.8,
# MSVC 2015 (Update 3), or Clang 3.1.

include(CheckCXXSourceCompiles)

if(NOT DEFINED POLAR_COMPILER_CHECKED)
   set(POLAR_COMPILER_NAME ${CMAKE_CXX_COMPILER_ID})
   set(POLAR_COMPILER_NAME ${CMAKE_CXX_COMPILER_ID})
   string(TOLOWER ${POLAR_COMPILER_NAME} POLAR_COMPILER_NORMAL_NAME)
   set(POLAR_COMPILER_CHECKED ON)
   # checking cxx compiler no
   if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7)
         message(FATAL_ERROR "Host GCC version must be at least 7!")
      endif()
      set(POLAR_CC_GCC ON)
   elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
      set(POLAR_CC_CLANG ON)
      if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5)
         message(FATAL_ERROR "Host Clang version must be at least 5!")
      endif()
      if (CMAKE_CXX_SIMULATE_ID MATCHES "MSVC")
         if (CMAKE_CXX_SIMULATE_VERSION VERSION_LESS 19.0)
            message(FATAL_ERROR "Host Clang must have at least -fms-compatibility-version=19.0")
         endif()
         set(CLANG_CL 1)
      elseif(NOT POLAR_ENABLE_LIBCXX)
         # Otherwise, test that we aren't using too old of a version of libstdc++
         # with the Clang compiler. This is tricky as there is no real way to
         # check the version of libstdc++ directly. Instead we test for a known
         # bug in libstdc++4.6 that is fixed in libstdc++4.7.
         set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
         set(OLD_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
         set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -std=c++0x")
         check_cxx_source_compiles("
            #include <atomic>
            std::atomic<float> x(0.0f);
            int main() { return (float)x; }"
            POLAR_NO_OLD_LIBSTDCXX)
         if(NOT POLAR_NO_OLD_LIBSTDCXX)
            message(FATAL_ERROR "Host Clang must be able to find libstdc++4.8 or newer!")
         endif()
         set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})
         set(CMAKE_REQUIRED_LIBRARIES ${OLD_CMAKE_REQUIRED_LIBRARIES})
      endif()
   elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
      set(POLAR_CC_MSVC ON)
      if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0)
         message(FATAL_ERROR "Host Visual Studio must be at least 2015")
      elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.00.24213.1)
         message(WARNING "Host Visual Studio should at least be 2015 Update 3 (MSVC 19.00.24213.1)"
            "  due to miscompiles from earlier versions")
      endif()
   endif()

   #checking c compiler
   if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set(POLAR_CC_GCC ON)
      if(CMAKE_C_COMPILER_VERSION VERSION_LESS 7)
         message(FATAL_ERROR "Host GCC version must be at least 7!")
      endif()
   elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
      set(POLAR_CC_CLANG ON)
      if(CMAKE_C_COMPILER_VERSION VERSION_LESS 5)
         message(FATAL_ERROR "Host Clang version must be at least 5!")
      endif()
      if (CMAKE_C_SIMULATE_ID MATCHES "MSVC")
         if (CMAKE_CXX_SIMULATE_VERSION VERSION_LESS 19.0)
            message(FATAL_ERROR "Host Clang must have at least -fms-compatibility-version=19.0")
         endif()
         set(CLANG_CL 1)
      endif()
   elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
      set(POLAR_CC_MSVC ON)
      if(CMAKE_C_COMPILER_VERSION VERSION_LESS 19.0)
         message(FATAL_ERROR "Host Visual Studio must be at least 2015")
      elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.00.24213.1)
         message(WARNING "Host Visual Studio should at least be 2015 Update 3 (MSVC 19.00.24213.1)"
            "  due to miscompiles from earlier versions")
      endif()
   endif()
endif()
