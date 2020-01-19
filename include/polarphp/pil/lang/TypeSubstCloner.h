//===--- TypeSubstCloner.h - Clones code and substitutes types --*- C++ -*-===//
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
// This file defines TypeSubstCloner, which derives from PILCloner and
// has support for type substitution while cloning code that uses generics.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_TYPESUBSTCLONER_H
#define POLARPHP_PIL_TYPESUBSTCLONER_H

#include "polarphp/ast/GenericEnvironment.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/ast/Type.h"
#include "polarphp/pil/lang/DynamicCasts.h"
#include "polarphp/pil/lang/PILCloner.h"
#include "polarphp/pil/lang/PILFunctionBuilder.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "polarphp/pil/optimizer/utils/SpecializationMangler.h"
#include "llvm/Support/Debug.h"

namespace polar {

/// A utility class for cloning code while remapping types.
///
/// \tparam FunctionBuilderTy Function builder type injected by
/// subclasses. Used to break a circular dependency from PIL <=>
/// PILOptimizer that would be caused by us needing to use
/// PILOptFunctionBuilder here.
template<typename ImplClass, typename FunctionBuilderTy>
class TypeSubstCloner : public PILClonerWithScopes<ImplClass> {
   friend class PILInstructionVisitor<ImplClass>;
   friend class PILCloner<ImplClass>;

   using super = PILClonerWithScopes<ImplClass>;

   void postProcess(PILInstruction *Orig, PILInstruction *Cloned) {
      llvm_unreachable("Clients need to explicitly call a base class impl!");
   }

   // A helper class for cloning different kinds of apply instructions.
   // Supports cloning of self-recursive functions.
   class ApplySiteCloningHelper {
      PILValue Callee;
      SubstitutionMap Subs;
      SmallVector<PILValue, 8> Args;
      SubstitutionMap RecursiveSubs;

   public:
      ApplySiteCloningHelper(ApplySite AI, TypeSubstCloner &Cloner)
         : Callee(Cloner.getOpValue(AI.getCallee())) {
         PILType SubstCalleePILType = Cloner.getOpType(AI.getSubstCalleePILType());

         Args = Cloner.template getOpValueArray<8>(AI.getArguments());
         PILBuilder &Builder = Cloner.getBuilder();
         Builder.setCurrentDebugScope(Cloner.super::getOpScope(AI.getDebugScope()));

         // Remap substitutions.
         Subs = Cloner.getOpSubstitutionMap(AI.getSubstitutionMap());

         if (!Cloner.Inlining) {
            FunctionRefInst *FRI = dyn_cast<FunctionRefInst>(AI.getCallee());
            if (FRI && FRI->getInitiallyReferencedFunction() == AI.getFunction() &&
                Subs == Cloner.SubsMap) {
               // Handle recursions by replacing the apply to the callee with an
               // apply to the newly specialized function, but only if substitutions
               // are the same.
               auto LoweredFnTy = Builder.getFunction().getLoweredFunctionType();
               auto RecursiveSubstCalleePILType = LoweredFnTy;
               auto GenSig = LoweredFnTy->getInvocationGenericSignature();
               if (GenSig) {
                  // Compute substitutions for the specialized function. These
                  // substitutions may be different from the original ones, e.g.
                  // there can be less substitutions.
                  RecursiveSubs = SubstitutionMap::get(
                     AI.getFunction()
                        ->getLoweredFunctionType()
                        ->getInvocationGenericSignature(),
                     Subs);

                  // Use the new set of substitutions to compute the new
                  // substituted callee type.
                  RecursiveSubstCalleePILType = LoweredFnTy->substGenericArgs(
                     AI.getModule(), RecursiveSubs,
                     Builder.getTypeExpansionContext());
               }

               // The specialized recursive function may have different calling
               // convention for parameters. E.g. some of former indirect parameters
               // may become direct. Some of indirect return values may become
               // direct. Do not replace the callee in that case.
               if (SubstCalleePILType.getAstType() == RecursiveSubstCalleePILType) {
                  Subs = RecursiveSubs;
                  Callee = Builder.createFunctionRef(
                     Cloner.getOpLocation(AI.getLoc()), &Builder.getFunction());
                  SubstCalleePILType =
                     PILType::getPrimitiveObjectType(RecursiveSubstCalleePILType);
               }
            }
         }

         assert(Subs.empty() ||
                SubstCalleePILType ==
                Callee->getType().substGenericArgs(
                   AI.getModule(), Subs, Builder.getTypeExpansionContext()));
      }

