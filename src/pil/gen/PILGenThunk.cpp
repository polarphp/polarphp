//===--- PILGenThunk.cpp - PILGen for thunks ------------------------------===//
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
// This file contains code for emitting various types of thunks that can be
// referenced from code, such as dynamic thunks, curry thunks, native to foreign
// thunks and foreign to native thunks.
//
// VTable thunks and witness thunks can be found in PILGenType.cpp, and the
// meat of the bridging thunk implementation is in PILGenBridging.cpp, and
// re-abstraction thunks are in PILGenPoly.cpp.
//
//===----------------------------------------------------------------------===//

#include "polarphp/pil/gen/ManagedValue.h"
#include "polarphp/pil/gen/PILGenFunction.h"
#include "polarphp/pil/gen/PILGenFunctionBuilder.h"
#include "polarphp/pil/gen/Scope.h"
#include "polarphp/ast/ASTMangler.h"
#include "polarphp/ast/DiagnosticsPIL.h"
#include "polarphp/ast/FileUnit.h"
#include "polarphp/ast/GenericEnvironment.h"
#include "polarphp/pil/lang/PrettyStackTrace.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/TypeLowering.h"

using namespace polar;
using namespace lowering;

PILValue PILGenFunction::emitClassMethodRef(PILLocation loc,
                                            PILValue selfPtr,
                                            PILDeclRef constant,
                                            CanPILFunctionType constantTy) {
   assert(!constant.isForeign);
   return B.createClassMethod(loc, selfPtr, constant,
                              PILType::getPrimitiveObjectType(constantTy));
}

PILFunction *PILGenModule::getDynamicThunk(PILDeclRef constant,
                                           CanPILFunctionType constantTy) {
   assert(constant.kind != PILDeclRef::Kind::Allocator &&
          "allocating entry point for constructor is never dynamic");
   // Mangle the constant with a TD suffix.
   auto nameTmp = constant.mangle(PILDeclRef::ManglingKind::DynamicThunk);
   auto name = M.allocateCopy(nameTmp);

   PILGenFunctionBuilder builder(*this);
   auto F = builder.getOrCreateFunction(
      constant.getDecl(), name, PILLinkage::Shared, constantTy, IsBare,
      IsTransparent, IsSerializable, IsNotDynamic, ProfileCounter(), IsThunk);

   if (F->empty()) {
      // Emit the thunk if we haven't yet.
      // Currently a dynamic thunk looks just like a foreign-to-native thunk around
      // an ObjC method. This would change if we introduced a native
      // runtime-hookable mechanism.
      PILGenFunction SGF(*this, *F, PolarphpModule);
      SGF.emitForeignToNativeThunk(constant);
      emitLazyConformancesForFunction(F);
   }

   return F;
}

ManagedValue
PILGenFunction::emitDynamicMethodRef(PILLocation loc, PILDeclRef constant,
                                     CanPILFunctionType constantTy) {
   // If the method is foreign, its foreign thunk will handle the dynamic
   // dispatch for us.
   if (constant.isForeignToNativeThunk()) {
      if (!SGM.hasFunction(constant))
         SGM.emitForeignToNativeThunk(constant);
      return ManagedValue::forUnmanaged(B.createFunctionRefFor(
         loc, SGM.getFunction(constant, NotForDefinition)));
   }

   // Otherwise, we need a dynamic dispatch thunk.
   PILFunction *F = SGM.getDynamicThunk(constant, constantTy);

   return ManagedValue::forUnmanaged(B.createFunctionRefFor(loc, F));
}

static std::pair<ManagedValue, PILDeclRef>
getNextUncurryLevelRef(PILGenFunction &SGF, PILLocation loc, PILDeclRef thunk,
                       ManagedValue selfArg, SubstitutionMap curriedSubs) {
   auto *vd = thunk.getDecl();

   // Reference the next uncurrying level of the function.
   PILDeclRef next = PILDeclRef(vd, thunk.kind);
   assert(!next.isCurried);

   auto constantInfo =
      SGF.SGM.Types.getConstantInfo(SGF.getTypeExpansionContext(), next);

   // If the function is natively foreign, reference its foreign entry point.
   if (requiresForeignToNativeThunk(vd))
      return {ManagedValue::forUnmanaged(SGF.emitGlobalFunctionRef(loc, next)),
              next};

   // If the thunk is a curry thunk for a direct method reference, we are
   // doing a direct dispatch (eg, a fragile 'super.foo()' call).
   if (thunk.isDirectReference)
      return {ManagedValue::forUnmanaged(SGF.emitGlobalFunctionRef(loc, next)),
              next};

   if (auto *func = dyn_cast<AbstractFunctionDecl>(vd)) {
      if (getMethodDispatch(func) == MethodDispatch::Class) {
         // Use the dynamic thunk if dynamic.
         // @todo
//         if (vd->isObjCDynamic()) {
//            return {SGF.emitDynamicMethodRef(loc, next, constantInfo.PILFnType),
//                    next};
//         }

         auto methodTy = SGF.SGM.Types.getConstantOverrideType(
            SGF.getTypeExpansionContext(), next);
         PILValue result =
            SGF.emitClassMethodRef(loc, selfArg.getValue(), next, methodTy);
         return {ManagedValue::forUnmanaged(result),
                 next.getOverriddenVTableEntry()};
      }

      // If the fully-uncurried reference is to a generic method, look up the
      // witness.
      if (constantInfo.PILFnType->getRepresentation()
          == PILFunctionTypeRepresentation::WitnessMethod) {
         auto protocol = func->getDeclContext()->getSelfInterfaceDecl();
         auto origSelfType = protocol->getSelfInterfaceType()->getCanonicalType();
         auto substSelfType = origSelfType.subst(curriedSubs)->getCanonicalType();
         auto conformance = curriedSubs.lookupConformance(origSelfType, protocol);
         auto result = SGF.B.createWitnessMethod(loc, substSelfType, conformance,
                                                 next, constantInfo.getPILType());
         return {ManagedValue::forUnmanaged(result), next};
      }
   }

   // Otherwise, emit a direct call.
   return {ManagedValue::forUnmanaged(SGF.emitGlobalFunctionRef(loc, next)),
           next};
}

