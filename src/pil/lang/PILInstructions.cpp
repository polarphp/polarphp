//===--- PILInstructions.cpp - Instructions for PIL code ------------------===//
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
// This file defines the high-level PILInstruction classes used for PIL code.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/Expr.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/basic/AssertImplements.h"
#include "polarphp/basic/Unicode.h"
#include "polarphp/pil/lang/FormalLinkage.h"
#include "polarphp/pil/lang/Projection.h"
#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/pil/lang/PILCloner.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/lang/PILVisitor.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/ErrorHandling.h"

using namespace polar;
using namespace polar::lowering;

/// Allocate an instruction that inherits from llvm::TrailingObjects<>.
template <class Inst, class... TrailingTypes, class... CountTypes>
static void *allocateTrailingInst(PILFunction &F, CountTypes... counts) {
   return F.getModule().allocateInst(
      Inst::template totalSizeToAlloc<TrailingTypes...>(counts...),
      alignof(Inst));
}

// Collect used open archetypes from a given type into the \p openedArchetypes.
// \p openedArchetypes is being used as a set. We don't use a real set type here
// for performance reasons.
static void
collectDependentTypeInfo(CanType Ty,
                         SmallVectorImpl<CanArchetypeType> &openedArchetypes,
                         bool &hasDynamicSelf) {
   if (!Ty)
      return;
   if (Ty->hasDynamicSelfType())
      hasDynamicSelf = true;
   if (!Ty->hasOpenedExistential())
      return;
   Ty.visit([&](CanType t) {
      if (t->isOpenedExistential()) {
         // Add this opened archetype if it was not seen yet.
         // We don't use a set here, because the number of open archetypes
         // is usually very small and using a real set may introduce too
         // much overhead.
         auto archetypeTy = cast<ArchetypeType>(t);
         if (std::find(openedArchetypes.begin(), openedArchetypes.end(),
                       archetypeTy) == openedArchetypes.end())
            openedArchetypes.push_back(archetypeTy);
      }
   });
}

// Takes a set of open archetypes as input and produces a set of
// references to open archetype definitions.
static void buildTypeDependentOperands(
   SmallVectorImpl<CanArchetypeType> &OpenedArchetypes,
   bool hasDynamicSelf,
   SmallVectorImpl<PILValue> &TypeDependentOperands,
   PILOpenedArchetypesState &OpenedArchetypesState, PILFunction &F) {

   for (auto archetype : OpenedArchetypes) {
      auto Def = OpenedArchetypesState.getOpenedArchetypeDef(archetype);
      assert(Def);
      assert(getOpenedArchetypeOf(Def->getType().getAstType()) &&
             "Opened archetype operands should be of an opened existential type");
      TypeDependentOperands.push_back(Def);
   }
   if (hasDynamicSelf)
      TypeDependentOperands.push_back(F.getSelfMetadataArgument());
}

// Collects all opened archetypes from a type and a substitutions list and form
// a corresponding list of opened archetype operands.
// We need to know the number of opened archetypes to estimate
// the number of opened archetype operands for the instruction
// being formed, because we need to reserve enough memory
// for these operands.
static void collectTypeDependentOperands(
   SmallVectorImpl<PILValue> &TypeDependentOperands,
   PILOpenedArchetypesState &OpenedArchetypesState,
   PILFunction &F,
   CanType Ty,
   SubstitutionMap subs = { }) {
   SmallVector<CanArchetypeType, 4> openedArchetypes;
   bool hasDynamicSelf = false;
   collectDependentTypeInfo(Ty, openedArchetypes, hasDynamicSelf);
   for (Type replacement : subs.getReplacementTypes()) {
      // Substitutions in PIL should really be canonical.
      auto ReplTy = replacement->getCanonicalType();
      collectDependentTypeInfo(ReplTy, openedArchetypes, hasDynamicSelf);
   }
   buildTypeDependentOperands(openedArchetypes, hasDynamicSelf,
                              TypeDependentOperands,
                              OpenedArchetypesState, F);
}

//===----------------------------------------------------------------------===//
// PILInstruction Subclasses
//===----------------------------------------------------------------------===//

template <typename INST>
static void *allocateDebugVarCarryingInst(PILModule &M,
                                          Optional<PILDebugVariable> Var,
                                          ArrayRef<PILValue> Operands = {}) {
   return M.allocateInst(sizeof(INST) + (Var ? Var->Name.size() : 0) +
                         sizeof(Operand) * Operands.size(),
                         alignof(INST));
}

TailAllocatedDebugVariable::TailAllocatedDebugVariable(
   Optional<PILDebugVariable> Var, char *buf) {
   if (!Var) {
      Bits.RawValue = 0;
      return;
   }

   Bits.Data.HasValue = true;
   Bits.Data.Constant = Var->Constant;
   Bits.Data.ArgNo = Var->ArgNo;
   Bits.Data.NameLength = Var->Name.size();
   assert(Bits.Data.ArgNo == Var->ArgNo && "Truncation");
   assert(Bits.Data.NameLength == Var->Name.size() && "Truncation");
   memcpy(buf, Var->Name.data(), Bits.Data.NameLength);
}

StringRef TailAllocatedDebugVariable::getName(const char *buf) const {
   if (Bits.Data.NameLength)
      return StringRef(buf, Bits.Data.NameLength);
   return StringRef();
}

AllocStackInst::AllocStackInst(PILDebugLocation Loc, PILType elementType,
                               ArrayRef<PILValue> TypeDependentOperands,
                               PILFunction &F,
                               Optional<PILDebugVariable> Var,
                               bool hasDynamicLifetime)
   : InstructionBase(Loc, elementType.getAddressType()),
     dynamicLifetime(hasDynamicLifetime) {
   PILInstruction::Bits.AllocStackInst.NumOperands =
      TypeDependentOperands.size();
   assert(PILInstruction::Bits.AllocStackInst.NumOperands ==
          TypeDependentOperands.size() && "Truncation");
   PILInstruction::Bits.AllocStackInst.VarInfo =
      TailAllocatedDebugVariable(Var, getTrailingObjects<char>()).getRawValue();
   TrailingOperandsList::InitOperandsList(getAllOperands().begin(), this,
                                          TypeDependentOperands);
}

AllocStackInst *
AllocStackInst::create(PILDebugLocation Loc,
                       PILType elementType, PILFunction &F,
                       PILOpenedArchetypesState &OpenedArchetypes,
                       Optional<PILDebugVariable> Var,
                       bool hasDynamicLifetime) {
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                elementType.getAstType());
   void *Buffer = allocateDebugVarCarryingInst<AllocStackInst>(
      F.getModule(), Var, TypeDependentOperands);
   return ::new (Buffer)
      AllocStackInst(Loc, elementType, TypeDependentOperands, F, Var,
                     hasDynamicLifetime);
}

VarDecl *AllocationInst::getDecl() const {
   return getLoc().getAsAstNode<VarDecl>();
}

DeallocStackInst *AllocStackInst::getSingleDeallocStack() const {
   DeallocStackInst *Dealloc = nullptr;
   for (auto *U : getUses()) {
      if (auto DS = dyn_cast<DeallocStackInst>(U->getUser())) {
         if (Dealloc == nullptr) {
            Dealloc = DS;
            continue;
         }
         // Already saw a dealloc_stack.
         return nullptr;
      }
   }
   return Dealloc;
}

AllocRefInstBase::AllocRefInstBase(PILInstructionKind Kind,
                                   PILDebugLocation Loc,
                                   PILType ObjectType,
                                   bool objc, bool canBeOnStack,
                                   ArrayRef<PILType> ElementTypes)
   : AllocationInst(Kind, Loc, ObjectType) {
   PILInstruction::Bits.AllocRefInstBase.ObjC = objc;
   PILInstruction::Bits.AllocRefInstBase.OnStack = canBeOnStack;
   PILInstruction::Bits.AllocRefInstBase.NumTailTypes = ElementTypes.size();
   assert(PILInstruction::Bits.AllocRefInstBase.NumTailTypes ==
          ElementTypes.size() && "Truncation");
   assert(!objc || ElementTypes.empty());
}

AllocRefInst *AllocRefInst::create(PILDebugLocation Loc, PILFunction &F,
                                   PILType ObjectType,
                                   bool objc, bool canBeOnStack,
                                   ArrayRef<PILType> ElementTypes,
                                   ArrayRef<PILValue> ElementCountOperands,
                                   PILOpenedArchetypesState &OpenedArchetypes) {
   assert(ElementTypes.size() == ElementCountOperands.size());
   assert(!objc || ElementTypes.empty());
   SmallVector<PILValue, 8> AllOperands(ElementCountOperands.begin(),
                                        ElementCountOperands.end());
   for (PILType ElemType : ElementTypes) {
      collectTypeDependentOperands(AllOperands, OpenedArchetypes, F,
                                   ElemType.getAstType());
   }
   collectTypeDependentOperands(AllOperands, OpenedArchetypes, F,
                                ObjectType.getAstType());
   auto Size = totalSizeToAlloc<polar::Operand, PILType>(AllOperands.size(),
                                                         ElementTypes.size());
   auto Buffer = F.getModule().allocateInst(Size, alignof(AllocRefInst));
   return ::new (Buffer) AllocRefInst(Loc, F, ObjectType, objc, canBeOnStack,
                                      ElementTypes, AllOperands);
}

AllocRefDynamicInst *
AllocRefDynamicInst::create(PILDebugLocation DebugLoc, PILFunction &F,
                            PILValue metatypeOperand, PILType ty, bool objc,
                            ArrayRef<PILType> ElementTypes,
                            ArrayRef<PILValue> ElementCountOperands,
                            PILOpenedArchetypesState &OpenedArchetypes) {
   SmallVector<PILValue, 8> AllOperands(ElementCountOperands.begin(),
                                        ElementCountOperands.end());
   AllOperands.push_back(metatypeOperand);
   collectTypeDependentOperands(AllOperands, OpenedArchetypes, F,
                                ty.getAstType());
   for (PILType ElemType : ElementTypes) {
      collectTypeDependentOperands(AllOperands, OpenedArchetypes, F,
                                   ElemType.getAstType());
   }
   auto Size = totalSizeToAlloc<polar::Operand, PILType>(AllOperands.size(),
                                                         ElementTypes.size());
   auto Buffer = F.getModule().allocateInst(Size, alignof(AllocRefDynamicInst));
   return ::new (Buffer)
      AllocRefDynamicInst(DebugLoc, ty, objc, ElementTypes, AllOperands);
}

AllocBoxInst::AllocBoxInst(PILDebugLocation Loc, CanPILBoxType BoxType,
                           ArrayRef<PILValue> TypeDependentOperands,
                           PILFunction &F, Optional<PILDebugVariable> Var,
                           bool hasDynamicLifetime)
   : InstructionBaseWithTrailingOperands(TypeDependentOperands, Loc,
                                         PILType::getPrimitiveObjectType(BoxType)),
     VarInfo(Var, getTrailingObjects<char>()),
     dynamicLifetime(hasDynamicLifetime) {
}

AllocBoxInst *AllocBoxInst::create(PILDebugLocation Loc,
                                   CanPILBoxType BoxType,
                                   PILFunction &F,
                                   PILOpenedArchetypesState &OpenedArchetypes,
                                   Optional<PILDebugVariable> Var,
                                   bool hasDynamicLifetime) {
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                BoxType);
   auto Sz = totalSizeToAlloc<polar::Operand, char>(TypeDependentOperands.size(),
                                                    Var ? Var->Name.size() : 0);
   auto Buf = F.getModule().allocateInst(Sz, alignof(AllocBoxInst));
   return ::new (Buf) AllocBoxInst(Loc, BoxType, TypeDependentOperands, F, Var,
                                   hasDynamicLifetime);
}

PILType AllocBoxInst::getAddressType() const {
   return getPILBoxFieldType(TypeExpansionContext(*this->getFunction()),
                             getBoxType(), getModule().Types, 0)
      .getAddressType();
}

DebugValueInst::DebugValueInst(PILDebugLocation DebugLoc, PILValue Operand,
                               PILDebugVariable Var)
   : UnaryInstructionBase(DebugLoc, Operand),
     VarInfo(Var, getTrailingObjects<char>()) {}

DebugValueInst *DebugValueInst::create(PILDebugLocation DebugLoc,
                                       PILValue Operand, PILModule &M,
                                       PILDebugVariable Var) {
   void *buf = allocateDebugVarCarryingInst<DebugValueInst>(M, Var);
   return ::new (buf) DebugValueInst(DebugLoc, Operand, Var);
}

DebugValueAddrInst::DebugValueAddrInst(PILDebugLocation DebugLoc,
                                       PILValue Operand,
                                       PILDebugVariable Var)
   : UnaryInstructionBase(DebugLoc, Operand),
     VarInfo(Var, getTrailingObjects<char>()) {}

DebugValueAddrInst *DebugValueAddrInst::create(PILDebugLocation DebugLoc,
                                               PILValue Operand, PILModule &M,
                                               PILDebugVariable Var) {
   void *buf = allocateDebugVarCarryingInst<DebugValueAddrInst>(M, Var);
   return ::new (buf) DebugValueAddrInst(DebugLoc, Operand, Var);
}

VarDecl *DebugValueInst::getDecl() const {
   return getLoc().getAsAstNode<VarDecl>();
}
VarDecl *DebugValueAddrInst::getDecl() const {
   return getLoc().getAsAstNode<VarDecl>();
}

