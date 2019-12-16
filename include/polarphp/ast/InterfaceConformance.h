//===--- InterfaceConformance.h - AST Interface Conformance -------*- C++ -*-===//
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
// This file defines the protocol conformance data structures.
//
//===----------------------------------------------------------------------===//
#ifndef POLARPHP_AST_PROTOCOLCONFORMANCE_H
#define POLARPHP_AST_PROTOCOLCONFORMANCE_H

#include "polarphp/ast/ConcreteDeclRef.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Type.h"
#include "polarphp/ast/Types.h"
#include "polarphp/ast/TypeAlignments.h"
#include "polarphp/ast/Witness.h"
#include "polarphp/basic/Debug.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/TinyPtrVector.h"
#include <utility>

namespace polar {

class AstContext;
class DiagnosticEngine;
class GenericParamList;
class NormalInterfaceConformance;
class RootInterfaceConformance;
class InterfaceConformance;
class ModuleDecl;
class SubstitutableType;
enum class AllocationArena;

/// Type substitution mapping from substitutable types to their
/// replacements.
typedef llvm::DenseMap<SubstitutableType *, Type> TypeSubstitutionMap;

/// Map from non-type requirements to the corresponding conformance witnesses.
typedef llvm::DenseMap<ValueDecl *, Witness> WitnessMap;

/// Map from associated type requirements to the corresponding type and
/// the type declaration that was used to satisfy the requirement.
typedef llvm::DenseMap<AssociatedTypeDecl *, TypeWitnessAndDecl>
  TypeWitnessMap;

/// Describes the kind of protocol conformance structure used to encode
/// conformance.
enum class InterfaceConformanceKind {
  /// "Normal" conformance of a (possibly generic) nominal type, which
  /// contains complete mappings.
  Normal,
  /// Self-conformance of a protocol to itself.
  Self,
  /// Conformance for a specialization of a generic type, which projects the
  /// underlying generic conformance.
  Specialized,
  /// Conformance of a generic class type projected through one of its
  /// superclass's conformances.
  Inherited
};

/// Describes the state of a protocol conformance, which may be complete,
/// incomplete, or currently being checked.
enum class InterfaceConformanceState {
  /// The conformance has been fully checked.
  Complete,
  /// The conformance is known but is not yet complete.
  Incomplete,
  /// The conformance's type witnesses are currently being resolved.
  CheckingTypeWitnesses,
  /// The conformance is being checked.
  Checking,
};

/// Describes how a particular type conforms to a given protocol,
/// providing the mapping from the protocol members to the type (or extension)
/// members that provide the functionality for the concrete type.
///
/// InterfaceConformance is an abstract base class, implemented by subclasses
/// for the various kinds of conformance (normal, specialized, inherited).
class alignas(1 << DeclAlignInBits) InterfaceConformance {
  /// The kind of protocol conformance.
  InterfaceConformanceKind Kind;

  /// The type that conforms to the protocol, in the context of the
  /// conformance definition.
  Type ConformingType;

protected:
  InterfaceConformance(InterfaceConformanceKind kind, Type conformingType)
    : Kind(kind), ConformingType(conformingType) {}

public:
  /// Determine the kind of protocol conformance.
  InterfaceConformanceKind getKind() const { return Kind; }

  /// Get the conforming type.
  Type getType() const { return ConformingType; }

  /// Get the protocol being conformed to.
  InterfaceDecl *getInterface() const;

  /// Get the declaration context that contains the conforming extension or
  /// nominal type declaration.
  DeclContext *getDeclContext() const;

  /// Retrieve the state of this conformance.
  InterfaceConformanceState getState() const;

  /// Get the kind of source from which this conformance comes.
  ConformanceEntryKind getSourceKind() const;
  /// Get the protocol conformance which implied this implied conformance.
  NormalInterfaceConformance *getImplyingConformance() const;

  /// Determine whether this conformance is complete.
  bool isComplete() const {
    return getState() == InterfaceConformanceState::Complete;
  }

  /// Determine whether this conformance is invalid.
  bool isInvalid() const;

  /// Determine whether this conformance is incomplete.
  bool isIncomplete() const {
    return getState() == InterfaceConformanceState::Incomplete ||
           getState() == InterfaceConformanceState::CheckingTypeWitnesses ||
           getState() == InterfaceConformanceState::Checking;
  }

  /// Determine whether this conformance is canonical.
  bool isCanonical() const;

  /// Create a canonical conformance from the current one.
  /// If the current conformance is canonical already, it will be returned.
  /// Otherwise a new conformance will be created.
  InterfaceConformance *getCanonicalConformance();

