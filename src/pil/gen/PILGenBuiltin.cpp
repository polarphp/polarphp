//===--- PILGenBuiltin.cpp - PIL generation for builtin call sites  -------===//
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

#include "polarphp/pil/gen/SpecializedEmitter.h"
#include "polarphp/pil/gen/ArgumentSource.h"
#include "polarphp/pil/gen/Cleanup.h"
#include "polarphp/pil/gen/Initialization.h"
#include "polarphp/pil/gen/RValue.h"
#include "polarphp/pil/gen/PILGenFunction.h"
#include "polarphp/ast/BuiltinTypes.h"
#include "polarphp/ast/DiagnosticsPIL.h"
#include "polarphp/ast/FileUnit.h"
#include "polarphp/ast/GenericEnvironment.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/ReferenceCounting.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILUndef.h"

using namespace polar;
using namespace lowering;

/// Break down an expression that's the formal argument expression to
/// a builtin function, returning its individualized arguments.
///
/// Because these are builtin operations, we can make some structural
/// assumptions about the expression used to call them.
static Optional<SmallVector<Expr*, 2>>
decomposeArguments(PILGenFunction &SGF,
                   PILLocation loc,
                   PreparedArguments &&args,
                   unsigned expectedCount) {
   SmallVector<Expr*, 2> result;
   auto sources = std::move(args).getSources();

   if (sources.size() == expectedCount) {
      for (auto &&source : sources)
         result.push_back(std::move(source).asKnownExpr());
      return result;
   }

   SGF.SGM.diagnose(loc, diag::invalid_sil_builtin,
                    "argument to builtin should be a literal tuple");

   return None;
}

static ManagedValue emitBuiltinRetain(PILGenFunction &SGF,
                                      PILLocation loc,
                                      SubstitutionMap substitutions,
                                      ArrayRef<ManagedValue> args,
                                      SGFContext C) {
   // The value was produced at +1; we can produce an unbalanced retain simply by
   // disabling the cleanup. But this would violate ownership semantics. Instead,
   // we must allow for the cleanup and emit a new unmanaged retain value.
   SGF.B.createUnmanagedRetainValue(loc, args[0].getValue(),
                                    SGF.B.getDefaultAtomicity());
   return ManagedValue::forUnmanaged(SGF.emitEmptyTuple(loc));
}

static ManagedValue emitBuiltinRelease(PILGenFunction &SGF,
                                       PILLocation loc,
                                       SubstitutionMap substitutions,
                                       ArrayRef<ManagedValue> args,
                                       SGFContext C) {
   // The value was produced at +1, so to produce an unbalanced
   // release we need to leave the cleanup intact and then do a *second*
   // release.
   SGF.B.createUnmanagedReleaseValue(loc, args[0].getValue(),
                                     SGF.B.getDefaultAtomicity());
   return ManagedValue::forUnmanaged(SGF.emitEmptyTuple(loc));
}

static ManagedValue emitBuiltinAutorelease(PILGenFunction &SGF,
                                           PILLocation loc,
                                           SubstitutionMap substitutions,
                                           ArrayRef<ManagedValue> args,
                                           SGFContext C) {
   SGF.B.createUnmanagedAutoreleaseValue(loc, args[0].getValue(),
                                         SGF.B.getDefaultAtomicity());
   return ManagedValue::forUnmanaged(SGF.emitEmptyTuple(loc));
}

/// Specialized emitter for Builtin.load and Builtin.take.
static ManagedValue emitBuiltinLoadOrTake(PILGenFunction &SGF,
                                          PILLocation loc,
                                          SubstitutionMap substitutions,
                                          ArrayRef<ManagedValue> args,
                                          SGFContext C,
                                          IsTake_t isTake,
                                          bool isStrict,
                                          bool isInvariant) {
   assert(substitutions.getReplacementTypes().size() == 1 &&
          "load should have single substitution");
   assert(args.size() == 1 && "load should have a single argument");

   // The substitution gives the type of the load.  This is always a
   // first-class type; there is no way to e.g. produce a @weak load
   // with this builtin.
   auto &rvalueTL = SGF.getTypeLowering(substitutions.getReplacementTypes()[0]);
   PILType loadedType = rvalueTL.getLoweredType();

   // Convert the pointer argument to a PIL address.
   PILValue addr = SGF.B.createPointerToAddress(loc, args[0].getUnmanagedValue(),
                                                loadedType.getAddressType(),
                                                isStrict, isInvariant);
   // Perform the load.
   return SGF.emitLoad(loc, addr, rvalueTL, C, isTake);
}

static ManagedValue emitBuiltinLoad(PILGenFunction &SGF,
                                    PILLocation loc,
                                    SubstitutionMap substitutions,
                                    ArrayRef<ManagedValue> args,
                                    SGFContext C) {
   return emitBuiltinLoadOrTake(SGF, loc, substitutions, args,
                                C, IsNotTake,
      /*isStrict*/ true, /*isInvariant*/ false);
}

static ManagedValue emitBuiltinLoadRaw(PILGenFunction &SGF,
                                       PILLocation loc,
                                       SubstitutionMap substitutions,
                                       ArrayRef<ManagedValue> args,
                                       SGFContext C) {
   return emitBuiltinLoadOrTake(SGF, loc, substitutions, args,
                                C, IsNotTake,
      /*isStrict*/ false, /*isInvariant*/ false);
}

static ManagedValue emitBuiltinLoadInvariant(PILGenFunction &SGF,
                                             PILLocation loc,
                                             SubstitutionMap substitutions,
                                             ArrayRef<ManagedValue> args,
                                             SGFContext C) {
   return emitBuiltinLoadOrTake(SGF, loc, substitutions, args,
                                C, IsNotTake,
      /*isStrict*/ false, /*isInvariant*/ true);
}

static ManagedValue emitBuiltinTake(PILGenFunction &SGF,
                                    PILLocation loc,
                                    SubstitutionMap substitutions,
                                    ArrayRef<ManagedValue> args,
                                    SGFContext C) {
   return emitBuiltinLoadOrTake(SGF, loc, substitutions, args,
                                C, IsTake,
      /*isStrict*/ true, /*isInvariant*/ false);
}

