# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarPHP software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
# Check if the host compiler is new enough. POLAR requires at least GCC 4.8,
# MSVC 2015 (Update 3), or Clang 3.1.
#
# Created by polarboy on 2018/08/20.

set(zendVM_SRCS
   zend_alloc.c
   zend_API.c
   zend_ast.c
   zend_builtin_functions.c
   zend_closures.c
   zend_compile.c
   zend_constants.c
   zend_cpuinfo.c
   zend_default_classes.c
   zend_exceptions.c
   zend_execute_API.c
   zend_execute.c
   zend_extensions.c
   zend_float.c
   zend_gc.c
   zend_generators.c
   zend_hash.c
   zend_highlight.c
   zend_inheritance.c
   zend_ini.c
   zend_interfaces.c
   zend_iterators.c
   zend_list.c
   zend_llist.c
   zend_multibyte.c
   zend_object_handlers.c
   zend_objects_API.c
   zend_objects.c
   zend_opcode.c
   zend_operators.c
   zend_ptr_stack.c
   zend_signal.c
   zend_smart_str.c
   zend_sort.c
   zend_stack.c
   zend_stream.c
   zend_string.c
   zend_strtod.c
   zend_ts_hash.c
   zend_variables.c
   zend_virtual_cwd.c
   zend_vm_opcodes.c
   zend.c
   ${CMAKE_CURRENT_BINARY_DIR}/zend_language_scanner.c
   ${CMAKE_CURRENT_BINARY_DIR}/zend_language_parser.h
   ${CMAKE_CURRENT_BINARY_DIR}/zend_language_parser.c
   ${CMAKE_CURRENT_BINARY_DIR}/zend_ini_scanner.c
   ${CMAKE_CURRENT_BINARY_DIR}/zend_ini_parser.h
   ${CMAKE_CURRENT_BINARY_DIR}/zend_ini_parser.c
   )

if (ZEND_BROKEN_SPRINTF)
   list(APPEND zendVM_SRCS zend_sprintf.c)
endif()
if (HAVE_DTRACE)
   list(APPEND zendVM_SRCS zend_dtrace.c)
endif()

re2c_target(NAME ZendIniScannerLex
   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zend_ini_scanner.c
   INPUT ${CMAKE_CURRENT_SOURCE_DIR}/zend_ini_scanner.l
   OPTIONS --no-generation-date --case-inverted -cbdFt ${CMAKE_CURRENT_SOURCE_DIR}/zend_ini_scanner_defs.h.h)

bison_target(ZendIniParser zend_ini_parser.y ${CMAKE_CURRENT_BINARY_DIR}/zend_ini_parser.c
   COMPILE_FLAGS "-p ini_ -d"
   DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/zend_ini_parser.h)

re2c_target(NAME ZendLanguageScannerLex
   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zend_language_scanner.c
   INPUT ${CMAKE_CURRENT_SOURCE_DIR}/zend_language_scanner.l
   OPTIONS --no-generation-date --case-inverted -cbdFt ${CMAKE_CURRENT_SOURCE_DIR}/zend_language_scanner_defs.h)

bison_target(ZendLangParser zend_language_parser.y ${CMAKE_CURRENT_BINARY_DIR}/zend_language_parser.c
   COMPILE_FLAGS "-p zend -d"
   DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/zend_language_parser.h)

add_library(zendVM SHARED ${zendVM_SRCS} $<TARGET_OBJECTS:ZendHeaders>)
target_link_libraries(zendVM tsrm)
set_target_properties(zendVM
   PROPERTIES
   COMPILE_DEFINITIONS "ZEND_ENABLE_STATIC_TSRMLS_CACHE=1")

polar_collect_files(
   TYPE_HEADER
   DIR ${CMAKE_CURRENT_SOURCE_DIR}
   OUTPUT_VAR POLAR_ZRNDVM_ZEND_HEADERS)

list(APPEND POLAR_ZRNDVM_ZEND_HEADERS
   ${CMAKE_CURRENT_BINARY_DIR}/zend_ini_parser.h
   ${CMAKE_CURRENT_BINARY_DIR}/zend_language_parser.h)

install(FILES ${POLAR_ZRNDVM_ZEND_HEADERS} DESTINATION ${POLAR_HEADER_INSTALL_DIR}/vm/Zend
   COMPONENT ZendVMHeaders)
