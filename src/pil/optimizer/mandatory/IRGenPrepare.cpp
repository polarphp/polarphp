//===--- IRGenPrepare.cpp -------------------------------------------------===//
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
/// Cleanup PIL to make it suitable for IRGen.
///
/// We perform the following canonicalizations:
///
/// 1. We remove calls to Builtin.poundAssert() and Builtin.staticReport(),
///    which are not needed post PIL.
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pil-cleanup"

#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "polarphp/global/NameStrings.h"

using namespace polar;

static bool cleanFunction(PILFunction &fn) {
   bool madeChange = false;

   for (auto &bb : fn) {
      for (auto i = bb.begin(), e = bb.end(); i != e;) {
         // Make sure there is no iterator invalidation if the inspected
         // instruction gets removed from the block.
         PILInstruction *inst = &*i;
         ++i;

         // Remove calls to Builtin.poundAssert() and Builtin.staticReport().
         auto *bi = dyn_cast<BuiltinInst>(inst);
         if (!bi) {
            continue;
         }

         switch (bi->getBuiltinInfo().ID) {
            case BuiltinValueKind::CondFailMessage: {
               PILBuilderWithScope Builder(bi);
               Builder.createCondFail(bi->getLoc(), bi->getOperand(0),
                                      "unknown program error");
               LLVM_FALLTHROUGH;
            }
            case BuiltinValueKind::PoundAssert:
            case BuiltinValueKind::StaticReport:
               // The call to the builtin should get removed before we reach
               // IRGen.
               recursivelyDeleteTriviallyDeadInstructions(bi, /* Force */ true);
               madeChange = true;
               break;
            default:
               break;
         }
      }
   }

   return madeChange;
}

//===----------------------------------------------------------------------===//
//                            Top Level Entrypoint
//===----------------------------------------------------------------------===//

namespace {

class IRGenPrepare : public PILFunctionTransform {
   void run() override {
      PILFunction *F = getFunction();

      bool shouldInvalidate = cleanFunction(*F);

      if (shouldInvalidate)
         invalidateAnalysis(PILAnalysis::InvalidationKind::Instructions);
   }
};

} // end anonymous namespace

PILTransform *polar::createIRGenPrepare() {
   return new IRGenPrepare();
}
