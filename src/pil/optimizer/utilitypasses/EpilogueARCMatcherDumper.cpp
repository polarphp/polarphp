//===--- EpilogueARCMatcherDumper.cpp - Find Epilogue Releases ------------===//
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

#define DEBUG_TYPE "pil-epilogue-arc-dumper"

#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "polarphp/pil/optimizer/analysis/AliasAnalysis.h"
#include "polarphp/pil/optimizer/analysis/EpilogueARCAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"

using namespace polar;

//===----------------------------------------------------------------------===//
//                              Top Level Driver
//===----------------------------------------------------------------------===//

namespace {

/// Find and dump the epilogue release instructions for the arguments.
class PILEpilogueARCMatcherDumper : public PILModuleTransform {
   void run() override {
      for (auto &F: *getModule()) {
         // Function is not definition.
         if (!F.isDefinition())
            continue;

         // Find the epilogue releases of each owned argument.
         for (auto Arg : F.getArguments()) {
            auto *EA = PM->getAnalysis<EpilogueARCAnalysis>()->get(&F);
            llvm::outs() <<"START: " <<  F.getName() << "\n";
            llvm::outs() << *Arg;

            // Find the retain instructions for the argument.
            llvm::SmallSetVector<PILInstruction *, 1> RelInsts =
               EA->computeEpilogueARCInstructions(EpilogueARCContext::EpilogueARCKind::Release,
                                                  Arg);
            for (auto I : RelInsts) {
               llvm::outs() << *I << "\n";
            }

            // Find the release instructions for the argument.
            llvm::SmallSetVector<PILInstruction *, 1> RetInsts =
               EA->computeEpilogueARCInstructions(EpilogueARCContext::EpilogueARCKind::Retain,
                                                  Arg);
            for (auto I : RetInsts) {
               llvm::outs() << *I << "\n";
            }

            llvm::outs() <<"FINISH: " <<  F.getName() << "\n";
         }
      }
   }

};

} // end anonymous namespace

PILTransform *polar::createEpilogueARCMatcherDumper() {
   return new PILEpilogueARCMatcherDumper();
}
