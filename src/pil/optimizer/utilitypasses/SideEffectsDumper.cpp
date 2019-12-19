//===--- SideEffectsDumper.cpp - Dumps the side effect analysis -----------===//
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

#define DEBUG_TYPE "dump-sea"

#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/analysis/SideEffectAnalysis.h"

using namespace polar;

namespace {

/// Dumps the side-effect information of all functions in the module.
/// Only dumps if the compiler is built with assertions.
/// For details see SideEffectAnalysis.
class SideEffectsDumper : public PILModuleTransform {

   void run() override {

      LLVM_DEBUG(llvm::dbgs() << "** SideEffectsDumper **\n");

#ifndef NDEBUG
      auto *SEA = PM->getAnalysis<SideEffectAnalysis>();

      llvm::outs() << "Side effects of module\n";
      for (auto &F : *getModule()) {
         llvm::outs() << "  sil @" << F.getName() << '\n';
         const auto &Effects = SEA->getEffects(&F);
         llvm::outs() << "    <" << Effects << ">\n";
      }
#endif
   }

};

} // end anonymous namespace

PILTransform *polar::createSideEffectsDumper() {
   return new SideEffectsDumper();
}
