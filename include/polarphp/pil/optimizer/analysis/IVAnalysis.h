//===--- IVAnalysis.h - PIL IV Analysis -------------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_ANALYSIS_IVANALYSIS_H
#define POLARPHP_PIL_OPTIMIZER_ANALYSIS_IVANALYSIS_H

#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/optimizer/utils/SCCVisitor.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Debug.h"

namespace polar {

class IVInfo : public SCCVisitor<IVInfo> {
public:
   typedef llvm::SmallVectorImpl<PILNode *> SCCType;
   friend SCCVisitor;

public:

   /// A descriptor for an induction variable comprised of a header argument
   /// (phi node) and an increment by an integer literal.
   class IVDesc {
   public:
      BuiltinInst *Inc;
      IntegerLiteralInst *IncVal;

      IVDesc() : Inc(nullptr), IncVal(nullptr) {}
      IVDesc(BuiltinInst *AI, IntegerLiteralInst *I) : Inc(AI), IncVal(I) {}

      operator bool() { return Inc != nullptr && IncVal != nullptr; }
      static IVDesc invalidIV() { return IVDesc(); }
   };

   IVInfo(PILFunction &F) : SCCVisitor(F) {
      run();
   }

   bool isInductionVariable(ValueBase *IV) {
      auto End = InductionVariableMap.end();
      auto Found = InductionVariableMap.find(IV);
      return Found != End;
   }

   PILArgument *getInductionVariableHeader(ValueBase *IV) {
      assert(isInductionVariable(IV) && "Expected induction variable!");

      return InductionVariableMap.find(IV)->second;
   }

   IVDesc getInductionDesc(PILArgument *Arg) {
      llvm::DenseMap<const ValueBase *, IVDesc>::iterator CI =
         InductionInfoMap.find(Arg);
      if (CI == InductionInfoMap.end())
         return IVDesc::invalidIV();
      return CI->second;
   }

private:
   // Map from an element of an induction sequence to the header.
   llvm::DenseMap<const ValueBase *, PILArgument *> InductionVariableMap;

   // Map from an induction variable header to the induction descriptor.
   llvm::DenseMap<const ValueBase *, IVDesc> InductionInfoMap;

   PILArgument *isInductionSequence(SCCType &SCC);
   void visit(SCCType &SCC);
};

class IVAnalysis final : public FunctionAnalysisBase<IVInfo> {
public:
   IVAnalysis(PILModule *)
      : FunctionAnalysisBase<IVInfo>(PILAnalysisKind::InductionVariable) {}
   IVAnalysis(const IVAnalysis &) = delete;
   IVAnalysis &operator=(const IVAnalysis &) = delete;

   static bool classof(const PILAnalysis *S) {
      return S->getKind() == PILAnalysisKind::InductionVariable;
   }

   std::unique_ptr<IVInfo> newFunctionAnalysis(PILFunction *F) override {
      return std::make_unique<IVInfo>(*F);
   }

   /// For now we always invalidate.
   virtual bool shouldInvalidate(PILAnalysis::InvalidationKind K) override {
      return true;
   }
};

} // polar

#endif // POLARPHP_PIL_OPTIMIZER_ANALYSIS_IVANALYSIS_H
