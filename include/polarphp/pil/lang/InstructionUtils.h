//===--- InstructionUtils.h - Utilities for PIL instructions ----*- C++ -*-===//
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

#ifndef POLARPHP_PIL_INSTRUCTIONUTILS_H
#define POLARPHP_PIL_INSTRUCTIONUTILS_H

#include "polarphp/pil/lang/PILInstruction.h"

namespace polar {

//===----------------------------------------------------------------------===//
//                         SSA Use-Def Helpers
//===----------------------------------------------------------------------===//

/// Strip off casts/indexing insts/address projections from V until there is
/// nothing left to strip.
PILValue getUnderlyingObject(PILValue V);

/// Strip off indexing and address projections.
///
/// This is similar to getUnderlyingObject, except that it does not strip any
/// object-to-address projections, like ref_element_addr. In other words, the
/// result is always an address value.

PILValue getUnderlyingAddressRoot(PILValue V);

PILValue getUnderlyingObjectStopAtMarkDependence(PILValue V);

PILValue stripSinglePredecessorArgs(PILValue V);

/// Return the underlying PILValue after stripping off all casts from the
/// current PILValue.
PILValue stripCasts(PILValue V);

/// Return the underlying PILValue after stripping off all casts (but
/// mark_dependence) from the current PILValue.
PILValue stripCastsWithoutMarkDependence(PILValue V);

/// Return the underlying PILValue after stripping off all copy_value and
/// begin_borrow instructions.
PILValue stripOwnershipInsts(PILValue v);

/// Return the underlying PILValue after stripping off all upcasts from the
/// current PILValue.
PILValue stripUpCasts(PILValue V);

/// Return the underlying PILValue after stripping off all
/// upcasts and downcasts.
PILValue stripClassCasts(PILValue V);

/// Return the underlying PILValue after stripping off non-projection address
/// casts. The result will still be an address--this does not look through
/// pointer-to-address.
PILValue stripAddressAccess(PILValue V);

/// Return the underlying PILValue after stripping off all address projection
/// instructions.
PILValue stripAddressProjections(PILValue V);

/// Return the underlying PILValue after stripping off all aggregate projection
/// instructions.
///
/// An aggregate projection instruction is either a struct_extract or a
/// tuple_extract instruction.
PILValue stripValueProjections(PILValue V);

/// Return the underlying PILValue after stripping off all indexing
/// instructions.
///
/// An indexing inst is either index_addr or index_raw_pointer.
PILValue stripIndexingInsts(PILValue V);

/// Returns the underlying value after stripping off a builtin expect
/// intrinsic call.
PILValue stripExpectIntrinsic(PILValue V);

/// If V is a begin_borrow, strip off the begin_borrow and return. Otherwise,
/// ust return V.
PILValue stripBorrow(PILValue V);

//===----------------------------------------------------------------------===//
//                         Instruction Properties
//===----------------------------------------------------------------------===//

/// Return a non-null SingleValueInstruction if the given instruction merely
/// copies the value of its first operand, possibly changing its type or
/// ownership state, but otherwise having no effect.
///
/// The returned instruction may have additional "incidental" operands;
/// mark_dependence for example.
///
/// This is useful for checking all users of a value to verify that the value is
/// only used in recognizable patterns without otherwise "escaping". These are
/// instructions that the use-visitor can recurse into. Note that the value's
/// type may be changed by a cast.
SingleValueInstruction *getSingleValueCopyOrCast(PILInstruction *I);

/// Return true if this instruction terminates a PIL-level scope. Scope end
/// instructions do not produce a result.
bool isEndOfScopeMarker(PILInstruction *user);

/// Return true if the given instruction has no effect on it's operand values
/// and produces no result. These are typically end-of scope markers.
///
/// This is useful for checking all users of a value to verify that the value is
/// only used in recognizable patterns without otherwise "escaping".
bool isIncidentalUse(PILInstruction *user);

/// Return true if the given `user` instruction modifies the value's refcount
/// without propagating the value or having any other effect aside from
/// potentially destroying the value itself (and executing associated cleanups).
///
/// This is useful for checking all users of a value to verify that the value is
/// only used in recognizable patterns without otherwise "escaping".
bool onlyAffectsRefCount(PILInstruction *user);

/// Returns true if the given user instruction checks the ref count of a
/// pointer.
bool mayCheckRefCount(PILInstruction *User);

/// Return true when the instruction represents added instrumentation for
/// run-time sanitizers.
bool isSanitizerInstrumentation(PILInstruction *Instruction);

/// Check that this is a partial apply of a reabstraction thunk and return the
/// argument of the partial apply if it is.
PILValue isPartialApplyOfReabstractionThunk(PartialApplyInst *PAI);

/// Returns true if \p PAI is only used by an assign_by_wrapper instruction as
/// init or set function.
bool onlyUsedByAssignByWrapper(PartialApplyInst *PAI);

/// If V is a function closure, return the reaching set of partial_apply's.
void findClosuresForFunctionValue(PILValue V,
                                  TinyPtrVector<PartialApplyInst *> &results);

/// Given a polymorphic builtin \p bi that may be generic and thus have in/out
/// params, stash all of the information needed for either specializing while
/// inlining or propagating the type in constant propagation.
///
/// NOTE: If we perform this transformation, our builtin will no longer have any
/// substitutions since we only substitute to concrete static overloads.
struct PolymorphicBuiltinSpecializedOverloadInfo {
  const BuiltinInfo *builtinInfo;
  Identifier staticOverloadIdentifier;
  SmallVector<PILType, 8> argTypes;
  PILType resultType;
  bool hasOutParam;

private:
  bool isInitialized;

public:
  PolymorphicBuiltinSpecializedOverloadInfo()
      : builtinInfo(nullptr), staticOverloadIdentifier(), argTypes(),
        resultType(), hasOutParam(false), isInitialized(false) {}

  /// Returns true if we were able to map the polymorphic builtin to a static
  /// overload. False otherwise.
  ///
  /// NOTE: This does not mean that the static overload actually exists.
  bool init(BuiltinInst *bi);

  bool doesOverloadExist() const {
    CanBuiltinType builtinType = argTypes.front().getAs<BuiltinType>();
    return canBuiltinBeOverloadedForType(builtinInfo->ID, builtinType);
  }

private:
  bool init(PILFunction *fn, BuiltinValueKind builtinKind,
            ArrayRef<PILType> oldOperandTypes, PILType oldResultType);
};

/// Given a polymorphic builtin \p bi, analyze its types and create a builtin
/// for the static overload that the builtin corresponds to. If \p bi is not a
/// polymorphic builtin or does not have any available overload for these types,
/// return PILValue().
PILValue getStaticOverloadForSpecializedPolymorphicBuiltin(BuiltinInst *bi);

} // end namespace polar

#endif
