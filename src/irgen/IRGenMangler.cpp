//===--- IRGenMangler.cpp - mangling of IRGen symbols ---------------------===//
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

#include "polarphp/irgen/internal/IRGenMangler.h"
#include "polarphp/ast/ExistentialLayout.h"
#include "polarphp/ast/IRGenOptions.h"
#include "polarphp/ast/InterfaceAssociations.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/demangling/ManglingMacros.h"
#include "polarphp/demangling/Demangle.h"
#include "polarphp/abi/MetadataValues.h"
#include "polarphp/clangimporter/ClangModule.h"
#include "llvm/Support/SaveAndRestore.h"

using namespace polar;
using namespace irgen;

const char *getManglingForWitness(polar::demangling::ValueWitnessKind kind) {
   switch (kind) {
#define VALUE_WITNESS(MANGLING, NAME) \
  case polar::demangling::ValueWitnessKind::NAME: return #MANGLING;
#include "polarphp/demangling/ValueWitnessManglingDef.h"
   }
   llvm_unreachable("not a function witness");
}

std::string IRGenMangler::mangleValueWitness(Type type, ValueWitness witness) {
   beginMangling();
   appendType(type);

   const char *Code = nullptr;
   switch (witness) {
#define GET_MANGLING(ID) \
    case ValueWitness::ID: Code = getManglingForWitness(polar::demangling::ValueWitnessKind::ID); break;
      GET_MANGLING(InitializeBufferWithCopyOfBuffer) \
    GET_MANGLING(Destroy) \
    GET_MANGLING(InitializeWithCopy) \
    GET_MANGLING(AssignWithCopy) \
    GET_MANGLING(InitializeWithTake) \
    GET_MANGLING(AssignWithTake) \
    GET_MANGLING(GetEnumTagSinglePayload) \
    GET_MANGLING(StoreEnumTagSinglePayload) \
    GET_MANGLING(GetEnumTag) \
    GET_MANGLING(DestructiveProjectEnumData) \
    GET_MANGLING(DestructiveInjectEnumTag)
#undef GET_MANGLING
      case ValueWitness::Size:
      case ValueWitness::Flags:
      case ValueWitness::ExtraInhabitantCount:
      case ValueWitness::Stride:
         llvm_unreachable("not a function witness");
   }
   appendOperator("w", Code);
   return finalize();
}

std::string IRGenMangler::manglePartialApplyForwarder(StringRef FuncName) {
   if (FuncName.empty()) {
      beginMangling();
   } else {
      if (FuncName.startswith(MANGLING_PREFIX_STR)) {
         Buffer << FuncName;
      } else {
         beginMangling();
         appendIdentifier(FuncName);
      }
   }
   appendOperator("TA");
   return finalize();
}

