//===--- LICM.cpp - Loop invariant code motion ----------------------------===//
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

#define DEBUG_TYPE "pil-licm"

#include "polarphp/pil/lang/Dominance.h"
#include "polarphp/pil/lang/InstructionUtils.h"
#include "polarphp/pil/lang/MemAccessUtils.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/optimizer/analysis/AccessedStorageAnalysis.h"
#include "polarphp/pil/optimizer/analysis/AliasAnalysis.h"
#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "polarphp/pil/optimizer/analysis/ArraySemantic.h"
#include "polarphp/pil/optimizer/analysis/DominanceAnalysis.h"
#include "polarphp/pil/optimizer/analysis/LoopAnalysis.h"
#include "polarphp/pil/optimizer/analysis/SideEffectAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/CFGOptUtils.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "polarphp/pil/optimizer/utils/PILSSAUpdater.h"

#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/Debug.h"

#include "llvm/Support/CommandLine.h"

using namespace polar;

namespace {

/// Instructions which can be hoisted:
/// loads, function calls without side effects and (some) exclusivity checks
using InstSet = llvm::SmallPtrSet<PILInstruction *, 8>;

using InstVector = llvm::SmallVector<PILInstruction *, 8>;

/// Returns true if the \p SideEffectInsts set contains any memory writes which
/// may alias with the memory addressed by \a LI.
template <PILInstructionKind K, typename T>
static bool mayWriteTo(AliasAnalysis *AA, InstSet &SideEffectInsts,
                       UnaryInstructionBase<K, T> *Inst) {
   for (auto *I : SideEffectInsts)
      if (AA->mayWriteToMemory(I, Inst->getOperand())) {
         LLVM_DEBUG(llvm::dbgs() << "  mayWriteTo\n" << *I << " to "
                                 << *Inst << "\n");
         return true;
      }
   return false;
}

/// Returns true if \p I is a store to \p addr.
static StoreInst *isStoreToAddr(PILInstruction *I, PILValue addr) {
   auto *SI = dyn_cast<StoreInst>(I);
   if (!SI)
      return nullptr;

   // TODO: handle StoreOwnershipQualifier::Init
   if (SI->getOwnershipQualifier() == StoreOwnershipQualifier::Init)
      return nullptr;

   if (SI->getDest() != addr)
      return nullptr;

   return SI;
}

/// Returns true if \p I is a load from \p addr or a projected address from
/// \p addr.
static LoadInst *isLoadFromAddr(PILInstruction *I, PILValue addr) {
   auto *LI = dyn_cast_or_null<LoadInst>(I);
   if (!LI)
      return nullptr;

   // TODO: handle StoreOwnershipQualifier::Take
   if (LI->getOwnershipQualifier() == LoadOwnershipQualifier::Take)
      return nullptr;

   PILValue v = LI->getOperand();
   for (;;) {
      if (v == addr) {
         return LI;
      } else if (isa<StructElementAddrInst>(v) || isa<TupleElementAddrInst>(v)) {
         v = cast<SingleValueInstruction>(v)->getOperand(0);
      } else {
         return nullptr;
      }
   }
}

/// Returns true if all instructions in \p SideEffectInsts which may alias with
/// \p addr are either loads or stores from \p addr.
static bool isOnlyLoadedAndStored(AliasAnalysis *AA, InstSet &SideEffectInsts,
                                  PILValue addr) {
   for (auto *I : SideEffectInsts) {
      if (AA->mayReadOrWriteMemory(I, addr) &&
          !isStoreToAddr(I, addr) && !isLoadFromAddr(I, addr)) {
         return false;
      }
   }
   return true;
}

/// Returns true if the \p SideEffectInsts set contains any memory writes which
/// may alias with any memory which is read by \p AI.
/// Note: This function should only be called on a read-only apply!
static bool mayWriteTo(AliasAnalysis *AA, SideEffectAnalysis *SEA,
                       InstSet &SideEffectInsts, ApplyInst *AI) {
   FunctionSideEffects E;
   SEA->getCalleeEffects(E, AI);
   assert(E.getMemBehavior(RetainObserveKind::IgnoreRetains) <=
          PILInstruction::MemoryBehavior::MayRead &&
          "apply should only read from memory");
   assert(!E.getGlobalEffects().mayRead() &&
          "apply should not have global effects");

   for (unsigned Idx = 0, End = AI->getNumArguments(); Idx < End; ++Idx) {
      auto &ArgEffect = E.getParameterEffects()[Idx];
      assert(!ArgEffect.mayRelease() && "apply should only read from memory");
      if (!ArgEffect.mayRead())
         continue;

      PILValue Arg = AI->getArgument(Idx);

      // Check if the memory addressed by the argument may alias any writes.
      for (auto *I : SideEffectInsts) {
         if (AA->mayWriteToMemory(I, Arg)) {
            LLVM_DEBUG(llvm::dbgs() << "  mayWriteTo\n" << *I << " to "
                                    << *AI << "\n");
            return true;
         }
      }
   }
   return false;
}

// When Hoisting / Sinking,
// Don't descend into control-dependent code.
// Only traverse into basic blocks that dominate all exits.
static void getDominatingBlocks(SmallVectorImpl<PILBasicBlock *> &domBlocks,
                                PILLoop *Loop, DominanceInfo *DT) {
   auto HeaderBB = Loop->getHeader();
   auto DTRoot = DT->getNode(HeaderBB);
   SmallVector<PILBasicBlock *, 8> ExitingBBs;
   Loop->getExitingBlocks(ExitingBBs);
   for (llvm::df_iterator<DominanceInfoNode *> It = llvm::df_begin(DTRoot),
           E = llvm::df_end(DTRoot);
        It != E;) {
      auto *CurBB = It->getBlock();

      // Don't decent into control-dependent code. Only traverse into basic blocks
      // that dominate all exits.
      if (!std::all_of(ExitingBBs.begin(), ExitingBBs.end(),
                       [=](PILBasicBlock *ExitBB) {
                          return DT->dominates(CurBB, ExitBB);
                       })) {
         LLVM_DEBUG(llvm::dbgs() << "  skipping conditional block "
                                 << *CurBB << "\n");
         It.skipChildren();
         continue;
      }
      domBlocks.push_back(CurBB);
      // Next block in dominator tree.
      ++It;
   }
}

/// Returns true if \p v is loop invariant in \p L.
static bool isLoopInvariant(PILValue v, PILLoop *L) {
   if (PILBasicBlock *parent = v->getParentBlock())
      return !L->contains(parent);
   return false;
}

static bool hoistInstruction(DominanceInfo *DT, PILInstruction *Inst,
                             PILLoop *Loop, PILBasicBlock *&Preheader) {
   auto Operands = Inst->getAllOperands();
   if (!std::all_of(Operands.begin(), Operands.end(), [=](Operand &Op) {
      return isLoopInvariant(Op.get(), Loop);
   })) {
      LLVM_DEBUG(llvm::dbgs() << "   loop variant operands\n");
      return false;
   }

   auto mvBefore = Preheader->getTerminator();
   ArraySemanticsCall semCall(Inst);
   if (semCall.canHoist(mvBefore, DT)) {
      semCall.hoist(mvBefore, DT);
   } else {
      Inst->moveBefore(mvBefore);
   }
   return true;
}

static bool hoistInstructions(PILLoop *Loop, DominanceInfo *DT,
                              InstSet &HoistUpSet) {
   LLVM_DEBUG(llvm::dbgs() << " Hoisting instructions.\n");
   auto Preheader = Loop->getLoopPreheader();
   assert(Preheader && "Expected a preheader");
   bool Changed = false;
   SmallVector<PILBasicBlock *, 8> domBlocks;
   getDominatingBlocks(domBlocks, Loop, DT);

   for (auto *CurBB : domBlocks) {
      // We know that the block is guaranteed to be executed. Hoist if we can.
      for (auto InstIt = CurBB->begin(), E = CurBB->end(); InstIt != E;) {
         PILInstruction *Inst = &*InstIt;
         ++InstIt;
         LLVM_DEBUG(llvm::dbgs() << "  looking at " << *Inst);
         if (!HoistUpSet.count(Inst)) {
            continue;
         }
         if (!hoistInstruction(DT, Inst, Loop, Preheader)) {
            continue;
         }
         LLVM_DEBUG(llvm::dbgs() << "Hoisted " << *Inst);
         Changed = true;
      }
   }

   return Changed;
}

/// Summary of side effect instructions occurring in the loop tree rooted at \p
/// Loop. This includes all writes of the sub loops and the loop itself.
struct LoopNestSummary {
   PILLoop *Loop;
   InstSet SideEffectInsts;

