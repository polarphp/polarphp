//===--- PILDebugScope.cpp - DebugScopes for PIL code ---------------------===//
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
///
/// \file
///
/// This file defines a container for scope information used to
/// generate debug info.
///
//===----------------------------------------------------------------------===//

#include "polarphp/pil/lang/PILDebugScope.h"
#include "polarphp/pil/lang/PILFunction.h"

using namespace polar;

PILDebugScope::PILDebugScope(PILLocation Loc, PILFunction *PILFn,
                             const PILDebugScope *ParentScope,
                             const PILDebugScope *InlinedCallSite)
   : Loc(Loc), InlinedCallSite(InlinedCallSite) {
   if (ParentScope)
      Parent = ParentScope;
   else {
      assert(PILFn && "no parent provided");
      Parent = PILFn;
   }
}

PILDebugScope::PILDebugScope(PILLocation Loc)
   : Loc(Loc), InlinedCallSite(nullptr) {}

PILFunction *PILDebugScope::getInlinedFunction() const {
   if (Parent.isNull())
      return nullptr;

   const PILDebugScope *Scope = this;
   while (Scope->Parent.is<const PILDebugScope *>())
      Scope = Scope->Parent.get<const PILDebugScope *>();
   assert(Scope->Parent.is<PILFunction *>() && "orphaned scope");
   return Scope->Parent.get<PILFunction *>();
}

PILFunction *PILDebugScope::getParentFunction() const {
   if (InlinedCallSite)
      return InlinedCallSite->getParentFunction();
   if (auto *ParentScope = Parent.dyn_cast<const PILDebugScope *>())
      return ParentScope->getParentFunction();
   return Parent.get<PILFunction *>();
}
