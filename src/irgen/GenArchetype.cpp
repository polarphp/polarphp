//===--- GenArchetype.cpp - Swift IR Generation for Archetype Types -------===//
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
//  This file implements IR generation for archetype types in Swift.
//
//===----------------------------------------------------------------------===//

#include "polarphp/irgen/internal/GenArchetype.h"

#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/GenericEnvironment.h"
#include "polarphp/ast/IRGenOptions.h"
#include "polarphp/ast/Types.h"
#include "polarphp/irgen/Linking.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/lang/TypeLowering.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "polarphp/irgen/internal/EnumPayload.h"
#include "polarphp/irgen/internal/Explosion.h"
#include "polarphp/irgen/internal/FixedTypeInfo.h"
#include "polarphp/irgen/internal/GenClass.h"
#include "polarphp/irgen/internal/GenHeap.h"
#include "polarphp/irgen/internal/GenMeta.h"
#include "polarphp/irgen/internal/GenOpaque.h"
#include "polarphp/irgen/internal/GenPoly.h"
#include "polarphp/irgen/internal/GenInterface.h"
#include "polarphp/irgen/internal/GenType.h"
#include "polarphp/irgen/internal/HeapTypeInfo.h"
#include "polarphp/irgen/internal/IRGenDebugInfo.h"
#include "polarphp/irgen/internal/IRGenFunction.h"
#include "polarphp/irgen/internal/IRGenModule.h"
#include "polarphp/irgen/internal/MetadataRequest.h"
#include "polarphp/irgen/internal/Outlining.h"
#include "polarphp/irgen/internal/InterfaceInfo.h"
#include "polarphp/irgen/internal/ResilientTypeInfo.h"
#include "polarphp/irgen/internal/TypeInfo.h"

using namespace polar;
using namespace irgen;

MetadataResponse
irgen::emitArchetypeTypeMetadataRef(IRGenFunction &IGF,
                                    CanArchetypeType archetype,
                                    DynamicMetadataRequest request) {
   // Check for an existing cache entry.
   if (auto response = IGF.tryGetLocalTypeMetadata(archetype, request))
      return response;

   // If this is an opaque archetype, we'll need to instantiate using its
   // descriptor.
   if (auto opaque = dyn_cast<OpaqueTypeArchetypeType>(archetype)) {
      return emitOpaqueTypeMetadataRef(IGF, opaque, request);
   }

   // If there's no local or opaque metadata, it must be a nested type.
   auto nested = cast<NestedArchetypeType>(archetype);

   CanArchetypeType parent(nested->getParent());
   AssociatedType association(nested->getAssocType());

   MetadataResponse response =
      emitAssociatedTypeMetadataRef(IGF, parent, association, request);

   setTypeMetadataName(IGF.IGM, response.getMetadata(), archetype);

   IGF.setScopedLocalTypeMetadata(archetype, response);

   return response;
}

namespace {

/// A type implementation for an ArchetypeType, otherwise known as a
/// type variable: for example, Self in a interface declaration, or T
/// in a generic declaration like foo<T>(x : T) -> T.  The critical
/// thing here is that performing an operation involving archetypes
/// is dependent on the witness binding we can see.
class OpaqueArchetypeTypeInfo
   : public ResilientTypeInfo<OpaqueArchetypeTypeInfo>
{
   OpaqueArchetypeTypeInfo(llvm::Type *type)
      : ResilientTypeInfo(type, IsABIAccessible) {}

public:
   static const OpaqueArchetypeTypeInfo *create(llvm::Type *type) {
      return new OpaqueArchetypeTypeInfo(type);
   }

   void collectMetadataForOutlining(OutliningMetadataCollector &collector,
                                    PILType T) const override {
      // We'll need formal type metadata for this archetype.
      collector.collectTypeMetadataForLayout(T);
   }
};

/// A type implementation for a class archetype, that is, an archetype
/// bounded by a class interface constraint. These archetypes can be
/// represented by a refcounted pointer instead of an opaque value buffer.
/// If ObjC interop is disabled, we can use Swift refcounting entry
/// points, otherwise we have to use the unknown ones.
class ClassArchetypeTypeInfo
   : public HeapTypeInfo<ClassArchetypeTypeInfo>
{
   ReferenceCounting RefCount;

   ClassArchetypeTypeInfo(llvm::PointerType *storageType,
                          Size size, const SpareBitVector &spareBits,
                          Alignment align,
                          ReferenceCounting refCount)
      : HeapTypeInfo(storageType, size, spareBits, align),
        RefCount(refCount)
   {}

public:
   static const ClassArchetypeTypeInfo *create(llvm::PointerType *storageType,
                                               Size size, const SpareBitVector &spareBits,
                                               Alignment align,
                                               ReferenceCounting refCount) {
      return new ClassArchetypeTypeInfo(storageType, size, spareBits, align,
                                        refCount);
   }

   ReferenceCounting getReferenceCounting() const {
      return RefCount;
   }
};

class FixedSizeArchetypeTypeInfo
   : public PODSingleScalarTypeInfo<FixedSizeArchetypeTypeInfo, LoadableTypeInfo>
{
   FixedSizeArchetypeTypeInfo(llvm::Type *type, Size size, Alignment align,
                              const SpareBitVector &spareBits)
      : PODSingleScalarTypeInfo(type, size, spareBits, align) {}

public:
   static const FixedSizeArchetypeTypeInfo *
   create(llvm::Type *type, Size size, Alignment align,
          const SpareBitVector &spareBits) {
      return new FixedSizeArchetypeTypeInfo(type, size, align, spareBits);
   }
};

} // end anonymous namespace

