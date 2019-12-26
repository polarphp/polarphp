//===--- Linking.h - Named declarations and how to link to them -*- C++ -*-===//
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

#ifndef POLARPHP_IRGEN_LINKING_H
#define POLARPHP_IRGEN_LINKING_H

#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/InterfaceAssociations.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/ast/Types.h"
#include "polarphp/irgen/ValueWitness.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILGlobalVariable.h"
#include "polarphp/pil/lang/PILModule.h"

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/GlobalObject.h"
#include "llvm/IR/Module.h"

namespace llvm {
   class Triple;
}

namespace polar {
   class AvailabilityContext;

   namespace irgen {
      class IRGenModule;
      class Alignment;

/// Determine if the triple uses the DLL storage.
      bool useDllStorage(const llvm::Triple &triple);

      class UniversalLinkageInfo {
         public:
         bool IsELFObject;
         bool UseDLLStorage;

         /// True iff are multiple llvm modules.
         bool HasMultipleIGMs;

         /// When this is true, the linkage for forward-declared private symbols will
         /// be promoted to public external. Used by the LLDB expression evaluator.
         bool ForcePublicDecls;

         bool IsWholeModule;

         explicit UniversalLinkageInfo(IRGenModule &IGM);

         UniversalLinkageInfo(const llvm::Triple &triple, bool hasMultipleIGMs,
         bool forcePublicDecls, bool isWholeModule);

         /// In case of multiple llvm modules (in multi-threaded compilation) all
         /// private decls must be visible from other files.
         bool shouldAllPrivateDeclsBeVisibleFromOtherFiles() const {
            return HasMultipleIGMs;
         }
         /// In case of multipe llvm modules, private lazy protocol
         /// witness table accessors could be emitted by two different IGMs during
         /// IRGen into different object files and the linker would complain about
         /// duplicate symbols.
         bool needLinkerToMergeDuplicateSymbols() const { return HasMultipleIGMs; }

         /// This  is used  by  the  LLDB expression  evaluator  since an  expression's
         /// llvm::Module  may   need  to  access   private  symbols  defined   in  the
         /// expression's  context.  This  flag  ensures  that  private  accessors  are
         /// forward-declared as public external in the expression's module.
         bool forcePublicDecls() const { return ForcePublicDecls; }
      };

/// Selector for type metadata symbol kinds.
      enum class TypeMetadataAddress {
         AddressPoint,
         FullMetadata,
      };

/// A link entity is some sort of named declaration, combined with all
/// the information necessary to distinguish specific implementations
/// of the declaration from each other.
///
/// For example, functions may be uncurried at different levels, each of
/// which potentially creates a different top-level function.
      class LinkEntity {
         /// ValueDecl*, PILFunction*, or TypeBase*, depending on Kind.
         void *Pointer;

         /// InterfaceConformance*, depending on Kind.
         void *SecondaryPointer;

         /// A hand-rolled bitfield with the following layout:
         unsigned Data;

         enum : unsigned {
            KindShift = 0, KindMask = 0xFF,

               // This field appears in the ValueWitness kind.
            ValueWitnessShift = 8, ValueWitnessMask = 0xFF00,

               // This field appears in the TypeMetadata and ObjCResilientClassStub kinds.
            MetadataAddressShift = 8, MetadataAddressMask = 0x0300,

               // This field appears in associated type access functions.
            AssociatedTypeIndexShift = 8, AssociatedTypeIndexMask = ~KindMask,

               // This field appears in associated conformance access functions.
            AssociatedConformanceIndexShift = 8,
            AssociatedConformanceIndexMask = ~KindMask,

               // This field appears in PILFunction.
            IsDynamicallyReplaceableImplShift = 8,
            IsDynamicallyReplaceableImplMask = ~KindMask,
         };
#define LINKENTITY_SET_FIELD(field, value) (value << field##Shift)
#define LINKENTITY_GET_FIELD(value, field) ((value & field##Mask) >> field##Shift)

         enum class Kind {
            /// A method dispatch thunk.  The pointer is a FuncDecl* inside a protocol
            /// or a class.
            DispatchThunk,

               /// A method dispatch thunk for an initializing constructor.  The pointer
               /// is a ConstructorDecl* inside a class.
               DispatchThunkInitializer,

               /// A method dispatch thunk for an allocating constructor.  The pointer is a
               /// ConstructorDecl* inside a protocol or a class.
               DispatchThunkAllocator,

               /// A method descriptor.  The pointer is a FuncDecl* inside a protocol
               /// or a class.
               MethodDescriptor,

               /// A method descriptor for an initializing constructor.  The pointer
               /// is a ConstructorDecl* inside a class.
               MethodDescriptorInitializer,

               /// A method descriptor for an allocating constructor.  The pointer is a
               /// ConstructorDecl* inside a protocol or a class.
               MethodDescriptorAllocator,

               /// A method lookup function for a class.  The pointer is a ClassDecl*.
               MethodLookupFunction,

               /// A resilient enum tag index. The pointer is a EnumElementDecl*.
               EnumCase,

               /// A field offset.  The pointer is a VarDecl*.
               FieldOffset,

               /// An Objective-C class reference.  The pointer is a ClassDecl*.
               ObjCClass,

               /// An Objective-C class reference reference.  The pointer is a ClassDecl*.
               ObjCClassRef,

               /// An Objective-C metaclass reference.  The pointer is a ClassDecl*.
               ObjCMetaclass,

               /// A swift metaclass-stub reference.  The pointer is a ClassDecl*.
               SwiftMetaclassStub,

