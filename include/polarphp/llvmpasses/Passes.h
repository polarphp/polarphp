//===--- Passes.h - LLVM optimizer passes for Swift -------------*- C++ -*-===//
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

#ifndef POLARPHP_LLVMPASSES_PASSES_H
#define POLARPHP_LLVMPASSES_PASSES_H

#include "polarphp/llvmpasses/PassesFwd.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Pass.h"

namespace polar {

struct PolarphpAAResult : llvm::AAResultBase<PolarphpAAResult> {
   friend llvm::AAResultBase<PolarphpAAResult>;

   explicit PolarphpAAResult() : AAResultBase() {}
   PolarphpAAResult(PolarphpAAResult &&Arg)
      : AAResultBase(std::move(Arg)) {}

   bool invalidate(llvm::Function &,
                   const llvm::PreservedAnalyses &) { return false; }

   using AAResultBase::getModRefInfo;
   llvm::ModRefInfo getModRefInfo(const llvm::CallBase *Call,
                                  const llvm::MemoryLocation &Loc) {
      llvm::AAQueryInfo AAQI;
      return getModRefInfo(Call, Loc, AAQI);
   }
   llvm::ModRefInfo getModRefInfo(const llvm::CallBase *Call,
                                  const llvm::MemoryLocation &Loc,
                                  llvm::AAQueryInfo &AAQI);
};

class PolarphpAAWrapperPass : public llvm::ImmutablePass {
   std::unique_ptr<PolarphpAAResult> Result;

public:
   static char ID; // Class identification, replacement for typeinfo
   PolarphpAAWrapperPass();

   PolarphpAAResult &getResult() { return *Result; }
   const PolarphpAAResult &getResult() const { return *Result; }

   bool doInitialization(llvm::Module &M) override;
   bool doFinalization(llvm::Module &M) override;
   void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

class SwiftRCIdentity : public llvm::ImmutablePass {
public:
   static char ID; // Class identification, replacement for typeinfo
   SwiftRCIdentity() : ImmutablePass(ID) {}

   /// Returns the root of the RC-equivalent value for the given V.
   llvm::Value *getSwiftRCIdentityRoot(llvm::Value *V);
private:
   enum { MaxRecursionDepth = 16 };
   bool doInitialization(llvm::Module &M) override;

   void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
      AU.setPreservesAll();
   }
   llvm::Value *stripPointerCasts(llvm::Value *Val);
   llvm::Value *stripReferenceForwarding(llvm::Value *Val);
};

class SwiftARCOpt : public llvm::FunctionPass {
   /// Swift RC Identity analysis.
   SwiftRCIdentity *RC;
   virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
   virtual bool runOnFunction(llvm::Function &F) override;
public:
   static char ID;
   SwiftARCOpt();
};

class SwiftARCContract : public llvm::FunctionPass {
   /// Swift RC Identity analysis.
   SwiftRCIdentity *RC;
   virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
   virtual bool runOnFunction(llvm::Function &F) override;
public:
   static char ID;
   SwiftARCContract() : llvm::FunctionPass(ID) {}
};

class InlineTreePrinter : public llvm::ModulePass {
   virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
   virtual bool runOnModule(llvm::Module &M) override;
public:
   static char ID;
   InlineTreePrinter() : llvm::ModulePass(ID) {}
};

} // end namespace polar

#endif // POLARPHP_LLVMPASSES_PASSES_H
