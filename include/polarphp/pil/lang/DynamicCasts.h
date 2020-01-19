//===--- DynamicCasts.h - PIL dynamic-cast utilities ------------*- C++ -*-===//
//
// This source file is part of the Polarphp.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Polarphp project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Polarphp project authors
//
//===----------------------------------------------------------------------===//
//
// This file provides basic utilities for working with subtyping
// relationships.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_DYNAMICCASTS_H
#define POLARPHP_PIL_DYNAMICCASTS_H

#include "polarphp/ast/Module.h"
#include "polarphp/basic/ProfileCounter.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/lang/PILValue.h"

namespace polar {

class CanType;
class ModuleDecl;
class PILBuilder;
class PILLocation;
class PILModule;
class PILType;
enum class CastConsumptionKind : unsigned char;
struct PILDynamicCastInst;

enum class DynamicCastFeasibility {
  /// The cast will always succeed.
  WillSucceed,

  /// The cast can succeed for some values.
  MaySucceed,

  /// The cast cannot succeed.
  WillFail,
};

static inline DynamicCastFeasibility
atWorst(DynamicCastFeasibility feasibility, DynamicCastFeasibility worstCase) {
  return (feasibility > worstCase ? worstCase : feasibility);
}

static inline DynamicCastFeasibility
atBest(DynamicCastFeasibility feasibility, DynamicCastFeasibility bestCase) {
  return (feasibility < bestCase ? bestCase : feasibility);
}

/// Classify the feasibility of a dynamic cast.  The source and target
/// types should be unlowered formal types.
DynamicCastFeasibility classifyDynamicCast(
    ModuleDecl *context,
    CanType sourceType, CanType targetType,
    bool isSourceTypeExact = false,
    bool isWholdModuleOpts = false);

PILValue emitSuccessfulScalarUnconditionalCast(PILBuilder &B, PILLocation loc,
                                               PILDynamicCastInst inst);

PILValue emitSuccessfulScalarUnconditionalCast(
    PILBuilder &B, ModuleDecl *M, PILLocation loc, PILValue value,
    PILType loweredTargetType,
    CanType formalSourceType, CanType formalTargetType,
    PILInstruction *existingCast = nullptr);

bool emitSuccessfulIndirectUnconditionalCast(
    PILBuilder &B, ModuleDecl *M, PILLocation loc,
    PILValue src, CanType sourceType,
    PILValue dest, CanType targetType,
    PILInstruction *existingCast = nullptr);

bool emitSuccessfulIndirectUnconditionalCast(PILBuilder &B, PILLocation loc,
                                             PILDynamicCastInst dynamicCast);

/// Can the given cast be performed by the scalar checked-cast
/// instructions, or does we need to use the indirect instructions?
bool canUseScalarCheckedCastInstructions(PILModule &M,
                                         CanType sourceType,CanType targetType);

/// Carry out the operations required for an indirect conditional cast
/// using a scalar cast operation.
void emitIndirectConditionalCastWithScalar(
    PILBuilder &B, ModuleDecl *M, PILLocation loc,
    CastConsumptionKind consumption, PILValue src, CanType sourceType,
    PILValue dest, CanType targetType, PILBasicBlock *trueBB,
    PILBasicBlock *falseBB, ProfileCounter TrueCount = ProfileCounter(),
    ProfileCounter FalseCount = ProfileCounter());

/// Does the type conform to the _ObjectiveCBridgeable protocol.
bool isObjectiveCBridgeable(ModuleDecl *M, CanType Ty);

/// Get the bridged NS class of a CF class if it exists. Returns
/// an empty CanType if such class does not exist.
CanType getNSBridgedClassOfCFClass(ModuleDecl *M, CanType type);

/// Does the type conform to Error.
bool isError(ModuleDecl *M, CanType Ty);

struct PILDynamicCastKind {
  enum innerty : std::underlying_type<PILInstructionKind>::type {
#define DYNAMICCAST_INST(ID, PARENT) ID = unsigned(PILInstructionKind::ID),
#include "polarphp/pil/lang/PILNodesDef.h"
  } value;

  explicit PILDynamicCastKind(PILInstructionKind kind) {
    auto newValue = PILDynamicCastKind::fromNodeKindHelper(kind);
    assert(newValue && "Non cast passed into PILDynamicCastKind");
    value = newValue.getValue();
  }

  PILDynamicCastKind(innerty value) : value(value) {}
  operator innerty() const { return value; }

