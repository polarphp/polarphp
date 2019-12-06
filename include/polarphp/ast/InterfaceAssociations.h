//===--- InterfaceAssociations.h - Associated types/conformances -*- C++ -*-===//
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
// This file defines types for representing types and conformances
// associated with a protocol.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_PROTOCOLASSOCIATIONS_H
#define POLARPHP_AST_PROTOCOLASSOCIATIONS_H

#include "polarphp/ast/Decl.h"
#include "llvm/ADT/DenseMapInfo.h"

namespace polar::ast {

/// A type associated with a protocol.
///
/// This struct exists largely so that we can maybe eventually
/// generalize it to an arbitrary path.
class AssociatedType
{
   AssociatedTypeDecl *Association;
   using AssociationInfo = llvm::DenseMapInfo<AssociatedTypeDecl*>;

   struct SpecialValue {};
   explicit AssociatedType(SpecialValue _, AssociatedTypeDecl *specialValue)
      : Association(specialValue) {}

public:
   explicit AssociatedType(AssociatedTypeDecl *association)
      : Association(association) {
      assert(association);
   }

   InterfaceDecl *getSourceInterface() const
   {
      return Association->getInterface();
   }

   AssociatedTypeDecl *getAssociation() const {
      return Association;
   }

   friend bool operator==(AssociatedType lhs, AssociatedType rhs) {
      return lhs.Association == rhs.Association;
   }
   friend bool operator!=(AssociatedType lhs, AssociatedType rhs) {
      return !(lhs == rhs);
   }

   unsigned getHashValue() const {
      return llvm::hash_value(Association);
   }

   static AssociatedType getEmptyKey() {
      return AssociatedType(SpecialValue(), AssociationInfo::getEmptyKey());
   }
   static AssociatedType getTombstoneKey() {
      return AssociatedType(SpecialValue(), AssociationInfo::getTombstoneKey());
   }
};

/// A base conformance of a protocol.
class BaseConformance {
   InterfaceDecl *Source;
   InterfaceDecl *Requirement;

public:
   explicit BaseConformance(InterfaceDecl *source,
                            InterfaceDecl *requirement)
      : Source(source), Requirement(requirement) {
      assert(source && requirement);
   }

   InterfaceDecl *getSourceInterface() const {
      return Source;
   }

   InterfaceDecl *getBaseRequirement() const {
      return Requirement;
   }
};

/// A conformance associated with a protocol.
class AssociatedConformance {
   InterfaceDecl *Source;
   CanType Association;
   InterfaceDecl *Requirement;

   using SourceInfo = llvm::DenseMapInfo<InterfaceDecl*>;

   explicit AssociatedConformance(InterfaceDecl *specialValue)
      : Source(specialValue), Association(CanType()), Requirement(nullptr) {}

public:
   explicit AssociatedConformance(InterfaceDecl *source, CanType association,
                                  InterfaceDecl *requirement)
      : Source(source), Association(association), Requirement(requirement) {
      assert(source && association && requirement);
   }

   InterfaceDecl *getSourceInterface() const {
      return Source;
   }

   CanType getAssociation() const {
      return Association;
   }

   InterfaceDecl *getAssociatedRequirement() const {
      return Requirement;
   }

   friend bool operator==(const AssociatedConformance &lhs,
                          const AssociatedConformance &rhs) {
      return lhs.Source == rhs.Source &&
            lhs.Association == rhs.Association &&
            lhs.Requirement == rhs.Requirement;
   }
   friend bool operator!=(const AssociatedConformance &lhs,
                          const AssociatedConformance &rhs) {
      return !(lhs == rhs);
   }

   unsigned getHashValue() const {
      return hash_value(llvm::hash_combine(Source,
                                           Association.getPointer(),
                                           Requirement));
   }

   static AssociatedConformance getEmptyKey() {
      return AssociatedConformance(SourceInfo::getEmptyKey());
   }
   static AssociatedConformance getTombstoneKey() {
      return AssociatedConformance(SourceInfo::getTombstoneKey());
   }
};

} // end namespace swift

namespace llvm {
template <> struct DenseMapInfo<polar::ast::AssociatedType> {
   static inline polar::ast::AssociatedType getEmptyKey() {
      return polar::ast::AssociatedType::getEmptyKey();
   }

   static inline polar::ast::AssociatedType getTombstoneKey() {
      return polar::ast::AssociatedType::getTombstoneKey();
   }

   static unsigned getHashValue(polar::ast::AssociatedType val) {
      return val.getHashValue();
   }

   static bool isEqual(polar::ast::AssociatedType lhs, polar::ast::AssociatedType rhs) {
      return lhs == rhs;
   }
};

template <> struct DenseMapInfo<polar::ast::AssociatedConformance> {
   static inline polar::ast::AssociatedConformance getEmptyKey() {
      return polar::ast::AssociatedConformance::getEmptyKey();
   }

   static inline polar::ast::AssociatedConformance getTombstoneKey() {
      return polar::ast::AssociatedConformance::getTombstoneKey();
   }

   static unsigned getHashValue(polar::ast::AssociatedConformance val) {
      return val.getHashValue();
   }

   static bool isEqual(polar::ast::AssociatedConformance lhs,
                       polar::ast::AssociatedConformance rhs) {
      return lhs == rhs;
   }
};
}

#endif