AllocExistentialBoxInst *AllocExistentialBoxInst::create(
   PILDebugLocation Loc, PILType ExistentialType, CanType ConcreteType,
   ArrayRef<InterfaceConformanceRef> Conformances,
   PILFunction *F,
   PILOpenedArchetypesState &OpenedArchetypes) {
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, *F,
                                ConcreteType);
   PILModule &Mod = F->getModule();
   auto Size = totalSizeToAlloc<polar::Operand>(TypeDependentOperands.size());
   auto Buffer = Mod.allocateInst(Size, alignof(AllocExistentialBoxInst));
   return ::new (Buffer) AllocExistentialBoxInst(Loc,
                                                 ExistentialType,
                                                 ConcreteType,
                                                 Conformances,
                                                 TypeDependentOperands,
                                                 F);
}

AllocValueBufferInst::AllocValueBufferInst(
   PILDebugLocation DebugLoc, PILType valueType, PILValue operand,
   ArrayRef<PILValue> TypeDependentOperands)
   : UnaryInstructionWithTypeDependentOperandsBase(DebugLoc, operand,
                                                   TypeDependentOperands,
                                                   valueType.getAddressType()) {}

AllocValueBufferInst *
AllocValueBufferInst::create(PILDebugLocation DebugLoc, PILType valueType,
                             PILValue operand, PILFunction &F,
                             PILOpenedArchetypesState &OpenedArchetypes) {
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                valueType.getAstType());
   void *Buffer = F.getModule().allocateInst(
      sizeof(AllocValueBufferInst) +
      sizeof(Operand) * (TypeDependentOperands.size() + 1),
      alignof(AllocValueBufferInst));
   return ::new (Buffer) AllocValueBufferInst(DebugLoc, valueType, operand,
                                              TypeDependentOperands);
}

BuiltinInst *BuiltinInst::create(PILDebugLocation Loc, Identifier Name,
                                 PILType ReturnType,
                                 SubstitutionMap Substitutions,
                                 ArrayRef<PILValue> Args,
                                 PILModule &M) {
   auto Size = totalSizeToAlloc<polar::Operand>(Args.size());
   auto Buffer = M.allocateInst(Size, alignof(BuiltinInst));
   return ::new (Buffer) BuiltinInst(Loc, Name, ReturnType, Substitutions,
                                     Args);
}

BuiltinInst::BuiltinInst(PILDebugLocation Loc, Identifier Name,
                         PILType ReturnType, SubstitutionMap Subs,
                         ArrayRef<PILValue> Args)
   : InstructionBaseWithTrailingOperands(Args, Loc, ReturnType), Name(Name),
     Substitutions(Subs) {
}

InitBlockStorageHeaderInst *
InitBlockStorageHeaderInst::create(PILFunction &F,
                                   PILDebugLocation DebugLoc, PILValue BlockStorage,
                                   PILValue InvokeFunction, PILType BlockType,
                                   SubstitutionMap Subs) {
   void *Buffer = F.getModule().allocateInst(
      sizeof(InitBlockStorageHeaderInst),
      alignof(InitBlockStorageHeaderInst));

   return ::new (Buffer) InitBlockStorageHeaderInst(DebugLoc, BlockStorage,
                                                    InvokeFunction, BlockType,
                                                    Subs);
}

ApplyInst::ApplyInst(PILDebugLocation Loc, PILValue Callee,
                     PILType SubstCalleeTy, PILType Result,
                     SubstitutionMap Subs,
                     ArrayRef<PILValue> Args,
                     ArrayRef<PILValue> TypeDependentOperands,
                     bool isNonThrowing,
                     const GenericSpecializationInformation *SpecializationInfo)
   : InstructionBase(Loc, Callee, SubstCalleeTy, Subs, Args,
                     TypeDependentOperands, SpecializationInfo, Result) {
   setNonThrowing(isNonThrowing);
   assert(!SubstCalleeTy.castTo<PILFunctionType>()->isCoroutine());
}

ApplyInst *
ApplyInst::create(PILDebugLocation Loc, PILValue Callee, SubstitutionMap Subs,
                  ArrayRef<PILValue> Args, bool isNonThrowing,
                  Optional<PILModuleConventions> ModuleConventions,
                  PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes,
                  const GenericSpecializationInformation *SpecializationInfo) {
   PILType SubstCalleePILTy = Callee->getType().substGenericArgs(
      F.getModule(), Subs, F.getTypeExpansionContext());
   auto SubstCalleeTy = SubstCalleePILTy.getAs<PILFunctionType>();
   PILFunctionConventions Conv(SubstCalleeTy,
                               ModuleConventions.hasValue()
                               ? ModuleConventions.getValue()
                               : PILModuleConventions(F.getModule()));
   PILType Result = Conv.getPILResultType();

   SmallVector<PILValue, 32> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                SubstCalleePILTy.getAstType(), Subs);
   void *Buffer =
      allocateTrailingInst<ApplyInst, Operand>(
         F, getNumAllOperands(Args, TypeDependentOperands));
   return ::new(Buffer) ApplyInst(Loc, Callee, SubstCalleePILTy,
                                  Result, Subs, Args,
                                  TypeDependentOperands, isNonThrowing,
                                  SpecializationInfo);
}

BeginApplyInst::BeginApplyInst(PILDebugLocation loc, PILValue callee,
                               PILType substCalleeTy,
                               ArrayRef<PILType> allResultTypes,
                               ArrayRef<ValueOwnershipKind> allResultOwnerships,
                               SubstitutionMap subs,
                               ArrayRef<PILValue> args,
                               ArrayRef<PILValue> typeDependentOperands,
                               bool isNonThrowing,
                               const GenericSpecializationInformation *specializationInfo)
   : InstructionBase(loc, callee, substCalleeTy, subs, args,
                     typeDependentOperands, specializationInfo),
     MultipleValueInstructionTrailingObjects(this, allResultTypes,
                                             allResultOwnerships) {
   setNonThrowing(isNonThrowing);
   assert(substCalleeTy.castTo<PILFunctionType>()->isCoroutine());
}

BeginApplyInst *
BeginApplyInst::create(PILDebugLocation loc, PILValue callee,
                       SubstitutionMap subs, ArrayRef<PILValue> args,
                       bool isNonThrowing,
                       Optional<PILModuleConventions> moduleConventions,
                       PILFunction &F,
                       PILOpenedArchetypesState &openedArchetypes,
                       const GenericSpecializationInformation *specializationInfo) {
   PILType substCalleePILType = callee->getType().substGenericArgs(
      F.getModule(), subs, F.getTypeExpansionContext());
   auto substCalleeType = substCalleePILType.castTo<PILFunctionType>();

   PILFunctionConventions conv(substCalleeType,
                               moduleConventions.hasValue()
                               ? moduleConventions.getValue()
                               : PILModuleConventions(F.getModule()));

   SmallVector<PILType, 8> resultTypes;
   SmallVector<ValueOwnershipKind, 8> resultOwnerships;

   for (auto &yield : substCalleeType->getYields()) {
      auto yieldType = conv.getPILType(yield);
      auto convention = PILArgumentConvention(yield.getConvention());
      resultTypes.push_back(yieldType);
      resultOwnerships.push_back(
         ValueOwnershipKind(F, yieldType, convention));
   }

   resultTypes.push_back(PILType::getPILTokenType(F.getAstContext()));
   resultOwnerships.push_back(ValueOwnershipKind::None);

   SmallVector<PILValue, 32> typeDependentOperands;
   collectTypeDependentOperands(typeDependentOperands, openedArchetypes, F,
                                substCalleeType, subs);
   void *buffer =
      allocateTrailingInst<BeginApplyInst, Operand,
         MultipleValueInstruction*, BeginApplyResult>(
         F, getNumAllOperands(args, typeDependentOperands),
         1, resultTypes.size());
   return ::new(buffer) BeginApplyInst(loc, callee, substCalleePILType,
                                       resultTypes, resultOwnerships, subs,
                                       args, typeDependentOperands,
                                       isNonThrowing, specializationInfo);
}

void BeginApplyInst::getCoroutineEndPoints(
   SmallVectorImpl<EndApplyInst *> &endApplyInsts,
   SmallVectorImpl<AbortApplyInst *> &abortApplyInsts) const {
   for (auto *tokenUse : getTokenResult()->getUses()) {
      auto *user = tokenUse->getUser();
      if (auto *end = dyn_cast<EndApplyInst>(user)) {
         endApplyInsts.push_back(end);
         continue;
      }

      abortApplyInsts.push_back(cast<AbortApplyInst>(user));
   }
}

void BeginApplyInst::getCoroutineEndPoints(
   SmallVectorImpl<Operand *> &endApplyInsts,
   SmallVectorImpl<Operand *> &abortApplyInsts) const {
   for (auto *tokenUse : getTokenResult()->getUses()) {
      auto *user = tokenUse->getUser();
      if (isa<EndApplyInst>(user)) {
         endApplyInsts.push_back(tokenUse);
         continue;
      }

      assert(isa<AbortApplyInst>(user));
      abortApplyInsts.push_back(tokenUse);
   }
}

bool polar::doesApplyCalleeHaveSemantics(PILValue callee, StringRef semantics) {
   if (auto *FRI = dyn_cast<FunctionRefBaseInst>(callee))
      if (auto *F = FRI->getReferencedFunctionOrNull())
         return F->hasSemanticsAttr(semantics);
   return false;
}

PartialApplyInst::PartialApplyInst(
   PILDebugLocation Loc, PILValue Callee, PILType SubstCalleeTy,
   SubstitutionMap Subs, ArrayRef<PILValue> Args,
   ArrayRef<PILValue> TypeDependentOperands, PILType ClosureType,
   const GenericSpecializationInformation *SpecializationInfo)
// FIXME: the callee should have a lowered PIL function type, and
// PartialApplyInst
// should derive the type of its result by partially applying the callee's
// type.
   : InstructionBase(Loc, Callee, SubstCalleeTy, Subs, Args,
                     TypeDependentOperands, SpecializationInfo, ClosureType) {}

PartialApplyInst *PartialApplyInst::create(
   PILDebugLocation Loc, PILValue Callee, ArrayRef<PILValue> Args,
   SubstitutionMap Subs, ParameterConvention CalleeConvention, PILFunction &F,
   PILOpenedArchetypesState &OpenedArchetypes,
   const GenericSpecializationInformation *SpecializationInfo,
   OnStackKind onStack) {
   PILType SubstCalleeTy = Callee->getType().substGenericArgs(
      F.getModule(), Subs, F.getTypeExpansionContext());
   PILType ClosureType = PILBuilder::getPartialApplyResultType(
      F.getTypeExpansionContext(), SubstCalleeTy, Args.size(), F.getModule(), {},
      CalleeConvention, onStack);

   SmallVector<PILValue, 32> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                SubstCalleeTy.getAstType(), Subs);
   void *Buffer =
      allocateTrailingInst<PartialApplyInst, Operand>(
         F, getNumAllOperands(Args, TypeDependentOperands));
   return ::new(Buffer) PartialApplyInst(Loc, Callee, SubstCalleeTy,
                                         Subs, Args,
                                         TypeDependentOperands, ClosureType,
                                         SpecializationInfo);
}

TryApplyInstBase::TryApplyInstBase(PILInstructionKind kind,
                                   PILDebugLocation loc,
                                   PILBasicBlock *normalBB,
                                   PILBasicBlock *errorBB)
   : TermInst(kind, loc), DestBBs{{this, normalBB}, {this, errorBB}} {}

TryApplyInst::TryApplyInst(
   PILDebugLocation Loc, PILValue callee, PILType substCalleeTy,
   SubstitutionMap subs, ArrayRef<PILValue> args,
   ArrayRef<PILValue> TypeDependentOperands, PILBasicBlock *normalBB,
   PILBasicBlock *errorBB,
   const GenericSpecializationInformation *SpecializationInfo)
   : InstructionBase(Loc, callee, substCalleeTy, subs, args,
                     TypeDependentOperands, SpecializationInfo, normalBB,
                     errorBB) {}

TryApplyInst *TryApplyInst::create(
   PILDebugLocation loc, PILValue callee, SubstitutionMap subs,
   ArrayRef<PILValue> args, PILBasicBlock *normalBB, PILBasicBlock *errorBB,
   PILFunction &F, PILOpenedArchetypesState &openedArchetypes,
   const GenericSpecializationInformation *specializationInfo) {
   PILType substCalleeTy = callee->getType().substGenericArgs(
      F.getModule(), subs, F.getTypeExpansionContext());

   SmallVector<PILValue, 32> typeDependentOperands;
   collectTypeDependentOperands(typeDependentOperands, openedArchetypes, F,
                                substCalleeTy.getAstType(),
                                subs);
   void *buffer =
      allocateTrailingInst<TryApplyInst, Operand>(
         F, getNumAllOperands(args, typeDependentOperands));
   return ::new (buffer) TryApplyInst(loc, callee, substCalleeTy, subs, args,
                                      typeDependentOperands,
                                      normalBB, errorBB, specializationInfo);
}

FunctionRefBaseInst::FunctionRefBaseInst(PILInstructionKind Kind,
                                         PILDebugLocation DebugLoc,
                                         PILFunction *F,
                                         TypeExpansionContext context)
   : LiteralInst(Kind, DebugLoc, F->getLoweredTypeInContext(context)), f(F) {
   F->incrementRefCount();
}

void FunctionRefBaseInst::dropReferencedFunction() {
   if (auto *Function = getInitiallyReferencedFunction())
      Function->decrementRefCount();
   f = nullptr;
}

