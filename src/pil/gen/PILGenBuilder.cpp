//===--- PILGenBuilder.cpp ------------------------------------------------===//
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

#include "polarphp/pil/gen/PILGenBuilder.h"
#include "polarphp/pil/gen/ArgumentSource.h"
#include "polarphp/pil/gen/RValue.h"
#include "polarphp/pil/gen/PILGenFunction.h"
#include "polarphp/pil/gen/Scope.h"
#include "polarphp/pil/gen/SwitchEnumBuilder.h"
#include "polarphp/ast/GenericSignature.h"
#include "polarphp/ast/SubstitutionMap.h"

using namespace polar;
using namespace lowering;

//===----------------------------------------------------------------------===//
//                              Utility Methods
//===----------------------------------------------------------------------===//

PILGenModule &PILGenBuilder::getPILGenModule() const { return SGF.SGM; }

//===----------------------------------------------------------------------===//
//                                Constructors
//===----------------------------------------------------------------------===//

PILGenBuilder::PILGenBuilder(PILGenFunction &SGF)
   : PILBuilder(SGF.F), SGF(SGF) {}

PILGenBuilder::PILGenBuilder(PILGenFunction &SGF, PILBasicBlock *insertBB,
                             SmallVectorImpl<PILInstruction *> *insertedInsts)
   : PILBuilder(insertBB, insertedInsts), SGF(SGF) {}

PILGenBuilder::PILGenBuilder(PILGenFunction &SGF, PILBasicBlock *insertBB,
                             PILBasicBlock::iterator insertInst)
   : PILBuilder(insertBB, insertInst), SGF(SGF) {}

//===----------------------------------------------------------------------===//
//                             Managed Value APIs
//===----------------------------------------------------------------------===//

ManagedValue PILGenBuilder::createPartialApply(PILLocation loc, PILValue fn,
                                               SubstitutionMap subs,
                                               ArrayRef<ManagedValue> args,
                                               ParameterConvention calleeConvention) {
   llvm::SmallVector<PILValue, 8> values;
   llvm::transform(args, std::back_inserter(values),
                   [&](ManagedValue mv) -> PILValue {
                      return mv.forward(getPILGenFunction());
                   });
   PILValue result =
      createPartialApply(loc, fn, subs, values, calleeConvention);
   // Partial apply instructions create a box, so we need to put on a cleanup.
   return getPILGenFunction().emitManagedRValueWithCleanup(result);
}

ManagedValue
PILGenBuilder::createConvertFunction(PILLocation loc, ManagedValue fn,
                                     PILType resultTy,
                                     bool withoutActuallyEscaping) {
   CleanupCloner cloner(*this, fn);
   PILValue result = createConvertFunction(loc, fn.forward(getPILGenFunction()),
                                           resultTy, withoutActuallyEscaping);
   return cloner.clone(result);
}

ManagedValue PILGenBuilder::createConvertEscapeToNoEscape(
   PILLocation loc, ManagedValue fn, PILType resultTy) {

   auto fnType = fn.getType().castTo<PILFunctionType>();
   auto resultFnType = resultTy.castTo<PILFunctionType>();

   // Escaping to noescape conversion.
   assert(resultFnType->getRepresentation() ==
          PILFunctionTypeRepresentation::Thick &&
          fnType->getRepresentation() ==
          PILFunctionTypeRepresentation::Thick &&
          !fnType->isNoEscape() && resultFnType->isNoEscape() &&
          "Expect a escaping to noescape conversion");
   (void)fnType;
   (void)resultFnType;
   PILValue fnValue = fn.getValue();
   PILValue result =
      createConvertEscapeToNoEscape(loc, fnValue, resultTy, false);
   return ManagedValue::forTrivialObjectRValue(result);
}

ManagedValue PILGenBuilder::createInitExistentialValue(
   PILLocation loc, PILType existentialType, CanType formalConcreteType,
   ManagedValue concrete, ArrayRef<InterfaceConformanceRef> conformances) {
   // *NOTE* we purposely do not use a cleanup cloner here. The reason why is no
   // matter whether we have a trivial or non-trivial input,
   // init_existential_value returns a +1 value (the COW box).
   PILValue v =
      createInitExistentialValue(loc, existentialType, formalConcreteType,
                                 concrete.forward(SGF), conformances);
   return SGF.emitManagedRValueWithCleanup(v);
}