/// Emit a single interface witness table reference.
llvm::Value *irgen::emitArchetypeWitnessTableRef(IRGenFunction &IGF,
                                                 CanArchetypeType archetype,
                                                 InterfaceDecl *interface) {
   assert(lowering::TypeConverter::interfaceRequiresWitnessTable(interface) &&
          "looking up witness table for interface that doesn't have one");

   // The following approach assumes that a interface will only appear in
   // an archetype's conformsTo array if the archetype is either explicitly
   // constrained to conform to that interface (in which case we should have
   // a cache entry for it) or there's an associated type declaration with
   // that interface listed as a direct requirement.

   auto localDataKind =
      LocalTypeDataKind::forAbstractInterfaceWitnessTable(interface);

   // Check immediately for an existing cache entry.
   // TODO: don't give this absolute precedence over other access paths.
   auto wtable = IGF.tryGetLocalTypeData(archetype, localDataKind);
   if (wtable) return wtable;

   auto origRoot = archetype->getRoot();
   auto environment = origRoot->getGenericEnvironment();

   // Otherwise, ask the generic signature for the environment for the best
   // path to the conformance.
   // TODO: this isn't necessarily optimal if the direct conformance isn't
   // concretely available; we really ought to be comparing the full paths
   // to this conformance from concrete sources.

   auto signature = environment->getGenericSignature()->getCanonicalSignature();
   auto archetypeDepType = archetype->getInterfaceType();

   auto astPath = signature->getConformanceAccessPath(archetypeDepType,
                                                      interface);

   auto i = astPath.begin(), e = astPath.end();
   assert(i != e && "empty path!");

   // The first entry in the path is a direct requirement of the signature,
   // for which we should always have local type data available.
   CanType rootArchetype =
      environment->mapTypeIntoContext(i->first)->getCanonicalType();
   InterfaceDecl *rootInterface = i->second;

   // Turn the rest of the path into a MetadataPath.
   auto lastInterface = rootInterface;
   MetadataPath path;
   while (++i != e) {
      auto &entry = *i;
      CanType depType = CanType(entry.first);
      InterfaceDecl *requirement = entry.second;

      const InterfaceInfo &lastPI =
         IGF.IGM.getInterfaceInfo(lastInterface,
                                  InterfaceInfoKind::RequirementSignature);

      // If it's a type parameter, it's self, and this is a base interface
      // requirement.
      if (isa<GenericTypeParamType>(depType)) {
         assert(depType->isEqual(lastInterface->getSelfInterfaceType()));
         WitnessIndex index = lastPI.getBaseIndex(requirement);
         path.addInheritedInterfaceComponent(index);

         // Otherwise, it's an associated conformance requirement.
      } else {
         AssociatedConformance association(lastInterface, depType, requirement);
         WitnessIndex index = lastPI.getAssociatedConformanceIndex(association);
         path.addAssociatedConformanceComponent(index);
      }

      lastInterface = requirement;
   }
   assert(lastInterface == interface);

   auto rootWTable = IGF.tryGetLocalTypeData(rootArchetype,
                                             LocalTypeDataKind::forAbstractInterfaceWitnessTable(rootInterface));
   // Fetch an opaque type's witness table if it wasn't cached yet.
   if (!rootWTable) {
      if (auto opaqueRoot = dyn_cast<OpaqueTypeArchetypeType>(rootArchetype)) {
         rootWTable = emitOpaqueTypeWitnessTableRef(IGF, opaqueRoot,
                                                    rootInterface);
      }
      assert(rootWTable && "root witness table not bound in local context!");
   }

   wtable = path.followFromWitnessTable(IGF, rootArchetype,
                                        InterfaceConformanceRef(rootInterface),
                                        MetadataResponse::forComplete(rootWTable),
      /*request*/ MetadataState::Complete,
                                        nullptr).getMetadata();

   return wtable;
}

