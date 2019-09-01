// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/15.

#include "polarphp/syntax/factory/StmtSyntaxNodeFactory.h"
#include "polarphp/syntax/syntaxnode/StmtSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/DeclSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/ExprSyntaxNodes.h"

namespace polar::syntax {

///
/// make collection nodes
///
Syntax StmtSyntaxNodeFactory::makeBlankCollectionSyntax(SyntaxKind kind)
{

}

ConditionElementListSyntax
StmtSyntaxNodeFactory::makeConditionElementList(const std::vector<ConditionElementSyntax> &elements,
                                                RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const ConditionElementSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ConditionElementList, layout, SourcePresence::Present,
            arena);
   return make<ConditionElementListSyntax>(target);
}

SwitchCaseListSyntax
StmtSyntaxNodeFactory::makeSwitchCaseList(const std::vector<SwitchCaseSyntax> &elements,
                                          RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const SwitchCaseSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SwitchCaseList, layout, SourcePresence::Present,
            arena);
   return make<SwitchCaseListSyntax>(target);
}

ElseIfListSyntax
StmtSyntaxNodeFactory::makeElseIfList(const std::vector<ElseIfClauseSyntax> &elements,
                                      RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const ElseIfClauseSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ElseIfList, layout, SourcePresence::Present,
            arena);
   return make<ElseIfListSyntax>(target);
}

InnerStmtListSyntax
StmtSyntaxNodeFactory::makeInnerStmtList(
      const std::vector<InnerStmtSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const InnerStmtSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::InnerStmtList, layout, SourcePresence::Present,
            arena);
   return make<InnerStmtListSyntax>(target);
}

TopStmtListSyntax
StmtSyntaxNodeFactory::makeTopStmtList(
      const std::vector<TopStmtSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const TopStmtSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::TopStmtList, layout, SourcePresence::Present,
            arena);
   return make<TopStmtListSyntax>(target);
}

CatchListSyntax
StmtSyntaxNodeFactory::makeCatchList(
      const std::vector<CatchListItemClauseSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const CatchListItemClauseSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CatchList, layout, SourcePresence::Present,
            arena);
   return make<CatchListSyntax>(target);
}

CatchArgTypeHintListSyntax
StmtSyntaxNodeFactory::makeCatchArgTypeHintList(
      const std::vector<CatchArgTypeHintItemSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const CatchArgTypeHintItemSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CatchArgTypeHintList, layout, SourcePresence::Present,
            arena);
   return make<CatchArgTypeHintListSyntax>(target);
}

UnsetVariableListSyntax
StmtSyntaxNodeFactory::makeUnsetVariableList(
      const std::vector<UnsetVariableSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const UnsetVariableSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::UnsetVariableList, layout, SourcePresence::Present,
            arena);
   return make<UnsetVariableListSyntax>(target);
}

GlobalVariableListSyntax
StmtSyntaxNodeFactory::makeGlobalVariableList(
      const std::vector<GlobalVariableListItemSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const GlobalVariableListItemSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::GlobalVariableList, layout, SourcePresence::Present,
            arena);
   return make<GlobalVariableListSyntax>(target);
}

StaticVariableListSyntax
StmtSyntaxNodeFactory::makeStaticVariableList(
      const std::vector<StaticVariableListItemSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const StaticVariableListItemSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::StaticVariableList, layout, SourcePresence::Present,
            arena);
   return make<StaticVariableListSyntax>(target);
}

NamespaceUseDeclarationListSyntax
StmtSyntaxNodeFactory::makeNamespaceUseDeclarationList(
      const std::vector<NamespaceUseDeclarationSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const NamespaceUseDeclarationSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceUseDeclarationList, layout, SourcePresence::Present,
            arena);
   return make<NamespaceUseDeclarationListSyntax>(target);
}

NamespaceInlineUseDeclarationListSyntax
StmtSyntaxNodeFactory::makeNamespaceInlineUseDeclarationList(
      const std::vector<NamespaceInlineUseDeclarationSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const NamespaceInlineUseDeclarationSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceInlineUseDeclarationList, layout, SourcePresence::Present,
            arena);
   return make<NamespaceInlineUseDeclarationListSyntax>(target);
}

NamespaceUnprefixedUseDeclarationListSyntax
StmtSyntaxNodeFactory::makeNamespaceUnprefixedUseDeclarationList(
      const std::vector<NamespaceUnprefixedUseDeclarationSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const NamespaceUnprefixedUseDeclarationSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceUnprefixedUseDeclarationList, layout, SourcePresence::Present,
            arena);
   return make<NamespaceUnprefixedUseDeclarationListSyntax>(target);
}

ConstDeclareItemListSyntax
StmtSyntaxNodeFactory::makeConstDeclareItemList(
      const std::vector<ConstDeclareItemSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const ConstDeclareItemSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ConstDeclareItemList, layout, SourcePresence::Present,
            arena);
   return make<ConstDeclareItemListSyntax>(target);
}

///
/// make normal nodes
///

EmptyStmtSyntax
StmtSyntaxNodeFactory::makeEmptyStmt(TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EmptyStmt, {
               semicolon.getRaw(),
            }, SourcePresence::Present, arena);
   return make<EmptyStmtSyntax>(target);
}

NestStmtSyntax
StmtSyntaxNodeFactory::makeNestStmt(TokenSyntax leftBrace, InnerStmtListSyntax statements,
                                    TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NestStmt, {
               leftBrace.getRaw(),
               statements.getRaw(),
               rightBrace.getRaw(),
            }, SourcePresence::Present, arena);
   return make<NestStmtSyntax>(target);
}

ExprStmtSyntax
StmtSyntaxNodeFactory::makeExprStmt(ExprSyntax expr, TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ExprStmt, {
               expr.getRaw(),
               semicolon.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ExprStmtSyntax>(target);
}

InnerStmtSyntax
StmtSyntaxNodeFactory::makeInnerStmt(StmtSyntax stmt, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::InnerStmt, {
               stmt.getRaw(),
            }, SourcePresence::Present, arena);
   return make<InnerStmtSyntax>(target);
}

