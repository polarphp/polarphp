//===--- InterfaceInfo.h - Abstract protocol witness layout ------*- C++ -*-===//
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
// This file defines types for representing the abstract layout of a
// interface.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IRGEN_INTERNAL_INTERFACE_INFO_H
#define POLARPHP_IRGEN_INTERNAL_INTERFACE_INFO_H

#include "polarphp/ast/Decl.h"
#include "polarphp/ast/InterfaceAssociations.h"

#include "polarphp/irgen/ValueWitness.h"
#include "polarphp/irgen/internal/WitnessIndex.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/TrailingObjects.h"

namespace polar {

class CanType;
class InterfaceConformance;

namespace irgen {

class IRGenModule;
class TypeInfo;

/// A witness to a specific element of a protocol.  Every
/// InterfaceTypeInfo stores one of these for each requirement
/// introduced by the protocol.
class WitnessTableEntry {
public:
   llvm::PointerUnion<Decl *, TypeBase *> MemberOrAssociatedType;
   InterfaceDecl *Interface;

   WitnessTableEntry(llvm::PointerUnion<Decl *, TypeBase *> member,
                     InterfaceDecl *protocol)
      : MemberOrAssociatedType(member), Interface(protocol) {}

public:
   WitnessTableEntry() = default;

   static WitnessTableEntry forOutOfLineBase(InterfaceDecl *proto) {
      assert(proto != nullptr);
      return WitnessTableEntry({}, proto);
   }

   /// Is this a base-protocol entry?
   bool isBase() const { return MemberOrAssociatedType.isNull(); }

   bool matchesBase(InterfaceDecl *proto) const {
      assert(proto != nullptr);
      return MemberOrAssociatedType.isNull() && Interface == proto;
   }

   /// Given that this is a base-protocol entry, is the table
   /// "out of line"?
   bool isOutOfLineBase() const {
      assert(isBase());
      return true;
   }

   InterfaceDecl *getBase() const {
      assert(isBase());
      return Interface;
   }

   static WitnessTableEntry forFunction(AbstractFunctionDecl *func) {
      assert(func != nullptr);
      return WitnessTableEntry(func, nullptr);
   }

   bool isFunction() const {
      auto decl = MemberOrAssociatedType.dyn_cast<Decl*>();
      return Interface == nullptr && decl && isa<AbstractFunctionDecl>(decl);
   }

   bool matchesFunction(AbstractFunctionDecl *func) const {
      assert(func != nullptr);
      if (auto decl = MemberOrAssociatedType.dyn_cast<Decl*>())
         return decl == func && Interface == nullptr;
      return false;
   }

   AbstractFunctionDecl *getFunction() const {
      assert(isFunction());
      auto decl = MemberOrAssociatedType.get<Decl*>();
      return static_cast<AbstractFunctionDecl*>(decl);
   }

   static WitnessTableEntry forAssociatedType(AssociatedType ty) {
      return WitnessTableEntry(ty.getAssociation(), nullptr);
   }

   bool isAssociatedType() const {
      if (auto decl = MemberOrAssociatedType.dyn_cast<Decl*>())
         return Interface == nullptr && isa<AssociatedTypeDecl>(decl);
      return false;
   }

   bool matchesAssociatedType(AssociatedType assocType) const {
      if (auto decl = MemberOrAssociatedType.dyn_cast<Decl*>())
         return decl == assocType.getAssociation() && Interface == nullptr;
      return false;
   }

   AssociatedTypeDecl *getAssociatedType() const {
      assert(isAssociatedType());
      auto decl = MemberOrAssociatedType.get<Decl*>();
      return static_cast<AssociatedTypeDecl*>(decl);
   }

   static WitnessTableEntry forAssociatedConformance(AssociatedConformance conf){
      return WitnessTableEntry(conf.getAssociation().getPointer(),
                               conf.getAssociatedRequirement());
   }

   bool isAssociatedConformance() const {
      return Interface != nullptr && !MemberOrAssociatedType.isNull();
   }

   bool matchesAssociatedConformance(const AssociatedConformance &conf) const {
      if (auto type = MemberOrAssociatedType.dyn_cast<TypeBase*>())
         return type == conf.getAssociation().getPointer() &&
                Interface == conf.getAssociatedRequirement();
      return false;
   }

   CanType getAssociatedConformancePath() const {
      assert(isAssociatedConformance());
      auto type = MemberOrAssociatedType.get<TypeBase*>();
      return CanType(type);
   }

   InterfaceDecl *getAssociatedConformanceRequirement() const {
      assert(isAssociatedConformance());
      return Interface;
   }

