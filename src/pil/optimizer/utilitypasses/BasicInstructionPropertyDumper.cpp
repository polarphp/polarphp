//===--- BasicInstructionPropertyDumper.cpp -------------------------------===//
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

#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

using namespace polar;

namespace {

class BasicInstructionPropertyDumper : public PILModuleTransform {
   void run() override {
      for (auto &Fn : *getModule()) {
         unsigned Count = 0;
         llvm::outs() << "@" << Fn.getName() << "\n";
         for (auto &BB : Fn) {
            for (auto &I : BB) {
               llvm::outs() << "Inst #: " << Count++ << "\n    " << I;
               llvm::outs() << "    Mem Behavior: " << I.getMemoryBehavior() << "\n";
               llvm::outs() << "    Release Behavior: " << I.getReleasingBehavior()
                            << "\n";
            }
         }
      }
   }

};

} // end anonymous namespace

PILTransform *polar::createBasicInstructionPropertyDumper() {
   return new BasicInstructionPropertyDumper();
}