/// Specialized emitter for Builtin.destroy.
static ManagedValue emitBuiltinDestroy(PILGenFunction &SGF,
                                       PILLocation loc,
                                       SubstitutionMap substitutions,
                                       ArrayRef<ManagedValue> args,
                                       SGFContext C) {
   assert(args.size() == 2 && "destroy should have two arguments");
   assert(substitutions.getReplacementTypes().size() == 1 &&
          "destroy should have a single substitution");
   // The substitution determines the type of the thing we're destroying.
   auto &ti = SGF.getTypeLowering(substitutions.getReplacementTypes()[0]);

   // Destroy is a no-op for trivial types.
   if (ti.isTrivial())
      return ManagedValue::forUnmanaged(SGF.emitEmptyTuple(loc));

   PILType destroyType = ti.getLoweredType();

   // Convert the pointer argument to a PIL address.
   PILValue addr =
      SGF.B.createPointerToAddress(loc, args[1].getUnmanagedValue(),
                                   destroyType.getAddressType(),
         /*isStrict*/ true,
         /*isInvariant*/ false);

   // Destroy the value indirectly. Canonicalization will promote to loads
   // and releases if appropriate.
   SGF.B.createDestroyAddr(loc, addr);

   return ManagedValue::forUnmanaged(SGF.emitEmptyTuple(loc));
}

static ManagedValue emitBuiltinAssign(PILGenFunction &SGF,
                                      PILLocation loc,
                                      SubstitutionMap substitutions,
                                      ArrayRef<ManagedValue> args,
                                      SGFContext C) {
   assert(args.size() >= 2 && "assign should have two arguments");
   assert(substitutions.getReplacementTypes().size() == 1 &&
          "assign should have a single substitution");

   // The substitution determines the type of the thing we're destroying.
   CanType assignFormalType =
      substitutions.getReplacementTypes()[0]->getCanonicalType();
   PILType assignType = SGF.getLoweredType(assignFormalType);

   // Convert the destination pointer argument to a PIL address.
   PILValue addr = SGF.B.createPointerToAddress(loc,
                                                args.back().getUnmanagedValue(),
                                                assignType.getAddressType(),
      /*isStrict*/ true,
      /*isInvariant*/ false);

   // Build the value to be assigned, reconstructing tuples if needed.
   auto src = RValue(SGF, args.slice(0, args.size() - 1), assignFormalType);

   std::move(src).ensurePlusOne(SGF, loc).assignInto(SGF, loc, addr);

   return ManagedValue::forUnmanaged(SGF.emitEmptyTuple(loc));
}

/// Emit Builtin.initialize by evaluating the operand directly into
/// the address.
static ManagedValue emitBuiltinInit(PILGenFunction &SGF,
                                    PILLocation loc,
                                    SubstitutionMap substitutions,
                                    PreparedArguments &&preparedArgs,
                                    SGFContext C) {
   auto argsOrError = decomposeArguments(SGF, loc, std::move(preparedArgs), 2);
   if (!argsOrError)
      return ManagedValue::forUnmanaged(SGF.emitEmptyTuple(loc));

   auto args = *argsOrError;

   CanType formalType =
      substitutions.getReplacementTypes()[0]->getCanonicalType();
   auto &formalTL = SGF.getTypeLowering(formalType);

   PILValue addr = SGF.emitRValueAsSingleValue(args[1]).getUnmanagedValue();
   addr = SGF.B.createPointerToAddress(
      loc, addr, formalTL.getLoweredType().getAddressType(),
      /*isStrict*/ true,
      /*isInvariant*/ false);

   TemporaryInitialization init(addr, CleanupHandle::invalid());
   SGF.emitExprInto(args[0], &init);

   return ManagedValue::forUnmanaged(SGF.emitEmptyTuple(loc));
}

/// Specialized emitter for Builtin.fixLifetime.
static ManagedValue emitBuiltinFixLifetime(PILGenFunction &SGF,
                                           PILLocation loc,
                                           SubstitutionMap substitutions,
                                           ArrayRef<ManagedValue> args,
                                           SGFContext C) {
   for (auto arg : args) {
      SGF.B.createFixLifetime(loc, arg.getValue());
   }
   return ManagedValue::forUnmanaged(SGF.emitEmptyTuple(loc));
}

static ManagedValue emitCastToReferenceType(PILGenFunction &SGF,
                                            PILLocation loc,
                                            SubstitutionMap substitutions,
                                            ArrayRef<ManagedValue> args,
                                            SGFContext C,
                                            PILType objPointerType) {
   assert(args.size() == 1 && "cast should have a single argument");
   assert(substitutions.getReplacementTypes().size() == 1 &&
          "cast should have a type substitution");

   // Bail if the source type is not a class reference of some kind.
   Type argTy = substitutions.getReplacementTypes()[0];
   if (!argTy->mayHaveSuperclass() && !argTy->isClassExistentialType()) {
      SGF.SGM.diagnose(loc, diag::invalid_sil_builtin,
                       "castToNativeObject source must be a class");
      return SGF.emitUndef(objPointerType);
   }

   // Grab the argument.
   ManagedValue arg = args[0];

   // If the argument is existential, open it.
   if (argTy->isClassExistentialType()) {
      auto openedTy = OpenedArchetypeType::get(argTy);
      PILType loweredOpenedTy = SGF.getLoweredLoadableType(openedTy);
      arg = SGF.B.createOpenExistentialRef(loc, arg, loweredOpenedTy);
   }

   // Return the cast result.
   return SGF.B.createUncheckedRefCast(loc, arg, objPointerType);
}

/// Specialized emitter for Builtin.unsafeCastToNativeObject.
static ManagedValue emitBuiltinUnsafeCastToNativeObject(PILGenFunction &SGF,
                                                        PILLocation loc,
                                                        SubstitutionMap substitutions,
                                                        ArrayRef<ManagedValue> args,
                                                        SGFContext C) {
   return emitCastToReferenceType(SGF, loc, substitutions, args, C,
                                  PILType::getNativeObjectType(SGF.F.getAstContext()));
}

