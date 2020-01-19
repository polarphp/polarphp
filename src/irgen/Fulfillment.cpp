//===--- Fulfillment.cpp - Static metadata search  ------------------------===//
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
//  This file implements routines for searching for ways to find metadata
//  from other metadata.
//
//===----------------------------------------------------------------------===//

#include "polarphp/irgen/internal/Fulfillment.h"
#include "polarphp/irgen/internal/IRGenModule.h"

#include "polarphp/irgen/internal/GenericRequirement.h"
#include "polarphp/irgen/internal/MetadataRequest.h"
#include "polarphp/irgen/internal/InterfaceInfo.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/ast/SubstitutionMap.h"
#include "polarphp/pil/lang/PILWitnessTable.h"
#include "polarphp/pil/lang/TypeLowering.h"

using namespace polar;
using namespace irgen;

/// Is metadata for the given type kind a "leaf", or does it possibly
/// store any other type metadata that we can statically extract?
///
/// It's okay to conservatively answer "no".  It's more important for this
/// to be quick than for it to be accurate; don't recurse.
static bool isLeafTypeMetadata(CanType type) {
   switch (type->getKind()) {
#define SUGARED_TYPE(ID, SUPER) \
  case TypeKind::ID:
#define UNCHECKED_TYPE(ID, SUPER) \
  case TypeKind::ID:
#define TYPE(ID, SUPER)
#include "polarphp/ast/TypeNodesDef.h"
      case TypeKind::Error:
         llvm_unreachable("kind is invalid for a canonical type");

#define ARTIFICIAL_TYPE(ID, SUPER) \
  case TypeKind::ID:
#define TYPE(ID, SUPER)
#include "polarphp/ast/TypeNodesDef.h"
      case TypeKind::LValue:
      case TypeKind::InOut:
      case TypeKind::DynamicSelf:
         llvm_unreachable("these types do not have metadata");

         // All the builtin types are leaves.
#define BUILTIN_TYPE(ID, SUPER) \
  case TypeKind::ID:
#define TYPE(ID, SUPER)
#include "polarphp/ast/TypeNodesDef.h"
      case TypeKind::Module:
         return true;

         // Type parameters are statically opaque.
      case TypeKind::PrimaryArchetype:
      case TypeKind::OpenedArchetype:
      case TypeKind::NestedArchetype:
      case TypeKind::OpaqueTypeArchetype:
      case TypeKind::GenericTypeParam:
      case TypeKind::DependentMember:
         return true;

         // Only the empty tuple is a leaf.
      case TypeKind::Tuple:
         return cast<TupleType>(type)->getNumElements() == 0;

         // Nominal types might have generic parents.
      case TypeKind::Class:
      case TypeKind::Enum:
      case TypeKind::Interface:
      case TypeKind::Struct:
         return !cast<NominalType>(type)->getDecl()->isGenericContext();

         // Bound generic types have type arguments.
      case TypeKind::BoundGenericClass:
      case TypeKind::BoundGenericEnum:
      case TypeKind::BoundGenericStruct:
         return false;

         // Functions have component types.
      case TypeKind::Function:
      case TypeKind::GenericFunction:  // included for future-proofing
         return false;

         // Interface compositions have component types.
      case TypeKind::InterfaceComposition:
         return false;

         // Metatypes have instance types.
      case TypeKind::Metatype:
      case TypeKind::ExistentialMetatype:
         return false;
   }
   llvm_unreachable("bad type kind");
}

