# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/08/26.

set(POLAR_HEADERS "")

polar_collect_files(
   TYPE_HEADER
   DIR global
   OUTPUT_VAR POLAR_GLOBAL_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR ast
   OUTPUT_VAR POLAR_AST_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR basic
   OUTPUT_VAR POLAR_BASIC_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR parser
   OUTPUT_VAR POLAR_PARSER_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR syntax
   OUTPUT_VAR POLAR_SYNTAX_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR utils
   OUTPUT_VAR POLAR_UTILS_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR runtime
   OUTPUT_VAR POLAR_RUNTIME_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR vm
   OUTPUT_VAR POLAR_ZENDVM_ZEND_HEADERS)

polar_merge_list(POLAR_HEADERS POLAR_GLOBAL_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_AST_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_BASIC_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_PARSER_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_SYNTAX_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_UTILS_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_RUNTIME_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_ZENDVM_ZEND_HEADERS)

configure_file(global/php_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/polarphp/global/php_config.h
   @ONLY)
configure_file(global/PolarVersion.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/polarphp/global/PolarVersion.h
   @ONLY)
configure_file(global/BuildDefs.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/polarphp/global/BuildDefs.h
   @ONLY)

configure_file(global/Config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/polarphp/global/Config.h @ONLY)
configure_file(global/AbiBreaking.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/polarphp/global/AbiBreaking.h @ONLY)
list(APPEND POLAR_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/polarphp/global/php_config.h)
list(APPEND POLAR_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/polarphp/global/Config.h)
list(APPEND POLAR_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/polarphp/global/AbiBreaking.h)

configure_file(vm/tsrm/tsrm_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/polarphp/vm/tsrm/tsrm_config.h
   @ONLY)
configure_file(vm/zend/zend_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/polarphp/vm/zend/zend_config.h
   @ONLY)

list(APPEND POLAR_ZENDVM_ZEND_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/polarphp/vm/zend/zend_config.h)
list(APPEND POLAR_ZENDVM_ZEND_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/polarphp/vm/tsrm/tsrm_config.h)
set(POLAR_ZENDVM_ZEND_HEADERS ${POLAR_ZENDVM_ZEND_HEADERS} PARENT_SCOPE)
set(POLAR_HEADERS ${POLAR_HEADERS} PARENT_SCOPE)
