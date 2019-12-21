//===--- PILDebuggerClient.h - Interfaces from PILGen to LLDB ---*- C++ -*-===//
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
//
// This file defines the abstract PILDebuggerClient class.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PILDEBUGGERCLIENT_H
#define POLARPHP_PIL_PILDEBUGGERCLIENT_H

#include "polarphp/ast/DebuggerClient.h"
#include "polarphp/pil/lang/PILLocation.h"
#include "polarphp/pil/lang/PILValue.h"

namespace polar {

class PILBuilder;

class PILDebuggerClient : public DebuggerClient {
public:
   using ResultVector = SmallVectorImpl<LookupResultEntry>;

   PILDebuggerClient(AstContext &C) : DebuggerClient(C) { }
   virtual ~PILDebuggerClient() = default;

   /// DebuggerClient is asked to emit PIL references to locals,
   /// permitting PILGen to access them like any other variables.
   /// This avoids generation of properties.
   virtual PILValue emitLValueForVariable(VarDecl *var,
                                          PILBuilder &builder) = 0;

   inline PILDebuggerClient *getAsPILDebuggerClient() {
      return this;
   }
private:
   virtual void anchor();
};

} // namespace polar

#endif
