//===--- AstNode.cpp - Swift Language ASTs --------------------------------===//
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
// This file implements the AstNode, which is a union of Stmt, Expr, and Decl.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/AstNode.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Expr.h"
#include "polarphp/ast/Stmt.h"
#include "polarphp/basic/SourceLoc.h"

namespace polar {

SourceRange AstNode::getSourceRange() const {
  if (const auto *E = this->dyn_cast<Expr*>())
    return E->getSourceRange();
  if (const auto *S = this->dyn_cast<Stmt*>())
    return S->getSourceRange();
  if (const auto *D = this->dyn_cast<Decl*>())
    return D->getSourceRange();
  llvm_unreachable("unsupported AST node");
}

/// Return the location of the start of the statement.
SourceLoc AstNode::getStartLoc() const {
  return getSourceRange().start;
}

/// Return the location of the end of the statement.
SourceLoc AstNode::getEndLoc() const {
  return getSourceRange().end;
}

DeclContext *AstNode::getAsDeclContext() const {
  if (auto *E = this->dyn_cast<Expr*>()) {
    if (isa<AbstractClosureExpr>(E))
      return static_cast<AbstractClosureExpr*>(E);
  } else if (is<Stmt*>()) {
    return nullptr;
  } else if (auto *D = this->dyn_cast<Decl*>()) {
    if (isa<DeclContext>(D))
      return cast<DeclContext>(D);
  } else if (getOpaqueValue())
    llvm_unreachable("unsupported AST node");
  return nullptr;
}

bool AstNode::isImplicit() const {
  if (const auto *E = this->dyn_cast<Expr*>())
    return E->isImplicit();
  if (const auto *S = this->dyn_cast<Stmt*>())
    return S->isImplicit();
  if (const auto *D = this->dyn_cast<Decl*>())
    return D->isImplicit();
  llvm_unreachable("unsupported AST node");
}

void AstNode::walk(AstWalker &Walker) {
  if (auto *E = this->dyn_cast<Expr*>())
    E->walk(Walker);
  else if (auto *S = this->dyn_cast<Stmt*>())
    S->walk(Walker);
  else if (auto *D = this->dyn_cast<Decl*>())
    D->walk(Walker);
  else
    llvm_unreachable("unsupported AST node");
}

void AstNode::dump(raw_ostream &OS, unsigned Indent) const {
  if (auto S = dyn_cast<Stmt*>())
    S->dump(OS, /*context=*/nullptr, Indent);
  else if (auto E = dyn_cast<Expr*>())
    E->dump(OS, Indent);
  else if (auto D = dyn_cast<Decl*>())
    D->dump(OS, Indent);
  else
    OS << "<null>";
}

void AstNode::dump() const {
  dump(llvm::errs());
}

#define FUNC(T)                                                               \
bool AstNode::is##T(T##Kind Kind) const {                                     \
  if (!is<T*>())                                                              \
    return false;                                                             \
  return get<T*>()->getKind() == Kind;                                        \
}
FUNC(Stmt)
FUNC(Expr)
FUNC(Decl)
#undef FUNC

} // polar
