//===--- ASTNode.cpp - Swift Language ASTs --------------------------------===//
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
// Created by polarboy on 2019/04/25.
//===----------------------------------------------------------------------===//
//
// This file implements the AstNode, which is a union of Stmt, Expr, and Decl.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/AstNode.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Expr.h"
#include "polarphp/ast/Stmt.h"
#include "polarphp/parser/SourceLoc.h"

namespace polar::ast {

SourceRange AstNode::getSourceRange() const
{
//  if (const auto *E = this->dyn_cast<Expr*>())
//    return E->getSourceRange();
//  if (const auto *S = this->dyn_cast<Stmt*>())
//    return S->getSourceRange();
//  if (const auto *D = this->dyn_cast<Decl*>())
//    return D->getSourceRange();
//  polar_unreachable("unsupported AST node");
   return SourceRange();
}

/// Return the location of the start of the statement.
SourceLoc AstNode::getStartLoc() const
{
  return getSourceRange().m_start;
}

/// Return the location of the end of the statement.
SourceLoc AstNode::getEndLoc() const
{
  return getSourceRange().m_end;
}

DeclContext *AstNode::getAsDeclContext() const
{
//  if (auto *E = this->dyn_cast<Expr*>()) {
//    if (isa<AbstractClosureExpr>(E))
//      return static_cast<AbstractClosureExpr*>(E);
//  } else if (is<Stmt*>()) {
//    return nullptr;
//  } else if (auto *D = this->dyn_cast<Decl*>()) {
//    if (isa<DeclContext>(D))
//      return cast<DeclContext>(D);
//  } else if (getOpaqueValue())
//    polar_unreachable("unsupported AST node");
  return nullptr;
}

bool AstNode::isImplicit() const
{
   return false;
//  if (const auto *E = this->dyn_cast<Expr*>())
//    return E->isImplicit();
//  if (const auto *S = this->dyn_cast<Stmt*>())
//    return S->isImplicit();
//  if (const auto *D = this->dyn_cast<Decl*>())
//    return D->isImplicit();
//  polar_unreachable("unsupported AST node");
}

void AstNode::walk(AstWalker &walker)
{
//  if (auto *E = this->dyn_cast<Expr*>())
//    E->walk(walker);
//  else if (auto *S = this->dyn_cast<Stmt*>())
//    S->walk(walker);
//  else if (auto *D = this->dyn_cast<Decl*>())
//    D->walk(walker);
//  else
//    polar_unreachable("unsupported AST node");
}

bool AstNode::isStmt(StmtKind kind) const
{
   if (!is<Stmt*>()) {
      return false;
   }
   return get<Stmt *>()->getKind() == kind;
}

bool AstNode::isDecl(DeclKind kind) const
{
   if (!is<Decl *>()) {
      return false;
   }
   return get<Decl*>()->getKind() == kind;
}

bool AstNode::isExpr(ExprKind kind) const
{
   if (!is<Expr *>()) {
      return false;
   }
   return get<Expr *>()->getKind() == kind;
}

} // polar::ast
