//===--- PILFunctionBuilder.cpp -------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "polarphp/pil/lang/PILFunctionBuilder.h"
#include "polarphp/ast/Availability.h"
#include "polarphp/ast/Decl.h"

using namespace polar;

PILFunction *PILFunctionBuilder::getOrCreateFunction(
   PILLocation loc, StringRef name, PILLinkage linkage,
   CanPILFunctionType type, IsBare_t isBarePILFunction,
   IsTransparent_t isTransparent, IsSerialized_t isSerialized,
   IsDynamicallyReplaceable_t isDynamic, ProfileCounter entryCount,
   IsThunk_t isThunk, SubclassScope subclassScope) {
   assert(!type->isNoEscape() && "Function decls always have escaping types.");
   if (auto fn = mod.lookUpFunction(name)) {
      assert(fn->getLoweredFunctionType() == type);
      assert(stripExternalFromLinkage(fn->getLinkage()) ==
             stripExternalFromLinkage(linkage));
      return fn;
   }

   auto fn = PILFunction::create(mod, linkage, name, type, nullptr, loc,
                                 isBarePILFunction, isTransparent, isSerialized,
                                 entryCount, isDynamic, IsNotExactSelfClass,
                                 isThunk, subclassScope);
   fn->setDebugScope(new (mod) PILDebugScope(loc, fn));
   return fn;
}

void PILFunctionBuilder::addFunctionAttributes(PILFunction *F,
                                               DeclAttributes &Attrs,
                                               PILModule &M,
                                               PILDeclRef constant) {

   for (auto *A : Attrs.getAttributes<SemanticsAttr>())
      F->addSemanticsAttr(cast<SemanticsAttr>(A)->Value);

   // Propagate @_specialize.
   for (auto *A : Attrs.getAttributes<SpecializeAttr>()) {
      auto *SA = cast<SpecializeAttr>(A);
      auto kind =
         SA->getSpecializationKind() == SpecializeAttr::SpecializationKind::Full
         ? PILSpecializeAttr::SpecializationKind::Full
         : PILSpecializeAttr::SpecializationKind::Partial;
      F->addSpecializeAttr(
         PILSpecializeAttr::create(M, SA->getSpecializedSgnature(),
                                   SA->isExported(), kind));
   }

   if (auto *OA = Attrs.getAttribute<OptimizeAttr>()) {
      F->setOptimizationMode(OA->getMode());
   }

   // @_silgen_name and @_cdecl functions may be called from C code somewhere.
   if (Attrs.hasAttribute<PILGenNameAttr>() || Attrs.hasAttribute<CDeclAttr>())
      F->setHasCReferences(true);

   // Propagate @_dynamicReplacement(for:).
   if (constant.isNull())
      return;
   auto *decl = constant.getDecl();

   // Only emit replacements for the objc entry point of objc methods.
   // @todo
//   if (decl->isObjC() &&
//       F->getLoweredFunctionType()->getExtInfo().getRepresentation() !=
//       PILFunctionTypeRepresentation::ObjCMethod)
//      return;

   // Only assign replacements when the thing being replaced is function-like and
   // explicitly declared.
   auto *origDecl = decl->getDynamicallyReplacedDecl();
   auto *replacedDecl = dyn_cast_or_null<AbstractFunctionDecl>(origDecl);
   if (!replacedDecl)
      return;
   // @todo
//   if (decl->isObjC()) {
//      F->setObjCReplacement(replacedDecl);
//      return;
//   }

   if (!constant.canBeDynamicReplacement())
      return;

   PILDeclRef declRef(replacedDecl, constant.kind, false);
   auto *replacedFunc =
      getOrCreateFunction(replacedDecl, declRef, NotForDefinition);

   assert(replacedFunc->getLoweredFunctionType() ==
          F->getLoweredFunctionType() ||
          replacedFunc->getLoweredFunctionType()->hasOpaqueArchetype());

   F->setDynamicallyReplacedFunction(replacedFunc);
}

