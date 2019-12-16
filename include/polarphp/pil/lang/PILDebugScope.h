//===--- PILDebugScope.h - DebugScopes for PIL code -------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_DEBUGSCOPE_H
#define POLARPHP_PIL_DEBUGSCOPE_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/raw_ostream.h"
#include "polarphp/pil/lang/PILAllocated.h"
#include "polarphp/pil/lang/PILLocation.h"

namespace polar {

class PILDebugLocation;
class PILDebugScope;
class PILFunction;
class PILInstruction;

/// This class stores a lexical scope as it is represented in the
/// debug info. In contrast to LLVM IR, PILDebugScope also holds all
/// the inlining information. In LLVM IR the inline info is part of
/// DILocation.
class PILDebugScope : public PILAllocated<PILDebugScope> {
public:
   /// The AST node this lexical scope represents.
   PILLocation Loc;
   /// Always points to the parent lexical scope.
   /// For top-level scopes, this is the PILFunction.
   PointerUnion<const PILDebugScope *, PILFunction *> Parent;
   /// An optional chain of inlined call sites.
   ///
   /// If this scope is inlined, this points to a special "scope" that
   /// holds the location of the call site.
   const PILDebugScope *InlinedCallSite;

   PILDebugScope(PILLocation Loc, PILFunction *PILFn,
                 const PILDebugScope *ParentScope = nullptr,
                 const PILDebugScope *InlinedCallSite = nullptr);

   /// Create a scope for an artificial function.
   PILDebugScope(PILLocation Loc);

   /// Return the function this scope originated from before being inlined.
   PILFunction *getInlinedFunction() const;

   /// Return the parent function of this scope. If the scope was
   /// inlined this recursively returns the function it was inlined
   /// into.
   PILFunction *getParentFunction() const;

#ifndef NDEBUG
   POLAR_DEBUG_DUMPER(dump(SourceManager &SM,
                              llvm::raw_ostream &OS = llvm::errs(),
                              unsigned Indent = 0));
   POLAR_DEBUG_DUMPER(dump(PILModule &Mod));
#endif
};

/// Determine whether an instruction may not have a PILDebugScope.
bool maybeScopeless(PILInstruction &I);

/// Knows how to make a deep copy of a debug scope.
class ScopeCloner {
   llvm::SmallDenseMap<const PILDebugScope *,
      const PILDebugScope *> ClonedScopeCache;
   PILFunction &NewFn;
public:
   /// ScopeCloner expects NewFn to be a clone of the original
   /// function, with all debug scopes and locations still pointing to
   /// the original function.
   ScopeCloner(PILFunction &NewFn);
   /// Return a (cached) deep copy of a scope.
   const PILDebugScope *getOrCreateClonedScope(const PILDebugScope *OrigScope);
};

} // polar

#endif