ManagedValue PILGenBuilder::createInitExistentialRef(
   PILLocation Loc, PILType ExistentialType, CanType FormalConcreteType,
   ManagedValue Concrete, ArrayRef<InterfaceConformanceRef> Conformances) {
   CleanupCloner Cloner(*this, Concrete);
   InitExistentialRefInst *IERI =
      createInitExistentialRef(Loc, ExistentialType, FormalConcreteType,
                               Concrete.forward(SGF), Conformances);
   return Cloner.clone(IERI);
}

ManagedValue PILGenBuilder::createStructExtract(PILLocation loc,
                                                ManagedValue base,
                                                VarDecl *decl) {
   ManagedValue borrowedBase = base.formalAccessBorrow(SGF, loc);
   PILValue extract = createStructExtract(loc, borrowedBase.getValue(), decl);
   return ManagedValue::forUnmanaged(extract);
}

ManagedValue PILGenBuilder::createRefElementAddr(PILLocation loc,
                                                 ManagedValue operand,
                                                 VarDecl *field,
                                                 PILType resultTy) {
   operand = operand.formalAccessBorrow(SGF, loc);
   PILValue result = createRefElementAddr(loc, operand.getValue(), field);
   return ManagedValue::forUnmanaged(result);
}

ManagedValue PILGenBuilder::createCopyValue(PILLocation loc,
                                            ManagedValue originalValue) {
   auto &lowering = SGF.getTypeLowering(originalValue.getType());
   return createCopyValue(loc, originalValue, lowering);
}

ManagedValue PILGenBuilder::createCopyValue(PILLocation loc,
                                            ManagedValue originalValue,
                                            const TypeLowering &lowering) {
   if (lowering.isTrivial())
      return originalValue;

   PILType ty = originalValue.getType();
   assert(!ty.isAddress() && "Can not perform a copy value of an address typed "
                             "value");

   if (ty.isObject() &&
       originalValue.getOwnershipKind() == ValueOwnershipKind::None) {
      return originalValue;
   }

   PILValue result =
      lowering.emitCopyValue(*this, loc, originalValue.getValue());
   return SGF.emitManagedRValueWithCleanup(result, lowering);
}

#define SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...)                      \
  ManagedValue PILGenBuilder::createStrongCopy##Name##Value(                   \
      PILLocation loc, ManagedValue originalValue) {                           \
    auto ty = originalValue.getType().castTo<Name##StorageType>();             \
    assert(ty->isLoadable(ResilienceExpansion::Maximal));                      \
    (void)ty;                                                                  \
    PILValue result =                                                          \
        createStrongCopy##Name##Value(loc, originalValue.getValue());          \
    return SGF.emitManagedRValueWithCleanup(result);                           \
  }
#define ALWAYS_LOADABLE_CHECKED_REF_STORAGE(Name, ...)                         \
  ManagedValue PILGenBuilder::createStrongCopy##Name##Value(                   \
      PILLocation loc, ManagedValue originalValue) {                           \
    PILValue result =                                                          \
        createStrongCopy##Name##Value(loc, originalValue.getValue());          \
    return SGF.emitManagedRValueWithCleanup(result);                           \
  }
#define UNCHECKED_REF_STORAGE(Name, ...)                                       \
  ManagedValue PILGenBuilder::createStrongCopy##Name##Value(                   \
      PILLocation loc, ManagedValue originalValue) {                           \
    /* *NOTE* The reason why this is unsafe is that we are converting and */   \
    /* unconditionally retaining, rather than before converting from */        \
    /* type->ref checking that our value is not yet uninitialized. */          \
    PILValue result =                                                          \
        createStrongCopy##Name##Value(loc, originalValue.getValue());          \
    return SGF.emitManagedRValueWithCleanup(result);                           \
  }
#include "polarphp/ast/ReferenceStorageDef.h"

ManagedValue PILGenBuilder::createOwnedPhiArgument(PILType type) {
   PILPhiArgument *arg =
      getInsertionBB()->createPhiArgument(type, ValueOwnershipKind::Owned);
   return SGF.emitManagedRValueWithCleanup(arg);
}

ManagedValue PILGenBuilder::createGuaranteedPhiArgument(PILType type) {
   PILPhiArgument *arg =
      getInsertionBB()->createPhiArgument(type, ValueOwnershipKind::Guaranteed);
   return SGF.emitManagedBorrowedArgumentWithCleanup(arg);
}

ManagedValue PILGenBuilder::createAllocRef(
   PILLocation loc, PILType refType, bool objc, bool canAllocOnStack,
   ArrayRef<PILType> inputElementTypes,
   ArrayRef<ManagedValue> inputElementCountOperands) {
   llvm::SmallVector<PILType, 8> elementTypes(inputElementTypes.begin(),
                                              inputElementTypes.end());
   llvm::SmallVector<PILValue, 8> elementCountOperands;
   llvm::transform(inputElementCountOperands,
                   std::back_inserter(elementCountOperands),
                   [](ManagedValue mv) -> PILValue { return mv.getValue(); });

   AllocRefInst *i = createAllocRef(loc, refType, objc, canAllocOnStack,
                                    elementTypes, elementCountOperands);
   return SGF.emitManagedRValueWithCleanup(i);
}

