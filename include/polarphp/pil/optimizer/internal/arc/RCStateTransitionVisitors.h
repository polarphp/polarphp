//===--- RCStateTransitionVisitors.h ----------------------------*- C++ -*-===//
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
// This file contains RCStateTransitionVisitors for performing ARC dataflow. It
// is necessary to prevent a circular dependency in between RefCountState.h and
// RCStateTransition.h
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_OPTIMIZER_INTERNAL_ARC_RCSTATETRANSITIONVISITORS_H
#define POLARPHP_PIL_OPTIMIZER_INTERNAL_ARC_RCSTATETRANSITIONVISITORS_H

#include "polarphp/pil/optimizer/internal/arc/ARCBBState.h"
#include "polarphp/pil/optimizer/internal/arc/ARCRegionState.h"
#include "polarphp/pil/optimizer/internal/arc/RCStateTransition.h"
#include "polarphp/basic/ImmutablePointerSet.h"
#include "polarphp/basic/BlotMapVector.h"

//===----------------------------------------------------------------------===//
//                          RCStateTransitionVisitor
//===----------------------------------------------------------------------===//

namespace polar {

/// Define a visitor for visiting instructions according to their
/// RCStateTransitionKind.
template <typename ImplTy, typename ResultTy>
class RCStateTransitionKindVisitor {
   ImplTy &asImpl() { return *reinterpret_cast<ImplTy *>(this); }

public:
#define KIND(K) ResultTy visit ## K(PILNode *) { return ResultTy(); }
#include "polarphp/pil/optimizer/internal/arc/RCStateTransitionDef.h"

   ResultTy visit(PILNode *N) {
      switch (getRCStateTransitionKind(N)) {
#define KIND(K)                                 \
  case RCStateTransitionKind::K:                \
    return asImpl().visit ## K(N);
#include "polarphp/pil/optimizer/internal/arc/RCStateTransitionDef.h"
      }
      llvm_unreachable("Covered switch isn't covered?!");
   }
};

} // end polar namespace

//===----------------------------------------------------------------------===//
//                      RCStateTransitionDataflowResult
//===----------------------------------------------------------------------===//

namespace polar {

enum class RCStateTransitionDataflowResultKind {
   /// Can this dataflow result have no further effects on any state. This means
   /// we can just early out and break early.
      NoEffects,

   /// Must we check for effects.
      CheckForEffects,
};

struct RCStateTransitionDataflowResult {
   using DataflowResultKind = RCStateTransitionDataflowResultKind;

   DataflowResultKind Kind;
   PILValue RCIdentity;
   bool NestingDetected = false;

   RCStateTransitionDataflowResult()
      : Kind(DataflowResultKind::CheckForEffects), RCIdentity() {}
   RCStateTransitionDataflowResult(DataflowResultKind Kind)
      : Kind(Kind), RCIdentity() {}
   RCStateTransitionDataflowResult(PILValue RCIdentity,
                                   bool NestingDetected = false)
      : Kind(DataflowResultKind::CheckForEffects), RCIdentity(RCIdentity),
        NestingDetected(NestingDetected) {}
   RCStateTransitionDataflowResult(const RCStateTransitionDataflowResult &) =
   default;
   ~RCStateTransitionDataflowResult() = default;
};

} // end polar namespace

namespace llvm {
raw_ostream &operator<<(raw_ostream &os,
                        polar::RCStateTransitionDataflowResult Kind);
} // end llvm namespace

//===----------------------------------------------------------------------===//
//                       BottomUpdataflowRCStateVisitor
//===----------------------------------------------------------------------===//

