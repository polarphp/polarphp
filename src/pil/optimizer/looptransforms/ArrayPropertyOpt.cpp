//===--- ArrayPropertyOpt.cpp - Optimize Array Properties -----------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
///
/// Optimize array property access by specializing loop bodies.
///
/// This optimization specializes loops with calls to
/// "array.props.isNative/needsElementTypeCheck".
///
/// The "array.props.isNative/needsElementTypeCheck" predicate has the property
/// that if it is true/false respectively for the array struct it is true/false
/// respectively until somebody writes a new array struct over the memory
/// location. Less abstractly, a fast native swift array does not transition to
/// a slow array (be it a cocoa array, or be it an array that needs type
/// checking) except if we store a new array to the variable that holds it.
///
/// Using this property we can hoist the predicate above a region where no such
/// store can take place.
///
///  func f(a : A[AClass]) {
///     for i in 0..a.count {
///       let b = a.props.isNative()
///        .. += _getElement(i, b)
///     }
///  }
///
///   ==>
///
///  func f(a : A[AClass]) {
///    let b = a.props.isNative
///    if (b) {
///      for i in 0..a.count {
///         .. += _getElement(i, false)
///      }
///    } else {
///      for i in 0..a.count {
///        let a = a.props.isNative
///        .. += _getElement(i, a)
///      }
///    }
///  }
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "array-property-opt"

#include "polarphp/pil/optimizer/internal/looptransforms/ArrayOpt.h"
#include "polarphp/pil/optimizer/analysis/LoopAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/CFGOptUtils.h"
#include "polarphp/pil/optimizer/utils/PILSSAUpdater.h"
#include "polarphp/pil/lang/DebugUtils.h"
#include "polarphp/pil/lang/InstructionUtils.h"
#include "polarphp/pil/lang/Projection.h"
#include "polarphp/pil/lang/LoopInfo.h"
#include "polarphp/pil/lang/PILCloner.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

using namespace polar;

namespace {
/// Analysis whether it is safe to specialize this loop nest based on the
/// array.props function calls it contains. It is safe to hoist array.props
/// calls if the array does not escape such that the array container could be
/// overwritten in the hoisted region.
/// This analysis also checks if we can clone the instructions in the loop nest.
class ArrayPropertiesAnalysis {
   using UserList = StructUseCollector::UserList;
   using UserOperList = StructUseCollector::UserOperList;

   PILFunction *Fun;
   PILLoop *Loop;
   PILBasicBlock *Preheader;
   DominanceInfo *DomTree;

   llvm::SmallSet<PILValue, 16> HoistableArray;

   SmallPtrSet<PILBasicBlock *, 16> ReachingBlocks;
   SmallPtrSet<PILBasicBlock *, 16> CachedExitingBlocks;
public:
   ArrayPropertiesAnalysis(PILLoop *L, DominanceAnalysis *DA)
      : Fun(L->getHeader()->getParent()), Loop(L), Preheader(nullptr),
        DomTree(DA->get(Fun)) {}

   bool run() {
      Preheader = Loop->getLoopPreheader();
      if (!Preheader) {
         LLVM_DEBUG(llvm::dbgs() << "ArrayPropertiesAnalysis: "
                                    "Missing preheader for "
                                 << *Loop);
         return false;
      }

      // Check whether this is a 'array.props' instruction and whether we
      // can hoist it. Heuristic: We only want to hoist array.props instructions
      // if we can hoist all of them - only then can we get rid of all the
      // control-flow if we specialize. Hoisting some but not others is not as
      // beneficial. This heuristic also simplifies which regions we want to
      // specialize on. We will specialize the outermost loopnest that has
      // 'array.props' instructions in its preheader.
      bool FoundHoistable = false;
      for (auto *BB : Loop->getBlocks()) {
         for (auto &Inst : *BB) {

            // Can't clone alloc_stack instructions whose dealloc_stack is outside
            // the loop.
            if (!Loop->canDuplicate(&Inst))
               return false;

            ArraySemanticsCall ArrayPropsInst(&Inst, "array.props", true);
            if (!ArrayPropsInst)
               continue;

            if (!canHoistArrayPropsInst(ArrayPropsInst))
               return false;
            FoundHoistable = true;
         }
      }

      return FoundHoistable;
   }

private:

