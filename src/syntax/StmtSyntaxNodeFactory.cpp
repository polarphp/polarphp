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
      const std::vector<NamespaceUseDeclarationListItemSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const NamespaceUseDeclarationListItemSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceUseDeclarationList, layout, SourcePresence::Present,
            arena);
   return make<NamespaceUseDeclarationListSyntax>(target);
}

NamespaceInlineUseDeclarationListSyntax
StmtSyntaxNodeFactory::makeNamespaceInlineUseDeclarationList(
      const std::vector<NamespaceInlineUseDeclarationListItemSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const NamespaceInlineUseDeclarationListItemSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceInlineUseDeclarationList, layout, SourcePresence::Present,
            arena);
   return make<NamespaceInlineUseDeclarationListSyntax>(target);
}

NamespaceUnprefixedUseDeclarationListSyntax
StmtSyntaxNodeFactory::makeNamespaceUnprefixedUseDeclarationList(
      const std::vector<NamespaceUnprefixedUseDeclarationListItemSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const NamespaceUnprefixedUseDeclarationListItemSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceUnprefixedUseDeclarationList, layout, SourcePresence::Present,
            arena);
   return make<NamespaceUnprefixedUseDeclarationListSyntax>(target);
}

ConstDeclareListSyntax
StmtSyntaxNodeFactory::makeConstDeclareList(
      const std::vector<ConstListItemSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const ConstListItemSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ConstDeclareList, layout, SourcePresence::Present,
            arena);
   return make<ConstDeclareListSyntax>(target);
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
                                       ConstDeclareListSyntax constList, TokenSyntax rightParen,
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
                                        StmtSyntax body, RefCountPtr<SyntaxArena> arena)
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
                                  TokenSyntax rightParen, StmtSyntax body, std::optional<ElseIfListSyntax> elseIfClauses,
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
                                     StmtSyntax body, RefCountPtr<SyntaxArena> arena)
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
                                       TokenSyntax doKeyword, StmtSyntax body, TokenSyntax whileKeyword,
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
StmtSyntaxNodeFactory::makeSwitchCase(Syntax label, InnerStmtListSyntax statements,
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
StmtSyntaxNodeFactory::makeDeferStmt(TokenSyntax deferKeyword, InnerCodeBlockStmtSyntax body,
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
      NamespaceNameSyntax ns, std::optional<TokenSyntax> asToken,
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

NamespaceUnprefixedUseDeclarationListItemSyntax
StmtSyntaxNodeFactory::makeNamespaceUnprefixedUseDeclarationListItem(
      std::optional<TokenSyntax> comma, NamespaceUnprefixedUseDeclarationSyntax declaration,
      RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceUnprefixedUseDeclarationListItem, {
               comma.has_value() ? comma->getRaw() : nullptr,
               declaration.getRaw()
            }, SourcePresence::Present, arena);
   return make<NamespaceUnprefixedUseDeclarationListItemSyntax>(target);
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

NamespaceUseDeclarationListItemSyntax
StmtSyntaxNodeFactory::makeNamespaceUseDeclarationListItem(
      std::optional<TokenSyntax> comma, NamespaceUseDeclarationSyntax declaration,
      RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceUseDeclaration, {
               comma.has_value() ? comma->getRaw() : nullptr,
               declaration.getRaw(),
            }, SourcePresence::Present, arena);
   return make<NamespaceUseDeclarationListItemSyntax>(target);
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

NamespaceInlineUseDeclarationListItemSyntax
StmtSyntaxNodeFactory::makeNamespaceInlineUseDeclarationListItem(
      std::optional<TokenSyntax> comma, NamespaceInlineUseDeclarationSyntax declaration,
      RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceInlineUseDeclarationListItem, {
               comma.has_value() ? comma->getRaw() : nullptr,
               declaration.getRaw(),
            }, SourcePresence::Present, arena);
   return make<NamespaceInlineUseDeclarationListItemSyntax>(target);
}

NamespaceGroupUseDeclarationSyntax
StmtSyntaxNodeFactory::makeNamespaceGroupUseDeclaration(
      std::optional<TokenSyntax> firstNsSeparator, NamespaceNameSyntax ns,
      TokenSyntax secondNsSeparator, TokenSyntax leftBrace,
      NamespaceUnprefixedUseDeclarationListSyntax unprefixedUseDeclarations, std::optional<TokenSyntax> comma,
      TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceGroupUseDeclaration, {
               firstNsSeparator.has_value() ? firstNsSeparator->getRaw() : nullptr,
               ns.getRaw(),
               secondNsSeparator.getRaw(),
               leftBrace.getRaw(),
               unprefixedUseDeclarations.getRaw(),
               comma.has_value() ? comma->getRaw() : nullptr,
               rightBrace.getRaw(),
            }, SourcePresence::Present, arena);
   return make<NamespaceGroupUseDeclarationSyntax>(target);
}

