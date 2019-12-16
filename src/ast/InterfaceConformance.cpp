//===--- InterfaceConformance.cpp - AST Interface Conformance ---------------===//
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
// This file implements the interface conformance data structures.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/ast/internal/ConformanceLookupTable.h"
#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/Availability.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/FileUnit.h"
#include "polarphp/ast/GenericEnvironment.h"
#include "polarphp/ast/LazyResolver.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/TypeCheckRequests.h"
#include "polarphp/ast/TypeWalker.h"
#include "polarphp/ast/Types.h"
#include "polarphp/global/Global.h"
#include "polarphp/basic/Statistic.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/SaveAndRestore.h"

#define DEBUG_TYPE "AST"

STATISTIC(NumConformanceLookupTables, "# of conformance lookup tables built");

namespace polar {

Witness::Witness(ValueDecl *decl, SubstitutionMap substitutions,
                 GenericEnvironment *syntheticEnv,
                 SubstitutionMap reqToSynthesizedEnvSubs) {
   if (!syntheticEnv && substitutions.empty() &&
       reqToSynthesizedEnvSubs.empty()) {
      storage = decl;
      return;
   }

   auto &ctx = decl->getAstContext();
   auto declRef = ConcreteDeclRef(decl, substitutions);
   auto storedMem = ctx.Allocate(sizeof(StoredWitness), alignof(StoredWitness));
   auto stored = new(storedMem) StoredWitness{declRef, syntheticEnv,
                                              reqToSynthesizedEnvSubs};

   storage = stored;
}

void Witness::dump() const { dump(llvm::errs()); }

void Witness::dump(llvm::raw_ostream &out) const {
   // FIXME: Implement!
}

InterfaceConformanceRef::InterfaceConformanceRef(InterfaceDecl *interface,
                                                 InterfaceConformance *conf) {
   assert(interface != nullptr &&
          "cannot construct InterfaceConformanceRef with null interface");
   if (conf) {
      assert(interface == conf->getInterface() && "interface conformance mismatch");
      Union = conf;
   } else {
      Union = interface;
   }
}

InterfaceDecl *InterfaceConformanceRef::getRequirement() const {
   assert(!isInvalid());

   if (isConcrete()) {
      return getConcrete()->getInterface();
   } else {
      return getAbstract();
   }
}

InterfaceConformanceRef
InterfaceConformanceRef::subst(Type origType,
                               SubstitutionMap subMap,
                               SubstOptions options) const {
   return subst(origType,
                QuerySubstitutionMap{subMap},
                LookUpConformanceInSubstitutionMap(subMap),
                options);
}

InterfaceConformanceRef
InterfaceConformanceRef::subst(Type origType,
                               TypeSubstitutionFn subs,
                               LookupConformanceFn conformances,
                               SubstOptions options) const {
   if (isInvalid())
      return *this;

   // If we have a concrete conformance, we need to substitute the
   // conformance to apply to the new type.
   if (isConcrete())
      return InterfaceConformanceRef(getConcrete()->subst(subs, conformances,
                                                          options));
   // If the type is an opaque archetype, the conformance will remain abstract,
   // unless we're specifically substituting opaque types.
   if (auto origArchetype = origType->getAs<ArchetypeType>()) {
      if (!options.contains(SubstFlags::SubstituteOpaqueArchetypes)
          && isa<OpaqueTypeArchetypeType>(origArchetype->getRoot())) {
         return *this;
      }
   }

   // Otherwise, compute the substituted type.
   auto substType = origType.subst(subs, conformances, options);

   // Opened existentials trivially conform and do not need to go through
   // substitution map lookup.
   if (substType->isOpenedExistential())
      return *this;

   auto *proto = getRequirement();

   // If the type is an existential, it must be self-conforming.
   if (substType->isExistentialType()) {
      auto optConformance =
         proto->getModuleContext()->lookupExistentialConformance(substType,
                                                                 proto);
      if (optConformance)
         return optConformance;

      return InterfaceConformanceRef::forInvalid();
   }

   // Check the conformance map.
   return conformances(origType->getCanonicalType(), substType, proto);
}

InterfaceConformanceRef InterfaceConformanceRef::mapConformanceOutOfContext() const {
   if (!isConcrete())
      return *this;

   auto *concrete = getConcrete()->subst(
      [](SubstitutableType *type) -> Type {
         if (auto *archetypeType = type->getAs<ArchetypeType>())
            return archetypeType->getInterfaceType();
         return type;
      },
      MakeAbstractConformanceForGenericType());
   return InterfaceConformanceRef(concrete);
}

Type
InterfaceConformanceRef::getTypeWitnessByName(Type type, Identifier name) const {
   assert(!isInvalid());

   // Find the named requirement.
   InterfaceDecl *proto = getRequirement();
   auto *assocType = proto->getAssociatedType(name);

   // FIXME: Shouldn't this be a hard error?
   if (!assocType)
      return ErrorType::get(proto->getAstContext());

   return assocType->getDeclaredInterfaceType().subst(
      SubstitutionMap::getInterfaceSubstitutions(proto, type, *this));
}

ConcreteDeclRef
InterfaceConformanceRef::getWitnessByName(Type type, DeclName name) const {
   // Find the named requirement.
   auto *proto = getRequirement();
   auto *requirement = proto->getSingleRequirement(name);
   if (requirement == nullptr)
      return ConcreteDeclRef();

   // For a type with dependent conformance, just return the requirement from
   // the interface. There are no interface conformance tables.
   if (!isConcrete()) {
      auto subs = SubstitutionMap::getInterfaceSubstitutions(proto, type, *this);
      return ConcreteDeclRef(requirement, subs);
   }

   return getConcrete()->getWitnessDeclRef(requirement);
}

void *InterfaceConformance::operator new(size_t bytes, AstContext &context,
                                         AllocationArena arena,
                                         unsigned alignment) {
   return context.Allocate(bytes, alignment, arena);

}

#define CONFORMANCE_SUBCLASS_DISPATCH(Method, Args)                          \
switch (getKind()) {                                                         \
  case InterfaceConformanceKind::Normal:                                      \
    POLAR_ASSERT_X(&InterfaceConformance::Method !=                            \
                  &NormalInterfaceConformance::Method,                      \
                  "InterfaceConformance::" # Method,                         \
                  "Must override NormalInterfaceConformance::" #Method);      \
    return cast<NormalInterfaceConformance>(this)->Method Args;               \
  case InterfaceConformanceKind::Self:                                        \
    POLAR_ASSERT_X(&InterfaceConformance::Method !=                            \
                  &SelfInterfaceConformance::Method,                        \
                  "InterfaceConformance::" # Method,                         \
                  "Must override SelfInterfaceConformance::" #Method);        \
    return cast<SelfInterfaceConformance>(this)->Method Args;                 \
  case InterfaceConformanceKind::Specialized:                                 \
    POLAR_ASSERT_X(&InterfaceConformance::Method !=                            \
                  &SpecializedInterfaceConformance::Method,                 \
                  "InterfaceConformance::" # Method,                         \
                  "Must override SpecializedInterfaceConformance::" #Method); \
    return cast<SpecializedInterfaceConformance>(this)->Method Args;          \
  case InterfaceConformanceKind::Inherited:                                   \
    POLAR_ASSERT_X(&InterfaceConformance::Method !=                            \
                  &InheritedInterfaceConformance::Method,                   \
                  "InterfaceConformance::" # Method,                         \
                  "Must override InheritedInterfaceConformance::" #Method);   \
    return cast<InheritedInterfaceConformance>(this)->Method Args;            \
}                                                                            \
llvm_unreachable("bad InterfaceConformanceKind");

#define ROOT_CONFORMANCE_SUBCLASS_DISPATCH(Method, Args)                     \
switch (getKind()) {                                                         \
  case InterfaceConformanceKind::Normal:                                      \
    POLAR_ASSERT_X(&RootInterfaceConformance::Method !=                        \
                  &NormalInterfaceConformance::Method,                      \
                  "InterfaceConformance::" # Method,                         \
                  "Must override NormalInterfaceConformance::" #Method);      \
    return cast<NormalInterfaceConformance>(this)->Method Args;               \
  case InterfaceConformanceKind::Self:                                        \
    POLAR_ASSERT_X(&RootInterfaceConformance::Method !=                        \
                   &SelfInterfaceConformance::Method,                        \
                   "InterfaceConformance::" # Method,                         \
                  "Must override SelfInterfaceConformance::" #Method);        \
    return cast<SelfInterfaceConformance>(this)->Method Args;                 \
  case InterfaceConformanceKind::Specialized:                                 \
  case InterfaceConformanceKind::Inherited:                                   \
    llvm_unreachable("not a root conformance");                              \
}                                                                            \
llvm_unreachable("bad InterfaceConformanceKind");

/// Get the interface being conformed to.
InterfaceDecl *InterfaceConformance::getInterface() const {
   CONFORMANCE_SUBCLASS_DISPATCH(getInterface, ())
}

DeclContext *InterfaceConformance::getDeclContext() const {
   CONFORMANCE_SUBCLASS_DISPATCH(getDeclContext, ())
}

/// Retrieve the state of this conformance.
InterfaceConformanceState InterfaceConformance::getState() const {
   CONFORMANCE_SUBCLASS_DISPATCH(getState, ())
}

ConformanceEntryKind InterfaceConformance::getSourceKind() const {
   CONFORMANCE_SUBCLASS_DISPATCH(getSourceKind, ())
}

NormalInterfaceConformance *InterfaceConformance::getImplyingConformance() const {
   CONFORMANCE_SUBCLASS_DISPATCH(getImplyingConformance, ())
}

bool
InterfaceConformance::hasTypeWitness(AssociatedTypeDecl *assocType) const {
   CONFORMANCE_SUBCLASS_DISPATCH(hasTypeWitness, (assocType));
}

TypeWitnessAndDecl
InterfaceConformance::getTypeWitnessAndDecl(AssociatedTypeDecl *assocType,
                                            SubstOptions options) const {
   CONFORMANCE_SUBCLASS_DISPATCH(getTypeWitnessAndDecl,
                                 (assocType, options))
}

Type InterfaceConformance::getTypeWitness(AssociatedTypeDecl *assocType,
                                          SubstOptions options) const {
   return getTypeWitnessAndDecl(assocType, options).getWitnessType();
}

ConcreteDeclRef
InterfaceConformance::getWitnessDeclRef(ValueDecl *requirement) const {
   CONFORMANCE_SUBCLASS_DISPATCH(getWitnessDeclRef, (requirement))
}

ValueDecl *InterfaceConformance::getWitnessDecl(ValueDecl *requirement) const {
   switch (getKind()) {
      case InterfaceConformanceKind::Normal:
         return cast<NormalInterfaceConformance>(this)->getWitness(requirement)
            .getDecl();
      case InterfaceConformanceKind::Self:
         return cast<SelfInterfaceConformance>(this)->getWitness(requirement)
            .getDecl();
      case InterfaceConformanceKind::Inherited:
         return cast<InheritedInterfaceConformance>(this)
            ->getInheritedConformance()->getWitnessDecl(requirement);

      case InterfaceConformanceKind::Specialized:
         return cast<SpecializedInterfaceConformance>(this)
            ->getGenericConformance()->getWitnessDecl(requirement);
   }
   llvm_unreachable("unhandled kind");
}

/// Determine whether the witness for the given requirement
/// is either the default definition or was otherwise deduced.
bool InterfaceConformance::
usesDefaultDefinition(AssociatedTypeDecl *requirement) const {
   CONFORMANCE_SUBCLASS_DISPATCH(usesDefaultDefinition, (requirement))
}

GenericEnvironment *InterfaceConformance::getGenericEnvironment() const {
   switch (getKind()) {
      case InterfaceConformanceKind::Inherited:
      case InterfaceConformanceKind::Normal:
      case InterfaceConformanceKind::Self:
         // If we have a normal or inherited interface conformance, look for its
         // generic parameters.
         return getDeclContext()->getGenericEnvironmentOfContext();

      case InterfaceConformanceKind::Specialized:
         // If we have a specialized interface conformance, since we do not support
         // currently partial specialization, we know that it cannot have any open
         // type variables.
         //
         // FIXME: We could return a meaningful GenericEnvironment here
         return nullptr;
   }

   llvm_unreachable("Unhandled InterfaceConformanceKind in switch.");
}

GenericSignature InterfaceConformance::getGenericSignature() const {
   switch (getKind()) {
      case InterfaceConformanceKind::Inherited:
      case InterfaceConformanceKind::Normal:
      case InterfaceConformanceKind::Self:
         // If we have a normal or inherited interface conformance, look for its
         // generic signature.
         return getDeclContext()->getGenericSignatureOfContext();

      case InterfaceConformanceKind::Specialized:
         // If we have a specialized interface conformance, since we do not support
         // currently partial specialization, we know that it cannot have any open
         // type variables.
         return nullptr;
   }

   llvm_unreachable("Unhandled InterfaceConformanceKind in switch.");
}

SubstitutionMap InterfaceConformance::getSubstitutions(ModuleDecl *M) const {
   // Walk down to the base NormalInterfaceConformance.
   SubstitutionMap subMap;
   const InterfaceConformance *parent = this;
   while (!isa<RootInterfaceConformance>(parent)) {
      switch (parent->getKind()) {
         case InterfaceConformanceKind::Normal:
         case InterfaceConformanceKind::Self:
            llvm_unreachable("should have exited the loop?!");
         case InterfaceConformanceKind::Inherited:
            parent =
               cast<InheritedInterfaceConformance>(parent)->getInheritedConformance();
            break;
         case InterfaceConformanceKind::Specialized: {
            auto SC = cast<SpecializedInterfaceConformance>(parent);
            parent = SC->getGenericConformance();
            assert(subMap.empty() && "multiple conformance specializations?!");
            subMap = SC->getSubstitutionMap();
            break;
         }
      }
   }

   // Found something; we're done!
   if (!subMap.empty())
      return subMap;

   // If the normal conformance is for a generic type, and we didn't hit a
   // specialized conformance, collect the substitutions from the generic type.
   // FIXME: The AST should do this for us.
   const NormalInterfaceConformance *normalC =
      dyn_cast<NormalInterfaceConformance>(parent);
   if (!normalC)
      return SubstitutionMap();

   if (!normalC->getType()->isSpecialized())
      return SubstitutionMap();

   auto *DC = normalC->getDeclContext();
   return normalC->getType()->getContextSubstitutionMap(M, DC);
}

bool RootInterfaceConformance::isInvalid() const {
   ROOT_CONFORMANCE_SUBCLASS_DISPATCH(isInvalid, ())
}

SourceLoc RootInterfaceConformance::getLoc() const {
   ROOT_CONFORMANCE_SUBCLASS_DISPATCH(getLoc, ())
}

bool
RootInterfaceConformance::isWeakImported(ModuleDecl *fromModule) const {
   auto *dc = getDeclContext();
   if (dc->getParentModule() == fromModule)
      return false;

   // If the interface is weak imported, so are any conformances to it.
   if (getInterface()->isWeakImported(fromModule))
      return true;

   // If the conforming type is weak imported, so are any of its conformances.
   if (auto *nominal = getType()->getAnyNominal())
      if (nominal->isWeakImported(fromModule))
         return true;

   // If the conformance is declared in an extension with the @_weakLinked
   // attribute, it is weak imported.
   if (auto *ext = dyn_cast<ExtensionDecl>(dc))
      if (ext->isWeakImported(fromModule))
         return true;

   return false;
}

bool RootInterfaceConformance::hasWitness(ValueDecl *requirement) const {
   ROOT_CONFORMANCE_SUBCLASS_DISPATCH(hasWitness, (requirement))
}

bool NormalInterfaceConformance::isRetroactive() const {
   auto module = getDeclContext()->getParentModule();

   // If the conformance occurs in the same module as the interface definition,
   // this is not a retroactive conformance.
   auto interfaceModule = getInterface()->getDeclContext()->getParentModule();
   if (module == interfaceModule)
      return false;

   // If the conformance occurs in the same module as the conforming type
   // definition, this is not a retroactive conformance.
   if (auto nominal = getType()->getAnyNominal()) {
      auto nominalModule = nominal->getParentModule();

      // Consider the overlay module to be the "home" of a nominal type
      // defined in a Clang module.
      if (auto nominalLoadedModule =
         dyn_cast<LoadedFile>(nominal->getModuleScopeContext())) {
         if (auto overlayModule = nominalLoadedModule->getOverlayModule())
            nominalModule = overlayModule;
      }

      if (module == nominalModule)
         return false;
   }

   // Everything else is retroactive.
   return true;
}

bool NormalInterfaceConformance::isSynthesizedNonUnique() const {
   if (auto *file = dyn_cast<FileUnit>(getDeclContext()->getModuleScopeContext()))
      return file->getKind() == FileUnitKind::ClangModule;
   return false;
}

bool NormalInterfaceConformance::isResilient() const {
   // If the type is non-resilient or the module we're in is non-resilient, the
   // conformance is non-resilient.
   // FIXME: Looking at the type is not the right long-term solution. We need an
   // explicit mechanism for declaring conformances as 'fragile', or even
   // individual witnesses.
   if (!getType()->getAnyNominal()->isResilient())
      return false;

   return getDeclContext()->getParentModule()->isResilient();
}

Optional<ArrayRef<Requirement>>
InterfaceConformance::getConditionalRequirementsIfAvailable() const {
   CONFORMANCE_SUBCLASS_DISPATCH(getConditionalRequirementsIfAvailable, ());
}

ArrayRef<Requirement> InterfaceConformance::getConditionalRequirements() const {
   CONFORMANCE_SUBCLASS_DISPATCH(getConditionalRequirements, ());
}

Optional<ArrayRef<Requirement>>
InterfaceConformanceRef::getConditionalRequirementsIfAvailable() const {
   if (isConcrete())
      return getConcrete()->getConditionalRequirementsIfAvailable();
   else
      // An abstract conformance is never conditional: any conditionality in the
      // concrete types that will eventually pass through this at runtime is
      // completely pre-checked and packaged up.
      return ArrayRef<Requirement>();
}

ArrayRef<Requirement>
InterfaceConformanceRef::getConditionalRequirements() const {
   if (isConcrete())
      return getConcrete()->getConditionalRequirements();
   else
      // An abstract conformance is never conditional, as above.
      return {};
}

void NormalInterfaceConformance::differenceAndStoreConditionalRequirements()
const {
   switch (CRState) {
      case ConditionalRequirementsState::Complete:
         // already done!
         return;
      case ConditionalRequirementsState::Computing:
         // recursive
         return;
      case ConditionalRequirementsState::Uncomputed:
         // try to compute it!
         break;
   };

   CRState = ConditionalRequirementsState::Computing;
   auto success = [this](ArrayRef<Requirement> reqs) {
      ConditionalRequirements = reqs;
      assert(CRState == ConditionalRequirementsState::Computing);
      CRState = ConditionalRequirementsState::Complete;
   };
   auto failure = [this] {
      assert(CRState == ConditionalRequirementsState::Computing);
      CRState = ConditionalRequirementsState::Uncomputed;
   };

   auto &ctxt = getInterface()->getAstContext();
   auto DC = getDeclContext();
   // A non-extension conformance won't have conditional requirements.
   if (!isa<ExtensionDecl>(DC)) {
      success({});
      return;
   }

   auto *ext = cast<ExtensionDecl>(DC);
   auto nominal = ext->getExtendedNominal();
   auto typeSig = nominal->getGenericSignature();

   // A non-generic type won't have conditional requirements.
   if (!typeSig) {
      success({});
      return;
   }

   // If the extension is invalid, it won't ever get a signature, so we
   // "succeed" with an empty result instead.
   if (ext->isInvalid()) {
      success({});
      return;
   }

   // Recursively validating the signature comes up frequently as expanding
   // conformance requirements might re-enter this method.  We can at least catch
   // this and come back to these requirements later.
   //
   // FIXME: In the long run, break this cycle in a more principled way.
   if (ext->isComputingGenericSignature()) {
      failure();
      return;
   }

   auto extensionSig = ext->getGenericSignature();
   auto canExtensionSig = extensionSig->getCanonicalSignature();
   auto canTypeSig = typeSig->getCanonicalSignature();
   if (canTypeSig == canExtensionSig) {
      success({});
      return;
   }

   // The extension signature should be a superset of the type signature, meaning
   // every thing in the type signature either is included too or is implied by
   // something else. The most important bit is having the same type
   // parameters. (NB. if/when Swift gets parameterized extensions, this needs to
   // change.)
   assert(canTypeSig.getGenericParams() == canExtensionSig.getGenericParams());

   // Find the requirements in the extension that aren't proved by the original
   // type, these are the ones that make the conformance conditional.
   success(ctxt.AllocateCopy(extensionSig->requirementsNotSatisfiedBy(typeSig)));
}

void NormalInterfaceConformance::setSignatureConformances(
   ArrayRef<InterfaceConformanceRef> conformances) {
   if (conformances.empty()) {
      SignatureConformances = {};
      return;
   }

   auto &ctx = getInterface()->getAstContext();
   SignatureConformances = ctx.AllocateCopy(conformances);

#if !NDEBUG
   unsigned idx = 0;
   for (const auto &req : getInterface()->getRequirementSignature()) {
      if (req.getKind() == RequirementKind::Conformance) {
         assert((!conformances[idx].isConcrete() ||
                         !conformances[idx].getConcrete()->getType()->hasArchetype()) &&
                "Should have interface types here");
         assert(idx < conformances.size());
         assert(conformances[idx].isInvalid() ||
                conformances[idx].getRequirement() ==
                req.getSecondType()->castTo<InterfaceType>()->getDecl());
         ++idx;
      }
   }
   assert(idx == conformances.size() && "Too many conformances");
#endif
}

void NormalInterfaceConformance::resolveLazyInfo() const {
   assert(Loader);

   auto *loader = Loader;
   auto *mutableThis = const_cast<NormalInterfaceConformance *>(this);
   mutableThis->Loader = nullptr;
   loader->finishNormalConformance(mutableThis, LoaderContextData);
}

void NormalInterfaceConformance::setLazyLoader(LazyConformanceLoader *loader,
                                               uint64_t contextData) {
   assert(!Loader && "already has a loader");
   Loader = loader;
   LoaderContextData = contextData;
}

namespace {
class PrettyStackTraceRequirement : public llvm::PrettyStackTraceEntry {
   const char *Action;
   const InterfaceConformance *Conformance;
   ValueDecl *Requirement;
public:
   PrettyStackTraceRequirement(const char *action,
                               const InterfaceConformance *conformance,
                               ValueDecl *requirement)
      : Action(action), Conformance(conformance), Requirement(requirement) {}

   void print(llvm::raw_ostream &out) const override {
      out << "While " << Action << " requirement ";
      Requirement->dumpRef(out);
      out << " in conformance ";
      Conformance->printName(out);
      out << "\n";
   }
};
} // end anonymous namespace

bool NormalInterfaceConformance::hasTypeWitness(
   AssociatedTypeDecl *assocType) const {
   if (Loader)
      resolveLazyInfo();

   auto found = TypeWitnesses.find(assocType);
   if (found != TypeWitnesses.end()) {
      return !found->getSecond().getWitnessType().isNull();
   }

   return false;
}

TypeWitnessAndDecl
NormalInterfaceConformance::getTypeWitnessAndDecl(AssociatedTypeDecl *assocType,
                                                  SubstOptions options) const {
   if (Loader)
      resolveLazyInfo();

   // Check whether we already have a type witness.
   auto known = TypeWitnesses.find(assocType);
   if (known != TypeWitnesses.end())
      return known->second;

   // If there is a tentative-type-witness function, use it.
   if (options.getTentativeTypeWitness) {
      if (Type witnessType =
         Type(options.getTentativeTypeWitness(this, assocType)))
         return {witnessType, nullptr};
   }

   // If this conformance is in a state where it is inferring type witnesses but
   // we didn't find anything, fail.
   if (getState() == InterfaceConformanceState::CheckingTypeWitnesses) {
      return {Type(), nullptr};
   }

   // If the conditional requirements aren't known, we can't properly run
   // inference.
   if (!getConditionalRequirementsIfAvailable()) {
      return TypeWitnessAndDecl();
   }

   return evaluateOrDefault(
      assocType->getAstContext().evaluator,
      TypeWitnessRequest{const_cast<NormalInterfaceConformance *>(this),
                         assocType},
      TypeWitnessAndDecl());
}

TypeWitnessAndDecl NormalInterfaceConformance::getTypeWitnessUncached(
   AssociatedTypeDecl *requirement) const {
   auto entry = TypeWitnesses.find(requirement);
   if (entry == TypeWitnesses.end()) {
      return TypeWitnessAndDecl();
   }
   return entry->second;
}

void NormalInterfaceConformance::setTypeWitness(AssociatedTypeDecl *assocType,
                                                Type type,
                                                TypeDecl *typeDecl) const {
   assert(getInterface() == cast<InterfaceDecl>(assocType->getDeclContext()) &&
          "associated type in wrong interface");
   assert((TypeWitnesses.count(assocType) == 0 ||
           TypeWitnesses[assocType].getWitnessType().isNull()) &&
          "Type witness already known");
   assert((!isComplete() || isInvalid()) && "Conformance already complete?");
   assert(!type->hasArchetype() && "type witnesses must be interface types");
   TypeWitnesses[assocType] = {type, typeDecl};
}

Type InterfaceConformance::getAssociatedType(Type assocType) const {
   assert(assocType->isTypeParameter() &&
          "associated type must be a type parameter");

   InterfaceConformanceRef ref(const_cast<InterfaceConformance *>(this));
   return ref.getAssociatedType(getType(), assocType);
}

Type InterfaceConformanceRef::getAssociatedType(Type conformingType,
                                                Type assocType) const {
   assert(!isConcrete() || getConcrete()->getType()->isEqual(conformingType));

   auto type = assocType->getCanonicalType();
   auto proto = getRequirement();

   // Fast path for generic parameters.
   if (isa<GenericTypeParamType>(type)) {
      assert(type->isEqual(proto->getSelfInterfaceType()) &&
             "type parameter in interface was not Self");
      return conformingType;
   }

   // Fast path for dependent member types on 'Self' of our associated types.
   auto memberType = cast<DependentMemberType>(type);
   if (memberType.getBase()->isEqual(proto->getSelfInterfaceType()) &&
       memberType->getAssocType()->getInterface() == proto &&
       isConcrete())
      return getConcrete()->getTypeWitness(memberType->getAssocType());

   // General case: consult the substitution map.
   auto substMap =
      SubstitutionMap::getInterfaceSubstitutions(proto, conformingType, *this);
   return type.subst(substMap);
}

InterfaceConformanceRef
InterfaceConformanceRef::getAssociatedConformance(Type conformingType,
                                                  Type assocType,
                                                  InterfaceDecl *interface) const {
   // If this is a concrete conformance, look up the associated conformance.
   if (isConcrete()) {
      auto conformance = getConcrete();
      assert(conformance->getType()->isEqual(conformingType));
      return conformance->getAssociatedConformance(assocType, interface);
   }

   // Otherwise, apply the substitution {self -> conformingType}
   // to the abstract conformance requirement laid upon the dependent type
   // by the interface.
   auto subMap =
      SubstitutionMap::getInterfaceSubstitutions(getRequirement(),
                                                 conformingType, *this);
   auto abstractConf = InterfaceConformanceRef(interface);
   return abstractConf.subst(assocType, subMap);
}

InterfaceConformanceRef
InterfaceConformance::getAssociatedConformance(Type assocType,
                                               InterfaceDecl *interface) const {
   CONFORMANCE_SUBCLASS_DISPATCH(getAssociatedConformance,
                                 (assocType, interface))
}

InterfaceConformanceRef
NormalInterfaceConformance::getAssociatedConformance(Type assocType,
                                                     InterfaceDecl *interface) const {
   assert(assocType->isTypeParameter() &&
          "associated type must be a type parameter");

   // Fill in the signature conformances, if we haven't done so yet.
   if (getSignatureConformances().empty()) {
      const_cast<NormalInterfaceConformance *>(this)->finishSignatureConformances();
   }

   assert(!getSignatureConformances().empty() &&
          "signature conformances not yet computed");

   unsigned conformanceIndex = 0;
   for (const auto &reqt : getInterface()->getRequirementSignature()) {
      if (reqt.getKind() == RequirementKind::Conformance) {
         // Is this the conformance we're looking for?
         if (reqt.getFirstType()->isEqual(assocType) &&
             reqt.getSecondType()->castTo<InterfaceType>()->getDecl() == interface)
            return getSignatureConformances()[conformanceIndex];

         ++conformanceIndex;
      }
   }

   llvm_unreachable(
      "requested conformance was not a direct requirement of the interface");
}


/// A stripped-down version of Type::subst that only works on the interface
/// Self type wrapped in zero or more DependentMemberTypes.
static Type
recursivelySubstituteBaseType(ModuleDecl *module,
                              NormalInterfaceConformance *conformance,
                              DependentMemberType *depMemTy) {
   Type origBase = depMemTy->getBase();

   // Recursive case.
   if (auto *depBase = origBase->getAs<DependentMemberType>()) {
      Type substBase = recursivelySubstituteBaseType(
         module, conformance, depBase);
      return depMemTy->substBaseType(module, substBase);
   }

   // Base case. The associated type's interface should be either the
   // conformance interface or an inherited interface.
   auto *reqProto = depMemTy->getAssocType()->getInterface();
   assert(origBase->isEqual(reqProto->getSelfInterfaceType()));

   InterfaceConformance *reqConformance = conformance;

   // If we have an inherited interface just look up the conformance.
   if (reqProto != conformance->getInterface()) {
      reqConformance = module->lookupConformance(conformance->getType(), reqProto)
         .getConcrete();
   }

   return reqConformance->getTypeWitness(depMemTy->getAssocType());
}

/// Collect conformances for the requirement signature.
void NormalInterfaceConformance::finishSignatureConformances() {
   if (!SignatureConformances.empty())
      return;

   auto *proto = getInterface();
   auto reqSig = proto->getRequirementSignature();
   if (reqSig.empty())
      return;

   SmallVector<InterfaceConformanceRef, 4> reqConformances;
   for (const auto &req : reqSig) {
      if (req.getKind() != RequirementKind::Conformance)
         continue;

      ModuleDecl *module = getDeclContext()->getParentModule();

      Type substTy;
      auto origTy = req.getFirstType();
      if (origTy->isEqual(proto->getSelfInterfaceType())) {
         substTy = getType();
      } else {
         auto *depMemTy = origTy->castTo<DependentMemberType>();
         substTy = recursivelySubstituteBaseType(module, this, depMemTy);
      }
      auto reqProto = req.getSecondType()->castTo<InterfaceType>()->getDecl();

      // Looking up a conformance for a contextual type and mapping the
      // conformance context produces a more accurate result than looking
      // up a conformance from an interface type.
      //
      // This can happen if the conformance has an associated conformance
      // depending on an associated type that is made concrete in a
      // refining interface.
      //
      // That is, the conformance of an interface type G<T> : P really
      // depends on the generic signature of the current context, because
      // performing the lookup in a "more" constrained extension than the
      // one where the conformance was defined must produce concrete
      // conformances.
      //
      // FIXME: Eliminate this, perhaps by adding a variant of
      // lookupConformance() taking a generic signature.
      if (substTy->hasTypeParameter())
         substTy = getDeclContext()->mapTypeIntoContext(substTy);

      reqConformances.push_back(module->lookupConformance(substTy, reqProto)
                                   .mapConformanceOutOfContext());
   }
   setSignatureConformances(reqConformances);
}

Witness RootInterfaceConformance::getWitness(ValueDecl *requirement) const {
   ROOT_CONFORMANCE_SUBCLASS_DISPATCH(getWitness, (requirement))
}

/// Retrieve the value witness corresponding to the given requirement.
Witness NormalInterfaceConformance::getWitness(ValueDecl *requirement) const {
   assert(!isa<AssociatedTypeDecl>(requirement) && "Request type witness");
   assert(requirement->isInterfaceRequirement() && "Not a requirement");

   if (Loader)
      resolveLazyInfo();

   return evaluateOrDefault(
      requirement->getAstContext().evaluator,
      ValueWitnessRequest{const_cast<NormalInterfaceConformance *>(this),
                          requirement},
      Witness());
}

Witness
NormalInterfaceConformance::getWitnessUncached(ValueDecl *requirement) const {
   auto entry = Mapping.find(requirement);
   if (entry == Mapping.end()) {
      return Witness();
   }
   return entry->second;
}

Witness SelfInterfaceConformance::getWitness(ValueDecl *requirement) const {
   return Witness(requirement, SubstitutionMap(), nullptr, SubstitutionMap());
}

ConcreteDeclRef
RootInterfaceConformance::getWitnessDeclRef(ValueDecl *requirement) const {
   if (auto witness = getWitness(requirement)) {
      auto *witnessDecl = witness.getDecl();

      // If the witness is generic, you have to call getWitness() and build
      // your own substitutions in terms of the synthetic environment.
      if (auto *witnessDC = dyn_cast<DeclContext>(witnessDecl))
         assert(!witnessDC->isInnermostContextGeneric());

      // If the witness is not generic, use type substitutions from the
      // witness's parent. Don't use witness.getSubstitutions(), which
      // are written in terms of the synthetic environment.
      auto subs =
         getType()->getContextSubstitutionMap(getDeclContext()->getParentModule(),
                                              witnessDecl->getDeclContext());
      return ConcreteDeclRef(witness.getDecl(), subs);
   }

   return ConcreteDeclRef();
}

void NormalInterfaceConformance::setWitness(ValueDecl *requirement,
                                            Witness witness) const {
   assert(!isa<AssociatedTypeDecl>(requirement) && "Request type witness");
   assert(getInterface() == cast<InterfaceDecl>(requirement->getDeclContext()) &&
          "requirement in wrong interface");
   assert(Mapping.count(requirement) == 0 && "Witness already known");
   assert((!isComplete() || isInvalid() ||
           requirement->getAttrs().hasAttribute<OptionalAttr>() ||
           requirement->getAttrs().isUnavailable(
              requirement->getAstContext())) &&
          "Conformance already complete?");
   Mapping[requirement] = witness;
}

SpecializedInterfaceConformance::SpecializedInterfaceConformance(
   Type conformingType,
   InterfaceConformance *genericConformance,
   SubstitutionMap substitutions)
   : InterfaceConformance(InterfaceConformanceKind::Specialized, conformingType),
     GenericConformance(genericConformance),
     GenericSubstitutions(substitutions) {
   assert(genericConformance->getKind() != InterfaceConformanceKind::Specialized);
}

void SpecializedInterfaceConformance::computeConditionalRequirements() const {
   // already computed?
   if (ConditionalRequirements)
      return;

   auto parentCondReqs =
      GenericConformance->getConditionalRequirementsIfAvailable();
   if (!parentCondReqs)
      return;

   if (!parentCondReqs->empty()) {
      // Substitute the conditional requirements so that they're phrased in
      // terms of the specialized types, not the conformance-declaring decl's
      // types.
      auto nominal = GenericConformance->getType()->getAnyNominal();
      auto module = nominal->getModuleContext();
      auto subMap = getType()->getContextSubstitutionMap(module, nominal);

      SmallVector<Requirement, 4> newReqs;
      for (auto oldReq : *parentCondReqs) {
         if (auto newReq = oldReq.subst(QuerySubstitutionMap{subMap},
                                        LookUpConformanceInModule(module)))
            newReqs.push_back(*newReq);
      }
      auto &ctxt = getInterface()->getAstContext();
      ConditionalRequirements = ctxt.AllocateCopy(newReqs);
   } else {
      ConditionalRequirements = ArrayRef<Requirement>();
   }
}

bool SpecializedInterfaceConformance::hasTypeWitness(
   AssociatedTypeDecl *assocType) const {
   return TypeWitnesses.find(assocType) != TypeWitnesses.end() ||
          GenericConformance->hasTypeWitness(assocType);
}

TypeWitnessAndDecl
SpecializedInterfaceConformance::getTypeWitnessAndDecl(
   AssociatedTypeDecl *assocType,
   SubstOptions options) const {
   assert(getInterface() == cast<InterfaceDecl>(assocType->getDeclContext()) &&
          "associated type in wrong interface");

   // If we've already created this type witness, return it.
   auto known = TypeWitnesses.find(assocType);
   if (known != TypeWitnesses.end()) {
      return known->second;
   }

   // Otherwise, perform substitutions to create this witness now.

   // Local function to determine whether we will end up referring to a
   // tentative witness that may not be chosen.
   auto root = GenericConformance->getRootConformance();
   auto isTentativeWitness = [&] {
      if (root->getState() != InterfaceConformanceState::CheckingTypeWitnesses)
         return false;

      return !root->hasTypeWitness(assocType);
   };

   auto genericWitnessAndDecl
      = GenericConformance->getTypeWitnessAndDecl(assocType, options);

   auto genericWitness = genericWitnessAndDecl.getWitnessType();
   if (!genericWitness)
      return {Type(), nullptr};

   auto *typeDecl = genericWitnessAndDecl.getWitnessDecl();

   // Form the substitution.
   auto substitutionMap = getSubstitutionMap();
   if (substitutionMap.empty())
      return TypeWitnessAndDecl();

   // Apply the substitution we computed above
   auto specializedType = genericWitness.subst(substitutionMap, options);
   if (specializedType->hasError()) {
      if (isTentativeWitness())
         return {Type(), nullptr};

      specializedType = ErrorType::get(genericWitness);
   }

   // If we aren't in a case where we used the tentative type witness
   // information, cache the result.
   auto specializedWitnessAndDecl = TypeWitnessAndDecl{specializedType, typeDecl};
   if (!isTentativeWitness() && !specializedType->hasError())
      TypeWitnesses[assocType] = specializedWitnessAndDecl;

   return specializedWitnessAndDecl;
}

InterfaceConformanceRef
SpecializedInterfaceConformance::getAssociatedConformance(Type assocType,
                                                          InterfaceDecl *interface) const {
   InterfaceConformanceRef conformance =
      GenericConformance->getAssociatedConformance(assocType, interface);

   auto subMap = getSubstitutionMap();

   Type origType =
      (conformance.isConcrete()
       ? conformance.getConcrete()->getType()
       : GenericConformance->getAssociatedType(assocType));

   return conformance.subst(origType, subMap);
}

ConcreteDeclRef
SpecializedInterfaceConformance::getWitnessDeclRef(
   ValueDecl *requirement) const {
   auto baseWitness = GenericConformance->getWitnessDeclRef(requirement);
   if (!baseWitness || !baseWitness.isSpecialized())
      return baseWitness;

   auto specializationMap = getSubstitutionMap();

   auto witnessDecl = baseWitness.getDecl();
   auto witnessMap = baseWitness.getSubstitutions();

   auto combinedMap = witnessMap.subst(specializationMap);
   return ConcreteDeclRef(witnessDecl, combinedMap);
}

InterfaceConformanceRef
InheritedInterfaceConformance::getAssociatedConformance(Type assocType,
                                                        InterfaceDecl *interface) const {
   auto underlying =
      InheritedConformance->getAssociatedConformance(assocType, interface);


   // If the conformance is for Self, return an inherited conformance.
   if (underlying.isConcrete() &&
       assocType->isEqual(getInterface()->getSelfInterfaceType())) {
      auto subclassType = getType();
      AstContext &ctx = subclassType->getAstContext();
      return InterfaceConformanceRef(
         ctx.getInheritedConformance(subclassType,
                                     underlying.getConcrete()));
   }

   return underlying;
}

ConcreteDeclRef
InheritedInterfaceConformance::getWitnessDeclRef(ValueDecl *requirement) const {
   // FIXME: substitutions?
   return InheritedConformance->getWitnessDeclRef(requirement);
}

const NormalInterfaceConformance *
InterfaceConformance::getRootNormalConformance() const {
   // This is an unsafe cast; remove this entire method.
   return cast<NormalInterfaceConformance>(getRootConformance());
}

const RootInterfaceConformance *
InterfaceConformance::getRootConformance() const {
   const InterfaceConformance *C = this;
   while (true) {
      switch (C->getKind()) {
         case InterfaceConformanceKind::Normal:
         case InterfaceConformanceKind::Self:
            return cast<RootInterfaceConformance>(C);
         case InterfaceConformanceKind::Inherited:
            C = cast<InheritedInterfaceConformance>(C)
               ->getInheritedConformance();
            break;
         case InterfaceConformanceKind::Specialized:
            C = cast<SpecializedInterfaceConformance>(C)
               ->getGenericConformance();
            break;
      }
   }
}

bool InterfaceConformance::isVisibleFrom(const DeclContext *dc) const {
   // FIXME: Implement me!
   return true;
}

InterfaceConformance *
InterfaceConformance::subst(SubstitutionMap subMap,
                            SubstOptions options) const {
   return subst(QuerySubstitutionMap{subMap},
                LookUpConformanceInSubstitutionMap(subMap),
                options);
}

InterfaceConformance *
InterfaceConformance::subst(TypeSubstitutionFn subs,
                            LookupConformanceFn conformances,
                            SubstOptions options) const {
   switch (getKind()) {
      case InterfaceConformanceKind::Normal: {
         auto origType = getType();
         if (!origType->hasTypeParameter() &&
             !origType->hasArchetype())
            return const_cast<InterfaceConformance *>(this);

         auto substType = origType.subst(subs, conformances, options);
         if (substType->isEqual(origType))
            return const_cast<InterfaceConformance *>(this);

         auto subMap = SubstitutionMap::get(getGenericSignature(),
                                            subs, conformances);
         return substType->getAstContext()
            .getSpecializedConformance(substType,
                                       const_cast<InterfaceConformance *>(this),
                                       subMap);
      }
      case InterfaceConformanceKind::Self:
         return const_cast<InterfaceConformance *>(this);
      case InterfaceConformanceKind::Inherited: {
         // Substitute the base.
         auto inheritedConformance
            = cast<InheritedInterfaceConformance>(this)->getInheritedConformance();

         auto origType = getType();
         if (!origType->hasTypeParameter() &&
             !origType->hasArchetype()) {
            return const_cast<InterfaceConformance *>(this);
         }

         auto origBaseType = inheritedConformance->getType();
         if (origBaseType->hasTypeParameter() ||
             origBaseType->hasArchetype()) {
            // Substitute into the superclass.
            inheritedConformance = inheritedConformance->subst(subs, conformances,
                                                               options);
         }

         auto substType = origType.subst(subs, conformances, options);
         return substType->getAstContext()
            .getInheritedConformance(substType, inheritedConformance);
      }
      case InterfaceConformanceKind::Specialized: {
         // Substitute the substitutions in the specialized conformance.
         auto spec = cast<SpecializedInterfaceConformance>(this);
         auto genericConformance = spec->getGenericConformance();
         auto subMap = spec->getSubstitutionMap();

         auto origType = getType();
         auto substType = origType.subst(subs, conformances, options);
         return substType->getAstContext()
            .getSpecializedConformance(substType, genericConformance,
                                       subMap.subst(subs, conformances, options));
      }
   }
   llvm_unreachable("bad InterfaceConformanceKind");
}

InterfaceConformance *
InterfaceConformance::getInheritedConformance(InterfaceDecl *interface) const {
   auto result =
      getAssociatedConformance(getInterface()->getSelfInterfaceType(), interface);
   return result.isConcrete() ? result.getConcrete() : nullptr;
}

#pragma mark Interface conformance lookup

void NominalTypeDecl::prepareConformanceTable() const {
   if (ConformanceTable)
      return;

   auto mutableThis = const_cast<NominalTypeDecl *>(this);
   AstContext &ctx = getAstContext();
   ConformanceTable = new(ctx) ConformanceLookupTable(ctx);
   ++NumConformanceLookupTables;

   // If this type declaration was not parsed from source code or introduced
   // via the Clang importer, don't add any synthesized conformances.
   auto *file = cast<FileUnit>(getModuleScopeContext());
   if (file->getKind() != FileUnitKind::Source &&
       file->getKind() != FileUnitKind::ClangModule &&
       file->getKind() != FileUnitKind::DWARFModule) {
      return;
   }

   SmallPtrSet<InterfaceDecl *, 2> interfaces;

   auto addSynthesized = [&](KnownInterfaceKind kind) {
      if (auto *proto = getAstContext().getInterface(kind)) {
         if (interfaces.count(proto) == 0) {
            ConformanceTable->addSynthesizedConformance(mutableThis, proto);
            interfaces.insert(proto);
         }
      }
   };

   // Add interfaces for any synthesized interface attributes.
   for (auto attr : getAttrs().getAttributes<SynthesizedInterfaceAttr>()) {
      addSynthesized(attr->getInterfaceKind());
   }

   // Add any implicit conformances.
   if (auto theEnum = dyn_cast<EnumDecl>(mutableThis)) {
      if (theEnum->hasCases() && theEnum->hasOnlyCasesWithoutAssociatedValues()) {
         // Simple enumerations conform to Equatable.
         addSynthesized(KnownInterfaceKind::Equatable);

         // Simple enumerations conform to Hashable.
         addSynthesized(KnownInterfaceKind::Hashable);
      }

      // Enumerations with a raw type conform to RawRepresentable.
      if (theEnum->hasRawType() && !theEnum->getRawType()->hasError()) {
         addSynthesized(KnownInterfaceKind::RawRepresentable);
      }
   }
}

bool NominalTypeDecl::lookupConformance(
   ModuleDecl *module, InterfaceDecl *interface,
   SmallVectorImpl<InterfaceConformance *> &conformances) const {
   prepareConformanceTable();
   return ConformanceTable->lookupConformance(
      module,
      const_cast<NominalTypeDecl *>(this),
      interface,
      conformances);
}

SmallVector<InterfaceDecl *, 2> NominalTypeDecl::getAllInterfaces() const {
   prepareConformanceTable();
   SmallVector<InterfaceDecl *, 2> result;
   ConformanceTable->getAllInterfaces(const_cast<NominalTypeDecl *>(this),
                                      result);
   return result;
}

SmallVector<InterfaceConformance *, 2> NominalTypeDecl::getAllConformances(
   bool sorted) const {
   prepareConformanceTable();
   SmallVector<InterfaceConformance *, 2> result;
   ConformanceTable->getAllConformances(const_cast<NominalTypeDecl *>(this),
                                        sorted,
                                        result);
   return result;
}

void NominalTypeDecl::getImplicitInterfaces(
   SmallVectorImpl<InterfaceDecl *> &interfaces) {
   prepareConformanceTable();
   ConformanceTable->getImplicitInterfaces(this, interfaces);
}

void NominalTypeDecl::registerInterfaceConformance(
   InterfaceConformance *conformance) {
   prepareConformanceTable();
   ConformanceTable->registerInterfaceConformance(conformance);
}

ArrayRef<ValueDecl *>
NominalTypeDecl::getSatisfiedInterfaceRequirementsForMember(
   const ValueDecl *member,
   bool sorted) const {
   assert(member->getDeclContext()->getSelfNominalTypeDecl() == this);
   assert(!isa<InterfaceDecl>(this));
   prepareConformanceTable();
   return ConformanceTable->getSatisfiedInterfaceRequirementsForMember(member,
                                                                       const_cast<NominalTypeDecl *>(this),
                                                                       sorted);
}

SmallVector<InterfaceDecl *, 2>
DeclContext::getLocalInterfaces(
   ConformanceLookupKind lookupKind,
   SmallVectorImpl<ConformanceDiagnostic> *diagnostics) const {
   SmallVector<InterfaceDecl *, 2> result;

   // Dig out the nominal type.
   NominalTypeDecl *nominal = getSelfNominalTypeDecl();
   if (!nominal)
      return result;

   // Update to record all potential conformances.
   nominal->prepareConformanceTable();
   nominal->ConformanceTable->lookupConformances(
      nominal,
      const_cast<DeclContext *>(this),
      lookupKind,
      &result,
      nullptr,
      diagnostics);

   return result;
}

SmallVector<InterfaceConformance *, 2>
DeclContext::getLocalConformances(
   ConformanceLookupKind lookupKind,
   SmallVectorImpl<ConformanceDiagnostic> *diagnostics) const {
   SmallVector<InterfaceConformance *, 2> result;

   // Dig out the nominal type.
   NominalTypeDecl *nominal = getSelfNominalTypeDecl();
   if (!nominal)
      return result;

   // Interfaces only have self-conformances.
   if (auto interface = dyn_cast<InterfaceDecl>(nominal)) {
      if (interface->requiresSelfConformanceWitnessTable())
         return {interface->getAstContext().getSelfConformance(interface)};
      return {};
   }

   // Update to record all potential conformances.
   nominal->prepareConformanceTable();
   nominal->ConformanceTable->lookupConformances(
      nominal,
      const_cast<DeclContext *>(this),
      lookupKind,
      nullptr,
      &result,
      diagnostics);

   return result;
}

/// Check of all types used by the conformance are canonical.
bool InterfaceConformance::isCanonical() const {
   // Normal conformances are always canonical by construction.
   if (getKind() == InterfaceConformanceKind::Normal)
      return true;

   if (!getType()->isCanonical())
      return false;

   switch (getKind()) {
      case InterfaceConformanceKind::Self:
      case InterfaceConformanceKind::Normal: {
         return true;
      }
      case InterfaceConformanceKind::Inherited: {
         // Substitute the base.
         auto inheritedConformance
            = cast<InheritedInterfaceConformance>(this);
         return inheritedConformance->getInheritedConformance()->isCanonical();
      }
      case InterfaceConformanceKind::Specialized: {
         // Substitute the substitutions in the specialized conformance.
         auto spec = cast<SpecializedInterfaceConformance>(this);
         auto genericConformance = spec->getGenericConformance();
         if (!genericConformance->isCanonical())
            return false;
         if (!spec->getSubstitutionMap().isCanonical()) return false;
         return true;
      }
   }
   llvm_unreachable("bad InterfaceConformanceKind");
}

/// Check of all types used by the conformance are canonical.
InterfaceConformance *InterfaceConformance::getCanonicalConformance() {
   if (isCanonical())
      return this;

   switch (getKind()) {
      case InterfaceConformanceKind::Self:
      case InterfaceConformanceKind::Normal: {
         // Root conformances are always canonical by construction.
         return this;
      }

      case InterfaceConformanceKind::Inherited: {
         auto &Ctx = getType()->getAstContext();
         auto inheritedConformance = cast<InheritedInterfaceConformance>(this);
         return Ctx.getInheritedConformance(
            getType()->getCanonicalType(),
            inheritedConformance->getInheritedConformance()
               ->getCanonicalConformance());
      }

      case InterfaceConformanceKind::Specialized: {
         auto &Ctx = getType()->getAstContext();
         // Substitute the substitutions in the specialized conformance.
         auto spec = cast<SpecializedInterfaceConformance>(this);
         auto genericConformance = spec->getGenericConformance();
         return Ctx.getSpecializedConformance(
            getType()->getCanonicalType(),
            genericConformance->getCanonicalConformance(),
            spec->getSubstitutionMap().getCanonical());
      }
   }
   llvm_unreachable("bad InterfaceConformanceKind");
}

/// Check of all types used by the conformance are canonical.
bool InterfaceConformanceRef::isCanonical() const {
   if (isAbstract() || isInvalid())
      return true;
   return getConcrete()->isCanonical();
}

InterfaceConformanceRef
InterfaceConformanceRef::getCanonicalConformanceRef() const {
   if (isAbstract() || isInvalid())
      return *this;
   return InterfaceConformanceRef(getConcrete()->getCanonicalConformance());
}

// See swift/Basic/Statistic.h for declaration: this enables tracing
// InterfaceConformances, is defined here to avoid too much layering violation /
// circular linkage dependency.

struct InterfaceConformanceTraceFormatter
   : public UnifiedStatsReporter::TraceFormatter {
   void traceName(const void *Entity, raw_ostream &OS) const {
      if (!Entity)
         return;
      const InterfaceConformance *C =
         static_cast<const InterfaceConformance *>(Entity);
      C->printName(OS);
   }

   void traceLoc(const void *Entity, SourceManager *SM,
                 clang::SourceManager *CSM, raw_ostream &OS) const {
      if (!Entity)
         return;
      const InterfaceConformance *C =
         static_cast<const InterfaceConformance *>(Entity);
      if (auto const *NPC = dyn_cast<NormalInterfaceConformance>(C)) {
         NPC->getLoc().print(OS, *SM);
      } else if (auto const *DC = C->getDeclContext()) {
         if (auto const *D = DC->getAsDecl())
            D->getLoc().print(OS, *SM);
      }
   }
};

void simple_display(llvm::raw_ostream &out,
                           const InterfaceConformance *conf) {
   conf->printName(out);
}

} // polar

namespace polar {

using namespace polar;

static InterfaceConformanceTraceFormatter TF;

template<>
const UnifiedStatsReporter::TraceFormatter *
FrontendStatsTracer::getTraceFormatter<const InterfaceConformance *>() {
   return &TF;
}
} // polar