               /// A callback used by newer Objective-C runtimes to initialize class
               /// metadata for classes where getClassMetadataStrategy() is equal to
               /// ClassMetadataStrategy::Update or ::FixedOrUpdate.
               ObjCMetadataUpdateFunction,

               /// A stub that we emit to allow Clang-generated code to statically refer
               /// to Swift classes with resiliently-sized metadata, since the metadata
               /// is not statically-emitted. Used when getClassMetadataStrategy() is
               /// equal to ClassMetadataStrategy::Resilient.
               ObjCResilientClassStub,

               /// A class metadata base offset global variable.  This stores the offset
               /// of the immediate members of a class (generic parameters, field offsets,
               /// vtable offsets) in the class's metadata.  The immediate members begin
               /// immediately after the superclass members end.
               ///
               /// The pointer is a ClassDecl*.
               ClassMetadataBaseOffset,

               /// The property descriptor for a public property or subscript.
               /// The pointer is an AbstractStorageDecl*.
               PropertyDescriptor,

               /// The nominal type descriptor for a nominal type.
               /// The pointer is a NominalTypeDecl*.
               NominalTypeDescriptor,

               /// The descriptor for an opaque type.
               /// The pointer is an OpaqueTypeDecl*.
               OpaqueTypeDescriptor,

               /// The descriptor accessor for an opaque type used for dynamic functions.
               /// The pointer is an OpaqueTypeDecl*.
               OpaqueTypeDescriptorAccessor,

               /// The descriptor accessor implementation for an opaque type used for
               /// dynamic functions.
               /// The pointer is an OpaqueTypeDecl*.
               OpaqueTypeDescriptorAccessorImpl,

               /// The descriptor accessor key of dynamic replacements for an opaque type.
               /// The pointer is an OpaqueTypeDecl*.
               OpaqueTypeDescriptorAccessorKey,

               /// The descriptor accessor variable of dynamic replacements for an opaque
               /// type.
               /// The pointer is an OpaqueTypeDecl*.
               OpaqueTypeDescriptorAccessorVar,

               /// The metadata pattern for a generic nominal type.
               /// The pointer is a NominalTypeDecl*.
               TypeMetadataPattern,

               /// The instantiation cache for a generic nominal type.
               /// The pointer is a NominalTypeDecl*.
               TypeMetadataInstantiationCache,

               /// The instantiation function for a generic nominal type.
               /// The pointer is a NominalTypeDecl*.
               TypeMetadataInstantiationFunction,

               /// The in-place initialization cache for a generic nominal type.
               /// The pointer is a NominalTypeDecl*.
               TypeMetadataSingletonInitializationCache,

               /// The completion function for a generic or resilient nominal type.
               /// The pointer is a NominalTypeDecl*.
               TypeMetadataCompletionFunction,

               /// The module descriptor for a module.
               /// The pointer is a ModuleDecl*.
               ModuleDescriptor,

               /// The protocol descriptor for a protocol type.
               /// The pointer is a InterfaceDecl*.
               InterfaceDescriptor,

               /// The alias referring to the base of the requirements within the
               /// protocol descriptor, which is used to determine the offset of a
               /// particular requirement in the witness table.
               /// The pointer is a InterfaceDecl*.
               InterfaceRequirementsBaseDescriptor,

               /// An descriptor for an associated type within a protocol, which
               /// will alias the TargetInterfaceRequirement descripting this
               /// particular associated type.
               /// The pointer is an AssociatedTypeDecl*.
               AssociatedTypeDescriptor,

               /// An descriptor for an associated conformance within a protocol, which
               /// will alias the TargetInterfaceRequirement descripting this
               /// particular associated conformance.
               /// The pointer is a InterfaceDecl*; the index of the associated conformance
               /// is stored in the data.
               AssociatedConformanceDescriptor,

               /// A default accessor for an associated conformance of a protocol.
               /// The pointer is a InterfaceDecl*; the index of the associated conformance
               /// is stored in the data.
               DefaultAssociatedConformanceAccessor,

               /// An descriptor for an base conformance within a protocol, which
               /// will alias the TargetInterfaceRequirement descripting this
               /// particular base conformance.
               /// The pointer is a InterfaceDecl*; the index of the base conformance
               /// is stored in the data.
               BaseConformanceDescriptor,

               /// A global function pointer for dynamically replaceable functions.
               /// The pointer is a AbstractStorageDecl*.
               DynamicallyReplaceableFunctionVariableAST,

               /// The pointer is a AbstractStorageDecl*.
               DynamicallyReplaceableFunctionKeyAST,

               /// The original implementation of a dynamically replaceable function.
               /// The pointer is a AbstractStorageDecl*.
               DynamicallyReplaceableFunctionImpl,

               /// The pointer is a PILFunction*.
               DynamicallyReplaceableFunctionKey,

               /// A PIL function. The pointer is a PILFunction*.
               PILFunction,

               /// The descriptor for an extension.
               /// The pointer is an ExtensionDecl*.
               ExtensionDescriptor,

               /// The descriptor for a runtime-anonymous context.
               /// The pointer is the DeclContext* of a child of the context that should
               /// be considered private.
               AnonymousDescriptor,

               /// A PIL global variable. The pointer is a PILGlobalVariable*.
               PILGlobalVariable,

               // These next few are protocol-conformance kinds.

               /// A direct protocol witness table. The secondary pointer is a
               /// RootInterfaceConformance*.
               InterfaceWitnessTable,

               /// A protocol witness table pattern. The secondary pointer is a
               /// InterfaceConformance*.
               InterfaceWitnessTablePattern,

               /// The instantiation function for a generic protocol witness table.
               /// The secondary pointer is a InterfaceConformance*.
               GenericInterfaceWitnessTableInstantiationFunction,

