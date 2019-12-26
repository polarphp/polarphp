//===--- Linking.cpp - Name mangling for IRGen entities -------------------===//
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
//  This file implements name mangling for IRGen entities with linkage.
//
//===----------------------------------------------------------------------===//

#include "polarphp/irgen/Linking.h"
#include "polarphp/irgen/internal/IRGenMangler.h"
#include "polarphp/irgen/internal/IRGenModule.h"
#include "polarphp/ast/AstMangler.h"
#include "polarphp/ast/IRGenOptions.h"
#include "polarphp/clangimporter/ClangModule.h"
#include "polarphp/pil/lang/PILGlobalVariable.h"
#include "polarphp/pil/lang/FormalLinkage.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/raw_ostream.h"

#include "polarphp/irgen/internal/MetadataRequest.h"

using namespace polar;
using namespace irgen;
using namespace mangle;

const IRLinkage IRLinkage::InternalLinkOnceODR = {
   llvm::GlobalValue::LinkOnceODRLinkage,
   llvm::GlobalValue::HiddenVisibility,
   llvm::GlobalValue::DefaultStorageClass,
};

const IRLinkage IRLinkage::InternalWeakODR = {
   llvm::GlobalValue::WeakODRLinkage,
   llvm::GlobalValue::HiddenVisibility,
   llvm::GlobalValue::DefaultStorageClass,
};

const IRLinkage IRLinkage::Internal = {
   llvm::GlobalValue::InternalLinkage,
   llvm::GlobalValue::DefaultVisibility,
   llvm::GlobalValue::DefaultStorageClass,
};

const IRLinkage IRLinkage::ExternalImport = {
   llvm::GlobalValue::ExternalLinkage,
   llvm::GlobalValue::DefaultVisibility,
   llvm::GlobalValue::DLLImportStorageClass,
};

const IRLinkage IRLinkage::ExternalWeakImport = {
   llvm::GlobalValue::ExternalWeakLinkage,
   llvm::GlobalValue::DefaultVisibility,
   llvm::GlobalValue::DLLImportStorageClass,
};

const IRLinkage IRLinkage::ExternalExport = {
   llvm::GlobalValue::ExternalLinkage,
   llvm::GlobalValue::DefaultVisibility,
   llvm::GlobalValue::DLLExportStorageClass,
};

bool polar::irgen::useDllStorage(const llvm::Triple &triple) {
   return triple.isOSBinFormatCOFF() && !triple.isOSCygMing();
}

UniversalLinkageInfo::UniversalLinkageInfo(IRGenModule &IGM)
   : UniversalLinkageInfo(IGM.Triple, IGM.IRGen.hasMultipleIGMs(),
                          IGM.IRGen.Opts.ForcePublicLinkage,
                          IGM.getPILModule().isWholeModule()) {}

UniversalLinkageInfo::UniversalLinkageInfo(const llvm::Triple &triple,
                                           bool hasMultipleIGMs,
                                           bool forcePublicDecls,
                                           bool isWholeModule)
   : IsELFObject(triple.isOSBinFormatELF()),
     UseDLLStorage(useDllStorage(triple)), HasMultipleIGMs(hasMultipleIGMs),
     ForcePublicDecls(forcePublicDecls), IsWholeModule(isWholeModule) {}

/// Mangle this entity into the given buffer.
void LinkEntity::mangle(SmallVectorImpl<char> &buffer) const {
   llvm::raw_svector_ostream stream(buffer);
   mangle(stream);
}

/// Mangle this entity into the given stream.
void LinkEntity::mangle(raw_ostream &buffer) const {
   std::string Result = mangleAsString();
   buffer.write(Result.data(), Result.size());
}