  static Optional<PILDynamicCastKind> fromNodeKind(PILInstructionKind kind) {
    if (auto innerTyOpt = PILDynamicCastKind::fromNodeKindHelper(kind))
      return PILDynamicCastKind(*innerTyOpt);
    return None;
  }

private:
  static Optional<innerty> fromNodeKindHelper(PILInstructionKind kind) {
    switch (kind) {
#define DYNAMICCAST_INST(ID, PARENT)                                           \
  case PILInstructionKind::ID:                                                 \
    return PILDynamicCastKind::ID;
#include "polarphp/pil/lang/PILNodesDef.h"
    default:
      return None;
    }
  }
};

struct PILDynamicCastInst {
  PILInstruction *inst;

public:
  PILDynamicCastInst() : inst(nullptr) {}

  explicit PILDynamicCastInst(PILInstruction *inst) : inst(inst) {
    assert(classof(inst) && "not a dynamic cast?!");
  }

  static bool classof(const PILInstruction *inst) {
    return bool(PILDynamicCastKind::fromNodeKind(inst->getKind()));
  }

#define DYNAMICCAST_INST(ID, PARENT)                                           \
  PILDynamicCastInst(ID *i) : inst(i) {}
#include "polarphp/pil/lang/PILNodesDef.h"

  static PILDynamicCastInst getAs(PILNode *node) {
    auto *i = dyn_cast<PILInstruction>(node);
    if (!i)
      return PILDynamicCastInst();
    auto kind = PILDynamicCastKind::fromNodeKind(i->getKind());
    if (!kind)
      return PILDynamicCastInst();
    switch (kind.getValue()) {
#define DYNAMICCAST_INST(ID, PARENT)                                           \
  case PILDynamicCastKind::ID:                                                 \
    return PILDynamicCastInst(cast<ID>(node));
#include "polarphp/pil/lang/PILNodesDef.h"
    }
  }

  PILDynamicCastKind getKind() const {
    return PILDynamicCastKind(inst->getKind());
  }

  explicit operator bool() const { return inst != nullptr; }

  PILInstruction *getInstruction() const { return inst; }

  CastConsumptionKind getBridgedConsumptionKind() const {
    switch (getKind()) {
    case PILDynamicCastKind::CheckedCastAddrBranchInst:
      return cast<CheckedCastAddrBranchInst>(inst)->getConsumptionKind();
    // TODO: Bridged casts cannot be expressed by checked_cast_br or
    // checked_cast_value_br yet. Should we ever support it, please
    // review this code.
    case PILDynamicCastKind::CheckedCastBranchInst:
    case PILDynamicCastKind::CheckedCastValueBranchInst:
      return CastConsumptionKind::CopyOnSuccess;
    case PILDynamicCastKind::UnconditionalCheckedCastAddrInst:
      return CastConsumptionKind::TakeAlways;
    case PILDynamicCastKind::UnconditionalCheckedCastInst:
      return CastConsumptionKind::CopyOnSuccess;
    case PILDynamicCastKind::UnconditionalCheckedCastValueInst:
      llvm_unreachable("unsupported");
    }
    llvm_unreachable("covered switch");
  }

  CastConsumptionKind getConsumptionKind() const {
    switch (getKind()) {
    case PILDynamicCastKind::CheckedCastAddrBranchInst:
    case PILDynamicCastKind::CheckedCastBranchInst:
    case PILDynamicCastKind::CheckedCastValueBranchInst:
    case PILDynamicCastKind::UnconditionalCheckedCastAddrInst:
    case PILDynamicCastKind::UnconditionalCheckedCastInst:
    case PILDynamicCastKind::UnconditionalCheckedCastValueInst:
      llvm_unreachable("unsupported");
    }
  }

  PILBasicBlock *getSuccessBlock() {
    switch (getKind()) {
    case PILDynamicCastKind::CheckedCastAddrBranchInst:
      return cast<CheckedCastAddrBranchInst>(inst)->getSuccessBB();
    case PILDynamicCastKind::CheckedCastBranchInst:
      return cast<CheckedCastBranchInst>(inst)->getSuccessBB();
    case PILDynamicCastKind::CheckedCastValueBranchInst:
      return cast<CheckedCastValueBranchInst>(inst)->getSuccessBB();
    case PILDynamicCastKind::UnconditionalCheckedCastAddrInst:
    case PILDynamicCastKind::UnconditionalCheckedCastInst:
      return nullptr;
    case PILDynamicCastKind::UnconditionalCheckedCastValueInst:
      llvm_unreachable("unsupported");
    }
    llvm_unreachable("covered switch");
  }

