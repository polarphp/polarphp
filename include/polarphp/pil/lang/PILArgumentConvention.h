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
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/28.

#ifndef POLAR_PIL_LANG_PILARGUMENTCONVENTION_H
#define POLAR_PIL_LANG_PILARGUMENTCONVENTION_H

#include "polarphp/ast/Types.h"
#include <cstdint>

namespace polar::pil {

enum class InoutAliasingAssumption
{
   /// Assume that an inout indirect parameter may alias other objects.
   /// This is the safe assumption an optimization should make if it may break
   /// memory safety in case the inout aliasing rule is violation.
   Aliasing,

   /// Assume that an inout indirect parameter cannot alias other objects.
   /// Optimizations should only use this if they can guarantee that they will
   /// not break memory safety even if the inout aliasing rule is violated.
   NotAliasing
};

/// Conventions for apply operands and function-entry arguments in PIL.
///
/// This is simply a union of ParameterConvention and ResultConvention
/// (ParameterConvention + Indirect_Out) for convenience when visiting all
/// arguments.
struct PILArgumentConvention
{
   enum ConventionType : std::uint8_t
   {
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
   } value;

   PILArgumentConvention(decltype(value) newValue) : value(newValue) {}

   /// Turn a ParameterConvention into a PILArgumentConvention.
   explicit PILArgumentConvention(ParameterConvention conv)
      : value()
   {
      switch (conv) {
      case ParameterConvention::Indirect_In:
         value = PILArgumentConvention::Indirect_In;
         return;
      case ParameterConvention::Indirect_In_Constant:
         value = PILArgumentConvention::Indirect_In_Constant;
         return;
      case ParameterConvention::Indirect_Inout:
         value = PILArgumentConvention::Indirect_Inout;
         return;
      case ParameterConvention::Indirect_InoutAliasable:
         value = PILArgumentConvention::Indirect_InoutAliasable;
         return;
      case ParameterConvention::Indirect_In_Guaranteed:
         value = PILArgumentConvention::Indirect_In_Guaranteed;
         return;
      case ParameterConvention::Direct_Unowned:
         value = PILArgumentConvention::Direct_Unowned;
         return;
      case ParameterConvention::Direct_Guaranteed:
         value = PILArgumentConvention::Direct_Guaranteed;
         return;
      case ParameterConvention::Direct_Owned:
         value = PILArgumentConvention::Direct_Owned;
         return;
      }
      llvm_unreachable("covered switch isn't covered?!");
   }

   operator ConventionType() const
   {
      return value;
   }

   bool isIndirectConvention() const {
      return value <= PILArgumentConvention::Indirect_Out;
   }

   bool isInoutConvention() const
   {
      switch (value) {
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
      switch (value) {
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
      switch (value) {
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

   /// Returns true if \p value is a not-aliasing indirect parameter.
   /// The \p isInoutAliasing specifies what to assume about the inout
   /// convention.
   /// See InoutAliasingAssumption.
   bool isNotAliasedIndirectParameter(InoutAliasingAssumption isInoutAliasing) {
      switch (value) {
      case PILArgumentConvention::Indirect_In:
      case PILArgumentConvention::Indirect_In_Constant:
      case PILArgumentConvention::Indirect_Out:
      case PILArgumentConvention::Indirect_In_Guaranteed:
         return true;

      case PILArgumentConvention::Indirect_Inout:
         return isInoutAliasing == InoutAliasingAssumption::NotAliasing;

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

} // polar::pil

#endif // POLAR_PIL_LANG_PILARGUMENTCONVENTION_H
