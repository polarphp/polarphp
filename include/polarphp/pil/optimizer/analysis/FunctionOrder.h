//===--- FunctionOrder.h - Utilities for function ordering  -----*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_ANALYSIS_FUNCTIONORDER_H
#define POLARPHP_PIL_OPTIMIZER_ANALYSIS_FUNCTIONORDER_H

#include "polarphp/pil/optimizer/analysis/BasicCalleeAnalysis.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/TinyPtrVector.h"

namespace polar {

class BasicCalleeAnalysis;
class PILFunction;
class PILModule;

class BottomUpFunctionOrder {
public:
   typedef TinyPtrVector<PILFunction *> SCC;

private:
   PILModule &M;
   llvm::SmallVector<SCC, 32> TheSCCs;
   llvm::SmallVector<PILFunction *, 32> TheFunctions;

   // The callee analysis we use to determine the callees at each call site.
   BasicCalleeAnalysis *BCA;

   unsigned NextDFSNum;
   llvm::DenseMap<PILFunction *, unsigned> DFSNum;
   llvm::DenseMap<PILFunction *, unsigned> MinDFSNum;
   llvm::SmallSetVector<PILFunction *, 4> DFSStack;

public:
   BottomUpFunctionOrder(PILModule &M, BasicCalleeAnalysis *BCA)
      : M(M), BCA(BCA), NextDFSNum(0) {}

   /// Get the SCCs in bottom-up order.
   ArrayRef<SCC> getSCCs() {
      if (!TheSCCs.empty())
         return TheSCCs;

      FindSCCs(M);
      return TheSCCs;
   }

   /// Get a flattened view of all functions in all the SCCs in
   /// bottom-up order
   ArrayRef<PILFunction *> getFunctions() {
      if (!TheFunctions.empty())
         return TheFunctions;

      for (auto SCC : getSCCs())
         for (auto *F : SCC)
            TheFunctions.push_back(F);

      return TheFunctions;
   }

private:
   void DFS(PILFunction *F);
   void FindSCCs(PILModule &M);
};

} // end namespace polar

#endif
