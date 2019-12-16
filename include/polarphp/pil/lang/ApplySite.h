//===--- ApplySite.h -------------------------------------*- mode: c++ -*--===//
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
///
/// \file
///
/// This file defines utilities for working with "call-site like" PIL
/// instructions. We use the term "call-site" like since we handle partial
/// applications in our utilities.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_APPLYSITE_H
#define POLARPHP_PIL_APPLYSITE_H

#include "polarphp/pil/lang/PILBasicBlock.h"
#include "polarphp/pil/lang/PILInstruction.h"

namespace polar {

//===----------------------------------------------------------------------===//
//                                 ApplySite
//===----------------------------------------------------------------------===//

struct ApplySiteKind {
   enum innerty : std::underlying_type<PILInstructionKind>::type {
#define APPLYSITE_INST(ID, PARENT) ID = unsigned(PILInstructionKind::ID),
#include "polarphp/pil/lang/PILNodesDef.h"
   } value;

   explicit ApplySiteKind(PILInstructionKind kind) {
      auto newValue = ApplySiteKind::fromNodeKindHelper(kind);
      assert(newValue && "Non apply site passed into ApplySiteKind");
      value = newValue.getValue();
   }

   ApplySiteKind(innerty value) : value(value) {}
   operator innerty() const { return value; }

   static Optional<ApplySiteKind> fromNodeKind(PILInstructionKind kind) {
      if (auto innerTyOpt = ApplySiteKind::fromNodeKindHelper(kind))
         return ApplySiteKind(*innerTyOpt);
      return None;
   }

private:
   static Optional<innerty> fromNodeKindHelper(PILInstructionKind kind) {
      switch (kind) {
#define APPLYSITE_INST(ID, PARENT)                                             \
  case PILInstructionKind::ID:                                                 \
    return ApplySiteKind::ID;
#include "polarphp/pil/lang/PILNodesDef.h"
         default:
            return None;
      }
   }
};

/// An apply instruction.
class ApplySite {
   PILInstruction *Inst;

protected:
   explicit ApplySite(void *p) : Inst(static_cast<PILInstruction *>(p)) {}

public:
   ApplySite() : Inst(nullptr) {}
   explicit ApplySite(PILInstruction *inst)
      : Inst(static_cast<PILInstruction *>(inst)) {
      assert(classof(inst) && "not an apply instruction?");
   }
   ApplySite(ApplyInst *inst) : Inst(inst) {}
   ApplySite(PartialApplyInst *inst) : Inst(inst) {}
   ApplySite(TryApplyInst *inst) : Inst(inst) {}
   ApplySite(BeginApplyInst *inst) : Inst(inst) {}

   PILModule &getModule() const { return Inst->getModule(); }

   static ApplySite isa(PILNode *node) {
      auto *i = dyn_cast<PILInstruction>(node);
      if (!i)
         return ApplySite();

      auto kind = ApplySiteKind::fromNodeKind(i->getKind());
      if (!kind)
         return ApplySite();

      switch (kind.getValue()) {
         case ApplySiteKind::ApplyInst:
            return ApplySite(cast<ApplyInst>(node));
         case ApplySiteKind::BeginApplyInst:
            return ApplySite(cast<BeginApplyInst>(node));
         case ApplySiteKind::TryApplyInst:
            return ApplySite(cast<TryApplyInst>(node));
         case ApplySiteKind::PartialApplyInst:
            return ApplySite(cast<PartialApplyInst>(node));
      }
      llvm_unreachable("covered switch");
   }

   ApplySiteKind getKind() const { return ApplySiteKind(Inst->getKind()); }

   explicit operator bool() const { return Inst != nullptr; }

