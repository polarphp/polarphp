//===--- RCIdentityAnalysis.h -----------------------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_ANALYSIS_RCIDENTITYANALYSIS_H
#define POLARPHP_PIL_OPTIMIZER_ANALYSIS_RCIDENTITYANALYSIS_H

#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "polarphp/pil/optimizer/analysis/DominanceAnalysis.h"
#include "polarphp/pil/optimizer/passmgr/PassManager.h"

namespace polar {

/// Limit the size of the rc identity cache. We keep a cache per function.
constexpr unsigned MaxRCIdentityCacheSize = 64;

class DominanceAnalysis;

/// This class is a simple wrapper around an identity cache.
class RCIdentityFunctionInfo {
   llvm::DenseSet<PILArgument *> VisitedArgs;
   // RC identity cache.
   llvm::DenseMap<PILValue, PILValue> RCCache;
   DominanceAnalysis *DA;

   /// This number is arbitrary and conservative. At some point if compile time
   /// is not an issue, this value should be made more aggressive (i.e. greater).
   enum { MaxRecursionDepth = 16 };

public:
   RCIdentityFunctionInfo(DominanceAnalysis *D) : VisitedArgs(),
                                                  DA(D) {}

   PILValue getRCIdentityRoot(PILValue V);

   /// Return all recursive users of V, looking through users which propagate
   /// RCIdentity.
   ///
   /// *NOTE* This ignores obvious ARC escapes where the a potential
   /// user of the RC is not managed by ARC. For instance
   /// unchecked_trivial_bit_cast.
   void getRCUses(PILValue V, llvm::SmallVectorImpl<Operand *> &Uses);

   /// A helper method that calls getRCUses and then maps each operand to the
   /// operands user and then uniques the list.
   ///
   /// *NOTE* The routine asserts that the passed in Users array is empty for
   /// simplicity. If needed this can be changed, but it is not necessary given
   /// current uses.
   void getRCUsers(PILValue V, llvm::SmallVectorImpl<PILInstruction *> &Users);

   void handleDeleteNotification(PILNode *node) {
      auto value = dyn_cast<ValueBase>(node);
      if (!value)
         return;

      // Check the cache. If we don't find it, there is nothing to do.
      auto Iter = RCCache.find(PILValue(value));
      if (Iter == RCCache.end())
         return;

      // Then erase Iter from the cache.
      RCCache.erase(Iter);
   }

private:
   PILValue getRCIdentityRootInner(PILValue V, unsigned RecursionDepth);
   PILValue stripRCIdentityPreservingOps(PILValue V, unsigned RecursionDepth);
   PILValue stripRCIdentityPreservingArgs(PILValue V, unsigned RecursionDepth);
   PILValue stripOneRCIdentityIncomingValue(PILArgument *Arg, PILValue V);
   bool findDominatingNonPayloadedEdge(PILBasicBlock *IncomingEdgeBB,
                                       PILValue RCIdentity);
};

class RCIdentityAnalysis : public FunctionAnalysisBase<RCIdentityFunctionInfo> {
   DominanceAnalysis *DA;

public:
   RCIdentityAnalysis(PILModule *)
      : FunctionAnalysisBase<RCIdentityFunctionInfo>(
      PILAnalysisKind::RCIdentity),
        DA(nullptr) {}

   RCIdentityAnalysis(const RCIdentityAnalysis &) = delete;
   RCIdentityAnalysis &operator=(const RCIdentityAnalysis &) = delete;

   virtual void handleDeleteNotification(PILNode *node) override {
      // If the parent function of this instruction was just turned into an
      // external declaration, bail. This happens during PILFunction destruction.
      PILFunction *F = node->getFunction();
      if (F->isExternalDeclaration()) {
         return;
      }
      get(F)->handleDeleteNotification(node);
   }

   virtual bool needsNotifications() override { return true; }

   static bool classof(const PILAnalysis *S) {
      return S->getKind() == PILAnalysisKind::RCIdentity;
   }

   virtual void initialize(PILPassManager *PM) override;

   virtual std::unique_ptr<RCIdentityFunctionInfo>
   newFunctionAnalysis(PILFunction *F) override {
      return std::make_unique<RCIdentityFunctionInfo>(DA);
   }

   virtual bool shouldInvalidate(PILAnalysis::InvalidationKind K) override {
      return true;
   }

};

} // end polar namespace

#endif // POLARPHP_PIL_OPTIMIZER_ANALYSIS_RCIDENTITYANALYSIS_H
