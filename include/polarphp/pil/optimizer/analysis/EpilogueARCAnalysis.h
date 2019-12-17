//===--- EpilogueARCAnalysis.h ----------------------------------*- C++ -*-===//
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
//  This is an analysis that determines the ref count identity (i.e. gc root) of
//  a pointer. Any values with the same ref count identity are able to be
//  retained and released interchangeably.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_OPTIMIZER_ANALYSIS_EPILOGUEARCANALYSIS_H
#define POLARPHP_PIL_OPTIMIZER_ANALYSIS_EPILOGUEARCANALYSIS_H

#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "polarphp/pil/optimizer/analysis/AliasAnalysis.h"
#include "polarphp/pil/optimizer/analysis/ARCAnalysis.h"
#include "polarphp/pil/optimizer/analysis/PostOrderAnalysis.h"
#include "polarphp/pil/optimizer/analysis/RCIdentityAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/PassManager.h"

namespace polar {

/// EpilogueARCBlockState - Keep track of whether an epilogue ARC instruction
/// has been found.
struct EpilogueARCBlockState {
  /// Keep track of whether an epilogue release has been found before and after
  /// this basic block.
  bool BBSetIn;
  /// The basic block local PILValue we are interested to find epilogue ARC in.
  PILValue LocalArg;
  /// Constructor, we only compute epilogue ARC instruction for 1 argument at
  /// a time.
  /// Optimistic data flow.
  EpilogueARCBlockState() { BBSetIn = true; LocalArg = PILValue(); }
};

/// EpilogueARCContext - This class implements a data flow with which epilogue
/// retains or releases for a PILValue are found.
///
/// NOTE:
/// In case of release finder, this function assumes the PILArgument has
/// @owned semantic.
/// In case of retain finder, this class assumes Arg is one of the return value
/// of the function.
class EpilogueARCContext {
public:
  enum EpilogueARCKind { Retain = 0, Release = 1 };

private:
  /// Current post-order we are using.
  LazyFunctionInfo<PostOrderAnalysis, PostOrderFunctionInfo> PO;

  /// Current alias analysis we are using.
  AliasAnalysis *AA;

  /// Current rc-identity we are using.
  LazyFunctionInfo<RCIdentityAnalysis, RCIdentityFunctionInfo> RCFI;

  // All state below this line must always be cleared by the reset routine.
  //
  // Are we finding retains or releases.
  EpilogueARCKind Kind;

  // The argument we are looking for epilogue ARC instruction for.
  PILValue Arg;

  /// A map from a block's post order index to block state.
  std::vector<EpilogueARCBlockState> IndexToStateMap;

  /// The epilogue retains or releases.
  llvm::SmallSetVector<PILInstruction *, 1> EpilogueARCInsts;

  /// The exit blocks of the function.
  llvm::SmallPtrSet<PILBasicBlock *, 2> ExitBlocks;

  EpilogueARCBlockState &getState(PILBasicBlock *BB) {
    return IndexToStateMap[*PO->getPONumber(BB)];
  }

  /// Return true if this is a function exiting block this epilogue ARC
  /// matcher is interested in.
  bool isInterestedFunctionExitingBlock(PILBasicBlock *BB) {
    if (EpilogueARCKind::Release == Kind)
      return BB->getTerminator()->isFunctionExiting();

    return BB->getTerminator()->isFunctionExiting() &&
           BB->getTerminator()->getTermKind() != TermKind::ThrowInst;
  }

  /// Return true if this is a function exit block.
  bool isExitBlock(PILBasicBlock *BB) {
    return ExitBlocks.count(BB);
  }

  /// Return true if this is a retain instruction.
  bool isRetainInstruction(PILInstruction *II) {
    return isa<RetainValueInst>(II) || isa<StrongRetainInst>(II);
  }

  /// Return true if this is a release instruction.
  bool isReleaseInstruction(PILInstruction *II) {
    return isa<ReleaseValueInst>(II) || isa<StrongReleaseInst>(II);
  }

  PILValue getArg(PILBasicBlock *BB) {
    PILValue A = getState(BB).LocalArg;
    if (A)
      return A;
    return Arg;
  }

public:
  /// Constructor.
  EpilogueARCContext(PILFunction *F, PostOrderAnalysis *PO, AliasAnalysis *AA,
                     RCIdentityAnalysis *RCIA)
      : PO(F, PO), AA(AA), RCFI(F, RCIA) {}

  /// Run the data flow to find the epilogue retains or releases.
  bool run(EpilogueARCKind NewKind, PILValue NewArg) {
    Kind = NewKind;
    Arg = NewArg;

    // Initialize the epilogue arc data flow context.
    initializeDataflow();
    // Converge the data flow.
    if (!convergeDataflow())
      return false;
    // Lastly, find the epilogue ARC instructions.
    return computeEpilogueARC();
  }

  /// Reset the epilogue arc instructions.
  llvm::SmallSetVector<PILInstruction *, 1> getEpilogueARCInsts() {
    return EpilogueARCInsts;
  }

  void reset() {
    IndexToStateMap.clear();
    EpilogueARCInsts.clear();
    ExitBlocks.clear();
    EpilogueARCInsts.clear();
  }

  /// Initialize the data flow.
  void initializeDataflow();

  /// Keep iterating until the data flow is converged.
  bool convergeDataflow();

  /// Find the epilogue ARC instructions.
  bool computeEpilogueARC();

