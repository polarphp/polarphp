# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

add_library(DynamicLibraryLib STATIC
   ExportedFuncs.cpp
   PipSqueak.cpp
   PipSqueak.h
   )
set_target_properties(DynamicLibraryLib PROPERTIES FOLDER "unittests")

# extract_symbols.py relies on all its library arguments being in the same
# directory, so we must set the output directory in the same way as if
# polar_add_library was used.
polar_set_output_directory(DynamicLibraryLib
   LIBRARY_DIR ${POLAR_LIBRARY_OUTPUT_INTDIR}
   )

polar_add_unittest(PolarBaseLibTests DynamicLibraryTests
   ../DynamicLibraryTest.cpp
   )

target_link_libraries(DynamicLibraryTests PRIVATE DynamicLibraryLib)
#export_executable_symbols(DynamicLibraryTests)

function(dynlib_add_module NAME)
   add_library(${NAME} SHARED
      PipSqueak.cpp
      )
   set_target_properties(${NAME} PROPERTIES FOLDER "Tests")

   polar_set_output_directory(${NAME}
      BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
      LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
      )

   set_target_properties(${NAME}
      PROPERTIES PREFIX ""
      SUFFIX ${LTDL_SHLIB_EXT}
      )

   add_dependencies(DynamicLibraryTests ${NAME})
endfunction(dynlib_add_module)

# Revert -Wl,-z,nodelete on this test since it relies on the file
# being unloaded.
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   string(REPLACE "-Wl,-z,nodelete" "" CMAKE_SHARED_LINKER_FLAGS
      ${CMAKE_SHARED_LINKER_FLAGS})
endif()

dynlib_add_module(PipSqueak)
dynlib_add_module(SecondLib)
