# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/09/19.

# A raw function to create a lit target. This is used to implement the testuite
# management functions.
function(polar_add_lit_target target comment)
   cmake_parse_arguments(ARG "" "" "PARAMS;DEPENDS;ARGS" ${ARGN})
   set(LIT_ARGS "${ARG_ARGS} ${POLAR_LIT_ARGS}")
   if (NOT CMAKE_CFG_INTDIR STREQUAL ".")
      list(APPEND LIT_ARGS --param build_mode=${CMAKE_CFG_INTDIR})
   endif ()
   # Get the path to the lit to *run* tests with.  This can be overriden by
   # the user by specifying
   set(lit_executable ${POLAR_DEVTOOLS_DIR}/phplit/lit)
   set(LIT_COMMAND "${PHP_EXECUTABLE};${lit_executable};run-test")
   list(APPEND LIT_COMMAND ${LIT_ARGS})
   foreach(param ${ARG_PARAMS})
      list(APPEND LIT_COMMAND --param ${param})
   endforeach()
   set(sources "")
   if (ARG_UNPARSED_ARGUMENTS)
      foreach(entry ${ARG_UNPARSED_ARGUMENTS})
         if (IS_DIRECTORY ${entry})
            file(GLOB_RECURSE files ${entry}/*[.php|.txt|.cpp])
            list(APPEND sources ${files})
         else()
            list(APPEND sources ${files})
         endif()
      endforeach()
      add_custom_target(${target}
         COMMAND ${LIT_COMMAND} -- ${ARG_UNPARSED_ARGUMENTS}
         COMMENT "${comment}"
         SOURCES ${sources}
         USES_TERMINAL
         )
   else()
      add_custom_target(${target}
         COMMAND ${CMAKE_COMMAND} -E echo "${target} does nothing, no tools built.")
      message(STATUS "${target} does nothing.")
   endif()
   if (ARG_DEPENDS)
      add_dependencies(${target} ${ARG_DEPENDS})
   endif()
   # Tests should be excluded from "Build Solution".
   # set_target_properties(${target} PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD ON)
endfunction()

# A function to add a set of lit test suites to be driven through 'check-*' targets.
function(polar_add_lit_testsuite target comment)
   cmake_parse_arguments(ARG "" "" "PARAMS;DEPENDS;ARGS" ${ARGN})

   # EXCLUDE_FROM_ALL excludes the test ${target} out of check-all.
   if(NOT EXCLUDE_FROM_ALL)
      # Register the testsuites, params and depends for the global check rule.
      set_property(GLOBAL APPEND PROPERTY POLAR_LIT_TESTSUITES ${ARG_UNPARSED_ARGUMENTS})
      set_property(GLOBAL APPEND PROPERTY POLAR_LIT_PARAMS ${ARG_PARAMS})
      set_property(GLOBAL APPEND PROPERTY POLAR_LIT_DEPENDS ${ARG_DEPENDS})
      set_property(GLOBAL APPEND PROPERTY POLAR_LIT_EXTRA_ARGS ${ARG_ARGS})
   endif()
   # Produce a specific suffixed check rule.
   polar_add_lit_target(${target} ${comment}
      ${ARG_UNPARSED_ARGUMENTS}
      PARAMS ${ARG_PARAMS}
      DEPENDS ${ARG_DEPENDS}
      ARGS ${ARG_ARGS}
      )
endfunction()

function(polar_add_lit_testsuites project directory)
   cmake_parse_arguments(ARG "" "" "PARAMS;DEPENDS;ARGS" ${ARGN})

   # Search recursively for test directories by assuming anything not
   # in a directory called Inputs contains tests.
   file(GLOB_RECURSE to_process LIST_DIRECTORIES true ${directory}/*)

   foreach(lit_suite ${to_process})
      if(NOT IS_DIRECTORY ${lit_suite})
         continue()
      endif()
      string(FIND ${lit_suite} Inputs is_inputs)
      string(FIND ${lit_suite} Output is_output)
      if (NOT (is_inputs EQUAL -1 AND is_output EQUAL -1))
         continue()
      endif()
      # Create a check- target for the directory.
      string(REPLACE ${directory} "" name_slash ${lit_suite})
      if (name_slash)
         string(REPLACE "/" "-" name_slash ${name_slash})
         string(REPLACE "\\" "-" name_dashes ${name_slash})
         string(TOLOWER "${project}${name_dashes}" name_var)
         polar_add_lit_target("check-${name_var}" "Running lit suite ${lit_suite}"
            ${lit_suite}
            PARAMS ${ARG_PARAMS}
            DEPENDS ${ARG_DEPENDS}
            ARGS ${ARG_ARGS}
            )
      endif()
   endforeach()
endfunction()
