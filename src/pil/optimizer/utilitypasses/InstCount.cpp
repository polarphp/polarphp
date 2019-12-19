//===--- InstCount.cpp - Collects the count of all instructions -----------===//
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
// This pass collects the count of all instructions and reports them
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pil-instcount"

#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/pil/optimizer/passmgr/PassManager.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/lang/PILVisitor.h"
#include "llvm/ADT/Statistic.h"

using namespace polar;

//===----------------------------------------------------------------------===//
//                                 Statistics
//===----------------------------------------------------------------------===//

// Local aggregate statistics
STATISTIC(TotalInsts, "Number of instructions (of all types) in non-external "
                      "functions");
STATISTIC(TotalBlocks, "Number of basic blocks in non-external functions");
STATISTIC(TotalFuncs , "Number of non-external functions");

// External aggregate statistics
STATISTIC(TotalExternalFuncInsts, "Number of instructions (of all types) in "
                                  "external functions");
STATISTIC(TotalExternalFuncBlocks, "Number of basic blocks in external "
                                   "functions");
STATISTIC(TotalExternalFuncDefs, "Number of external funcs definitions");
STATISTIC(TotalExternalFuncDecls, "Number of external funcs declarations");

// Linkage statistics
STATISTIC(TotalPublicFuncs, "Number of public funcs");
STATISTIC(TotalPublicNonABIFuncs, "Number of public non-ABI funcs");
STATISTIC(TotalHiddenFuncs, "Number of hidden funcs");
STATISTIC(TotalPrivateFuncs, "Number of private funcs");
STATISTIC(TotalSharedFuncs, "Number of shared funcs");
STATISTIC(TotalPublicExternalFuncs, "Number of public external funcs");
STATISTIC(TotalHiddenExternalFuncs, "Number of hidden external funcs");
STATISTIC(TotalPrivateExternalFuncs, "Number of private external funcs");
STATISTIC(TotalSharedExternalFuncs, "Number of shared external funcs");

// Individual instruction statistics
#define INST(Id, Parent) \
  STATISTIC(Num##Id, "Number of " #Id);
#include "polarphp/pil/lang/PILNodesDef.h"

namespace {

struct InstCountVisitor : PILInstructionVisitor<InstCountVisitor> {
   // We store these locally so that we do not continually check if the function
   // is external or not. Instead, we just check once at the end and accumulate.
   unsigned InstCount = 0;
   unsigned BlockCount = 0;

   void visitPILBasicBlock(PILBasicBlock *BB) {
      BlockCount++;
      PILInstructionVisitor<InstCountVisitor>::visitPILBasicBlock(BB);
   }

   void visitPILFunction(PILFunction *F) {
      PILInstructionVisitor<InstCountVisitor>::visitPILFunction(F);
   }

   void visitValueBase(ValueBase *V) { }

#define INST(Id, Parent)                                                       \
  void visit##Id(Id *I) {                                                      \
    ++Num##Id;                                                                 \
    ++InstCount;                                                               \
  }
#include "polarphp/pil/lang/PILNodesDef.h"

};

} // end anonymous namespace

//===----------------------------------------------------------------------===//
//                              Top Level Driver
//===----------------------------------------------------------------------===//

namespace {
class InstCount : public PILFunctionTransform {

   /// The entry point to the transformation.
   void run() override {
      PILFunction *F = getFunction();
      InstCountVisitor V;
      V.visitPILFunction(F);
      if (F->isAvailableExternally()) {
         if (F->isDefinition()) {
            TotalExternalFuncInsts += V.InstCount;
            TotalExternalFuncBlocks += V.BlockCount;
            TotalExternalFuncDefs++;
         } else {
            TotalExternalFuncDecls++;
         }
      } else {
         TotalInsts += V.InstCount;
         TotalBlocks += V.BlockCount;
         TotalFuncs++;
      }

      switch (F->getLinkage()) {
         case PILLinkage::Public:
            ++TotalPublicFuncs;
            break;
         case PILLinkage::PublicNonABI:
            ++TotalPublicNonABIFuncs;
            break;
         case PILLinkage::Hidden:
            ++TotalHiddenFuncs;
            break;
         case PILLinkage::Shared:
            ++TotalSharedFuncs;
            break;
         case PILLinkage::Private:
            ++TotalPrivateFuncs;
            break;
         case PILLinkage::PublicExternal:
            ++TotalPublicExternalFuncs;
            break;
         case PILLinkage::HiddenExternal:
            ++TotalHiddenExternalFuncs;
            break;
         case PILLinkage::SharedExternal:
            ++TotalSharedExternalFuncs;
            break;
         case PILLinkage::PrivateExternal:
            ++TotalPrivateExternalFuncs;
            break;
      }
   }
};

} // end anonymous namespace

PILTransform *polar::createInstCount() {
   return new InstCount();
}

void polar::performPILInstCountIfNeeded(PILModule *M) {
   if (!M->getOptions().PrintInstCounts)
      return;
   PILPassManager PrinterPM(M);
   PrinterPM.executePassPipelinePlan(
      PILPassPipelinePlan::getInstCountPassPipeline(M->getOptions()));
}
