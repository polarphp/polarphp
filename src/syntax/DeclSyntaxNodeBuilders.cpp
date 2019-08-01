// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/17.

#include "polarphp/syntax/builder/DeclSyntaxNodeBuilders.h"

namespace polar::syntax {

//SourceFileSyntaxBuilder &SourceFileSyntaxBuilder::useStatements(CodeBlockItemListSyntax statements)
//{
//   m_layout[cursor_index(Cursor::Statements)] = statements.getRaw();
//   return *this;
//}

//SourceFileSyntaxBuilder &SourceFileSyntaxBuilder::addStatement(CodeBlockItemSyntax statement)
//{
//   RefCountPtr<RawSyntax> &rawStatemens = m_layout[cursor_index(Cursor::Statements)];
//   if (!rawStatemens) {
//      rawStatemens = RawSyntax::make(SyntaxKind::CodeBlockItemList, {statement.getRaw()}, SourcePresence::Present, m_arena);
//   } else {
//      rawStatemens = rawStatemens->append(statement.getRaw());
//   }
//   return *this;
//}

SourceFileSyntaxBuilder &SourceFileSyntaxBuilder::useEofToken(TokenSyntax eofToken)
{
   m_layout[cursor_index(Cursor::EOFToken)] = eofToken.getRaw();
   return *this;
}

SourceFileSyntax SourceFileSyntaxBuilder::build()
{
   CursorIndex statementsIndex = cursor_index(Cursor::Statements);
   CursorIndex eofTokenIndex = cursor_index(Cursor::EOFToken);
   if (!m_layout[statementsIndex]) {
      m_layout[statementsIndex] = RawSyntax::missing(SyntaxKind::CodeBlockItemList);
   }
   if (!m_layout[eofTokenIndex]) {
      m_layout[eofTokenIndex] = RawSyntax::missing(TokenKindType::END, OwnedString::makeUnowned(""));
   }
   RefCountPtr<RawSyntax> raw = RawSyntax::make(SyntaxKind::SourceFile, m_layout, SourcePresence::Present, m_arena);
   return make<SourceFileSyntax>(raw);
}

} // polar::syntax
