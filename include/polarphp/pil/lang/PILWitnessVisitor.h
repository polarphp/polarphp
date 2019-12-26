//===--- PILWitnessVisitor.h - Witness method table visitor -----*- C++ -*-===//
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
// This file defines the PILWitnessVisitor class, which is used to generate and
// perform lookups in witness method tables for interfaces and interface
// conformances.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PILWITNESSVISITOR_H
#define POLARPHP_PIL_PILWITNESSVISITOR_H

#include "polarphp/ast/AstVisitor.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/InterfaceAssociations.h"
#include "polarphp/ast/Types.h"
#include "polarphp/pil/lang/TypeLowering.h"
#include "llvm/Support/ErrorHandling.h"

namespace polar {

/// A CRTP class for visiting the witnesses of a interface.
///
/// The design here is that each entry (or small group of entries)
/// gets turned into a call to the implementation class describing
/// the exact variant of witness.  For example, for member
/// variables, there should be separate callbacks for adding a
/// getter/setter pair, for just adding a getter, and for adding a
/// physical projection (if we decide to support that).
///
/// You must override the following methods:
/// - addInterfaceConformanceDescriptor()
/// - addOutOfLineBaseInterface()
/// - addAssociatedType()
/// - addAssociatedConformance()
/// - addMethod()
/// - addPlaceholder()

template <class T> class PILWitnessVisitor : public AstVisitor<T> {
  T &asDerived() { return *static_cast<T*>(this); }

public:
  void visitInterfaceDecl(InterfaceDecl *interface) {
    // The interface conformance descriptor gets added first.
    asDerived().addInterfaceConformanceDescriptor();

    for (const auto &reqt : interface->getRequirementSignature()) {
      switch (reqt.getKind()) {
      // These requirements don't show up in the witness table.
      case RequirementKind::Superclass:
      case RequirementKind::SameType:
      case RequirementKind::Layout:
        continue;

      case RequirementKind::Conformance: {
        auto type = reqt.getFirstType()->getCanonicalType();
        assert(type->isTypeParameter());
        auto requirement =
          cast<InterfaceType>(reqt.getSecondType()->getCanonicalType())
            ->getDecl();

        // ObjC interfaces do not have witnesses.
        if (!lowering::TypeConverter::interfaceRequiresWitnessTable(requirement))
          continue;

        // If the type parameter is 'self', consider this to be interface
        // inheritance.  In the canonical signature, these should all
        // come before any interface requirements on associated types.
        if (auto parameter = dyn_cast<GenericTypeParamType>(type)) {
          assert(type->isEqual(interface->getSelfInterfaceType()));
          assert(parameter->getDepth() == 0 && parameter->getIndex() == 0 &&
                 "non-self type parameter in interface");
          asDerived().addOutOfLineBaseInterface(requirement);
          continue;
        }

        // Otherwise, add an associated requirement.
        AssociatedConformance assocConf(interface, type, requirement);
        asDerived().addAssociatedConformance(assocConf);
        continue;
      }
      }
      llvm_unreachable("bad requirement kind");
    }

    // Add the associated types.
    for (auto *associatedType : interface->getAssociatedTypeMembers()) {
      // If this is a new associated type (which does not override an
      // existing associated type), add it.
      if (associatedType->getOverriddenDecls().empty())
        asDerived().addAssociatedType(AssociatedType(associatedType));
    }

    if (asDerived().shouldVisitRequirementSignatureOnly())
      return;

    // Visit the witnesses for the direct members of a interface.
    for (Decl *member : interface->getMembers()) {
      AstVisitor<T>::visit(member);
    }
  }

  /// If true, only the base interfaces and associated types will be visited.
  /// The base implementation returns false.
  bool shouldVisitRequirementSignatureOnly() const {
    return false;
  }

  /// Fallback for unexpected interface requirements.
  void visitDecl(Decl *d) {
    llvm_unreachable("unhandled interface requirement");
  }

  void visitAbstractStorageDecl(AbstractStorageDecl *sd) {
    sd->visitOpaqueAccessors([&](AccessorDecl *accessor) {
      if (PILDeclRef::requiresNewWitnessTableEntry(accessor))
        asDerived().addMethod(PILDeclRef(accessor, PILDeclRef::Kind::Func));
    });
  }

  void visitConstructorDecl(ConstructorDecl *cd) {
    if (PILDeclRef::requiresNewWitnessTableEntry(cd))
      asDerived().addMethod(PILDeclRef(cd, PILDeclRef::Kind::Allocator));
  }

  void visitAccessorDecl(AccessorDecl *func) {
    // Accessors are emitted by visitAbstractStorageDecl, above.
  }

  void visitFuncDecl(FuncDecl *func) {
    assert(!isa<AccessorDecl>(func));
    if (PILDeclRef::requiresNewWitnessTableEntry(func))
      asDerived().addMethod(PILDeclRef(func, PILDeclRef::Kind::Func));
  }

  void visitMissingMemberDecl(MissingMemberDecl *placeholder) {
    asDerived().addPlaceholder(placeholder);
  }

  void visitAssociatedTypeDecl(AssociatedTypeDecl *td) {
    // We already visited these in the first pass.
  }

  void visitTypeAliasDecl(TypeAliasDecl *tad) {
    // We don't care about these by themselves for witnesses.
  }

  void visitPatternBindingDecl(PatternBindingDecl *pbd) {
    // We only care about the contained VarDecls.
  }

  void visitIfConfigDecl(IfConfigDecl *icd) {
    // We only care about the active members, which were already subsumed by the
    // enclosing type.
  }

  void visitPoundDiagnosticDecl(PoundDiagnosticDecl *pdd) {
    // We don't care about diagnostics at this stage.
  }
};

} // end namespace polar

#endif
