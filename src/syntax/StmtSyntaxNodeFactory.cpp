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

namespace polar::syntax {

Syntax StmtSyntaxNodeFactory::makeBlankCollectionSyntax(SyntaxKind kind)
{

}

///
/// make collection nodes
///
ConditionElementListSyntax StmtSyntaxNodeFactory::makeConditionElementListSyntax(const std::vector<ConditionElementSyntax> &elements,
                                                                                 RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const ConditionElementSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> rawConditionElementListSyntax = RawSyntax::make(SyntaxKind::ConditionElementList, layout, SourcePresence::Present,
                                                                          arena);
   return make<ConditionElementListSyntax>(rawConditionElementListSyntax);
}

SwitchCaseListSyntax StmtSyntaxNodeFactory::makeSwitchCaseListSyntax(const std::vector<SwitchCaseSyntax> &elements,
                                                                     RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const SwitchCaseSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> rawSwitchCaseListSyntax = RawSyntax::make(SyntaxKind::SwitchCaseList, layout, SourcePresence::Present,
                                                                    arena);
   return make<SwitchCaseListSyntax>(rawSwitchCaseListSyntax);
}

ElseIfListSyntax StmtSyntaxNodeFactory::makeElseIfListSyntax(const std::vector<ElseIfClauseSyntax> &elements,
                                                             RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const ElseIfClauseSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> rawElseIfListSyntax = RawSyntax::make(SyntaxKind::ElseIfList, layout, SourcePresence::Present,
                                                                arena);
   return make<ElseIfListSyntax>(rawElseIfListSyntax);
}

///
/// make normal nodes
///
ConditionElementSyntax StmtSyntaxNodeFactory::makeConditionElementSyntax(Syntax condition, std::optional<TokenSyntax> trailingComma,
                                                                         RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawConditionElementSyntax = RawSyntax::make(SyntaxKind::ConditionElement, {
                                                                         condition.getRaw(),
                                                                         trailingComma.has_value() ? trailingComma->getRaw() : nullptr
                                                                      }, SourcePresence::Present, arena);
   return make<ConditionElementSyntax>(rawConditionElementSyntax);
}

ContinueStmtSyntax StmtSyntaxNodeFactory::makeContinueStmtSyntax(TokenSyntax continueKeyword, std::optional<TokenSyntax> numberToken,
                                                                 RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawContinueStmtSyntax = RawSyntax::make(SyntaxKind::ContinueStmt, {
                                                                     continueKeyword.getRaw(),
                                                                     numberToken.has_value() ? numberToken->getRaw() : nullptr
                                                                  }, SourcePresence::Present, arena);
   return make<ContinueStmtSyntax>(rawContinueStmtSyntax);
}

BreakStmtSyntax StmtSyntaxNodeFactory::makeBreakStmtSyntax(TokenSyntax breakKeyword, std::optional<TokenSyntax> numberToken,
                                                           RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawBreakStmtSyntax = RawSyntax::make(SyntaxKind::BreakStmt, {
                                                                  breakKeyword.getRaw(),
                                                                  numberToken.has_value() ? numberToken->getRaw() : nullptr
                                                               }, SourcePresence::Present, arena);
   return make<BreakStmtSyntax>(rawBreakStmtSyntax);
}

FallthroughStmtSyntax StmtSyntaxNodeFactory::makeFallthroughStmtSyntax(TokenSyntax fallthroughKeyword, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawFallthroughStmtSyntax = RawSyntax::make(SyntaxKind::FallthroughStmt, {
                                                                        fallthroughKeyword.getRaw()
                                                                     }, SourcePresence::Present, arena);
   return make<FallthroughStmtSyntax>(rawFallthroughStmtSyntax);
}

ElseIfClauseSyntax StmtSyntaxNodeFactory::makeElseIfClauseSyntax(TokenSyntax elseIfKeyword, TokenSyntax leftParen,
                                                                 ExprSyntax condition, TokenSyntax rightParen,
                                                                 CodeBlockSyntax body, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawElseIfClauseSyntax = RawSyntax::make(SyntaxKind::ElseIfClause, {
                                                                     elseIfKeyword.getRaw(),
                                                                     leftParen.getRaw(),
                                                                     condition.getRaw(),
                                                                     rightParen.getRaw(),
                                                                     body.getRaw(),
                                                                  }, SourcePresence::Present, arena);
   return make<ElseIfClauseSyntax>(rawElseIfClauseSyntax);
}

IfStmtSyntax StmtSyntaxNodeFactory::makeIfStmtSyntax(std::optional<TokenSyntax> labelName, std::optional<TokenSyntax> labelColon,
                                                     TokenSyntax ifKeyword, TokenSyntax leftParen, ExprSyntax condition,
                                                     TokenSyntax rightParen, CodeBlockSyntax body, std::optional<ElseIfListSyntax> elseIfClauses,
                                                     std::optional<TokenSyntax> elseKeyword, std::optional<Syntax> elseBody,
                                                     RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawIfStmtSyntax = RawSyntax::make(SyntaxKind::IfStmt, {
                                                               labelName.has_value() ? labelName->getRaw() : nullptr,
                                                               labelColon.has_value() ? labelColon->getRaw() : nullptr,
                                                               ifKeyword.getRaw(),
                                                               leftParen.getRaw(),
                                                               condition.getRaw(),
                                                               rightParen.getRaw(),
                                                               body.getRaw(),
                                                               elseKeyword.has_value() ? elseKeyword->getRaw() : nullptr,
                                                               elseBody.has_value() ? elseBody->getRaw() : nullptr,
                                                            }, SourcePresence::Present, arena);
   return make<IfStmtSyntax>(rawIfStmtSyntax);
}

WhileStmtSyntax StmtSyntaxNodeFactory::makeWhileStmtSyntax(std::optional<TokenSyntax> labelName, std::optional<TokenSyntax> labelColon,
                                                           TokenSyntax whileKeyword, ConditionElementListSyntax conditions,
                                                           CodeBlockSyntax body, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawWhileStmtSyntax = RawSyntax::make(SyntaxKind::WhileStmt, {
                                                                  labelName.has_value() ? labelName->getRaw() : nullptr,
                                                                  labelColon.has_value() ? labelColon->getRaw() : nullptr,
                                                                  whileKeyword.getRaw(),
                                                                  conditions.getRaw(),
                                                                  body.getRaw()
                                                               }, SourcePresence::Present, arena);
   return make<WhileStmtSyntax>(rawWhileStmtSyntax);
}

DoWhileStmtSyntax StmtSyntaxNodeFactory::makeDoWhileStmtSyntax(std::optional<TokenSyntax> labelName, std::optional<TokenSyntax> labelColon,
                                                               TokenSyntax doKeyword, CodeBlockSyntax body, TokenSyntax whileKeyword,
                                                               TokenSyntax leftParen, ExprSyntax condition, TokenSyntax rightParen,
                                                               RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawDoWhileStmtSyntax = RawSyntax::make(SyntaxKind::DoWhileStmt, {
                                                                    labelName.has_value() ? labelName->getRaw() : nullptr,
                                                                    labelColon.has_value() ? labelColon->getRaw() : nullptr,
                                                                    doKeyword.getRaw(),
                                                                    body.getRaw(),
                                                                    whileKeyword.getRaw(),
                                                                    leftParen.getRaw(),
                                                                    condition.getRaw(),
                                                                    rightParen.getRaw(),
                                                                 }, SourcePresence::Present, arena);
   return make<DoWhileStmtSyntax>(rawDoWhileStmtSyntax);
}

SwitchDefaultLabelSyntax StmtSyntaxNodeFactory::makeSwitchDefaultLabelSyntax(TokenSyntax defaultKeyword, TokenSyntax colon,
                                                                             RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawSwitchDefaultLabelSyntax = RawSyntax::make(SyntaxKind::SwitchDefaultLabel, {
                                                                           defaultKeyword.getRaw(),
                                                                           colon.getRaw()
                                                                        }, SourcePresence::Present, arena);
   return make<SwitchDefaultLabelSyntax>(rawSwitchDefaultLabelSyntax);
}

SwitchCaseLabelSyntax StmtSyntaxNodeFactory::makeSwitchCaseLabelSyntax(TokenSyntax caseKeyword, ExprSyntax expr, TokenSyntax colon,
                                                                       RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawSwitchCaseLabelSyntax = RawSyntax::make(SyntaxKind::SwitchCaseLabel, {
                                                                        caseKeyword.getRaw(),
                                                                        expr.getRaw(),
                                                                        colon.getRaw()
                                                                     }, SourcePresence::Present, arena);
   return make<SwitchCaseLabelSyntax>(rawSwitchCaseLabelSyntax);
}

SwitchCaseSyntax StmtSyntaxNodeFactory::makeSwitchCaseSyntax(Syntax label, CodeBlockItemListSyntax statements,
                                                             RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawSwitchCaseSyntax = RawSyntax::make(SyntaxKind::SwitchCase, {
                                                                   label.getRaw(),
                                                                   statements.getRaw()
                                                                }, SourcePresence::Present, arena);
   return make<SwitchCaseSyntax>(rawSwitchCaseSyntax);
}

SwitchStmtSyntax StmtSyntaxNodeFactory::makeSwitchStmtSyntax(std::optional<TokenSyntax> labelName, std::optional<TokenSyntax> labelColon,
                                                             TokenSyntax switchKeyword, TokenSyntax leftParen, ExprSyntax conditionExpr,
                                                             TokenSyntax rightParen, TokenSyntax leftBrace, SwitchCaseListSyntax cases,
                                                             TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawSwitchStmtSyntax = RawSyntax::make(SyntaxKind::SwitchStmt, {
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
   return make<SwitchStmtSyntax>(rawSwitchStmtSyntax);
}

DeferStmtSyntax StmtSyntaxNodeFactory::makeDeferStmtSyntax(TokenSyntax deferKeyword, CodeBlockSyntax body,
                                                           RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawDeferStmtSyntax = RawSyntax::make(SyntaxKind::DeferStmt, {
                                                                  deferKeyword.getRaw(),
                                                                  body.getRaw()
                                                               }, SourcePresence::Present, arena);
   return make<DeferStmtSyntax>(rawDeferStmtSyntax);
}

ExpressionStmtSyntax StmtSyntaxNodeFactory::makeExpressionStmtSyntax(ExprSyntax expr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawExpressionStmtSyntax = RawSyntax::make(SyntaxKind::ExpressionStmt, {
                                                                       expr.getRaw()
                                                                    }, SourcePresence::Present, arena);
   return make<ExpressionStmtSyntax>(rawExpressionStmtSyntax);
}

ThrowStmtSyntax StmtSyntaxNodeFactory::makeThrowStmtSyntax(TokenSyntax throwKeyword, ExprSyntax expr,
                                                           RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawThrowStmtSyntax = RawSyntax::make(SyntaxKind::ThrowStmt, {
                                                                  throwKeyword.getRaw(),
                                                                  expr.getRaw()
                                                               }, SourcePresence::Present, arena);
   return make<ThrowStmtSyntax>(rawThrowStmtSyntax);
}

///
/// StmtSyntaxNodeFactory::make blank nodes
///
ConditionElementListSyntax StmtSyntaxNodeFactory::makeBlankConditionElementListSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawConditionElementListSyntax = RawSyntax::make(SyntaxKind::ConditionElementList, {},
                                                                          SourcePresence::Present, arena);
   return make<ConditionElementListSyntax>(rawConditionElementListSyntax);
}

SwitchCaseListSyntax StmtSyntaxNodeFactory::makeBlankSwitchCaseListSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawSwitchCaseListSyntax = RawSyntax::make(SyntaxKind::SwitchCaseList, {},
                                                                    SourcePresence::Present, arena);
   return make<SwitchCaseListSyntax>(rawSwitchCaseListSyntax);
}

ElseIfListSyntax StmtSyntaxNodeFactory::makeBlankElseIfListSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawElseIfListSyntax = RawSyntax::make(SyntaxKind::ElseIfList, {}, SourcePresence::Present, arena);
   return make<ElseIfListSyntax>(rawElseIfListSyntax);
}

