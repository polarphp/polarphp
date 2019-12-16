//===--- ConstExpr.h - Constant expression evaluator -----------*- C++ -*-===//
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
// This defines an interface to evaluate Swift language level constant
// expressions.  Its model is intended to be general and reasonably powerful,
// with the goal of standardization in a future version of Swift.
//
// Constant expressions are functions without side effects that take constant
// values and return constant values.  These constants may be integer, and
// floating point values.   We allow abstractions to be built out of fragile
// structs and tuples.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_OPTIMIZER_UTILS_CONSTEXPR_H
#define POLARPHP_PIL_OPTIMIZER_UTILS_CONSTEXPR_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/SourceLoc.h"
#include "polarphp/pil/lang/PILBasicBlock.h"
#include "llvm/ADT/SmallPtrSet.h"

namespace polar {

class AstContext;
class Operand;
class PILFunction;
class PILModule;
class PILNode;
class SymbolicValue;
class SymbolicValueAllocator;
class ConstExprFunctionState;
class UnknownReason;

/// This class is the main entrypoint for evaluating constant expressions.  It
/// also handles caching of previously computed constexpr results.
class ConstExprEvaluator {
   SymbolicValueAllocator &allocator;

   // Assert configuration that must be used by the evaluator. This determines
   // the result of the builtin "assert_configuration".
   unsigned assertConfig;

   /// The current call stack, used for providing accurate diagnostics.
   llvm::SmallVector<SourceLoc, 4> callStack;

   /// When set to true, keep track of all functions called during an evaluation.
   bool trackCallees;
   /// Functions called during the evaluation. This is an auxiliary information
   /// provided to the clients.
   llvm::SmallPtrSet<PILFunction *, 2> calledFunctions;

   void operator=(const ConstExprEvaluator &) = delete;

public:
   explicit ConstExprEvaluator(SymbolicValueAllocator &alloc,
                               unsigned assertConf, bool trackCallees = false);
   ~ConstExprEvaluator();

   explicit ConstExprEvaluator(const ConstExprEvaluator &other);

   SymbolicValueAllocator &getAllocator() { return allocator; }

   unsigned getAssertConfig() { return assertConfig; }

   void pushCallStack(SourceLoc loc) { callStack.push_back(loc); }

   void popCallStack() {
      assert(!callStack.empty());
      callStack.pop_back();
   }

   const llvm::SmallVector<SourceLoc, 4> &getCallStack() { return callStack; }

   // As SymbolicValue::getUnknown(), but handles passing the call stack and
   // allocator.
   SymbolicValue getUnknown(PILNode *node, UnknownReason reason);

   /// Analyze the specified values to determine if they are constant values.
   /// This is done in code that is not necessarily itself a constexpr
   /// function.  The results are added to the results list which is a parallel
   /// structure to the input values.
   void computeConstantValues(ArrayRef<PILValue> values,
                              SmallVectorImpl<SymbolicValue> &results);

   void recordCalledFunctionIfEnabled(PILFunction *callee) {
      if (trackCallees) {
         calledFunctions.insert(callee);
      }
   }

   /// If the evaluator was initialized with \c trackCallees enabled, return the
   /// PIL functions encountered during the evaluations performed with this
   /// evaluator. The returned functions include those that were called but
   /// failed to complete successfully.
   const SmallPtrSetImpl<PILFunction *> &getFuncsCalledDuringEvaluation() const {
      assert(trackCallees && "evaluator not configured to track callees");
      return calledFunctions;
   }
};

/// A constant-expression evaluator that can be used to step through a control
/// flow graph (PILFunction body) by evaluating one instruction at a time.
/// This evaluator can also "skip" instructions without evaluating them and
/// only track constant values of variables whose values could be computed.
class ConstExprStepEvaluator {
private:
   ConstExprEvaluator evaluator;

   ConstExprFunctionState *internalState;

   unsigned stepsEvaluated = 0;

   /// Targets of branches that were visited. This is used to detect loops during
   /// evaluation.
   SmallPtrSet<PILBasicBlock *, 8> visitedBlocks;

   ConstExprStepEvaluator(const ConstExprStepEvaluator &) = delete;
   void operator=(const ConstExprStepEvaluator &) = delete;

public:
   /// Constructs a step evaluator given an allocator and a non-null pointer to a
   /// PILFunction.
   explicit ConstExprStepEvaluator(SymbolicValueAllocator &alloc,
                                   PILFunction *fun, unsigned assertConf,
                                   bool trackCallees = false);
   ~ConstExprStepEvaluator();