FunctionRefBaseInst::~FunctionRefBaseInst() {
   if (getInitiallyReferencedFunction())
      getInitiallyReferencedFunction()->decrementRefCount();
}

FunctionRefInst::FunctionRefInst(PILDebugLocation Loc, PILFunction *F,
                                 TypeExpansionContext context)
   : FunctionRefBaseInst(PILInstructionKind::FunctionRefInst, Loc, F,
                         context) {
   assert(!F->isDynamicallyReplaceable());
}

DynamicFunctionRefInst::DynamicFunctionRefInst(PILDebugLocation Loc,
                                               PILFunction *F,
                                               TypeExpansionContext context)
   : FunctionRefBaseInst(PILInstructionKind::DynamicFunctionRefInst, Loc, F,
                         context) {
   assert(F->isDynamicallyReplaceable());
}

PreviousDynamicFunctionRefInst::PreviousDynamicFunctionRefInst(
   PILDebugLocation Loc, PILFunction *F, TypeExpansionContext context)
   : FunctionRefBaseInst(PILInstructionKind::PreviousDynamicFunctionRefInst,
                         Loc, F, context) {
   assert(!F->isDynamicallyReplaceable());
}

AllocGlobalInst::AllocGlobalInst(PILDebugLocation Loc,
                                 PILGlobalVariable *Global)
   : InstructionBase(Loc),
     Global(Global) {}

GlobalAddrInst::GlobalAddrInst(PILDebugLocation DebugLoc,
                               PILGlobalVariable *Global,
                               TypeExpansionContext context)
   : InstructionBase(DebugLoc,
                     Global->getLoweredTypeInContext(context).getAddressType(),
                     Global) {}

GlobalValueInst::GlobalValueInst(PILDebugLocation DebugLoc,
                                 PILGlobalVariable *Global,
                                 TypeExpansionContext context)
   : InstructionBase(DebugLoc,
                     Global->getLoweredTypeInContext(context).getObjectType(),
                     Global) {}

const IntrinsicInfo &BuiltinInst::getIntrinsicInfo() const {
   return getModule().getIntrinsicInfo(getName());
}

const BuiltinInfo &BuiltinInst::getBuiltinInfo() const {
   return getModule().getBuiltinInfo(getName());
}

static unsigned getWordsForBitWidth(unsigned bits) {
   return ((bits + llvm::APInt::APINT_BITS_PER_WORD - 1)
           / llvm::APInt::APINT_BITS_PER_WORD);
}

template<typename INST>
static void *allocateLiteralInstWithTextSize(PILModule &M, unsigned length) {
   return M.allocateInst(sizeof(INST) + length, alignof(INST));
}

template<typename INST>
static void *allocateLiteralInstWithBitSize(PILModule &M, unsigned bits) {
   unsigned words = getWordsForBitWidth(bits);
   return M.allocateInst(
      sizeof(INST) + sizeof(llvm::APInt::WordType)*words, alignof(INST));
}

IntegerLiteralInst::IntegerLiteralInst(PILDebugLocation Loc, PILType Ty,
                                       const llvm::APInt &Value)
   : InstructionBase(Loc, Ty) {
   PILInstruction::Bits.IntegerLiteralInst.numBits = Value.getBitWidth();
   std::uninitialized_copy_n(Value.getRawData(), Value.getNumWords(),
                             getTrailingObjects<llvm::APInt::WordType>());
}

IntegerLiteralInst *IntegerLiteralInst::create(PILDebugLocation Loc,
                                               PILType Ty, const APInt &Value,
                                               PILModule &M) {
#ifndef NDEBUG
   if (auto intTy = Ty.getAs<BuiltinIntegerType>()) {
      assert(intTy->getGreatestWidth() == Value.getBitWidth() &&
             "IntegerLiteralInst APInt value's bit width doesn't match type");
   } else {
      assert(Ty.is<BuiltinIntegerLiteralType>());
      assert(Value.getBitWidth() == Value.getMinSignedBits());
   }
#endif

   void *buf = allocateLiteralInstWithBitSize<IntegerLiteralInst>(M,
                                                                  Value.getBitWidth());
   return ::new (buf) IntegerLiteralInst(Loc, Ty, Value);
}

static APInt getAPInt(AnyBuiltinIntegerType *anyIntTy, intmax_t value) {
   // If we're forming a fixed-width type, build using the greatest width.
   if (auto intTy = dyn_cast<BuiltinIntegerType>(anyIntTy))
      return APInt(intTy->getGreatestWidth(), value);

   // Otherwise, build using the size of the type and then truncate to the
   // minimum width necessary.
   APInt result(8 * sizeof(value), value, /*signed*/ true);
   result = result.trunc(result.getMinSignedBits());
   return result;
}

IntegerLiteralInst *IntegerLiteralInst::create(PILDebugLocation Loc,
                                               PILType Ty, intmax_t Value,
                                               PILModule &M) {
   auto intTy = Ty.castTo<AnyBuiltinIntegerType>();
   return create(Loc, Ty, getAPInt(intTy, Value), M);
}

static PILType getGreatestIntegerType(Type type, PILModule &M) {
   if (auto intTy = type->getAs<BuiltinIntegerType>()) {
      return PILType::getBuiltinIntegerType(intTy->getGreatestWidth(),
                                            M.getAstContext());
   } else {
      assert(type->is<BuiltinIntegerLiteralType>());
      return PILType::getBuiltinIntegerLiteralType(M.getAstContext());
   }
}

IntegerLiteralInst *IntegerLiteralInst::create(IntegerLiteralExpr *E,
                                               PILDebugLocation Loc,
                                               PILModule &M) {
   return create(Loc, getGreatestIntegerType(E->getType(), M), E->getValue(), M);
}

/// getValue - Return the APInt for the underlying integer literal.
APInt IntegerLiteralInst::getValue() const {
   auto numBits = PILInstruction::Bits.IntegerLiteralInst.numBits;
   return APInt(numBits, {getTrailingObjects<llvm::APInt::WordType>(),
                          getWordsForBitWidth(numBits)});
}

FloatLiteralInst::FloatLiteralInst(PILDebugLocation Loc, PILType Ty,
                                   const APInt &Bits)
   : InstructionBase(Loc, Ty) {
   PILInstruction::Bits.FloatLiteralInst.numBits = Bits.getBitWidth();
   std::uninitialized_copy_n(Bits.getRawData(), Bits.getNumWords(),
                             getTrailingObjects<llvm::APInt::WordType>());
}

FloatLiteralInst *FloatLiteralInst::create(PILDebugLocation Loc, PILType Ty,
                                           const APFloat &Value,
                                           PILModule &M) {
   auto floatTy = Ty.castTo<BuiltinFloatType>();
   assert(&floatTy->getAPFloatSemantics() == &Value.getSemantics() &&
          "FloatLiteralInst value's APFloat semantics do not match type");
   (void)floatTy;

   APInt Bits = Value.bitcastToAPInt();

   void *buf = allocateLiteralInstWithBitSize<FloatLiteralInst>(M,
                                                                Bits.getBitWidth());
   return ::new (buf) FloatLiteralInst(Loc, Ty, Bits);
}

FloatLiteralInst *FloatLiteralInst::create(FloatLiteralExpr *E,
                                           PILDebugLocation Loc,
                                           PILModule &M) {
   return create(Loc,
      // Builtin floating-point types are always valid PIL types.
                 PILType::getBuiltinFloatType(
                    E->getType()->castTo<BuiltinFloatType>()->getFPKind(),
                    M.getAstContext()),
                 E->getValue(), M);
}

APInt FloatLiteralInst::getBits() const {
   auto numBits = PILInstruction::Bits.FloatLiteralInst.numBits;
   return APInt(numBits, {getTrailingObjects<llvm::APInt::WordType>(),
                          getWordsForBitWidth(numBits)});
}

APFloat FloatLiteralInst::getValue() const {
   return APFloat(getType().castTo<BuiltinFloatType>()->getAPFloatSemantics(),
                  getBits());
}

StringLiteralInst::StringLiteralInst(PILDebugLocation Loc, StringRef Text,
                                     Encoding encoding, PILType Ty)
   : InstructionBase(Loc, Ty) {
   PILInstruction::Bits.StringLiteralInst.TheEncoding = unsigned(encoding);
   PILInstruction::Bits.StringLiteralInst.Length = Text.size();
   memcpy(getTrailingObjects<char>(), Text.data(), Text.size());
}

StringLiteralInst *StringLiteralInst::create(PILDebugLocation Loc,
                                             StringRef text, Encoding encoding,
                                             PILModule &M) {
   void *buf
      = allocateLiteralInstWithTextSize<StringLiteralInst>(M, text.size());

   auto Ty = PILType::getRawPointerType(M.getAstContext());
   return ::new (buf) StringLiteralInst(Loc, text, encoding, Ty);
}

CondFailInst::CondFailInst(PILDebugLocation DebugLoc, PILValue Operand,
                           StringRef Message)
   : UnaryInstructionBase(DebugLoc, Operand),
     MessageSize(Message.size()) {
   memcpy(getTrailingObjects<char>(), Message.data(), Message.size());
}

CondFailInst *CondFailInst::create(PILDebugLocation DebugLoc, PILValue Operand,
                                   StringRef Message, PILModule &M) {

   auto Size = totalSizeToAlloc<char>(Message.size());
   auto Buffer = M.allocateInst(Size, alignof(CondFailInst));
   return ::new (Buffer) CondFailInst(DebugLoc, Operand, Message);
}

uint64_t StringLiteralInst::getCodeUnitCount() {
   auto E = unsigned(Encoding::UTF16);
   if (PILInstruction::Bits.StringLiteralInst.TheEncoding == E)
      return unicode::getUTF16Length(getValue());
   return PILInstruction::Bits.StringLiteralInst.Length;
}

StoreInst::StoreInst(
   PILDebugLocation Loc, PILValue Src, PILValue Dest,
   StoreOwnershipQualifier Qualifier = StoreOwnershipQualifier::Unqualified)
   : InstructionBase(Loc), Operands(this, Src, Dest) {
   PILInstruction::Bits.StoreInst.OwnershipQualifier = unsigned(Qualifier);
}

StoreBorrowInst::StoreBorrowInst(PILDebugLocation DebugLoc, PILValue Src,
                                 PILValue Dest)
   : InstructionBase(DebugLoc, Dest->getType()),
     Operands(this, Src, Dest) {}

StringRef polar::getPILAccessKindName(PILAccessKind kind) {
   switch (kind) {
      case PILAccessKind::Init: return "init";
      case PILAccessKind::Read: return "read";
      case PILAccessKind::Modify: return "modify";
      case PILAccessKind::Deinit: return "deinit";
   }
   llvm_unreachable("bad access kind");
}

StringRef polar::getPILAccessEnforcementName(PILAccessEnforcement enforcement) {
   switch (enforcement) {
      case PILAccessEnforcement::Unknown: return "unknown";
      case PILAccessEnforcement::Static: return "static";
      case PILAccessEnforcement::Dynamic: return "dynamic";
      case PILAccessEnforcement::Unsafe: return "unsafe";
   }
   llvm_unreachable("bad access enforcement");
}

AssignInst::AssignInst(PILDebugLocation Loc, PILValue Src, PILValue Dest,
                       AssignOwnershipQualifier Qualifier) :
   AssignInstBase(Loc, Src, Dest) {
   PILInstruction::Bits.AssignInst.OwnershipQualifier = unsigned(Qualifier);
}

AssignByWrapperInst::AssignByWrapperInst(PILDebugLocation Loc,
                                         PILValue Src, PILValue Dest,
                                         PILValue Initializer,
                                         PILValue Setter,
                                         AssignOwnershipQualifier Qualifier) :
   AssignInstBase(Loc, Src, Dest, Initializer, Setter) {
   assert(Initializer->getType().is<PILFunctionType>());
   PILInstruction::Bits.AssignByWrapperInst.OwnershipQualifier =
      unsigned(Qualifier);
}

MarkFunctionEscapeInst *
MarkFunctionEscapeInst::create(PILDebugLocation Loc,
                               ArrayRef<PILValue> Elements, PILFunction &F) {
   auto Size = totalSizeToAlloc<polar::Operand>(Elements.size());
   auto Buf = F.getModule().allocateInst(Size, alignof(MarkFunctionEscapeInst));
   return ::new(Buf) MarkFunctionEscapeInst(Loc, Elements);
}

CopyAddrInst::CopyAddrInst(PILDebugLocation Loc, PILValue SrcLValue,
                           PILValue DestLValue, IsTake_t isTakeOfSrc,
                           IsInitialization_t isInitializationOfDest)
   : InstructionBase(Loc), Operands(this, SrcLValue, DestLValue) {
   PILInstruction::Bits.CopyAddrInst.IsTakeOfSrc = bool(isTakeOfSrc);
   PILInstruction::Bits.CopyAddrInst.IsInitializationOfDest =
      bool(isInitializationOfDest);
}

BindMemoryInst *
BindMemoryInst::create(PILDebugLocation Loc, PILValue Base, PILValue Index,
                       PILType BoundType, PILFunction &F,
                       PILOpenedArchetypesState &OpenedArchetypes) {
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                BoundType.getAstType());
   auto Size = totalSizeToAlloc<polar::Operand>(TypeDependentOperands.size() +
                                                NumFixedOpers);
   auto Buffer = F.getModule().allocateInst(Size, alignof(BindMemoryInst));
   return ::new (Buffer) BindMemoryInst(Loc, Base, Index, BoundType,
                                        TypeDependentOperands);
}

