//===--- PILGenCleanup.cpp ------------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
///
/// Perform peephole-style "cleanup" to aid PIL diagnostic passes.
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "silgen-cleanup"

#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/CanonicalizeInstruction.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"

using namespace polar;

// Define a CanonicalizeInstruction subclass for use in PILGenCleanup.
struct PILGenCanonicalize final : CanonicalizeInstruction {
   bool changed = false;
   llvm::SmallPtrSet<PILInstruction *, 16> deadOperands;

   PILGenCanonicalize() : CanonicalizeInstruction(DEBUG_TYPE) {}

   void notifyNewInstruction(PILInstruction *) override { changed = true; }

   // Just delete the given 'inst' and record its operands. The callback isn't
   // allowed to mutate any other instructions.
   void killInstruction(PILInstruction *inst) override {
      deadOperands.erase(inst);
      for (auto &operand : inst->getAllOperands()) {
         if (auto *operInst = operand.get()->getDefiningInstruction())
            deadOperands.insert(operInst);
      }
      inst->eraseFromParent();
      changed = true;
   }

   void notifyHasNewUsers(PILValue) override { changed = true; }

   PILBasicBlock::iterator deleteDeadOperands(PILBasicBlock::iterator nextII) {
      // Delete trivially dead instructions in non-determistic order.
      while (!deadOperands.empty()) {
         PILInstruction *deadOperInst = *deadOperands.begin();
         // Make sure at least the first instruction is removed from the set.
         deadOperands.erase(deadOperInst);
         recursivelyDeleteTriviallyDeadInstructions(
            deadOperInst, false,
            [&](PILInstruction *deadInst) {
               LLVM_DEBUG(llvm::dbgs() << "Trivially dead: " << *deadInst);
               if (nextII == deadInst->getIterator())
                  ++nextII;
               deadOperands.erase(deadInst);
            });
      }
      return nextII;
   }
};

//===----------------------------------------------------------------------===//
// PILGenCleanup: Top-Level Module Transform
//===----------------------------------------------------------------------===//

namespace {

// PILGenCleanup must run on all functions that will be seen by any analysis
// used by diagnostics before transforming the function that requires the
// analysis. e.g. Closures need to be cleaned up before the closure's parent can
// be diagnosed.
//
// TODO: This pass can be converted to a function transform if the mandatory
// pipeline runs in bottom-up closure order.
struct PILGenCleanup : PILModuleTransform {
   void run() override;
};

void PILGenCleanup::run() {
   auto &module = *getModule();
   for (auto &function : module) {
      LLVM_DEBUG(llvm::dbgs()
                    << "\nRunning PILGenCleanup on " << function.getName() << "\n");

      PILGenCanonicalize sgCanonicalize;

      // Iterate over all blocks even if they aren't reachable. No phi-less
      // dataflow cycles should have been created yet, and these transformations
      // are simple enough they shouldn't be affected by cycles.
      for (auto &bb : function) {
         for (auto ii = bb.begin(), ie = bb.end(); ii != ie;) {
            ii = sgCanonicalize.canonicalize(&*ii);
            ii = sgCanonicalize.deleteDeadOperands(ii);
         }
      }
      if (sgCanonicalize.changed) {
         auto invalidKind = PILAnalysis::InvalidationKind::Instructions;
         invalidateAnalysis(&function, invalidKind);
      }
   }
}

} // end anonymous namespace

PILTransform *polar::createPILGenCleanup() { return new PILGenCleanup(); }