NamespaceMixedGroupUseDeclarationSyntax
StmtSyntaxNodeFactory::makeNamespaceMixedGroupUseDeclaration(
      std::optional<TokenSyntax> firstNsSeparator, NamespaceNameSyntax ns,
      TokenSyntax secondNsSeparator, TokenSyntax leftBrace,
      NamespaceInlineUseDeclarationListSyntax inlineUseDeclarations, std::optional<TokenSyntax> comma,
      TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceMixedGroupUseDeclaration, {
               firstNsSeparator.has_value() ? firstNsSeparator->getRaw() : nullptr,
               ns.getRaw(),
               secondNsSeparator.getRaw(),
               leftBrace.getRaw(),
               inlineUseDeclarations.getRaw(),
               comma.has_value() ? comma->getRaw() : nullptr,
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
      TokenSyntax nsToken, NamespaceNameSyntax ns,
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
StmtSyntaxNodeFactory::makeNamespaceBlockStmt(TokenSyntax nsToken, std::optional<NamespaceNameSyntax> ns,
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

ConstListItemSyntax
StmtSyntaxNodeFactory::makeConstListItem(std::optional<TokenSyntax> comma, ConstDeclareItemSyntax declaration,
                                         RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ConstDeclareItem, {
               comma.has_value() ? comma->getRaw() : nullptr,
               declaration.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ConstListItemSyntax>(target);
}

ConstDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeConstDefinitionStmt(TokenSyntax constToken, ConstDeclareListSyntax declarations,
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
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::InnerStmtList, {}, SourcePresence::Present, arena);
   return make<InnerStmtListSyntax>(target);
}

TopStmtListSyntax
StmtSyntaxNodeFactory::makeBlankTopStmtList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::TopStmtList, {}, SourcePresence::Present, arena);
   return make<TopStmtListSyntax>(target);
}

CatchListSyntax
StmtSyntaxNodeFactory::makeBlankCatchList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CatchList, {}, SourcePresence::Present, arena);
   return make<CatchListSyntax>(target);
}

CatchArgTypeHintListSyntax
StmtSyntaxNodeFactory::makeBlankCatchArgTypeHintList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CatchArgTypeHintList, {}, SourcePresence::Present, arena);
   return make<CatchArgTypeHintListSyntax>(target);
}

UnsetVariableListSyntax
StmtSyntaxNodeFactory::makeBlankUnsetVariableList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::UnsetVariableList, {}, SourcePresence::Present, arena);
   return make<UnsetVariableListSyntax>(target);
}

GlobalVariableListSyntax
StmtSyntaxNodeFactory::makeBlankGlobalVariableList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::GlobalVariableList, {}, SourcePresence::Present, arena);
   return make<GlobalVariableListSyntax>(target);
}

StaticVariableListSyntax
StmtSyntaxNodeFactory::makeBlankStaticVariableList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::StaticVariableList, {}, SourcePresence::Present, arena);
   return make<StaticVariableListSyntax>(target);
}

NamespaceUseDeclarationListSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceUseDeclarationList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceUseDeclarationList, {}, SourcePresence::Present, arena);
   return make<NamespaceUseDeclarationListSyntax>(target);
}

NamespaceInlineUseDeclarationListSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceInlineUseDeclarationList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceInlineUseDeclarationList, {}, SourcePresence::Present, arena);
   return make<NamespaceInlineUseDeclarationListSyntax>(target);
}

NamespaceUnprefixedUseDeclarationListSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceUnprefixedUseDeclarationList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceUnprefixedUseDeclarationList, {}, SourcePresence::Present, arena);
   return make<NamespaceUnprefixedUseDeclarationListSyntax>(target);
}

ConstDeclareListSyntax
StmtSyntaxNodeFactory::makeBlankConstDeclareList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ConstDeclareList, {}, SourcePresence::Present, arena);
   return make<ConstDeclareListSyntax>(target);
}

EmptyStmtSyntax
StmtSyntaxNodeFactory::makeBlankEmptyStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EmptyStmt, {
               make_missing_token(T_SEMICOLON), // Semicolon
            }, SourcePresence::Present, arena);
   return make<EmptyStmtSyntax>(target);
}

NestStmtSyntax
StmtSyntaxNodeFactory::makeBlankNestStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NestStmt, {
               make_missing_token(T_LEFT_BRACE), // LeftBraceToken
               RawSyntax::missing(SyntaxKind::InnerStmtList), // Statements
               make_missing_token(T_LEFT_BRACE), // RightBraceToken
            }, SourcePresence::Present, arena);
   return make<NestStmtSyntax>(target);
}

ExprStmtSyntax
StmtSyntaxNodeFactory::makeBlankExprStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ExprStmt, {
               RawSyntax::missing(SyntaxKind::Expr), // Expr
               make_missing_token(T_SEMICOLON), // Semicolon
            }, SourcePresence::Present, arena);
   return make<ExprStmtSyntax>(target);
}

InnerStmtSyntax
StmtSyntaxNodeFactory::makeBlankInnerStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::InnerStmt, {
               RawSyntax::missing(SyntaxKind::Stmt), // Stmt
            }, SourcePresence::Present, arena);
   return make<InnerStmtSyntax>(target);
}

InnerCodeBlockStmtSyntax
StmtSyntaxNodeFactory::makeBlankInnerCodeBlockStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::InnerCodeBlockStmt, {
               make_missing_token(T_LEFT_BRACE), // LeftBrace
               RawSyntax::missing(SyntaxKind::InnerStmtList), // Stmt
               make_missing_token(T_RIGHT_BRACE), // RightBrace
            }, SourcePresence::Present, arena);
   return make<InnerCodeBlockStmtSyntax>(target);
}

TopStmtSyntax
StmtSyntaxNodeFactory::makeBlankTopStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::TopStmt, {
               RawSyntax::missing(SyntaxKind::Stmt), // Stmt
            }, SourcePresence::Present, arena);
   return make<TopStmtSyntax>(target);
}

TopCodeBlockStmtSyntax
StmtSyntaxNodeFactory::makeBlankTopCodeBlockStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::TopCodeBlockStmt, {
               make_missing_token(T_LEFT_BRACE), // leftBrace
               RawSyntax::missing(SyntaxKind::TopStmtList), // Statements
               make_missing_token(T_RIGHT_BRACE), // rightBrace
            }, SourcePresence::Present, arena);
   return make<TopCodeBlockStmtSyntax>(target);
}

DeclareStmtSyntax
StmtSyntaxNodeFactory::makeBlankDeclareStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::DeclareStmt, {
               make_missing_token(T_DECLARE), // DeclareToken
               make_missing_token(T_LEFT_PAREN), // LeftParenToken
               RawSyntax::missing(SyntaxKind::ConstDeclareList), // ConstList
               make_missing_token(T_RIGHT_PAREN), // RightParenToken
               RawSyntax::missing(SyntaxKind::Stmt), // Stmt
            }, SourcePresence::Present, arena);
   return make<DeclareStmtSyntax>(target);
}

GotoStmtSyntax
StmtSyntaxNodeFactory::makeBlankGotoStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::GotoStmt, {
               make_missing_token(T_GOTO), // GotoToken
               make_missing_token(T_IDENTIFIER_STRING), // Target
               make_missing_token(T_SEMICOLON), // Semicolon
            }, SourcePresence::Present, arena);
   return make<GotoStmtSyntax>(target);
}

