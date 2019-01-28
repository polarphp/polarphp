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

include(AddFileDependencies)
include(CMakeParseArguments)

function(polar_replace_compiler_option var old new)
   # Replaces a compiler option or switch `old' in `var' by `new'.
   # If `old' is not in `var', appends `new' to `var'.
   # Example: polar_replace_compiler_option(CMAKE_CXX_FLAGS_RELEASE "-O3" "-O2")
   # If the option already is on the variable, don't add it:
   if( "${${var}}" MATCHES "(^| )${new}($| )" )
      set(n "")
   else()
      set(n "${new}")
   endif()
   if( "${${var}}" MATCHES "(^| )${old}($| )" )
      string( REGEX REPLACE "(^| )${old}($| )" " ${n} " ${var} "${${var}}" )
   else()
      set( ${var} "${${var}} ${n}" )
   endif()
   set( ${var} "${${var}}" PARENT_SCOPE )
endfunction(polar_replace_compiler_option)

function(polar_add_header_files_for_glob hdrs_out glob)
   file(GLOB hds ${glob})
   set(${hdrs_out} ${hds} PARENT_SCOPE)
endfunction(polar_add_header_files_for_glob)

function(polar_find_all_header_files hdrs_out additional_headerdirs)
   polar_add_header_files_for_glob(hds *.h)
   list(APPEND all_headers ${hds})

   foreach(additional_dir ${additional_headerdirs})
      polar_add_header_files_for_glob(hds "${additional_dir}/*.h")
      list(APPEND all_headers ${hds})
      polar_add_header_files_for_glob(hds "${additional_dir}/*.inc")
      list(APPEND all_headers ${hds})
   endforeach(additional_dir)

   set( ${hdrs_out} ${all_headers} PARENT_SCOPE )
endfunction(polar_find_all_header_files)

function(polar_process_sources OUT_VAR)
   cmake_parse_arguments(ARG "" "" "ADDITIONAL_HEADERS;ADDITIONAL_HEADER_DIRS" ${ARGN})
   set(sources ${ARG_UNPARSED_ARGUMENTS})
   # TODO need add opt to turn off this check
   # polar_check_source_file_list(${sources})
   if(MSVC_IDE OR XCODE)
      polar_find_all_header_files(hdrs "${ARG_ADDITIONAL_HEADER_DIRS}")
      if (hdrs)
         set_source_files_properties(${hdrs} PROPERTIES HEADER_FILE_ONLY ON)
      endif()
      set_source_files_properties(${ARG_ADDITIONAL_HEADERS} PROPERTIES HEADER_FILE_ONLY ON)
      list(APPEND sources ${ARG_ADDITIONAL_HEADERS} ${hdrs})
   endif()
   set(${OUT_VAR} ${sources} PARENT_SCOPE)
endfunction(polar_process_sources)

function(polar_check_source_file_list)
   cmake_parse_arguments(ARG "" "SOURCE_DIR" "" ${ARGN})
   set(listed ${ARG_UNPARSED_ARGUMENTS})
   if(ARG_SOURCE_DIR)
      file(GLOB globbed
         RELATIVE "${CMAKE_CURRENT_LIST_DIR}"
         "${ARG_SOURCE_DIR}/*.c" "${ARG_SOURCE_DIR}/*.cpp")
   else()
      file(GLOB globbed *.c *.cpp)
   endif()
   foreach(g ${globbed})
      get_filename_component(fn ${g} NAME)
      if(ARG_SOURCE_DIR)
         set(entry "${g}")
      else()
         set(entry "${fn}")
      endif()

      # Don't reject hidden files. Some editors create backups in the
      # same directory as the file.
      if (NOT "${fn}" MATCHES "^\\.")
         list(FIND POLAR_OPTIONAL_SOURCES ${entry} idx)
         if(idx LESS 0)
            list(FIND listed ${entry} idx)
            if(idx LESS 0)
               message(SEND_ERROR "Found unknown source file ${g}
                  Please update ${CMAKE_CURRENT_LIST_FILE}\n")
            endif()
         endif()
      endif()
   endforeach()
endfunction(polar_check_source_file_list)
