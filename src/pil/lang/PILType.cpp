//===--- PILType.cpp - Defines PILType ------------------------------------===//
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

#include "polarphp/pil/lang/PILType.h"
#include "polarphp/ast/AstMangler.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/ExistentialLayout.h"
#include "polarphp/ast/GenericEnvironment.h"
#include "polarphp/ast/LazyResolver.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/Type.h"
#include "polarphp/pil/lang/AbstractionPattern.h"
#include "polarphp/pil/lang/PILFunctionConventions.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/lang/TypeLowering.h"

using namespace polar;
using namespace polar::lowering;

PILType PILType::getExceptionType(const AstContext &C) {
   return PILType::getPrimitiveObjectType(C.getExceptionType());
}

PILType PILType::getNativeObjectType(const AstContext &C) {
   return PILType(C.TheNativeObjectType, PILValueCategory::Object);
}

PILType PILType::getBridgeObjectType(const AstContext &C) {
   return PILType(C.TheBridgeObjectType, PILValueCategory::Object);
}

PILType PILType::getRawPointerType(const AstContext &C) {
   return getPrimitiveObjectType(C.TheRawPointerType);
}

PILType PILType::getBuiltinIntegerLiteralType(const AstContext &C) {
   return getPrimitiveObjectType(C.TheIntegerLiteralType);
}

PILType PILType::getBuiltinIntegerType(unsigned bitWidth,
                                       const AstContext &C) {
   return getPrimitiveObjectType(CanType(BuiltinIntegerType::get(bitWidth, C)));
}

PILType PILType::getBuiltinFloatType(BuiltinFloatType::FPKind Kind,
                                     const AstContext &C) {
   CanType ty;
   switch (Kind) {
      case BuiltinFloatType::IEEE16:  ty = C.TheIEEE16Type; break;
      case BuiltinFloatType::IEEE32:  ty = C.TheIEEE32Type; break;
      case BuiltinFloatType::IEEE64:  ty = C.TheIEEE64Type; break;
      case BuiltinFloatType::IEEE80:  ty = C.TheIEEE80Type; break;
      case BuiltinFloatType::IEEE128: ty = C.TheIEEE128Type; break;
      case BuiltinFloatType::PPC128:  ty = C.ThePPC128Type; break;
   }
   return getPrimitiveObjectType(ty);
}

PILType PILType::getBuiltinWordType(const AstContext &C) {
   return getPrimitiveObjectType(CanType(BuiltinIntegerType::getWordType(C)));
}

PILType PILType::getOptionalType(PILType type) {
   auto &ctx = type.getAstContext();
   auto optType = BoundGenericEnumType::get(ctx.getOptionalDecl(), Type(),
                                            { type.getAstType() });
   return getPrimitiveType(CanType(optType), type.getCategory());
}

PILType PILType::getPILTokenType(const AstContext &C) {
   return getPrimitiveObjectType(C.ThePILTokenType);
}

bool PILType::isTrivial(const PILFunction &F) const {
   auto contextType = hasTypeParameter() ? F.mapTypeIntoContext(*this) : *this;

   return F.getTypeLowering(contextType).isTrivial();
}

bool PILType::isReferenceCounted(PILModule &M) const {
   return M.Types.getTypeLowering(*this,
                                  TypeExpansionContext::minimal())
      .isReferenceCounted();
}

bool PILType::isNoReturnFunction(PILModule &M) const {
   if (auto funcTy = dyn_cast<PILFunctionType>(getAstType()))
      return funcTy->isNoReturnFunction(M);

   return false;
}

std::string PILType::getMangledName() const {
   mangle::AstMangler mangler(false/*use dwarf mangling*/);
   return mangler.mangleTypeWithoutPrefix(getAstType());
}

std::string PILType::getAsString() const {
   std::string Result;
   llvm::raw_string_ostream OS(Result);
   print(OS);
   return OS.str();
}

bool PILType::isPointerSizeAndAligned() {
   auto &C = getAstContext();
   if (isHeapObjectReferenceType()
       || getAstType()->isEqual(C.TheRawPointerType)) {
      return true;
   }
   if (auto intTy = dyn_cast<BuiltinIntegerType>(getAstType()))
      return intTy->getWidth().isPointerWidth();

   return false;
}

