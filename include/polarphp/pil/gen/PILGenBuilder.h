//===--- PILGenBuilder.h ----------------------------------------*- C++ -*-===//
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
///
/// \file
///
/// This file defines PILGenBuilder, a subclass of PILBuilder that provides APIs
/// that traffic in ManagedValue. The intention is that if one is using a
/// PILGenBuilder, the PILGenBuilder will handle preserving ownership invariants
/// (or assert upon failure) freeing the implementor of such concerns.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_GEN_PILGENBUILDER_H
#define POLARPHP_PIL_GEN_PILGENBUILDER_H

#include "polarphp/pil/gen/Cleanup.h"
#include "polarphp/pil/gen/JumpDest.h"
#include "polarphp/pil/gen/ManagedValue.h"
#include "polarphp/pil/gen/RValue.h"
#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/basic/ProfileCounter.h"

namespace polar::lowering {

class PILGenFunction;
class SGFContext;

/// A subclass of PILBuilder that wraps APIs to vend ManagedValues.
/// APIs only vend ManagedValues.
class PILGenBuilder : public PILBuilder {
   PILGenFunction &SGF;

public:
   PILGenBuilder(PILGenFunction &SGF);
   PILGenBuilder(PILGenFunction &SGF, PILBasicBlock *insertBB,
                 SmallVectorImpl<PILInstruction *> *insertedInsts = nullptr);
   PILGenBuilder(PILGenFunction &SGF, PILBasicBlock *insertBB,
                 PILBasicBlock::iterator insertInst);

   // Create a new builder, inheriting the given builder's context and debug
   // scope.
   PILGenBuilder(PILGenBuilder &builder, PILBasicBlock *insertBB)
      : PILBuilder(insertBB, builder.getCurrentDebugScope(),
                   builder.getBuilderContext()),
        SGF(builder.SGF) {}

   PILGenModule &getPILGenModule() const;
   PILGenFunction &getPILGenFunction() const { return SGF; }

   using PILBuilder::createInitExistentialValue;
   ManagedValue
   createInitExistentialValue(PILLocation loc, PILType existentialType,
                              CanType formalConcreteType, ManagedValue concrete,
                              ArrayRef<InterfaceConformanceRef> conformances);
   using PILBuilder::createInitExistentialRef;
   ManagedValue
   createInitExistentialRef(PILLocation loc, PILType existentialType,
                            CanType formalConcreteType, ManagedValue concrete,
                            ArrayRef<InterfaceConformanceRef> conformances);

   using PILBuilder::createPartialApply;
   ManagedValue createPartialApply(PILLocation loc, PILValue fn,
                                   SubstitutionMap subs,
                                   ArrayRef<ManagedValue> args,
                                   ParameterConvention calleeConvention);
   ManagedValue createPartialApply(PILLocation loc, ManagedValue fn,
                                   SubstitutionMap subs,
                                   ArrayRef<ManagedValue> args,
                                   ParameterConvention calleeConvention) {
      return createPartialApply(loc, fn.getValue(), subs, args,
                                calleeConvention);
   }

   using PILBuilder::createStructExtract;
   ManagedValue createStructExtract(PILLocation loc, ManagedValue base,
                                    VarDecl *decl);

   using PILBuilder::createRefElementAddr;
   ManagedValue createRefElementAddr(PILLocation loc, ManagedValue operand,
                                     VarDecl *field, PILType resultTy);

   using PILBuilder::createCopyValue;

   /// Emit a +1 copy on \p originalValue that lives until the end of the current
   /// lexical scope.
   ManagedValue createCopyValue(PILLocation loc, ManagedValue originalValue);

   /// Emit a +1 copy on \p originalValue that lives until the end of the current
   /// lexical scope.
   ///
   /// This reuses a passed in lowering.
   ManagedValue createCopyValue(PILLocation loc, ManagedValue originalValue,
                                const TypeLowering &lowering);

