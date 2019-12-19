//===--- GenericSpecializer.cpp - Specialization of generic functions -----===//
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
// Specialize calls to generic functions by substituting static type
// information.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pil-generic-specializer"

#include "polarphp/pil/lang/OptimizationRemark.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/Generics.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "polarphp/pil/optimizer/utils/PILOptFunctionBuilder.h"
#include "llvm/ADT/SmallVector.h"

using namespace polar;

namespace {

class GenericSpecializer : public PILFunctionTransform {

   bool specializeAppliesInFunction(PILFunction &F);

   /// The entry point to the transformation.
   void run() override {
      PILFunction &F = *getFunction();

      // TODO: We should be able to handle ownership.
      if (F.hasOwnership())
         return;

      LLVM_DEBUG(llvm::dbgs() << "***** GenericSpecializer on function:"
                              << F.getName() << " *****\n");

      if (specializeAppliesInFunction(F))
         invalidateAnalysis(PILAnalysis::InvalidationKind::Everything);
   }

};

} // end anonymous namespace

bool GenericSpecializer::specializeAppliesInFunction(PILFunction &F) {
   PILOptFunctionBuilder FunctionBuilder(*this);
   DeadInstructionSet DeadApplies;
   llvm::SmallSetVector<PILInstruction *, 8> Applies;
   optremark::Emitter ORE(DEBUG_TYPE, F.getModule());

   bool Changed = false;
   for (auto &BB : F) {
      // Collect the applies for this block in reverse order so that we
      // can pop them off the end of our vector and process them in
      // forward order.
      for (auto It = BB.rbegin(), End = BB.rend(); It != End; ++It) {
         auto *I = &*It;

         // Skip non-apply instructions, apply instructions with no
         // substitutions, apply instructions where we do not statically
         // know the called function, and apply instructions where we do
         // not have the body of the called function.
         ApplySite Apply = ApplySite::isa(I);
         if (!Apply || !Apply.hasSubstitutions())
            continue;

         auto *Callee = Apply.getReferencedFunctionOrNull();
         if (!Callee)
            continue;
         if (!Callee->isDefinition()) {
            ORE.emit([&]() {
               using namespace optremark;
               return RemarkMissed("NoDef", *I)
                  << "Unable to specialize generic function "
                  << NV("Callee", Callee) << " since definition is not visible";
            });
            continue;
         }

         Applies.insert(Apply.getInstruction());
      }

      // Attempt to specialize each apply we collected, deleting any
      // that we do specialize (along with other instructions we clone
      // in the process of doing so). We pop from the end of the list to
      // avoid tricky iterator invalidation issues.
      while (!Applies.empty()) {
         auto *I = Applies.pop_back_val();
         auto Apply = ApplySite::isa(I);
         assert(Apply && "Expected an apply!");
         PILFunction *Callee = Apply.getReferencedFunctionOrNull();
         assert(Callee && "Expected to have a known callee");

         if (!Apply.canOptimize() || !Callee->shouldOptimize())
            continue;

         // We have a call that can potentially be specialized, so
         // attempt to do so.
         llvm::SmallVector<PILFunction *, 2> NewFunctions;
         trySpecializeApplyOfGeneric(FunctionBuilder, Apply, DeadApplies,
                                     NewFunctions, ORE);

         // Remove all the now-dead applies. We must do this immediately
         // rather than defer it in order to avoid problems with cloning
         // dead instructions when doing recursive specialization.
         while (!DeadApplies.empty()) {
            auto *AI = DeadApplies.pop_back_val();

            // Remove any applies we are deleting so that we don't attempt
            // to specialize them.
            Applies.remove(AI);

            recursivelyDeleteTriviallyDeadInstructions(AI, true);
            Changed = true;
         }

         // If calling the specialization utility resulted in new functions
         // (as opposed to returning a previous specialization), we need to notify
         // the pass manager so that the new functions get optimized.
         for (PILFunction *NewF : reverse(NewFunctions)) {
            addFunctionToPassManagerWorklist(NewF, Callee);
         }
      }
   }

   return Changed;
}

PILTransform *polar::createGenericSpecializer() {
   return new GenericSpecializer();
}
