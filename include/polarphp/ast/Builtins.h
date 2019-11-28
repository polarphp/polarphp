//===--- Builtins.h - Swift Builtin Functions -------------------*- C++ -*-===//
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
// Created by polarboy on 2019/04/26.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interface to builtin functions.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_BUILTINS_H
#define POLARPHP_AST_BUILTINS_H

#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Attributes.h"
#include "polarphp/ast/Type.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {
enum class AtomicOrdering;
} // llvm

namespace polar::ast {

class AstContext;
class Identifier;
class ValueDecl;
class Type;

/// Get the builtin type for the given name.
///
/// Returns a null type if the name is not a known builtin type name.
Type get_builtin_type(AstContext &context, StringRef name);

/// OverloadedBuiltinKind - Whether and how a builtin is overloaded.
enum class OverloadedBuiltinKind : uint8_t
{
   /// The builtin is not overloaded.
   None,

   /// The builtin is overloaded over all integer types.
   Integer,

   /// The builtin is overloaded over all integer types and vectors of integers.
   IntegerOrVector,

   /// The builtin is overloaded over all integer types and the raw pointer type.
   IntegerOrRawPointer,

   /// The builtin is overloaded over all integer types, the raw pointer type,
   /// and vectors of integers.
   IntegerOrRawPointerOrVector,

   /// The builtin is overloaded over all floating-point types.
   Float,

   /// The builtin is overloaded over all floating-point types and vectors of
   /// floating-point types.
   FloatOrVector,

   /// The builtin has custom processing.
   Special
};

/// BuiltinValueKind - The set of (possibly overloaded) builtin functions.
enum class BuiltinValueKind
{
   None = 0,
#define BUILTIN(Id, Name, m_attrs) Id,
#include "polarphp/ast/BuiltinsDef.h"
};

/// Decode the type list of a builtin (e.g. mul_Int32) and return the base
/// name (e.g. "mul").
StringRef get_builtin_base_name(AstContext &context, StringRef name,
                                SmallVectorImpl<Type> &types);

/// Given an LLVM IR intrinsic name with argument types remove (e.g. like
/// "bswap") return the LLVM IR IntrinsicID for the intrinsic or not_intrinsic
/// (0) if the intrinsic name doesn't match anything.
llvm::Intrinsic::ID get_llvm_intrinsic_id(StringRef name);

/// Get the LLVM intrinsic ID that corresponds to the given builtin with
/// overflow.
llvm::Intrinsic::ID get_llvm_intrinsic_id_for_builtin_with_overflow(BuiltinValueKind id);


/// Create a ValueDecl for the builtin with the given name.
///
/// Returns null if the name does not identifier a known builtin value.
ValueDecl *get_builtin_value_decl(AstContext &context, Identifier name);

/// Returns the name of a builtin declaration given a builtin ID.
StringRef get_builtin_name(BuiltinValueKind id);

/// The information identifying the builtin - its kind and types.
class BuiltinInfo
{
public:
   BuiltinValueKind id;
   SmallVector<Type, 4> types;
   bool isReadNone() const;
};

/// The information identifying the llvm intrinsic - its id and types.
class IntrinsicInfo
{
public:
   llvm::Intrinsic::ID id;
   SmallVector<Type, 4> types;
   bool hasAttribute(llvm::Attribute::AttrKind Kind) const;
private:
   mutable llvm::AttributeList m_attrs =
         llvm::DenseMapInfo<llvm::AttributeList>::getEmptyKey();
};

/// Turn a string like "release" into the LLVM enum.
llvm::AtomicOrdering decode_llvm_atomic_ordering(StringRef order);


} // polar::ast

#endif // POLARPHP_AST_BUILTINS_H