  /// Return true if the conformance has a witness for the given associated
  /// type.
  bool hasTypeWitness(AssociatedTypeDecl *assocType) const;

  /// Retrieve the type witness for the given associated type.
  Type getTypeWitness(AssociatedTypeDecl *assocType,
                      SubstOptions options=None) const;

  /// Retrieve the type witness and type decl (if one exists)
  /// for the given associated type.
  TypeWitnessAndDecl
  getTypeWitnessAndDecl(AssociatedTypeDecl *assocType,
                        SubstOptions options=None) const;

  /// Apply the given function object to each type witness within this
  /// protocol conformance.
  ///
  /// The function object should accept an \c AssociatedTypeDecl* for the
  /// requirement followed by the \c Type for the witness and a
  /// (possibly null) \c TypeDecl* that explicitly declared the type.
  /// It should return true to indicate an early exit.
  ///
  /// \returns true if the function ever returned true
  template<typename F>
  bool forEachTypeWitness(F f, bool useResolver=false) const {
    const InterfaceDecl *protocol = getInterface();
    for (auto assocTypeReq : protocol->getAssociatedTypeMembers()) {
      if (assocTypeReq->isInvalid())
        continue;

      // If we don't have and cannot resolve witnesses, skip it.
      if (!useResolver && !hasTypeWitness(assocTypeReq))
        continue;

      const auto &TWInfo = getTypeWitnessAndDecl(assocTypeReq);
      if (f(assocTypeReq, TWInfo.getWitnessType(), TWInfo.getWitnessDecl()))
        return true;
    }

    return false;
  }

  /// Retrieve the value witness declaration corresponding to the given
  /// requirement.
  ValueDecl *getWitnessDecl(ValueDecl *requirement) const;

  /// Retrieve the witness corresponding to the given value requirement.
  /// TODO: maybe this should return a Witness?
  ConcreteDeclRef getWitnessDeclRef(ValueDecl *requirement) const;

private:
  /// Determine whether we have a witness for the given requirement.
  bool hasWitness(ValueDecl *requirement) const;

public:
  /// Apply the given function object to each requirement, either type or value,
  /// that is not witnessed.
  ///
  /// The function object should accept a \c ValueDecl* for the requirement.
  template<typename F>
  void forEachNonWitnessedRequirement(F f) const {
    const InterfaceDecl *protocol = getInterface();
    for (auto req : protocol->getMembers()) {
      auto valueReq = dyn_cast<ValueDecl>(req);
      if (!valueReq || valueReq->isInvalid())
        continue;

      if (auto assocTypeReq = dyn_cast<AssociatedTypeDecl>(req)) {
        // If we don't have witness for the associated type, apply the function.
        if (getTypeWitness(assocTypeReq)->hasError()) {
          f(valueReq);
        }
        continue;
      }

      if (!valueReq->isInterfaceRequirement())
        continue;

      // If we don't have witness for the value, apply the function.
      if (!hasWitness(valueReq)) {
        f(valueReq);
      }
    }
  }

  /// Retrieve the protocol conformance for the inherited protocol.
  InterfaceConformance *getInheritedConformance(InterfaceDecl *protocol) const;

  /// Given a dependent type expressed in terms of the self parameter,
  /// map it into the context of this conformance.
  Type getAssociatedType(Type assocType) const;

  /// Given that the requirement signature of the protocol directly states
  /// that the given dependent type must conform to the given protocol,
  /// return its associated conformance.
  InterfaceConformanceRef
  getAssociatedConformance(Type assocType, InterfaceDecl *protocol) const;

  /// Get the generic parameters open on the conforming type.
  GenericEnvironment *getGenericEnvironment() const;

  /// Get the generic signature containing the parameters open on the conforming
  /// interface type.
  GenericSignature getGenericSignature() const;

  /// Get the substitutions associated with this conformance.
  SubstitutionMap getSubstitutions(ModuleDecl *M) const;

  /// Get the underlying normal conformance.
  /// FIXME: remove uses of this.
  const NormalInterfaceConformance *getRootNormalConformance() const;

  /// Get the underlying normal conformance.
  NormalInterfaceConformance *getRootNormalConformance() {
    return const_cast<NormalInterfaceConformance *>(
             const_cast<const InterfaceConformance *>(this)
               ->getRootNormalConformance());
  }

  /// Get the underlying root conformance.
  const RootInterfaceConformance *getRootConformance() const;

  /// Get the underlying root conformance.
  RootInterfaceConformance *getRootConformance() {
    return const_cast<RootInterfaceConformance *>(
      const_cast<const InterfaceConformance *>(this)->getRootConformance());
  }

