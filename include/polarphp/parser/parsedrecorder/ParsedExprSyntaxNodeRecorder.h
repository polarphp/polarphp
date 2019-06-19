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

#ifndef POLARPHP_PARSER_PARSED_RECORDER_PARSED_EXPR_SYNTAX_NODE_RECORDER_H
#define POLARPHP_PARSER_PARSED_RECORDER_PARSED_EXPR_SYNTAX_NODE_RECORDER_H

#include "polarphp/parser/parsedsyntaxnode/ParsedCommonSyntaxNodes.h"
#include "polarphp/parser/parsedsyntaxnode/ParsedExprSyntaxNodes.h"
#include "polarphp/parser/ParsedAbstractSyntaxNodeRecorder.h"
#include "polarphp/syntax/SyntaxKind.h"

namespace polar::parser {

class ParsedRawSyntaxRecorder;
class SyntaxParsingContext;

class ParsedExprSyntaxNodeRecorder : public AbstractSyntaxNodeRecorder
{
public:
   ///
   /// normal nodes
   ///
   ParsedNullExprSyntax deferNullExpr(ParsedTokenSyntax nullKeyword, SyntaxParsingContext &context);
   ParsedNullExprSyntax makeNullExpr(ParsedTokenSyntax nullKeyword, SyntaxParsingContext &context);

   ParsedClassRefParentExprSyntax deferClassRefParentExpr(ParsedTokenSyntax parentKeyword, SyntaxParsingContext &context);
   ParsedClassRefParentExprSyntax makeClassRefParentExpr(ParsedTokenSyntax parentKeyword, SyntaxParsingContext &context);

   ParsedClassRefSelfExprSyntax deferClassRefSelfExpr(ParsedTokenSyntax selfKeyword, SyntaxParsingContext &context);
   ParsedClassRefSelfExprSyntax makeClassRefSelfExpr(ParsedTokenSyntax selfKeyword, SyntaxParsingContext &context);

   ParsedClassRefStaticExprSyntax deferClassRefStaticExpr(ParsedTokenSyntax staticKeyword, SyntaxParsingContext &context);
   ParsedClassRefStaticExprSyntax makeClassRefStaticExpr(ParsedTokenSyntax staticKeyword, SyntaxParsingContext &context);

   ParsedIntegerLiteralExprSyntax deferIntegerLiteralExpr(ParsedTokenSyntax digits, SyntaxParsingContext &context);
   ParsedIntegerLiteralExprSyntax makeIntegerLiteralExpr(ParsedTokenSyntax digits, SyntaxParsingContext &context);

   ParsedFloatLiteralExprSyntax deferFloatLiteralExpr(ParsedTokenSyntax floatDigits, SyntaxParsingContext &context);
   ParsedFloatLiteralExprSyntax makeFloatLiteralExpr(ParsedTokenSyntax floatDigits, SyntaxParsingContext &context);

   ParsedStringLiteralExprSyntax deferStringLiteralExpr(ParsedTokenSyntax str, SyntaxParsingContext &context);
   ParsedStringLiteralExprSyntax makeStringLiteralExpr(ParsedTokenSyntax str, SyntaxParsingContext &context);

   ParsedBooleanLiteralExprSyntax deferBooleanLiteralExpr(ParsedTokenSyntax booleanToken, SyntaxParsingContext &context);
   ParsedBooleanLiteralExprSyntax makeBooleanLiteralExpr(ParsedTokenSyntax booleanToken, SyntaxParsingContext &context);

   ParsedTernaryExprSyntax deferTernaryExpr(ParsedExprSyntax conditionExpr, ParsedTokenSyntax questionMark,
                                            ParsedExprSyntax firstChoice, ParsedTokenSyntax colonMark,
                                            ParsedExprSyntax secondChoice, SyntaxParsingContext &context);
   ParsedTernaryExprSyntax makeTernaryExpr(ParsedExprSyntax conditionExpr, ParsedTokenSyntax questionMark,
                                           ParsedExprSyntax firstChoice, ParsedTokenSyntax colonMark,
                                           ParsedExprSyntax secondChoice, SyntaxParsingContext &context);

   ParsedAssignmentExprSyntax deferAssignmentExpr(ParsedTokenSyntax assignToken, SyntaxParsingContext &context);
   ParsedAssignmentExprSyntax makeAssignmentExpr(ParsedTokenSyntax assignToken, SyntaxParsingContext &context);

   ParsedSequenceExprSyntax deferSequenceExpr(ParsedExprListSyntax elements, SyntaxParsingContext &context);
   ParsedSequenceExprSyntax makeSequenceExpr(ParsedExprListSyntax elements, SyntaxParsingContext &context);