/// Mangle this entity as a std::string.
std::string LinkEntity::mangleAsString() const {
   IRGenMangler mangler;
   switch (getKind()) {
      case Kind::DispatchThunk: {
         auto *func = cast<FuncDecl>(getDecl());
         return mangler.mangleDispatchThunk(func);
      }

      case Kind::DispatchThunkInitializer: {
         auto *ctor = cast<ConstructorDecl>(getDecl());
         return mangler.mangleConstructorDispatchThunk(ctor,
            /*isAllocating=*/false);
      }

      case Kind::DispatchThunkAllocator: {
         auto *ctor = cast<ConstructorDecl>(getDecl());
         return mangler.mangleConstructorDispatchThunk(ctor,
            /*isAllocating=*/true);
      }

      case Kind::MethodDescriptor: {
         auto *func = cast<FuncDecl>(getDecl());
         return mangler.mangleMethodDescriptor(func);
      }

      case Kind::MethodDescriptorInitializer: {
         auto *ctor = cast<ConstructorDecl>(getDecl());
         return mangler.mangleConstructorMethodDescriptor(ctor,
            /*isAllocating=*/false);
      }

      case Kind::MethodDescriptorAllocator: {
         auto *ctor = cast<ConstructorDecl>(getDecl());
         return mangler.mangleConstructorMethodDescriptor(ctor,
            /*isAllocating=*/true);
      }

      case Kind::MethodLookupFunction: {
         auto *classDecl = cast<ClassDecl>(getDecl());
         return mangler.mangleMethodLookupFunction(classDecl);
      }

      case Kind::ValueWitness:
         return mangler.mangleValueWitness(getType(), getValueWitness());

      case Kind::ValueWitnessTable:
         return mangler.mangleValueWitnessTable(getType());

      case Kind::TypeMetadataAccessFunction:
         return mangler.mangleTypeMetadataAccessFunction(getType());

      case Kind::TypeMetadataLazyCacheVariable:
         return mangler.mangleTypeMetadataLazyCacheVariable(getType());

      case Kind::TypeMetadataDemanglingCacheVariable:
         return mangler.mangleTypeMetadataDemanglingCacheVariable(getType());

      case Kind::TypeMetadataInstantiationCache:
         return mangler.mangleTypeMetadataInstantiationCache(
            cast<NominalTypeDecl>(getDecl()));

      case Kind::TypeMetadataInstantiationFunction:
         return mangler.mangleTypeMetadataInstantiationFunction(
            cast<NominalTypeDecl>(getDecl()));

      case Kind::TypeMetadataSingletonInitializationCache:
         return mangler.mangleTypeMetadataSingletonInitializationCache(
            cast<NominalTypeDecl>(getDecl()));

      case Kind::TypeMetadataCompletionFunction:
         return mangler.mangleTypeMetadataCompletionFunction(
            cast<NominalTypeDecl>(getDecl()));

      case Kind::TypeMetadata:
         switch (getMetadataAddress()) {
            case TypeMetadataAddress::FullMetadata:
               return mangler.mangleTypeFullMetadataFull(getType());
            case TypeMetadataAddress::AddressPoint:
               return mangler.mangleTypeMetadataFull(getType());
         }
         llvm_unreachable("invalid metadata address");

      case Kind::TypeMetadataPattern:
         return mangler.mangleTypeMetadataPattern(
            cast<NominalTypeDecl>(getDecl()));

      case Kind::SwiftMetaclassStub:
         return mangler.mangleClassMetaClass(cast<ClassDecl>(getDecl()));

      case Kind::ObjCMetadataUpdateFunction:
         return mangler.mangleObjCMetadataUpdateFunction(cast<ClassDecl>(getDecl()));

      case Kind::ObjCResilientClassStub:
         switch (getMetadataAddress()) {
            case TypeMetadataAddress::FullMetadata:
               return mangler.mangleFullObjCResilientClassStub(cast<ClassDecl>(getDecl()));
            case TypeMetadataAddress::AddressPoint:
               return mangler.mangleObjCResilientClassStub(cast<ClassDecl>(getDecl()));
         }
         llvm_unreachable("invalid metadata address");

      case Kind::ClassMetadataBaseOffset:               // class metadata base offset
         return mangler.mangleClassMetadataBaseOffset(cast<ClassDecl>(getDecl()));

      case Kind::NominalTypeDescriptor:
         return mangler.mangleNominalTypeDescriptor(
            cast<NominalTypeDecl>(getDecl()));

      case Kind::OpaqueTypeDescriptor:
         return mangler.mangleOpaqueTypeDescriptor(cast<OpaqueTypeDecl>(getDecl()));

      case Kind::OpaqueTypeDescriptorAccessor:
         return mangler.mangleOpaqueTypeDescriptorAccessor(
            cast<OpaqueTypeDecl>(getDecl()));

      case Kind::OpaqueTypeDescriptorAccessorImpl:
         return mangler.mangleOpaqueTypeDescriptorAccessorImpl(
            cast<OpaqueTypeDecl>(getDecl()));

      case Kind::OpaqueTypeDescriptorAccessorKey:
         return mangler.mangleOpaqueTypeDescriptorAccessorKey(
            cast<OpaqueTypeDecl>(getDecl()));

      case Kind::OpaqueTypeDescriptorAccessorVar:
         return mangler.mangleOpaqueTypeDescriptorAccessorVar(
            cast<OpaqueTypeDecl>(getDecl()));

      case Kind::PropertyDescriptor:
         return mangler.manglePropertyDescriptor(
            cast<AbstractStorageDecl>(getDecl()));

      case Kind::ModuleDescriptor:
         return mangler.mangleModuleDescriptor(cast<ModuleDecl>(getDecl()));

      case Kind::ExtensionDescriptor:
         return mangler.mangleExtensionDescriptor(getExtension());

      case Kind::AnonymousDescriptor:
         return mangler.mangleAnonymousDescriptor(getAnonymousDeclContext());

      case Kind::InterfaceDescriptor:
         return mangler.mangleInterfaceDescriptor(cast<InterfaceDecl>(getDecl()));

      case Kind::InterfaceRequirementsBaseDescriptor:
         return mangler.mangleInterfaceRequirementsBaseDescriptor(
            cast<InterfaceDecl>(getDecl()));

      case Kind::AssociatedTypeDescriptor:
         return mangler.mangleAssociatedTypeDescriptor(
            cast<AssociatedTypeDecl>(getDecl()));

      case Kind::AssociatedConformanceDescriptor: {
         auto assocConformance = getAssociatedConformance();
         return mangler.mangleAssociatedConformanceDescriptor(
            cast<InterfaceDecl>(getDecl()),
            assocConformance.first,
            assocConformance.second);
      }

      case Kind::BaseConformanceDescriptor: {
         auto assocConformance = getAssociatedConformance();
         return mangler.mangleBaseConformanceDescriptor(
            cast<InterfaceDecl>(getDecl()),
            assocConformance.second);
      }

      case Kind::DefaultAssociatedConformanceAccessor: {
         auto assocConformance = getAssociatedConformance();
         return mangler.mangleDefaultAssociatedConformanceAccessor(
            cast<InterfaceDecl>(getDecl()),
            assocConformance.first,
            assocConformance.second);
      }

      case Kind::InterfaceConformanceDescriptor:
         return mangler.mangleInterfaceConformanceDescriptor(
            getRootInterfaceConformance());

      case Kind::EnumCase:
         return mangler.mangleEnumCase(getDecl());

      case Kind::FieldOffset:
         return mangler.mangleFieldOffset(getDecl());

      case Kind::InterfaceWitnessTable:
         return mangler.mangleWitnessTable(getRootInterfaceConformance());

      case Kind::GenericInterfaceWitnessTableInstantiationFunction:
         return mangler.mangleGenericInterfaceWitnessTableInstantiationFunction(
            getInterfaceConformance());

      case Kind::InterfaceWitnessTablePattern:
         return mangler.mangleInterfaceWitnessTablePattern(getInterfaceConformance());

      case Kind::InterfaceWitnessTableLazyAccessFunction:
         return mangler.mangleInterfaceWitnessTableLazyAccessFunction(getType(),
                                                                      getInterfaceConformance());

      case Kind::InterfaceWitnessTableLazyCacheVariable:
         return mangler.mangleInterfaceWitnessTableLazyCacheVariable(getType(),
                                                                     getInterfaceConformance());

      case Kind::AssociatedTypeWitnessTableAccessFunction: {
         auto assocConf = getAssociatedConformance();
         if (isa<GenericTypeParamType>(assocConf.first)) {
            return mangler.mangleBaseWitnessTableAccessFunction(
               getInterfaceConformance(), assocConf.second);
         }

         return mangler.mangleAssociatedTypeWitnessTableAccessFunction(
            getInterfaceConformance(), assocConf.first, assocConf.second);
      }

      case Kind::CoroutineContinuationPrototype:
         return mangler.mangleCoroutineContinuationPrototype(
            cast<PILFunctionType>(getType()));

         // An Objective-C class reference reference. The symbol is private, so
         // the mangling is unimportant; it should just be readable in LLVM IR.
      case Kind::ObjCClassRef: {
         llvm::SmallString<64> tempBuffer;
         StringRef name = cast<ClassDecl>(getDecl())->getObjCRuntimeName(tempBuffer);
         std::string Result("OBJC_CLASS_REF_$_");
         Result.append(name.data(), name.size());
         return Result;
      }

         // An Objective-C class reference;  not a swift mangling.
      case Kind::ObjCClass: {
         llvm::SmallString<64> TempBuffer;
         StringRef Name = cast<ClassDecl>(getDecl())->getObjCRuntimeName(TempBuffer);
         std::string Result("OBJC_CLASS_$_");
         Result.append(Name.data(), Name.size());
         return Result;
      }

         // An Objective-C metaclass reference;  not a swift mangling.
      case Kind::ObjCMetaclass: {
         llvm::SmallString<64> TempBuffer;
         StringRef Name = cast<ClassDecl>(getDecl())->getObjCRuntimeName(TempBuffer);
         std::string Result("OBJC_METACLASS_$_");
         Result.append(Name.data(), Name.size());
         return Result;
      }

      case Kind::PILFunction: {
         std::string Result(getPILFunction()->getName());
         if (isDynamicallyReplaceable()) {
            Result.append("TI");
         }
         return Result;
      }
      case Kind::DynamicallyReplaceableFunctionImpl: {
         assert(isa<AbstractFunctionDecl>(getDecl()));
         std::string Result;
         if (auto *Constructor = dyn_cast<ConstructorDecl>(getDecl())) {
            Result = mangler.mangleConstructorEntity(Constructor, isAllocator(),
               /*isCurried=*/false);
         } else  {
            Result = mangler.mangleEntity(getDecl(), /*isCurried=*/false);
         }
         Result.append("TI");
         return Result;
      }

      case Kind::DynamicallyReplaceableFunctionVariable: {
         std::string Result(getPILFunction()->getName());
         Result.append("TX");
         return Result;
      }

      case Kind::DynamicallyReplaceableFunctionKey: {
         std::string Result(getPILFunction()->getName());
         Result.append("Tx");
         return Result;
      }


      case Kind::DynamicallyReplaceableFunctionVariableAST: {
         assert(isa<AbstractFunctionDecl>(getDecl()));
         std::string Result;
         if (auto *Constructor = dyn_cast<ConstructorDecl>(getDecl())) {
            Result =
               mangler.mangleConstructorEntity(Constructor, isAllocator(),
                  /*isCurried=*/false);
         } else  {
            Result = mangler.mangleEntity(getDecl(), /*isCurried=*/false);
         }
         Result.append("TX");
         return Result;
      }

      case Kind::DynamicallyReplaceableFunctionKeyAST: {
         assert(isa<AbstractFunctionDecl>(getDecl()));
         std::string Result;
         if (auto *Constructor = dyn_cast<ConstructorDecl>(getDecl())) {
            Result =
               mangler.mangleConstructorEntity(Constructor, isAllocator(),
                  /*isCurried=*/false);
         } else  {
            Result = mangler.mangleEntity(getDecl(), /*isCurried=*/false);
         }
         Result.append("Tx");
         return Result;
      }

      case Kind::PILGlobalVariable:
         return getPILGlobalVariable()->getName();

      case Kind::ReflectionBuiltinDescriptor:
         return mangler.mangleReflectionBuiltinDescriptor(getType());
      case Kind::ReflectionFieldDescriptor:
         return mangler.mangleReflectionFieldDescriptor(getType());
      case Kind::ReflectionAssociatedTypeDescriptor:
         return mangler.mangleReflectionAssociatedTypeDescriptor(
            getInterfaceConformance());
   }
   llvm_unreachable("bad entity kind!");
}

