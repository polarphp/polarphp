# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

include(CMakeParseArguments)
include(CheckIncludeFiles)
include(CheckCSourceCompiles)
include(CheckSymbolExists)

function(polar_precondition var)
   cmake_parse_arguments(
      PRECONDITION # prefix
      "NEGATE" # options
      "MESSAGE" # single-value args
      "" # multi-value args
      ${ARGN})

   if (PRECONDITION_NEGATE)
      if (${var})
         if (PRECONDITION_MESSAGE)
            message(FATAL_ERROR "Error! ${PRECONDITION_MESSAGE}")
         else()
            message(FATAL_ERROR "Error! Variable ${var} is true or not empty. The value of ${var} is ${${var}}.")
         endif()
      endif()
   else()
      if (NOT ${var})
         if (PRECONDITION_MESSAGE)
            message(FATAL_ERROR "Error! ${PRECONDITION_MESSAGE}")
         else()
            message(FATAL_ERROR "Error! Variable ${var} is false, empty or not set.")
         endif()
      endif()
   endif()
endfunction()

# Assert is 'NOT ${LHS} ${OP} ${RHS}' is true.
function(polar_precondition_binary_op OP LHS RHS)
   cmake_parse_arguments(
      PRECONDITIONBINOP # prefix
      "NEGATE" # options
      "MESSAGE" # single-value args
      "" # multi-value args
      ${ARGN})

   if (PRECONDITIONBINOP_NEGATE)
      if (${LHS} ${OP} ${RHS})
         if (PRECONDITIONBINOP_MESSAGE)
            message(FATAL_ERROR "Error! ${PRECONDITIONBINOP_MESSAGE}")
         else()
            message(FATAL_ERROR "Error! ${LHS} ${OP} ${RHS} is true!")
         endif()
      endif()
   else()
      if (NOT ${LHS} ${OP} ${RHS})
         if (PRECONDITIONBINOP_MESSAGE)
            message(FATAL_ERROR "Error! ${PRECONDITIONBINOP_MESSAGE}")
         else()
            message(FATAL_ERROR "Error! ${LHS} ${OP} ${RHS} is false!")
         endif()
      endif()
   endif()
endfunction()


# Translate a yes/no variable to the presence of a given string in a
# variable.
#
# Usage:
#   translate_flag(is_set flag_name var_name)
#
# If is_set is true, sets ${var_name} to ${flag_name}. Otherwise,
# unsets ${var_name}.
function(polar_translate_flag is_set flag_name var_name)
   if(${is_set})
      set("${var_name}" "${flag_name}" PARENT_SCOPE)
   else()
      set("${var_name}" "" PARENT_SCOPE)
   endif()
endfunction()

macro(polar_translate_flags prefix options)
   foreach(var ${options})
      polar_translate_flag("${${prefix}_${var}}" "${var}" "${prefix}_${var}_keyword")
   endforeach()
endmacro()

# Set ${outvar} to ${${invar}}, asserting if ${invar} is not set.
function(polar_precondition_translate_flag invar outvar)
   polar_precondition(${invar})
   set(${outvar} "${${invar}}" PARENT_SCOPE)
endfunction()

function(polar_is_build_type_optimized build_type result_var_name)
   if("${build_type}" STREQUAL "Debug")
      set("${result_var_name}" FALSE PARENT_SCOPE)
   elseif("${build_type}" STREQUAL "RelWithDebInfo" OR
      "${build_type}" STREQUAL "Release" OR
      "${build_type}" STREQUAL "MinSizeRel")
      set("${result_var_name}" TRUE PARENT_SCOPE)
   else()
      message(FATAL_ERROR "Unknown build type: ${build_type}")
   endif()
endfunction()

function(polar_is_build_type_with_debuginfo build_type result_var_name)
   if("${build_type}" STREQUAL "Debug" OR
      "${build_type}" STREQUAL "RelWithDebInfo")
      set("${result_var_name}" TRUE PARENT_SCOPE)
   elseif("${build_type}" STREQUAL "Release" OR
      "${build_type}" STREQUAL "MinSizeRel")
      set("${result_var_name}" FALSE PARENT_SCOPE)
   else()
      message(FATAL_ERROR "Unknown build type: ${build_type}")
   endif()
endfunction()

# Set variable to value if value is not null or false. Otherwise set variable to
# default_value.
function(polar_set_with_default variable value)
   cmake_parse_argument(
      SWD
      ""
      "DEFAULT"
      "" ${ARGN})
   precondition(SWD_DEFAULT
      MESSAGE "Must specify a default argument")
   if (value)
      set(${variable} ${value} PARENT_SCOPE)
   else()
      set(${variable} ${SWD_DEFAULT} PARENT_SCOPE)
   endif()
