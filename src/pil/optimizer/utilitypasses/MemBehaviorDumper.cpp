//===--- MemBehaviorDumper.cpp --------------------------------------------===//
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

#define DEBUG_TYPE "pil-mem-behavior-dumper"

#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/optimizer/analysis/AliasAnalysis.h"
#include "polarphp/pil/optimizer/analysis/SideEffectAnalysis.h"
#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"

using namespace polar;

//===----------------------------------------------------------------------===//
//                               Value Gatherer
//===----------------------------------------------------------------------===//

// Return a list of all instruction values in Fn. Returns true if we have at
// least two values to compare.
static bool gatherValues(PILFunction &Fn, std::vector<PILValue> &Values) {
   for (auto &BB : Fn) {
      for (auto *Arg : BB.getArguments())
         Values.push_back(PILValue(Arg));
      for (auto &II : BB) {
         for (auto result : II.getResults())
            Values.push_back(result);
      }
   }
   return Values.size() > 1;
}

//===----------------------------------------------------------------------===//
//                              Top Level Driver
//===----------------------------------------------------------------------===//

namespace {

/// Dumps the memory behavior of instructions in a function.
class MemBehaviorDumper : public PILModuleTransform {

   // To reduce the amount of output, we only dump the memory behavior of
   // selected types of instructions.
   static bool shouldTestInstruction(PILInstruction *I) {
      // Only consider function calls.
      if (FullApplySite::isa(I))
         return true;

      return false;
   }

   void run() override {
      for (auto &Fn : *getModule()) {
         llvm::outs() << "@" << Fn.getName() << "\n";
         // Gather up all Values in Fn.
         std::vector<PILValue> Values;
         if (!gatherValues(Fn, Values))
            continue;

         AliasAnalysis *AA = PM->getAnalysis<AliasAnalysis>();

         unsigned PairCount = 0;
         for (auto &BB : Fn) {
            for (auto &I : BB) {
               if (shouldTestInstruction(&I)) {

                  // Print the memory behavior in relation to all other values in the
                  // function.
                  for (auto &V : Values) {
                     bool Read = AA->mayReadFromMemory(&I, V);
                     bool Write = AA->mayWriteToMemory(&I, V);
                     bool SideEffects = AA->mayHaveSideEffects(&I, V);
                     llvm::outs() << "PAIR #" << PairCount++ << ".\n"
                                  << "  " << I << "  " << V
                                  << "  r=" << Read << ",w=" << Write
                                  << ",se=" << SideEffects << "\n";
                  }
               }
            }
         }
         llvm::outs() << "\n";
      }
   }
};

} // end anonymous namespace

PILTransform *polar::createMemBehaviorDumper() {
   return new MemBehaviorDumper();
}