      ArrayRef<PILValue> getArguments() const {
         return Args;
      }

      PILValue getCallee() const {
         return Callee;
      }

      SubstitutionMap getSubstitutions() const {
         return Subs;
      }
   };

public:
   using PILClonerWithScopes<ImplClass>::asImpl;
   using PILClonerWithScopes<ImplClass>::getBuilder;
   using PILClonerWithScopes<ImplClass>::getOpLocation;
   using PILClonerWithScopes<ImplClass>::getOpValue;
   using PILClonerWithScopes<ImplClass>::getAstTypeInClonedContext;
   using PILClonerWithScopes<ImplClass>::getOpASTType;
   using PILClonerWithScopes<ImplClass>::getTypeInClonedContext;
   using PILClonerWithScopes<ImplClass>::getOpType;
   using PILClonerWithScopes<ImplClass>::getOpBasicBlock;
   using PILClonerWithScopes<ImplClass>::recordClonedInstruction;
   using PILClonerWithScopes<ImplClass>::recordFoldedValue;
   using PILClonerWithScopes<ImplClass>::addBlockWithUnreachable;
   using PILClonerWithScopes<ImplClass>::OpenedArchetypesTracker;

   TypeSubstCloner(PILFunction &To,
                   PILFunction &From,
                   SubstitutionMap ApplySubs,
                   PILOpenedArchetypesTracker &OpenedArchetypesTracker,
                   bool Inlining = false)
      : PILClonerWithScopes<ImplClass>(To, OpenedArchetypesTracker, Inlining),
        PolarphpMod(From.getModule().getTypePHPModule()),
        SubsMap(ApplySubs),
        Original(From),
        Inlining(Inlining) {
   }

   TypeSubstCloner(PILFunction &To,
                   PILFunction &From,
                   SubstitutionMap ApplySubs,
                   bool Inlining = false)
      : PILClonerWithScopes<ImplClass>(To, Inlining),
        PolarphpMod(From.getModule().getTypePHPModule()),
        SubsMap(ApplySubs),
        Original(From),
        Inlining(Inlining) {
   }

protected:
   PILType remapType(PILType Ty) {
      PILType &Sty = TypeCache[Ty];
      if (!Sty) {
         Sty = Ty.subst(Original.getModule(), SubsMap);
         if (!Sty.getAstType()->hasOpaqueArchetype() ||
             !getBuilder()
                .getTypeExpansionContext()
                .shouldLookThroughOpaqueTypeArchetypes())
            return Sty;
         // Remap types containing opaque result types in the current context.
         Sty = getBuilder().getTypeLowering(Sty).getLoweredType().getCategoryType(
            Sty.getCategory());
      }
      return Sty;
   }

   CanType remapASTType(CanType ty) {
      auto substTy = ty.subst(SubsMap)->getCanonicalType();
      if (!substTy->hasOpaqueArchetype() ||
          !getBuilder().getTypeExpansionContext()
             .shouldLookThroughOpaqueTypeArchetypes())
         return substTy;
      // Remap types containing opaque result types in the current context.
      return getBuilder().getModule().Types.getLoweredRValueType(
         TypeExpansionContext(getBuilder().getFunction()), substTy);
   }

   InterfaceConformanceRef remapConformance(Type ty,
                                           InterfaceConformanceRef conf) {
      auto conformance = conf.subst(ty, SubsMap);
      auto substTy = ty.subst(SubsMap)->getCanonicalType();
      auto context = getBuilder().getTypeExpansionContext();
      if (substTy->hasOpaqueArchetype() &&
          context.shouldLookThroughOpaqueTypeArchetypes()) {
         conformance =
            substOpaqueTypesWithUnderlyingTypes(conformance, substTy, context);
      }
      return conformance;
   }

   SubstitutionMap remapSubstitutionMap(SubstitutionMap Subs) {
      return Subs.subst(SubsMap);
   }

