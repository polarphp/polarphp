# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

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