PILLinkage LinkEntity::getLinkage(ForDefinition_t forDefinition) const {
   // For when `this` is a protocol conformance of some kind.
   auto getLinkageAsConformance = [&] {
      return getLinkageForInterfaceConformance(
         getInterfaceConformance()->getRootConformance(), forDefinition);
   };

   switch (getKind()) {
      case Kind::DispatchThunk:
      case Kind::DispatchThunkInitializer:
      case Kind::DispatchThunkAllocator:
      case Kind::MethodDescriptor:
      case Kind::MethodDescriptorInitializer:
      case Kind::MethodDescriptorAllocator: {
         auto *decl = getDecl();

         // Interface requirements don't have their own access control
         if (auto *proto = dyn_cast<InterfaceDecl>(decl->getDeclContext()))
            decl = proto;

         return getPILLinkage(getDeclLinkage(decl), forDefinition);
      }

         // Most type metadata depend on the formal linkage of their type.
      case Kind::ValueWitnessTable: {
         auto type = getType();

         // Builtin types, (), () -> () and so on are in the runtime.
         if (!type.getAnyNominal())
            return getPILLinkage(FormalLinkage::PublicUnique, forDefinition);

         // Imported types.
         if (isAccessorLazilyGenerated(getTypeMetadataAccessStrategy(type)))
            return PILLinkage::Shared;

         // Everything else is only referenced inside its module.
         return PILLinkage::Private;
      }

      case Kind::ObjCMetadataUpdateFunction:
      case Kind::TypeMetadataInstantiationCache:
      case Kind::TypeMetadataInstantiationFunction:
      case Kind::TypeMetadataSingletonInitializationCache:
      case Kind::TypeMetadataCompletionFunction:
      case Kind::TypeMetadataPattern:
         return PILLinkage::Private;

      case Kind::TypeMetadataLazyCacheVariable: {
         auto type = getType();

         // Imported types, non-primitive structural types.
         if (isAccessorLazilyGenerated(getTypeMetadataAccessStrategy(type)))
            return PILLinkage::Shared;

         // Everything else is only referenced inside its module.
         return PILLinkage::Private;
      }

      case Kind::TypeMetadataDemanglingCacheVariable:
         return PILLinkage::Shared;

      case Kind::TypeMetadata: {
         auto *nominal = getType().getAnyNominal();
         switch (getMetadataAddress()) {
            case TypeMetadataAddress::FullMetadata:
               // For imported types, the full metadata object is a candidate
               // for uniquing.
               if (getDeclLinkage(nominal) == FormalLinkage::PublicNonUnique)
                  return PILLinkage::Shared;

               // The full metadata object is private to the containing module.
               return PILLinkage::Private;
            case TypeMetadataAddress::AddressPoint: {
               return getPILLinkage(nominal
                                    ? getDeclLinkage(nominal)
                                    : FormalLinkage::PublicUnique,
                                    forDefinition);
            }
         }
         llvm_unreachable("bad kind");
      }

         // ...but we don't actually expose individual value witnesses (right now).
      case Kind::ValueWitness: {
         auto *nominal = getType().getAnyNominal();
         if (getDeclLinkage(nominal) == FormalLinkage::PublicNonUnique)
            return PILLinkage::Shared;
         return forDefinition ? PILLinkage::Private : PILLinkage::PrivateExternal;
      }

      case Kind::TypeMetadataAccessFunction:
         switch (getTypeMetadataAccessStrategy(getType())) {
            case MetadataAccessStrategy::PublicUniqueAccessor:
               return getPILLinkage(FormalLinkage::PublicUnique, forDefinition);
            case MetadataAccessStrategy::HiddenUniqueAccessor:
               return getPILLinkage(FormalLinkage::HiddenUnique, forDefinition);
            case MetadataAccessStrategy::PrivateAccessor:
               return getPILLinkage(FormalLinkage::Private, forDefinition);
            case MetadataAccessStrategy::ForeignAccessor:
            case MetadataAccessStrategy::NonUniqueAccessor:
               return PILLinkage::Shared;
         }
         llvm_unreachable("bad metadata access kind");

      case Kind::ObjCClassRef:
         return PILLinkage::Private;

         // Continuation prototypes need to be external or else LLVM will fret.
      case Kind::CoroutineContinuationPrototype:
         return PILLinkage::PublicExternal;


      case Kind::ObjCResilientClassStub: {
         switch (getMetadataAddress()) {
            case TypeMetadataAddress::FullMetadata:
               // The full class stub object is private to the containing module,
               // excpet for foreign types.
               return PILLinkage::Private;
            case TypeMetadataAddress::AddressPoint: {
               auto *classDecl = cast<ClassDecl>(getDecl());
               return getPILLinkage(getDeclLinkage(classDecl),
                                    forDefinition);
            }
         }
         llvm_unreachable("invalid metadata address");
      }

      case Kind::EnumCase: {
         auto *elementDecl = cast<EnumElementDecl>(getDecl());
         return getPILLinkage(getDeclLinkage(elementDecl), forDefinition);
      }

      case Kind::FieldOffset: {
         auto *varDecl = cast<VarDecl>(getDecl());

         auto linkage = getDeclLinkage(varDecl);

         // Classes with resilient storage don't expose field offset symbols.
         if (cast<ClassDecl>(varDecl->getDeclContext())->isResilient()) {
            assert(linkage != FormalLinkage::PublicNonUnique &&
                   "Cannot have a resilient class with non-unique linkage");

            if (linkage == FormalLinkage::PublicUnique)
               linkage = FormalLinkage::HiddenUnique;
         }

         return getPILLinkage(linkage, forDefinition);
      }

      case Kind::PropertyDescriptor: {
         // Return the linkage of the getter, which may be more permissive than the
         // property itself (for instance, with a private/internal property whose
         // accessor is @inlinable or @usableFromInline)
         auto getterDecl = cast<AbstractStorageDecl>(getDecl())
            ->getOpaqueAccessor(AccessorKind::Get);
         return getPILLinkage(getDeclLinkage(getterDecl), forDefinition);
      }

      case Kind::AssociatedConformanceDescriptor:
      case Kind::BaseConformanceDescriptor:
      case Kind::ObjCClass:
      case Kind::ObjCMetaclass:
      case Kind::SwiftMetaclassStub:
      case Kind::NominalTypeDescriptor:
      case Kind::ClassMetadataBaseOffset:
      case Kind::InterfaceDescriptor:
      case Kind::InterfaceRequirementsBaseDescriptor:
      case Kind::MethodLookupFunction:
      case Kind::OpaqueTypeDescriptor:
      case Kind::OpaqueTypeDescriptorAccessor:
      case Kind::OpaqueTypeDescriptorAccessorImpl:
      case Kind::OpaqueTypeDescriptorAccessorKey:
      case Kind::OpaqueTypeDescriptorAccessorVar:
         return getPILLinkage(getDeclLinkage(getDecl()), forDefinition);

      case Kind::AssociatedTypeDescriptor:
         return getPILLinkage(getDeclLinkage(getAssociatedType()->getInterface()),
                              forDefinition);

      case Kind::InterfaceWitnessTable:
      case Kind::InterfaceConformanceDescriptor:
         return getLinkageForInterfaceConformance(getRootInterfaceConformance(),
                                                  forDefinition);

      case Kind::InterfaceWitnessTablePattern:
         if (getLinkageAsConformance() == PILLinkage::Shared)
            return PILLinkage::Shared;
         return PILLinkage::Private;

      case Kind::InterfaceWitnessTableLazyAccessFunction:
      case Kind::InterfaceWitnessTableLazyCacheVariable: {
         auto *nominal = getType().getAnyNominal();
         assert(nominal);
         if (getDeclLinkage(nominal) == FormalLinkage::Private ||
             getLinkageAsConformance() == PILLinkage::Private) {
            return PILLinkage::Private;
         } else {
            return PILLinkage::Shared;
         }
      }

      case Kind::AssociatedTypeWitnessTableAccessFunction:
      case Kind::DefaultAssociatedConformanceAccessor:
      case Kind::GenericInterfaceWitnessTableInstantiationFunction:
         return PILLinkage::Private;

      case Kind::DynamicallyReplaceableFunctionKey:
      case Kind::PILFunction:
         return getPILFunction()->getEffectiveSymbolLinkage();

      case Kind::DynamicallyReplaceableFunctionImpl:
      case Kind::DynamicallyReplaceableFunctionKeyAST:
         return getPILLinkage(getDeclLinkage(getDecl()), forDefinition);


      case Kind::DynamicallyReplaceableFunctionVariable:
         return getPILFunction()->getEffectiveSymbolLinkage();
      case Kind::DynamicallyReplaceableFunctionVariableAST:
         return getPILLinkage(getDeclLinkage(getDecl()), forDefinition);

      case Kind::PILGlobalVariable:
         return getPILGlobalVariable()->getLinkage();

      case Kind::ReflectionBuiltinDescriptor:
      case Kind::ReflectionFieldDescriptor: {
         // Reflection descriptors for imported types have shared linkage,
         // since we may emit them in other TUs in the same module.
         if (auto *nominal = getType().getAnyNominal())
            if (getDeclLinkage(nominal) == FormalLinkage::PublicNonUnique)
               return PILLinkage::Shared;
         return PILLinkage::Private;
      }
      case Kind::ReflectionAssociatedTypeDescriptor:
         if (getLinkageAsConformance() == PILLinkage::Shared)
            return PILLinkage::Shared;
         return PILLinkage::Private;

      case Kind::ModuleDescriptor:
      case Kind::ExtensionDescriptor:
      case Kind::AnonymousDescriptor:
         return PILLinkage::Shared;
   }
   llvm_unreachable("bad link entity kind");
}

