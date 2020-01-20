//===--- Devirtualize.cpp - Helper for devirtualizing apply ---------------===//
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

#define DEBUG_TYPE "pil-devirtualize-utility"

#include "polarphp/pil/optimizer/analysis/ClassHierarchyAnalysis.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "polarphp/pil/optimizer/utils/Devirtualize.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/GenericSignature.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/ast/SubstitutionMap.h"
#include "polarphp/ast/Types.h"
#include "polarphp/pil/lang/InstructionUtils.h"
#include "polarphp/pil/lang/OptimizationRemark.h"
#include "polarphp/pil/lang/PILDeclRef.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/lang/PILType.h"
#include "polarphp/pil/lang/PILValue.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Casting.h"

using namespace polar;

STATISTIC(NumClassDevirt, "Number of class_method applies devirtualized");
STATISTIC(NumWitnessDevirt, "Number of witness_method applies devirtualized");

//===----------------------------------------------------------------------===//
//                         Class Method Optimization
//===----------------------------------------------------------------------===//

void polar::getAllSubclasses(ClassHierarchyAnalysis *cha, ClassDecl *cd,
                             CanType classType, PILModule &module,
                             ClassHierarchyAnalysis::ClassList &subs) {
   // Collect the direct and indirect subclasses for the class.
   // Sort these subclasses in the order they should be tested by the
   // speculative devirtualization. Different strategies could be used,
   // E.g. breadth-first, depth-first, etc.
   // Currently, let's use the breadth-first strategy.
   // The exact static type of the instance should be tested first.
   auto &directSubs = cha->getDirectSubClasses(cd);
   auto &indirectSubs = cha->getIndirectSubClasses(cd);

   subs.append(directSubs.begin(), directSubs.end());
   subs.append(indirectSubs.begin(), indirectSubs.end());

   // FIXME: This is wrong -- we could have a non-generic class nested
   // inside a generic class
   if (isa<BoundGenericClassType>(classType)) {
      // Filter out any subclasses that do not inherit from this
      // specific bound class.
      auto removedIt =
         std::remove_if(subs.begin(), subs.end(), [&classType](ClassDecl *sub) {
            // FIXME: Add support for generic subclasses.
            if (sub->isGenericContext())
               return false;
            auto subCanTy = sub->getDeclaredInterfaceType()->getCanonicalType();
            // Handle the usual case here: the class in question
            // should be a real subclass of a bound generic class.
            return !classType->isBindableToSuperclassOf(subCanTy);
         });
      subs.erase(removedIt, subs.end());
   }
}

/// Returns true, if a method implementation corresponding to
/// the class_method applied to an instance of the class cd is
/// effectively final, i.e. it is statically known to be not overridden
/// by any subclasses of the class cd.
///
/// \p applySite  invocation instruction
/// \p classType type of the instance
/// \p cd  static class of the instance whose method is being invoked
/// \p cha class hierarchy analysis
static bool isEffectivelyFinalMethod(FullApplySite applySite, CanType classType,
                                     ClassDecl *cd,
                                     ClassHierarchyAnalysis *cha) {
   if (cd && cd->isFinal())
      return true;

   const DeclContext *dc = applySite.getModule().getAssociatedContext();

   // Without an associated context we cannot perform any
   // access-based optimizations.
   if (!dc)
      return false;

   auto *cmi = cast<MethodInst>(applySite.getCallee());

   if (!calleesAreStaticallyKnowable(applySite.getModule(), cmi->getMember()))
      return false;

   auto *method = cmi->getMember().getAbstractFunctionDecl();
   assert(method && "Expected abstract function decl!");
   assert(!method->isFinal() && "Unexpected indirect call to final method!");

   // If this method is not overridden in the module,
   // there is no other implementation.
   if (!method->isOverridden())
      return true;

   // Class declaration may be nullptr, e.g. for cases like:
   // func foo<C:Base>(c: C) {}, where C is a class, but
   // it does not have a class decl.
   if (!cd)
      return false;

   if (!cha)
      return false;

   // We can analyze the class hierarchy rooted at this class and
   // eventually devirtualize a method call more efficiently.

   ClassHierarchyAnalysis::ClassList subs;
   getAllSubclasses(cha, cd, classType, applySite.getModule(), subs);

   // This is the implementation of the method to be used
   // if the exact class of the instance would be cd.
   auto *ImplMethod = cd->findImplementingMethod(method);

   // First, analyze all direct subclasses.
   for (auto S : subs) {
      // Check if the subclass overrides a method and provides
      // a different implementation.
      auto *ImplFD = S->findImplementingMethod(method);
      if (ImplFD != ImplMethod)
         return false;
   }

   return true;
}

/// Check if a given class is final in terms of a current
/// compilation, i.e.:
/// - it is really final
/// - or it is private and has not sub-classes
/// - or it is an internal class without sub-classes and
///   it is a whole-module compilation.
static bool isKnownFinalClass(ClassDecl *cd, PILModule &module,
                              ClassHierarchyAnalysis *cha) {
   const DeclContext *dc = module.getAssociatedContext();

   if (cd->isFinal())
      return true;

   // Without an associated context we cannot perform any
   // access-based optimizations.
   if (!dc)
      return false;

   // Only handle classes defined within the PILModule's associated context.
   if (!cd->isChildContextOf(dc))
      return false;

   if (!cd->hasAccess())
      return false;

   // Only consider 'private' members, unless we are in whole-module compilation.
   switch (cd->getEffectiveAccess()) {
      case AccessLevel::Open:
         return false;
      case AccessLevel::Public:
      case AccessLevel::Internal:
         if (!module.isWholeModule())
            return false;
         break;
      case AccessLevel::FilePrivate:
      case AccessLevel::Private:
         break;
   }

   // Take the ClassHierarchyAnalysis into account.
   // If a given class has no subclasses and
   // - private
   // - or internal and it is a WMO compilation
   // then this class can be considered final for the purpose
   // of devirtualization.
   if (cha) {
      if (!cha->hasKnownDirectSubclasses(cd)) {
         switch (cd->getEffectiveAccess()) {
            case AccessLevel::Open:
               return false;
            case AccessLevel::Public:
            case AccessLevel::Internal:
               if (!module.isWholeModule())
                  return false;
               break;
            case AccessLevel::FilePrivate:
            case AccessLevel::Private:
               break;
         }

         return true;
      }
   }

   return false;
}