MetadataResponse
irgen::emitAssociatedTypeMetadataRef(IRGenFunction &IGF,
                                     CanArchetypeType origin,
                                     AssociatedType association,
                                     DynamicMetadataRequest request) {
   // Find the conformance of the origin to the associated type's interface.
   llvm::Value *wtable = emitArchetypeWitnessTableRef(IGF, origin,
                                                      association.getSourceInterface());

   // Find the origin's type metadata.
   llvm::Value *originMetadata =
      emitArchetypeTypeMetadataRef(IGF, origin, MetadataState::Abstract)
         .getMetadata();

   return emitAssociatedTypeMetadataRef(IGF, originMetadata, wtable,
                                        association, request);
}

const TypeInfo *TypeConverter::convertArchetypeType(ArchetypeType *archetype) {
   assert(isExemplarArchetype(archetype) && "lowering non-exemplary archetype");

   auto layout = archetype->getLayoutConstraint();

   // If the archetype is class-constrained, use a class pointer
   // representation.
   if (archetype->requiresClass() ||
       (layout && layout->isRefCounted())) {
      auto refcount = archetype->getReferenceCounting();

      llvm::PointerType *reprTy;

      // If the archetype has a superclass constraint, it has at least the
      // retain semantics of its superclass, and it can be represented with
      // the supertype's pointer type.
      if (auto super = archetype->getSuperclass()) {
         auto &superTI = IGM.getTypeInfoForUnlowered(super);
         reprTy = cast<llvm::PointerType>(superTI.StorageType);
      } else {
         if (refcount == ReferenceCounting::Native) {
            reprTy = IGM.RefCountedPtrTy;
         } else {
            reprTy = IGM.UnknownRefCountedPtrTy;
         }
      }

      // As a hack, assume class archetypes never have spare bits. There's a
      // corresponding hack in MultiPayloadEnumImplStrategy::completeEnumTypeLayout
      // to ignore spare bits of dependent-typed payloads.
      auto spareBits =
         SpareBitVector::getConstant(IGM.getPointerSize().getValueInBits(), false);

      return ClassArchetypeTypeInfo::create(reprTy,
                                            IGM.getPointerSize(),
                                            spareBits,
                                            IGM.getPointerAlignment(),
                                            refcount);
   }

   // If the archetype is trivial fixed-size layout-constrained, use a fixed size
   // representation.
   if (layout && layout->isFixedSizeTrivial()) {
      Size size(layout->getTrivialSizeInBytes());
      auto layoutAlignment = layout->getAlignmentInBytes();
      assert(layoutAlignment && "layout constraint alignment should not be 0");
      Alignment align(layoutAlignment);
      auto spareBits =
         SpareBitVector::getConstant(size.getValueInBits(), false);
      // Get an integer type of the required size.
      auto ProperlySizedIntTy = PILType::getBuiltinIntegerType(
         size.getValueInBits(), IGM.getTypePHPModule()->getAstContext());
      auto storageType = IGM.getStorageType(ProperlySizedIntTy);
      return FixedSizeArchetypeTypeInfo::create(storageType, size, align,
                                                spareBits);
   }

   // If the archetype is a trivial layout-constrained, use a POD
   // representation. This type is not loadable, but it is known
   // to be a POD.
   if (layout && layout->isAddressOnlyTrivial()) {
      // TODO: Create NonFixedSizeArchetypeTypeInfo and return it.
   }

   // Otherwise, for now, always use an opaque indirect type.
   llvm::Type *storageType = IGM.OpaquePtrTy->getElementType();
   return OpaqueArchetypeTypeInfo::create(storageType);
}

static void setMetadataRef(IRGenFunction &IGF,
                           ArchetypeType *archetype,
                           llvm::Value *metadata,
                           MetadataState metadataState) {
   assert(metadata->getType() == IGF.IGM.TypeMetadataPtrTy);
   IGF.setUnscopedLocalTypeMetadata(CanType(archetype),
                                    MetadataResponse::forBounded(metadata, metadataState));
}

