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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/05.
//
// This file defines the AstNode, which is a union of Stmt, Expr, and Decl.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_AST_NODE_H
#define POLARPHP_AST_AST_NODE_H

#include "llvm/ADT/PointerUnion.h"
#include "polarphp/ast/TypeAlignments.h"

namespace llvm {
class raw_ostream;
}

namespace polar::basic {
class SourceLoc;
class SourceRange;
}

namespace polar::ast {

class Expr;
class Stmt;
class Decl;
class DeclContext;

using polar::basic::SourceLoc;
using polar::basic::SourceRange;

class AstWalker;
enum class ExprKind : uint8_t;
enum class DeclKind : uint8_t;
enum class StmtKind;
using AstBaseNodeType = llvm::PointerUnion3<Expr*, Stmt*, Decl*>;

struct AstNode : public AstBaseNodeType
{
   // Inherit the constructors from PointerUnion.
   using AstBaseNodeType::PointerUnion;
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

   LLVM_ATTRIBUTE_DEPRECATED(
         void dump() const LLVM_ATTRIBUTE_USED,
         "only for use within the debugger");
   void dump(llvm::raw_ostream &ostream, unsigned Indent = 0) const;

   /// Whether the AST node is implicit.
   bool isImplicit() const;
};

} // polar::ast

#endif // POLARPHP_AST_AST_NODE_H
