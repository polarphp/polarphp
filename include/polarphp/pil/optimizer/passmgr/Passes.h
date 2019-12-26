//===--- Passes.h - Swift Compiler PIL Pass Entrypoints ---------*- C++ -*-===//
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
//  This file declares the main entrypoints to PIL passes.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_OPTIMIZER_PASSMANAGER_PASSES_H
#define POLARPHP_PIL_OPTIMIZER_PASSMANAGER_PASSES_H

#include "polarphp/pil/lang/PILModule.h"

namespace polar {
class PILOptions;
class PILTransform;
class PILModuleTransform;

namespace irgen {
class IRGenModule;

PILTransform *createAllocStackHoisting();
}

/// Run all the PIL diagnostic passes on \p M.
///
/// \returns true if the diagnostic passes produced an error
bool runPILDiagnosticPasses(PILModule &M);

/// Prepare PIL for the -O pipeline.
void runPILOptPreparePasses(PILModule &Module);

/// Run all the PIL performance optimization passes on \p M.
void runPILOptimizationPasses(PILModule &M);

/// Run all PIL passes for -Onone on module \p M.
void runPILPassesForOnone(PILModule &M);

/// Run the PIL ownership eliminator pass on \p M.
bool runPILOwnershipEliminatorPass(PILModule &M);

void runPILOptimizationPassesWithFileSpecification(PILModule &Module,
                                                   StringRef FileName);

/// Detect and remove unreachable code. Diagnose provably unreachable
/// user code.
void performPILDiagnoseUnreachable(PILModule *M);

/// Remove dead functions from \p M.
void performPILDeadFunctionElimination(PILModule *M);

/// Convert PIL to a lowered form suitable for IRGen.
void runPILLoweringPasses(PILModule &M);

/// Perform PIL Inst Count on M if needed.
void performPILInstCountIfNeeded(PILModule *M);

/// Identifiers for all passes. Used to procedurally create passes from
/// lists of passes.
enum class PassKind {
#define PASS(ID, TAG, NAME) ID,
#define PASS_RANGE(ID, START, END) ID##_First = START, ID##_Last = END,
#include "polarphp/pil/optimizer/passmgr/PassesDef.h"
   invalidPassKind
};

PassKind PassKindFromString(StringRef ID);
StringRef PassKindID(PassKind Kind);
StringRef PassKindTag(PassKind Kind);

#define PASS(ID, TAG, NAME) PILTransform *create##ID();
#define IRGEN_PASS(ID, TAG, NAME)
#include "polarphp/pil/optimizer/passmgr/PassesDef.h"

} // end namespace polar

#endif
