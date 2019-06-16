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

#include "polarphp/syntax/syntaxnode/DeclSyntaxNodes.h"

namespace polar::syntax {

void SourceFileSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SourceFileSyntax::CHILDREN_COUNT);
}

TokenSyntax SourceFileSyntax::getEofToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::EOFToken).get()};
}

CodeBlockItemListSyntax SourceFileSyntax::getStatements()
{
   return CodeBlockItemListSyntax{m_root, m_data->getChild(Cursor::Statements).get()};
}

SourceFileSyntax SourceFileSyntax::withStatements(std::optional<CodeBlockItemListSyntax> statements)
{
   RefCountPtr<RawSyntax> rawStatements;
   if (statements.has_value()) {
      rawStatements = statements->getRaw();
   } else {
      rawStatements = RawSyntax::missing(SyntaxKind::CodeBlockItemList);
   }
   return m_data->replaceChild<SourceFileSyntax>(rawStatements, Cursor::Statements);
}

SourceFileSyntax SourceFileSyntax::addStatement(CodeBlockItemSyntax statement)
{
   RefCountPtr<RawSyntax> rawStatements = getRaw()->getChild(Cursor::Statements);
   if (rawStatements) {
      rawStatements->append(statement.getRaw());
   } else {
      rawStatements = RawSyntax::make(SyntaxKind::CodeBlockItemList, {statement.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<SourceFileSyntax>(rawStatements, Cursor::Statements);
}

SourceFileSyntax SourceFileSyntax::withEofToken(std::optional<TokenSyntax> eofToken)
{
   RefCountPtr<RawSyntax> rawEofToken;
   if (eofToken.has_value()) {
      rawEofToken = eofToken->getRaw();
   } else {
      rawEofToken = RawSyntax::missing(TokenKindType::END, OwnedString::makeUnowned(""));
   }
   return m_data->replaceChild<SourceFileSyntax>(rawEofToken, Cursor::EOFToken);
}

} // polar::syntax
