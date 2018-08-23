# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarPHP software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See http://polarphp.org/LICENSE.txt for license information
# See http://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
#
# Created by polarboy on 2018/08/22.

add_custom_target(PolarPHPUnitTests)
set_target_properties(PolarPHPUnitTests PROPERTIES FOLDER "Tests")

# Generic support for adding a unittest.
function(polar_add_unittest test_suite test_name)
   if(NOT POLAR_INCLUDE_TESTS)
      set(EXCLUDE_FROM_ALL ON)
   endif()
   # Our current version of gtest does not properly recognize C++11 support
   # with MSVC, so it falls back to tr1 / experimental classes.  Since PolarPHP
   # itself requires C++11, we can safely force it on unconditionally so that
   # we don't have to fight with the buggy gtest check.
   add_definitions(-DGTEST_LANG_CXX11=1)
   add_definitions(-DGTEST_HAS_TR1_TUPLE=0)
   if(POLAR_FOUND_NATIVE_GTEST)
       include_directories(${GTEST_INCLUDE_DIRS} ${GMOCK_INCLUDE_DIRS})
       set(POLAR_TEMP_GTEST_LIBS ${GTEST_BOTH_LIBRARIES} ${GMOCK_BOTH_LIBRARIES})
   else()
       include_directories(${POLAR_THIRDPARTY_DIR}/unittest/googletest/include)
       include_directories(${POLAR_THIRDPARTY_DIR}/unittest/googlemock/include)
       set(POLAR_TEMP_GTEST_LIBS gtest_main gtest gmock gmock_main)
   endif()
   if (NOT POLAR_ENABLE_THREADS)
      list(APPEND POLAR_COMPILE_DEFINITIONS GTEST_HAS_PTHREAD=0)
   endif ()

   if (SUPPORTS_VARIADIC_MACROS_FLAG)
      list(APPEND POLAR_COMPILE_FLAGS "-Wno-variadic-macros")
   endif ()
   # Some parts of gtest rely on this GNU extension, don't warn on it.
   if(SUPPORTS_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS_FLAG)
      list(APPEND POLAR_COMPILE_FLAGS "-Wno-gnu-zero-variadic-macro-arguments")
   endif()

   set(POLAR_REQUIRES_RTTI OFF)

   #list(APPEND POLAR_LINK_COMPONENTS Utils) # gtest needs it for RawOutStream
   polar_add_executable(${test_name} IGNORE_EXTERNALIZE_DEBUGINFO NO_INSTALL_RPATH ${ARGN})
   set(outdir ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
   polar_set_output_directory(${test_name} BINARY_DIR ${outdir} LIBRARY_DIR ${outdir})
   # libpthreads overrides some standard library symbols, so main
   # executable must be linked with it in order to provide consistent
   # API for all shared libaries loaded by this executable.
   target_link_libraries(${test_name} PRIVATE ${POLAR_TEMP_GTEST_LIBS} PolarUtils ${POLAR_PTHREAD_LIB})

   add_dependencies(${test_suite} ${test_name})
   get_target_property(test_suite_folder ${test_suite} FOLDER)
   if (NOT ${test_suite_folder} STREQUAL "NOTFOUND")
      set_property(TARGET ${test_name} PROPERTY FOLDER "${test_suite_folder}")
   endif ()
endfunction()
