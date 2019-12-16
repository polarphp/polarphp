//===--- PILVTable.h - Defines the PILVTable class --------------*- C++ -*-===//
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
// This file defines the PILVTable class, which is used to map dynamically
// dispatchable class methods and properties to their concrete implementations
// for a dynamic type. This information is used by IRGen to emit class vtables,
// by the devirtualization pass to promote class_method instructions to static
// function_refs.
//
// Note that vtable layout itself is implemented in PILVTableLayout.h and is
// independent of the PILVTable; in general, for a class from another module we
// might not have a PILVTable to deserialize, and for a class in a different
// translation in the same module the PILVTable is not available either.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PILVTABLE_H
#define POLARPHP_PIL_PILVTABLE_H

#include "polarphp/pil/lang/PILAllocated.h"
#include "polarphp/pil/lang/PILDeclRef.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "llvm/ADT/ilist_node.h"
#include "llvm/ADT/ilist.h"
#include "llvm/ADT/Optional.h"
#include <algorithm>

namespace polar {

class ClassDecl;
enum IsSerialized_t : unsigned char;
class PILFunction;
class PILModule;

/// A mapping from each dynamically-dispatchable method of a class to the
/// PILFunction that implements the method for that class.
/// Note that dead methods are completely removed from the vtable.
class PILVTable : public llvm::ilist_node<PILVTable>,
                  public PILAllocated<PILVTable> {
public:
   // TODO: Entry should include substitutions needed to invoke an overridden
   // generic base class method.
   struct Entry {
      enum Kind : uint8_t {
         /// The vtable entry is for a method defined directly in this class.
            Normal,
         /// The vtable entry is inherited from the superclass.
            Inherited,
         /// The vtable entry is inherited from the superclass, and overridden
         /// in this class.
            Override,
      };

      Entry()
         : Implementation(nullptr), TheKind(Kind::Normal) { }

      Entry(PILDeclRef Method, PILFunction *Implementation, Kind TheKind)
         : Method(Method), Implementation(Implementation), TheKind(TheKind) { }

      /// The declaration reference to the least-derived method visible through
      /// the class.
      PILDeclRef Method;

      /// The function which implements the method for the class.
      PILFunction *Implementation;

      /// The entry kind.
      Kind TheKind;
   };

   // Disallow copying into temporary objects.
   PILVTable(const PILVTable &other) = delete;
   PILVTable &operator=(const PILVTable &) = delete;

private:
   /// The ClassDecl mapped to this VTable.
   ClassDecl *Class;

   /// Whether or not this vtable is serialized, which allows
   /// devirtualization from another module.
   bool Serialized : 1;

   /// The number of PILVTables entries.
   unsigned NumEntries : 31;

   /// Tail-allocated PILVTable entries.
   Entry Entries[1];

   /// Private constructor. Create PILVTables by calling PILVTable::create.
   PILVTable(ClassDecl *c, IsSerialized_t serialized, ArrayRef<Entry> entries);

public:
   ~PILVTable();

   /// Create a new PILVTable with the given method-to-implementation mapping.
   /// The PILDeclRef keys should reference the most-overridden members available
   /// through the class.
   static PILVTable *create(PILModule &M, ClassDecl *Class,
                            IsSerialized_t Serialized,
                            ArrayRef<Entry> Entries);

   /// Return the class that the vtable represents.
   ClassDecl *getClass() const { return Class; }

   /// Returns true if this vtable is going to be (or was) serialized.
   IsSerialized_t isSerialized() const {
      return Serialized ? IsSerialized : IsNotSerialized;
   }

   /// Sets the serialized flag.
   void setSerialized(IsSerialized_t serialized) {
      assert(serialized != IsSerializable);
      Serialized = (serialized ? 1 : 0);
   }

   /// Return all of the method entries.
   ArrayRef<Entry> getEntries() const { return {Entries, NumEntries}; }

   /// Look up the implementation function for the given method.
   Optional<Entry> getEntry(PILModule &M, PILDeclRef method) const;

   /// Removes entries from the vtable.
   /// \p predicate Returns true if the passed entry should be removed.
   template <typename Predicate> void removeEntries_if(Predicate predicate) {
      Entry *end = std::remove_if(Entries, Entries + NumEntries,
                                  [&](Entry &entry) -> bool {
                                     if (predicate(entry)) {
                                        entry.Implementation->decrementRefCount();
                                        removeFromVTableCache(entry);
                                        return true;
                                     }
                                     return false;
                                  });
      NumEntries = end - Entries;
   }

   /// Verify that the vtable is well-formed for the given class.
   void verify(const PILModule &M) const;

   /// Print the vtable.
   void print(llvm::raw_ostream &OS, bool Verbose = false) const;
   void dump() const;

private:
   void removeFromVTableCache(Entry &entry);
};

} // end polar namespace

//===----------------------------------------------------------------------===//
// ilist_traits for PILVTable
//===----------------------------------------------------------------------===//

namespace llvm {

template <>
struct ilist_traits<::polar::PILVTable> :
   public ilist_node_traits<::polar::PILVTable> {
   using PILVTable = ::polar::PILVTable;

   static void deleteNode(PILVTable *VT) { VT->~PILVTable(); }

private:
   void createNode(const PILVTable &);
};

} // end llvm namespace

#endif