static bool isAvailableExternally(IRGenModule &IGM, PILFunction *F) {
   // TODO
   return true;
}

static bool isAvailableExternally(IRGenModule &IGM, const DeclContext *dc) {
   dc = dc->getModuleScopeContext();
   if (isa<ClangModuleUnit>(dc) ||
       dc == IGM.getPILModule().getAssociatedContext())
      return false;
   return true;
}

static bool isAvailableExternally(IRGenModule &IGM, const Decl *decl) {
   return isAvailableExternally(IGM, decl->getDeclContext());
}

static bool isAvailableExternally(IRGenModule &IGM, Type type) {
   if (auto decl = type->getAnyNominal())
      return isAvailableExternally(IGM, decl->getDeclContext());
   return true;
}

bool LinkEntity::isAvailableExternally(IRGenModule &IGM) const {
   switch (getKind()) {
      case Kind::DispatchThunk:
      case Kind::DispatchThunkInitializer:
      case Kind::DispatchThunkAllocator: {
         auto *decl = getDecl();

         // Interface requirements don't have their own access control
         if (auto *proto = dyn_cast<InterfaceDecl>(decl->getDeclContext()))
            decl = proto;

         return ::isAvailableExternally(IGM, getDecl());
      }

      case Kind::MethodDescriptor:
      case Kind::MethodDescriptorInitializer:
      case Kind::MethodDescriptorAllocator: {
         auto *decl = getDecl();

         // Interface requirements don't have their own access control
         if (auto *proto = dyn_cast<InterfaceDecl>(decl->getDeclContext()))
            decl = proto;

         // Method descriptors for internal class initializers can be referenced
         // from outside the module.
         if (auto *ctor = dyn_cast<ConstructorDecl>(decl)) {
            auto *classDecl = cast<ClassDecl>(ctor->getDeclContext());
            if (classDecl->getEffectiveAccess() == AccessLevel::Open)
               decl = classDecl;
         }

         return ::isAvailableExternally(IGM, getDecl());
      }

      case Kind::ValueWitnessTable:
      case Kind::TypeMetadata:
         return ::isAvailableExternally(IGM, getType());

      case Kind::ObjCClass:
      case Kind::ObjCMetaclass:
         // FIXME: Removing this triggers a linker bug
         return true;

      case Kind::AssociatedConformanceDescriptor:
      case Kind::BaseConformanceDescriptor:
      case Kind::SwiftMetaclassStub:
      case Kind::ClassMetadataBaseOffset:
      case Kind::PropertyDescriptor:
      case Kind::NominalTypeDescriptor:
      case Kind::InterfaceDescriptor:
      case Kind::InterfaceRequirementsBaseDescriptor:
      case Kind::MethodLookupFunction:
      case Kind::OpaqueTypeDescriptor:
      case Kind::OpaqueTypeDescriptorAccessor:
      case Kind::OpaqueTypeDescriptorAccessorImpl:
      case Kind::OpaqueTypeDescriptorAccessorKey:
      case Kind::OpaqueTypeDescriptorAccessorVar:
         return ::isAvailableExternally(IGM, getDecl());

      case Kind::AssociatedTypeDescriptor:
         return ::isAvailableExternally(
            IGM,
            (const Decl *)getAssociatedType()->getInterface());

      case Kind::EnumCase:
         return ::isAvailableExternally(IGM, getDecl());

      case Kind::InterfaceWitnessTable:
      case Kind::InterfaceConformanceDescriptor:
         return ::isAvailableExternally(IGM,
                                        getRootInterfaceConformance()->getDeclContext());

      case Kind::InterfaceWitnessTablePattern:
      case Kind::ObjCClassRef:
      case Kind::ModuleDescriptor:
      case Kind::ExtensionDescriptor:
      case Kind::AnonymousDescriptor:
      case Kind::TypeMetadataInstantiationCache:
      case Kind::TypeMetadataInstantiationFunction:
      case Kind::TypeMetadataSingletonInitializationCache:
      case Kind::TypeMetadataCompletionFunction:
      case Kind::TypeMetadataPattern:
      case Kind::DefaultAssociatedConformanceAccessor:
         return false;
      case Kind::DynamicallyReplaceableFunctionKey:
      case Kind::DynamicallyReplaceableFunctionVariable:
         return true;

      case Kind::PILFunction:
         return ::isAvailableExternally(IGM, getPILFunction());

      case Kind::FieldOffset: {
         return ::isAvailableExternally(IGM,
                                        cast<VarDecl>(getDecl())
                                           ->getDeclContext()
                                           ->getInnermostTypeContext());
      }

      case Kind::ObjCMetadataUpdateFunction:
      case Kind::ObjCResilientClassStub:
      case Kind::ValueWitness:
      case Kind::TypeMetadataAccessFunction:
      case Kind::TypeMetadataLazyCacheVariable:
      case Kind::TypeMetadataDemanglingCacheVariable:
      case Kind::InterfaceWitnessTableLazyAccessFunction:
      case Kind::InterfaceWitnessTableLazyCacheVariable:
      case Kind::AssociatedTypeWitnessTableAccessFunction:
      case Kind::GenericInterfaceWitnessTableInstantiationFunction:
      case Kind::PILGlobalVariable:
      case Kind::ReflectionBuiltinDescriptor:
      case Kind::ReflectionFieldDescriptor:
      case Kind::ReflectionAssociatedTypeDescriptor:
      case Kind::CoroutineContinuationPrototype:
      case Kind::DynamicallyReplaceableFunctionVariableAST:
      case Kind::DynamicallyReplaceableFunctionImpl:
      case Kind::DynamicallyReplaceableFunctionKeyAST:
         llvm_unreachable("Relative reference to unsupported link entity");
   }
   llvm_unreachable("bad link entity kind");
}