               /// A function which returns the witness table for a protocol-constrained
               /// associated type of a protocol.  The secondary pointer is a
               /// InterfaceConformance*.  The index of the associated conformance
               /// requirement is stored in the data.
               AssociatedTypeWitnessTableAccessFunction,

               /// A reflection metadata descriptor for the associated type witnesses of a
               /// nominal type in a protocol conformance.
               ReflectionAssociatedTypeDescriptor,

               /// The protocol conformance descriptor for a conformance.
               /// The pointer is a RootInterfaceConformance*.
               InterfaceConformanceDescriptor,

               // These are both type kinds and protocol-conformance kinds.

               /// A lazy protocol witness accessor function. The pointer is a
               /// canonical TypeBase*, and the secondary pointer is a
               /// InterfaceConformance*.
               InterfaceWitnessTableLazyAccessFunction,

               /// A lazy protocol witness cache variable. The pointer is a
               /// canonical TypeBase*, and the secondary pointer is a
               /// InterfaceConformance*.
               InterfaceWitnessTableLazyCacheVariable,

               // Everything following this is a type kind.

               /// A value witness for a type.
               /// The pointer is a canonical TypeBase*.
               ValueWitness,

               /// The value witness table for a type.
               /// The pointer is a canonical TypeBase*.
               ValueWitnessTable,

               /// The metadata or metadata template for a type.
               /// The pointer is a canonical TypeBase*.
               TypeMetadata,

               /// An access function for type metadata.
               /// The pointer is a canonical TypeBase*.
               TypeMetadataAccessFunction,

               /// A lazy cache variable for type metadata.
               /// The pointer is a canonical TypeBase*.
               TypeMetadataLazyCacheVariable,

               /// A lazy cache variable for fetching type metadata from a mangled name.
               /// The pointer is a canonical TypeBase*.
               TypeMetadataDemanglingCacheVariable,

               /// A reflection metadata descriptor for a builtin or imported type.
               ReflectionBuiltinDescriptor,

               /// A reflection metadata descriptor for a struct, enum, class or protocol.
               ReflectionFieldDescriptor,

               /// A coroutine continuation prototype function.
               CoroutineContinuationPrototype,

               /// A global function pointer for dynamically replaceable functions.
               DynamicallyReplaceableFunctionVariable,
         };
         friend struct llvm::DenseMapInfo<LinkEntity>;

         Kind getKind() const {
            return Kind(LINKENTITY_GET_FIELD(Data, Kind));
         }

         static bool isDeclKind(Kind k) {
            return k <= Kind::DynamicallyReplaceableFunctionImpl;
         }
         static bool isTypeKind(Kind k) {
            return k >= Kind::InterfaceWitnessTableLazyAccessFunction;
         }

         static bool isRootInterfaceConformanceKind(Kind k) {
            return (k == Kind::InterfaceConformanceDescriptor ||
                    k == Kind::InterfaceWitnessTable);
         }

         static bool isInterfaceConformanceKind(Kind k) {
            return (k >= Kind::InterfaceWitnessTable &&
                    k <= Kind::InterfaceWitnessTableLazyCacheVariable);
         }

         void setForDecl(Kind kind, const ValueDecl *decl) {
            assert(isDeclKind(kind));
            Pointer = const_cast<void*>(static_cast<const void*>(decl));
            SecondaryPointer = nullptr;
            Data = LINKENTITY_SET_FIELD(Kind, unsigned(kind));
         }

         void setForInterfaceAndAssociatedConformance(Kind kind,
         const InterfaceDecl *proto,
         CanType associatedType,
         InterfaceDecl *associatedInterface){
            assert(isDeclKind(kind));
            Pointer = static_cast<ValueDecl *>(const_cast<InterfaceDecl *>(proto));
            SecondaryPointer = nullptr;
            Data = LINKENTITY_SET_FIELD(Kind, unsigned(kind)) |
            LINKENTITY_SET_FIELD(AssociatedConformanceIndex,
                                 getAssociatedConformanceIndex(
                                    proto,
                                    associatedType,
                                    associatedInterface));
         }

         void setForInterfaceConformance(Kind kind, const InterfaceConformance *c) {
            assert(isInterfaceConformanceKind(kind) && !isTypeKind(kind));
            Pointer = nullptr;
            SecondaryPointer = const_cast<void*>(static_cast<const void*>(c));
            Data = LINKENTITY_SET_FIELD(Kind, unsigned(kind));
         }

         void setForInterfaceConformanceAndType(Kind kind, const InterfaceConformance *c,
         CanType type) {
            assert(isInterfaceConformanceKind(kind) && isTypeKind(kind));
            Pointer = type.getPointer();
            SecondaryPointer = const_cast<void*>(static_cast<const void*>(c));
            Data = LINKENTITY_SET_FIELD(Kind, unsigned(kind));
         }

         void setForInterfaceConformanceAndAssociatedType(Kind kind,
         const InterfaceConformance *c,
         AssociatedTypeDecl *associate) {
            assert(isInterfaceConformanceKind(kind));
            Pointer = nullptr;
            SecondaryPointer = const_cast<void*>(static_cast<const void*>(c));
            Data = LINKENTITY_SET_FIELD(Kind, unsigned(kind)) |
            LINKENTITY_SET_FIELD(AssociatedTypeIndex,
                                 getAssociatedTypeIndex(c, associate));
         }

