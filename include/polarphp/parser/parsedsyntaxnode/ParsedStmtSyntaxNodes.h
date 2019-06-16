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

#ifndef POLARPHP_PARSER_PARSED_SYNTAX_NODE_PARSED_STMT_SYNTAX_NODES_H
#define POLARPHP_PARSER_PARSED_SYNTAX_NODE_PARSED_STMT_SYNTAX_NODES_H

#include "polarphp/parser/ParsedSyntax.h"
#include "polarphp/syntax/SyntaxKind.h"
#include "polarphp/parser/parsedsyntaxnode/ParsedCommonSyntaxNodes.h"

namespace polar::parser {

class ParsedConditionElementSyntax;
class ParsedContinueStmtSyntax;
class ParsedBreakStmtSyntax;
class ParsedFallthroughStmtSyntax;
class ParsedElseIfClauseSyntax;
class ParsedIfStmtSyntax;
class ParsedWhileStmtSyntax;
class ParsedDoWhileStmtSyntax;
class ParsedSwitchCaseSyntax;
class ParsedSwitchDefaultLabelSyntax;
class ParsedSwitchCaseLabelSyntax;
class ParsedSwitchStmtSyntax;
class ParsedDeferStmtSyntax;
class ParsedExpressionStmtSyntax;
class ParsedThrowStmtSyntax;
class ParsedReturnStmtSyntax;

using ParsedConditionElementListSyntax = ParsedSyntaxCollection<SyntaxKind::ConditionElementList>;
using ParsedSwitchCaseListSyntax = ParsedSyntaxCollection<SyntaxKind::SwitchCaseList>;
using ParsedElseIfListSyntax = ParsedSyntaxCollection<SyntaxKind::ElseIfList>;

class ParsedConditionElementSyntax final : public ParsedStmtSyntax
{
public:
   explicit ParsedConditionElementSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedStmtSyntax(std::move(rawNode))
   {}

   ParsedSyntax getDeferredCondition();
   std::optional<ParsedTokenSyntax> getDeferredTrailingComma();

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ConditionElement;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ParsedConditionElementSyntaxBuilder;
};

class ParsedContinueStmtSyntax final : public ParsedStmtSyntax
{
public:
   explicit ParsedContinueStmtSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedStmtSyntax(std::move(rawNode))
   {}

   ParsedTokenSyntax getDeferredContinueKeyword();
   std::optional<ParsedTokenSyntax> getDeferredLNumberToken();

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ContinueStmt;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ParsedContinueStmtSyntaxBuilder;
};

class ParsedBreakStmtSyntax final : public ParsedStmtSyntax
{
public:
   explicit ParsedBreakStmtSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedStmtSyntax(std::move(rawNode))
   {}

   ParsedTokenSyntax getDeferredBreakKeyword();
   std::optional<ParsedTokenSyntax> getDeferredLNumberToken();

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::BreakStmt;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ParsedBreakStmtSyntaxBuilder;
};

class ParsedFallthroughStmtSyntax final : public ParsedStmtSyntax
{
public:
   explicit ParsedFallthroughStmtSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedStmtSyntax(std::move(rawNode))
   {}

   ParsedTokenSyntax getDeferredFallthroughKeyword();

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::FallthroughStmt;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ParsedFallthroughStmtSyntaxBuilder;
};

class ParsedElseIfClauseSyntax final : public ParsedStmtSyntax
{
public:
   explicit ParsedElseIfClauseSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedStmtSyntax(std::move(rawNode))
   {}

   ParsedTokenSyntax getDeferredElseIfKeyword();
   ParsedTokenSyntax getDeferredLeftParen();
   ParsedExprSyntax getDeferredCondition();
   ParsedTokenSyntax getDeferredRightParen();
   ParsedCodeBlockSyntax getDeferredBody();

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ElseIfClause;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ParsedElseIfClauseSyntaxBuilder;
};

class ParsedIfStmtSyntax final : public ParsedStmtSyntax
{
public:
   explicit ParsedIfStmtSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedStmtSyntax(std::move(rawNode))
   {}

   std::optional<ParsedTokenSyntax> getDeferredLabelName();
   std::optional<ParsedTokenSyntax> getDeferredLabelColon();
   ParsedTokenSyntax getDeferredIfKeyword();
   ParsedTokenSyntax getDeferredLeftParen();
   ParsedExprSyntax getDeferredCondition();
   ParsedTokenSyntax getDeferredRightParen();
   ParsedCodeBlockSyntax getDeferredBody();
   std::optional<ParsedElseIfListSyntax> getDeferredElseIfClauses();
   std::optional<ParsedTokenSyntax> getDeferredElseKeyword();
   std::optional<ParsedSyntax> getDeferredElseBody();

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::IfStmt;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ParsedIfStmtSyntaxBuilder;
};

class ParsedWhileStmtSyntax final : public ParsedStmtSyntax
{
public:
   explicit ParsedWhileStmtSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedStmtSyntax(std::move(rawNode))
   {}

   std::optional<ParsedTokenSyntax> getDeferredLabelName();
   std::optional<ParsedTokenSyntax> getDeferredLabelColon();
   ParsedTokenSyntax getDeferredWhileKeyword();
   ParsedConditionElementListSyntax getDeferredConditions();
   ParsedCodeBlockSyntax getDeferredBody();

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::WhileStmt;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ParsedWhileStmtSyntaxBuilder;
};

class ParsedDoWhileStmtSyntax final : public ParsedStmtSyntax
{
public:
   explicit ParsedDoWhileStmtSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedStmtSyntax(std::move(rawNode))
   {}

   std::optional<ParsedTokenSyntax> getDeferredLabelName();
   std::optional<ParsedTokenSyntax> getDeferredLabelColon();
   ParsedTokenSyntax getDeferredDoKeyword();
   ParsedCodeBlockSyntax getDeferredBody();
   ParsedTokenSyntax getDeferredWhileKeyword();
   ParsedTokenSyntax getDeferredLeftParen();
   ParsedExprSyntax getDeferredCondition();
   ParsedTokenSyntax getDeferredRightParen();

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::DoWhileStmt;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ParsedDoWhileStmtSyntaxBuilder;
};

class ParsedSwitchDefaultLabelSyntax final : public ParsedStmtSyntax
{
public:
   explicit ParsedSwitchDefaultLabelSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedStmtSyntax(std::move(rawNode))
   {}

   ParsedTokenSyntax getDeferredDefaultKeyword();
   ParsedTokenSyntax getDeferredColon();

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::SwitchDefaultLabel;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ParsedSwitchDefaultLabelSyntaxBuilder;
};

class ParsedSwitchCaseLabelSyntax final : public ParsedStmtSyntax
{
public:
   explicit ParsedSwitchCaseLabelSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedStmtSyntax(std::move(rawNode))
   {}

   ParsedTokenSyntax getDeferredCaseKeyword();
   ParsedExprSyntax getDeferredExpr();
   ParsedTokenSyntax getDeferredColon();

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::SwitchCaseLabel;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ParsedSwitchCaseLabelSyntaxBuilder;
};


} // polar::parser

#endif // POLARPHP_PARSER_PARSED_SYNTAX_NODE_PARSED_STMT_SYNTAX_NODES_H
