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

polar_collect_files(
   TYPE_HEADER
   DIR polarphp/global
   OUTPUT_VAR POLAR_GLOBAL_HEADERS)

add_library(GlobalHeaders OBJECT ${POLAR_GLOBAL_HEADERS})

polar_collect_files(
   TYPE_HEADER
   DIR polarphp/vm/tsrm
   OUTPUT_VAR POLAR_ZRNDVM_TSRM_HEADERS)

add_library(TsrmHeaders OBJECT ${POLAR_ZRNDVM_TSRM_HEADERS})

polar_collect_files(
   TYPE_HEADER
   DIR polarphp/vm/zend
   OUTPUT_VAR POLAR_ZRNDVM_ZEND_HEADERS)

add_library(ZendHeaders OBJECT ${POLAR_ZRNDVM_ZEND_HEADERS})
target_link_libraries(ZendHeaders GlobalHeaders)

configure_file(polarphp/global/php_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/polarphp/global/php_config.h
   @ONLY)
configure_file(polarphp/vm/TSRM/tsrm_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/polarphp/vm/TSRM/tsrm_config.h
   @ONLY)
configure_file(polarphp/vm/Zend/zend_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/polarphp/vm/Zend/zend_config.h
   @ONLY)
