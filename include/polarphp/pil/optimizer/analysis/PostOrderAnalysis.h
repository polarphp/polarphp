//===--- PostOrderAnalysis.h - PIL POT and RPOT Analysis --------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_ANALYSIS_POSTORDERANALYSIS_H
#define POLARPHP_PIL_OPTIMIZER_ANALYSIS_POSTORDERANALYSIS_H

#include "polarphp/basic/Range.h"
#include "polarphp/pil/lang/PILBasicBlockCFG.h"
#include "polarphp/pil/lang/PostOrder.h"
#include "polarphp/pil/lang/PILBasicBlock.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/iterator_range.h"
#include <vector>

namespace polar {

class PILBasicBlock;
class PILFunction;

/// This class is a simple wrapper around the POT iterator provided by LLVM. It
/// lazily re-evaluates the post order when it is invalidated so that we do not
/// reform the post order over and over again (it can be expensive).
class PostOrderAnalysis : public FunctionAnalysisBase<PostOrderFunctionInfo> {
protected:
   virtual std::unique_ptr<PostOrderFunctionInfo>
   newFunctionAnalysis(PILFunction *F) override {
      return std::make_unique<PostOrderFunctionInfo>(F);
   }

   virtual bool shouldInvalidate(PILAnalysis::InvalidationKind K) override {
      return K & InvalidationKind::Branches;
   }

public:
   PostOrderAnalysis()
      : FunctionAnalysisBase<PostOrderFunctionInfo>(
      PILAnalysisKind::PostOrder) {}

   // This is a cache and shouldn't be copied around.
   PostOrderAnalysis(const PostOrderAnalysis &) = delete;
   PostOrderAnalysis &operator=(const PostOrderAnalysis &) = delete;

   static bool classof(const PILAnalysis *S) {
      return S->getKind() == PILAnalysisKind::PostOrder;
   }
};

} // end namespace polar

#endif
