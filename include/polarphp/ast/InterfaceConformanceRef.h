//===--- InterfaceConformanceRef.h - AST Interface Conformance ----*- C++ -*-===//
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
// This file defines the InterfaceConformanceRef type.
//
//===----------------------------------------------------------------------===//
#ifndef POLARPHP_AST_INTERFACE_CONFORMANCEREF_H
#define POLARPHP_AST_INTERFACE_CONFORMANCEREF_H

#include "polarphp/basic/Debug.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/PointerUnion.h"
#include "polarphp/ast/Requirement.h"
#include "polarphp/ast/TypeAlignments.h"
#include "polarphp/ast/Type.h"
#include "polarphp/ast/Identifier.h"

namespace llvm {
class raw_ostream;
}

namespace polar {

class ConcreteDeclRef;
class InterfaceConformance;

/// A InterfaceConformanceRef is a handle to a protocol conformance which
/// may be either concrete or abstract.
///
/// A concrete conformance is derived from a specific protocol conformance
/// declaration.
///
/// An abstract conformance is derived from context: the conforming type
/// is either existential or opaque (i.e. an archetype), and while the
/// type-checker promises that the conformance exists, it is not known
/// statically which concrete conformance it refers to.
///
/// InterfaceConformanceRef allows the efficient recovery of the protocol
/// even when the conformance is abstract.
class InterfaceConformanceRef
{
   using UnionType = llvm::PointerUnion<InterfaceDecl*, InterfaceConformance*>;
   UnionType Union;

   explicit InterfaceConformanceRef(UnionType value) : Union(value) {}

public:
   /// Create an abstract protocol conformance reference.
   explicit InterfaceConformanceRef(InterfaceDecl *proto) : Union(proto) {
      assert(proto != nullptr &&
            "cannot construct InterfaceConformanceRef with null");
   }

   /// Create a concrete protocol conformance reference.
   explicit InterfaceConformanceRef(InterfaceConformance *conf) : Union(conf) {
      assert(conf != nullptr &&
            "cannot construct InterfaceConformanceRef with null");
   }

   InterfaceConformanceRef(std::nullptr_t = nullptr)
      : Union((InterfaceDecl *)nullptr) {}

   static InterfaceConformanceRef forInvalid() {
      return InterfaceConformanceRef();
   }

   bool isInvalid() const {
      return !Union;
   }

   explicit operator bool() const { return !isInvalid(); }

   /// Create either a concrete or an abstract protocol conformance reference,
   /// depending on whether InterfaceConformance is null.
   explicit InterfaceConformanceRef(InterfaceDecl *protocol,
                                    InterfaceConformance *conf);

   bool isConcrete() const {
      return !isInvalid() && Union.is<InterfaceConformance*>();
   }

   InterfaceConformance *getConcrete() const {
      return Union.get<InterfaceConformance*>();
   }

   bool isAbstract() const {
      return !isInvalid() && Union.is<InterfaceDecl*>();
   }

   InterfaceDecl *getAbstract() const {
      return Union.get<InterfaceDecl*>();
   }

   using OpaqueValue = void*;
   OpaqueValue getOpaqueValue() const { return Union.getOpaqueValue(); }
   static InterfaceConformanceRef getFromOpaqueValue(OpaqueValue value) {
      return InterfaceConformanceRef(UnionType::getFromOpaqueValue(value));
   }

   /// Return the protocol requirement.
   InterfaceDecl *getRequirement() const;

   /// Apply a substitution to the conforming type.
   InterfaceConformanceRef subst(Type origType,
                                 SubstitutionMap subMap,
                                 SubstOptions options=None) const;

   /// Apply a substitution to the conforming type.
   InterfaceConformanceRef subst(Type origType,
                                 TypeSubstitutionFn subs,
                                 LookupConformanceFn conformances,
                                 SubstOptions options=None) const;

   /// Map contextual types to interface types in the conformance.
   InterfaceConformanceRef mapConformanceOutOfContext() const;

   /// Given a dependent type (expressed in terms of this conformance's
   /// protocol), follow it from the conforming type.
   Type getAssociatedType(Type origType, Type dependentType) const;

   /// Given a dependent type (expressed in terms of this conformance's
   /// protocol) and conformance, follow it from the conforming type.
   InterfaceConformanceRef
   getAssociatedConformance(Type origType, Type dependentType,
                            InterfaceDecl *requirement) const;

   POLAR_DEBUG_DUMP;
   void dump(llvm::raw_ostream &out, unsigned indent = 0) const;

   bool operator==(InterfaceConformanceRef other) const {
      return Union == other.Union;
   }
   bool operator!=(InterfaceConformanceRef other) const {
      return Union != other.Union;
   }

   friend llvm::hash_code hash_value(InterfaceConformanceRef conformance) {
      return llvm::hash_value(conformance.Union.getOpaqueValue());
   }

   Type getTypeWitnessByName(Type type, Identifier name) const;

   /// Find a particular named function witness for a type that conforms to
   /// the given protocol.
   ///
   /// \param type The conforming type.
   ///
   /// \param name The name of the requirement.
   ConcreteDeclRef getWitnessByName(Type type, DeclName name) const;

   /// Determine whether this conformance is canonical.
   bool isCanonical() const;

   /// Create a canonical conformance from the current one.
   InterfaceConformanceRef getCanonicalConformanceRef() const;

   /// Get any additional requirements that are required for this conformance to
   /// be satisfied, if they're possible to compute.
   Optional<ArrayRef<Requirement>> getConditionalRequirementsIfAvailable() const;

   /// Get any additional requirements that are required for this conformance to
   /// be satisfied.
   ArrayRef<Requirement> getConditionalRequirements() const;
};

} // end namespace polar

#endif // POLARPHP_AST_PROTOCOLCONFORMANCEREF_H