  Optional<ProfileCounter> getSuccessBlockCount() {
    switch (getKind()) {
    case PILDynamicCastKind::CheckedCastAddrBranchInst:
      llvm_unreachable("unsupported");
    case PILDynamicCastKind::CheckedCastBranchInst:
      return cast<CheckedCastBranchInst>(inst)->getTrueBBCount();
    case PILDynamicCastKind::CheckedCastValueBranchInst:
      llvm_unreachable("unsupported");
    case PILDynamicCastKind::UnconditionalCheckedCastAddrInst:
    case PILDynamicCastKind::UnconditionalCheckedCastInst:
      return None;
    case PILDynamicCastKind::UnconditionalCheckedCastValueInst:
      llvm_unreachable("unsupported");
    }
    llvm_unreachable("covered switch");
  }

  const PILBasicBlock *getSuccessBlock() const {
    return const_cast<PILDynamicCastInst &>(*this).getSuccessBlock();
  }

  PILBasicBlock *getFailureBlock() {
    switch (getKind()) {
    case PILDynamicCastKind::CheckedCastAddrBranchInst:
      return cast<CheckedCastAddrBranchInst>(inst)->getFailureBB();
    case PILDynamicCastKind::CheckedCastBranchInst:
      return cast<CheckedCastBranchInst>(inst)->getFailureBB();
    case PILDynamicCastKind::CheckedCastValueBranchInst:
      return cast<CheckedCastValueBranchInst>(inst)->getFailureBB();
    case PILDynamicCastKind::UnconditionalCheckedCastAddrInst:
    case PILDynamicCastKind::UnconditionalCheckedCastInst:
      return nullptr;
    case PILDynamicCastKind::UnconditionalCheckedCastValueInst:
      llvm_unreachable("unsupported");
    }
    llvm_unreachable("covered switch");
  }

  Optional<ProfileCounter> getFailureBlockCount() {
    switch (getKind()) {
    case PILDynamicCastKind::CheckedCastAddrBranchInst:
      llvm_unreachable("unsupported");
    case PILDynamicCastKind::CheckedCastBranchInst:
      return cast<CheckedCastBranchInst>(inst)->getFalseBBCount();
    case PILDynamicCastKind::CheckedCastValueBranchInst:
      llvm_unreachable("unsupported");
    case PILDynamicCastKind::UnconditionalCheckedCastAddrInst:
    case PILDynamicCastKind::UnconditionalCheckedCastInst:
      return None;
    case PILDynamicCastKind::UnconditionalCheckedCastValueInst:
      llvm_unreachable("unsupported");
    }
    llvm_unreachable("covered switch");
  }

  const PILBasicBlock *getFailureBlock() const {
    return const_cast<PILDynamicCastInst &>(*this).getFailureBlock();
  }

  PILValue getSource() const {
    switch (getKind()) {
    case PILDynamicCastKind::CheckedCastAddrBranchInst:
      return cast<CheckedCastAddrBranchInst>(inst)->getSrc();
    case PILDynamicCastKind::CheckedCastBranchInst:
      return cast<CheckedCastBranchInst>(inst)->getOperand();
    case PILDynamicCastKind::CheckedCastValueBranchInst:
      return cast<CheckedCastValueBranchInst>(inst)->getOperand();
    case PILDynamicCastKind::UnconditionalCheckedCastAddrInst:
      return cast<UnconditionalCheckedCastAddrInst>(inst)->getSrc();
    case PILDynamicCastKind::UnconditionalCheckedCastInst:
      return cast<UnconditionalCheckedCastInst>(inst)->getOperand();
    case PILDynamicCastKind::UnconditionalCheckedCastValueInst:
      llvm_unreachable("unsupported");
    }
    llvm_unreachable("covered switch");
  }

