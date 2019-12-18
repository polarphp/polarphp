//===--- ARCSequenceOpts.h --------------------------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_INTERNAL_ARC_ARCSEQUENCEOPTS_H
#define POLARPHP_PIL_OPTIMIZER_INTERNAL_ARC_ARCSEQUENCEOPTS_H

#include "polarphp/pil/optimizer/internal/arc/GlobalARCSequenceDataflow.h"
#include "polarphp/pil/optimizer/internal/arc/GlobalLoopARCSequenceDataflow.h"
#include "ARCMatchingSet.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/optimizer/utils/LoopUtils.h"
#include "llvm/ADT/SetVector.h"

namespace polar {

class PILInstruction;
class PILFunction;
class AliasAnalysis;
class PostOrderAnalysis;
class LoopRegionFunctionInfo;
class PILLoopInfo;
class RCIdentityFunctionInfo;

struct ARCPairingContext {
   PILFunction &F;
   BlotMapVector<PILInstruction *, TopDownRefCountState> DecToIncStateMap;
   BlotMapVector<PILInstruction *, BottomUpRefCountState> IncToDecStateMap;
   RCIdentityFunctionInfo *RCIA;
   bool MadeChange = false;

   ARCPairingContext(PILFunction &F, RCIdentityFunctionInfo *RCIA)
      : F(F), DecToIncStateMap(), IncToDecStateMap(), RCIA(RCIA) {}
   bool performMatching(llvm::SmallVectorImpl<PILInstruction *> &NewInsts,
                        llvm::SmallVectorImpl<PILInstruction *> &DeadInsts);

   void optimizeMatchingSet(ARCMatchingSet &MatchSet,
                            llvm::SmallVectorImpl<PILInstruction *> &NewInsts,
                            llvm::SmallVectorImpl<PILInstruction *> &DeadInsts);
};

/// A composition of an ARCSequenceDataflowEvaluator and an
/// ARCPairingContext. The evaluator performs top down/bottom up dataflows
/// clearing the dataflow at loop boundaries. Then the results of the evaluator
/// are placed into the ARCPairingContext and then the ARCPairingContext is used
/// to pair retains/releases.
struct BlockARCPairingContext {
   ARCPairingContext Context;
   ARCSequenceDataflowEvaluator Evaluator;

   BlockARCPairingContext(PILFunction &F, AliasAnalysis *AA,
                          PostOrderAnalysis *POTA, RCIdentityFunctionInfo *RCFI,
                          EpilogueARCFunctionInfo *EAFI,
                          ProgramTerminationFunctionInfo *PTFI)
      : Context(F, RCFI),
        Evaluator(F, AA, POTA, RCFI, EAFI, PTFI, Context.DecToIncStateMap,
                  Context.IncToDecStateMap) {}

   bool run(bool FreezePostDomReleases) {
      bool NestingDetected = Evaluator.run(FreezePostDomReleases);
      Evaluator.clear();

      llvm::SmallVector<PILInstruction *, 8> NewInsts;
      llvm::SmallVector<PILInstruction *, 8> DeadInsts;
      bool MatchedPair = Context.performMatching(NewInsts, DeadInsts);
      NewInsts.clear();
      while (!DeadInsts.empty())
         DeadInsts.pop_back_val()->eraseFromParent();
      return NestingDetected && MatchedPair;
   }

   bool madeChange() const { return Context.MadeChange; }
};

/// A composition of a LoopARCSequenceDataflowEvaluator and an
/// ARCPairingContext. The loop nest is processed bottom up. For each loop, we
/// run the evaluator on the loop and then use the ARCPairingContext to pair
/// retains/releases and eliminate them.
struct LoopARCPairingContext : PILLoopVisitor {
   ARCPairingContext Context;
   LoopARCSequenceDataflowEvaluator Evaluator;
   LoopRegionFunctionInfo *LRFI;
   PILLoopInfo *SLI;

   LoopARCPairingContext(PILFunction &F, AliasAnalysis *AA,
                         LoopRegionFunctionInfo *LRFI, PILLoopInfo *SLI,
                         RCIdentityFunctionInfo *RCFI,
                         EpilogueARCFunctionInfo *EAFI,
                         ProgramTerminationFunctionInfo *PTFI)
      : PILLoopVisitor(&F, SLI), Context(F, RCFI),
        Evaluator(F, AA, LRFI, SLI, RCFI, EAFI, PTFI, Context.DecToIncStateMap,
                  Context.IncToDecStateMap),
        LRFI(LRFI), SLI(SLI) {}

   bool process() {
      run();
      if (!madeChange())
         return false;
      run();
      return true;
   }

   bool madeChange() const { return Context.MadeChange; }

   void runOnLoop(PILLoop *L) override;
   void runOnFunction(PILFunction *F) override;

   bool processRegion(const LoopRegion *R, bool FreezePostDomReleases,
                      bool RecomputePostDomReleases);
};

} // end polar namespace

#endif // POLARPHP_PIL_OPTIMIZER_INTERNAL_ARC_ARCSEQUENCEOPTS_H
