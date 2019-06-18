// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/17.

#ifndef POLARPHP_PARSER_PARSED_STMT_SYNTAX_NODE_BUILDERS_H
#define POLARPHP_PARSER_PARSED_STMT_SYNTAX_NODE_BUILDERS_H

#include "polarphp/parser/ParsedRawSyntaxNode.h"
#include "polarphp/parser/ParsedSyntaxNodes.h"
#include "polarphp/parser/SyntaxParsingContext.h"
#include "polarphp/syntax/syntaxnode/StmtSyntaxNodes.h"

namespace polar::parser {

using namespace polar::syntax;
using polar::basic::SmallVector;

class ParsedRawSyntaxRecorder;
class SyntaxParsingContext;

class ParsedConditionElementSyntaxBuilder
{
public:
   using Cursor = ConditionElementSyntax::Cursor;
public:
   explicit ParsedConditionElementSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}

   ParsedConditionElementSyntaxBuilder &useCondition(ParsedSyntax condition);
   ParsedConditionElementSyntaxBuilder &useTrailingComma(ParsedTokenSyntax trailingComma);

   ParsedConditionElementSyntax build();
   ParsedConditionElementSyntax makeDeferred();
private:
   ParsedConditionElementSyntax record();
   void finishLayout(bool deferred);

   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[ConditionElementSyntax::CHILDREN_COUNT];
};

class ParsedContinueStmtSyntaxBuilder
{
public:
   using Cursor = ContinueStmtSyntax::Cursor;
public:
   explicit ParsedContinueStmtSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}

   ParsedContinueStmtSyntaxBuilder &useContinueKeyword(ParsedTokenSyntax continueKeyword);
   ParsedContinueStmtSyntaxBuilder &useLNumberToken(ParsedTokenSyntax numberToken);

   ParsedContinueStmtSyntax build();
   ParsedContinueStmtSyntax makeDeferred();
private:
   ParsedContinueStmtSyntax record();
   void finishLayout(bool deferred);

   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[ContinueStmtSyntax::CHILDREN_COUNT];
};

class ParsedBreakStmtSyntaxBuilder
{
public:
   using Cursor = BreakStmtSyntax::Cursor;
public:
   explicit ParsedBreakStmtSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}

   ParsedBreakStmtSyntaxBuilder &useBreakKeyword(ParsedTokenSyntax breakKeyword);
   ParsedBreakStmtSyntaxBuilder &useLNumberToken(ParsedTokenSyntax numberToken);

   ParsedBreakStmtSyntax build();
   ParsedBreakStmtSyntax makeDeferred();
private:
   ParsedBreakStmtSyntax record();
   void finishLayout(bool deferred);

   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[BreakStmtSyntax::CHILDREN_COUNT];
};

class ParsedFallthroughStmtSyntaxBuilder
{
public:
   using Cursor = FallthroughStmtSyntax::Cursor;
public:
   explicit ParsedFallthroughStmtSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}

   ParsedFallthroughStmtSyntaxBuilder &useFallthroughKeyword(ParsedTokenSyntax fallthroughKeyword);

   ParsedFallthroughStmtSyntax build();
   ParsedFallthroughStmtSyntax makeDeferred();
private:
   ParsedFallthroughStmtSyntax record();
   void finishLayout(bool deferred);

   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[FallthroughStmtSyntax::CHILDREN_COUNT];
};

class ParsedElseIfClauseSyntaxBuilder
{
public:
   using Cursor = ElseIfClauseSyntax::Cursor;
public:
   explicit ParsedElseIfClauseSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}

   ParsedElseIfClauseSyntaxBuilder &useElseIfKeyword(ParsedTokenSyntax elseIfKeyword);
   ParsedElseIfClauseSyntaxBuilder &useLeftParen(ParsedTokenSyntax leftParen);
   ParsedElseIfClauseSyntaxBuilder &useCondition(ParsedSyntax condition);
   ParsedElseIfClauseSyntaxBuilder &useRightParen(ParsedTokenSyntax rightParen);
   ParsedElseIfClauseSyntaxBuilder &useBody(ParsedCodeBlockSyntax body);

   ParsedElseIfClauseSyntax build();
   ParsedElseIfClauseSyntax makeDeferred();
private:
   ParsedElseIfClauseSyntax record();
   void finishLayout(bool deferred);

   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[ElseIfClauseSyntax::CHILDREN_COUNT];
};


} // polar::parser

#endif // POLARPHP_PARSER_PARSED_STMT_SYNTAX_NODE_BUILDERS_H
