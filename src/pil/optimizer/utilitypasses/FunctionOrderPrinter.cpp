//===--- FunctionOrderPrinter.cpp - Function ordering test pass -----------===//
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
// This pass prints a bottom-up ordering of functions in the module (in the
// sense that each function is printed before functions that call it).
//
//===----------------------------------------------------------------------===//

#include "polarphp/pil/optimizer/analysis/FunctionOrder.h"
#include "polarphp/demangling/Demangle.h"
#include "polarphp/pil/optimizer/analysis/BasicCalleeAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "llvm/Support/raw_ostream.h"

using namespace polar;

#define DEBUG_TYPE "function-order-printer"

namespace {

class FunctionOrderPrinterPass : public PILModuleTransform {
   BasicCalleeAnalysis *BCA;

   /// The entry point to the transformation.
   void run() override {
      BCA = getAnalysis<BasicCalleeAnalysis>();
      auto &M = *getModule();
      BottomUpFunctionOrder Orderer(M, BCA);

      llvm::outs() << "Bottom up function order:\n";
      auto SCCs = Orderer.getSCCs();
      for (auto &SCC : SCCs) {
         std::string Indent;

         if (SCC.size() != 1) {
            llvm::outs() << "Non-trivial SCC:\n";
            Indent = std::string(2, ' ');
         }

         for (auto *F : SCC) {
            llvm::outs() << Indent
                         << demangling::demangleSymbolAsString(F->getName())
                         << "\n";
         }
      }
      llvm::outs() << "\n";
   }

};

} // end anonymous namespace

PILTransform *polar::createFunctionOrderPrinter() {
   return new FunctionOrderPrinterPass();
}
