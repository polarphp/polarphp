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
#include "polarphp/syntax/SyntaxKind.h"

namespace polar::parser {

class ParsedRawSyntaxRecorder;
class SyntaxParsingContext;

class ParsedCommonSyntaxNodeRecorder
{
public:
   ///
   /// normal nodes
   ///
   static ParsedCodeBlockItemSyntax deferSyntax(ParsedSyntax item, ParsedTokenSyntax semicolon,
                                                std::optional<ParsedSyntax> errorTokens, SyntaxParsingContext &context);
   static ParsedCodeBlockItemSyntax makeSyntax(ParsedSyntax item, ParsedTokenSyntax semicolon,
                                               std::optional<ParsedSyntax> errorTokens, SyntaxParsingContext &context);

   static ParsedCodeBlockSyntax deferSyntax(ParsedTokenSyntax leftBrace, ParsedCodeBlockItemListSyntax statements,
                                            ParsedTokenSyntax rightBrace, SyntaxParsingContext &context);
   static ParsedCodeBlockSyntax makeSyntax(ParsedTokenSyntax leftBrace, ParsedCodeBlockItemListSyntax statements,
                                           ParsedTokenSyntax rightBrace, SyntaxParsingContext &context);
   ///
   /// collection nodes
   ///
private:
   ///
   /// normal nodes
   ///
   static ParsedCodeBlockItemSyntax recordSyntax(ParsedSyntax item, ParsedTokenSyntax semicolon,
                                                 std::optional<ParsedSyntax> errorTokens, ParsedRawSyntaxRecorder &recorder);
   static ParsedCodeBlockSyntax recordSyntax(ParsedTokenSyntax leftBrace, ParsedCodeBlockItemListSyntax statements,
                                             ParsedTokenSyntax rightBrace, ParsedRawSyntaxRecorder &recorder);
   ///
   /// collection nodes
   ///
};

} // polar::parser

#endif // POLARPHP_PARSER_PARSED_RECORDER_PARSED_COMMON_SYNTAX_NODE_RECORDER_H
