//===--- PILArgument.h - PIL BasicBlock Argument Representation -*- C++ -*-===//
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

#ifndef POLARPHP_PIL_PILARGUMENT_H
#define POLARPHP_PIL_PILARGUMENT_H

#include "polarphp/global/CompilerFeature.h"
#include "polarphp/pil/lang/PILArgumentConvention.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILBasicBlock.h"
#include "polarphp/pil/lang/PILValue.h"

namespace polar {

class PILBasicBlock;
class PILModule;
class PILUndef;

// Map an argument index onto a PILArgumentConvention.
inline PILArgumentConvention
PILFunctionConventions::getPILArgumentConvention(unsigned index) const {
   assert(index <= getNumPILArguments());
   if (index < getNumIndirectPILResults()) {
      assert(silConv.loweredAddresses);
      return PILArgumentConvention::Indirect_Out;
   } else {
      auto param = funcTy->getParameters()[index - getNumIndirectPILResults()];
      return PILArgumentConvention(param.getConvention());
   }
}

struct PILArgumentKind {
   enum innerty : std::underlying_type<ValueKind>::type {
#define ARGUMENT(ID, PARENT) ID = unsigned(PILNodeKind::ID),
#define ARGUMENT_RANGE(ID, FIRST, LAST) First_##ID = FIRST, Last_##ID = LAST,
#include "polarphp/pil/lang/PILNodesDef.h"
   } value;

   explicit PILArgumentKind(ValueKind kind)
      : value(*PILArgumentKind::fromValueKind(kind)) {}
   PILArgumentKind(innerty value) : value(value) {}
   operator innerty() const { return value; }

   static Optional<PILArgumentKind> fromValueKind(ValueKind kind) {
      switch (kind) {
#define ARGUMENT(ID, PARENT)                                                   \
  case ValueKind::ID:                                                          \
    return PILArgumentKind(ID);
#include "polarphp/pil/lang/PILNodesDef.h"
         default:
            return None;
      }
   }
};

class PILArgument : public ValueBase {
   friend class PILBasicBlock;

   PILBasicBlock *parentBlock;
   const ValueDecl *decl;

protected:
   PILArgument(ValueKind subClassKind, PILBasicBlock *inputParentBlock,
               PILType type, ValueOwnershipKind ownershipKind,
               const ValueDecl *inputDecl = nullptr);

   PILArgument(ValueKind subClassKind, PILBasicBlock *inputParentBlock,
               PILBasicBlock::arg_iterator positionInArgumentArray, PILType type,
               ValueOwnershipKind ownershipKind,
               const ValueDecl *inputDecl = nullptr);

   // A special constructor, only intended for use in
   // PILBasicBlock::replacePHIArg and replaceFunctionArg.
   explicit PILArgument(ValueKind subClassKind, PILType type,
                        ValueOwnershipKind ownershipKind,
                        const ValueDecl *inputDecl = nullptr)
      : ValueBase(subClassKind, type, IsRepresentative::Yes),
        parentBlock(nullptr), decl(inputDecl) {
      Bits.PILArgument.VOKind = static_cast<unsigned>(ownershipKind);
   }

public:
   void operator=(const PILArgument &) = delete;
   void operator delete(void *, size_t) POLAR_DELETE_OPERATOR_DELETED;

   ValueOwnershipKind getOwnershipKind() const {
      return static_cast<ValueOwnershipKind>(Bits.PILArgument.VOKind);
   }

   void setOwnershipKind(ValueOwnershipKind newKind) {
      Bits.PILArgument.VOKind = static_cast<unsigned>(newKind);
   }

   PILBasicBlock *getParent() { return parentBlock; }
   const PILBasicBlock *getParent() const { return parentBlock; }

   PILFunction *getFunction();
   const PILFunction *getFunction() const;

   PILModule &getModule() const;

   const ValueDecl *getDecl() const { return decl; }

   static bool classof(const PILInstruction *) = delete;
   static bool classof(const PILUndef *) = delete;
   static bool classof(const PILNode *node) {
      return node->getKind() >= PILNodeKind::First_PILArgument &&
             node->getKind() <= PILNodeKind::Last_PILArgument;
   }