ManagedValue PILGenBuilder::createAllocRefDynamic(
   PILLocation loc, ManagedValue operand, PILType refType, bool objc,
   ArrayRef<PILType> inputElementTypes,
   ArrayRef<ManagedValue> inputElementCountOperands) {
   llvm::SmallVector<PILType, 8> elementTypes(inputElementTypes.begin(),
                                              inputElementTypes.end());
   llvm::SmallVector<PILValue, 8> elementCountOperands;
   llvm::transform(inputElementCountOperands,
                   std::back_inserter(elementCountOperands),
                   [](ManagedValue mv) -> PILValue { return mv.getValue(); });

   AllocRefDynamicInst *i =
      createAllocRefDynamic(loc, operand.getValue(), refType, objc,
                            elementTypes, elementCountOperands);
   return SGF.emitManagedRValueWithCleanup(i);
}

ManagedValue PILGenBuilder::createTupleExtract(PILLocation loc,
                                               ManagedValue base,
                                               unsigned index, PILType type) {
   ManagedValue borrowedBase = SGF.emitManagedBeginBorrow(loc, base.getValue());
   PILValue extract =
      createTupleExtract(loc, borrowedBase.getValue(), index, type);
   return ManagedValue::forUnmanaged(extract);
}

ManagedValue PILGenBuilder::createTupleExtract(PILLocation loc,
                                               ManagedValue value,
                                               unsigned index) {
   PILType type = value.getType().getTupleElementType(index);
   return createTupleExtract(loc, value, index, type);
}

ManagedValue PILGenBuilder::createLoadBorrow(PILLocation loc,
                                             ManagedValue base) {
   if (SGF.getTypeLowering(base.getType()).isTrivial()) {
      auto *i = createLoad(loc, base.getValue(), LoadOwnershipQualifier::Trivial);
      return ManagedValue::forUnmanaged(i);
   }

   auto *i = createLoadBorrow(loc, base.getValue());
   return SGF.emitManagedBorrowedRValueWithCleanup(base.getValue(), i);
}

ManagedValue PILGenBuilder::createFormalAccessLoadBorrow(PILLocation loc,
                                                         ManagedValue base) {
   if (SGF.getTypeLowering(base.getType()).isTrivial()) {
      auto *i = createLoad(loc, base.getValue(), LoadOwnershipQualifier::Trivial);
      return ManagedValue::forUnmanaged(i);
   }

   PILValue baseValue = base.getValue();
   auto *i = createLoadBorrow(loc, baseValue);
   return SGF.emitFormalEvaluationManagedBorrowedRValueWithCleanup(loc,
                                                                   baseValue, i);
}

ManagedValue
PILGenBuilder::createFormalAccessCopyValue(PILLocation loc,
                                           ManagedValue originalValue) {
   PILType ty = originalValue.getType();
   const auto &lowering = SGF.getTypeLowering(ty);
   if (lowering.isTrivial())
      return originalValue;

   assert(!lowering.isAddressOnly() && "cannot perform a copy value of an "
                                       "address only type");

   if (ty.isObject() &&
       originalValue.getOwnershipKind() == ValueOwnershipKind::None) {
      return originalValue;
   }

   PILValue result =
      lowering.emitCopyValue(*this, loc, originalValue.getValue());
   return SGF.emitFormalAccessManagedRValueWithCleanup(loc, result);
}

ManagedValue PILGenBuilder::createFormalAccessCopyAddr(
   PILLocation loc, ManagedValue originalAddr, PILValue newAddr,
   IsTake_t isTake, IsInitialization_t isInit) {
   createCopyAddr(loc, originalAddr.getValue(), newAddr, isTake, isInit);
   return SGF.emitFormalAccessManagedBufferWithCleanup(loc, newAddr);
}

