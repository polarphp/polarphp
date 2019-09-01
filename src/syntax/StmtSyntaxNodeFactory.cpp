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

///
/// StmtSyntaxNodeFactory::make blank nodes
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

} // polar::syntax
