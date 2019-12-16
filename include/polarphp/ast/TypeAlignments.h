//===--- TypeAlignments.h - Alignments of various Swift types ---*- C++ -*-===//
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
/// \file This file defines the alignment of various Swift AST classes.
///
/// It's useful to do this in a dedicated place to avoid recursive header
/// problems. To make sure we don't have any ODR violations, this header
/// should be included in every header that defines one of the forward-
/// declared types listed here.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_TYPEALIGNMENTS_H
#define POLARPHP_AST_TYPEALIGNMENTS_H

#include <cstddef>

namespace polar {

class AbstractClosureExpr;
class AbstractStorageDecl;
class ArchetypeType;
class AssociatedTypeDecl;
class AstContext;
class AttributeBase;
class BraceStmt;
class Decl;
class DeclContext;
class Expr;
class ExtensionDecl;
class GenericEnvironment;
class GenericTypeParamDecl;
class NominalTypeDecl;
class NormalInterfaceConformance;
class OpaqueValueExpr;
class OperatorDecl;
class Pattern;
class InterfaceDecl;
class InterfaceConformance;
class Stmt;
class TypeVariableType;
class TypeBase;
class TypeDecl;
class ValueDecl;
class PILFunction;

/// We frequently use three tag bits on all of these types.
constexpr size_t AttrAlignInBits = 3;
constexpr size_t DeclAlignInBits = 3;
constexpr size_t DeclContextAlignInBits = 3;
constexpr size_t ExprAlignInBits = 3;
constexpr size_t StmtAlignInBits = 3;
constexpr size_t TypeAlignInBits = 3;
constexpr size_t PatternAlignInBits = 3;
constexpr size_t PILFunctionAlignInBits = 2;
constexpr size_t TypeVariableAlignInBits = 4;
}

namespace llvm {
/// Helper class for declaring the expected alignment of a pointer.
/// TODO: LLVM should provide this.
template <class T, size_t AlignInBits> struct MoreAlignedPointerTraits {
   enum { NumLowBitsAvailable = AlignInBits };
   static inline void *getAsVoidPointer(T *ptr) { return ptr; }
   static inline T *getFromVoidPointer(void *ptr) {
      return static_cast<T*>(ptr);
   }
};

template <class T> struct PointerLikeTypeTraits;
}

/// Declare the expected alignment of pointers to the given type.
/// This macro should be invoked from a top-level file context.
#define LLVM_DECLARE_TYPE_ALIGNMENT(CLASS, ALIGNMENT)     \
   namespace llvm {                                          \
   template <> struct PointerLikeTypeTraits<CLASS*>          \
   : public MoreAlignedPointerTraits<CLASS, ALIGNMENT> {}; \
   }

LLVM_DECLARE_TYPE_ALIGNMENT(polar::Decl, polar::DeclAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::AbstractStorageDecl, polar::DeclAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::AssociatedTypeDecl, polar::DeclAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::GenericTypeParamDecl, polar::DeclAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::OperatorDecl, polar::DeclAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::InterfaceDecl, polar::DeclAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::TypeDecl, polar::DeclAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::ValueDecl, polar::DeclAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::NominalTypeDecl, polar::DeclAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::ExtensionDecl, polar::DeclAlignInBits)

LLVM_DECLARE_TYPE_ALIGNMENT(polar::TypeBase, polar::TypeAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::ArchetypeType, polar::TypeAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::TypeVariableType, polar::TypeVariableAlignInBits)

LLVM_DECLARE_TYPE_ALIGNMENT(polar::Stmt, polar::StmtAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::BraceStmt, polar::StmtAlignInBits)

LLVM_DECLARE_TYPE_ALIGNMENT(polar::AstContext, 2)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::DeclContext, polar::DeclContextAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::Expr, polar::ExprAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::AbstractClosureExpr, polar::ExprAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::OpaqueValueExpr, polar::ExprAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::InterfaceConformance, polar::DeclAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::NormalInterfaceConformance,
                            polar::DeclAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::GenericEnvironment,
                            polar::DeclAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::Pattern,
                            polar::PatternAlignInBits)
LLVM_DECLARE_TYPE_ALIGNMENT(polar::PILFunction,
                            polar::PILFunctionAlignInBits)

LLVM_DECLARE_TYPE_ALIGNMENT(polar::AttributeBase, polar::AttrAlignInBits)

static_assert(alignof(void*) >= 2, "pointer alignment is too small");

#endif // POLARPHP_AST_TYPEALIGNMENTS_H