// Attempt to get the instance for S, whose static type is the same as
// its exact dynamic type, returning a null PILValue() if we cannot find it.
// The information that a static type is the same as the exact dynamic,
// can be derived e.g.:
// - from a constructor or
// - from a successful outcome of a checked_cast_br [exact] instruction.
PILValue polar::getInstanceWithExactDynamicType(PILValue instance,
                                                ClassHierarchyAnalysis *cha) {
   auto *f = instance->getFunction();
   auto &module = f->getModule();

   while (instance) {
      instance = stripCasts(instance);

      if (isa<AllocRefInst>(instance) || isa<MetatypeInst>(instance)) {
         if (instance->getType().getAstType()->hasDynamicSelfType())
            return PILValue();
         return instance;
      }

      auto *arg = dyn_cast<PILArgument>(instance);
      if (!arg)
         break;

      auto *singlePred = arg->getParent()->getSinglePredecessorBlock();
      if (!singlePred) {
         if (!isa<PILFunctionArgument>(arg))
            break;
         auto *cd = arg->getType().getClassOrBoundGenericClass();
         // Check if this class is effectively final.
         if (!cd || !isKnownFinalClass(cd, module, cha))
            break;
         return arg;
      }

      // Traverse the chain of predecessors.
      if (isa<BranchInst>(singlePred->getTerminator())
          || isa<CondBranchInst>(singlePred->getTerminator())) {
         instance = cast<PILPhiArgument>(arg)->getIncomingPhiValue(singlePred);
         continue;
      }

      // If it is a BB argument received on a success branch
      // of a checked_cast_br, then we know its exact type.
      auto *ccbi = dyn_cast<CheckedCastBranchInst>(singlePred->getTerminator());
      if (!ccbi)
         break;
      if (!ccbi->isExact() || ccbi->getSuccessBB() != arg->getParent())
         break;
      return instance;
   }

   return PILValue();
}

/// Try to determine the exact dynamic type of an object.
/// returns the exact dynamic type of the object, or an empty type if the exact
/// type could not be determined.
PILType polar::getExactDynamicType(PILValue instance,
                                   ClassHierarchyAnalysis *cha,
                                   bool forUnderlyingObject) {
   auto *f = instance->getFunction();
   auto &module = f->getModule();

   // Set of values to be checked for their exact types.
   SmallVector<PILValue, 8> worklist;
   // The detected type of the underlying object.
   PILType resultType;
   // Set of processed values.
   llvm::SmallSet<PILValue, 8> processed;
   worklist.push_back(instance);

   while (!worklist.empty()) {
      auto v = worklist.pop_back_val();
      if (!v)
         return PILType();
      if (processed.count(v))
         continue;
      processed.insert(v);
      // For underlying object strip casts and projections.
      // For the object itself, simply strip casts.
      v = forUnderlyingObject ? getUnderlyingObject(v) : stripCasts(v);

      if (isa<AllocRefInst>(v) || isa<MetatypeInst>(v)) {
         if (resultType && resultType != v->getType())
            return PILType();
         resultType = v->getType();
         continue;
      }

      if (isa<LiteralInst>(v)) {
         if (resultType && resultType != v->getType())
            return PILType();
         resultType = v->getType();
         continue;
      }

      if (isa<StructInst>(v) || isa<TupleInst>(v) || isa<EnumInst>(v)) {
         if (resultType && resultType != v->getType())
            return PILType();
         resultType = v->getType();
         continue;
      }

      if (forUnderlyingObject) {
         if (isa<AllocationInst>(v)) {
            if (resultType && resultType != v->getType())
               return PILType();
            resultType = v->getType();
            continue;
         }
      }

      auto arg = dyn_cast<PILArgument>(v);
      if (!arg) {
         // We don't know what it is.
         return PILType();
      }

      if (auto *fArg = dyn_cast<PILFunctionArgument>(arg)) {
         // Bail on metatypes for now.
         if (fArg->getType().is<AnyMetatypeType>()) {
            return PILType();
         }
         auto *cd = fArg->getType().getClassOrBoundGenericClass();
         // If it is not class and it is a trivial type, then it
         // should be the exact type.
         if (!cd && fArg->getType().isTrivial(*f)) {
            if (resultType && resultType != fArg->getType())
               return PILType();
            resultType = fArg->getType();
            continue;
         }

         if (!cd) {
            // It is not a class or a trivial type, so we don't know what it is.
            return PILType();
         }

         // Check if this class is effectively final.
         if (!isKnownFinalClass(cd, module, cha)) {
            return PILType();
         }

         if (resultType && resultType != fArg->getType())
            return PILType();
         resultType = fArg->getType();
         continue;
      }

      auto *singlePred = arg->getParent()->getSinglePredecessorBlock();
      if (singlePred) {
         // If it is a BB argument received on a success branch
         // of a checked_cast_br, then we know its exact type.
         auto *ccbi = dyn_cast<CheckedCastBranchInst>(singlePred->getTerminator());
         if (ccbi && ccbi->isExact() && ccbi->getSuccessBB() == arg->getParent()) {
            if (resultType && resultType != arg->getType())
               return PILType();
            resultType = arg->getType();
            continue;
         }
      }

      // It is a BB argument, look through incoming values. If they all have the
      // same exact type, then we consider it to be the type of the BB argument.
      SmallVector<PILValue, 4> incomingValues;
      if (arg->getSingleTerminatorOperands(incomingValues)) {
         for (auto inValue : incomingValues) {
            worklist.push_back(inValue);
         }
         continue;
      }

      // The exact type is unknown.
      return PILType();
   }

   return resultType;
}

