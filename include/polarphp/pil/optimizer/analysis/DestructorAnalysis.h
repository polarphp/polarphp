//===--- DestructorAnalysis.h -----------------------------------*- C++ -*-===//
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
#ifndef POLARPHP_PIL_OPTIMIZER_ANALYSIS_DESTRUCTORANALYSIS_H
#define POLARPHP_PIL_OPTIMIZER_ANALYSIS_DESTRUCTORANALYSIS_H

#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "llvm/ADT/DenseMap.h"

namespace polar {

/// This analysis determines memory effects during destruction.
class DestructorAnalysis : public PILAnalysis {
   PILModule *Mod;
   llvm::DenseMap<CanType, bool> Cached;
public:
   DestructorAnalysis(PILModule *M)
      : PILAnalysis(PILAnalysisKind::Destructor), Mod(M) {}

   static bool classof(const PILAnalysis *S) {
      return S->getKind() == PILAnalysisKind::Destructor;
   }

   /// Returns true if destruction of T may store to memory.
   bool mayStoreToMemoryOnDestruction(PILType T);

   /// No invalidation is needed.
   virtual void invalidate() override {
      // Nothing can invalidate, because types are static and cannot be changed
      // during the PIL pass pipeline.
   }

   /// No invalidation is needed.
   virtual void invalidate(PILFunction *F, InvalidationKind K)  override {
      // Nothing can invalidate, because types are static and cannot be changed
      // during the PIL pass pipeline.
   }

   /// Notify the analysis about a newly created function.
   virtual void notifyAddedOrModifiedFunction(PILFunction *F) override {}

   /// Notify the analysis about a function which will be deleted from the
   /// module.
   virtual void notifyWillDeleteFunction(PILFunction *F) override {}

   /// Notify the analysis about changed witness or vtables.
   virtual void invalidateFunctionTables() override { }

protected:
   bool cacheResult(CanType Type, bool Result);
   bool isSafeType(CanType Ty);
   bool implementsDestructorSafeContainerInterface(NominalTypeDecl *NomDecl);
   bool areTypeParametersSafe(CanType Ty);
   AstContext &getAstContext();
};

} // polar

#endif // POLARPHP_PIL_OPTIMIZER_ANALYSIS_DESTRUCTORANALYSIS_H
