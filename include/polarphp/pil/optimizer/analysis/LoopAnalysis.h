//===--- LoopAnalysis.h - PIL Loop Analysis ---------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_ANALYSIS_LOOPINFOANALYSIS_H
#define POLARPHP_PIL_OPTIMIZER_ANALYSIS_LOOPINFOANALYSIS_H

#include "polarphp/pil/lang/PILBasicBlockCFG.h"
#include "polarphp/pil/lang/LoopInfo.h"
#include "polarphp/pil/lang/PILBasicBlock.h"
#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "llvm/ADT/DenseMap.h"

namespace polar {

class DominanceInfo;
class PILLoop;
class DominanceAnalysis;

/// Computes natural loop information for PIL basic blocks.
class PILLoopAnalysis : public FunctionAnalysisBase<PILLoopInfo> {
   DominanceAnalysis *DA;
public:
   PILLoopAnalysis(PILModule *)
      : FunctionAnalysisBase(PILAnalysisKind::Loop), DA(nullptr) {}

   static bool classof(const PILAnalysis *S) {
      return S->getKind() == PILAnalysisKind::Loop;
   }

   virtual bool shouldInvalidate(PILAnalysis::InvalidationKind K) override {
      return K & InvalidationKind::Branches;
   }

   // Computes loop information for the given function using dominance
   // information.
   virtual std::unique_ptr<PILLoopInfo>
   newFunctionAnalysis(PILFunction *F) override;

   virtual void initialize(PILPassManager *PM) override;
};

} // end namespace polar

#endif // POLARPHP_PIL_OPTIMIZER_ANALYSIS_LOOPINFOANALYSIS_H
