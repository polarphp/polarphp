//===--- StripDebugInfo.cpp - Strip debug info from PIL -------------------===//
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

#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILFunction.h"

using namespace polar;

static void stripFunction(PILFunction *F) {
   for (auto &BB : *F)
      for (auto II = BB.begin(), IE = BB.end(); II != IE;) {
         PILInstruction *Inst = &*II;
         ++II;

         if (!isa<DebugValueInst>(Inst) &&
             !isa<DebugValueAddrInst>(Inst))
            continue;

         Inst->eraseFromParent();
      }
}

namespace {
class StripDebugInfo : public polar::PILFunctionTransform {
   ~StripDebugInfo() override {}

   /// The entry point to the transformation.
   void run() override {
      stripFunction(getFunction());
      invalidateAnalysis(PILAnalysis::InvalidationKind::Instructions);
   }
};
} // end anonymous namespace

PILTransform *polar::createStripDebugInfo() {
   return new StripDebugInfo();
}
