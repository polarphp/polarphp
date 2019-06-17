// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/14.

#include "polarphp/syntax/builder/CommonSyntaxNodeBuilders.h"

namespace polar::syntax {

CodeBlockItemSyntaxBuilder &CodeBlockItemSyntaxBuilder::useItem(Syntax item)
{
   m_layout[cursor_index(Cursor::Item)] = item.getRaw();
   return *this;
}

CodeBlockItemSyntaxBuilder &CodeBlockItemSyntaxBuilder::useSemicolon(TokenSyntax semicolon)
{
   m_layout[cursor_index(Cursor::Semicolon)] = semicolon.getRaw();
   return *this;
}

CodeBlockItemSyntaxBuilder &CodeBlockItemSyntaxBuilder::useErrorTokens(Syntax errorTokens)
{
   m_layout[cursor_index(Cursor::ErrorTokens)] = errorTokens.getRaw();
   return *this;
}

CodeBlockItemSyntax CodeBlockItemSyntaxBuilder::build()
{
   /// ensure node exist
   CursorIndex itemNodeIndex = cursor_index(Cursor::Item);
   CursorIndex semicolonIndex = cursor_index(Cursor::Semicolon);
   if (!m_layout[itemNodeIndex]) {
      m_layout[itemNodeIndex] = RawSyntax::missing(SyntaxKind::Unknown);
   }
   if (!m_layout[semicolonIndex]) {
      m_layout[itemNodeIndex] = RawSyntax::missing(TokenKindType::T_SEMICOLON,
                                                   OwnedString::makeUnowned(get_token_text(TokenKindType::T_SEMICOLON)));
   }
   auto raw = RawSyntax::make(SyntaxKind::CodeBlockItem,
                              m_layout, SourcePresence::Present, m_arena);
   return make<CodeBlockItemSyntax>(raw);
}

CodeBlockSyntaxBuilder &CodeBlockSyntaxBuilder::useLeftBrace(TokenSyntax leftBrace)
{
   m_layout[cursor_index(Cursor::LeftBrace)] = leftBrace.getRaw();
   return *this;
}

CodeBlockSyntaxBuilder &CodeBlockSyntaxBuilder::useRightBrace(TokenSyntax rightBrace)
{
   m_layout[cursor_index(Cursor::RightBrace)] = rightBrace.getRaw();
   return *this;
}

CodeBlockSyntaxBuilder &CodeBlockSyntaxBuilder::useStatements(CodeBlockItemListSyntax stmts)
{
   m_layout[cursor_index(Cursor::Statements)] = stmts.getRaw();
   return *this;
}

CodeBlockSyntaxBuilder &CodeBlockSyntaxBuilder::addCodeBlockItem(CodeBlockItemSyntax stmt)
{
   auto &raw = m_layout[cursor_index(Cursor::Statements)];
   if (!raw) {
      raw = RawSyntax::make(SyntaxKind::CodeBlockItemList, stmt.getRaw(), SourcePresence::Present, m_arena);
   } else {
      raw = raw->append(stmt.getRaw());
   }
   return *this;
}

CodeBlockSyntax CodeBlockSyntaxBuilder::build()
{
   /// ensure node exist
   CursorIndex leftBraceIndex = cursor_index(Cursor::LeftBrace);
   CursorIndex stmtsIndex = cursor_index(Cursor::Statements);
   CursorIndex rightBraceIndex = cursor_index(Cursor::RightBrace);
   if (!m_layout[leftBraceIndex]) {
      m_layout[leftBraceIndex] = RawSyntax::missing(TokenKindType::T_LEFT_BRACE,
                                                    OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_BRACE)));
   }
   if (!m_layout[rightBraceIndex]) {
      m_layout[rightBraceIndex] = RawSyntax::missing(TokenKindType::T_RIGHT_BRACE,
                                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_BRACE)));
   }
   if (!m_layout[stmtsIndex]) {
      m_layout[stmtsIndex] = RawSyntax::missing(SyntaxKind::CodeBlockItemList);
   }

   auto raw = RawSyntax::make(SyntaxKind::CodeBlock,
                              m_layout, SourcePresence::Present, m_arena);
   return make<CodeBlockSyntax>(raw);
}

} // polar::syntax