SymbolicMangling
IRGenMangler::withSymbolicReferences(IRGenModule &IGM,
                                     llvm::function_ref<void ()> body) {
   Mod = IGM.getTypePHPModule();
   OptimizeInterfaceNames = false;
   UseObjCInterfaceNames = true;

   llvm::SaveAndRestore<bool>
      AllowSymbolicReferencesLocally(AllowSymbolicReferences);
   llvm::SaveAndRestore<std::function<bool (SymbolicReferent)>>
      CanSymbolicReferenceLocally(CanSymbolicReference);

   AllowSymbolicReferences = true;
   CanSymbolicReference = [&IGM](SymbolicReferent s) -> bool {
      if (auto type = s.dyn_cast<const NominalTypeDecl *>()) {
         // The short-substitution types in the standard library have compact
         // manglings already, and the runtime ought to have a lookup table for
         // them. Symbolic referencing would be wasteful.
         if (type->getModuleContext()->isStdlibModule()
             && mangle::getStandardTypeSubst(type->getName().str())) {
            return false;
         }

         // TODO: We could assign a symbolic reference discriminator to refer
         // TODO
         // to objc protocol refs.
//      if (auto proto = dyn_cast<InterfaceDecl>(type)) {
//        if (proto->isObjC()) {
//          return false;
//        }
//      }

         // Classes defined in Objective-C don't have descriptors.
         // TODO: We could assign a symbolic reference discriminator to refer
         // to objc class refs.
         if (auto clas = dyn_cast<ClassDecl>(type)) {
            if (clas->hasClangNode()
                && clas->getForeignClassKind() != ClassDecl::ForeignKind::CFType) {
               return false;
            }
         }

         // TODO: ObjectMemoryReader for PE platforms still does not
         // implement symbol relocations. For now, on non-Mach-O platforms,
         // only symbolic reference things in the same module.
         if (IGM.TargetInfo.OutputObjectFormat != llvm::Triple::MachO
             && IGM.TargetInfo.OutputObjectFormat != llvm::Triple::ELF) {
            auto formalAccessScope = type->getFormalAccessScope(nullptr, true);
            if ((formalAccessScope.isPublic() || formalAccessScope.isInternal()) &&
                (!IGM.CurSourceFile ||
                 IGM.CurSourceFile != type->getParentSourceFile())) {
               return false;
            }
         }

         return true;
      } else if (auto opaque = s.dyn_cast<const OpaqueTypeDecl *>()) {
         // Always symbolically reference opaque types.
         return true;
      } else {
         llvm_unreachable("symbolic referent not handled");
      }
   };

   SymbolicReferences.clear();

   body();

   return {finalize(), std::move(SymbolicReferences)};
}

SymbolicMangling
IRGenMangler::mangleTypeForReflection(IRGenModule &IGM,
                                      Type Ty) {
   return withSymbolicReferences(IGM, [&]{
      appendType(Ty);
   });
}

std::string IRGenMangler::mangleInterfaceConformanceDescriptor(
   const RootInterfaceConformance *conformance) {
   beginMangling();
   if (isa<NormalInterfaceConformance>(conformance)) {
      appendInterfaceConformance(conformance);
      appendOperator("Mc");
   } else {
      auto protocol = cast<SelfInterfaceConformance>(conformance)->getInterface();
      appendInterfaceName(protocol);
      appendOperator("MS");
   }
   return finalize();
}

SymbolicMangling
IRGenMangler::mangleInterfaceConformanceForReflection(IRGenModule &IGM,
                                                      Type ty, InterfaceConformanceRef conformance) {
   return withSymbolicReferences(IGM, [&]{
      if (conformance.isConcrete()) {
         appendInterfaceConformance(conformance.getConcrete());
      } else {
         // Use a special mangling for abstract conformances.
         appendType(ty);
         appendInterfaceName(conformance.getAbstract());
      }
   });
}

std::string IRGenMangler::mangleTypeForLLVMTypeName(CanType Ty) {
   // To make LLVM IR more readable we always add a 'T' prefix so that type names
   // don't start with a digit and don't need to be quoted.
   Buffer << 'T';
   if (auto P = dyn_cast<InterfaceType>(Ty)) {
      appendInterfaceName(P->getDecl(), /*allowStandardSubstitution=*/false);
      appendOperator("P");
   } else {
      appendType(Ty);
   }
   return finalize();
}

std::string IRGenMangler::
mangleInterfaceForLLVMTypeName(InterfaceCompositionType *type) {
   ExistentialLayout layout = type->getExistentialLayout();

   if (type->isAny()) {
      Buffer << "Any";
   } else if (layout.isAnyObject()) {
      Buffer << "AnyObject";
   } else {
      // To make LLVM IR more readable we always add a 'T' prefix so that type names
      // don't start with a digit and don't need to be quoted.
      Buffer << 'T';
      auto protocols = layout.getInterfaces();
      for (unsigned i = 0, e = protocols.size(); i != e; ++i) {
         appendInterfaceName(protocols[i]->getDecl());
         if (i == 0)
            appendOperator("_");
      }
      if (auto superclass = layout.explicitSuperclass) {
         // We share type infos for different instantiations of a generic type
         // when the archetypes have the same exemplars.  We cannot mangle
         // archetypes, and the mangling does not have to be unique, so we just
         // mangle the unbound generic form of the type.
         if (superclass->hasArchetype()) {
            superclass = superclass->getClassOrBoundGenericClass()
               ->getDeclaredType();
         }

         appendType(CanType(superclass));
         appendOperator("Xc");
      } else if (layout.getLayoutConstraint()) {
         appendOperator("Xl");
      } else {
         appendOperator("p");
      }
   }
   return finalize();
}

