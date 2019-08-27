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
#include "polarphp/syntax/syntaxnode/ExprSyntaxNodesFwd.h"
#include <optional>

namespace polar::syntax {

class ExprSyntaxNodeFactory final : public AbstractFactory
{
public:
   static Syntax makeBlankCollection(SyntaxKind kind);
   ///
   /// make collection nodes
   ///
   static ExprListSyntax makeExprList(const std::vector<ExprSyntax> elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static LexicalVarListSyntax makeLexicalVarList(const std::vector<LexicalVarItemSyntax> elements,
                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayPairItemListSyntax makeArrayPairItemList(const std::vector<ArrayPairItemSyntax> elements,
                                                        RefCountPtr<SyntaxArena> arena = nullptr);
   static ListPairItemListSyntax makeListPairItemList(const std::vector<ListPairItemSyntax> elements,
                                                      RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsItemListSyntax makeEncapsItemList(const std::vector<EncapsListItemSyntax> elements,
                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static ArgumentListSyntax makeArgumentList(const std::vector<ArgumentListItemSyntax> elements,
                                              RefCountPtr<SyntaxArena> arena = nullptr);
   static IssetVariablesListSyntax makeIssetVariablesList(const std::vector<IsSetVarItemSyntax> elements,
                                                          RefCountPtr<SyntaxArena> arena = nullptr);
   ///
   /// make normal nodes
   ///
   static NullExprSyntax makeNullExpr(TokenSyntax nullKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefParentExprSyntax makeClassRefParentExpr(TokenSyntax parentKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefSelfExprSyntax makeClassRefSelfExpr(TokenSyntax selfKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefStaticExprSyntax makeClassRefStaticExpr(TokenSyntax staticKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static IntegerLiteralExprSyntax makeIntegerLiteralExpr(TokenSyntax digits, RefCountPtr<SyntaxArena> arena = nullptr);
   static FloatLiteralExprSyntax makeFloatLiteralExpr(TokenSyntax floatDigits, RefCountPtr<SyntaxArena> arena = nullptr);
   static StringLiteralExprSyntax makeStringLiteralExpr(TokenSyntax str, RefCountPtr<SyntaxArena> arena = nullptr);
   static BooleanLiteralExprSyntax makeBooleanLiteralExpr(TokenSyntax boolean, RefCountPtr<SyntaxArena> arena = nullptr);
   static TernaryExprSyntax makeTernaryExpr(ExprSyntax conditionExpr, TokenSyntax questionMark, ExprSyntax firstChoice,
                                                  TokenSyntax colonMark, ExprSyntax secondChoice, RefCountPtr<SyntaxArena> arena = nullptr);
   static AssignmentExprSyntax makeAssignmentExpr(TokenSyntax assignToken, RefCountPtr<SyntaxArena> arena = nullptr);
   static SequenceExprSyntax makeSequenceExpr(ExprListSyntax elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static PrefixOperatorExprSyntax makePrefixOperatorExpr(std::optional<TokenSyntax> operatorToken, ExprSyntax expr,
                                                                RefCountPtr<SyntaxArena> arena = nullptr);
   static PostfixOperatorExprSyntax makePostfixOperatorExpr(ExprSyntax expr, TokenSyntax operatorToken,
                                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static BinaryOperatorExprSyntax makeBinaryOperatorExpr(TokenSyntax operatorToken, RefCountPtr<SyntaxArena> arena = nullptr);

   /// make blank nodes
   static ExprListSyntax makeBlankExprList(RefCountPtr<SyntaxArena> arena = nullptr);
   static NullExprSyntax makeNullExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefParentExprSyntax makeBlankClassRefParentExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefSelfExprSyntax makeBlankClassRefSelfExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefStaticExprSyntax makeBlankClassRefStaticExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static IntegerLiteralExprSyntax makeBlankIntegerLiteralExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static FloatLiteralExprSyntax makeBlankFloatLiteralExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static StringLiteralExprSyntax makeBlankStringLiteralExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static BooleanLiteralExprSyntax makeBlankBooleanLiteralExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static TernaryExprSyntax makeBlankTernaryExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static AssignmentExprSyntax makeBlankAssignmentExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static SequenceExprSyntax makeBlankSequenceExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static PrefixOperatorExprSyntax makeBlankPrefixOperatorExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static PostfixOperatorExprSyntax makeBlankPostfixOperatorExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static BinaryOperatorExprSyntax makeBlankBinaryOperatorExpr(RefCountPtr<SyntaxArena> arena = nullptr);
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_FACTORY_EXPR_SYNTAX_NODE_FACTORY_H
