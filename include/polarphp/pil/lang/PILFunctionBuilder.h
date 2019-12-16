//===--- PILFunctionBuilder.h ---------------------------------------------===//
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

#ifndef POLARPHP_PIL_PILFUNCTIONBUILDER_H
#define POLARPHP_PIL_PILFUNCTIONBUILDER_H

#include "polarphp/ast/Availability.h"
#include "polarphp/pil/lang/PILModule.h"

namespace polar {

class PILParserFunctionBuilder;
class PILSerializationFunctionBuilder;
class PILOptFunctionBuilder;
namespace lowering {
class PILGenFunctionBuilder;
} // namespace Lowering

/// A class for creating PILFunctions in a specific PILModule.
///
/// The intention is that this class is not used directly, but rather that each
/// part of the compiler that needs to create functions create a composition
/// type with PILFunctionBuilder as a field. This enables subsystems that use
/// PIL to:
///
/// 1. Enforce invariants in the type system. An example of this is in the
///    PILOptimizer where we want to ensure that the pass manager properly
///    notifies analyses whenever functions are created/destroyed.
///
/// 2. Have a convenient place to place utility functionality for creating
///    functions. Today the compiler has many small utility functions for
///    creating the underlying PILFunction that are generally quite verbose and
///    have shared code. These PILFunctionBuilder composition types will enable
///    code-reuse in between these different PILFunction creation sites.
class PILFunctionBuilder {
   PILModule &mod;
   AvailabilityContext availCtx;

   friend class PILParserFunctionBuilder;
   friend class PILSerializationFunctionBuilder;
   friend class PILOptFunctionBuilder;
   friend class lowering::PILGenFunctionBuilder;

   PILFunctionBuilder(PILModule &mod)
      : PILFunctionBuilder(mod,
                           AvailabilityContext::forDeploymentTarget(
                              mod.getAstContext())) {}

   PILFunctionBuilder(PILModule &mod, AvailabilityContext availCtx)
      : mod(mod), availCtx(availCtx) {}

   /// Return the declaration of a utility function that can, but needn't, be
   /// shared between different parts of a program.
   PILFunction *getOrCreateSharedFunction(PILLocation loc, StringRef name,
                                          CanPILFunctionType type,
                                          IsBare_t isBarePILFunction,
                                          IsTransparent_t isTransparent,
                                          IsSerialized_t isSerialized,
                                          ProfileCounter entryCount,
                                          IsThunk_t isThunk,
                                          IsDynamicallyReplaceable_t isDynamic);

   /// Return the declaration of a function, or create it if it doesn't exist.
   PILFunction *getOrCreateFunction(
      PILLocation loc, StringRef name, PILLinkage linkage,
      CanPILFunctionType type, IsBare_t isBarePILFunction,
      IsTransparent_t isTransparent, IsSerialized_t isSerialized,
      IsDynamicallyReplaceable_t isDynamic,
      ProfileCounter entryCount = ProfileCounter(),
      IsThunk_t isThunk = IsNotThunk,
      SubclassScope subclassScope = SubclassScope::NotApplicable);

   /// Return the declaration of a function, or create it if it doesn't exist.
   PILFunction *
   getOrCreateFunction(PILLocation loc, PILDeclRef constant,
                       ForDefinition_t forDefinition,
                       ProfileCounter entryCount = ProfileCounter());

   /// Create a function declaration.
   ///
   /// This signature is a direct copy of the signature of PILFunction::create()
   /// in order to simplify refactoring all PILFunction creation use-sites to use
   /// PILFunctionBuilder. Eventually the uses should probably be refactored.
   PILFunction *
   createFunction(PILLinkage linkage, StringRef name,
                  CanPILFunctionType loweredType, GenericEnvironment *genericEnv,
                  Optional<PILLocation> loc, IsBare_t isBarePILFunction,
                  IsTransparent_t isTrans, IsSerialized_t isSerialized,
                  IsDynamicallyReplaceable_t isDynamic,
                  ProfileCounter entryCount = ProfileCounter(),
                  IsThunk_t isThunk = IsNotThunk,
                  SubclassScope subclassScope = SubclassScope::NotApplicable,
                  Inline_t inlineStrategy = InlineDefault,
                  EffectsKind EK = EffectsKind::Unspecified,
                  PILFunction *InsertBefore = nullptr,
                  const PILDebugScope *DebugScope = nullptr);

   void addFunctionAttributes(PILFunction *F, DeclAttributes &Attrs,
                              PILModule &M, PILDeclRef constant = PILDeclRef());

   /// We do not expose this to everyone, instead we allow for our users to opt
   /// into this if they need to. Please do not do this in general! We only want
   /// to use this when deserializing a function body.
   static void setHasOwnership(PILFunction *F, bool newValue) {
      F->setHasOwnership(newValue);
   }
};

} // namespace polar

#endif
