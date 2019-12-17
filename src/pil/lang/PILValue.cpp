//===--- PILValue.cpp - Implementation for PILValue -----------------------===//
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

#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILBuiltinVisitor.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/lang/PILVisitor.h"
#include "llvm/ADT/StringSwitch.h"

using namespace polar;

//===----------------------------------------------------------------------===//
//                       Check PILNode Type Properties
//===----------------------------------------------------------------------===//

/// These are just for performance and verification. If one needs to make
/// changes that cause the asserts the fire, please update them. The purpose is
/// to prevent these predicates from changing values by mistake.

//===----------------------------------------------------------------------===//
//                       Check PILValue Type Properties
//===----------------------------------------------------------------------===//

/// These are just for performance and verification. If one needs to make
/// changes that cause the asserts the fire, please update them. The purpose is
/// to prevent these predicates from changing values by mistake.
static_assert(std::is_standard_layout<PILValue>::value,
              "Expected PILValue to be standard layout");
static_assert(sizeof(PILValue) == sizeof(uintptr_t),
              "PILValue should be pointer sized");

//===----------------------------------------------------------------------===//
//                              Utility Methods
//===----------------------------------------------------------------------===//

void ValueBase::replaceAllUsesWith(ValueBase *RHS) {
   assert(this != RHS && "Cannot RAUW a value with itself");
   while (!use_empty()) {
      Operand *Op = *use_begin();
      Op->set(RHS);
   }
}

void ValueBase::replaceAllUsesWithUndef() {
   auto *F = getFunction();
   if (!F) {
      llvm_unreachable("replaceAllUsesWithUndef can only be used on ValueBase "
                       "that have access to the parent function.");
   }
   while (!use_empty()) {
      Operand *Op = *use_begin();
      Op->set(PILUndef::get(Op->get()->getType(), *F));
   }
}

PILInstruction *ValueBase::getDefiningInstruction() {
   if (auto *inst = dyn_cast<SingleValueInstruction>(this))
      return inst;
   if (auto *result = dyn_cast<MultipleValueInstructionResult>(this))
      return result->getParent();
   return nullptr;
}

Optional<ValueBase::DefiningInstructionResult>
ValueBase::getDefiningInstructionResult() {
   if (auto *inst = dyn_cast<SingleValueInstruction>(this))
      return DefiningInstructionResult{inst, 0};
   if (auto *result = dyn_cast<MultipleValueInstructionResult>(this))
      return DefiningInstructionResult{result->getParent(), result->getIndex()};
   return None;
}

PILBasicBlock *PILNode::getParentBlock() const {
   auto *CanonicalNode =
      const_cast<PILNode *>(this)->getRepresentativePILNodeInObject();
   if (auto *Inst = dyn_cast<PILInstruction>(CanonicalNode))
      return Inst->getParent();
   if (auto *Arg = dyn_cast<PILArgument>(CanonicalNode))
      return Arg->getParent();
   return nullptr;
}

PILFunction *PILNode::getFunction() const {
   auto *CanonicalNode =
      const_cast<PILNode *>(this)->getRepresentativePILNodeInObject();
   if (auto *Inst = dyn_cast<PILInstruction>(CanonicalNode))
      return Inst->getFunction();
   if (auto *Arg = dyn_cast<PILArgument>(CanonicalNode))
      return Arg->getFunction();
   return nullptr;
}

PILModule *PILNode::getModule() const {
   auto *CanonicalNode =
      const_cast<PILNode *>(this)->getRepresentativePILNodeInObject();
   if (auto *Inst = dyn_cast<PILInstruction>(CanonicalNode))
      return &Inst->getModule();
   if (auto *Arg = dyn_cast<PILArgument>(CanonicalNode))
      return &Arg->getModule();
   return nullptr;
}

const PILNode *PILNode::getRepresentativePILNodeSlowPath() const {
   assert(getStorageLoc() != PILNodeStorageLocation::Instruction);

   if (isa<SingleValueInstruction>(this)) {
      assert(hasMultiplePILNodeBases(getKind()));
      return &static_cast<const PILInstruction &>(
         static_cast<const SingleValueInstruction &>(
            static_cast<const ValueBase &>(*this)));
   }

   if (auto *MVR = dyn_cast<MultipleValueInstructionResult>(this)) {
      return MVR->getParent();
   }

   llvm_unreachable("Invalid value for slow path");
}

/// Get a location for this value.
PILLocation PILValue::getLoc() const {
   if (auto *instr = Value->getDefiningInstruction())
      return instr->getLoc();

   if (auto *arg = dyn_cast<PILArgument>(*this)) {
      if (arg->getDecl())
         return RegularLocation(const_cast<ValueDecl *>(arg->getDecl()));
   }
   // TODO: bbargs should probably use one of their operand locations.
   return Value->getFunction()->getLocation();
}

//===----------------------------------------------------------------------===//
//                             ValueOwnershipKind
//===----------------------------------------------------------------------===//