ManagedValue
PILGenBuilder::bufferForExpr(PILLocation loc, PILType ty,
                             const TypeLowering &lowering, SGFContext context,
                             llvm::function_ref<void(PILValue)> rvalueEmitter) {
   // If we have a single-buffer "emit into" initialization, use that for the
   // result.
   PILValue address = context.getAddressForInPlaceInitialization(SGF, loc);

   // If we couldn't emit into the Initialization, emit into a temporary
   // allocation.
   if (!address) {
      address = SGF.emitTemporaryAllocation(loc, ty.getObjectType());
   }

   rvalueEmitter(address);

   // If we have a single-buffer "emit into" initialization, use that for the
   // result.
   if (context.finishInPlaceInitialization(SGF)) {
      return ManagedValue::forInContext();
   }

   // Add a cleanup for the temporary we allocated.
   if (lowering.isTrivial())
      return ManagedValue::forUnmanaged(address);

   return SGF.emitManagedBufferWithCleanup(address);
}

ManagedValue PILGenBuilder::formalAccessBufferForExpr(
   PILLocation loc, PILType ty, const TypeLowering &lowering,
   SGFContext context, llvm::function_ref<void(PILValue)> rvalueEmitter) {
   // If we have a single-buffer "emit into" initialization, use that for the
   // result.
   PILValue address = context.getAddressForInPlaceInitialization(SGF, loc);

   // If we couldn't emit into the Initialization, emit into a temporary
   // allocation.
   if (!address) {
      address = SGF.emitTemporaryAllocation(loc, ty.getObjectType());
   }

   rvalueEmitter(address);

   // If we have a single-buffer "emit into" initialization, use that for the
   // result.
   if (context.finishInPlaceInitialization(SGF)) {
      return ManagedValue::forInContext();
   }

   // Add a cleanup for the temporary we allocated.
   if (lowering.isTrivial())
      return ManagedValue::forUnmanaged(address);

   return SGF.emitFormalAccessManagedBufferWithCleanup(loc, address);
}

ManagedValue PILGenBuilder::createUncheckedEnumData(PILLocation loc,
                                                    ManagedValue operand,
                                                    EnumElementDecl *element) {
   CleanupCloner cloner(*this, operand);
   PILValue result = createUncheckedEnumData(loc, operand.forward(SGF), element);
   return cloner.clone(result);
}

ManagedValue PILGenBuilder::createUncheckedTakeEnumDataAddr(
   PILLocation loc, ManagedValue operand, EnumElementDecl *element,
   PILType ty) {
   CleanupCloner cloner(*this, operand);
   PILValue result =
      createUncheckedTakeEnumDataAddr(loc, operand.forward(SGF), element);
   return cloner.clone(result);
}

ManagedValue PILGenBuilder::createLoadTake(PILLocation loc, ManagedValue v) {
   auto &lowering = SGF.getTypeLowering(v.getType());
   return createLoadTake(loc, v, lowering);
}

ManagedValue PILGenBuilder::createLoadTake(PILLocation loc, ManagedValue v,
                                           const TypeLowering &lowering) {
   assert(lowering.getLoweredType().getAddressType() == v.getType());
   PILValue result =
      lowering.emitLoadOfCopy(*this, loc, v.forward(SGF), IsTake);
   if (lowering.isTrivial())
      return ManagedValue::forUnmanaged(result);
   assert(!lowering.isAddressOnly() && "cannot retain an unloadable type");
   return SGF.emitManagedRValueWithCleanup(result, lowering);
}

ManagedValue PILGenBuilder::createLoadCopy(PILLocation loc, ManagedValue v) {
   auto &lowering = SGF.getTypeLowering(v.getType());
   return createLoadCopy(loc, v, lowering);
}

ManagedValue PILGenBuilder::createLoadCopy(PILLocation loc, ManagedValue v,
                                           const TypeLowering &lowering) {
   assert(lowering.getLoweredType().getAddressType() == v.getType());
   PILValue result =
      lowering.emitLoadOfCopy(*this, loc, v.getValue(), IsNotTake);
   if (lowering.isTrivial())
      return ManagedValue::forUnmanaged(result);
   assert((!lowering.isAddressOnly()
           || !SGF.silConv.useLoweredAddresses()) &&
          "cannot retain an unloadable type");
   return SGF.emitManagedRValueWithCleanup(result, lowering);
}

