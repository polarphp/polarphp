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

#ifndef POLARPHP_SYNTAX_SYNTAX_NODE_COMMON_NODES_H
#define POLARPHP_SYNTAX_SYNTAX_NODE_COMMON_NODES_H

#include "polarphp/syntax/Syntax.h"
#include "polarphp/syntax/TokenSyntax.h"
#include "polarphp/syntax/UnknownSyntax.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodesFwd.h"

namespace polar::syntax {

class DeclSyntax : public Syntax
{
public:
   DeclSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return is_decl_kind(kind);
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class StmtSyntax : public Syntax
{
public:
   StmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return is_stmt_kind(kind);
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

///
/// expr:
/// 	variable
/// | T_LIST '(' array_pair_list ')' '=' expr
/// | '[' array_pair_list ']' '=' expr
/// | variable '=' expr
/// | variable '=' '&' variable
/// | T_CLONE expr
/// | variable T_PLUS_EQUAL expr
/// | variable T_MINUS_EQUAL expr
/// | variable T_MUL_EQUAL expr
/// | variable T_POW_EQUAL expr
/// | variable T_DIV_EQUAL expr
/// | variable T_CONCAT_EQUAL expr
/// | variable T_MOD_EQUAL expr
/// | variable T_AND_EQUAL expr
/// | variable T_OR_EQUAL expr
/// | variable T_XOR_EQUAL expr
/// | variable T_SL_EQUAL expr
/// | variable T_SR_EQUAL expr
/// | variable T_COALESCE_EQUAL expr
/// | variable T_INC
/// | T_INC variable
/// | variable T_DEC
/// | T_DEC variable
/// | expr T_BOOLEAN_OR expr
/// | expr T_BOOLEAN_AND expr
/// | expr T_LOGICAL_OR expr
/// | expr T_LOGICAL_AND expr
/// | expr T_LOGICAL_XOR expr
/// | expr '|' expr
/// | expr '&' expr
/// | expr '^' expr
/// | expr '.' expr
/// | expr '+' expr
/// | expr '-' expr
/// | expr '*' expr
/// | expr T_POW expr
/// | expr '/' expr
/// | expr '%' expr
/// | expr T_SL expr
/// | expr T_SR expr
/// | '+' expr %prec T_INC
/// | '-' expr %prec T_INC
/// | '!' expr
/// | '~' expr
/// | expr T_IS_IDENTICAL expr
/// | expr T_IS_NOT_IDENTICAL expr
/// | expr T_IS_EQUAL expr
/// | expr T_IS_NOT_EQUAL expr
/// | expr '<' expr
/// | expr T_IS_SMALLER_OR_EQUAL expr
/// | expr '>' expr
/// | expr T_IS_GREATER_OR_EQUAL expr
/// | expr T_SPACESHIP expr
/// | expr T_INSTANCEOF class_name_reference
/// | '(' expr ')'
/// | new_expr
/// | expr '?' expr ':' expr
/// | expr '?' ':' expr
/// | expr T_COALESCE expr
/// | internal_functions_in_yacc
/// | T_INT_CAST expr
/// | T_DOUBLE_CAST expr
/// | T_STRING_CAST expr
/// | T_ARRAY_CAST expr
/// | T_OBJECT_CAST expr
/// | T_BOOL_CAST expr
/// | T_UNSET_CAST expr
/// | T_EXIT exit_expr
/// | '@' expr
/// | scalar
/// | '`' backticks_expr '`'
/// | T_PRINT expr
/// | T_YIELD
/// | T_YIELD expr
/// | T_YIELD expr T_DOUBLE_ARROW expr
/// | T_YIELD_FROM expr
/// | inline_function
/// | T_STATIC inline_function
///
class ExprSyntax : public Syntax
{
public:
   ExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return is_expr_kind(kind);
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class UnknownDeclSyntax final : public DeclSyntax
{
public:
   UnknownDeclSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : DeclSyntax(root, data)
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::UnknownDecl == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class UnknownExprSyntax final : public ExprSyntax
{
public:
   UnknownExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::UnknownExpr == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class UnknownStmtSyntax final : public StmtSyntax
{
public:
   UnknownStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::UnknownStmt == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_COMMON_NODES_H
