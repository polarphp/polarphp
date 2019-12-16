//===--- PrettyStackTrace.h - Crash trace information -----------*- C++ -*-===//
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
// This file defines PIL-specific RAII classes that give better diagnostic
// output about when, exactly, a crash is occurring.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PRETTYSTACKTRACE_H
#define POLARPHP_PIL_PRETTYSTACKTRACE_H

#include "polarphp/pil/lang/PILLocation.h"
#include "llvm/Support/PrettyStackTrace.h"

namespace polar {

class AstContext;
class PILFunction;
class PILNode;

void printPILLocationDescription(llvm::raw_ostream &out, PILLocation loc,
                                 AstContext &ctx);

/// PrettyStackTraceLocation - Observe that we are doing some
/// processing starting at a PIL location.
class PrettyStackTracePILLocation : public llvm::PrettyStackTraceEntry {
   PILLocation Loc;
   const char *Action;
   AstContext &Context;
public:
   PrettyStackTracePILLocation(const char *action, PILLocation loc,
                               AstContext &C)
      : Loc(loc), Action(action), Context(C) {}
   virtual void print(llvm::raw_ostream &OS) const;
};


/// Observe that we are doing some processing of a PIL function.
class PrettyStackTracePILFunction : public llvm::PrettyStackTraceEntry {
   const PILFunction *TheFn;
   const char *Action;
public:
   PrettyStackTracePILFunction(const char *action, const PILFunction *F)
      : TheFn(F), Action(action) {}
   virtual void print(llvm::raw_ostream &OS) const;
protected:
   void printFunctionInfo(llvm::raw_ostream &out) const;
};

/// Observe that we are visiting PIL nodes.
class PrettyStackTracePILNode : public llvm::PrettyStackTraceEntry {
   const PILNode *Node;
   const char *Action;

public:
   PrettyStackTracePILNode(const char *action, const PILNode *node)
      : Node(node), Action(action) {}

   virtual void print(llvm::raw_ostream &OS) const;
};

} // end namespace polar

#endif
