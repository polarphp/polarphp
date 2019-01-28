# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2018 polarphp software foundation
# Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/09/18.

polar_add_lit_extra_test_executable(LitBuiltinCat Cat.cpp
   LINK_LIBS
   "PolarUtils")

get_filename_component(LIT_CONFIG_DIR ${CMAKE_CURRENT_BINARY_DIR} DIRECTORY)

get_target_property(props LitBuiltinCat INCLUDE_DIRECTORIES)
list(APPEND props ${LIT_CONFIG_DIR})
list(APPEND props ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND props ${LIT_CONFIG_DIR})
list(APPEND props ${CURSES_INCLUDE_DIRS})
set_target_properties(
   LitBuiltinCat
   PROPERTIES
   INCLUDE_DIRECTORIES "${props}"
   INSTALL_RPATH ${POLAR_LIBRARY_OUTPUT_INTDIR}
   )