static void setWitnessTable(IRGenFunction &IGF,
                            ArchetypeType *archetype,
                            unsigned interfaceIndex,
                            llvm::Value *wtable) {
   assert(wtable->getType() == IGF.IGM.WitnessTablePtrTy);
   assert(interfaceIndex < archetype->getConformsTo().size());
   auto interface = archetype->getConformsTo()[interfaceIndex];
   IGF.setUnscopedLocalTypeData(CanType(archetype),
                                LocalTypeDataKind::forAbstractInterfaceWitnessTable(interface),
                                wtable);
}

/// Inform IRGenFunction that the given archetype has the given value
/// witness value within this scope.
void IRGenFunction::bindArchetype(ArchetypeType *archetype,
                                  llvm::Value *metadata,
                                  MetadataState metadataState,
                                  ArrayRef<llvm::Value*> wtables) {
   // Set the metadata pointer.
   setTypeMetadataName(IGM, metadata, CanType(archetype));
   setMetadataRef(*this, archetype, metadata, metadataState);

   // Set the interface witness tables.

   unsigned wtableI = 0;
   for (unsigned i = 0, e = archetype->getConformsTo().size(); i != e; ++i) {
      auto proto = archetype->getConformsTo()[i];
      if (!lowering::TypeConverter::interfaceRequiresWitnessTable(proto))
         continue;
      auto wtable = wtables[wtableI++];
      setInterfaceWitnessTableName(IGM, wtable, CanType(archetype), proto);
      setWitnessTable(*this, archetype, i, wtable);
   }
   assert(wtableI == wtables.size());
}

llvm::Value *irgen::emitDynamicTypeOfOpaqueArchetype(IRGenFunction &IGF,
                                                     Address addr,
                                                     PILType type) {
   auto archetype = type.castTo<ArchetypeType>();
   // Acquire the archetype's static metadata.
   llvm::Value *metadata =
      emitArchetypeTypeMetadataRef(IGF, archetype, MetadataState::Complete)
         .getMetadata();
   return IGF.Builder.CreateCall(IGF.IGM.getGetDynamicTypeFn(),
                                 {addr.getAddress(), metadata,
                                  llvm::ConstantInt::get(IGF.IGM.Int1Ty, 0)});
}

static void
withOpaqueTypeGenericArgs(IRGenFunction &IGF,
                          CanOpaqueTypeArchetypeType archetype,
                          llvm::function_ref<void (llvm::Value*)> body) {
   // Collect the generic arguments of the opaque decl.
   auto opaqueDecl = archetype->getDecl();
   auto generics = opaqueDecl->getGenericSignatureOfContext();
   llvm::Value *genericArgs;
   Address alloca;
   Size allocaSize(0);
   if (!generics || generics->areAllParamsConcrete()) {
      genericArgs = llvm::UndefValue::get(IGF.IGM.Int8PtrTy);
   } else {
      SmallVector<llvm::Value *, 4> args;
      SmallVector<llvm::Type *, 4> types;

      enumerateGenericSignatureRequirements(
         opaqueDecl->getGenericSignature()->getCanonicalSignature(),
         [&](GenericRequirement reqt) {
            auto ty = reqt.TypeParameter.subst(archetype->getSubstitutions())
               ->getCanonicalType(opaqueDecl->getGenericSignature());
            if (reqt.Interface) {
               auto ref = InterfaceConformanceRef(reqt.Interface)
                  .subst(reqt.TypeParameter, archetype->getSubstitutions());
               args.push_back(emitWitnessTableRef(IGF, ty, ref));
            } else {
               args.push_back(IGF.emitAbstractTypeMetadataRef(ty));
            }
            types.push_back(args.back()->getType());
         });
      auto bufTy = llvm::StructType::get(IGF.IGM.LLVMContext, types);
      alloca = IGF.createAlloca(bufTy, IGF.IGM.getPointerAlignment());
      allocaSize = IGF.IGM.getPointerSize() * args.size();
      IGF.Builder.CreateLifetimeStart(alloca, allocaSize);
      for (auto i : indices(args)) {
         IGF.Builder.CreateStore(args[i],
                                 IGF.Builder.CreateStructGEP(alloca, i, i * IGF.IGM.getPointerSize()));
      }
      genericArgs = IGF.Builder.CreateBitCast(alloca.getAddress(),
                                              IGF.IGM.Int8PtrTy);
   }

   // Pass them down to the body.
   body(genericArgs);

   // End the alloca's lifetime, if we allocated one.
   if (alloca.getAddress()) {
      IGF.Builder.CreateLifetimeEnd(alloca, allocaSize);
   }
}

