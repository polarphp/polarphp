# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2018/08/26.

set(POLAR_SOURCE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(ast)
#add_subdirectory(clangimporter)
add_subdirectory(global)
add_subdirectory(basic)
#add_subdirectory(pil)
#add_subdirectory(irgen)
#add_subdirectory(markup)
add_subdirectory(demangling)
add_subdirectory(utils)
#add_subdirectory(sema)
add_subdirectory(option)
add_subdirectory(driver)
add_subdirectory(frontend)
add_subdirectory(frontendtool)
#add_subdirectory(serialization)
add_subdirectory(llparser)
add_subdirectory(tbdgen)
#add_subdirectory(ide)
#add_subdirectory(index)
#add_subdirectory(immediate)
#add_subdirectory(llvmpasses)

#add_subdirectory(reflection)
#add_subdirectory(syntax)
#add_subdirectory(parser)
#add_subdirectory(vm)
#add_subdirectory(runtime)

