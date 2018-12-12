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

# There is no clear way of keeping track of compiler command-line
# options chosen via `add_definitions'

# Beware that there is no implementation of remove_llvm_definitions.

macro(polar_add_definitions)
   # We don't want no semicolons on POLAR_DEFINITIONS:
   foreach(arg ${ARGN})
      if(DEFINED POLAR_COMPILE_DEFINITIONS)
         set(POLAR_COMPILE_DEFINITIONS "${POLAR_DEFINITIONS} ${arg}")
      else()
         set(POLAR_COMPILE_DEFINITIONS ${arg})
      endif()
   endforeach(arg)
   add_definitions(${ARGN})
endmacro(polar_add_definitions)