UnsetVariableSyntax
StmtSyntaxNodeFactory::makeBlankUnsetVariable(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::UnsetVariable, {
               make_missing_token(T_VARIABLE), // Variable
               nullptr // TrailingComma
            }, SourcePresence::Present, arena);
   return make<UnsetVariableSyntax>(target);
}

UnsetStmtSyntax
StmtSyntaxNodeFactory::makeBlankUnsetStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::UnsetStmt, {
               make_missing_token(T_UNSET), // UnsetToken
               make_missing_token(T_LEFT_PAREN), // LeftParenToken
               RawSyntax::missing(SyntaxKind::UnsetVariableList), // UnsetVariables
               make_missing_token(T_RIGHT_PAREN), // RightParenToken
               make_missing_token(T_COMMA), // Semicolon
            }, SourcePresence::Present, arena);
   return make<UnsetStmtSyntax>(target);
}

LabelStmtSyntax
StmtSyntaxNodeFactory::makeBlankLabelStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::LabelStmt, {
               make_missing_token(T_IDENTIFIER_STRING), // Name
               make_missing_token(T_COLON), // Colon
            }, SourcePresence::Present, arena);
   return make<LabelStmtSyntax>(target);
}

ConditionElementSyntax
StmtSyntaxNodeFactory::makeBlankConditionElement(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ConditionElement, {
               RawSyntax::missing(SyntaxKind::Expr), // Condition
               nullptr // TrailingComma
            }, SourcePresence::Present, arena);
   return make<ConditionElementSyntax>(target);
}

ContinueStmtSyntax
StmtSyntaxNodeFactory::makeBlankContinueStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ContinueStmt, {
               make_missing_token(T_CONTINUE), // ContinueKeyword
               nullptr, // Expr
               make_missing_token(T_SEMICOLON), // Semicolon
            }, SourcePresence::Present, arena);
   return make<ContinueStmtSyntax>(target);
}

BreakStmtSyntax
StmtSyntaxNodeFactory::makeBlankBreakStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BreakStmt, {
               make_missing_token(T_BREAK), // BreakKeyword
               nullptr, // Expr
               make_missing_token(T_SEMICOLON), // Semicolon
            }, SourcePresence::Present, arena);
   return make<BreakStmtSyntax>(target);
}

FallthroughStmtSyntax
StmtSyntaxNodeFactory::makeBlankFallthroughStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::FallthroughStmt, {
               make_missing_token(T_FALLTHROUGH), // FallthroughKeyword
               make_missing_token(T_SEMICOLON), // Semicolon
            }, SourcePresence::Present, arena);
   return make<FallthroughStmtSyntax>(target);
}

ElseIfClauseSyntax
StmtSyntaxNodeFactory::makeBlankElseIfClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ElseIfClause, {
               make_missing_token(T_ELSEIF), // ElseIfKeyword
               make_missing_token(T_LEFT_PAREN), // LeftParen
               RawSyntax::missing(SyntaxKind::Expr), // Condition
               make_missing_token(T_RIGHT_PAREN), // RightParen
               RawSyntax::missing(SyntaxKind::CodeBlock) // Body
            }, SourcePresence::Present, arena);
   return make<ElseIfClauseSyntax>(target);
}

IfStmtSyntax
StmtSyntaxNodeFactory::makeBlankIfStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::IfStmt, {
               nullptr, // LabelName
               nullptr, // LabelColon
               make_missing_token(T_IF), // IfKeyword
               make_missing_token(T_LEFT_PAREN), // LeftParen
               RawSyntax::missing(SyntaxKind::Expr), // Condition
               make_missing_token(T_RIGHT_PAREN), // RightParen
               RawSyntax::missing(SyntaxKind::CodeBlock), // Body
               nullptr, // ElseIfClauses
               nullptr, // ElseKeyword
               nullptr, // ElseBody
            }, SourcePresence::Present, arena);
   return make<IfStmtSyntax>(target);
}

