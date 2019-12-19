//===--- LoopInfoPrinter.cpp - Print PIL Loop Info ------------------------===//
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

#include "polarphp/pil/optimizer/analysis/LoopAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/lang/PILVisitor.h"
#include "llvm/ADT/Statistic.h"

using namespace polar;

namespace {

class LoopInfoPrinter : public PILModuleTransform {

   /// The entry point to the transformation.
   void run() override {
      PILLoopAnalysis *LA = PM->getAnalysis<PILLoopAnalysis>();
      assert(LA && "Invalid LoopAnalysis");
      for (auto &Fn : *getModule()) {
         if (Fn.isExternalDeclaration()) continue;

         PILLoopInfo *LI = LA->get(&Fn);
         assert(LI && "Invalid loop info for function");


         if (LI->empty()) {
            llvm::errs() << "No loops in " << Fn.getName() << "\n";
            return;
         }

         llvm::errs() << "Loops in " << Fn.getName() << "\n";
         for (auto *LoopIt : *LI) {
            LoopIt->dump();
         }
      }
   }
};

} // end anonymous namespace

PILTransform *polar::createLoopInfoPrinter() {
   return new LoopInfoPrinter();
}
