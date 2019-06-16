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

#ifndef POLARPHP_PARSER_PARSED_SYNTAX_NODE_PARSED_EXPR_SYNTAX_NODES_H
#define POLARPHP_PARSER_PARSED_SYNTAX_NODE_PARSED_EXPR_SYNTAX_NODES_H

#include "polarphp/parser/ParsedSyntax.h"
#include "polarphp/syntax/SyntaxKind.h"
#include "polarphp/parser/parsedsyntaxnode/ParsedCommonSyntaxNodes.h"

namespace polar::parser {

class ParsedNullExprSyntax;
class ParsedClassRefParentExprSyntax;
class ParsedClassRefStaticExprSyntax;
class ParsedClassRefSelfExprSyntax;
class ParsedIntegerLiteralExprSyntax;
class ParsedFloatLiteralExprSyntax;
class ParsedStringLiteralExprSyntax;
class ParsedBooleanLiteralExprSyntax;
class ParsedTernaryExprSyntax;
class ParsedAssignmentExprSyntax;
class ParsedSequenceExprSyntax;
class ParsedPrefixOperatorExprSyntax;
class ParsedPostfixOperatorExprSyntax;
class ParsedBinaryOperatorExprSyntax;
using ParsedExprListSyntax = ParsedSyntaxCollection<SyntaxKind::ExprList>;

class ParsedNullExprSyntax final : public ParsedExprSyntax
{
public:
   explicit ParsedNullExprSyntax(ParsedRawSyntaxNode rawNode)
      : ParsedExprSyntax(std::move(rawNode))
   {}

   ParsedTokenSyntax getDeferredNullKeyword();

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NullExpr;
   }

   static bool classOf(const ParsedSyntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ParsedNullExprSyntaxBuilder;
};

} // polar::parser

#endif // POLARPHP_PARSER_PARSED_SYNTAX_NODE_PARSED_EXPR_SYNTAX_NODES_H