  /// Determine whether this protocol conformance is visible from the
  /// given declaration context.
  bool isVisibleFrom(const DeclContext *dc) const;

  /// Determine whether the witness for the given requirement
  /// is either the default definition or was otherwise deduced.
  bool usesDefaultDefinition(AssociatedTypeDecl *requirement) const;

  // Make vanilla new/delete illegal for protocol conformances.
  void *operator new(size_t bytes) = delete;
  void operator delete(void *data) POLAR_DELETE_OPERATOR_DELETED;

  // Only allow allocation of protocol conformances using the allocator in
  // AstContext or by doing a placement new.
  void *operator new(size_t bytes, AstContext &context,
                     AllocationArena arena,
                     unsigned alignment = alignof(InterfaceConformance));
  void *operator new(size_t bytes, void *mem) {
    assert(mem);
    return mem;
  }

  /// Print a parseable and human-readable description of the identifying
  /// information of the protocol conformance.
  void printName(raw_ostream &os,
                 const PrintOptions &PO = PrintOptions()) const;

  /// Get any additional requirements that are required for this conformance to
  /// be satisfied, if it is possible for them to be computed.
  Optional<ArrayRef<Requirement>> getConditionalRequirementsIfAvailable() const;

  /// Get any additional requirements that are required for this conformance to
  /// be satisfied.
  ArrayRef<Requirement> getConditionalRequirements() const;

  /// Substitute the conforming type and produce a InterfaceConformance that
  /// applies to the substituted type.
  InterfaceConformance *subst(SubstitutionMap subMap,
                             SubstOptions options=None) const;

  /// Substitute the conforming type and produce a InterfaceConformance that
  /// applies to the substituted type.
  InterfaceConformance *subst(TypeSubstitutionFn subs,
                             LookupConformanceFn conformances,
                             SubstOptions options=None) const;

  POLAR_DEBUG_DUMP;
  void dump(llvm::raw_ostream &out, unsigned indent = 0) const;
};

/// A "root" protocol conformance states some sort of ground truth
/// about the conforming type and the required protocol.  Either:
///
/// - the type is directly declared to conform to the protocol (a
///   normal conformance) or
/// - the protocol's existential type is known to conform to itself (a
///   self-conformance).
class RootInterfaceConformance : public InterfaceConformance {
protected:
  RootInterfaceConformance(InterfaceConformanceKind kind, Type conformingType)
    : InterfaceConformance(kind, conformingType) {}

public:
  /// Retrieve the location of this conformance.
  SourceLoc getLoc() const;

  bool isInvalid() const;

  /// Whether this conformance is weak-imported.
  bool isWeakImported(ModuleDecl *fromModule) const;

  bool hasWitness(ValueDecl *requirement) const;
  Witness getWitness(ValueDecl *requirement) const;

  /// Retrieve the witness corresponding to the given value requirement.
  /// TODO: maybe this should return a Witness?
  ConcreteDeclRef getWitnessDeclRef(ValueDecl *requirement) const;

  /// Apply the given function object to each value witness within this
  /// protocol conformance.
  ///
  /// The function object should accept a \c ValueDecl* for the requirement
  /// followed by the \c Witness for the witness. Note that a generic
  /// witness will only be specialized if the conformance came from the current
  /// file.
  template<typename F>
  void forEachValueWitness(F f, bool useResolver=false) const {
    const InterfaceDecl *protocol = getInterface();
    for (auto req : protocol->getMembers()) {
      auto valueReq = dyn_cast<ValueDecl>(req);
      if (!valueReq || isa<AssociatedTypeDecl>(valueReq) ||
          valueReq->isInvalid())
        continue;

      if (!valueReq->isInterfaceRequirement())
        continue;

      // If we don't have and cannot resolve witnesses, skip it.
      if (!useResolver && !hasWitness(valueReq))
        continue;

      f(valueReq, getWitness(valueReq));
    }
  }