   unsigned getIndex() const {
      for (auto p : llvm::enumerate(getParent()->getArguments())) {
         if (p.value() == this) {
            return p.index();
         }
      }
      llvm_unreachable("PILArgument not argument of its parent BB");
   }

   /// Return true if this block argument is actually a phi argument as
   /// opposed to a cast or projection.
   bool isPhiArgument() const;

   /// If this argument is a phi, return the incoming phi value for the given
   /// predecessor BB. If this argument is not a phi, return an invalid PILValue.
   PILValue getIncomingPhiValue(PILBasicBlock *predBlock) const;

   /// If this argument is a phi, populate `OutArray` with the incoming phi
   /// values for each predecessor BB. If this argument is not a phi, return
   /// false.
   bool getIncomingPhiValues(SmallVectorImpl<PILValue> &returnedPhiValues) const;

   /// If this argument is a phi, populate `OutArray` with each predecessor block
   /// and its incoming phi value. If this argument is not a phi, return false.
   bool
   getIncomingPhiValues(SmallVectorImpl<std::pair<PILBasicBlock *, PILValue>>
                        &returnedPredAndPhiValuePairs) const;

   /// Returns true if we were able to find a single terminator operand value for
   /// each predecessor of this arguments basic block. The found values are
   /// stored in OutArray.
   ///
   /// Note: this peeks through any projections or cast implied by the
   /// terminator. e.g. the incoming value for a switch_enum payload argument is
   /// the enum itself (the operand of the switch_enum).
   bool getSingleTerminatorOperands(
      SmallVectorImpl<PILValue> &returnedSingleTermOperands) const;

   /// Returns true if we were able to find single terminator operand values for
   /// each predecessor of this arguments basic block. The found values are
   /// stored in OutArray alongside their predecessor block.
   ///
   /// Note: this peeks through any projections or cast implied by the
   /// terminator. e.g. the incoming value for a switch_enum payload argument is
   /// the enum itself (the operand of the switch_enum).
   bool getSingleTerminatorOperands(
      SmallVectorImpl<std::pair<PILBasicBlock *, PILValue>>
      &returnedSingleTermOperands) const;

   /// If this PILArgument's parent block has a single predecessor whose
   /// terminator has a single operand, return the incoming operand of the
   /// predecessor's terminator. Returns PILValue() otherwise.  Note that for
   /// some predecessor terminators the incoming value is not exactly the
   /// argument value. E.g. the incoming value for a switch_enum payload argument
   /// is the enum itself (the operand of the switch_enum).
   PILValue getSingleTerminatorOperand() const;

   /// Return the PILArgumentKind of this argument.
   PILArgumentKind getKind() const {
      return PILArgumentKind(ValueBase::getKind());
   }

protected:
   void setParent(PILBasicBlock *newParentBlock) {
      parentBlock = newParentBlock;
   }
};

class PILPhiArgument : public PILArgument {
   friend class PILBasicBlock;

   PILPhiArgument(PILBasicBlock *parentBlock, PILType type,
                  ValueOwnershipKind ownershipKind,
                  const ValueDecl *decl = nullptr)
      : PILArgument(ValueKind::PILPhiArgument, parentBlock, type, ownershipKind,
                    decl) {}

   PILPhiArgument(PILBasicBlock *parentBlock,
                  PILBasicBlock::arg_iterator argArrayInsertPt, PILType type,
                  ValueOwnershipKind ownershipKind,
                  const ValueDecl *decl = nullptr)
      : PILArgument(ValueKind::PILPhiArgument, parentBlock, argArrayInsertPt,
                    type, ownershipKind, decl) {}

   // A special constructor, only intended for use in
   // PILBasicBlock::replacePHIArg.
   explicit PILPhiArgument(PILType type, ValueOwnershipKind ownershipKind,
                           const ValueDecl *decl = nullptr)
      : PILArgument(ValueKind::PILPhiArgument, type, ownershipKind, decl) {}

public:
   /// Return true if this is block argument is actually a phi argument as
   /// opposed to a cast or projection.
   bool isPhiArgument() const;

