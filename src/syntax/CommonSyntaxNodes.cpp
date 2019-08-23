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

#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"
#include "polarphp/syntax/TokenKinds.h"

namespace polar::syntax {
///
/// CodeBlockItemSyntax
///
#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType CodeBlockItemSyntax::CHILD_NODE_CHOICES
{
   {
      CodeBlockItemSyntax::Item, {
         SyntaxKind::Decl,
               SyntaxKind::Expr,
               SyntaxKind::Stmt,
      }
   }
};
#endif // POLAR_DEBUG_BUILD

void CodeBlockItemSyntax::validate()
{
   auto raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == CodeBlockItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Item, CHILD_NODE_CHOICES.at(Cursor::Item));
}

Syntax CodeBlockItemSyntax::getItem()
{
   return Syntax{m_root, m_data->getChild(Cursor::Item).get()};
}

TokenSyntax CodeBlockItemSyntax::getSemicolon()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Semicolon).get()};
}

CodeBlockItemSyntax CodeBlockItemSyntax::withItem(std::optional<Syntax> item)
{
   RefCountPtr<RawSyntax> rawItem;
   if (item.has_value()) {
      rawItem = item->getRaw();
   } else {
      rawItem = RawSyntax::missing(SyntaxKind::Decl);
   }
   return m_data->replaceChild<CodeBlockItemSyntax>(rawItem, Cursor::Item);
}

CodeBlockItemSyntax CodeBlockItemSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = nullptr;
   }
   return m_data->replaceChild<CodeBlockItemSyntax>(rawSemicolon, Cursor::Item);
}

///
/// CodeBlockSyntax
///
void CodeBlockSyntax::validate()
{
   auto raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == CodeBlockSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LeftBrace, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_kind(raw, Statements, std::set{SyntaxKind::CodeBlockItemList});
   syntax_assert_child_token(raw, RightBrace, std::set{TokenKindType::T_RIGHT_PAREN});
}

TokenSyntax CodeBlockSyntax::getLeftBrace()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftBrace).get()};
}

TokenSyntax CodeBlockSyntax::getRightBrace()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::RightBrace).get()};
}

CodeBlockItemListSyntax CodeBlockSyntax::getStatements()
{
   return CodeBlockItemListSyntax{m_root, m_data->getChild(Cursor::Statements).get()};
}

CodeBlockSyntax CodeBlockSyntax::addCodeBlockItem(CodeBlockItemSyntax codeBlockItem)
{
   RefCountPtr<RawSyntax> statements = getRaw()->getChild(Cursor::Statements);
   if (statements) {
      statements = statements->append(codeBlockItem.getRaw());
   } else {
      statements = RawSyntax::make(SyntaxKind::CodeBlockItemList, {codeBlockItem.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<CodeBlockSyntax>(statements, Cursor::Statements);
}

CodeBlockSyntax CodeBlockSyntax::withLeftBrace(std::optional<TokenSyntax> leftBrace)
{
   RefCountPtr<RawSyntax> rawLeftBrace;
   if (leftBrace.has_value()) {
      rawLeftBrace = leftBrace->getRaw();
   } else {
      rawLeftBrace = RawSyntax::missing(TokenKindType::T_LEFT_BRACE,
                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_BRACE)));
   }
   return m_data->replaceChild<CodeBlockSyntax>(rawLeftBrace, Cursor::LeftBrace);
}

CodeBlockSyntax CodeBlockSyntax::withRightBrace(std::optional<TokenSyntax> rightBrace)
{
   RefCountPtr<RawSyntax> rawRightBrace;
   if (rightBrace.has_value()) {
      rawRightBrace = rightBrace->getRaw();
   } else {
      rawRightBrace = RawSyntax::missing(TokenKindType::T_RIGHT_BRACE,
                                         OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_BRACE)));
   }
   return m_data->replaceChild<CodeBlockSyntax>(rawRightBrace, Cursor::RightBrace);
}

CodeBlockSyntax CodeBlockSyntax::withStatements(std::optional<CodeBlockItemListSyntax> statements)
{
   RefCountPtr<RawSyntax> rawStatements;
   if (statements.has_value()) {
      rawStatements = statements->getRaw();
   } else {
      rawStatements = RawSyntax::missing(SyntaxKind::CodeBlockItemList);
   }
   return m_data->replaceChild<CodeBlockSyntax>(rawStatements, Cursor::Statements);
}

} // polar::syntax
