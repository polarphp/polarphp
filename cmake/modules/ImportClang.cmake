# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/11/16.

include(ExternalProject)

ExternalProject_Add(clang-project
   PREFIX clang
   SOURCE_DIR "${POLAR_CLANG_SOURCE_DIR}"
   INSTALL_DIR "${POLAR_DEPS_INSTALL_DIR}"
   CMAKE_CACHE_ARGS
   -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
   -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
   -DCMAKE_BUILD_TYPE:STRING=Release
   -DLLVM_CMAKE_PATH:STRING=${POLAR_CMAKE_MODULES_DIR}/llvm
   -DPOLAR_CMAKE_MODULES_DIR:PATH=${POLAR_CMAKE_MODULES_DIR}
   -DPOLAR_DEPS_INSTALL_DIR:PATH=${POLAR_DEPS_INSTALL_DIR}
   -DLLVM_EXPORT_SYMBOLS_FOR_PLUGINS:BOOL=OFF
   -DLLVM_INCLUDE_TESTS:BOOL=OFF
   -DLLVM_ENABLE_EH:BOOL=ON
   -DLLVM_ENABLE_RTTI:BOOL=ON
   -DBUILD_SHARED_LIBS:BOOL=OFF
#   -DBUILD_STATIC_LIBS:BOOL=ON
   -DCMAKE_INSTALL_PREFIX:PATH=${POLAR_DEPS_INSTALL_DIR}
   -DCMAKE_INSTALL_RPATH:PATH=${POLAR_DEPS_INSTALL_DIR}/lib
   -DCMAKE_CXX_FLAGS:STRING=-w
   BUILD_COMMAND make -j
   DEPENDS llvm-project
   )

find_package(Clang
   PATHS ${POLAR_CMAKE_MODULES_DIR}/clang
   NO_DEFAULT_PATH)

if (Clang_FOUND)
   message("found clang compiler")
else()
   message(FATAL_ERROR "clang is required to build polarphp")
endif()
