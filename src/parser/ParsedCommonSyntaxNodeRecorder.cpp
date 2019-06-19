// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/19.

#include "polarphp/parser/parsedrecorder/ParsedCommonSyntaxNodeRecorder.h"
#include "polarphp/parser/ParsedRawSyntaxRecorder.h"
#include "polarphp/parser/SyntaxParsingContext.h"

namespace polar::parser {

using namespace polar::syntax;

/// ======================================= normal nodes =======================================
///
/// CodeBlockItem
///
ParsedCodeBlockItemSyntax
ParsedCommonSyntaxNodeRecorder::deferCodeBlockItem(ParsedSyntax item, ParsedTokenSyntax semicolon,
                                                   std::optional<ParsedSyntax> errorTokens, SyntaxParsingContext &context)
{
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::CodeBlockItem, {
                                                                      item.getRaw(),
                                                                      semicolon.getRaw(),
                                                                      errorTokens.has_value() ? errorTokens->getRaw() : ParsedRawSyntaxNode::null()
                                                                   }, context);
   return ParsedCodeBlockItemSyntax(std::move(rawNode));
}

ParsedCodeBlockItemSyntax
ParsedCommonSyntaxNodeRecorder::recordCodeBlockItem(ParsedSyntax item, ParsedTokenSyntax semicolon,
                                                    std::optional<ParsedSyntax> errorTokens, ParsedRawSyntaxRecorder &recorder)
{
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::CodeBlockItem, {
                                                             item.getRaw(),
                                                             semicolon.getRaw(),
                                                             errorTokens.has_value() ? errorTokens->getRaw() : ParsedRawSyntaxNode::null()
                                                          });
   return ParsedCodeBlockItemSyntax(std::move(rawNode));
}

ParsedCodeBlockItemSyntax
ParsedCommonSyntaxNodeRecorder::makeCodeBlockItem(ParsedSyntax item, ParsedTokenSyntax semicolon,
                                                  std::optional<ParsedSyntax> errorTokens, SyntaxParsingContext &context)
{
   if (context.isBacktracking()) {
      return makeCodeBlockItem(item, semicolon, errorTokens, context);
   }
   return recordCodeBlockItem(item, semicolon, errorTokens, context.getRecorder());
}

///
/// CodeBlockItem
///
ParsedCodeBlockSyntax
ParsedCommonSyntaxNodeRecorder::recordCodeBlock(ParsedTokenSyntax leftBrace, ParsedCodeBlockItemListSyntax statements,
                                                ParsedTokenSyntax rightBrace, ParsedRawSyntaxRecorder &recorder)
{
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::CodeBlock, {
                                                             leftBrace.getRaw(),
                                                             statements.getRaw(),
                                                             rightBrace.getRaw()
                                                          });
   return ParsedCodeBlockSyntax(std::move(rawNode));
}

ParsedCodeBlockSyntax
ParsedCommonSyntaxNodeRecorder::deferCodeBlock(ParsedTokenSyntax leftBrace, ParsedCodeBlockItemListSyntax statements,
                                               ParsedTokenSyntax rightBrace, SyntaxParsingContext &context)
{
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::CodeBlock, {
                                                                      leftBrace.getRaw(),
                                                                      statements.getRaw(),
                                                                      rightBrace.getRaw()
                                                                   }, context);
   return ParsedCodeBlockSyntax(std::move(rawNode));
}

ParsedCodeBlockSyntax
ParsedCommonSyntaxNodeRecorder::makeCodeBlock(ParsedTokenSyntax leftBrace, ParsedCodeBlockItemListSyntax statements,
                                              ParsedTokenSyntax rightBrace, SyntaxParsingContext &context)
{
   if (context.isBacktracking()) {
      return deferCodeBlock(leftBrace, statements, rightBrace, context);
   }
   return recordCodeBlock(leftBrace, statements, rightBrace, context.getRecorder());
}

/// ======================================= collection nodes =======================================

///
/// CodeBlockItemList
///
ParsedCodeBlockItemListSyntax
ParsedCommonSyntaxNodeRecorder::recordCodeBlockItemList(ArrayRef<ParsedCodeBlockItemSyntax> elements,
                                                        ParsedRawSyntaxRecorder &recorder)
{
   SmallVector<ParsedRawSyntaxNode, 16> layout;
   layout.reserve(elements.size());
   for (const ParsedCodeBlockItemSyntax &item : elements) {
      layout.push_back(item.getRaw());
   }
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::CodeBlockItemList, layout);
   return ParsedCodeBlockItemListSyntax(std::move(rawNode));
}

ParsedCodeBlockItemListSyntax
ParsedCommonSyntaxNodeRecorder::deferCodeBlockItemList(ArrayRef<ParsedCodeBlockItemSyntax> elements,
                                                       SyntaxParsingContext &context)
{
   SmallVector<ParsedRawSyntaxNode, 16> layout;
   layout.reserve(elements.size());
   for (const ParsedCodeBlockItemSyntax &item : elements) {
      layout.push_back(item.getRaw());
   }
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::CodeBlockItemList, layout, context);
   return ParsedCodeBlockItemListSyntax(std::move(rawNode));
}