static ManagedValue createInputFunctionArgument(PILGenBuilder &B, PILType type,
                                                PILLocation loc,
                                                ValueDecl *decl = nullptr) {
   auto &SGF = B.getPILGenFunction();
   PILFunction &F = B.getFunction();
   assert((F.isBare() || decl) &&
          "Function arguments of non-bare functions must have a decl");
   auto *arg = F.begin()->createFunctionArgument(type, decl);
   switch (arg->getArgumentConvention()) {
      case PILArgumentConvention::Indirect_In_Guaranteed:
      case PILArgumentConvention::Direct_Guaranteed:
         // Guaranteed parameters are passed at +0.
         return ManagedValue::forUnmanaged(arg);
      case PILArgumentConvention::Direct_Unowned:
         // Unowned parameters are only guaranteed at the instant of the call, so we
         // must retain them even if we're in a context that can accept a +0 value.
         return ManagedValue::forUnmanaged(arg).copy(SGF, loc);

      case PILArgumentConvention::Direct_Owned:
         return SGF.emitManagedRValueWithCleanup(arg);

      case PILArgumentConvention::Indirect_In:
         if (SGF.silConv.useLoweredAddresses())
            return SGF.emitManagedBufferWithCleanup(arg);
         return SGF.emitManagedRValueWithCleanup(arg);

      case PILArgumentConvention::Indirect_Inout:
      case PILArgumentConvention::Indirect_InoutAliasable:
         // An inout parameter is +0 and guaranteed, but represents an lvalue.
         return ManagedValue::forLValue(arg);
      case PILArgumentConvention::Indirect_In_Constant:
         llvm_unreachable("Convention not produced by PILGen");
      case PILArgumentConvention::Direct_Deallocating:
      case PILArgumentConvention::Indirect_Out:
         llvm_unreachable("unsupported convention for API");
   }
   llvm_unreachable("bad parameter convention");
}

ManagedValue PILGenBuilder::createInputFunctionArgument(PILType type,
                                                        ValueDecl *decl) {
   return ::createInputFunctionArgument(*this, type, PILLocation(decl), decl);
}

ManagedValue
PILGenBuilder::createInputFunctionArgument(PILType type,
                                           Optional<PILLocation> inputLoc) {
   assert(inputLoc.hasValue() && "This optional is only for overload resolution "
                                 "purposes! Do not pass in None here!");
   return ::createInputFunctionArgument(*this, type, *inputLoc);
}

ManagedValue
PILGenBuilder::createMarkUninitialized(ValueDecl *decl, ManagedValue operand,
                                       MarkUninitializedInst::Kind muKind) {
   // We either have an owned or trivial value.
   PILValue value = createMarkUninitialized(decl, operand.forward(SGF), muKind);
   assert(value->getType().isObject() && "Expected only objects here");

   // If we have a trivial value, just return without a cleanup.
   if (operand.getOwnershipKind() != ValueOwnershipKind::Owned) {
      return ManagedValue::forUnmanaged(value);
   }

   // Otherwise, recreate the cleanup.
   return SGF.emitManagedRValueWithCleanup(value);
}

ManagedValue PILGenBuilder::createEnum(PILLocation loc, ManagedValue payload,
                                       EnumElementDecl *decl, PILType type) {
   PILValue result = createEnum(loc, payload.forward(SGF), decl, type);
   if (result.getOwnershipKind() != ValueOwnershipKind::Owned)
      return ManagedValue::forUnmanaged(result);
   return SGF.emitManagedRValueWithCleanup(result);
}

ManagedValue PILGenBuilder::createUnconditionalCheckedCastValue(
   PILLocation loc, ManagedValue op, CanType srcFormalTy,
   PILType destLoweredTy, CanType destFormalTy) {
   PILValue result =
      createUnconditionalCheckedCastValue(loc, op.forward(SGF),
                                          srcFormalTy, destLoweredTy,
                                          destFormalTy);
   return SGF.emitManagedRValueWithCleanup(result);
}

ManagedValue PILGenBuilder::createUnconditionalCheckedCast(
   PILLocation loc, ManagedValue op,
   PILType destLoweredTy, CanType destFormalTy) {
   PILValue result =
      createUnconditionalCheckedCast(loc, op.forward(SGF),
                                     destLoweredTy, destFormalTy);
   return SGF.emitManagedRValueWithCleanup(result);
}

void PILGenBuilder::createCheckedCastBranch(PILLocation loc, bool isExact,
                                            ManagedValue op,
                                            PILType destLoweredTy,
                                            CanType destFormalTy,
                                            PILBasicBlock *trueBlock,
                                            PILBasicBlock *falseBlock,
                                            ProfileCounter Target1Count,
                                            ProfileCounter Target2Count) {
   createCheckedCastBranch(loc, isExact, op.forward(SGF),
                           destLoweredTy, destFormalTy,
                           trueBlock, falseBlock,
                           Target1Count, Target2Count);
}

void PILGenBuilder::createCheckedCastValueBranch(PILLocation loc,
                                                 ManagedValue op,
                                                 CanType srcFormalTy,
                                                 PILType destLoweredTy,
                                                 CanType destFormalTy,
                                                 PILBasicBlock *trueBlock,
                                                 PILBasicBlock *falseBlock) {
   createCheckedCastValueBranch(loc, op.forward(SGF), srcFormalTy,
                                destLoweredTy, destFormalTy,
                                trueBlock, falseBlock);
}

