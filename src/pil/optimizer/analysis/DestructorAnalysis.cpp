//===----------------------------------------------------------------------===//
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

#include "polarphp/pil/optimizer/analysis/DestructorAnalysis.h"
#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/LazyResolver.h"
#include "polarphp/pil/lang/PILModule.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "destructor-analysis"

using namespace polar;

/// A type T's destructor does not store to memory if the type
///  * is a trivial builtin type like builtin float or int types
///  * is a value type with stored properties that are safe or
///  * is a value type that implements the _DestructorSafeContainer protocol and
///    whose type parameters are safe types T1...Tn.
bool DestructorAnalysis::mayStoreToMemoryOnDestruction(PILType T) {
   bool IsSafe = isSafeType(T.getAstType());
   LLVM_DEBUG(llvm::dbgs() << " DestructorAnalysis::"
                              "mayStoreToMemoryOnDestruction is"
                           << (IsSafe ? " false: " : " true: "));
   LLVM_DEBUG(T.getAstType()->print(llvm::errs()));
   LLVM_DEBUG(llvm::errs() << "\n");
   return !IsSafe;
}

bool DestructorAnalysis::cacheResult(CanType Type, bool Result) {
   Cached[Type] = Result;
   return Result;
}

bool DestructorAnalysis::isSafeType(CanType Ty) {
   // Don't visit types twice.
   auto CachedRes = Cached.find(Ty);
   if (CachedRes != Cached.end()) {
      return CachedRes->second;
   }

   // Before we recurse mark the type as safe i.e if we see it in a recursive
   // position it is safe in the absence of another fact that proves otherwise.
   // We will reset this value to the correct value once we return from the
   // recursion below.
   cacheResult(Ty, true);

   // Trivial value types.
   if (Ty->is<BuiltinIntegerType>())
      return cacheResult(Ty, true);
   if (Ty->is<BuiltinFloatType>())
      return cacheResult(Ty, true);

   // A struct is safe if
   //   * either it implements the _DestructorSafeContainer protocol and
   //     all the type parameters are safe types.
   //   * or all stored properties are safe types.
   if (auto *Struct = Ty->getStructOrBoundGenericStruct()) {

      if (implementsDestructorSafeContainerInterface(Struct) &&
          areTypeParametersSafe(Ty))
         return cacheResult(Ty, true);

      // Check the stored properties.
      for (auto SP : Struct->getStoredProperties()) {
         if (!isSafeType(SP->getInterfaceType()->getCanonicalType()))
            return cacheResult(Ty, false);
      }
      return cacheResult(Ty, true);
   }

   // A tuple type is safe if its elements are safe.
   if (auto Tuple = dyn_cast<TupleType>(Ty)) {
      for (auto &Elt : Tuple->getElements())
         if (!isSafeType(Elt.getType()->getCanonicalType()))
            return cacheResult(Ty, false);
      return cacheResult(Ty, true);
   }

   // TODO: enum types.

   return cacheResult(Ty, false);
}

bool DestructorAnalysis::implementsDestructorSafeContainerInterface(
   NominalTypeDecl *NomDecl) {
   InterfaceDecl *DestructorSafeContainer =
      getAstContext().getInterface(KnownInterfaceKind::DestructorSafeContainer);
   for (auto Proto : NomDecl->getAllInterfaces())
      if (Proto == DestructorSafeContainer)
         return true;
   return false;
}

bool DestructorAnalysis::areTypeParametersSafe(CanType Ty) {
   auto BGT = dyn_cast<BoundGenericType>(Ty);
   if (!BGT)
      return false;

   // Make sure all type parameters are safe.
   for (auto TP : BGT->getGenericArgs()) {
      if (!isSafeType(TP->getCanonicalType()))
         return false;
   }
   return true;
}

AstContext &DestructorAnalysis::getAstContext() {
   return Mod->getAstContext();
}

PILAnalysis *polar::createDestructorAnalysis(PILModule *M) {
   return new DestructorAnalysis(M);
}
