//===--- PILGenType.cpp - PILGen for types and their members --------------===//
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
// This file contains code for emitting code associated with types:
//   - methods
//   - vtables and vtable thunks
//   - witness tables and witness thunks
//
//===----------------------------------------------------------------------===//

#include "polarphp/pil/gen/ManagedValue.h"
#include "polarphp/pil/gen/PILGenFunction.h"
#include "polarphp/pil/gen/PILGenFunctionBuilder.h"
#include "polarphp/pil/gen/Scope.h"
#include "polarphp/ast/AstMangler.h"
#include "polarphp/ast/GenericEnvironment.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/ast/PrettyStackTrace.h"
#include "polarphp/ast/PropertyWrappers.h"
#include "polarphp/ast/SourceFile.h"
#include "polarphp/ast/SubstitutionMap.h"
#include "polarphp/ast/TypeMemberVisitor.h"
#include "polarphp/pil/lang/FormalLinkage.h"
#include "polarphp/pil/lang/PrettyStackTrace.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILVTableVisitor.h"
#include "polarphp/pil/lang/PILWitnessVisitor.h"
#include "polarphp/pil/lang/TypeLowering.h"

using namespace polar;
using namespace lowering;

Optional<PILVTable::Entry>
PILGenModule::emitVTableMethod(ClassDecl *theClass,
                               PILDeclRef derived, PILDeclRef base) {
   assert(base.kind == derived.kind);

   auto *baseDecl = cast<AbstractFunctionDecl>(base.getDecl());
   auto *derivedDecl = cast<AbstractFunctionDecl>(derived.getDecl());

   // Note: We intentionally don't support extension members here.
   //
   // Once extensions can override or introduce new vtable entries, this will
   // all likely change anyway.
   auto *baseClass = cast<ClassDecl>(baseDecl->getDeclContext());
   auto *derivedClass = cast<ClassDecl>(derivedDecl->getDeclContext());

   // Figure out if the vtable entry comes from the superclass, in which
   // case we won't emit it if building a resilient module.
   PILVTable::Entry::Kind implKind;
   if (baseClass == theClass) {
      // This is a vtable entry for a method of the immediate class.
      implKind = PILVTable::Entry::Kind::Normal;
   } else if (derivedClass == theClass) {
      // This is a vtable entry for a method of a base class, but it is being
      // overridden in the immediate class.
      implKind = PILVTable::Entry::Kind::Override;
   } else {
      // This vtable entry is copied from the superclass.
      implKind = PILVTable::Entry::Kind::Inherited;

      // If the override is defined in a class from a different resilience
      // domain, don't emit the vtable entry.
      if (derivedClass->isResilient(M.getTypePHPModule(),
                                    ResilienceExpansion::Maximal)) {
         return None;
      }
   }

   PILFunction *implFn;

   // If the member is dynamic, reference its dynamic dispatch thunk so that
   // it will be redispatched, funneling the method call through the runtime
   // hook point.
   // @todo
//   bool usesObjCDynamicDispatch =
//      (derivedDecl->isObjCDynamic() &&
//       derived.kind != PILDeclRef::Kind::Allocator);
//
//   if (usesObjCDynamicDispatch) {
//      implFn = getDynamicThunk(
//         derived, Types.getConstantInfo(TypeExpansionContext::minimal(), derived)
//            .PILFnType);
//   } else {
   implFn = getFunction(derived, NotForDefinition);
//   }

   // As a fast path, if there is no override, definitely no thunk is necessary.
   if (derived == base)
      return PILVTable::Entry(base, implFn, implKind);

   // If the base method is less visible than the derived method, we need
   // a thunk.
   // @todo
   bool baseLessVisibleThanDerived =
      (/*!usesObjCDynamicDispatch &&*/
       !derivedDecl->isFinal() &&
       derivedDecl->isEffectiveLinkageMoreVisibleThan(baseDecl));

   // Determine the derived thunk type by lowering the derived type against the
   // abstraction pattern of the base.
   auto baseInfo = Types.getConstantInfo(TypeExpansionContext::minimal(), base);
   auto derivedInfo =
      Types.getConstantInfo(TypeExpansionContext::minimal(), derived);
   auto basePattern = AbstractionPattern(baseInfo.LoweredType);

   auto overrideInfo = M.Types.getConstantOverrideInfo(
      TypeExpansionContext::minimal(), derived, base);

   // If base method's generic requirements are not satisfied by the derived
   // method then we need a thunk.
   using Direction = AstContext::OverrideGenericSignatureReqCheck;
   auto doesNotHaveGenericRequirementDifference =
      getAstContext().overrideGenericSignatureReqsSatisfied(
         baseDecl, derivedDecl, Direction::BaseReqSatisfiedByDerived);

   // The override member type is semantically a subtype of the base
   // member type. If the override is ABI compatible, we do not need
   // a thunk.
   bool compatibleCallingConvention;
   switch (M.Types.checkFunctionForABIDifferences(M,
                                                  derivedInfo.PILFnType,
                                                  overrideInfo.PILFnType)) {
      case TypeConverter::ABIDifference::CompatibleCallingConvention:
      case TypeConverter::ABIDifference::CompatibleRepresentation:
         compatibleCallingConvention = true;
         break;
      case TypeConverter::ABIDifference::NeedsThunk:
         compatibleCallingConvention = false;
         break;
      case TypeConverter::ABIDifference::CompatibleCallingConvention_ThinToThick:
      case TypeConverter::ABIDifference::CompatibleRepresentation_ThinToThick:
         llvm_unreachable("shouldn't be thick methods");
   }
   if (doesNotHaveGenericRequirementDifference
       && !baseLessVisibleThanDerived
       && compatibleCallingConvention)
      return PILVTable::Entry(base, implFn, implKind);

   // Generate the thunk name.
   std::string name;
   {
      mangle::AstMangler mangler;
      if (isa<FuncDecl>(baseDecl)) {
         name = mangler.mangleVTableThunk(
            cast<FuncDecl>(baseDecl),
            cast<FuncDecl>(derivedDecl));
      } else {
         name = mangler.mangleConstructorVTableThunk(
            cast<ConstructorDecl>(baseDecl),
            cast<ConstructorDecl>(derivedDecl),
            base.kind == PILDeclRef::Kind::Allocator);
      }
   }

   // If we already emitted this thunk, reuse it.
   if (auto existingThunk = M.lookUpFunction(name))
      return PILVTable::Entry(base, existingThunk, implKind);

   // Emit the thunk.
   PILLocation loc(derivedDecl);
   PILGenFunctionBuilder builder(*this);
   auto thunk = builder.createFunction(
      PILLinkage::Private, name, overrideInfo.PILFnType,
      cast<AbstractFunctionDecl>(derivedDecl)->getGenericEnvironment(), loc,
      IsBare, IsNotTransparent, IsNotSerialized, IsNotDynamic,
      ProfileCounter(), IsThunk);
   thunk->setDebugScope(new (M) PILDebugScope(loc, thunk));

   PrettyStackTracePILFunction trace("generating vtable thunk", thunk);

   PILGenFunction(*this, *thunk, theClass)
      .emitVTableThunk(base, derived, implFn, basePattern,
                       overrideInfo.LoweredType,
                       derivedInfo.LoweredType,
                       baseLessVisibleThanDerived);
   emitLazyConformancesForFunction(thunk);

   return PILVTable::Entry(base, thunk, implKind);
}