// Reference cast from representations with single pointer low bits.
// Only reference cast to simple single pointer representations.
//
// TODO: handle casting to a loadable existential by generating
// init_existential_ref. Until then, only promote to a heap object dest.
bool PILType::canRefCast(PILType operTy, PILType resultTy, PILModule &M) {
   auto fromTy = operTy.unwrapOptionalType();
   auto toTy = resultTy.unwrapOptionalType();
   return (fromTy.isHeapObjectReferenceType() || fromTy.isClassExistentialType())
          && toTy.isHeapObjectReferenceType();
}

PILType PILType::getFieldType(VarDecl *field, TypeConverter &TC,
                              TypeExpansionContext context) const {
   AbstractionPattern origFieldTy = TC.getAbstractionPattern(field);
   CanType substFieldTy;
   if (field->hasClangNode()) {
      substFieldTy = origFieldTy.getType();
   } else {
      substFieldTy =
         getAstType()->getTypeOfMember(&TC.M, field, nullptr)->getCanonicalType();
   }

   auto loweredTy =
      TC.getLoweredRValueType(context, origFieldTy, substFieldTy);
   if (isAddress() || getClassOrBoundGenericClass() != nullptr) {
      return PILType::getPrimitiveAddressType(loweredTy);
   } else {
      return PILType::getPrimitiveObjectType(loweredTy);
   }
}

PILType PILType::getFieldType(VarDecl *field, PILModule &M,
                              TypeExpansionContext context) const {
   return getFieldType(field, M.Types, context);
}

PILType PILType::getEnumElementType(EnumElementDecl *elt, TypeConverter &TC,
                                    TypeExpansionContext context) const {
   assert(elt->getDeclContext() == getEnumOrBoundGenericEnum());
   assert(elt->hasAssociatedValues());

   if (auto objectType = getAstType().getOptionalObjectType()) {
      assert(elt == TC.Context.getOptionalSomeDecl());
      return PILType(objectType, getCategory());
   }

   // If the case is indirect, then the payload is boxed.
   if (elt->isIndirect() || elt->getParentEnum()->isIndirect()) {
      auto box = TC.getBoxTypeForEnumElement(context, *this, elt);
      return PILType(PILType::getPrimitiveObjectType(box).getAstType(),
                     getCategory());
   }

   auto substEltTy =
      getAstType()->getTypeOfMember(&TC.M, elt,
                                    elt->getArgumentInterfaceType());
   auto loweredTy = TC.getLoweredRValueType(
      context, TC.getAbstractionPattern(elt), substEltTy);

   return PILType(loweredTy, getCategory());
}

PILType PILType::getEnumElementType(EnumElementDecl *elt, PILModule &M,
                                    TypeExpansionContext context) const {
   return getEnumElementType(elt, M.Types, context);
}

bool PILType::isLoadableOrOpaque(const PILFunction &F) const {
   PILModule &M = F.getModule();
   return isLoadable(F) || !PILModuleConventions(M).useLoweredAddresses();
}

bool PILType::isAddressOnly(const PILFunction &F) const {
   auto contextType = hasTypeParameter() ? F.mapTypeIntoContext(*this) : *this;

   return F.getTypeLowering(contextType).isAddressOnly();
}

PILType PILType::substGenericArgs(PILModule &M, SubstitutionMap SubMap,
                                  TypeExpansionContext context) const {
   auto fnTy = castTo<PILFunctionType>();
   auto canFnTy = CanPILFunctionType(fnTy->substGenericArgs(M, SubMap, context));
   return PILType::getPrimitiveObjectType(canFnTy);
}

bool PILType::isHeapObjectReferenceType() const {
   auto &C = getAstContext();
   auto Ty = getAstType();
   if (Ty->isBridgeableObjectType())
      return true;
   if (Ty->isEqual(C.TheNativeObjectType))
      return true;
   if (Ty->isEqual(C.TheBridgeObjectType))
      return true;
   if (is<PILBoxType>())
      return true;
   return false;
}

