# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/05/09.

set(POLAR_PARSER_INCLUDE_DIR ${POLAR_MAIN_INCLUDE_DIR}/polarphp/parser)
set(POLAR_SYNTAX_SRC_DIR ${POLAR_SOURCE_DIR}/src/syntax)
set(POLAR_TOKEN_DESC_MAP_FILE ${POLAR_SYNTAX_SRC_DIR}/TokenDescMap.cpp)
set(POLAR_GENERATE_TOKEN_DESC_MAP_SCRIPT ${POLAR_CMAKE_SCRIPTS_DIR}/GenerateTokenDescMap.php)

if ((NOT EXISTS ${POLAR_TOKEN_DESC_MAP_FILE}) OR
      (NOT (POLAR_GRAMMER_FILE_MD5_FOR_TOKEN_DESC_MAP AND POLAR_GRAMMER_FILE_MD5_FOR_TOKEN_DESC_MAP STREQUAL grammerFileHash)))
   execute_process(COMMAND ${PHP_EXECUTABLE}
      ${POLAR_GENERATE_TOKEN_DESC_MAP_SCRIPT}
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result
      ERROR_VARIABLE _error
      WORKING_DIRECTORY ${POLAR_SOURCE_DIR})
   if (NOT _result EQUAL 0)
      message(FATAL_ERROR ${_output})
   else()
      message("${_output}")
   endif()
   set(POLAR_GRAMMER_FILE_MD5_FOR_TOKEN_DESC_MAP ${grammerFileHash} CACHE STRING "language grammer file md5 value" FORCE)
   mark_as_advanced(POLAR_GRAMMER_FILE_MD5_FOR_TOKEN_DESC_MAP)
endif()

list(APPEND POLAR_COMPILER_SOURCES ${POLAR_TOKEN_DESC_MAP_FILE})