llvm::Type *LinkEntity::getDefaultDeclarationType(IRGenModule &IGM) const {
   switch (getKind()) {
      case Kind::ModuleDescriptor:
      case Kind::ExtensionDescriptor:
      case Kind::AnonymousDescriptor:
      case Kind::NominalTypeDescriptor:
      case Kind::PropertyDescriptor:
         return IGM.TypeContextDescriptorTy;
      case Kind::OpaqueTypeDescriptor:
         return IGM.OpaqueTypeDescriptorTy;
      case Kind::InterfaceDescriptor:
         return IGM.InterfaceDescriptorStructTy;
      case Kind::AssociatedTypeDescriptor:
      case Kind::AssociatedConformanceDescriptor:
      case Kind::BaseConformanceDescriptor:
      case Kind::InterfaceRequirementsBaseDescriptor:
         return IGM.InterfaceRequirementStructTy;
      case Kind::InterfaceConformanceDescriptor:
         return IGM.InterfaceConformanceDescriptorTy;
      case Kind::ObjCClassRef:
         return IGM.ObjCClassPtrTy;
      case Kind::ObjCClass:
      case Kind::ObjCMetaclass:
      case Kind::SwiftMetaclassStub:
         return IGM.ObjCClassStructTy;
      case Kind::TypeMetadataLazyCacheVariable:
         return IGM.TypeMetadataPtrTy;
      case Kind::TypeMetadataDemanglingCacheVariable:
         return llvm::StructType::get(IGM.Int32Ty, IGM.Int32Ty);
      case Kind::TypeMetadataSingletonInitializationCache:
         // TODO: put a cache variable on IGM
         return llvm::StructType::get(IGM.getLLVMContext(),
                                      {IGM.TypeMetadataPtrTy, IGM.Int8PtrTy});
      case Kind::TypeMetadata:
         switch (getMetadataAddress()) {
            case TypeMetadataAddress::FullMetadata:
               if (getType().getClassOrBoundGenericClass())
                  return IGM.FullHeapMetadataStructTy;
               else
                  return IGM.FullTypeMetadataStructTy;
            case TypeMetadataAddress::AddressPoint:
               return IGM.TypeMetadataStructTy;
         }
         llvm_unreachable("invalid metadata address");

      case Kind::TypeMetadataPattern:
         // TODO: Use a real type?
         return IGM.Int8Ty;

      case Kind::ClassMetadataBaseOffset:
         // TODO: put a cache variable on IGM
         return llvm::StructType::get(IGM.getLLVMContext(), {
            IGM.SizeTy,  // Immediate members offset
            IGM.Int32Ty, // Negative size in words
            IGM.Int32Ty  // Positive size in words
         });

      case Kind::TypeMetadataInstantiationCache:
         // TODO: put a cache variable on IGM
         return llvm::ArrayType::get(IGM.Int8PtrTy,
                                     NumGenericMetadataPrivateDataWords);
      case Kind::ReflectionBuiltinDescriptor:
      case Kind::ReflectionFieldDescriptor:
      case Kind::ReflectionAssociatedTypeDescriptor:
         return IGM.FieldDescriptorTy;
      case Kind::ValueWitnessTable: // TODO: use ValueWitnessTableTy
      case Kind::InterfaceWitnessTable:
      case Kind::InterfaceWitnessTablePattern:
         return IGM.WitnessTableTy;
      case Kind::FieldOffset:
         return IGM.SizeTy;
      case Kind::EnumCase:
         return IGM.Int32Ty;
      case Kind::InterfaceWitnessTableLazyCacheVariable:
         return IGM.WitnessTablePtrTy;
      case Kind::PILFunction:
         return IGM.FunctionPtrTy->getPointerTo();
      case Kind::MethodDescriptor:
      case Kind::MethodDescriptorInitializer:
      case Kind::MethodDescriptorAllocator:
         return IGM.MethodDescriptorStructTy;
      case Kind::DynamicallyReplaceableFunctionKey:
      case Kind::OpaqueTypeDescriptorAccessorKey:
         return IGM.DynamicReplacementKeyTy;
      case Kind::DynamicallyReplaceableFunctionVariable:
      case Kind::OpaqueTypeDescriptorAccessorVar:
         return IGM.DynamicReplacementLinkEntryTy;
      case Kind::ObjCMetadataUpdateFunction:
         return IGM.ObjCUpdateCallbackTy;
      case Kind::ObjCResilientClassStub:
         switch (getMetadataAddress()) {
            case TypeMetadataAddress::FullMetadata:
               return IGM.ObjCFullResilientClassStubTy;
            case TypeMetadataAddress::AddressPoint:
               return IGM.ObjCResilientClassStubTy;
         }
         llvm_unreachable("invalid metadata address");
      default:
         llvm_unreachable("declaration LLVM type not specified");
   }
}