/// Try to determine the exact dynamic type of the underlying object.
/// returns the exact dynamic type of a value, or an empty type if the exact
/// type could not be determined.
PILType
polar::getExactDynamicTypeOfUnderlyingObject(PILValue instance,
                                             ClassHierarchyAnalysis *cha) {
   return getExactDynamicType(instance, cha, /* forUnderlyingObject */ true);
}

// Start with the substitutions from the apply.
// Try to propagate them to find out the real substitutions required
// to invoke the method.
static SubstitutionMap
getSubstitutionsForCallee(PILModule &module, CanPILFunctionType baseCalleeType,
                          CanType derivedSelfType, FullApplySite applySite) {

   // If the base method is not polymorphic, no substitutions are required,
   // even if we originally had substitutions for calling the derived method.
   if (!baseCalleeType->isPolymorphic())
      return SubstitutionMap();

   // Add any generic substitutions for the base class.
   Type baseSelfType = baseCalleeType->getSelfParameter()
      .getArgumentType(module, baseCalleeType);
   if (auto metatypeType = baseSelfType->getAs<MetatypeType>())
      baseSelfType = metatypeType->getInstanceType();

   auto *baseClassDecl = baseSelfType->getClassOrBoundGenericClass();
   assert(baseClassDecl && "not a class method");

   unsigned baseDepth = 0;
   SubstitutionMap baseSubMap;
   if (auto baseClassSig = baseClassDecl->getGenericSignatureOfContext()) {
      baseDepth = baseClassSig->getGenericParams().back()->getDepth() + 1;

      // Compute the type of the base class, starting from the
      // derived class type and the type of the method's self
      // parameter.
      Type derivedClass = derivedSelfType;
      if (auto metatypeType = derivedClass->getAs<MetatypeType>())
         derivedClass = metatypeType->getInstanceType();
      baseSubMap = derivedClass->getContextSubstitutionMap(
         module.getTypePHPModule(), baseClassDecl);
   }

   SubstitutionMap origSubMap = applySite.getSubstitutionMap();

   Type calleeSelfType =
      applySite.getOrigCalleeType()->getSelfParameter()
         .getArgumentType(module, applySite.getOrigCalleeType());
   if (auto metatypeType = calleeSelfType->getAs<MetatypeType>())
      calleeSelfType = metatypeType->getInstanceType();
   auto *calleeClassDecl = calleeSelfType->getClassOrBoundGenericClass();
   assert(calleeClassDecl && "self is not a class type");

   // Add generic parameters from the method itself, ignoring any generic
   // parameters from the derived class.
   unsigned origDepth = 0;
   if (auto calleeClassSig = calleeClassDecl->getGenericSignatureOfContext())
      origDepth = calleeClassSig->getGenericParams().back()->getDepth() + 1;

   auto baseCalleeSig = baseCalleeType->getInvocationGenericSignature();

   return
      SubstitutionMap::combineSubstitutionMaps(baseSubMap,
                                               origSubMap,
                                               CombineSubstitutionMaps::AtDepth,
                                               baseDepth,
                                               origDepth,
                                               baseCalleeSig);
}

static ApplyInst *replaceApplyInst(PILBuilder &builder, PILLocation loc,
                                   ApplyInst *oldAI, PILValue newFn,
                                   SubstitutionMap newSubs,
                                   ArrayRef<PILValue> newArgs,
                                   ArrayRef<PILValue> newArgBorrows) {
   auto *newAI =
      builder.createApply(loc, newFn, newSubs, newArgs, oldAI->isNonThrowing());

   if (!newArgBorrows.empty()) {
      for (PILValue arg : newArgBorrows) {
         builder.createEndBorrow(loc, arg);
      }
   }

   // Check if any casting is required for the return value.
   PILValue resultValue = castValueToABICompatibleType(
      &builder, loc, newAI, newAI->getType(), oldAI->getType());

   oldAI->replaceAllUsesWith(resultValue);
   return newAI;
}

static TryApplyInst *replaceTryApplyInst(PILBuilder &builder, PILLocation loc,
                                         TryApplyInst *oldTAI, PILValue newFn,
                                         SubstitutionMap newSubs,
                                         ArrayRef<PILValue> newArgs,
                                         PILFunctionConventions conv,
                                         ArrayRef<PILValue> newArgBorrows) {
   PILBasicBlock *normalBB = oldTAI->getNormalBB();
   PILBasicBlock *resultBB = nullptr;

   PILType newResultTy = conv.getPILResultType();

   // Does the result value need to be casted?
   auto oldResultTy = normalBB->getArgument(0)->getType();
   bool resultCastRequired = newResultTy != oldResultTy;

   // Create a new normal BB only if the result of the new apply differs
   // in type from the argument of the original normal BB.
   if (!resultCastRequired) {
      resultBB = normalBB;
   } else {
      resultBB = builder.getFunction().createBasicBlockBefore(normalBB);
      resultBB->createPhiArgument(newResultTy, ValueOwnershipKind::Owned);
   }

   // We can always just use the original error BB because we'll be
   // deleting the edge to it from the old TAI.
   PILBasicBlock *errorBB = oldTAI->getErrorBB();

   // Insert a try_apply here.
   // Note that this makes this block temporarily double-terminated!
   // We won't fix that until deleteDevirtualizedApply.
   auto newTAI =
      builder.createTryApply(loc, newFn, newSubs, newArgs, resultBB, errorBB);

   if (!newArgBorrows.empty()) {
      builder.setInsertionPoint(normalBB->begin());
      for (PILValue arg : newArgBorrows) {
         builder.createEndBorrow(loc, arg);
      }
      builder.setInsertionPoint(errorBB->begin());
      for (PILValue arg : newArgBorrows) {
         builder.createEndBorrow(loc, arg);
      }
   }

   if (resultCastRequired) {
      builder.setInsertionPoint(resultBB);

      PILValue resultValue = resultBB->getArgument(0);
      resultValue = castValueToABICompatibleType(&builder, loc, resultValue,
                                                 newResultTy, oldResultTy);
      builder.createBranch(loc, normalBB, {resultValue});
   }

   builder.setInsertionPoint(normalBB->begin());
   return newTAI;
}

