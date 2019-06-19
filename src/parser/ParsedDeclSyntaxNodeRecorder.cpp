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

#include "polarphp/parser/parsedrecorder/ParsedDeclSyntaxNodeRecorder.h"
#include "polarphp/parser/ParsedRawSyntaxRecorder.h"
#include "polarphp/parser/SyntaxParsingContext.h"

namespace polar::parser {

/// ======================================= normal nodes =======================================

ParsedSourceFileSyntax
ParsedDeclSyntaxNodeRecorder::recordSourceFile(ParsedCodeBlockItemListSyntax statements, ParsedTokenSyntax eofToken,
                                               ParsedRawSyntaxRecorder &recorder)
{

   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::CodeBlockItem, {
                                                             statements.getRaw(),
                                                             eofToken.getRaw()
                                                          });
   return ParsedSourceFileSyntax(std::move(rawNode));
}

ParsedSourceFileSyntax
ParsedDeclSyntaxNodeRecorder::deferSourceFile(ParsedCodeBlockItemListSyntax statements, ParsedTokenSyntax eofToken,
                                              SyntaxParsingContext &context)
{
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::SourceFile, {
                                                                      statements.getRaw(),
                                                                      eofToken.getRaw()
                                                                   }, context);
   return ParsedSourceFileSyntax(std::move(rawNode));
}

ParsedSourceFileSyntax
ParsedDeclSyntaxNodeRecorder::makeSourceFile(ParsedCodeBlockItemListSyntax statements, ParsedTokenSyntax eofToken,
                                             SyntaxParsingContext &context)
{
   if (context.isBacktracking()) {
      return makeSourceFile(statements, eofToken, context);
   }
   return recordSourceFile(statements, eofToken, context.getRecorder());
}

} // polar::parser
