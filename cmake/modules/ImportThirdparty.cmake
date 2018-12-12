# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/08/21.

include(ExternalProject)

if (NOT EXISTS ${POLAR_DEPS_INSTALL_DIR}/include)
   file(MAKE_DIRECTORY ${POLAR_DEPS_INSTALL_DIR}/include)
endif()

ExternalProject_Add(thirdparty_cli11
   PREFIX thirdparty
   SOURCE_DIR "${POLAR_THIRDPARTY_DIR}/CLI11"
   INSTALL_DIR "${POLAR_DEPS_INSTALL_DIR}"
   CMAKE_CACHE_ARGS
   -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
   -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
   -DCLI11_TESTING:BOOL=OFF
   -DCLI11_EXAMPLES:BOOL=OFF
   -DCMAKE_BUILD_TYPE:STRING=Release
   -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
   -DBUILD_STATIC_LIBS:BOOL=${BUILD_STATIC_LIBS}
   -DCMAKE_INSTALL_PREFIX:PATH=${POLAR_DEPS_INSTALL_DIR}
   )

add_library(CLI11::CLI11 INTERFACE IMPORTED)
set_target_properties(CLI11::CLI11 PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${POLAR_DEPS_INSTALL_DIR}/include"
)

ExternalProject_Add(thirdparty_json
   PREFIX thirdparty
   SOURCE_DIR "${POLAR_THIRDPARTY_DIR}/json"
   INSTALL_DIR "${POLAR_DEPS_INSTALL_DIR}"
   CMAKE_CACHE_ARGS
   -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
   -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
   -DCMAKE_BUILD_TYPE:STRING=Release
   -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
   -DBUILD_STATIC_LIBS:BOOL=${BUILD_STATIC_LIBS}
   -DCMAKE_INSTALL_PREFIX:PATH=${POLAR_DEPS_INSTALL_DIR}
   -DJSON_BuildTests:BOOL=OFF
   )

find_package(nlohmann_json CONFIG REQUIRED
   PATHS ${POLAR_CMAKE_MODULES_DIR}/json)
message(STATUS "found json parser version: ${nlohmann_json_VERSION}")

if(POLAR_INCLUDE_TESTS)
   ExternalProject_Add(thirdparty_gtest
      PREFIX thirdparty
      SOURCE_DIR "${POLAR_THIRDPARTY_DIR}/googletest"
      INSTALL_DIR "${POLAR_DEPS_INSTALL_DIR}"
      CMAKE_CACHE_ARGS
      -DCMAKE_MACOSX_RPATH:BOOL=ON
      -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
      -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
      -DBUILD_GTEST:BOOL=ON
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
      -DBUILD_STATIC_LIBS:BOOL=${BUILD_STATIC_LIBS}
      -DCMAKE_INSTALL_PREFIX:PATH=${POLAR_DEPS_INSTALL_DIR})

# setup googletest targets
find_package(googletest CONFIG
   PATHS ${POLAR_CMAKE_MODULES_DIR}/googletest)
if (googletest_FOUND)
   set(POLAR_FOUND_NATIVE_GTEST ON)
endif()

endif()