PILFunction *
PILFunctionBuilder::getOrCreateFunction(PILLocation loc, PILDeclRef constant,
                                        ForDefinition_t forDefinition,
                                        ProfileCounter entryCount) {
   auto nameTmp = constant.mangle();
   auto constantType = mod.Types.getConstantFunctionType(
      TypeExpansionContext::minimal(), constant);
   PILLinkage linkage = constant.getLinkage(forDefinition);

   if (auto fn = mod.lookUpFunction(nameTmp)) {
      assert(fn->getLoweredFunctionType() == constantType);
      assert(fn->getLinkage() == linkage ||
             (forDefinition == ForDefinition_t::NotForDefinition &&
              fn->getLinkage() ==
              constant.getLinkage(ForDefinition_t::ForDefinition)));
      if (forDefinition) {
         // In all the cases where getConstantLinkage returns something
         // different for ForDefinition, it returns an available-externally
         // linkage.
         if (is_available_externally(fn->getLinkage())) {
            fn->setLinkage(constant.getLinkage(ForDefinition));
         }
      }
      return fn;
   }

   IsTransparent_t IsTrans =
      constant.isTransparent() ? IsTransparent : IsNotTransparent;
   IsSerialized_t IsSer = constant.isSerialized();

   EffectsKind EK = constant.hasEffectsAttribute()
                    ? constant.getEffectsAttribute()
                    : EffectsKind::Unspecified;

   Inline_t inlineStrategy = InlineDefault;
   if (constant.isNoinline())
      inlineStrategy = NoInline;
   else if (constant.isAlwaysInline())
      inlineStrategy = AlwaysInline;

   StringRef name = mod.allocateCopy(nameTmp);
   IsDynamicallyReplaceable_t IsDyn = IsNotDynamic;
   if (constant.isDynamicallyReplaceable()) {
      IsDyn = IsDynamic;
      IsTrans = IsNotTransparent;
   }

   auto *F = PILFunction::create(mod, linkage, name, constantType, nullptr, None,
                                 IsNotBare, IsTrans, IsSer, entryCount, IsDyn,
                                 IsNotExactSelfClass,
                                 IsNotThunk, constant.getSubclassScope(),
                                 inlineStrategy, EK);
   F->setDebugScope(new (mod) PILDebugScope(loc, F));

   F->setGlobalInit(constant.isGlobal());
   if (constant.hasDecl()) {
      auto decl = constant.getDecl();

      if (constant.isForeign && decl->hasClangNode())
         F->setClangNodeOwner(decl);

      F->setAvailabilityForLinkage(decl->getAvailabilityForLinkage());
      F->setAlwaysWeakImported(decl->isAlwaysWeakImported());

      if (auto *accessor = dyn_cast<AccessorDecl>(decl)) {
         auto *storage = accessor->getStorage();
         // Add attributes for e.g. computed properties.
         addFunctionAttributes(F, storage->getAttrs(), mod);
      }
      addFunctionAttributes(F, decl->getAttrs(), mod, constant);
   }

   return F;
}

PILFunction *PILFunctionBuilder::getOrCreateSharedFunction(
   PILLocation loc, StringRef name, CanPILFunctionType type,
   IsBare_t isBarePILFunction, IsTransparent_t isTransparent,
   IsSerialized_t isSerialized, ProfileCounter entryCount, IsThunk_t isThunk,
   IsDynamicallyReplaceable_t isDynamic) {
   return getOrCreateFunction(loc, name, PILLinkage::Shared, type,
                              isBarePILFunction, isTransparent, isSerialized,
                              isDynamic, entryCount, isThunk,
                              SubclassScope::NotApplicable);
}

PILFunction *PILFunctionBuilder::createFunction(
   PILLinkage linkage, StringRef name, CanPILFunctionType loweredType,
   GenericEnvironment *genericEnv, Optional<PILLocation> loc,
   IsBare_t isBarePILFunction, IsTransparent_t isTrans,
   IsSerialized_t isSerialized, IsDynamicallyReplaceable_t isDynamic,
   ProfileCounter entryCount, IsThunk_t isThunk, SubclassScope subclassScope,
   Inline_t inlineStrategy, EffectsKind EK, PILFunction *InsertBefore,
   const PILDebugScope *DebugScope) {
   return PILFunction::create(mod, linkage, name, loweredType, genericEnv, loc,
                              isBarePILFunction, isTrans, isSerialized,
                              entryCount, isDynamic, IsNotExactSelfClass,
                              isThunk, subclassScope,
                              inlineStrategy, EK, InsertBefore, DebugScope);
}
