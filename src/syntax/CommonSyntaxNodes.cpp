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
   RefCountPtr<SyntaxData> errorTokensData = m_data->getChild(Cursor::ErrorTokens);
   if (!errorTokensData) {
      return std::nullopt;
   }
   return Syntax{m_root, errorTokensData.get()};
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
      rawSemicolon = RawSyntax::missing(TokenKindType::T_SEMICOLON,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_SEMICOLON)));
   }
   return m_data->replaceChild<CodeBlockItemSyntax>(rawSemicolon, Cursor::Item);
}

CodeBlockItemSyntax CodeBlockItemSyntax::withErrorTokens(std::optional<Syntax> errorTokens)
{
   RefCountPtr<RawSyntax> rawErrorTokens;
   if (errorTokens.has_value()) {
      rawErrorTokens = errorTokens->getRaw();
   } else {
      rawErrorTokens = nullptr;
   }
   return m_data->replaceChild<CodeBlockItemSyntax>(rawErrorTokens, Cursor::ErrorTokens);
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
