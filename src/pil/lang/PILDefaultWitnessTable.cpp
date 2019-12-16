//===--- PILDefaultWitnessTable.cpp ---------------------------------------===//
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
// This file defines the PILDefaultWitnessTable class, which is used to provide
// default implementations of protocol requirements for resilient protocols,
// allowing IRGen to generate the appropriate metadata so that the runtime can
// insert those requirements to witness tables that were emitted prior to the
// requirement being added.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/AstMangler.h"
#include "polarphp/pil/lang/PILDefaultWitnessTable.h"
#include "polarphp/pil/lang/PILModule.h"
#include "llvm/ADT/SmallString.h"

using namespace polar;

void PILDefaultWitnessTable::addDefaultWitnessTable() {
   // Make sure we have not seen this witness table yet.
   assert(Mod.DefaultWitnessTableMap.find(Interface) ==
          Mod.DefaultWitnessTableMap.end() && "Attempting to create duplicate "
                                              "default witness table.");
   Mod.DefaultWitnessTableMap[Interface] = this;
   Mod.defaultWitnessTables.push_back(this);
}

PILDefaultWitnessTable *
PILDefaultWitnessTable::create(PILModule &M, PILLinkage Linkage,
                               const InterfaceDecl *Interface,
                               ArrayRef<PILDefaultWitnessTable::Entry> entries){
   // Allocate the witness table and initialize it.
   auto *buf = M.allocate<PILDefaultWitnessTable>(1);
   PILDefaultWitnessTable *wt =
      ::new (buf) PILDefaultWitnessTable(M, Linkage, Interface, entries);

   wt->addDefaultWitnessTable();

   // Return the resulting default witness table.
   return wt;
}

PILDefaultWitnessTable *
PILDefaultWitnessTable::create(PILModule &M, PILLinkage Linkage,
                               const InterfaceDecl *Interface) {
   // Allocate the witness table and initialize it.
   auto *buf = M.allocate<PILDefaultWitnessTable>(1);
   PILDefaultWitnessTable *wt =
      ::new (buf) PILDefaultWitnessTable(M, Linkage, Interface);

   wt->addDefaultWitnessTable();

   // Return the resulting default witness table.
   return wt;
}

PILDefaultWitnessTable::
PILDefaultWitnessTable(PILModule &M,
                       PILLinkage Linkage,
                       const InterfaceDecl *Interface,
                       ArrayRef<Entry> entries)
   : Mod(M), Linkage(Linkage), Interface(Interface), Entries(),
     IsDeclaration(true) {

   convertToDefinition(entries);
}

PILDefaultWitnessTable::PILDefaultWitnessTable(PILModule &M,
                                               PILLinkage Linkage,
                                               const InterfaceDecl *Interface)
   : Mod(M), Linkage(Linkage), Interface(Interface), Entries(),
     IsDeclaration(true) {}

void PILDefaultWitnessTable::
convertToDefinition(ArrayRef<Entry> entries) {
   assert(IsDeclaration);
   IsDeclaration = false;

   Entries = Mod.allocateCopy(entries);

   // Bump the reference count of witness functions referenced by this table.
   for (auto entry : getEntries()) {
      if (entry.isValid() && entry.getKind() == PILWitnessTable::Method) {
         entry.getMethodWitness().Witness->incrementRefCount();
      }
   }
}

std::string PILDefaultWitnessTable::getUniqueName() const {
   mangle::AstMangler Mangler;
   return Mangler.mangleTypeWithoutPrefix(getInterface()->getDeclaredType());
}

PILDefaultWitnessTable::~PILDefaultWitnessTable() {
   // Drop the reference count of witness functions referenced by this table.
   for (auto entry : getEntries()) {
      if (entry.isValid() && entry.getKind() == PILWitnessTable::Method) {
         entry.getMethodWitness().Witness->decrementRefCount();
      }
   }
}