   void visitApplyInst(ApplyInst *Inst) {
      ApplySiteCloningHelper Helper(ApplySite(Inst), *this);
      ApplyInst *N =
         getBuilder().createApply(getOpLocation(Inst->getLoc()),
                                  Helper.getCallee(), Helper.getSubstitutions(),
                                  Helper.getArguments(), Inst->isNonThrowing(),
                                  GenericSpecializationInformation::create(
                                     Inst, getBuilder()));
      // Specialization can return noreturn applies that were not identified as
      // such before.
      if (N->isCalleeNoReturn() &&
          !isa<UnreachableInst>(*std::next(PILBasicBlock::iterator(Inst)))) {
         noReturnApplies.push_back(N);
      }

      recordClonedInstruction(Inst, N);
   }

   void visitTryApplyInst(TryApplyInst *Inst) {
      ApplySiteCloningHelper Helper(ApplySite(Inst), *this);
      TryApplyInst *N = getBuilder().createTryApply(
         getOpLocation(Inst->getLoc()), Helper.getCallee(),
         Helper.getSubstitutions(), Helper.getArguments(),
         getOpBasicBlock(Inst->getNormalBB()),
         getOpBasicBlock(Inst->getErrorBB()),
         GenericSpecializationInformation::create(
            Inst, getBuilder()));
      recordClonedInstruction(Inst, N);
   }

   void visitPartialApplyInst(PartialApplyInst *Inst) {
      ApplySiteCloningHelper Helper(ApplySite(Inst), *this);
      auto ParamConvention =
         Inst->getType().getAs<PILFunctionType>()->getCalleeConvention();
      PartialApplyInst *N = getBuilder().createPartialApply(
         getOpLocation(Inst->getLoc()), Helper.getCallee(),
         Helper.getSubstitutions(), Helper.getArguments(), ParamConvention,
         Inst->isOnStack(),
         GenericSpecializationInformation::create(Inst, getBuilder()));
      recordClonedInstruction(Inst, N);
   }

   /// Attempt to simplify a conditional checked cast.
   void visitCheckedCastAddrBranchInst(CheckedCastAddrBranchInst *inst) {
      PILLocation loc = getOpLocation(inst->getLoc());
      PILValue src = getOpValue(inst->getSrc());
      PILValue dest = getOpValue(inst->getDest());
      CanType sourceType = getOpASTType(inst->getSourceFormalType());
      CanType targetType = getOpASTType(inst->getTargetFormalType());
      PILBasicBlock *succBB = getOpBasicBlock(inst->getSuccessBB());
      PILBasicBlock *failBB = getOpBasicBlock(inst->getFailureBB());

      PILBuilderWithPostProcess<TypeSubstCloner, 16> B(this, inst);
      B.setCurrentDebugScope(super::getOpScope(inst->getDebugScope()));

      auto TrueCount = inst->getTrueBBCount();
      auto FalseCount = inst->getFalseBBCount();

      // Try to use the scalar cast instruction.
      if (canUseScalarCheckedCastInstructions(B.getModule(),
                                              sourceType, targetType)) {
         emitIndirectConditionalCastWithScalar(
            B, PolarphpMod, loc, inst->getConsumptionKind(), src, sourceType, dest,
            targetType, succBB, failBB, TrueCount, FalseCount);
         return;
      }

      // Otherwise, use the indirect cast.
      B.createCheckedCastAddrBranch(loc, inst->getConsumptionKind(),
                                    src, sourceType,
                                    dest, targetType,
                                    succBB, failBB);
      return;
   }

   void visitUpcastInst(UpcastInst *Upcast) {
      // If the type substituted type of the operand type and result types match
      // there is no need for an upcast and we can just use the operand.
      if (getOpType(Upcast->getType()) ==
          getOpValue(Upcast->getOperand())->getType()) {
         recordFoldedValue(PILValue(Upcast), getOpValue(Upcast->getOperand()));
         return;
      }
      super::visitUpcastInst(Upcast);
   }

   void visitCopyValueInst(CopyValueInst *Copy) {
      // If the substituted type is trivial, ignore the copy.
      PILType copyTy = getOpType(Copy->getType());
      if (copyTy.isTrivial(*Copy->getFunction())) {
         recordFoldedValue(PILValue(Copy), getOpValue(Copy->getOperand()));
         return;
      }
      super::visitCopyValueInst(Copy);
   }

   void visitDestroyValueInst(DestroyValueInst *Destroy) {
      // If the substituted type is trivial, ignore the destroy.
      PILType destroyTy = getOpType(Destroy->getOperand()->getType());
      if (destroyTy.isTrivial(*Destroy->getFunction())) {
         return;
      }
      super::visitDestroyValueInst(Destroy);
   }

