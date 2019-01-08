# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/09/19.

function(polar_configure_lit_site_cfg)
endfunction()

function(polar_get_lit_path)

endfunction()

function(polar_add_lit_target)

endfunction()

function(polar_add_lit_testsuite)

endfunction()

function(polar_add_lit_testsuites)

endfunction()

macro(polar_add_lit_extra_test_executable name)
   cmake_parse_arguments(ARG "" "DEPENDS;LINK_LIBS" "" ${ARGN})
   polar_process_sources(ALL_FILES ${ARG_UNPARSED_ARGUMENTS})
   string(REPLACE " " ";" ARG_DEPENDS "${ARG_DEPENDS}")
   string(REPLACE " " ";" ARG_LINK_LIBS "${ARG_LINK_LIBS}")
   if(EXCLUDE_FROM_ALL)
      add_executable(${name} EXCLUDE_FROM_ALL ${ALL_FILES})
   else()
      add_executable(${name} ${ALL_FILES})
   endif()
   if(NOT ARG_NO_INSTALL_RPATH)
      polar_setup_rpath(${name})
   endif()
   set(EXCLUDE_FROM_ALL OFF)
   polar_set_output_directory(${name} BINARY_DIR ${POLAR_LIT_TEST_BIN_DIR} LIBRARY_DIR ${POLAR_LIT_TEST_LIB_DIR})
   if(ARG_DEPENDS)
      add_dependencies(${name} ${ARG_DEPENDS})
   endif(ARG_DEPENDS)
   if (POLAR_THREADS_WORKING)
      # libpthreads overrides some standard library symbols, so main
      # executable must be linked with it in order to provide consistent
      # API for all shared libaries loaded by this executable.
      list(APPEND ARG_LINK_LIBS ${POLAR_THREADS_LIBRARY})
   endif()
   target_link_libraries(${name}
      PRIVATE
      ${ARG_LINK_LIBS} CLI11::CLI11
      ${POLAR_RT_REQUIRE_LIBS})
endmacro()

function(polar_add_lit_cfg_setter)
   cmake_parse_arguments(ARG "LOCAL;NEED_CONFIGURE" "" "EXTRA_SOURCES" ${ARGN})
   set(targetOutputName "")
   set(targetCfgSourceFilename "")
   if (ARG_LOCAL)
      set(targetOutputName localcfgsetter)
      set(targetCfgSourceFilename litlocalcfg.cpp)
      set(sourceFilename ${CMAKE_CURRENT_LIST_DIR}/${targetCfgSourceFilename})
      set(headerFilename ${CMAKE_CURRENT_LIST_DIR}/litlocalcfg.h)
      polar_get_lit_cfgsetter_name(localcfgsetter targetName)
   else()
      set(targetCfgSourceFilename litcfg.cpp)
      set(sourceFilename ${CMAKE_CURRENT_LIST_DIR}/${targetCfgSourceFilename})
      polar_get_lit_cfgsetter_name(cfgsetter targetName)
      set(headerFilename ${CMAKE_CURRENT_LIST_DIR}/litcfg.h)
      set(targetOutputName cfgsetter)
   endif()

   polar_find_parent_dir(${CMAKE_CURRENT_SOURCE_DIR} baseDir)
   set(setterModuleDir ${CMAKE_CURRENT_LIST_DIR})
   string(REPLACE ${baseDir}/ "" setterModuleDir ${setterModuleDir})
   set(setterModuleDir ${POLAR_SETTER_PLUGIN_DIR}/${setterModuleDir})
   if (ARG_NEED_CONFIGURE)
      set(headerTplFilename ${headerFilename}.in)
      set(headerFilename ${setterModuleDir}/${headerFilename})
      if (EXISTS ${headerTplFilename})
         message("${headerTplFilename}")
         message("${headerFilename}")
         configure_file(${headerTplFilename} ${headerFilename}
            @ONLY)
         list(APPEND POLAR_CFG_SETTER_SRCS ${sourceFilename})
      endif()
   endif()
   list(APPEND POLAR_CFG_SETTER_SRCS ${sourceFilename})
   foreach(_filename ${ARG_EXTRA_SOURCES})
      list(APPEND POLAR_CFG_SETTER_SRCS ${CMAKE_CURRENT_LIST_DIR}/${_filename})
   endforeach()
   set(POLAR_CFG_SETTER_SRCS ${POLAR_CFG_SETTER_SRCS} PARENT_SCOPE)
endfunction()

macro(polar_get_lit_cfgsetter_name suffix output)
   get_filename_component(_setterName ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
   get_filename_component(_setterBaseDir ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
   string(REPLACE ${_setterBaseDir}/ "" _setterName ${_setterName})
   string(REPLACE / "" _setterName ${_setterName})
   set(_setterName ${_setterName}${suffix})
   set(${output} ${_setterName})
endmacro()

function(polar_setup_lit_cfg_setters)
   cmake_parse_arguments(ARG "" "TEST_DIR;OUTPUT_NAME;SKIP_DIRS;" "" ${ARGN})
   if (NOT EXISTS ${ARG_TEST_DIR} OR NOT IS_DIRECTORY ${ARG_TEST_DIR})
      message(FATAL_ERROR "test directory is not exist")
   endif()
   string(REPLACE " " ";" ARG_SKIP_DIRS "${ARG_SKIP_DIRS}")
   set(POLAR_SETTER_PLUGIN_DIR ${POLAR_LIT_RUNTIME_DIR}/${ARG_OUTPUT_NAME})
   set(POLAR_CFG_SETTER_SRCS)
   file(GLOB_RECURSE cfgSetterScripts RELATIVE ${ARG_TEST_DIR}
      *cfg.cmake)
   foreach(script ${cfgSetterScripts})
      include(${ARG_TEST_DIR}/${script})
   endforeach()
   list(TRANSFORM ARG_SKIP_DIRS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/ OUTPUT_VARIABLE ARG_SKIP_DIRS)
   # skip dirs filter
   set(filteredSources)
   foreach(_filename ${POLAR_CFG_SETTER_SRCS})
      set(found OFF)
      foreach(skip ${ARG_SKIP_DIRS})
         string(LENGTH ${skip} skipLength)
         string(SUBSTRING ${_filename} 0 ${skipLength} subFilename)
         if (subFilename STREQUAL skip)
            set(found ON)
         endif()
      endforeach()
      if (NOT found)
         list(APPEND filteredSources ${_filename})
      endif()
   endforeach()
   add_library(${ARG_OUTPUT_NAME} MODULE ${filteredSources})
   set_target_properties(${ARG_OUTPUT_NAME}
      PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY ${POLAR_SETTER_PLUGIN_DIR}
      LIBRARY_OUTPUT_NAME ${ARG_OUTPUT_NAME}
      COMPILE_DEFINITIONS "LIT_SOURCE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
   target_link_libraries(${ARG_OUTPUT_NAME} PRIVATE litkernel nlohmann_json::nlohmann_json)
endfunction()

macro(polar_get_cfgsetterplugin_path name output)
   set(${output} ${POLAR_SETTER_PLUGIN_DIR}${DIR_SEPARATOR}${name}${CMAKE_SHARED_MODULE_SUFFIX})
endmacro()
