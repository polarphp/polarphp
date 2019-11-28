//===--- LLVMContext.h ------------------------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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

#ifndef POLARPHP_BASIC_LLVMCONTEXT_H
#define POLARPHP_BASIC_LLVMCONTEXT_H

namespace llvm {
class LLVMContext;
} // end llvm namespace

namespace polar {
/// Returns a global LLVM context for Swift clients that only care about
/// operating on a single thread.
llvm::LLVMContext &get_global_llvm_context();
} // polar

#endif // POLARPHP_BASIC_LLVMCONTEXT_H