UncheckedRefCastAddrInst::UncheckedRefCastAddrInst(PILDebugLocation Loc,
                                                   PILValue src,
                                                   CanType srcType,
                                                   PILValue dest,
                                                   CanType targetType)
   : InstructionBase(Loc),
     Operands(this, src, dest), SourceType(srcType), TargetType(targetType) {}

UnconditionalCheckedCastAddrInst::UnconditionalCheckedCastAddrInst(
   PILDebugLocation Loc, PILValue src, CanType srcType, PILValue dest,
   CanType targetType)
   : InstructionBase(Loc),
     Operands(this, src, dest), SourceType(srcType), TargetType(targetType) {}

StructInst *StructInst::create(PILDebugLocation Loc, PILType Ty,
                               ArrayRef<PILValue> Elements, PILModule &M,
                               bool HasOwnership) {
   auto Size = totalSizeToAlloc<polar::Operand>(Elements.size());
   auto Buffer = M.allocateInst(Size, alignof(StructInst));
   return ::new (Buffer) StructInst(Loc, Ty, Elements, HasOwnership);
}

StructInst::StructInst(PILDebugLocation Loc, PILType Ty,
                       ArrayRef<PILValue> Elems, bool HasOwnership)
   : InstructionBaseWithTrailingOperands(
   Elems, Loc, Ty,
   HasOwnership ? *mergePILValueOwnership(Elems)
                : ValueOwnershipKind(ValueOwnershipKind::None)) {
   assert(!Ty.getStructOrBoundGenericStruct()->hasUnreferenceableStorage());
}

ObjectInst *ObjectInst::create(PILDebugLocation Loc, PILType Ty,
                               ArrayRef<PILValue> Elements,
                               unsigned NumBaseElements, PILModule &M,
                               bool HasOwnership) {
   auto Size = totalSizeToAlloc<polar::Operand>(Elements.size());
   auto Buffer = M.allocateInst(Size, alignof(ObjectInst));
   return ::new (Buffer)
      ObjectInst(Loc, Ty, Elements, NumBaseElements, HasOwnership);
}

TupleInst *TupleInst::create(PILDebugLocation Loc, PILType Ty,
                             ArrayRef<PILValue> Elements, PILModule &M,
                             bool HasOwnership) {
   auto Size = totalSizeToAlloc<polar::Operand>(Elements.size());
   auto Buffer = M.allocateInst(Size, alignof(TupleInst));
   return ::new (Buffer) TupleInst(Loc, Ty, Elements, HasOwnership);
}

bool TupleExtractInst::isTrivialEltOfOneRCIDTuple() const {
   auto *F = getFunction();

   // If we are not trivial, bail.
   if (!getType().isTrivial(*F))
      return false;

   // If the elt we are extracting is trivial, we cannot have any non trivial
   // fields.
   if (getOperand()->getType().isTrivial(*F))
      return false;

   // Ok, now we know that our tuple has non-trivial fields. Make sure that our
   // parent tuple has only one non-trivial field.
   bool FoundNonTrivialField = false;
   PILType OpTy = getOperand()->getType();
   unsigned FieldNo = getFieldNo();

   // For each element index of the tuple...
   for (unsigned i = 0, e = getNumTupleElts(); i != e; ++i) {
      // If the element index is the one we are extracting, skip it...
      if (i == FieldNo)
         continue;

      // Otherwise check if we have a non-trivial type. If we don't have one,
      // continue.
      if (OpTy.getTupleElementType(i).isTrivial(*F))
         continue;

      // Ok, this type is non-trivial. If we have not seen a non-trivial field
      // yet, set the FoundNonTrivialField flag.
      if (!FoundNonTrivialField) {
         FoundNonTrivialField = true;
         continue;
      }

      // If we have seen a field and thus the FoundNonTrivialField flag is set,
      // return false.
      return false;
   }

   // We found only one trivial field.
   assert(FoundNonTrivialField && "Tuple is non-trivial, but does not have a "
                                  "non-trivial element?!");
   return true;
}

bool TupleExtractInst::isEltOnlyNonTrivialElt() const {
   auto *F = getFunction();

   // If the elt we are extracting is trivial, we cannot be a non-trivial
   // field... return false.
   if (getType().isTrivial(*F))
      return false;

   // Ok, we know that the elt we are extracting is non-trivial. Make sure that
   // we have no other non-trivial elts.
   PILType OpTy = getOperand()->getType();
   unsigned FieldNo = getFieldNo();

   // For each element index of the tuple...
   for (unsigned i = 0, e = getNumTupleElts(); i != e; ++i) {
      // If the element index is the one we are extracting, skip it...
      if (i == FieldNo)
         continue;

      // Otherwise check if we have a non-trivial type. If we don't have one,
      // continue.
      if (OpTy.getTupleElementType(i).isTrivial(*F))
         continue;

      // If we do have a non-trivial type, return false. We have multiple
      // non-trivial types violating our condition.
      return false;
   }

   // We checked every other elt of the tuple and did not find any
   // non-trivial elt except for ourselves. Return true.
   return true;
}

unsigned FieldIndexCacheBase::cacheFieldIndex() {
   unsigned i = 0;
   for (VarDecl *property : getParentDecl()->getStoredProperties()) {
      if (field == property) {
         PILInstruction::Bits.FieldIndexCacheBase.FieldIndex = i;
         return i;
      }
      ++i;
   }
   llvm_unreachable("The field decl for a struct_extract, struct_element_addr, "
                    "or ref_element_addr must be an accessible stored property "
                    "of the operand's type");
}

// FIXME: this should be cached during cacheFieldIndex().
bool StructExtractInst::isTrivialFieldOfOneRCIDStruct() const {
   auto *F = getFunction();

   // If we are not trivial, bail.
   if (!getType().isTrivial(*F))
      return false;

   PILType StructTy = getOperand()->getType();

   // If the elt we are extracting is trivial, we cannot have any non trivial
   // fields.
   if (StructTy.isTrivial(*F))
      return false;

   // Ok, now we know that our tuple has non-trivial fields. Make sure that our
   // parent tuple has only one non-trivial field.
   bool FoundNonTrivialField = false;

   // For each element index of the tuple...
   for (VarDecl *D : getStructDecl()->getStoredProperties()) {
      // If the field is the one we are extracting, skip it...
      if (getField() == D)
         continue;

      // Otherwise check if we have a non-trivial type. If we don't have one,
      // continue.
      if (StructTy.getFieldType(D, F->getModule(), TypeExpansionContext(*F))
         .isTrivial(*F))
         continue;

      // Ok, this type is non-trivial. If we have not seen a non-trivial field
      // yet, set the FoundNonTrivialField flag.
      if (!FoundNonTrivialField) {
         FoundNonTrivialField = true;
         continue;
      }

      // If we have seen a field and thus the FoundNonTrivialField flag is set,
      // return false.
      return false;
   }

   // We found only one trivial field.
   assert(FoundNonTrivialField && "Struct is non-trivial, but does not have a "
                                  "non-trivial field?!");
   return true;
}

/// Return true if we are extracting the only non-trivial field of out parent
/// struct. This implies that a ref count operation on the aggregate is
/// equivalent to a ref count operation on this field.
///
/// FIXME: this should be cached during cacheFieldIndex().
bool StructExtractInst::isFieldOnlyNonTrivialField() const {
   auto *F = getFunction();

   // If the field we are extracting is trivial, we cannot be a non-trivial
   // field... return false.
   if (getType().isTrivial(*F))
      return false;

   PILType StructTy = getOperand()->getType();

   // Ok, we are visiting a non-trivial field. Then for every stored field...
   for (VarDecl *D : getStructDecl()->getStoredProperties()) {
      // If we are visiting our own field continue.
      if (getField() == D)
         continue;

      // Ok, we have a field that is not equal to the field we are
      // extracting. If that field is trivial, we do not care about
      // it... continue.
      if (StructTy.getFieldType(D, F->getModule(), TypeExpansionContext(*F))
         .isTrivial(*F))
         continue;

      // We have found a non trivial member that is not the member we are
      // extracting, fail.
      return false;
   }

   // We checked every other field of the struct and did not find any
   // non-trivial fields except for ourselves. Return true.
   return true;
}

//===----------------------------------------------------------------------===//
// Instructions representing terminators
//===----------------------------------------------------------------------===//


TermInst::SuccessorListTy TermInst::getSuccessors() {
   switch (getKind()) {
#define TERMINATOR(ID, NAME, PARENT, MEMBEHAVIOR, MAYRELEASE) \
  case PILInstructionKind::ID: return cast<ID>(this)->getSuccessors();
#include "polarphp/pil/lang/PILNodesDef.h"
      default: llvm_unreachable("not a terminator");
   }
   llvm_unreachable("bad instruction kind");
}

bool TermInst::isFunctionExiting() const {
   switch (getTermKind()) {
      case TermKind::BranchInst:
      case TermKind::CondBranchInst:
      case TermKind::SwitchValueInst:
      case TermKind::SwitchEnumInst:
      case TermKind::SwitchEnumAddrInst:
      case TermKind::DynamicMethodBranchInst:
      case TermKind::CheckedCastBranchInst:
      case TermKind::CheckedCastValueBranchInst:
      case TermKind::CheckedCastAddrBranchInst:
      case TermKind::UnreachableInst:
      case TermKind::TryApplyInst:
      case TermKind::YieldInst:
         return false;
      case TermKind::ReturnInst:
      case TermKind::ThrowInst:
      case TermKind::UnwindInst:
         return true;
   }

   llvm_unreachable("Unhandled TermKind in switch.");
}

bool TermInst::isProgramTerminating() const {
   switch (getTermKind()) {
      case TermKind::BranchInst:
      case TermKind::CondBranchInst:
      case TermKind::SwitchValueInst:
      case TermKind::SwitchEnumInst:
      case TermKind::SwitchEnumAddrInst:
      case TermKind::DynamicMethodBranchInst:
      case TermKind::CheckedCastBranchInst:
      case TermKind::CheckedCastValueBranchInst:
      case TermKind::CheckedCastAddrBranchInst:
      case TermKind::ReturnInst:
      case TermKind::ThrowInst:
      case TermKind::UnwindInst:
      case TermKind::TryApplyInst:
      case TermKind::YieldInst:
         return false;
      case TermKind::UnreachableInst:
         return true;
   }

   llvm_unreachable("Unhandled TermKind in switch.");
}

TermInst::SuccessorBlockArgumentsListTy
TermInst::getSuccessorBlockArguments() const {
   function_ref<PILPhiArgumentArrayRef(const PILSuccessor &)> op;
   op = [](const PILSuccessor &succ) -> PILPhiArgumentArrayRef {
      return succ.getBB()->getPILPhiArguments();
   };
   return SuccessorBlockArgumentsListTy(getSuccessors(), op);
}

YieldInst *YieldInst::create(PILDebugLocation loc,
                             ArrayRef<PILValue> yieldedValues,
                             PILBasicBlock *normalBB, PILBasicBlock *unwindBB,
                             PILFunction &F) {
   auto Size = totalSizeToAlloc<polar::Operand>(yieldedValues.size());
   void *Buffer = F.getModule().allocateInst(Size, alignof(YieldInst));
   return ::new (Buffer) YieldInst(loc, yieldedValues, normalBB, unwindBB);
}

PILYieldInfo YieldInst::getYieldInfoForOperand(const Operand &op) const {
   // We expect op to be our operand.
   assert(op.getUser() == this);
   auto conv = getFunction()->getConventions();
   return conv.getYieldInfoForOperandIndex(op.getOperandNumber());
}

PILArgumentConvention
YieldInst::getArgumentConventionForOperand(const Operand &op) const {
   auto conv = getYieldInfoForOperand(op).getConvention();
   return PILArgumentConvention(conv);
}

BranchInst *BranchInst::create(PILDebugLocation Loc, PILBasicBlock *DestBB,
                               PILFunction &F) {
   return create(Loc, DestBB, {}, F);
}

BranchInst *BranchInst::create(PILDebugLocation Loc,
                               PILBasicBlock *DestBB, ArrayRef<PILValue> Args,
                               PILFunction &F) {
   auto Size = totalSizeToAlloc<polar::Operand>(Args.size());
   auto Buffer = F.getModule().allocateInst(Size, alignof(BranchInst));
   return ::new (Buffer) BranchInst(Loc, DestBB, Args);
}

CondBranchInst::CondBranchInst(PILDebugLocation Loc, PILValue Condition,
                               PILBasicBlock *TrueBB, PILBasicBlock *FalseBB,
                               ArrayRef<PILValue> Args, unsigned NumTrue,
                               unsigned NumFalse, ProfileCounter TrueBBCount,
                               ProfileCounter FalseBBCount)
   : InstructionBaseWithTrailingOperands(Condition, Args, Loc),
     DestBBs{{this, TrueBB, TrueBBCount},
             {this, FalseBB, FalseBBCount}} {
   assert(Args.size() == (NumTrue + NumFalse) && "Invalid number of args");
   PILInstruction::Bits.CondBranchInst.NumTrueArgs = NumTrue;
   assert(PILInstruction::Bits.CondBranchInst.NumTrueArgs == NumTrue &&
          "Truncation");
   assert(TrueBB != FalseBB && "Identical destinations");
}

