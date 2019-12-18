//===--- PILCombiner.h ------------------------------------------*- C++ -*-===//
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
// A port of LLVM's InstCombiner to PIL. Its main purpose is for performing
// small combining operations/peepholes at the PIL level. It additionally
// performs dead code elimination when it initially adds instructions to the
// work queue in order to reduce compile time by not visiting trivially dead
// instructions.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_OPTIMIZER_INTERNAL_PILCOMBINER_H
#define POLARPHP_PIL_OPTIMIZER_INTERNAL_PILCOMBINER_H

#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILInstructionWorklist.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/lang/PILVisitor.h"
#include "polarphp/pil/optimizer/analysis/ClassHierarchyAnalysis.h"
#include "polarphp/pil/optimizer/analysis/InterfaceConformanceAnalysis.h"
#include "polarphp/pil/optimizer/utils/CastOptimizer.h"
#include "polarphp/pil/optimizer/utils/Existential.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"

namespace polar {

class AliasAnalysis;

/// This is a class which maintains the state of the combiner and simplifies
/// many operations such as removing/adding instructions and syncing them with
/// the worklist.
class PILCombiner :
   public PILInstructionVisitor<PILCombiner, PILInstruction *> {

   AliasAnalysis *AA;

   DominanceAnalysis *DA;

   // Determine the set of types a protocol conforms to in whole-module
   // compilation mode.
   InterfaceConformanceAnalysis *PCA;

   // Class hierarchy analysis needed to confirm no derived classes of a sole
   // conforming class.
   ClassHierarchyAnalysis *CHA;

   /// Worklist containing all of the instructions primed for simplification.
   SmallPILInstructionWorklist<256> Worklist;

   /// Variable to track if the PILCombiner made any changes.
   bool MadeChange;

   /// If set to true then the optimizer is free to erase cond_fail instructions.
   bool RemoveCondFails;

   /// The current iteration of the PILCombine.
   unsigned Iteration;

   /// Builder used to insert instructions.
   PILBuilder &Builder;

   /// Cast optimizer
   CastOptimizer CastOpt;

public:
   PILCombiner(PILOptFunctionBuilder &FuncBuilder, PILBuilder &B,
               AliasAnalysis *AA, DominanceAnalysis *DA,
               InterfaceConformanceAnalysis *PCA, ClassHierarchyAnalysis *CHA,
               bool removeCondFails)
      : AA(AA), DA(DA), PCA(PCA), CHA(CHA), Worklist("SC"), MadeChange(false),
        RemoveCondFails(removeCondFails), Iteration(0), Builder(B),
        CastOpt(FuncBuilder, nullptr /*PILBuilderContext*/,
           /* ReplaceValueUsesAction */
                [&](PILValue Original, PILValue Replacement) {
                   replaceValueUsesWith(Original, Replacement);
                },
           /* ReplaceInstUsesAction */
                [&](SingleValueInstruction *I, ValueBase *V) {
                   replaceInstUsesWith(*I, V);
                },
           /* EraseAction */
                [&](PILInstruction *I) { eraseInstFromFunction(*I); }) {}

   bool runOnFunction(PILFunction &F);

   void clear() {
      Iteration = 0;
      Worklist.resetChecked();
      MadeChange = false;
   }

   // Insert the instruction New before instruction Old in Old's parent BB. Add
   // New to the worklist.
   PILInstruction *insertNewInstBefore(PILInstruction *New,
                                       PILInstruction &Old) {
      return Worklist.insertNewInstBefore(New, Old);
   }

   // This method is to be used when an instruction is found to be dead,
   // replaceable with another preexisting expression. Here we add all uses of I
   // to the worklist, replace all uses of I with the new value, then return I,
   // so that the combiner will know that I was modified.
   void replaceInstUsesWith(SingleValueInstruction &I, ValueBase *V) {
      return Worklist.replaceInstUsesWith(I, V);
   }

   // This method is to be used when a value is found to be dead,
   // replaceable with another preexisting expression. Here we add all
   // uses of oldValue to the worklist, replace all uses of oldValue
   // with newValue.
   void replaceValueUsesWith(PILValue oldValue, PILValue newValue) {
      Worklist.replaceValueUsesWith(oldValue, newValue);
   }

   void replaceInstUsesPairwiseWith(PILInstruction *oldI, PILInstruction *newI) {
      Worklist.replaceInstUsesPairwiseWith(oldI, newI);
   }

   // Some instructions can never be "trivially dead" due to side effects or
   // producing a void value. In those cases, since we cannot rely on
   // PILCombines trivially dead instruction DCE in order to delete the
   // instruction, visit methods should use this method to delete the given
   // instruction and upon completion of their peephole return the value returned
   // by this method.
   PILInstruction *eraseInstFromFunction(PILInstruction &I,
                                         PILBasicBlock::iterator &InstIter,
                                         bool AddOperandsToWorklist = true) {
      Worklist.eraseInstFromFunction(I, InstIter, AddOperandsToWorklist);
      MadeChange = true;
      // Dummy return, so the caller doesn't need to explicitly return nullptr.
      return nullptr;
   }

   PILInstruction *eraseInstFromFunction(PILInstruction &I,
                                         bool AddOperandsToWorklist = true) {
      PILBasicBlock::iterator nullIter;
      return eraseInstFromFunction(I, nullIter, AddOperandsToWorklist);
   }

   void addInitialGroup(ArrayRef<PILInstruction *> List) {
      Worklist.addInitialGroup(List);
   }

   /// Base visitor that does not do anything.
   PILInstruction *visitPILInstruction(PILInstruction *I) { return nullptr; }

   /// Instruction visitors.
   PILInstruction *visitReleaseValueInst(ReleaseValueInst *DI);
   PILInstruction *visitRetainValueInst(RetainValueInst *CI);
   PILInstruction *visitReleaseValueAddrInst(ReleaseValueAddrInst *DI);
   PILInstruction *visitRetainValueAddrInst(RetainValueAddrInst *CI);
   PILInstruction *visitPartialApplyInst(PartialApplyInst *AI);
   PILInstruction *visitApplyInst(ApplyInst *AI);
   PILInstruction *visitBeginApplyInst(BeginApplyInst *BAI);
   PILInstruction *visitTryApplyInst(TryApplyInst *AI);
   PILInstruction *optimizeStringObject(BuiltinInst *BI);
   PILInstruction *visitBuiltinInst(BuiltinInst *BI);
   PILInstruction *visitCondFailInst(CondFailInst *CFI);
   PILInstruction *visitStrongRetainInst(StrongRetainInst *SRI);
   PILInstruction *visitRefToRawPointerInst(RefToRawPointerInst *RRPI);
   PILInstruction *visitUpcastInst(UpcastInst *UCI);
   PILInstruction *optimizeLoadFromStringLiteral(LoadInst *LI);
   PILInstruction *visitLoadInst(LoadInst *LI);
   PILInstruction *visitIndexAddrInst(IndexAddrInst *IA);
   PILInstruction *visitAllocStackInst(AllocStackInst *AS);
   PILInstruction *visitAllocRefInst(AllocRefInst *AR);
   PILInstruction *visitSwitchEnumAddrInst(SwitchEnumAddrInst *SEAI);
   PILInstruction *visitInjectEnumAddrInst(InjectEnumAddrInst *IEAI);
   PILInstruction *visitPointerToAddressInst(PointerToAddressInst *PTAI);
   PILInstruction *visitUncheckedAddrCastInst(UncheckedAddrCastInst *UADCI);
   PILInstruction *visitUncheckedRefCastInst(UncheckedRefCastInst *URCI);
   PILInstruction *visitUncheckedRefCastAddrInst(UncheckedRefCastAddrInst *URCI);
   PILInstruction *visitBridgeObjectToRefInst(BridgeObjectToRefInst *BORI);
   PILInstruction *visitUnconditionalCheckedCastInst(
      UnconditionalCheckedCastInst *UCCI);
   PILInstruction *
   visitUnconditionalCheckedCastAddrInst(UnconditionalCheckedCastAddrInst *UCCAI);
   PILInstruction *visitRawPointerToRefInst(RawPointerToRefInst *RPTR);
   PILInstruction *
   visitUncheckedTakeEnumDataAddrInst(UncheckedTakeEnumDataAddrInst *TEDAI);
   PILInstruction *visitStrongReleaseInst(StrongReleaseInst *SRI);
   PILInstruction *visitCondBranchInst(CondBranchInst *CBI);
   PILInstruction *
   visitUncheckedTrivialBitCastInst(UncheckedTrivialBitCastInst *UTBCI);
   PILInstruction *
   visitUncheckedBitwiseCastInst(UncheckedBitwiseCastInst *UBCI);
   PILInstruction *visitSelectEnumInst(SelectEnumInst *EIT);
   PILInstruction *visitSelectEnumAddrInst(SelectEnumAddrInst *EIT);
   PILInstruction *visitAllocExistentialBoxInst(AllocExistentialBoxInst *S);
   // @todo
//   PILInstruction *visitThickToObjCMetatypeInst(ThickToObjCMetatypeInst *TTOCMI);
//   PILInstruction *visitObjCToThickMetatypeInst(ObjCToThickMetatypeInst *OCTTMI);
   PILInstruction *visitTupleExtractInst(TupleExtractInst *TEI);
   PILInstruction *visitFixLifetimeInst(FixLifetimeInst *FLI);
   PILInstruction *visitSwitchValueInst(SwitchValueInst *SVI);
   PILInstruction *visitSelectValueInst(SelectValueInst *SVI);
   PILInstruction *
   visitCheckedCastAddrBranchInst(CheckedCastAddrBranchInst *CCABI);
   PILInstruction *
   visitCheckedCastBranchInst(CheckedCastBranchInst *CBI);
   PILInstruction *visitUnreachableInst(UnreachableInst *UI);
   PILInstruction *visitAllocRefDynamicInst(AllocRefDynamicInst *ARDI);
   PILInstruction *visitEnumInst(EnumInst *EI);

   PILInstruction *visitMarkDependenceInst(MarkDependenceInst *MDI);
   PILInstruction *visitClassifyBridgeObjectInst(ClassifyBridgeObjectInst *CBOI);
   PILInstruction *visitConvertFunctionInst(ConvertFunctionInst *CFI);
   PILInstruction *
   visitConvertEscapeToNoEscapeInst(ConvertEscapeToNoEscapeInst *Cvt);

   /// Instruction visitor helpers.
   PILInstruction *optimizeBuiltinCanBeObjCClass(BuiltinInst *AI);

   // Optimize the "isConcrete" builtin.
   PILInstruction *optimizeBuiltinIsConcrete(BuiltinInst *I);

   // Optimize the "trunc_N1_M2" builtin. if N1 is a result of "zext_M1_*" and
   // the following holds true: N1 > M1 and M2>= M1
   PILInstruction *optimizeBuiltinTruncOrBitCast(BuiltinInst *I);

   // Optimize the "zext_M2_M3" builtin. if M2 is a result of "zext_M1_M2"
   PILInstruction *optimizeBuiltinZextOrBitCast(BuiltinInst *I);

   // Optimize the "cmp_eq_XXX" builtin. If \p NegateResult is true then negate
   // the result bit.
   PILInstruction *optimizeBuiltinCompareEq(BuiltinInst *AI, bool NegateResult);

   PILInstruction *tryOptimizeApplyOfPartialApply(PartialApplyInst *PAI);

   PILInstruction *optimizeApplyOfConvertFunctionInst(FullApplySite AI,
                                                      ConvertFunctionInst *CFI);

   bool tryOptimizeKeypath(ApplyInst *AI);
   bool tryOptimizeInoutKeypath(BeginApplyInst *AI);

   // Optimize concatenation of string literals.
   // Constant-fold concatenation of string literals known at compile-time.
   PILInstruction *optimizeConcatenationOfStringLiterals(ApplyInst *AI);

   // Optimize an application of f_inverse(f(x)) -> x.
   bool optimizeIdentityCastComposition(ApplyInst *FInverse,
                                        StringRef FInverseName, StringRef FName);

private:
   FullApplySite rewriteApplyCallee(FullApplySite apply, PILValue callee);

   // Build concrete existential information using findInitExistential.
   Optional<ConcreteOpenedExistentialInfo>
   buildConcreteOpenedExistentialInfo(Operand &ArgOperand);

   // Build concrete existential information using SoleConformingType.
   Optional<ConcreteOpenedExistentialInfo>
   buildConcreteOpenedExistentialInfoFromSoleConformingType(Operand &ArgOperand);

   // Common utility function to build concrete existential information for all
   // arguments of an apply instruction.
   void buildConcreteOpenedExistentialInfos(
      FullApplySite Apply,
      llvm::SmallDenseMap<unsigned, ConcreteOpenedExistentialInfo> &COEIs,
      PILBuilderContext &BuilderCtx,
      PILOpenedArchetypesTracker &OpenedArchetypesTracker);

   bool canReplaceArg(FullApplySite Apply, const OpenedArchetypeInfo &OAI,
                      const ConcreteExistentialInfo &CEI, unsigned ArgIdx);

   PILInstruction *createApplyWithConcreteType(
      FullApplySite Apply,
      const llvm::SmallDenseMap<unsigned, ConcreteOpenedExistentialInfo> &COEIs,
      PILBuilderContext &BuilderCtx);

   // Common utility function to replace the WitnessMethodInst using a
   // BuilderCtx.
   void replaceWitnessMethodInst(WitnessMethodInst *WMI,
                                 PILBuilderContext &BuilderCtx,
                                 CanType ConcreteType,
                                 const InterfaceConformanceRef ConformanceRef);

   PILInstruction *
   propagateConcreteTypeOfInitExistential(FullApplySite Apply,
                                          WitnessMethodInst *WMI);

   PILInstruction *propagateConcreteTypeOfInitExistential(FullApplySite Apply);

   /// Propagate concrete types from InterfaceConformanceAnalysis.
   PILInstruction *propagateSoleConformingType(FullApplySite Apply,
                                               WitnessMethodInst *WMI);

   /// Perform one PILCombine iteration.
   bool doOneIteration(PILFunction &F, unsigned Iteration);

   /// Add reachable code to the worklist. Meant to be used when starting to
   /// process a new function.
   void addReachableCodeToWorklist(PILBasicBlock *BB);

   typedef SmallVector<PILInstruction*, 4> UserListTy;

   /// Returns a list of instructions that project or perform reference
   /// counting operations on \p Value or on its uses.
   /// \return return false if \p Value has other than ARC uses.
   static bool recursivelyCollectARCUsers(UserListTy &Uses, ValueBase *Value);

   /// Erases an apply instruction including all it's uses \p.
   /// Inserts release/destroy instructions for all owner and in-parameters.
   /// \return Returns true if successful.
   bool eraseApply(FullApplySite FAS, const UserListTy &Users);

   /// Returns true if the results of a try_apply are not used.
   static bool isTryApplyResultNotUsed(UserListTy &AcceptedUses,
                                       TryApplyInst *TAI);
};

} // end namespace polar

#endif // POLARPHP_PIL_OPTIMIZER_INTERNAL_PILCOMBINER_H