bool shouldUseOpaqueTypeDescriptorAccessor(OpaqueTypeDecl *opaque) {
   auto *namingDecl = opaque->getNamingDecl();

   // Don't emit accessors for abstract storage that is not dynamic or a dynamic
   // replacement.
   if (auto *abstractStorage = dyn_cast<AbstractStorageDecl>(namingDecl)) {
      return abstractStorage->hasAnyNativeDynamicAccessors() ||
             abstractStorage->getDynamicallyReplacedDecl();
   }

   // Don't emit accessors for functions that are not dynamic or dynamic
   // replacements.
   return namingDecl->isNativeDynamic() ||
          (bool)namingDecl->getDynamicallyReplacedDecl();
}

static llvm::Value *
getAddressOfOpaqueTypeDescriptor(IRGenFunction &IGF,
                                 OpaqueTypeDecl *opaqueDecl) {
   auto &IGM = IGF.IGM;

   // Support dynamically replacing the return type as part of dynamic function
   // replacement.
   if (!IGM.getOptions().shouldOptimize() &&
       shouldUseOpaqueTypeDescriptorAccessor(opaqueDecl)) {
      auto descriptorAccessor = IGM.getAddrOfOpaqueTypeDescriptorAccessFunction(
         opaqueDecl, NotForDefinition, false);
      auto desc = IGF.Builder.CreateCall(descriptorAccessor, {});
      desc->setDoesNotThrow();
      desc->setCallingConv(IGM.SwiftCC);
      return desc;
   }
   return IGM.getAddrOfOpaqueTypeDescriptor(opaqueDecl, ConstantInit());
}

MetadataResponse irgen::emitOpaqueTypeMetadataRef(IRGenFunction &IGF,
                                                  CanOpaqueTypeArchetypeType archetype,
                                                  DynamicMetadataRequest request) {
   auto accessorFn = IGF.IGM.getGetOpaqueTypeMetadataFn();
   auto opaqueDecl = archetype->getDecl();

   auto *descriptor = getAddressOfOpaqueTypeDescriptor(IGF, opaqueDecl);
   auto indexValue = llvm::ConstantInt::get(IGF.IGM.SizeTy, 0);

   llvm::CallInst *result = nullptr;
   withOpaqueTypeGenericArgs(IGF, archetype,
                             [&](llvm::Value *genericArgs) {
                                result = IGF.Builder.CreateCall(accessorFn,
                                                                {request.get(IGF), genericArgs, descriptor, indexValue});
                                result->setDoesNotThrow();
                                result->setCallingConv(IGF.IGM.SwiftCC);
                                result->addAttribute(llvm::AttributeList::FunctionIndex,
                                                     llvm::Attribute::ReadOnly);
                             });
   assert(result);

   auto response = MetadataResponse::handle(IGF, request, result);
   IGF.setScopedLocalTypeMetadata(archetype, response);
   return response;
}

llvm::Value *irgen::emitOpaqueTypeWitnessTableRef(IRGenFunction &IGF,
                                                  CanOpaqueTypeArchetypeType archetype,
                                                  InterfaceDecl *interface) {
   auto accessorFn = IGF.IGM.getGetOpaqueTypeConformanceFn();
   auto opaqueDecl = archetype->getDecl();


   llvm::Value *descriptor = getAddressOfOpaqueTypeDescriptor(IGF, opaqueDecl);

   auto foundInterface = std::find(archetype->getConformsTo().begin(),
                                   archetype->getConformsTo().end(),
                                   interface);
   assert(foundInterface != archetype->getConformsTo().end());

   unsigned index = foundInterface - archetype->getConformsTo().begin() + 1;
   auto indexValue = llvm::ConstantInt::get(IGF.IGM.SizeTy, index);

   llvm::CallInst *result = nullptr;
   withOpaqueTypeGenericArgs(IGF, archetype,
                             [&](llvm::Value *genericArgs) {
                                result = IGF.Builder.CreateCall(accessorFn,
                                                                {genericArgs, descriptor, indexValue});
                                result->setDoesNotThrow();
                                result->setCallingConv(IGF.IGM.SwiftCC);
                                result->addAttribute(llvm::AttributeList::FunctionIndex,
                                                     llvm::Attribute::ReadOnly);
                             });
   assert(result);

   IGF.setScopedLocalTypeData(archetype,
                              LocalTypeDataKind::forAbstractInterfaceWitnessTable(interface),
                              result);
   return result;
}
