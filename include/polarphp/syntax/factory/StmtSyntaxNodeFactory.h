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

#ifndef POLARPHP_SYNTAX_FACTORY_STMT_SYNTAX_NODE_FACTORY_H
#define POLARPHP_SYNTAX_FACTORY_STMT_SYNTAX_NODE_FACTORY_H

#include "polarphp/syntax/AbstractFactory.h"
#include "polarphp/syntax/syntaxnode/StmtSyntaxNodes.h"

namespace polar::syntax {

class StmtSyntaxNodeFactory final : public AbstractFactory
{
public:
   ///
   /// make collection nodes
   ///
   static Syntax makeBlankCollectionSyntax(SyntaxKind kind);

   static ConditionElementListSyntax makeConditionElementListSyntax(const std::vector<ConditionElementSyntax> &elements,
                                                                    RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchCaseListSyntax makeSwitchCaseListSyntax(const std::vector<SwitchCaseSyntax> &elements,
                                                        RefCountPtr<SyntaxArena> arena = nullptr);
   static ElseIfListSyntax makeElseIfListSyntax(const std::vector<ElseIfClauseSyntax> &elements,
                                                RefCountPtr<SyntaxArena> arena = nullptr);
   ///
   /// make normal nodes
   ///
   static ConditionElementSyntax makeConditionElementSyntax(Syntax condition, std::optional<TokenSyntax> trailingComma,
                                                            RefCountPtr<SyntaxArena> arena = nullptr);
   static ContinueStmtSyntax makeContinueStmtSyntax(TokenSyntax continueKeyword, std::optional<TokenSyntax> numberToken,
                                                    RefCountPtr<SyntaxArena> arena = nullptr);
   static BreakStmtSyntax makeBreakStmtSyntax(TokenSyntax breakKeyword, std::optional<TokenSyntax> numberToken,
                                              RefCountPtr<SyntaxArena> arena = nullptr);
   static FallthroughStmtSyntax makeFallthroughStmtSyntax(TokenSyntax fallthroughKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
//   static ElseIfClauseSyntax makeElseIfClauseSyntax(TokenSyntax elseIfKeyword, TokenSyntax leftParen,
//                                                    ExprSyntax condition, TokenSyntax rightParen,
//                                                    CodeBlockSyntax body, RefCountPtr<SyntaxArena> arena = nullptr);
//   static IfStmtSyntax makeIfStmtSyntax(std::optional<TokenSyntax> labelName, std::optional<TokenSyntax> labelColon,
//                                        TokenSyntax ifKeyword, TokenSyntax leftParen, ExprSyntax condition,
//                                        TokenSyntax rightParen, CodeBlockSyntax body, std::optional<ElseIfListSyntax> elseIfClauses,
//                                        std::optional<TokenSyntax> elseKeyword, std::optional<Syntax> elseBody,
//                                        RefCountPtr<SyntaxArena> arena = nullptr);
//   static WhileStmtSyntax makeWhileStmtSyntax(std::optional<TokenSyntax> labelName, std::optional<TokenSyntax> labelColon,
//                                              TokenSyntax whileKeyword, ConditionElementListSyntax conditions,
//                                              CodeBlockSyntax body, RefCountPtr<SyntaxArena> arena = nullptr);
//   static DoWhileStmtSyntax makeDoWhileStmtSyntax(std::optional<TokenSyntax> labelName, std::optional<TokenSyntax> labelColon,
//                                                  TokenSyntax doKeyword, CodeBlockSyntax body, TokenSyntax whileKeyword,
//                                                  TokenSyntax leftParen, ExprSyntax condition, TokenSyntax rightParen,
//                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchDefaultLabelSyntax makeSwitchDefaultLabelSyntax(TokenSyntax defaultKeyword, TokenSyntax colon,
                                                                RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchCaseLabelSyntax makeSwitchCaseLabelSyntax(TokenSyntax caseKeyword, ExprSyntax expr, TokenSyntax colon,
                                                          RefCountPtr<SyntaxArena> arena = nullptr);
//   static SwitchCaseSyntax makeSwitchCaseSyntax(Syntax label, CodeBlockItemListSyntax statements, RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchStmtSyntax makeSwitchStmtSyntax(std::optional<TokenSyntax> labelName, std::optional<TokenSyntax> labelColon,
                                                TokenSyntax switchKeyword, TokenSyntax leftParen, ExprSyntax conditionExpr,
                                                TokenSyntax rightParen, TokenSyntax leftBrace, SwitchCaseListSyntax cases,
                                                TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena = nullptr);
//   static DeferStmtSyntax makeDeferStmtSyntax(TokenSyntax deferKeyword, CodeBlockSyntax body, RefCountPtr<SyntaxArena> arena = nullptr);
   static ExpressionStmtSyntax makeExpressionStmtSyntax(ExprSyntax expr, RefCountPtr<SyntaxArena> arena = nullptr);
   static ThrowStmtSyntax makeThrowStmtSyntax(TokenSyntax throwKeyword, ExprSyntax expr, RefCountPtr<SyntaxArena> arena = nullptr);

   ///
   /// make blank nodes
   ///
   static ConditionElementListSyntax makeBlankConditionElementListSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchCaseListSyntax makeBlankSwitchCaseListSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static ElseIfListSyntax makeBlankElseIfListSyntax(RefCountPtr<SyntaxArena> arena = nullptr);

   static ConditionElementSyntax makeBlankConditionElementSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static ContinueStmtSyntax makeBlankContinueStmtSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static BreakStmtSyntax makeBlankBreakStmtSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static FallthroughStmtSyntax makeBlankFallthroughStmtSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static ElseIfClauseSyntax makeBlankElseIfClauseSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static IfStmtSyntax makeBlankIfStmtSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static WhileStmtSyntax makeBlankWhileStmtSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static DoWhileStmtSyntax makeBlankDoWhileStmtSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchDefaultLabelSyntax makeBlankSwitchDefaultLabelSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchCaseLabelSyntax makeBlankSwitchCaseLabelSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchCaseSyntax makeBlankSwitchCaseSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchStmtSyntax makeBlankSwitchStmtSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static DeferStmtSyntax makeBlankDeferStmtSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static ExpressionStmtSyntax makeBlankExpressionStmtSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
   static ThrowStmtSyntax makeBlankThrowStmtSyntax(RefCountPtr<SyntaxArena> arena = nullptr);
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_FACTORY_STMT_SYNTAX_NODE_FACTORY_H