CondBranchInst *CondBranchInst::create(PILDebugLocation Loc, PILValue Condition,
                                       PILBasicBlock *TrueBB,
                                       PILBasicBlock *FalseBB,
                                       ProfileCounter TrueBBCount,
                                       ProfileCounter FalseBBCount,
                                       PILFunction &F) {
   return create(Loc, Condition, TrueBB, {}, FalseBB, {}, TrueBBCount,
                 FalseBBCount, F);
}

CondBranchInst *
CondBranchInst::create(PILDebugLocation Loc, PILValue Condition,
                       PILBasicBlock *TrueBB, ArrayRef<PILValue> TrueArgs,
                       PILBasicBlock *FalseBB, ArrayRef<PILValue> FalseArgs,
                       ProfileCounter TrueBBCount, ProfileCounter FalseBBCount,
                       PILFunction &F) {
   SmallVector<PILValue, 4> Args;
   Args.append(TrueArgs.begin(), TrueArgs.end());
   Args.append(FalseArgs.begin(), FalseArgs.end());

   auto Size = totalSizeToAlloc<polar::Operand>(Args.size() + NumFixedOpers);
   auto Buffer = F.getModule().allocateInst(Size, alignof(CondBranchInst));
   return ::new (Buffer) CondBranchInst(Loc, Condition, TrueBB, FalseBB, Args,
                                        TrueArgs.size(), FalseArgs.size(),
                                        TrueBBCount, FalseBBCount);
}

PILValue CondBranchInst::getArgForDestBB(const PILBasicBlock *DestBB,
                                         const PILArgument *Arg) const {
   return getArgForDestBB(DestBB, Arg->getIndex());
}

PILValue CondBranchInst::getArgForDestBB(const PILBasicBlock *DestBB,
                                         unsigned ArgIndex) const {
   // If TrueBB and FalseBB equal, we cannot find an arg for this DestBB so
   // return an empty PILValue.
   if (getTrueBB() == getFalseBB()) {
      assert(DestBB == getTrueBB() && "DestBB is not a target of this cond_br");
      return PILValue();
   }

   if (DestBB == getTrueBB())
      return getAllOperands()[NumFixedOpers + ArgIndex].get();

   assert(DestBB == getFalseBB()
          && "By process of elimination BB must be false BB");
   return getAllOperands()[NumFixedOpers + getNumTrueArgs() + ArgIndex].get();
}

void CondBranchInst::swapSuccessors() {
   // Swap our destinations.
   PILBasicBlock *First = DestBBs[0].getBB();
   DestBBs[0] = DestBBs[1].getBB();
   DestBBs[1] = First;

   // If we don't have any arguments return.
   if (!getNumTrueArgs() && !getNumFalseArgs())
      return;

   // Otherwise swap our true and false arguments.
   MutableArrayRef<Operand> Ops = getAllOperands();
   llvm::SmallVector<PILValue, 4> TrueOps;
   for (PILValue V : getTrueArgs())
      TrueOps.push_back(V);

   auto FalseArgs = getFalseArgs();
   for (unsigned i = 0, e = getNumFalseArgs(); i < e; ++i) {
      Ops[NumFixedOpers+i].set(FalseArgs[i]);
   }

   for (unsigned i = 0, e = getNumTrueArgs(); i < e; ++i) {
      Ops[NumFixedOpers+i+getNumFalseArgs()].set(TrueOps[i]);
   }

   // Finally swap the number of arguments that we have. The number of false
   // arguments is derived from the number of true arguments, therefore:
   PILInstruction::Bits.CondBranchInst.NumTrueArgs = getNumFalseArgs();
}

SwitchValueInst::SwitchValueInst(PILDebugLocation Loc, PILValue Operand,
                                 PILBasicBlock *DefaultBB,
                                 ArrayRef<PILValue> Cases,
                                 ArrayRef<PILBasicBlock *> BBs)
   : InstructionBaseWithTrailingOperands(Operand, Cases, Loc) {
   PILInstruction::Bits.SwitchValueInst.HasDefault = bool(DefaultBB);
   // Initialize the successor array.
   auto *succs = getSuccessorBuf();
   unsigned OperandBitWidth = 0;

   if (auto OperandTy = Operand->getType().getAs<BuiltinIntegerType>()) {
      OperandBitWidth = OperandTy->getGreatestWidth();
   }

   for (unsigned i = 0, size = Cases.size(); i < size; ++i) {
      // If we have undef, just add the case and continue.
      if (isa<PILUndef>(Cases[i])) {
         ::new (succs + i) PILSuccessor(this, BBs[i]);
         continue;
      }

      if (OperandBitWidth) {
         auto *IL = dyn_cast<IntegerLiteralInst>(Cases[i]);
         assert(IL && "switch_value case value should be of an integer type");
         assert(IL->getValue().getBitWidth() == OperandBitWidth &&
                "switch_value case value is not same bit width as operand");
         (void)IL;
      } else {
         auto *FR = dyn_cast<FunctionRefInst>(Cases[i]);
         if (!FR) {
            if (auto *CF = dyn_cast<ConvertFunctionInst>(Cases[i])) {
               FR = dyn_cast<FunctionRefInst>(CF->getOperand());
            }
         }
         assert(FR && "switch_value case value should be a function reference");
      }
      ::new (succs + i) PILSuccessor(this, BBs[i]);
   }

   if (hasDefault())
      ::new (succs + getNumCases()) PILSuccessor(this, DefaultBB);
}

SwitchValueInst::~SwitchValueInst() {
   // Destroy the successor records to keep the CFG up to date.
   auto *succs = getSuccessorBuf();
   for (unsigned i = 0, end = getNumCases() + hasDefault(); i < end; ++i) {
      succs[i].~PILSuccessor();
   }
}

SwitchValueInst *SwitchValueInst::create(
   PILDebugLocation Loc, PILValue Operand, PILBasicBlock *DefaultBB,
   ArrayRef<std::pair<PILValue, PILBasicBlock *>> CaseBBs, PILFunction &F) {
   // Allocate enough room for the instruction with tail-allocated data for all
   // the case values and the PILSuccessor arrays. There are `CaseBBs.size()`
   // PILValues and `CaseBBs.size() + (DefaultBB ? 1 : 0)` successors.
   SmallVector<PILValue, 8> Cases;
   SmallVector<PILBasicBlock *, 8> BBs;
   unsigned numCases = CaseBBs.size();
   unsigned numSuccessors = numCases + (DefaultBB ? 1 : 0);
   for (auto pair: CaseBBs) {
      Cases.push_back(pair.first);
      BBs.push_back(pair.second);
   }
   auto size = totalSizeToAlloc<polar::Operand, PILSuccessor>(numCases + 1,
                                                              numSuccessors);
   auto buf = F.getModule().allocateInst(size, alignof(SwitchValueInst));
   return ::new (buf) SwitchValueInst(Loc, Operand, DefaultBB, Cases, BBs);
}

SelectValueInst::SelectValueInst(PILDebugLocation DebugLoc, PILValue Operand,
                                 PILType Type, PILValue DefaultResult,
                                 ArrayRef<PILValue> CaseValuesAndResults,
                                 bool HasOwnership)
   : InstructionBaseWithTrailingOperands(
   Operand, CaseValuesAndResults, DebugLoc, Type,
   HasOwnership ? *mergePILValueOwnership(CaseValuesAndResults)
                : ValueOwnershipKind(ValueOwnershipKind::None)) {}

SelectValueInst *
SelectValueInst::create(PILDebugLocation Loc, PILValue Operand, PILType Type,
                        PILValue DefaultResult,
                        ArrayRef<std::pair<PILValue, PILValue>> CaseValues,
                        PILModule &M, bool HasOwnership) {
   // Allocate enough room for the instruction with tail-allocated data for all
   // the case values and the PILSuccessor arrays. There are `CaseBBs.size()`
   // PILValues and `CaseBBs.size() + (DefaultBB ? 1 : 0)` successors.
   SmallVector<PILValue, 8> CaseValuesAndResults;
   for (auto pair : CaseValues) {
      CaseValuesAndResults.push_back(pair.first);
      CaseValuesAndResults.push_back(pair.second);
   }

   if ((bool)DefaultResult)
      CaseValuesAndResults.push_back(DefaultResult);

   auto Size = totalSizeToAlloc<polar::Operand>(CaseValuesAndResults.size() + 1);
   auto Buf = M.allocateInst(Size, alignof(SelectValueInst));
   return ::new (Buf) SelectValueInst(Loc, Operand, Type, DefaultResult,
                                      CaseValuesAndResults, HasOwnership);
}

template <typename SELECT_ENUM_INST>
SELECT_ENUM_INST *SelectEnumInstBase::createSelectEnum(
   PILDebugLocation Loc, PILValue Operand, PILType Ty, PILValue DefaultValue,
   ArrayRef<std::pair<EnumElementDecl *, PILValue>> DeclsAndValues,
   PILModule &Mod, Optional<ArrayRef<ProfileCounter>> CaseCounts,
   ProfileCounter DefaultCount, bool HasOwnership) {
   // Allocate enough room for the instruction with tail-allocated
   // EnumElementDecl and operand arrays. There are `CaseBBs.size()` decls
   // and `CaseBBs.size() + (DefaultBB ? 1 : 0)` values.
   SmallVector<PILValue, 4> CaseValues;
   SmallVector<EnumElementDecl*, 4> CaseDecls;
   for (auto &pair : DeclsAndValues) {
      CaseValues.push_back(pair.second);
      CaseDecls.push_back(pair.first);
   }

   if (DefaultValue)
      CaseValues.push_back(DefaultValue);

   auto Size = SELECT_ENUM_INST::template
   totalSizeToAlloc<polar::Operand, EnumElementDecl*>(CaseValues.size() + 1,
                                                      CaseDecls.size());
   auto Buf = Mod.allocateInst(Size + sizeof(ProfileCounter),
                               alignof(SELECT_ENUM_INST));
   return ::new (Buf)
      SELECT_ENUM_INST(Loc, Operand, Ty, bool(DefaultValue), CaseValues,
                       CaseDecls, CaseCounts, DefaultCount, HasOwnership);
}

SelectEnumInst *SelectEnumInst::create(
   PILDebugLocation Loc, PILValue Operand, PILType Type, PILValue DefaultValue,
   ArrayRef<std::pair<EnumElementDecl *, PILValue>> CaseValues, PILModule &M,
   Optional<ArrayRef<ProfileCounter>> CaseCounts, ProfileCounter DefaultCount,
   bool HasOwnership) {
   return createSelectEnum<SelectEnumInst>(Loc, Operand, Type, DefaultValue,
                                           CaseValues, M, CaseCounts,
                                           DefaultCount, HasOwnership);
}

SelectEnumAddrInst *SelectEnumAddrInst::create(
   PILDebugLocation Loc, PILValue Operand, PILType Type, PILValue DefaultValue,
   ArrayRef<std::pair<EnumElementDecl *, PILValue>> CaseValues, PILModule &M,
   Optional<ArrayRef<ProfileCounter>> CaseCounts,
   ProfileCounter DefaultCount) {
   // We always pass in false since SelectEnumAddrInst doesn't use ownership. We
   // have to pass something in since SelectEnumInst /does/ need to consider
   // ownership and both use the same creation function.
   return createSelectEnum<SelectEnumAddrInst>(
      Loc, Operand, Type, DefaultValue, CaseValues, M, CaseCounts, DefaultCount,
      false /*HasOwnership*/);
}

SwitchEnumInstBase::SwitchEnumInstBase(
   PILInstructionKind Kind, PILDebugLocation Loc, PILValue Operand,
   PILBasicBlock *DefaultBB,
   ArrayRef<std::pair<EnumElementDecl *, PILBasicBlock *>> CaseBBs,
   Optional<ArrayRef<ProfileCounter>> CaseCounts, ProfileCounter DefaultCount)
   : TermInst(Kind, Loc), Operands(this, Operand) {
   PILInstruction::Bits.SwitchEnumInstBase.HasDefault = bool(DefaultBB);
   PILInstruction::Bits.SwitchEnumInstBase.NumCases = CaseBBs.size();
   // Initialize the case and successor arrays.
   auto *cases = getCaseBuf();
   auto *succs = getSuccessorBuf();
   for (unsigned i = 0, size = CaseBBs.size(); i < size; ++i) {
      cases[i] = CaseBBs[i].first;
      if (CaseCounts) {
         ::new (succs + i)
            PILSuccessor(this, CaseBBs[i].second, CaseCounts.getValue()[i]);
      } else {
         ::new (succs + i) PILSuccessor(this, CaseBBs[i].second);
      }
   }

   if (hasDefault()) {
      ::new (succs + getNumCases()) PILSuccessor(this, DefaultBB, DefaultCount);
   }
}

void SwitchEnumInstBase::swapCase(unsigned i, unsigned j) {
   assert(i < getNumCases() && "First index is out of bounds?!");
   assert(j < getNumCases() && "Second index is out of bounds?!");

   auto *succs = getSuccessorBuf();

   // First grab our destination blocks.
   PILBasicBlock *iBlock = succs[i].getBB();
   PILBasicBlock *jBlock = succs[j].getBB();

   // Then destroy the sil successors and reinitialize them with the new things
   // that they are pointing at.
   succs[i].~PILSuccessor();
   ::new (succs + i) PILSuccessor(this, jBlock);
   succs[j].~PILSuccessor();
   ::new (succs + j) PILSuccessor(this, iBlock);

   // Now swap our cases.
   auto *cases = getCaseBuf();
   std::swap(cases[i], cases[j]);
}