   friend bool operator==(WitnessTableEntry left, WitnessTableEntry right) {
      return left.MemberOrAssociatedType == right.MemberOrAssociatedType &&
             left.Interface == right.Interface;
   }
};

/// Describes the information available in a InterfaceInfo.
///
/// Each kind includes the information of the kinds before it.
enum class InterfaceInfoKind : uint8_t {
   RequirementSignature,
   Full
};

/// An abstract description of a protocol.
class InterfaceInfo final :
   private llvm::TrailingObjects<InterfaceInfo, WitnessTableEntry> {
   friend TrailingObjects;
   friend class TypeConverter;

   /// The number of table entries in this protocol layout.
   unsigned NumTableEntries;

   InterfaceInfoKind Kind;

   InterfaceInfoKind getKind() const {
      return Kind;
   }

   InterfaceInfo(ArrayRef<WitnessTableEntry> table, InterfaceInfoKind kind)
      : NumTableEntries(table.size()), Kind(kind) {
      std::uninitialized_copy(table.begin(), table.end(),
                              getTrailingObjects<WitnessTableEntry>());
   }

   static std::unique_ptr<InterfaceInfo> create(ArrayRef<WitnessTableEntry> table,
                                                InterfaceInfoKind kind);

public:
   /// The number of witness slots in a conformance to this protocol;
   /// in other words, the size of the table in words.
   unsigned getNumWitnesses() const {
      assert(getKind() == InterfaceInfoKind::Full);
      return NumTableEntries;
   }

   /// Return all of the entries in this protocol witness table.
   ///
   /// The addresses of the entries in this array can be passed to
   /// getBaseWitnessIndex/getNonBaseWitnessIndex, below.
   ArrayRef<WitnessTableEntry> getWitnessEntries() const {
      return {getTrailingObjects<WitnessTableEntry>(), NumTableEntries};
   }

   /// Given the address of a witness entry from this PI for a base protocol
   /// conformance, return its witness index.
   WitnessIndex getBaseWitnessIndex(const WitnessTableEntry *witness) const {
      assert(witness && witness->isBase());
      auto entries = getWitnessEntries();
      assert(entries.begin() <= witness && witness < entries.end() &&
             "argument witness entry does not belong to this InterfaceInfo");
      if (witness->isOutOfLineBase()) {
         return WitnessIndex(witness - entries.begin(), false);
      } else {
         return WitnessIndex(0, true);
      }
   }

   /// Given the address of a witness entry from this PI for a non-base
   /// witness, return its witness index.
   WitnessIndex getNonBaseWitnessIndex(const WitnessTableEntry *witness) const {
      assert(witness && !witness->isBase());
      auto entries = getWitnessEntries();
      assert(entries.begin() <= witness && witness < entries.end());
      return WitnessIndex(witness - entries.begin(), false);
   }

   /// Return the witness index for the protocol conformance pointer
   /// for the given base protocol requirement.
   WitnessIndex getBaseIndex(InterfaceDecl *protocol) const {
      for (auto &witness : getWitnessEntries()) {
         if (witness.matchesBase(protocol))
            return getBaseWitnessIndex(&witness);
      }
      llvm_unreachable("didn't find entry for base");
   }

   /// Return the witness index for the witness function for the given
   /// function requirement.
   WitnessIndex getFunctionIndex(AbstractFunctionDecl *function) const {
      assert(getKind() >= InterfaceInfoKind::Full);
      for (auto &witness : getWitnessEntries()) {
         if (witness.matchesFunction(function))
            return getNonBaseWitnessIndex(&witness);
      }
      llvm_unreachable("didn't find entry for function");
   }

   /// Return the witness index for the type metadata access function
   /// for the given associated type.
   WitnessIndex getAssociatedTypeIndex(IRGenModule &IGM,
                                       AssociatedType assocType) const;

   /// Return the witness index for the protocol witness table access
   /// function for the given associated protocol conformance.
   WitnessIndex
   getAssociatedConformanceIndex(const AssociatedConformance &conf) const {
      for (auto &witness : getWitnessEntries()) {
         if (witness.matchesAssociatedConformance(conf))
            return getNonBaseWitnessIndex(&witness);
      }
      llvm_unreachable("didn't find entry for associated conformance");
   }
};

/// Detail about how an object conforms to a protocol.
class ConformanceInfo {
   virtual void anchor();
public:
   virtual ~ConformanceInfo() = default;
   virtual llvm::Value *getTable(IRGenFunction &IGF,
                                 llvm::Value **conformingMetadataCache) const = 0;
   /// Try to get this table as a constant pointer.  This might just
   /// not be supportable at all.
   virtual llvm::Constant *tryGetConstantTable(IRGenModule &IGM,
                                               CanType conformingType) const = 0;
};

} // end namespace irgen
} // end namespace polar

#endif // POLARPHP_IRGEN_INTERNAL_INTERFACE_INFO_H
