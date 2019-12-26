//===--- GenPoly.cpp - Swift IR Generation for Polymorphism ---------------===//
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
//  This file implements IR generation for polymorphic operations in Swift.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/AstVisitor.h"
#include "polarphp/ast/Types.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/GenericEnvironment.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/lang/PILType.h"
#include "llvm/IR/DerivedTypes.h"

#include "polarphp/irgen/internal/Explosion.h"
#include "polarphp/irgen/internal/IRGenFunction.h"
#include "polarphp/irgen/internal/IRGenModule.h"
#include "polarphp/irgen/internal/LoadableTypeInfo.h"
#include "polarphp/irgen/internal/TypeVisitor.h"
#include "polarphp/irgen/internal/GenTuple.h"
#include "polarphp/irgen/internal/GenPoly.h"
#include "polarphp/irgen/internal/GenType.h"

using namespace polar;
using namespace irgen;

static PILType applyPrimaryArchetypes(IRGenFunction &IGF,
                                      PILType type) {
   if (!type.hasTypeParameter()) {
      return type;
   }

   auto substType =
      IGF.IGM.getGenericEnvironment()->mapTypeIntoContext(type.getAstType())
         ->getCanonicalType();
   return PILType::getPrimitiveType(substType, type.getCategory());
}

/// Given a substituted explosion, re-emit it as an unsubstituted one.
///
/// For example, given an explosion which begins with the
/// representation of an (Int, Float), consume that and produce the
/// representation of an (Int, T).
///
/// The substitutions must carry origTy to substTy.
void irgen::reemitAsUnsubstituted(IRGenFunction &IGF,
                                  PILType expectedTy, PILType substTy,
                                  Explosion &in, Explosion &out) {
   expectedTy = applyPrimaryArchetypes(IGF, expectedTy);

   ExplosionSchema expectedSchema;
   cast<LoadableTypeInfo>(IGF.IGM.getTypeInfo(expectedTy))
      .getSchema(expectedSchema);

#ifndef NDEBUG
   auto &substTI = IGF.IGM.getTypeInfo(applyPrimaryArchetypes(IGF, substTy));
   assert(expectedSchema.size() ==
          cast<LoadableTypeInfo>(substTI).getExplosionSize());
#endif

   for (ExplosionSchema::Element &elt : expectedSchema) {
      llvm::Value *value = in.claimNext();
      assert(elt.isScalar());

      // The only type differences we expect here should be due to
      // substitution of class archetypes.
      if (value->getType() != elt.getScalarType()) {
         value = IGF.Builder.CreateBitCast(value, elt.getScalarType(),
                                           value->getName() + ".asUnsubstituted");
      }
      out.add(value);
   }
}
