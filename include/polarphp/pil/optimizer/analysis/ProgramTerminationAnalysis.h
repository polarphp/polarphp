//===--- ProgramTerminationAnalysis.h ---------------------------*- C++ -*-===//
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
///
/// \file
///
/// This is an analysis which determines if a block is a "program terminating
/// block". Define a program terminating block is defined as follows:
///
/// 1. A block at whose end point according to the PIL model, the program must
/// end. An example of such a block is one that includes a call to fatalError.
/// 2. Any block that is joint post-dominated by program terminating blocks.
///
/// For now we only identify instances of 1. But the analysis could be extended
/// appropriately via simple dataflow or through the use of post-dominator
/// trees.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_OPTIMIZER_ANALYSIS_PROGRAMTERMINATIONANALYSIS_H
#define POLARPHP_PIL_OPTIMIZER_ANALYSIS_PROGRAMTERMINATIONANALYSIS_H

#include "polarphp/pil/optimizer/analysis/ARCAnalysis.h"
#include "llvm/ADT/SmallPtrSet.h"

namespace polar {

class ProgramTerminationFunctionInfo {
   llvm::SmallPtrSet<const PILBasicBlock *, 4> ProgramTerminatingBlocks;

public:
   ProgramTerminationFunctionInfo(const PILFunction *F) {
      for (const auto &BB : *F) {
         if (!isARCInertTrapBB(&BB))
            continue;
         ProgramTerminatingBlocks.insert(&BB);
      }
   }

   bool isProgramTerminatingBlock(const PILBasicBlock *BB) const {
      return ProgramTerminatingBlocks.count(BB);
   }
};

} // end polar namespace

#endif // POLARPHP_PIL_OPTIMIZER_ANALYSIS_PROGRAMTERMINATIONANALYSIS_HProtocolConformanceAnalysis.h
