//===--- LoopAnalysis.cpp - PIL Loop Analysis -----------------------------===//
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

#include "polarphp/pil/lang/Dominance.h"
#include "polarphp/pil/optimizer/analysis/DominanceAnalysis.h"
#include "polarphp/pil/optimizer/analysis/LoopAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/PassManager.h"

using namespace polar;

std::unique_ptr<PILLoopInfo>
PILLoopAnalysis::newFunctionAnalysis(PILFunction *F) {
   assert(DA != nullptr && "Expect a valid dominance analysis");
   DominanceInfo *DT = DA->get(F);
   assert(DT != nullptr && "Expect a valid dominance information");
   return std::make_unique<PILLoopInfo>(F, DT);
}

void PILLoopAnalysis::initialize(PILPassManager *PM) {
   DA = PM->getAnalysis<DominanceAnalysis>();
}

PILAnalysis *polar::createLoopAnalysis(PILModule *M) {
   return new PILLoopAnalysis(M);
}
