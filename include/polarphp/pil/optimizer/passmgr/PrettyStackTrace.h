//===--- PrettyStackTrace.h - PrettyStackTrace for Transforms ---*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_PASSMANAGER_PRETTYSTACKTRACE_H
#define POLARPHP_PIL_OPTIMIZER_PASSMANAGER_PRETTYSTACKTRACE_H

#include "polarphp/pil/lang/PrettyStackTrace.h"
#include "llvm/Support/PrettyStackTrace.h"

namespace polar {

class PILFunctionTransform;
class PILModuleTransform;

class PrettyStackTracePILFunctionTransform
   : public PrettyStackTracePILFunction {
   PILFunctionTransform *SFT;
   unsigned PassNumber;

public:
   PrettyStackTracePILFunctionTransform(PILFunctionTransform *SFT,
                                        unsigned PassNumber);

   virtual void print(llvm::raw_ostream &OS) const;
};

class PrettyStackTracePILModuleTransform : public llvm::PrettyStackTraceEntry {
   PILModuleTransform *SMT;
   unsigned PassNumber;

public:
   PrettyStackTracePILModuleTransform(PILModuleTransform *SMT,
                                      unsigned PassNumber)
      : SMT(SMT), PassNumber(PassNumber) {}
   virtual void print(llvm::raw_ostream &OS) const;
};

} // end namespace polar

#endif