/// Specialized emitter for Builtin.castToNativeObject.
static ManagedValue emitBuiltinCastToNativeObject(PILGenFunction &SGF,
                                                  PILLocation loc,
                                                  SubstitutionMap substitutions,
                                                  ArrayRef<ManagedValue> args,
                                                  SGFContext C) {
   auto ty = args[0].getType().getAstType();
   (void)ty;
   assert(ty->getReferenceCounting() == ReferenceCounting::Native &&
          "Can only cast types that use native reference counting to native "
          "object");
   return emitBuiltinUnsafeCastToNativeObject(SGF, loc, substitutions,
                                              args, C);
}


static ManagedValue emitCastFromReferenceType(PILGenFunction &SGF,
                                              PILLocation loc,
                                              SubstitutionMap substitutions,
                                              ArrayRef<ManagedValue> args,
                                              SGFContext C) {
   assert(args.size() == 1 && "cast should have a single argument");
   assert(substitutions.getReplacementTypes().size() == 1 &&
          "cast should have a single substitution");

   // The substitution determines the destination type.
   PILType destType =
      SGF.getLoweredType(substitutions.getReplacementTypes()[0]);

   // Bail if the source type is not a class reference of some kind.
   if (!substitutions.getReplacementTypes()[0]->isBridgeableObjectType()
       || !destType.isObject()) {
      SGF.SGM.diagnose(loc, diag::invalid_sil_builtin,
                       "castFromNativeObject dest must be an object type");
      // Recover by propagating an undef result.
      return SGF.emitUndef(destType);
   }

   return SGF.B.createUncheckedRefCast(loc, args[0], destType);
}

/// Specialized emitter for Builtin.castFromNativeObject.
static ManagedValue emitBuiltinCastFromNativeObject(PILGenFunction &SGF,
                                                    PILLocation loc,
                                                    SubstitutionMap substitutions,
                                                    ArrayRef<ManagedValue> args,
                                                    SGFContext C) {
   return emitCastFromReferenceType(SGF, loc, substitutions, args, C);
}

/// Specialized emitter for Builtin.bridgeToRawPointer.
static ManagedValue emitBuiltinBridgeToRawPointer(PILGenFunction &SGF,
                                                  PILLocation loc,
                                                  SubstitutionMap substitutions,
                                                  ArrayRef<ManagedValue> args,
                                                  SGFContext C) {
   assert(args.size() == 1 && "bridge should have a single argument");

   // Take the reference type argument and cast it to RawPointer.
   // RawPointers do not have ownership semantics, so the cleanup on the
   // argument remains.
   PILType rawPointerType = PILType::getRawPointerType(SGF.F.getAstContext());
   PILValue result = SGF.B.createRefToRawPointer(loc, args[0].getValue(),
                                                 rawPointerType);
   return ManagedValue::forUnmanaged(result);
}

/// Specialized emitter for Builtin.bridgeFromRawPointer.
static ManagedValue emitBuiltinBridgeFromRawPointer(PILGenFunction &SGF,
                                                    PILLocation loc,
                                                    SubstitutionMap substitutions,
                                                    ArrayRef<ManagedValue> args,
                                                    SGFContext C) {
   assert(substitutions.getReplacementTypes().size() == 1 &&
          "bridge should have a single substitution");
   assert(args.size() == 1 && "bridge should have a single argument");

   // The substitution determines the destination type.
   // FIXME: Archetype destination type?
   auto &destLowering =
      SGF.getTypeLowering(substitutions.getReplacementTypes()[0]);
   assert(destLowering.isLoadable());
   PILType destType = destLowering.getLoweredType();

   // Take the raw pointer argument and cast it to the destination type.
   PILValue result = SGF.B.createRawPointerToRef(loc, args[0].getUnmanagedValue(),
                                                 destType);
   // The result has ownership semantics, so retain it with a cleanup.
   return SGF.emitManagedRetain(loc, result, destLowering);
}

/// Specialized emitter for Builtin.addressof.
static ManagedValue emitBuiltinAddressOf(PILGenFunction &SGF,
                                         PILLocation loc,
                                         SubstitutionMap substitutions,
                                         PreparedArguments &&preparedArgs,
                                         SGFContext C) {
   PILType rawPointerType = PILType::getRawPointerType(SGF.getAstContext());

   auto argsOrError = decomposeArguments(SGF, loc, std::move(preparedArgs), 1);
   if (!argsOrError)
      return SGF.emitUndef(rawPointerType);

   auto argument = (*argsOrError)[0];

   // If the argument is inout, try forming its lvalue. This builtin only works
   // if it's trivially physically projectable.
   auto inout = cast<InOutExpr>(argument->getSemanticsProvidingExpr());
   auto lv = SGF.emitLValue(inout->getSubExpr(), SGFAccessKind::ReadWrite);
   if (!lv.isPhysical() || !lv.isLoadingPure()) {
      SGF.SGM.diagnose(argument->getLoc(), diag::non_physical_addressof);
      return SGF.emitUndef(rawPointerType);
   }

   auto addr = SGF.emitAddressOfLValue(argument, std::move(lv))
      .getLValueAddress();

   // Take the address argument and cast it to RawPointer.
   PILValue result = SGF.B.createAddressToPointer(loc, addr,
                                                  rawPointerType);
   return ManagedValue::forUnmanaged(result);
}