// @todo
//bool PILGenModule::requiresObjCMethodEntryPoint(FuncDecl *method) {
//   // Property accessors should be generated alongside the property unless
//   // the @NSManaged attribute is present.
//   if (auto accessor = dyn_cast<AccessorDecl>(method)) {
//      if (accessor->isGetterOrSetter()) {
//         auto asd = accessor->getStorage();
//         return asd->isObjC() && !asd->getAttrs().hasAttribute<NSManagedAttr>();
//      }
//   }
//
//   if (method->getAttrs().hasAttribute<NSManagedAttr>())
//      return false;
//
//   return method->isObjC();
//}

//bool PILGenModule::requiresObjCMethodEntryPoint(ConstructorDecl *constructor) {
//   return constructor->isObjC();
//}

namespace {

/// An ASTVisitor for populating PILVTable entries from ClassDecl members.
class PILGenVTable : public PILVTableVisitor<PILGenVTable> {
public:
   PILGenModule &SGM;
   ClassDecl *theClass;
   bool isResilient;

   // Map a base PILDeclRef to the corresponding element in vtableMethods.
   llvm::DenseMap<PILDeclRef, unsigned> baseToIndexMap;

   // For each base method, store the corresponding override.
   SmallVector<std::pair<PILDeclRef, PILDeclRef>, 8> vtableMethods;

   PILGenVTable(PILGenModule &SGM, ClassDecl *theClass)
      : SGM(SGM), theClass(theClass) {
      isResilient = theClass->isResilient();
   }

   void emitVTable() {
      // Imported types don't have vtables right now.
      if (theClass->hasClangNode())
         return;

      // Populate our list of base methods and overrides.
      visitAncestor(theClass);

      SmallVector<PILVTable::Entry, 8> vtableEntries;
      vtableEntries.reserve(vtableMethods.size() + 2);

      // For each base method/override pair, emit a vtable thunk or direct
      // reference to the method implementation.
      for (auto method : vtableMethods) {
         PILDeclRef baseRef, derivedRef;
         std::tie(baseRef, derivedRef) = method;

         auto entry = SGM.emitVTableMethod(theClass, derivedRef, baseRef);

         // We might skip emitting entries if the base class is resilient.
         if (entry)
            vtableEntries.push_back(*entry);
      }

      // Add the deallocating destructor to the vtable just for the purpose
      // that it is referenced and cannot be eliminated by dead function removal.
      // In reality, the deallocating destructor is referenced directly from
      // the HeapMetadata for the class.
      {
         auto *dtor = theClass->getDestructor();
         PILDeclRef dtorRef(dtor, PILDeclRef::Kind::Deallocator);
         auto *dtorFn = SGM.getFunction(dtorRef, NotForDefinition);
         vtableEntries.emplace_back(dtorRef, dtorFn,
                                    PILVTable::Entry::Kind::Normal);
      }

      if (SGM.requiresIVarDestroyer(theClass)) {
         PILDeclRef dtorRef(theClass, PILDeclRef::Kind::IVarDestroyer);
         auto *dtorFn = SGM.getFunction(dtorRef, NotForDefinition);
         vtableEntries.emplace_back(dtorRef, dtorFn,
                                    PILVTable::Entry::Kind::Normal);
      }

      IsSerialized_t serialized = IsNotSerialized;
      auto classIsPublic = theClass->getEffectiveAccess() >= AccessLevel::Public;
      // Only public, fixed-layout classes should have serialized vtables.
      if (classIsPublic && !theClass->isResilient())
         serialized = IsSerialized;

      // Finally, create the vtable.
      PILVTable::create(SGM.M, theClass, serialized, vtableEntries);
   }

   void visitAncestor(ClassDecl *ancestor) {
      auto superTy = ancestor->getSuperclass();
      if (superTy)
         visitAncestor(superTy->getClassOrBoundGenericClass());

      addVTableEntries(ancestor);
   }

   // Try to find an overridden entry.
   void addMethodOverride(PILDeclRef baseRef, PILDeclRef declRef) {
      auto found = baseToIndexMap.find(baseRef);
      assert(found != baseToIndexMap.end());
      auto &method = vtableMethods[found->second];
      assert(method.first == baseRef);
      method.second = declRef;
   }