WhileStmtSyntax
StmtSyntaxNodeFactory::makeBlankWhileStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::WhileStmt, {
               nullptr, // LabelName
               nullptr, // LabelColon
               make_missing_token(T_WHILE), // WhileKeyword
               make_missing_token(T_LEFT_PAREN), // LeftParen
               RawSyntax::missing(SyntaxKind::ConditionElementList), // Conditions
               make_missing_token(T_RIGHT_PAREN), // RightParen
               RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt), // Body
            }, SourcePresence::Present, arena);
   return make<WhileStmtSyntax>(target);
}

DoWhileStmtSyntax
StmtSyntaxNodeFactory::makeBlankDoWhileStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::DoWhileStmt, {
               nullptr, // LabelName
               nullptr, // LabelColon
               make_missing_token(T_DO), // DoKeyword
               RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt), // Body
               make_missing_token(T_WHILE), // WhileKeyword
               make_missing_token(T_LEFT_PAREN), // LeftParen
               RawSyntax::missing(SyntaxKind::Expr), // Condition
               make_missing_token(T_RIGHT_PAREN), // RightParen
            }, SourcePresence::Present, arena);
   return make<DoWhileStmtSyntax>(target);
}

ForStmtSyntax
StmtSyntaxNodeFactory::makeBlankForStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ForStmt, {
               make_missing_token(T_FOR), // ForToken
               make_missing_token(T_LEFT_PAREN), // LeftParenToken
               nullptr, // InitializedExprs
               make_missing_token(T_SEMICOLON), // InitializedSemicolonToken
               nullptr, // ConditionalExprs
               make_missing_token(T_SEMICOLON), // ConditionalSemicolonToken
               nullptr, // OperationalExprs
               make_missing_token(T_SEMICOLON), // OperationalSemicolonToken
               make_missing_token(T_RIGHT_PAREN), // RightParenToken
               RawSyntax::missing(SyntaxKind::Stmt), // Stmt
            }, SourcePresence::Present, arena);
   return make<ForStmtSyntax>(target);
}

ForeachVariableSyntax
StmtSyntaxNodeFactory::makeBlankForeachVariable(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ForeachVariable, {
               RawSyntax::missing(SyntaxKind::Expr), // Variable
            }, SourcePresence::Present, arena);
   return make<ForeachVariableSyntax>(target);
}

ForeachStmtSyntax
StmtSyntaxNodeFactory::makeBlankForeachStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ForeachStmt, {
               make_missing_token(T_FOREACH), // ForeachToken
               make_missing_token(T_LEFT_PAREN), // LeftParenToken
               RawSyntax::missing(SyntaxKind::Expr), // VariableExpr
               make_missing_token(T_AS), // AsToken
               nullptr, // KeyVariable
               nullptr, // DoubleArrowToken
               RawSyntax::missing(SyntaxKind::ForeachVariable), // ValueVariable
               make_missing_token(T_RIGHT_PAREN), // RightParenToken
               RawSyntax::missing(SyntaxKind::Stmt), // Stmt
            }, SourcePresence::Present, arena);
   return make<ForeachStmtSyntax>(target);
}

SwitchDefaultLabelSyntax
StmtSyntaxNodeFactory::makeBlankSwitchDefaultLabel(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SwitchDefaultLabel, {
               make_missing_token(T_DEFAULT), // DefaultKeyword
               make_missing_token(T_COLON), // Colon
            }, SourcePresence::Present, arena);
   return make<SwitchDefaultLabelSyntax>(target);
}

SwitchCaseLabelSyntax
StmtSyntaxNodeFactory::makeBlankSwitchCaseLabel(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SwitchCaseLabel, {
               make_missing_token(T_CASE), // CaseKeyword
               RawSyntax::missing(SyntaxKind::Expr), // Expr
               make_missing_token(T_COLON), // Colon
            }, SourcePresence::Present, arena);
   return make<SwitchCaseLabelSyntax>(target);
}

SwitchCaseSyntax
StmtSyntaxNodeFactory::makeBlankSwitchCase(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SwitchCase, {
               RawSyntax::missing(SyntaxKind::SwitchDefaultLabel), // Label
               RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt), // Statements
            }, SourcePresence::Present, arena);
   return make<SwitchCaseSyntax>(target);
}

