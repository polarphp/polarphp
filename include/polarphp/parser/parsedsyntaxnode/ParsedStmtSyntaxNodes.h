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

} // polar::parser

#endif // POLARPHP_PARSER_PARSED_SYNTAX_NODE_PARSED_STMT_SYNTAX_NODES_H