   /// One abstract function in the debug info can only have one set of variables
   /// and types. This function determines whether applying the substitutions in
   /// \p SubsMap on the generic signature \p Sig will change the generic type
   /// parameters in the signature. This is used to decide whether it's necessary
   /// to clone a unique copy of the function declaration with the substitutions
   /// applied for the debug info.
   static bool substitutionsChangeGenericTypeParameters(SubstitutionMap SubsMap,
                                                        GenericSignature Sig) {

      // If there are no substitutions, just reuse
      // the original decl.
      if (SubsMap.empty())
         return false;

      bool Result = false;
      Sig->forEachParam([&](GenericTypeParamType *ParamType, bool Canonical) {
         if (!Canonical)
            return;
         if (!Type(ParamType).subst(SubsMap)->isEqual(ParamType))
            Result = true;
      });

      return Result;
   }

   enum { ForInlining = true };
   /// Helper function to clone the parent function of a PILDebugScope if
   /// necessary when inlining said function into a new generic context.
   /// \param SubsMap - the substitutions of the inlining/specialization process.
   /// \param RemappedSig - the generic signature.
   static PILFunction *remapParentFunction(FunctionBuilderTy &FuncBuilder,
                                           PILModule &M,
                                           PILFunction *ParentFunction,
                                           SubstitutionMap SubsMap,
                                           GenericSignature RemappedSig,
                                           bool ForInlining = false) {
      // If the original, non-inlined version of the function had no generic
      // environment, there is no need to remap it.
      auto *OriginalEnvironment = ParentFunction->getGenericEnvironment();
      if (!RemappedSig || !OriginalEnvironment)
         return ParentFunction;

      if (SubsMap.hasArchetypes())
         SubsMap = SubsMap.mapReplacementTypesOutOfContext();

      if (!substitutionsChangeGenericTypeParameters(SubsMap, RemappedSig))
         return ParentFunction;

      // Note that mapReplacementTypesOutOfContext() can't do anything for
      // opened existentials, and since archetypes can't be mangled, ignore
      // this case for now.
      if (SubsMap.hasArchetypes())
         return ParentFunction;

      // Clone the function with the substituted type for the debug info.
      mangle::GenericSpecializationMangler Mangler(
         ParentFunction, SubsMap, IsNotSerialized, false, ForInlining);
      std::string MangledName = Mangler.mangle(RemappedSig);

      if (ParentFunction->getName() == MangledName)
         return ParentFunction;
      if (auto *CachedFn = M.lookUpFunction(MangledName))
         ParentFunction = CachedFn;
      else {
         // Create a new function with this mangled name with an empty
         // body. There won't be any IR generated for it (hence the linkage),
         // but the symbol will be refered to by the debug info metadata.
         ParentFunction = FuncBuilder.getOrCreateFunction(
            ParentFunction->getLocation(), MangledName, PILLinkage::Shared,
            ParentFunction->getLoweredFunctionType(), ParentFunction->isBare(),
            ParentFunction->isTransparent(), ParentFunction->isSerialized(),
            IsNotDynamic, 0, ParentFunction->isThunk(),
            ParentFunction->getClassSubclassScope());
         // Increment the ref count for the inlined function, so it doesn't
         // get deleted before we can emit abstract debug info for it.
         if (!ParentFunction->isZombie()) {
            ParentFunction->setInlined();
            // If the function was newly created with an empty body mark it as
            // undead.
            if (ParentFunction->empty()) {
               FuncBuilder.eraseFunction(ParentFunction);
               ParentFunction->setGenericEnvironment(OriginalEnvironment);
            }
         }
      }
      return ParentFunction;
   }

   /// The Polarphp module that the cloned function belongs to.
   ModuleDecl *PolarphpMod;
   /// The substitutions list for the specialization.
   SubstitutionMap SubsMap;
   /// Cache for substituted types.
   llvm::DenseMap<PILType, PILType> TypeCache;
   /// The original function to specialize.
   PILFunction &Original;
   /// True, if used for inlining.
   bool Inlining;
   // Generic specialization can create noreturn applications that where
   // previously not identifiable as such.
   SmallVector<ApplyInst *, 16> noReturnApplies;
};

} // end namespace polar

#endif
