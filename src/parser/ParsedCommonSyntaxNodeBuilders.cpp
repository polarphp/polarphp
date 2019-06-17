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

#include "polarphp/parser/parsedbuilder/ParsedCommonSyntaxNodeBuilders.h"
#include "polarphp/parser/ParsedRawSyntaxRecorder.h"
#include "polarphp/parser/SyntaxParsingContext.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"

namespace polar::parser {

ParsedCodeBlockItemSyntax ParsedCodeBlockItemSyntaxBuilder::build()
{
   if (m_context.isBacktracking()) {
      return makeDeferred();
   }
   return record();
}

ParsedCodeBlockItemSyntaxBuilder &ParsedCodeBlockItemSyntaxBuilder::useItem(ParsedSyntax item)
{
   m_layout[cursor_index(Cursor::Item)] = item.getRaw();
   return *this;
}

ParsedCodeBlockItemSyntaxBuilder &ParsedCodeBlockItemSyntaxBuilder::useSemicolon(ParsedTokenSyntax semicolon)
{
   m_layout[cursor_index(Cursor::Semicolon)] = semicolon.getRaw();
   return *this;
}

ParsedCodeBlockItemSyntaxBuilder &ParsedCodeBlockItemSyntaxBuilder::useErrorTokens(ParsedSyntax errorTokens)
{
   m_layout[cursor_index(Cursor::ErrorTokens)] = errorTokens.getRaw();
   return *this;
}

ParsedCodeBlockItemSyntax ParsedCodeBlockItemSyntaxBuilder::makeDeferred()
{
   finishLayout(/*deferred=*/true);
   ParsedRawSyntaxNode raw = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::CodeBlockItem,
                                                               m_layout, m_context);
   return ParsedCodeBlockItemSyntax(std::move(raw));
}

ParsedCodeBlockItemSyntax ParsedCodeBlockItemSyntaxBuilder::record()
{
   finishLayout(/*deferred=*/false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode raw = recorder.recordRawSyntax(SyntaxKind::CodeBlockItem, m_layout);
   return ParsedCodeBlockItemSyntax(std::move(raw));
}

void ParsedCodeBlockItemSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void)recorder;
   CursorIndex itemIndex = cursor_index(Cursor::Item);
   CursorIndex semicolonIndex = cursor_index(Cursor::Semicolon);
   if (m_layout[itemIndex].isNull()) {
      polar_unreachable("need missing non-token nodes ?");
   }
   if (m_layout[semicolonIndex].isNull()) {
      if (deferred) {
         m_layout[semicolonIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_SEMICOLON, SourceLoc());
      } else {
         m_layout[semicolonIndex] = recorder.recordMissingToken(TokenKindType::T_SEMICOLON, SourceLoc());
      }
   }
}

} // polar::parser
