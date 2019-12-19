//===--- AccessSummaryDumper.cpp - Dump access summaries for functions -----===//
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

#define DEBUG_TYPE "pil-access-summary-dumper"

#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/optimizer/analysis/AccessSummaryAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"

using namespace polar;

namespace {

/// Dumps summaries of kinds of accesses a function performs on its
/// @inout_aliasiable arguments.
class AccessSummaryDumper : public PILModuleTransform {

   void run() override {
      auto *analysis = PM->getAnalysis<AccessSummaryAnalysis>();

      for (auto &fn : *getModule()) {
         llvm::outs() << "@" << fn.getName() << "\n";
         if (fn.empty()) {
            llvm::outs() << "<unknown>\n";
            continue;
         }
         const AccessSummaryAnalysis::FunctionSummary &summary =
            analysis->getOrCreateSummary(&fn);
         summary.print(llvm::outs(), &fn);
         llvm::outs() << "\n";
      }
   }
};

} // end anonymous namespace

PILTransform *polar::createAccessSummaryDumper() {
   return new AccessSummaryDumper();
}
