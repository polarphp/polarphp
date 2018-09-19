# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
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

function(polar_add_lit_cfg_setter)
   cmake_parse_arguments(ARG "LOCAL" "" "" ${ARGN})
   if (ARG_LOCAL)
      set(sourceFilename ${CMAKE_CURRENT_LIST_DIR}/lit.local.cfg.cpp)
      polar_get_lit_cfgsetter_name(localcfgsetter targetName)
   else()
      set(sourceFilename ${CMAKE_CURRENT_LIST_DIR}/lit.cfg.cpp)
      polar_get_lit_cfgsetter_name(cfgsetter targetName)
   endif()
   add_library(${targetName} ${sourceFilename})
endfunction()

macro(polar_get_lit_cfgsetter_name suffix output)
   get_filename_component(_setterName ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
   get_filename_component(_setterBaseDir ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
   string(REPLACE ${_setterBaseDir}/ "" _setterName ${_setterName})
   string(REPLACE / "" _setterName ${_setterName})
   set(_setterName ${_setterName}${suffix})
   set(${output} ${_setterName})
endmacro()

macro(polar_register_localcfg_setter)
   polar_get_lit_cfgsetter_name(localcfgsetter _setterName)
   list(APPEND POLAR_LIT_LOCALCFG_SETTER_POOL ${_setterName})
   polar_add_lit_cfg_setter(LOCAL)
endmacro()

macro(polar_register_cfg_setter)
   polar_get_lit_cfgsetter_name(cfgsetter _setterName)
   list(APPEND POLAR_LIT_CFG_SETTER_POOL ${_setterName})
   polar_add_lit_cfg_setter()
endmacro()

function(polar_setup_lit_cfg_setters)
   cmake_parse_arguments(ARG "" "TEST_DIR;OUTPUT_DIR" "" ${ARGN})
   if (NOT EXISTS ${ARG_TEST_DIR} OR NOT IS_DIRECTORY ${ARG_TEST_DIR})
      message(FATAL_ERROR "test directory is not exist")
   endif()
   set(setterPluginDir ${POLAR_LIT_CFG_SETTER_DIR}/${ARG_OUTPUT_DIR})
   file(GLOB_RECURSE cfgSetterScripts RELATIVE ${ARG_TEST_DIR}
      *cfg.cmake)
   set(POLAR_LIT_CFG_SETTER_POOL "")
   set(POLAR_LIT_LOCALCFG_SETTER_POOL)
   foreach(script ${cfgSetterScripts})
      include(${ARG_TEST_DIR}/${script})
   endforeach()
   list(REMOVE_DUPLICATES POLAR_LIT_CFG_SETTER_POOL)
   list(REMOVE_DUPLICATES POLAR_LIT_LOCALCFG_SETTER_POOL)
   set(setterJsonStr "{\"cfgsetters\":[__CFG_SETTERS_],\"localcfgsetters\":[__LOCAL_CFG_SETTERS__]}")
   set(listStr "")
   list(LENGTH POLAR_LIT_CFG_SETTER_POOL listSize)
   if (listSize GREATER 0)
      string(JOIN "\", \"" listStr ${POLAR_LIT_CFG_SETTER_POOL})
      set(listStr "\"${listStr}\"")
   else()
      set(listStr "")
   endif()
   string(REPLACE __CFG_SETTERS_ ${listStr} setterJsonStr ${setterJsonStr})
   list(LENGTH POLAR_LIT_LOCALCFG_SETTER_POOL listSize)
   if (listSize GREATER 0)
      string(JOIN "\", \"" listStr ${POLAR_LIT_LOCALCFG_SETTER_POOL})
      set(listStr "\"${listStr}\"")
   else()
      set(listStr "")
   endif()
   string(REPLACE __LOCAL_CFG_SETTERS__ "${listStr}" setterJsonStr ${setterJsonStr})
   set(setterJsonFilename ${setterPluginDir}/CfgSetterPool.json)
   file(WRITE ${setterJsonFilename} ${setterJsonStr})
endfunction()
