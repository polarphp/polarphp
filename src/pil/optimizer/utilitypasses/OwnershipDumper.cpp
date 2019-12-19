//===--- OwnershipDumper.cpp ----------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
///
/// \file
///
/// This is a simple utility pass that dumps the ValueOwnershipKind of all
/// PILValue in a module. It is meant to trigger assertions and verification of
/// these values.
///
//===----------------------------------------------------------------------===//

#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"

using namespace polar;

//===----------------------------------------------------------------------===//
//                               Implementation
//===----------------------------------------------------------------------===//

static void dumpInstruction(PILInstruction &ii) {
   llvm::outs() << "Visiting: " << ii;

   auto ops = ii.getAllOperands();
   if (!ops.empty()) {
      llvm::outs() << "Operand Ownership Map:\n";
      for (const auto &op : ops) {
         llvm::outs() << "Op #: " << op.getOperandNumber() << "\n"
                      << "Ownership Map: " << op.getOwnershipKindMap();
      }
   }

   // If the instruction doesn't have any results, bail.
   auto results = ii.getResults();
   if (!results.empty()) {
      llvm::outs() << "Results Ownership Kinds:\n";
      for (auto v : results) {
         auto kind = v.getOwnershipKind();
         llvm::outs() << "Result: " << v;
         llvm::outs() << "Kind: " << kind << "\n";
      }
   }
}

//===----------------------------------------------------------------------===//
//                            Top Level Entrypoint
//===----------------------------------------------------------------------===//

namespace {

class OwnershipDumper : public PILFunctionTransform {
   void run() override {
      PILFunction *f = getFunction();
      llvm::outs() << "*** Dumping Function: '" << f->getName() << "'\n";
      for (auto &bb : *f) {
         // We only dump instructions right now.
         for (auto &ii : bb) {
            dumpInstruction(ii);
         }
      }
   }
};

} // end anonymous namespace

PILTransform *polar::createOwnershipDumper() { return new OwnershipDumper(); }