namespace {
template <class Inst> EnumElementDecl *
getUniqueCaseForDefaultValue(Inst *inst, PILValue enumValue) {
   assert(inst->hasDefault() && "doesn't have a default");
   PILType enumType = enumValue->getType();

   EnumDecl *decl = enumType.getEnumOrBoundGenericEnum();
   assert(decl && "switch_enum operand is not an enum");

   const PILFunction *F = inst->getFunction();
   if (!decl->isEffectivelyExhaustive(F->getModule().getTypePHPModule(),
                                      F->getResilienceExpansion())) {
      return nullptr;
   }

   llvm::SmallPtrSet<EnumElementDecl *, 4> unswitchedElts;
   for (auto elt : decl->getAllElements())
      unswitchedElts.insert(elt);

   for (unsigned i = 0, e = inst->getNumCases(); i != e; ++i) {
      auto Entry = inst->getCase(i);
      unswitchedElts.erase(Entry.first);
   }

   if (unswitchedElts.size() == 1)
      return *unswitchedElts.begin();

   return nullptr;
}
} // end anonymous namespace

NullablePtr<EnumElementDecl> SelectEnumInstBase::getUniqueCaseForDefault() {
   return getUniqueCaseForDefaultValue(this, getEnumOperand());
}

NullablePtr<EnumElementDecl> SelectEnumInstBase::getSingleTrueElement() const {
   auto SEIType = getType().getAs<BuiltinIntegerType>();
   if (!SEIType)
      return nullptr;
   if (SEIType->getWidth() != BuiltinIntegerWidth::fixed(1))
      return nullptr;

   // Try to find a single literal "true" case.
   Optional<EnumElementDecl*> TrueElement;
   for (unsigned i = 0, e = getNumCases(); i < e; ++i) {
      auto casePair = getCase(i);
      if (auto intLit = dyn_cast<IntegerLiteralInst>(casePair.second)) {
         if (intLit->getValue() == APInt(1, 1)) {
            if (!TrueElement)
               TrueElement = casePair.first;
            else
               // Use Optional(nullptr) to represent more than one.
               TrueElement = Optional<EnumElementDecl*>(nullptr);
         }
      }
   }

   if (!TrueElement || !*TrueElement)
      return nullptr;
   return *TrueElement;
}

SwitchEnumInstBase::~SwitchEnumInstBase() {
   // Destroy the successor records to keep the CFG up to date.
   auto *succs = getSuccessorBuf();
   for (unsigned i = 0, end = getNumCases() + hasDefault(); i < end; ++i) {
      succs[i].~PILSuccessor();
   }
}

template <typename SWITCH_ENUM_INST>
SWITCH_ENUM_INST *SwitchEnumInstBase::createSwitchEnum(
   PILDebugLocation Loc, PILValue Operand, PILBasicBlock *DefaultBB,
   ArrayRef<std::pair<EnumElementDecl *, PILBasicBlock *>> CaseBBs,
   PILFunction &F, Optional<ArrayRef<ProfileCounter>> CaseCounts,
   ProfileCounter DefaultCount) {
   // Allocate enough room for the instruction with tail-allocated
   // EnumElementDecl and PILSuccessor arrays. There are `CaseBBs.size()` decls
   // and `CaseBBs.size() + (DefaultBB ? 1 : 0)` successors.
   unsigned numCases = CaseBBs.size();
   unsigned numSuccessors = numCases + (DefaultBB ? 1 : 0);

   void *buf = F.getModule().allocateInst(
      sizeof(SWITCH_ENUM_INST) + sizeof(EnumElementDecl *) * numCases +
      sizeof(PILSuccessor) * numSuccessors,
      alignof(SWITCH_ENUM_INST));
   return ::new (buf) SWITCH_ENUM_INST(Loc, Operand, DefaultBB, CaseBBs,
                                       CaseCounts, DefaultCount);
}

NullablePtr<EnumElementDecl> SwitchEnumInstBase::getUniqueCaseForDefault() {
   return getUniqueCaseForDefaultValue(this, getOperand());
}

NullablePtr<EnumElementDecl>
SwitchEnumInstBase::getUniqueCaseForDestination(PILBasicBlock *BB) {
   PILValue value = getOperand();
   PILType enumType = value->getType();
   EnumDecl *decl = enumType.getEnumOrBoundGenericEnum();
   assert(decl && "switch_enum operand is not an enum");
   (void)decl;

   EnumElementDecl *D = nullptr;
   for (unsigned i = 0, e = getNumCases(); i != e; ++i) {
      auto Entry = getCase(i);
      if (Entry.second == BB) {
         if (D != nullptr)
            return nullptr;
         D = Entry.first;
      }
   }
   if (!D && hasDefault() && getDefaultBB() == BB) {
      return getUniqueCaseForDefault();
   }
   return D;
}

NullablePtr<PILBasicBlock> SwitchEnumInstBase::getDefaultBBOrNull() const {
   if (!hasDefault())
      return nullptr;
   return getDefaultBB();
}

SwitchEnumInst *SwitchEnumInst::create(
   PILDebugLocation Loc, PILValue Operand, PILBasicBlock *DefaultBB,
   ArrayRef<std::pair<EnumElementDecl *, PILBasicBlock *>> CaseBBs,
   PILFunction &F, Optional<ArrayRef<ProfileCounter>> CaseCounts,
   ProfileCounter DefaultCount) {
   return createSwitchEnum<SwitchEnumInst>(Loc, Operand, DefaultBB, CaseBBs, F,
                                           CaseCounts, DefaultCount);
}

SwitchEnumAddrInst *SwitchEnumAddrInst::create(
   PILDebugLocation Loc, PILValue Operand, PILBasicBlock *DefaultBB,
   ArrayRef<std::pair<EnumElementDecl *, PILBasicBlock *>> CaseBBs,
   PILFunction &F, Optional<ArrayRef<ProfileCounter>> CaseCounts,
   ProfileCounter DefaultCount) {
   return createSwitchEnum<SwitchEnumAddrInst>(Loc, Operand, DefaultBB, CaseBBs,
                                               F, CaseCounts, DefaultCount);
}

DynamicMethodBranchInst::DynamicMethodBranchInst(PILDebugLocation Loc,
                                                 PILValue Operand,
                                                 PILDeclRef Member,
                                                 PILBasicBlock *HasMethodBB,
                                                 PILBasicBlock *NoMethodBB)
   : InstructionBase(Loc),
     Member(Member),
     DestBBs{{this, HasMethodBB}, {this, NoMethodBB}},
     Operands(this, Operand)
{
}

DynamicMethodBranchInst *
DynamicMethodBranchInst::create(PILDebugLocation Loc, PILValue Operand,
                                PILDeclRef Member, PILBasicBlock *HasMethodBB,
                                PILBasicBlock *NoMethodBB, PILFunction &F) {
   void *Buffer = F.getModule().allocateInst(sizeof(DynamicMethodBranchInst),
                                             alignof(DynamicMethodBranchInst));
   return ::new (Buffer)
      DynamicMethodBranchInst(Loc, Operand, Member, HasMethodBB, NoMethodBB);
}

WitnessMethodInst *
WitnessMethodInst::create(PILDebugLocation Loc, CanType LookupType,
                          InterfaceConformanceRef Conformance, PILDeclRef Member,
                          PILType Ty, PILFunction *F,
                          PILOpenedArchetypesState &OpenedArchetypes) {
   assert(cast<InterfaceDecl>(Member.getDecl()->getDeclContext())
          == Conformance.getRequirement());

   PILModule &Mod = F->getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, *F,
                                LookupType);
   auto Size = totalSizeToAlloc<polar::Operand>(TypeDependentOperands.size());
   auto Buffer = Mod.allocateInst(Size, alignof(WitnessMethodInst));

   return ::new (Buffer) WitnessMethodInst(Loc, LookupType, Conformance, Member,
                                           Ty, TypeDependentOperands);
}

ObjCMethodInst *
ObjCMethodInst::create(PILDebugLocation DebugLoc, PILValue Operand,
                       PILDeclRef Member, PILType Ty, PILFunction *F,
                       PILOpenedArchetypesState &OpenedArchetypes) {
   PILModule &Mod = F->getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, *F,
                                Ty.getAstType());

   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());
   void *Buffer = Mod.allocateInst(size, alignof(ObjCMethodInst));
   return ::new (Buffer) ObjCMethodInst(DebugLoc, Operand,
                                        TypeDependentOperands,
                                        Member, Ty);
}

InitExistentialAddrInst *InitExistentialAddrInst::create(
   PILDebugLocation Loc, PILValue Existential, CanType ConcreteType,
   PILType ConcreteLoweredType, ArrayRef<InterfaceConformanceRef> Conformances,
   PILFunction *F, PILOpenedArchetypesState &OpenedArchetypes) {
   PILModule &Mod = F->getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, *F,
                                ConcreteType);
   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());
   void *Buffer = Mod.allocateInst(size,
                                   alignof(InitExistentialAddrInst));
   return ::new (Buffer) InitExistentialAddrInst(Loc, Existential,
                                                 TypeDependentOperands,
                                                 ConcreteType,
                                                 ConcreteLoweredType,
                                                 Conformances);
}

InitExistentialValueInst *InitExistentialValueInst::create(
   PILDebugLocation Loc, PILType ExistentialType, CanType ConcreteType,
   PILValue Instance, ArrayRef<InterfaceConformanceRef> Conformances,
   PILFunction *F, PILOpenedArchetypesState &OpenedArchetypes) {
   PILModule &Mod = F->getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, *F,
                                ConcreteType);
   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());

   void *Buffer = Mod.allocateInst(size, alignof(InitExistentialRefInst));
   return ::new (Buffer)
      InitExistentialValueInst(Loc, ExistentialType, ConcreteType, Instance,
                               TypeDependentOperands, Conformances);
}

InitExistentialRefInst *
InitExistentialRefInst::create(PILDebugLocation Loc, PILType ExistentialType,
                               CanType ConcreteType, PILValue Instance,
                               ArrayRef<InterfaceConformanceRef> Conformances,
                               PILFunction *F,
                               PILOpenedArchetypesState &OpenedArchetypes) {
   PILModule &Mod = F->getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, *F,
                                ConcreteType);
   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());

   void *Buffer = Mod.allocateInst(size,
                                   alignof(InitExistentialRefInst));
   return ::new (Buffer) InitExistentialRefInst(Loc, ExistentialType,
                                                ConcreteType,
                                                Instance,
                                                TypeDependentOperands,
                                                Conformances);
}

InitExistentialMetatypeInst::InitExistentialMetatypeInst(
   PILDebugLocation Loc, PILType existentialMetatypeType, PILValue metatype,
   ArrayRef<PILValue> TypeDependentOperands,
   ArrayRef<InterfaceConformanceRef> conformances)
   : UnaryInstructionWithTypeDependentOperandsBase(Loc, metatype,
                                                   TypeDependentOperands,
                                                   existentialMetatypeType),
     NumConformances(conformances.size()) {
   std::uninitialized_copy(conformances.begin(), conformances.end(),
                           getTrailingObjects<InterfaceConformanceRef>());
}

InitExistentialMetatypeInst *InitExistentialMetatypeInst::create(
   PILDebugLocation Loc, PILType existentialMetatypeType, PILValue metatype,
   ArrayRef<InterfaceConformanceRef> conformances, PILFunction *F,
   PILOpenedArchetypesState &OpenedArchetypes) {
   PILModule &M = F->getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, *F,
                                existentialMetatypeType.getAstType());

   unsigned size = totalSizeToAlloc<polar::Operand, InterfaceConformanceRef>(
      1 + TypeDependentOperands.size(), conformances.size());

   void *buffer = M.allocateInst(size, alignof(InitExistentialMetatypeInst));
   return ::new (buffer) InitExistentialMetatypeInst(
      Loc, existentialMetatypeType, metatype,
      TypeDependentOperands, conformances);
}

ArrayRef<InterfaceConformanceRef>
InitExistentialMetatypeInst::getConformances() const {
   return {getTrailingObjects<InterfaceConformanceRef>(), NumConformances};
}

OpenedExistentialAccess polar::getOpenedExistentialAccessFor(AccessKind access) {
   switch (access) {
      case AccessKind::Read:
         return OpenedExistentialAccess::Immutable;
      case AccessKind::ReadWrite:
      case AccessKind::Write:
         return OpenedExistentialAccess::Mutable;
   }
   llvm_unreachable("Uncovered covered switch?");
}

OpenExistentialAddrInst::OpenExistentialAddrInst(
   PILDebugLocation DebugLoc, PILValue Operand, PILType SelfTy,
   OpenedExistentialAccess AccessKind)
   : UnaryInstructionBase(DebugLoc, Operand, SelfTy), ForAccess(AccessKind) {}

OpenExistentialRefInst::OpenExistentialRefInst(PILDebugLocation DebugLoc,
                                               PILValue Operand, PILType Ty,
                                               bool HasOwnership)
   : UnaryInstructionBase(DebugLoc, Operand, Ty,
                          HasOwnership
                          ? Operand.getOwnershipKind()
                          : ValueOwnershipKind(ValueOwnershipKind::None)) {
   assert(Operand->getType().isObject() && "Operand must be an object.");
   assert(Ty.isObject() && "Result type must be an object type.");
}

OpenExistentialMetatypeInst::OpenExistentialMetatypeInst(
   PILDebugLocation DebugLoc, PILValue operand, PILType ty)
   : UnaryInstructionBase(DebugLoc, operand, ty) {
}

OpenExistentialBoxInst::OpenExistentialBoxInst(
   PILDebugLocation DebugLoc, PILValue operand, PILType ty)
   : UnaryInstructionBase(DebugLoc, operand, ty) {
}