   // Add an entry to the vtable.
   void addMethod(PILDeclRef member) {
      unsigned index = vtableMethods.size();
      vtableMethods.push_back(std::make_pair(member, member));
      auto result = baseToIndexMap.insert(std::make_pair(member, index));
      assert(result.second);
      (void) result;
   }

   void addPlaceholder(MissingMemberDecl *m) {
      assert(m->getNumberOfVTableEntries() == 0
             && "Should not be emitting class with missing members");
   }
};

} // end anonymous namespace

static void emitTypeMemberGlobalVariable(PILGenModule &SGM,
                                         VarDecl *var) {
   if (var->getDeclContext()->isGenericContext()) {
      assert(var->getDeclContext()->getGenericSignatureOfContext()
                ->areAllParamsConcrete()
             && "generic static vars are not implemented yet");
   }

   if (var->getDeclContext()->getSelfClassDecl()) {
      assert(var->isFinal() && "only 'static' ('class final') stored properties are implemented in classes");
   }

   SGM.addGlobalVariable(var);
}

namespace {

// Is this a free function witness satisfying a static method requirement?
static IsFreeFunctionWitness_t isFreeFunctionWitness(ValueDecl *requirement,
                                                     ValueDecl *witness) {
   if (!witness->getDeclContext()->isTypeContext()) {
      assert(!requirement->isInstanceMember()
             && "free function satisfying instance method requirement?!");
      return IsFreeFunctionWitness;
   }

   return IsNotFreeFunctionWitness;
}

/// A CRTP class for emitting witness thunks for the requirements of a
/// interface.
///
/// There are two subclasses:
///
/// - PILGenConformance: emits witness thunks for a conformance of a
///   a concrete type to a interface
/// - PILGenDefaultWitnessTable: emits default witness thunks for
///   default implementations of interface requirements
///
template<typename T> class PILGenWitnessTable : public PILWitnessVisitor<T> {
   T &asDerived() { return *static_cast<T*>(this); }

public:
   void addMethod(PILDeclRef requirementRef) {
      auto reqAccessor = dyn_cast<AccessorDecl>(requirementRef.getDecl());

      // If it's not an accessor, just look for the witness.
      if (!reqAccessor) {
         if (auto witness = asDerived().getWitness(requirementRef.getDecl())) {
            return addMethodImplementation(requirementRef,
                                           PILDeclRef(witness.getDecl(),
                                                      requirementRef.kind),
                                           witness);
         }

         return asDerived().addMissingMethod(requirementRef);
      }

      // Otherwise, we need to map the storage declaration and then get
      // the appropriate accessor for it.
      auto witness = asDerived().getWitness(reqAccessor->getStorage());
      if (!witness)
         return asDerived().addMissingMethod(requirementRef);

      auto witnessStorage = cast<AbstractStorageDecl>(witness.getDecl());
      if (reqAccessor->isSetter() && !witnessStorage->supportsMutation())
         return asDerived().addMissingMethod(requirementRef);

      auto witnessAccessor =
         witnessStorage->getSynthesizedAccessor(reqAccessor->getAccessorKind());

      return addMethodImplementation(requirementRef,
                                     PILDeclRef(witnessAccessor,
                                                PILDeclRef::Kind::Func),
                                     witness);
   }

private:
   void addMethodImplementation(PILDeclRef requirementRef,
                                PILDeclRef witnessRef,
                                Witness witness) {
      // Free function witnesses have an implicit uncurry layer imposed on them by
      // the inserted metatype argument.
      auto isFree =
         isFreeFunctionWitness(requirementRef.getDecl(), witnessRef.getDecl());
      asDerived().addMethodImplementation(requirementRef, witnessRef,
                                          isFree, witness);
   }
};

static IsSerialized_t isConformanceSerialized(RootInterfaceConformance *conf) {
   return PILWitnessTable::conformanceIsSerialized(conf)
          ? IsSerialized : IsNotSerialized;
}

/// Emit a witness table for a interface conformance.
class PILGenConformance : public PILGenWitnessTable<PILGenConformance> {
   using super = PILGenWitnessTable<PILGenConformance>;

public:
   PILGenModule &SGM;
   NormalInterfaceConformance *Conformance;
   std::vector<PILWitnessTable::Entry> Entries;
   std::vector<PILWitnessTable::ConditionalConformance> ConditionalConformances;
   PILLinkage Linkage;
   IsSerialized_t Serialized;

   PILGenConformance(PILGenModule &SGM, NormalInterfaceConformance *C)
      : SGM(SGM), Conformance(C),
        Linkage(getLinkageForInterfaceConformance(Conformance,
                                                  ForDefinition)),
        Serialized(isConformanceSerialized(Conformance))
   {
      auto *proto = Conformance->getInterface();

      // Not all interfaces use witness tables; in this case we just skip
      // all of emit() below completely.
      if (!lowering::TypeConverter::interfaceRequiresWitnessTable(proto))
         Conformance = nullptr;
   }

   PILWitnessTable *emit() {
      // Nothing to do if this wasn't a normal conformance.
      if (!Conformance)
         return nullptr;

      PrettyStackTraceConformance trace(SGM.getAstContext(),
                                        "generating PIL witness table",
                                        Conformance);

      auto *proto = Conformance->getInterface();
      visitInterfaceDecl(proto);

      addConditionalRequirements();

      // Check if we already have a declaration or definition for this witness
      // table.
      if (auto *wt = SGM.M.lookUpWitnessTable(Conformance, false)) {
         // If we have a definition already, just return it.
         //
         // FIXME: I am not sure if this is possible, if it is not change this to an
         // assert.
         if (wt->isDefinition())
            return wt;

         // If we have a declaration, convert the witness table to a definition.
         if (wt->isDeclaration()) {
            wt->convertToDefinition(Entries, ConditionalConformances, Serialized);

            // Since we had a declaration before, its linkage should be external,
            // ensure that we have a compatible linkage for sanity. *NOTE* we are ok
            // with both being shared since we do not have a shared_external
            // linkage.
            assert(stripExternalFromLinkage(wt->getLinkage()) == Linkage &&
                   "Witness table declaration has inconsistent linkage with"
                   " silgen definition.");

            // And then override the linkage with the new linkage.
            wt->setLinkage(Linkage);
            return wt;
         }
      }

      // Otherwise if we have no witness table yet, create it.
      return PILWitnessTable::create(SGM.M, Linkage, Serialized, Conformance,
                                     Entries, ConditionalConformances);
   }