         void setForInterfaceConformanceAndAssociatedConformance(Kind kind,
         const InterfaceConformance *c,
         CanType associatedType,
         InterfaceDecl *associatedInterface) {
            assert(isInterfaceConformanceKind(kind));
            Pointer = associatedInterface;
            SecondaryPointer = const_cast<void*>(static_cast<const void*>(c));
            Data = LINKENTITY_SET_FIELD(Kind, unsigned(kind)) |
            LINKENTITY_SET_FIELD(AssociatedConformanceIndex,
                                 getAssociatedConformanceIndex(c, associatedType,
                                                               associatedInterface));
         }

         // We store associated types using their index in their parent protocol
         // in order to avoid bloating LinkEntity out to three key pointers.
         static unsigned getAssociatedTypeIndex(const InterfaceConformance *conformance,
         AssociatedTypeDecl *associate) {
            auto *proto = associate->getInterface();
            assert(conformance->getInterface() == proto);
            unsigned result = 0;
            for (auto requirement : proto->getAssociatedTypeMembers()) {
               if (requirement == associate) return result;
               result++;
            }
            llvm_unreachable("didn't find associated type in protocol?");
         }

         static AssociatedTypeDecl *
         getAssociatedTypeByIndex(const InterfaceConformance *conformance,
         unsigned index) {
            for (auto associate : conformance->getInterface()->getAssociatedTypeMembers()) {
               if (index == 0) return associate;
               index--;
            }
            llvm_unreachable("didn't find associated type in protocol?");
         }

         // We store associated conformances using their index in the requirement
         // list of the requirement signature of the protocol.
         static unsigned getAssociatedConformanceIndex(const InterfaceDecl *proto,
         CanType associatedType,
         InterfaceDecl *requirement) {
            unsigned index = 0;
            for (const auto &reqt : proto->getRequirementSignature()) {
               if (reqt.getKind() == RequirementKind::Conformance &&
                   reqt.getFirstType()->getCanonicalType() == associatedType &&
                   reqt.getSecondType()->castTo<InterfaceType>()->getDecl() ==
                   requirement) {
                  return index;
               }
               ++index;
            }
            llvm_unreachable("requirement not found in protocol");
         }

         // We store associated conformances using their index in the requirement
         // list of the requirement signature of the conformance's protocol.
         static unsigned getAssociatedConformanceIndex(
         const InterfaceConformance *conformance,
         CanType associatedType,
         InterfaceDecl *requirement) {
            return getAssociatedConformanceIndex(conformance->getInterface(),
                                                 associatedType, requirement);
         }

         static std::pair<CanType, InterfaceDecl*>
         getAssociatedConformanceByIndex(const InterfaceDecl *proto,
         unsigned index) {
            auto &reqt = proto->getRequirementSignature()[index];
            assert(reqt.getKind() == RequirementKind::Conformance);
            return { reqt.getFirstType()->getCanonicalType(),
                     reqt.getSecondType()->castTo<InterfaceType>()->getDecl() };
         }

         static std::pair<CanType, InterfaceDecl*>
         getAssociatedConformanceByIndex(const InterfaceConformance *conformance,
         unsigned index) {
            return getAssociatedConformanceByIndex(conformance->getInterface(), index);
         }

         void setForType(Kind kind, CanType type) {
            assert(isTypeKind(kind));
            Pointer = type.getPointer();
            SecondaryPointer = nullptr;
            Data = LINKENTITY_SET_FIELD(Kind, unsigned(kind));
         }

         LinkEntity() = default;

         static bool isValidResilientMethodRef(PILDeclRef declRef) {
            if (declRef.isForeign ||
                declRef.isDirectReference ||
                declRef.isCurried)
               return false;

            auto *decl = declRef.getDecl();
            return (isa<ClassDecl>(decl->getDeclContext()) ||
                    isa<InterfaceDecl>(decl->getDeclContext()));
         }

         public:
         static LinkEntity forDispatchThunk(PILDeclRef declRef) {
            assert(isValidResilientMethodRef(declRef));

            LinkEntity::Kind kind;
            switch (declRef.kind) {
               case PILDeclRef::Kind::Func:
                  kind = Kind::DispatchThunk;
                  break;
               case PILDeclRef::Kind::Initializer:
                  kind = Kind::DispatchThunkInitializer;
                  break;
               case PILDeclRef::Kind::Allocator:
                  kind = Kind::DispatchThunkAllocator;
                  break;
               default:
                  llvm_unreachable("Bad PILDeclRef for dispatch thunk");
            }

            LinkEntity entity;
            entity.setForDecl(kind, declRef.getDecl());
            return entity;
         }

         static LinkEntity forMethodDescriptor(PILDeclRef declRef) {
            assert(isValidResilientMethodRef(declRef));

            LinkEntity::Kind kind;
            switch (declRef.kind) {
               case PILDeclRef::Kind::Func:
                  kind = Kind::MethodDescriptor;
                  break;
               case PILDeclRef::Kind::Initializer:
                  kind = Kind::MethodDescriptorInitializer;
                  break;
               case PILDeclRef::Kind::Allocator:
                  kind = Kind::MethodDescriptorAllocator;
                  break;
               default:
                  llvm_unreachable("Bad PILDeclRef for method descriptor");
            }

            LinkEntity entity;
            entity.setForDecl(kind, declRef.getDecl());
            return entity;
         }

         static LinkEntity forMethodLookupFunction(ClassDecl *classDecl) {
            LinkEntity entity;
            entity.setForDecl(Kind::MethodLookupFunction, classDecl);
            return entity;
         }

         static LinkEntity forFieldOffset(VarDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::FieldOffset, decl);
            return entity;
         }

         static LinkEntity forEnumCase(EnumElementDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::EnumCase, decl);
            return entity;
         }