static BeginApplyInst *
replaceBeginApplyInst(PILBuilder &builder, PILLocation loc,
                      BeginApplyInst *oldBAI, PILValue newFn,
                      SubstitutionMap newSubs, ArrayRef<PILValue> newArgs,
                      ArrayRef<PILValue> newArgBorrows) {
   auto *newBAI = builder.createBeginApply(loc, newFn, newSubs, newArgs,
                                           oldBAI->isNonThrowing());

   // Forward the token.
   oldBAI->getTokenResult()->replaceAllUsesWith(newBAI->getTokenResult());

   auto oldYields = oldBAI->getYieldedValues();
   auto newYields = newBAI->getYieldedValues();
   assert(oldYields.size() == newYields.size());

   for (auto i : indices(oldYields)) {
      auto oldYield = oldYields[i];
      auto newYield = newYields[i];
      newYield = castValueToABICompatibleType(
         &builder, loc, newYield, newYield->getType(), oldYield->getType());
      oldYield->replaceAllUsesWith(newYield);
   }

   if (newArgBorrows.empty())
      return newBAI;

   PILValue token = newBAI->getTokenResult();

   // The token will only be used by end_apply and abort_apply. Use that to
   // insert the end_borrows we need /after/ those uses.
   for (auto *use : token->getUses()) {
      PILBuilderWithScope borrowBuilder(
         &*std::next(use->getUser()->getIterator()),
         builder.getBuilderContext());
      for (PILValue borrow : newArgBorrows) {
         borrowBuilder.createEndBorrow(loc, borrow);
      }
   }

   return newBAI;
}

static PartialApplyInst *
replacePartialApplyInst(PILBuilder &builder, PILLocation loc,
                        PartialApplyInst *oldPAI, PILValue newFn,
                        SubstitutionMap newSubs, ArrayRef<PILValue> newArgs) {
   auto convention =
      oldPAI->getType().getAs<PILFunctionType>()->getCalleeConvention();
   auto *newPAI =
      builder.createPartialApply(loc, newFn, newSubs, newArgs, convention);

   // Check if any casting is required for the partially-applied function.
   PILValue resultValue = castValueToABICompatibleType(
      &builder, loc, newPAI, newPAI->getType(), oldPAI->getType());
   oldPAI->replaceAllUsesWith(resultValue);

   return newPAI;
}

static ApplySite replaceApplySite(PILBuilder &builder, PILLocation loc,
                                  ApplySite oldAS, PILValue newFn,
                                  SubstitutionMap newSubs,
                                  ArrayRef<PILValue> newArgs,
                                  PILFunctionConventions conv,
                                  ArrayRef<PILValue> newArgBorrows) {
   switch (oldAS.getKind()) {
      case ApplySiteKind::ApplyInst: {
         auto *oldAI = cast<ApplyInst>(oldAS);
         return replaceApplyInst(builder, loc, oldAI, newFn, newSubs, newArgs,
                                 newArgBorrows);
      }
      case ApplySiteKind::TryApplyInst: {
         auto *oldTAI = cast<TryApplyInst>(oldAS);
         return replaceTryApplyInst(builder, loc, oldTAI, newFn, newSubs, newArgs,
                                    conv, newArgBorrows);
      }
      case ApplySiteKind::BeginApplyInst: {
         auto *oldBAI = dyn_cast<BeginApplyInst>(oldAS);
         return replaceBeginApplyInst(builder, loc, oldBAI, newFn, newSubs, newArgs,
                                      newArgBorrows);
      }
      case ApplySiteKind::PartialApplyInst: {
         assert(newArgBorrows.empty());
         auto *oldPAI = cast<PartialApplyInst>(oldAS);
         return replacePartialApplyInst(builder, loc, oldPAI, newFn, newSubs,
                                        newArgs);
      }
   }
   llvm_unreachable("covered switch");
}

/// Delete an apply site that's been successfully devirtualized.
void polar::deleteDevirtualizedApply(ApplySite old) {
   auto *oldApply = old.getInstruction();
   recursivelyDeleteTriviallyDeadInstructions(oldApply, true);
}

PILFunction *polar::getTargetClassMethod(PILModule &module, ClassDecl *cd,
                                         MethodInst *mi) {
   assert((isa<ClassMethodInst>(mi) || isa<SuperMethodInst>(mi))
          && "Only class_method and super_method instructions are supported");

   PILDeclRef member = mi->getMember();
   return module.lookUpFunctionInVTable(cd, member);
}

CanType polar::getSelfInstanceType(CanType classOrMetatypeType) {
   if (auto metaType = dyn_cast<MetatypeType>(classOrMetatypeType))
      classOrMetatypeType = metaType.getInstanceType();

   if (auto selfType = dyn_cast<DynamicSelfType>(classOrMetatypeType))
      classOrMetatypeType = selfType.getSelfType();

   return classOrMetatypeType;
}

