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

#include "polarphp/parser/parsedbuilder/ParsedDeclSyntaxNodeBuilders.h"
#include "polarphp/parser/ParsedRawSyntaxRecorder.h"
#include "polarphp/parser/SyntaxParsingContext.h"
#include "polarphp/parser/parsedsyntaxnode/ParsedDeclSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"

namespace polar::parser {

ParsedSourceFileSyntaxBuilder
&ParsedSourceFileSyntaxBuilder::useStatements(ParsedCodeBlockItemListSyntax statements)
{
   assert(m_statementsMembers.empty() && "use either 'use' function or 'add', not both");
   m_layout[cursor_index(Cursor::Statements)] = statements.getRaw();
   return *this;
}

ParsedSourceFileSyntaxBuilder
&ParsedSourceFileSyntaxBuilder::addStatmentMember(ParsedCodeBlockItemSyntax statement)
{
   assert(m_layout[cursor_index(Cursor::Statements)].isNull() && "use either 'use' function or 'add', not both");
   m_statementsMembers.push_back(std::move(statement.getRaw()));
   return *this;
}

ParsedSourceFileSyntaxBuilder
&ParsedSourceFileSyntaxBuilder::useEofToken(ParsedTokenSyntax eofToken)
{
   m_layout[cursor_index(Cursor::EOFToken)] = eofToken.getRaw();
   return *this;
}

ParsedSourceFileSyntax ParsedSourceFileSyntaxBuilder::build()
{
   if (m_context.isBacktracking()) {
      return makeDeferred();
   }
   return record();
}

ParsedSourceFileSyntax ParsedSourceFileSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::SourceFile, m_layout, m_context);
   return ParsedSourceFileSyntax(std::move(rawNode));
}

ParsedSourceFileSyntax ParsedSourceFileSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::SourceFile, m_layout);
   return ParsedSourceFileSyntax(std::move(rawNode));
}

void ParsedSourceFileSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void)recorder;
   CursorIndex statementsIndex = cursor_index(Cursor::Statements);
   CursorIndex eofTokenIndex = cursor_index(Cursor::EOFToken);
   if (!m_statementsMembers.empty()) {
      if (deferred) {
         m_layout[statementsIndex] = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::CodeBlockItemList, m_statementsMembers, m_context);
      } else {
         m_layout[statementsIndex] = recorder.recordRawSyntax(SyntaxKind::CodeBlockItemList, m_statementsMembers);
      }
   }
   if (m_layout[statementsIndex].isNull()) {
      if (deferred) {
         m_layout[statementsIndex] = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::CodeBlockItemList, {}, m_context);
      } else {
         m_layout[statementsIndex] = recorder.recordRawSyntax(SyntaxKind::CodeBlockItemList, {});
      }
   }
   if (m_layout[eofTokenIndex].isNull()){
      if (deferred) {
          m_layout[eofTokenIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::END, SourceLoc());
      } else {
         m_layout[eofTokenIndex] = recorder.recordMissingToken(TokenKindType::END, SourceLoc());
      }
   }
}

} // polar::parser
