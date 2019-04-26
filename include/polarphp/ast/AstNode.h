//===--- ASTNode.h - Swift Language ASTs ------------------------*- C++ -*-===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/26.
//===----------------------------------------------------------------------===//
//
// This file defines the ASTNode, which is a union of Stmt, Expr, and Decl.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_ASTNODE_H
#define POLARPHP_AST_ASTNODE_H

#include "polarphp/basic/adt/PointerUnion.h"
#include "polarphp/ast/TypeAlignments.h"

/// forward declare class with namespace
namespace polar::parser {
class SourceLoc;
class SourceRange;
}

namespace polar::ast {

using polar::basic::PointerUnion3;
using polar::parser::SourceLoc;
using polar::parser::SourceRange;

class Expr;
class Stmt;
class Decl;
class DeclContext;
class AstWalker;
enum class ExprKind : uint8_t;
enum class DeclKind : uint8_t;
enum class StmtKind;

struct AstNode : public PointerUnion3<Expr*, Stmt*, Decl*>
{
   // Inherit the constructors from PointerUnion.
   using PointerUnion3::PointerUnion3;

   SourceRange getSourceRange() const;

   /// Return the location of the start of the statement.
   SourceLoc getStartLoc() const;

   /// Return the location of the end of the statement.
   SourceLoc getEndLoc() const;

   void walk(AstWalker &walker);
   void walk(AstWalker &&walker)
   {
      walk(walker);
   }

   /// get the underlying entity as a decl context if it is one,
   /// otherwise, return nullptr;
   DeclContext *getAsDeclContext() const;

   /// Provides some utilities to decide detailed node kind.
   bool isStmt(StmtKind kind) const;
   bool isExpr(ExprKind kind) const;
   bool isDecl(DeclKind kind) const;

   /// Whether the AST node is implicit.
   bool isImplicit() const;
};

} // polar::ast

namespace polar::basic {
using polar::ast::AstNode;
template <>
struct DenseMapInfo<AstNode>
{
   static inline AstNode getEmptyKey()
   {
      return DenseMapInfo<polar::ast::Expr *>::getEmptyKey();
   }

   static inline AstNode getTombstoneKey()
   {
      return DenseMapInfo<polar::ast::Expr *>::getTombstoneKey();
   }

   static unsigned getHashValue(const AstNode value)
   {
      return DenseMapInfo<void *>::getHashValue(value.getOpaqueValue());
   }

   static bool isEqual(const AstNode lhs, const AstNode rhs)
   {
      return lhs.getOpaqueValue() == rhs.getOpaqueValue();
   }
};
} // polar::basic

#endif // POLARPHP_AST_ASTNODE_H