         static LinkEntity forObjCClassRef(ClassDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::ObjCClassRef, decl);
            return entity;
         }

         static LinkEntity forObjCClass(ClassDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::ObjCClass, decl);
            return entity;
         }

         static LinkEntity forObjCMetaclass(ClassDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::ObjCMetaclass, decl);
            return entity;
         }

         static LinkEntity forSwiftMetaclassStub(ClassDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::SwiftMetaclassStub, decl);
            return entity;
         }

         static LinkEntity forObjCMetadataUpdateFunction(ClassDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::ObjCMetadataUpdateFunction, decl);
            return entity;
         }

         static LinkEntity forObjCResilientClassStub(ClassDecl *decl,
         TypeMetadataAddress addr) {
            LinkEntity entity;
            entity.setForDecl(Kind::ObjCResilientClassStub, decl);
            entity.Data |= LINKENTITY_SET_FIELD(MetadataAddress, unsigned(addr));
            return entity;
         }

         static LinkEntity forTypeMetadata(CanType concreteType,
         TypeMetadataAddress addr) {
            LinkEntity entity;
            entity.setForType(Kind::TypeMetadata, concreteType);
            entity.Data |= LINKENTITY_SET_FIELD(MetadataAddress, unsigned(addr));
            return entity;
         }

         static LinkEntity forTypeMetadataPattern(NominalTypeDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::TypeMetadataPattern, decl);
            return entity;
         }

         static LinkEntity forTypeMetadataAccessFunction(CanType type) {
            LinkEntity entity;
            entity.setForType(Kind::TypeMetadataAccessFunction, type);
            return entity;
         }

         static LinkEntity forTypeMetadataInstantiationCache(NominalTypeDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::TypeMetadataInstantiationCache, decl);
            return entity;
         }

         static LinkEntity forTypeMetadataInstantiationFunction(NominalTypeDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::TypeMetadataInstantiationFunction, decl);
            return entity;
         }

         static LinkEntity forTypeMetadataSingletonInitializationCache(
         NominalTypeDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::TypeMetadataSingletonInitializationCache, decl);
            return entity;
         }

         static LinkEntity forTypeMetadataCompletionFunction(NominalTypeDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::TypeMetadataCompletionFunction, decl);
            return entity;
         }

         static LinkEntity forTypeMetadataLazyCacheVariable(CanType type) {
            LinkEntity entity;
            entity.setForType(Kind::TypeMetadataLazyCacheVariable, type);
            return entity;
         }

         static LinkEntity forTypeMetadataDemanglingCacheVariable(CanType type) {
            LinkEntity entity;
            entity.setForType(Kind::TypeMetadataDemanglingCacheVariable, type);
            return entity;
         }

         static LinkEntity forClassMetadataBaseOffset(ClassDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::ClassMetadataBaseOffset, decl);
            return entity;
         }

         static LinkEntity forNominalTypeDescriptor(NominalTypeDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::NominalTypeDescriptor, decl);
            return entity;
         }

         static LinkEntity forOpaqueTypeDescriptor(OpaqueTypeDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::OpaqueTypeDescriptor, decl);
            return entity;
         }

         static LinkEntity forOpaqueTypeDescriptorAccessor(OpaqueTypeDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::OpaqueTypeDescriptorAccessor, decl);
            return entity;
         }

         static LinkEntity forOpaqueTypeDescriptorAccessorImpl(OpaqueTypeDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::OpaqueTypeDescriptorAccessorImpl, decl);
            return entity;
         }

         static LinkEntity forOpaqueTypeDescriptorAccessorKey(OpaqueTypeDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::OpaqueTypeDescriptorAccessorKey, decl);
            return entity;
         }

         static LinkEntity forOpaqueTypeDescriptorAccessorVar(OpaqueTypeDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::OpaqueTypeDescriptorAccessorVar, decl);
            return entity;
         }

         static LinkEntity forPropertyDescriptor(AbstractStorageDecl *decl) {
            assert(decl->exportsPropertyDescriptor());
            LinkEntity entity;
            entity.setForDecl(Kind::PropertyDescriptor, decl);
            return entity;
         }

         static LinkEntity forModuleDescriptor(ModuleDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::ModuleDescriptor, decl);
            return entity;
         }

         static LinkEntity forExtensionDescriptor(ExtensionDecl *decl) {
            LinkEntity entity;
            entity.Pointer = const_cast<void*>(static_cast<const void*>(decl));
            entity.SecondaryPointer = nullptr;
            entity.Data =
               LINKENTITY_SET_FIELD(Kind, unsigned(Kind::ExtensionDescriptor));
            return entity;
         }

         static LinkEntity forAnonymousDescriptor(
         PointerUnion<DeclContext *, VarDecl *> dc) {
            LinkEntity entity;
            entity.Pointer = dc.getOpaqueValue();
            entity.SecondaryPointer = nullptr;
            entity.Data =
               LINKENTITY_SET_FIELD(Kind, unsigned(Kind::AnonymousDescriptor));
            return entity;
         }

         static LinkEntity forInterfaceDescriptor(InterfaceDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::InterfaceDescriptor, decl);
            return entity;
         }

         static LinkEntity forInterfaceRequirementsBaseDescriptor(InterfaceDecl *decl) {
            LinkEntity entity;
            entity.setForDecl(Kind::InterfaceRequirementsBaseDescriptor, decl);
            return entity;
         }

         static LinkEntity forValueWitness(CanType concreteType, ValueWitness witness) {
            LinkEntity entity;
            entity.Pointer = concreteType.getPointer();
            entity.Data = LINKENTITY_SET_FIELD(Kind, unsigned(Kind::ValueWitness))
            | LINKENTITY_SET_FIELD(ValueWitness, unsigned(witness));
            return entity;
         }

         static LinkEntity forValueWitnessTable(CanType type) {
            LinkEntity entity;
            entity.setForType(Kind::ValueWitnessTable, type);
            return entity;
         }

         static LinkEntity
         forPILFunction(PILFunction *F, bool IsDynamicallyReplaceableImplementation) {
            LinkEntity entity;
            entity.Pointer = F;
            entity.SecondaryPointer = nullptr;
            entity.Data =
               LINKENTITY_SET_FIELD(Kind, unsigned(Kind::PILFunction)) |
            LINKENTITY_SET_FIELD(IsDynamicallyReplaceableImpl,
                                 (unsigned)IsDynamicallyReplaceableImplementation);
            return entity;
         }

         static LinkEntity forPILGlobalVariable(PILGlobalVariable *G) {
            LinkEntity entity;
            entity.Pointer = G;
            entity.SecondaryPointer = nullptr;
            entity.Data = LINKENTITY_SET_FIELD(Kind, unsigned(Kind::PILGlobalVariable));
            return entity;
         }

         static LinkEntity forInterfaceWitnessTable(const RootInterfaceConformance *C) {
            LinkEntity entity;
            entity.setForInterfaceConformance(Kind::InterfaceWitnessTable, C);
            return entity;
         }

         static LinkEntity
         forInterfaceWitnessTablePattern(const InterfaceConformance *C) {
            LinkEntity entity;
            entity.setForInterfaceConformance(Kind::InterfaceWitnessTablePattern, C);
            return entity;
         }

         static LinkEntity
         forGenericInterfaceWitnessTableInstantiationFunction(
         const InterfaceConformance *C) {
            LinkEntity entity;
            entity.setForInterfaceConformance(
               Kind::GenericInterfaceWitnessTableInstantiationFunction, C);
            return entity;
         }

         static LinkEntity
         forInterfaceWitnessTableLazyAccessFunction(const InterfaceConformance *C,
         CanType type) {
            LinkEntity entity;
            entity.setForInterfaceConformanceAndType(
               Kind::InterfaceWitnessTableLazyAccessFunction, C, type);
            return entity;
         }

         static LinkEntity
         forInterfaceWitnessTableLazyCacheVariable(const InterfaceConformance *C,
         CanType type) {
            LinkEntity entity;
            entity.setForInterfaceConformanceAndType(
               Kind::InterfaceWitnessTableLazyCacheVariable, C, type);
            return entity;
         }

         static LinkEntity
         forAssociatedTypeDescriptor(AssociatedTypeDecl *assocType) {
            LinkEntity entity;
            entity.setForDecl(Kind::AssociatedTypeDescriptor, assocType);
            return entity;
         }

         static LinkEntity
         forAssociatedConformanceDescriptor(AssociatedConformance conformance) {
            LinkEntity entity;
            entity.setForInterfaceAndAssociatedConformance(
               Kind::AssociatedConformanceDescriptor,
               conformance.getSourceInterface(),
               conformance.getAssociation(),
               conformance.getAssociatedRequirement());
            return entity;
         }

         static LinkEntity
         forBaseConformanceDescriptor(BaseConformance conformance) {
            LinkEntity entity;
            entity.setForInterfaceAndAssociatedConformance(
               Kind::BaseConformanceDescriptor,
               conformance.getSourceInterface(),
               conformance.getSourceInterface()->getSelfInterfaceType()
                  ->getCanonicalType(),
               conformance.getBaseRequirement());
            return entity;
         }

         static LinkEntity
         forAssociatedTypeWitnessTableAccessFunction(const InterfaceConformance *C,
         const AssociatedConformance &association) {
            LinkEntity entity;
            entity.setForInterfaceConformanceAndAssociatedConformance(
               Kind::AssociatedTypeWitnessTableAccessFunction, C,
               association.getAssociation(),
               association.getAssociatedRequirement());
            return entity;
         }

         static LinkEntity
         forDefaultAssociatedConformanceAccessor(AssociatedConformance conformance) {
            LinkEntity entity;
            entity.setForInterfaceAndAssociatedConformance(
               Kind::DefaultAssociatedConformanceAccessor,
               conformance.getSourceInterface(),
               conformance.getAssociation(),
               conformance.getAssociatedRequirement());
            return entity;
         }

         static LinkEntity forReflectionBuiltinDescriptor(CanType type) {
            LinkEntity entity;
            entity.setForType(Kind::ReflectionBuiltinDescriptor, type);
            return entity;
         }

         static LinkEntity forReflectionFieldDescriptor(CanType type) {
            LinkEntity entity;
            entity.setForType(Kind::ReflectionFieldDescriptor, type);
            return entity;
         }

         static LinkEntity
         forReflectionAssociatedTypeDescriptor(const InterfaceConformance *C) {
            LinkEntity entity;
            entity.setForInterfaceConformance(
               Kind::ReflectionAssociatedTypeDescriptor, C);
            return entity;
         }

         static LinkEntity
         forInterfaceConformanceDescriptor(const RootInterfaceConformance *C) {
            LinkEntity entity;
            entity.setForInterfaceConformance(Kind::InterfaceConformanceDescriptor, C);
            return entity;
         }

         static LinkEntity forCoroutineContinuationPrototype(CanPILFunctionType type) {
            LinkEntity entity;
            entity.setForType(Kind::CoroutineContinuationPrototype, type);
            return entity;
         }

         static LinkEntity forDynamicallyReplaceableFunctionVariable(PILFunction *F) {
            LinkEntity entity;
            entity.Pointer = F;
            entity.SecondaryPointer = nullptr;
            entity.Data = LINKENTITY_SET_FIELD(
               Kind, unsigned(Kind::DynamicallyReplaceableFunctionVariable));
            return entity;
         }

         static LinkEntity
         forDynamicallyReplaceableFunctionVariable(AbstractFunctionDecl *decl,
         bool isAllocator) {
            LinkEntity entity;
            entity.setForDecl(Kind::DynamicallyReplaceableFunctionVariableAST, decl);
            entity.SecondaryPointer = isAllocator ? decl : nullptr;
            return entity;
         }

         static LinkEntity forDynamicallyReplaceableFunctionKey(PILFunction *F) {
            LinkEntity entity;
            entity.Pointer = F;
            entity.SecondaryPointer = nullptr;
            entity.Data = LINKENTITY_SET_FIELD(
               Kind, unsigned(Kind::DynamicallyReplaceableFunctionKey));
            return entity;
         }

         static LinkEntity
         forDynamicallyReplaceableFunctionKey(AbstractFunctionDecl *decl,
         bool isAllocator) {
            LinkEntity entity;
            entity.setForDecl(Kind::DynamicallyReplaceableFunctionKeyAST, decl);
            entity.SecondaryPointer = isAllocator ? decl : nullptr;
            return entity;
         }

         static LinkEntity
         forDynamicallyReplaceableFunctionImpl(AbstractFunctionDecl *decl,
         bool isAllocator) {
            LinkEntity entity;
            entity.setForDecl(Kind::DynamicallyReplaceableFunctionImpl, decl);
            entity.SecondaryPointer = isAllocator ? decl : nullptr;
            return entity;
         }

         void mangle(llvm::raw_ostream &out) const;
         void mangle(SmallVectorImpl<char> &buffer) const;
         std::string mangleAsString() const;
         PILLinkage getLinkage(ForDefinition_t isDefinition) const;

         /// Returns true if this function or global variable is potentially defined
         /// in a different module.
         ///
         bool isAvailableExternally(IRGenModule &IGM) const;

         const ValueDecl *getDecl() const {
            assert(isDeclKind(getKind()));
            return reinterpret_cast<ValueDecl*>(Pointer);
         }

         const ExtensionDecl *getExtension() const {
            assert(getKind() == Kind::ExtensionDescriptor);
            return reinterpret_cast<ExtensionDecl*>(Pointer);
         }

         const PointerUnion<DeclContext *, VarDecl *> getAnonymousDeclContext() const {
            assert(getKind() == Kind::AnonymousDescriptor);
            return PointerUnion<DeclContext *, VarDecl *>
            ::getFromOpaqueValue(reinterpret_cast<void*>(Pointer));
         }

         PILFunction *getPILFunction() const {
            assert(getKind() == Kind::PILFunction ||
                   getKind() == Kind::DynamicallyReplaceableFunctionVariable ||
                   getKind() == Kind::DynamicallyReplaceableFunctionKey);
            return reinterpret_cast<PILFunction*>(Pointer);
         }

         PILGlobalVariable *getPILGlobalVariable() const {
            assert(getKind() == Kind::PILGlobalVariable);
            return reinterpret_cast<PILGlobalVariable*>(Pointer);
         }

         const RootInterfaceConformance *getRootInterfaceConformance() const {
            assert(isRootInterfaceConformanceKind(getKind()));
            return cast<RootInterfaceConformance>(getInterfaceConformance());
         }

         const InterfaceConformance *getInterfaceConformance() const {
            assert(isInterfaceConformanceKind(getKind()));
            return reinterpret_cast<InterfaceConformance*>(SecondaryPointer);
         }

         AssociatedTypeDecl *getAssociatedType() const {
            assert(getKind() == Kind::AssociatedTypeDescriptor);
            return reinterpret_cast<AssociatedTypeDecl *>(Pointer);
         }

         std::pair<CanType, InterfaceDecl *> getAssociatedConformance() const {
            if (getKind() == Kind::AssociatedTypeWitnessTableAccessFunction) {
               return getAssociatedConformanceByIndex(getInterfaceConformance(),
                                                      LINKENTITY_GET_FIELD(Data, AssociatedConformanceIndex));
            }

            assert(getKind() == Kind::AssociatedConformanceDescriptor ||
                   getKind() == Kind::DefaultAssociatedConformanceAccessor ||
                   getKind() == Kind::BaseConformanceDescriptor);
            return getAssociatedConformanceByIndex(
               cast<InterfaceDecl>(getDecl()),
               LINKENTITY_GET_FIELD(Data, AssociatedConformanceIndex));
         }

         InterfaceDecl *getAssociatedInterface() const {
            assert(getKind() == Kind::AssociatedTypeWitnessTableAccessFunction);
            return reinterpret_cast<InterfaceDecl*>(Pointer);
         }
         bool isDynamicallyReplaceable() const {
            assert(getKind() == Kind::PILFunction);
            return LINKENTITY_GET_FIELD(Data, IsDynamicallyReplaceableImpl);
         }
         bool isAllocator() const {
            assert(getKind() == Kind::DynamicallyReplaceableFunctionImpl ||
                   getKind() == Kind::DynamicallyReplaceableFunctionKeyAST ||
                   getKind() == Kind::DynamicallyReplaceableFunctionVariableAST);
            return SecondaryPointer != nullptr;
         }
         bool isValueWitness() const { return getKind() == Kind::ValueWitness; }
         CanType getType() const {
            assert(isTypeKind(getKind()));
            return CanType(reinterpret_cast<TypeBase*>(Pointer));
         }
         ValueWitness getValueWitness() const {
            assert(getKind() == Kind::ValueWitness);
            return ValueWitness(LINKENTITY_GET_FIELD(Data, ValueWitness));
         }
         TypeMetadataAddress getMetadataAddress() const {
            assert(getKind() == Kind::TypeMetadata ||
                   getKind() == Kind::ObjCResilientClassStub);
            return (TypeMetadataAddress)LINKENTITY_GET_FIELD(Data, MetadataAddress);
         }
         bool isObjCClassRef() const {
            return getKind() == Kind::ObjCClassRef;
         }
         bool isPILFunction() const {
            return getKind() == Kind::PILFunction;
         }
         bool isNominalTypeDescriptor() const {
            return getKind() == Kind::NominalTypeDescriptor;
         }

         /// Determine whether this entity will be weak-imported.
         bool isWeakImported(ModuleDecl *module) const;

         /// Return the source file whose codegen should trigger emission of this
         /// link entity, if one can be identified.
         const SourceFile *getSourceFileForEmission() const;

         /// Get the preferred alignment for the definition of this entity.
         Alignment getAlignment(IRGenModule &IGM) const;

         /// Get the default LLVM type to use for forward declarations of this
         /// entity.
         llvm::Type *getDefaultDeclarationType(IRGenModule &IGM) const;