   /// Strip the struct load and the address projection to the location
   /// holding the array struct.
   PILValue stripArrayStructLoad(PILValue V) {
      if (auto LI = dyn_cast<LoadInst>(V)) {
         auto Val = LI->getOperand();
         // We could have two arrays in a surrounding container so we can only
         // strip off the 'array struct' project.
         // struct Container {
         //   var a1 : [ClassA]
         //   var a2 : [ClassA]
         // }
         // 'a1' and 'a2' are different arrays.
         if (auto SEAI = dyn_cast<StructElementAddrInst>(Val))
            Val = SEAI->getOperand();
         return Val;
      }
      return V;
   }

   SmallPtrSetImpl<PILBasicBlock *> &getReachingBlocks() {
      if (ReachingBlocks.empty()) {
         SmallVector<PILBasicBlock *, 8> Worklist;
         ReachingBlocks.insert(Preheader);
         Worklist.push_back(Preheader);
         while (!Worklist.empty()) {
            PILBasicBlock *BB = Worklist.pop_back_val();
            for (auto PI = BB->pred_begin(), PE = BB->pred_end(); PI != PE; ++PI) {
               if (ReachingBlocks.insert(*PI).second)
                  Worklist.push_back(*PI);
            }
         }
      }
      return ReachingBlocks;
   }

   /// Array address uses are safe if they don't store to the array struct. We
   /// could for example store an NSArray array struct on top of the array. For
   /// example, an opaque function that uses the array's address could store a
   /// new array onto it.
   bool checkSafeArrayAddressUses(UserList &AddressUsers) {
      for (auto *UseInst : AddressUsers) {

         if (UseInst->isDebugInstruction())
            continue;

         if (isa<DeallocStackInst>(UseInst)) {
            // Handle destruction of a local array.
            continue;
         }

         if (auto *AI = dyn_cast<ApplyInst>(UseInst)) {
            if (ArraySemanticsCall(AI))
               continue;

            // Check if this escape can reach the current loop.
            if (!Loop->contains(UseInst->getParent()) &&
                !getReachingBlocks().count(UseInst->getParent())) {
               continue;
            }
            LLVM_DEBUG(llvm::dbgs()
                          << "    Skipping Array: may escape through call!\n"
                          << "    " << *UseInst);
            return false;
         }

         if (auto *StInst = dyn_cast<StoreInst>(UseInst)) {
            // Allow a local array to be initialized outside the loop via a by-value
            // argument or return value. The array value may be returned by its
            // initializer or some other factory function.
            if (Loop->contains(StInst->getParent())) {
               LLVM_DEBUG(llvm::dbgs() << "    Skipping Array: store inside loop!\n"
                                       << "    " << *StInst);
               return false;
            }
            PILValue InitArray = StInst->getSrc();
            if (isa<PILArgument>(InitArray) || isa<ApplyInst>(InitArray))
               continue;

            return false;
         }

         LLVM_DEBUG(llvm::dbgs() << "    Skipping Array: unknown Array use!\n"
                                 << "    " << *UseInst);
         // Found an unsafe or unknown user. The Array may escape here.
         return false;
      }

      // Otherwise, all of our users are sane. The array does not escape.
      return true;
   }

   /// Value uses are generally safe. We can't change the state of an array
   /// through a value use.
   bool checkSafeArrayValueUses(UserList &ValueUsers) {
      return true;
   }
   bool checkSafeElementValueUses(UserOperList &ElementValueUsers) {
      return true;
   }

