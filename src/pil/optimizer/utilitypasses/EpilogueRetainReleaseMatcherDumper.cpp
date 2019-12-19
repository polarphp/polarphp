//===--- EpilogueRetainReleaseMatcherDumper.cpp - Find Epilogue Releases --===//
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
/// This pass finds the epilogue releases matched to each argument of the
/// function.
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pil-epilogue-release-dumper"

#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/optimizer/analysis/AliasAnalysis.h"
#include "polarphp/pil/optimizer/analysis/ARCAnalysis.h"
#include "polarphp/pil/optimizer/analysis/RCIdentityAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"

using namespace polar;

//===----------------------------------------------------------------------===//
//                              Top Level Driver
//===----------------------------------------------------------------------===//

namespace {

/// Find and dump the epilogue release instructions for the arguments.
class PILEpilogueRetainReleaseMatcherDumper : public PILModuleTransform {

   void run() override {
      auto *AA = PM->getAnalysis<AliasAnalysis>();
      auto *RCIA = getAnalysis<RCIdentityAnalysis>();
      for (auto &Fn: *getModule()) {
         // Function is not definition.
         if (!Fn.isDefinition())
            continue;

         llvm::outs() << "START: pil @" << Fn.getName() << "\n";

         // Handle @owned return value.
         ConsumedResultToEpilogueRetainMatcher RetMap(RCIA->get(&Fn), AA, &Fn);
         for (auto &RI : RetMap)
            llvm::outs() << *RI;

         // Handle @owned function arguments.
         ConsumedArgToEpilogueReleaseMatcher RelMap(RCIA->get(&Fn), &Fn);
         // Iterate over arguments and dump their epilogue releases.
         for (auto Arg : Fn.getArguments()) {
            llvm::outs() << *Arg;
            // Can not find an epilogue release instruction for the argument.
            for (auto &RI : RelMap.getReleasesForArgument(Arg))
               llvm::outs() << *RI;
         }

         llvm::outs() << "END: pil @" << Fn.getName() << "\n";
      }
   }

};

} // end anonymous namespace

PILTransform *polar::createEpilogueRetainReleaseMatcherDumper() {
   return new PILEpilogueRetainReleaseMatcherDumper();
}
