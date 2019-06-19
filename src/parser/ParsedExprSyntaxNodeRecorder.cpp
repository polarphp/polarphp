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

#include "polarphp/parser/parsedrecorder/ParsedExprSyntaxNodeRecorder.h"
#include "polarphp/parser/ParsedRawSyntaxRecorder.h"
#include "polarphp/parser/SyntaxParsingContext.h"

namespace polar::parser {

/// ======================================= normal nodes =======================================
///
/// NullExpr
///
ParsedNullExprSyntax
ParsedExprSyntaxNodeRecorder::recordNullExpr(ParsedTokenSyntax nullKeyword, ParsedRawSyntaxRecorder &recorder)
{
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::NullExpr, {
                                                             nullKeyword.getRaw()
                                                          });
   return ParsedNullExprSyntax(std::move(rawNode));
}

ParsedNullExprSyntax
ParsedExprSyntaxNodeRecorder::deferNullExpr(ParsedTokenSyntax nullKeyword, SyntaxParsingContext &context)
{
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::NullExpr, {
                                                                      nullKeyword.getRaw()
                                                                   }, context);
   return ParsedNullExprSyntax(std::move(rawNode));
}

ParsedNullExprSyntax
ParsedExprSyntaxNodeRecorder::makeNullExpr(ParsedTokenSyntax nullKeyword, SyntaxParsingContext &context)
{
   if (context.isBacktracking()) {
      return deferNullExpr(nullKeyword, context);
   }
   return recordNullExpr(nullKeyword, context.getRecorder());
}

///
/// ClassRefParentExpr
///
ParsedClassRefParentExprSyntax
ParsedExprSyntaxNodeRecorder::recordClassRefParentExpr(ParsedTokenSyntax parentKeyword, ParsedRawSyntaxRecorder &recorder)
{
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::ClassRefParentExpr, {
                                                             parentKeyword.getRaw()
                                                          });
   return ParsedClassRefParentExprSyntax(std::move(rawNode));
}

ParsedClassRefParentExprSyntax
ParsedExprSyntaxNodeRecorder::deferClassRefParentExpr(ParsedTokenSyntax parentKeyword, SyntaxParsingContext &context)
{
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::ClassRefParentExpr, {
                                                                      parentKeyword.getRaw()
                                                                   }, context);
   return ParsedClassRefParentExprSyntax(std::move(rawNode));
}

ParsedClassRefParentExprSyntax
ParsedExprSyntaxNodeRecorder::makeClassRefParentExpr(ParsedTokenSyntax parentKeyword, SyntaxParsingContext &context)
{
   if (context.isBacktracking()) {
      return deferClassRefParentExpr(parentKeyword, context);
   }
   return recordClassRefParentExpr(parentKeyword, context.getRecorder());
}

///
/// ClassRefSelfExpr
///
ParsedClassRefSelfExprSyntax
ParsedExprSyntaxNodeRecorder::recordClassRefSelfExpr(ParsedTokenSyntax selfKeyword, ParsedRawSyntaxRecorder &recorder)
{
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::ClassRefSelfExpr, {
                                                             selfKeyword.getRaw()
                                                          });
   return ParsedClassRefSelfExprSyntax(std::move(rawNode));
}

ParsedClassRefSelfExprSyntax
ParsedExprSyntaxNodeRecorder::deferClassRefSelfExpr(ParsedTokenSyntax selfKeyword, SyntaxParsingContext &context)
{
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::ClassRefSelfExpr, {
                                                                      selfKeyword.getRaw()
                                                                   }, context);
   return ParsedClassRefSelfExprSyntax(std::move(rawNode));
}

ParsedClassRefSelfExprSyntax
ParsedExprSyntaxNodeRecorder::makeClassRefSelfExpr(ParsedTokenSyntax selfKeyword, SyntaxParsingContext &context)
{
   if (context.isBacktracking()) {
      return deferClassRefSelfExpr(selfKeyword, context);
   }
   return recordClassRefSelfExpr(selfKeyword, context.getRecorder());
}

} // polar::parser