ConditionElementSyntax StmtSyntaxNodeFactory::makeBlankConditionElementSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawConditionElementSyntax = RawSyntax::make(SyntaxKind::ConditionElement, {
                                                                         RawSyntax::missing(SyntaxKind::Expr),
                                                                         nullptr
                                                                      }, SourcePresence::Present, arena);
   return make<ConditionElementSyntax>(rawConditionElementSyntax);
}

ContinueStmtSyntax StmtSyntaxNodeFactory::makeBlankContinueStmtSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawContinueStmtSyntax = RawSyntax::make(SyntaxKind::ContinueStmt, {
                                                                     RawSyntax::missing(TokenKindType::T_CONTINUE,
                                                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_CONTINUE))),
                                                                     nullptr
                                                                  }, SourcePresence::Present, arena);
   return make<ContinueStmtSyntax>(rawContinueStmtSyntax);
}

BreakStmtSyntax StmtSyntaxNodeFactory::makeBlankBreakStmtSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawBreakStmtSyntax = RawSyntax::make(SyntaxKind::BreakStmt, {
                                                                  RawSyntax::missing(TokenKindType::T_BREAK,
                                                                  OwnedString::makeUnowned(get_token_text(TokenKindType::T_BREAK))),
                                                                  nullptr
                                                               }, SourcePresence::Present, arena);
   return make<BreakStmtSyntax>(rawBreakStmtSyntax);
}

