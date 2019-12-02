# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/12/02.

include(TableGen)

# This needs to be a macro since tablegen (which is a function) needs to set
# variables in its parent scope.
macro(polar_tablegen)
  tablegen(LLVM ${ARGN})
endmacro()

# This needs to be a macro since add_public_tablegen_target (which is a
# function) needs to set variables in its parent scope.
macro(polar_add_public_tablegen_target target)
  add_public_tablegen_target(${target})
endmacro()