OpenExistentialBoxValueInst::OpenExistentialBoxValueInst(
   PILDebugLocation DebugLoc, PILValue operand, PILType ty)
   : UnaryInstructionBase(DebugLoc, operand, ty) {
}

OpenExistentialValueInst::OpenExistentialValueInst(PILDebugLocation DebugLoc,
                                                   PILValue Operand,
                                                   PILType SelfTy)
   : UnaryInstructionBase(DebugLoc, Operand, SelfTy) {}

UncheckedRefCastInst *
UncheckedRefCastInst::create(PILDebugLocation DebugLoc, PILValue Operand,
                             PILType Ty, PILFunction &F,
                             PILOpenedArchetypesState &OpenedArchetypes) {
   PILModule &Mod = F.getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                Ty.getAstType());
   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());
   void *Buffer = Mod.allocateInst(size, alignof(UncheckedRefCastInst));
   return ::new (Buffer) UncheckedRefCastInst(DebugLoc, Operand,
                                              TypeDependentOperands, Ty);
}

UncheckedAddrCastInst *
UncheckedAddrCastInst::create(PILDebugLocation DebugLoc, PILValue Operand,
                              PILType Ty, PILFunction &F,
                              PILOpenedArchetypesState &OpenedArchetypes) {
   PILModule &Mod = F.getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                Ty.getAstType());
   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());
   void *Buffer = Mod.allocateInst(size, alignof(UncheckedAddrCastInst));
   return ::new (Buffer) UncheckedAddrCastInst(DebugLoc, Operand,
                                               TypeDependentOperands, Ty);
}

UncheckedTrivialBitCastInst *
UncheckedTrivialBitCastInst::create(PILDebugLocation DebugLoc, PILValue Operand,
                                    PILType Ty, PILFunction &F,
                                    PILOpenedArchetypesState &OpenedArchetypes) {
   PILModule &Mod = F.getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                Ty.getAstType());
   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());
   void *Buffer = Mod.allocateInst(size, alignof(UncheckedTrivialBitCastInst));
   return ::new (Buffer) UncheckedTrivialBitCastInst(DebugLoc, Operand,
                                                     TypeDependentOperands,
                                                     Ty);
}

UncheckedBitwiseCastInst *
UncheckedBitwiseCastInst::create(PILDebugLocation DebugLoc, PILValue Operand,
                                 PILType Ty, PILFunction &F,
                                 PILOpenedArchetypesState &OpenedArchetypes) {
   PILModule &Mod = F.getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                Ty.getAstType());
   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());
   void *Buffer = Mod.allocateInst(size, alignof(UncheckedBitwiseCastInst));
   return ::new (Buffer) UncheckedBitwiseCastInst(DebugLoc, Operand,
                                                  TypeDependentOperands, Ty);
}

UnconditionalCheckedCastInst *UnconditionalCheckedCastInst::create(
   PILDebugLocation DebugLoc, PILValue Operand,
   PILType DestLoweredTy, CanType DestFormalTy,
   PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes) {
   PILModule &Mod = F.getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                DestFormalTy);
   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());
   void *Buffer = Mod.allocateInst(size, alignof(UnconditionalCheckedCastInst));
   return ::new (Buffer) UnconditionalCheckedCastInst(DebugLoc, Operand,
                                                      TypeDependentOperands,
                                                      DestLoweredTy,
                                                      DestFormalTy);
}

UnconditionalCheckedCastValueInst *UnconditionalCheckedCastValueInst::create(
   PILDebugLocation DebugLoc,
   PILValue Operand, CanType SrcFormalTy,
   PILType DestLoweredTy, CanType DestFormalTy,
   PILFunction &F, PILOpenedArchetypesState &OpenedArchetypes) {
   PILModule &Mod = F.getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                DestFormalTy);
   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());
   void *Buffer =
      Mod.allocateInst(size, alignof(UnconditionalCheckedCastValueInst));
   return ::new (Buffer) UnconditionalCheckedCastValueInst(
      DebugLoc, Operand, SrcFormalTy, TypeDependentOperands,
      DestLoweredTy, DestFormalTy);
}

CheckedCastBranchInst *CheckedCastBranchInst::create(
   PILDebugLocation DebugLoc, bool IsExact, PILValue Operand,
   PILType DestLoweredTy, CanType DestFormalTy,
   PILBasicBlock *SuccessBB, PILBasicBlock *FailureBB, PILFunction &F,
   PILOpenedArchetypesState &OpenedArchetypes, ProfileCounter Target1Count,
   ProfileCounter Target2Count) {
   PILModule &Mod = F.getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                DestFormalTy);
   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());
   void *Buffer = Mod.allocateInst(size, alignof(CheckedCastBranchInst));
   return ::new (Buffer) CheckedCastBranchInst(
      DebugLoc, IsExact, Operand, TypeDependentOperands,
      DestLoweredTy, DestFormalTy, SuccessBB, FailureBB,
      Target1Count, Target2Count);
}

CheckedCastValueBranchInst *
CheckedCastValueBranchInst::create(PILDebugLocation DebugLoc,
                                   PILValue Operand, CanType SrcFormalTy,
                                   PILType DestLoweredTy, CanType DestFormalTy,
                                   PILBasicBlock *SuccessBB, PILBasicBlock *FailureBB,
                                   PILFunction &F,
                                   PILOpenedArchetypesState &OpenedArchetypes) {
   PILModule &Mod = F.getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                DestFormalTy);
   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());
   void *Buffer = Mod.allocateInst(size, alignof(CheckedCastValueBranchInst));
   return ::new (Buffer) CheckedCastValueBranchInst(
      DebugLoc, Operand, SrcFormalTy, TypeDependentOperands,
      DestLoweredTy, DestFormalTy,
      SuccessBB, FailureBB);
}

MetatypeInst *MetatypeInst::create(PILDebugLocation Loc, PILType Ty,
                                   PILFunction *F,
                                   PILOpenedArchetypesState &OpenedArchetypes) {
   PILModule &Mod = F->getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, *F,
                                Ty.castTo<MetatypeType>().getInstanceType());
   auto Size = totalSizeToAlloc<polar::Operand>(TypeDependentOperands.size());
   auto Buffer = Mod.allocateInst(Size, alignof(MetatypeInst));
   return ::new (Buffer) MetatypeInst(Loc, Ty, TypeDependentOperands);
}

UpcastInst *UpcastInst::create(PILDebugLocation DebugLoc, PILValue Operand,
                               PILType Ty, PILFunction &F,
                               PILOpenedArchetypesState &OpenedArchetypes) {
   PILModule &Mod = F.getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                Ty.getAstType());
   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());
   void *Buffer = Mod.allocateInst(size, alignof(UpcastInst));
   return ::new (Buffer) UpcastInst(DebugLoc, Operand,
                                    TypeDependentOperands, Ty);
}

ThinToThickFunctionInst *
ThinToThickFunctionInst::create(PILDebugLocation DebugLoc, PILValue Operand,
                                PILType Ty, PILFunction &F,
                                PILOpenedArchetypesState &OpenedArchetypes) {
   PILModule &Mod = F.getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                Ty.getAstType());
   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());
   void *Buffer = Mod.allocateInst(size, alignof(ThinToThickFunctionInst));
   return ::new (Buffer) ThinToThickFunctionInst(DebugLoc, Operand,
                                                 TypeDependentOperands, Ty);
}

PointerToThinFunctionInst *
PointerToThinFunctionInst::create(PILDebugLocation DebugLoc, PILValue Operand,
                                  PILType Ty, PILFunction &F,
                                  PILOpenedArchetypesState &OpenedArchetypes) {
   PILModule &Mod = F.getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                Ty.getAstType());
   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());
   void *Buffer = Mod.allocateInst(size, alignof(PointerToThinFunctionInst));
   return ::new (Buffer) PointerToThinFunctionInst(DebugLoc, Operand,
                                                   TypeDependentOperands, Ty);
}

ConvertFunctionInst *ConvertFunctionInst::create(
   PILDebugLocation DebugLoc, PILValue Operand, PILType Ty, PILFunction &F,
   PILOpenedArchetypesState &OpenedArchetypes, bool WithoutActuallyEscaping) {
   PILModule &Mod = F.getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                Ty.getAstType());
   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());
   void *Buffer = Mod.allocateInst(size, alignof(ConvertFunctionInst));
   auto *CFI = ::new (Buffer) ConvertFunctionInst(
      DebugLoc, Operand, TypeDependentOperands, Ty, WithoutActuallyEscaping);
   // If we do not have lowered PIL, make sure that are not performing
   // ABI-incompatible conversions.
   //
   // *NOTE* We purposely do not use an early return here to ensure that in
   // builds without assertions this whole if statement is optimized out.
   if (F.getModule().getStage() != PILStage::Lowered) {
      // Make sure we are not performing ABI-incompatible conversions.
      CanPILFunctionType opTI =
         CFI->getOperand()->getType().castTo<PILFunctionType>();
      (void)opTI;
      CanPILFunctionType resTI = CFI->getType().castTo<PILFunctionType>();
      (void)resTI;
      assert(opTI->isABICompatibleWith(resTI, F).isCompatible() &&
             "Can not convert in between ABI incompatible function types");
   }
   return CFI;
}

ConvertEscapeToNoEscapeInst *ConvertEscapeToNoEscapeInst::create(
   PILDebugLocation DebugLoc, PILValue Operand, PILType Ty, PILFunction &F,
   PILOpenedArchetypesState &OpenedArchetypes, bool isLifetimeGuaranteed) {
   PILModule &Mod = F.getModule();
   SmallVector<PILValue, 8> TypeDependentOperands;
   collectTypeDependentOperands(TypeDependentOperands, OpenedArchetypes, F,
                                Ty.getAstType());
   unsigned size =
      totalSizeToAlloc<polar::Operand>(1 + TypeDependentOperands.size());
   void *Buffer = Mod.allocateInst(size, alignof(ConvertEscapeToNoEscapeInst));
   auto *CFI = ::new (Buffer) ConvertEscapeToNoEscapeInst(
      DebugLoc, Operand, TypeDependentOperands, Ty, isLifetimeGuaranteed);
   // If we do not have lowered PIL, make sure that are not performing
   // ABI-incompatible conversions.
   //
   // *NOTE* We purposely do not use an early return here to ensure that in
   // builds without assertions this whole if statement is optimized out.
   if (F.getModule().getStage() != PILStage::Lowered) {
      // Make sure we are not performing ABI-incompatible conversions.
      CanPILFunctionType opTI =
         CFI->getOperand()->getType().castTo<PILFunctionType>();
      (void)opTI;
      CanPILFunctionType resTI = CFI->getType().castTo<PILFunctionType>();
      (void)resTI;
      assert(opTI->isABICompatibleWith(resTI, F)
                .isCompatibleUpToNoEscapeConversion() &&
             "Can not convert in between ABI incompatible function types");
   }
   return CFI;
}

bool KeyPathPatternComponent::isComputedSettablePropertyMutating() const {
   switch (getKind()) {
      case Kind::StoredProperty:
      case Kind::GettableProperty:
      case Kind::OptionalChain:
      case Kind::OptionalWrap:
      case Kind::OptionalForce:
      case Kind::TupleElement:
         llvm_unreachable("not a settable computed property");
      case Kind::SettableProperty: {
         auto setter = getComputedPropertySetter();
         return setter->getLoweredFunctionType()->getParameters()[1].getConvention()
                == ParameterConvention::Indirect_Inout;
      }
   }
   llvm_unreachable("unhandled kind");
}

static void
forEachRefcountableReference(const KeyPathPatternComponent &component,
                             llvm::function_ref<void (PILFunction*)> forFunction) {
   switch (component.getKind()) {
      case KeyPathPatternComponent::Kind::StoredProperty:
      case KeyPathPatternComponent::Kind::OptionalChain:
      case KeyPathPatternComponent::Kind::OptionalWrap:
      case KeyPathPatternComponent::Kind::OptionalForce:
      case KeyPathPatternComponent::Kind::TupleElement:
         return;
      case KeyPathPatternComponent::Kind::SettableProperty:
         forFunction(component.getComputedPropertySetter());
         LLVM_FALLTHROUGH;
      case KeyPathPatternComponent::Kind::GettableProperty:
         forFunction(component.getComputedPropertyGetter());

         switch (component.getComputedPropertyId().getKind()) {
            case KeyPathPatternComponent::ComputedPropertyId::DeclRef:
               // Mark the vtable entry as used somehow?
               break;
            case KeyPathPatternComponent::ComputedPropertyId::Function:
               forFunction(component.getComputedPropertyId().getFunction());
               break;
            case KeyPathPatternComponent::ComputedPropertyId::Property:
               break;
         }

         if (auto equals = component.getSubscriptIndexEquals())
            forFunction(equals);
         if (auto hash = component.getSubscriptIndexHash())
            forFunction(hash);
         return;
   }
}

void KeyPathPatternComponent::incrementRefCounts() const {
   forEachRefcountableReference(*this,
                                [&](PILFunction *f) { f->incrementRefCount(); });
}
void KeyPathPatternComponent::decrementRefCounts() const {
   forEachRefcountableReference(*this,
                                [&](PILFunction *f) { f->decrementRefCount(); });
}