   /// If this argument is a phi, return the incoming phi value for the given
   /// predecessor BB. If this argument is not a phi, return an invalid PILValue.
   ///
   /// FIXME: Once PILPhiArgument actually implies that it is a phi argument,
   /// this will be guaranteed to return a valid PILValue.
   PILValue getIncomingPhiValue(PILBasicBlock *predBlock) const;

   /// If this argument is a phi, populate `OutArray` with the incoming phi
   /// values for each predecessor BB. If this argument is not a phi, return
   /// false.
   ///
   /// FIXME: Once PILPhiArgument actually implies that it is a phi argument,
   /// this will always succeed.
   bool getIncomingPhiValues(SmallVectorImpl<PILValue> &returnedPhiValues) const;

   /// If this argument is a phi, populate `OutArray` with each predecessor block
   /// and its incoming phi value. If this argument is not a phi, return false.
   ///
   /// FIXME: Once PILPhiArgument actually implies that it is a phi argument,
   /// this will always succeed.
   bool
   getIncomingPhiValues(SmallVectorImpl<std::pair<PILBasicBlock *, PILValue>>
                        &returnedPredAndPhiValuePairs) const;

   /// Returns true if we were able to find a single terminator operand value for
   /// each predecessor of this arguments basic block. The found values are
   /// stored in OutArray.
   ///
   /// Note: this peeks through any projections or cast implied by the
   /// terminator. e.g. the incoming value for a switch_enum payload argument is
   /// the enum itself (the operand of the switch_enum).
   bool getSingleTerminatorOperands(
      SmallVectorImpl<PILValue> &returnedSingleTermOperands) const;

   /// Returns true if we were able to find single terminator operand values for
   /// each predecessor of this arguments basic block. The found values are
   /// stored in OutArray alongside their predecessor block.
   ///
   /// Note: this peeks through any projections or cast implied by the
   /// terminator. e.g. the incoming value for a switch_enum payload argument is
   /// the enum itself (the operand of the switch_enum).
   bool getSingleTerminatorOperands(
      SmallVectorImpl<std::pair<PILBasicBlock *, PILValue>>
      &returnedSingleTermOperands) const;

   /// If this PILArgument's parent block has a single predecessor whose
   /// terminator has a single operand, return the incoming operand of the
   /// predecessor's terminator. Returns PILValue() otherwise.  Note that for
   /// some predecessor terminators the incoming value is not exactly the
   /// argument value. E.g. the incoming value for a switch_enum payload argument
   /// is the enum itself (the operand of the switch_enum).
   PILValue getSingleTerminatorOperand() const;

   static bool classof(const PILInstruction *) = delete;
   static bool classof(const PILUndef *) = delete;
   static bool classof(const PILNode *node) {
      return node->getKind() == PILNodeKind::PILPhiArgument;
   }
};

class PILFunctionArgument : public PILArgument {
   friend class PILBasicBlock;

   PILFunctionArgument(PILBasicBlock *parentBlock, PILType type,
                       ValueOwnershipKind ownershipKind,
                       const ValueDecl *decl = nullptr)
      : PILArgument(ValueKind::PILFunctionArgument, parentBlock, type,
                    ownershipKind, decl) {}
   PILFunctionArgument(PILBasicBlock *parentBlock,
                       PILBasicBlock::arg_iterator argArrayInsertPt,
                       PILType type, ValueOwnershipKind ownershipKind,
                       const ValueDecl *decl = nullptr)
      : PILArgument(ValueKind::PILFunctionArgument, parentBlock,
                    argArrayInsertPt, type, ownershipKind, decl) {}

   // A special constructor, only intended for use in
   // PILBasicBlock::replaceFunctionArg.
   explicit PILFunctionArgument(PILType type, ValueOwnershipKind ownershipKind,
                                const ValueDecl *decl = nullptr)
      : PILArgument(ValueKind::PILFunctionArgument, type, ownershipKind, decl) {
   }

public:
   bool isIndirectResult() const {
      auto numIndirectResults =
         getFunction()->getConventions().getNumIndirectPILResults();
      return getIndex() < numIndirectResults;
   }

