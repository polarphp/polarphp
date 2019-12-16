//===--- Bridging.cpp - Bridging imported Clang types to Swift ------------===//
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
// This file defines routines relating to bridging Swift types to C types,
// working in concert with the Clang importer.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "libpil"

#include "polarphp/pil/lang/PILType.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/DiagnosticsPIL.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/ModuleLoader.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "clang/AST/DeclObjC.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"

namespace polar {

using namespace polar::lowering;

CanType TypeConverter::getLoweredTypeOfGlobal(VarDecl *var) {
   AbstractionPattern origType = getAbstractionPattern(var);
   assert(!origType.isTypeParameter());
   return getLoweredRValueType(TypeExpansionContext::minimal(), origType,
                               origType.getType());
}

AnyFunctionType::Param
TypeConverter::getBridgedParam(PILFunctionTypeRepresentation rep,
                               AbstractionPattern pattern,
                               AnyFunctionType::Param param,
                               Bridgeability bridging) {
   assert(!param.getParameterFlags().isInOut() &&
          !param.getParameterFlags().isVariadic());

   auto bridged = getLoweredBridgedType(pattern, param.getPlainType(), bridging,
                                        rep, TypeConverter::ForArgument);
   if (!bridged) {
      Context.Diags.diagnose(SourceLoc(), diag::could_not_find_bridge_type,
                             param.getPlainType());
      llvm::report_fatal_error("unable to set up the ObjC bridge!");
   }

   return AnyFunctionType::Param(bridged->getCanonicalType(),
                                 param.getLabel(),
                                 param.getParameterFlags());
}

void TypeConverter::
getBridgedParams(PILFunctionTypeRepresentation rep,
                 AbstractionPattern pattern,
                 ArrayRef<AnyFunctionType::Param> params,
                 SmallVectorImpl<AnyFunctionType::Param> &bridgedParams,
                 Bridgeability bridging) {
   for (unsigned i : indices(params)) {
      auto paramPattern = pattern.getFunctionParamType(i);
      auto bridgedParam = getBridgedParam(rep, paramPattern, params[i], bridging);
      bridgedParams.push_back(bridgedParam);
   }
}

/// Bridge a result type.
CanType TypeConverter::getBridgedResultType(PILFunctionTypeRepresentation rep,
                                            AbstractionPattern pattern,
                                            CanType result,
                                            Bridgeability bridging,
                                            bool suppressOptional) {
   auto loweredType =
      getLoweredBridgedType(pattern, result, bridging, rep,
                            suppressOptional
                            ? TypeConverter::ForNonOptionalResult
                            : TypeConverter::ForResult);

   if (!loweredType) {
      Context.Diags.diagnose(SourceLoc(), diag::could_not_find_bridge_type,
                             result);

      llvm::report_fatal_error("unable to set up the ObjC bridge!");
   }

   return loweredType->getCanonicalType();
}

Type TypeConverter::getLoweredBridgedType(AbstractionPattern pattern,
                                          Type t,
                                          Bridgeability bridging,
                                          PILFunctionTypeRepresentation rep,
                                          BridgedTypePurpose purpose) {
   switch (rep) {
      case PILFunctionTypeRepresentation::Thick:
      case PILFunctionTypeRepresentation::Thin:
      case PILFunctionTypeRepresentation::Method:
      case PILFunctionTypeRepresentation::WitnessMethod:
      case PILFunctionTypeRepresentation::Closure:
         // No bridging needed for native CCs.
         return t;
      case PILFunctionTypeRepresentation::CFunctionPointer:
      case PILFunctionTypeRepresentation::ObjCMethod:
      case PILFunctionTypeRepresentation::Block:
         // Map native types back to bridged types.

         // Look through optional types.
         if (auto valueTy = t->getOptionalObjectType()) {
            pattern = pattern.getOptionalObjectType();
            auto ty = getLoweredCBridgedType(pattern, valueTy, bridging, rep,
                                             BridgedTypePurpose::ForNonOptionalResult);
            return ty ? OptionalType::get(ty) : ty;
         }
         return getLoweredCBridgedType(pattern, t, bridging, rep, purpose);
   }

   llvm_unreachable("Unhandled PILFunctionTypeRepresentation in switch.");
}

Type TypeConverter::getLoweredCBridgedType(AbstractionPattern pattern,
                                           Type t, Bridgeability bridging,
                                           PILFunctionTypeRepresentation rep,
                                           BridgedTypePurpose purpose) {
   auto clangTy = pattern.isClangType() ? pattern.getClangType() : nullptr;

   // Bridge Bool back to ObjC bool, unless the original Clang type was _Bool
   // or the Darwin Boolean type.
   auto nativeBoolTy = getBoolType();
   if (nativeBoolTy && t->isEqual(nativeBoolTy)) {
      // If we have a Clang type that was imported as Bool, it had better be
      // one of a small set of types.
      if (clangTy) {
         auto builtinTy = clangTy->castAs<clang::BuiltinType>();
         if (builtinTy->getKind() == clang::BuiltinType::Bool)
            return t;
         if (builtinTy->getKind() == clang::BuiltinType::UChar)
            return getDarwinBooleanType();
         if (builtinTy->getKind() == clang::BuiltinType::Int)
            return getWindowsBoolType();
         assert(builtinTy->getKind() == clang::BuiltinType::SChar);
         return getObjCBoolType();
      }

      // Otherwise, always assume ObjC methods should use ObjCBool.
      if (bridging != Bridgeability::None &&
          rep == PILFunctionTypeRepresentation::ObjCMethod)
         return getObjCBoolType();

      return t;
   }

   // Class metatypes bridge to ObjC metatypes.
   if (auto metaTy = t->getAs<MetatypeType>()) {
      if (metaTy->getInstanceType()->getClassOrBoundGenericClass()
          // Self argument of an ObjC protocol
          || metaTy->getInstanceType()->is<GenericTypeParamType>()) {
         return MetatypeType::get(metaTy->getInstanceType(),
                                  MetatypeRepresentation::ObjC);
      }
   }

   // ObjC-compatible existential metatypes.
   if (auto metaTy = t->getAs<ExistentialMetatypeType>()) {
      if (metaTy->getInstanceType()->isObjCExistentialType()) {
         return ExistentialMetatypeType::get(metaTy->getInstanceType(),
                                             MetatypeRepresentation::ObjC);
      }
   }

   // `Any` can bridge to `AnyObject` (`id` in ObjC).
   if (t->isAny())
      return Context.getAnyObjectType();

   if (auto funTy = t->getAs<FunctionType>()) {
      switch (funTy->getExtInfo().getPILRepresentation()) {
         // Functions that are already represented as blocks or C function pointers
         // don't need bridging.
         case PILFunctionType::Representation::Block:
         case PILFunctionType::Representation::CFunctionPointer:
         case PILFunctionType::Representation::Thin:
         case PILFunctionType::Representation::Method:
         case PILFunctionType::Representation::ObjCMethod:
         case PILFunctionType::Representation::WitnessMethod:
         case PILFunctionType::Representation::Closure:
            return t;
         case PILFunctionType::Representation::Thick: {
            // Thick functions (TODO: conditionally) get bridged to blocks.
            // This bridging is more powerful than usual block bridging, however,
            // so we use the ObjCMethod representation.
            SmallVector<AnyFunctionType::Param, 8> newParams;
            getBridgedParams(PILFunctionType::Representation::ObjCMethod,
                             pattern, funTy->getParams(), newParams, bridging);

            Type newResult =
               getBridgedResultType(PILFunctionType::Representation::ObjCMethod,
                                    pattern.getFunctionResultType(),
                                    funTy->getResult()->getCanonicalType(),
                                    bridging,
                  /*non-optional*/false);

            return FunctionType::get(newParams, newResult,
                                     funTy->getExtInfo().withPILRepresentation(
                                        PILFunctionType::Representation::Block));
         }
      }
   }

   auto foreignRepresentation =
      t->getForeignRepresentableIn(ForeignLanguage::ObjectiveC, &M);
   switch (foreignRepresentation.first) {
      case ForeignRepresentableKind::None:
      case ForeignRepresentableKind::Trivial:
      case ForeignRepresentableKind::Object:
         return t;

      case ForeignRepresentableKind::Bridged:
      case ForeignRepresentableKind::StaticBridged: {
         auto conformance = foreignRepresentation.second;
         assert(conformance && "Missing conformance?");
         Type bridgedTy =
            InterfaceConformanceRef(conformance).getTypeWitnessByName(
               t, M.getAstContext().Id_ObjectiveCType);
         if (purpose == BridgedTypePurpose::ForResult && clangTy)
            bridgedTy = OptionalType::get(bridgedTy);
         return bridgedTy;
      }

      case ForeignRepresentableKind::BridgedError: {
      ///@todo
//         auto nsErrorTy = M.getAstContext().getNSErrorType();
//         assert(nsErrorTy && "Cannot bridge when NSError isn't available");
//         return nsErrorTy;
      }
   }

   return t;
}

} // polar