InnerCodeBlockStmtSyntax
StmtSyntaxNodeFactory::makeInnerCodeBlockStmt(TokenSyntax leftBrace, InnerStmtListSyntax statements,
                                              TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::InnerCodeBlockStmt, {
               leftBrace.getRaw(),
               statements.getRaw(),
               rightBrace.getRaw()
            }, SourcePresence::Present, arena);
   return make<InnerCodeBlockStmtSyntax>(target);
}

TopStmtSyntax
StmtSyntaxNodeFactory::makeTopStmt(StmtSyntax stmt, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::TopStmt, {
               stmt.getRaw(),
            }, SourcePresence::Present, arena);
   return make<TopStmtSyntax>(target);
}

TopCodeBlockStmtSyntax
StmtSyntaxNodeFactory::makeTopCodeBlockStmt(TokenSyntax leftBrace, TopStmtListSyntax statements,
                                            TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::TopCodeBlockStmt, {
               leftBrace.getRaw(),
               statements.getRaw(),
               rightBrace.getRaw()
            }, SourcePresence::Present, arena);
   return make<TopCodeBlockStmtSyntax>(target);
}

DeclareStmtSyntax
StmtSyntaxNodeFactory::makeDeclareStmt(TokenSyntax declareToken, TokenSyntax leftParen,
                                       ConstDeclareItemListSyntax constList, TokenSyntax rightParen,
                                       StmtSyntax stmt, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::DeclareStmt, {
               declareToken.getRaw(),
               leftParen.getRaw(),
               constList.getRaw(),
               rightParen.getRaw(),
               stmt.getRaw(),
            }, SourcePresence::Present, arena);
   return make<DeclareStmtSyntax>(target);
}

GotoStmtSyntax
StmtSyntaxNodeFactory::makeGotoStmt(TokenSyntax gotoTokens, TokenSyntax target,
                                    TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> targetSyntaxNode = RawSyntax::make(
            SyntaxKind::GotoStmt, {
               gotoTokens.getRaw(),
               target.getRaw(),
               semicolon.getRaw(),
            }, SourcePresence::Present, arena);
   return make<GotoStmtSyntax>(targetSyntaxNode);
}

UnsetVariableSyntax
StmtSyntaxNodeFactory::makeUnsetVariable(TokenSyntax variable, std::optional<TokenSyntax> trailingComma,
                                         RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> targetSyntaxNode = RawSyntax::make(
            SyntaxKind::UnsetVariable, {
               variable.getRaw(),
               trailingComma.has_value() ? trailingComma->getRaw() : nullptr,
            }, SourcePresence::Present, arena);
   return make<UnsetVariableSyntax>(targetSyntaxNode);
}

UnsetStmtSyntax
StmtSyntaxNodeFactory::makeUnsetStmt(TokenSyntax unsetToken, TokenSyntax leftParen,
                                     UnsetVariableListSyntax unsetVariables, TokenSyntax rightParen,
                                     TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> targetSyntaxNode = RawSyntax::make(
            SyntaxKind::UnsetStmt, {
               unsetToken.getRaw(),
               leftParen.getRaw(),
               unsetVariables.getRaw(),
               rightParen.getRaw(),
               semicolon.getRaw(),
            }, SourcePresence::Present, arena);
   return make<UnsetStmtSyntax>(targetSyntaxNode);
}

LabelStmtSyntax
StmtSyntaxNodeFactory::makeLabelStmt(TokenSyntax name, TokenSyntax colon, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> targetSyntaxNode = RawSyntax::make(
            SyntaxKind::LabelStmt, {
               name.getRaw(),
               colon.getRaw(),
            }, SourcePresence::Present, arena);
   return make<LabelStmtSyntax>(targetSyntaxNode);
}

ConditionElementSyntax
StmtSyntaxNodeFactory::makeConditionElement(Syntax condition, std::optional<TokenSyntax> trailingComma,
                                            RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ConditionElement, {
               condition.getRaw(),
               trailingComma.has_value() ? trailingComma->getRaw() : nullptr
            }, SourcePresence::Present, arena);
   return make<ConditionElementSyntax>(target);
}

ContinueStmtSyntax
StmtSyntaxNodeFactory::makeContinueStmt(TokenSyntax continueKeyword, std::optional<TokenSyntax> numberToken,
                                        RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ContinueStmt, {
               continueKeyword.getRaw(),
               numberToken.has_value() ? numberToken->getRaw() : nullptr
            }, SourcePresence::Present, arena);
   return make<ContinueStmtSyntax>(target);
}

BreakStmtSyntax
StmtSyntaxNodeFactory::makeBreakStmt(TokenSyntax breakKeyword, std::optional<TokenSyntax> numberToken,
                                     RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BreakStmt, {
               breakKeyword.getRaw(),
               numberToken.has_value() ? numberToken->getRaw() : nullptr
            }, SourcePresence::Present, arena);
   return make<BreakStmtSyntax>(target);
}

FallthroughStmtSyntax
StmtSyntaxNodeFactory::makeFallthroughStmt(TokenSyntax fallthroughKeyword, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::FallthroughStmt, {
               fallthroughKeyword.getRaw()
            }, SourcePresence::Present, arena);
   return make<FallthroughStmtSyntax>(target);
}

