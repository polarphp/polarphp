//===--- AstVisitor.h - SILGen AstVisitor specialization --------*- C++ -*-===//
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
// This file defines swift::Lowering::AstVisitor.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_GEN_ASTVISITOR_H
#define POLARPHP_PIL_GEN_ASTVISITOR_H

#include "polarphp/ast/AstVisitor.h"

namespace polar::lowering {

/// Lowering::AstVisitor - This is a specialization of
/// swift::AstVisitor which works only on resolved nodes and
/// which automatically ignores certain AST node kinds.
template<typename ImplClass,
   typename ExprRetTy = void,
   typename StmtRetTy = void,
   typename DeclRetTy = void,
   typename PatternRetTy = void,
   typename... Args>
class AstVisitor : public polar::AstVisitor<ImplClass,
   ExprRetTy,
   StmtRetTy,
   DeclRetTy,
   PatternRetTy,
   void,
   void,
   Args...>
{
public:
#define EXPR(Id, Parent)
#define UNCHECKED_EXPR(Id, Parent) \
  ExprRetTy visit##Id##Expr(Id##Expr *E, Args...AA) { \
    llvm_unreachable(#Id "Expr should not survive to SILGen"); \
  }
#include "polarphp/ast/ExprNodesDef.h"

   ExprRetTy visitErrorExpr(ErrorExpr *E, Args...AA) {
      llvm_unreachable("expression kind should not survive to SILGen");
   }

   ExprRetTy visitCodeCompletionExpr(CodeCompletionExpr *E, Args...AA) {
      llvm_unreachable("expression kind should not survive to SILGen");
   }

   ExprRetTy visitDefaultArgumentExpr(DefaultArgumentExpr *E, Args...AA) {
      llvm_unreachable("DefaultArgumentExpr should not appear in this position");
   }

   ExprRetTy visitVarargExpansionExpr(VarargExpansionExpr *E, Args... AA) {
      return static_cast<ImplClass*>(this)->visit(E->getSubExpr(),
                                                  std::forward<Args>(AA)...);
   }

   ExprRetTy visitIdentityExpr(IdentityExpr *E, Args...AA) {
      return static_cast<ImplClass*>(this)->visit(E->getSubExpr(),
                                                  std::forward<Args>(AA)...);
   }

   ExprRetTy visitTryExpr(TryExpr *E, Args...AA) {
      return static_cast<ImplClass*>(this)->visit(E->getSubExpr(),
                                                  std::forward<Args>(AA)...);
   }

   ExprRetTy visitLazyInitializerExpr(LazyInitializerExpr *E, Args...AA) {
      return static_cast<ImplClass*>(this)->visit(E->getSubExpr(),
                                                  std::forward<Args>(AA)...);
   }
};

template <typename ImplClass,
   typename ExprRetTy = void,
   typename... Args>
using ExprVisitor = AstVisitor<ImplClass, ExprRetTy, void, void, void, Args...>;

} // end namespace polar::lowering

#endif
