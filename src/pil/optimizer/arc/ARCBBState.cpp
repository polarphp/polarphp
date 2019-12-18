//===--- ARCBBState.cpp ---------------------------------------------------===//
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

#define DEBUG_TYPE "arc-sequence-opts"

#include "polarphp/pil/optimizer/internal/arc/ARCBBState.h"
#include "llvm/Support/Debug.h"

using namespace polar;

//===----------------------------------------------------------------------===//
//                                 ARCBBState
//===----------------------------------------------------------------------===//

namespace {

using ARCBBState = ARCSequenceDataflowEvaluator::ARCBBState;

} // end anonymous namespace

/// Merge in the state of the successor basic block. This is an intersection
/// operation.
void ARCBBState::mergeSuccBottomUp(ARCBBState &SuccBBState) {
   // For each [(PILValue, BottomUpState)] that we are tracking...
   for (auto &Pair : getBottomupStates()) {
      if (!Pair.hasValue())
         continue;

      PILValue RefCountedValue = Pair->first;

      // If our PILValue was blotted, skip it. This will be ignored for the rest
      // of the ARC optimization.
      if (!RefCountedValue)
         continue;

      // Then attempt to lookup the corresponding (PILValue, BottomUpState) from
      // SuccBB. If we fail to do so, blot this PILValue and continue.
      //
      // Since we are already initialized by initSuccBottomUp(), this has the
      // effect of an intersection.
      auto Other = SuccBBState.PtrToBottomUpState.find(RefCountedValue);
      if (Other == SuccBBState.PtrToBottomUpState.end()) {
         PtrToBottomUpState.erase(RefCountedValue);
         continue;
      }

      PILValue OtherRefCountedValue = (*Other)->first;

      // If the other ref count value was blotted, blot our value and continue.
      // This has the effect of an intersection since we already checked earlier
      // that RefCountedValue was not blotted.
      if (!OtherRefCountedValue) {
         PtrToBottomUpState.erase(RefCountedValue);
         continue;
      }

      BottomUpRefCountState &RefCountState = Pair->second;
      BottomUpRefCountState &OtherRefCountState = (*Other)->second;

      // Ok, now we know that the merged set can safely represent a set of
      // of instructions which together semantically act as one ref count
      // increment. Merge the two states together.
      if (!RefCountState.merge(OtherRefCountState)) {
         PtrToBottomUpState.erase(RefCountedValue);
      }
   }
}

/// Initialize this BB with the state of the successor basic block. This is
/// called on a basic block's state and then any other successors states are
/// merged in.
void ARCBBState::initSuccBottomUp(ARCBBState &SuccBBState) {
   PtrToBottomUpState = SuccBBState.PtrToBottomUpState;
}

/// Merge in the state of the predecessor basic block.
void ARCBBState::mergePredTopDown(ARCBBState &PredBBState) {
   // For each [(PILValue, TopDownState)] that we are tracking...
   for (auto &Pair : getTopDownStates()) {
      if (!Pair.hasValue())
         continue;

      PILValue RefCountedValue = Pair->first;

      // If our PILValue was blotted, skip it. This will be ignored in the rest of
      // the optimizer.
      if (!RefCountedValue)
         continue;

      // Then attempt to lookup the corresponding (PILValue, TopDownState) from
      // PredBB. If we fail to do so, blot this PILValue and continue.
      //
      // Since we are already initialized by initPredTopDown(), this has the
      // effect of an intersection.
      auto Other = PredBBState.PtrToTopDownState.find(RefCountedValue);
      if (Other == PredBBState.PtrToTopDownState.end()) {
         PtrToTopDownState.erase(RefCountedValue);
         continue;
      }

      PILValue OtherRefCountedValue = (*Other)->first;

      // If the other ref count value was blotted, blot our value and continue.
      // This has the effect of an intersection.
      if (!OtherRefCountedValue) {
         PtrToTopDownState.erase(RefCountedValue);
         continue;
      }

      // Ok, so now we know that the ref counted value we are tracking was not
      // blotted on either side. Grab the states.
      TopDownRefCountState &RefCountState = Pair->second;
      TopDownRefCountState &OtherRefCountState = (*Other)->second;

      // Attempt to merge Other into this ref count state. If we fail, blot this
      // ref counted value and continue.
      if (!RefCountState.merge(OtherRefCountState)) {
         LLVM_DEBUG(llvm::dbgs() << "Failed to merge!\n");
         PtrToTopDownState.erase(RefCountedValue);
         continue;
      }
   }
}