FallthroughStmtSyntax StmtSyntaxNodeFactory::makeBlankFallthroughStmtSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawFallthroughStmtSyntax = RawSyntax::make(SyntaxKind::FallthroughStmt, {
                                                                        RawSyntax::missing(TokenKindType::T_FALLTHROUGH,
                                                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_FALLTHROUGH)))
                                                                     }, SourcePresence::Present, arena);
   return make<FallthroughStmtSyntax>(rawFallthroughStmtSyntax);
}

ElseIfClauseSyntax StmtSyntaxNodeFactory::makeBlankElseIfClauseSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawElseIfClauseSyntax = RawSyntax::make(SyntaxKind::ElseIfClause, {
                                                                     RawSyntax::missing(TokenKindType::T_ELSEIF,
                                                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_ELSEIF))),
                                                                     RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
                                                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN))),
                                                                     RawSyntax::missing(SyntaxKind::Expr),
                                                                     RawSyntax::missing(TokenKindType::T_RIGHT_PAREN,
                                                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN))),
                                                                     RawSyntax::missing(SyntaxKind::CodeBlock)
                                                                  }, SourcePresence::Present, arena);
   return make<ElseIfClauseSyntax>(rawElseIfClauseSyntax);
}

IfStmtSyntax StmtSyntaxNodeFactory::makeBlankIfStmtSyntax(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> rawIfStmtSyntax = RawSyntax::make(SyntaxKind::IfStmt, {
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
   return make<IfStmtSyntax>(rawIfStmtSyntax);
}

WhileStmtSyntax StmtSyntaxNodeFactory::makeBlankWhileStmtSyntax(RefCountPtr<SyntaxArena> arena)
{

}

DoWhileStmtSyntax StmtSyntaxNodeFactory::makeBlankDoWhileStmtSyntax(RefCountPtr<SyntaxArena> arena)
{

}

SwitchDefaultLabelSyntax StmtSyntaxNodeFactory::makeBlankSwitchDefaultLabelSyntax(RefCountPtr<SyntaxArena> arena)
{

}

SwitchCaseLabelSyntax StmtSyntaxNodeFactory::makeBlankSwitchCaseLabelSyntax(RefCountPtr<SyntaxArena> arena)
{

}

SwitchCaseSyntax StmtSyntaxNodeFactory::makeBlankSwitchCaseSyntax(RefCountPtr<SyntaxArena> arena)
{

}

SwitchStmtSyntax StmtSyntaxNodeFactory::makeBlankSwitchStmtSyntax(RefCountPtr<SyntaxArena> arena)
{

}

DeferStmtSyntax StmtSyntaxNodeFactory::makeBlankDeferStmtSyntax(RefCountPtr<SyntaxArena> arena)
{

}

ExpressionStmtSyntax StmtSyntaxNodeFactory::makeBlankExpressionStmtSyntax(RefCountPtr<SyntaxArena> arena)
{

}

ThrowStmtSyntax StmtSyntaxNodeFactory::makeBlankThrowStmtSyntax(RefCountPtr<SyntaxArena> arena)
{

}

} // polar::syntax
