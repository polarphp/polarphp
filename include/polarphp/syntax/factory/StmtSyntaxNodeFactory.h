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
   static EmptyStmtSyntax makeEmptyStmt(TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena = nullptr);
   static NestStmtSyntax makeNestStmt(TokenSyntax leftBrace, InnerStmtListSyntax statements,
                                      TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena = nullptr);
   static ExprStmtSyntax makeExprStmt(ExprSyntax expr, TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena = nullptr);
   static InnerStmtSyntax makeInnerStmt(StmtSyntax stmt, RefCountPtr<SyntaxArena> arena = nullptr);
   static InnerCodeBlockStmtSyntax makeInnerCodeBlockStmt(TokenSyntax leftBrace, InnerStmtListSyntax statements,
                                                          TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena = nullptr);
   static TopStmtSyntax makeTopStmt(StmtSyntax stmt, RefCountPtr<SyntaxArena> arena = nullptr);
   static TopCodeBlockStmtSyntax makeTopCodeBlockStmt(TokenSyntax leftBrace, TopStmtListSyntax statements,
                                                      TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena = nullptr);
   static DeclareStmtSyntax makeDeclareStmt(TokenSyntax declareToken, TokenSyntax leftParen,
                                            ConstDeclareItemListSyntax constList, TokenSyntax rightParen,
                                            StmtSyntax stmt, RefCountPtr<SyntaxArena> arena = nullptr);
   static GotoStmtSyntax makeGotoStmt(TokenSyntax gotoTokens, TokenSyntax target,
                                      TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena = nullptr);
   static UnsetVariableSyntax makeUnsetVariable(TokenSyntax variable, std::optional<TokenSyntax> trailingComma,
                                                RefCountPtr<SyntaxArena> arena = nullptr);
   static UnsetStmtSyntax makeUnsetStmt(TokenSyntax unsetToken, TokenSyntax leftParen,
                                        UnsetVariableListSyntax unsetVariables, TokenSyntax rightParen,
                                        TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena = nullptr);
   static LabelStmtSyntax makeLabelStmt(TokenSyntax name, TokenSyntax colon, RefCountPtr<SyntaxArena> arena = nullptr);
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
   static ForStmtSyntax makeForStmt(TokenSyntax forToken, TokenSyntax leftParen,
                                    std::optional<ExprListSyntax> initializedExprs, TokenSyntax initializedSemicolon,
                                    std::optional<ExprListSyntax> conditionalExprs, TokenSyntax conditionalSemicolon,
                                    std::optional<ExprListSyntax> operationalExprs, TokenSyntax operationalSemicolon,
                                    TokenSyntax rightParen, StmtSyntax stmt,
                                    RefCountPtr<SyntaxArena> arena = nullptr);
   static ForeachVariableSyntax makeForeachVariable(ExprSyntax variable, RefCountPtr<SyntaxArena> arena = nullptr);
   static ForeachStmtSyntax makeForeachStmt(TokenSyntax foreachToken, TokenSyntax leftParen,
                                            ExprSyntax iterableExpr, TokenSyntax asToken,
                                            std::optional<ForeachVariableSyntax> keyVariable, std::optional<TokenSyntax> doubleArrowToken,
                                            ForeachVariableSyntax valueVariable, TokenSyntax rightParen,
                                            StmtSyntax stmt, RefCountPtr<SyntaxArena> arena = nullptr);

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
   static TryStmtSyntax makeTryStmt(TokenSyntax tryToken, InnerCodeBlockStmtSyntax codeBlock,
                                    std::optional<CatchListSyntax> catchList, std::optional<FinallyClauseSyntax> finallyClause,
                                    RefCountPtr<SyntaxArena> arena = nullptr);
   static FinallyClauseSyntax makeFinallyClause(TokenSyntax finallyToken, InnerCodeBlockStmtSyntax codeBlock,
                                                RefCountPtr<SyntaxArena> arena = nullptr);
   static CatchArgTypeHintItemSyntax makeCatchArgTypeHintItem(NameSyntax typeName, std::optional<TokenSyntax> separator,
                                                              RefCountPtr<SyntaxArena> arena = nullptr);
   static CatchListItemClauseSyntax makeCatchListItemClause(TokenSyntax catchToken, TokenSyntax leftParen,
                                                            std::optional<InnerCodeBlockStmtSyntax> catchArgTypeHintList, TokenSyntax variable,
                                                            TokenSyntax rightParen, InnerCodeBlockStmtSyntax codeBlock,
                                                            RefCountPtr<SyntaxArena> arena = nullptr);
   static ReturnStmtSyntax makeReturnStmt(TokenSyntax returnKeyword, std::optional<ExprSyntax> expr,
                                          TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena = nullptr);
   static EchoStmtSyntax makeEchoStmt(TokenSyntax echoToken, ExprListSyntax exprListClause,
                                      TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena = nullptr);
   static HaltCompilerStmtSyntax makeHaltCompilerStmt(TokenSyntax haltCompilerToken, TokenSyntax leftParen,
                                                      TokenSyntax rightParen, TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena = nullptr);
   static GlobalVariableListItemSyntax makeGlobalVariableListItem(SimpleVariableExprSyntax variable, std::optional<TokenSyntax> trailingComma,
                                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static GlobalVariableDeclarationsStmtSyntax makeGlobalVariableDeclarationsStmt(TokenSyntax globalToken, GlobalVariableListSyntax variables,
                                                                                  TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena = nullptr);
   static StaticVariableListItemSyntax makeStaticVariableListItem(TokenSyntax variable, TokenSyntax equalToken,
                                                                  ExprSyntax valueExpr, RefCountPtr<SyntaxArena> arena = nullptr);
   static StaticVariableDeclarationsStmtSyntax makeStaticVariableDeclarationsStmt(TokenSyntax staticToken, StaticVariableListSyntax variables,
                                                                                  TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceUseTypeSyntax makeNamespaceUseType(TokenSyntax typeToken, RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceUnprefixedUseDeclarationSyntax makeNamespaceUnprefixedUseDeclaration(
         NamespacePartListSyntax ns, std::optional<TokenSyntax> asToken,
         std::optional<TokenSyntax> identifierToken, RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceUseDeclarationSyntax makeNamespaceUseDeclaration(
         std::optional<TokenSyntax> nsSeparator, NamespaceUnprefixedUseDeclarationSyntax unprefixedUseDeclaration,
         RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceInlineUseDeclarationSyntax makeNamespaceInlineUseDeclaration(
         std::optional<NamespaceUseTypeSyntax> useType, NamespaceUnprefixedUseDeclarationSyntax unprefixedUseDeclaration,
         RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceGroupUseDeclarationSyntax makeNamespaceGroupUseDeclaration(
         std::optional<TokenSyntax> firstNsSeparator, NamespacePartListSyntax ns,
         TokenSyntax secondNsSeparator, TokenSyntax leftBrace,
         NamespaceUnprefixedUseDeclarationListSyntax unprefixedUseDeclarations, std::optional<TokenSyntax> commaToken,
         TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceMixedGroupUseDeclarationSyntax makeNamespaceMixedGroupUseDeclaration(
         std::optional<TokenSyntax> firstNsSeparator, NamespacePartListSyntax ns,
         TokenSyntax secondNsSeparator, TokenSyntax leftBrace,
         NamespaceInlineUseDeclarationListSyntax inlineUseDeclarations, std::optional<TokenSyntax> commaToken,
         TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceUseStmtSyntax makeNamespaceUseStmt(TokenSyntax useToken, std::optional<NamespaceUseTypeSyntax> useType,
                                                      Syntax declarations, TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceDefinitionStmtSyntax makeNamespaceDefinitionStmt(
         TokenSyntax nsToken, NamespacePartListSyntax ns,
         TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceBlockStmtSyntax makeNamespaceBlockStmt(TokenSyntax nsToken, std::optional<NamespacePartListSyntax> ns,
                                                          TopCodeBlockStmtSyntax codeBlock, RefCountPtr<SyntaxArena> arena = nullptr);
   static ConstDeclareItemSyntax makeConstDeclareItem(TokenSyntax name, InitializerClauseSyntax initializerClause,
                                                      RefCountPtr<SyntaxArena> arena = nullptr);
   static ConstDefinitionStmtSyntax makeConstDefinitionStmt(TokenSyntax constToken, ConstDeclareItemListSyntax declarations,
                                                            TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassDefinitionStmtSyntax makeClassDefinitionStmt(ClassDefinitionSyntax classDefinition, RefCountPtr<SyntaxArena> arena = nullptr);
   static InterfaceDefinitionStmtSyntax makeInterfaceDefinitionStmt(InterfaceDefinitionSyntax interfaceDefinition,
                                                                    RefCountPtr<SyntaxArena> arena = nullptr);
   static TraitDefinitionStmtSyntax makeTraitDefinitionStmt(TraitDefinitionSyntax traitDefinition, RefCountPtr<SyntaxArena> arena = nullptr);
   static FunctionDefinitionStmtSyntax makeFunctionDefinitionStmt(FunctionDefinitionSyntax funcDefinition, RefCountPtr<SyntaxArena> arena = nullptr);

   ///
   /// make blank nodes
   ///
   static ConditionElementListSyntax makeBlankConditionElementList(RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchCaseListSyntax makeBlankSwitchCaseList(RefCountPtr<SyntaxArena> arena = nullptr);
   static ElseIfListSyntax makeBlankElseIfList(RefCountPtr<SyntaxArena> arena = nullptr);
   static InnerStmtListSyntax makeBlankInnerStmtList(RefCountPtr<SyntaxArena> arena = nullptr);
   static TopStmtListSyntax makeBlankTopStmtList(RefCountPtr<SyntaxArena> arena = nullptr);
   static CatchListSyntax makeBlankCatchList(RefCountPtr<SyntaxArena> arena = nullptr);
   static CatchArgTypeHintListSyntax makeBlankCatchArgTypeHintList(RefCountPtr<SyntaxArena> arena = nullptr);
   static UnsetVariableListSyntax makeBlankUnsetVariableList(RefCountPtr<SyntaxArena> arena = nullptr);
   static GlobalVariableListSyntax makeBlankGlobalVariableList(RefCountPtr<SyntaxArena> arena = nullptr);
   static StaticVariableListSyntax makeBlankStaticVariableList(RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceUseDeclarationListSyntax makeBlankNamespaceUseDeclarationList(RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceInlineUseDeclarationListSyntax makeBlankNamespaceInlineUseDeclarationList(RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceUnprefixedUseDeclarationListSyntax makeBlankNamespaceUnprefixedUseDeclarationList(RefCountPtr<SyntaxArena> arena = nullptr);
   static ConstDeclareItemListSyntax makeBlankConstDeclareItemList(RefCountPtr<SyntaxArena> arena = nullptr);

   static EmptyStmtSyntax makeBlankEmptyStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static NestStmtSyntax makeBlankNestStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static ExprStmtSyntax makeBlankExprStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static InnerStmtSyntax makeBlankInnerStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static InnerCodeBlockStmtSyntax makeBlankInnerCodeBlockStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static TopStmtSyntax makeBlankTopStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static TopCodeBlockStmtSyntax makeBlankTopCodeBlockStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static DeclareStmtSyntax makeBlankDeclareStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static GotoStmtSyntax makeBlankGotoStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static UnsetVariableSyntax makeBlankUnsetVariable(RefCountPtr<SyntaxArena> arena = nullptr);
   static UnsetStmtSyntax makeBlankUnsetStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static LabelStmtSyntax makeBlankLabelStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static ConditionElementSyntax makeBlankConditionElement(RefCountPtr<SyntaxArena> arena = nullptr);
   static ContinueStmtSyntax makeBlankContinueStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static BreakStmtSyntax makeBlankBreakStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static FallthroughStmtSyntax makeBlankFallthroughStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static ElseIfClauseSyntax makeBlankElseIfClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static IfStmtSyntax makeBlankIfStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static WhileStmtSyntax makeBlankWhileStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static DoWhileStmtSyntax makeBlankDoWhileStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static ForStmtSyntax makeBlankForStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static ForeachVariableSyntax makeBlankForeachVariable(RefCountPtr<SyntaxArena> arena = nullptr);
   static ForeachStmtSyntax makeBlankForeachStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchDefaultLabelSyntax makeBlankSwitchDefaultLabel(RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchCaseLabelSyntax makeBlankSwitchCaseLabel(RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchCaseSyntax makeBlankSwitchCase(RefCountPtr<SyntaxArena> arena = nullptr);
   static SwitchStmtSyntax makeBlankSwitchStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static DeferStmtSyntax makeBlankDeferStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static ThrowStmtSyntax makeBlankThrowStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static TryStmtSyntax makeBlankTryStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static FinallyClauseSyntax makeBlankFinallyClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static CatchArgTypeHintItemSyntax makeBlankCatchArgTypeHintItem(RefCountPtr<SyntaxArena> arena = nullptr);
   static CatchListItemClauseSyntax makeBlankCatchListItemClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static ReturnStmtSyntax makeBlankReturnStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static EchoStmtSyntax makeBlankEchoStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static HaltCompilerStmtSyntax makeBlankHaltCompilerStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static GlobalVariableListItemSyntax makeBlankGlobalVariableListItem(RefCountPtr<SyntaxArena> arena = nullptr);
   static GlobalVariableDeclarationsStmtSyntax makeBlankGlobalVariableDeclarationsStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static StaticVariableListItemSyntax makeBlankStaticVariableListItem(RefCountPtr<SyntaxArena> arena = nullptr);
   static StaticVariableDeclarationsStmtSyntax makeBlankStaticVariableDeclarationsStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceUseTypeSyntax makeBlankNamespaceUseType(RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceUnprefixedUseDeclarationSyntax makeBlankNamespaceUnprefixedUseDeclaration(RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceUseDeclarationSyntax makeBlankNamespaceUseDeclaration(RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceInlineUseDeclarationSyntax makeBlankNamespaceInlineUseDeclaration(RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceGroupUseDeclarationSyntax makeBlankNamespaceGroupUseDeclaration(RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceMixedGroupUseDeclarationSyntax makeBlankNamespaceMixedGroupUseDeclaration(RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceUseStmtSyntax makeBlankNamespaceUseStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceDefinitionStmtSyntax makeBlankNamespaceDefinitionStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceBlockStmtSyntax makeBlankNamespaceBlockStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static ConstDeclareItemSyntax makeBlankConstDeclareItem(RefCountPtr<SyntaxArena> arena = nullptr);
   static ConstDefinitionStmtSyntax makeBlankConstDefinitionStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassDefinitionStmtSyntax makeBlankClassDefinitionStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static InterfaceDefinitionStmtSyntax makeBlankInterfaceDefinitionStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static TraitDefinitionStmtSyntax makeBlankTraitDefinitionStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static FunctionDefinitionStmtSyntax makeBlankFunctionDefinitionStmt(RefCountPtr<SyntaxArena> arena = nullptr);
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_FACTORY_STMT_SYNTAX_NODE_FACTORY_H
