//===--- PILWitnessTable.h - Defines the PILWitnessTable class --*- C++ -*-===//
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
// This file defines the PILWitnessTable class, which is used to map a protocol
// conformance for a type to its implementing PILFunctions. This information is
// (FIXME will be) used by IRGen to create witness tables for protocol dispatch.
// It can also be used by generic specialization and existential
// devirtualization passes to promote witness_method and protocol_method
// instructions to static function_refs.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PILWITNESSTABLE_H
#define POLARPHP_PIL_PILWITNESSTABLE_H

#include "polarphp/pil/lang/PILAllocated.h"
#include "polarphp/pil/lang/PILDeclRef.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/ast/InterfaceConformanceRef.h"
#include "llvm/ADT/ilist_node.h"
#include "llvm/ADT/ilist.h"
#include <string>

namespace polar {

class PILFunction;
class PILModule;
enum IsSerialized_t : unsigned char;
class InterfaceConformance;
class RootInterfaceConformance;

/// A mapping from each requirement of a protocol to the PIL-level entity
/// satisfying the requirement for a concrete type.
class PILWitnessTable : public llvm::ilist_node<PILWitnessTable>,
                        public PILAllocated<PILWitnessTable>
{
public:
   /// A witness table entry describing the witness for a method.
   struct MethodWitness {
      /// The method required.
      PILDeclRef Requirement;
      /// The witness for the method.
      /// This can be null in case dead function elimination has removed the method.
      PILFunction *Witness;
   };

   /// A witness table entry describing the witness for an associated type.
   struct AssociatedTypeWitness {
      /// The associated type required.
      AssociatedTypeDecl *Requirement;
      /// The concrete semantic type of the witness.
      CanType Witness;
   };

   /// A witness table entry describing the witness for an associated type's
   /// protocol requirement.
   struct AssociatedTypeInterfaceWitness {
      /// The associated type required.  A dependent type in the protocol's
      /// context.
      CanType Requirement;
      /// The protocol requirement on the type.
      InterfaceDecl *Interface;
      /// The InterfaceConformance satisfying the requirement. Null if the
      /// conformance is dependent.
      InterfaceConformanceRef Witness;
   };

   /// A witness table entry referencing the protocol conformance for a refined
   /// base protocol.
   struct BaseInterfaceWitness {
      /// The base protocol.
      InterfaceDecl *Requirement;
      /// The InterfaceConformance for the base protocol.
      InterfaceConformance *Witness;
   };

   /// A witness table entry kind.
   enum WitnessKind {
      Invalid,
      Method,
      AssociatedType,
      AssociatedTypeInterface,
      BaseInterface
   };

   /// A witness table entry.
   class Entry {
      WitnessKind Kind;
      union {
         MethodWitness Method;
         AssociatedTypeWitness AssociatedType;
         AssociatedTypeInterfaceWitness AssociatedTypeInterface;
         BaseInterfaceWitness BaseInterface;
      };

   public:
      Entry() : Kind(WitnessKind::Invalid) {}

      Entry(const MethodWitness &Method)
         : Kind(WitnessKind::Method), Method(Method)
      {}

      Entry(const AssociatedTypeWitness &AssociatedType)
         : Kind(WitnessKind::AssociatedType), AssociatedType(AssociatedType)
      {}

      Entry(const AssociatedTypeInterfaceWitness &AssociatedTypeInterface)
         : Kind(WitnessKind::AssociatedTypeInterface),
           AssociatedTypeInterface(AssociatedTypeInterface)
      {}

      Entry(const BaseInterfaceWitness &BaseInterface)
         : Kind(WitnessKind::BaseInterface),
           BaseInterface(BaseInterface)
      {}

      WitnessKind getKind() const { return Kind; }

      bool isValid() const { return Kind != WitnessKind::Invalid; }

      const MethodWitness &getMethodWitness() const {
         assert(Kind == WitnessKind::Method);
         return Method;
      }
      const AssociatedTypeWitness &getAssociatedTypeWitness() const {
         assert(Kind == WitnessKind::AssociatedType);
         return AssociatedType;
      }
      const AssociatedTypeInterfaceWitness &
      getAssociatedTypeInterfaceWitness() const {
         assert(Kind == WitnessKind::AssociatedTypeInterface);
         return AssociatedTypeInterface;
      }
      const BaseInterfaceWitness &getBaseInterfaceWitness() const {
         assert(Kind == WitnessKind::BaseInterface);
         return BaseInterface;
      }

      void removeWitnessMethod() {
         assert(Kind == WitnessKind::Method);
         if (Method.Witness) {
            Method.Witness->decrementRefCount();
         }
         Method.Witness = nullptr;
      }

      void print(llvm::raw_ostream &out, bool verbose,
                 const PrintOptions &options) const;
   };

   /// An entry for a conformance requirement that makes the requirement
   /// conditional. These aren't public, but any witness thunks need to feed them
   /// into the true witness functions.
   struct ConditionalConformance {
      CanType Requirement;
      InterfaceConformanceRef Conformance;
   };

private:
   /// The module which contains the PILWitnessTable.
   PILModule &Mod;

   /// The symbol name of the witness table that will be propagated to the object
   /// file level.
   StringRef Name;

   /// The linkage of the witness table.
   PILLinkage Linkage;

   /// The conformance mapped to this witness table.
   RootInterfaceConformance *Conformance;

   /// The various witnesses containing in this witness table. Is empty if the
   /// table has no witness entries or if it is a declaration.
   MutableArrayRef<Entry> Entries;

   /// Any conditional conformances required for this witness table. These are
   /// private to this conformance.
   ///
   /// (If other private entities are introduced this could/should be switched
   /// into a private version of Entries.)
   MutableArrayRef<ConditionalConformance> ConditionalConformances;

   /// Whether or not this witness table is a declaration. This is separate from
   /// whether or not entries is empty since you can have an empty witness table
   /// that is not a declaration.
   bool IsDeclaration;

   /// Whether or not this witness table is serialized, which allows
   /// devirtualization from another module.
   bool Serialized;

   /// Private constructor for making PILWitnessTable definitions.
   PILWitnessTable(PILModule &M, PILLinkage Linkage, IsSerialized_t Serialized,
                   StringRef name, RootInterfaceConformance *conformance,
                   ArrayRef<Entry> entries,
                   ArrayRef<ConditionalConformance> conditionalConformances);

   /// Private constructor for making PILWitnessTable declarations.
   PILWitnessTable(PILModule &M, PILLinkage Linkage, StringRef Name,
                   RootInterfaceConformance *conformance);

   void addWitnessTable();

public:
   /// Create a new PILWitnessTable definition with the given entries.
   static PILWitnessTable *
   create(PILModule &M, PILLinkage Linkage, IsSerialized_t Serialized,
          RootInterfaceConformance *conformance, ArrayRef<Entry> entries,
          ArrayRef<ConditionalConformance> conditionalConformances);

   /// Create a new PILWitnessTable declaration.
   static PILWitnessTable *create(PILModule &M, PILLinkage Linkage,
                                  RootInterfaceConformance *conformance);

   ~PILWitnessTable();

   /// Return the AST InterfaceConformance this witness table represents.
   RootInterfaceConformance *getConformance() const {
      return Conformance;
   }

   /// Return the context in which the conformance giving rise to this
   /// witness table was defined.
   DeclContext *getDeclContext() const;

   /// Return the protocol for which this witness table is a conformance.
   InterfaceDecl *getInterface() const;

   /// Return the formal type which conforms to the protocol.
   ///
   /// Note that this will not be a substituted type: it may only be meaningful
   /// in the abstract context of the conformance rather than the context of any
   /// particular use of it.
   CanType getConformingType() const;

   /// Return the symbol name of the witness table that will be propagated to the
   /// object file level.
   StringRef getName() const { return Name; }

   /// Returns true if this witness table is a declaration.
   bool isDeclaration() const { return IsDeclaration; }

   /// Returns true if this witness table is a definition.
   bool isDefinition() const { return !isDeclaration(); }

   /// Returns true if this witness table is going to be (or was) serialized.
   IsSerialized_t isSerialized() const {
      return Serialized ? IsSerialized : IsNotSerialized;
   }

   /// Sets the serialized flag.
   void setSerialized(IsSerialized_t serialized) {
      assert(serialized != IsSerializable);
      Serialized = (serialized ? 1 : 0);
   }

   /// Return all of the witness table entries.
   ArrayRef<Entry> getEntries() const { return Entries; }

   /// Return all of the conditional conformances.
   ArrayRef<ConditionalConformance> getConditionalConformances() const {
      return ConditionalConformances;
   }

   /// Clears methods in MethodWitness entries.
   /// \p predicate Returns true if the passed entry should be set to null.
   template <typename Predicate> void clearMethods_if(Predicate predicate) {
      for (Entry &entry : Entries) {
         if (entry.getKind() == WitnessKind::Method) {
            const MethodWitness &MW = entry.getMethodWitness();
            if (MW.Witness && predicate(MW)) {
               entry.removeWitnessMethod();
            }
         }
      }
   }

   /// Verify that the witness table is well-formed.
   void verify(const PILModule &M) const;

   /// Get the linkage of the witness table.
   PILLinkage getLinkage() const { return Linkage; }

   /// Set the linkage of the witness table.
   void setLinkage(PILLinkage l) { Linkage = l; }

   /// Change a PILWitnessTable declaration into a PILWitnessTable definition.
   void
   convertToDefinition(ArrayRef<Entry> newEntries,
                       ArrayRef<ConditionalConformance> conditionalConformances,
                       IsSerialized_t isSerialized);

   // Whether a conformance should be serialized.
   static bool
   conformanceIsSerialized(const RootInterfaceConformance *conformance);

   /// Call \c fn on each (split apart) conditional requirement of \c conformance
   /// that should appear in a witness table, i.e., conformance requirements that
   /// need witness tables themselves.
   ///
   /// The \c unsigned argument to \c fn is a counter for the conditional
   /// conformances, and should be used for indexing arrays of them.
   ///
   /// This acts like \c any_of: \c fn returning \c true will stop the
   /// enumeration and \c enumerateWitnessTableConditionalConformances will
   /// return \c true, while \c fn returning \c false will let it continue.
   static bool enumerateWitnessTableConditionalConformances(
      const InterfaceConformance *conformance,
      llvm::function_ref<bool(unsigned, CanType, InterfaceDecl *)> fn);

   /// Print the witness table.
   void print(llvm::raw_ostream &OS, bool Verbose = false) const;

   /// Dump the witness table to stderr.
   void dump() const;
};

} // end polar::ast namespace

//===----------------------------------------------------------------------===//
// ilist_traits for PILWitnessTable
//===----------------------------------------------------------------------===//

namespace llvm {

template <>
struct ilist_traits<::polar::PILWitnessTable> :
   public ilist_node_traits<::polar::PILWitnessTable> {
   using PILWitnessTable = ::polar::PILWitnessTable;

public:
   static void deleteNode(PILWitnessTable *WT) { WT->~PILWitnessTable(); }

private:
   void createNode(const PILWitnessTable &);
};

} // end llvm namespace

#endif