   void addInterfaceConformanceDescriptor() {
   }


   void addOutOfLineBaseInterface(InterfaceDecl *baseInterface) {
      assert(lowering::TypeConverter::interfaceRequiresWitnessTable(baseInterface));

      auto conformance = Conformance->getInheritedConformance(baseInterface);

      Entries.push_back(PILWitnessTable::BaseInterfaceWitness{
         baseInterface,
         conformance,
      });

      // Emit the witness table for the base conformance if it is shared.
      SGM.useConformance(InterfaceConformanceRef(conformance));
   }

   Witness getWitness(ValueDecl *decl) {
      return Conformance->getWitness(decl);
   }

   void addPlaceholder(MissingMemberDecl *placeholder) {
      llvm_unreachable("generating a witness table with placeholders in it");
   }

   void addMissingMethod(PILDeclRef requirement) {
      llvm_unreachable("generating a witness table with placeholders in it");
   }

   void addMethodImplementation(PILDeclRef requirementRef,
                                PILDeclRef witnessRef,
                                IsFreeFunctionWitness_t isFree,
                                Witness witness) {
      // Emit the witness thunk and add it to the table.
      auto witnessLinkage = witnessRef.getLinkage(ForDefinition);
      auto witnessSerialized = Serialized;
      if (witnessSerialized &&
          fixmeWitnessHasLinkageThatNeedsToBePublic(witnessLinkage)) {
         witnessLinkage = PILLinkage::Public;
         witnessSerialized = IsNotSerialized;
      } else {
         // This is the "real" rule; the above case should go away once we
         // figure out what's going on.

         // Normally witness thunks can be private.
         witnessLinkage = PILLinkage::Private;

         // Unless the witness table is going to be serialized.
         if (witnessSerialized)
            witnessLinkage = PILLinkage::Shared;

         // Or even if its not serialized, it might be for an imported
         // conformance in which case it can be emitted multiple times.
         if (Linkage == PILLinkage::Shared)
            witnessLinkage = PILLinkage::Shared;
      }

      PILFunction *witnessFn = SGM.emitInterfaceWitness(
         InterfaceConformanceRef(Conformance), witnessLinkage, witnessSerialized,
         requirementRef, witnessRef, isFree, witness);
      Entries.push_back(
         PILWitnessTable::MethodWitness{requirementRef, witnessFn});
   }

   void addAssociatedType(AssociatedType requirement) {
      // Find the substitution info for the witness type.
      auto td = requirement.getAssociation();
      Type witness = Conformance->getTypeWitness(td);

      // Emit the record for the type itself.
      Entries.push_back(PILWitnessTable::AssociatedTypeWitness{td,
                                                               witness->getCanonicalType()});
   }

   void addAssociatedConformance(AssociatedConformance req) {
      auto assocConformance =
         Conformance->getAssociatedConformance(req.getAssociation(),
                                               req.getAssociatedRequirement());

      SGM.useConformance(assocConformance);

      Entries.push_back(PILWitnessTable::AssociatedTypeInterfaceWitness{
         req.getAssociation(), req.getAssociatedRequirement(),
         assocConformance});
   }

   void addConditionalRequirements() {
      PILWitnessTable::enumerateWitnessTableConditionalConformances(
         Conformance, [&](unsigned, CanType type, InterfaceDecl *interface) {
            auto conformance =
               Conformance->getGenericSignature()->lookupConformance(type,
                                                                     interface);
            assert(conformance &&
                      "unable to find conformance that should be known");

            ConditionalConformances.push_back(
               PILWitnessTable::ConditionalConformance{type, conformance});

            return /*finished?*/ false;
         });
   }
};

} // end anonymous namespace

PILWitnessTable *
PILGenModule::getWitnessTable(NormalInterfaceConformance *conformance) {
   // If we've already emitted this witness table, return it.
   auto found = emittedWitnessTables.find(conformance);
   if (found != emittedWitnessTables.end())
      return found->second;

   PILWitnessTable *table = PILGenConformance(*this, conformance).emit();
   emittedWitnessTables.insert({conformance, table});

   return table;
}

