//===--- AstNode.h - Swift Language ASTs ------------------------*- C++ -*-===//
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
// This file defines the AstNode, which is a union of Stmt, Expr, and Decl.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_AST_NODE_H
#define POLARPHP_AST_AST_NODE_H

#include "llvm/ADT/PointerUnion.h"
#include "polarphp/basic/Debug.h"
#include "polarphp/ast/TypeAlignments.h"

namespace llvm {
class raw_ostream;
}

namespace polar {
class SourceLoc;
class SourceRange;
class Expr;
class Stmt;
class Decl;
class DeclContext;
class AstWalker;
enum class ExprKind : uint8_t;
enum class DeclKind : uint8_t;
enum class StmtKind;

struct AstNode : public llvm::PointerUnion<Expr*, Stmt*, Decl*> {
   // Inherit the constructors from PointerUnion.
   using PointerUnion::PointerUnion;

   SourceRange getSourceRange() const;

   /// Return the location of the start of the statement.
   SourceLoc getStartLoc() const;

   /// Return the location of the end of the statement.
   SourceLoc getEndLoc() const;

   void walk(AstWalker &Walker);
   void walk(AstWalker &&walker) { walk(walker); }

   /// get the underlying entity as a decl context if it is one,
   /// otherwise, return nullptr;
   DeclContext *getAsDeclContext() const;

   /// Provides some utilities to decide detailed node kind.
#define FUNC(T) bool is##T(T##Kind Kind) const;
   FUNC(Stmt)
   FUNC(Expr)
   FUNC(Decl)
#undef FUNC

   POLAR_DEBUG_DUMP;
   void dump(llvm::raw_ostream &OS, unsigned Indent = 0) const;

   /// Whether the AST node is implicit.
   bool isImplicit() const;
};
} // namespace polar

namespace llvm {
using polar::AstNode;
template <> struct DenseMapInfo<AstNode> {
   static inline AstNode getEmptyKey() {
      return DenseMapInfo<polar::Expr *>::getEmptyKey();
   }
   static inline AstNode getTombstoneKey() {
      return DenseMapInfo<polar::Expr *>::getTombstoneKey();
   }
   static unsigned getHashValue(const AstNode Val) {
      return DenseMapInfo<void *>::getHashValue(Val.getOpaqueValue());
   }
   static bool isEqual(const AstNode LHS, const AstNode RHS) {
      return LHS.getOpaqueValue() == RHS.getOpaqueValue();
   }
};
}

#endif // LLVM_POLARPHP_AST_AST_NODE_H