/// Check if it is possible to devirtualize an Apply instruction
/// and a class member obtained using the class_method instruction into
/// a direct call to a specific member of a specific class.
///
/// \p applySite is the apply to devirtualize.
/// \p cd is the class declaration we are devirtualizing for.
/// return true if it is possible to devirtualize, false - otherwise.
bool polar::canDevirtualizeClassMethod(FullApplySite applySite, ClassDecl *cd,
                                       optremark::Emitter *ore,
                                       bool isEffectivelyFinalMethod) {

   LLVM_DEBUG(llvm::dbgs() << "    Trying to devirtualize : "
                           << *applySite.getInstruction());

   PILModule &module = applySite.getModule();

   auto *mi = cast<MethodInst>(applySite.getCallee());

   // Find the implementation of the member which should be invoked.
   auto *f = getTargetClassMethod(module, cd, mi);

   // If we do not find any such function, we have no function to devirtualize
   // to... so bail.
   if (!f) {
      LLVM_DEBUG(llvm::dbgs() << "        FAIL: Could not find matching VTable "
                                 "or vtable method for this class.\n");
      return false;
   }

   // We need to disable the  “effectively final” opt if a function is inlinable
   if (isEffectivelyFinalMethod && applySite.getFunction()->isSerialized()) {
      LLVM_DEBUG(llvm::dbgs() << "        FAIL: Could not optimize function "
                                 "because it is an effectively-final inlinable: "
                              << applySite.getFunction()->getName() << "\n");
      return false;
   }

   // Mandatory inlining does class method devirtualization. I'm not sure if this
   // is really needed, but some test rely on this.
   // So even for Onone functions we have to do it if the PILStage is raw.
   if (f->getModule().getStage() != PILStage::Raw && !f->shouldOptimize()) {
      // Do not consider functions that should not be optimized.
      LLVM_DEBUG(llvm::dbgs()
                    << "        FAIL: Could not optimize function "
                    << " because it is marked no-opt: " << f->getName() << "\n");
      return false;
   }

   if (applySite.getFunction()->isSerialized()) {
      // function_ref inside fragile function cannot reference a private or
      // hidden symbol.
      if (!f->hasValidLinkageForFragileRef())
         return false;
   }

   // devirtualizeClassMethod below does not support this case. It currently
   // assumes it can try_apply call the target.
   if (!f->getLoweredFunctionType()->hasErrorResult()
       && isa<TryApplyInst>(applySite.getInstruction())) {
      LLVM_DEBUG(llvm::dbgs() << "        FAIL: Trying to devirtualize a "
                                 "try_apply but vtable entry has no error result.\n");
      return false;
   }

   return true;
}

/// Devirtualize an apply of a class method.
///
/// \p applySite is the apply to devirtualize.
/// \p ClassOrMetatype is a class value or metatype value that is the
///    self argument of the apply we will devirtualize.
/// return the result value of the new ApplyInst if created one or null.
FullApplySite polar::devirtualizeClassMethod(FullApplySite applySite,
                                             PILValue classOrMetatype,
                                             ClassDecl *cd,
                                             optremark::Emitter *ore) {
   LLVM_DEBUG(llvm::dbgs() << "    Trying to devirtualize : "
                           << *applySite.getInstruction());

   PILModule &module = applySite.getModule();
   auto *mi = cast<MethodInst>(applySite.getCallee());

   auto *f = getTargetClassMethod(module, cd, mi);

   CanPILFunctionType genCalleeType = f->getLoweredFunctionTypeInContext(
      TypeExpansionContext(*applySite.getFunction()));

   SubstitutionMap subs = getSubstitutionsForCallee(
      module, genCalleeType, classOrMetatype->getType().getAstType(),
      applySite);
   CanPILFunctionType substCalleeType = genCalleeType;
   if (genCalleeType->isPolymorphic())
      substCalleeType = genCalleeType->substGenericArgs(
         module, subs, TypeExpansionContext(*applySite.getFunction()));
   PILFunctionConventions substConv(substCalleeType, module);

   PILBuilderWithScope builder(applySite.getInstruction());
   PILLocation loc = applySite.getLoc();
   auto *fri = builder.createFunctionRefFor(loc, f);

   // Create the argument list for the new apply, casting when needed
   // in order to handle covariant indirect return types and
   // contravariant argument types.
   SmallVector<PILValue, 8> newArgs;

   // If we have a value that is owned, but that we are going to use in as a
   // guaranteed argument, we need to borrow/unborrow the argument. Otherwise, we
   // will introduce new consuming uses. In contrast, if we have an owned value,
   // we are ok due to the forwarding nature of upcasts.
   SmallVector<PILValue, 8> newArgBorrows;

   auto indirectResultArgIter = applySite.getIndirectPILResults().begin();
   for (auto resultTy : substConv.getIndirectPILResultTypes()) {
      newArgs.push_back(castValueToABICompatibleType(
         &builder, loc, *indirectResultArgIter, indirectResultArgIter->getType(),
         resultTy));
      ++indirectResultArgIter;
   }

   auto paramArgIter = applySite.getArgumentsWithoutIndirectResults().begin();
   // Skip the last parameter, which is `self`. Add it below.
   for (auto param : substConv.getParameters()) {
      auto paramType = substConv.getPILType(param);
      PILValue arg = *paramArgIter;
      if (builder.hasOwnership() && arg->getType().isObject()
          && arg.getOwnershipKind() == ValueOwnershipKind::Owned
          && param.isGuaranteed()) {
         PILBuilderWithScope borrowBuilder(applySite.getInstruction(), builder);
         arg = borrowBuilder.createBeginBorrow(loc, arg);
         newArgBorrows.push_back(arg);
      }
      arg = castValueToABICompatibleType(&builder, loc, arg,
                                         paramArgIter->getType(), paramType);
      newArgs.push_back(arg);
      ++paramArgIter;
   }
   ApplySite newAS = replaceApplySite(builder, loc, applySite, fri, subs,
                                      newArgs, substConv, newArgBorrows);
   FullApplySite newAI = FullApplySite::isa(newAS.getInstruction());
   assert(newAI);

   LLVM_DEBUG(llvm::dbgs() << "        SUCCESS: " << f->getName() << "\n");
   if (ore)
      ore->emit([&]() {
         using namespace optremark;
         return RemarkPassed("ClassMethodDevirtualized",
                             *applySite.getInstruction())
            << "Devirtualized call to class method " << NV("Method", f);
      });
   NumClassDevirt++;

   return newAI;
}