PILFunction *PILGenModule::emitInterfaceWitness(
   InterfaceConformanceRef conformance, PILLinkage linkage,
   IsSerialized_t isSerialized, PILDeclRef requirement, PILDeclRef witnessRef,
   IsFreeFunctionWitness_t isFree, Witness witness) {
   auto requirementInfo =
      Types.getConstantInfo(TypeExpansionContext::minimal(), requirement);

   // Work out the lowered function type of the PIL witness thunk.
   auto reqtOrigTy = cast<GenericFunctionType>(requirementInfo.LoweredType);

   // Mapping from the requirement's generic signature to the witness
   // thunk's generic signature.
   auto reqtSubMap = witness.getRequirementToSyntheticSubs();

   // The generic environment for the witness thunk.
   auto *genericEnv = witness.getSyntheticEnvironment();
   CanGenericSignature genericSig;
   if (genericEnv)
      genericSig = genericEnv->getGenericSignature()->getCanonicalSignature();

   // The type of the witness thunk.
   auto reqtSubstTy = cast<AnyFunctionType>(
      reqtOrigTy->substGenericArgs(reqtSubMap)
         ->getCanonicalType(genericSig));

   // Generic signatures where all parameters are concrete are lowered away
   // at the PILFunctionType level.
   if (genericSig && genericSig->areAllParamsConcrete()) {
      genericSig = nullptr;
      genericEnv = nullptr;
   }

   // Rewrite the conformance in terms of the requirement environment's Self
   // type, which might have a different generic signature than the type
   // itself.
   //
   // For example, if the conforming type is a class and the witness is defined
   // in a interface extension, the generic signature will have an additional
   // generic parameter representing Self, so the generic parameters of the
   // class will all be shifted down by one.
   if (reqtSubMap) {
      auto requirement = conformance.getRequirement();
      auto self = requirement->getSelfInterfaceType()->getCanonicalType();

      conformance = reqtSubMap.lookupConformance(self, requirement);
   }

   reqtSubstTy =
      CanAnyFunctionType::get(genericSig,
                              reqtSubstTy->getParams(),
                              reqtSubstTy.getResult(),
                              reqtOrigTy->getExtInfo());

   // Coroutine lowering requires us to provide these substitutions
   // in order to recreate the appropriate yield types for the accessor
   // because they aren't reflected in the accessor's AST type.
   // But this is expensive, so we only do it for coroutine lowering.
   // When they're part of the AST function type, we can remove this
   // parameter completely.
   Optional<SubstitutionMap> witnessSubsForTypeLowering;
   if (auto accessor = dyn_cast<AccessorDecl>(requirement.getDecl())) {
      if (accessor->isCoroutine()) {
         witnessSubsForTypeLowering =
            witness.getSubstitutions().mapReplacementTypesOutOfContext();
      }
   }

   // Lower the witness thunk type with the requirement's abstraction level.
   auto witnessPILFnType = getNativePILFunctionType(
      M.Types, TypeExpansionContext::minimal(), AbstractionPattern(reqtOrigTy),
      reqtSubstTy, requirement, witnessRef, witnessSubsForTypeLowering,
      conformance);

   // mangle the name of the witness thunk.
   mangle::AstMangler Newmangler;
   auto manglingConformance =
      conformance.isConcrete() ? conformance.getConcrete() : nullptr;
   std::string nameBuffer =
      Newmangler.mangleWitnessThunk(manglingConformance, requirement.getDecl());

   // If the thunked-to function is set to be always inlined, do the
   // same with the witness, on the theory that the user wants all
   // calls removed if possible, e.g. when we're able to devirtualize
   // the witness method call. Otherwise, use the default inlining
   // setting on the theory that forcing inlining off should only
   // effect the user's function, not otherwise invisible thunks.
   Inline_t InlineStrategy = InlineDefault;
   if (witnessRef.isAlwaysInline())
      InlineStrategy = AlwaysInline;

   PILGenFunctionBuilder builder(*this);
   auto *f = builder.createFunction(
      linkage, nameBuffer, witnessPILFnType, genericEnv,
      PILLocation(witnessRef.getDecl()), IsNotBare, IsTransparent, isSerialized,
      IsNotDynamic, ProfileCounter(), IsThunk, SubclassScope::NotApplicable,
      InlineStrategy);

   f->setDebugScope(new (M)
                       PILDebugScope(RegularLocation(witnessRef.getDecl()), f));

   PrettyStackTracePILFunction trace("generating interface witness thunk", f);

   // Create the witness.
   PILGenFunction SGF(*this, *f, PolarphpModule);

   // Substitutions mapping the generic parameters of the witness to
   // archetypes of the witness thunk generic environment.
   auto witnessSubs = witness.getSubstitutions();

   SGF.emitInterfaceWitness(AbstractionPattern(reqtOrigTy), reqtSubstTy,
                            requirement, reqtSubMap, witnessRef,
                            witnessSubs, isFree, /*isSelfConformance*/ false);

   emitLazyConformancesForFunction(f);
   return f;
}

