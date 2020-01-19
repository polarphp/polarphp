//===--- LLVMARCOpts.h - LLVM level ARC Opts Util. Declarations -*- C++ -*-===//
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
#ifndef POLARPHP_LLVMPASSES_LLVMARCOPTS_H
#define POLARPHP_LLVMPASSES_LLVMARCOPTS_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/runtime/Config.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"

namespace polar {

enum RT_Kind {
#define KIND(Name, MemBehavior) RT_ ## Name,
#include "polarphp/llvmpasses/internal/LLVMTypephpDef.h"
};

/// Take a look at the specified instruction and classify it into what kind of
/// runtime entrypoint it is, if any.
inline RT_Kind classifyInstruction(const llvm::Instruction &I) {
   if (!I.mayReadOrWriteMemory())
      return RT_NoMemoryAccessed;

   // Non-calls or calls to indirect functions are unknown.
   auto *CI = dyn_cast<llvm::CallInst>(&I);
   if (CI == 0) return RT_Unknown;

   // First check if we have an objc intrinsic.
   auto intrinsic = CI->getIntrinsicID();
   switch (intrinsic) {
      // This is an intrinsic that we do not understand. It can not be one of our
      // "special" runtime functions as well... so return RT_Unknown early.
      default:
         return RT_Unknown;
      case llvm::Intrinsic::not_intrinsic:
         // If we do not have an intrinsic, break and move onto runtime functions
         // that we identify by name.
         break;
/// TODO
//#define OBJC_FUNC(Name, MemBehavior, TextualName)                              \
//  case llvm::Intrinsic::objc_##TextualName:                                    \
//    return RT_##Name;
#include "polarphp/llvmpasses/internal/LLVMTypephpDef.h"
   }

   llvm::Function *F = CI->getCalledFunction();
   if (F == nullptr)
      return RT_Unknown;

   return llvm::StringSwitch<RT_Kind>(F->getName())
#define POLAR_FUNC(Name, MemBehavior, TextualName) \
    .Case("typephp_" #TextualName, RT_ ## Name)
#define POLAR_INTERNAL_FUNC_NEVER_NONATOMIC(Name, MemBehavior, TextualName) \
    .Case("__typephp_" #TextualName, RT_ ## Name)
#include "polarphp/llvmpasses/internal/LLVMTypephpDef.h"

      // Support non-atomic versions of reference counting entry points.
#define POLAR_FUNC(Name, MemBehavior, TextualName) \
    .Case("typephp_nonatomic_" #TextualName, RT_ ## Name)
#define POLAR_INTERNAL_FUNC_NEVER_NONATOMIC(Name, MemBehavior, TextualName)
#include "polarphp/llvmpasses/internal/LLVMTypephpDef.h"
      .Default(RT_Unknown);
}

} // end namespace polar

#endif