ManagedValue PILGenBuilder::createUpcast(PILLocation loc, ManagedValue original,
                                         PILType type) {
   CleanupCloner cloner(*this, original);
   PILValue convertedValue = createUpcast(loc, original.forward(SGF), type);
   return cloner.clone(convertedValue);
}

ManagedValue PILGenBuilder::createOptionalSome(PILLocation loc,
                                               ManagedValue arg) {
   CleanupCloner cloner(*this, arg);
   auto &argTL = SGF.getTypeLowering(arg.getType());
   PILType optionalType = PILType::getOptionalType(arg.getType());
   if (argTL.isLoadable() || !SGF.silConv.useLoweredAddresses()) {
      PILValue someValue =
         createOptionalSome(loc, arg.forward(SGF), optionalType);
      return cloner.clone(someValue);
   }

   PILValue tempResult = SGF.emitTemporaryAllocation(loc, optionalType);
   RValue rvalue(SGF, loc, arg.getType().getAstType(), arg);
   ArgumentSource argValue(loc, std::move(rvalue));
   SGF.emitInjectOptionalValueInto(
      loc, std::move(argValue), tempResult,
      SGF.getTypeLowering(tempResult->getType()));
   return ManagedValue::forUnmanaged(tempResult);
}

ManagedValue PILGenBuilder::createManagedOptionalNone(PILLocation loc,
                                                      PILType type) {
   if (!type.isAddressOnly(getFunction()) ||
       !SGF.silConv.useLoweredAddresses()) {
      PILValue noneValue = createOptionalNone(loc, type);
      return ManagedValue::forUnmanaged(noneValue);
   }

   PILValue tempResult = SGF.emitTemporaryAllocation(loc, type);
   SGF.emitInjectOptionalNothingInto(loc, tempResult,
                                     SGF.getTypeLowering(type));
   return ManagedValue::forUnmanaged(tempResult);
}

ManagedValue PILGenBuilder::createManagedFunctionRef(PILLocation loc,
                                                     PILFunction *f) {
   return ManagedValue::forUnmanaged(createFunctionRefFor(loc, f));
}

ManagedValue PILGenBuilder::createTupleElementAddr(PILLocation Loc,
                                                   ManagedValue Base,
                                                   unsigned Index,
                                                   PILType Type) {
   PILValue TupleEltAddr =
      createTupleElementAddr(Loc, Base.getValue(), Index, Type);
   return ManagedValue::forUnmanaged(TupleEltAddr);
}

ManagedValue PILGenBuilder::createTupleElementAddr(PILLocation Loc,
                                                   ManagedValue Value,
                                                   unsigned Index) {
   PILType Type = Value.getType().getTupleElementType(Index);
   return createTupleElementAddr(Loc, Value, Index, Type);
}

ManagedValue PILGenBuilder::createUncheckedRefCast(PILLocation loc,
                                                   ManagedValue value,
                                                   PILType type) {
   CleanupCloner cloner(*this, value);
   PILValue cast = createUncheckedRefCast(loc, value.forward(SGF), type);
   return cloner.clone(cast);
}

ManagedValue PILGenBuilder::createUncheckedBitCast(PILLocation loc,
                                                   ManagedValue value,
                                                   PILType type) {
   CleanupCloner cloner(*this, value);
   PILValue cast = createUncheckedBitCast(loc, value.getValue(), type);

   // Currently createUncheckedBitCast only produces these
   // instructions. We assert here to make sure if this changes, this code is
   // updated.
   assert((isa<UncheckedTrivialBitCastInst>(cast) ||
           isa<UncheckedRefCastInst>(cast) ||
           isa<UncheckedBitwiseCastInst>(cast)) &&
          "PILGenBuilder is out of sync with PILBuilder.");

   // If we have a trivial inst, just return early.
   if (isa<UncheckedTrivialBitCastInst>(cast))
      return ManagedValue::forUnmanaged(cast);

   // If we perform an unchecked bitwise case, then we are producing a new RC
   // identity implying that we need a copy of the casted value to be returned so
   // that the inputs/outputs of the case have separate ownership.
   if (isa<UncheckedBitwiseCastInst>(cast)) {
      return ManagedValue::forUnmanaged(cast).copy(SGF, loc);
   }

   // Otherwise, we forward the cleanup of the input value and place the cleanup
   // on the cast value since unchecked_ref_cast is "forwarding".
   value.forward(SGF);
   return cloner.clone(cast);
}

