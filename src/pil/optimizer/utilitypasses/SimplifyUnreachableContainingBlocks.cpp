//===--- SimplifyUnreachableContainingBlocks.cpp --------------------------===//
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
///
/// This file contains a simple utility pass that simplifies blocks that contain
/// unreachables by eliminating all other instructions. This includes
/// instructions with side-effects and no-return functions. It is only intended
/// to be used to simplify IR for testing or exploratory purposes.
///
//===----------------------------------------------------------------------===//

#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/lang/PILBasicBlock.h"

using namespace polar;

namespace {

class SimplifyUnreachableContainingBlocks : public PILFunctionTransform {
   void run() override {
      // For each block...
      for (auto &BB : *getFunction()) {
         // If the block does not contain an unreachable, just continue. There is
         // no further work to do.
         auto *UI = dyn_cast<UnreachableInst>(BB.getTerminator());
         if (!UI)
            continue;

         // Otherwise, eliminate all other instructions in the block.
         for (auto II = BB.begin(); &*II != UI;) {
            // Avoid iterator invalidation.
            auto *I = &*II;
            ++II;

            I->replaceAllUsesOfAllResultsWithUndef();
            I->eraseFromParent();
         }
      }
   }
};

} // end anonymous namespace

//===----------------------------------------------------------------------===//
//                           Top Level Entry Point
//===----------------------------------------------------------------------===//

PILTransform *polar::createSimplifyUnreachableContainingBlocks() {
   return new SimplifyUnreachableContainingBlocks();
}
