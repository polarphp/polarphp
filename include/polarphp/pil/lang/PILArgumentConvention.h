//===--- PILArgumentConvention.h --------------------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_PILARGUMENTCONVENTION_H
#define POLARPHP_PIL_PILARGUMENTCONVENTION_H

#include "polarphp/ast/Types.h"

namespace polar {

/// Conventions for apply operands and function-entry arguments in PIL.
///
/// This is simply a union of ParameterConvention and ResultConvention
/// (ParameterConvention + Indirect_Out) for convenience when visiting all
/// arguments.
struct PILArgumentConvention {
  enum ConventionType : uint8_t {
    Indirect_In,
    Indirect_In_Constant,
    Indirect_In_Guaranteed,
    Indirect_Inout,
    Indirect_InoutAliasable,
    Indirect_Out,
    Direct_Owned,
    Direct_Unowned,
    Direct_Deallocating,
    Direct_Guaranteed,
  } Value;

  PILArgumentConvention(decltype(Value) NewValue) : Value(NewValue) {}

  /// Turn a ParameterConvention into a PILArgumentConvention.
  explicit PILArgumentConvention(ParameterConvention Conv) : Value() {
    switch (Conv) {
    case ParameterConvention::Indirect_In:
      Value = PILArgumentConvention::Indirect_In;
      return;
    case ParameterConvention::Indirect_In_Constant:
      Value = PILArgumentConvention::Indirect_In_Constant;
      return;
    case ParameterConvention::Indirect_Inout:
      Value = PILArgumentConvention::Indirect_Inout;
      return;
    case ParameterConvention::Indirect_InoutAliasable:
      Value = PILArgumentConvention::Indirect_InoutAliasable;
      return;
    case ParameterConvention::Indirect_In_Guaranteed:
      Value = PILArgumentConvention::Indirect_In_Guaranteed;
      return;
    case ParameterConvention::Direct_Unowned:
      Value = PILArgumentConvention::Direct_Unowned;
      return;
    case ParameterConvention::Direct_Guaranteed:
      Value = PILArgumentConvention::Direct_Guaranteed;
      return;
    case ParameterConvention::Direct_Owned:
      Value = PILArgumentConvention::Direct_Owned;
      return;
    }
    llvm_unreachable("covered switch isn't covered?!");
  }

  operator ConventionType() const { return Value; }

  bool isIndirectConvention() const {
    return Value <= PILArgumentConvention::Indirect_Out;
  }

  bool isInoutConvention() const {
    switch (Value) {
      case PILArgumentConvention::Indirect_Inout:
      case PILArgumentConvention::Indirect_InoutAliasable:
        return true;
      case PILArgumentConvention::Indirect_In_Guaranteed:
      case PILArgumentConvention::Indirect_In:
      case PILArgumentConvention::Indirect_In_Constant:
      case PILArgumentConvention::Indirect_Out:
      case PILArgumentConvention::Direct_Unowned:
      case PILArgumentConvention::Direct_Owned:
      case PILArgumentConvention::Direct_Deallocating:
      case PILArgumentConvention::Direct_Guaranteed:
        return false;
    }
    llvm_unreachable("covered switch isn't covered?!");
  }

  bool isOwnedConvention() const {
    switch (Value) {
    case PILArgumentConvention::Indirect_In:
    case PILArgumentConvention::Direct_Owned:
      return true;
    case PILArgumentConvention::Indirect_In_Guaranteed:
    case PILArgumentConvention::Direct_Guaranteed:
    case PILArgumentConvention::Indirect_Inout:
    case PILArgumentConvention::Indirect_In_Constant:
    case PILArgumentConvention::Indirect_Out:
    case PILArgumentConvention::Indirect_InoutAliasable:
    case PILArgumentConvention::Direct_Unowned:
    case PILArgumentConvention::Direct_Deallocating:
      return false;
    }
    llvm_unreachable("covered switch isn't covered?!");
  }

  bool isGuaranteedConvention() const {
    switch (Value) {
    case PILArgumentConvention::Indirect_In_Guaranteed:
    case PILArgumentConvention::Direct_Guaranteed:
      return true;
    case PILArgumentConvention::Indirect_Inout:
    case PILArgumentConvention::Indirect_In:
    case PILArgumentConvention::Indirect_In_Constant:
    case PILArgumentConvention::Indirect_Out:
    case PILArgumentConvention::Indirect_InoutAliasable:
    case PILArgumentConvention::Direct_Unowned:
    case PILArgumentConvention::Direct_Owned:
    case PILArgumentConvention::Direct_Deallocating:
      return false;
    }
    llvm_unreachable("covered switch isn't covered?!");
  }

  /// Returns true if \p Value is a non-aliasing indirect parameter.
  bool isExclusiveIndirectParameter() {
    switch (Value) {
    case PILArgumentConvention::Indirect_In:
    case PILArgumentConvention::Indirect_In_Constant:
    case PILArgumentConvention::Indirect_Out:
    case PILArgumentConvention::Indirect_In_Guaranteed:
    case PILArgumentConvention::Indirect_Inout:
      return true;

    case PILArgumentConvention::Indirect_InoutAliasable:
    case PILArgumentConvention::Direct_Unowned:
    case PILArgumentConvention::Direct_Guaranteed:
    case PILArgumentConvention::Direct_Owned:
    case PILArgumentConvention::Direct_Deallocating:
      return false;
    }
    llvm_unreachable("covered switch isn't covered?!");
  }
};

} // namespace polar

#endif