bool PILType::aggregateContainsRecord(PILType Record, PILModule &Mod,
                                      TypeExpansionContext context) const {
   assert(!hasArchetype() && "Agg should be proven to not be generic "
                             "before passed to this function.");
   assert(!Record.hasArchetype() && "Record should be proven to not be generic "
                                    "before passed to this function.");

   llvm::SmallVector<PILType, 8> Worklist;
   Worklist.push_back(*this);

   // For each "subrecord" of agg in the worklist...
   while (!Worklist.empty()) {
      PILType Ty = Worklist.pop_back_val();

      // If it is record, we succeeded. Return true.
      if (Ty == Record)
         return true;

      // Otherwise, we gather up sub-records that need to be checked for
      // checking... First handle the tuple case.
      if (CanTupleType TT = Ty.getAs<TupleType>()) {
         for (unsigned i = 0, e = TT->getNumElements(); i != e; ++i)
            Worklist.push_back(Ty.getTupleElementType(i));
         continue;
      }

      // Then if we have an enum...
      if (EnumDecl *E = Ty.getEnumOrBoundGenericEnum()) {
         for (auto Elt : E->getAllElements())
            if (Elt->hasAssociatedValues())
               Worklist.push_back(Ty.getEnumElementType(Elt, Mod, context));
         continue;
      }

      // Then if we have a struct address...
      if (StructDecl *S = Ty.getStructOrBoundGenericStruct())
         for (VarDecl *Var : S->getStoredProperties())
            Worklist.push_back(Ty.getFieldType(Var, Mod, context));

      // If we have a class address, it is a pointer so it cannot contain other
      // types.

      // If we reached this point, then this type has no subrecords. Since it does
      // not equal our record, we can skip it.
   }

   // Could not find the record in the aggregate.
   return false;
}

bool PILType::aggregateHasUnreferenceableStorage() const {
   if (auto s = getStructOrBoundGenericStruct()) {
      return s->hasUnreferenceableStorage();
   }
   return false;
}

PILType PILType::getOptionalObjectType() const {
   if (auto objectTy = getAstType().getOptionalObjectType()) {
      return PILType(objectTy, getCategory());
   }

   return PILType();
}

PILType PILType::unwrapOptionalType() const {
   if (auto objectTy = getOptionalObjectType()) {
      return objectTy;
   }

   return *this;
}

/// True if the given type value is nonnull, and the represented type is NSError
/// or CFError, the error classes for which we support "toll-free" bridging to
/// Error existentials.
static bool isBridgedErrorClass(AstContext &ctx, Type t) {
   // There's no bridging if ObjC interop is disabled.
   if (!ctx.LangOpts.EnableObjCInterop)
      return false;

   if (!t)
      return false;

   if (auto archetypeType = t->getAs<ArchetypeType>())
      t = archetypeType->getSuperclass();

   // NSError (TODO: and CFError) can be bridged.
   // @todo
//   auto nsErrorType = ctx.getNSErrorType();
//   if (t && nsErrorType && nsErrorType->isExactSuperclassOf(t))
//      return true;

   return false;
}

ExistentialRepresentation
PILType::getPreferredExistentialRepresentation(Type containedType) const {
   // Existential metatypes always use metatype representation.
   if (is<ExistentialMetatypeType>())
      return ExistentialRepresentation::Metatype;

   // If the type isn't existential, then there is no representation.
   if (!isExistentialType())
      return ExistentialRepresentation::None;

   auto layout = getAstType().getExistentialLayout();

   if (layout.isErrorExistential()) {
      // NSError or CFError references can be adopted directly as Error
      // existentials.
      if (isBridgedErrorClass(getAstContext(), containedType)) {
         return ExistentialRepresentation::Class;
      } else {
         return ExistentialRepresentation::Boxed;
      }
   }

   // A class-constrained protocol composition can adopt the conforming
   // class reference directly.
   if (layout.requiresClass())
      return ExistentialRepresentation::Class;

   // Otherwise, we need to use a fixed-sized buffer.
   return ExistentialRepresentation::Opaque;
}

bool
PILType::canUseExistentialRepresentation(ExistentialRepresentation repr,
                                         Type containedType) const {
   switch (repr) {
      case ExistentialRepresentation::None:
         return !isAnyExistentialType();
      case ExistentialRepresentation::Opaque:
      case ExistentialRepresentation::Class:
      case ExistentialRepresentation::Boxed: {
         // Look at the protocols to see what representation is appropriate.
         if (!isExistentialType())
            return false;

         auto layout = getAstType().getExistentialLayout();

         switch (layout.getKind()) {
            // A class-constrained composition uses ClassReference representation;
            // otherwise, we use a fixed-sized buffer.
            case ExistentialLayout::Kind::Class:
               return repr == ExistentialRepresentation::Class;
               // The (uncomposed) Error existential uses a special boxed
               // representation. It can also adopt class references of bridged
               // error types directly.
            case ExistentialLayout::Kind::Error:
               return repr == ExistentialRepresentation::Boxed
                      || (repr == ExistentialRepresentation::Class
                          && isBridgedErrorClass(getAstContext(), containedType));
            case ExistentialLayout::Kind::Opaque:
               return repr == ExistentialRepresentation::Opaque;
         }
         llvm_unreachable("unknown existential kind!");
      }
      case ExistentialRepresentation::Metatype:
         return is<ExistentialMetatypeType>();
   }

   llvm_unreachable("Unhandled ExistentialRepresentation in switch.");
}