ElseIfClauseSyntax
StmtSyntaxNodeFactory::makeElseIfClause(TokenSyntax elseIfKeyword, TokenSyntax leftParen,
                                        ExprSyntax condition, TokenSyntax rightParen,
                                        CodeBlockSyntax body, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ElseIfClause, {
               elseIfKeyword.getRaw(),
               leftParen.getRaw(),
               condition.getRaw(),
               rightParen.getRaw(),
               body.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ElseIfClauseSyntax>(target);
}

IfStmtSyntax
StmtSyntaxNodeFactory::makeIfStmt(std::optional<TokenSyntax> labelName, std::optional<TokenSyntax> labelColon,
                                  TokenSyntax ifKeyword, TokenSyntax leftParen, ExprSyntax condition,
                                  TokenSyntax rightParen, CodeBlockSyntax body, std::optional<ElseIfListSyntax> elseIfClauses,
                                  std::optional<TokenSyntax> elseKeyword, std::optional<Syntax> elseBody,
                                  RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::IfStmt, {
               labelName.has_value() ? labelName->getRaw() : nullptr,
               labelColon.has_value() ? labelColon->getRaw() : nullptr,
               ifKeyword.getRaw(),
               leftParen.getRaw(),
               condition.getRaw(),
               rightParen.getRaw(),
               body.getRaw(),
               elseIfClauses.has_value() ? elseIfClauses->getRaw() : nullptr,
               elseKeyword.has_value() ? elseKeyword->getRaw() : nullptr,
               elseBody.has_value() ? elseBody->getRaw() : nullptr,
            }, SourcePresence::Present, arena);
   return make<IfStmtSyntax>(target);
}

WhileStmtSyntax
StmtSyntaxNodeFactory::makeWhileStmt(std::optional<TokenSyntax> labelName, std::optional<TokenSyntax> labelColon,
                                     TokenSyntax whileKeyword, ConditionElementListSyntax conditions,
                                     CodeBlockSyntax body, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::WhileStmt, {
               labelName.has_value() ? labelName->getRaw() : nullptr,
               labelColon.has_value() ? labelColon->getRaw() : nullptr,
               whileKeyword.getRaw(),
               conditions.getRaw(),
               body.getRaw()
            }, SourcePresence::Present, arena);
   return make<WhileStmtSyntax>(target);
}

DoWhileStmtSyntax
StmtSyntaxNodeFactory::makeDoWhileStmt(std::optional<TokenSyntax> labelName, std::optional<TokenSyntax> labelColon,
                                       TokenSyntax doKeyword, CodeBlockSyntax body, TokenSyntax whileKeyword,
                                       TokenSyntax leftParen, ExprSyntax condition, TokenSyntax rightParen,
                                       RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::DoWhileStmt, {
               labelName.has_value() ? labelName->getRaw() : nullptr,
               labelColon.has_value() ? labelColon->getRaw() : nullptr,
               doKeyword.getRaw(),
               body.getRaw(),
               whileKeyword.getRaw(),
               leftParen.getRaw(),
               condition.getRaw(),
               rightParen.getRaw(),
            }, SourcePresence::Present, arena);
   return make<DoWhileStmtSyntax>(target);
}

ForStmtSyntax
StmtSyntaxNodeFactory::makeForStmt(TokenSyntax forToken, TokenSyntax leftParen,
                                   std::optional<ExprListSyntax> initializedExprs, TokenSyntax initializedSemicolon,
                                   std::optional<ExprListSyntax> conditionalExprs, TokenSyntax conditionalSemicolon,
                                   std::optional<ExprListSyntax> operationalExprs, TokenSyntax operationalSemicolon,
                                   TokenSyntax rightParen, StmtSyntax stmt,
                                   RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ForStmt, {
               forToken.getRaw(),
               leftParen.getRaw(),
               initializedExprs.has_value() ? initializedExprs->getRaw() : nullptr,
               initializedSemicolon.getRaw(),
               conditionalExprs.has_value() ? conditionalExprs->getRaw() : nullptr,
               conditionalSemicolon.getRaw(),
               operationalExprs.has_value() ? operationalExprs->getRaw() : nullptr,
               operationalSemicolon.getRaw(),
               rightParen.getRaw(),
               stmt.getRaw()
            }, SourcePresence::Present, arena);
   return make<ForStmtSyntax>(target);
}

ForeachVariableSyntax
StmtSyntaxNodeFactory::makeForeachVariable(ExprSyntax variable, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ForeachVariable, {
               variable.getRaw()
            }, SourcePresence::Present, arena);
   return make<ForeachVariableSyntax>(target);
}

ForeachStmtSyntax
StmtSyntaxNodeFactory::makeForeachStmt(TokenSyntax foreachToken, TokenSyntax leftParen,
                                       ExprSyntax iterableExpr, TokenSyntax asToken,
                                       std::optional<ForeachVariableSyntax> keyVariable, std::optional<TokenSyntax> doubleArrowToken,
                                       ForeachVariableSyntax valueVariable, TokenSyntax rightParen,
                                       StmtSyntax stmt, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ForeachStmt, {
               foreachToken.getRaw(),
               leftParen.getRaw(),
               iterableExpr.getRaw(),
               asToken.getRaw(),
               keyVariable.has_value() ? keyVariable->getRaw() : nullptr,
               doubleArrowToken.has_value() ? doubleArrowToken->getRaw() : nullptr,
               valueVariable.getRaw(),
               rightParen.getRaw(),
               stmt.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ForeachStmtSyntax>(target);
}

SwitchDefaultLabelSyntax
StmtSyntaxNodeFactory::makeSwitchDefaultLabel(TokenSyntax defaultKeyword, TokenSyntax colon,
                                              RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SwitchDefaultLabel, {
               defaultKeyword.getRaw(),
               colon.getRaw()
            }, SourcePresence::Present, arena);
   return make<SwitchDefaultLabelSyntax>(target);
}

SwitchCaseLabelSyntax
StmtSyntaxNodeFactory::makeSwitchCaseLabel(TokenSyntax caseKeyword, ExprSyntax expr, TokenSyntax colon,
                                           RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SwitchCaseLabel, {
               caseKeyword.getRaw(),
               expr.getRaw(),
               colon.getRaw()
            }, SourcePresence::Present, arena);
   return make<SwitchCaseLabelSyntax>(target);
}

