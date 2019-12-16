//===--- Condition.h - Defines the PILGen Condition class -------*- C++ -*-===//
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
// This file defines the Condition class, used by PIL Generation.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_GEN_CONDITION_H
#define POLARPHP_PIL_GEN_CONDITION_H

#include "llvm/ADT/ArrayRef.h"
#include "polarphp/pil/lang/PILLocation.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/gen/PILGenFunction.h"
#include "polarphp/pil/gen/Scope.h"
#include "llvm/Support/Compiler.h"

namespace polar {

class PatternBindingDecl;
class PILBasicBlock;

namespace lowering {

/// A condition is the result of evaluating a boolean expression as
/// control flow.
///
/// For each Condition instance, `enterTrue` must be called before `complete`.
/// If `enterFalse` is skipped, then an empty fall-through block is created.
class LLVM_LIBRARY_VISIBILITY Condition {
   /// The blocks responsible for executing the true and false conditions. These
   /// are initialized non-null and set to null after being emitted.
   PILBasicBlock *TrueBB;
   PILBasicBlock *FalseBB;

   /// The continuation block if both branches are possible.
   PILBasicBlock *ContBB;

   /// The location wrapping the originator conditional expression.
   PILLocation Loc;

public:
   Condition(PILBasicBlock *TrueBB, PILBasicBlock *FalseBB,
             PILBasicBlock *ContBB,
             PILLocation L)
      : TrueBB(TrueBB), FalseBB(FalseBB), ContBB(ContBB), Loc(L)
   {
      assert((TrueBB != nullptr && FalseBB != nullptr) &&
             "Requires non-null block pointers.");
   }

   /// enterTrue - Begin the emission of the true block.
   void enterTrue(PILGenFunction &SGF) { enter(SGF, TrueBB); }

   /// exitTrue - End the emission of the true block.
   void exitTrue(PILGenFunction &SGF, ArrayRef<PILValue> Args = {}) {
      exit(SGF, TrueBB, Args);
      TrueBB = nullptr;
   }

   /// enterFalse - Begin the emission of the false block.
   void enterFalse(PILGenFunction &SGF) { enter(SGF, FalseBB); }

   /// exitFalse - End the emission of the true block.
   void exitFalse(PILGenFunction &SGF, ArrayRef<PILValue> Args = {}) {
      exit(SGF, FalseBB, Args);
      FalseBB = nullptr;
   }

   /// complete - Complete this conditional execution.  This should be called
   /// only after all other calls on this Condition have been made.
   /// This leaves SGF's PILGenBuilder at the continuation block.
   PILBasicBlock *complete(PILGenFunction &SGF);

protected:
   void enter(PILGenFunction &SGF, PILBasicBlock *destBB);

   void exit(PILGenFunction &SGF, PILBasicBlock *destBB,
             ArrayRef<PILValue> Args = {});
};

/// A conditional value is one that depends on conditional execution.
/// Depending on whether a type is address-only, it may be representable using
/// BB argument passing or by storing to a common result buffer.
class ConditionalValue {
   PILGenFunction &SGF;
   const TypeLowering &tl;

   /// The continuation block that receives the conditional value.
   PILBasicBlock *contBB;

   /// The location associated with the value.
   PILLocation loc;

   /// The buffer to receive an address-only result, or the BB argument that
   /// a loadable result is passed to.
   PILValue result;

   /// The Scope for the current branch.
   Optional<Scope> scope;

   /// A place to hold conditional Initializations of our result.
   std::unique_ptr<Initialization> currentInitialization;

public:
   /// Begins a conditional computation of the type represented by the given
   /// type lowering. This potentially emits a temporary allocation for the
   /// result, so it must be called with the insertion point valid and dominating
   /// any branches that will be involved in the computation.
   ConditionalValue(PILGenFunction &SGF, SGFContext C, PILLocation loc,
                    const TypeLowering &valueTL);

   /// Enter a branch of the conditional value computation. Expression evaluation
   /// within this branch may use the returned SGFContext to potentially find a
   /// buffer to emit into. If a basic block is given, the insertion point must
   /// be invalid, and on return, the given basic block will be emitted, and the
   /// insertion point will be inside it. If the basic block is null, then
   /// codegen proceeds in the current basic block.
   SGFContext enterBranch(PILBasicBlock *bb = nullptr);

   /// Exit a branch of the conditional value computation, using the given value
   /// as the result of the computation on this branch. Branches to the
   /// continuation block for the conditional value. On return, the insertion
   /// point will be invalid.
   void exitBranch(RValue &&result);

   /// Complete the conditional computation. The insertion point must be invalid.
   /// On return, the continuation block for the conditional will be emitted, and
   /// the insertion point will be inside it. The result of the conditional
   /// computation will be returned.
   ManagedValue complete();
};

} // end namespace lowering
} // end namespace polar

#endif // POLARPHP_PIL_GEN_CONDITION_H