   PILInstruction *getInstruction() const { return Inst; }
   PILLocation getLoc() const { return Inst->getLoc(); }
   const PILDebugScope *getDebugScope() const { return Inst->getDebugScope(); }
   PILFunction *getFunction() const { return Inst->getFunction(); }
   PILBasicBlock *getParent() const { return Inst->getParent(); }

#define FOREACH_IMPL_RETURN(OPERATION)                                         \
  do {                                                                         \
    switch (ApplySiteKind(Inst->getKind())) {                                  \
    case ApplySiteKind::ApplyInst:                                             \
      return cast<ApplyInst>(Inst)->OPERATION;                                 \
    case ApplySiteKind::BeginApplyInst:                                        \
      return cast<BeginApplyInst>(Inst)->OPERATION;                            \
    case ApplySiteKind::PartialApplyInst:                                      \
      return cast<PartialApplyInst>(Inst)->OPERATION;                          \
    case ApplySiteKind::TryApplyInst:                                          \
      return cast<TryApplyInst>(Inst)->OPERATION;                              \
    }                                                                          \
    llvm_unreachable("covered switch");                                        \
  } while (0)

   /// Return the callee operand as a value.
   PILValue getCallee() const { return getCalleeOperand()->get(); }

   /// Return the callee operand.
   const Operand *getCalleeOperand() const {
      FOREACH_IMPL_RETURN(getCalleeOperand());
   }

   /// Return the callee value by looking through function conversions until we
   /// find a function_ref, partial_apply, or unrecognized callee value.
   PILValue getCalleeOrigin() const { FOREACH_IMPL_RETURN(getCalleeOrigin()); }

   /// Gets the referenced function by looking through partial apply,
   /// convert_function, and thin to thick function until we find a function_ref.
   PILFunction *getCalleeFunction() const {
      FOREACH_IMPL_RETURN(getCalleeFunction());
   }

   /// Return the referenced function if the callee is a function_ref
   /// instruction.
   PILFunction *getReferencedFunctionOrNull() const {
      FOREACH_IMPL_RETURN(getReferencedFunctionOrNull());
   }

   /// Return the referenced function if the callee is a function_ref like
   /// instruction.
   ///
   /// WARNING: This not necessarily the function that will be called at runtime.
   /// If the callee is a (prev_)dynamic_function_ref the actual function called
   /// might be different because it could be dynamically replaced at runtime.
   ///
   /// If the client of this API wants to look at the content of the returned PIL
   /// function it should call getReferencedFunctionOrNull() instead.
   PILFunction *getInitiallyReferencedFunction() const {
      FOREACH_IMPL_RETURN(getInitiallyReferencedFunction());
   }

   /// Should we optimize this call.
   /// Calls to (previous_)dynamic_function_ref have a dynamic target function so
   /// we should not optimize them.
   bool canOptimize() const {
      return !DynamicFunctionRefInst::classof(getCallee()) &&
             !PreviousDynamicFunctionRefInst::classof(getCallee());
   }

   /// Return the type.
   PILType getType() const { return getSubstCalleeConv().getPILResultType(); }

   /// Get the type of the callee without the applied substitutions.
   CanPILFunctionType getOrigCalleeType() const {
      return getCallee()->getType().castTo<PILFunctionType>();
   }
   /// Get the conventions of the callee without the applied substitutions.
   PILFunctionConventions getOrigCalleeConv() const {
      return PILFunctionConventions(getOrigCalleeType(), getModule());
   }

   /// Get the type of the callee with the applied substitutions.
   CanPILFunctionType getSubstCalleeType() const {
      return getSubstCalleePILType().castTo<PILFunctionType>();
   }
   PILType getSubstCalleePILType() const {
      FOREACH_IMPL_RETURN(getSubstCalleePILType());
   }
   /// Get the conventions of the callee with the applied substitutions.
   PILFunctionConventions getSubstCalleeConv() const {
      return PILFunctionConventions(getSubstCalleeType(), getModule());
   }

   /// Returns true if the callee function is annotated with
   /// @_semantics("programtermination_point")
   bool isCalleeKnownProgramTerminationPoint() const {
      FOREACH_IMPL_RETURN(isCalleeKnownProgramTerminationPoint());
   }

   /// Check if this is a call of a never-returning function.
   bool isCalleeNoReturn() const { FOREACH_IMPL_RETURN(isCalleeNoReturn()); }

