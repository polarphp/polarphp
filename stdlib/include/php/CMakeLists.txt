# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/12/26.

set(STDLIB_HEADERS "")

polar_collect_files(
   TYPE_HEADER
   DIR global
   OUTPUT_VAR POLAR_GLOBAL_HEADERS)

polar_merge_list(STDLIB_HEADERS POLAR_GLOBAL_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR base
   OUTPUT_VAR POLAR_MODULE_BASE_HEADERS)

polar_merge_list(STDLIB_HEADERS POLAR_MODULE_BASE_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR kernel
   OUTPUT_VAR POLAR_KERNEL_HEADERS)

polar_merge_list(STDLIB_HEADERS POLAR_KERNEL_HEADERS)

polar_collect_files(
   TYPE_HEADER
   DIR vmbinder
   OUTPUT_VAR POLAR_VMBINDER_HEADERS)

polar_merge_list(STDLIB_HEADERS POLAR_VMBINDER_HEADERS)

set(STDLIB_HEADERS ${STDLIB_HEADERS} PARENT_SCOPE)
