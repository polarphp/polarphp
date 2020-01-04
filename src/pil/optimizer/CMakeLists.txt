# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/12/17.

set(POLAR_PIL_OPTIMIZER_SOURCES)

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLAR_PIL_OPTIMIZER_SOURCES)

polar_merge_list(POLAR_PIL_OPTIMIZER_SOURCES POLAR_HEADERS)

set(PILOPTIMIZER_SOURCES)

function(_list_transform newvar)
  set(sources ${ARGN})
  set(dir ${CMAKE_CURRENT_SOURCE_DIR})
  set(tmp)
  foreach (s ${sources})
    list(APPEND tmp "${dir}/${s}")
  endforeach()
  set(${newvar} "${tmp}" PARENT_SCOPE)
endfunction()

macro(pilopt_register_sources)
  precondition(new_transformed_sources
    NEGATE
    MESSAGE "Expected this to be empty since we clear after each run")
  _list_transform(new_transformed_sources ${ARGN})
  list_union("${PILOPTIMIZER_SOURCES}" "${new_transformed_sources}" out)
  set(PILOPTIMIZER_SOURCES "${out}" PARENT_SCOPE)
  set(new_transformed_sources)
endmacro()

polar_add_host_library(PolarPILOptimizer STATIC
   ${POLAR_PIL_OPTIMIZER_SOURCES})

target_link_libraries(PolarPILOptimizer PRIVATE PolarPIL)