   LoopNestSummary(PILLoop *Curr) : Loop(Curr) {}


   void copySummary(LoopNestSummary &Other) {
      SideEffectInsts.insert(Other.SideEffectInsts.begin(), Other.SideEffectInsts.end());
   }

   LoopNestSummary(const LoopNestSummary &) = delete;
   LoopNestSummary &operator=(const LoopNestSummary &) = delete;
   LoopNestSummary(LoopNestSummary &&) = delete;
};

static unsigned getEdgeIndex(PILBasicBlock *BB, PILBasicBlock *ExitingBB) {
   auto Succs = ExitingBB->getSuccessors();
   for (unsigned EdgeIdx = 0; EdgeIdx < Succs.size(); ++EdgeIdx) {
      PILBasicBlock *CurrBB = Succs[EdgeIdx];
      if (CurrBB == BB) {
         return EdgeIdx;
      }
   }
   llvm_unreachable("BB is not a Successor");
}

static bool sinkInstruction(DominanceInfo *DT,
                            std::unique_ptr<LoopNestSummary> &LoopSummary,
                            PILInstruction *Inst, PILLoopInfo *LI) {
   auto *Loop = LoopSummary->Loop;
   SmallVector<PILBasicBlock *, 8> ExitBBs;
   Loop->getExitBlocks(ExitBBs);
   SmallVector<PILBasicBlock *, 8> NewExitBBs;
   SmallVector<PILBasicBlock *, 8> ExitingBBs;
   Loop->getExitingBlocks(ExitingBBs);
   auto *ExitBB = Loop->getExitBlock();

   bool Changed = false;
   for (auto *ExitingBB : ExitingBBs) {
      SmallVector<PILBasicBlock *, 8> BBSuccessors;
      auto Succs = ExitingBB->getSuccessors();
      for (unsigned EdgeIdx = 0; EdgeIdx < Succs.size(); ++EdgeIdx) {
         PILBasicBlock *BB = Succs[EdgeIdx];
         BBSuccessors.push_back(BB);
      }
      while (!BBSuccessors.empty()) {
         PILBasicBlock *BB = BBSuccessors.pop_back_val();
         if (std::find(NewExitBBs.begin(), NewExitBBs.end(), BB) !=
             NewExitBBs.end()) {
            // Already got a copy there
            continue;
         }
         auto EdgeIdx = getEdgeIndex(BB, ExitingBB);
         PILBasicBlock *OutsideBB = nullptr;
         if (std::find(ExitBBs.begin(), ExitBBs.end(), BB) != ExitBBs.end()) {
            auto *SplitBB =
               splitCriticalEdge(ExitingBB->getTerminator(), EdgeIdx, DT, LI);
            OutsideBB = SplitBB ? SplitBB : BB;
            NewExitBBs.push_back(OutsideBB);
         }
         if (!OutsideBB) {
            continue;
         }
         // If OutsideBB already contains Inst -> skip
         // This might happen if we have a conditional control flow
         // And a pair
         // We hoisted the first part, we can safely ignore sinking
         auto matchPred = [&](PILInstruction &CurrIns) {
            return Inst->isIdenticalTo(&CurrIns);
         };
         if (std::find_if(OutsideBB->begin(), OutsideBB->end(), matchPred) !=
             OutsideBB->end()) {
            LLVM_DEBUG(llvm::errs() << "  instruction already at exit BB "
                                    << *Inst);
            ExitBB = nullptr;
         } else if (ExitBB) {
            // easy case
            LLVM_DEBUG(llvm::errs() << "  moving instruction to exit BB " << *Inst);
            Inst->moveBefore(&*OutsideBB->begin());
         } else {
            LLVM_DEBUG(llvm::errs() << "  cloning instruction to exit BB "
                                    << *Inst);
            Inst->clone(&*OutsideBB->begin());
         }
         Changed = true;
      }
   }
   if (Changed && !ExitBB) {
      // Created clones of instruction
      // Remove it from the side-effect set - dangling pointer
      LoopSummary->SideEffectInsts.erase(Inst);
      Inst->getParent()->erase(Inst);
   }
   return Changed;
}

static bool sinkInstructions(std::unique_ptr<LoopNestSummary> &LoopSummary,
                             DominanceInfo *DT, PILLoopInfo *LI,
                             InstVector &SinkDownSet) {
   auto *Loop = LoopSummary->Loop;
   LLVM_DEBUG(llvm::errs() << " Sink instructions attempt\n");
   SmallVector<PILBasicBlock *, 8> domBlocks;
   getDominatingBlocks(domBlocks, Loop, DT);

   bool Changed = false;
   for (auto *Inst : SinkDownSet) {
      // only sink if the block is guaranteed to be executed.
      if (std::find(domBlocks.begin(), domBlocks.end(), Inst->getParent()) ==
          domBlocks.end()) {
         continue;
      }
      Changed |= sinkInstruction(DT, LoopSummary, Inst, LI);
   }

   return Changed;
}

static void getEndAccesses(BeginAccessInst *BI,
                           SmallVectorImpl<EndAccessInst *> &EndAccesses) {
   for (auto Use : BI->getUses()) {
      auto *User = Use->getUser();
      auto *EI = dyn_cast<EndAccessInst>(User);
      if (!EI) {
         continue;
      }
      EndAccesses.push_back(EI);
   }
}

static bool
hoistSpecialInstruction(std::unique_ptr<LoopNestSummary> &LoopSummary,
                        DominanceInfo *DT, PILLoopInfo *LI, InstVector &Special) {
   auto *Loop = LoopSummary->Loop;
   LLVM_DEBUG(llvm::errs() << " Hoist and Sink pairs attempt\n");
   auto Preheader = Loop->getLoopPreheader();
   assert(Preheader && "Expected a preheader");

   bool Changed = false;

   for (auto *Inst : Special) {
      if (!hoistInstruction(DT, Inst, Loop, Preheader)) {
         continue;
      }
      if (auto *BI = dyn_cast<BeginAccessInst>(Inst)) {
         SmallVector<EndAccessInst *, 2> Ends;
         getEndAccesses(BI, Ends);
         LLVM_DEBUG(llvm::dbgs() << "Hoisted BeginAccess " << *BI);
         for (auto *instSink : Ends) {
            if (!sinkInstruction(DT, LoopSummary, instSink, LI)) {
               llvm_unreachable("LICM: Could not perform must-sink instruction");
            }
         }
         LLVM_DEBUG(llvm::errs() << " Successfully hoisted and sank pair\n");
      } else {
         LLVM_DEBUG(llvm::dbgs() << "Hoisted RefElementAddr "
                                 << *static_cast<RefElementAddrInst *>(Inst));
      }
      Changed = true;
   }

   return Changed;
}

/// Optimize the loop tree bottom up propagating loop's summaries up the
/// loop tree.
class LoopTreeOptimization {
   llvm::DenseMap<PILLoop *, std::unique_ptr<LoopNestSummary>>
      LoopNestSummaryMap;
   SmallVector<PILLoop *, 8> BotUpWorkList;
   PILLoopInfo *LoopInfo;
   AliasAnalysis *AA;
   SideEffectAnalysis *SEA;
   DominanceInfo *DomTree;
   AccessedStorageAnalysis *ASA;
   bool Changed;

