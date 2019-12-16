//===--- DominanceAnalysis.h - PIL Dominance Analysis -----------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_ANALYSIS_DOMINANCEANALYSIS_H
#define POLARPHP_PIL_OPTIMIZER_ANALYSIS_DOMINANCEANALYSIS_H

#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/Dominance.h"
#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "llvm/ADT/DenseMap.h"

namespace polar {

class PILModule;
class PILInstruction;

class DominanceAnalysis : public FunctionAnalysisBase<DominanceInfo> {
protected:
  virtual void verify(DominanceInfo *DI) const override {
    if (DI->getRoots().empty())
      return;
    DI->verify();
  }

public:
  DominanceAnalysis()
      : FunctionAnalysisBase<DominanceInfo>(PILAnalysisKind::Dominance) {}

  DominanceAnalysis(const DominanceAnalysis &) = delete;
  DominanceAnalysis &operator=(const DominanceAnalysis &) = delete;

  static bool classof(const PILAnalysis *S) {
    return S->getKind() == PILAnalysisKind::Dominance;
  }

  std::unique_ptr<DominanceInfo> newFunctionAnalysis(PILFunction *F) override {
    return std::make_unique<DominanceInfo>(F);
  }

  virtual bool shouldInvalidate(PILAnalysis::InvalidationKind K) override {
    return K & InvalidationKind::Branches;
  }
};

class PostDominanceAnalysis : public FunctionAnalysisBase<PostDominanceInfo> {
protected:
  virtual void verify(PostDominanceInfo *PDI) const override {
    if (PDI->getRoots().empty())
      return;
    PDI->verify();
  }

public:
  PostDominanceAnalysis()
      : FunctionAnalysisBase<PostDominanceInfo>(
            PILAnalysisKind::PostDominance) {}

  PostDominanceAnalysis(const PostDominanceAnalysis &) = delete;
  PostDominanceAnalysis &operator=(const PostDominanceAnalysis &) = delete;

  static bool classof(const PILAnalysis *S) {
    return S->getKind() == PILAnalysisKind::PostDominance;
  }

  std::unique_ptr<PostDominanceInfo>
  newFunctionAnalysis(PILFunction *F) override {
    return std::make_unique<PostDominanceInfo>(F);
  }

  virtual bool shouldInvalidate(PILAnalysis::InvalidationKind K) override {
    return K & InvalidationKind::Branches;
  }
};

} // end namespace polar

#endif