void PILGenFunction::emitCurryThunk(PILDeclRef thunk) {
   assert(thunk.isCurried);

   auto *vd = thunk.getDecl();

   if (auto *fd = dyn_cast<AbstractFunctionDecl>(vd)) {
      assert(!SGM.M.Types.hasLoweredLocalCaptures(PILDeclRef(fd)) &&
             "methods cannot have captures");
      (void) fd;
   }

   PILLocation loc(vd);
   Scope S(*this, vd);

   auto thunkInfo = SGM.Types.getConstantInfo(getTypeExpansionContext(), thunk);
   auto thunkFnTy = thunkInfo.PILFnType;
   PILFunctionConventions fromConv(thunkFnTy, SGM.M);

   auto selfTy = fromConv.getPILType(thunkFnTy->getSelfParameter());
   selfTy = F.mapTypeIntoContext(selfTy);
   ManagedValue selfArg = B.createInputFunctionArgument(selfTy, loc);

   // Forward substitutions.
   auto subs = F.getForwardingSubstitutionMap();

   auto toFnAndRef = getNextUncurryLevelRef(*this, loc, thunk, selfArg, subs);
   ManagedValue toFn = toFnAndRef.first;
   PILDeclRef calleeRef = toFnAndRef.second;

   PILType resultTy = fromConv.getSinglePILResultType();
   resultTy = F.mapTypeIntoContext(resultTy);

   // Partially apply the next uncurry level and return the result closure.
   selfArg = selfArg.ensurePlusOne(*this, loc);
   auto calleeConvention = ParameterConvention::Direct_Guaranteed;
   ManagedValue toClosure =
      B.createPartialApply(loc, toFn, subs, {selfArg},
                           calleeConvention);
   if (resultTy != toClosure.getType()) {
      CanPILFunctionType resultFnTy = resultTy.castTo<PILFunctionType>();
      CanPILFunctionType closureFnTy = toClosure.getType().castTo<PILFunctionType>();
      if (resultFnTy->isABICompatibleWith(closureFnTy, F).isCompatible()) {
         toClosure = B.createConvertFunction(loc, toClosure, resultTy);
      } else {
         // Compute the partially-applied abstraction pattern for the callee:
         // just grab the pattern for the curried fn ref and "call" it.
         assert(!calleeRef.isCurried);
         calleeRef.isCurried = true;
         auto appliedFnPattern =
            SGM.Types.getConstantInfo(getTypeExpansionContext(), calleeRef)
               .FormalPattern.getFunctionResultType();

         auto appliedThunkPattern =
            thunkInfo.FormalPattern.getFunctionResultType();

         // The formal type should be the same for the callee and the thunk.
         auto formalType = thunkInfo.FormalType;
         if (auto genericSubstType = dyn_cast<GenericFunctionType>(formalType)) {
            formalType = genericSubstType.substGenericArgs(subs);
         }
         formalType = cast<AnyFunctionType>(formalType.getResult());

         toClosure =
            emitTransformedValue(loc, toClosure,
                                 appliedFnPattern, formalType,
                                 appliedThunkPattern, formalType);
      }
   }
   toClosure = S.popPreservingValue(toClosure);
   B.createReturn(ImplicitReturnLocation::getImplicitReturnLoc(loc), toClosure);
}

void PILGenModule::emitCurryThunk(PILDeclRef constant) {
   assert(constant.isCurried);

   // Thunks are always emitted by need, so don't need delayed emission.
   PILFunction *f = getFunction(constant, ForDefinition);
   f->setThunk(IsThunk);
   f->setBare(IsBare);

   auto *fd = constant.getDecl();
   preEmitFunction(constant, fd, f, fd);
   PrettyStackTracePILFunction X("pilgen emitCurryThunk", f);

   PILGenFunction(*this, *f, PolarphpModule).emitCurryThunk(constant);
   postEmitFunction(constant, f);
}

