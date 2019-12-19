//===-----ConstantEvaluatorTester.cpp - Test Constant Evaluator -----------===//
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

#define DEBUG_TYPE "pil-constant-evaluation-tester"

#include "polarphp/ast/DiagnosticsPIL.h"
#include "polarphp/pil/lang/PILConstants.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/ConstExpr.h"

using namespace polar;

namespace {

template <typename... T, typename... U>
static InFlightDiagnostic diagnose(AstContext &Context, SourceLoc loc,
                                   Diag<T...> diag, U &&... args) {
   return Context.Diags.diagnose(loc, diag, std::forward<U>(args)...);
}

/// A compiler pass for testing constant evaluator in the step-wise evaluation
/// mode. The pass evaluates PIL functions whose names start with "interpret"
/// and outputs the constant value returned by the function or diagnostics if
/// the evaluation fails.
class ConstantEvaluatorTester : public PILFunctionTransform {

   bool shouldInterpret() {
      auto *fun = getFunction();
      return fun->getName().startswith("interpret");
   }

   bool shouldSkipInstruction(PILInstruction *inst) {
      auto *applyInst = dyn_cast<ApplyInst>(inst);
      if (!applyInst)
         return false;

      auto *callee = applyInst->getReferencedFunctionOrNull();
      if (!callee)
         return false;

      return callee->getName().startswith("skip");
   }

   void run() override {
      PILFunction *fun = getFunction();

      if (!shouldInterpret() || fun->empty())
         return;

      llvm::errs() << "@" << fun->getName() << "\n";

      SymbolicValueBumpAllocator allocator;
      ConstExprStepEvaluator stepEvaluator(allocator, fun,
                                           getOptions().AssertConfig);

      for (auto currI = fun->getEntryBlock()->begin();;) {
         auto *inst = &(*currI);

         if (auto *returnInst = dyn_cast<ReturnInst>(inst)) {
            auto returnVal =
               stepEvaluator.lookupConstValue(returnInst->getOperand());

            if (!returnVal) {
               llvm::errs() << "Returns unknown"
                            << "\n";
               break;
            }
            llvm::errs() << "Returns " << returnVal.getValue() << "\n";
            break;
         }

         Optional<PILBasicBlock::iterator> nextInstOpt;
         Optional<SymbolicValue> errorVal;

         // If the instruction is marked as skip, skip it and make its effects
         // non-constant. Otherwise, try evaluating the instruction and if the
         // evaluation fails due to a previously skipped instruction,
         // skip the current instruction.
         if (shouldSkipInstruction(inst)) {
            std::tie(nextInstOpt, errorVal) =
               stepEvaluator.skipByMakingEffectsNonConstant(currI);
         } else {
            std::tie(nextInstOpt, errorVal) =
               stepEvaluator.tryEvaluateOrElseMakeEffectsNonConstant(currI);
         }

         // Diagnose errors in the evaluation. Unknown symbolic values produced
         // by skipping instructions are not considered errors.
         if (errorVal.hasValue() &&
             !errorVal->isUnknownDueToUnevaluatedInstructions()) {
            errorVal->emitUnknownDiagnosticNotes(inst->getLoc());
            break;
         }

         if (!nextInstOpt) {
            diagnose(fun->getAstContext(), inst->getLoc().getSourceLoc(),
                     diag::constexpr_unknown_control_flow_due_to_skip);
            errorVal->emitUnknownDiagnosticNotes(inst->getLoc());
            break;
         }

         currI = nextInstOpt.getValue();
      }
   }
};

} // end anonymous namespace

PILTransform *polar::createConstantEvaluatorTester() {
   return new ConstantEvaluatorTester();
}
