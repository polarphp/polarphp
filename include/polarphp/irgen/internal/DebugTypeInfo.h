//===--- DebugTypeInfo.h - Type Info for Debugging --------------*- C++ -*-===//
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
// This file defines the data structure that holds all the debug info
// we want to emit for types.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IRGEN_INTERNAL_DEBUGTYPEINFO_H
#define POLARPHP_IRGEN_INTERNAL_DEBUGTYPEINFO_H

#include "polarphp/irgen/internal/IRGen.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Types.h"

namespace llvm {
class Type;
}

namespace polar {
class PILDebugScope;
class PILGlobalVariable;

namespace irgen {

class TypeInfo;

/// This data structure holds everything needed to emit debug info
/// for a type.
class DebugTypeInfo {
public:
   /// The type we need to emit may be different from the type
   /// mentioned in the Decl, for example, stripped of qualifiers.
   TypeBase *Type = nullptr;
   /// Needed to determine the size of basic types and to determine
   /// the storage type for undefined variables.
   llvm::Type *StorageType = nullptr;
   Size size = Size(0);
   Alignment align = Alignment(0);
   bool DefaultAlignment = true;
   bool IsMetadataType = false;

   DebugTypeInfo() {}
   DebugTypeInfo(polar::Type Ty, llvm::Type *StorageTy, Size SizeInBytes,
                 Alignment AlignInBytes, bool HasDefaultAlignment,
                 bool IsMetadataType);

   /// Create type for a local variable.
   static DebugTypeInfo getLocalVariable(VarDecl *Decl,
                                         polar::Type Ty, const TypeInfo &Info);
   /// Create type for global type metadata.
   static DebugTypeInfo getMetadata(polar::Type Ty, llvm::Type *StorageTy,
                                    Size size, Alignment align);
   /// Create type for an artificial metadata variable.
   static DebugTypeInfo getArchetype(polar::Type Ty, llvm::Type *StorageTy,
                                     Size size, Alignment align);

   /// Create a standalone type from a TypeInfo object.
   static DebugTypeInfo getFromTypeInfo(polar::Type Ty, const TypeInfo &Info);
   /// Global variables.
   static DebugTypeInfo getGlobal(PILGlobalVariable *GV, llvm::Type *StorageType,
                                  Size size, Alignment align);
   /// ObjC classes.
   static DebugTypeInfo getObjCClass(ClassDecl *theClass,
                                     llvm::Type *StorageType, Size size,
                                     Alignment align);
   /// Error type.
   static DebugTypeInfo getErrorResult(polar::Type Ty, llvm::Type *StorageType,
                                       Size size, Alignment align);

   TypeBase *getType() const { return Type; }

   TypeDecl *getDecl() const;

   // Determine whether this type is an Archetype dependent on a generic context.
   bool isContextArchetype() const {
      if (auto archetype = Type->getWithoutSpecifierType()->getAs<ArchetypeType>()) {
         return !isa<OpaqueTypeArchetypeType>(archetype->getRoot());
      }
      return false;
   }

   bool isNull() const { return Type == nullptr; }
   bool operator==(DebugTypeInfo T) const;
   bool operator!=(DebugTypeInfo T) const;

   void dump() const;
};
}
}

namespace llvm {

// Dense map specialization.
template <> struct DenseMapInfo<polar::irgen::DebugTypeInfo> {
   static polar::irgen::DebugTypeInfo getEmptyKey() {
      return {};
   }
   static polar::irgen::DebugTypeInfo getTombstoneKey() {
      return polar::irgen::DebugTypeInfo(
         llvm::DenseMapInfo<polar::TypeBase *>::getTombstoneKey(), nullptr,
         polar::irgen::Size(0), polar::irgen::Alignment(0), false, false);
   }
   static unsigned getHashValue(polar::irgen::DebugTypeInfo Val) {
      return DenseMapInfo<polar::CanType>::getHashValue(Val.getType());
   }
   static bool isEqual(polar::irgen::DebugTypeInfo LHS,
                       polar::irgen::DebugTypeInfo RHS) {
      return LHS == RHS;
   }
};

} // polar::irgen

#endif // POLARPHP_IRGEN_INTERNAL_DEBUGTYPEINFO_H