   /// True if LICM is done on high-level PIL, i.e. semantic calls are not
   /// inlined yet. In this case some semantic calls can be hoisted.
   bool RunsOnHighLevelPIL;

   /// Instructions that we may be able to hoist up
   InstSet HoistUp;

   /// Instructions that we may be able to sink down
   InstVector SinkDown;

   /// Load and store instructions that we may be able to move out of the loop.
   InstVector LoadsAndStores;

   /// All addresses of the \p LoadsAndStores instructions.
   llvm::SetVector<PILValue> LoadAndStoreAddrs;

   /// Hoistable Instructions that need special treatment
   /// e.g. begin_access
   InstVector SpecialHoist;

public:
   LoopTreeOptimization(PILLoop *TopLevelLoop, PILLoopInfo *LI,
                        AliasAnalysis *AA, SideEffectAnalysis *SEA,
                        DominanceInfo *DT, AccessedStorageAnalysis *ASA,
                        bool RunsOnHighLevelSil)
      : LoopInfo(LI), AA(AA), SEA(SEA), DomTree(DT), ASA(ASA), Changed(false),
        RunsOnHighLevelPIL(RunsOnHighLevelSil) {
      // Collect loops for a recursive bottom-up traversal in the loop tree.
      BotUpWorkList.push_back(TopLevelLoop);
      for (unsigned i = 0; i < BotUpWorkList.size(); ++i) {
         auto *L = BotUpWorkList[i];
         for (auto *SubLoop : *L)
            BotUpWorkList.push_back(SubLoop);
      }
   }