SwitchCaseSyntax
StmtSyntaxNodeFactory::makeSwitchCase(Syntax label, CodeBlockItemListSyntax statements,
                                      RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SwitchCase, {
               label.getRaw(),
               statements.getRaw()
            }, SourcePresence::Present, arena);
   return make<SwitchCaseSyntax>(target);
}

SwitchStmtSyntax
StmtSyntaxNodeFactory::makeSwitchStmt(std::optional<TokenSyntax> labelName, std::optional<TokenSyntax> labelColon,
                                      TokenSyntax switchKeyword, TokenSyntax leftParen, ExprSyntax conditionExpr,
                                      TokenSyntax rightParen, TokenSyntax leftBrace, SwitchCaseListSyntax cases,
                                      TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SwitchStmt, {
               labelName.has_value() ? labelName->getRaw() : nullptr,
               labelColon.has_value() ? labelColon->getRaw() : nullptr,
               switchKeyword.getRaw(),
               leftParen.getRaw(),
               conditionExpr.getRaw(),
               rightParen.getRaw(),
               leftBrace.getRaw(),
               cases.getRaw(),
               rightBrace.getRaw()
            }, SourcePresence::Present, arena);
   return make<SwitchStmtSyntax>(target);
}

DeferStmtSyntax
StmtSyntaxNodeFactory::makeDeferStmt(TokenSyntax deferKeyword, CodeBlockSyntax body,
                                     RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::DeferStmt, {
               deferKeyword.getRaw(),
               body.getRaw()
            }, SourcePresence::Present, arena);
   return make<DeferStmtSyntax>(target);
}

ThrowStmtSyntax
StmtSyntaxNodeFactory::makeThrowStmt(TokenSyntax throwKeyword, ExprSyntax expr,
                                     RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ThrowStmt, {
               throwKeyword.getRaw(),
               expr.getRaw()
            }, SourcePresence::Present, arena);
   return make<ThrowStmtSyntax>(target);
}

TryStmtSyntax
StmtSyntaxNodeFactory::makeTryStmt(TokenSyntax tryToken, InnerCodeBlockStmtSyntax codeBlock,
                                   std::optional<CatchListSyntax> catchList, std::optional<FinallyClauseSyntax> finallyClause,
                                   RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::TryStmt, {
               tryToken.getRaw(),
               codeBlock.getRaw(),
               catchList.has_value() ? catchList->getRaw() : nullptr,
               finallyClause.has_value() ? finallyClause->getRaw() : nullptr,
            }, SourcePresence::Present, arena);
   return make<TryStmtSyntax>(target);
}

FinallyClauseSyntax
StmtSyntaxNodeFactory::makeFinallyClause(TokenSyntax finallyToken, InnerCodeBlockStmtSyntax codeBlock,
                                         RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::FinallyClause, {
               finallyToken.getRaw(),
               codeBlock.getRaw(),
            }, SourcePresence::Present, arena);
   return make<FinallyClauseSyntax>(target);
}

CatchArgTypeHintItemSyntax
StmtSyntaxNodeFactory::makeCatchArgTypeHintItem(NameSyntax typeName, std::optional<TokenSyntax> separator,
                                                RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CatchArgTypeHintItem, {
               typeName.getRaw(),
               separator.has_value() ? separator->getRaw() : nullptr,
            }, SourcePresence::Present, arena);
   return make<CatchArgTypeHintItemSyntax>(target);
}

CatchListItemClauseSyntax
StmtSyntaxNodeFactory::makeCatchListItemClause(TokenSyntax catchToken, TokenSyntax leftParen,
                                               std::optional<InnerCodeBlockStmtSyntax> catchArgTypeHintList, TokenSyntax variable,
                                               TokenSyntax rightParen, InnerCodeBlockStmtSyntax codeBlock,
                                               RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CatchListItemClause, {
               catchToken.getRaw(),
               leftParen.getRaw(),
               catchArgTypeHintList.has_value() ? catchArgTypeHintList->getRaw() : nullptr,
               variable.getRaw(),
               rightParen.getRaw(),
               codeBlock.getRaw(),
            }, SourcePresence::Present, arena);
   return make<CatchListItemClauseSyntax>(target);
}

ReturnStmtSyntax
StmtSyntaxNodeFactory::makeReturnStmt(TokenSyntax returnKeyword, std::optional<ExprSyntax> expr,
                                      TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ReturnStmt, {
               returnKeyword.getRaw(),
               expr.has_value() ? expr->getRaw() : nullptr,
               semicolon.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ReturnStmtSyntax>(target);
}

EchoStmtSyntax
StmtSyntaxNodeFactory::makeEchoStmt(TokenSyntax echoToken, ExprListSyntax exprListClause,
                                    TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EchoStmt, {
               echoToken.getRaw(),
               exprListClause.getRaw(),
               semicolon.getRaw(),
            }, SourcePresence::Present, arena);
   return make<EchoStmtSyntax>(target);
}

HaltCompilerStmtSyntax
StmtSyntaxNodeFactory::makeHaltCompilerStmt(TokenSyntax haltCompilerToken, TokenSyntax leftParen,
                                            TokenSyntax rightParen, TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::HaltCompilerStmt, {
               haltCompilerToken.getRaw(),
               leftParen.getRaw(),
               rightParen.getRaw(),
               semicolon.getRaw(),
            }, SourcePresence::Present, arena);
   return make<HaltCompilerStmtSyntax>(target);
}

GlobalVariableListItemSyntax
StmtSyntaxNodeFactory::makeGlobalVariableListItem(SimpleVariableExprSyntax variable, std::optional<TokenSyntax> trailingComma,
                                                  RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::GlobalVariableListItem, {
               variable.getRaw(),
               trailingComma.has_value() ? trailingComma->getRaw() : nullptr,
            }, SourcePresence::Present, arena);
   return make<GlobalVariableListItemSyntax>(target);
}

