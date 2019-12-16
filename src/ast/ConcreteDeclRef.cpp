//===--- ConcreteDeclRef.cpp - Reference to a concrete decl ---------------===//
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
// This file implements the ConcreteDeclRef class, which provides a reference to
// a declaration that is potentially specialized.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/ConcreteDeclRef.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/GenericSignature.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/ast/SubstitutionMap.h"
#include "polarphp/ast/Types.h"
#include "llvm/Support/raw_ostream.h"

namespace polar {

ConcreteDeclRef ConcreteDeclRef::getOverriddenDecl() const {
   auto *derivedDecl = getDecl();
   auto *baseDecl = derivedDecl->getOverriddenDecl();

   auto baseSig = baseDecl->getInnermostDeclContext()
      ->getGenericSignatureOfContext();
   auto derivedSig = derivedDecl->getInnermostDeclContext()
      ->getGenericSignatureOfContext();

   SubstitutionMap subs;
   if (baseSig) {
      Optional <SubstitutionMap> derivedSubMap;
      if (derivedSig)
         derivedSubMap = getSubstitutions();
      subs = SubstitutionMap::getOverrideSubstitutions(baseDecl, derivedDecl,
                                                       derivedSubMap);
   }
   return ConcreteDeclRef(baseDecl, subs);
}

void ConcreteDeclRef::dump(raw_ostream &os) const {
   if (!getDecl()) {
      os << "**NULL**";
      return;
   }

   getDecl()->dumpRef(os);

   // If specialized, dump the substitutions.
   if (isSpecialized()) {
      os << " [with ";
      getSubstitutions().dump(os, SubstitutionMap::DumpStyle::Minimal);
      os << ']';
   }
}

void ConcreteDeclRef::dump() const {
   dump(llvm::errs());
}

} // polar