SwitchStmtSyntax
StmtSyntaxNodeFactory::makeBlankSwitchStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SwitchCaseLabel, {
               nullptr, // LabelName
               nullptr, // LabelColon
               make_missing_token(T_SWITCH), // SwitchKeyword
               make_missing_token(T_LEFT_PAREN), // LeftParen
               RawSyntax::missing(SyntaxKind::Expr), // ConditionExpr
               make_missing_token(T_RIGHT_PAREN), // RightParen
               make_missing_token(T_LEFT_BRACE), // LeftBrace
               RawSyntax::missing(SyntaxKind::SwitchCaseList), // Cases
               make_missing_token(T_RIGHT_BRACE), // RightBrace
            }, SourcePresence::Present, arena);
   return make<SwitchStmtSyntax>(target);
}

DeferStmtSyntax
StmtSyntaxNodeFactory::makeBlankDeferStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::DeferStmt, {
               make_missing_token(T_DEFER), // DeferKeyword
               RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt) // Body
            }, SourcePresence::Present, arena);
   return make<DeferStmtSyntax>(target);
}

ThrowStmtSyntax
StmtSyntaxNodeFactory::makeBlankThrowStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ThrowStmt, {
               make_missing_token(T_THROW), // ThrowKeyword
               RawSyntax::missing(SyntaxKind::Expr), // Expr
               make_missing_token(T_SEMICOLON), // Semicolon
            }, SourcePresence::Present, arena);
   return make<ThrowStmtSyntax>(target);
}

TryStmtSyntax
StmtSyntaxNodeFactory::makeBlankTryStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::TryStmt, {
               make_missing_token(T_TRY), // TryToken
               RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt), // CodeBlock
               nullptr, // CatchList
               nullptr, // FinallyClause
            }, SourcePresence::Present, arena);
   return make<TryStmtSyntax>(target);
}

FinallyClauseSyntax
StmtSyntaxNodeFactory::makeBlankFinallyClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::FinallyClause, {
               make_missing_token(T_FINALLY), // FinallyToken
               RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt), // CodeBlock
            }, SourcePresence::Present, arena);
   return make<FinallyClauseSyntax>(target);
}

CatchArgTypeHintItemSyntax
StmtSyntaxNodeFactory::makeBlankCatchArgTypeHintItem(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CatchArgTypeHintItem, {
               RawSyntax::missing(SyntaxKind::Name), // TypeName
               nullptr, // Separator
            }, SourcePresence::Present, arena);
   return make<CatchArgTypeHintItemSyntax>(target);
}

CatchListItemClauseSyntax
StmtSyntaxNodeFactory::makeBlankCatchListItemClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CatchListItemClause, {
               make_missing_token(T_CATCH), // CatchToken
               make_missing_token(T_LEFT_PAREN), // LeftParenToken
               RawSyntax::missing(SyntaxKind::CatchArgTypeHintList), // CatchArgTypeHintList
               make_missing_token(T_VARIABLE), // Variable
               make_missing_token(T_RIGHT_PAREN), // RightParenToken
               RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt), // CodeBlock
            }, SourcePresence::Present, arena);
   return make<CatchListItemClauseSyntax>(target);
}

ReturnStmtSyntax
StmtSyntaxNodeFactory::makeBlankReturnStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ReturnStmt, {
               make_missing_token(T_RETURN), // ReturnKeyword
               nullptr, // Expr
               make_missing_token(T_SEMICOLON), // Semicolon
            }, SourcePresence::Present, arena);
   return make<ReturnStmtSyntax>(target);
}

EchoStmtSyntax
StmtSyntaxNodeFactory::makeBlankEchoStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EchoStmt, {
               make_missing_token(T_ECHO), // EchoToken
               RawSyntax::missing(SyntaxKind::ExprList), // ExprListClause
               make_missing_token(T_SEMICOLON), // Semicolon
            }, SourcePresence::Present, arena);
   return make<EchoStmtSyntax>(target);
}