   /// Optimize this loop tree.
   bool optimize();

protected:
   /// Propagate the sub-loops' summaries up to the current loop.
   void propagateSummaries(std::unique_ptr<LoopNestSummary> &CurrSummary);

   /// Collect a set of instructions that can be hoisted
   void analyzeCurrentLoop(std::unique_ptr<LoopNestSummary> &CurrSummary);

   /// Optimize the current loop nest.
   bool optimizeLoop(std::unique_ptr<LoopNestSummary> &CurrSummary);

   /// Move all loads and stores from/to \p addr out of the \p loop.
   void hoistLoadsAndStores(PILValue addr, PILLoop *loop, InstVector &toDelete);

   /// Move all loads and stores from all addresses in LoadAndStoreAddrs out of
   /// the \p loop.
   ///
   /// This is a combination of load hoisting and store sinking, e.g.
   /// \code
   ///   preheader:
   ///     br header_block
   ///   header_block:
   ///     %x = load %not_aliased_addr
   ///     // use %x and define %y
   ///     store %y to %not_aliased_addr
   ///     ...
   ///   exit_block:
   /// \endcode
   /// is transformed to:
   /// \code
   ///   preheader:
   ///     %x = load %not_aliased_addr
   ///     br header_block
   ///   header_block:
   ///     // use %x and define %y
   ///     ...
   ///   exit_block:
   ///     store %y to %not_aliased_addr
   /// \endcode
   bool hoistAllLoadsAndStores(PILLoop *loop);
};
} // end anonymous namespace

bool LoopTreeOptimization::optimize() {
   // Process loops bottom up in the loop tree.
   while (!BotUpWorkList.empty()) {
      PILLoop *CurrentLoop = BotUpWorkList.pop_back_val();
      LLVM_DEBUG(llvm::dbgs() << "Processing loop " << *CurrentLoop);

      // Collect all summary of all sub loops of the current loop. Since we
      // process the loop tree bottom up they are guaranteed to be available in
      // the map.
      auto CurrLoopSummary = std::make_unique<LoopNestSummary>(CurrentLoop);
      propagateSummaries(CurrLoopSummary);

      // If the current loop changed, then we might reveal more instr to hoist
      // For example, a fix_lifetime's operand, if hoisted outside,
      // Might allow us to sink the instruction out of the loop
      bool currChanged = false;
      do {
         // Analyze the current loop for instructions that can be hoisted.
         analyzeCurrentLoop(CurrLoopSummary);

         currChanged = optimizeLoop(CurrLoopSummary);
         if (currChanged) {
            CurrLoopSummary->SideEffectInsts.clear();
            Changed = true;
         }

         // Reset the data structures for next loop in the list
         HoistUp.clear();
         SinkDown.clear();
         SpecialHoist.clear();
      } while (currChanged);

      // Store the summary for parent loops to use.
      LoopNestSummaryMap[CurrentLoop] = std::move(CurrLoopSummary);
   }
   return Changed;
}

void LoopTreeOptimization::propagateSummaries(
   std::unique_ptr<LoopNestSummary> &CurrSummary) {
   for (auto *SubLoop : *CurrSummary->Loop) {
      assert(LoopNestSummaryMap.count(SubLoop) && "Must have data for sub loops");
      CurrSummary->copySummary(*LoopNestSummaryMap[SubLoop]);
      LoopNestSummaryMap.erase(SubLoop);
   }
}

