//===--- Analysis.cpp - Swift Analysis ------------------------------------===//
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

#define DEBUG_TYPE "pil-analysis"

#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/PILOptions.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/optimizer/analysis/BasicCalleeAnalysis.h"
#include "polarphp/pil/optimizer/analysis/ClassHierarchyAnalysis.h"
#include "polarphp/pil/optimizer/analysis/DominanceAnalysis.h"
#include "polarphp/pil/optimizer/analysis/IVAnalysis.h"
#include "polarphp/pil/optimizer/analysis/PostOrderAnalysis.h"
#include "polarphp/pil/optimizer/analysis/InterfaceConformanceAnalysis.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"

using namespace polar;

void PILAnalysis::verifyFunction(PILFunction *F) {
   // Only functions with bodies can be analyzed by the analysis.
   assert(F->isDefinition() && "Can't analyze external functions");
}

PILAnalysis *polar::createDominanceAnalysis(PILModule *) {
   return new DominanceAnalysis();
}

PILAnalysis *polar::createPostDominanceAnalysis(PILModule *) {
   return new PostDominanceAnalysis();
}

PILAnalysis *polar::createInductionVariableAnalysis(PILModule *M) {
   return new IVAnalysis(M);
}

PILAnalysis *polar::createPostOrderAnalysis(PILModule *M) {
   return new PostOrderAnalysis();
}

PILAnalysis *polar::createClassHierarchyAnalysis(PILModule *M) {
   return new ClassHierarchyAnalysis(M);
}

PILAnalysis *polar::createBasicCalleeAnalysis(PILModule *M) {
   return new BasicCalleeAnalysis(M);
}

PILAnalysis *polar::createInterfaceConformanceAnalysis(PILModule *M) {
   return new InterfaceConformanceAnalysis(M);
}
