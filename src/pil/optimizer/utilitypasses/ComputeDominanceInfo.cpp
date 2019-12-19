//===--- ComputeDominanceInfo.cpp -----------------------------------------===//
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

#define DEBUG_TYPE "sil-compute-dominance-info"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/analysis/DominanceAnalysis.h"

using namespace polar;

class ComputeDominanceInfo : public PILFunctionTransform {

   void run() override {
      PM->getAnalysis<DominanceAnalysis>()->get(getFunction());
      PM->getAnalysis<PostDominanceAnalysis>()->get(getFunction());
   }

};

PILTransform *polar::createComputeDominanceInfo() { return new ComputeDominanceInfo(); }
