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

find_package(PkgConfig)

pkg_check_modules(PC_UUID QUIET uuid)
set(UUID_DEFINITIONS ${PC_UUID_CFLAGS_OTHER})

find_path(UUID_INCLUDE_DIR uuid/uuid.h
    HINTS ${PC_UUID_INCLUDEDIR} ${PC_UUID_INCLUDE_DIRS})
set(UUID_INCLUDE_DIRS ${UUID_INCLUDE_DIR})

# On OS X we don't need the library
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(UUID_REQUIRED UUID_INCLUDE_DIR)
else()
  find_library(UUID_LIBRARY NAMES uuid
      HINTS ${PC_UUID_LIBDIR} ${PC_UUID_LIBRARY_DIRS})
  set(UUID_LIBRARIES ${UUID_LIBRARY})

  set(UUID_REQUIRED UUID_INCLUDE_DIR UUID_LIBRARY)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UUID DEFAULT_MSG ${UUID_REQUIRED})

mark_as_advanced(${UUID_REQUIRED})