PILType PILType::mapTypeOutOfContext() const {
   return PILType::getPrimitiveType(getAstType()->mapTypeOutOfContext()
                                       ->getCanonicalType(),
                                    getCategory());
}

CanType polar::getPILBoxFieldLoweredType(TypeExpansionContext context,
                                         PILBoxType *type, TypeConverter &TC,
                                         unsigned index) {
   auto fieldTy = PILType::getPrimitiveObjectType(
      type->getLayout()->getFields()[index].getLoweredType());

   // Map the type into the new expansion context, which might substitute opaque
   // types.
   auto sig = type->getLayout()->getGenericSignature();
   fieldTy = TC.getTypeLowering(fieldTy, context, sig)
      .getLoweredType();

   // Apply generic arguments if the layout is generic.
   if (auto subMap = type->getSubstitutions()) {
      fieldTy = fieldTy.subst(TC,
                              QuerySubstitutionMap{subMap},
                              LookUpConformanceInSubstitutionMap(subMap),
                              sig);
   }

   return fieldTy.getAstType();
}

ValueOwnershipKind
PILResultInfo::getOwnershipKind(PILFunction &F) const {
   auto &M = F.getModule();
   auto FTy = F.getLoweredFunctionType();
   auto sig = FTy->getInvocationGenericSignature();

   bool IsTrivial = getPILStorageType(M, FTy).isTrivial(F);
   switch (getConvention()) {
      case ResultConvention::Indirect:
         return PILModuleConventions(M).isPILIndirect(*this)
                ? ValueOwnershipKind::None
                : ValueOwnershipKind::Owned;
      case ResultConvention::Autoreleased:
      case ResultConvention::Owned:
         return ValueOwnershipKind::Owned;
      case ResultConvention::Unowned:
      case ResultConvention::UnownedInnerPointer:
         if (IsTrivial)
            return ValueOwnershipKind::None;
         return ValueOwnershipKind::Unowned;
   }

   llvm_unreachable("Unhandled ResultConvention in switch.");
}

PILModuleConventions::PILModuleConventions(PILModule &M)
   : M(&M),
     loweredAddresses(!M.getAstContext().LangOpts.EnablePILOpaqueValues
                      || M.getStage() == PILStage::Lowered)
{}

bool PILModuleConventions::isReturnedIndirectlyInPIL(PILType type,
                                                     PILModule &M) {
   if (PILModuleConventions(M).loweredAddresses) {
      return M.Types.getTypeLowering(type, TypeExpansionContext::minimal())
         .isAddressOnly();
   }

   return false;
}

bool PILModuleConventions::isPassedIndirectlyInPIL(PILType type, PILModule &M) {
   if (PILModuleConventions(M).loweredAddresses) {
      return M.Types.getTypeLowering(type, TypeExpansionContext::minimal())
         .isAddressOnly();
   }

   return false;
}


bool PILFunctionType::isNoReturnFunction(PILModule &M) const {
   for (unsigned i = 0, e = getNumResults(); i < e; ++i) {
      if (getResults()[i].getReturnValueType(M, this)->isUninhabited())
         return true;
   }

   return false;
}

