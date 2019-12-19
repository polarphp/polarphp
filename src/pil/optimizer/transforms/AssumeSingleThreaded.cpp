//===--- AssumeSingleThreaded.cpp - Assume single-threaded execution  -----===//
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
// Assume that user code is single-thread.
//
// Convert all reference counting operations into non-atomic ones.
//
// To get read of most atomic reference counting operations, the standard
// library should be compiled in this mode as well
//
// This pass affects only reference counting operations resulting from PIL
// instructions. It wouldn't affect places in the runtime C++ code which
// hard-code calls to retain/release. We could take advantage of the Instruments
// instrumentation stubs to redirect calls from the runtime if it was
// significant, or else just build a single-threaded variant of the runtime.
//
//===----------------------------------------------------------------------===//

#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "llvm/Support/CommandLine.h"

using namespace polar;

namespace {
class AssumeSingleThreaded : public polar::PILFunctionTransform {
   /// The entry point to the transformation.
   void run() override {
      if (!getOptions().AssumeSingleThreaded)
         return;
      for (auto &BB : *getFunction()) {
         for (auto &I : BB) {
            if (auto RCInst = dyn_cast<RefCountingInst>(&I))
               RCInst->setNonAtomic();
         }
      }
      invalidateAnalysis(PILAnalysis::InvalidationKind::Instructions);
   }

};
} // end anonymous namespace


PILTransform *polar::createAssumeSingleThreaded() {
   return new AssumeSingleThreaded();
}