Alignment LinkEntity::getAlignment(IRGenModule &IGM) const {
   switch (getKind()) {
      case Kind::ModuleDescriptor:
      case Kind::ExtensionDescriptor:
      case Kind::AnonymousDescriptor:
      case Kind::NominalTypeDescriptor:
      case Kind::InterfaceDescriptor:
      case Kind::AssociatedTypeDescriptor:
      case Kind::AssociatedConformanceDescriptor:
      case Kind::BaseConformanceDescriptor:
      case Kind::InterfaceConformanceDescriptor:
      case Kind::InterfaceRequirementsBaseDescriptor:
      case Kind::ReflectionBuiltinDescriptor:
      case Kind::ReflectionFieldDescriptor:
      case Kind::ReflectionAssociatedTypeDescriptor:
      case Kind::PropertyDescriptor:
      case Kind::EnumCase:
      case Kind::MethodDescriptor:
      case Kind::MethodDescriptorInitializer:
      case Kind::MethodDescriptorAllocator:
      case Kind::OpaqueTypeDescriptor:
         return Alignment(4);
      case Kind::ObjCClassRef:
      case Kind::ObjCClass:
      case Kind::TypeMetadataLazyCacheVariable:
      case Kind::TypeMetadataSingletonInitializationCache:
      case Kind::TypeMetadata:
      case Kind::TypeMetadataPattern:
      case Kind::ClassMetadataBaseOffset:
      case Kind::TypeMetadataInstantiationCache:
      case Kind::ValueWitnessTable:
      case Kind::FieldOffset:
      case Kind::InterfaceWitnessTableLazyCacheVariable:
      case Kind::InterfaceWitnessTable:
      case Kind::InterfaceWitnessTablePattern:
      case Kind::ObjCMetaclass:
      case Kind::SwiftMetaclassStub:
      case Kind::DynamicallyReplaceableFunctionVariable:
      case Kind::DynamicallyReplaceableFunctionKey:
      case Kind::OpaqueTypeDescriptorAccessorKey:
      case Kind::OpaqueTypeDescriptorAccessorVar:
      case Kind::ObjCResilientClassStub:
         return IGM.getPointerAlignment();
      case Kind::TypeMetadataDemanglingCacheVariable:
         return Alignment(8);
      case Kind::PILFunction:
         return Alignment(1);
      default:
         llvm_unreachable("alignment not specified");
   }
}