/// Specialized emitter for Builtin.addressOfBorrow.
static ManagedValue emitBuiltinAddressOfBorrow(PILGenFunction &SGF,
                                               PILLocation loc,
                                               SubstitutionMap substitutions,
                                               PreparedArguments &&preparedArgs,
                                               SGFContext C) {
   PILType rawPointerType = PILType::getRawPointerType(SGF.getAstContext());

   auto argsOrError = decomposeArguments(SGF, loc, std::move(preparedArgs), 1);
   if (!argsOrError)
      return SGF.emitUndef(rawPointerType);

   auto argument = (*argsOrError)[0];

   PILValue addr;
   // Try to borrow the argument at +0. We only support if it's
   // naturally emitted borrowed in memory.
   auto borrow = SGF.emitRValue(argument, SGFContext::AllowGuaranteedPlusZero)
      .getAsSingleValue(SGF, argument);
   if (!borrow.isPlusZero() || !borrow.getType().isAddress()) {
      SGF.SGM.diagnose(argument->getLoc(), diag::non_borrowed_indirect_addressof);
      return SGF.emitUndef(rawPointerType);
   }

   addr = borrow.getValue();

   // Take the address argument and cast it to RawPointer.
   PILValue result = SGF.B.createAddressToPointer(loc, addr,
                                                  rawPointerType);
   return ManagedValue::forUnmanaged(result);
}

/// Specialized emitter for Builtin.gepRaw.
static ManagedValue emitBuiltinGepRaw(PILGenFunction &SGF,
                                      PILLocation loc,
                                      SubstitutionMap substitutions,
                                      ArrayRef<ManagedValue> args,
                                      SGFContext C) {
   assert(args.size() == 2 && "gepRaw should be given two arguments");

   PILValue offsetPtr = SGF.B.createIndexRawPointer(loc,
                                                    args[0].getUnmanagedValue(),
                                                    args[1].getUnmanagedValue());
   return ManagedValue::forUnmanaged(offsetPtr);
}

/// Specialized emitter for Builtin.gep.
static ManagedValue emitBuiltinGep(PILGenFunction &SGF,
                                   PILLocation loc,
                                   SubstitutionMap substitutions,
                                   ArrayRef<ManagedValue> args,
                                   SGFContext C) {
   assert(substitutions.getReplacementTypes().size() == 1 &&
          "gep should have two substitutions");
   assert(args.size() == 3 && "gep should be given three arguments");

   PILType ElemTy = SGF.getLoweredType(substitutions.getReplacementTypes()[0]);
   PILType RawPtrType = args[0].getUnmanagedValue()->getType();
   PILValue addr = SGF.B.createPointerToAddress(loc,
                                                args[0].getUnmanagedValue(),
                                                ElemTy.getAddressType(),
      /*strict*/ true,
      /*invariant*/ false);
   addr = SGF.B.createIndexAddr(loc, addr, args[1].getUnmanagedValue());
   addr = SGF.B.createAddressToPointer(loc, addr, RawPtrType);

   return ManagedValue::forUnmanaged(addr);
}

/// Specialized emitter for Builtin.getTailAddr.
static ManagedValue emitBuiltinGetTailAddr(PILGenFunction &SGF,
                                           PILLocation loc,
                                           SubstitutionMap substitutions,
                                           ArrayRef<ManagedValue> args,
                                           SGFContext C) {
   assert(substitutions.getReplacementTypes().size() == 2 &&
          "getTailAddr should have two substitutions");
   assert(args.size() == 4 && "gep should be given four arguments");

   PILType ElemTy = SGF.getLoweredType(substitutions.getReplacementTypes()[0]);
   PILType TailTy = SGF.getLoweredType(substitutions.getReplacementTypes()[1]);
   PILType RawPtrType = args[0].getUnmanagedValue()->getType();
   PILValue addr = SGF.B.createPointerToAddress(loc,
                                                args[0].getUnmanagedValue(),
                                                ElemTy.getAddressType(),
      /*strict*/ true,
      /*invariant*/ false);
   addr = SGF.B.createTailAddr(loc, addr, args[1].getUnmanagedValue(),
                               TailTy.getAddressType());
   addr = SGF.B.createAddressToPointer(loc, addr, RawPtrType);

   return ManagedValue::forUnmanaged(addr);
}

/// Specialized emitter for Builtin.beginUnpairedModifyAccess.
static ManagedValue emitBuiltinBeginUnpairedModifyAccess(PILGenFunction &SGF,
                                                         PILLocation loc,
                                                         SubstitutionMap substitutions,
                                                         ArrayRef<ManagedValue> args,
                                                         SGFContext C) {
   assert(substitutions.getReplacementTypes().size() == 1 &&
          "Builtin.beginUnpairedModifyAccess should have one substitution");
   assert(args.size() == 3 &&
          "beginUnpairedModifyAccess should be given three arguments");

   PILType elemTy = SGF.getLoweredType(substitutions.getReplacementTypes()[0]);
   PILValue addr = SGF.B.createPointerToAddress(loc,
                                                args[0].getUnmanagedValue(),
                                                elemTy.getAddressType(),
      /*strict*/ true,
      /*invariant*/ false);

   PILType valueBufferTy =
      SGF.getLoweredType(SGF.getAstContext().TheUnsafeValueBufferType);

   PILValue buffer =
      SGF.B.createPointerToAddress(loc, args[1].getUnmanagedValue(),
                                   valueBufferTy.getAddressType(),
         /*strict*/ true,
         /*invariant*/ false);
   SGF.B.createBeginUnpairedAccess(loc, addr, buffer, PILAccessKind::Modify,
                                   PILAccessEnforcement::Dynamic,
      /*noNestedConflict*/ false,
      /*fromBuiltin*/ true);

   return ManagedValue::forUnmanaged(SGF.emitEmptyTuple(loc));
}

/// Specialized emitter for Builtin.performInstantaneousReadAccess
static ManagedValue emitBuiltinPerformInstantaneousReadAccess(
   PILGenFunction &SGF, PILLocation loc, SubstitutionMap substitutions,
   ArrayRef<ManagedValue> args, SGFContext C) {

   assert(substitutions.getReplacementTypes().size() == 1 &&
          "Builtin.performInstantaneousReadAccess should have one substitution");
   assert(args.size() == 2 &&
          "Builtin.performInstantaneousReadAccess should be given "
          "two arguments");

   PILType elemTy = SGF.getLoweredType(substitutions.getReplacementTypes()[0]);
   PILValue addr = SGF.B.createPointerToAddress(loc,
                                                args[0].getUnmanagedValue(),
                                                elemTy.getAddressType(),
      /*strict*/ true,
      /*invariant*/ false);

   PILType valueBufferTy =
      SGF.getLoweredType(SGF.getAstContext().TheUnsafeValueBufferType);
   PILValue unusedBuffer = SGF.emitTemporaryAllocation(loc, valueBufferTy);

   // Begin an "unscoped" read access. No nested conflict is possible because
   // the compiler should generate the actual read for the KeyPath expression
   // immediately after the call to this builtin, which forms the address of
   // that real access. When noNestedConflict=true, no EndUnpairedAccess should
   // be emitted.
   //
   // Unpaired access is necessary because a BeginAccess/EndAccess pair with no
   // use will be trivially optimized away.
   SGF.B.createBeginUnpairedAccess(loc, addr, unusedBuffer, PILAccessKind::Read,
                                   PILAccessEnforcement::Dynamic,
      /*noNestedConflict*/ true,
      /*fromBuiltin*/ true);

   return ManagedValue::forUnmanaged(SGF.emitEmptyTuple(loc));
}

