# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/10/12.

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLAR_UTILS_SOURCES)

polar_merge_list(POLAR_UTILS_SOURCES POLAR_HEADERS)

set(system_libs)
if (POLAR_ENABLE_ZLIB AND HAVE_LIBZ)
   set(system_libs ${system_libs} ${ZLIB_LIBRARIES})
endif()
if( MSVC OR MINGW )
   # libuuid required for FOLDERID_Profile usage in lib/Support/Windows/Path.inc.
   # advapi32 required for CryptAcquireContextW in lib/Support/Windows/Path.inc.
   set(system_libs ${system_libs} psapi shell32 ole32 uuid advapi32)
elseif( CMAKE_HOST_UNIX )
   if(HAVE_LIBRT)
      set(system_libs ${system_libs} rt)
   endif()
   if(HAVE_LIBDL)
      set(system_libs ${system_libs} ${CMAKE_DL_LIBS})
   endif()
   if(HAVE_BACKTRACE AND NOT "${Backtrace_LIBRARIES}" STREQUAL "")
      # On BSDs, CMake returns a fully qualified path to the backtrace library.
      # We need to remove the path and the 'lib' prefix, to make it look like a
      # regular short library name, suitable for appending to a -l link flag.
      get_filename_component(Backtrace_LIBFILE ${Backtrace_LIBRARIES} NAME_WE)
      STRING(REGEX REPLACE "^lib" "" Backtrace_LIBFILE ${Backtrace_LIBFILE})
      set(system_libs ${system_libs} ${Backtrace_LIBFILE})
   endif()
   if(POLAR_ENABLE_TERMINFO)
      if(HAVE_TERMINFO)
         set(system_libs ${system_libs} ${TERMINFO_LIBS})
      endif()
   endif()
   if(UNIX AND NOT (BEOS OR HAIKU))
      set(system_libs ${system_libs} m)
   endif()
   if(FUCHSIA)
      set(system_libs ${system_libs} zircon)
   endif()
endif(MSVC OR MINGW)

set(system_libs ${system_libs} PolarBasic)
set(system_libs ${system_libs} nlohmann_json::nlohmann_json)

if (LINUX)
   set(system_libs ${system_libs} pthread uuid)
endif()

polar_add_library(PolarUtils SHARED
   ${POLAR_UTILS_SOURCES}
   LINK_LIBS ${system_libs})

set_target_properties(
   PolarUtils
   PROPERTIES
   INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR};"
   INTERFACE_LINK_LIBRARIES "PolarBasic;PolarGlobal;"
   )