   // We have a safe container if the array container is passed as a function
   // argument by-value or by inout reference. In either case there can't be an
   // alias of the container. Alternatively, we can have a local variable. We
   // will check in checkSafeArrayAddressUses that all initialization stores to
   // this variable are safe (i.e the store dominates the loop etc).
   bool isSafeArrayContainer(PILValue V) {
      if (auto *Arg = dyn_cast<PILArgument>(V)) {
         // Check that the argument is passed as an inout or by value type. This
         // means there are no aliases accessible within this function scope.
         auto Params = Fun->getLoweredFunctionType()->getParameters();
         ArrayRef<PILArgument *> FunctionArgs = Fun->begin()->getArguments();
         for (unsigned ArgIdx = 0, ArgEnd = Params.size(); ArgIdx != ArgEnd;
              ++ArgIdx) {
            if (FunctionArgs[ArgIdx] != Arg)
               continue;

            if (!Params[ArgIdx].isIndirectInOut()
                && Params[ArgIdx].isFormalIndirect()) {
               LLVM_DEBUG(llvm::dbgs() << "    Skipping Array: Not an inout or "
                                          "by val argument!\n");
               return false;
            }
         }
         return true;
      } else if (isa<AllocStackInst>(V))
         return true;

      LLVM_DEBUG(llvm::dbgs()
                    << "    Skipping Array: Not a know array container type!\n");

      return false;
   }

   SmallPtrSetImpl<PILBasicBlock *> &getLoopExitingBlocks() {
      if (!CachedExitingBlocks.empty())
         return CachedExitingBlocks;
      SmallVector<PILBasicBlock *, 16> ExitingBlocks;
      Loop->getExitingBlocks(ExitingBlocks);
      CachedExitingBlocks.insert(ExitingBlocks.begin(), ExitingBlocks.end());
      return CachedExitingBlocks;
   }

   bool isConditionallyExecuted(ArraySemanticsCall Call) {
      auto CallBB = (*Call).getParent();
      for (auto *ExitingBlk : getLoopExitingBlocks())
         if (!DomTree->dominates(CallBB, ExitingBlk))
            return true;
      return false;
   }

   bool isClassElementTypeArray(PILValue Arr) {
      auto Ty = Arr->getType();
      if (auto BGT = Ty.getAs<BoundGenericStructType>()) {
         // Check the array element type parameter.
         bool isClass = false;
         for (auto EltTy : BGT->getGenericArgs()) {
            if (!EltTy->hasReferenceSemantics())
               return false;
            isClass = true;
         }
         return isClass;
      }
      return false;
   }

   bool canHoistArrayPropsInst(ArraySemanticsCall Call) {
      // TODO: This is way conservative. If there is an unconditionally
      // executed call to the same array we can still hoist it.
      if (isConditionallyExecuted(Call))
         return false;

      PILValue Arr = Call.getSelf();

      // We don't attempt to hoist non-class element type arrays.
      if (!isClassElementTypeArray(Arr))
         return false;

      // We can strip the load that might even occur in the loop because we make
      // sure that no unsafe store to the array's address takes place.
      Arr = stripArrayStructLoad(Arr);

      // Have we already seen this array and deemed it safe?
      if (HoistableArray.count(Arr))
         return true;

      // Do we know how to hoist the arguments of this call.
      if (!Call.canHoist(Preheader->getTerminator(), DomTree))
         return false;

      SmallVector<unsigned, 4> AccessPath;
      PILValue ArrayContainer =
         StructUseCollector::getAccessPath(Arr, AccessPath);

      if (!isSafeArrayContainer(ArrayContainer))
         return false;

      StructUseCollector StructUses;
      StructUses.collectUses(ArrayContainer, AccessPath);

      if (!checkSafeArrayAddressUses(StructUses.AggregateAddressUsers) ||
          !checkSafeArrayAddressUses(StructUses.StructAddressUsers) ||
          !checkSafeArrayValueUses(StructUses.StructValueUsers) ||
          !checkSafeElementValueUses(StructUses.ElementValueUsers) ||
          !StructUses.ElementAddressUsers.empty())
         return false;

      HoistableArray.insert(Arr);
      return true;
   }
};
} // end anonymous namespace

