//===--- PrettyStackTrace.cpp ---------------------------------------------===//
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

#include "polarphp/pil/optimizer/passmgr/PrettyStackTrace.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "llvm/Support/raw_ostream.h"

using namespace polar;

PrettyStackTracePILFunctionTransform::PrettyStackTracePILFunctionTransform(
   PILFunctionTransform *SFT, unsigned PassNumber):
   PrettyStackTracePILFunction("Running PIL Function Transform",
                               SFT->getFunction()),
   SFT(SFT), PassNumber(PassNumber) {}

void PrettyStackTracePILFunctionTransform::print(llvm::raw_ostream &out) const {
   out << "While running pass #" << PassNumber
       << " PILFunctionTransform \"" << SFT->getID()
       << "\" on PILFunction ";
   if (!SFT->getFunction()) {
      out << " <<null>>";
      return;
   }
   printFunctionInfo(out);
}

void PrettyStackTracePILModuleTransform::print(llvm::raw_ostream &out) const {
   out << "While running pass #" << PassNumber
       << " PILModuleTransform \"" << SMT->getID() << "\".\n";
}