  // Returns the success value.
  PILValue getDest() const {
    switch (getKind()) {
    case PILDynamicCastKind::CheckedCastAddrBranchInst:
      return cast<CheckedCastAddrBranchInst>(inst)->getDest();
    case PILDynamicCastKind::CheckedCastBranchInst:
    case PILDynamicCastKind::CheckedCastValueBranchInst:
      // TODO: Shouldn't this return getSuccessBlock()->getArgument(0)?
      return PILValue();
    case PILDynamicCastKind::UnconditionalCheckedCastAddrInst:
      return cast<UnconditionalCheckedCastAddrInst>(inst)->getDest();
    case PILDynamicCastKind::UnconditionalCheckedCastInst:
      // TODO: Why isn't this:
      //
      // return cast<UnconditionalCheckedCastInst>(inst);
      return PILValue();
    case PILDynamicCastKind::UnconditionalCheckedCastValueInst:
      llvm_unreachable("unimplemented");
    }
    llvm_unreachable("covered switch");
  }

  CanType getSourceFormalType() const {
    switch (getKind()) {
    case PILDynamicCastKind::CheckedCastAddrBranchInst:
      return cast<CheckedCastAddrBranchInst>(inst)->getSourceFormalType();
    case PILDynamicCastKind::CheckedCastBranchInst:
      return cast<CheckedCastBranchInst>(inst)->getSourceFormalType();
    case PILDynamicCastKind::CheckedCastValueBranchInst:
      return cast<CheckedCastValueBranchInst>(inst)->getSourceFormalType();
    case PILDynamicCastKind::UnconditionalCheckedCastAddrInst:
      return cast<UnconditionalCheckedCastAddrInst>(inst)->getSourceFormalType();
    case PILDynamicCastKind::UnconditionalCheckedCastInst:
      return cast<UnconditionalCheckedCastInst>(inst)->getSourceFormalType();
    case PILDynamicCastKind::UnconditionalCheckedCastValueInst:
      return cast<UnconditionalCheckedCastValueInst>(inst)->getSourceFormalType();
    }
    llvm_unreachable("covered switch");
  }

  PILType getSourceLoweredType() const {
    switch (getKind()) {
    case PILDynamicCastKind::CheckedCastAddrBranchInst:
      return cast<CheckedCastAddrBranchInst>(inst)->getSourceLoweredType();
    case PILDynamicCastKind::CheckedCastBranchInst:
      return cast<CheckedCastBranchInst>(inst)->getSourceLoweredType();
    case PILDynamicCastKind::CheckedCastValueBranchInst:
      return cast<CheckedCastValueBranchInst>(inst)->getSourceLoweredType();
    case PILDynamicCastKind::UnconditionalCheckedCastAddrInst:
      return cast<UnconditionalCheckedCastAddrInst>(inst)->getSourceLoweredType();
    case PILDynamicCastKind::UnconditionalCheckedCastInst:
      return cast<UnconditionalCheckedCastInst>(inst)->getSourceLoweredType();
    case PILDynamicCastKind::UnconditionalCheckedCastValueInst:
      return cast<UnconditionalCheckedCastValueInst>(inst)->getSourceLoweredType();
    }
  }

  CanType getTargetFormalType() const {
    switch (getKind()) {
    case PILDynamicCastKind::CheckedCastAddrBranchInst:
      return cast<CheckedCastAddrBranchInst>(inst)->getTargetFormalType();
    case PILDynamicCastKind::CheckedCastBranchInst:
      return cast<CheckedCastBranchInst>(inst)->getTargetFormalType();
    case PILDynamicCastKind::CheckedCastValueBranchInst:
      return cast<CheckedCastValueBranchInst>(inst)->getTargetFormalType();
    case PILDynamicCastKind::UnconditionalCheckedCastAddrInst:
      return cast<UnconditionalCheckedCastAddrInst>(inst)->getTargetFormalType();
    case PILDynamicCastKind::UnconditionalCheckedCastInst:
      return cast<UnconditionalCheckedCastInst>(inst)->getTargetFormalType();
    case PILDynamicCastKind::UnconditionalCheckedCastValueInst:
      return cast<UnconditionalCheckedCastValueInst>(inst)->getTargetFormalType();
    }
    llvm_unreachable("covered switch");
  }

  PILType getTargetLoweredType() const {
    switch (getKind()) {
    case PILDynamicCastKind::CheckedCastAddrBranchInst:
      return cast<CheckedCastAddrBranchInst>(inst)->getDest()->getType();
    case PILDynamicCastKind::CheckedCastBranchInst:
      return cast<CheckedCastBranchInst>(inst)->getTargetLoweredType();
    case PILDynamicCastKind::CheckedCastValueBranchInst:
      return cast<CheckedCastValueBranchInst>(inst)->getTargetLoweredType();
    case PILDynamicCastKind::UnconditionalCheckedCastAddrInst:
      return cast<UnconditionalCheckedCastAddrInst>(inst)->getDest()->getType();
    case PILDynamicCastKind::UnconditionalCheckedCastInst:
      return cast<UnconditionalCheckedCastInst>(inst)->getTargetLoweredType();
    case PILDynamicCastKind::UnconditionalCheckedCastValueInst:
      return cast<UnconditionalCheckedCastValueInst>(inst)->getTargetLoweredType();
    }
    llvm_unreachable("covered switch");
  }

