//===--- ParsePILSupport.h - Interface with ParsePIL ------------*- C++ -*-===//
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

#ifndef POLARPHP_LLPARSER_PARSEPILSUPPORT_H
#define POLARPHP_LLPARSER_PARSEPILSUPPORT_H

#include "llvm/Support/PrettyStackTrace.h"

namespace polar::llparser {
class Parser;

/// Interface between the Parse and ParsePIL libraries, to avoid circular
/// dependencies.
class PILParserTUStateBase {
   virtual void anchor();
protected:
   PILParserTUStateBase() = default;
   virtual ~PILParserTUStateBase() = default;
public:
   virtual bool parseDeclPIL(Parser &P) = 0;
   virtual bool parseDeclPILStage(Parser &P) = 0;
   virtual bool parsePILVTable(Parser &P) = 0;
   virtual bool parsePILGlobal(Parser &P) = 0;
   virtual bool parsePILWitnessTable(Parser &P) = 0;
   virtual bool parsePILDefaultWitnessTable(Parser &P) = 0;
   virtual bool parsePILCoverageMap(Parser &P) = 0;
   virtual bool parsePILProperty(Parser &P) = 0;
   virtual bool parsePILScope(Parser &P) = 0;
};

/// To assist debugging parser crashes, tell us the location of the
/// current token.
class PrettyStackTraceParser : public llvm::PrettyStackTraceEntry {
   Parser &P;
public:
   explicit PrettyStackTraceParser(Parser &P) : P(P) {}
   void print(llvm::raw_ostream &out) const override;
};
} // end namespace polar::llparser

#endif // POLARPHP_LLPARSER_PARSEPILSUPPORT_H