bool LinkEntity::isWeakImported(ModuleDecl *module) const {
   switch (getKind()) {
      case Kind::PILGlobalVariable:
         if (getPILGlobalVariable()->getDecl()) {
            return getPILGlobalVariable()->getDecl()->isWeakImported(module);
         }
         return false;
      case Kind::DynamicallyReplaceableFunctionKey:
      case Kind::DynamicallyReplaceableFunctionVariable:
      case Kind::PILFunction: {
         return getPILFunction()->isWeakImported();
      }

      case Kind::AssociatedConformanceDescriptor:
      case Kind::DefaultAssociatedConformanceAccessor: {
         // Associated conformance descriptors use the protocol as
         // their declaration, but are weak linked if the associated
         // type stored in extra storage area is weak linked.
         auto assocConformance = getAssociatedConformance();
         auto *depMemTy = assocConformance.first->castTo<DependentMemberType>();
         return depMemTy->getAssocType()->isWeakImported(module);
      }

      case Kind::BaseConformanceDescriptor:
         return cast<InterfaceDecl>(getDecl())->isWeakImported(module);

      case Kind::TypeMetadata:
      case Kind::TypeMetadataAccessFunction: {
         if (auto *nominalDecl = getType()->getAnyNominal())
            return nominalDecl->isWeakImported(module);
         return false;
      }

      case Kind::DispatchThunk:
      case Kind::DispatchThunkInitializer:
      case Kind::DispatchThunkAllocator:
      case Kind::MethodDescriptor:
      case Kind::MethodDescriptorInitializer:
      case Kind::MethodDescriptorAllocator:
      case Kind::MethodLookupFunction:
      case Kind::EnumCase:
      case Kind::FieldOffset:
      case Kind::ObjCClass:
      case Kind::ObjCClassRef:
      case Kind::ObjCMetaclass:
      case Kind::SwiftMetaclassStub:
      case Kind::ClassMetadataBaseOffset:
      case Kind::PropertyDescriptor:
      case Kind::NominalTypeDescriptor:
      case Kind::ModuleDescriptor:
      case Kind::InterfaceDescriptor:
      case Kind::InterfaceRequirementsBaseDescriptor:
      case Kind::AssociatedTypeDescriptor:
      case Kind::DynamicallyReplaceableFunctionKeyAST:
      case Kind::DynamicallyReplaceableFunctionVariableAST:
      case Kind::DynamicallyReplaceableFunctionImpl:
      case Kind::OpaqueTypeDescriptor:
      case Kind::OpaqueTypeDescriptorAccessor:
      case Kind::OpaqueTypeDescriptorAccessorImpl:
      case Kind::OpaqueTypeDescriptorAccessorKey:
      case Kind::OpaqueTypeDescriptorAccessorVar:
         return getDecl()->isWeakImported(module);

      case Kind::InterfaceWitnessTable:
      case Kind::InterfaceConformanceDescriptor:
         return getInterfaceConformance()->getRootConformance()
            ->isWeakImported(module);

         // TODO: Revisit some of the below, for weak conformances.
      case Kind::ObjCMetadataUpdateFunction:
      case Kind::ObjCResilientClassStub:
      case Kind::TypeMetadataPattern:
      case Kind::TypeMetadataInstantiationCache:
      case Kind::TypeMetadataInstantiationFunction:
      case Kind::TypeMetadataSingletonInitializationCache:
      case Kind::TypeMetadataCompletionFunction:
      case Kind::ExtensionDescriptor:
      case Kind::AnonymousDescriptor:
      case Kind::InterfaceWitnessTablePattern:
      case Kind::GenericInterfaceWitnessTableInstantiationFunction:
      case Kind::AssociatedTypeWitnessTableAccessFunction:
      case Kind::ReflectionAssociatedTypeDescriptor:
      case Kind::InterfaceWitnessTableLazyAccessFunction:
      case Kind::InterfaceWitnessTableLazyCacheVariable:
      case Kind::ValueWitness:
      case Kind::ValueWitnessTable:
      case Kind::TypeMetadataLazyCacheVariable:
      case Kind::TypeMetadataDemanglingCacheVariable:
      case Kind::ReflectionBuiltinDescriptor:
      case Kind::ReflectionFieldDescriptor:
      case Kind::CoroutineContinuationPrototype:
         return false;
   }

   llvm_unreachable("Bad link entity kind");
}