/// Given that we have a source for metadata of the given type, check
/// to see if it fulfills anything.
///
/// \param isExact - true if the metadata is known to be exactly the
///   metadata for the given type, false if it might be a subtype
bool FulfillmentMap::searchTypeMetadata(IRGenModule &IGM, CanType type,
                                        IsExact_t isExact,
                                        MetadataState metadataState,
                                        unsigned source, MetadataPath &&path,
                                        const InterestingKeysCallback &keys) {

   // If this is an exact source, and it's an interesting type, add this
   // as a fulfillment.
   if (isExact && keys.isInterestingType(type)) {
      // If the type isn't a leaf type, also check it as an inexact match.
      bool hadFulfillment = false;
      if (!isLeafTypeMetadata(type)) {
         hadFulfillment |= searchTypeMetadata(IGM, type, IsInexact, metadataState,
                                              source, MetadataPath(path), keys);
      }

      // Consider its super class bound.
      if (metadataState == MetadataState::Complete) {
         if (auto superclassTy = keys.getSuperclassBound(type)) {
            hadFulfillment |= searchNominalTypeMetadata(
               IGM, superclassTy, metadataState, source, std::move(path), keys);
         }
      }

      // Add the fulfillment.
      hadFulfillment |= addFulfillment({type, nullptr},
                                       source, std::move(path), metadataState);
      return hadFulfillment;
   }

   // Search the superclass fields.  We can only do this if the metadata
   // is complete.
   if (metadataState == MetadataState::Complete &&
       keys.isInterestingType(type)) {
      if (auto superclassTy = keys.getSuperclassBound(type)) {
         return searchNominalTypeMetadata(IGM, superclassTy, metadataState,
                                          source, std::move(path), keys);
      }
   }

   // Inexact metadata will be a problem if we ever try to use this
   // to remember that we already have the metadata for something.
   if (isa<NominalType>(type) || isa<BoundGenericType>(type)) {
      return searchNominalTypeMetadata(IGM, type, metadataState,
                                       source, std::move(path), keys);
   }

   // TODO: tuples
   // TODO: functions
   // TODO: metatypes

   return false;
}

bool FulfillmentMap::searchConformance(
   IRGenModule &IGM, const InterfaceConformance *conformance,
   unsigned sourceIndex, MetadataPath &&path,
   const InterestingKeysCallback &interestingKeys) {
   bool hadFulfillment = false;

   PILWitnessTable::enumerateWitnessTableConditionalConformances(
      conformance, [&](unsigned index, CanType type, InterfaceDecl *Interface) {
         MetadataPath conditionalPath = path;
         conditionalPath.addConditionalConformanceComponent(index);
         hadFulfillment |=
            searchWitnessTable(IGM, type, Interface, sourceIndex,
                               std::move(conditionalPath), interestingKeys);

         return /*finished?*/ false;
      });

   return hadFulfillment;
}

bool FulfillmentMap::searchWitnessTable(IRGenModule &IGM,
                                        CanType type, InterfaceDecl *Interface,
                                        unsigned source, MetadataPath &&path,
                                        const InterestingKeysCallback &keys) {
   assert(lowering::TypeConverter::interfaceRequiresWitnessTable(Interface));

   llvm::SmallPtrSet<InterfaceDecl*, 4> interestingConformancesBuffer;
   llvm::SmallPtrSetImpl<InterfaceDecl *> *interestingConformances = nullptr;

   // If the interesting-keys set is limiting the set of interesting
   // conformances, collect that filter.
   if (keys.hasInterestingType(type) &&
       keys.hasLimitedInterestingConformances(type)) {
      // Bail out immediately if the set is empty.
      // This only makes sense because we're not trying to fulfill
      // associated types this way.
      auto requiredConformances = keys.getInterestingConformances(type);
      if (requiredConformances.empty()) return false;

      interestingConformancesBuffer.insert(requiredConformances.begin(),
                                           requiredConformances.end());
      interestingConformances = &interestingConformancesBuffer;
   }

   return searchWitnessTable(IGM, type, Interface, source, std::move(path), keys,
                             interestingConformances);
}

bool FulfillmentMap::searchWitnessTable(
   IRGenModule &IGM, CanType type, InterfaceDecl *Interface, unsigned source,
   MetadataPath &&path, const InterestingKeysCallback &keys,
   llvm::SmallPtrSetImpl<InterfaceDecl *> *interestingConformances) {

   bool hadFulfillment = false;

   auto &pi = IGM.getInterfaceInfo(Interface,
                                  InterfaceInfoKind::RequirementSignature);

   for (auto &entry : pi.getWitnessEntries()) {
      if (!entry.isBase()) continue;

      InterfaceDecl *inherited = entry.getBase();
      MetadataPath inheritedPath = path;
      inheritedPath.addInheritedInterfaceComponent(pi.getBaseWitnessIndex(&entry));
      hadFulfillment |= searchWitnessTable(IGM, type, inherited,
                                           source, std::move(inheritedPath),
                                           keys, interestingConformances);
   }

   // If we're not limiting the set of interesting conformances, or if
   // this is an interesting conformance, record it.
   if (!interestingConformances || interestingConformances->count(Interface)) {
      hadFulfillment |= addFulfillment({type, Interface}, source,
                                       std::move(path), MetadataState::Complete);
   }

   return hadFulfillment;
}


