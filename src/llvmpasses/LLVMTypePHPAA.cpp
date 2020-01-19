//===--- LLVMSwiftAA.cpp - LLVM Alias Analysis for Swift ------------------===//
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

#include "polarphp/llvmpasses/Passes.h"
#include "polarphp/llvmpasses/internal/LLVMARCOpts.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"

using namespace llvm;
using namespace polar;

//===----------------------------------------------------------------------===//
//                           Alias Analysis Result
//===----------------------------------------------------------------------===//

static ModRefInfo getConservativeModRefForKind(const llvm::Instruction &I) {
   switch (classifyInstruction(I)) {
#define KIND(Name, MemBehavior) case RT_ ## Name: return ModRefInfo:: MemBehavior;
#include "polarphp/llvmpasses/internal/LLVMTypephpDef.h"
   }

   llvm_unreachable("Not a valid Instruction.");
}

ModRefInfo TypePHPAAResult::getModRefInfo(const llvm::CallBase *Call,
                                        const llvm::MemoryLocation &Loc,
                                        llvm::AAQueryInfo &AAQI) {
   // We know at compile time that certain entry points do not modify any
   // compiler-visible state ever. Quickly check if we have one of those
   // instructions and return if so.
   if (ModRefInfo::NoModRef == getConservativeModRefForKind(*Call))
      return ModRefInfo::NoModRef;

   // Otherwise, delegate to the rest of the AA ModRefInfo machinery.
   return AAResultBase::getModRefInfo(Call, Loc, AAQI);
}

//===----------------------------------------------------------------------===//
//                        Alias Analysis Wrapper Pass
//===----------------------------------------------------------------------===//

char TypePHPAAWrapperPass::ID = 0;
INITIALIZE_PASS_BEGIN(TypePHPAAWrapperPass, "typephp-aa",
                      "polarphp Alias Analysis", false, true)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
INITIALIZE_PASS_END(TypePHPAAWrapperPass, "typephp-aa",
                    "polarphp Alias Analysis", false, true)

TypePHPAAWrapperPass::TypePHPAAWrapperPass() : ImmutablePass(ID) {
   initializeTypePHPAAWrapperPassPass(*PassRegistry::getPassRegistry());
}

bool TypePHPAAWrapperPass::doInitialization(Module &M) {
   Result.reset(new TypePHPAAResult());
   return false;
}

bool TypePHPAAWrapperPass::doFinalization(Module &M) {
   Result.reset();
   return false;
}

void TypePHPAAWrapperPass::getAnalysisUsage(AnalysisUsage &AU) const {
   AU.setPreservesAll();
   AU.addRequired<TargetLibraryInfoWrapperPass>();
}

//===----------------------------------------------------------------------===//
//                           Top Level Entry Point
//===----------------------------------------------------------------------===//

llvm::ImmutablePass *polar::createTypePHPAAWrapperPass() {
   return new TypePHPAAWrapperPass();
}