ValueOwnershipKind::ValueOwnershipKind(const PILFunction &F, PILType Type,
                                       PILArgumentConvention Convention)
   : Value() {
   auto &M = F.getModule();

   // Trivial types can be passed using a variety of conventions. They always
   // have trivial ownership.
   if (Type.isTrivial(F)) {
      Value = ValueOwnershipKind::None;
      return;
   }

   switch (Convention) {
      case PILArgumentConvention::Indirect_In:
      case PILArgumentConvention::Indirect_In_Constant:
         Value = PILModuleConventions(M).useLoweredAddresses()
                 ? ValueOwnershipKind::None
                 : ValueOwnershipKind::Owned;
         break;
      case PILArgumentConvention::Indirect_In_Guaranteed:
         Value = PILModuleConventions(M).useLoweredAddresses()
                 ? ValueOwnershipKind::None
                 : ValueOwnershipKind::Guaranteed;
         break;
      case PILArgumentConvention::Indirect_Inout:
      case PILArgumentConvention::Indirect_InoutAliasable:
      case PILArgumentConvention::Indirect_Out:
         Value = ValueOwnershipKind::None;
         return;
      case PILArgumentConvention::Direct_Owned:
         Value = ValueOwnershipKind::Owned;
         return;
      case PILArgumentConvention::Direct_Unowned:
         Value = ValueOwnershipKind::Unowned;
         return;
      case PILArgumentConvention::Direct_Guaranteed:
         Value = ValueOwnershipKind::Guaranteed;
         return;
      case PILArgumentConvention::Direct_Deallocating:
         llvm_unreachable("Not handled");
   }
}

StringRef ValueOwnershipKind::asString() const {
   switch (Value) {
      case ValueOwnershipKind::Unowned:
         return "unowned";
      case ValueOwnershipKind::Owned:
         return "owned";
      case ValueOwnershipKind::Guaranteed:
         return "guaranteed";
      case ValueOwnershipKind::None:
         return "any";
   }
   llvm_unreachable("Unhandled ValueOwnershipKind in switch.");
}

llvm::raw_ostream &polar::operator<<(llvm::raw_ostream &os,
                                     ValueOwnershipKind kind) {
   return os << kind.asString();
}

Optional<ValueOwnershipKind>
ValueOwnershipKind::merge(ValueOwnershipKind RHS) const {
   auto LHSVal = Value;
   auto RHSVal = RHS.Value;

   // Any merges with anything.
   if (LHSVal == ValueOwnershipKind::None) {
      return ValueOwnershipKind(RHSVal);
   }
   // Any merges with anything.
   if (RHSVal == ValueOwnershipKind::None) {
      return ValueOwnershipKind(LHSVal);
   }

   return (LHSVal == RHSVal) ? Optional<ValueOwnershipKind>(*this) : llvm::None;
}

ValueOwnershipKind::ValueOwnershipKind(StringRef S) {
   auto Result = llvm::StringSwitch<Optional<ValueOwnershipKind::innerty>>(S)
      .Case("unowned", ValueOwnershipKind::Unowned)
      .Case("owned", ValueOwnershipKind::Owned)
      .Case("guaranteed", ValueOwnershipKind::Guaranteed)
      .Case("any", ValueOwnershipKind::None)
      .Default(None);
   if (!Result.hasValue())
      llvm_unreachable("Invalid string representation of ValueOwnershipKind");
   Value = Result.getValue();
}

ValueOwnershipKind
ValueOwnershipKind::getProjectedOwnershipKind(const PILFunction &F,
                                              PILType Proj) const {
   if (Proj.isTrivial(F))
      return ValueOwnershipKind::None;
   return *this;
}

#if 0
/// Map a PILValue mnemonic name to its ValueKind.
ValueKind polar::getPILValueKind(StringRef Name) {
#define SINGLE_VALUE_INST(Id, TextualName, Parent, MemoryBehavior,             \
                          ReleasingBehavior)                                   \
  if (Name == #TextualName)                                                    \
    return ValueKind::Id;

#define VALUE(Id, Parent)                                                      \
  if (Name == #Id)                                                             \
    return ValueKind::Id;

#include "polarphp/pil/lang/PILNodes.def"

#ifdef NDEBUG
  llvm::errs()
    << "Unknown PILValue name\n";
  abort();
#endif
  llvm_unreachable("Unknown PILValue name");
}

/// Map ValueKind to a corresponding mnemonic name.
StringRef polar::getPILValueName(ValueKind Kind) {
  switch (Kind) {
#define SINGLE_VALUE_INST(Id, TextualName, Parent, MemoryBehavior,             \
                          ReleasingBehavior)                                   \
  case ValueKind::Id:                                                          \
    return #TextualName;

#define VALUE(Id, Parent)                                                      \
  case ValueKind::Id:                                                          \
    return #Id;

#include "polarphp/pil/lang/PILNodes.def"
  }
}
#endif

//===----------------------------------------------------------------------===//
//                          OperandOwnershipKindMap
//===----------------------------------------------------------------------===//

void OperandOwnershipKindMap::print(llvm::raw_ostream &os) const {
   os << "-- OperandOwnershipKindMap --\n";

   unsigned index = 0;
   unsigned end = unsigned(ValueOwnershipKind::LastValueOwnershipKind) + 1;
   while (index != end) {
      auto kind = ValueOwnershipKind(index);
      if (canAcceptKind(kind)) {
         os << kind << ": Yes. Liveness: " << getLifetimeConstraint(kind) << "\n";
      } else {
         os << kind << ":  No."
            << "\n";
      }
      ++index;
   }
}

void OperandOwnershipKindMap::dump() const { print(llvm::dbgs()); }

//===----------------------------------------------------------------------===//
//                           UseLifetimeConstraint
//===----------------------------------------------------------------------===//

llvm::raw_ostream &polar::operator<<(llvm::raw_ostream &os,
                                     UseLifetimeConstraint constraint) {
   switch (constraint) {
      case UseLifetimeConstraint::MustBeLive:
         os << "MustBeLive";
         break;
      case UseLifetimeConstraint::MustBeInvalidated:
         os << "MustBeInvalidated";
         break;
   }
   return os;
}
