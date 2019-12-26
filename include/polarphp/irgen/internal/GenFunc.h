//===--- GenFunc.h - Swift IR generation for functions ----------*- C++ -*-===//
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
//  This file provides the private interface to the function and
//  function-type emission code.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IRGEN_INTERNAL_GENFUNC_H
#define POLARPHP_IRGEN_INTERNAL_GENFUNC_H

#include "polarphp/ast/Types.h"

namespace llvm {
class Function;
class Value;
}

namespace polar::irgen {

class Address;
class Explosion;
class ForeignFunctionInfo;
class IRGenFunction;

/// Project the capture address from on-stack block storage.
Address projectBlockStorageCapture(IRGenFunction &IGF,
                                   Address storageAddr,
                                   CanPILBlockStorageType storageTy);

/// Emit the block header into a block storage slot.
void emitBlockHeader(IRGenFunction &IGF,
                     Address storage,
                     CanPILBlockStorageType blockTy,
                     llvm::Constant *invokeFunction,
                     CanPILFunctionType invokeTy,
                     ForeignFunctionInfo foreignInfo);

/// Emit a partial application thunk for a function pointer applied to a
/// partial set of argument values.
Optional<StackAddress> emitFunctionPartialApplication(
   IRGenFunction &IGF, PILFunction &PILFn, const FunctionPointer &fnPtr,
   llvm::Value *fnContext, Explosion &args,
   ArrayRef<PILParameterInfo> argTypes, SubstitutionMap subs,
   CanPILFunctionType origType, CanPILFunctionType substType,
   CanPILFunctionType outType, Explosion &out, bool isOutlined);

} // end namespace polar::irgen

#endif // POLARPHP_IRGEN_INTERNAL_GENFUNC_H
