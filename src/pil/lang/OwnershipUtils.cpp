//===--- OwnershipUtils.cpp -----------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "polarphp/pil/lang/OwnershipUtils.h"
#include "polarphp/basic/Defer.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILInstruction.h"

using namespace polar;

bool polar::isValueAddressOrTrivial(PILValue v) {
   return v->getType().isAddress() ||
          v.getOwnershipKind() == ValueOwnershipKind::None;
}

// These operations forward both owned and guaranteed ownership.
bool polar::isOwnershipForwardingValueKind(PILNodeKind kind) {
   switch (kind) {
      case PILNodeKind::TupleInst:
      case PILNodeKind::StructInst:
      case PILNodeKind::EnumInst:
      case PILNodeKind::OpenExistentialRefInst:
      case PILNodeKind::UpcastInst:
      case PILNodeKind::UncheckedRefCastInst:
      case PILNodeKind::ConvertFunctionInst:
      case PILNodeKind::RefToBridgeObjectInst:
      case PILNodeKind::BridgeObjectToRefInst:
      case PILNodeKind::UnconditionalCheckedCastInst:
      case PILNodeKind::UncheckedEnumDataInst:
      case PILNodeKind::MarkUninitializedInst:
      case PILNodeKind::SelectEnumInst:
      case PILNodeKind::SwitchEnumInst:
      case PILNodeKind::CheckedCastBranchInst:
      case PILNodeKind::BranchInst:
      case PILNodeKind::CondBranchInst:
      case PILNodeKind::DestructureStructInst:
      case PILNodeKind::DestructureTupleInst:
      case PILNodeKind::MarkDependenceInst:
         return true;
      default:
         return false;
   }
}

// These operations forward guaranteed ownership, but don't necessarily forward
// owned values.
bool polar::isGuaranteedForwardingValueKind(PILNodeKind kind) {
   switch (kind) {
      case PILNodeKind::TupleExtractInst:
      case PILNodeKind::StructExtractInst:
      case PILNodeKind::OpenExistentialValueInst:
      case PILNodeKind::OpenExistentialBoxValueInst:
         return true;
      default:
         return isOwnershipForwardingValueKind(kind);
   }
}

bool polar::isGuaranteedForwardingValue(PILValue value) {
   return isGuaranteedForwardingValueKind(
      value->getKindOfRepresentativePILNodeInObject());
}

bool polar::isGuaranteedForwardingInst(PILInstruction *i) {
   return isGuaranteedForwardingValueKind(PILNodeKind(i->getKind()));
}

bool polar::isOwnershipForwardingInst(PILInstruction *i) {
   return isOwnershipForwardingValueKind(PILNodeKind(i->getKind()));
}

//===----------------------------------------------------------------------===//
//                             Borrow Introducers
//===----------------------------------------------------------------------===//

void BorrowScopeIntroducingValueKind::print(llvm::raw_ostream &os) const {
   switch (value) {
      case BorrowScopeIntroducingValueKind::PILFunctionArgument:
         os << "PILFunctionArgument";
         return;
      case BorrowScopeIntroducingValueKind::BeginBorrow:
         os << "BeginBorrowInst";
         return;
      case BorrowScopeIntroducingValueKind::LoadBorrow:
         os << "LoadBorrowInst";
         return;
   }
   llvm_unreachable("Covered switch isn't covered?!");
}

void BorrowScopeIntroducingValueKind::dump() const {
#ifndef NDEBUG
   print(llvm::dbgs());
#endif
}

void BorrowScopeIntroducingValue::getLocalScopeEndingInstructions(
   SmallVectorImpl<PILInstruction *> &scopeEndingInsts) const {
   assert(isLocalScope() && "Should only call this given a local scope");

   switch (kind) {
      case BorrowScopeIntroducingValueKind::PILFunctionArgument:
         llvm_unreachable("Should only call this with a local scope");
      case BorrowScopeIntroducingValueKind::BeginBorrow:
         llvm::copy(cast<BeginBorrowInst>(value)->getEndBorrows(),
                    std::back_inserter(scopeEndingInsts));
         return;
      case BorrowScopeIntroducingValueKind::LoadBorrow:
         llvm::copy(cast<LoadBorrowInst>(value)->getEndBorrows(),
                    std::back_inserter(scopeEndingInsts));
         return;
   }
   llvm_unreachable("Covered switch isn't covered?!");
}