   /// Evaluate an instruction in the current interpreter state.
   /// \param instI instruction to be evaluated in the current interpreter state.
   /// \returns a pair where the first and second elements are defined as
   /// follows:
   ///   The first element is the iterator to the next instruction from where
   ///   the evaluation can continue, if the evaluation is successful.
   ///   Otherwise, it is None.
   ///
   ///   Second element is None, if the evaluation is successful.
   ///   Otherwise, is an unknown symbolic value that contains the error.
   std::pair<Optional<PILBasicBlock::iterator>, Optional<SymbolicValue>>
   evaluate(PILBasicBlock::iterator instI);

   /// Skip the instruction without evaluating it and conservatively account for
   /// the effects of the instruction on the internal state. This operation
   /// resets to an unknown symbolic value any portion of a
   /// SymbolicValueMemoryObject that could possibly be mutated by the given
   /// instruction. This function preserves the soundness of the interpretation.
   /// \param instI instruction to be skipped.
   /// \returns a pair where the first and second elements are defined as
   /// follows:
   ///   The first element, if is not None, is the iterator to the next
   ///   instruction from the where the evaluation must continue.
   ///   The first element is None if the next instruction from where the
   ///   evaluation must continue cannot be determined.
   ///   This would be the case if `instI` is a branch like a `condbr`.
   ///
   ///   Second element is None if skipping the instruction is successful.
   ///   Otherwise, it is an unknown symbolic value containing the error.
   std::pair<Optional<PILBasicBlock::iterator>, Optional<SymbolicValue>>
   skipByMakingEffectsNonConstant(PILBasicBlock::iterator instI);

   /// Try evaluating an instruction and if the evaluation fails, skip the
   /// instruction and make it effects non constant. Note that it may not always
   /// be possible to skip an instruction whose evaluation failed and
   /// continue evalution (e.g. a conditional branch).
   /// See `evaluate` and `skipByMakingEffectsNonConstant` functions for their
   /// semantics.
   /// \param instI instruction to be evaluated in the current interpreter state.
   /// \returns a pair where the first and second elements are defined as
   /// follows:
   ///   The first element, if is not None, is the iterator to the next
   ///   instruction from the where the evaluation must continue.
   ///   The first element is None iff both `evaluate` and `skip` functions
   ///   failed to determine the next instruction to continue evaluation from.
   ///
   ///   Second element is None if the evaluation is successful.
   ///   Otherwise, it is an unknown symbolic value containing the error.
   std::pair<Optional<PILBasicBlock::iterator>, Optional<SymbolicValue>>
   tryEvaluateOrElseMakeEffectsNonConstant(PILBasicBlock::iterator instI);

   Optional<SymbolicValue> lookupConstValue(PILValue value);

   /// Return the number of instructions evaluated for the last `evaluate`
   /// operation. This could be used by the clients to limit the number of
   /// instructions that should be evaluated by the step-wise evaluator.
   /// Note that 'skipByMakingEffectsNonConstant' operation is not considered
   /// as an evaluation.
   unsigned instructionsEvaluatedByLastEvaluation() { return stepsEvaluated; }

   /// If the evaluator was initialized with \c trackCallees enabled, return the
   /// PIL functions encountered during the evaluations performed with this
   /// evaluator. The returned functions include those that were called but
   /// failed to complete successfully. Targets of skipped apply instructions
   /// will not be included in the returned set.
   const SmallPtrSetImpl<PILFunction *> &getFuncsCalledDuringEvaluation() {
      return evaluator.getFuncsCalledDuringEvaluation();
   }

   /// Dump the internal state to standard error for debugging.
   void dumpState();
};

bool isConstantEvaluable(PILFunction *fun);

/// Return true if and only if the given function \p fun is specially modeled
/// by the constant evaluator. These are typically functions in the standard
/// library, such as String.+=, Array.append, whose semantics is built into the
/// evaluator.
bool isKnownConstantEvaluableFunction(PILFunction *fun);

/// Return true if and only if \p errorVal denotes an error that requires
/// aborting interpretation and returning the error. Skipping an instruction
/// that produces such errors is not a valid behavior.
bool isFailStopError(SymbolicValue errorVal);

} // end namespace polar

#endif // POLARPHP_PIL_OPTIMIZER_UTILS_CONSTEXPR_H
