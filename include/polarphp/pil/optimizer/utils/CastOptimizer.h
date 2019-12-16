//===--- CastOptimizer.h ----------------------------------*- C++ -*-------===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_UTILS_CASTOPTIMIZER_H
#define POLARPHP_PIL_OPTIMIZER_UTILS_CASTOPTIMIZER_H

#include "polarphp/basic/ArrayRefView.h"
#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/pil/lang/PILCloner.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/optimizer/analysis/ARCAnalysis.h"
#include "polarphp/pil/optimizer/analysis/ClassHierarchyAnalysis.h"
#include "polarphp/pil/optimizer/analysis/EpilogueARCAnalysis.h"
#include "polarphp/pil/optimizer/analysis/SimplifyInstruction.h"
#include "polarphp/pil/optimizer/utils/PILOptFunctionBuilder.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/Allocator.h"
#include <functional>
#include <utility>

namespace polar {

class PILOptFunctionBuilder;
struct PILDynamicCastInst;

/// This is a helper class used to optimize casts.
class CastOptimizer {
   PILOptFunctionBuilder &functionBuilder;

   /// Temporary context for clients that do not provide their own.
   PILBuilderContext tempBuilderContext;

   /// Reference to the provided PILBuilderContext.
   PILBuilderContext &builderContext;

   /// Callback that replaces the first PILValue's uses with a use of the second
   /// value.
   std::function<void(PILValue, PILValue)> replaceValueUsesAction;

   /// Callback that replaces a SingleValueInstruction with a ValueBase after
   /// updating any status in the caller.
   std::function<void(SingleValueInstruction *, ValueBase *)>
      replaceInstUsesAction;

   /// Callback that erases an instruction and performs any state updates in the
   /// caller required.
   std::function<void(PILInstruction *)> eraseInstAction;

   /// Callback to call after an optimization was performed based on the fact
   /// that a cast will succeed.
   std::function<void()> willSucceedAction;

   /// Callback to call after an optimization was performed based on the fact
   /// that a cast will fail.
   std::function<void()> willFailAction;

public:
   CastOptimizer(PILOptFunctionBuilder &FunctionBuilder,
                 PILBuilderContext *BuilderContext,
                 std::function<void(PILValue, PILValue)> ReplaceValueUsesAction,
                 std::function<void(SingleValueInstruction *, ValueBase *)>
                 ReplaceInstUsesAction,
                 std::function<void(PILInstruction *)> EraseAction,
                 std::function<void()> WillSucceedAction,
                 std::function<void()> WillFailAction = []() {})
      : functionBuilder(FunctionBuilder),
        tempBuilderContext(FunctionBuilder.getModule()),
        builderContext(BuilderContext ? *BuilderContext : tempBuilderContext),
        replaceValueUsesAction(ReplaceValueUsesAction),
        replaceInstUsesAction(ReplaceInstUsesAction),
        eraseInstAction(EraseAction), willSucceedAction(WillSucceedAction),
        willFailAction(WillFailAction) {}

   // This constructor is used in
   // 'PILOptimizer/Mandatory/ConstantPropagation.cpp'. MSVC2015 compiler
   // couldn't use the single constructor version which has three default
   // arguments. It seems the number of the default argument with lambda is
   // limited.
   CastOptimizer(PILOptFunctionBuilder &FunctionBuilder,
                 PILBuilderContext *BuilderContext,
                 std::function<void(PILValue, PILValue)> ReplaceValueUsesAction,
                 std::function<void(SingleValueInstruction *I, ValueBase *V)>
                 ReplaceInstUsesAction,
                 std::function<void(PILInstruction *)> EraseAction =
                 [](PILInstruction *) {})
      : CastOptimizer(FunctionBuilder, BuilderContext, ReplaceValueUsesAction,
                      ReplaceInstUsesAction, EraseAction, []() {}, []() {}) {}

   /// Simplify checked_cast_br. It may change the control flow.
   PILInstruction *simplifyCheckedCastBranchInst(CheckedCastBranchInst *Inst);

   /// Simplify checked_cast_value_br. It may change the control flow.
   PILInstruction *
   simplifyCheckedCastValueBranchInst(CheckedCastValueBranchInst *Inst);

   /// Simplify checked_cast_addr_br. It may change the control flow.
   PILInstruction *
   simplifyCheckedCastAddrBranchInst(CheckedCastAddrBranchInst *Inst);

   /// Optimize checked_cast_br. This cannot change the control flow.
   PILInstruction *optimizeCheckedCastBranchInst(CheckedCastBranchInst *Inst);

   /// Optimize checked_cast_value_br. This cannot change the control flow.
   PILInstruction *
   optimizeCheckedCastValueBranchInst(CheckedCastValueBranchInst *Inst);

   /// Optimize checked_cast_addr_br. This cannot change the control flow.
   PILInstruction *
   optimizeCheckedCastAddrBranchInst(CheckedCastAddrBranchInst *Inst);

   /// Optimize unconditional_checked_cast. This cannot change the control flow.
   ValueBase *
   optimizeUnconditionalCheckedCastInst(UnconditionalCheckedCastInst *Inst);

   /// Optimize unconditional_checked_cast_addr. This cannot change the control
   /// flow.
   PILInstruction *optimizeUnconditionalCheckedCastAddrInst(
      UnconditionalCheckedCastAddrInst *Inst);

   /// Check if it is a bridged cast and optimize it.
   ///
   /// May change the control flow.
   PILInstruction *optimizeBridgedCasts(PILDynamicCastInst cast);

   /// Optimize a cast from a bridged ObjC type into
   /// a corresponding Swift type implementing _ObjectiveCBridgeable.
   PILInstruction *
   optimizeBridgedObjCToSwiftCast(PILDynamicCastInst dynamicCast);

   /// Optimize a cast from a Swift type implementing _ObjectiveCBridgeable
   /// into a bridged ObjC type.
   PILInstruction *
   optimizeBridgedSwiftToObjCCast(PILDynamicCastInst dynamicCast);

   void deleteInstructionsAfterUnreachable(PILInstruction *UnreachableInst,
                                           PILInstruction *TrapInst);

   PILValue optimizeMetatypeConversion(ConversionInst *mci,
                                       MetatypeRepresentation representation);
};

} // namespace polar

#endif // POLARPHP_PIL_OPTIMIZER_UTILS_CASTOPTIMIZER_H