static bool isSafeReadOnlyApply(SideEffectAnalysis *SEA, ApplyInst *AI) {
   FunctionSideEffects E;
   SEA->getCalleeEffects(E, AI);

   if (E.getGlobalEffects().mayRead()) {
      // If we have Global effects,
      // we don't know which memory is read in the callee.
      // Therefore we bail for safety
      return false;
   }

   auto MB = E.getMemBehavior(RetainObserveKind::ObserveRetains);
   return (MB <= PILInstruction::MemoryBehavior::MayRead);
}

static void checkSideEffects(polar::PILInstruction &Inst,
                             InstSet &SideEffectInsts) {
   if (Inst.mayHaveSideEffects()) {
      SideEffectInsts.insert(&Inst);
   }
}

/// Returns true if the \p Inst follows the default hoisting heuristic
static bool canHoistUpDefault(PILInstruction *inst, PILLoop *Loop,
                              DominanceInfo *DT, bool RunsOnHighLevelSil) {
   auto Preheader = Loop->getLoopPreheader();
   if (!Preheader) {
      return false;
   }

   if (isa<TermInst>(inst) || isa<AllocationInst>(inst) ||
       isa<DeallocationInst>(inst)) {
      return false;
   }

   if (inst->getMemoryBehavior() == PILInstruction::MemoryBehavior::None) {
      return true;
   }

   if (!RunsOnHighLevelSil) {
      return false;
   }

   // We can’t hoist everything that is hoist-able
   // The canHoist method does not do all the required analysis
   // Some of the work is done at COW Array Opt
   // TODO: Refactor COW Array Opt + canHoist - radar 41601468
   ArraySemanticsCall semCall(inst);
   switch (semCall.getKind()) {
      case ArrayCallKind::kGetCount:
      case ArrayCallKind::kGetCapacity:
         return semCall.canHoist(Preheader->getTerminator(), DT);
      default:
         return false;
   }
}

// Check If all the end accesses of the given begin do not prevent hoisting
// There are only two legal placements for the end access instructions:
// 1) Inside the same loop (sink to loop exists)
// Potential TODO: At loop exit block
static bool handledEndAccesses(BeginAccessInst *BI, PILLoop *Loop) {
   SmallVector<EndAccessInst *, 2> AllEnds;
   getEndAccesses(BI, AllEnds);
   if (AllEnds.empty()) {
      return false;
   }
   for (auto *User : AllEnds) {
      auto *BB = User->getParent();
      if (Loop->getBlocksSet().count(BB) != 0) {
         continue;
      }
      return false;
   }
   return true;
}

static bool isCoveredByScope(BeginAccessInst *BI, DominanceInfo *DT,
                             PILInstruction *applyInstr) {
   if (!DT->dominates(BI, applyInstr))
      return false;
   for (auto *EI : BI->getEndAccesses()) {
      if (!DT->dominates(applyInstr, EI))
         return false;
   }
   return true;
}

static bool analyzeBeginAccess(BeginAccessInst *BI,
                               SmallVector<BeginAccessInst *, 8> &BeginAccesses,
                               SmallVector<FullApplySite, 8> &fullApplies,
                               InstSet &SideEffectInsts,
                               AccessedStorageAnalysis *ASA,
                               DominanceInfo *DT) {
   const AccessedStorage &storage =
      findAccessedStorageNonNested(BI->getSource());
   if (!storage) {
      return false;
   }

   auto BIAccessedStorageNonNested = findAccessedStorageNonNested(BI);
   auto safeBeginPred = [&](BeginAccessInst *OtherBI) {
      if (BI == OtherBI) {
         return true;
      }
      return BIAccessedStorageNonNested.isDistinctFrom(
         findAccessedStorageNonNested(OtherBI));
   };

   if (!std::all_of(BeginAccesses.begin(), BeginAccesses.end(), safeBeginPred))
      return false;

   for (auto fullApply : fullApplies) {
      FunctionAccessedStorage callSiteAccesses;
      ASA->getCallSiteEffects(callSiteAccesses, fullApply);
      PILAccessKind accessKind = BI->getAccessKind();
      if (!callSiteAccesses.mayConflictWith(accessKind, storage))
         continue;
      // Check if we can ignore this conflict:
      // If the apply is “sandwiched” between the begin and end access,
      // there’s no reason we can’t hoist out of the loop.
      auto *applyInstr = fullApply.getInstruction();
      if (!isCoveredByScope(BI, DT, applyInstr))
         return false;
   }

   // Check may releases
   // Only class and global access that may alias would conflict
   const AccessedStorage::Kind kind = storage.getKind();
   if (kind != AccessedStorage::Class && kind != AccessedStorage::Global) {
      return true;
   }
   // TODO Introduce "Pure Swift" deinitializers
   // We can then make use of alias information for instr's operands
   // If they don't alias - we might get away with not recording a conflict
   for (PILInstruction *I : SideEffectInsts) {
      // we actually compute all SideEffectInsts in analyzeCurrentLoop
      if (!I->mayRelease()) {
         continue;
      }
      if (!isCoveredByScope(BI, DT, I))
         return false;
   }

   return true;
}