/// Initialize the state for this BB with the state of its predecessor
/// BB. Used to create an initial state before we merge in other
/// predecessors.
void ARCBBState::initPredTopDown(ARCBBState &PredBBState) {
   PtrToTopDownState = PredBBState.PtrToTopDownState;
}

//===----------------------------------------------------------------------===//
//                               ARCBBStateInfo
//===----------------------------------------------------------------------===//

namespace {

using ARCBBStateInfo = ARCSequenceDataflowEvaluator::ARCBBStateInfo;
using ARCBBStateInfoHandle = ARCSequenceDataflowEvaluator::ARCBBStateInfoHandle;

} // end anonymous namespace

ARCBBStateInfo::ARCBBStateInfo(PILFunction *F, PostOrderAnalysis *POA,
                               ProgramTerminationFunctionInfo *PTFI)
   : BBToBBIDMap(), BBIDToBottomUpBBStateMap(POA->get(F)->size()),
     BBIDToTopDownBBStateMap(POA->get(F)->size()), BackedgeMap() {

   // Initialize state for each one of our BB's in the RPOT. *NOTE* This means
   // that unreachable predecessors will not have any BBState associated with
   // them.
   for (PILBasicBlock *BB : POA->get(F)->getReversePostOrder()) {
      unsigned BBID = BBToBBIDMap.size();
      BBToBBIDMap[BB] = BBID;

      bool IsLeakingBB = PTFI->isProgramTerminatingBlock(BB);
      BBIDToBottomUpBBStateMap[BBID].init(BB, IsLeakingBB);
      BBIDToTopDownBBStateMap[BBID].init(BB, IsLeakingBB);

      for (auto &Succ : BB->getSuccessors())
         if (PILBasicBlock *SuccBB = Succ.getBB())
            if (BBToBBIDMap.count(SuccBB))
               BackedgeMap[BB].insert(SuccBB);
   }
}

llvm::Optional<ARCBBStateInfoHandle>
ARCBBStateInfo::getBottomUpBBHandle(PILBasicBlock *BB) {
   auto OptID = getBBID(BB);
   if (!OptID.hasValue())
      return None;

   unsigned ID = OptID.getValue();

   auto BackedgeIter = BackedgeMap.find(BB);
   if (BackedgeIter == BackedgeMap.end())
      return ARCBBStateInfoHandle(BB, ID, BBIDToBottomUpBBStateMap[ID]);
   return ARCBBStateInfoHandle(BB, ID, BBIDToBottomUpBBStateMap[ID],
                               BackedgeIter->second);
}

llvm::Optional<ARCBBStateInfoHandle>
ARCBBStateInfo::getTopDownBBHandle(PILBasicBlock *BB) {
   auto MaybeID = getBBID(BB);
   if (!MaybeID.hasValue())
      return None;

   unsigned ID = MaybeID.getValue();

   auto BackedgeIter = BackedgeMap.find(BB);
   if (BackedgeIter == BackedgeMap.end())
      return ARCBBStateInfoHandle(BB, ID, BBIDToTopDownBBStateMap[ID]);
   return ARCBBStateInfoHandle(BB, ID, BBIDToTopDownBBStateMap[ID],
                               BackedgeIter->second);
}

llvm::Optional<unsigned> ARCBBStateInfo::getBBID(PILBasicBlock *BB) const {
   auto Iter = BBToBBIDMap.find(BB);
   if (Iter == BBToBBIDMap.end())
      return None;
   return Iter->second;
}

void ARCBBStateInfo::clear() {
   assert(BBIDToBottomUpBBStateMap.size() == BBIDToTopDownBBStateMap.size() &&
          "These should be one to one mapped to basic blocks so should"
          " have the same size");
   for (unsigned i : indices(BBIDToBottomUpBBStateMap)) {
      BBIDToBottomUpBBStateMap[i].clear();
      BBIDToTopDownBBStateMap[i].clear();
   }
}