#ifndef NDEBUG
static bool areOnlyAbstractionDifferent(CanType type1, CanType type2) {
   assert(type1->isLegalPILType());
   assert(type2->isLegalPILType());

   // Exact equality is fine.
   if (type1 == type2)
      return true;

   // Either both types should be optional or neither should be.
   if (auto object1 = type1.getOptionalObjectType()) {
      auto object2 = type2.getOptionalObjectType();
      if (!object2)
         return false;
      return areOnlyAbstractionDifferent(object1, object2);
   }
   if (type2.getOptionalObjectType())
      return false;

   // Either both types should be tuples or neither should be.
   if (auto tuple1 = dyn_cast<TupleType>(type1)) {
      auto tuple2 = dyn_cast<TupleType>(type2);
      if (!tuple2)
         return false;
      if (tuple1->getNumElements() != tuple2->getNumElements())
         return false;
      for (auto i : indices(tuple2->getElementTypes()))
         if (!areOnlyAbstractionDifferent(tuple1.getElementType(i),
                                          tuple2.getElementType(i)))
            return false;
      return true;
   }
   if (isa<TupleType>(type2))
      return false;

   // Either both types should be metatypes or neither should be.
   if (auto meta1 = dyn_cast<AnyMetatypeType>(type1)) {
      auto meta2 = dyn_cast<AnyMetatypeType>(type2);
      if (!meta2)
         return false;
      if (meta1.getInstanceType() != meta2.getInstanceType())
         return false;
      return true;
   }

   // Either both types should be functions or neither should be.
   if (auto fn1 = dyn_cast<PILFunctionType>(type1)) {
      auto fn2 = dyn_cast<PILFunctionType>(type2);
      if (!fn2)
         return false;
      // TODO: maybe there are checks we can do here?
      (void)fn1;
      (void)fn2;
      return true;
   }
   if (isa<PILFunctionType>(type2))
      return false;

   llvm_unreachable("no other types should differ by abstraction");
}
#endif

/// Given two PIL types which are representations of the same type,
/// check whether they have an abstraction difference.
bool PILType::hasAbstractionDifference(PILFunctionTypeRepresentation rep,
                                       PILType type2) {
   CanType ct1 = getAstType();
   CanType ct2 = type2.getAstType();
   assert(getPILFunctionLanguage(rep) == PILFunctionLanguage::C ||
          areOnlyAbstractionDifferent(ct1, ct2));
   (void)ct1;
   (void)ct2;

   // Assuming that we've applied the same substitutions to both types,
   // abstraction equality should equal type equality.
   return (*this != type2);
}

bool PILType::isLoweringOf(TypeExpansionContext context, PILModule &Mod,
                           CanType formalType) {
   PILType loweredType = *this;
   if (formalType->hasOpaqueArchetype() &&
       context.shouldLookThroughOpaqueTypeArchetypes() &&
       loweredType.getAstType() ==
       Mod.Types.getLoweredRValueType(context, formalType))
      return true;

   // Optional lowers its contained type.
   PILType loweredObjectType = loweredType.getOptionalObjectType();
   CanType formalObjectType = formalType.getOptionalObjectType();

   if (loweredObjectType) {
      return formalObjectType &&
             loweredObjectType.isLoweringOf(context, Mod, formalObjectType);
   }

   // Metatypes preserve their instance type through lowering.
   if (auto loweredMT = loweredType.getAs<MetatypeType>()) {
      if (auto formalMT = dyn_cast<MetatypeType>(formalType)) {
         return loweredMT.getInstanceType() == formalMT.getInstanceType();
      }
   }

   if (auto loweredEMT = loweredType.getAs<ExistentialMetatypeType>()) {
      if (auto formalEMT = dyn_cast<ExistentialMetatypeType>(formalType)) {
         return loweredEMT.getInstanceType() == formalEMT.getInstanceType();
      }
   }

   // TODO: Function types go through a more elaborate lowering.
   // For now, just check that a PIL function type came from some AST function
   // type.
   if (loweredType.is<PILFunctionType>())
      return isa<AnyFunctionType>(formalType);

   // Tuples are lowered elementwise.
   // TODO: Will this always be the case?
   if (auto loweredTT = loweredType.getAs<TupleType>()) {
      if (auto formalTT = dyn_cast<TupleType>(formalType)) {
         if (loweredTT->getNumElements() != formalTT->getNumElements())
            return false;
         for (unsigned i = 0, e = loweredTT->getNumElements(); i < e; ++i) {
            auto loweredTTEltType =
               PILType::getPrimitiveAddressType(loweredTT.getElementType(i));
            if (!loweredTTEltType.isLoweringOf(context, Mod,
                                               formalTT.getElementType(i)))
               return false;
         }
         return true;
      }
   }

   // Dynamic self has the same lowering as its contained type.
   if (auto dynamicSelf = dyn_cast<DynamicSelfType>(formalType))
      formalType = dynamicSelf.getSelfType();

   // Other types are preserved through lowering.
   return loweredType.getAstType() == formalType;
}
