# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
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
   DIR bytecode
   OUTPUT_VAR POLAR_BYTECODE_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR codegen
   OUTPUT_VAR POLAR_CODEGEN_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR driver
   OUTPUT_VAR POLAR_DRIVER_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR frontend
   OUTPUT_VAR POLAR_FRONTEND_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR frontendtool
   OUTPUT_VAR POLAR_FRONTEND_TOOL_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR basic
   OUTPUT_VAR POLAR_BASIC_HEADERS)

if (UNIX)
   list(APPEND POLAR_BASIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/basic/internal/_platform/TaskQueueImplUnix.h)
else()
   list(APPEND POLAR_BASIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/basic/internal/_platform/TaskQueueImplDefault.h)
endif()

polar_collect_files(
   TYPE_HEADER
   DIR pil
   OUTPUT_VAR POLAR_PIL_HEADERS)

#polar_collect_files(
#   TYPE_HEADER
#   DIR parser
#   OUTPUT_VAR POLAR_PARSER_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR llparser
   OUTPUT_VAR POLAR_LLPARSER_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR demangling
   OUTPUT_VAR POLAR_DEMANGLING_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR markup
   OUTPUT_VAR POLAR_MARKUP_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR option
   OUTPUT_VAR POLAR_OPTION_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR reflection
   OUTPUT_VAR POLAR_REFLECTION_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR sema
   OUTPUT_VAR POLAR_SEMA_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR serialization
   OUTPUT_VAR POLAR_SERIALIZATION_HEADERS)

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
polar_merge_list(POLAR_HEADERS POLAR_BYTECODE_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_CODEGEN_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_DRIVER_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_FRONTEND_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_FRONTEND_TOOL_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_BASIC_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_PIL_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_LLPARSER_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_DEMANGLING_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_OPTION_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_MARKUP_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_REFLECTION_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_SEMA_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_SERIALIZATION_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_SYNTAX_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_UTILS_HEADERS)
polar_merge_list(POLAR_HEADERS POLAR_RUNTIME_HEADERS)

add_subdirectory(syntax)
add_subdirectory(parser)
add_subdirectory(option)

configure_file(global/PolarVersion.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/global/PolarVersion.h
   @ONLY)
configure_file(global/BuildDefs.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/global/BuildDefs.h
   @ONLY)

configure_file(global/Config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/global/Config.h @ONLY)
configure_file(global/AbiBreaking.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/global/AbiBreaking.h @ONLY)
list(APPEND POLAR_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/global/Config.h)
list(APPEND POLAR_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/global/AbiBreaking.h)

set(POLAR_HEADERS ${POLAR_HEADERS} PARENT_SCOPE)
