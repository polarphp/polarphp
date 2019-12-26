//===--- GenDecl.h - Swift IR generation for some decl ----------*- C++ -*-===//
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
//  This file provides the private interface to some decl emission code.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IRGEN_INTERNAL_GENDECL_H
#define POLARPHP_IRGEN_INTERNAL_GENDECL_H

#include "polarphp/basic/OptimizationMode.h"
#include "polarphp/pil/lang/PILLocation.h"
#include "polarphp/irgen/internal/DebugTypeInfo.h"
#include "polarphp/irgen/internal/IRGen.h"

#include "llvm/IR/CallingConv.h"
#include "llvm/Support/CommandLine.h"

namespace llvm {
class AttributeList;
class Function;
class FunctionType;
}

namespace polar::irgen {

class IRGenModule;
class LinkEntity;
class LinkInfo;
class Signature;

void updateLinkageForDefinition(IRGenModule &IGM,
                                llvm::GlobalValue *global,
                                const LinkEntity &entity);

llvm::Function *createFunction(IRGenModule &IGM,
                               LinkInfo &linkInfo,
                               const Signature &signature,
                               llvm::Function *insertBefore = nullptr,
                               OptimizationMode FuncOptMode =
                               OptimizationMode::NotSet);

llvm::GlobalVariable *
createVariable(IRGenModule &IGM, LinkInfo &linkInfo, llvm::Type *objectType,
               Alignment alignment, DebugTypeInfo DebugType = DebugTypeInfo(),
               Optional<PILLocation> DebugLoc = None,
               StringRef DebugName = StringRef(), bool heapAllocated = false);

void disableAddressSanitizer(IRGenModule &IGM, llvm::GlobalVariable *var);

} // polar::irgen

extern llvm::cl::opt<bool> UseBasicDynamicReplacement;

#endif // POLARPHP_IRGEN_INTERNAL_GENDECL_H