/// Specialized emitter for Builtin.endUnpairedAccessModifyAccess.
static ManagedValue emitBuiltinEndUnpairedAccess(PILGenFunction &SGF,
                                                 PILLocation loc,
                                                 SubstitutionMap substitutions,
                                                 ArrayRef<ManagedValue> args,
                                                 SGFContext C) {
   assert(substitutions.empty() &&
          "Builtin.endUnpairedAccess should have no substitutions");
   assert(args.size() == 1 &&
          "endUnpairedAccess should be given one argument");

   PILType valueBufferTy =
      SGF.getLoweredType(SGF.getAstContext().TheUnsafeValueBufferType);

   PILValue buffer = SGF.B.createPointerToAddress(loc,
                                                  args[0].getUnmanagedValue(),
                                                  valueBufferTy.getAddressType(),
      /*strict*/ true,
      /*invariant*/ false);
   SGF.B.createEndUnpairedAccess(loc, buffer, PILAccessEnforcement::Dynamic,
      /*aborted*/ false,
      /*fromBuiltin*/ true);

   return ManagedValue::forUnmanaged(SGF.emitEmptyTuple(loc));
}

/// Specialized emitter for the legacy Builtin.condfail.
static ManagedValue emitBuiltinLegacyCondFail(PILGenFunction &SGF,
                                              PILLocation loc,
                                              SubstitutionMap substitutions,
                                              ArrayRef<ManagedValue> args,
                                              SGFContext C) {
   assert(args.size() == 1 && "condfail should be given one argument");

   SGF.B.createCondFail(loc, args[0].getUnmanagedValue(),
                        "unknown runtime failure");
   return ManagedValue::forUnmanaged(SGF.emitEmptyTuple(loc));
}

/// Specialized emitter for Builtin.castReference.
static ManagedValue
emitBuiltinCastReference(PILGenFunction &SGF,
                         PILLocation loc,
                         SubstitutionMap substitutions,
                         ArrayRef<ManagedValue> args,
                         SGFContext C) {
   assert(args.size() == 1 && "castReference should be given one argument");
   assert(substitutions.getReplacementTypes().size() == 2 &&
          "castReference should have two subs");

   auto fromTy = substitutions.getReplacementTypes()[0];
   auto toTy = substitutions.getReplacementTypes()[1];
   auto &fromTL = SGF.getTypeLowering(fromTy);
   auto &toTL = SGF.getTypeLowering(toTy);
   assert(!fromTL.isTrivial() && !toTL.isTrivial() && "expected ref type");

   // TODO: Fix this API.
   if (!fromTL.isAddress() || !toTL.isAddress()) {
      if (auto refCast = SGF.B.tryCreateUncheckedRefCast(loc, args[0],
                                                         toTL.getLoweredType())) {
         // Create a reference cast, forwarding the cleanup.
         // The cast takes the source reference.
         return refCast;
      }
   }

   // We are either casting between address-only types, or cannot promote to a
   // cast of reference values.
   //
   // If the from/to types are invalid, then use a cast that will fail at
   // runtime. We cannot catch these errors with PIL verification because they
   // may legitimately occur during code specialization on dynamically
   // unreachable paths.
   //
   // TODO: For now, we leave invalid casts in address form so that the runtime
   // will trap. We could emit a noreturn call here instead which would provide
   // more information to the optimizer.
   PILValue srcVal = args[0].ensurePlusOne(SGF, loc).forward(SGF);
   PILValue fromAddr;
   if (!fromTL.isAddress()) {
      // Move the loadable value into a "source temp".  Since the source and
      // dest are RC identical, store the reference into the source temp without
      // a retain. The cast will load the reference from the source temp and
      // store it into a dest temp effectively forwarding the cleanup.
      fromAddr = SGF.emitTemporaryAllocation(loc, srcVal->getType());
      fromTL.emitStore(SGF.B, loc, srcVal, fromAddr,
                       StoreOwnershipQualifier::Init);
   } else {
      // The cast loads directly from the source address.
      fromAddr = srcVal;
   }
   // Create a "dest temp" to hold the reference after casting it.
   PILValue toAddr = SGF.emitTemporaryAllocation(loc, toTL.getLoweredType());
   SGF.B.createUncheckedRefCastAddr(loc, fromAddr, fromTy->getCanonicalType(),
                                    toAddr, toTy->getCanonicalType());
   // Forward it along and register a cleanup.
   if (toTL.isAddress())
      return SGF.emitManagedBufferWithCleanup(toAddr);

   // Load the destination value.
   auto result = toTL.emitLoad(SGF.B, loc, toAddr, LoadOwnershipQualifier::Take);
   return SGF.emitManagedRValueWithCleanup(result);
}