endfunction()

function(polar_create_post_build_symlink target)
   set(options IS_DIRECTORY)
   set(oneValueArgs SOURCE DESTINATION WORKING_DIRECTORY COMMENT)
   cmake_parse_arguments(CS
      "${options}"
      "${oneValueArgs}"
      ""
      ${ARGN})

   if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
      if(CS_IS_DIRECTORY)
         set(cmake_symlink_option "copy_directory")
      else()
         set(cmake_symlink_option "copy_if_different")
      endif()
   else()
      set(cmake_symlink_option "create_symlink")
   endif()

   add_custom_command(TARGET "${target}" POST_BUILD
      COMMAND
      "${CMAKE_COMMAND}" "-E" "${cmake_symlink_option}"
      "${CS_SOURCE}"
      "${CS_DESTINATION}"
      WORKING_DIRECTORY "${CS_WORKING_DIRECTORY}"
      COMMENT "${CS_COMMENT}")
endfunction()

function(polar_dump_vars)
   set(POLAR_STDLIB_GLOBAL_CMAKE_CACHE)
   get_cmake_property(variableNames VARIABLES)
   foreach(variableName ${variableNames})
      if(variableName MATCHES "^POLAR")
         set(POLAR_STDLIB_GLOBAL_CMAKE_CACHE "${POLAR_STDLIB_GLOBAL_CMAKE_CACHE}set(${variableName} \"${${variableName}}\")\n")
         message("set(${variableName} \"${${variableName}}\")")
      endif()
   endforeach()
endfunction()

function(polar_is_sdk_requested name result_var_name)
   if("${POLAR_HOST_VARIANT_SDK}" STREQUAL "${name}")
      set("${result_var_name}" "TRUE" PARENT_SCOPE)
   else()
      if("${name}" IN_LIST POLARPHP_SDKS)
         set("${result_var_name}" "TRUE" PARENT_SCOPE)
      else()
         set("${result_var_name}" "FALSE" PARENT_SCOPE)
      endif()
   endif()
endfunction()

# Look for either a program in execute_process()'s path or for a hardcoded path.
# Find a program's version and set it in the parent scope.
# Replace newlines with spaces so it prints on one line.
function(polar_find_version cmd flag find_in_path)
   if(find_in_path)
      message(STATUS "Finding installed version for: ${cmd}")
   else()
      message(STATUS "Finding version for: ${cmd}")
   endif()
   execute_process(
      COMMAND ${cmd} ${flag}
      OUTPUT_VARIABLE out
      OUTPUT_STRIP_TRAILING_WHITESPACE)
   if(NOT out)
      if(find_in_path)
         message(STATUS "tried to find version for ${cmd}, but ${cmd} not found in path, continuing")
      else()
         message(FATAL_ERROR "tried to find version for ${cmd}, but ${cmd} not found")
      endif()
   else()
      string(REPLACE "\n" " " out2 ${out})
      message(STATUS "Found version: ${out2}")
   endif()
   message(STATUS "")
endfunction()

# Print out path and version of any installed commands.
# We migth be using the wrong version of a command, so record them all.
function(polar_print_versions)
   polar_find_version("cmake" "--version" TRUE)

   message(STATUS "Finding version for: ${CMAKE_COMMAND}")
   message(STATUS "Found version: ${CMAKE_VERSION}")
   message(STATUS "")

   get_filename_component(CMAKE_MAKE_PROGRAM_BN "${CMAKE_MAKE_PROGRAM}" NAME_WE)
   if(${CMAKE_MAKE_PROGRAM_BN} STREQUAL "ninja" OR
      ${CMAKE_MAKE_PROGRAM_BN} STREQUAL "make")
      polar_find_version(${CMAKE_MAKE_PROGRAM_BN} "--version" TRUE)
      polar_find_version(${CMAKE_MAKE_PROGRAM} "--version" FALSE)
   endif()

   if(${POLARPHP_PATH_TO_CMARK_BUILD})
      polar_find_version("cmark" "--version" TRUE)
      polar_find_version("${POLARPHP_PATH_TO_CMARK_BUILD}/src/cmark" "--version" FALSE)
   endif()

   message(STATUS "Finding version for: ${CMAKE_C_COMPILER}")
   message(STATUS "Found version: ${CMAKE_C_COMPILER_VERSION}")
   message(STATUS "")

   message(STATUS "Finding version for: ${CMAKE_CXX_COMPILER}")
   message(STATUS "Found version: ${CMAKE_CXX_COMPILER_VERSION}")
   message(STATUS "")
