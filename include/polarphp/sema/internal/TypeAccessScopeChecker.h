//===--- TypeAccessScopeChecker.h - Computes access for Types ---*- C++ -*-===//
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

#ifndef POLARPHP_SEMA_INTERNAL_TYPEACCESSSCOPECHECKER_H
#define POLARPHP_SEMA_INTERNAL_TYPEACCESSSCOPECHECKER_H

#include "polarphp/ast/Decl.h"
#include "polarphp/ast/DeclContext.h"
#include "polarphp/ast/SourceFile.h"
#include "polarphp/ast/Type.h"
#include "polarphp/ast/TypeDeclFinder.h"
#include "polarphp/ast/TypeRepr.h"

namespace polar {

class SourceFile;

/// Computes the access scope where a Type or TypeRepr is available, which is
/// the intersection of all the scopes where the declarations referenced in the
/// Type or TypeRepr are available.
class TypeAccessScopeChecker {
   const SourceFile *File;
   bool TreatUsableFromInlineAsPublic;

   Optional<AccessScope> Scope = AccessScope::getPublic();

   TypeAccessScopeChecker(const DeclContext *useDC,
                          bool treatUsableFromInlineAsPublic)
      : File(useDC->getParentSourceFile()),
        TreatUsableFromInlineAsPublic(treatUsableFromInlineAsPublic) {}

   bool visitDecl(const ValueDecl *VD) {
      if (isa<GenericTypeParamDecl>(VD))
         return true;

      auto AS = VD->getFormalAccessScope(File, TreatUsableFromInlineAsPublic);
      Scope = Scope->intersectWith(AS);
      return Scope.hasValue();
   }

public:
   static Optional<AccessScope>
   getAccessScope(TypeRepr *TR, const DeclContext *useDC,
                  bool treatUsableFromInlineAsPublic = false) {
      TypeAccessScopeChecker checker(useDC, treatUsableFromInlineAsPublic);
      TR->walk(TypeReprIdentFinder([&](const ComponentIdentTypeRepr *typeRepr) {
         return checker.visitDecl(typeRepr->getBoundDecl());
      }));
      return checker.Scope;
   }

   static Optional<AccessScope>
   getAccessScope(Type T, const DeclContext *useDC,
                  bool treatUsableFromInlineAsPublic = false) {
      TypeAccessScopeChecker checker(useDC, treatUsableFromInlineAsPublic);
      T.walk(SimpleTypeDeclFinder([&](const ValueDecl *VD) {
         if (checker.visitDecl(VD))
            return TypeWalker::Action::Continue;
         return TypeWalker::Action::Stop;
      }));
      return checker.Scope;
   }
};

} // end namespace polar

#endif
