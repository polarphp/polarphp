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

#ifndef POLARPHP_PARSER_PARSED_DECL_SYNTAX_NODE_BUILDERS_H
#define POLARPHP_PARSER_PARSED_DECL_SYNTAX_NODE_BUILDERS_H

#include "polarphp/parser/ParsedRawSyntaxNode.h"
#include "polarphp/parser/ParsedSyntaxNodes.h"
#include "polarphp/parser/SyntaxParsingContext.h"
#include "polarphp/syntax/syntaxnode/DeclSyntaxNodes.h"

namespace polar::parser {

using namespace polar::syntax;
using polar::basic::SmallVector;

class ParsedRawSyntaxRecorder;
class SyntaxParsingContext;

class ParsedSourceFileSyntaxBuilder
{
public:
   using Cursor = SourceFileSyntax::Cursor;
public:
   explicit ParsedSourceFileSyntaxBuilder(SyntaxParsingContext &context)
      : m_context(context)
   {}

   ParsedSourceFileSyntaxBuilder &useStatements(ParsedCodeBlockItemListSyntax statements);
   ParsedSourceFileSyntaxBuilder &addStatmentMember(ParsedCodeBlockItemSyntax statement);
   ParsedSourceFileSyntaxBuilder &useEofToken(ParsedTokenSyntax eofToken);

   ParsedSourceFileSyntax build();
   ParsedSourceFileSyntax makeDeferred();
private:
   ParsedSourceFileSyntax record();
   void finishLayout(bool deferred);

   SyntaxParsingContext &m_context;
   ParsedRawSyntaxNode m_layout[SourceFileSyntax::CHILDREN_COUNT];
   SmallVector<ParsedRawSyntaxNode, 8> m_statementsMembers;
};

} // polar::parser

#endif // POLARPHP_PARSER_PARSED_DECL_SYNTAX_NODE_BUILDERS_H
