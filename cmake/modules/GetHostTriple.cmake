# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/08/22.

# Returns the host triple.
# Invokes config.guess

function(polar_get_host_triple var)
   if(MSVC)
      if( CMAKE_SIZEOF_VOID_P EQUAL 8)
         set(value "x86_64-pc-win32")
      else()
         set(value "i686-pc-win32")
      endif()
   elseif(MINGW AND NOT MSYS)
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
         set(value "x86_64-w64-mingw32")
      else()
         set(value "i686-pc-mingw32")
      endif()
   else(MSVC)
      set(config_guess ${POLAR_SOURCE_DIR}/cmake/Config.guess)
      execute_process(COMMAND sh ${config_guess}
         RESULT_VARIABLE TT_RV
         OUTPUT_VARIABLE TT_OUT
         ERROR_VARIABLE ERROR_MGS
         OUTPUT_STRIP_TRAILING_WHITESPACE)
      if(NOT TT_RV EQUAL 0)
         message(FATAL_ERROR "Failed to execute ${config_guess}, error: ${ERROR_MGS}")
      endif(NOT TT_RV EQUAL 0)
      set(value ${TT_OUT})
   endif(MSVC)
   set( ${var} ${value} PARENT_SCOPE)
endfunction(polar_get_host_triple var)