   bool isCalleeThin() const {
      switch (getSubstCalleeType()->getRepresentation()) {
         case PILFunctionTypeRepresentation::CFunctionPointer:
         case PILFunctionTypeRepresentation::Thin:
         case PILFunctionTypeRepresentation::Method:
         case PILFunctionTypeRepresentation::ObjCMethod:
         case PILFunctionTypeRepresentation::WitnessMethod:
         case PILFunctionTypeRepresentation::Closure:
            return true;
         case PILFunctionTypeRepresentation::Block:
         case PILFunctionTypeRepresentation::Thick:
            return false;
      }
   }

   /// True if this application has generic substitutions.
   bool hasSubstitutions() const { FOREACH_IMPL_RETURN(hasSubstitutions()); }

   /// The substitutions used to bind the generic arguments of this function.
   SubstitutionMap getSubstitutionMap() const {
      FOREACH_IMPL_RETURN(getSubstitutionMap());
   }

   /// Return the associated specialization information.
   const GenericSpecializationInformation *getSpecializationInfo() const {
      FOREACH_IMPL_RETURN(getSpecializationInfo());
   }

   /// Return an operand list corresponding to the applied arguments.
   MutableArrayRef<Operand> getArgumentOperands() const {
      FOREACH_IMPL_RETURN(getArgumentOperands());
   }

   /// Return a list of applied argument values.
   OperandValueArrayRef getArguments() const {
      FOREACH_IMPL_RETURN(getArguments());
   }

   /// Return the number of applied arguments.
   unsigned getNumArguments() const { FOREACH_IMPL_RETURN(getNumArguments()); }

   /// Return the apply operand for the given applied argument index.
   Operand &getArgumentRef(unsigned i) const { return getArgumentOperands()[i]; }

   /// Return the ith applied argument.
   PILValue getArgument(unsigned i) const { return getArguments()[i]; }

   /// Set the ith applied argument.
   void setArgument(unsigned i, PILValue V) const {
      getArgumentOperands()[i].set(V);
   }

   /// Return the operand index of the first applied argument.
   unsigned getOperandIndexOfFirstArgument() const {
      FOREACH_IMPL_RETURN(getArgumentOperandNumber());
   }
#undef FOREACH_IMPL_RETURN

   /// Returns true if \p oper is an argument operand and not the callee
   /// operand.
   bool isArgumentOperand(const Operand &oper) const {
      return oper.getOperandNumber() >= getOperandIndexOfFirstArgument() &&
             oper.getOperandNumber() < getOperandIndexOfFirstArgument() + getNumArguments();
   }

   /// Return the applied argument index for the given operand.
   unsigned getAppliedArgIndex(const Operand &oper) const {
      assert(oper.getUser() == Inst);
      assert(isArgumentOperand(oper));

      return oper.getOperandNumber() - getOperandIndexOfFirstArgument();
   }

   /// Return the callee's function argument index corresponding to the first
   /// applied argument: 0 for full applies; >= 0 for partial applies.
   unsigned getCalleeArgIndexOfFirstAppliedArg() const {
      switch (ApplySiteKind(Inst->getKind())) {
         case ApplySiteKind::ApplyInst:
         case ApplySiteKind::BeginApplyInst:
         case ApplySiteKind::TryApplyInst:
            return 0;
         case ApplySiteKind::PartialApplyInst:
            // The arguments to partial_apply are a suffix of the partial_apply's
            // callee. Note that getSubstCalleeConv is function type of the callee
            // argument passed to this apply, not necessarilly the function type of
            // the underlying callee function (i.e. it is based on the `getCallee`
            // type, not the `getCalleeOrigin` type).
            //
            // pa1 = partial_apply f(c) : $(a, b, c)
            // pa2 = partial_apply pa1(b) : $(a, b)
            // apply pa2(a)
            return getSubstCalleeConv().getNumPILArguments() - getNumArguments();
      }
      llvm_unreachable("covered switch");
   }