  static bool classof(const InterfaceConformance *conformance) {
    return conformance->getKind() == InterfaceConformanceKind::Normal ||
           conformance->getKind() == InterfaceConformanceKind::Self;
  }
};

/// Normal protocol conformance, which involves mapping each of the protocol
/// requirements to a witness.
///
/// Normal protocol conformance is used for the explicit conformances placed on
/// nominal types and extensions. For example:
///
/// \code
/// protocol P { func foo() }
/// struct A : P { func foo() { } }
/// class B<T> : P { func foo() { } }
/// \endcode
///
/// Here, there is a normal protocol conformance for both \c A and \c B<T>,
/// providing the witnesses \c A.foo and \c B<T>.foo, respectively, for the
/// requirement \c foo.
class NormalInterfaceConformance : public RootInterfaceConformance,
                                  public llvm::FoldingSetNode
{
  friend class ValueWitnessRequest;
  friend class TypeWitnessRequest;

  /// The protocol being conformed to and its current state.
  llvm::PointerIntPair<InterfaceDecl *, 2, InterfaceConformanceState>
    InterfaceAndState;

  /// The location of this protocol conformance in the source.
  SourceLoc Loc;

  /// The declaration context containing the ExtensionDecl or
  /// NominalTypeDecl that declared the conformance.
  ///
  /// Also stores the "invalid" bit.
  llvm::PointerIntPair<DeclContext *, 1, bool> ContextAndInvalid;

  /// The reason that this conformance exists.
  ///
  /// Either Explicit (e.g. 'struct Foo: Interface {}' or 'extension Foo:
  /// Interface {}'), Synthesized (e.g. RawRepresentable for 'enum Foo: Int {}')
  /// or Implied (e.g. 'Foo : Interface' in 'protocol Other: Interface {} struct
  /// Foo: Other {}'). In only the latter case, the conformance is non-null and
  /// points to the conformance that implies this one.
  ///
  /// This should never be Inherited: that is handled by
  /// InheritedInterfaceConformance.
  llvm::PointerIntPair<NormalInterfaceConformance *, 2, ConformanceEntryKind>
      SourceKindAndImplyingConformance = {nullptr,
                                          ConformanceEntryKind::Explicit};

  /// The mapping of individual requirements in the protocol over to
  /// the declarations that satisfy those requirements.
  mutable WitnessMap Mapping;

  /// The mapping from associated type requirements to their types.
  mutable TypeWitnessMap TypeWitnesses;

  /// Conformances that satisfy each of conformance requirements of the
  /// requirement signature of the protocol.
  ArrayRef<InterfaceConformanceRef> SignatureConformances;

  /// Any additional requirements that are required for this conformance to
  /// apply, e.g. 'Something: Baz' in 'extension Foo: Bar where Something: Baz'.
  mutable ArrayRef<Requirement> ConditionalRequirements;
  enum class ConditionalRequirementsState {
    Uncomputed,
    Computing,
    Complete,
  };
  /// The state of the ConditionalRequirements field: whether it has been
  /// computed or not.
  mutable ConditionalRequirementsState CRState =
      ConditionalRequirementsState::Uncomputed;

  /// The lazy member loader provides callbacks for populating imported and
  /// deserialized conformances.
  ///
  /// This is not use for parsed conformances -- those are lazily populated
  /// by the AstContext's LazyResolver, which is really a Sema instance.
  LazyConformanceLoader *Loader = nullptr;
  uint64_t LoaderContextData;
  friend class AstContext;

  NormalInterfaceConformance(Type conformingType, InterfaceDecl *protocol,
                            SourceLoc loc, DeclContext *dc,
                            InterfaceConformanceState state)
    : RootInterfaceConformance(InterfaceConformanceKind::Normal, conformingType),
      InterfaceAndState(protocol, state), Loc(loc), ContextAndInvalid(dc, false)
  {
    assert(!conformingType->hasArchetype() &&
           "InterfaceConformances should store interface types");
  }

  void resolveLazyInfo() const;

  void differenceAndStoreConditionalRequirements() const;

public:
  /// Get the protocol being conformed to.
  InterfaceDecl *getInterface() const { return InterfaceAndState.getPointer(); }

  /// Retrieve the location of this
  SourceLoc getLoc() const { return Loc; }

  /// Get the declaration context that contains the conforming extension or
  /// nominal type declaration.
  DeclContext *getDeclContext() const {
    return ContextAndInvalid.getPointer();
  }

  /// Get any additional requirements that are required for this conformance to
  /// be satisfied if they can be computed.
  ///
  /// If \c computeIfPossible is false, this will not do the lazy computation of
  /// the conditional requirements and will just query the current state. This
  /// should almost certainly only be used for debugging purposes, prefer \c
  /// getConditionalRequirementsIfAvailable (these are separate because
  /// CONFORMANCE_SUBCLASS_DISPATCH does some type checks and a defaulted
  /// parameter gets in the way of that).
  Optional<ArrayRef<Requirement>>
  getConditionalRequirementsIfAvailableOrCached(bool computeIfPossible) const {
    if (computeIfPossible)
      differenceAndStoreConditionalRequirements();

    if (CRState == ConditionalRequirementsState::Complete)
      return ConditionalRequirements;

    return None;
  }
  /// Get any additional requirements that are required for this conformance to
  /// be satisfied if they can be computed.
  Optional<ArrayRef<Requirement>>
  getConditionalRequirementsIfAvailable() const {
    return getConditionalRequirementsIfAvailableOrCached(
        /*computeIfPossible=*/true);
  }

  /// Get any additional requirements that are required for this conformance to
  /// be satisfied, e.g. for Array<T>: Equatable, T: Equatable also needs
  /// to be satisfied.
  ArrayRef<Requirement> getConditionalRequirements() const {
    return *getConditionalRequirementsIfAvailable();
  }

  /// Retrieve the state of this conformance.
  InterfaceConformanceState getState() const {
    return InterfaceAndState.getInt();
  }

  /// Set the state of this conformance.
  void setState(InterfaceConformanceState state) {
    InterfaceAndState.setInt(state);
  }

  /// Determine whether this conformance is invalid.
  bool isInvalid() const {
    return ContextAndInvalid.getInt();
  }

  /// Mark this conformance as invalid.
  void setInvalid() {
    ContextAndInvalid.setInt(true);
    SignatureConformances = {};
  }

  /// Get the kind of source from which this conformance comes.
  ConformanceEntryKind getSourceKind() const {
    return SourceKindAndImplyingConformance.getInt();
  }

  /// Get the protocol conformance which implied this implied conformance.
  NormalInterfaceConformance *getImplyingConformance() const {
    assert(getSourceKind() == ConformanceEntryKind::Implied);
    return SourceKindAndImplyingConformance.getPointer();
  }

  void setSourceKindAndImplyingConformance(
      ConformanceEntryKind sourceKind,
      NormalInterfaceConformance *implyingConformance) {
    assert(sourceKind != ConformanceEntryKind::Inherited &&
           "a normal conformance cannot be inherited");
    assert((sourceKind == ConformanceEntryKind::Implied) ==
               (bool)implyingConformance &&
           "an implied conformance needs something that implies it");
    SourceKindAndImplyingConformance = {implyingConformance, sourceKind};
  }

  /// Determine whether this conformance is lazily loaded.
  ///
  /// This only matters to the AST verifier.
  bool isLazilyLoaded() const { return Loader != nullptr; }

  /// A "retroactive" conformance is one that is defined in a module that
  /// is neither the module that defines the protocol nor the module that
  /// defines the conforming type.
  bool isRetroactive() const;

  /// Whether this conformance was synthesized automatically in multiple
  /// modules, but in a manner that ensures that all copies are equivalent.
  bool isSynthesizedNonUnique() const;

  /// Whether clients from outside the module can rely on the value witnesses
  /// being consistent across versions of the framework.
  bool isResilient() const;

  /// Retrieve the type witness and type decl (if one exists)
  /// for the given associated type.
  TypeWitnessAndDecl
  getTypeWitnessAndDecl(AssociatedTypeDecl *assocType,
                        SubstOptions options=None) const;

  TypeWitnessAndDecl
  getTypeWitnessUncached(AssociatedTypeDecl *requirement) const;

  /// Determine whether the protocol conformance has a type witness for the
  /// given associated type.
  bool hasTypeWitness(AssociatedTypeDecl *assocType) const;

  /// Set the type witness for the given associated type.
  /// \param typeDecl the type decl the witness type came from, if one exists.
  void setTypeWitness(AssociatedTypeDecl *assocType, Type type,
                      TypeDecl *typeDecl) const;

  /// Given that the requirement signature of the protocol directly states
  /// that the given dependent type must conform to the given protocol,
  /// return its associated conformance.
  InterfaceConformanceRef
  getAssociatedConformance(Type assocType, InterfaceDecl *protocol) const;

  /// Retrieve the value witness corresponding to the given requirement.
  Witness getWitness(ValueDecl *requirement) const;

  Witness getWitnessUncached(ValueDecl *requirement) const;

  /// Determine whether the protocol conformance has a witness for the given
  /// requirement.
  bool hasWitness(ValueDecl *requirement) const {
    if (Loader)
      resolveLazyInfo();
    return Mapping.count(requirement) > 0;
  }

  /// Set the witness for the given requirement.
  void setWitness(ValueDecl *requirement, Witness witness) const;

  /// Retrieve the protocol conformances that satisfy the requirements of the
  /// protocol, which line up with the conformance constraints in the
  /// protocol's requirement signature.
  ArrayRef<InterfaceConformanceRef> getSignatureConformances() const {
    if (Loader)
      resolveLazyInfo();
    return SignatureConformances;
  }

  /// Copy the given protocol conformances for the requirement signature into
  /// the normal conformance.
  void setSignatureConformances(ArrayRef<InterfaceConformanceRef> conformances);

  /// Populate the signature conformances without checking if they satisfy
  /// requirements. Can only be used with parsed or imported conformances.
  void finishSignatureConformances();

  /// Determine whether the witness for the given type requirement
  /// is the default definition.
  bool usesDefaultDefinition(AssociatedTypeDecl *requirement) const {
    TypeDecl *witnessDecl = getTypeWitnessAndDecl(requirement).getWitnessDecl();
    if (witnessDecl)
      return witnessDecl->isImplicit();
    // Conservatively assume it does not.
    return false;
  }

  void setLazyLoader(LazyConformanceLoader *resolver, uint64_t contextData);

  void Profile(llvm::FoldingSetNodeID &ID) {
    Profile(ID, getInterface(), getDeclContext());
  }

  static void Profile(llvm::FoldingSetNodeID &ID, InterfaceDecl *protocol,
                      DeclContext *dc) {
    ID.AddPointer(protocol);
    ID.AddPointer(dc);
  }

  static bool classof(const InterfaceConformance *conformance) {
    return conformance->getKind() == InterfaceConformanceKind::Normal;
  }
};

/// The conformance of a protocol to itself.
///
/// For now, we generally do not use this type in InterfaceConformanceRefs;
/// it's only used to anchor structures relating to emitting witness tables
/// for self-conformances.
class SelfInterfaceConformance : public RootInterfaceConformance {
  friend class AstContext;

