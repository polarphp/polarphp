//===--- BranchPropagatedUser.h -------------------------------------------===//
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

#ifndef POLARPHP_PIL_BRANCHPROPAGATEDUSER_H
#define POLARPHP_PIL_BRANCHPROPAGATEDUSER_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/pil/lang/PILBasicBlock.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/PointerLikeTypeTraits.h"

namespace polar {

/// This is a class that models normal users and also cond_br users that are
/// associated with the block in the target block. This is safe to do since in
/// Semantic PIL, cond_br with non-trivial arguments are not allowed to have
/// critical edges. In non-semantic PIL, it is expected that any user of
/// BranchPropagatedUser and friends break all such critical edges.
class BranchPropagatedUser {
  using InnerTy = llvm::PointerIntPair<PILInstruction *, 1>;
  InnerTy user;

public:
  BranchPropagatedUser(Operand *op) : user() {
    auto *opUser = op->getUser();
    auto *cbi = dyn_cast<CondBranchInst>(opUser);
    if (!cbi) {
      user = InnerTy(opUser, 0);
      return;
    }
    unsigned operandIndex = op->getOperandNumber();
    if (cbi->isConditionOperandIndex(operandIndex)) {
      // TODO: Is this correct?
      user = InnerTy(cbi, CondBranchInst::TrueIdx);
      return;
    }
    bool isTrueOperand = cbi->isTrueOperandIndex(operandIndex);
    if (isTrueOperand) {
      user = InnerTy(cbi, CondBranchInst::TrueIdx);
    } else {
      user = InnerTy(cbi, CondBranchInst::FalseIdx);
    }
  }
  BranchPropagatedUser(const Operand *op)
      : BranchPropagatedUser(const_cast<Operand *>(op)) {}

  BranchPropagatedUser(const BranchPropagatedUser &other) : user(other.user) {}
  BranchPropagatedUser &operator=(const BranchPropagatedUser &other) {
    user = other.user;
    return *this;
  }

  operator PILInstruction *() { return user.getPointer(); }
  operator const PILInstruction *() const { return user.getPointer(); }

  PILInstruction *getInst() const { return user.getPointer(); }

  PILBasicBlock *getParent() const {
    if (!isCondBranchUser()) {
      return getInst()->getParent();
    }

    auto *cbi = cast<CondBranchInst>(getInst());
    unsigned number = getCondBranchSuccessorID();
    if (number == CondBranchInst::TrueIdx)
      return cbi->getTrueBB();
    return cbi->getFalseBB();
  }

  bool isCondBranchUser() const {
    return isa<CondBranchInst>(user.getPointer());
  }

  unsigned getCondBranchSuccessorID() const {
    assert(isCondBranchUser());
    return user.getInt();
  }

  PILBasicBlock::iterator getIterator() const {
    return user.getPointer()->getIterator();
  }

  void *getAsOpaqueValue() const {
    return llvm::PointerLikeTypeTraits<InnerTy>::getAsVoidPointer(user);
  }

  static BranchPropagatedUser getFromOpaqueValue(void *p) {
    InnerTy tmpUser =
        llvm::PointerLikeTypeTraits<InnerTy>::getFromVoidPointer(p);
    if (auto *cbi = dyn_cast<CondBranchInst>(tmpUser.getPointer())) {
      return BranchPropagatedUser(cbi, tmpUser.getInt());
    }
    return BranchPropagatedUser(tmpUser.getPointer());
  }

  enum {
    NumLowBitsAvailable =
        llvm::PointerLikeTypeTraits<InnerTy>::NumLowBitsAvailable
  };

private:
  BranchPropagatedUser(PILInstruction *inst) : user(inst) {
    assert(!isa<CondBranchInst>(inst));
  }

  BranchPropagatedUser(CondBranchInst *cbi) : user(cbi) {}

  BranchPropagatedUser(CondBranchInst *cbi, unsigned successorIndex)
      : user(cbi, successorIndex) {
    assert(successorIndex == CondBranchInst::TrueIdx ||
           successorIndex == CondBranchInst::FalseIdx);
  }
};

} // namespace polar

namespace llvm {

template <> struct PointerLikeTypeTraits<polar::BranchPropagatedUser> {
public:
  using BranchPropagatedUser = polar::BranchPropagatedUser;

  static void *getAsVoidPointer(BranchPropagatedUser v) {
    return v.getAsOpaqueValue();
  }

  static BranchPropagatedUser getFromVoidPointer(void *p) {
    return BranchPropagatedUser::getFromOpaqueValue(p);
  }

  enum { NumLowBitsAvailable = BranchPropagatedUser::NumLowBitsAvailable };
};

} // namespace llvm

#endif