// Analyzes current loop for hosting/sinking potential:
// Computes set of instructions we may be able to move out of the loop
// Important Note:
// We can't bail out of this method! we have to run it on all loops.
// We *need* to discover all SideEffectInsts -
// even if the loop is otherwise skipped!
// This is because outer loops will depend on the inner loop's writes.
void LoopTreeOptimization::analyzeCurrentLoop(
   std::unique_ptr<LoopNestSummary> &CurrSummary) {
   InstSet &sideEffects = CurrSummary->SideEffectInsts;
   PILLoop *Loop = CurrSummary->Loop;
   LLVM_DEBUG(llvm::dbgs() << " Analyzing accesses.\n");

   auto *Preheader = Loop->getLoopPreheader();
   if (!Preheader) {
      // Can't hoist/sink instructions
      return;
   }

   // Interesting instructions in the loop:
   SmallVector<ApplyInst *, 8> ReadOnlyApplies;
   SmallVector<LoadInst *, 8> Loads;
   SmallVector<StoreInst *, 8> Stores;
   SmallVector<FixLifetimeInst *, 8> FixLifetimes;
   SmallVector<BeginAccessInst *, 8> BeginAccesses;
   SmallVector<FullApplySite, 8> fullApplies;

   for (auto *BB : Loop->getBlocks()) {
      for (auto &Inst : *BB) {
         switch (Inst.getKind()) {
            case PILInstructionKind::FixLifetimeInst: {
               auto *FL = cast<FixLifetimeInst>(&Inst);
               if (DomTree->dominates(FL->getOperand()->getParentBlock(), Preheader))
                  FixLifetimes.push_back(FL);
               // We can ignore the side effects of FixLifetimes
               break;
            }
            case PILInstructionKind::LoadInst:
               Loads.push_back(cast<LoadInst>(&Inst));
               LoadsAndStores.push_back(&Inst);
               break;
            case PILInstructionKind::StoreInst: {
               Stores.push_back(cast<StoreInst>(&Inst));
               LoadsAndStores.push_back(&Inst);
               checkSideEffects(Inst, sideEffects);
               break;
            }
            case PILInstructionKind::BeginAccessInst:
               BeginAccesses.push_back(cast<BeginAccessInst>(&Inst));
               checkSideEffects(Inst, sideEffects);
               break;
            case PILInstructionKind::RefElementAddrInst:
               SpecialHoist.push_back(cast<RefElementAddrInst>(&Inst));
               break;
            case polar::PILInstructionKind::CondFailInst:
               // We can (and must) hoist cond_fail instructions if the operand is
               // invariant. We must hoist them so that we preserve memory safety. A
               // cond_fail that would have protected (executed before) a memory access
               // must - after hoisting - also be executed before said access.
               HoistUp.insert(&Inst);
               checkSideEffects(Inst, sideEffects);
               break;
            case PILInstructionKind::ApplyInst: {
               auto *AI = cast<ApplyInst>(&Inst);
               if (isSafeReadOnlyApply(SEA, AI)) {
                  ReadOnlyApplies.push_back(AI);
               }
               // check for array semantics and side effects - same as default
               LLVM_FALLTHROUGH;
            }
            default:
               if (auto fullApply = FullApplySite::isa(&Inst)) {
                  fullApplies.push_back(fullApply);
               }
               checkSideEffects(Inst, sideEffects);
               if (canHoistUpDefault(&Inst, Loop, DomTree, RunsOnHighLevelPIL)) {
                  HoistUp.insert(&Inst);
               }
               break;
         }
      }
   }

   for (auto *AI : ReadOnlyApplies) {
      if (!mayWriteTo(AA, SEA, sideEffects, AI)) {
         HoistUp.insert(AI);
      }
   }
   for (auto *LI : Loads) {
      if (!mayWriteTo(AA, sideEffects, LI)) {
         HoistUp.insert(LI);
      }
   }
   // Collect memory locations for which we can move all loads and stores out
   // of the loop.
   for (StoreInst *SI : Stores) {
      PILValue addr = SI->getDest();
      if (isLoopInvariant(addr, Loop) &&
          isOnlyLoadedAndStored(AA, sideEffects, addr)) {
         LoadAndStoreAddrs.insert(addr);
      }
   }
   if (!FixLifetimes.empty()) {
      bool sideEffectsMayRelease =
         std::any_of(sideEffects.begin(), sideEffects.end(),
                     [&](PILInstruction *W) { return W->mayRelease(); });
      for (auto *FL : FixLifetimes) {
         if (!sideEffectsMayRelease || !mayWriteTo(AA, sideEffects, FL)) {
            SinkDown.push_back(FL);
         }
      }
   }
   for (auto *BI : BeginAccesses) {
      if (!handledEndAccesses(BI, Loop)) {
         LLVM_DEBUG(llvm::dbgs() << "Skipping: " << *BI);
         LLVM_DEBUG(llvm::dbgs() << "Some end accesses can't be handled\n");
         continue;
      }
      if (analyzeBeginAccess(BI, BeginAccesses, fullApplies, sideEffects, ASA,
                             DomTree)) {
         SpecialHoist.push_back(BI);
      }
   }
}