endfunction()

function(polar_append_flag value)
   foreach(variable ${ARGN})
      set(${variable} "${${variable}} ${value}" PARENT_SCOPE)
   endforeach(variable)
endfunction()

function(polar_append_flag_if condition value)
   if (${condition})
      foreach(variable ${ARGN})
         set(${variable} "${${variable}} ${value}" PARENT_SCOPE)
      endforeach(variable)
   endif()
endfunction()

macro(polar_add_flag_if_supported flag name)
   check_c_compiler_flag("-Werror ${flag}" "C_SUPPORTS_${name}")
   polar_append_flag_if("C_SUPPORTS_${name}" "${flag}" CMAKE_C_FLAGS)
   check_cxx_compiler_flag("-Werror ${flag}" "CXX_SUPPORTS_${name}")
   polar_append_flag_if("CXX_SUPPORTS_${name}" "${flag}" CMAKE_CXX_FLAGS)
endmacro()

function(polar_add_flag_or_print_warning flag name)
   check_c_compiler_flag("-Werror ${flag}" "C_SUPPORTS_${name}")
   check_cxx_compiler_flag("-Werror ${flag}" "CXX_SUPPORTS_${name}")
   if (C_SUPPORTS_${name} AND CXX_SUPPORTS_${name})
      message(STATUS "Building with ${flag}")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" PARENT_SCOPE)
      set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${flag}" PARENT_SCOPE)
   else()
      message(WARNING "${flag} is not supported.")
   endif()
endfunction()

function(polar_add_files target)
   list(REMOVE_AT ARGV 0)
   foreach(file ${ARGV})
      list(APPEND ${target} ${CMAKE_CURRENT_SOURCE_DIR}/${file})
   endforeach()
   set(${target} ${${target}} PARENT_SCOPE)
endfunction(polar_add_files)

function(polar_collect_files)
   cmake_parse_arguments(ARG "TYPE_HEADER;TYPE_SOURCE;TYPE_BOTH;OVERWRITE;RELATIVE" "OUTPUT_VAR;DIR;SKIP_DIR" "" ${ARGN})
   set(GLOB ${ARG_DIR})
   if(ARG_TYPE_BOTH OR (ARG_TYPE_HEADER AND ARG_TYPE_SOURCE))
      string(APPEND GLOB "/*[.h|.macros|.cpp]")
   elseif(ARG_TYPE_HEADER)
      string(APPEND GLOB "/*.h")
   elseif(ARG_TYPE_SOURCE)
      string(APPEND GLOB "/*.cpp")
   else()
      string(APPEND GLOB "/*.h")
   endif()
   if(ARG_OVERWRITE)
      set(${ARG_OUTPUT_VAR} "")
   endif()
   if (ARG_RELATIVE)
      file(GLOB_RECURSE TEMP_OUTPUTFILES
         LIST_DIRECTORIES false
         RELATIVE ${ARG_DIR}
         ${GLOB})
   else()
      file(GLOB_RECURSE TEMP_OUTPUTFILES
         LIST_DIRECTORIES false
         ${GLOB})
   endif()

   list(FILTER TEMP_OUTPUTFILES EXCLUDE REGEX "_platform/")
   if(ARG_SKIP_DIR)
      list(FILTER TEMP_OUTPUTFILES EXCLUDE REGEX ${ARG_SKIP_DIR})
   endif()
   list(APPEND ${ARG_OUTPUT_VAR} ${TEMP_OUTPUTFILES})
   set(${ARG_OUTPUT_VAR} ${${ARG_OUTPUT_VAR}} PARENT_SCOPE)
endfunction()

function (polar_generate_header_guard filename output)
   string(TOUPPER ${filename} filename)
   string(REPLACE "." "_" filename ${filename})
   string(REPLACE "/" "_" filename ${filename})
   set(${output} HAVE_${filename} PARENT_SCOPE)
endfunction()

function(polar_define_have name)
   polar_generate_header_guard(${name} haveName)
   set(${haveName} ON PARENT_SCOPE)
endfunction()

function(polar_append_flag value)
   foreach(variable ${ARGN})
      set(${variable} "${${variable}} ${value}" PARENT_SCOPE)
   endforeach(variable)
endfunction()

function(polar_libgcc_path path)
   execute_process(COMMAND gcc --print-libgcc-file-name
      RESULT_VARIABLE _retcode
      OUTPUT_VARIABLE _output)
   if (NOT _retcode EQUAL 0)
      message(FATAL_ERROR "execute command gcc --print-libgcc-file-name error: ${_output}")
   endif()
   get_filename_component(libdir ${_output} DIRECTORY)
   set(${path} ${libdir} PARENT_SCOPE)