namespace {

static PILFunction *emitSelfConformanceWitness(PILGenModule &SGM,
                                               SelfInterfaceConformance *conformance,
                                               PILLinkage linkage,
                                               PILDeclRef requirement) {
   auto requirementInfo =
      SGM.Types.getConstantInfo(TypeExpansionContext::minimal(), requirement);

   // Work out the lowered function type of the PIL witness thunk.
   auto reqtOrigTy = cast<GenericFunctionType>(requirementInfo.LoweredType);

   // The transformations we do here don't work for generic requirements.
   GenericEnvironment *genericEnv = nullptr;

   // A mapping from the requirement's generic signature to the type parameters
   // of the witness thunk (which is non-generic).
   auto interface = conformance->getInterface();
   auto interfaceType = interface->getDeclaredInterfaceType();
   auto reqtSubs = SubstitutionMap::getInterfaceSubstitutions(interface,
                                                              interfaceType,
                                                              InterfaceConformanceRef(interface));

   // Open the interface type.
   auto openedType = OpenedArchetypeType::get(interfaceType);

   // Form the substitutions for calling the witness.
   auto witnessSubs = SubstitutionMap::getInterfaceSubstitutions(interface,
                                                                 openedType,
                                                                 InterfaceConformanceRef(interface));

   // Substitute to get the formal substituted type of the thunk.
   auto reqtSubstTy =
      cast<AnyFunctionType>(reqtOrigTy.subst(reqtSubs)->getCanonicalType());

   // Substitute into the requirement type to get the type of the thunk.
   auto witnessPILFnType = requirementInfo.PILFnType->substGenericArgs(
      SGM.M, reqtSubs, TypeExpansionContext::minimal());

   // mangle the name of the witness thunk.
   std::string name = [&] {
      mangle::AstMangler mangler;
      return mangler.mangleWitnessThunk(conformance, requirement.getDecl());
   }();

   PILGenFunctionBuilder builder(SGM);
   auto *f = builder.createFunction(
      linkage, name, witnessPILFnType, genericEnv,
      PILLocation(requirement.getDecl()), IsNotBare, IsTransparent,
      IsSerialized, IsNotDynamic, ProfileCounter(), IsThunk,
      SubclassScope::NotApplicable, InlineDefault);

   f->setDebugScope(new (SGM.M)
                       PILDebugScope(RegularLocation(requirement.getDecl()), f));

   PrettyStackTracePILFunction trace("generating interface witness thunk", f);

   // Create the witness.
   PILGenFunction SGF(SGM, *f, SGM.PolarphpModule);

   auto isFree = isFreeFunctionWitness(requirement.getDecl(),
                                       requirement.getDecl());

   SGF.emitInterfaceWitness(AbstractionPattern(reqtOrigTy), reqtSubstTy,
                            requirement, reqtSubs, requirement,
                            witnessSubs, isFree, /*isSelfConformance*/ true);

   SGM.emitLazyConformancesForFunction(f);

   return f;
}

/// Emit a witness table for a self-conformance.
class PILGenSelfConformanceWitnessTable
   : public PILWitnessVisitor<PILGenSelfConformanceWitnessTable> {
   using super = PILWitnessVisitor<PILGenSelfConformanceWitnessTable>;

   PILGenModule &SGM;
   SelfInterfaceConformance *conformance;
   PILLinkage linkage;
   IsSerialized_t serialized;

   SmallVector<PILWitnessTable::Entry, 8> entries;
public:
   PILGenSelfConformanceWitnessTable(PILGenModule &SGM,
                                     SelfInterfaceConformance *conformance)
      : SGM(SGM), conformance(conformance),
        linkage(getLinkageForInterfaceConformance(conformance, ForDefinition)),
        serialized(isConformanceSerialized(conformance)) {
   }

   void emit() {
      PrettyStackTraceConformance trace(SGM.getAstContext(),
                                        "generating PIL witness table",
                                        conformance);

      // Add entries for all the requirements.
      visitInterfaceDecl(conformance->getInterface());

      // Create the witness table.
      (void) PILWitnessTable::create(SGM.M, linkage, serialized, conformance,
                                     entries, /*conditional*/ {});
   }

   void addInterfaceConformanceDescriptor() {}

   void addOutOfLineBaseInterface(InterfaceDecl *interface) {
      // This is an unnecessary restriction that's just not necessary for Error.
      llvm_unreachable("base interfaces not supported in self-conformance");
   }

   // These are real semantic restrictions.
   void addAssociatedConformance(AssociatedConformance conformance) {
      llvm_unreachable("associated conformances not supported in self-conformance");
   }
   void addAssociatedType(AssociatedType type) {
      llvm_unreachable("associated types not supported in self-conformance");
   }
   void addPlaceholder(MissingMemberDecl *placeholder) {
      llvm_unreachable("placeholders not supported in self-conformance");
   }

   void addMethod(PILDeclRef requirement) {
      auto witness = emitSelfConformanceWitness(SGM, conformance, linkage,
                                                requirement);
      entries.push_back(PILWitnessTable::MethodWitness{requirement, witness});
   }
};
}

void PILGenModule::emitSelfConformanceWitnessTable(InterfaceDecl *interface) {
   auto conformance = getAstContext().getSelfConformance(interface);
   PILGenSelfConformanceWitnessTable(*this, conformance).emit();
}

namespace {

/// Emit a default witness table for a resilient interface definition.
class PILGenDefaultWitnessTable
   : public PILGenWitnessTable<PILGenDefaultWitnessTable> {
   using super = PILGenWitnessTable<PILGenDefaultWitnessTable>;

public:
   PILGenModule &SGM;
   InterfaceDecl *Proto;
   PILLinkage Linkage;

   SmallVector<PILDefaultWitnessTable::Entry, 8> DefaultWitnesses;

   PILGenDefaultWitnessTable(PILGenModule &SGM, InterfaceDecl *proto,
                             PILLinkage linkage)
      : SGM(SGM), Proto(proto), Linkage(linkage) { }

   void addMissingDefault() {
      DefaultWitnesses.push_back(PILDefaultWitnessTable::Entry());
   }

   void addInterfaceConformanceDescriptor() { }

   void addOutOfLineBaseInterface(InterfaceDecl *baseProto) {
      addMissingDefault();
   }

   void addMissingMethod(PILDeclRef ref) {
      addMissingDefault();
   }

   void addPlaceholder(MissingMemberDecl *placeholder) {
      llvm_unreachable("generating a witness table with placeholders in it");
   }

   Witness getWitness(ValueDecl *decl) {
      return Proto->getDefaultWitness(decl);
   }

   void addMethodImplementation(PILDeclRef requirementRef,
                                PILDeclRef witnessRef,
                                IsFreeFunctionWitness_t isFree,
                                Witness witness) {
      PILFunction *witnessFn = SGM.emitInterfaceWitness(
         InterfaceConformanceRef(Proto), PILLinkage::Private, IsNotSerialized,
         requirementRef, witnessRef, isFree, witness);
      auto entry = PILWitnessTable::MethodWitness{requirementRef, witnessFn};
      DefaultWitnesses.push_back(entry);
   }

   void addAssociatedType(AssociatedType req) {
      Type witness = Proto->getDefaultTypeWitness(req.getAssociation());
      if (!witness)
         return addMissingDefault();

      Type witnessInContext = Proto->mapTypeIntoContext(witness);
      auto entry = PILWitnessTable::AssociatedTypeWitness{
         req.getAssociation(),
         witnessInContext->getCanonicalType()};
      DefaultWitnesses.push_back(entry);
   }

   void addAssociatedConformance(const AssociatedConformance &req) {
      auto witness =
         Proto->getDefaultAssociatedConformanceWitness(
            req.getAssociation(),
            req.getAssociatedRequirement());
      if (witness.isInvalid())
         return addMissingDefault();

      auto entry = PILWitnessTable::AssociatedTypeInterfaceWitness{
         req.getAssociation(), req.getAssociatedRequirement(), witness};
      DefaultWitnesses.push_back(entry);
   }
};

} // end anonymous namespace