bool LoopTreeOptimization::optimizeLoop(
   std::unique_ptr<LoopNestSummary> &CurrSummary) {
   auto *CurrentLoop = CurrSummary->Loop;
   // We only support Loops with a preheader
   if (!CurrentLoop->getLoopPreheader())
      return false;
   bool currChanged = false;
   if (hoistAllLoadsAndStores(CurrentLoop))
      return true;

   currChanged |= hoistInstructions(CurrentLoop, DomTree, HoistUp);
   currChanged |= sinkInstructions(CurrSummary, DomTree, LoopInfo, SinkDown);
   currChanged |=
      hoistSpecialInstruction(CurrSummary, DomTree, LoopInfo, SpecialHoist);
   return currChanged;
}

/// Creates a value projection from \p rootVal based on the address projection
/// from \a rootAddr to \a addr.
static PILValue projectLoadValue(PILValue addr, PILValue rootAddr,
                                 PILValue rootVal, PILInstruction *beforeInst) {
   if (addr == rootAddr)
      return rootVal;

   if (auto *SEI = dyn_cast<StructElementAddrInst>(addr)) {
      PILValue val = projectLoadValue(SEI->getOperand(), rootAddr, rootVal,
                                      beforeInst);
      PILBuilder B(beforeInst);
      return B.createStructExtract(beforeInst->getLoc(), val, SEI->getField(),
                                   SEI->getType().getObjectType());
   }
   if (auto *TEI = dyn_cast<TupleElementAddrInst>(addr)) {
      PILValue val = projectLoadValue(TEI->getOperand(), rootAddr, rootVal,
                                      beforeInst);
      PILBuilder B(beforeInst);
      return B.createTupleExtract(beforeInst->getLoc(), val, TEI->getFieldNo(),
                                  TEI->getType().getObjectType());
   }
   llvm_unreachable("unknown projection");
}

/// Returns true if all stores to \p addr commonly dominate the loop exitst of
/// \p loop.
static bool storesCommonlyDominateLoopExits(PILValue addr, PILLoop *loop,
                                            ArrayRef<PILBasicBlock *> exitingBlocks) {
   SmallPtrSet<PILBasicBlock *, 16> stores;
   for (Operand *use : addr->getUses()) {
      PILInstruction *user = use->getUser();
      if (isa<StoreInst>(user))
         stores.insert(user->getParent());
   }
   PILBasicBlock *header = loop->getHeader();
   // If a store is in the loop header, we already know that it's dominating all
   // loop exits.
   if (stores.count(header) != 0)
      return true;

   // Propagate the store-is-not-alive flag through the control flow in the loop,
   // starting at the header.
   SmallPtrSet<PILBasicBlock *, 16> storesNotAlive;
   storesNotAlive.insert(header);
   bool changed = false;
   do {
      changed = false;
      for (PILBasicBlock *block : loop->blocks()) {
         bool storeAlive = (storesNotAlive.count(block) == 0);
         if (storeAlive && stores.count(block) == 0 &&
             std::any_of(block->pred_begin(), block->pred_end(),
                         [&](PILBasicBlock *b) { return storesNotAlive.count(b) != 0; })) {
            storesNotAlive.insert(block);
            changed = true;
         }
      }
   } while (changed);

   auto isUnreachableBlock = [](PILBasicBlock *succ) {
      return isa<UnreachableInst>(succ->getTerminator());
   };

   // Check if the store-is-not-alive flag reaches any of the exits.
   for (PILBasicBlock *eb : exitingBlocks) {
      // Ignore loop exits to blocks which end in an unreachable.
      if (!std::any_of(eb->succ_begin(), eb->succ_end(), isUnreachableBlock) &&
          storesNotAlive.count(eb) != 0) {
         return false;
      }
   }
   return true;
}