endfunction()

macro(polar_find_parent_dir path output)
   get_filename_component(${output} ${path} ABSOLUTE)
   get_filename_component(${output} ${${output}} DIRECTORY)
endmacro()

macro(polar_merge_list target list)
   foreach(_listItem ${${list}})
      list(APPEND ${target} ${_listItem})
   endforeach()
   if (target)
      list(REMOVE_DUPLICATES ${target})
   endif()
endmacro()

macro(polar_add_rt_require_lib name)
   list(APPEND POLAR_RT_REQUIRE_LIBS ${name})
   if (NOT ${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
      set(POLAR_RT_REQUIRE_LIBS ${POLAR_RT_REQUIRE_LIBS} PARENT_SCOPE)
   endif()
endmacro()

macro(polar_check_library_exists library function location variable)
   check_library_exists("${library}" "${function}" "${location}" "${variable}")
   if (${${variable}})
      set(POLAR_${variable} ON)
   endif()
endmacro()

macro(polar_check_symbol_exists symbol files variable)
   check_symbol_exists("${symbol}" "${files}" "${variable}")
   if (${${variable}})
      set(POLAR_${variable} ON)
   endif()
endmacro()

macro(polar_detect_compiler_root_dir _targetDir)
   set(_tempDir ${CMAKE_CXX_COMPILER})
   get_filename_component(_tempDir ${_tempDir} DIRECTORY)
   get_filename_component(_tempDir ${_tempDir} DIRECTORY)
   set(${_targetDir} ${_tempDir})
endmacro()

# There is no clear way of keeping track of compiler command-line
# options chosen via `add_definitions'

# Beware that there is no implementation of remove_llvm_definitions.

macro(polar_add_definitions)
   # We don't want no semicolons on POLAR_DEFINITIONS:
   foreach(arg ${ARGN})
      if(DEFINED POLAR_COMPILE_DEFINITIONS)
         set(POLAR_COMPILE_DEFINITIONS "${POLAR_COMPILE_DEFINITIONS} ${arg}")
      else()
         set(POLAR_COMPILE_DEFINITIONS ${arg})
      endif()
   endforeach(arg)
   add_definitions(${ARGN})
endmacro(polar_add_definitions)

# utils function for check

macro(polar_check_headers)
   foreach(_filename ${ARGV})
      polar_generate_header_guard(${_filename} _guardName)
      check_include_file(${_filename} ${_guardName})
      if (${${_guardName}})
         set(POLAR_${_guardName} ON)
      endif()
   endforeach()
endmacro()

macro(polar_check_funcs)
   foreach(_func ${ARGV})
      string(TOUPPER ${_func} upcase)
      check_function_exists(${_func} HAVE_${upcase})
      if (${HAVE_${upcase}})
         set(POLAR_HAVE_${upcase} ON)
      endif()
   endforeach()
endmacro()

macro(polar_check_prog_awk)
   find_program(POLOAR_PROGRAM_AWK awk NAMES gawk nawk mawk
      PATHS /usr/xpg4/bin/)
   if (NOT POLOAR_PROGRAM_AWK)
      message(FATAL_ERROR "Could not find awk; Install GNU awk")
   else()
      if(POLOAR_PROGRAM_AWK MATCHES ".*mawk")
         message(WARNING "mawk is known to have problems on some systems. You should install GNU awk")
      else()
         message(STATUS "checking wether ${POLOAR_PROGRAM_AWK} is broken")
         execute_process(COMMAND ${POLOAR_PROGRAM_AWK} "function foo() {}" ">/dev/null 2>&1"
            RESULT_VARIABLE _awkExecRet)
         if (_awkExecRet)
            message(FATAL_ERROR "You should install GNU awk")
            unset(POLOAR_PROGRAM_AWK)
         else()
            message("${POLOAR_PROGRAM_AWK} is works")
         endif()
      endif()
   endif()
endmacro()


# We need to execute this script at installation time because the
# DESTDIR environment variable may be unset at configuration time.
# See PR8397.

function(polar_install_symlink name target outdir)
   if(UNIX)
      set(LINK_OR_COPY create_symlink)
      set(DESTDIR $ENV{DESTDIR})
   else()
      set(LINK_OR_COPY copy)
   endif()
   set(bindir "${DESTDIR}${CMAKE_INSTALL_PREFIX}/${outdir}/")
   message("Creating ${name}")
   execute_process(
      COMMAND "${CMAKE_COMMAND}" -E ${LINK_OR_COPY} "${target}" "${name}"
      WORKING_DIRECTORY "${bindir}")
endfunction()
