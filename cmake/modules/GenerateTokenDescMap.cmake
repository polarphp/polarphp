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

macro(polar_generate_token_desc_source)
   file(STRINGS ${POLAR_GRAMMER_FILE} _fileLines
      REGEX "^%(token)")
   set(_processLines "")
   set(_mapItemStr "")
   foreach(_line ${_fileLines})
      string(REGEX MATCH "[][]" _hasBracket "${_line}")
      if (_hasBracket)
         string(FIND "${_line}" ";" _index)
         if (_index EQUAL -1)
            list(APPEND _processLines "${_line}")
         else()
            string(SUBSTRING "${_line}" 0 ${_index} _left)
            math(EXPR _index "${_index} + 1")
            string(SUBSTRING "${_line}" ${_index} -1 _right)
            string(REGEX REPLACE "[[]" "__LEFT_BRACKET__" _left "${_left}")
            string(REGEX REPLACE "[]]" "__RIGHT_BRACKET__" _right "${_right}")
            list(APPEND _processLines "${_left}")
            list(APPEND _processLines "${_right}")
         endif()
      else()
         string(REPLACE ";" "__SEMICOLON__" _line "${_line}")
         list(APPEND _processLines "${_line}")
      endif()
   endforeach()
   foreach(_line ${_processLines})
      string(REPLACE "%token" "" _line "${_line}")
      string(REPLACE "<ast>" "" _line "${_line}")
      string(REGEX REPLACE "[-0-9]+" "" _line "${_line}")
      string(REGEX MATCH "(\".*\")" _desc "${_line}")
      if (_desc)
         string(REPLACE "${_desc}" "" _line "${_line}")
      endif()
      string(STRIP "${_line}" _token)
      if (NOT _desc)
         set(_desc ${_token})
      else()
         string(STRIP "${_desc}" _desc)
         string(FIND "${_desc}" "\"" _startQuote)
         string(FIND "${_desc}" "\"" _stopQuote REVERSE)
         math(EXPR _startQuote "${_startQuote} + 1")
         math(EXPR _stopQuote "${_stopQuote} - 1")
         string(SUBSTRING "${_desc}" ${_startQuote} ${_stopQuote} _desc)
      endif()
      # get token text and token string
      string(REGEX MATCH "[(](.+)[)]" _tokenStr "${_desc}")
      string(REPLACE "${_tokenStr}" "" _tokenText "${_desc}")
      string(REGEX REPLACE "[)(]" "" _tokenStr "${_tokenStr}")
      string(STRIP "${_tokenText}" _tokenText)

      if (NOT _tokenText)
         set(_tokenText "")
      endif()
      if (NOT _tokenStr)
         set(_tokenStr "")
      endif()
      if (NOT _mapItemStr)
         set(_mapItemStr "{TokenKindType::${_token}, {\"${_tokenStr}\", \"${_tokenText}\", \"${_desc}\"}},")
      else ()
         string(APPEND _mapItemStr "\n      {TokenKindType::${_token}, {\"${_tokenStr}\", \"${_tokenText}\", \"${_desc}\"}},")
      endif()
      string(REPLACE "__LEFT_BRACKET__" "[" _mapItemStr "${_mapItemStr}")
      string(REPLACE "__RIGHT_BRACKET__" "]" _mapItemStr "${_mapItemStr}")
   endforeach()
   file(READ ${POLAR_TOKEN_DESC_MAP_FILE}.in _tplFileContent)
   string(REPLACE "__TOKEN_RECORDS__" "${_mapItemStr}" _tplFileContent "${_tplFileContent}")
   file(WRITE ${POLAR_TOKEN_DESC_MAP_FILE} "${_tplFileContent}")
endmacro()

if ((NOT EXISTS ${POLAR_TOKEN_DESC_MAP_FILE}) OR
      (NOT (POLAR_GRAMMER_FILE_MD5_FOR_TOKEN_DESC_MAP AND POLAR_GRAMMER_FILE_MD5_FOR_TOKEN_DESC_MAP STREQUAL grammerFileHash)))
   polar_generate_token_desc_source()
   set(POLAR_GRAMMER_FILE_MD5_FOR_TOKEN_DESC_MAP ${grammerFileHash} CACHE STRING "language grammer file md5 value" FORCE)
   mark_as_advanced(POLAR_GRAMMER_FILE_MD5_FOR_TOKEN_DESC_MAP)
endif()

list(APPEND POLAR_COMPILER_SOURCES ${POLAR_TOKEN_DESC_MAP_FILE})
