# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarPHP software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
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
   -DCLI11_TESTING:BOOL=OFF
   -DCLI11_EXAMPLES:BOOL=OFF
   -DCMAKE_BUILD_TYPE:BOOL=${CMAKE_BUILD_TYPE}
   -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
   -DBUILD_STATIC_LIBS:BOOL=${BUILD_STATIC_LIBS}
   -DCMAKE_INSTALL_PREFIX:PATH=${POLAR_DEPS_INSTALL_DIR}
   )