   /// Return the callee's function argument index corresponding to the given
   /// apply operand. Each function argument index identifies a
   /// PILFunctionArgument in the callee and can be used as a
   /// PILFunctionConvention argument index.
   ///
   /// Note: Passing an applied argument index into PILFunctionConvention, as
   /// opposed to a function argument index, is incorrect.
   unsigned getCalleeArgIndex(const Operand &oper) const {
      return getCalleeArgIndexOfFirstAppliedArg() + getAppliedArgIndex(oper);
   }

   /// Return the PILArgumentConvention for the given applied argument operand.
   PILArgumentConvention getArgumentConvention(Operand &oper) const {
      unsigned calleeArgIdx =
         getCalleeArgIndexOfFirstAppliedArg() + getAppliedArgIndex(oper);
      return getSubstCalleeConv().getPILArgumentConvention(calleeArgIdx);
   }

   /// Return true if 'self' is an applied argument.
   bool hasSelfArgument() const {
      switch (ApplySiteKind(Inst->getKind())) {
         case ApplySiteKind::ApplyInst:
            return cast<ApplyInst>(Inst)->hasSelfArgument();
         case ApplySiteKind::BeginApplyInst:
            return cast<BeginApplyInst>(Inst)->hasSelfArgument();
         case ApplySiteKind::TryApplyInst:
            return cast<TryApplyInst>(Inst)->hasSelfArgument();
         case ApplySiteKind::PartialApplyInst:
            llvm_unreachable("unhandled case");
      }
      llvm_unreachable("covered switch");
   }

   /// Return the applied 'self' argument value.
   PILValue getSelfArgument() const {
      switch (ApplySiteKind(Inst->getKind())) {
         case ApplySiteKind::ApplyInst:
            return cast<ApplyInst>(Inst)->getSelfArgument();
         case ApplySiteKind::BeginApplyInst:
            return cast<BeginApplyInst>(Inst)->getSelfArgument();
         case ApplySiteKind::TryApplyInst:
            return cast<TryApplyInst>(Inst)->getSelfArgument();
         case ApplySiteKind::PartialApplyInst:
            llvm_unreachable("unhandled case");
      }
      llvm_unreachable("covered switch");
   }

   /// Return the 'self' apply operand.
   Operand &getSelfArgumentOperand() {
      switch (ApplySiteKind(Inst->getKind())) {
         case ApplySiteKind::ApplyInst:
            return cast<ApplyInst>(Inst)->getSelfArgumentOperand();
         case ApplySiteKind::BeginApplyInst:
            return cast<BeginApplyInst>(Inst)->getSelfArgumentOperand();
         case ApplySiteKind::TryApplyInst:
            return cast<TryApplyInst>(Inst)->getSelfArgumentOperand();
         case ApplySiteKind::PartialApplyInst:
            llvm_unreachable("Unhandled cast");
      }
      llvm_unreachable("covered switch");
   }

   /// Return a list of applied arguments without self.
   OperandValueArrayRef getArgumentsWithoutSelf() const {
      switch (ApplySiteKind(Inst->getKind())) {
         case ApplySiteKind::ApplyInst:
            return cast<ApplyInst>(Inst)->getArgumentsWithoutSelf();
         case ApplySiteKind::BeginApplyInst:
            return cast<BeginApplyInst>(Inst)->getArgumentsWithoutSelf();
         case ApplySiteKind::TryApplyInst:
            return cast<TryApplyInst>(Inst)->getArgumentsWithoutSelf();
         case ApplySiteKind::PartialApplyInst:
            llvm_unreachable("Unhandled case");
      }
      llvm_unreachable("covered switch");
   }

   /// Return whether the given apply is of a formally-throwing function
   /// which is statically known not to throw.
   bool isNonThrowing() const {
      switch (ApplySiteKind(getInstruction()->getKind())) {
         case ApplySiteKind::ApplyInst:
            return cast<ApplyInst>(Inst)->isNonThrowing();
         case ApplySiteKind::BeginApplyInst:
            return cast<BeginApplyInst>(Inst)->isNonThrowing();
         case ApplySiteKind::TryApplyInst:
            return false;
         case ApplySiteKind::PartialApplyInst:
            llvm_unreachable("Unhandled case");
      }
   }