KeyPathPattern *
KeyPathPattern::get(PILModule &M, CanGenericSignature signature,
                    CanType rootType, CanType valueType,
                    ArrayRef<KeyPathPatternComponent> components,
                    StringRef objcString) {
   llvm::FoldingSetNodeID id;
   Profile(id, signature, rootType, valueType, components, objcString);

   void *insertPos;
   auto existing = M.KeyPathPatterns.FindNodeOrInsertPos(id, insertPos);
   if (existing)
      return existing;

   // Determine the number of operands.
   int maxOperandNo = -1;
   for (auto component : components) {
      switch (component.getKind()) {
         case KeyPathPatternComponent::Kind::StoredProperty:
         case KeyPathPatternComponent::Kind::OptionalChain:
         case KeyPathPatternComponent::Kind::OptionalWrap:
         case KeyPathPatternComponent::Kind::OptionalForce:
         case KeyPathPatternComponent::Kind::TupleElement:
            break;

         case KeyPathPatternComponent::Kind::GettableProperty:
         case KeyPathPatternComponent::Kind::SettableProperty:
            for (auto &index : component.getSubscriptIndices()) {
               maxOperandNo = std::max(maxOperandNo, (int)index.Operand);
            }
      }
   }

   auto newPattern = KeyPathPattern::create(M, signature, rootType, valueType,
                                            components, objcString,
                                            maxOperandNo + 1);
   M.KeyPathPatterns.InsertNode(newPattern, insertPos);
   return newPattern;
}

KeyPathPattern *
KeyPathPattern::create(PILModule &M, CanGenericSignature signature,
                       CanType rootType, CanType valueType,
                       ArrayRef<KeyPathPatternComponent> components,
                       StringRef objcString,
                       unsigned numOperands) {
   auto totalSize = totalSizeToAlloc<KeyPathPatternComponent>(components.size());
   void *mem = M.allocate(totalSize, alignof(KeyPathPatternComponent));
   return ::new (mem) KeyPathPattern(signature, rootType, valueType,
                                     components, objcString, numOperands);
}

KeyPathPattern::KeyPathPattern(CanGenericSignature signature,
                               CanType rootType, CanType valueType,
                               ArrayRef<KeyPathPatternComponent> components,
                               StringRef objcString,
                               unsigned numOperands)
   : NumOperands(numOperands), NumComponents(components.size()),
     Signature(signature), RootType(rootType), ValueType(valueType),
     ObjCString(objcString)
{
   auto *componentsBuf = getTrailingObjects<KeyPathPatternComponent>();
   std::uninitialized_copy(components.begin(), components.end(),
                           componentsBuf);
}

ArrayRef<KeyPathPatternComponent>
KeyPathPattern::getComponents() const {
   return {getTrailingObjects<KeyPathPatternComponent>(), NumComponents};
}

void KeyPathPattern::Profile(llvm::FoldingSetNodeID &ID,
                             CanGenericSignature signature,
                             CanType rootType,
                             CanType valueType,
                             ArrayRef<KeyPathPatternComponent> components,
                             StringRef objcString) {
   ID.AddPointer(signature.getPointer());
   ID.AddPointer(rootType.getPointer());
   ID.AddPointer(valueType.getPointer());
   ID.AddString(objcString);

   auto profileIndices = [&](ArrayRef<KeyPathPatternComponent::Index> indices) {
      for (auto &index : indices) {
         ID.AddInteger(index.Operand);
         ID.AddPointer(index.FormalType.getPointer());
         ID.AddPointer(index.LoweredType.getOpaqueValue());
         ID.AddPointer(index.Hashable.getOpaqueValue());
      }
   };

   for (auto &component : components) {
      ID.AddInteger((unsigned)component.getKind());
      switch (component.getKind()) {
         case KeyPathPatternComponent::Kind::OptionalForce:
         case KeyPathPatternComponent::Kind::OptionalWrap:
         case KeyPathPatternComponent::Kind::OptionalChain:
            break;

         case KeyPathPatternComponent::Kind::StoredProperty:
            ID.AddPointer(component.getStoredPropertyDecl());
            break;

         case KeyPathPatternComponent::Kind::TupleElement:
            ID.AddInteger(component.getTupleIndex());
            break;

         case KeyPathPatternComponent::Kind::SettableProperty:
            ID.AddPointer(component.getComputedPropertySetter());
            LLVM_FALLTHROUGH;
         case KeyPathPatternComponent::Kind::GettableProperty:
            ID.AddPointer(component.getComputedPropertyGetter());
            auto id = component.getComputedPropertyId();
            ID.AddInteger(id.getKind());
            switch (id.getKind()) {
               case KeyPathPatternComponent::ComputedPropertyId::DeclRef: {
                  auto declRef = id.getDeclRef();
                  ID.AddPointer(declRef.loc.getOpaqueValue());
                  ID.AddInteger((unsigned)declRef.kind);
                  ID.AddInteger(declRef.isCurried);
                  ID.AddBoolean(declRef.isCurried);
                  ID.AddBoolean(declRef.isForeign);
                  ID.AddBoolean(declRef.isDirectReference);
                  ID.AddBoolean(declRef.defaultArgIndex);
                  break;
               }
               case KeyPathPatternComponent::ComputedPropertyId::Function: {
                  ID.AddPointer(id.getFunction());
                  break;
               }
               case KeyPathPatternComponent::ComputedPropertyId::Property: {
                  ID.AddPointer(id.getProperty());
                  break;
               }
            }
            profileIndices(component.getSubscriptIndices());
            ID.AddPointer(component.getExternalDecl());
            component.getExternalSubstitutions().profile(ID);
            break;
      }
   }
}

KeyPathInst *
KeyPathInst::create(PILDebugLocation Loc,
                    KeyPathPattern *Pattern,
                    SubstitutionMap Subs,
                    ArrayRef<PILValue> Args,
                    PILType Ty,
                    PILFunction &F) {
   assert(Args.size() == Pattern->getNumOperands()
          && "number of key path args doesn't match pattern");

   auto totalSize = totalSizeToAlloc<Operand>(Args.size());
   void *mem = F.getModule().allocateInst(totalSize, alignof(KeyPathInst));
   return ::new (mem) KeyPathInst(Loc, Pattern, Subs, Args, Ty);
}

KeyPathInst::KeyPathInst(PILDebugLocation Loc,
                         KeyPathPattern *Pattern,
                         SubstitutionMap Subs,
                         ArrayRef<PILValue> Args,
                         PILType Ty)
   : InstructionBase(Loc, Ty),
     Pattern(Pattern),
     NumOperands(Pattern->getNumOperands()),
     Substitutions(Subs)
{
   auto *operandsBuf = getTrailingObjects<Operand>();
   for (unsigned i = 0; i < Args.size(); ++i) {
      ::new ((void*)&operandsBuf[i]) Operand(this, Args[i]);
   }

   // Increment the use of any functions referenced from the keypath pattern.
   for (auto component : Pattern->getComponents()) {
      component.incrementRefCounts();
   }
}

MutableArrayRef<Operand>
KeyPathInst::getAllOperands() {
   return {getTrailingObjects<Operand>(), NumOperands};
}

KeyPathInst::~KeyPathInst() {
   if (!Pattern)
      return;

   // Decrement the use of any functions referenced from the keypath pattern.
   for (auto component : Pattern->getComponents()) {
      component.decrementRefCounts();
   }
   // Destroy operands.
   for (auto &operand : getAllOperands())
      operand.~Operand();
}

KeyPathPattern *KeyPathInst::getPattern() const {
   assert(Pattern && "pattern was reset!");
   return Pattern;
}

void KeyPathInst::dropReferencedPattern() {
   for (auto component : Pattern->getComponents()) {
      component.decrementRefCounts();
   }
   Pattern = nullptr;
}

void KeyPathPatternComponent::
visitReferencedFunctionsAndMethods(
   std::function<void (PILFunction *)> functionCallBack,
   std::function<void (PILDeclRef)> methodCallBack) const {
   switch (getKind()) {
      case KeyPathPatternComponent::Kind::SettableProperty:
         functionCallBack(getComputedPropertySetter());
         LLVM_FALLTHROUGH;
      case KeyPathPatternComponent::Kind::GettableProperty: {
         functionCallBack(getComputedPropertyGetter());
         auto id = getComputedPropertyId();
         switch (id.getKind()) {
            case KeyPathPatternComponent::ComputedPropertyId::DeclRef: {
               methodCallBack(id.getDeclRef());
               break;
            }
            case KeyPathPatternComponent::ComputedPropertyId::Function:
               functionCallBack(id.getFunction());
               break;
            case KeyPathPatternComponent::ComputedPropertyId::Property:
               break;
         }

         if (auto equals = getSubscriptIndexEquals())
            functionCallBack(equals);
         if (auto hash = getSubscriptIndexHash())
            functionCallBack(hash);

         break;
      }
      case KeyPathPatternComponent::Kind::StoredProperty:
      case KeyPathPatternComponent::Kind::OptionalChain:
      case KeyPathPatternComponent::Kind::OptionalForce:
      case KeyPathPatternComponent::Kind::OptionalWrap:
      case KeyPathPatternComponent::Kind::TupleElement:
         break;
   }
}


GenericSpecializationInformation::GenericSpecializationInformation(
   PILFunction *Caller, PILFunction *Parent, SubstitutionMap Subs)
   : Caller(Caller), Parent(Parent), Subs(Subs) {}

const GenericSpecializationInformation *
GenericSpecializationInformation::create(PILFunction *Caller,
                                         PILFunction *Parent,
                                         SubstitutionMap Subs) {
   auto &M = Parent->getModule();
   void *Buf = M.allocate(sizeof(GenericSpecializationInformation),
                          alignof(GenericSpecializationInformation));
   return new (Buf) GenericSpecializationInformation(Caller, Parent, Subs);
}

const GenericSpecializationInformation *
GenericSpecializationInformation::create(PILInstruction *Inst, PILBuilder &B) {
   auto Apply = ApplySite::isa(Inst);
   // Preserve history only for apply instructions for now.
   // NOTE: We may want to preserve history for all instructions in the future,
   // because it may allow us to track their origins.
   assert(Apply);
   auto *F = Inst->getFunction();
   auto &BuilderF = B.getFunction();

   // If cloning inside the same function, don't change the specialization info.
   if (F == &BuilderF) {
      return Apply.getSpecializationInfo();
   }

   // The following lines are used in case of inlining.

   // If a call-site has a history already, simply preserve it.
   if (Apply.getSpecializationInfo())
      return Apply.getSpecializationInfo();

   // If a call-site has no history, use the history of a containing function.
   if (F->isSpecialization())
      return F->getSpecializationInfo();

   return nullptr;
}

static void computeAggregateFirstLevelSubtypeInfo(
   const PILFunction &F, PILValue Operand,
   llvm::SmallVectorImpl<PILType> &Types,
   llvm::SmallVectorImpl<ValueOwnershipKind> &OwnershipKinds) {
   auto &M = F.getModule();
   PILType OpType = Operand->getType();

   // TODO: Create an iterator for accessing first level projections to eliminate
   // this SmallVector.
   llvm::SmallVector<Projection, 8> Projections;
   Projection::getFirstLevelProjections(OpType, M, F.getTypeExpansionContext(),
                                        Projections);

   auto OpOwnershipKind = Operand.getOwnershipKind();
   for (auto &P : Projections) {
      PILType ProjType = P.getType(OpType, M, F.getTypeExpansionContext());
      Types.emplace_back(ProjType);
      OwnershipKinds.emplace_back(
         OpOwnershipKind.getProjectedOwnershipKind(F, ProjType));
   }
}

DestructureStructInst *DestructureStructInst::create(const PILFunction &F,
                                                     PILDebugLocation Loc,
                                                     PILValue Operand) {
   auto &M = F.getModule();

   assert(Operand->getType().getStructOrBoundGenericStruct() &&
          "Expected a struct typed operand?!");

   llvm::SmallVector<PILType, 8> Types;
   llvm::SmallVector<ValueOwnershipKind, 8> OwnershipKinds;
   computeAggregateFirstLevelSubtypeInfo(F, Operand, Types, OwnershipKinds);
   assert(Types.size() == OwnershipKinds.size() &&
          "Expected same number of Types and OwnerKinds");

   unsigned NumElts = Types.size();
   unsigned Size =
      totalSizeToAlloc<MultipleValueInstruction *, DestructureStructResult>(
         1, NumElts);

   void *Buffer = M.allocateInst(Size, alignof(DestructureStructInst));

   return ::new (Buffer)
      DestructureStructInst(M, Loc, Operand, Types, OwnershipKinds);
}

DestructureTupleInst *DestructureTupleInst::create(const PILFunction &F,
                                                   PILDebugLocation Loc,
                                                   PILValue Operand) {
   auto &M = F.getModule();

   assert(Operand->getType().is<TupleType>() &&
          "Expected a tuple typed operand?!");

   llvm::SmallVector<PILType, 8> Types;
   llvm::SmallVector<ValueOwnershipKind, 8> OwnershipKinds;
   computeAggregateFirstLevelSubtypeInfo(F, Operand, Types, OwnershipKinds);
   assert(Types.size() == OwnershipKinds.size() &&
          "Expected same number of Types and OwnerKinds");

   // We add 1 since we store an offset to our
   unsigned NumElts = Types.size();
   unsigned Size =
      totalSizeToAlloc<MultipleValueInstruction *, DestructureTupleResult>(
         1, NumElts);

   void *Buffer = M.allocateInst(Size, alignof(DestructureTupleInst));

   return ::new (Buffer)
      DestructureTupleInst(M, Loc, Operand, Types, OwnershipKinds);
}
