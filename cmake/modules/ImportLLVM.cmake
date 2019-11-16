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

ExternalProject_Add(llvm-project
   PREFIX llvm
   SOURCE_DIR "${POLAR_LLVM_SOURCE_DIR}"
   INSTALL_DIR "${POLAR_DEPS_INSTALL_DIR}"
   CMAKE_CACHE_ARGS
   -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
   -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
   -DCMAKE_BUILD_TYPE:STRING=Release
   -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
   -DBUILD_STATIC_LIBS:BOOL=${BUILD_STATIC_LIBS}
   -DCMAKE_INSTALL_PREFIX:PATH=${POLAR_DEPS_INSTALL_DIR}
   -DLLVM_ENABLE_ASSERTIONS:BOOL=ON
   -DLLVM_ENABLE_EH:BOOL=ON
   -DLLVM_ENABLE_RTTI:BOOL=ON
   -DCMAKE_CXX_FLAGS:STRING=-w
   BUILD_COMMAND make -j
   )

find_package(LLVM
   PATHS ${POLAR_CMAKE_MODULES_DIR}/llvm)
if (LLVM_FOUND)
   message("found LLVM version: ${LLVM_VERSION}")
else()
   message(FATAL_ERROR "LLVM is required to build polarphp")
endif()