   PILArgumentConvention getArgumentConvention() const {
      return getFunction()->getConventions().getPILArgumentConvention(getIndex());
   }

   /// Given that this is an entry block argument, and given that it does
   /// not correspond to an indirect result, return the corresponding
   /// PILParameterInfo.
   PILParameterInfo getKnownParameterInfo() const {
      return getFunction()->getConventions().getParamInfoForPILArg(getIndex());
   }

   /// Returns true if this PILArgument is the self argument of its
   /// function. This means that this will return false always for PILArguments
   /// of PILFunctions that do not have self argument and for non-function
   /// argument PILArguments.
   bool isSelf() const;

   /// Returns true if this PILArgument is passed via the given convention.
   bool hasConvention(PILArgumentConvention convention) const {
      return getArgumentConvention() == convention;
   }

   static bool classof(const PILInstruction *) = delete;
   static bool classof(const PILUndef *) = delete;
   static bool classof(const PILNode *node) {
      return node->getKind() == PILNodeKind::PILFunctionArgument;
   }
};

//===----------------------------------------------------------------------===//
// Out of line Definitions for PILArgument to avoid Forward Decl issues
//===----------------------------------------------------------------------===//

inline bool PILArgument::isPhiArgument() const {
   switch (getKind()) {
      case PILArgumentKind::PILPhiArgument:
         return cast<PILPhiArgument>(this)->isPhiArgument();
      case PILArgumentKind::PILFunctionArgument:
         return false;
   }
   llvm_unreachable("Covered switch is not covered?!");
}

inline PILValue
PILArgument::getIncomingPhiValue(PILBasicBlock *predBlock) const {
   switch (getKind()) {
      case PILArgumentKind::PILPhiArgument:
         return cast<PILPhiArgument>(this)->getIncomingPhiValue(predBlock);
      case PILArgumentKind::PILFunctionArgument:
         return PILValue();
   }
   llvm_unreachable("Covered switch is not covered?!");
}

inline bool PILArgument::getIncomingPhiValues(
   SmallVectorImpl<PILValue> &returnedPhiValues) const {
   switch (getKind()) {
      case PILArgumentKind::PILPhiArgument:
         return cast<PILPhiArgument>(this)->getIncomingPhiValues(returnedPhiValues);
      case PILArgumentKind::PILFunctionArgument:
         return false;
   }
   llvm_unreachable("Covered switch is not covered?!");
}

inline bool PILArgument::getIncomingPhiValues(
   SmallVectorImpl<std::pair<PILBasicBlock *, PILValue>>
   &returnedPredAndPhiValuePairs) const {
   switch (getKind()) {
      case PILArgumentKind::PILPhiArgument:
         return cast<PILPhiArgument>(this)->getIncomingPhiValues(
            returnedPredAndPhiValuePairs);
      case PILArgumentKind::PILFunctionArgument:
         return false;
   }
   llvm_unreachable("Covered switch is not covered?!");
}

inline bool PILArgument::getSingleTerminatorOperands(
   SmallVectorImpl<PILValue> &returnedSingleTermOperands) const {
   switch (getKind()) {
      case PILArgumentKind::PILPhiArgument:
         return cast<PILPhiArgument>(this)->getSingleTerminatorOperands(
            returnedSingleTermOperands);
      case PILArgumentKind::PILFunctionArgument:
         return false;
   }
   llvm_unreachable("Covered switch is not covered?!");
}

inline bool PILArgument::getSingleTerminatorOperands(
   SmallVectorImpl<std::pair<PILBasicBlock *, PILValue>>
   &returnedSingleTermOperands) const {
   switch (getKind()) {
      case PILArgumentKind::PILPhiArgument:
         return cast<PILPhiArgument>(this)->getSingleTerminatorOperands(
            returnedSingleTermOperands);
      case PILArgumentKind::PILFunctionArgument:
         return false;
   }
   llvm_unreachable("Covered switch is not covered?!");
}

} // end polar namespace

#endif