   /// If this is a terminator apply site, then pass the first instruction of
   /// each successor to fun. Otherwise, pass std::next(Inst).
   ///
   /// The intention is that this abstraction will enable the compiler writer to
   /// ignore whether or not an apply site is a terminator when inserting
   /// instructions after an apply site. This results in eliminating unnecessary
   /// if-else code otherwise required to handle such situations.
   void insertAfter(llvm::function_ref<void(PILBasicBlock::iterator)> func) {
      auto *ti = dyn_cast<TermInst>(Inst);
      if (!ti) {
         return func(std::next(Inst->getIterator()));
      }

      for (auto *succBlock : ti->getSuccessorBlocks()) {
         func(succBlock->begin());
      }
   }

   static ApplySite getFromOpaqueValue(void *p) { return ApplySite(p); }

   friend bool operator==(ApplySite lhs, ApplySite rhs) {
      return lhs.getInstruction() == rhs.getInstruction();
   }
   friend bool operator!=(ApplySite lhs, ApplySite rhs) {
      return lhs.getInstruction() != rhs.getInstruction();
   }

   static bool classof(const PILInstruction *inst) {
      return bool(ApplySiteKind::fromNodeKind(inst->getKind()));
   }

   void dump() const LLVM_ATTRIBUTE_USED { getInstruction()->dump(); }
};

//===----------------------------------------------------------------------===//
//                               FullApplySite
//===----------------------------------------------------------------------===//

struct FullApplySiteKind {
   enum innerty : std::underlying_type<PILInstructionKind>::type {
#define FULLAPPLYSITE_INST(ID, PARENT) ID = unsigned(PILInstructionKind::ID),
#include "polarphp/pil/lang/PILNodesDef.h"
   } value;

   explicit FullApplySiteKind(PILInstructionKind kind) {
      auto fullApplySiteKind = FullApplySiteKind::fromNodeKindHelper(kind);
      assert(fullApplySiteKind && "PILNodeKind is not a FullApplySiteKind?!");
      value = fullApplySiteKind.getValue();
   }

   FullApplySiteKind(innerty value) : value(value) {}
   operator innerty() const { return value; }

   static Optional<FullApplySiteKind> fromNodeKind(PILInstructionKind kind) {
      if (auto innerOpt = FullApplySiteKind::fromNodeKindHelper(kind))
         return FullApplySiteKind(*innerOpt);
      return None;
   }

private:
   static Optional<innerty> fromNodeKindHelper(PILInstructionKind kind) {
      switch (kind) {
#define FULLAPPLYSITE_INST(ID, PARENT)                                         \
  case PILInstructionKind::ID:                                                 \
    return FullApplySiteKind::ID;
#include "polarphp/pil/lang/PILNodesDef.h"
         default:
            return None;
      }
   }
};

/// A full function application.
class FullApplySite : public ApplySite {
   explicit FullApplySite(void *p) : ApplySite(p) {}

public:
   FullApplySite() : ApplySite() {}
   explicit FullApplySite(PILInstruction *inst) : ApplySite(inst) {
      assert(classof(inst) && "not an apply instruction?");
   }
   FullApplySite(ApplyInst *inst) : ApplySite(inst) {}
   FullApplySite(BeginApplyInst *inst) : ApplySite(inst) {}
   FullApplySite(TryApplyInst *inst) : ApplySite(inst) {}

   static FullApplySite isa(PILNode *node) {
      auto *i = dyn_cast<PILInstruction>(node);
      if (!i)
         return FullApplySite();
      auto kind = FullApplySiteKind::fromNodeKind(i->getKind());
      if (!kind)
         return FullApplySite();
      switch (kind.getValue()) {
         case FullApplySiteKind::ApplyInst:
            return FullApplySite(cast<ApplyInst>(node));
         case FullApplySiteKind::BeginApplyInst:
            return FullApplySite(cast<BeginApplyInst>(node));
         case FullApplySiteKind::TryApplyInst:
            return FullApplySite(cast<TryApplyInst>(node));
      }
      llvm_unreachable("covered switch");
   }