   /// Emit a +1 copy of \p originalValue into newAddr that lives until the end
   /// of the current Formal Evaluation Scope.
   ManagedValue createFormalAccessCopyAddr(PILLocation loc,
                                           ManagedValue originalAddr,
                                           PILValue newAddr, IsTake_t isTake,
                                           IsInitialization_t isInit);

   /// Emit a +1 copy of \p originalValue into newAddr that lives until the end
   /// Formal Evaluation Scope.
   ManagedValue createFormalAccessCopyValue(PILLocation loc,
                                            ManagedValue originalValue);

#define ALWAYS_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...)            \
  using PILBuilder::createStrongCopy##Name##Value;                             \
  ManagedValue createStrongCopy##Name##Value(PILLocation loc,                  \
                                             ManagedValue originalValue);
#define UNCHECKED_REF_STORAGE(Name, ...)                                       \
  using PILBuilder::createStrongCopy##Name##Value;                             \
  ManagedValue createStrongCopy##Name##Value(PILLocation loc,                  \
                                             ManagedValue originalValue);
#include "polarphp/ast/ReferenceStorageDef.h"

   ManagedValue createOwnedPhiArgument(PILType type);
   ManagedValue createGuaranteedPhiArgument(PILType type);

   using PILBuilder::createMarkUninitialized;
   ManagedValue createMarkUninitialized(ValueDecl *decl, ManagedValue operand,
                                        MarkUninitializedInst::Kind muKind);

   using PILBuilder::createAllocRef;
   ManagedValue createAllocRef(PILLocation loc, PILType refType, bool objc,
                               bool canAllocOnStack,
                               ArrayRef<PILType> elementTypes,
                               ArrayRef<ManagedValue> elementCountOperands);
   using PILBuilder::createAllocRefDynamic;
   ManagedValue
   createAllocRefDynamic(PILLocation loc, ManagedValue operand, PILType refType,
                         bool objc, ArrayRef<PILType> elementTypes,
                         ArrayRef<ManagedValue> elementCountOperands);

   using PILBuilder::createTuple;
   ManagedValue createTuple(PILLocation loc, PILType type,
                            ArrayRef<ManagedValue> elements);
   using PILBuilder::createTupleExtract;
   ManagedValue createTupleExtract(PILLocation loc, ManagedValue value,
                                   unsigned index, PILType type);
   ManagedValue createTupleExtract(PILLocation loc, ManagedValue value,
                                   unsigned index);
   using PILBuilder::createTupleElementAddr;
   ManagedValue createTupleElementAddr(PILLocation loc, ManagedValue addr,
                                       unsigned index, PILType type);
   ManagedValue createTupleElementAddr(PILLocation loc, ManagedValue addr,
                                       unsigned index);

   using PILBuilder::createLoadBorrow;
   ManagedValue createLoadBorrow(PILLocation loc, ManagedValue base);
   ManagedValue createFormalAccessLoadBorrow(PILLocation loc, ManagedValue base);

   using PILBuilder::createStoreBorrow;
   void createStoreBorrow(PILLocation loc, ManagedValue value, PILValue address);

   /// Create a store_borrow if we have a non-trivial value and a store [trivial]
   /// otherwise.
   void createStoreBorrowOrTrivial(PILLocation loc, ManagedValue value,
                                   PILValue address);

   /// Prepares a buffer to receive the result of an expression, either using the
   /// 'emit into' initialization buffer if available, or allocating a temporary
   /// allocation if not. After the buffer has been prepared, the rvalueEmitter
   /// closure will be called with the buffer ready for initialization. After the
   /// emitter has been called, the buffer will complete its initialization.
   ///
   /// \return an empty value if the buffer was taken from the context.
   ManagedValue bufferForExpr(PILLocation loc, PILType ty,
                              const TypeLowering &lowering, SGFContext context,
                              llvm::function_ref<void(PILValue)> rvalueEmitter);

   using PILBuilder::createUncheckedEnumData;
   ManagedValue createUncheckedEnumData(PILLocation loc, ManagedValue operand,
                                        EnumElementDecl *element);

   using PILBuilder::createUncheckedTakeEnumDataAddr;
   ManagedValue createUncheckedTakeEnumDataAddr(PILLocation loc, ManagedValue operand,
                                                EnumElementDecl *element, PILType ty);

   ManagedValue createLoadTake(PILLocation loc, ManagedValue addr);
   ManagedValue createLoadTake(PILLocation loc, ManagedValue addr,
                               const TypeLowering &lowering);
   ManagedValue createLoadCopy(PILLocation loc, ManagedValue addr);
   ManagedValue createLoadCopy(PILLocation loc, ManagedValue addr,
                               const TypeLowering &lowering);

   /// Create a PILArgument for an input parameter. Asserts if used to create a
   /// function argument for an out parameter.
   ManagedValue createInputFunctionArgument(PILType type, ValueDecl *decl);

   /// Create a PILArgument for an input parameter. Uses \p loc to create any
   /// copies necessary. Asserts if used to create a function argument for an out
   /// parameter.
   ///
   /// *NOTE* This API purposely used an Optional<PILLocation> to distinguish
   /// this API from the ValueDecl * API in C++. This is necessary since
   /// ValueDecl * can implicitly convert to PILLocation. The optional forces the
   /// user to be explicit that they want to use this API.
   ManagedValue createInputFunctionArgument(PILType type,
                                            Optional<PILLocation> loc);

   using PILBuilder::createEnum;
   ManagedValue createEnum(PILLocation loc, ManagedValue payload,
                           EnumElementDecl *decl, PILType type);

   ManagedValue createSemanticLoadBorrow(PILLocation loc, ManagedValue addr);

   ManagedValue
   formalAccessBufferForExpr(PILLocation loc, PILType ty,
                             const TypeLowering &lowering, SGFContext context,
                             llvm::function_ref<void(PILValue)> rvalueEmitter);

   using PILBuilder::createUnconditionalCheckedCastValue;
   ManagedValue
   createUnconditionalCheckedCastValue(PILLocation loc,
                                       ManagedValue op,
                                       CanType srcFormalTy,
                                       PILType destLoweredTy,
                                       CanType destFormalTy);
   using PILBuilder::createUnconditionalCheckedCast;
   ManagedValue createUnconditionalCheckedCast(PILLocation loc,
                                               ManagedValue op,
                                               PILType destLoweredTy,
                                               CanType destFormalTy);

   using PILBuilder::createCheckedCastBranch;
   void createCheckedCastBranch(PILLocation loc, bool isExact,
                                ManagedValue op,
                                PILType destLoweredTy,
                                CanType destFormalTy,
                                PILBasicBlock *trueBlock,
                                PILBasicBlock *falseBlock,
                                ProfileCounter Target1Count,
                                ProfileCounter Target2Count);

   using PILBuilder::createCheckedCastValueBranch;
   void createCheckedCastValueBranch(PILLocation loc,
                                     ManagedValue op,
                                     CanType srcFormalTy,
                                     PILType destLoweredTy,
                                     CanType destFormalTy,
                                     PILBasicBlock *trueBlock,
                                     PILBasicBlock *falseBlock);

   using PILBuilder::createUpcast;
   ManagedValue createUpcast(PILLocation loc, ManagedValue original,
                             PILType type);

   using PILBuilder::tryCreateUncheckedRefCast;
   ManagedValue tryCreateUncheckedRefCast(PILLocation loc,
                                          ManagedValue op,
                                          PILType type);

   using PILBuilder::createUncheckedTrivialBitCast;
   ManagedValue createUncheckedTrivialBitCast(PILLocation loc,
                                              ManagedValue original,
                                              PILType type);

   using PILBuilder::createUncheckedRefCast;
   ManagedValue createUncheckedRefCast(PILLocation loc, ManagedValue original,
                                       PILType type);

   using PILBuilder::createUncheckedAddrCast;
   ManagedValue createUncheckedAddrCast(PILLocation loc, ManagedValue op,
                                        PILType resultTy);

   using PILBuilder::createUncheckedBitCast;
   ManagedValue createUncheckedBitCast(PILLocation loc, ManagedValue original,
                                       PILType type);

   using PILBuilder::createOpenExistentialRef;
   ManagedValue createOpenExistentialRef(PILLocation loc, ManagedValue arg,
                                         PILType openedType);

   using PILBuilder::createOpenExistentialValue;
   ManagedValue createOpenExistentialValue(PILLocation loc,
                                           ManagedValue original, PILType type);

   using PILBuilder::createOpenExistentialBoxValue;
   ManagedValue createOpenExistentialBoxValue(PILLocation loc,
                                              ManagedValue original, PILType type);

   using PILBuilder::createOpenExistentialMetatype;
   ManagedValue createOpenExistentialMetatype(PILLocation loc,
                                              ManagedValue value,
                                              PILType openedType);

   /// Convert a @convention(block) value to AnyObject.
   ManagedValue createBlockToAnyObject(PILLocation loc, ManagedValue block,
                                       PILType type);

   using PILBuilder::createOptionalSome;
   ManagedValue createOptionalSome(PILLocation Loc, ManagedValue Arg);
   ManagedValue createManagedOptionalNone(PILLocation Loc, PILType Type);

   // TODO: Rename this to createFunctionRef once all calls to createFunctionRef
   // are removed.
   ManagedValue createManagedFunctionRef(PILLocation loc, PILFunction *f);

   using PILBuilder::createConvertFunction;
   ManagedValue createConvertFunction(PILLocation loc, ManagedValue fn,
                                      PILType resultTy,
                                      bool WithoutActuallyEscaping = false);

   using PILBuilder::createConvertEscapeToNoEscape;
   ManagedValue
   createConvertEscapeToNoEscape(PILLocation loc, ManagedValue fn,
                                 PILType resultTy);

   using PILBuilder::createStore;
   /// Forward \p value into \p address.
   ///
   /// This will forward value's cleanup (if it has one) into the equivalent
   /// cleanup on address. In practice this means if the value is non-trivial,
   /// the memory location will at end of scope have a destroy_addr applied to
   /// it.
   ManagedValue createStore(PILLocation loc, ManagedValue value,
                            PILValue address, StoreOwnershipQualifier qualifier);

   using PILBuilder::createSuperMethod;
   ManagedValue createSuperMethod(PILLocation loc, ManagedValue operand,
                                  PILDeclRef member, PILType methodTy);

   using PILBuilder::createObjCSuperMethod;
   ManagedValue createObjCSuperMethod(PILLocation loc, ManagedValue operand,
                                      PILDeclRef member, PILType methodTy);

   using PILBuilder::createValueMetatype;
   ManagedValue createValueMetatype(PILLocation loc, PILType metatype,
                                    ManagedValue base);

   using PILBuilder::createBridgeObjectToRef;
   ManagedValue createBridgeObjectToRef(PILLocation loc, ManagedValue mv,
                                        PILType destType);

   using PILBuilder::createRefToBridgeObject;
   ManagedValue createRefToBridgeObject(PILLocation loc, ManagedValue mv,
                                        PILValue bits);

   using PILBuilder::createBranch;
   BranchInst *createBranch(PILLocation Loc, PILBasicBlock *TargetBlock,
                            ArrayRef<ManagedValue> Args);

   using PILBuilder::createReturn;
   ReturnInst *createReturn(PILLocation Loc, ManagedValue ReturnValue);

   using PILBuilder::emitDestructureValueOperation;
   /// Perform either a tuple or struct destructure and then pass its components
   /// as managed value one by one with an index to the closure.
   void emitDestructureValueOperation(
      PILLocation loc, ManagedValue value,
      function_ref<void(unsigned, ManagedValue)> func);

   using PILBuilder::createProjectBox;
   ManagedValue createProjectBox(PILLocation loc, ManagedValue mv,
                                 unsigned index);
};

} // namespace polar::lowering

#endif // POLARPHP_PIL_GEN_PILGENBUILDER_H