std::string IRGenMangler::
mangleSymbolNameForSymbolicMangling(const SymbolicMangling &mangling,
                                    MangledTypeRefRole role) {
   beginManglingWithoutPrefix();
   const char *prefix;
   switch (role) {
      case MangledTypeRefRole::DefaultAssociatedTypeWitness:
         prefix = "default assoc type ";
         break;

      case MangledTypeRefRole::Metadata:
      case MangledTypeRefRole::Reflection:
         prefix = "symbolic ";
         break;
   }
   auto prefixLen = strlen(prefix);

   Buffer << prefix << mangling.String;

   for (auto &symbol : mangling.SymbolicReferences) {
      // Fill in the placeholder space with something printable.
      auto referent = symbol.first;
      auto offset = symbol.second;
      Storage[prefixLen + offset]
         = Storage[prefixLen + offset+1]
         = Storage[prefixLen + offset+2]
         = Storage[prefixLen + offset+3]
         = Storage[prefixLen + offset+4]
         = '_';
      Buffer << ' ';
      if (auto ty = referent.dyn_cast<const NominalTypeDecl*>())
         appendContext(ty, ty->getAlternateModuleName());
      else if (auto opaque = referent.dyn_cast<const OpaqueTypeDecl*>())
         appendOpaqueDeclName(opaque);
      else
         llvm_unreachable("unhandled referent");
   }

   return finalize();
}

std::string IRGenMangler::mangleSymbolNameForAssociatedConformanceWitness(
   const NormalInterfaceConformance *conformance,
   CanType associatedType,
   const InterfaceDecl *proto) {
   beginManglingWithoutPrefix();
   if (conformance) {
      Buffer << "associated conformance ";
      appendInterfaceConformance(conformance);
   } else {
      Buffer << "default associated conformance";
   }

   bool isFirstAssociatedTypeIdentifier = true;
   appendAssociatedTypePath(associatedType, isFirstAssociatedTypeIdentifier);
   appendInterfaceName(proto);
   return finalize();
}

std::string IRGenMangler::mangleSymbolNameForMangledMetadataAccessorString(
   const char *kind,
   CanGenericSignature genericSig,
   CanType type) {
   beginManglingWithoutPrefix();
   Buffer << kind << " ";

   if (genericSig)
      appendGenericSignature(genericSig);

   if (type)
      appendType(type);
   return finalize();
}

std::string IRGenMangler::mangleSymbolNameForMangledConformanceAccessorString(
   const char *kind,
   CanGenericSignature genericSig,
   CanType type,
   InterfaceConformanceRef conformance) {
   beginManglingWithoutPrefix();
   Buffer << kind << " ";

   if (genericSig)
      appendGenericSignature(genericSig);

   if (type)
      appendType(type);

   if (conformance.isConcrete())
      appendConcreteInterfaceConformance(conformance.getConcrete());
   else if (conformance.isAbstract())
      appendInterfaceName(conformance.getAbstract());
   else
      assert(conformance.isInvalid() && "Unknown protocol conformance");
   return finalize();
}

std::string IRGenMangler::mangleSymbolNameForGenericEnvironment(
   CanGenericSignature genericSig) {
   beginManglingWithoutPrefix();
   Buffer << "generic environment ";
   appendGenericSignature(genericSig);
   return finalize();
}
