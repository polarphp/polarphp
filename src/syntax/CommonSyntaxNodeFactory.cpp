// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/15.

#include "polarphp/syntax/factory/CommonSyntaxNodeFactory.h"

namespace polar::syntax {

Syntax CommonSyntaxNodeFactory::makeBlankCollectionSyntax(SyntaxKind kind)
{

}

CodeBlockItemSyntax CommonSyntaxNodeFactory::makeBlankCodeBlockItem(RefCountPtr<SyntaxArena> arena)
{
   auto raw = RawSyntax::make(SyntaxKind::CodeBlockItem, {
                                 RawSyntax::missing(SyntaxKind::Unknown),
                                 RawSyntax::missing(TokenKindType::T_SEMICOLON, OwnedString::makeUnowned(get_token_text(TokenKindType::T_SEMICOLON))),
                                 nullptr
                              }, SourcePresence::Present, arena);
   return make<CodeBlockItemSyntax>(raw);
}

CodeBlockSyntax CommonSyntaxNodeFactory::makeBlankCodeBlock(RefCountPtr<SyntaxArena> arena)
{
   auto raw = RawSyntax::make(SyntaxKind::CodeBlock, {
                                 RawSyntax::missing(TokenKindType::T_LEFT_BRACE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_BRACE))),
                                 RawSyntax::missing(SyntaxKind::CodeBlockItemList),
                                 RawSyntax::missing(TokenKindType::T_RIGHT_BRACE, OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_BRACE)))
                              }, SourcePresence::Present, arena);
   return make<CodeBlockSyntax>(raw);
}

CodeBlockItemListSyntax CommonSyntaxNodeFactory::makeBlankCodeBlockItemList(RefCountPtr<SyntaxArena> arena)
{
   auto raw = RawSyntax::make(SyntaxKind::CodeBlockItemList, {}, SourcePresence::Present, arena);
   return make<CodeBlockItemListSyntax>(raw);
}

TokenListSyntax CommonSyntaxNodeFactory::makeBlankTokenList(RefCountPtr<SyntaxArena> arena)
{
   auto raw = RawSyntax::make(SyntaxKind::TokenList, {}, SourcePresence::Present, arena);
   return make<TokenListSyntax>(raw);
}

NonEmptyTokenListSyntax CommonSyntaxNodeFactory::makeBlankNonEmptyTokenList(RefCountPtr<SyntaxArena> arena)
{
   auto raw = RawSyntax::make(SyntaxKind::NonEmptyTokenList, {}, SourcePresence::Present, arena);
   return make<NonEmptyTokenListSyntax>(raw);
}

/// make syntax collection node
CodeBlockItemListSyntax CommonSyntaxNodeFactory::makeCodeBlockItemList(const std::vector<CodeBlockItemSyntax> &elements,
                                                               RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (auto &element : elements) {
      layout.push_back(element.getRaw());
   }
   auto raw = RawSyntax::make(SyntaxKind::CodeBlockItemList,
                              layout, SourcePresence::Present, arena);
   return make<CodeBlockItemListSyntax>(raw);
}

TokenListSyntax CommonSyntaxNodeFactory::makeTokenList(const std::vector<TokenSyntax> &elements,
                                               RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (auto &element : elements) {
      layout.push_back(element.getRaw());
   }
   auto raw = RawSyntax::make(SyntaxKind::TokenList,
                              layout, SourcePresence::Present, arena);
   return make<TokenListSyntax>(raw);
}

NonEmptyTokenListSyntax CommonSyntaxNodeFactory::makeNonEmptyTokenList(const std::vector<TokenSyntax> &elements,
                                                               RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (auto &element : elements) {
      layout.push_back(element.getRaw());
   }
   auto raw = RawSyntax::make(SyntaxKind::NonEmptyTokenList,
                              layout, SourcePresence::Present, arena);
   return make<NonEmptyTokenListSyntax>(raw);
}

/// make has children syntax node

CodeBlockItemSyntax CommonSyntaxNodeFactory::makeCodeBlockItem(Syntax item, TokenSyntax semicolon,
                                                       std::optional<TokenSyntax> errorTokens, RefCountPtr<SyntaxArena> arena)
{
   auto raw = RawSyntax::make(SyntaxKind::CodeBlockItem, {
                                 item.getRaw(),
                                 semicolon.getRaw(),
                                 errorTokens.has_value() ? errorTokens->getRaw() : nullptr
                              }, SourcePresence::Present, arena);
   return make<CodeBlockItemSyntax>(raw);
}

CodeBlockSyntax CommonSyntaxNodeFactory::makeCodeBlock(TokenSyntax leftBrace, CodeBlockItemListSyntax statements,
                                               TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena)
{
   auto raw = RawSyntax::make(SyntaxKind::CodeBlock, {
                                 leftBrace.getRaw(),
                                 statements.getRaw(),
                                 rightBrace.getRaw()
                              }, SourcePresence::Present, arena);
   return make<CodeBlockSyntax>(raw);
}

} // polar::syntax