namespace {
/// Clone a single exit multiple exit region starting at basic block and ending
/// in a set of basic blocks. Updates the dominator tree with the cloned blocks.
/// However, the client needs to update the dominator of the exit blocks.
///
/// FIXME: PILCloner is used to cloned CFG regions by multiple clients. All
/// functionality for generating valid PIL (including the DomTree) should be
/// handled by the common PILCloner.
class RegionCloner : public PILCloner<RegionCloner> {
   DominanceInfo &DomTree;
   PILBasicBlock *StartBB;

   friend class PILInstructionVisitor<RegionCloner>;
   friend class PILCloner<RegionCloner>;

public:
   RegionCloner(PILBasicBlock *EntryBB, DominanceInfo &DT)
      : PILCloner<RegionCloner>(*EntryBB->getParent()), DomTree(DT),
        StartBB(EntryBB) {}

   PILBasicBlock *cloneRegion(ArrayRef<PILBasicBlock *> exitBBs) {
      assert (DomTree.getNode(StartBB) != nullptr && "Can't cloned dead code");

      // We need to split any edge from a non cond_br basic block leading to a
      // exit block. After cloning this edge will become critical if it came from
      // inside the cloned region. The SSAUpdater can't handle critical non
      // cond_br edges.
      //
      // FIXME: remove this in the next commit. The PILCloner will always do it.
      for (auto *BB : exitBBs) {
         SmallVector<PILBasicBlock *, 8> Preds(BB->getPredecessorBlocks());
         for (auto *Pred : Preds)
            if (!isa<CondBranchInst>(Pred->getTerminator()) &&
                !isa<BranchInst>(Pred->getTerminator()))
               splitEdgesFromTo(Pred, BB, &DomTree, nullptr);
      }

      cloneReachableBlocks(StartBB, exitBBs);

      // Add dominator tree nodes for the new basic blocks.
      fixDomTree();

      // Update SSA form for values used outside of the copied region.
      updateSSAForm();
      return getOpBasicBlock(StartBB);
   }

protected:
   /// Clone the dominator tree from the original region to the cloned region.
   void fixDomTree() {
      for (auto *BB : originalPreorderBlocks()) {
         auto *ClonedBB = getOpBasicBlock(BB);
         auto *OrigDomBB = DomTree.getNode(BB)->getIDom()->getBlock();
         if (BB == StartBB) {
            // The cloned start node shares the same dominator as the original node.
            auto *ClonedNode = DomTree.addNewBlock(ClonedBB, OrigDomBB);
            (void)ClonedNode;
            assert(ClonedNode);
            continue;
         }
         // Otherwise, map the dominator structure using the mapped block.
         DomTree.addNewBlock(ClonedBB, getOpBasicBlock(OrigDomBB));
      }
   }

   PILValue getMappedValue(PILValue V) {
      if (auto *BB = V->getParentBlock()) {
         if (!DomTree.dominates(StartBB, BB)) {
            // Must be a value that dominates the start basic block.
            assert(DomTree.dominates(BB, StartBB) &&
                   "Must dominated the start of the cloned region");
            return V;
         }
      }
      return PILCloner<RegionCloner>::getMappedValue(V);
   }

   void postProcess(PILInstruction *Orig, PILInstruction *Cloned) {
      PILCloner<RegionCloner>::postProcess(Orig, Cloned);
   }

   /// Update SSA form for values that are used outside the region.
   void updateSSAForValue(PILBasicBlock *OrigBB, PILValue V,
                          PILSSAUpdater &SSAUp) {
      // Collect outside uses.
      SmallVector<UseWrapper, 16> UseList;
      for (auto Use : V->getUses())
         if (!isBlockCloned(Use->getUser()->getParent())) {
            UseList.push_back(UseWrapper(Use));
         }
      if (UseList.empty())
         return;

      // Update SSA form.
      SSAUp.Initialize(V->getType());
      SSAUp.AddAvailableValue(OrigBB, V);
      PILValue NewVal = getMappedValue(V);
      SSAUp.AddAvailableValue(getOpBasicBlock(OrigBB), NewVal);
      for (auto U : UseList) {
         Operand *Use = U;
         SSAUp.RewriteUse(*Use);
      }
   }