/// Specialized emitter for Builtin.reinterpretCast.
static ManagedValue emitBuiltinReinterpretCast(PILGenFunction &SGF,
                                               PILLocation loc,
                                               SubstitutionMap substitutions,
                                               ArrayRef<ManagedValue> args,
                                               SGFContext C) {
   assert(args.size() == 1 && "reinterpretCast should be given one argument");
   assert(substitutions.getReplacementTypes().size() == 2 &&
          "reinterpretCast should have two subs");

   auto &fromTL = SGF.getTypeLowering(substitutions.getReplacementTypes()[0]);
   auto &toTL = SGF.getTypeLowering(substitutions.getReplacementTypes()[1]);

   // If casting between address types, cast the address.
   if (fromTL.isAddress() || toTL.isAddress()) {
      PILValue fromAddr;

      // If the from value is not an address, move it to a buffer.
      if (!fromTL.isAddress()) {
         fromAddr = SGF.emitTemporaryAllocation(loc, args[0].getValue()->getType());
         fromTL.emitStore(SGF.B, loc, args[0].getValue(), fromAddr,
                          StoreOwnershipQualifier::Init);
      } else {
         fromAddr = args[0].getValue();
      }
      auto toAddr = SGF.B.createUncheckedAddrCast(loc, fromAddr,
                                                  toTL.getLoweredType().getAddressType());

      // Load and retain the destination value if it's loadable. Leave the cleanup
      // on the original value since we don't know anything about it's type.
      if (!toTL.isAddress()) {
         return SGF.emitManagedLoadCopy(loc, toAddr, toTL);
      }
      // Leave the cleanup on the original value.
      if (toTL.isTrivial())
         return ManagedValue::forUnmanaged(toAddr);

      // Initialize the +1 result buffer without taking the incoming value. The
      // source and destination cleanups will be independent.
      return SGF.B.bufferForExpr(
         loc, toTL.getLoweredType(), toTL, C,
         [&](PILValue bufferAddr) {
            SGF.B.createCopyAddr(loc, toAddr, bufferAddr, IsNotTake,
                                 IsInitialization);
         });
   }
   // Create the appropriate bitcast based on the source and dest types.
   ManagedValue in = args[0];
   PILType resultTy = toTL.getLoweredType();
   if (resultTy.isTrivial(SGF.F))
      return SGF.B.createUncheckedTrivialBitCast(loc, in, resultTy);

   // If we can perform a ref cast, just return.
   if (auto refCast = SGF.B.tryCreateUncheckedRefCast(loc, in, resultTy))
      return refCast;

   // Otherwise leave the original cleanup and retain the cast value.
   PILValue out = SGF.B.createUncheckedBitwiseCast(loc, in.getValue(), resultTy);
   return SGF.emitManagedRetain(loc, out, toTL);
}

/// Specialized emitter for Builtin.castToBridgeObject.
static ManagedValue emitBuiltinCastToBridgeObject(PILGenFunction &SGF,
                                                  PILLocation loc,
                                                  SubstitutionMap subs,
                                                  ArrayRef<ManagedValue> args,
                                                  SGFContext C) {
   assert(args.size() == 2 && "cast should have two arguments");
   assert(subs.getReplacementTypes().size() == 1 &&
          "cast should have a type substitution");

   // Take the reference type argument and cast it to BridgeObject.
   PILType objPointerType = PILType::getBridgeObjectType(SGF.F.getAstContext());

   // Bail if the source type is not a class reference of some kind.
   auto sourceType = subs.getReplacementTypes()[0];
   if (!sourceType->mayHaveSuperclass() &&
       !sourceType->isClassExistentialType()) {
      SGF.SGM.diagnose(loc, diag::invalid_sil_builtin,
                       "castToBridgeObject source must be a class");
      return SGF.emitUndef(objPointerType);
   }

   ManagedValue ref = args[0];
   PILValue bits = args[1].getUnmanagedValue();

   // If the argument is existential, open it.
   if (sourceType->isClassExistentialType()) {
      auto openedTy = OpenedArchetypeType::get(sourceType);
      PILType loweredOpenedTy = SGF.getLoweredLoadableType(openedTy);
      ref = SGF.B.createOpenExistentialRef(loc, ref, loweredOpenedTy);
   }

   return SGF.B.createRefToBridgeObject(loc, ref, bits);
}

/// Specialized emitter for Builtin.castReferenceFromBridgeObject.
static ManagedValue emitBuiltinCastReferenceFromBridgeObject(
   PILGenFunction &SGF,
   PILLocation loc,
   SubstitutionMap subs,
   ArrayRef<ManagedValue> args,
   SGFContext C) {
   assert(args.size() == 1 && "cast should have one argument");
   assert(subs.getReplacementTypes().size() == 1 &&
          "cast should have a type substitution");

   // The substitution determines the destination type.
   auto destTy = subs.getReplacementTypes()[0];
   PILType destType = SGF.getLoweredType(destTy);

   // Bail if the source type is not a class reference of some kind.
   if (!destTy->isBridgeableObjectType() || !destType.isObject()) {
      SGF.SGM.diagnose(loc, diag::invalid_sil_builtin,
                       "castReferenceFromBridgeObject dest must be an object type");
      // Recover by propagating an undef result.
      return SGF.emitUndef(destType);
   }

   return SGF.B.createBridgeObjectToRef(loc, args[0], destType);
}

static ManagedValue emitBuiltinCastBitPatternFromBridgeObject(
   PILGenFunction &SGF,
   PILLocation loc,
   SubstitutionMap subs,
   ArrayRef<ManagedValue> args,
   SGFContext C) {
   assert(args.size() == 1 && "cast should have one argument");
   assert(subs.empty() && "cast should not have subs");

   PILType wordType = PILType::getBuiltinWordType(SGF.getAstContext());
   PILValue result = SGF.B.createBridgeObjectToWord(loc, args[0].getValue(),
                                                    wordType);
   return ManagedValue::forUnmanaged(result);
}

static ManagedValue emitBuiltinClassifyBridgeObject(PILGenFunction &SGF,
                                                    PILLocation loc,
                                                    SubstitutionMap subs,
                                                    ArrayRef<ManagedValue> args,
                                                    SGFContext C) {
   assert(args.size() == 1 && "classify should have one argument");
   assert(subs.empty() && "classify should not have subs");

   PILValue result = SGF.B.createClassifyBridgeObject(loc, args[0].getValue());
   return ManagedValue::forUnmanaged(result);
}

