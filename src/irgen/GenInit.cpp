//===--- GenInit.cpp - IR Generation for Initialization -------------------===//
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
//  This file implements IR generation for the initialization of
//  local and global variables.
//
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/Pattern.h"
#include "polarphp/pil/lang/PILDeclRef.h"
#include "polarphp/pil/lang/PILGlobalVariable.h"
#include "polarphp/irgen/Linking.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"

#include "polarphp/irgen/internal/DebugTypeInfo.h"
#include "polarphp/irgen/internal/GenTuple.h"
#include "polarphp/irgen/internal/IRGenDebugInfo.h"
#include "polarphp/irgen/internal/IRGenFunction.h"
#include "polarphp/irgen/internal/IRGenModule.h"
#include "polarphp/irgen/internal/FixedTypeInfo.h"
#include "polarphp/irgen/internal/Temporary.h"

using namespace polar;
using namespace irgen;

/// Emit a global variable.
void IRGenModule::emitPILGlobalVariable(PILGlobalVariable *var) {
   auto &ti = getTypeInfo(var->getLoweredType());
   auto expansion = getResilienceExpansionForLayout(var);

   // If the variable is empty in all resilience domains that can access this
   // variable directly, don't actually emit it; just return undef.
   if (ti.isKnownEmpty(expansion)) {
      if (DebugInfo && var->getDecl()) {
         auto DbgTy = DebugTypeInfo::getGlobal(var, Int8Ty, Size(0), Alignment(1));
         DebugInfo->emitGlobalVariableDeclaration(
            nullptr, var->getDecl()->getName().str(), "", DbgTy,
            var->getLinkage() != PILLinkage::Public,
            IRGenDebugInfo::NotHeapAllocated, PILLocation(var->getDecl()));
      }
      return;
   }

   /// Create the global variable.
   getAddrOfPILGlobalVariable(var, ti,
                              var->isDefinition() ? ForDefinition : NotForDefinition);
}

StackAddress FixedTypeInfo::allocateStack(IRGenFunction &IGF, PILType T,
                                          const Twine &name) const {
   // If the type is known to be empty, don't actually allocate anything.
   if (isKnownEmpty(ResilienceExpansion::Maximal)) {
      auto addr = getUndefAddress();
      return { addr };
   }

   Address alloca =
      IGF.createAlloca(getStorageType(), getFixedAlignment(), name);
   IGF.Builder.CreateLifetimeStart(alloca, getFixedSize());

   return { alloca };
}

void FixedTypeInfo::destroyStack(IRGenFunction &IGF, StackAddress addr,
                                 PILType T, bool isOutlined) const {
   destroy(IGF, addr.getAddress(), T, isOutlined);
   FixedTypeInfo::deallocateStack(IGF, addr, T);
}

void FixedTypeInfo::deallocateStack(IRGenFunction &IGF, StackAddress addr,
                                    PILType T) const {
   if (isKnownEmpty(ResilienceExpansion::Maximal))
      return;
   IGF.Builder.CreateLifetimeEnd(addr.getAddress(), getFixedSize());
}

void TemporarySet::destroyAll(IRGenFunction &IGF) const {
   assert(!hasBeenCleared() && "destroying a set that's been cleared?");

   // Deallocate all the temporaries.
   for (auto &temporary : llvm::reverse(Stack)) {
      temporary.destroy(IGF);
   }
}

void Temporary::destroy(IRGenFunction &IGF) const {
   auto &ti = IGF.getTypeInfo(Type);
   ti.deallocateStack(IGF, Addr, Type);
}
