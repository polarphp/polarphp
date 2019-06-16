// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/16.

#ifndef POLARPHP_PARSER_PARSED_SYNTAX_NODE_PARSED_COMMON_SYNTAX_NODES_H
#define POLARPHP_PARSER_PARSED_SYNTAX_NODE_PARSED_COMMON_SYNTAX_NODES_H

#include "polarphp/parser/ParsedSyntax.h"
#include "polarphp/syntax/SyntaxKind.h"

namespace polar::parser {

class ParsedDeclSyntax;
class ParsedExprSyntax;
class ParsedStmtSyntax;
class ParsedTypeSyntax;
class ParsedUnknownDeclSyntax;
class ParsedUnknownExprSyntax;
class ParsedUnknownStmtSyntax;
class ParsedUnknownTypeSyntax;
class ParsedCodeBlockItemSyntax;
class ParsedCodeBlockSyntax;

using ParsedCodeBlockItemListSyntax = ParsedSyntaxCollection<SyntaxKind::CodeBlockItemList>;
using ParsedTokenListSyntax = ParsedSyntaxCollection<SyntaxKind::TokenList>;
using ParsedNonEmptyTokenListSyntax = ParsedSyntaxCollection<SyntaxKind::NonEmptyTokenList>;

class ParsedDeclSyntax : public ParsedSyntax
{
public:
   explicit ParsedDeclSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedSyntax(std::move(rawNode))
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return polar::syntax::is_decl_kind(kind);
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class ParsedExprSyntax : public ParsedSyntax
{
public:
   explicit ParsedExprSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedSyntax(std::move(rawNode))
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return polar::syntax::is_expr_kind(kind);
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class ParsedStmtSyntax : public ParsedSyntax
{
public:
   explicit ParsedStmtSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedSyntax(std::move(rawNode))
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return polar::syntax::is_stmt_kind(kind);
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class ParsedTypeSyntax : public ParsedSyntax
{
public:
   explicit ParsedTypeSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedSyntax(std::move(rawNode))
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return polar::syntax::is_type_kind(kind);
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class ParsedUnknownDeclSyntax final : public ParsedSyntax
{
public:
   explicit ParsedUnknownDeclSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedSyntax(std::move(rawNode))
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::UnknownDecl == kind;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class ParsedUnknownExprSyntax final : public ParsedSyntax
{
public:
   explicit ParsedUnknownExprSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedSyntax(std::move(rawNode))
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::UnknownExpr == kind;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class ParsedUnknownStmtSyntax final : public ParsedSyntax
{
public:
   explicit ParsedUnknownStmtSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedSyntax(std::move(rawNode))
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::UnknownStmt == kind;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class ParsedUnknownTypeSyntax final : public ParsedSyntax
{
public:
   explicit ParsedUnknownTypeSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedSyntax(std::move(rawNode))
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::UnknownType == kind;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class ParsedCodeBlockItemSyntax final : public ParsedSyntax
{
public:
   explicit ParsedCodeBlockItemSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedSyntax(std::move(rawNode))
   {}

   ParsedSyntax getDeferredItem();
   ParsedTokenSyntax getDeferredSemicolon();
   std::optional<ParsedSyntax> getgetDeferredErrorTokens();

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::CodeBlockItem == kind;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class ParsedCodeBlockItemSyntaxBuilder;
};

class ParsedCodeBlockSyntax final : public ParsedSyntax
{
public:
   explicit ParsedCodeBlockSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedSyntax(std::move(rawNode))
   {}

   ParsedTokenSyntax getDeferredLeftBrace();
   ParsedCodeBlockItemListSyntax getDeferredStatements();
   ParsedTokenSyntax getDeferredRightBrace();

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::CodeBlock == kind;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class ParsedCodeBlockItemSyntaxBuilder;
};

} // polar::parser

#endif // POLARPHP_PARSER_PARSED_SYNTAX_NODE_PARSED_COMMON_SYNTAX_NODES_H
