//===--- CFGPrinter.cpp - CFG printer pass --------------------------------===//
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
// This file defines external functions that can be called to explicitly
// instantiate the CFG printer.
//
//===----------------------------------------------------------------------===//

#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "llvm/Support/CommandLine.h"

using namespace polar;

//===----------------------------------------------------------------------===//
//                                  Options
//===----------------------------------------------------------------------===//

llvm::cl::opt<std::string> PILViewCFGOnlyFun(
   "pil-view-cfg-only-function", llvm::cl::init(""),
   llvm::cl::desc("Only produce a graphviz file for this function"));

llvm::cl::opt<std::string> PILViewCFGOnlyFuns(
   "pil-view-cfg-only-functions", llvm::cl::init(""),
   llvm::cl::desc("Only produce a graphviz file for the sil for the functions "
                  "whose name contains this substring"));

//===----------------------------------------------------------------------===//
//                              Top Level Driver
//===----------------------------------------------------------------------===//

namespace {

class PILCFGPrinter : public PILFunctionTransform {
   /// The entry point to the transformation.
   void run() override {
      PILFunction *F = getFunction();

      // If we are not supposed to dump view this cfg, return.
      if (!PILViewCFGOnlyFun.empty() && F && F->getName() != PILViewCFGOnlyFun)
         return;
      if (!PILViewCFGOnlyFuns.empty() && F &&
          F->getName().find(PILViewCFGOnlyFuns, 0) == StringRef::npos)
         return;

      F->viewCFG();
   }
};

} // end anonymous namespace

PILTransform *polar::createCFGPrinter() {
   return new PILCFGPrinter();
}
