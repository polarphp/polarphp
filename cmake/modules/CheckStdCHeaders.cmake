# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

# detect std c headers
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckCSourceRuns)
include(CheckLibraryExists)

macro(polar_check_stdc_headers)
   set(_checkHeaders stdarg.h string.h float.h)
   check_include_files(
      "${_checkHeaders}"
      _hasStdcHeaders)
   if (_hasStdcHeaders)
      set(HAVE_STDARG_H ON)
      set(HAVE_STRING_H ON)
      set(HAVE_FLOAT_H ON)
   endif()
   if (_hasStdcHeaders)
      check_symbol_exists(memchr string.h _hasMemchrFunc)
      if (NOT _hasMemchrFunc)
         set(_hasStdcHeaders OFF)
      endif()
   endif()
   if (_hasStdcHeaders)
      check_symbol_exists(free stdlib.h _hasMemchrFunc)
      if (NOT _hasMemchrFunc)
         set(_hasStdcHeaders OFF)
      endif()
   endif()
   file(READ ${POLAR_CMAKE_TEST_CODE_DIR}/test_stdc_code.c _testcode)
   check_c_source_runs("${_testcode}" runStdCHeaderTestCode)
   if (runStdCHeaderTestCode)
      set(STDC_HEADERS ON)
   endif()
endmacro()

macro(polar_check_dirent_headers)
   set(_headerDirEntry OFF)
   foreach(_header dirent.h sys/ndir.h sys/dir.h ndir.h)
      check_include_file(${_header} _haveDirHeader)
      if (_haveDirHeader)
         polar_generate_header_guard(${_header} _guardName)
         set(${_guardName} ON)
      else()
         set(_headerDirEntry ${_header})
         break()
      endif()
   endforeach()
   if ("${_headerDirEntry}" STREQUAL "dirent.h")
      check_library_exists(dir opendir "" _haveLibDir)
      if (_haveLibDir)
         polar_append_flag(-ldir CMAKE_EXE_LINKER_FLAGS)
      endif()
   else()
      check_library_exists(x opendir "" _haveLibDir)
      if (_haveLibDir)
         polar_append_flag(-lx CMAKE_EXE_LINKER_FLAGS)
      endif()
   endif()
endmacro()