FullApplySite polar::tryDevirtualizeClassMethod(FullApplySite applySite,
                                                PILValue classInstance,
                                                ClassDecl *cd,
                                                optremark::Emitter *ore,
                                                bool isEffectivelyFinalMethod) {
   if (!canDevirtualizeClassMethod(applySite, cd, ore, isEffectivelyFinalMethod))
      return FullApplySite();
   return devirtualizeClassMethod(applySite, classInstance, cd, ore);
}

//===----------------------------------------------------------------------===//
//                        Witness Method Optimization
//===----------------------------------------------------------------------===//

/// Compute substitutions for making a direct call to a PIL function with
/// @convention(witness_method) convention.
///
/// Such functions have a substituted generic signature where the
/// abstract `Self` parameter from the original type of the protocol
/// requirement is replaced by a concrete type.
///
/// Thus, the original substitutions of the apply instruction that
/// are written in terms of the requirement's generic signature need
/// to be remapped to substitutions suitable for the witness signature.
///
/// Supported remappings are:
///
/// - (Concrete witness thunk) Original substitutions:
///   [Self := ConcreteType, R0 := X0, R1 := X1, ...]
/// - Requirement generic signature:
///   <Self : P, R0, R1, ...>
/// - Witness thunk generic signature:
///   <W0, W1, ...>
/// - Remapped substitutions:
///   [W0 := X0, W1 := X1, ...]
///
/// - (Class witness thunk) Original substitutions:
///   [Self := C<A0, A1>, T0 := X0, T1 := X1, ...]
/// - Requirement generic signature:
///   <Self : P, R0, R1, ...>
/// - Witness thunk generic signature:
///   <Self : C<B0, B1>, B0, B1, W0, W1, ...>
/// - Remapped substitutions:
///   [Self := C<B0, B1>, B0 := A0, B1 := A1, W0 := X0, W1 := X1]
///
/// - (Default witness thunk) Original substitutions:
///   [Self := ConcreteType, R0 := X0, R1 := X1, ...]
/// - Requirement generic signature:
///   <Self : P, R0, R1, ...>
/// - Witness thunk generic signature:
///   <Self : P, W0, W1, ...>
/// - Remapped substitutions:
///   [Self := ConcreteType, W0 := X0, W1 := X1, ...]
///
/// \param conformanceRef The (possibly-specialized) conformance
/// \param requirementSig The generic signature of the requirement
/// \param witnessThunkSig The generic signature of the witness method
/// \param origSubMap The substitutions from the call instruction
/// \param isSelfAbstract True if the Self type of the witness method is
/// still abstract (i.e., not a concrete type).
/// \param classWitness The ClassDecl if this is a class witness method
static SubstitutionMap
getWitnessMethodSubstitutions(
   ModuleDecl *mod,
   InterfaceConformanceRef conformanceRef,
   GenericSignature requirementSig,
   GenericSignature witnessThunkSig,
   SubstitutionMap origSubMap,
   bool isSelfAbstract,
   ClassDecl *classWitness) {

   if (witnessThunkSig.isNull())
      return SubstitutionMap();

   if (isSelfAbstract && !classWitness)
      return origSubMap;

   assert(!conformanceRef.isAbstract());
   auto conformance = conformanceRef.getConcrete();

   // If `Self` maps to a bound generic type, this gives us the
   // substitutions for the concrete type's generic parameters.
   auto baseSubMap = conformance->getSubstitutions(mod);

   unsigned baseDepth = 0;
   auto *rootConformance = conformance->getRootConformance();
   if (auto witnessSig = rootConformance->getGenericSignature())
      baseDepth = witnessSig->getGenericParams().back()->getDepth() + 1;

   // If the witness has a class-constrained 'Self' generic parameter,
   // we have to build a new substitution map that shifts all generic
   // parameters down by one.
   if (classWitness != nullptr) {
      auto *proto = conformance->getInterface();
      auto selfType = proto->getSelfInterfaceType();

      auto selfSubMap = SubstitutionMap::getInterfaceSubstitutions(
         proto, selfType.subst(origSubMap), conformanceRef);
      if (baseSubMap.empty()) {
         assert(baseDepth == 0);
         baseSubMap = selfSubMap;
      } else {
         baseSubMap = SubstitutionMap::combineSubstitutionMaps(
            selfSubMap,
            baseSubMap,
            CombineSubstitutionMaps::AtDepth,
            /*firstDepth=*/1,
            /*secondDepth=*/0,
            witnessThunkSig);
      }
      baseDepth += 1;
   }

   return SubstitutionMap::combineSubstitutionMaps(
      baseSubMap,
      origSubMap,
      CombineSubstitutionMaps::AtDepth,
      /*firstDepth=*/baseDepth,
      /*secondDepth=*/1,
      witnessThunkSig);
}