  SelfInterfaceConformance(Type conformingType)
    : RootInterfaceConformance(InterfaceConformanceKind::Self, conformingType) {
  }

public:
  /// Get the protocol being conformed to.
  InterfaceDecl *getInterface() const {
    return getType()->castTo<InterfaceType>()->getDecl();
  }

  /// Get the declaration context in which this conformance was declared.
  DeclContext *getDeclContext() const {
    return getInterface();
  }

  /// Retrieve the location of this conformance.
  SourceLoc getLoc() const {
    return getInterface()->getLoc();
  }

  InterfaceConformanceState getState() const {
    return InterfaceConformanceState::Complete;
  }

  bool isInvalid() const {
    return false;
  }

  ConformanceEntryKind getSourceKind() const {
    return ConformanceEntryKind::Explicit; // FIXME?
  }

  NormalInterfaceConformance *getImplyingConformance() const {
    llvm_unreachable("never an implied conformance");
  }

  bool hasTypeWitness(AssociatedTypeDecl *assocType) const {
    llvm_unreachable("self-conformances never have associated types");
  }

  TypeWitnessAndDecl
  getTypeWitnessAndDecl(AssociatedTypeDecl *assocType,
                        SubstOptions options=None) const {
    llvm_unreachable("self-conformances never have associated types");
  }