#undef LINKENTITY_GET_FIELD
#undef LINKENTITY_SET_FIELD
      };

      struct IRLinkage {
         llvm::GlobalValue::LinkageTypes Linkage;
         llvm::GlobalValue::VisibilityTypes Visibility;
         llvm::GlobalValue::DLLStorageClassTypes DLLStorage;

         static const IRLinkage InternalLinkOnceODR;
         static const IRLinkage InternalWeakODR;
         static const IRLinkage Internal;

         static const IRLinkage ExternalImport;
         static const IRLinkage ExternalWeakImport;
         static const IRLinkage ExternalExport;
      };

      class ApplyIRLinkage {
         IRLinkage IRL;
         public:
         ApplyIRLinkage(IRLinkage IRL) : IRL(IRL) {}
         void to(llvm::GlobalValue *GV) const {
            llvm::Module *M = GV->getParent();
            const llvm::Triple Triple(M->getTargetTriple());

            GV->setLinkage(IRL.Linkage);
            GV->setVisibility(IRL.Visibility);
            if (Triple.isOSBinFormatCOFF() && !Triple.isOSCygMing())
               GV->setDLLStorageClass(IRL.DLLStorage);

            // TODO: BFD and gold do not handle COMDATs properly
            if (Triple.isOSBinFormatELF())
               return;

            if (IRL.Linkage == llvm::GlobalValue::LinkOnceODRLinkage ||
                IRL.Linkage == llvm::GlobalValue::WeakODRLinkage)
               if (Triple.supportsCOMDAT())
                  if (llvm::GlobalObject *GO = dyn_cast<llvm::GlobalObject>(GV))
                     GO->setComdat(M->getOrInsertComdat(GV->getName()));
         }
      };

