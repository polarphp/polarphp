# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/12/10.

list(APPEND POLAR_ZENDVM_INCLUDE_DIRS
   ${POLAR_MAIN_INCLUDE_DIR}/polarphp/global
   ${POLAR_MAIN_INCLUDE_DIR}/polarphp/zend
   ${POLAR_MAIN_INCLUDE_DIR}/polarphp/tsrm
   ${POLAR_SOURCE_DIR}/src/vm/TSRM
   ${POLAR_SOURCE_DIR}/src/vm/Zend
   ${POLAR_INCLUDE_DIR}/polarphp/global
   ${POLAR_INCLUDE_DIR}/polarphp/vm/zend
   ${POLAR_INCLUDE_DIR}/polarphp/vm/tsrm)

add_subdirectory(TSRM)
add_subdirectory(Zend)