void PILGenModule::emitForeignToNativeThunk(PILDeclRef thunk) {
   // Thunks are always emitted by need, so don't need delayed emission.
   assert(!thunk.isForeign && "foreign-to-native thunks only");
   PILFunction *f = getFunction(thunk, ForDefinition);
   f->setThunk(IsThunk);
   if (thunk.asForeign().isClangGenerated())
      f->setSerialized(IsSerializable);
   preEmitFunction(thunk, thunk.getDecl(), f, thunk.getDecl());
   PrettyStackTracePILFunction X("pilgen emitForeignToNativeThunk", f);
   PILGenFunction(*this, *f, PolarphpModule).emitForeignToNativeThunk(thunk);
   postEmitFunction(thunk, f);
}

void PILGenModule::emitNativeToForeignThunk(PILDeclRef thunk) {
   // Thunks are always emitted by need, so don't need delayed emission.
   assert(thunk.isForeign && "native-to-foreign thunks only");

   PILFunction *f = getFunction(thunk, ForDefinition);
   if (thunk.hasDecl())
      preEmitFunction(thunk, thunk.getDecl(), f, thunk.getDecl());
   else
      preEmitFunction(thunk, thunk.getAbstractClosureExpr(), f,
                      thunk.getAbstractClosureExpr());
   PrettyStackTracePILFunction X("pilgen emitNativeToForeignThunk", f);
   f->setBare(IsBare);
   f->setThunk(IsThunk);
   PILGenFunction(*this, *f, PolarphpModule).emitNativeToForeignThunk(thunk);
   postEmitFunction(thunk, f);
}

PILValue
PILGenFunction::emitGlobalFunctionRef(PILLocation loc, PILDeclRef constant,
                                      PILConstantInfo constantInfo,
                                      bool callPreviousDynamicReplaceableImpl) {
   assert(constantInfo == getConstantInfo(getTypeExpansionContext(), constant));

   // Builtins must be fully applied at the point of reference.
   if (constant.hasDecl() &&
       isa<BuiltinUnit>(constant.getDecl()->getDeclContext())) {
      SGM.diagnose(loc.getSourceLoc(), diag::not_implemented,
                   "delayed application of builtin");
      return PILUndef::get(constantInfo.getPILType(), F);
   }

   // If the constant is a thunk we haven't emitted yet, emit it.
   if (!SGM.hasFunction(constant)) {
      if (constant.isCurried) {
         SGM.emitCurryThunk(constant);
      } else if (constant.isForeignToNativeThunk()) {
         SGM.emitForeignToNativeThunk(constant);
      } else if (constant.isNativeToForeignThunk()) {
         SGM.emitNativeToForeignThunk(constant);
      } else if (constant.kind == PILDeclRef::Kind::EnumElement) {
         SGM.emitEnumConstructor(cast<EnumElementDecl>(constant.getDecl()));
      }
   }

   auto f = SGM.getFunction(constant, NotForDefinition);
   assert(f->getLoweredFunctionTypeInContext(B.getTypeExpansionContext()) ==
          constantInfo.PILFnType);
   if (callPreviousDynamicReplaceableImpl)
      return B.createPreviousDynamicFunctionRef(loc, f);
   else
      return B.createFunctionRefFor(loc, f);
}

PILFunction *PILGenModule::
getOrCreateReabstractionThunk(CanPILFunctionType thunkType,
                              CanPILFunctionType fromType,
                              CanPILFunctionType toType,
                              CanType dynamicSelfType) {
   // The reference to the thunk is likely @noescape, but declarations are always
   // escaping.
   auto thunkDeclType =
      thunkType->getWithExtInfo(thunkType->getExtInfo().withNoEscape(false));

   // Mangle the reabstraction thunk.
   // Substitute context parameters out of the "from" and "to" types.
   auto fromInterfaceType = fromType->mapTypeOutOfContext()
      ->getCanonicalType();
   auto toInterfaceType = toType->mapTypeOutOfContext()
      ->getCanonicalType();
   CanType dynamicSelfInterfaceType;
   if (dynamicSelfType)
      dynamicSelfInterfaceType = dynamicSelfType->mapTypeOutOfContext()
         ->getCanonicalType();

   mangle::AstMangler NewMangler;
   std::string name = NewMangler.mangleReabstractionThunkHelper(thunkType,
                                                                fromInterfaceType, toInterfaceType,
                                                                dynamicSelfInterfaceType,
                                                                M.getTypePHPModule());

   auto loc = RegularLocation::getAutoGeneratedLocation();

   PILGenFunctionBuilder builder(*this);
   return builder.getOrCreateSharedFunction(
      loc, name, thunkDeclType, IsBare, IsTransparent, IsSerializable,
      ProfileCounter(), IsReabstractionThunk, IsNotDynamic);
}