GlobalVariableDeclarationsStmtSyntax
StmtSyntaxNodeFactory::makeGlobalVariableDeclarationsStmt(TokenSyntax globalToken, GlobalVariableListSyntax variables,
                                                          TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::GlobalVariableDeclarationsStmt, {
               globalToken.getRaw(),
               variables.getRaw(),
               semicolon.getRaw(),
            }, SourcePresence::Present, arena);
   return make<GlobalVariableDeclarationsStmtSyntax>(target);
}

StaticVariableListItemSyntax
StmtSyntaxNodeFactory::makeStaticVariableListItem(TokenSyntax variable, TokenSyntax equalToken,
                                                  ExprSyntax valueExpr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::StaticVariableListItem, {
               variable.getRaw(),
               equalToken.getRaw(),
               valueExpr.getRaw(),
            }, SourcePresence::Present, arena);
   return make<StaticVariableListItemSyntax>(target);
}

StaticVariableDeclarationsStmtSyntax
StmtSyntaxNodeFactory::makeStaticVariableDeclarationsStmt(TokenSyntax staticToken, StaticVariableListSyntax variables,
                                                          TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::StaticVariableDeclarationsStmt, {
               staticToken.getRaw(),
               variables.getRaw(),
               semicolon.getRaw(),
            }, SourcePresence::Present, arena);
   return make<StaticVariableDeclarationsStmtSyntax>(target);
}

NamespaceUseTypeSyntax
StmtSyntaxNodeFactory::makeNamespaceUseType(TokenSyntax typeToken, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceUseType, {
               typeToken.getRaw(),
            }, SourcePresence::Present, arena);
   return make<NamespaceUseTypeSyntax>(target);
}

NamespaceUnprefixedUseDeclarationSyntax
StmtSyntaxNodeFactory::makeNamespaceUnprefixedUseDeclaration(
      NamespacePartListSyntax ns, std::optional<TokenSyntax> asToken,
      std::optional<TokenSyntax> identifierToken, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceUnprefixedUseDeclaration, {
               ns.getRaw(),
               asToken.has_value() ? asToken->getRaw() : nullptr,
               identifierToken.has_value() ? identifierToken->getRaw() : nullptr,
            }, SourcePresence::Present, arena);
   return make<NamespaceUnprefixedUseDeclarationSyntax>(target);
}

NamespaceUseDeclarationSyntax
StmtSyntaxNodeFactory::makeNamespaceUseDeclaration(
      std::optional<TokenSyntax> nsSeparator, NamespaceUnprefixedUseDeclarationSyntax unprefixedUseDeclaration,
      RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceUseDeclaration, {
               nsSeparator.has_value() ? nsSeparator->getRaw() : nullptr,
               unprefixedUseDeclaration.getRaw(),
            }, SourcePresence::Present, arena);
   return make<NamespaceUseDeclarationSyntax>(target);
}

NamespaceInlineUseDeclarationSyntax
StmtSyntaxNodeFactory::makeNamespaceInlineUseDeclaration(
      std::optional<NamespaceUseTypeSyntax> useType, NamespaceUnprefixedUseDeclarationSyntax unprefixedUseDeclaration,
      RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceInlineUseDeclaration, {
               useType.has_value() ? useType->getRaw() : nullptr,
               unprefixedUseDeclaration.getRaw(),
            }, SourcePresence::Present, arena);
   return make<NamespaceInlineUseDeclarationSyntax>(target);
}

NamespaceGroupUseDeclarationSyntax
StmtSyntaxNodeFactory::makeNamespaceGroupUseDeclaration(
      std::optional<TokenSyntax> firstNsSeparator, NamespacePartListSyntax ns,
      TokenSyntax secondNsSeparator, TokenSyntax leftBrace,
      NamespaceUnprefixedUseDeclarationListSyntax unprefixedUseDeclarations, std::optional<TokenSyntax> commaToken,
      TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceGroupUseDeclaration, {
               firstNsSeparator.has_value() ? firstNsSeparator->getRaw() : nullptr,
               ns.getRaw(),
               secondNsSeparator.getRaw(),
               leftBrace.getRaw(),
               unprefixedUseDeclarations.getRaw(),
               commaToken.has_value() ? commaToken->getRaw() : nullptr,
               rightBrace.getRaw(),
            }, SourcePresence::Present, arena);
   return make<NamespaceGroupUseDeclarationSyntax>(target);
}

NamespaceMixedGroupUseDeclarationSyntax
StmtSyntaxNodeFactory::makeNamespaceMixedGroupUseDeclaration(
      std::optional<TokenSyntax> firstNsSeparator, NamespacePartListSyntax ns,
      TokenSyntax secondNsSeparator, TokenSyntax leftBrace,
      NamespaceInlineUseDeclarationListSyntax inlineUseDeclarations, std::optional<TokenSyntax> commaToken,
      TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceMixedGroupUseDeclaration, {
               firstNsSeparator.has_value() ? firstNsSeparator->getRaw() : nullptr,
               ns.getRaw(),
               secondNsSeparator.getRaw(),
               leftBrace.getRaw(),
               inlineUseDeclarations.getRaw(),
               commaToken.has_value() ? commaToken->getRaw() : nullptr,
               rightBrace.getRaw(),
            }, SourcePresence::Present, arena);
   return make<NamespaceMixedGroupUseDeclarationSyntax>(target);
}

NamespaceUseStmtSyntax
StmtSyntaxNodeFactory::makeNamespaceUseStmt(TokenSyntax useToken, std::optional<NamespaceUseTypeSyntax> useType,
                                            Syntax declarations, TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceUseStmt, {
               useToken.getRaw(),
               useType.has_value() ? useType->getRaw() : nullptr,
               declarations.getRaw(),
               semicolon.getRaw(),
            }, SourcePresence::Present, arena);
   return make<NamespaceUseStmtSyntax>(target);
}

NamespaceDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeNamespaceDefinitionStmt(
      TokenSyntax nsToken, NamespacePartListSyntax ns,
      TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceDefinitionStmt, {
               nsToken.getRaw(),
               ns.getRaw(),
               semicolon.getRaw(),
            }, SourcePresence::Present, arena);
   return make<NamespaceDefinitionStmtSyntax>(target);
}

NamespaceBlockStmtSyntax
StmtSyntaxNodeFactory::makeNamespaceBlockStmt(TokenSyntax nsToken, std::optional<NamespacePartListSyntax> ns,
                                              TopCodeBlockStmtSyntax codeBlock, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceBlockStmt, {
               nsToken.getRaw(),
               ns.has_value() ? ns->getRaw() : nullptr,
               codeBlock.getRaw(),
            }, SourcePresence::Present, arena);
   return make<NamespaceBlockStmtSyntax>(target);
}

ConstDeclareItemSyntax
StmtSyntaxNodeFactory::makeConstDeclareItem(TokenSyntax name, InitializerClauseSyntax initializerClause,
                                            RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ConstDeclareItem, {
               name.getRaw(),
               initializerClause.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ConstDeclareItemSyntax>(target);
}

ConstDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeConstDefinitionStmt(TokenSyntax constToken, ConstDeclareItemListSyntax declarations,
                                               TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ConstDefinitionStmt, {
               constToken.getRaw(),
               declarations.getRaw(),
               semicolon.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ConstDefinitionStmtSyntax>(target);
}

ClassDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeClassDefinitionStmt(ClassDefinitionSyntax classDefinition, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ClassDefinitionStmt, {
               classDefinition.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ClassDefinitionStmtSyntax>(target);
}

InterfaceDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeInterfaceDefinitionStmt(InterfaceDefinitionSyntax interfaceDefinition,
                                                   RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::InterfaceDefinitionStmt, {
               interfaceDefinition.getRaw(),
            }, SourcePresence::Present, arena);
   return make<InterfaceDefinitionStmtSyntax>(target);
}

TraitDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeTraitDefinitionStmt(TraitDefinitionSyntax traitDefinition, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::TraitDefinitionStmt, {
               traitDefinition.getRaw(),
            }, SourcePresence::Present, arena);
   return make<TraitDefinitionStmtSyntax>(target);
}

FunctionDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeFunctionDefinitionStmt(FunctionDefinitionSyntax funcDefinition, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::FunctionDefinitionStmt, {
               funcDefinition.getRaw(),
            }, SourcePresence::Present, arena);
   return make<FunctionDefinitionStmtSyntax>(target);
}

///
/// StmtSyntaxNodeFactory::make blank nodes
///
///
ConditionElementListSyntax
StmtSyntaxNodeFactory::makeBlankConditionElementList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ConditionElementList, {},
            SourcePresence::Present, arena);
   return make<ConditionElementListSyntax>(target);
}

SwitchCaseListSyntax
StmtSyntaxNodeFactory::makeBlankSwitchCaseList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SwitchCaseList, {}, SourcePresence::Present, arena);
   return make<SwitchCaseListSyntax>(target);
}

ElseIfListSyntax
StmtSyntaxNodeFactory::makeBlankElseIfList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ElseIfList, {}, SourcePresence::Present, arena);
   return make<ElseIfListSyntax>(target);
}

InnerStmtListSyntax
StmtSyntaxNodeFactory::makeBlankInnerStmtList(RefCountPtr<SyntaxArena> arena)
{

}

TopStmtListSyntax
StmtSyntaxNodeFactory::makeBlankTopStmtList(RefCountPtr<SyntaxArena> arena)
{

}

CatchListSyntax
StmtSyntaxNodeFactory::makeBlankCatchList(RefCountPtr<SyntaxArena> arena)
{

}

CatchArgTypeHintListSyntax
StmtSyntaxNodeFactory::makeBlankCatchArgTypeHintList(RefCountPtr<SyntaxArena> arena)
{

}

UnsetVariableListSyntax
StmtSyntaxNodeFactory::makeBlankUnsetVariableList(RefCountPtr<SyntaxArena> arena)
{

}

GlobalVariableListSyntax
StmtSyntaxNodeFactory::makeBlankGlobalVariableList(RefCountPtr<SyntaxArena> arena)
{

}

StaticVariableListSyntax
StmtSyntaxNodeFactory::makeBlankStaticVariableList(RefCountPtr<SyntaxArena> arena)
{

}

NamespaceUseDeclarationListSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceUseDeclarationList(RefCountPtr<SyntaxArena> arena)
{

}

NamespaceInlineUseDeclarationListSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceInlineUseDeclarationList(RefCountPtr<SyntaxArena> arena)
{

}

NamespaceUnprefixedUseDeclarationListSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceUnprefixedUseDeclarationList(RefCountPtr<SyntaxArena> arena)
{

}

ConstDeclareItemListSyntax
StmtSyntaxNodeFactory::makeBlankConstDeclareItemList(RefCountPtr<SyntaxArena> arena)
{

}

EmptyStmtSyntax
StmtSyntaxNodeFactory::makeBlankEmptyStmt(RefCountPtr<SyntaxArena> arena)
{

}

NestStmtSyntax
StmtSyntaxNodeFactory::makeBlankNestStmt(RefCountPtr<SyntaxArena> arena)
{

}

ExprStmtSyntax
StmtSyntaxNodeFactory::makeBlankExprStmt(RefCountPtr<SyntaxArena> arena)
{

}

InnerStmtSyntax
StmtSyntaxNodeFactory::makeBlankInnerStmt(RefCountPtr<SyntaxArena> arena)
{

}

InnerCodeBlockStmtSyntax
StmtSyntaxNodeFactory::makeBlankInnerCodeBlockStmt(RefCountPtr<SyntaxArena> arena)
{

}