void PILGenModule::emitDefaultWitnessTable(InterfaceDecl *interface) {
   PILLinkage linkage =
      getPILLinkage(getDeclLinkage(interface), ForDefinition);

   PILGenDefaultWitnessTable builder(*this, interface, linkage);
   builder.visitInterfaceDecl(interface);

   PILDefaultWitnessTable *defaultWitnesses =
      M.createDefaultWitnessTableDeclaration(interface, linkage);
   defaultWitnesses->convertToDefinition(builder.DefaultWitnesses);
}

namespace {

/// An ASTVisitor for generating PIL from method declarations
/// inside nominal types.
class PILGenType : public TypeMemberVisitor<PILGenType> {
public:
   PILGenModule &SGM;
   NominalTypeDecl *theType;

   PILGenType(PILGenModule &SGM, NominalTypeDecl *theType)
      : SGM(SGM), theType(theType) {}

   /// Emit PIL functions for all the members of the type.
   void emitType() {
      SGM.emitLazyConformancesForType(theType);

      // Build a vtable if this is a class.
      if (auto theClass = dyn_cast<ClassDecl>(theType)) {
         for (Decl *member : theClass->getEmittedMembers())
            visit(member);

         PILGenVTable genVTable(SGM, theClass);
         genVTable.emitVTable();
      } else {
         for (Decl *member : theType->getMembers())
            visit(member);
      }
      // @todo
      // Build a default witness table if this is a interface that needs one.
      if (auto interface = dyn_cast<InterfaceDecl>(theType)) {
         if (/*!interface->isObjC() && */ interface->isResilient()) {
            auto *SF = interface->getParentSourceFile();
            if (!SF || SF->Kind != SourceFileKind::Interface)
               SGM.emitDefaultWitnessTable(interface);
         }
         if (interface->requiresSelfConformanceWitnessTable()) {
            SGM.emitSelfConformanceWitnessTable(interface);
         }
         return;
      }

      // Emit witness tables for conformances of concrete types. Interface types
      // are existential and do not have witness tables.
      for (auto *conformance : theType->getLocalConformances(
         ConformanceLookupKind::NonInherited, nullptr)) {
         if (conformance->isComplete()) {
            if (auto *normal = dyn_cast<NormalInterfaceConformance>(conformance))
               SGM.getWitnessTable(normal);
         }
      }
   }

   //===--------------------------------------------------------------------===//
   // Visitors for subdeclarations
   //===--------------------------------------------------------------------===//
   void visitTypeAliasDecl(TypeAliasDecl *tad) {}
   void visitOpaqueTypeDecl(OpaqueTypeDecl *otd) {}
   void visitAbstractTypeParamDecl(AbstractTypeParamDecl *tpd) {}
   void visitModuleDecl(ModuleDecl *md) {}
   void visitMissingMemberDecl(MissingMemberDecl *) {}
   void visitNominalTypeDecl(NominalTypeDecl *ntd) {
      PILGenType(SGM, ntd).emitType();
   }
   void visitFuncDecl(FuncDecl *fd) {
      SGM.emitFunction(fd);
      // FIXME: Default implementations in interfaces.
      // @todo
//      if (SGM.requiresObjCMethodEntryPoint(fd) &&
//          !isa<InterfaceDecl>(fd->getDeclContext()))
//         SGM.emitObjCMethodThunk(fd);
   }
   void visitConstructorDecl(ConstructorDecl *cd) {
      SGM.emitConstructor(cd);
      // @todo
//      if (SGM.requiresObjCMethodEntryPoint(cd) &&
//          !isa<InterfaceDecl>(cd->getDeclContext()))
//         SGM.emitObjCConstructorThunk(cd);
   }
   void visitDestructorDecl(DestructorDecl *dd) {
      assert(isa<ClassDecl>(theType) && "destructor in a non-class type");
      SGM.emitDestructor(cast<ClassDecl>(theType), dd);
   }

   void visitEnumCaseDecl(EnumCaseDecl *ecd) {}
   void visitEnumElementDecl(EnumElementDecl *EED) {
      if (!EED->hasAssociatedValues())
         return;

      // Emit any default argument generators.
      SGM.emitDefaultArgGenerators(EED, EED->getParameterList());
   }

   void visitPatternBindingDecl(PatternBindingDecl *pd) {
      // Emit initializers.
      for (auto i : range(pd->getNumPatternEntries())) {
         if (pd->getExecutableInit(i)) {
            if (pd->isStatic())
               SGM.emitGlobalInitialization(pd, i);
            else
               SGM.emitStoredPropertyInitialization(pd, i);
         }
      }
   }

   void visitVarDecl(VarDecl *vd) {
      // Collect global variables for static properties.
      // FIXME: We can't statically emit a global variable for generic properties.
      if (vd->isStatic() && vd->hasStorage()) {
         emitTypeMemberGlobalVariable(SGM, vd);
         visitAccessors(vd);
         return;
      }

      // If this variable has an attached property wrapper with an initialization
      // function, emit the backing initializer function.
      if (auto wrapperInfo = vd->getPropertyWrapperBackingPropertyInfo()) {
         if (wrapperInfo.initializeFromOriginal && !vd->isStatic()) {
            SGM.emitPropertyWrapperBackingInitializer(vd);
         }
      }

      visitAbstractStorageDecl(vd);
   }

   void visitSubscriptDecl(SubscriptDecl *sd) {
      SGM.emitDefaultArgGenerators(sd, sd->getIndices());
      visitAbstractStorageDecl(sd);
   }