   FullApplySiteKind getKind() const {
      return FullApplySiteKind(getInstruction()->getKind());
   }

   bool hasIndirectPILResults() const {
      return getSubstCalleeConv().hasIndirectPILResults();
   }

   unsigned getNumIndirectPILResults() const {
      return getSubstCalleeConv().getNumIndirectPILResults();
   }

   OperandValueArrayRef getIndirectPILResults() const {
      return getArguments().slice(0, getNumIndirectPILResults());
   }

   OperandValueArrayRef getArgumentsWithoutIndirectResults() const {
      return getArguments().slice(getNumIndirectPILResults());
   }

   /// Returns true if \p op is the callee operand of this apply site
   /// and not an argument operand.
   bool isCalleeOperand(const Operand &op) const {
      return op.getOperandNumber() < getOperandIndexOfFirstArgument();
   }

   /// Returns true if \p op is an operand that passes an indirect
   /// result argument to the apply site.
   bool isIndirectResultOperand(const Operand &op) const {
      return getCalleeArgIndex(op) < getNumIndirectPILResults();
   }

   static FullApplySite getFromOpaqueValue(void *p) { return FullApplySite(p); }

   static bool classof(const PILInstruction *inst) {
      return bool(FullApplySiteKind::fromNodeKind(inst->getKind()));
   }
};

} // namespace polar

namespace llvm {

// An ApplySite casts like a PILInstruction*.
template <> struct simplify_type<const ::polar::ApplySite> {
   using SimpleType = ::polar::PILInstruction *;
   static SimpleType getSimplifiedValue(const ::polar::ApplySite &Val) {
      return Val.getInstruction();
   }
};
template <>
struct simplify_type<::polar::ApplySite>
   : public simplify_type<const ::polar::ApplySite> {};
template <>
struct simplify_type<::polar::FullApplySite>
   : public simplify_type<const ::polar::ApplySite> {};
template <>
struct simplify_type<const ::polar::FullApplySite>
   : public simplify_type<const ::polar::ApplySite> {};

template <> struct DenseMapInfo<::polar::ApplySite> {
   static ::polar::ApplySite getEmptyKey() {
      return ::polar::ApplySite::getFromOpaqueValue(
         llvm::DenseMapInfo<void *>::getEmptyKey());
   }
   static ::polar::ApplySite getTombstoneKey() {
      return ::polar::ApplySite::getFromOpaqueValue(
         llvm::DenseMapInfo<void *>::getTombstoneKey());
   }
   static unsigned getHashValue(::polar::ApplySite AS) {
      auto *I = AS.getInstruction();
      return DenseMapInfo<::polar::PILInstruction *>::getHashValue(I);
   }
   static bool isEqual(::polar::ApplySite LHS, ::polar::ApplySite RHS) {
      return LHS == RHS;
   }
};

template <> struct DenseMapInfo<::polar::FullApplySite> {
   static ::polar::FullApplySite getEmptyKey() {
      return ::polar::FullApplySite::getFromOpaqueValue(
         llvm::DenseMapInfo<void *>::getEmptyKey());
   }
   static ::polar::FullApplySite getTombstoneKey() {
      return ::polar::FullApplySite::getFromOpaqueValue(
         llvm::DenseMapInfo<void *>::getTombstoneKey());
   }
   static unsigned getHashValue(::polar::FullApplySite AS) {
      auto *I = AS.getInstruction();
      return DenseMapInfo<::polar::PILInstruction *>::getHashValue(I);
   }
   static bool isEqual(::polar::FullApplySite LHS, ::polar::FullApplySite RHS) {
      return LHS == RHS;
   }
};

} // namespace llvm

#endif
