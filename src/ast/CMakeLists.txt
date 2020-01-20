# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/12/12.

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLAR_AST_SOURCES)

polar_merge_list(POLAR_AST_SOURCES POLAR_HEADERS)

set(POLAR_LLVM_LINK_COMPONENTS
   bitreader bitwriter coroutines coverage irreader debuginfoDWARF
   profiledata instrumentation object mc mcparser
   bitreader bitwriter lto ipo option core support
   )

if (POLAR_FORCE_OPTIMIZED_TYPECHECKER)
   set(EXTRA_AST_FLAGS "FORCE_BUILD_OPTIMIZED")
endif()


set(supportTargets
   AArch64
   AMDGPU
   ARM
   BPF
   Hexagon
   Lanai
   Mips
   MSP430
   NVPTX
   PowerPC
   RISCV
   Sparc
   SystemZ
   WebAssembly
   X86
   XCore)

set(targetLLVMComponents "")

foreach(_targetName ${supportTargets})
   string(TOLOWER ${_targetName} _targetName)
   set(targetSuffix asmparser codegen disassembler)
   foreach(suffix ${targetSuffix})
      list(APPEND targetLLVMComponents "${_targetName}${suffix}")
   endforeach()
endforeach()

list(REMOVE_ITEM targetLLVMComponents
   "xcoreasmparser"
   "nvptxasmparser"
   "nvptxdisassembler")

polar_add_host_library(PolarAST STATIC
   ${POLAR_AST_SOURCES}
   LLVM_LINK_COMPONENTS
   ${POLAR_LLVM_LINK_COMPONENTS}
   ${EXTRA_AST_FLAGS}
   LLVM_LINK_COMPONENTS
   ${targetLLVMComponents}
   )

target_link_libraries(PolarAST INTERFACE
   clangCodeGen
   clangIndex
   clangFormat
   clangToolingCore
   clangFrontendTool
   clangFrontend
   clangDriver
   clangSerialization
   clangParse
   clangSema
   clangAnalysis
   clangEdit
   clangRewriteFrontend
   clangRewrite
   clangAST
   clangLex
   clangBasic
)

set(polar_ast_verifier_flag)
if(POLAR_AST_VERIFIER)
   set(polar_ast_verifier_flag " -UPOLAR_DISABLE_AST_VERIFIER")
else()
   set(polar_ast_verifier_flag " -DPOLAR_DISABLE_AST_VERIFIER=1")
endif()

set_property(SOURCE AstVerifier.cpp APPEND_STRING PROPERTY COMPILE_FLAGS
   "${polar_ast_verifier_flag}")

