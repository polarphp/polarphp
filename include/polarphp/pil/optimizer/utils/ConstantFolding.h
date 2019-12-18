//===--- ConstantFolding.h - Utilities for PIL constant folding -*- C++ -*-===//
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
// This file defines utility functions for constant folding.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_OPTIMIZER_UTILS_CONSTANTFOLDING_H
#define POLARPHP_PIL_OPTIMIZER_UTILS_CONSTANTFOLDING_H

#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "llvm/ADT/SetVector.h"
#include <functional>

namespace polar {

class PILOptFunctionBuilder;

/// Evaluates the constant result of a binary bit-operation.
///
/// The \p ID must be the ID of a binary bit-operation builtin.
APInt constantFoldBitOperation(APInt lhs, APInt rhs, BuiltinValueKind ID);

/// Evaluates the constant result of an integer comparison.
///
/// The \p ID must be the ID of an integer builtin operation.
APInt constantFoldComparison(APInt lhs, APInt rhs, BuiltinValueKind ID);

/// Evaluates the constant result of a binary operation with overflow.
///
/// The \p ID must be the ID of a binary operation with overflow.
APInt constantFoldBinaryWithOverflow(APInt lhs, APInt rhs, bool &Overflow,
                                     llvm::Intrinsic::ID ID);

/// Evaluates the constant result of a division operation.
///
/// The \p ID must be the ID of a division operation.
APInt constantFoldDiv(APInt lhs, APInt rhs, bool &Overflow, BuiltinValueKind ID);

/// Evaluates the constant result of an integer cast operation.
///
/// The \p ID must be the ID of a trunc/sext/zext builtin.
APInt constantFoldCast(APInt val, const BuiltinInfo &BI);


/// A utility class to do constant folding.
class ConstantFolder {
private:
   PILOptFunctionBuilder &FuncBuilder;

   /// The worklist of the constants that could be folded into their users.
   llvm::SetVector<PILInstruction *> WorkList;

   /// The assert configuration of PILOptions.
   unsigned AssertConfiguration;

   /// Print diagnostics as part of mandatory constant propagation.
   bool EnableDiagnostics;

   /// Called for each constant folded instruction.
   std::function<void (PILInstruction *)> Callback;

   bool constantFoldStringConcatenation(ApplyInst *AI);

public:
   /// The constructor.
   ///
   /// \param AssertConfiguration The assert configuration of PILOptions.
   /// \param EnableDiagnostics Print diagnostics as part of mandatory constant
   ///                          propagation.
   /// \param Callback Called for each constant folded instruction.
   ConstantFolder(PILOptFunctionBuilder &FuncBuilder,
                  unsigned AssertConfiguration,
                  bool EnableDiagnostics = false,
                  std::function<void (PILInstruction *)> Callback =
                  [](PILInstruction *){}) :
      FuncBuilder(FuncBuilder),
      AssertConfiguration(AssertConfiguration),
      EnableDiagnostics(EnableDiagnostics),
      Callback(Callback) { }

   /// Initialize the worklist with all instructions of the function \p F.
   void initializeWorklist(PILFunction &F);

   /// When asserts are enabled, dumps the worklist for diagnostic
   /// purposes. Without asserts this is a no-op.
   void dumpWorklist() const;

   /// Initialize the worklist with a single instruction \p I.
   void addToWorklist(PILInstruction *I) {
      WorkList.insert(I);
   }

   /// Constant fold everything in the worklist and transitively all uses of
   /// folded instructions.
   PILAnalysis::InvalidationKind processWorkList();
};

} // end namespace polar

#endif // POLARPHP_PIL_OPTIMIZER_UTILS_CONSTANTFOLDING_H