const SourceFile *LinkEntity::getSourceFileForEmission() const {
   const SourceFile *sf;

   // Shared-linkage entities don't get emitted with any particular file.
   if (hasSharedVisibility(getLinkage(NotForDefinition)))
      return nullptr;

   auto getSourceFileForDeclContext =
      [](const DeclContext *dc) -> const SourceFile * {
         if (!dc)
            return nullptr;
         return dc->getParentSourceFile();
      };

   switch (getKind()) {
      case Kind::DispatchThunk:
      case Kind::DispatchThunkInitializer:
      case Kind::DispatchThunkAllocator:
      case Kind::MethodDescriptor:
      case Kind::MethodDescriptorInitializer:
      case Kind::MethodDescriptorAllocator:
      case Kind::MethodLookupFunction:
      case Kind::EnumCase:
      case Kind::FieldOffset:
      case Kind::ObjCClass:
      case Kind::ObjCMetaclass:
      case Kind::SwiftMetaclassStub:
      case Kind::ObjCMetadataUpdateFunction:
      case Kind::ObjCResilientClassStub:
      case Kind::ClassMetadataBaseOffset:
      case Kind::PropertyDescriptor:
      case Kind::NominalTypeDescriptor:
      case Kind::TypeMetadataPattern:
      case Kind::TypeMetadataInstantiationCache:
      case Kind::TypeMetadataInstantiationFunction:
      case Kind::TypeMetadataSingletonInitializationCache:
      case Kind::TypeMetadataCompletionFunction:
      case Kind::InterfaceDescriptor:
      case Kind::InterfaceRequirementsBaseDescriptor:
      case Kind::AssociatedTypeDescriptor:
      case Kind::AssociatedConformanceDescriptor:
      case Kind::DefaultAssociatedConformanceAccessor:
      case Kind::BaseConformanceDescriptor:
      case Kind::DynamicallyReplaceableFunctionVariableAST:
      case Kind::DynamicallyReplaceableFunctionKeyAST:
      case Kind::DynamicallyReplaceableFunctionImpl:
      case Kind::OpaqueTypeDescriptor:
      case Kind::OpaqueTypeDescriptorAccessor:
      case Kind::OpaqueTypeDescriptorAccessorImpl:
      case Kind::OpaqueTypeDescriptorAccessorKey:
      case Kind::OpaqueTypeDescriptorAccessorVar:
         sf = getSourceFileForDeclContext(getDecl()->getDeclContext());
         if (!sf)
            return nullptr;
         break;

      case Kind::PILFunction:
      case Kind::DynamicallyReplaceableFunctionVariable:
      case Kind::DynamicallyReplaceableFunctionKey:
         sf = getSourceFileForDeclContext(getPILFunction()->getDeclContext());
         if (!sf)
            return nullptr;
         break;

      case Kind::PILGlobalVariable:
         if (auto decl = getPILGlobalVariable()->getDecl()) {
            sf = getSourceFileForDeclContext(decl->getDeclContext());
            if (!sf)
               return nullptr;
         } else {
            return nullptr;
         }
         break;

      case Kind::InterfaceWitnessTable:
      case Kind::InterfaceConformanceDescriptor:
         sf = getSourceFileForDeclContext(
            getRootInterfaceConformance()->getDeclContext());
         if (!sf)
            return nullptr;
         break;

      case Kind::InterfaceWitnessTablePattern:
      case Kind::GenericInterfaceWitnessTableInstantiationFunction:
      case Kind::AssociatedTypeWitnessTableAccessFunction:
      case Kind::ReflectionAssociatedTypeDescriptor:
      case Kind::InterfaceWitnessTableLazyCacheVariable:
      case Kind::InterfaceWitnessTableLazyAccessFunction:
         sf = getSourceFileForDeclContext(
            getInterfaceConformance()->getRootConformance()->getDeclContext());
         if (!sf)
            return nullptr;
         break;

      case Kind::TypeMetadata: {
         auto ty = getType();
         // Only fully concrete nominal type metadata gets emitted eagerly.
         auto nom = ty->getAnyNominal();
         if (!nom || nom->isGenericContext())
            return nullptr;

         sf = getSourceFileForDeclContext(nom);
         if (!sf)
            return nullptr;
         break;
      }

         // Always shared linkage
      case Kind::ModuleDescriptor:
      case Kind::ExtensionDescriptor:
      case Kind::AnonymousDescriptor:
      case Kind::ObjCClassRef:
      case Kind::TypeMetadataAccessFunction:
      case Kind::TypeMetadataLazyCacheVariable:
      case Kind::TypeMetadataDemanglingCacheVariable:
         return nullptr;

         // TODO
      case Kind::CoroutineContinuationPrototype:
      case Kind::ReflectionFieldDescriptor:
      case Kind::ReflectionBuiltinDescriptor:
      case Kind::ValueWitness:
      case Kind::ValueWitnessTable:
         return nullptr;
   }

   return sf;
}