static ManagedValue emitBuiltinValueToBridgeObject(PILGenFunction &SGF,
                                                   PILLocation loc,
                                                   SubstitutionMap subs,
                                                   ArrayRef<ManagedValue> args,
                                                   SGFContext C) {
   assert(args.size() == 1 && "ValueToBridgeObject should have one argument");
   assert(subs.getReplacementTypes().size() == 1 &&
          "ValueToBridgeObject should have one sub");

   Type argTy = subs.getReplacementTypes()[0];
   if (!argTy->is<BuiltinIntegerType>()) {
      SGF.SGM.diagnose(loc, diag::invalid_sil_builtin,
                       "argument to builtin should be a builtin integer");
      PILType objPointerType = PILType::getBridgeObjectType(SGF.F.getAstContext());
      return SGF.emitUndef(objPointerType);
   }

   PILValue result = SGF.B.createValueToBridgeObject(loc, args[0].getValue());
   return SGF.emitManagedRetain(loc, result);
}

// This should only accept as an operand type single-refcounted-pointer types,
// class existentials, or single-payload enums (optional). Type checking must be
// deferred until IRGen so Builtin.isUnique can be called from a transparent
// generic wrapper (we can only type check after specialization).
static ManagedValue emitBuiltinIsUnique(PILGenFunction &SGF,
                                        PILLocation loc,
                                        SubstitutionMap subs,
                                        ArrayRef<ManagedValue> args,
                                        SGFContext C) {

   assert(subs.getReplacementTypes().size() == 1 &&
          "isUnique should have a single substitution");
   assert(args.size() == 1 && "isUnique should have a single argument");
   assert((args[0].getType().isAddress() && !args[0].hasCleanup()) &&
          "Builtin.isUnique takes an address.");

   return ManagedValue::forUnmanaged(
      SGF.B.createIsUnique(loc, args[0].getValue()));
}

// This force-casts the incoming address to NativeObject assuming the caller has
// performed all necessary checks. For example, this may directly cast a
// single-payload enum to a NativeObject reference.
static ManagedValue
emitBuiltinIsUnique_native(PILGenFunction &SGF,
                           PILLocation loc,
                           SubstitutionMap subs,
                           ArrayRef<ManagedValue> args,
                           SGFContext C) {

   assert(subs.getReplacementTypes().size() == 1 &&
          "isUnique_native should have one sub.");
   assert(args.size() == 1 && "isUnique_native should have one arg.");

   auto ToType =
      PILType::getNativeObjectType(SGF.getAstContext()).getAddressType();
   auto toAddr = SGF.B.createUncheckedAddrCast(loc, args[0].getValue(), ToType);
   PILValue result = SGF.B.createIsUnique(loc, toAddr);
   return ManagedValue::forUnmanaged(result);
}

static ManagedValue emitBuiltinBindMemory(PILGenFunction &SGF,
                                          PILLocation loc,
                                          SubstitutionMap subs,
                                          ArrayRef<ManagedValue> args,
                                          SGFContext C) {
   assert(subs.getReplacementTypes().size() == 1 && "bindMemory should have a single substitution");
   assert(args.size() == 3 && "bindMemory should have three argument");

   // The substitution determines the element type for bound memory.
   CanType boundFormalType = subs.getReplacementTypes()[0]->getCanonicalType();
   PILType boundType = SGF.getLoweredType(boundFormalType);

   SGF.B.createBindMemory(loc, args[0].getValue(),
                          args[1].getValue(), boundType);

   return ManagedValue::forUnmanaged(SGF.emitEmptyTuple(loc));
}

static ManagedValue emitBuiltinAllocWithTailElems(PILGenFunction &SGF,
                                                  PILLocation loc,
                                                  SubstitutionMap subs,
                                                  ArrayRef<ManagedValue> args,
                                                  SGFContext C) {
   unsigned NumTailTypes = subs.getReplacementTypes().size() - 1;
   assert(args.size() == NumTailTypes * 2 + 1 &&
          "wrong number of substitutions for allocWithTailElems");

   // The substitution determines the element type for bound memory.
   auto replacementTypes = subs.getReplacementTypes();
   PILType RefType = SGF.getLoweredType(replacementTypes[0]->
      getCanonicalType()).getObjectType();

   SmallVector<ManagedValue, 4> Counts;
   SmallVector<PILType, 4> ElemTypes;
   for (unsigned Idx = 0; Idx < NumTailTypes; ++Idx) {
      Counts.push_back(args[Idx * 2 + 1]);
      ElemTypes.push_back(SGF.getLoweredType(replacementTypes[Idx+1]->
         getCanonicalType()).getObjectType());
   }
   ManagedValue Metatype = args[0];
   if (isa<MetatypeInst>(Metatype)) {
      auto InstanceType =
         Metatype.getType().castTo<MetatypeType>().getInstanceType();
      assert(InstanceType == RefType.getAstType() &&
             "substituted type does not match operand metatype");
      (void) InstanceType;
      return SGF.B.createAllocRef(loc, RefType, false, false,
                                  ElemTypes, Counts);
   } else {
      return SGF.B.createAllocRefDynamic(loc, Metatype, RefType, false,
                                         ElemTypes, Counts);
   }
}

static ManagedValue emitBuiltinProjectTailElems(PILGenFunction &SGF,
                                                PILLocation loc,
                                                SubstitutionMap subs,
                                                ArrayRef<ManagedValue> args,
                                                SGFContext C) {
   assert(subs.getReplacementTypes().size() == 2 &&
          "allocWithTailElems should have two substitutions");
   assert(args.size() == 2 &&
          "allocWithTailElems should have three arguments");

   // The substitution determines the element type for bound memory.
   PILType ElemType = SGF.getLoweredType(subs.getReplacementTypes()[1]->
      getCanonicalType()).getObjectType();

   PILValue result = SGF.B.createRefTailAddr(loc, args[0].getValue(),
                                             ElemType.getAddressType());
   PILType rawPointerType = PILType::getRawPointerType(SGF.F.getAstContext());
   result = SGF.B.createAddressToPointer(loc, result, rawPointerType);
   return ManagedValue::forUnmanaged(result);
}

