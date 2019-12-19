//===--- LoopCanonicalizer.cpp --------------------------------------------===//
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
///
/// \file
/// This is a simple pass that can be used to apply loop canonicalizations to a
/// cfg. It also enables loop canonicalizations to be tested via FileCheck.
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pil-loop-canonicalizer"

#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "polarphp/pil/optimizer/analysis/DominanceAnalysis.h"
#include "polarphp/pil/optimizer/analysis/LoopAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/LoopUtils.h"

using namespace polar;

namespace {

class LoopCanonicalizer : public PILFunctionTransform {

   void run() override {
      PILFunction *F = getFunction();

      LLVM_DEBUG(llvm::dbgs() << "Attempt to canonicalize loops in "
                              << F->getName() << "\n");

      auto *LA = PM->getAnalysis<PILLoopAnalysis>();
      auto *LI = LA->get(F);

      if (LI->empty()) {
         LLVM_DEBUG(llvm::dbgs() << "    No loops to canonicalize!\n");
         return;
      }

      auto *DA = PM->getAnalysis<DominanceAnalysis>();
      auto *DI = DA->get(F);
      if (canonicalizeAllLoops(DI, LI)) {
         // We preserve loop info and the dominator tree.
         DA->lockInvalidation();
         LA->lockInvalidation();
         PM->invalidateAnalysis(F, PILAnalysis::InvalidationKind::FunctionBody);
         DA->unlockInvalidation();
         LA->unlockInvalidation();
      }
   }

};

} // end anonymous namespace

//===----------------------------------------------------------------------===//
//                            Top Level Entrypoint
//===----------------------------------------------------------------------===//

PILTransform *polar::createLoopCanonicalizer() {
   return new LoopCanonicalizer();
}
