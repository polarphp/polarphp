//===--- TypeAlignments.h - Alignments of various Swift types ---*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polar::ast.org/LICENSE.txt for license information
// See https://polar::ast.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/25.
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

#ifndef POLARPHP_AST_TYPE_ALIGNMENTS_H
#define POLARPHP_AST_TYPE_ALIGNMENTS_H

#include <cstddef>

namespace polar::ast {

class AbstractClosureExpr;
class AbstractStorageDecl;
class ArchetypeType;
class AssociatedTypeDecl;
class AstContext;
class BraceStmt;
class Decl;
class DeclContext;
class Expr;
class ExtensionDecl;
class GenericEnvironment;
class GenericTypeParamDecl;
class NominalTypeDecl;
class NormalProtocolConformance;
class OperatorDecl;
class Pattern;
class ProtocolDecl;
class ProtocolConformance;
class Stmt;
class TypeVariableType;
class TypeBase;
class TypeDecl;
class ValueDecl;

/// We frequently use three tag bits on all of these types.
constexpr size_t DeclAlignInBits = 3;
constexpr size_t DeclContextAlignInBits = 3;
constexpr size_t ExprAlignInBits = 3;
constexpr size_t StmtAlignInBits = 3;
constexpr size_t TypeAlignInBits = 3;
constexpr size_t PatternAlignInBits = 3;
constexpr size_t SILFunctionAlignInBits = 2;

} // polar::ast

namespace polar::utils {
/// Helper class for declaring the expected alignment of a pointer.
/// TODO: LLVM should provide this.
template <class T, size_t AlignInBits>
struct MoreAlignedPointerTraits
{
   enum { NumLowBitsAvailable = AlignInBits };

   static inline void *getAsVoidPointer(T *ptr)
   {
      return ptr;
   }

   static inline T *getFromVoidPointer(void *ptr)
   {
      return static_cast<T*>(ptr);
   }
};
template <class T>
struct PointerLikeTypeTraits;
} // polar::utils

/// Declare the expected alignment of pointers to the given type.
/// This macro should be invoked from a top-level file context.
#define POLAR_DECLARE_TYPE_ALIGNMENT(CLASS, ALIGNMENT)     \
   namespace polar::utils {                                          \
   template <> struct PointerLikeTypeTraits<CLASS*>          \
   : public MoreAlignedPointerTraits<CLASS, ALIGNMENT> {}; \
   }

POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::Decl, polar::ast::DeclAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::AbstractStorageDecl, polar::ast::DeclAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::AssociatedTypeDecl, polar::ast::DeclAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::GenericTypeParamDecl, polar::ast::DeclAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::OperatorDecl, polar::ast::DeclAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::ProtocolDecl, polar::ast::DeclAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::TypeDecl, polar::ast::DeclAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::ValueDecl, polar::ast::DeclAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::NominalTypeDecl, polar::ast::DeclAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::ExtensionDecl, polar::ast::DeclAlignInBits)

POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::TypeBase, polar::ast::TypeAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::ArchetypeType, polar::ast::TypeAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::TypeVariableType, polar::ast::TypeAlignInBits)

POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::Stmt, polar::ast::StmtAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::BraceStmt, polar::ast::StmtAlignInBits)

POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::AstContext, 2)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::DeclContext, polar::ast::DeclContextAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::Expr, polar::ast::ExprAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::AbstractClosureExpr, polar::ast::ExprAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::ProtocolConformance, polar::ast::DeclAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::NormalProtocolConformance, polar::ast::DeclAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::GenericEnvironment, polar::ast::DeclAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::Pattern, polar::ast::PatternAlignInBits)
POLAR_DECLARE_TYPE_ALIGNMENT(polar::ast::SILFunction, polar::ast::SILFunctionAlignInBits)

static_assert(alignof(void*) >= 2, "pointer alignment is too small");

#endif // POLARPHP_AST_TYPE_ALIGNMENTS_H
