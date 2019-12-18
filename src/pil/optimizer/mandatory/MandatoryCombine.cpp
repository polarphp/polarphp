//===------- MandatoryCombiner.cpp ----------------------------------------===//
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
///  \file
///
///  Defines the MandatoryCombiner function transform.  The pass contains basic
///  instruction combines to be performed at the begining of both the Onone and
///  also the performance pass pipelines, after the diagnostics passes have been
///  run.  It is intended to be run before and to be independent of other
///  transforms.
///
///  The intention of this pass is to be a place for mandatory peepholes that
///  are not needed for diagnostics. Please put any such peepholes here instead
///  of in the diagnostic passes.
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pil-mandatory-combiner"

#include "polarphp/basic/LLVM.h"
#include "polarphp/pil/lang/PILInstructionWorklist.h"
#include "polarphp/pil/lang/PILVisitor.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>

using namespace polar;

//===----------------------------------------------------------------------===//
//                                  Utility
//===----------------------------------------------------------------------===//

/// \returns whether all the values are of trivial type in the provided
///          function.
template <typename Values>
static bool areAllValuesTrivial(Values values, PILFunction &function) {
   return llvm::all_of(values, [&](PILValue value) -> bool {
      return value->getType().isTrivial(function);
   });
}

//===----------------------------------------------------------------------===//
//                        MandatoryCombiner Interface
//===----------------------------------------------------------------------===//

namespace {

class MandatoryCombiner final
   : public PILInstructionVisitor<MandatoryCombiner, PILInstruction *> {

   using Worklist = SmallPILInstructionWorklist<256>;

   /// The list of instructions remaining to visit, perhaps to combine.
   Worklist worklist;

   /// Whether any changes have been made.
   bool madeChange;

   /// The number of times that the worklist has been processed.
   unsigned iteration;

   InstModCallbacks instModCallbacks;
   SmallVectorImpl<PILInstruction *> &createdInstructions;
   SmallVector<PILInstruction *, 16> instructionsPendingDeletion;

public:
   MandatoryCombiner(
      SmallVectorImpl<PILInstruction *> &createdInstructions)
      : worklist("MC"), madeChange(false), iteration(0),
        instModCallbacks(
           [&](PILInstruction *instruction) {
              worklist.erase(instruction);
              instructionsPendingDeletion.push_back(instruction);
           },
           [&](PILInstruction *instruction) { worklist.add(instruction); }),
        createdInstructions(createdInstructions){};

   void addReachableCodeToWorklist(PILFunction &function);

   /// \return whether a change was made.
   bool doOneIteration(PILFunction &function, unsigned iteration);

   void clear() {
      iteration = 0;
      worklist.resetChecked();
      madeChange = false;
   }

   /// Applies the MandatoryCombiner to the provided function.
   ///
   /// \param function the function to which to apply the MandatoryCombiner.
   ///
   /// \return whether a change was made.
   bool runOnFunction(PILFunction &function) {
      bool changed = false;

      while (doOneIteration(function, iteration)) {
         changed = true;
         ++iteration;
      }

      return changed;
   }

   /// Base visitor that does not do anything.
   PILInstruction *visitPILInstruction(PILInstruction *) { return nullptr; }
   PILInstruction *visitApplyInst(ApplyInst *instruction);
};

} // end anonymous namespace

//===----------------------------------------------------------------------===//
//               MandatoryCombiner Non-Visitor Utility Methods
//===----------------------------------------------------------------------===//

void MandatoryCombiner::addReachableCodeToWorklist(PILFunction &function) {
   SmallVector<PILBasicBlock *, 32> blockWorklist;
   SmallPtrSet<PILBasicBlock *, 32> blockAlreadyAddedToWorklist;
   SmallVector<PILInstruction *, 128> initialInstructionWorklist;

   {
      auto *firstBlock = &*function.begin();
      blockWorklist.push_back(firstBlock);
      blockAlreadyAddedToWorklist.insert(firstBlock);
   }

   while (!blockWorklist.empty()) {
      auto *block = blockWorklist.pop_back_val();

      for (auto iterator = block->begin(), end = block->end(); iterator != end;) {
         auto *instruction = &*iterator;
         ++iterator;

         if (isInstructionTriviallyDead(instruction)) {
            continue;
         }

         initialInstructionWorklist.push_back(instruction);
      }

      llvm::copy_if(block->getSuccessorBlocks(),
                    std::back_inserter(blockWorklist),
                    [&](PILBasicBlock *block) -> bool {
                       return blockAlreadyAddedToWorklist.insert(block).second;
                    });
   }

   worklist.addInitialGroup(initialInstructionWorklist);
}