TopStmtSyntax
StmtSyntaxNodeFactory::makeBlankTopStmt(RefCountPtr<SyntaxArena> arena)
{

}

TopCodeBlockStmtSyntax
StmtSyntaxNodeFactory::makeBlankTopCodeBlockStmt(RefCountPtr<SyntaxArena> arena)
{

}

DeclareStmtSyntax
StmtSyntaxNodeFactory::makeBlankDeclareStmt(RefCountPtr<SyntaxArena> arena)
{

}

GotoStmtSyntax
StmtSyntaxNodeFactory::makeBlankGotoStmt(RefCountPtr<SyntaxArena> arena)
{

}

UnsetVariableSyntax
StmtSyntaxNodeFactory::makeBlankUnsetVariable(RefCountPtr<SyntaxArena> arena)
{

}

UnsetStmtSyntax
StmtSyntaxNodeFactory::makeBlankUnsetStmt(RefCountPtr<SyntaxArena> arena)
{

}

LabelStmtSyntax
StmtSyntaxNodeFactory::makeBlankLabelStmt(RefCountPtr<SyntaxArena> arena)
{

}

ConditionElementSyntax
StmtSyntaxNodeFactory::makeBlankConditionElement(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ConditionElement, {
               RawSyntax::missing(SyntaxKind::Expr),
               nullptr
            }, SourcePresence::Present, arena);
   return make<ConditionElementSyntax>(target);
}

ContinueStmtSyntax
StmtSyntaxNodeFactory::makeBlankContinueStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ContinueStmt, {
               RawSyntax::missing(TokenKindType::T_CONTINUE,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_CONTINUE))),
               nullptr
            }, SourcePresence::Present, arena);
   return make<ContinueStmtSyntax>(target);
}

BreakStmtSyntax
StmtSyntaxNodeFactory::makeBlankBreakStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BreakStmt, {
               RawSyntax::missing(TokenKindType::T_BREAK,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_BREAK))),
               nullptr
            }, SourcePresence::Present, arena);
   return make<BreakStmtSyntax>(target);
}

FallthroughStmtSyntax
StmtSyntaxNodeFactory::makeBlankFallthroughStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::FallthroughStmt, {
               RawSyntax::missing(TokenKindType::T_FALLTHROUGH,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_FALLTHROUGH)))
            }, SourcePresence::Present, arena);
   return make<FallthroughStmtSyntax>(target);
}

ElseIfClauseSyntax
StmtSyntaxNodeFactory::makeBlankElseIfClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ElseIfClause, {
               RawSyntax::missing(TokenKindType::T_ELSEIF,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_ELSEIF))),
               RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN))),
               RawSyntax::missing(SyntaxKind::Expr),
               RawSyntax::missing(TokenKindType::T_RIGHT_PAREN,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN))),
               RawSyntax::missing(SyntaxKind::CodeBlock)
            }, SourcePresence::Present, arena);
   return make<ElseIfClauseSyntax>(target);
}

IfStmtSyntax
StmtSyntaxNodeFactory::makeBlankIfStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::IfStmt, {
               nullptr,
               nullptr,
               RawSyntax::missing(TokenKindType::T_IF,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_IF))),
               RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN))),
               RawSyntax::missing(SyntaxKind::Expr),
               RawSyntax::missing(TokenKindType::T_RIGHT_PAREN,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN))),
               RawSyntax::missing(SyntaxKind::CodeBlock),
               nullptr,
               nullptr,
               nullptr
            }, SourcePresence::Present, arena);
   return make<IfStmtSyntax>(target);
}

WhileStmtSyntax
StmtSyntaxNodeFactory::makeBlankWhileStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::WhileStmt, {
               nullptr,
               nullptr,
               RawSyntax::missing(TokenKindType::T_WHILE,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_WHILE))),
               RawSyntax::missing(SyntaxKind::ConditionElementList),
               RawSyntax::missing(SyntaxKind::CodeBlock)
            }, SourcePresence::Present, arena);
   return make<WhileStmtSyntax>(target);
}

DoWhileStmtSyntax
StmtSyntaxNodeFactory::makeBlankDoWhileStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::DoWhileStmt, {
               nullptr,
               nullptr,
               RawSyntax::missing(TokenKindType::T_DO,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_DO))),
               RawSyntax::missing(SyntaxKind::CodeBlock),
               RawSyntax::missing(TokenKindType::T_WHILE,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_WHILE))),
               RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN))),
               RawSyntax::missing(SyntaxKind::Expr),
               RawSyntax::missing(TokenKindType::T_RIGHT_PAREN,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN))),
            }, SourcePresence::Present, arena);
   return make<DoWhileStmtSyntax>(target);
}


ForStmtSyntax
StmtSyntaxNodeFactory::makeBlankForStmt(RefCountPtr<SyntaxArena> arena)
{

}

ForeachVariableSyntax
StmtSyntaxNodeFactory::makeBlankForeachVariable(RefCountPtr<SyntaxArena> arena)
{

}

ForeachStmtSyntax
StmtSyntaxNodeFactory::makeBlankForeachStmt(RefCountPtr<SyntaxArena> arena)
{

}

SwitchDefaultLabelSyntax
StmtSyntaxNodeFactory::makeBlankSwitchDefaultLabel(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SwitchDefaultLabel, {
               RawSyntax::missing(TokenKindType::T_DEFAULT,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_DEFAULT))),\
               RawSyntax::missing(TokenKindType::T_COLON,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON))),
            }, SourcePresence::Present, arena);
   return make<SwitchDefaultLabelSyntax>(target);
}

SwitchCaseLabelSyntax
StmtSyntaxNodeFactory::makeBlankSwitchCaseLabel(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SwitchCaseLabel, {
               RawSyntax::missing(TokenKindType::T_CASE,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_CASE))),
               RawSyntax::missing(SyntaxKind::Expr),
               RawSyntax::missing(TokenKindType::T_COLON,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON))),
            }, SourcePresence::Present, arena);
   return make<SwitchCaseLabelSyntax>(target);
}