/// Specialized emitter for type traits.
template<TypeTraitResult (TypeBase::*Trait)(),
   BuiltinValueKind Kind>
static ManagedValue emitBuiltinTypeTrait(PILGenFunction &SGF,
                                         PILLocation loc,
                                         SubstitutionMap substitutions,
                                         ArrayRef<ManagedValue> args,
                                         SGFContext C) {
   assert(substitutions.getReplacementTypes().size() == 1
          && "type trait should take a single type parameter");
   assert(args.size() == 1
          && "type trait should take a single argument");

   unsigned result;

   auto traitTy = substitutions.getReplacementTypes()[0]->getCanonicalType();

   switch ((traitTy.getPointer()->*Trait)()) {
      // If the type obviously has or lacks the trait, emit a constant result.
      case TypeTraitResult::IsNot:
         result = 0;
         break;
      case TypeTraitResult::Is:
         result = 1;
         break;

         // If not, emit the builtin call normally. Specialization may be able to
         // eliminate it later, or we'll lower it away at IRGen time.
      case TypeTraitResult::CanBe: {
         auto &C = SGF.getAstContext();
         auto int8Ty = BuiltinIntegerType::get(8, C)->getCanonicalType();
         auto apply = SGF.B.createBuiltin(loc,
                                          C.getIdentifier(getBuiltinName(Kind)),
                                          PILType::getPrimitiveObjectType(int8Ty),
                                          substitutions, args[0].getValue());

         return ManagedValue::forUnmanaged(apply);
      }
   }

   // Produce the result as an integer literal constant.
   auto val = SGF.B.createIntegerLiteral(
      loc, PILType::getBuiltinIntegerType(8, SGF.getAstContext()),
      (uintmax_t)result);
   return ManagedValue::forUnmanaged(val);
}

/// Emit PIL for the named builtin: globalStringTablePointer. Unlike the default
/// ownership convention for named builtins, which is to take (non-trivial)
/// arguments as Owned, this builtin accepts owned as well as guaranteed
/// arguments, and hence doesn't require the arguments to be at +1. Therefore,
/// this builtin is emitted specially.
static ManagedValue
emitBuiltinGlobalStringTablePointer(PILGenFunction &SGF, PILLocation loc,
                                    SubstitutionMap subs,
                                    ArrayRef<ManagedValue> args, SGFContext C) {
   assert(args.size() == 1);

   PILValue argValue = args[0].getValue();
   auto &astContext = SGF.getAstContext();
   Identifier builtinId = astContext.getIdentifier(
      getBuiltinName(BuiltinValueKind::GlobalStringTablePointer));

   auto resultVal = SGF.B.createBuiltin(loc, builtinId,
                                        PILType::getRawPointerType(astContext),
                                        subs, ArrayRef<PILValue>(argValue));
   return SGF.emitManagedRValueWithCleanup(resultVal);
}

Optional<SpecializedEmitter>
SpecializedEmitter::forDecl(PILGenModule &SGM, PILDeclRef function) {
   // Only consider standalone declarations in the Builtin module.
   if (function.kind != PILDeclRef::Kind::Func)
      return None;
   if (!function.hasDecl())
      return None;
   ValueDecl *decl = function.getDecl();
   if (!isa<BuiltinUnit>(decl->getDeclContext()))
      return None;

   auto name = decl->getBaseName().getIdentifier();
   const BuiltinInfo &builtin = SGM.M.getBuiltinInfo(name);
   switch (builtin.ID) {
      // All the non-PIL, non-type-trait builtins should use the
      // named-builtin logic, which just emits the builtin as a call to a
      // builtin function.  This includes builtins that aren't even declared
      // in Builtins.def, i.e. all of the LLVM intrinsics.
      //
      // We do this in a separate pass over Builtins.def to avoid creating
      // a bunch of identical cases.
#define BUILTIN(Id, Name, Attrs)                                            \
  case BuiltinValueKind::Id:
#define BUILTIN_PIL_OPERATION(Id, Name, Overload)
#define BUILTIN_MISC_OPERATION_WITH_PILGEN(Id, Name, Attrs, Overload)
#define BUILTIN_SANITIZER_OPERATION(Id, Name, Attrs)
#define BUILTIN_TYPE_CHECKER_OPERATION(Id, Name)
#define BUILTIN_TYPE_TRAIT_OPERATION(Id, Name)
#include "polarphp/ast/BuiltinsDef.h"
      case BuiltinValueKind::None:
         return SpecializedEmitter(name);

         // Do a second pass over Builtins.def, ignoring all the cases for
         // which we emitted something above.
#define BUILTIN(Id, Name, Attrs)

         // Use specialized emitters for PIL builtins.
#define BUILTIN_PIL_OPERATION(Id, Name, Overload)                           \
  case BuiltinValueKind::Id:                                                \
    return SpecializedEmitter(&emitBuiltin##Id);

#define BUILTIN_MISC_OPERATION_WITH_PILGEN(Id, Name, Attrs, Overload)          \
  case BuiltinValueKind::Id:                                                   \
    return SpecializedEmitter(&emitBuiltin##Id);

         // Sanitizer builtins should never directly be called; they should only
         // be inserted as instrumentation by PILGen.
#define BUILTIN_SANITIZER_OPERATION(Id, Name, Attrs)                        \
  case BuiltinValueKind::Id:                                                \
    llvm_unreachable("Sanitizer builtin called directly?");

#define BUILTIN_TYPE_CHECKER_OPERATION(Id, Name)                               \
  case BuiltinValueKind::Id:                                                   \
    llvm_unreachable(                                                          \
        "Compile-time type checker operation should not make it to PIL!");

         // Lower away type trait builtins when they're trivially solvable.
#define BUILTIN_TYPE_TRAIT_OPERATION(Id, Name)                              \
  case BuiltinValueKind::Id:                                                \
    return SpecializedEmitter(&emitBuiltinTypeTrait<&TypeBase::Name,        \
                                                    BuiltinValueKind::Id>);

#include "polarphp/ast/BuiltinsDef.h"
   }
   llvm_unreachable("bad builtin kind");
}
