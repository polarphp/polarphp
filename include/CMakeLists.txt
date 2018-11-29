# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/08/26.

set(POLAR_HEADERS "")

polar_collect_files(
   TYPE_HEADER
   DIR polarphp/global
   OUTPUT_VAR POLAR_GLOBAL_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR polarphp/basic
   OUTPUT_VAR POLARPHP_BASIC_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR polarphp/utils
   OUTPUT_VAR POLAR_UTILS_HEADERS)

polar_merge_list(POLAR_HEADERS POLAR_GLOBAL_HEADERS)
polar_merge_list(POLAR_HEADERS POLARPHP_BASIC_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_UTILS_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR polarphp/vm
   OUTPUT_VAR POLAR_ZENDVM_ZEND_HEADERS)

configure_file(polarphp/global/php_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/polarphp/global/php_config.h
   @ONLY)
configure_file(polarphp/global/Config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/polarphp/global/Config.h @ONLY)
configure_file(polarphp/global/AbiBreaking.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/polarphp/global/AbiBreaking.h @ONLY)
list(APPEND POLAR_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/polarphp/global/php_config.h)
list(APPEND POLAR_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/polarphp/global/Config.h)
list(APPEND POLAR_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/polarphp/global/AbiBreaking.h)

configure_file(polarphp/vm/tsrm/tsrm_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/polarphp/vm/tsrm/tsrm_config.h
   @ONLY)
configure_file(polarphp/vm/zend/zend_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/polarphp/vm/zend/zend_config.h
   @ONLY)

list(APPEND POLAR_ZENDVM_ZEND_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/polarphp/vm/zend/zend_config.h)
list(APPEND POLAR_ZENDVM_ZEND_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/polarphp/vm/tsrm/tsrm_config.h)
set(POLAR_ZENDVM_ZEND_HEADERS ${POLAR_ZENDVM_ZEND_HEADERS} PARENT_SCOPE)
set(POLAR_HEADERS ${POLAR_HEADERS} PARENT_SCOPE)