HaltCompilerStmtSyntax
StmtSyntaxNodeFactory::makeBlankHaltCompilerStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::HaltCompilerStmt, {
               make_missing_token(T_HALT_COMPILER), // HaltCompilerToken
               make_missing_token(T_LEFT_PAREN), // LeftParen
               make_missing_token(T_LEFT_PAREN), // RightParen
               make_missing_token(T_SEMICOLON), // Semicolon
            }, SourcePresence::Present, arena);
   return make<HaltCompilerStmtSyntax>(target);
}

GlobalVariableListItemSyntax
StmtSyntaxNodeFactory::makeBlankGlobalVariableListItem(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::GlobalVariableListItem, {
               make_missing_token(T_HALT_COMPILER), // HaltCompilerToken
               make_missing_token(T_LEFT_PAREN), // LeftParen
               make_missing_token(T_LEFT_PAREN), // RightParen
               make_missing_token(T_SEMICOLON), // Semicolon
            }, SourcePresence::Present, arena);
   return make<GlobalVariableListItemSyntax>(target);
}

GlobalVariableDeclarationsStmtSyntax
StmtSyntaxNodeFactory::makeBlankGlobalVariableDeclarationsStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::GlobalVariableDeclarationsStmt, {
               make_missing_token(T_GLOBAL), // GlobalToken
               RawSyntax::missing(SyntaxKind::GlobalVariableList), // Variables
               make_missing_token(T_SEMICOLON), // Semicolon
            }, SourcePresence::Present, arena);
   return make<GlobalVariableDeclarationsStmtSyntax>(target);
}

StaticVariableListItemSyntax
StmtSyntaxNodeFactory::makeBlankStaticVariableListItem(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::StaticVariableListItem, {
               make_missing_token(T_VARIABLE), // Variable
               make_missing_token(T_EQUAL),
               RawSyntax::missing(SyntaxKind::GlobalVariableList), // Variables
               make_missing_token(T_SEMICOLON), // Semicolon
            }, SourcePresence::Present, arena);
   return make<StaticVariableListItemSyntax>(target);
}

StaticVariableDeclarationsStmtSyntax
StmtSyntaxNodeFactory::makeBlankStaticVariableDeclarationsStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::StaticVariableDeclarationsStmt, {
               make_missing_token(T_STATIC), // StaticToken
               RawSyntax::missing(SyntaxKind::StaticVariableList), // Variables
               make_missing_token(T_SEMICOLON), // Semicolon
            }, SourcePresence::Present, arena);
   return make<StaticVariableDeclarationsStmtSyntax>(target);
}

NamespaceUseTypeSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceUseType(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceUseType, {
               make_missing_token(T_FUNCTION), // TypeToken
            }, SourcePresence::Present, arena);
   return make<NamespaceUseTypeSyntax>(target);
}

NamespaceUnprefixedUseDeclarationSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceUnprefixedUseDeclaration(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceUnprefixedUseDeclaration, {
               RawSyntax::missing(SyntaxKind::NamespaceName), // Namespace
               nullptr, // AsToken
               nullptr, // IdentifierToken
            }, SourcePresence::Present, arena);
   return make<NamespaceUnprefixedUseDeclarationSyntax>(target);
}

NamespaceUseDeclarationSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceUseDeclaration(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceUseDeclaration, {
               nullptr, // NsSeparator
               RawSyntax::missing(SyntaxKind::NamespaceUnprefixedUseDeclaration), // UnprefixedUseDeclaration
            }, SourcePresence::Present, arena);
   return make<NamespaceUseDeclarationSyntax>(target);
}

NamespaceInlineUseDeclarationSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceInlineUseDeclaration(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceInlineUseDeclaration, {
               nullptr, // UseType
               RawSyntax::missing(SyntaxKind::NamespaceUnprefixedUseDeclaration), // UnprefixedUseDeclaration
            }, SourcePresence::Present, arena);
   return make<NamespaceInlineUseDeclarationSyntax>(target);
}

NamespaceGroupUseDeclarationSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceGroupUseDeclaration(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceGroupUseDeclaration, {
               nullptr, // UseType
               RawSyntax::missing(SyntaxKind::NamespaceUnprefixedUseDeclaration), // UnprefixedUseDeclaration
            }, SourcePresence::Present, arena);
   return make<NamespaceGroupUseDeclarationSyntax>(target);
}

NamespaceMixedGroupUseDeclarationSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceMixedGroupUseDeclaration(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceMixedGroupUseDeclaration, {
               nullptr, // FirstNsSeparator
               RawSyntax::missing(SyntaxKind::NamespaceName), // ns
               make_missing_token(T_NS_SEPARATOR), // SecondNsSeparator
               make_missing_token(T_LEFT_PAREN), // LeftBrace
               RawSyntax::missing(SyntaxKind::NamespaceInlineUseDeclarationList), // InlineUseDeclarations
               nullptr, // comma
               make_missing_token(T_RIGHT_PAREN), // RightBrace
            }, SourcePresence::Present, arena);
   return make<NamespaceMixedGroupUseDeclarationSyntax>(target);
}

NamespaceUseStmtSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceUseStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceUseStmt, {
               make_missing_token(T_USE), // UseToken
               nullptr, // UseType
               RawSyntax::missing(SyntaxKind::Unknown), // Declarations
               make_missing_token(T_SEMICOLON), // SemicolonToken
            }, SourcePresence::Present, arena);
   return make<NamespaceUseStmtSyntax>(target);
}

NamespaceDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceDefinitionStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceDefinitionStmt, {
               make_missing_token(T_NAMESPACE), // NamespaceToken
               RawSyntax::missing(SyntaxKind::NamespaceName), // NamespaceName
               make_missing_token(T_SEMICOLON), // SemicolonToken
            }, SourcePresence::Present, arena);
   return make<NamespaceDefinitionStmtSyntax>(target);
}

NamespaceBlockStmtSyntax
StmtSyntaxNodeFactory::makeBlankNamespaceBlockStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NamespaceBlockStmt, {
               make_missing_token(T_NAMESPACE), // NamespaceToken
               nullptr, // NamespaceName
               RawSyntax::missing(SyntaxKind::TopCodeBlockStmt), // CodeBlock
            }, SourcePresence::Present, arena);
   return make<NamespaceBlockStmtSyntax>(target);
}

ConstDeclareItemSyntax
StmtSyntaxNodeFactory::makeBlankConstDeclareItem(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ConstDeclareItem, {
               make_missing_token(T_IDENTIFIER_STRING), // Name
               RawSyntax::missing(SyntaxKind::InitializerClause), // InitializerClause
            }, SourcePresence::Present, arena);
   return make<ConstDeclareItemSyntax>(target);
}

ConstDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeBlankConstDefinitionStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ConstDefinitionStmt, {
               make_missing_token(T_CONST), // ConstToken
               RawSyntax::missing(SyntaxKind::ConstDeclareList), // Declarations
               make_missing_token(T_SEMICOLON), // Semicolon
            }, SourcePresence::Present, arena);
   return make<ConstDefinitionStmtSyntax>(target);
}

ClassDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeBlankClassDefinitionStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ClassDefinition, {
               RawSyntax::missing(SyntaxKind::ClassDefinition), // ClassDefinition
            }, SourcePresence::Present, arena);
   return make<ClassDefinitionStmtSyntax>(target);
}

InterfaceDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeBlankInterfaceDefinitionStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::InterfaceDefinitionStmt, {
               RawSyntax::missing(SyntaxKind::InterfaceDefinition), // InterfaceDefinition
            }, SourcePresence::Present, arena);
   return make<InterfaceDefinitionStmtSyntax>(target);
}

TraitDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeBlankTraitDefinitionStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::TraitDefinitionStmt, {
               RawSyntax::missing(SyntaxKind::TraitDefinitionStmt), // TraitDefinitionSyntax
            }, SourcePresence::Present, arena);
   return make<TraitDefinitionStmtSyntax>(target);
}

FunctionDefinitionStmtSyntax
StmtSyntaxNodeFactory::makeBlankFunctionDefinitionStmt(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::FunctionDefinitionStmt, {
               RawSyntax::missing(SyntaxKind::FunctionDefinitionStmt), // FunctionDefinition
            }, SourcePresence::Present, arena);
   return make<FunctionDefinitionStmtSyntax>(target);
}

} // polar::syntax