   void updateSSAForm() {
      PILSSAUpdater SSAUp;
      for (auto *origBB : originalPreorderBlocks()) {
         // Update outside used phi values.
         for (auto *arg : origBB->getArguments())
            updateSSAForValue(origBB, arg, SSAUp);

         // Update outside used instruction values.
         for (auto &inst : *origBB) {
            for (auto result : inst.getResults())
               updateSSAForValue(origBB, result, SSAUp);
         }
      }
   }
};
} // end anonymous namespace

namespace {
/// This class transforms a hoistable loop nest into a speculatively specialized
/// loop based on array.props calls.
class ArrayPropertiesSpecializer {
   DominanceInfo *DomTree;
   PILLoopAnalysis *LoopAnalysis;
   PILBasicBlock *HoistableLoopPreheader;

public:
   ArrayPropertiesSpecializer(DominanceInfo *DT, PILLoopAnalysis *LA,
                              PILBasicBlock *Hoistable)
      : DomTree(DT), LoopAnalysis(LA), HoistableLoopPreheader(Hoistable) {}

   void run() {
      specializeLoopNest();
   }

   PILLoop *getLoop() {
      auto *LoopInfo = LoopAnalysis->get(HoistableLoopPreheader->getParent());
      return LoopInfo->getLoopFor(
         HoistableLoopPreheader->getSingleSuccessorBlock());
   }

protected:
   void specializeLoopNest();
};
} // end anonymous namespace

static PILValue createStructExtract(PILBuilder &B, PILLocation Loc,
                                    PILValue Opd, unsigned FieldNo) {
   PILType Ty = Opd->getType();
   auto SD = Ty.getStructOrBoundGenericStruct();
   auto Properties = SD->getStoredProperties();
   unsigned Counter = 0;
   for (auto *D : Properties)
      if (Counter++ == FieldNo)
         return B.createStructExtract(Loc, Opd, D);
   llvm_unreachable("Wrong field number");
}

static Identifier getBinaryFunction(StringRef Name, PILType IntPILTy,
                                    AstContext &C) {
   auto IntTy = IntPILTy.castTo<BuiltinIntegerType>();
   unsigned NumBits = IntTy->getWidth().getFixedWidth();
   // Name is something like: add_Int64
   std::string NameStr = Name;
   NameStr += "_Int" + llvm::utostr(NumBits);
   return C.getIdentifier(NameStr);
}

/// Create a binary and function.
static PILValue createAnd(PILBuilder &B, PILLocation Loc, PILValue Opd1,
                          PILValue Opd2) {
   auto AndFn = getBinaryFunction("and", Opd1->getType(), B.getAstContext());
   PILValue Args[] = {Opd1, Opd2};
   return B.createBuiltin(Loc, AndFn, Opd1->getType(), {}, Args);
}

/// Create a check over all array.props calls that they have the 'fast native
/// swift' array value: isNative && !needsElementTypeCheck must be true.
static PILValue
createFastNativeArraysCheck(SmallVectorImpl<ArraySemanticsCall> &ArrayProps,
                            PILBuilder &B) {
   assert(!ArrayProps.empty() && "Must have array.pros calls");

   PILType IntBoolTy = PILType::getBuiltinIntegerType(1, B.getAstContext());
   PILValue Result =
      B.createIntegerLiteral((*ArrayProps[0]).getLoc(), IntBoolTy, 1);

   for (auto Call : ArrayProps) {
      auto Loc = (*Call).getLoc();
      auto CallKind = Call.getKind();
      if (CallKind == ArrayCallKind::kArrayPropsIsNativeTypeChecked) {
         auto Val = createStructExtract(B, Loc, PILValue(Call), 0);
         Result = createAnd(B, Loc, Result, Val);
      }
   }
   return Result;
}