  Type getTypeWitness(AssociatedTypeDecl *assocType,
                      SubstOptions options=None) const {
    llvm_unreachable("self-conformances never have associated types");
  }

  bool usesDefaultDefinition(AssociatedTypeDecl *requirement) const {
    llvm_unreachable("self-conformances never have associated types");
  }

  InterfaceConformanceRef getAssociatedConformance(Type assocType,
                                                  InterfaceDecl *protocol) const{
    llvm_unreachable("self-conformances never have associated types");
  }

  bool hasWitness(ValueDecl *requirement) const {
    return true;
  }
  Witness getWitness(ValueDecl *requirement) const;

  Optional<ArrayRef<Requirement>> getConditionalRequirementsIfAvailable() const{
    return ArrayRef<Requirement>();
  }

  /// Get any additional requirements that are required for this conformance to
  /// be satisfied.
  ArrayRef<Requirement> getConditionalRequirements() const {
    return ArrayRef<Requirement>();
  }

  static bool classof(const InterfaceConformance *conformance) {
    return conformance->getKind() == InterfaceConformanceKind::Self;
  }
};

/// Specialized protocol conformance, which projects a generic protocol
/// conformance to one of the specializations of the generic type.
///
/// For example:
/// \code
/// protocol P { func foo() }
/// class A<T> : P { func foo() { } }
/// \endcode
///
/// \c A<T> conforms to \c P via normal protocol conformance. Any specialization
/// of \c A<T> conforms to \c P via a specialized protocol conformance. For
/// example, \c A<Int> conforms to \c P via a specialized protocol conformance
/// that refers to the normal protocol conformance \c A<T> to \c P with the
/// substitution \c T -> \c Int.
class SpecializedInterfaceConformance : public InterfaceConformance,
                                       public llvm::FoldingSetNode {
  /// The generic conformance from which this conformance was derived.
  InterfaceConformance *GenericConformance;

  /// The substitutions applied to the generic conformance to produce this
  /// conformance.
  SubstitutionMap GenericSubstitutions;

  /// The mapping from associated type requirements to their substitutions.
  ///
  /// This mapping is lazily produced by specializing the underlying,
  /// generic conformance.
  mutable TypeWitnessMap TypeWitnesses;

  /// Any conditional requirements, in substituted form. (E.g. given Foo<T>: Bar
  /// where T: Bar, Foo<Baz<U>> will include Baz<U>: Bar.)
  mutable Optional<ArrayRef<Requirement>> ConditionalRequirements;

  friend class AstContext;

  SpecializedInterfaceConformance(Type conformingType,
                                 InterfaceConformance *genericConformance,
                                 SubstitutionMap substitutions);

  void computeConditionalRequirements() const;

public:
  /// Get the generic conformance from which this conformance was derived,
  /// if there is one.
  InterfaceConformance *getGenericConformance() const {
    return GenericConformance;
  }

  /// Get the substitution map representing the substitutions used to produce
  /// this specialized conformance.
  SubstitutionMap getSubstitutionMap() const { return GenericSubstitutions; }

  /// Get any requirements that must be satisfied for this conformance to apply.
  ///
  /// If \c computeIfPossible is false, this will not do the lazy computation of
  /// the conditional requirements and will just query the current state. This
  /// should almost certainly only be used for debugging purposes, prefer \c
  /// getConditionalRequirementsIfAvailable (these are separate because
  /// CONFORMANCE_SUBCLASS_DISPATCH does some type checks and a defaulted
  /// parameter gets in the way of that).
  Optional<ArrayRef<Requirement>>
  getConditionalRequirementsIfAvailableOrCached(bool computeIfPossible) const {
    if (computeIfPossible)
      computeConditionalRequirements();
    return ConditionalRequirements;
  }
  Optional<ArrayRef<Requirement>>
  getConditionalRequirementsIfAvailable() const {
    return getConditionalRequirementsIfAvailableOrCached(
        /*computeIfPossible=*/true);
  }

  /// Get any requirements that must be satisfied for this conformance to apply.
  ArrayRef<Requirement> getConditionalRequirements() const {
    return *getConditionalRequirementsIfAvailable();
  }

  /// Get the protocol being conformed to.
  InterfaceDecl *getInterface() const {
    return GenericConformance->getInterface();
  }

  /// Get the declaration context that contains the conforming extension or
  /// nominal type declaration.
  DeclContext *getDeclContext() const {
    return GenericConformance->getDeclContext();
  }

  /// Retrieve the state of this conformance.
  InterfaceConformanceState getState() const {
    return GenericConformance->getState();
  }

  /// Get the kind of source from which this conformance comes.
  ConformanceEntryKind getSourceKind() const {
    return GenericConformance->getSourceKind();
  }
  /// Get the protocol conformance which implied this implied conformance.
  NormalInterfaceConformance *getImplyingConformance() const {
    return GenericConformance->getImplyingConformance();
  }

  bool hasTypeWitness(AssociatedTypeDecl *assocType) const;

  /// Retrieve the type witness and type decl (if one exists)
  /// for the given associated type.
  TypeWitnessAndDecl
  getTypeWitnessAndDecl(AssociatedTypeDecl *assocType,
                        SubstOptions options=None) const;

  /// Given that the requirement signature of the protocol directly states
  /// that the given dependent type must conform to the given protocol,
  /// return its associated conformance.
  InterfaceConformanceRef
  getAssociatedConformance(Type assocType, InterfaceDecl *protocol) const;

  /// Retrieve the witness corresponding to the given value requirement.
  ConcreteDeclRef getWitnessDeclRef(ValueDecl *requirement) const;

  /// Determine whether the witness for the given requirement
  /// is either the default definition or was otherwise deduced.
  bool usesDefaultDefinition(AssociatedTypeDecl *requirement) const {
    return GenericConformance->usesDefaultDefinition(requirement);
  }

  void Profile(llvm::FoldingSetNodeID &ID) {
    Profile(ID, getType(), getGenericConformance(), getSubstitutionMap());
  }

  static void Profile(llvm::FoldingSetNodeID &ID, Type type,
                      InterfaceConformance *genericConformance,
                      SubstitutionMap subs) {
    ID.AddPointer(type.getPointer());
    ID.AddPointer(genericConformance);
    subs.profile(ID);
  }

  static bool classof(const InterfaceConformance *conformance) {
    return conformance->getKind() == InterfaceConformanceKind::Specialized;
  }
};

/// Inherited protocol conformance, which projects the conformance of a
/// superclass to its subclasses.
///
/// An example:
/// \code
/// protocol P { func foo() }
/// class A : P { func foo() { } }
/// class B : A { }
/// \endcode
///
/// \c A conforms to \c P via normal protocol conformance. The subclass \c B
/// of \c A conforms to \c P via an inherited protocol conformance.
class InheritedInterfaceConformance : public InterfaceConformance,
                                     public llvm::FoldingSetNode {
  /// The conformance inherited from the superclass.
  InterfaceConformance *InheritedConformance;

  friend class AstContext;

  InheritedInterfaceConformance(Type conformingType,
                               InterfaceConformance *inheritedConformance)
    : InterfaceConformance(InterfaceConformanceKind::Inherited, conformingType),
      InheritedConformance(inheritedConformance)
  {
  }

public:
  /// Retrieve the conformance for the inherited type.
  InterfaceConformance *getInheritedConformance() const {
    return InheritedConformance;
  }

  /// Get the protocol being conformed to.
  InterfaceDecl *getInterface() const {
    return InheritedConformance->getInterface();
  }

  /// Get any requirements that must be satisfied for this conformance to apply.
  Optional<ArrayRef<Requirement>>
  getConditionalRequirementsIfAvailable() const {
    return InheritedConformance->getConditionalRequirementsIfAvailable();
  }

  /// Get any requirements that must be satisfied for this conformance to apply.
  ArrayRef<Requirement> getConditionalRequirements() const {
    return InheritedConformance->getConditionalRequirements();
  }

  /// Get the declaration context that contains the conforming extension or
  /// nominal type declaration.
  DeclContext *getDeclContext() const {
    auto bgc = getType()->getClassOrBoundGenericClass();

    // In some cases, we may not have a BGC handy, in which case we should
    // delegate to the inherited conformance for the decl context.
    return bgc ? bgc : InheritedConformance->getDeclContext();
  }

  /// Retrieve the state of this conformance.
  InterfaceConformanceState getState() const {
    return InheritedConformance->getState();
  }

  /// Get the kind of source from which this conformance comes.
  ConformanceEntryKind getSourceKind() const {
    return ConformanceEntryKind::Inherited;
  }
  /// Get the protocol conformance which implied this implied conformance.
  NormalInterfaceConformance *getImplyingConformance() const { return nullptr; }

  bool hasTypeWitness(AssociatedTypeDecl *assocType) const {
    return InheritedConformance->hasTypeWitness(assocType);
  }

  /// Retrieve the type witness and type decl (if one exists)
  /// for the given associated type.
  TypeWitnessAndDecl
  getTypeWitnessAndDecl(AssociatedTypeDecl *assocType,
                        SubstOptions options=None) const {
    return InheritedConformance->getTypeWitnessAndDecl(assocType, options);
  }

  /// Given that the requirement signature of the protocol directly states
  /// that the given dependent type must conform to the given protocol,
  /// return its associated conformance.
  InterfaceConformanceRef
  getAssociatedConformance(Type assocType, InterfaceDecl *protocol) const;

  /// Retrieve the witness corresponding to the given value requirement.
  ConcreteDeclRef getWitnessDeclRef(ValueDecl *requirement) const;

  /// Determine whether the witness for the given requirement
  /// is either the default definition or was otherwise deduced.
  bool usesDefaultDefinition(AssociatedTypeDecl *requirement) const {
    return InheritedConformance->usesDefaultDefinition(requirement);
  }

  void Profile(llvm::FoldingSetNodeID &ID) {
    Profile(ID, getType(), getInheritedConformance());
  }

  static void Profile(llvm::FoldingSetNodeID &ID, Type type,
                      InterfaceConformance *inheritedConformance) {
    ID.AddPointer(type.getPointer());
    ID.AddPointer(inheritedConformance);
  }

  static bool classof(const InterfaceConformance *conformance) {
    return conformance->getKind() == InterfaceConformanceKind::Inherited;
  }
};

inline bool InterfaceConformance::isInvalid() const {
  return getRootConformance()->isInvalid();
}

inline bool InterfaceConformance::hasWitness(ValueDecl *requirement) const {
  return getRootConformance()->hasWitness(requirement);
}

void simple_display(llvm::raw_ostream &out, const InterfaceConformance *conf);

} // end namespace swift

#endif // LLVM_POLARPHP_AST_PROTOCOLCONFORMANCE_H
