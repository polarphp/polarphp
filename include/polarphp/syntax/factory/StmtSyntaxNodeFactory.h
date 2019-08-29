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
#include "polarphp/syntax/syntaxnode/StmtSyntaxNodesFwd.h"

namespace polar::syntax {

class StmtSyntaxNodeFactory final : public AbstractFactory
{
public:
   ///
   /// make collection nodes
   ///
   static Syntax makeBlankCollectionSyntax(SyntaxKind kind);

   static ConditionElementListSyntax makeConditionElementList(const std::vector<ConditionElementSyntax> &elements,
                                                              RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchCaseListSyntax makeSwitchCaseList(const std::vector<SwitchCaseSyntax> &elements,
                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static ElseIfListSyntax makeElseIfList(const std::vector<ElseIfClauseSyntax> &elements,
                                          RefCountPtr<SyntaxArena> arena = nullptr);
   static InnerStmtListSyntax makeInnerStmtList(
         const std::vector<InnerStmtSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static TopStmtListSyntax makeTopStmtList(
         const std::vector<TopStmtSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static CatchListSyntax makeCatchList(
         const std::vector<CatchListItemClauseSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static CatchArgTypeHintListSyntax makeCatchArgTypeHintList(
         const std::vector<CatchArgTypeHintItemSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static UnsetVariableListSyntax makeUnsetVariableList(
         const std::vector<UnsetVariableSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static GlobalVariableListSyntax makeGlobalVariableList(
         const std::vector<GlobalVariableListItemSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static StaticVariableListSyntax makeStaticVariableList(
         const std::vector<StaticVariableListItemSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceUseDeclarationListSyntax makeNamespaceUseDeclarationList(
         const std::vector<NamespaceUseDeclarationSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceInlineUseDeclarationListSyntax makeNamespaceInlineUseDeclarationList(
         const std::vector<NamespaceInlineUseDeclarationSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceUnprefixedUseDeclarationListSyntax makeNamespaceUnprefixedUseDeclarationList(
         const std::vector<NamespaceUnprefixedUseDeclarationSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static ConstDeclareItemListSyntax makeConstDeclareItemList(
         const std::vector<ConstDeclareItemSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);

   ///
   /// make normal nodes
   ///
   static ConditionElementSyntax makeConditionElement(Syntax condition, std::optional<TokenSyntax> trailingComma,
                                                      RefCountPtr<SyntaxArena> arena = nullptr);
   static ContinueStmtSyntax makeContinueStmt(TokenSyntax continueKeyword, std::optional<TokenSyntax> numberToken,
                                              RefCountPtr<SyntaxArena> arena = nullptr);
   static BreakStmtSyntax makeBreakStmt(TokenSyntax breakKeyword, std::optional<TokenSyntax> numberToken,
                                        RefCountPtr<SyntaxArena> arena = nullptr);
   static FallthroughStmtSyntax makeFallthroughStmt(TokenSyntax fallthroughKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static ElseIfClauseSyntax makeElseIfClause(TokenSyntax elseIfKeyword, TokenSyntax leftParen,
                                              ExprSyntax condition, TokenSyntax rightParen,
                                              CodeBlockSyntax body, RefCountPtr<SyntaxArena> arena = nullptr);
   static IfStmtSyntax makeIfStmt(std::optional<TokenSyntax> labelName, std::optional<TokenSyntax> labelColon,
                                  TokenSyntax ifKeyword, TokenSyntax leftParen, ExprSyntax condition,
                                  TokenSyntax rightParen, CodeBlockSyntax body, std::optional<ElseIfListSyntax> elseIfClauses,
                                  std::optional<TokenSyntax> elseKeyword, std::optional<Syntax> elseBody,
                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static WhileStmtSyntax makeWhileStmt(std::optional<TokenSyntax> labelName, std::optional<TokenSyntax> labelColon,
                                        TokenSyntax whileKeyword, ConditionElementListSyntax conditions,
                                        CodeBlockSyntax body, RefCountPtr<SyntaxArena> arena = nullptr);
   static DoWhileStmtSyntax makeDoWhileStmt(std::optional<TokenSyntax> labelName, std::optional<TokenSyntax> labelColon,
                                            TokenSyntax doKeyword, CodeBlockSyntax body, TokenSyntax whileKeyword,
                                            TokenSyntax leftParen, ExprSyntax condition, TokenSyntax rightParen,
                                            RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchDefaultLabelSyntax makeSwitchDefaultLabel(TokenSyntax defaultKeyword, TokenSyntax colon,
                                                          RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchCaseLabelSyntax makeSwitchCaseLabel(TokenSyntax caseKeyword, ExprSyntax expr, TokenSyntax colon,
                                                    RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchCaseSyntax makeSwitchCase(Syntax label, CodeBlockItemListSyntax statements, RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchStmtSyntax makeSwitchStmt(std::optional<TokenSyntax> labelName, std::optional<TokenSyntax> labelColon,
                                          TokenSyntax switchKeyword, TokenSyntax leftParen, ExprSyntax conditionExpr,
                                          TokenSyntax rightParen, TokenSyntax leftBrace, SwitchCaseListSyntax cases,
                                          TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena = nullptr);
   static DeferStmtSyntax makeDeferStmt(TokenSyntax deferKeyword, CodeBlockSyntax body, RefCountPtr<SyntaxArena> arena = nullptr);
   static ThrowStmtSyntax makeThrowStmt(TokenSyntax throwKeyword, ExprSyntax expr, RefCountPtr<SyntaxArena> arena = nullptr);

   ///
   /// make blank nodes
   ///
   static ConditionElementListSyntax makeBlankConditionElementList(RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchCaseListSyntax makeBlankSwitchCaseList(RefCountPtr<SyntaxArena> arena = nullptr);
   static ElseIfListSyntax makeBlankElseIfList(RefCountPtr<SyntaxArena> arena = nullptr);

   static ConditionElementSyntax makeBlankConditionElement(RefCountPtr<SyntaxArena> arena = nullptr);
   static ContinueStmtSyntax makeBlankContinueStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static BreakStmtSyntax makeBlankBreakStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static FallthroughStmtSyntax makeBlankFallthroughStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static ElseIfClauseSyntax makeBlankElseIfClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static IfStmtSyntax makeBlankIfStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static WhileStmtSyntax makeBlankWhileStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static DoWhileStmtSyntax makeBlankDoWhileStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchDefaultLabelSyntax makeBlankSwitchDefaultLabel(RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchCaseLabelSyntax makeBlankSwitchCaseLabel(RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchCaseSyntax makeBlankSwitchCase(RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchStmtSyntax makeBlankSwitchStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static DeferStmtSyntax makeBlankDeferStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static ThrowStmtSyntax makeBlankThrowStmt(RefCountPtr<SyntaxArena> arena = nullptr);
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_FACTORY_STMT_SYNTAX_NODE_FACTORY_H
