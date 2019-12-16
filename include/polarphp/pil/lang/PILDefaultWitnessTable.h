//===--- PILDefaultWitnessTable.h -------------------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_PILDEFAULTWITNESSTABLE_H
#define POLARPHP_PIL_PILDEFAULTWITNESSTABLE_H

#include "polarphp/pil/lang/PILAllocated.h"
#include "polarphp/pil/lang/PILDeclRef.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILWitnessTable.h"
#include "llvm/ADT/ilist_node.h"
#include "llvm/ADT/ilist.h"
#include <string>

namespace polar {

class InterfaceDecl;
class PILFunction;
class PILModule;

/// A mapping from each requirement of a protocol to the PIL-level entity
/// satisfying the requirement for conformances which do not explicitly
/// provide a witness.
class PILDefaultWitnessTable : public llvm::ilist_node<PILDefaultWitnessTable>,
                               public PILAllocated<PILDefaultWitnessTable>
{
public:
   /// A default witness table entry describing the default witness for a
   /// requirement.
   using Entry = PILWitnessTable::Entry;

private:
   /// The module which contains the PILDefaultWitnessTable.
   PILModule &Mod;

   /// The linkage of the witness table.
   PILLinkage Linkage;

   /// The protocol declaration to which this default witness table applies.
   const InterfaceDecl *Interface;

   /// The minimum size of a valid witness table conforming to this protocol,
   /// with all resilient default requirements omitted.
   unsigned MinimumWitnessTableSizeInWords;

   /// The various witnesses containing in this default witness table.
   MutableArrayRef<Entry> Entries;

   /// Temporary state while PILGen is emitting a default witness table.
   /// We can never have a true declaration since there's no way to reference
   /// the default witness table from outside its defining translation unit.
   bool IsDeclaration;

   /// Private constructor for making PILDefaultWitnessTable declarations.
   PILDefaultWitnessTable(PILModule &M, PILLinkage Linkage,
                          const InterfaceDecl *Interface);

   /// Private constructor for making PILDefaultWitnessTable definitions.
   PILDefaultWitnessTable(PILModule &M, PILLinkage Linkage,
                          const InterfaceDecl *Interface,
                          ArrayRef<Entry> entries);

   void addDefaultWitnessTable();

public:
   /// Create a new PILDefaultWitnessTable declaration.
   static PILDefaultWitnessTable *create(PILModule &M, PILLinkage Linkage,
                                         const InterfaceDecl *Interface);

   /// Create a new PILDefaultWitnessTable definition with the given entries.
   static PILDefaultWitnessTable *create(PILModule &M, PILLinkage Linkage,
                                         const InterfaceDecl *Interface,
                                         ArrayRef<Entry> entries);

   /// Get a name that uniquely identifies this default witness table.
   ///
   /// Note that this is /not/ valid as a symbol name; it is only guaranteed to
   /// be unique among default witness tables, not all symbols.
   std::string getUniqueName() const;

   /// Get the linkage of the default witness table.
   PILLinkage getLinkage() const { return Linkage; }

   /// Set the linkage of the default witness table.
   void setLinkage(PILLinkage l) { Linkage = l; }

   void convertToDefinition(ArrayRef<Entry> entries);

   ~PILDefaultWitnessTable();

   /// Return true if this is a declaration with no body.
   bool isDeclaration() const { return IsDeclaration; }

   /// Return the AST InterfaceDecl this default witness table is associated with.
   const InterfaceDecl *getInterface() const { return Interface; }

   /// Clears methods in witness entries.
   /// \p predicate Returns true if the passed entry should be set to null.
   template <typename Predicate> void clearMethods_if(Predicate predicate) {
      for (Entry &entry : Entries) {
         if (!entry.isValid())
            continue;
         if (entry.getKind() != PILWitnessTable::Method)
            continue;

         auto *MW = entry.getMethodWitness().Witness;
         if (MW && predicate(MW)) {
            entry.removeWitnessMethod();
         }
      }
   }

   /// Return all of the default witness table entries.
   ArrayRef<Entry> getEntries() const { return Entries; }

   /// Verify that the default witness table is well-formed.
   void verify(const PILModule &M) const;

   /// Print the default witness table.
   void print(llvm::raw_ostream &OS, bool Verbose = false) const;

   /// Dump the default witness table to stderr.
   void dump() const;
};

} // end polar namespace

//===----------------------------------------------------------------------===//
// ilist_traits for PILDefaultWitnessTable
//===----------------------------------------------------------------------===//

namespace llvm {

template <>
struct ilist_traits<::polar::PILDefaultWitnessTable> :
   public ilist_node_traits<::polar::PILDefaultWitnessTable> {
   using PILDefaultWitnessTable = ::polar::PILDefaultWitnessTable;

public:
   static void deleteNode(PILDefaultWitnessTable *WT) { WT->~PILDefaultWitnessTable(); }

private:
   void createNode(const PILDefaultWitnessTable &);
};

} // end llvm namespace

#endif
