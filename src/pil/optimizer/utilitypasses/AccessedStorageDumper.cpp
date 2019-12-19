//===--- AccessedStorageDumper.cpp - Dump accessed storage for functions ---===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pil-accessed-storage-dumper"

#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/optimizer/analysis/AccessedStorageAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"

using namespace polar;

namespace {

/// Dumps per-function information on dynamically enforced formal accesses.
class AccessedStorageDumper : public PILModuleTransform {

   void run() override {
      auto *analysis = PM->getAnalysis<AccessedStorageAnalysis>();

      for (auto &fn : *getModule()) {
         llvm::outs() << "@" << fn.getName() << "\n";
         if (fn.empty()) {
            llvm::outs() << "<unknown>\n";
            continue;
         }
         const FunctionAccessedStorage &summary = analysis->getEffects(&fn);
         summary.print(llvm::outs());
      }
   }
};

} // end anonymous namespace

PILTransform *polar::createAccessedStorageDumper() {
   return new AccessedStorageDumper();
}
