//===--- ComputeLoopInfo.cpp ----------------------------------------------===//
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

#define DEBUG_TYPE "pil-compute-loop-info"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/analysis/LoopAnalysis.h"

using namespace polar;

class ComputeLoopInfo : public PILFunctionTransform {

   void run() override {
      PM->getAnalysis<PILLoopAnalysis>()->get(getFunction());
   }
};

PILTransform *polar::createComputeLoopInfo() { return new ComputeLoopInfo(); }