/// Encapsulated information about the linkage of an entity.
      class LinkInfo {
         LinkInfo() = default;

         llvm::SmallString<32> Name;
         IRLinkage IRL;
         ForDefinition_t ForDefinition;

         public:
         /// Compute linkage information for the given
         static LinkInfo get(IRGenModule &IGM, const LinkEntity &entity,
         ForDefinition_t forDefinition);

         static LinkInfo get(const UniversalLinkageInfo &linkInfo,
         ModuleDecl *swiftModule,
         const LinkEntity &entity,
         ForDefinition_t forDefinition);

         static LinkInfo get(const UniversalLinkageInfo &linkInfo, StringRef name,
         PILLinkage linkage, ForDefinition_t isDefinition,
         bool isWeakImported);

         StringRef getName() const {
            return Name.str();
         }
         llvm::GlobalValue::LinkageTypes getLinkage() const {
            return IRL.Linkage;
         }
         llvm::GlobalValue::VisibilityTypes getVisibility() const {
            return IRL.Visibility;
         }
         llvm::GlobalValue::DLLStorageClassTypes getDLLStorage() const {
            return IRL.DLLStorage;
         }

         bool isForDefinition() const { return ForDefinition; }
         bool isUsed() const { return ForDefinition && isUsed(IRL); }

         static bool isUsed(IRLinkage IRL);
      };

      StringRef encodeForceLoadSymbolName(llvm::SmallVectorImpl<char> &buf,
                                          StringRef name);
   }
} // polar

/// Allow LinkEntity to be used as a key for a DenseMap.
namespace llvm {
   template <> struct DenseMapInfo<polar::irgen::LinkEntity> {
      using LinkEntity = polar::irgen::LinkEntity;
      static LinkEntity getEmptyKey() {
         LinkEntity entity;
         entity.Pointer = nullptr;
         entity.SecondaryPointer = nullptr;
         entity.Data = 0;
         return entity;
      }
      static LinkEntity getTombstoneKey() {
         LinkEntity entity;
         entity.Pointer = nullptr;
         entity.SecondaryPointer = nullptr;
         entity.Data = 1;
         return entity;
      }
      static unsigned getHashValue(const LinkEntity &entity) {
         return DenseMapInfo<void *>::getHashValue(entity.Pointer) ^
                DenseMapInfo<void *>::getHashValue(entity.SecondaryPointer) ^
                entity.Data;
      }
      static bool isEqual(const LinkEntity &LHS, const LinkEntity &RHS) {
         return LHS.Pointer == RHS.Pointer &&
                LHS.SecondaryPointer == RHS.SecondaryPointer && LHS.Data == RHS.Data;
      }
   };
}

#endif // POLARPHP_IRGEN_LINKING_H