bool FulfillmentMap::searchNominalTypeMetadata(IRGenModule &IGM,
                                               CanType type,
                                               MetadataState metadataState,
                                               unsigned source,
                                               MetadataPath &&path,
                                               const InterestingKeysCallback &keys) {
   // Objective-C generics don't preserve their generic parameters at runtime,
   // so they aren't able to fulfill type metadata requirements.
   if (type.getAnyNominal()->hasClangNode()) {
      return false;
   }

   auto *nominal = type.getAnyNominal();
   if (!nominal->isGenericContext() || isa<InterfaceDecl>(nominal)) {
      return false;
   }

   bool hadFulfillment = false;

   GenericTypeRequirements requirements(IGM, nominal);
   requirements.enumerateFulfillments(
      IGM, type->getContextSubstitutionMap(IGM.getTypePHPModule(), nominal),
      [&](unsigned reqtIndex, CanType arg, InterfaceConformanceRef conf) {
         // Skip uninteresting type arguments.
         if (!keys.hasInterestingType(arg))
            return;

         // If the fulfilled value is type metadata, refine the path.
         if (conf.isInvalid()) {
            auto argState =
               getPresumedMetadataStateForTypeArgument(metadataState);
            MetadataPath argPath = path;
            argPath.addNominalTypeArgumentComponent(reqtIndex);
            hadFulfillment |= searchTypeMetadata(
               IGM, arg, IsExact, argState, source, std::move(argPath), keys);
            return;
         }

         // Otherwise, it's a conformance.

         // Ignore it unless the type itself is interesting.
         if (!keys.isInterestingType(arg))
            return;

         // Refine the path.
         MetadataPath argPath = path;
         argPath.addNominalTypeArgumentConformanceComponent(reqtIndex);

         hadFulfillment |= searchWitnessTable(IGM, arg, conf.getRequirement(),
                                              source, std::move(argPath), keys);
      });

   return hadFulfillment;
}

/// Testify that there's a fulfillment at the given path.
bool FulfillmentMap::addFulfillment(FulfillmentKey key,
                                    unsigned source,
                                    MetadataPath &&path,
                                    MetadataState metadataState) {
   // Only add a fulfillment if we don't have any previous
   // fulfillment for that value or if it 's cheaper than the existing
   // fulfillment.
   auto it = Fulfillments.find(key);
   if (it != Fulfillments.end()) {
      // If the new fulfillment is worse than the existing one, ignore it.
      auto existingState = it->second.getState();
      if (!isAtLeast(metadataState, existingState)) {
         return false;
      }

      // Consider cost only if the fulfillments are equivalent in state.
      // TODO: this is potentially suboptimal, but it generally won't matter.
      if (metadataState == existingState &&
          path.cost() >= it->second.Path.cost()) {
         return false;
      }

      it->second.SourceIndex = source;
      it->second.Path = std::move(path);
      return true;
   } else {
      Fulfillments.insert({ key, Fulfillment(source, std::move(path),
                                             metadataState) });
      return true;
   }
}

static StringRef getStateName(MetadataState state) {
   switch (state) {
      case MetadataState::Complete: return "complete";
      case MetadataState::NonTransitiveComplete: return "non-transitive-complete";
      case MetadataState::LayoutComplete: return "layout-complete";
      case MetadataState::Abstract: return "abstract";
   }
   llvm_unreachable("unhandled state");
}

void FulfillmentMap::dump() const {
   auto &out = llvm::errs();
   for (auto &entry : Fulfillments) {
      out << "(" << entry.first.first;
      if (auto proto = entry.first.second) {
         out << ", " << proto->getNameStr();
      }
      out << ") => " << getStateName(entry.second.getState())
          << " at sources[" << entry.second.SourceIndex
          << "]." << entry.second.Path << "\n";
   }
}
