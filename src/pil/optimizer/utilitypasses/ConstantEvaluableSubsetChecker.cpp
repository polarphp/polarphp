//===--ConstantEvaluableSubsetChecker.cpp - Test Constant Evaluable Swift--===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

// This file implements a pass for checking the constant evaluability of Swift
// code snippets. This pass is only used in tests and is not part of the
// compilation pipeline.

#define DEBUG_TYPE "pil-constant-evaluable-subset-checker"

#include "polarphp/ast/DiagnosticsPIL.h"
#include "polarphp/ast/Module.h"
#include "polarphp/demangling/Demangle.h"
#include "polarphp/pil/lang/PILConstants.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/ConstExpr.h"

using namespace polar;

namespace {

static const StringRef testDriverSemanticsAttr = "test_driver";

template <typename... T, typename... U>
static InFlightDiagnostic diagnose(AstContext &Context, SourceLoc loc,
                                   Diag<T...> diag, U &&... args) {
   return Context.Diags.diagnose(loc, diag, std::forward<U>(args)...);
}

static std::string demangleSymbolName(StringRef name) {
   demangling::DemangleOptions options;
   options.QualifyEntities = false;
   return demangling::demangleSymbolAsString(name, options);
}

/// A PILModule pass that invokes the constant evaluator on all functions in a
/// PILModule with the semantics attribute "test_driver". Each "test_driver"
/// must invoke one or more functions in the module annotated as
/// "constant_evaluable" with constant arguments.
class ConstantEvaluableSubsetChecker : public PILModuleTransform {

   llvm::SmallPtrSet<PILFunction *, 4> constantEvaluableFunctions;
   llvm::SmallPtrSet<PILFunction *, 4> evaluatedFunctions;

   /// Evaluate the body of \c fun with the constant evaluator. \c fun must be
   /// annotated as "test_driver" and must invoke one or more functions annotated
   /// as "constant_evaluable" with constant arguments. Emit diagnostics if the
   /// evaluation of any  "constant_evaluable" function called in the body of
   /// \c fun fails.
   void constantEvaluateDriver(PILFunction *fun) {
      AstContext &astContext = fun->getAstContext();

      // Create a step evaluator and run it on the function.
      SymbolicValueBumpAllocator allocator;
      ConstExprStepEvaluator stepEvaluator(allocator, fun,
                                           getOptions().AssertConfig,
         /*trackCallees*/ true);
      bool previousEvaluationHadFatalError = false;

      for (auto currI = fun->getEntryBlock()->begin();;) {
         auto *inst = &(*currI);

         if (isa<ReturnInst>(inst))
            break;

         auto *applyInst = dyn_cast<ApplyInst>(inst);
         PILFunction *callee = nullptr;
         if (applyInst) {
            callee = applyInst->getReferencedFunctionOrNull();
         }

         Optional<PILBasicBlock::iterator> nextInstOpt;
         Optional<SymbolicValue> errorVal;

         if (!applyInst || !callee || !isConstantEvaluable(callee)) {

            // Ignore these instructions if we had a fatal error already.
            if (previousEvaluationHadFatalError) {
               if (isa<TermInst>(inst)) {
                  assert(false && "non-constant control flow in the test driver");
               }
               ++currI;
               continue;
            }

            std::tie(nextInstOpt, errorVal) =
               stepEvaluator.tryEvaluateOrElseMakeEffectsNonConstant(currI);
            if (!nextInstOpt) {
               // This indicates an error in the test driver.
               errorVal->emitUnknownDiagnosticNotes(inst->getLoc());
               assert(false && "non-constant control flow in the test driver");
            }
            currI = nextInstOpt.getValue();
            continue;
         }

         assert(!previousEvaluationHadFatalError &&
                "cannot continue evaluation of test driver as previous call "
                "resulted in non-skippable evaluation error.");

         // Here, a function annotated as "constant_evaluable" is called.
         llvm::errs() << "@" << demangleSymbolName(callee->getName()) << "\n";
         std::tie(nextInstOpt, errorVal) =
            stepEvaluator.tryEvaluateOrElseMakeEffectsNonConstant(currI);

         if (errorVal) {
            SourceLoc instLoc = inst->getLoc().getSourceLoc();
            diagnose(astContext, instLoc, diag::not_constant_evaluable);
            errorVal->emitUnknownDiagnosticNotes(inst->getLoc());
         }

         if (nextInstOpt) {
            currI = nextInstOpt.getValue();
            continue;
         }

         // Here, a non-skippable error like "instruction-limit exceeded" has been
         // encountered during evaluation. Proceed to the next instruction but
         // ensure that an assertion failure occurs if there is any instruction
         // to evaluate after this instruction.
         ++currI;
         previousEvaluationHadFatalError = true;
      }

      // For every function seen during the evaluation of this constant evaluable
      // function:
      // 1. Record it so as to detect whether the test drivers in the PILModule
      // cover all function annotated as "constant_evaluable".
      //
      // 2. If the callee is annotated as constant_evaluable and is imported from
      // a different Swift module (other than stdlib), check that the function is
      // marked as Onone. Otherwise, it could have been optimized, which will
      // break constant evaluability.
      for (PILFunction *callee : stepEvaluator.getFuncsCalledDuringEvaluation()) {
         evaluatedFunctions.insert(callee);

         PILModule &calleeModule = callee->getModule();
         if (callee->isAvailableExternally() && isConstantEvaluable(callee) &&
             callee->getOptimizationMode() != OptimizationMode::NoOptimization) {
            diagnose(calleeModule.getAstContext(),
                     callee->getLocation().getSourceLoc(),
                     diag::constexpr_imported_func_not_onone,
                     demangleSymbolName(callee->getName()));
         }
      }
   }

   void run() override {
      PILModule *module = getModule();
      assert(module);

      for (PILFunction &fun : *module) {
         // Record functions annotated as constant evaluable.
         if (isConstantEvaluable(&fun)) {
            constantEvaluableFunctions.insert(&fun);
            continue;
         }

         // Evaluate test drivers.
         if (!fun.hasSemanticsAttr(testDriverSemanticsAttr))
            continue;
         constantEvaluateDriver(&fun);
      }

      // Assert that every function annotated as "constant_evaluable" was convered
      // by a test driver.
      bool error = false;
      for (PILFunction *constEvalFun : constantEvaluableFunctions) {
         if (!evaluatedFunctions.count(constEvalFun)) {
            llvm::errs() << "Error: function "
                         << demangleSymbolName(constEvalFun->getName());
            llvm::errs() << " annotated as constant evaluable";
            llvm::errs() << " does not have a test driver"
                         << "\n";
            error = true;
         }
      }
      assert(!error);
   }
};

} // end anonymous namespace

PILTransform *polar::createConstantEvaluableSubsetChecker() {
   return new ConstantEvaluableSubsetChecker();
}
