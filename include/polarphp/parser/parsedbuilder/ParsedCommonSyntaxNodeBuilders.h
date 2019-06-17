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

#ifndef POLARPHP_PARSER_PARSED_COMMON_SYNTAX_NODE_BUILDERS_H
#define POLARPHP_PARSER_PARSED_COMMON_SYNTAX_NODE_BUILDERS_H

#include "polarphp/parser/ParsedRawSyntaxNode.h"
#include "polarphp/parser/ParsedSyntaxNodes.h"
#include "polarphp/parser/SyntaxParsingContext.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"

namespace polar::parser {

using namespace polar::syntax;
using polar::basic::SmallVector;

class ParsedRawSyntaxRecorder;
class SyntaxParsingContext;

class ParsedCodeBlockItemSyntaxBuilder
{
public:
   using Cursor = CodeBlockItemSyntax::Cursor;

public:
   explicit ParsedCodeBlockItemSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}

   ParsedCodeBlockItemSyntaxBuilder &useItem(ParsedSyntax item);
   ParsedCodeBlockItemSyntaxBuilder &useSemicolon(ParsedTokenSyntax semicolon);
   ParsedCodeBlockItemSyntaxBuilder &useErrorTokens(ParsedSyntax errorTokens);

   ParsedCodeBlockItemSyntax build();
   ParsedCodeBlockItemSyntax makeDeferred();

private:
   ParsedCodeBlockItemSyntax record();
   void finishLayout(bool deferred);

   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[CodeBlockItemSyntax::CHILDREN_COUNT];
};

class ParsedCodeBlockSyntaxBuilder
{
public:
   using Cursor = CodeBlockSyntax::Cursor;

public:
   explicit ParsedCodeBlockSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}

   ParsedCodeBlockSyntaxBuilder &useLeftBrace(ParsedTokenSyntax leftBrace);
   ParsedCodeBlockSyntaxBuilder &useStatements(ParsedCodeBlockItemListSyntax statements);
   ParsedCodeBlockSyntaxBuilder &addStatementMember(ParsedCodeBlockItemSyntax statemenet);
   ParsedCodeBlockSyntaxBuilder &useRightBrace(ParsedTokenSyntax rightBrace);

   ParsedCodeBlockSyntax build();
   ParsedCodeBlockSyntax makeDeferred();
private:
   ParsedCodeBlockSyntax record();
   void finishLayout(bool deferred);

   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[CodeBlockSyntax::CHILDREN_COUNT];
   SmallVector<ParsedRawSyntaxNode, 8> m_statementsMembers;
};

} // polar::parser

#endif // POLARPHP_PARSER_PARSED_COMMON_SYNTAX_NODE_BUILDERS_H