/// Collect all array.props calls in the cloned basic blocks stored in the map,
/// asserting that we found at least one.
static void collectArrayPropsCalls(RegionCloner &Cloner,
                                   SmallVectorImpl<PILBasicBlock *> &ExitBlocks,
                                   SmallVectorImpl<ArraySemanticsCall> &Calls) {
   for (auto *origBB : Cloner.originalPreorderBlocks()) {
      auto clonedBB = Cloner.getOpBasicBlock(origBB);
      for (auto &Inst : *clonedBB) {
         ArraySemanticsCall ArrayProps(&Inst, "array.props", true);
         if (!ArrayProps)
            continue;
         Calls.push_back(ArrayProps);
      }
   }
   assert(!Calls.empty() && "Should have a least one array.props call");
}

/// Replace an array.props call by the 'fast swift array' value.
///
/// This is true for array.props.isNative and false for
/// array.props.needsElementTypeCheck.
static void replaceArrayPropsCall(PILBuilder &B, ArraySemanticsCall C) {
   assert(C.getKind() == ArrayCallKind::kArrayPropsIsNativeTypeChecked);
   ApplyInst *AI = C;

   PILType IntBoolTy = PILType::getBuiltinIntegerType(1, B.getAstContext());

   auto BoolTy = AI->getType();
   auto C0 = B.createIntegerLiteral(AI->getLoc(), IntBoolTy, 1);
   auto BoolVal = B.createStruct(AI->getLoc(), BoolTy, {C0});

   (*C).replaceAllUsesWith(BoolVal);
   // Remove call to array.props.read/write.
   C.removeCall();
}

/// Collects all loop dominated blocks outside the loop that are immediately
/// dominated by the loop.
static void
collectImmediateLoopDominatedBlocks(const PILLoop *Lp, DominanceInfoNode *Node,
                                    SmallVectorImpl<PILBasicBlock *> &Blocks) {
   PILBasicBlock *BB = Node->getBlock();

   // Base case: First loop dominated block outside of loop.
   if (!Lp->contains(BB)) {
      Blocks.push_back(BB);
      return;
   }

   // Loop contains the basic block. Look at immediately dominated nodes.
   for (auto *Child : *Node)
      collectImmediateLoopDominatedBlocks(Lp, Child, Blocks);
}

void ArrayPropertiesSpecializer::specializeLoopNest() {
   auto *Lp = getLoop();
   assert(Lp);

   // Split of a new empty preheader. We don't want to duplicate the whole
   // original preheader it might contain instructions that we can't clone.
   // This will be block that will contain the check whether to execute the
   // 'native swift array' loop or the original loop.
   PILBuilder B(HoistableLoopPreheader);
   auto *CheckBlock = splitBasicBlockAndBranch(B,
                                               HoistableLoopPreheader->getTerminator(), DomTree, nullptr);

   auto *Header = CheckBlock->getSingleSuccessorBlock();
   assert(Header);

   // Collect all loop dominated blocks (e.g exit blocks could be among them). We
   // need to update their dominator.
   SmallVector<PILBasicBlock *, 16> LoopDominatedBlocks;
   collectImmediateLoopDominatedBlocks(Lp, DomTree->getNode(Header),
                                       LoopDominatedBlocks);

   // Collect all exit blocks.
   SmallVector<PILBasicBlock *, 16> ExitBlocks;
   Lp->getExitBlocks(ExitBlocks);

   // Split the preheader before the first instruction.
   PILBasicBlock *NewPreheader =
      splitBasicBlockAndBranch(B, &*CheckBlock->begin(), DomTree, nullptr);

   // Clone the region from the new preheader up to (not including) the exit
   // blocks. This creates a second loop nest.
   RegionCloner Cloner(NewPreheader, *DomTree);
   auto *ClonedPreheader = Cloner.cloneRegion(ExitBlocks);

   // Collect the array.props call that we will specialize on that we have
   // cloned in the cloned loop.
   SmallVector<ArraySemanticsCall, 16> ArrayPropCalls;
   collectArrayPropsCalls(Cloner, ExitBlocks, ArrayPropCalls);

   // Move them to the check block.
   SmallVector<ArraySemanticsCall, 16> HoistedArrayPropCalls;
   for (auto C: ArrayPropCalls)
      HoistedArrayPropCalls.push_back(
         ArraySemanticsCall(C.copyTo(CheckBlock->getTerminator(), DomTree)));

   // Create a conditional branch on the fast condition being true.
   B.setInsertionPoint(CheckBlock->getTerminator());
   auto IsFastNativeArray =
      createFastNativeArraysCheck(HoistedArrayPropCalls, B);
   B.createCondBranch(CheckBlock->getTerminator()->getLoc(),
                      IsFastNativeArray, ClonedPreheader, NewPreheader);
   CheckBlock->getTerminator()->eraseFromParent();

   // Fixup the loop dominated blocks. They are now dominated by the check block.
   for (auto *BB : LoopDominatedBlocks)
      DomTree->changeImmediateDominator(DomTree->getNode(BB),
                                        DomTree->getNode(CheckBlock));

   // Replace the array.props calls uses in the cloned loop by their 'fast'
   // value.
   PILBuilder B2(ClonedPreheader->getTerminator());
   for (auto C : ArrayPropCalls)
      replaceArrayPropsCall(B2, C);

   // We have potentially cloned a loop - invalidate loop info.
   LoopAnalysis->invalidate(Header->getParent(),
                            PILAnalysis::InvalidationKind::FunctionBody);
}

