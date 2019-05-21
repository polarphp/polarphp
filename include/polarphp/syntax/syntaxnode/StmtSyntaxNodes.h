// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/08.

#ifndef POLARPHP_SYNTAX_SYNTAX_NODE_STMT_NODES_H
#define POLARPHP_SYNTAX_SYNTAX_NODE_STMT_NODES_H

#include "polarphp/syntax/Syntax.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"
#include "polarphp/syntax/SyntaxCollection.h"
#include "polarphp/syntax/TokenSyntax.h"
#include "polarphp/syntax/UnknownSyntax.h"

namespace polar::syntax {

class ThrowStmtSyntax;

class ThrowStmtSyntax : public StmtSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 1;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;

   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: false
      ThrowKeyword,
      /// type: ExprSyntax
      /// optional: false
      Expr
   };
public:
   ThrowStmtSyntax(RefCountPtr<SyntaxData> parent, const SyntaxData *data)
      : StmtSyntax(parent, data)
   {
      this->validate();
   }

   TokenSyntax getThrowKeyword();
   ExprSyntax getExpr();

   ThrowStmtSyntax withThrowKeyword(std::optional<TokenSyntax> throwKeyword);
   ThrowStmtSyntax withExpr(std::optional<ExprSyntax> expr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ThrowStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ThrowStmtSyntaxBuilder;
   void validate();
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_STMT_NODES_H
