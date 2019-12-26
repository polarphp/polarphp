//===-- LocalTypeDataKind.h - Kinds of locally-cached type data -*- C++ -*-===//
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
//  This file defines the LocalTypeDataKind class, which opaquely
//  represents a particular kind of local type data that we might
//  want to cache during emission.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IRGEN_INTERNAL_LOCALTYPEDATAKIND_H
#define POLARPHP_IRGEN_INTERNAL_LOCALTYPEDATAKIND_H

#include "polarphp/ast/InterfaceConformanceRef.h"
#include "polarphp/ast/Type.h"
#include <stdint.h>
#include "llvm/ADT/DenseMapInfo.h"

namespace polar {
class InterfaceDecl;

namespace irgen {

enum class ValueWitness : unsigned;

/// The kind of local type data we might want to store for a type.
class LocalTypeDataKind {
public:
   using RawType = uintptr_t;
private:
   RawType Value;

   explicit LocalTypeDataKind(RawType Value) : Value(Value) {}

   /// Magic values for special kinds of type metadata.  These should be
   /// small so that they should never conflict with a valid pointer.
   ///
   /// Since this representation is opaque, we don't worry about being able
   /// to distinguish different kinds of pointer; we just assume that e.g. a
   /// InterfaceConformance will never have the same address as a Decl.
   enum : RawType {
      FormalTypeMetadata,
      RepresentationTypeMetadata,
      ValueWitnessTable,
      // <- add more special cases here

      // The first enumerator for an individual value witness.
         ValueWitnessBase,

      FirstPayloadValue = 2048,
      Kind_Decl = 0,
      Kind_Conformance = 1,
      KindMask = 0x1,
   };

public:
   LocalTypeDataKind() = default;

   // The magic values are all odd and so do not collide with pointer values.

   /// A reference to the formal type metadata.
   static LocalTypeDataKind forFormalTypeMetadata() {
      return LocalTypeDataKind(FormalTypeMetadata);
   }

   /// A reference to type metadata for a representation-compatible type.
   static LocalTypeDataKind forRepresentationTypeMetadata() {
      return LocalTypeDataKind(RepresentationTypeMetadata);
   }

   /// A reference to the value witness table for a representation-compatible
   /// type.
   static LocalTypeDataKind forValueWitnessTable() {
      return LocalTypeDataKind(ValueWitnessTable);
   }

   /// A reference to a specific value witness for a representation-compatible
   /// type.
   static LocalTypeDataKind forValueWitness(ValueWitness witness) {
      return LocalTypeDataKind(ValueWitnessBase + (unsigned)witness);
   }

   /// A reference to a protocol witness table for an archetype.
   ///
   /// This only works for non-concrete types because in principle we might
   /// have multiple concrete conformances for a concrete type used in the
   /// same function.
   static LocalTypeDataKind
   forAbstractInterfaceWitnessTable(InterfaceDecl *protocol) {
      assert(protocol && "protocol reference may not be null");
      return LocalTypeDataKind(uintptr_t(protocol) | Kind_Decl);
   }

   /// A reference to a protocol witness table for a concrete type.
   static LocalTypeDataKind
   forConcreteInterfaceWitnessTable(InterfaceConformance *conformance) {
      assert(conformance && "conformance reference may not be null");
      return LocalTypeDataKind(uintptr_t(conformance) | Kind_Conformance);
   }

   static LocalTypeDataKind
   forInterfaceWitnessTable(InterfaceConformanceRef conformance) {
      if (conformance.isConcrete()) {
         return forConcreteInterfaceWitnessTable(conformance.getConcrete());
      } else {
         return forAbstractInterfaceWitnessTable(conformance.getAbstract());
      }
   }

   LocalTypeDataKind getCachingKind() const;

   bool isAnyTypeMetadata() const {
      return Value == FormalTypeMetadata ||
             Value == RepresentationTypeMetadata;
   }

   bool isSingletonKind() const {
      return (Value < FirstPayloadValue);
   }

   bool isConcreteInterfaceConformance() const {
      return (!isSingletonKind() &&
              ((Value & KindMask) == Kind_Conformance));
   }

   InterfaceConformance *getConcreteInterfaceConformance() const {
      assert(isConcreteInterfaceConformance());
      return reinterpret_cast<InterfaceConformance*>(Value - Kind_Conformance);
   }

   bool isAbstractInterfaceConformance() const {
      return (!isSingletonKind() &&
              ((Value & KindMask) == Kind_Decl));
   }

   InterfaceDecl *getAbstractInterfaceConformance() const {
      assert(isAbstractInterfaceConformance());
      return reinterpret_cast<InterfaceDecl*>(Value - Kind_Decl);
   }

   InterfaceConformanceRef getInterfaceConformance() const {
      assert(!isSingletonKind());
      if ((Value & KindMask) == Kind_Decl) {
         return InterfaceConformanceRef(getAbstractInterfaceConformance());
      } else {
         return InterfaceConformanceRef(getConcreteInterfaceConformance());
      }
   }

   RawType getRawValue() const {
      return Value;
   }

   void dump() const;
   void print(llvm::raw_ostream &out) const;

   bool operator==(LocalTypeDataKind other) const {
      return Value == other.Value;
   }
   bool operator!=(LocalTypeDataKind other) const {
      return Value != other.Value;
   }
};

class LocalTypeDataKey {
public:
   CanType Type;
   LocalTypeDataKind Kind;

   LocalTypeDataKey(CanType type, LocalTypeDataKind kind)
      : Type(type), Kind(kind) {}

   LocalTypeDataKey getCachingKey() const;

   bool operator==(const LocalTypeDataKey &other) const {
      return Type == other.Type && Kind == other.Kind;
   }

   void dump() const;
   void print(llvm::raw_ostream &out) const;
};

} // irgen
} // polar

namespace llvm {
template <> struct DenseMapInfo<polar::irgen::LocalTypeDataKey> {
   using LocalTypeDataKey = polar::irgen::LocalTypeDataKey;
   using CanTypeInfo = DenseMapInfo<polar::CanType>;
   static inline LocalTypeDataKey getEmptyKey() {
      return { CanTypeInfo::getEmptyKey(),
               polar::irgen::LocalTypeDataKind::forFormalTypeMetadata() };
   }
   static inline LocalTypeDataKey getTombstoneKey() {
      return { CanTypeInfo::getTombstoneKey(),
               polar::irgen::LocalTypeDataKind::forFormalTypeMetadata() };
   }
   static unsigned getHashValue(const LocalTypeDataKey &key) {
      return hash_combine(CanTypeInfo::getHashValue(key.Type),
                              key.Kind.getRawValue());
   }
   static bool isEqual(const LocalTypeDataKey &a, const LocalTypeDataKey &b) {
      return a == b;
   }
};
}

#endif // POLARPHP_IRGEN_INTERNAL_LOCALTYPEDATAKIND_H
