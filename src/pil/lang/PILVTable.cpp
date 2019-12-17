//===--- PILVTable.cpp - Defines the PILVTable class ----------------------===//
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
// for a dynamic type. This information (FIXME) will be used by IRGen to lay
// out class vtables, and can be used by devirtualization passes to lower
// class_method instructions to static function_refs.
//
//===----------------------------------------------------------------------===//

#include "polarphp/pil/lang/PILVTable.h"
#include "polarphp/pil/lang/PILModule.h"

using namespace polar;

PILVTable *PILVTable::create(PILModule &M, ClassDecl *Class,
                             IsSerialized_t Serialized,
                             ArrayRef<Entry> Entries) {
   // PILVTable contains one element declared in Entries.  We must allocate
   // space for it, because its default ctor will write to it.
   unsigned NumTailElements = std::max((unsigned)Entries.size(), 1U)-1;
   void *buf = M.allocate(sizeof(PILVTable) + sizeof(Entry) * NumTailElements,
                          alignof(PILVTable));
   PILVTable *vt = ::new (buf) PILVTable(Class, Serialized, Entries);
   M.vtables.push_back(vt);
   M.VTableMap[Class] = vt;
   // Update the Module's cache with new vtable + vtable entries:
   for (const Entry &entry : Entries) {
      M.VTableEntryCache.insert({{vt, entry.Method}, entry});
   }
   return vt;
}

Optional<PILVTable::Entry>
PILVTable::getEntry(PILModule &M, PILDeclRef method) const {
   PILDeclRef m = method;
   do {
      auto entryIter = M.VTableEntryCache.find({this, m});
      if (entryIter != M.VTableEntryCache.end()) {
         return (*entryIter).second;
      }
   } while ((m = m.getOverridden()));
   return None;
}

void PILVTable::removeFromVTableCache(Entry &entry) {
   PILModule &M = entry.Implementation->getModule();
   M.VTableEntryCache.erase({this, entry.Method});
}

PILVTable::PILVTable(ClassDecl *c, IsSerialized_t serialized,
                     ArrayRef<Entry> entries)
   : Class(c), Serialized(serialized), NumEntries(entries.size()) {
   memcpy(Entries, entries.begin(), sizeof(Entry) * NumEntries);

   // Bump the reference count of functions referenced by this table.
   for (const Entry &entry : getEntries()) {
      entry.Implementation->incrementRefCount();
   }
}

PILVTable::~PILVTable() {
   // Drop the reference count of functions referenced by this table.
   for (const Entry &entry : getEntries()) {
      entry.Implementation->decrementRefCount();
   }
}
