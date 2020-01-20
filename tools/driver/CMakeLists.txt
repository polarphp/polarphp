# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2020/01/04.

polar_add_host_tool(polarphp
   Driver.cpp
   ModuleWrapMain.cpp
   PhpIndentMain.cpp
   AutolinkExtractMain.cpp
   POLAR_COMPONENT compiler)

target_link_libraries(polarphp
   PRIVATE
   PolarDriver
   PolarLLParser
   PolarDemangling
   PolarFrontendTool
   LLVMAArch64AsmParser
   LLVMAArch64CodeGen)

if(HAVE_UNICODE_LIBEDIT)
   target_link_libraries(polarphp PRIVATE edit)
endif()

polar_create_post_build_symlink(polarphp
   SOURCE "polarphp${CMAKE_EXECUTABLE_SUFFIX}"
   DESTINATION "polarphpc${CMAKE_EXECUTABLE_SUFFIX}"
   WORKING_DIRECTORY "${POLAR_RUNTIME_OUTPUT_INTDIR}")

polar_create_post_build_symlink(polarphp
   SOURCE "polarphp${CMAKE_EXECUTABLE_SUFFIX}"
   DESTINATION "polarphp-indent${CMAKE_EXECUTABLE_SUFFIX}"
   WORKING_DIRECTORY "${POLAR_RUNTIME_OUTPUT_INTDIR}")

polar_create_post_build_symlink(polarphp
   SOURCE "polarphp${CMAKE_EXECUTABLE_SUFFIX}"
   DESTINATION "polarphp-autolink-extract${CMAKE_EXECUTABLE_SUFFIX}"
   WORKING_DIRECTORY "${POLAR_RUNTIME_OUTPUT_INTDIR}")

polar_add_tool_symlink(polarphpc polarphp compiler)
polar_add_tool_symlink(polarphp-autolink-extract polarphp autolink-driver)
polar_add_tool_symlink(polarphp-indent polarphp editor-integration)

add_dependencies(compiler polarphp)
polar_install_in_component(FILES "${POLAR_RUNTIME_OUTPUT_INTDIR}/polarphpc${CMAKE_EXECUTABLE_SUFFIX}"
   DESTINATION "bin"
   COMPONENT compiler)
add_dependencies(autolink-driver polarphp)
polar_install_in_component(FILES "${POLAR_RUNTIME_OUTPUT_INTDIR}/polarphp-autolink-extract${CMAKE_EXECUTABLE_SUFFIX}"
   DESTINATION "bin"
   COMPONENT autolink-driver)
add_dependencies(editor-integration polarphp)
polar_install_in_component(FILES "${POLAR_RUNTIME_OUTPUT_INTDIR}/polarphp-indent${CMAKE_EXECUTABLE_SUFFIX}"
   DESTINATION "bin"
   COMPONENT editor-integration)