namespace polar {

/// A visitor for performing the bottom up dataflow depending on the
/// RCState. Enables behavior to be cleanly customized depending on the
/// RCStateTransition associated with an instruction.
template <class ARCState>
class BottomUpDataflowRCStateVisitor
   : public RCStateTransitionKindVisitor<BottomUpDataflowRCStateVisitor<ARCState>,
      RCStateTransitionDataflowResult> {
   /// A local typedef to make things cleaner.
   using DataflowResult = RCStateTransitionDataflowResult;
   using IncToDecStateMapTy =
   BlotMapVector<PILInstruction *, BottomUpRefCountState>;

public:
   RCIdentityFunctionInfo *RCFI;
   EpilogueARCFunctionInfo *EAFI;
   ARCState &DataflowState;
   bool FreezeOwnedArgEpilogueReleases;
   BlotMapVector<PILInstruction *, BottomUpRefCountState> &IncToDecStateMap;
   ImmutablePointerSetFactory<PILInstruction> &SetFactory;

public:
   BottomUpDataflowRCStateVisitor(
      RCIdentityFunctionInfo *RCFI, EpilogueARCFunctionInfo *EAFI,
      ARCState &DataflowState, bool FreezeOwnedArgEpilogueReleases,
      IncToDecStateMapTy &IncToDecStateMap,
      ImmutablePointerSetFactory<PILInstruction> &SetFactory);
   DataflowResult visitAutoreleasePoolCall(PILNode *N);
   DataflowResult visitStrongDecrement(PILNode *N);
   DataflowResult visitStrongIncrement(PILNode *N);
};

} // end polar namespace

//===----------------------------------------------------------------------===//
//                       TopDownDataflowRCStateVisitor
//===----------------------------------------------------------------------===//

namespace polar {

/// A visitor for performing the bottom up dataflow depending on the
/// RCState. Enables behavior to be cleanly customized depending on the
/// RCStateTransition associated with an instruction.
template <class ARCState>
class TopDownDataflowRCStateVisitor
   : public RCStateTransitionKindVisitor<TopDownDataflowRCStateVisitor<ARCState>,
      RCStateTransitionDataflowResult> {
   /// A local typedef to make things cleaner.
   using DataflowResult = RCStateTransitionDataflowResult;
   using DecToIncStateMapTy =
   BlotMapVector<PILInstruction *, TopDownRefCountState>;

   RCIdentityFunctionInfo *RCFI;
   ARCState &DataflowState;
   DecToIncStateMapTy &DecToIncStateMap;
   ImmutablePointerSetFactory<PILInstruction> &SetFactory;

public:
   TopDownDataflowRCStateVisitor(
      RCIdentityFunctionInfo *RCFI, ARCState &State,
      DecToIncStateMapTy &DecToIncStateMap,
      ImmutablePointerSetFactory<PILInstruction> &SetFactory);
   DataflowResult visitAutoreleasePoolCall(PILNode *N);
   DataflowResult visitStrongDecrement(PILNode *N);
   DataflowResult visitStrongIncrement(PILNode *N);
   DataflowResult visitStrongEntrance(PILNode *N);

private:
   DataflowResult visitStrongEntranceApply(ApplyInst *AI);
   DataflowResult visitStrongEntrancePartialApply(PartialApplyInst *PAI);
   DataflowResult visitStrongEntranceArgument(PILFunctionArgument *Arg);
   DataflowResult visitStrongEntranceAllocRef(AllocRefInst *ARI);
   DataflowResult visitStrongEntranceAllocRefDynamic(AllocRefDynamicInst *ARI);
   DataflowResult visitStrongAllocBox(AllocBoxInst *ABI);
};

} // end polar namespace

//===----------------------------------------------------------------------===//
//                       Forward Template Declarations
//===----------------------------------------------------------------------===//

namespace polar {

extern template class BottomUpDataflowRCStateVisitor<
   ARCSequenceDataflowEvaluator::ARCBBState>;
extern template class BottomUpDataflowRCStateVisitor<ARCRegionState>;

extern template class TopDownDataflowRCStateVisitor<
   ARCSequenceDataflowEvaluator::ARCBBState>;
extern template class TopDownDataflowRCStateVisitor<ARCRegionState>;

} // end polar namespace

#endif // POLARPHP_PIL_OPTIMIZER_INTERNAL_ARC_RCSTATETRANSITIONVISITORS_H