   ParsedPrefixOperatorExprSyntax deferPrefixOperatorExpr(std::optional<ParsedTokenSyntax> operatorToken,
                                                          ParsedExprSyntax expr,
                                                          SyntaxParsingContext &context);
   ParsedPrefixOperatorExprSyntax makePrefixOperatorExpr(std::optional<ParsedTokenSyntax> operatorToken,
                                                         ParsedExprSyntax expr, SyntaxParsingContext &context);

   ParsedPostfixOperatorExprSyntax deferPostfixOperatorExpr(ParsedExprSyntax expr, ParsedTokenSyntax operatorToken,
                                                            SyntaxParsingContext &context);
   ParsedPostfixOperatorExprSyntax makePostfixOperatorExpr(ParsedExprSyntax expr, ParsedTokenSyntax operatorToken,
                                                           SyntaxParsingContext &context);

   ParsedBinaryOperatorExprSyntax deferBinaryOperatorExpr(ParsedTokenSyntax OperatorToken, SyntaxParsingContext &context);
   ParsedBinaryOperatorExprSyntax makeBinaryOperatorExpr(ParsedTokenSyntax OperatorToken, SyntaxParsingContext &context);

   ///
   /// collection nodes
   ///
   ParsedExprListSyntax deferExprList(ArrayRef<ParsedExprSyntax> elements, SyntaxParsingContext &context);
   ParsedExprListSyntax makeExprList(ArrayRef<ParsedExprSyntax> elements, SyntaxParsingContext &context);
   ParsedExprListSyntax makeBlankExprList(SourceLoc loc, SyntaxParsingContext &context);

private:
   ///
   /// normal nodes
   ///
   ParsedNullExprSyntax recordNullExpr(ParsedTokenSyntax nullKeyword, ParsedRawSyntaxRecorder &recorder);
   ParsedClassRefParentExprSyntax recordClassRefParentExpr(ParsedTokenSyntax parentKeyword, ParsedRawSyntaxRecorder &recorder);
   ParsedClassRefSelfExprSyntax recordClassRefSelfExpr(ParsedTokenSyntax selfKeyword, ParsedRawSyntaxRecorder &recorder);
   ParsedClassRefStaticExprSyntax recordClassRefStaticExpr(ParsedTokenSyntax staticKeyword, ParsedRawSyntaxRecorder &recorder);
   ParsedIntegerLiteralExprSyntax recordIntegerLiteralExpr(ParsedTokenSyntax digits, ParsedRawSyntaxRecorder &recorder);
   ParsedFloatLiteralExprSyntax recordFloatLiteralExpr(ParsedTokenSyntax floatDigits, ParsedRawSyntaxRecorder &recorder);
   ParsedStringLiteralExprSyntax recordStringLiteralExpr(ParsedTokenSyntax str, ParsedRawSyntaxRecorder &recorder);
   ParsedBooleanLiteralExprSyntax recordBooleanLiteralExpr(ParsedTokenSyntax booleanToken, ParsedRawSyntaxRecorder &recorder);
   ParsedTernaryExprSyntax recordTernaryExpr(ParsedExprSyntax conditionExpr, ParsedTokenSyntax questionMark,
                                             ParsedExprSyntax firstChoice, ParsedTokenSyntax colonMark,
                                             ParsedExprSyntax secondChoice, ParsedRawSyntaxRecorder &recorder);
   ParsedAssignmentExprSyntax recordAssignmentExpr(ParsedTokenSyntax assignToken, ParsedRawSyntaxRecorder &recorder);
   ParsedAssignmentExprSyntax recordSequenceExpr(ParsedExprListSyntax elements, ParsedRawSyntaxRecorder &recorder);
   ParsedAssignmentExprSyntax recordPrefixOperatorExpr(std::optional<ParsedTokenSyntax> operatorToken,
                                                       ParsedExprSyntax expr, ParsedRawSyntaxRecorder &recorder);
   ParsedAssignmentExprSyntax recordPostfixOperatorExpr(ParsedExprSyntax expr, ParsedTokenSyntax operatorToken,
                                                        ParsedRawSyntaxRecorder &recorder);
   ParsedAssignmentExprSyntax recordBinaryOperatorExpr(ParsedTokenSyntax OperatorToken, ParsedRawSyntaxRecorder &recorder);

   ///
   /// collection nodes
   ///
   ParsedExprListSyntax recordExprList(ArrayRef<ParsedExprSyntax> elements, SyntaxParsingContext &context);
};

} // polar::parser

#endif // POLARPHP_PARSER_PARSED_RECORDER_PARSED_EXPR_SYNTAX_NODE_RECORDER_H