bool MandatoryCombiner::doOneIteration(PILFunction &function,
                                       unsigned iteration) {
   madeChange = false;

   addReachableCodeToWorklist(function);

   while (!worklist.isEmpty()) {
      auto *instruction = worklist.pop_back_val();
      if (instruction == nullptr) {
         continue;
      }

#ifndef NDEBUG
      std::string instructionDescription;
#endif
      LLVM_DEBUG(llvm::raw_string_ostream SS(instructionDescription);
                    instruction->print(SS); instructionDescription = SS.str(););
      LLVM_DEBUG(llvm::dbgs()
                    << "MC: Visiting: " << instructionDescription << '\n');

      if (auto replacement = visit(instruction)) {
         worklist.replaceInstructionWithInstruction(instruction, replacement
#ifndef NDEBUG
            ,
                                                    instructionDescription
#endif
         );
      }

      for (PILInstruction *instruction : instructionsPendingDeletion) {
         worklist.eraseInstFromFunction(*instruction);
      }
      instructionsPendingDeletion.clear();

      // Our tracking list has been accumulating instructions created by the
      // PILBuilder during this iteration. Go through the tracking list and add
      // its contents to the worklist and then clear said list in preparation
      // for the next iteration.
      for (PILInstruction *instruction : createdInstructions) {
         LLVM_DEBUG(llvm::dbgs() << "MC: add " << *instruction
                                 << " from tracking list to worklist\n");
         worklist.add(instruction);
      }
      createdInstructions.clear();
   }

   worklist.resetChecked();
   return madeChange;
}

//===----------------------------------------------------------------------===//
//                     MandatoryCombiner Visitor Methods
//===----------------------------------------------------------------------===//

PILInstruction *MandatoryCombiner::visitApplyInst(ApplyInst *instruction) {
   // Apply this pass only to partial applies all of whose arguments are
   // trivial.
   auto calledValue = instruction->getCallee();
   if (calledValue == nullptr) {
      return nullptr;
   }
   auto fullApplyCallee = calledValue->getDefiningInstruction();
   if (fullApplyCallee == nullptr) {
      return nullptr;
   }
   auto partialApply = dyn_cast<PartialApplyInst>(fullApplyCallee);
   if (partialApply == nullptr) {
      return nullptr;
   }
   auto *function = partialApply->getCalleeFunction();
   if (function == nullptr) {
      return nullptr;
   }
   ApplySite fullApplySite(instruction);
   auto fullApplyArguments = fullApplySite.getArguments();
   if (!areAllValuesTrivial(fullApplyArguments, *function)) {
      return nullptr;
   }
   auto partialApplyArguments = ApplySite(partialApply).getArguments();
   if (!areAllValuesTrivial(partialApplyArguments, *function)) {
      return nullptr;
   }

   auto callee = partialApply->getCallee();

   ApplySite partialApplySite(partialApply);

   SmallVector<PILValue, 8> argsVec;
   llvm::copy(fullApplyArguments, std::back_inserter(argsVec));
   llvm::copy(partialApplyArguments, std::back_inserter(argsVec));

   PILBuilderWithScope builder(instruction, &createdInstructions);
   ApplyInst *replacement = builder.createApply(
      /*Loc=*/instruction->getDebugLocation().getLocation(), /*Fn=*/callee,
      /*Subs=*/partialApply->getSubstitutionMap(),
      /*Args*/ argsVec,
      /*isNonThrowing=*/instruction->isNonThrowing(),
      /*SpecializationInfo=*/partialApply->getSpecializationInfo());

   worklist.replaceInstructionWithInstruction(instruction, replacement
#ifndef NDEBUG
      ,
      /*instructionDescription=*/""
#endif
   );
   tryDeleteDeadClosure(partialApply, instModCallbacks);
   return nullptr;
}

//===----------------------------------------------------------------------===//
//                            Top Level Entrypoint
//===----------------------------------------------------------------------===//

namespace {

class MandatoryCombine final : public PILFunctionTransform {

   SmallVector<PILInstruction *, 64> createdInstructions;

   void run() override {
      auto *function = getFunction();

      // If this function is an external declaration, bail. We only want to visit
      // functions with bodies.
      if (function->isExternalDeclaration()) {
         return;
      }

      MandatoryCombiner combiner(createdInstructions);
      bool madeChange = combiner.runOnFunction(*function);

      if (madeChange) {
         invalidateAnalysis(PILAnalysis::InvalidationKind::Instructions);
      }
   }

   void handleDeleteNotification(PILNode *node) override {
      // Remove instructions that were both created and deleted from the list of
      // created instructions which will eventually be added to the worklist.

      auto *instruction = dyn_cast<PILInstruction>(node);
      if (instruction == nullptr) {
         return;
      }

      // Linear searching the tracking list doesn't hurt because usually it only
      // contains a few elements.
      auto iterator = find(createdInstructions, instruction);
      if (createdInstructions.end() != iterator) {
         createdInstructions.erase(iterator);
      }
   }

   bool needsNotifications() override { return true; }
};

} // end anonymous namespace

PILTransform *polar::createMandatoryCombine() { return new MandatoryCombine(); }