ManagedValue PILGenBuilder::createOpenExistentialRef(PILLocation loc,
                                                     ManagedValue original,
                                                     PILType type) {
   CleanupCloner cloner(*this, original);
   PILValue openedExistential =
      createOpenExistentialRef(loc, original.forward(SGF), type);
   return cloner.clone(openedExistential);
}

ManagedValue PILGenBuilder::createOpenExistentialValue(PILLocation loc,
                                                       ManagedValue original,
                                                       PILType type) {
   ManagedValue borrowedExistential = original.formalAccessBorrow(SGF, loc);
   PILValue openedExistential =
      createOpenExistentialValue(loc, borrowedExistential.getValue(), type);
   return ManagedValue::forUnmanaged(openedExistential);
}

ManagedValue PILGenBuilder::createOpenExistentialBoxValue(PILLocation loc,
                                                          ManagedValue original,
                                                          PILType type) {
   ManagedValue borrowedExistential = original.formalAccessBorrow(SGF, loc);
   PILValue openedExistential =
      createOpenExistentialBoxValue(loc, borrowedExistential.getValue(), type);
   return ManagedValue::forUnmanaged(openedExistential);
}

ManagedValue PILGenBuilder::createOpenExistentialMetatype(PILLocation loc,
                                                          ManagedValue value,
                                                          PILType openedType) {
   PILValue result = PILGenBuilder::createOpenExistentialMetatype(
      loc, value.getValue(), openedType);
   return ManagedValue::forTrivialRValue(result);
}

ManagedValue PILGenBuilder::createStore(PILLocation loc, ManagedValue value,
                                        PILValue address,
                                        StoreOwnershipQualifier qualifier) {
   CleanupCloner cloner(*this, value);
   if (value.getType().isTrivial(SGF.F) ||
       value.getOwnershipKind() == ValueOwnershipKind::None)
      qualifier = StoreOwnershipQualifier::Trivial;
   createStore(loc, value.forward(SGF), address, qualifier);
   return cloner.clone(address);
}

ManagedValue PILGenBuilder::createSuperMethod(PILLocation loc,
                                              ManagedValue operand,
                                              PILDeclRef member,
                                              PILType methodTy) {
   PILValue v = createSuperMethod(loc, operand.getValue(), member, methodTy);
   return ManagedValue::forUnmanaged(v);
}

ManagedValue PILGenBuilder::createObjCSuperMethod(PILLocation loc,
                                                  ManagedValue operand,
                                                  PILDeclRef member,
                                                  PILType methodTy) {
   PILValue v = createObjCSuperMethod(loc, operand.getValue(), member, methodTy);
   return ManagedValue::forUnmanaged(v);
}

ManagedValue PILGenBuilder::
createValueMetatype(PILLocation loc, PILType metatype,
                    ManagedValue base) {
   PILValue v = createValueMetatype(loc, metatype, base.getValue());
   return ManagedValue::forUnmanaged(v);
}

void PILGenBuilder::createStoreBorrow(PILLocation loc, ManagedValue value,
                                      PILValue address) {
   assert(value.getOwnershipKind() == ValueOwnershipKind::Guaranteed);
   createStoreBorrow(loc, value.getValue(), address);
}

void PILGenBuilder::createStoreBorrowOrTrivial(PILLocation loc,
                                               ManagedValue value,
                                               PILValue address) {
   if (value.getOwnershipKind() == ValueOwnershipKind::None) {
      createStore(loc, value, address, StoreOwnershipQualifier::Trivial);
      return;
   }

   createStoreBorrow(loc, value, address);
}

ManagedValue PILGenBuilder::createBridgeObjectToRef(PILLocation loc,
                                                    ManagedValue mv,
                                                    PILType destType) {
   CleanupCloner cloner(*this, mv);
   PILValue result = createBridgeObjectToRef(loc, mv.forward(SGF), destType);
   return cloner.clone(result);
}

ManagedValue PILGenBuilder::createRefToBridgeObject(PILLocation loc,
                                                    ManagedValue mv,
                                                    PILValue bits) {
   CleanupCloner cloner(*this, mv);
   PILValue result = createRefToBridgeObject(loc, mv.forward(SGF), bits);
   return cloner.clone(result);
}

ManagedValue PILGenBuilder::createBlockToAnyObject(PILLocation loc,
                                                   ManagedValue v,
                                                   PILType destType) {
   assert(SGF.getAstContext().LangOpts.EnableObjCInterop);
   assert(destType.isAnyObject());
   assert(v.getType().is<PILFunctionType>());
   assert(v.getType().castTo<PILFunctionType>()->getRepresentation() ==
          PILFunctionTypeRepresentation::Block);

   // For now, we don't have a better instruction than this.
   return createUncheckedRefCast(loc, v, destType);
}