SubstitutionMap
polar::getWitnessMethodSubstitutions(PILModule &module, ApplySite applySite,
                                     PILFunction *f,
                                     InterfaceConformanceRef cRef) {
   auto witnessFnTy = f->getLoweredFunctionTypeInContext(
      TypeExpansionContext(*applySite.getFunction()));
   assert(witnessFnTy->getRepresentation() ==
          PILFunctionTypeRepresentation::WitnessMethod);

   auto requirementSig = applySite.getOrigCalleeType()->getInvocationGenericSignature();
   auto witnessThunkSig = witnessFnTy->getInvocationGenericSignature();

   SubstitutionMap origSubs = applySite.getSubstitutionMap();

   auto *mod = module.getTypePHPModule();
   bool isSelfAbstract =
      witnessFnTy->getSelfInstanceType(module)->is<GenericTypeParamType>();
   auto *classWitness = witnessFnTy->getWitnessMethodClass(module);

   return ::getWitnessMethodSubstitutions(mod, cRef, requirementSig,
                                          witnessThunkSig, origSubs,
                                          isSelfAbstract, classWitness);
}

/// Generate a new apply of a function_ref to replace an apply of a
/// witness_method when we've determined the actual function we'll end
/// up calling.
static ApplySite devirtualizeWitnessMethod(ApplySite applySite, PILFunction *f,
                                           InterfaceConformanceRef cRef,
                                           optremark::Emitter *ore) {
   // We know the witness thunk and the corresponding set of substitutions
   // required to invoke the protocol method at this point.
   auto &module = applySite.getModule();

   // Collect all the required substitutions.
   //
   // The complete set of substitutions may be different, e.g. because the found
   // witness thunk f may have been created by a specialization pass and have
   // additional generic parameters.
   auto subMap = getWitnessMethodSubstitutions(module, applySite, f, cRef);

   // Figure out the exact bound type of the function to be called by
   // applying all substitutions.
   auto calleeCanType = f->getLoweredFunctionTypeInContext(
      TypeExpansionContext(*applySite.getFunction()));
   auto substCalleeCanType = calleeCanType->substGenericArgs(
      module, subMap, TypeExpansionContext(*applySite.getFunction()));

   // Collect arguments from the apply instruction.
   SmallVector<PILValue, 4> arguments;
   SmallVector<PILValue, 4> borrowedArgs;

   // Iterate over the non self arguments and add them to the
   // new argument list, upcasting when required.
   PILBuilderWithScope argBuilder(applySite.getInstruction());
   PILFunctionConventions substConv(substCalleeCanType, module);
   unsigned substArgIdx = applySite.getCalleeArgIndexOfFirstAppliedArg();
   for (auto arg : applySite.getArguments()) {
      auto paramInfo = substConv.getPILArgumentConvention(substArgIdx);
      auto paramType = substConv.getPILArgumentType(substArgIdx++);
      if (arg->getType() != paramType) {
         if (argBuilder.hasOwnership()
             && applySite.getKind() != ApplySiteKind::PartialApplyInst
             && arg->getType().isObject()
             && arg.getOwnershipKind() == ValueOwnershipKind::Owned
             && paramInfo.isGuaranteedConvention()) {
            PILBuilderWithScope borrowBuilder(applySite.getInstruction(),
                                              argBuilder);
            arg = borrowBuilder.createBeginBorrow(applySite.getLoc(), arg);
            borrowedArgs.push_back(arg);
         }
         arg = castValueToABICompatibleType(&argBuilder, applySite.getLoc(), arg,
                                            arg->getType(), paramType);
      }
      arguments.push_back(arg);
   }
   assert(substArgIdx == substConv.getNumPILArguments());

   // Replace old apply instruction by a new apply instruction that invokes
   // the witness thunk.
   PILBuilderWithScope applyBuilder(applySite.getInstruction());
   PILLocation loc = applySite.getLoc();
   auto *fri = applyBuilder.createFunctionRefFor(loc, f);

   ApplySite newApplySite =
      replaceApplySite(applyBuilder, loc, applySite, fri, subMap, arguments,
                       substConv, borrowedArgs);

   if (ore)
      ore->emit([&]() {
         using namespace optremark;
         return RemarkPassed("WitnessMethodDevirtualized",
                             *applySite.getInstruction())
            << "Devirtualized call to " << NV("Method", f);
      });
   NumWitnessDevirt++;
   return newApplySite;
}

static bool canDevirtualizeWitnessMethod(ApplySite applySite) {
   PILFunction *f;
   PILWitnessTable *wt;

   auto *wmi = cast<WitnessMethodInst>(applySite.getCallee());

   std::tie(f, wt) = applySite.getModule().lookUpFunctionInWitnessTable(
      wmi->getConformance(), wmi->getMember());

   if (!f)
      return false;

   if (applySite.getFunction()->isSerialized()) {
      // function_ref inside fragile function cannot reference a private or
      // hidden symbol.
      if (!f->hasValidLinkageForFragileRef())
         return false;
   }

   // devirtualizeWitnessMethod below does not support this case. It currently
   // assumes it can try_apply call the target.
   if (!f->getLoweredFunctionType()->hasErrorResult()
       && isa<TryApplyInst>(applySite.getInstruction())) {
      LLVM_DEBUG(llvm::dbgs() << "        FAIL: Trying to devirtualize a "
                                 "try_apply but wtable entry has no error result.\n");
      return false;
   }

   return true;
}