  bool isSourceTypeExact() const {
    switch (getKind()) {
    case PILDynamicCastKind::CheckedCastValueBranchInst:
    case PILDynamicCastKind::CheckedCastBranchInst:
    case PILDynamicCastKind::CheckedCastAddrBranchInst:
    case PILDynamicCastKind::UnconditionalCheckedCastAddrInst:
    case PILDynamicCastKind::UnconditionalCheckedCastInst:
      return isa<MetatypeInst>(getSource());
    case PILDynamicCastKind::UnconditionalCheckedCastValueInst:
      llvm_unreachable("unsupported");
    }
    llvm_unreachable("covered switch");
  }

  PILLocation getLocation() const { return inst->getLoc(); }

  PILModule &getModule() const { return inst->getModule(); }
  PILFunction *getFunction() const { return inst->getFunction(); }

  DynamicCastFeasibility classifyFeasibility(bool allowWholeModule) const {
    return polar::classifyDynamicCast(
        getModule().getTypePHPModule(),
        getSourceFormalType(), getTargetFormalType(),
        isSourceTypeExact(), allowWholeModule && getModule().isWholeModule());
  }

  bool isBridgingCast() const {
    // Bridging casts cannot be further simplified.
    auto TargetIsBridgeable = getTargetFormalType()->isBridgeableObjectType();
    auto SourceIsBridgeable = getSourceFormalType()->isBridgeableObjectType();
    return TargetIsBridgeable != SourceIsBridgeable;
  }

  /// If getSourceType() is a Polarphp type that can bridge to an ObjC type, return
  /// the ObjC type it bridges to. If the source type is an objc type, an empty
  /// CanType() is returned.
//  CanType getBridgedSourceType() const {
//    PILModule &mod = getModule();
//    Type t = mod.getAstContext().getBridgedToObjC(mod.getTypePHPModule(),
//                                                  getSourceFormalType());
//    if (!t)
//      return CanType();
//    return t->getCanonicalType();
//  }

   /// @todo
  /// If getTargetType() is a Polarphp type that can bridge to an ObjC type, return
  /// the ObjC type it bridges to. If the target type is an objc type, an empty
  /// CanType() is returned.
//  CanType getBridgedTargetType() const {
//    PILModule &mod = getModule();
//    Type t = mod.getAstContext().getBridgedToObjC(mod.getTypePHPModule(),
//                                                  getTargetFormalType());
//    if (!t)
//      return CanType();
//    return t->getCanonicalType();
//  }

//  Optional<PILType> getLoweredBridgedTargetObjectType() const {
//    CanType t = getBridgedTargetType();
//    if (!t)
//      return None;
//    return PILType::getPrimitiveObjectType(t);
//  }

  bool isConditional() const {
    switch (getKind()) {
    case PILDynamicCastKind::CheckedCastAddrBranchInst: {
      auto f = classifyFeasibility(true /*allow wmo*/);
      return f == DynamicCastFeasibility::MaySucceed;
    }
    case PILDynamicCastKind::CheckedCastBranchInst: {
      auto f = classifyFeasibility(false /*allow wmo*/);
      return f == DynamicCastFeasibility::MaySucceed;
    }
    case PILDynamicCastKind::CheckedCastValueBranchInst: {
      auto f = classifyFeasibility(false /*allow wmo opts*/);
      return f == DynamicCastFeasibility::MaySucceed;
    }
    case PILDynamicCastKind::UnconditionalCheckedCastAddrInst:
    case PILDynamicCastKind::UnconditionalCheckedCastInst:
      return false;
    case PILDynamicCastKind::UnconditionalCheckedCastValueInst:
      llvm_unreachable("unsupported");
    }
    llvm_unreachable("covered switch");
  }

  bool canUseScalarCheckedCastInstructions() const {
    return polar::canUseScalarCheckedCastInstructions(
        getModule(), getSourceFormalType(), getTargetFormalType());
  }
};

} // end namespace polar

#endif

