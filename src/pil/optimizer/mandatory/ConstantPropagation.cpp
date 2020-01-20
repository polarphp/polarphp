//===--- ConstantPropagation.cpp - Constant fold and diagnose overflows ---===//
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

#define DEBUG_TYPE "constant-propagation"

#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/PILOptFunctionBuilder.h"
#include "polarphp/pil/optimizer/utils/ConstantFolding.h"

using namespace polar;

//===----------------------------------------------------------------------===//
//                              Top Level Driver
//===----------------------------------------------------------------------===//

namespace {

class ConstantPropagation : public PILFunctionTransform {
   bool EnableDiagnostics;

public:
   ConstantPropagation(bool EnableDiagnostics) :
      EnableDiagnostics(EnableDiagnostics) {}

private:
   /// The entry point to the transformation.
   void run() override {
      PILOptFunctionBuilder FuncBuilder(*this);
      ConstantFolder Folder(FuncBuilder, getOptions().AssertConfig,
                            EnableDiagnostics);
      Folder.initializeWorklist(*getFunction());
      auto Invalidation = Folder.processWorkList();

      if (Invalidation != PILAnalysis::InvalidationKind::Nothing) {
         invalidateAnalysis(Invalidation);
      }
   }
};

} // end anonymous namespace

PILTransform *polar::createDiagnosticConstantPropagation() {
   // Diagostic propagation is rerun on deserialized PIL because it is sensitive
   // to assert configuration.
   return new ConstantPropagation(true /*enable diagnostics*/);
}

PILTransform *polar::createPerformanceConstantPropagation() {
   return new ConstantPropagation(false /*disable diagnostics*/);
}
