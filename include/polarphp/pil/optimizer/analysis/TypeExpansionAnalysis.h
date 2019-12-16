//===--- TypeExpansionAnalysis.h --------------------------------*- C++ -*-===//
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
#ifndef POLARPHP_PIL_OPTIMIZER_ANALYSIS_TYPEEXPANSIONANALYSIS_H
#define POLARPHP_PIL_OPTIMIZER_ANALYSIS_TYPEEXPANSIONANALYSIS_H

#include "polarphp/ast/TypeExpansionContext.h"
#include "polarphp/pil/lang/Projection.h"
#include "polarphp/pil/lang/PILType.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "llvm/ADT/DenseMap.h"

namespace polar {

/// This analysis determines memory effects during destruction.
class TypeExpansionAnalysis : public PILAnalysis {
   llvm::DenseMap<std::pair<PILType, TypeExpansionContext>, ProjectionPathList>
      ExpansionCache;

public:
   TypeExpansionAnalysis(PILModule *M)
      : PILAnalysis(PILAnalysisKind::TypeExpansion) {}

   static bool classof(const PILAnalysis *S) {
      return S->getKind() == PILAnalysisKind::TypeExpansion;
   }

   /// Return ProjectionPath to every leaf or intermediate node of the given type.
   const ProjectionPathList &getTypeExpansion(PILType B, PILModule *Mod,
                                              TypeExpansionContext context);

   /// Invalidate all information in this analysis.
   virtual void invalidate() override {
      // Nothing can invalidate, because types are static and cannot be changed
      // during the PIL pass pipeline.
   }

   /// Invalidate all of the information for a specific function.
   virtual void invalidate(PILFunction *F, InvalidationKind K)  override { }

   /// Notify the analysis about a newly created function.
   virtual void notifyAddedOrModifiedFunction(PILFunction *F) override {}

   /// Notify the analysis about a function which will be deleted from the
   /// module.
   virtual void notifyWillDeleteFunction(PILFunction *F) override {}

   /// Notify the analysis about changed witness or vtables.
   virtual void invalidateFunctionTables() override { }
};

} // polar

#endif // POLARPHP_PIL_OPTIMIZER_ANALYSIS_TYPEEXPANSIONANALYSIS_H