void BorrowScopeIntroducingValue::visitLocalScopeEndingUses(
   function_ref<void(Operand *)> visitor) const {
   assert(isLocalScope() && "Should only call this given a local scope");
   switch (kind) {
      case BorrowScopeIntroducingValueKind::PILFunctionArgument:
         llvm_unreachable("Should only call this with a local scope");
      case BorrowScopeIntroducingValueKind::BeginBorrow:
         for (auto *use : value->getUses()) {
            if (isa<EndBorrowInst>(use->getUser())) {
               visitor(use);
            }
         }
         return;
      case BorrowScopeIntroducingValueKind::LoadBorrow:
         for (auto *use : value->getUses()) {
            if (isa<EndBorrowInst>(use->getUser())) {
               visitor(use);
            }
         }
         return;
   }
   llvm_unreachable("Covered switch isn't covered?!");
}

bool polar::getUnderlyingBorrowIntroducingValues(
   PILValue inputValue, SmallVectorImpl<BorrowScopeIntroducingValue> &out) {
   if (inputValue.getOwnershipKind() != ValueOwnershipKind::Guaranteed)
      return false;

   SmallVector<PILValue, 32> worklist;
   worklist.emplace_back(inputValue);

   while (!worklist.empty()) {
      PILValue v = worklist.pop_back_val();

      // First check if v is an introducer. If so, stash it and continue.
      if (auto scopeIntroducer = BorrowScopeIntroducingValue::get(v)) {
         out.push_back(*scopeIntroducer);
         continue;
      }

      // If v produces .none ownership, then we can ignore it. It is important
      // that we put this before checking for guaranteed forwarding instructions,
      // since we want to ignore guaranteed forwarding instructions that in this
      // specific case produce a .none value.
      if (v.getOwnershipKind() == ValueOwnershipKind::None)
         continue;

      // Otherwise if v is an ownership forwarding value, add its defining
      // instruction
      if (isGuaranteedForwardingValue(v)) {
         auto *i = v->getDefiningInstruction();
         assert(i);
         llvm::transform(i->getAllOperands(), std::back_inserter(worklist),
                         [](const Operand &op) -> PILValue { return op.get(); });
         continue;
      }

      // Otherwise, this is an introducer we do not understand. Bail and return
      // false.
      return false;
   }

   return true;
}

llvm::raw_ostream &polar::operator<<(llvm::raw_ostream &os,
                                     BorrowScopeIntroducingValueKind kind) {
   kind.print(os);
   return os;
}

bool BorrowScopeIntroducingValue::areInstructionsWithinScope(
   ArrayRef<BranchPropagatedUser> instructions,
   SmallVectorImpl<BranchPropagatedUser> &scratchSpace,
   SmallPtrSetImpl<PILBasicBlock *> &visitedBlocks,
   DeadEndBlocks &deadEndBlocks) const {
   // Make sure that we clear our scratch space/utilities before we exit.
   POLAR_DEFER {
      scratchSpace.clear();
      visitedBlocks.clear();
   };

   // First make sure that we actually have a local scope. If we have a non-local
   // scope, then we have something (like a PILFunctionArgument) where a larger
   // semantic construct (in the case of PILFunctionArgument, the function
   // itself) acts as the scope. So we already know that our passed in
   // instructions must be in the same scope.
   if (!isLocalScope())
      return true;

   // Otherwise, gather up our local scope ending instructions.
   visitLocalScopeEndingUses(
      [&scratchSpace](Operand *op) { scratchSpace.emplace_back(op); });

   LinearLifetimeChecker checker(visitedBlocks, deadEndBlocks);
   return checker.validateLifetime(value, scratchSpace, instructions);
}
