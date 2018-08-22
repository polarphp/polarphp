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
   ../global/php_stdint.h
   zend_alloc_sizes.h
   zend_alloc.h
   zend_alloc.c
   zend_API.h
   zend_API.c
   zend_arena.h
   zend_ast.h
   zend_ast.c
   zend_bitset.h
   zend_build.h
   zend_builtin_functions.h
   zend_builtin_functions.c
   zend_closures.h
   zend_closures.c
   zend_compile.h
   zend_compile.c
   zend_constants.h
   zend_constants.c
   zend_cpuinfo.h
   zend_cpuinfo.c
   zend_default_classes.c
   zend_dtrace.h
   zend_errors.h
   zend_exceptions.h
   zend_exceptions.c
   zend_execute_API.c
   zend_execute.h
   zend_execute.c
   zend_extensions.h
   zend_extensions.c
   zend_float.h
   zend_float.c
   zend_gc.h
   zend_gc.c
   zend_generators.h
   zend_generators.c
   zend_globals_macros.h
   zend_globals.h
   zend_hash.h
   zend_hash.c
   zend_highlight.h
   zend_highlight.c
   zend_inheritance.h
   zend_inheritance.c
   zend_ini.h
   zend_ini.c
   zend_interfaces.h
   zend_interfaces.c
   zend_istdiostream.h
   zend_iterators.h
   zend_iterators.c
   zend_list.h
   zend_list.c
   zend_llist.h
   zend_llist.c
   zend_long.h
   zend_modules.h
   zend_multibyte.h
   zend_multibyte.c
   zend_multiply.h
   zend_object_handlers.h
   zend_object_handlers.c
   zend_objects_API.h
   zend_objects_API.c
   zend_objects.h
   zend_objects.c
   zend_opcode.c
   zend_operators.h
   zend_operators.c
   zend_portability.h
   zend_ptr_stack.h
   zend_ptr_stack.c
   zend_range_check.h
   zend_signal.h
   zend_signal.c
   zend_smart_str_public.h
   zend_smart_str.h
   zend_smart_str.c
   zend_smart_string_public.h
   zend_smart_string.h
   zend_sort.h
   zend_sort.c
   zend_stack.c
   zend_stack.h
   zend_stream.h
   zend_stream.c
   zend_string.h
   zend_string.c
   zend_strtod_int.h
   zend_strtod.c
   zend_ts_hash.h
   zend_ts_hash.c
   zend_type_info.h
   zend_types.h
   zend_variables.h
   zend_variables.c
   zend_virtual_cwd.h
   zend_virtual_cwd.c
   zend_vm_def.h
   zend_vm_execute.h
   zend_vm_handlers.h
   zend_vm_opcodes.h
   zend_vm_opcodes.c
   zend_vm_trace_handlers.h
   zend_vm_trace_map.h
   zend_vm.h
   zend.h
   zend.c
   zend_language_scanner_defs.h
   zend_language_scanner.h
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


add_library(zendVM SHARED ${zendVM_SRCS})
target_link_libraries(zendVM tsrm)
set_target_properties(zendVM
   PROPERTIES
   COMPILE_DEFINITIONS "ZEND_ENABLE_STATIC_TSRMLS_CACHE=1")
