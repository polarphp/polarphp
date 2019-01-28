# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/12/10.

set(STDLIB_SOURCES)

polar_collect_files(
   TYPE_BOTH
   DIR global
   OUTPUT_VAR STDLIB_GLOBAL_SOURCES)

polar_merge_list(STDLIB_SOURCES STDLIB_GLOBAL_SOURCES)

polar_collect_files(
   TYPE_BOTH
   DIR kernel
   OUTPUT_VAR STDLIB_KERNEL_SOURCES)

polar_merge_list(STDLIB_SOURCES STDLIB_KERNEL_SOURCES)

polar_collect_files(
   TYPE_BOTH
   DIR module/base
   OUTPUT_VAR STDLIB_MODULE_BASE_SOURCES)

polar_merge_list(STDLIB_SOURCES STDLIB_MODULE_BASE_SOURCES)

polar_collect_files(
   TYPE_BOTH
   DIR vmbinder
   OUTPUT_VAR STDLIB_VM_BINDER_SOURCES)

polar_merge_list(STDLIB_SOURCES STDLIB_VM_BINDER_SOURCES)
polar_merge_list(STDLIB_SOURCES STDLIB_HEADERS)

polar_add_library(Stdlib SHARED
   ${STDLIB_SOURCES}
   LINK_LIBS ZendApi)

set_target_properties(
   Stdlib
   PROPERTIES
   INTERFACE_INCLUDE_DIRECTORIES "${POLARPHP_STDLIB_INCLUDE_DIR}")