namespace {
class TypePHPArrayPropertyOptPass : public PILFunctionTransform {

   void run() override {
      auto *Fn = getFunction();

      // FIXME: Add support for ownership.
      if (Fn->hasOwnership())
         return;

      // Don't hoist array property calls at Osize.
      if (Fn->optimizeForSize())
         return;

      DominanceAnalysis *DA = PM->getAnalysis<DominanceAnalysis>();
      PILLoopAnalysis *LA = PM->getAnalysis<PILLoopAnalysis>();
      PILLoopInfo *LI = LA->get(Fn);

      bool HasChanged = false;

      // Check whether we can hoist 'array.props' calls out of loops, collecting
      // the preheader we can hoist to. We only hoist out of loops if 'all'
      // array.props call can be hoisted for a given loop nest.
      // We process the loop tree preorder (top-down) to hoist over the biggest
      // possible loop-nest.
      SmallVector<PILBasicBlock *, 16> HoistableLoopNests;
      std::function<void(PILLoop *)> processChildren = [&](PILLoop *L) {
         ArrayPropertiesAnalysis Analysis(L, DA);
         if (Analysis.run()) {
            // Hoist in the current loop nest.
            HasChanged = true;
            HoistableLoopNests.push_back(L->getLoopPreheader());
         } else {
            // Otherwise, try hoisting sub-loops.
            for (auto *SubLoop : *L)
               processChildren(SubLoop);
         }
      };
      for (auto *L : *LI)
         processChildren(L);

      // Specialize the identified loop nest based on the 'array.props' calls.
      if (HasChanged) {
         LLVM_DEBUG(getFunction()->viewCFG());
         DominanceInfo *DT = DA->get(getFunction());

         // Process specialized loop-nests in loop-tree post-order (bottom-up).
         std::reverse(HoistableLoopNests.begin(), HoistableLoopNests.end());

         // Hoist the loop nests.
         for (auto &HoistableLoopNest : HoistableLoopNests)
            ArrayPropertiesSpecializer(DT, LA, HoistableLoopNest).run();

         // Verify that no illegal critical edges were created.
         getFunction()->verifyCriticalEdges();

         LLVM_DEBUG(getFunction()->viewCFG());

         // We preserve the dominator tree. Let's invalidate everything
         // else.
         DA->lockInvalidation();
         invalidateAnalysis(PILAnalysis::InvalidationKind::FunctionBody);
         DA->unlockInvalidation();
      }
   }

};
} // end anonymous namespace

PILTransform *polar::createTypePHPArrayPropertyOpt() {
   return new TypePHPArrayPropertyOptPass();
}
