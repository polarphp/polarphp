//===----------------------------------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/ASTPrinter.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/ParameterList.h"
#include "polarphp/basic/SourceMgr.h"
#include "polarphp/sema/IDETypeCheckingRequests.h"
#include "polarphp/global/Subsystems.h"
#include "polarphp/sema/internal/TypeChecker.h"
#include "polarphp/sema/internal/ConstraintGraph.h"
#include "polarphp/sema/internal/ConstraintSystem.h"

using namespace polar;

namespace polar {
// Implement the IDE type zone.
#define POLAR_TYPEID_ZONE IDETypeChecking
#define POLAR_TYPEID_HEADER "polarphp/sema/IDETypeCheckingRequestIDZoneDef.h"
#include "polarphp/basic/ImplementTypeIdZone.h"
#undef POLAR_TYPEID_ZONE
#undef POLAR_TYPEID_HEADER
}

// Define request evaluation functions for each of the IDE type check requests.
static AbstractRequestFunction *ideTypeCheckRequestFunctions[] = {
#define POLAR_REQUEST(Zone, Name, Sig, Caching, LocOptions)                    \
reinterpret_cast<AbstractRequestFunction *>(&Name::evaluateRequest),
#include "polarphp/sema/IDETypeCheckingRequestIDZoneDef.h"
#undef POLAR_REQUEST
};

void polar::registerIDETypeCheckRequestFunctions(Evaluator &evaluator) {
   evaluator.registerRequestFunctions(Zone::IDETypeChecking,
                                      ideTypeCheckRequestFunctions);
}

static bool isExtensionAppliedInternal(const DeclContext *DC, Type BaseTy,
                                       const ExtensionDecl *ED) {
   // We can't do anything if the base type has unbound generic parameters.
   // We can't leak type variables into another constraint system.
   if (BaseTy->hasTypeVariable() || BaseTy->hasUnboundGenericType() ||
       BaseTy->hasUnresolvedType() || BaseTy->hasError())
      return true;

   if (!ED->isConstrainedExtension())
      return true;

   (void)polar::createTypeChecker(DC->getAstContext());
   GenericSignature genericSig = ED->getGenericSignature();
   SubstitutionMap substMap = BaseTy->getContextSubstitutionMap(
      DC->getParentModule(), ED->getExtendedNominal());
   return areGenericRequirementsSatisfied(DC, genericSig, substMap,
      /*isExtension=*/true);
}

static bool isMemberDeclAppliedInternal(const DeclContext *DC, Type BaseTy,
                                        const ValueDecl *VD) {
   // We can't leak type variables into another constraint system.
   // We can't do anything if the base type has unbound generic parameters.
   if (BaseTy->hasTypeVariable() || BaseTy->hasUnboundGenericType()||
       BaseTy->hasUnresolvedType() || BaseTy->hasError())
      return true;

   const GenericContext *genericDecl = VD->getAsGenericContext();
   if (!genericDecl)
      return true;
   GenericSignature genericSig = genericDecl->getGenericSignature();
   if (!genericSig)
      return true;

   SubstitutionMap substMap = BaseTy->getContextSubstitutionMap(
      DC->getParentModule(), VD->getDeclContext());
   return areGenericRequirementsSatisfied(DC, genericSig, substMap,
      /*isExtension=*/false);
}

llvm::Expected<bool>
IsDeclApplicableRequest::evaluate(Evaluator &evaluator,
                                  DeclApplicabilityOwner Owner) const {
   if (auto *VD = dyn_cast<ValueDecl>(Owner.ExtensionOrMember)) {
      return isMemberDeclAppliedInternal(Owner.DC, Owner.Ty, VD);
   } else if (auto *ED = dyn_cast<ExtensionDecl>(Owner.ExtensionOrMember)) {
      return isExtensionAppliedInternal(Owner.DC, Owner.Ty, ED);
   } else {
      llvm_unreachable("unhandled decl kind");
   }
}

llvm::Expected<bool>
TypeRelationCheckRequest::evaluate(Evaluator &evaluator,
                                   TypeRelationCheckInput Owner) const {
   Optional<constraints::ConstraintKind> CKind;
   switch (Owner.Relation) {
      case TypeRelation::ConvertTo:
         CKind = constraints::ConstraintKind::Conversion;
         break;
   }
   assert(CKind.hasValue());
   return TypeChecker::typesSatisfyConstraint(Owner.Pair.FirstTy,
                                              Owner.Pair.SecondTy,
                                              Owner.OpenArchetypes,
                                              *CKind, Owner.DC);
}

llvm::Expected<TypePair>
RootAndResultTypeOfKeypathDynamicMemberRequest::evaluate(Evaluator &evaluator,
                                                         SubscriptDecl *subscript) const {
   if (!isValidKeyPathDynamicMemberLookup(subscript))
      return TypePair();

   const auto *param = subscript->getIndices()->get(0);
   auto keyPathType = param->getType()->getAs<BoundGenericType>();
   if (!keyPathType)
      return TypePair();
   auto genericArgs = keyPathType->getGenericArgs();
   assert(!genericArgs.empty() && genericArgs.size() == 2 &&
          "invalid keypath dynamic member");
   return TypePair(genericArgs[0], genericArgs[1]);
}

llvm::Expected<bool>
HasDynamicMemberLookupAttributeRequest::evaluate(Evaluator &evaluator,
                                                 TypeBase *ty) const {
   llvm::DenseMap<CanType, bool> DynamicMemberLookupCache;
   return hasDynamicMemberLookupAttribute(Type(ty), DynamicMemberLookupCache);
}
