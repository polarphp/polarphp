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

#ifndef POLARPHP_PARSER_PARSED_RECORDER_PARSED_COMMON_SYNTAX_NODE_RECORDER_H
#define POLARPHP_PARSER_PARSED_RECORDER_PARSED_COMMON_SYNTAX_NODE_RECORDER_H

#include "polarphp/parser/parsedsyntaxnode/ParsedCommonSyntaxNodes.h"
#include "polarphp/parser/ParsedAbstractSyntaxNodeRecorder.h"
#include "polarphp/syntax/SyntaxKind.h"

namespace polar::parser {

class ParsedRawSyntaxRecorder;
class SyntaxParsingContext;

class ParsedCommonSyntaxNodeRecorder : public AbstractSyntaxNodeRecorder
{
public:
   ///
   /// normal nodes
   ///
   static ParsedCodeBlockItemSyntax deferCodeBlockItem(ParsedSyntax item, ParsedTokenSyntax semicolon,
                                                       std::optional<ParsedSyntax> errorTokens, SyntaxParsingContext &context);
   static ParsedCodeBlockItemSyntax makeCodeBlockItem(ParsedSyntax item, ParsedTokenSyntax semicolon,
                                                      std::optional<ParsedSyntax> errorTokens, SyntaxParsingContext &context);

   static ParsedCodeBlockSyntax deferCodeBlock(ParsedTokenSyntax leftBrace, ParsedCodeBlockItemListSyntax statements,
                                               ParsedTokenSyntax rightBrace, SyntaxParsingContext &context);
   static ParsedCodeBlockSyntax makeCodeBlock(ParsedTokenSyntax leftBrace, ParsedCodeBlockItemListSyntax statements,
                                              ParsedTokenSyntax rightBrace, SyntaxParsingContext &context);
   ///
   /// collection nodes
   ///
   static ParsedCodeBlockItemListSyntax deferCodeBlockItemList(ArrayRef<ParsedCodeBlockItemSyntax> elements,
                                                               SyntaxParsingContext &context);
   static ParsedCodeBlockItemListSyntax makeCodeBlockItemList(ArrayRef<ParsedCodeBlockItemSyntax> elements,
                                                              SyntaxParsingContext &context);
   static ParsedCodeBlockItemListSyntax makeBlankCodeBlockItemList(SourceLoc loc, SyntaxParsingContext &context);

   static ParsedTokenListSyntax deferTokenList(ArrayRef<ParsedTokenSyntax> elements,
                                               SyntaxParsingContext &context);
   static ParsedTokenListSyntax makeTokenList(ArrayRef<ParsedTokenSyntax> elements,
                                              SyntaxParsingContext &context);
   static ParsedTokenListSyntax makeBlankTokenList(SourceLoc loc, SyntaxParsingContext &context);

   static ParsedNonEmptyTokenListSyntax deferNonEmptyTokenList(ArrayRef<ParsedTokenSyntax> elements,
                                                               SyntaxParsingContext &context);
   static ParsedNonEmptyTokenListSyntax makeNonEmptyTokenList(ArrayRef<ParsedTokenSyntax> elements,
                                                              SyntaxParsingContext &context);
   static ParsedNonEmptyTokenListSyntax makeBlankNonEmptyTokenList(SourceLoc loc, SyntaxParsingContext &context);

private:
   ///
   /// normal nodes
   ///
   static ParsedCodeBlockItemSyntax recordCodeBlockItem(ParsedSyntax item, ParsedTokenSyntax semicolon,
                                                        std::optional<ParsedSyntax> errorTokens, ParsedRawSyntaxRecorder &recorder);
   static ParsedCodeBlockSyntax recordCodeBlock(ParsedTokenSyntax leftBrace, ParsedCodeBlockItemListSyntax statements,
                                                ParsedTokenSyntax rightBrace, ParsedRawSyntaxRecorder &recorder);
   ///
   /// collection nodes
   ///
   static ParsedCodeBlockItemListSyntax recordCodeBlockItemList(ArrayRef<ParsedCodeBlockItemSyntax> elements,
                                                                ParsedRawSyntaxRecorder &recorder);
   static ParsedTokenListSyntax recordTokenList(ArrayRef<ParsedTokenSyntax> elements,
                                                ParsedRawSyntaxRecorder &recorder);
   static ParsedNonEmptyTokenListSyntax recordNonEmptyTokenList(ArrayRef<ParsedTokenSyntax> elements,
                                                                ParsedRawSyntaxRecorder &recorder);

};

} // polar::parser

#endif // POLARPHP_PARSER_PARSED_RECORDER_PARSED_COMMON_SYNTAX_NODE_RECORDER_H
