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

const std::map<SyntaxChildrenCountType, std::set<SyntaxKind>> CodeBlockItemSyntax::CHILD_NODE_CHOICES{
   {CodeBlockItemSyntax::Cursor::Item, {
         SyntaxKind::Decl,
               SyntaxKind::Expr,
               SyntaxKind::Stmt,
               SyntaxKind::TokenList,
               SyntaxKind::NonEmptyTokenList
      }}
};

void CodeBlockItemSyntax::validate()
{
   auto raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == CodeBlockItemSyntax::CHILDREN_COUNT);
   /// validate every child token choices
   /// validate every child token text choices
   /// validate every child node choices
   if (auto &item = raw->getChild(Cursor::Item)) {
      bool declStatus = DeclSyntax::kindOf(item->getKind());
      bool stmtStatus = StmtSyntax::kindOf(item->getKind());
      bool tokenListStatus = TokenListSyntax::kindOf(item->getKind());
      bool nonEmptyTokenListStatus = NonEmptyTokenListSyntax::kindOf(item->getKind());
      assert(declStatus || stmtStatus || tokenListStatus || nonEmptyTokenListStatus);
   }
}

Syntax CodeBlockItemSyntax::getItem()
{
   return Syntax{m_root, m_data->getChild(Cursor::Item).get()};
}

TokenSyntax CodeBlockItemSyntax::getSemicolon()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Semicolon).get()};
}

std::optional<Syntax> CodeBlockItemSyntax::getErrorTokens()
{
   auto childData = m_data->getChild(Cursor::ErrorTokens);
   if (!childData) {
      return std::nullopt;
   }
   return Syntax{m_root, childData.get()};
}

CodeBlockItemSyntax CodeBlockItemSyntax::withItem(std::optional<Syntax> item)
{
   RefCountPtr<RawSyntax> raw;
   if (item.has_value()) {
      raw = item->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::Decl);
   }
   return m_data->replaceChild<CodeBlockItemSyntax>(raw, Cursor::Item);
}

CodeBlockItemSyntax CodeBlockItemSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> raw;
   if (semicolon.has_value()) {
      raw = semicolon->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_SEMICOLON,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_SEMICOLON)));
   }
   return m_data->replaceChild<CodeBlockItemSyntax>(raw, Cursor::Item);
}

CodeBlockItemSyntax CodeBlockItemSyntax::withErrorTokens(std::optional<Syntax> errorTokens)
{
   RefCountPtr<RawSyntax> raw;
   if (errorTokens.has_value()) {
      raw = errorTokens->getRaw();
   } else {
      raw = nullptr;
   }
   return m_data->replaceChild<CodeBlockItemSyntax>(raw, Cursor::ErrorTokens);
}

void CodeBlockSyntax::validate()
{
   auto raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == CodeBlockSyntax::CHILDREN_COUNT);
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
   RefCountPtr<RawSyntax> raw = getRaw()->getChild(Cursor::Statements);
   if (raw) {
      raw = raw->append(codeBlockItem.getRaw());
   } else {
      raw = RawSyntax::make(SyntaxKind::CodeBlockItemList, {codeBlockItem.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<CodeBlockSyntax>(raw, Cursor::Statements);
}

CodeBlockSyntax CodeBlockSyntax::withLeftBrace(std::optional<TokenSyntax> leftBrace)
{
   RefCountPtr<RawSyntax> raw;
   if (leftBrace.has_value()) {
      raw = leftBrace->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_LEFT_BRACE,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_BRACE)));
   }
   return m_data->replaceChild<CodeBlockSyntax>(raw, Cursor::LeftBrace);
}

CodeBlockSyntax CodeBlockSyntax::withRightBrace(std::optional<TokenSyntax> rightBrace)
{
   RefCountPtr<RawSyntax> raw;
   if (rightBrace.has_value()) {
      raw = rightBrace->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_RIGHT_BRACE,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_BRACE)));
   }
   return m_data->replaceChild<CodeBlockSyntax>(raw, Cursor::RightBrace);
}

CodeBlockSyntax CodeBlockSyntax::withStatements(std::optional<CodeBlockItemListSyntax> statements)
{
   RefCountPtr<RawSyntax> raw;
   if (statements.has_value()) {
      raw = statements->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::CodeBlockItemList);
   }
   return m_data->replaceChild<CodeBlockSyntax>(raw, Cursor::Statements);
}

} // polar::syntax
