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
#include "polarphp/syntax/SyntaxCollection.h"
#include "polarphp/syntax/TokenSyntax.h"
#include "polarphp/syntax/UnknownSyntax.h"

namespace polar::syntax {

class DeclSyntax;
class ExprSyntax;
class StmtSyntax;
class UnknownDeclSyntax;
class UnknownExprSyntax;
class UnknownStmtSyntax;
class CodeBlockItemSyntax;
class CodeBlockItemListSyntax;
class CodeBlockSyntax;

using CodeBlockItemList = SyntaxCollection<SyntaxKind::CodeBlockItemList, CodeBlockItemSyntax>;

class DeclSyntax : public Syntax
{
public:
   DeclSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   static bool kindof(SyntaxKind kind)
   {
      return is_decl_kind(kind);
   }

   static bool classof(const Syntax *syntax)
   {
      return kindof(syntax->getKind());
   }
};

class StmtSyntax : public Syntax
{
public:
   StmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   static bool kindof(SyntaxKind kind)
   {
      return is_stmt_kind(kind);
   }

   static bool classof(const Syntax *syntax)
   {
      return kindof(syntax->getKind());
   }
};

class ExprSyntax : public Syntax
{
public:
   ExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   static bool kindof(SyntaxKind kind)
   {
      return is_expr_kind(kind);
   }

   static bool classof(const Syntax *syntax)
   {
      return kindof(syntax->getKind());
   }
};

class UnknownDeclSyntax : public Syntax
{
public:
   UnknownDeclSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   static bool kindof(SyntaxKind kind)
   {
      return SyntaxKind::UnknownDecl == kind;
   }

   static bool classof(const Syntax *syntax)
   {
      return kindof(syntax->getKind());
   }
};

class UnknownExprSyntax : public Syntax
{
public:
   UnknownExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   static bool kindof(SyntaxKind kind)
   {
      return SyntaxKind::UnknownExpr == kind;
   }

   static bool classof(const Syntax *syntax)
   {
      return kindof(syntax->getKind());
   }
};

class UnknownStmtSyntax : public Syntax
{
public:
   UnknownStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   static bool kindof(SyntaxKind kind)
   {
      return SyntaxKind::UnknownStmt == kind;
   }

   static bool classof(const Syntax *syntax)
   {
      return kindof(syntax->getKind());
   }
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_COMMON_NODES_H
