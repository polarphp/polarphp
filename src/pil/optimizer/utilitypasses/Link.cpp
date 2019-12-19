//===--- Link.cpp - Link in transparent PILFunctions from module ----------===//
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

#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/lang/PILModule.h"

using namespace polar;

//===----------------------------------------------------------------------===//
//                          Top Level Driver
//===----------------------------------------------------------------------===//

namespace {

/// Copies code from the standard library into the user program to enable
/// optimizations.
class PILLinker : public PILModuleTransform {
   PILModule::LinkingMode LinkMode;

public:
   explicit PILLinker(PILModule::LinkingMode LinkMode) : LinkMode(LinkMode) {}

   void run() override {
      PILModule &M = *getModule();
      for (auto &Fn : M)
         if (M.linkFunction(&Fn, LinkMode))
            invalidateAnalysis(&Fn, PILAnalysis::InvalidationKind::Everything);
   }

};
} // end anonymous namespace

PILTransform *polar::createMandatoryPILLinker() {
   return new PILLinker(PILModule::LinkingMode::LinkNormal);
}

PILTransform *polar::createPerformancePILLinker() {
   return new PILLinker(PILModule::LinkingMode::LinkAll);
}
