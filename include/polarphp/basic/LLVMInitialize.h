//===--- LLVMInitialize.h ---------------------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/27.
//
//===----------------------------------------------------------------------===//
//
// A file that declares macros for initializing all parts of LLVM that various
// binaries in swift use. Please call PROGRAM_START in the main routine of all
// binaries, and INITIALIZE_LLVM in anything that uses Clang or LLVM IR.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_LLVMINITIALIZE_H
#define POLARPHP_BASIC_LLVMINITIALIZE_H

#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/TargetSelect.h"

#define PROGRAM_START(argc, argv) \
   llvm::InitLLVM _INITIALIZE_LLVM(argc, argv)

#define INITIALIZE_LLVM() \
   do { \
      llvm::InitializeAllTargets(); \
      llvm::InitializeAllTargetMCs(); \
      llvm::InitializeAllAsmPrinters(); \
      llvm::InitializeAllAsmParsers(); \
      llvm::InitializeAllDisassemblers(); \
      llvm::InitializeAllTargetInfos(); \
   } while (0)

#endif // POLARPHP_BASIC_LLVMINITIALIZE_H