/// In the cases where we can statically determine the function that
/// we'll call to, replace an apply of a witness_method with an apply
/// of a function_ref, returning the new apply.
ApplySite polar::tryDevirtualizeWitnessMethod(ApplySite applySite,
                                              optremark::Emitter *ore) {
   if (!canDevirtualizeWitnessMethod(applySite))
      return ApplySite();

   PILFunction *f;
   PILWitnessTable *wt;

   auto *wmi = cast<WitnessMethodInst>(applySite.getCallee());

   std::tie(f, wt) = applySite.getModule().lookUpFunctionInWitnessTable(
      wmi->getConformance(), wmi->getMember());

   return devirtualizeWitnessMethod(applySite, f, wmi->getConformance(), ore);
}

//===----------------------------------------------------------------------===//
//                              Top Level Driver
//===----------------------------------------------------------------------===//

/// Attempt to devirtualize the given apply if possible, and return a
/// new instruction in that case, or nullptr otherwise.
ApplySite polar::tryDevirtualizeApply(ApplySite applySite,
                                      ClassHierarchyAnalysis *cha,
                                      optremark::Emitter *ore) {
   LLVM_DEBUG(llvm::dbgs() << "    Trying to devirtualize: "
                           << *applySite.getInstruction());

   // Devirtualize apply instructions that call witness_method instructions:
   //
   //   %8 = witness_method $Optional<UInt16>, #LogicValue.boolValue!getter.1
   //   %9 = apply %8<Self = CodeUnit?>(%6#1) : ...
   //
   if (isa<WitnessMethodInst>(applySite.getCallee()))
      return tryDevirtualizeWitnessMethod(applySite, ore);

   // TODO: check if we can also de-virtualize partial applies of class methods.
   FullApplySite fas = FullApplySite::isa(applySite.getInstruction());
   if (!fas)
      return ApplySite();

   /// Optimize a class_method and alloc_ref pair into a direct function
   /// reference:
   ///
   /// \code
   /// %XX = alloc_ref $Foo
   /// %YY = class_method %XX : $Foo, #Foo.get!1 : $@convention(method)...
   /// \endcode
   ///
   ///  or
   ///
   /// %XX = metatype $...
   /// %YY = class_method %XX : ...
   ///
   ///  into
   ///
   /// %YY = function_ref @...
   if (auto *cmi = dyn_cast<ClassMethodInst>(fas.getCallee())) {
      auto instance = stripUpCasts(cmi->getOperand());
      auto classType = getSelfInstanceType(instance->getType().getAstType());
      auto *cd = classType.getClassOrBoundGenericClass();

      if (isEffectivelyFinalMethod(fas, classType, cd, cha))
         return tryDevirtualizeClassMethod(fas, instance, cd, ore,
                                           true /*isEffectivelyFinalMethod*/);

      // Try to check if the exact dynamic type of the instance is statically
      // known.
      if (auto instance = getInstanceWithExactDynamicType(cmi->getOperand(), cha))
         return tryDevirtualizeClassMethod(fas, instance, cd, ore);

      if (auto exactTy = getExactDynamicType(cmi->getOperand(), cha)) {
         if (exactTy == cmi->getOperand()->getType())
            return tryDevirtualizeClassMethod(fas, cmi->getOperand(), cd, ore);
      }
   }

   if (isa<SuperMethodInst>(fas.getCallee())) {
      auto instance = fas.getArguments().back();
      auto classType = getSelfInstanceType(instance->getType().getAstType());
      auto *cd = classType.getClassOrBoundGenericClass();

      return tryDevirtualizeClassMethod(fas, instance, cd, ore);
   }

   return ApplySite();
}

bool polar::canDevirtualizeApply(FullApplySite applySite,
                                 ClassHierarchyAnalysis *cha) {
   LLVM_DEBUG(llvm::dbgs() << "    Trying to devirtualize: "
                           << *applySite.getInstruction());

   // Devirtualize apply instructions that call witness_method instructions:
   //
   //   %8 = witness_method $Optional<UInt16>, #LogicValue.boolValue!getter.1
   //   %9 = apply %8<Self = CodeUnit?>(%6#1) : ...
   //
   if (isa<WitnessMethodInst>(applySite.getCallee()))
      return canDevirtualizeWitnessMethod(applySite);

   /// Optimize a class_method and alloc_ref pair into a direct function
   /// reference:
   ///
   /// \code
   /// %XX = alloc_ref $Foo
   /// %YY = class_method %XX : $Foo, #Foo.get!1 : $@convention(method)...
   /// \endcode
   ///
   ///  or
   ///
   /// %XX = metatype $...
   /// %YY = class_method %XX : ...
   ///
   ///  into
   ///
   /// %YY = function_ref @...
   if (auto *cmi = dyn_cast<ClassMethodInst>(applySite.getCallee())) {
      auto instance = stripUpCasts(cmi->getOperand());
      auto classType = getSelfInstanceType(instance->getType().getAstType());
      auto *cd = classType.getClassOrBoundGenericClass();

      if (isEffectivelyFinalMethod(applySite, classType, cd, cha))
         return canDevirtualizeClassMethod(applySite, cd, nullptr /*ore*/,
                                           true /*isEffectivelyFinalMethod*/);

      // Try to check if the exact dynamic type of the instance is statically
      // known.
      if (auto instance = getInstanceWithExactDynamicType(cmi->getOperand(), cha))
         return canDevirtualizeClassMethod(applySite, cd);

      if (auto exactTy = getExactDynamicType(cmi->getOperand(), cha)) {
         if (exactTy == cmi->getOperand()->getType())
            return canDevirtualizeClassMethod(applySite, cd);
      }
   }

   if (isa<SuperMethodInst>(applySite.getCallee())) {
      auto instance = applySite.getArguments().back();
      auto classType = getSelfInstanceType(instance->getType().getAstType());
      auto *cd = classType.getClassOrBoundGenericClass();

      return canDevirtualizeClassMethod(applySite, cd);
   }

   return false;
}