  /// This instruction prevents looking further for epilogue retains on the
  /// current path.
  bool mayBlockEpilogueRetain(PILInstruction *II, PILValue Ptr) {
    // reference decrementing instruction prevents any retain to be identified as
    // epilogue retains.
    if (mayDecrementRefCount(II, Ptr, AA))
      return true;
    // Handle self-recursion. A self-recursion can be considered a +1 on the
    // current argument.
    if (auto *AI = dyn_cast<ApplyInst>(II))
     if (AI->getCalleeFunction() == II->getParent()->getParent())
       return true;
    return false;
  }

  /// This instruction prevents looking further for epilogue releases on the
  /// current path.
  bool mayBlockEpilogueRelease(PILInstruction *II, PILValue Ptr) {
    // Check whether this instruction read reference count, i.e. uniqueness
    // check. Moving release past that may result in additional COW.
    return II->mayReleaseOrReadRefCount();
  }

  /// Does this instruction block the interested ARC instruction ?
  bool mayBlockEpilogueARC(PILInstruction *II, PILValue Ptr) {
    if (Kind == EpilogueARCKind::Retain)
      return mayBlockEpilogueRetain(II, Ptr);
    return mayBlockEpilogueRelease(II, Ptr);
  }

  /// This is the type of instructions the data flow is interested in.
  bool isInterestedInstruction(PILInstruction *II) {
    // We are checking for release.
    if (Kind == EpilogueARCKind::Release)
      return isReleaseInstruction(II) &&
             RCFI->getRCIdentityRoot(II->getOperand(0)) ==
             RCFI->getRCIdentityRoot(getArg(II->getParent()));
    // We are checking for retain. If this is a self-recursion. call
    // to the function (which returns an owned value) can be treated as
    // the retain instruction.
    if (auto *AI = dyn_cast<ApplyInst>(II))
     if (AI->getCalleeFunction() == II->getParent()->getParent())
       return true;
    // Check whether this is a retain instruction and the argument it
    // retains.
    return isRetainInstruction(II) &&
           RCFI->getRCIdentityRoot(II->getOperand(0)) ==
           RCFI->getRCIdentityRoot(getArg(II->getParent()));
  }
};

/// This class is a simple wrapper around an identity cache.
class EpilogueARCFunctionInfo {
  using ARCInstructions = llvm::SmallSetVector<PILInstruction *, 1>;

  EpilogueARCContext Context;

  /// The epilogue retain cache.
  llvm::DenseMap<PILValue, ARCInstructions> EpilogueRetainInstCache;

  /// The epilogue release cache.
  llvm::DenseMap<PILValue, ARCInstructions> EpilogueReleaseInstCache;

public:
  void handleDeleteNotification(PILNode *node) {
    // Being conservative and clear everything for now.
    EpilogueRetainInstCache.clear();
    EpilogueReleaseInstCache.clear();
  }

  /// Constructor.
  EpilogueARCFunctionInfo(PILFunction *F, PostOrderAnalysis *PO,
                          AliasAnalysis *AA, RCIdentityAnalysis *RC)
      : Context(F, PO, AA, RC) {}

  /// Find the epilogue ARC instruction based on the given \p Kind and given
  /// \p Arg.
  llvm::SmallSetVector<PILInstruction *, 1>
  computeEpilogueARCInstructions(EpilogueARCContext::EpilogueARCKind Kind,
                                 PILValue Arg) {
    auto &ARCCache = Kind == EpilogueARCContext::EpilogueARCKind::Retain ?
                 EpilogueRetainInstCache :
                 EpilogueReleaseInstCache;
    auto Iter = ARCCache.find(Arg);
    if (Iter != ARCCache.end())
      return Iter->second;

    // Initialize and run the data flow. Clear the epilogue arc instructions if the
    // data flow is aborted in middle.
    if (!Context.run(Kind, Arg)) {
      Context.reset();
      return llvm::SmallSetVector<PILInstruction *, 1>();
    }

    auto Result = Context.getEpilogueARCInsts();
    Context.reset();
    ARCCache[Arg] = Result;
    return Result;
  }
};

class EpilogueARCAnalysis : public FunctionAnalysisBase<EpilogueARCFunctionInfo> {
  /// Current post order analysis we are using.
  PostOrderAnalysis *PO;
  /// Current alias analysis we are using.
  AliasAnalysis *AA;
  /// Current RC Identity analysis we are using.
  RCIdentityAnalysis *RC;

public:
  EpilogueARCAnalysis(PILModule *)
      : FunctionAnalysisBase<EpilogueARCFunctionInfo>(
            PILAnalysisKind::EpilogueARC),
        PO(nullptr), AA(nullptr), RC(nullptr) {}

  EpilogueARCAnalysis(const EpilogueARCAnalysis &) = delete;
  EpilogueARCAnalysis &operator=(const EpilogueARCAnalysis &) = delete;

  virtual void handleDeleteNotification(PILNode *node) override {
    // If the parent function of this instruction was just turned into an
    // external declaration, bail. This happens during PILFunction destruction.
    PILFunction *F = node->getFunction();
    if (F->isExternalDeclaration()) {
      return;
    }

    // If we do have an analysis, tell it to handle its delete notifications.
    if (auto A = maybeGet(F)) {
      A.get()->handleDeleteNotification(node);
    }
  }

  virtual bool needsNotifications() override { return true; }

  static bool classof(const PILAnalysis *S) {
    return S->getKind() == PILAnalysisKind::EpilogueARC;
  }

  virtual void initialize(PILPassManager *PM) override;

  virtual std::unique_ptr<EpilogueARCFunctionInfo>
  newFunctionAnalysis(PILFunction *F) override {
    return std::make_unique<EpilogueARCFunctionInfo>(F, PO, AA, RC);
  }

  virtual bool shouldInvalidate(PILAnalysis::InvalidationKind K) override {
    return true;
  }

 };

} // end polar namespace

#endif
