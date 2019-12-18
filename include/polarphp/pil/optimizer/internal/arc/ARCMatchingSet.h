//===--- ARCMatchingSet.h ---------------------------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_INTERNAL_ARC_ARCMATCHINGSET_H
#define POLARPHP_PIL_OPTIMIZER_INTERNAL_ARC_ARCMATCHINGSET_H

#include "polarphp/pil/optimizer/internal/arc/GlobalARCSequenceDataflow.h"
#include "polarphp/pil/optimizer/internal/arc/GlobalLoopARCSequenceDataflow.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/optimizer/utils/LoopUtils.h"
#include "polarphp/pil/optimizer/analysis/RCIdentityAnalysis.h"
#include "llvm/ADT/SetVector.h"

namespace polar {

class PILInstruction;
class PILFunction;
class AliasAnalysis;
class PostOrderAnalysis;
class LoopRegionFunctionInfo;
class PILLoopInfo;
class RCIdentityFunctionInfo;

/// A set of matching reference count increments, decrements, increment
/// insertion pts, and decrement insertion pts.
struct ARCMatchingSet {

   /// The pointer that this ARCMatchingSet is providing matching increment and
   /// decrement sets for.
   ///
   /// TODO: This should really be called RCIdentity.
   PILValue Ptr;

   /// The set of reference count increments that were paired.
   llvm::SetVector<PILInstruction *> Increments;

   /// The set of reference count decrements that were paired.
   llvm::SetVector<PILInstruction *> Decrements;

   // This is a data structure that cannot be moved or copied.
   ARCMatchingSet() = default;
   ARCMatchingSet(const ARCMatchingSet &) = delete;
   ARCMatchingSet(ARCMatchingSet &&) = delete;
   ARCMatchingSet &operator=(const ARCMatchingSet &) = delete;
   ARCMatchingSet &operator=(ARCMatchingSet &&) = delete;

   void clear() {
      Ptr = PILValue();
      Increments.clear();
      Decrements.clear();
   }
};

struct MatchingSetFlags {
   bool KnownSafe;
   bool CodeMotionSafe;
};
static_assert(std::is_pod<MatchingSetFlags>::value,
              "MatchingSetFlags should be a pod.");

struct ARCMatchingSetBuilder {
   using TDMapTy = BlotMapVector<PILInstruction *, TopDownRefCountState>;
   using BUMapTy = BlotMapVector<PILInstruction *, BottomUpRefCountState>;

   TDMapTy &TDMap;
   BUMapTy &BUMap;

   llvm::SmallVector<PILInstruction *, 8> NewIncrements;
   llvm::SmallVector<PILInstruction *, 8> NewDecrements;
   bool MatchedPair;
   ARCMatchingSet MatchSet;
   bool PtrIsGuaranteedArg;

   RCIdentityFunctionInfo *RCIA;

public:
   ARCMatchingSetBuilder(TDMapTy &TDMap, BUMapTy &BUMap,
                         RCIdentityFunctionInfo *RCIA)
      : TDMap(TDMap), BUMap(BUMap), MatchedPair(false),
        PtrIsGuaranteedArg(false), RCIA(RCIA) {}

   void init(PILInstruction *Inst) {
      clear();
      MatchSet.Ptr = RCIA->getRCIdentityRoot(Inst->getOperand(0));

      // If we have a function argument that is guaranteed, set the guaranteed
      // flag so we know that it is always known safe.
      if (auto *A = dyn_cast<PILFunctionArgument>(MatchSet.Ptr)) {
         auto C = A->getArgumentConvention();
         PtrIsGuaranteedArg = C == PILArgumentConvention::Direct_Guaranteed;
      }
      NewIncrements.push_back(Inst);
   }

   void clear() {
      MatchSet.clear();
      MatchedPair = false;
      NewIncrements.clear();
      NewDecrements.clear();
   }

   bool matchUpIncDecSetsForPtr();

   // We only allow for get result when this object is invalidated via a move.
   ARCMatchingSet &getResult() { return MatchSet; }

   bool matchedPair() const { return MatchedPair; }

private:
   /// Returns .Some(MatchingSetFlags) on success and .None on failure.
   Optional<MatchingSetFlags> matchIncrementsToDecrements();
   Optional<MatchingSetFlags> matchDecrementsToIncrements();
};

} // end polar namespace

#endif // POLARPHP_PIL_OPTIMIZER_INTERNAL_ARC_ARCMATCHINGSET_H
