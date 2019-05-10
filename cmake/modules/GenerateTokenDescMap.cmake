# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/05/09.

set(POLAR_PARSER_INCLUDE_DIR ${POLAR_MAIN_INCLUDE_DIR}/polarphp/parser)
set(POLAR_SYNTAX_SRC_DIR ${POLAR_SOURCE_DIR}/src/syntax)
set(POLAR_GRAMMER_FILE ${POLAR_PARSER_INCLUDE_DIR}/LangGrammer.y)
set(POLAR_TOKEN_DESC_MAP_FILE ${POLAR_SYNTAX_SRC_DIR}/TokenDescMap.cpp)

file(MD5 ${POLAR_GRAMMER_FILE} grammerFileHash)

macro(polar_generate_token_desc_source)
   file(STRINGS ${POLAR_GRAMMER_FILE} _fileLines
      REGEX "^%(token)")
   set(_mapItemStr "")
   foreach(_line ${_fileLines})
      string(REPLACE "%token" "" _line ${_line})
      string(REPLACE "<ast>" "" _line ${_line})
      string(REGEX REPLACE "[-0-9]+" "" _line ${_line})
      string(REGEX MATCH "(\".*\")" _desc ${_line})
      if (_desc)
         string(REPLACE ${_desc} "" _line ${_line})
      endif()
      string(STRIP ${_line} _token)
      if (NOT _desc)
         set(_desc \"${_token}\")
      endif()
      # get token text and token string
      string(REGEX MATCH "[(](.+)[)]" _tokenStr "${_desc}")
      string(REPLACE "${_tokenStr}" "" _tokenText "${_desc}")
      string(REGEX REPLACE "[)(]" "" _tokenStr "${_tokenStr}")
      string(REGEX REPLACE "\"" "" _tokenText "${_tokenText}")
      string(STRIP "${_tokenText}" _tokenText)
      if (NOT _tokenText)
         set(_tokenText "")
      endif()
      if (NOT _tokenStr)
         set(_tokenStr "")
      endif()
      if (NOT _mapItemStr)
         set(_mapItemStr "{TokenKindType::${_token}, {\"${_tokenStr}\", \"${_tokenText}\", ${_desc}}},")
      else ()
         string(APPEND _mapItemStr "\n      {TokenKindType::${_token}, {\"${_tokenStr}\", \"${_tokenText}\", ${_desc}}},")
      endif()
   endforeach()
   file(READ ${POLAR_TOKEN_DESC_MAP_FILE}.in _tplFileContent)
   string(REPLACE "__TOKEN_RECORDS__" "${_mapItemStr}" _tplFileContent "${_tplFileContent}")
   file(WRITE ${POLAR_TOKEN_DESC_MAP_FILE} "${_tplFileContent}")
endmacro()

polar_generate_token_desc_source()
if ((NOT EXISTS ${POLAR_TOKEN_DESC_MAP_FILE}) OR
      (NOT (POLAR_GRAMMER_FILE_MD5 AND POLAR_GRAMMER_FILE_MD5 STREQUAL grammerFileHash)))
   polar_generate_token_desc_source()
   set(POLAR_GRAMMER_FILE_MD5 ${grammerFileHash} CACHE STRING "language grammer file md5 value" FORCE)
   mark_as_advanced(POLAR_GRAMMER_FILE_MD5)
endif()

list(APPEND POLAR_COMPILER_SOURCES ${POLAR_TOKEN_DESC_MAP_FILE})