ParsedCodeBlockItemListSyntax
ParsedCommonSyntaxNodeRecorder::makeCodeBlockItemList(ArrayRef<ParsedCodeBlockItemSyntax> elements,
                                                      SyntaxParsingContext &context)
{
   if (context.isBacktracking()) {
      return deferCodeBlockItemList(elements, context);
   }
   return recordCodeBlockItemList(elements, context.getRecorder());
}

ParsedCodeBlockItemListSyntax
ParsedCommonSyntaxNodeRecorder::makeBlankCodeBlockItemList(SourceLoc loc, SyntaxParsingContext &context)
{
   ParsedRawSyntaxNode rawNode;
   if (context.isBacktracking()) {
      // FIXME: 'loc' is not preserved when capturing a deferred layout.
      rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::CodeBlockItemList, {}, context);
   } else {
      rawNode = context.getRecorder().recordEmptyRawSyntaxCollection(SyntaxKind::CodeBlockItemList, loc);
   }
   return ParsedCodeBlockItemListSyntax(std::move(rawNode));
}

///
/// TokenList
///
ParsedTokenListSyntax
ParsedCommonSyntaxNodeRecorder::recordTokenList(ArrayRef<ParsedTokenSyntax> elements,
                                                ParsedRawSyntaxRecorder &recorder)
{
   SmallVector<ParsedRawSyntaxNode, 16> layout;
   layout.reserve(elements.size());
   for (const ParsedTokenSyntax &item : elements) {
      layout.push_back(item.getRaw());
   }
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::TokenList, layout);
   return ParsedTokenListSyntax(std::move(rawNode));
}

ParsedTokenListSyntax
ParsedCommonSyntaxNodeRecorder::deferTokenList(ArrayRef<ParsedTokenSyntax> elements,
                                               SyntaxParsingContext &context)
{
   SmallVector<ParsedRawSyntaxNode, 16> layout;
   layout.reserve(elements.size());
   for (const ParsedTokenSyntax &item : elements) {
      layout.push_back(item.getRaw());
   }
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::TokenList, layout, context);
   return ParsedTokenListSyntax(std::move(rawNode));
}

ParsedTokenListSyntax
ParsedCommonSyntaxNodeRecorder::makeTokenList(ArrayRef<ParsedTokenSyntax> elements,
                                              SyntaxParsingContext &context)
{
   if (context.isBacktracking()) {
      return deferTokenList(elements, context);
   }
   return recordTokenList(elements, context.getRecorder());
}

ParsedTokenListSyntax
ParsedCommonSyntaxNodeRecorder::makeBlankTokenList(SourceLoc loc, SyntaxParsingContext &context)
{
   ParsedRawSyntaxNode rawNode;
   if (context.isBacktracking()) {
      // FIXME: 'loc' is not preserved when capturing a deferred layout.
      rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::TokenList, {}, context);
   } else {
      rawNode = context.getRecorder().recordEmptyRawSyntaxCollection(SyntaxKind::TokenList, loc);
   }
   return ParsedTokenListSyntax(std::move(rawNode));
}

ParsedNonEmptyTokenListSyntax
ParsedCommonSyntaxNodeRecorder::recordNonEmptyTokenList(ArrayRef<ParsedTokenSyntax> elements,
                                                      ParsedRawSyntaxRecorder &recorder)
{
   SmallVector<ParsedRawSyntaxNode, 16> layout;
   layout.reserve(elements.size());
   for (const ParsedTokenSyntax &item : elements) {
      layout.push_back(item.getRaw());
   }
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::NonEmptyTokenList, layout);
   return ParsedNonEmptyTokenListSyntax(std::move(rawNode));
}

ParsedNonEmptyTokenListSyntax
ParsedCommonSyntaxNodeRecorder::deferNonEmptyTokenList(ArrayRef<ParsedTokenSyntax> elements,
                                                     SyntaxParsingContext &context)
{
   SmallVector<ParsedRawSyntaxNode, 16> layout;
   layout.reserve(elements.size());
   for (const ParsedTokenSyntax &item : elements) {
      layout.push_back(item.getRaw());
   }
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::NonEmptyTokenList, layout, context);
   return ParsedNonEmptyTokenListSyntax(std::move(rawNode));
}

ParsedNonEmptyTokenListSyntax
ParsedCommonSyntaxNodeRecorder::makeNonEmptyTokenList(ArrayRef<ParsedTokenSyntax> elements,
                                                    SyntaxParsingContext &context)
{
   if (context.isBacktracking()) {
      return deferNonEmptyTokenList(elements, context);
   }
   return recordNonEmptyTokenList(elements, context.getRecorder());
}

ParsedNonEmptyTokenListSyntax
ParsedCommonSyntaxNodeRecorder::makeBlankNonEmptyTokenList(SourceLoc loc, SyntaxParsingContext &context)
{
   ParsedRawSyntaxNode rawNode;
   if (context.isBacktracking()) {
      // FIXME: 'loc' is not preserved when capturing a deferred layout.
      rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::NonEmptyTokenList, {}, context);
   } else {
      rawNode = context.getRecorder().recordEmptyRawSyntaxCollection(SyntaxKind::NonEmptyTokenList, loc);
   }
   return ParsedNonEmptyTokenListSyntax(std::move(rawNode));
}

} // polar::parser