void LoopTreeOptimization::hoistLoadsAndStores(PILValue addr, PILLoop *loop, InstVector &toDelete) {

   SmallVector<PILBasicBlock *, 4> exitingBlocks;
   loop->getExitingBlocks(exitingBlocks);

   // This is not a requirement for functional correctness, but we don't want to
   // _speculatively_ load and store the value (outside of the loop).
   if (!storesCommonlyDominateLoopExits(addr, loop, exitingBlocks))
      return;

   // Inserting the stores requires the exit edges to be not critical.
   for (PILBasicBlock *exitingBlock : exitingBlocks) {
      for (unsigned idx = 0, e = exitingBlock->getSuccessors().size();
           idx != e; ++idx) {
         // exitingBlock->getSuccessors() must not be moved out of this loop,
         // because the successor list is invalidated by splitCriticalEdge.
         if (!loop->contains(exitingBlock->getSuccessors()[idx])) {
            splitCriticalEdge(exitingBlock->getTerminator(), idx, DomTree, LoopInfo);
         }
      }
   }

   PILBasicBlock *preheader = loop->getLoopPreheader();
   assert(preheader && "Expected a preheader");

   // Initially load the value in the loop pre header.
   PILBuilder B(preheader->getTerminator());
   auto *initialLoad = B.createLoad(preheader->getTerminator()->getLoc(), addr,
                                    LoadOwnershipQualifier::Unqualified);

   PILSSAUpdater ssaUpdater;
   ssaUpdater.Initialize(initialLoad->getType());
   ssaUpdater.AddAvailableValue(preheader, initialLoad);

   // Set all stored values as available values in the ssaUpdater.
   // If there are multiple stores in a block, only the last one counts.
   Optional<PILLocation> loc;
   for (PILInstruction *I : LoadsAndStores) {
      if (auto *SI = isStoreToAddr(I, addr)) {
         loc = SI->getLoc();

         // If a store just stores the loaded value, bail. The operand (= the load)
         // will be removed later, so it cannot be used as available value.
         // This corner case is suprisingly hard to handle, so we just give up.
         if (isLoadFromAddr(dyn_cast<LoadInst>(SI->getSrc()), addr))
            return;

         ssaUpdater.AddAvailableValue(SI->getParent(), SI->getSrc());
      }
   }

   // Remove all stores and replace the loads with the current value.
   PILBasicBlock *currentBlock = nullptr;
   PILValue currentVal;
   for (PILInstruction *I : LoadsAndStores) {
      PILBasicBlock *block = I->getParent();
      if (block != currentBlock) {
         currentBlock = block;
         currentVal = PILValue();
      }
      if (auto *SI = isStoreToAddr(I, addr)) {
         currentVal = SI->getSrc();
         toDelete.push_back(SI);
      } else if (auto *LI = isLoadFromAddr(I, addr)) {
         // If we didn't see a store in this block yet, get the current value from
         // the ssaUpdater.
         if (!currentVal)
            currentVal = ssaUpdater.GetValueInMiddleOfBlock(block);
         PILValue projectedValue = projectLoadValue(LI->getOperand(), addr,
                                                    currentVal, LI);
         LI->replaceAllUsesWith(projectedValue);
         toDelete.push_back(LI);
      }
   }

   // Store back the value at all loop exits.
   for (PILBasicBlock *exitingBlock : exitingBlocks) {
      for (PILBasicBlock *succ : exitingBlock->getSuccessors()) {
         if (!loop->contains(succ)) {
            assert(succ->getSinglePredecessorBlock() &&
                   "should have split critical edges");
            PILBuilder B(succ->begin());
            B.createStore(loc.getValue(), ssaUpdater.GetValueInMiddleOfBlock(succ),
                          addr, StoreOwnershipQualifier::Unqualified);
         }
      }
   }

   // In case the value is only stored but never loaded in the loop.
   recursivelyDeleteTriviallyDeadInstructions(initialLoad);
}

bool LoopTreeOptimization::hoistAllLoadsAndStores(PILLoop *loop) {
   InstVector toDelete;
   for (PILValue addr : LoadAndStoreAddrs) {
      hoistLoadsAndStores(addr, loop, toDelete);
   }
   LoadsAndStores.clear();
   LoadAndStoreAddrs.clear();

   for (PILInstruction *I : toDelete) {
      I->eraseFromParent();
   }
   return !toDelete.empty();
}

namespace {
/// Hoist loop invariant code out of innermost loops.
///
/// Transforms are identified by type, not instance. Split this
/// Into two types: "High-level Loop Invariant Code Motion"
/// and "Loop Invariant Code Motion".
class LICM : public PILFunctionTransform {

public:
   LICM(bool RunsOnHighLevelSil) : RunsOnHighLevelSil(RunsOnHighLevelSil) {}

   /// True if LICM is done on high-level PIL, i.e. semantic calls are not
   /// inlined yet. In this case some semantic calls can be hoisted.
   /// We only hoist semantic calls on high-level PIL because we can be sure that
   /// e.g. an Array as PILValue is really immutable (including its content).
   bool RunsOnHighLevelSil;

   void run() override {
      PILFunction *F = getFunction();

      // If our function has ownership, skip it.
      if (F->hasOwnership())
         return;

      PILLoopAnalysis *LA = PM->getAnalysis<PILLoopAnalysis>();
      PILLoopInfo *LoopInfo = LA->get(F);

      if (LoopInfo->empty()) {
         LLVM_DEBUG(llvm::dbgs() << "No loops in " << F->getName() << "\n");
         return;
      }

      DominanceAnalysis *DA = PM->getAnalysis<DominanceAnalysis>();
      AliasAnalysis *AA = PM->getAnalysis<AliasAnalysis>();
      SideEffectAnalysis *SEA = PM->getAnalysis<SideEffectAnalysis>();
      AccessedStorageAnalysis *ASA = getAnalysis<AccessedStorageAnalysis>();
      DominanceInfo *DomTree = nullptr;

      LLVM_DEBUG(llvm::dbgs() << "Processing loops in " << F->getName() << "\n");
      bool Changed = false;

      for (auto *TopLevelLoop : *LoopInfo) {
         if (!DomTree) DomTree = DA->get(F);
         LoopTreeOptimization Opt(TopLevelLoop, LoopInfo, AA, SEA, DomTree, ASA,
                                  RunsOnHighLevelSil);
         Changed |= Opt.optimize();
      }

      if (Changed) {
         LA->lockInvalidation();
         DA->lockInvalidation();
         PM->invalidateAnalysis(F, PILAnalysis::InvalidationKind::FunctionBody);
         LA->unlockInvalidation();
         DA->unlockInvalidation();
      }
   }
};
} // end anonymous namespace

PILTransform *polar::createLICM() {
   return new LICM(false);
}

PILTransform *polar::createHighLevelLICM() {
   return new LICM(true);
}
