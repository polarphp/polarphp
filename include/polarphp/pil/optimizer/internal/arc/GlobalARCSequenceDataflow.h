//===- GlobalARCSequenceDataflow.h - ARC Sequence Flow Analysis -*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_INTERNAL_ARC_GLOBALARCSEQUENCEDATAFLOW_H
#define POLARPHP_PIL_OPTIMIZER_INTERNAL_ARC_GLOBALARCSEQUENCEDATAFLOW_H

#include "polarphp/pil/optimizer/internal/arc/RefCountState.h"
#include "polarphp/pil/optimizer/analysis/PostOrderAnalysis.h"
#include "polarphp/pil/optimizer/analysis/ProgramTerminationAnalysis.h"
#include "polarphp/basic/BlotMapVector.h"
#include "polarphp/basic/NullablePtr.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/Optional.h"

namespace polar {

class PILFunction;
class AliasAnalysis;

} // end polar namespace

namespace polar {

/// A class that implements the ARC sequence data flow.
class ARCSequenceDataflowEvaluator {
public:
   // Forward declaration of private classes that are opaque in this header.
   class ARCBBStateInfoHandle;
   class ARCBBStateInfo;
   class ARCBBState;

private:
   /// The PILFunction that we are applying the dataflow to.
   PILFunction &F;

   /// The alias analysis that we are using for alias queries.
   AliasAnalysis *AA;

   /// The post order analysis we are using for computing post orders, reverse
   /// post orders.
   PostOrderAnalysis *POA;

   /// An analysis which computes the identity root of a PILValue(), i.e. the
   /// dominating origin PILValue of the reference count that by retaining or
   /// releasing this value one is affecting.
   RCIdentityFunctionInfo *RCIA;

   /// An analysis to get the epilogue ARC instructions.
   EpilogueARCFunctionInfo *EAFI;

   /// The map from dataflow terminating decrements -> increment dataflow state.
   BlotMapVector<PILInstruction *, TopDownRefCountState> &DecToIncStateMap;

   /// The map from dataflow terminating increment -> decrement dataflow state.
   BlotMapVector<PILInstruction *, BottomUpRefCountState> &IncToDecStateMap;

   llvm::BumpPtrAllocator Allocator;
   ImmutablePointerSetFactory<PILInstruction> SetFactory;

   /// Stashed BB information.
   std::unique_ptr<ARCBBStateInfo> BBStateInfo;

public:
   ARCSequenceDataflowEvaluator(
      PILFunction &F, AliasAnalysis *AA, PostOrderAnalysis *POA,
      RCIdentityFunctionInfo *RCIA, EpilogueARCFunctionInfo *EAFI,
      ProgramTerminationFunctionInfo *PTFI,
      BlotMapVector<PILInstruction *, TopDownRefCountState> &DecToIncStateMap,
      BlotMapVector<PILInstruction *, BottomUpRefCountState> &IncToDecStateMap);
   ~ARCSequenceDataflowEvaluator();

   /// Run the dataflow evaluator.
   bool run(bool FreezePostDomReleases);

   /// Clear all of the states we are tracking for the various basic blocks.
   void clear();

   PILFunction *getFunction() const { return &F; }

private:
   /// Perform the bottom up data flow.
   bool processBottomUp(bool freezePostDomReleases);

   /// Perform the top down dataflow.
   bool processTopDown();

   /// Merge in the BottomUp state of any successors of DataHandle.getBB() into
   /// DataHandle.getState().
   void mergeSuccessors(ARCBBStateInfoHandle &DataHandle);

   /// Merge in the TopDown state of any predecessors of DataHandle.getBB() into
   /// DataHandle.getState().
   void mergePredecessors(ARCBBStateInfoHandle &DataHandle);

   bool processBBBottomUp(ARCBBState &BBState,
                          bool FreezeOwnedArgEpilogueReleases);
   bool processBBTopDown(ARCBBState &BBState);
   void computePostDominatingConsumedArgMap();

   llvm::Optional<ARCBBStateInfoHandle> getBottomUpBBState(PILBasicBlock *BB);
   llvm::Optional<ARCBBStateInfoHandle> getTopDownBBState(PILBasicBlock *BB);
};

} // end polar namespace

#endif // POLARPHP_PIL_OPTIMIZER_INTERNAL_ARC_GLOBALARCSEQUENCEDATAFLOW_H
