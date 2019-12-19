//===--- AADumper.cpp - Compare all values in Function with AA ------------===//
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
/// This pass collects all values in a function and applies alias analysis to
/// them. The purpose of this is to enable unit tests for PIL Alias Analysis
/// implementations independent of any other passes.
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pil-aa-evaluator"

#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/optimizer/analysis/AliasAnalysis.h"
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
      for (auto &II : BB)
         for (auto result : II.getResults())
            Values.push_back(result);
   }
   return Values.size() > 1;
}

//===----------------------------------------------------------------------===//
//                              Top Level Driver
//===----------------------------------------------------------------------===//

namespace {

/// Dumps the alias relations between all instructions of a function.
class PILAADumper : public PILModuleTransform {

   void run() override {
      for (auto &Fn: *getModule()) {
         llvm::outs() << "@" << Fn.getName() << "\n";
         // Gather up all Values in Fn.
         std::vector<PILValue> Values;
         if (!gatherValues(Fn, Values))
            continue;

         AliasAnalysis *AA = PM->getAnalysis<AliasAnalysis>();

         // A cache
         llvm::DenseMap<uint64_t, AliasAnalysis::AliasResult> Results;

         // Emit the N^2 alias evaluation of the values.
         unsigned PairCount = 0;
         for (unsigned i1 = 0, e1 = Values.size(); i1 != e1; ++i1) {
            for (unsigned i2 = 0, e2 = Values.size(); i2 != e2; ++i2) {
               auto V1 = Values[i1];
               auto V2 = Values[i2];

               auto Result =
                  AA->alias(V1, V2, computeTBAAType(V1), computeTBAAType(V2));

               // Results should always be the same. But if they are different print
               // it out so we find the error. This should make our test results less
               // verbose.
               uint64_t Key = uint64_t(i1) | (uint64_t(i2) << 32);
               uint64_t OpKey = uint64_t(i2) | (uint64_t(i1) << 32);
               auto R = Results.find(OpKey);
               if (R != Results.end() && R->second == Result)
                  continue;

               Results[Key] = Result;
               llvm::outs() << "PAIR #" << PairCount++ << ".\n" << V1 << V2 << Result
                            << "\n";
            }
         }
         llvm::outs() << "\n";
      }
   }

};

} // end anonymous namespace

PILTransform *polar::createAADumper() { return new PILAADumper(); }
