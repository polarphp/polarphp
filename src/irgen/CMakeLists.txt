# This source file is part of the polarphp.org open source project
#
# Copyright (c) 2017 - 2019 polarphp software foundation
# Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
# Licensed under Apache License v2.0 with Runtime Library Exception
#
# See https://polarphp.org/LICENSE.txt for license information
# See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
#
# Created by polarboy on 2019/11/18.

polar_collect_files(
   TYPE_BOTH
   DIR .
   OUTPUT_VAR POLAR_IRGEN_SOURCES)
polar_merge_list(POLAR_IRGEN_SOURCES POLAR_HEADERS)

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

polar_add_host_library(PolarIRGen STATIC
   ${POLAR_IRGEN_SOURCES}
   LLVM_LINK_COMPONENTS
   target
   ${targetLLVMComponents}
   )

target_link_libraries(PolarIRGen
   INTERFACE
   clangCodeGen
   clangAST)

target_link_libraries(PolarIRGen PRIVATE
   PolarAST
   PolarPILGen
   PolarPIL
   PolarPILOptimizer
   PolarLLVMPasses
   LLVMPasses)