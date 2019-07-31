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

#ifndef POLARPHP_SYNTAX_FACTORY_EXPR_SYNTAX_NODE_FACTORY_H
#define POLARPHP_SYNTAX_FACTORY_EXPR_SYNTAX_NODE_FACTORY_H

#include "polarphp/syntax/AbstractFactory.h"
#include "polarphp/syntax/syntaxnode/ExprSyntaxNodes.h"
#include <optional>

namespace polar::syntax {

class ExprSyntaxNodeFactory final : public AbstractFactory
{
public:
   static Syntax makeBlankCollectionSyntax(SyntaxKind kind);
   ///
   /// make collection nodes
   ///
   static ExprListSyntax makeExprListSyntax(const std::vector<ExprSyntax> elements, RefCountPtr<SyntaxArena> arena = nullptr);

   ///
   /// make normal nodes
   ///
   static NullExprSyntax makeNullExprSyntax(TokenSyntax nullKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefParentExprSyntax makeClassRefParentExprSyntax(TokenSyntax parentKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefSelfExprSyntax makeClassRefSelfExprSyntax(TokenSyntax selfKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefStaticExprSyntax makeClassRefStaticExprSyntax(TokenSyntax staticKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static IntegerLiteralExprSyntax makeIntegerLiteralExprSyntax(TokenSyntax digits, RefCountPtr<SyntaxArena> arena = nullptr);
   static FloatLiteralExprSyntax makeFloatLiteralExprSyntax(TokenSyntax floatDigits, RefCountPtr<SyntaxArena> arena = nullptr);
   static StringLiteralExprSyntax makeStringLiteralExprSyntax(TokenSyntax str, RefCountPtr<SyntaxArena> arena = nullptr);
   static BooleanLiteralExprSyntax makeBooleanLiteralExprSyntax(TokenSyntax boolean, RefCountPtr<SyntaxArena> arena = nullptr);
   static TernaryExprSyntax makeTernaryExprSyntax(ExprSyntax conditionExpr, TokenSyntax questionMark, ExprSyntax firstChoice,
                                                  TokenSyntax colonMark, ExprSyntax secondChoice, RefCountPtr<SyntaxArena> arena = nullptr);
   static AssignmentExprSyntax makeAssignmentExprSyntax(TokenSyntax assignToken, RefCountPtr<SyntaxArena> arena = nullptr);
   static SequenceExprSyntax makeSequenceExprSyntax(ExprListSyntax elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static PrefixOperatorExprSyntax makePrefixOperatorExprSyntax(std::optional<TokenSyntax> operatorToken, ExprSyntax expr,
                                                                RefCountPtr<SyntaxArena> arena = nullptr);
   static PostfixOperatorExprSyntax makePostfixOperatorExprSyntax(ExprSyntax expr, TokenSyntax operatorToken,
                                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static BinaryOperatorExprSyntax makeBinaryOperatorExprSyntax(TokenSyntax operatorToken, RefCountPtr<SyntaxArena> arena = nullptr);

   /// make blank nodes
   static ExprListSyntax makeBlankExprListSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static NullExprSyntax makeNullExprSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefParentExprSyntax makeBlankClassRefParentExprSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefSelfExprSyntax makeBlankClassRefSelfExprSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefStaticExprSyntax makeBlankClassRefStaticExprSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static IntegerLiteralExprSyntax makeBlankIntegerLiteralExprSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static FloatLiteralExprSyntax makeBlankFloatLiteralExprSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static StringLiteralExprSyntax makeBlankStringLiteralExprSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static BooleanLiteralExprSyntax makeBlankBooleanLiteralExprSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static TernaryExprSyntax makeBlankTernaryExprSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static AssignmentExprSyntax makeBlankAssignmentExprSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static SequenceExprSyntax makeBlankSequenceExprSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static PrefixOperatorExprSyntax makeBlankPrefixOperatorExprSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static PostfixOperatorExprSyntax makeBlankPostfixOperatorExprSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static BinaryOperatorExprSyntax makeBlankBinaryOperatorExprSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_FACTORY_EXPR_SYNTAX_NODE_FACTORY_H
