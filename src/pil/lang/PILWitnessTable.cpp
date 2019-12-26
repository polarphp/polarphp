//===--- PILWitnessTable.cpp - Defines the PILWitnessTable class ----------===//
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
// This file defines the PILWitnessTable class, which is used to map a interface
// conformance for a type to its implementing PILFunctions. This information is
// used by IRGen to create witness tables for interface dispatch.
//
// It can also be used by generic specialization and existential
// devirtualization passes to promote witness_method instructions to static
// function_refs.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pil-witness-table"

#include "polarphp/pil/lang/PILWitnessTable.h"
#include "polarphp/ast/AstMangler.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/pil/lang/PILModule.h"
#include "llvm/ADT/SmallString.h"

using namespace polar;

static std::string mangleConstant(RootInterfaceConformance *C) {
   mangle::AstMangler Mangler;
   return Mangler.mangleWitnessTable(C);
}

DeclContext *PILWitnessTable::getDeclContext() const {
   return getConformance()->getDeclContext();
}

InterfaceDecl *PILWitnessTable::getInterface() const {
   return getConformance()->getInterface();
}

CanType PILWitnessTable::getConformingType() const {
   return getConformance()->getType()->getCanonicalType();
}

void PILWitnessTable::addWitnessTable() {
   // Make sure we have not seen this witness table yet.
   assert(Mod.WitnessTableMap.find(Conformance) ==
          Mod.WitnessTableMap.end() && "Attempting to create duplicate "
                                       "witness table.");
   Mod.WitnessTableMap[Conformance] = this;
   Mod.witnessTables.push_back(this);
}

PILWitnessTable *PILWitnessTable::create(
   PILModule &M, PILLinkage Linkage, IsSerialized_t Serialized,
   RootInterfaceConformance *Conformance,
   ArrayRef<PILWitnessTable::Entry> entries,
   ArrayRef<ConditionalConformance> conditionalConformances) {
   assert(Conformance && "Cannot create a witness table for a null "
                         "conformance.");

   // Create the mangled name of our witness table...
   Identifier Name = M.getAstContext().getIdentifier(mangleConstant(Conformance));

   LLVM_DEBUG(llvm::dbgs() << "PILWitnessTable Creating: " << Name.str() << '\n');

   // Allocate the witness table and initialize it.
   void *buf = M.allocate(sizeof(PILWitnessTable), alignof(PILWitnessTable));
   PILWitnessTable *wt = ::new (buf)
      PILWitnessTable(M, Linkage, Serialized, Name.str(), Conformance, entries,
                      conditionalConformances);

   wt->addWitnessTable();

   // Return the resulting witness table.
   return wt;
}

PILWitnessTable *
PILWitnessTable::create(PILModule &M, PILLinkage Linkage,
                        RootInterfaceConformance *Conformance) {
   assert(Conformance && "Cannot create a witness table for a null "
                         "conformance.");

   // Create the mangled name of our witness table...
   Identifier Name = M.getAstContext().getIdentifier(mangleConstant(Conformance));


   // Allocate the witness table and initialize it.
   void *buf = M.allocate(sizeof(PILWitnessTable), alignof(PILWitnessTable));
   PILWitnessTable *wt = ::new (buf) PILWitnessTable(M, Linkage, Name.str(),
                                                     Conformance);

   wt->addWitnessTable();

   // Return the resulting witness table.
   return wt;
}

PILWitnessTable::PILWitnessTable(
   PILModule &M, PILLinkage Linkage, IsSerialized_t Serialized, StringRef N,
   RootInterfaceConformance *Conformance, ArrayRef<Entry> entries,
   ArrayRef<ConditionalConformance> conditionalConformances)
   : Mod(M), Name(N), Linkage(Linkage), Conformance(Conformance), Entries(),
     ConditionalConformances(), IsDeclaration(true), Serialized(false) {
   convertToDefinition(entries, conditionalConformances, Serialized);
}

PILWitnessTable::PILWitnessTable(PILModule &M, PILLinkage Linkage, StringRef N,
                                 RootInterfaceConformance *Conformance)
   : Mod(M), Name(N), Linkage(Linkage), Conformance(Conformance), Entries(),
     ConditionalConformances(), IsDeclaration(true), Serialized(false) {}

PILWitnessTable::~PILWitnessTable() {
   if (isDeclaration())
      return;

   // Drop the reference count of witness functions referenced by this table.
   for (auto entry : getEntries()) {
      switch (entry.getKind()) {
         case Method:
            if (entry.getMethodWitness().Witness) {
               entry.getMethodWitness().Witness->decrementRefCount();
            }
            break;
         case AssociatedType:
         case AssociatedTypeInterface:
         case BaseInterface:
         case Invalid:
            break;
      }
   }
}

void PILWitnessTable::convertToDefinition(
   ArrayRef<Entry> entries,
   ArrayRef<ConditionalConformance> conditionalConformances,
   IsSerialized_t isSerialized) {
   assert(isDeclaration() && "Definitions should never call this method.");
   IsDeclaration = false;
   assert(isSerialized != IsSerializable);
   Serialized = (isSerialized == IsSerialized);

   Entries = Mod.allocateCopy(entries);
   ConditionalConformances = Mod.allocateCopy(conditionalConformances);

   // Bump the reference count of witness functions referenced by this table.
   for (auto entry : getEntries()) {
      switch (entry.getKind()) {
         case Method:
            if (entry.getMethodWitness().Witness) {
               entry.getMethodWitness().Witness->incrementRefCount();
            }
            break;
         case AssociatedType:
         case AssociatedTypeInterface:
         case BaseInterface:
         case Invalid:
            break;
      }
   }
}

bool PILWitnessTable::conformanceIsSerialized(
   const RootInterfaceConformance *conformance) {
   auto normalConformance = dyn_cast<NormalInterfaceConformance>(conformance);
   if (normalConformance && normalConformance->isResilient())
      return false;

   if (conformance->getInterface()->getEffectiveAccess() < AccessLevel::Public)
      return false;

   auto *nominal = conformance->getType()->getAnyNominal();
   return nominal->getEffectiveAccess() >= AccessLevel::Public;
}

bool PILWitnessTable::enumerateWitnessTableConditionalConformances(
   const InterfaceConformance *conformance,
   llvm::function_ref<bool(unsigned, CanType, InterfaceDecl *)> fn) {
   unsigned conformanceIndex = 0;
   for (auto req : conformance->getConditionalRequirements()) {
      if (req.getKind() != RequirementKind::Conformance)
         continue;

      auto proto = req.getSecondType()->castTo<InterfaceType>()->getDecl();

      if (lowering::TypeConverter::interfaceRequiresWitnessTable(proto)) {
         if (fn(conformanceIndex, req.getFirstType()->getCanonicalType(), proto))
            return true;

         conformanceIndex++;
      }
   }
   return false;
}