   void visitAbstractStorageDecl(AbstractStorageDecl *asd) {
      // @todo
      // FIXME: Default implementations in interfaces.
//      if (asd->isObjC() && !isa<InterfaceDecl>(asd->getDeclContext()))
//         SGM.emitObjCPropertyMethodThunks(asd);

      SGM.tryEmitPropertyDescriptor(asd);
      visitAccessors(asd);
   }

   void visitAccessors(AbstractStorageDecl *asd) {
      asd->visitEmittedAccessors([&](AccessorDecl *accessor) {
         visitFuncDecl(accessor);
      });
   }
};

} // end anonymous namespace

void PILGenModule::visitNominalTypeDecl(NominalTypeDecl *ntd) {
   PILGenType(*this, ntd).emitType();
}

/// PILGenExtension - an ASTVisitor for generating PIL from method declarations
/// and interface conformances inside type extensions.
class PILGenExtension : public TypeMemberVisitor<PILGenExtension> {
public:
   PILGenModule &SGM;

   PILGenExtension(PILGenModule &SGM)
      : SGM(SGM) {}

   /// Emit PIL functions for all the members of the extension.
   void emitExtension(ExtensionDecl *e) {
      for (Decl *member : e->getMembers())
         visit(member);

      if (!isa<InterfaceDecl>(e->getExtendedNominal())) {
         // Emit witness tables for interface conformances introduced by the
         // extension.
         for (auto *conformance : e->getLocalConformances(
            ConformanceLookupKind::All,
            nullptr)) {
            if (conformance->isComplete()) {
               if (auto *normal =dyn_cast<NormalInterfaceConformance>(conformance))
                  SGM.getWitnessTable(normal);
            }
         }
      }
   }

   //===--------------------------------------------------------------------===//
   // Visitors for subdeclarations
   //===--------------------------------------------------------------------===//
   void visitTypeAliasDecl(TypeAliasDecl *tad) {}
   void visitOpaqueTypeDecl(OpaqueTypeDecl *tad) {}
   void visitAbstractTypeParamDecl(AbstractTypeParamDecl *tpd) {}
   void visitModuleDecl(ModuleDecl *md) {}
   void visitMissingMemberDecl(MissingMemberDecl *) {}
   void visitNominalTypeDecl(NominalTypeDecl *ntd) {
      PILGenType(SGM, ntd).emitType();
   }
   void visitFuncDecl(FuncDecl *fd) {
      // Don't emit other accessors for a dynamic replacement of didSet inside of
      // an extension. We only allow such a construct to allow definition of a
      // didSet/willSet dynamic replacement. Emitting other accessors is
      // problematic because there is no storage.
      //
      // extension SomeStruct {
      //   @_dynamicReplacement(for: someProperty)
      //   var replacement : Int {
      //     didSet {
      //     }
      //   }
      // }
      if (auto *accessor = dyn_cast<AccessorDecl>(fd)) {
         auto *storage = accessor->getStorage();
         bool hasDidSetOrWillSetDynamicReplacement =
            storage->hasDidSetOrWillSetDynamicReplacement();

         if (hasDidSetOrWillSetDynamicReplacement &&
             isa<ExtensionDecl>(storage->getDeclContext()) &&
             fd != storage->getParsedAccessor(AccessorKind::WillSet) &&
             fd != storage->getParsedAccessor(AccessorKind::DidSet))
            return;
      }
      SGM.emitFunction(fd);
      // @todo
//      if (SGM.requiresObjCMethodEntryPoint(fd))
//         SGM.emitObjCMethodThunk(fd);
   }
   void visitConstructorDecl(ConstructorDecl *cd) {
      SGM.emitConstructor(cd);
      // @todo
//      if (SGM.requiresObjCMethodEntryPoint(cd))
//         SGM.emitObjCConstructorThunk(cd);
   }
   void visitDestructorDecl(DestructorDecl *dd) {
      llvm_unreachable("destructor in extension?!");
   }

   void visitPatternBindingDecl(PatternBindingDecl *pd) {
      // Emit initializers for static variables.
      for (auto i : range(pd->getNumPatternEntries())) {
         if (pd->getExecutableInit(i)) {
            assert(pd->isStatic() && "stored property in extension?!");
            SGM.emitGlobalInitialization(pd, i);
         }
      }
   }

   void visitVarDecl(VarDecl *vd) {
      if (vd->hasStorage()) {
         bool hasDidSetOrWillSetDynamicReplacement =
            vd->hasDidSetOrWillSetDynamicReplacement();
         assert((vd->isStatic() || hasDidSetOrWillSetDynamicReplacement) &&
                "stored property in extension?!");
         if (!hasDidSetOrWillSetDynamicReplacement) {
            emitTypeMemberGlobalVariable(SGM, vd);
            visitAccessors(vd);
            return;
         }
      }
      visitAbstractStorageDecl(vd);
   }

   void visitSubscriptDecl(SubscriptDecl *sd) {
      SGM.emitDefaultArgGenerators(sd, sd->getIndices());
      visitAbstractStorageDecl(sd);
   }

   void visitEnumCaseDecl(EnumCaseDecl *ecd) {}
   void visitEnumElementDecl(EnumElementDecl *ed) {
      llvm_unreachable("enum elements aren't allowed in extensions");
   }

   void visitAbstractStorageDecl(AbstractStorageDecl *asd) {
      // @todo
//      if (asd->isObjC())
//         SGM.emitObjCPropertyMethodThunks(asd);

      SGM.tryEmitPropertyDescriptor(asd);
      visitAccessors(asd);
   }

   void visitAccessors(AbstractStorageDecl *asd) {
      asd->visitEmittedAccessors([&](AccessorDecl *accessor) {
         visitFuncDecl(accessor);
      });
   }
};

void PILGenModule::visitExtensionDecl(ExtensionDecl *ed) {
   PILGenExtension(*this).emitExtension(ed);
}