BranchInst *PILGenBuilder::createBranch(PILLocation loc,
                                        PILBasicBlock *targetBlock,
                                        ArrayRef<ManagedValue> args) {
   llvm::SmallVector<PILValue, 8> newArgs;
   llvm::transform(args, std::back_inserter(newArgs),
                   [&](ManagedValue mv) -> PILValue { return mv.forward(SGF); });
   return createBranch(loc, targetBlock, newArgs);
}

ReturnInst *PILGenBuilder::createReturn(PILLocation loc,
                                        ManagedValue returnValue) {
   return createReturn(loc, returnValue.forward(SGF));
}

ManagedValue PILGenBuilder::createTuple(PILLocation loc, PILType type,
                                        ArrayRef<ManagedValue> elements) {
   // Handle the empty tuple case.
   if (elements.empty()) {
      PILValue result = createTuple(loc, type, ArrayRef<PILValue>());
      return ManagedValue::forUnmanaged(result);
   }

   // We need to look for the first value without .none ownership and use that as
   // our cleanup cloner value.
   auto iter = find_if(elements, [&](ManagedValue mv) -> bool {
      return mv.getOwnershipKind() != ValueOwnershipKind::None;
   });

   llvm::SmallVector<PILValue, 8> forwardedValues;

   // If we have all .none values, then just create the tuple and return. No
   // cleanups need to be cloned.
   if (iter == elements.end()) {
      llvm::transform(elements, std::back_inserter(forwardedValues),
                      [&](ManagedValue mv) -> PILValue {
                         return mv.forward(getPILGenFunction());
                      });
      PILValue result = createTuple(loc, type, forwardedValues);
      return ManagedValue::forUnmanaged(result);
   }

   // Otherwise, we use that values cloner. This is taking advantage of
   // instructions that forward ownership requiring that all input values have
   // the same ownership if they are non-trivial.
   CleanupCloner cloner(*this, *iter);
   llvm::transform(elements, std::back_inserter(forwardedValues),
                   [&](ManagedValue mv) -> PILValue {
                      return mv.forward(getPILGenFunction());
                   });
   return cloner.clone(createTuple(loc, type, forwardedValues));
}

ManagedValue PILGenBuilder::createUncheckedAddrCast(PILLocation loc, ManagedValue op,
                                                    PILType resultTy) {
   CleanupCloner cloner(*this, op);
   PILValue cast = createUncheckedAddrCast(loc, op.forward(SGF), resultTy);
   return cloner.clone(cast);
}

ManagedValue PILGenBuilder::tryCreateUncheckedRefCast(PILLocation loc,
                                                      ManagedValue original,
                                                      PILType type) {
   CleanupCloner cloner(*this, original);
   PILValue result = tryCreateUncheckedRefCast(loc, original.getValue(), type);
   if (!result)
      return ManagedValue();
   original.forward(SGF);
   return cloner.clone(result);
}

ManagedValue PILGenBuilder::createUncheckedTrivialBitCast(PILLocation loc,
                                                          ManagedValue original,
                                                          PILType type) {
   PILValue result =
      SGF.B.createUncheckedTrivialBitCast(loc, original.getValue(), type);
   return ManagedValue::forUnmanaged(result);
}

void PILGenBuilder::emitDestructureValueOperation(
   PILLocation loc, ManagedValue value,
   llvm::function_ref<void(unsigned, ManagedValue)> func) {
   CleanupCloner cloner(*this, value);

   // NOTE: We can not directly use PILBuilder::emitDestructureValueOperation()
   // here since we need to create all of our cleanups before invoking \p
   // func. This is necessary since our func may want to emit conditional code
   // with an early exit, emitting unused cleanups from the current scope via the
   // function emitBranchAndCleanups(). If we have not yet created those
   // cleanups, we will introduce a leak along that path.
   SmallVector<ManagedValue, 8> destructuredValues;
   emitDestructureValueOperation(
      loc, value.forward(SGF), [&](unsigned index, PILValue subValue) {
         destructuredValues.push_back(cloner.clone(subValue));
      });
   for (auto p : llvm::enumerate(destructuredValues)) {
      func(p.index(), p.value());
   }
}

ManagedValue PILGenBuilder::createProjectBox(PILLocation loc, ManagedValue mv,
                                             unsigned index) {
   auto *pbi = createProjectBox(loc, mv.getValue(), index);
   return ManagedValue::forUnmanaged(pbi);
}