SwitchCaseSyntax
StmtSyntaxNodeFactory::makeBlankSwitchCase(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SwitchCase, {
               RawSyntax::missing(SyntaxKind::SwitchDefaultLabel),
               RawSyntax::missing(TokenKindType::T_COLON,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON))),
            }, SourcePresence::Present, arena);
   return make<SwitchCaseSyntax>(target);
}

SwitchStmtSyntax
StmtSyntaxNodeFactory::makeBlankSwitchStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SwitchCaseLabel, {
               nullptr,
               nullptr,
               RawSyntax::missing(TokenKindType::T_SWITCH,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_SWITCH))),
               RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN))),
               RawSyntax::missing(SyntaxKind::Expr),
               RawSyntax::missing(TokenKindType::T_RIGHT_PAREN,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN))),
               RawSyntax::missing(TokenKindType::T_LEFT_BRACE,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_BRACE))),
               RawSyntax::missing(SyntaxKind::SwitchCaseList),
               RawSyntax::missing(TokenKindType::T_RIGHT_BRACE,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_BRACE))),
            }, SourcePresence::Present, arena);
   return make<SwitchStmtSyntax>(target);
}

DeferStmtSyntax
StmtSyntaxNodeFactory::makeBlankDeferStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::DeferStmt, {
               RawSyntax::missing(TokenKindType::T_DEFER,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_DEFER))),
               RawSyntax::missing(SyntaxKind::CodeBlock)
            }, SourcePresence::Present, arena);
   return make<DeferStmtSyntax>(target);
}

ThrowStmtSyntax
StmtSyntaxNodeFactory::makeBlankThrowStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ThrowStmt, {
               RawSyntax::missing(TokenKindType::T_THROW,
               OwnedString::makeUnowned(get_token_text(TokenKindType::T_THROW))),
               RawSyntax::missing(SyntaxKind::Expr)
            }, SourcePresence::Present, arena);
   return make<ThrowStmtSyntax>(target);
}

TryStmtSyntax
StmtSyntaxNodeFactory::makeBlankTryStmt(RefCountPtr<SyntaxArena> arena)
{

}

FinallyClauseSyntax
StmtSyntaxNodeFactory::makeBlankFinallyClause(RefCountPtr<SyntaxArena> arena)
{

}

CatchArgTypeHintItemSyntax
StmtSyntaxNodeFactory::makeBlankCatchArgTypeHintItem(RefCountPtr<SyntaxArena> arena)
{

}

CatchListItemClauseSyntax
StmtSyntaxNodeFactory::makeBlankCatchListItemClause(RefCountPtr<SyntaxArena> arena)
{

}

ReturnStmtSyntax
StmtSyntaxNodeFactory::makeBlankReturnStmt(RefCountPtr<SyntaxArena> arena)
{

}

EchoStmtSyntax
StmtSyntaxNodeFactory::makeBlankEchoStmt(RefCountPtr<SyntaxArena> arena)
{

}

HaltCompilerStmtSyntax
StmtSyntaxNodeFactory::makeBlankHaltCompilerStmt(RefCountPtr<SyntaxArena> arena)
{

}

GlobalVariableListItemSyntax
StmtSyntaxNodeFactory::makeBlankGlobalVariableListItem(RefCountPtr<SyntaxArena> arena)
{

}

GlobalVariableDeclarationsStmtSyntax
StmtSyntaxNodeFactory::makeBlankGlobalVariableDeclarationsStmt(RefCountPtr<SyntaxArena> arena)
{

}

StaticVariableListItemSyntax
StmtSyntaxNodeFactory::makeBlankStaticVariableListItem(RefCountPtr<SyntaxArena> arena)
{

}

StaticVariableDeclarationsStmtSyntax
StmtSyntaxNodeFactory::makeBlankStaticVariableDeclarationsStmt(RefCountPtr<SyntaxArena> arena)
{

}

NamespaceUseTypeSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceUseType(RefCountPtr<SyntaxArena> arena)
{

}

NamespaceUnprefixedUseDeclarationSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceUnprefixedUseDeclaration(RefCountPtr<SyntaxArena> arena)
{

}

NamespaceUseDeclarationSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceUseDeclaration(RefCountPtr<SyntaxArena> arena)
{

}

NamespaceInlineUseDeclarationSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceInlineUseDeclaration(RefCountPtr<SyntaxArena> arena)
{

}

NamespaceGroupUseDeclarationSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceGroupUseDeclaration(RefCountPtr<SyntaxArena> arena)
{

}

NamespaceMixedGroupUseDeclarationSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceMixedGroupUseDeclaration(RefCountPtr<SyntaxArena> arena)
{

}

NamespaceUseStmtSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceUseStmt(RefCountPtr<SyntaxArena> arena)
{

}

NamespaceDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceDefinitionStmt(RefCountPtr<SyntaxArena> arena)
{

}

NamespaceBlockStmtSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceBlockStmt(RefCountPtr<SyntaxArena> arena)
{

}

ConstDeclareItemSyntax
StmtSyntaxNodeFactory::makeBlankConstDeclareItem(RefCountPtr<SyntaxArena> arena)
{

}

ConstDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeBlankConstDefinitionStmt(RefCountPtr<SyntaxArena> arena)
{

}

ClassDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeBlankClassDefinitionStmt(RefCountPtr<SyntaxArena> arena)
{

}

InterfaceDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeBlankInterfaceDefinitionStmt(RefCountPtr<SyntaxArena> arena)
{

}

TraitDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeBlankTraitDefinitionStmt(RefCountPtr<SyntaxArena> arena)
{

}

FunctionDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeBlankFunctionDefinitionStmt(RefCountPtr<SyntaxArena> arena)
{

}

} // polar::syntax
