//===--- RCIdentityDumper.cpp ---------------------------------------------===//
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
/// This pass applies the RCIdentityAnalysis to all PILValues in a function in
/// order to apply FileCheck testing to RCIdentityAnalysis without needing to
/// test any other passes.
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pil-rc-identity-dumper"

#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/optimizer/analysis/RCIdentityAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"

using namespace polar;

namespace {

/// Dumps the alias relations between all instructions of a function.
class RCIdentityDumper : public PILFunctionTransform {

   void run() override {
      auto *Fn = getFunction();
      auto *RCId = PM->getAnalysis<RCIdentityAnalysis>()->get(Fn);

      std::vector<std::pair<PILValue, PILValue>> Results;
      unsigned ValueCount = 0;
      llvm::MapVector<PILValue, uint64_t> ValueToValueIDMap;

      llvm::outs() << "@" << Fn->getName() << "@\n";

      for (auto &BB : *Fn) {
         for (auto *Arg : BB.getArguments()) {
            ValueToValueIDMap[Arg] = ValueCount++;
            Results.push_back({Arg, RCId->getRCIdentityRoot(Arg)});
         }
         for (auto &II : BB) {
            for (auto V : II.getResults()) {
               ValueToValueIDMap[V] = ValueCount++;
               Results.push_back({V, RCId->getRCIdentityRoot(V)});
            }
         }
      }

      llvm::outs() << "ValueMap:\n";
      for (auto P : ValueToValueIDMap) {
         llvm::outs() << "\tValueMap[" << P.second << "] = " << P.first;
      }

      unsigned ResultCount = 0;
      for (auto P : Results) {
         llvm::outs() << "RESULT #" << ResultCount++ << ": "
                      << ValueToValueIDMap[P.first] << " = "
                      << ValueToValueIDMap[P.second] << "\n";
      }

      llvm::outs() << "\n";
   }

};

} // end anonymous namespace

PILTransform *polar::createRCIdentityDumper() { return new RCIdentityDumper(); }
