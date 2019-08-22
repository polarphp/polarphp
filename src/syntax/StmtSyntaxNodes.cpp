// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/17.

#include "polarphp/syntax/syntaxnode/StmtSyntaxNodes.h"

namespace polar::syntax {

///
/// EmptyStmtSyntax
///
void EmptyStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == EmptyStmtSyntax::CHILDREN_COUNT);
   // check condition child node choices
   syntax_assert_child_token(raw, Semicolon, std::set {TokenKindType::T_SEMICOLON});
#endif
}

TokenSyntax EmptyStmtSyntax::getSemicolon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Semicolon).get()};
}

EmptyStmtSyntax EmptyStmtSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> semicolonRaw;
   if (semicolon.has_value()) {
      semicolonRaw = semicolon->getRaw();
   } else {
      semicolonRaw = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<EmptyStmtSyntax>(semicolonRaw, Cursor::Semicolon);
}

///
/// ConditionElementSyntax
/// current only support expr condition
///
const NodeChoicesType ConditionElementSyntax::CHILD_NODE_CHOICES
{
   {
      ConditionElementSyntax::Cursor::Condition, {
         SyntaxKind::Expr
      }
   }
};

void ConditionElementSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ConditionElementSyntax::CHILDREN_COUNT);
   // check condition child node choices
   syntax_assert_child_kind(raw, Condition, std::set{SyntaxKind::Expr});
#endif
}

Syntax ConditionElementSyntax::getCondition()
{
   return Syntax{m_root, m_data->getChild(Cursor::Condition).get()};
}

std::optional<TokenSyntax> ConditionElementSyntax::getTrailingComma()
{
   RefCountPtr<SyntaxData> trailingCommaData = m_data->getChild(Cursor::TrailingComma);
   if (!trailingCommaData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, trailingCommaData.get()};
}

ConditionElementSyntax ConditionElementSyntax::withCondition(std::optional<Syntax> condition)
{
   RefCountPtr<RawSyntax> rawCondition;
   if (condition.has_value()) {
      rawCondition = condition->getRaw();
   } else {
      rawCondition = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<ConditionElementSyntax>(rawCondition, Cursor::Condition);
}

ConditionElementSyntax ConditionElementSyntax::withTrailingComma(std::optional<TokenSyntax> trailingComma)
{
   RefCountPtr<RawSyntax> rawTrailingComma;
   if (trailingComma.has_value()) {
      rawTrailingComma = trailingComma->getRaw();
   } else {
      rawTrailingComma = nullptr;
   }
   return m_data->replaceChild<ConditionElementSyntax>(rawTrailingComma, Cursor::TrailingComma);
}

///
/// ContinueStmtSyntax
///

void ContinueStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ContinueStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ContinueKeyword, std::set{TokenKindType::T_CONTINUE});
   syntax_assert_child_token(raw, LNumberToken, std::set{TokenKindType::T_LNUMBER});
#endif
}

TokenSyntax ContinueStmtSyntax::getContinueKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::ContinueKeyword).get()};
}

std::optional<TokenSyntax> ContinueStmtSyntax::getLNumberToken()
{
   RefCountPtr<SyntaxData> numberTokenData = m_data->getChild(Cursor::LNumberToken);
   if (!numberTokenData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, numberTokenData.get()};
}

ContinueStmtSyntax ContinueStmtSyntax::withContinueKeyword(std::optional<TokenSyntax> continueKeyword)
{
   RefCountPtr<RawSyntax> rawContinueKeyword;
   if (continueKeyword.has_value()) {
      rawContinueKeyword = continueKeyword->getRaw();
   } else {
      rawContinueKeyword = RawSyntax::missing(TokenKindType::T_CONTINUE,
                                              OwnedString::makeUnowned(get_token_text(TokenKindType::T_CONTINUE)));
   }
   return m_data->replaceChild<ContinueStmtSyntax>(rawContinueKeyword, Cursor::ContinueKeyword);
}

ContinueStmtSyntax ContinueStmtSyntax::withLNumberToken(std::optional<TokenSyntax> numberToken)
{
   RefCountPtr<RawSyntax> rawNumberToken;
   if (numberToken.has_value()) {
      rawNumberToken = numberToken->getRaw();
   } else {
      rawNumberToken = nullptr;
   }
   return m_data->replaceChild<ContinueStmtSyntax>(rawNumberToken, Cursor::LNumberToken);
}

///
/// BreakStmtSyntax
///
void BreakStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == BreakStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, BreakKeyword, std::set{TokenKindType::T_BREAK});
   syntax_assert_child_token(raw, LNumberToken, std::set{TokenKindType::T_LNUMBER});
#endif
}

TokenSyntax BreakStmtSyntax::getBreakKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::BreakKeyword).get()};
}

std::optional<TokenSyntax> BreakStmtSyntax::getLNumberToken()
{
   RefCountPtr<SyntaxData> numberTokenData = m_data->getChild(Cursor::LNumberToken);
   if (!numberTokenData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, numberTokenData.get()};
}

BreakStmtSyntax BreakStmtSyntax::withBreakKeyword(std::optional<TokenSyntax> breakKeyword)
{
   RefCountPtr<RawSyntax> rawBreakKeyword;
   if (breakKeyword.has_value()) {
      rawBreakKeyword = breakKeyword->getRaw();
   } else {
      rawBreakKeyword = RawSyntax::missing(TokenKindType::T_BREAK,
                                           OwnedString::makeUnowned(get_token_text(TokenKindType::T_BREAK)));
   }
   return m_data->replaceChild<BreakStmtSyntax>(rawBreakKeyword, Cursor::BreakKeyword);
}

BreakStmtSyntax BreakStmtSyntax::withLNumberToken(std::optional<TokenSyntax> numberToken)
{
   RefCountPtr<RawSyntax> rawNumberToken;
   if (numberToken.has_value()) {
      rawNumberToken = numberToken->getRaw();
   } else {
      rawNumberToken = nullptr;
   }
   return m_data->replaceChild<BreakStmtSyntax>(rawNumberToken, Cursor::LNumberToken);
}

///
/// FallthroughStmtSyntax
///
void FallthroughStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if(isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == FallthroughStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, FallthroughKeyword, std::set{TokenKindType::T_FALLTHROUGH});
#endif
}

TokenSyntax FallthroughStmtSyntax::getFallthroughKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::FallthroughKeyword).get()};
}

FallthroughStmtSyntax FallthroughStmtSyntax::withFallthroughStmtSyntax(std::optional<TokenSyntax> fallthroughKeyword)
{
   RefCountPtr<RawSyntax> rawFallthroughKeyword;
   if (fallthroughKeyword.has_value()) {
      rawFallthroughKeyword = fallthroughKeyword->getRaw();
   } else {
      rawFallthroughKeyword = RawSyntax::missing(TokenKindType::T_FALLTHROUGH,
                                                 OwnedString::makeUnowned(get_token_text(TokenKindType::T_FALLTHROUGH)));
   }
   return m_data->replaceChild<FallthroughStmtSyntax>(rawFallthroughKeyword, TokenKindType::T_FALLTHROUGH);
}

///
/// ElseIfClauseSyntax
///
void ElseIfClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ElseIfClauseSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax ElseIfClauseSyntax::getElseIfKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::ElseIfKeyword).get()};
}

TokenSyntax ElseIfClauseSyntax::getLeftParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftParen).get()};
}

ExprSyntax ElseIfClauseSyntax::getCondition()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Condition).get()};
}

TokenSyntax ElseIfClauseSyntax::getRightParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::RightParen).get()};
}

CodeBlockSyntax ElseIfClauseSyntax::getBody()
{
   return CodeBlockSyntax{m_root, m_data->getChild(Cursor::Body).get()};
}

ElseIfClauseSyntax ElseIfClauseSyntax::withElseIfKeyword(std::optional<TokenSyntax> elseIfKeyword)
{
   RefCountPtr<RawSyntax> rawElseIfKeyword;
   if (elseIfKeyword.has_value()) {
      rawElseIfKeyword = elseIfKeyword->getRaw();
   } else {
      rawElseIfKeyword = RawSyntax::missing(TokenKindType::T_ELSEIF,
                                            OwnedString::makeUnowned(get_token_text(TokenKindType::T_ELSEIF)));
   }
   return m_data->replaceChild<ElseIfClauseSyntax>(rawElseIfKeyword, Cursor::ElseIfKeyword);
}

ElseIfClauseSyntax ElseIfClauseSyntax::withLeftParen(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> rawLeftParen;
   if (leftParen.has_value()) {
      rawLeftParen = leftParen->getRaw();
   } else {
      rawLeftParen = RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN)));
   }
   return m_data->replaceChild<ElseIfClauseSyntax>(rawLeftParen, Cursor::LeftParen);
}

ElseIfClauseSyntax ElseIfClauseSyntax::withCondition(std::optional<ExprSyntax> condition)
{
   RefCountPtr<RawSyntax> rawCondition;
   if (condition.has_value()) {
      rawCondition = condition->getRaw();
   } else {
      rawCondition = RawSyntax::missing(SyntaxKind::UnknownExpr);
   }
   return m_data->replaceChild<ElseIfClauseSyntax>(rawCondition, Cursor::Condition);
}

ElseIfClauseSyntax ElseIfClauseSyntax::withRightParen(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rawRightParen;
   if (rightParen.has_value()) {
      rawRightParen = rightParen->getRaw();
   } else {
      rawRightParen = RawSyntax::missing(TokenKindType::T_RIGHT_PAREN,
                                         OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN)));
   }
   return m_data->replaceChild<ElseIfClauseSyntax>(rawRightParen, Cursor::LeftParen);
}

ElseIfClauseSyntax ElseIfClauseSyntax::withBody(std::optional<CodeBlockSyntax> body)
{
   RefCountPtr<RawSyntax> rawBody;
   if (body.has_value()) {
      rawBody = body->getRaw();
   } else {
      rawBody = RawSyntax::missing(SyntaxKind::CodeBlock);
   }
   return m_data->replaceChild<ElseIfClauseSyntax>(rawBody, Cursor::Body);
}

///
/// IfStmtSyntax
///

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType IfStmtSyntax::CHILD_NODE_CHOICES
{
   {IfStmtSyntax::ElseBody, {
         SyntaxKind::IfStmt,
               SyntaxKind::CodeBlock
      }}
};
#endif

void IfStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   if (const RefCountPtr<RawSyntax> &elseBody = raw->getChild(Cursor::ElseBody)) {
      bool isIfStmt = elseBody->kindOf(SyntaxKind::IfStmt);
      bool isCodeBlock = elseBody->kindOf(SyntaxKind::CodeBlock);
      assert(isIfStmt || isCodeBlock);
   }
   assert(raw->getLayout().size() == IfStmtSyntax::CHILDREN_COUNT);
#endif
}

std::optional<TokenSyntax> IfStmtSyntax::getLabelName()
{
   RefCountPtr<SyntaxData> labelNameData = m_data->getChild(Cursor::LabelName);
   if (!labelNameData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, labelNameData.get()};
}

std::optional<TokenSyntax> IfStmtSyntax::getLabelColon()
{
   RefCountPtr<SyntaxData> labelColonData = m_data->getChild(Cursor::LabelColon);
   if (!labelColonData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, labelColonData.get()};
}

TokenSyntax IfStmtSyntax::getIfKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::IfKeyword).get()};
}

TokenSyntax IfStmtSyntax::getLeftParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftParen).get()};
}

ExprSyntax IfStmtSyntax::getCondition()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Condition).get()};
}

TokenSyntax IfStmtSyntax::getRightParen()
{
   return TokenSyntax(m_root, m_data->getChild(Cursor::RightParen).get());
}

CodeBlockSyntax IfStmtSyntax::getBody()
{
   return CodeBlockSyntax{m_root, m_data->getChild(Cursor::Body).get()};
}

std::optional<ElseIfListSyntax> IfStmtSyntax::getElseIfClauses()
{
   RefCountPtr<SyntaxData> elseIfClausesData = m_data->getChild(Cursor::ElseIfClauses);
   if (!elseIfClausesData) {
      return std::nullopt;
   }
   return ElseIfListSyntax{m_root, elseIfClausesData.get()};
}

std::optional<TokenSyntax> IfStmtSyntax::getElseKeyword()
{
   RefCountPtr<SyntaxData> elseKeywordData = m_data->getChild(Cursor::ElseKeyword);
   if (!elseKeywordData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, elseKeywordData.get()};
}

std::optional<Syntax> IfStmtSyntax::getElseBody()
{
   RefCountPtr<SyntaxData> elseBodyData = m_data->getChild(Cursor::ElseBody);
   if (!elseBodyData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, elseBodyData.get()};
}

IfStmtSyntax IfStmtSyntax::withLabelName(std::optional<TokenSyntax> labelName)
{
   RefCountPtr<RawSyntax> rawLabelName;
   if (labelName.has_value()) {
      rawLabelName = labelName->getRaw();
   } else {
      rawLabelName = nullptr;
   }
   return m_data->replaceChild<IfStmtSyntax>(rawLabelName, Cursor::LabelName);
}

IfStmtSyntax IfStmtSyntax::withLabelColon(std::optional<TokenSyntax> labelColon)
{
   RefCountPtr<RawSyntax> rawLabelColon;
   if (labelColon.has_value()) {
      rawLabelColon = labelColon->getRaw();
   } else {
      rawLabelColon = nullptr;
   }
   return m_data->replaceChild<IfStmtSyntax>(rawLabelColon, Cursor::LabelColon);
}

IfStmtSyntax IfStmtSyntax::withIfKeyword(std::optional<TokenSyntax> ifKeyword)
{
   RefCountPtr<RawSyntax> rawIfKeyword;
   if (ifKeyword.has_value()) {
      rawIfKeyword = ifKeyword->getRaw();
   } else {
      rawIfKeyword = RawSyntax::missing(TokenKindType::T_IF,
                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_IF)));
   }
   return m_data->replaceChild<IfStmtSyntax>(rawIfKeyword, Cursor::IfKeyword);
}

IfStmtSyntax IfStmtSyntax::withLeftParen(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> rawLeftParen;
   if (leftParen.has_value()) {
      rawLeftParen = leftParen->getRaw();
   } else {
      rawLeftParen = RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN)));
   }
   return m_data->replaceChild<IfStmtSyntax>(rawLeftParen, Cursor::LeftParen);
}

IfStmtSyntax IfStmtSyntax::withCondition(std::optional<ExprSyntax> condition)
{
   RefCountPtr<RawSyntax> rawCondition;
   if (condition.has_value()) {
      rawCondition = condition->getRaw();
   } else {
      rawCondition = RawSyntax::missing(SyntaxKind::UnknownExpr);
   }
   return m_data->replaceChild<IfStmtSyntax>(rawCondition, Cursor::Condition);
}

IfStmtSyntax IfStmtSyntax::withRightParen(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rawRightParen;
   if (rightParen.has_value()) {
      rawRightParen = rightParen->getRaw();
   } else {
      rawRightParen = RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
                                         OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN)));
   }
   return m_data->replaceChild<IfStmtSyntax>(rawRightParen, Cursor::LeftParen);
}

IfStmtSyntax IfStmtSyntax::withBody(std::optional<CodeBlockSyntax> body)
{
   RefCountPtr<RawSyntax> rawBody;
   if (body.has_value()) {
      rawBody = body->getRaw();
   } else {
      rawBody = RawSyntax::missing(SyntaxKind::CodeBlock);
   }
   return m_data->replaceChild<IfStmtSyntax>(rawBody, Cursor::Body);
}

IfStmtSyntax IfStmtSyntax::withElseIfClauses(std::optional<ElseIfListSyntax> elseIfClauses)
{
   RefCountPtr<RawSyntax> rawElseIfClauses;
   if (elseIfClauses.has_value()) {
      rawElseIfClauses = elseIfClauses->getRaw();
   } else {
      rawElseIfClauses = nullptr;
   }
   return m_data->replaceChild<IfStmtSyntax>(rawElseIfClauses, Cursor::ElseIfClauses);
}

IfStmtSyntax IfStmtSyntax::withElseKeyword(std::optional<TokenSyntax> elseKeyword)
{
   RefCountPtr<RawSyntax> rawElseKeyword;
   if (elseKeyword.has_value()) {
      rawElseKeyword = elseKeyword->getRaw();
   } else {
      rawElseKeyword = nullptr;
   }
   return m_data->replaceChild<IfStmtSyntax>(rawElseKeyword, Cursor::ElseKeyword);
}

IfStmtSyntax IfStmtSyntax::withElseBody(std::optional<Syntax> elseBody)
{
   RefCountPtr<RawSyntax> rawElseBody;
   if (elseBody.has_value()) {
      rawElseBody = elseBody->getRaw();
   } else {
      rawElseBody = nullptr;
   }
   return m_data->replaceChild<IfStmtSyntax>(rawElseBody, Cursor::ElseBody);
}

IfStmtSyntax IfStmtSyntax::addElseIfClause(ElseIfClauseSyntax elseIfClause)
{
   RefCountPtr<RawSyntax> elseIfClauses = getRaw()->getChild(Cursor::ElseIfClauses);
   if (elseIfClauses) {
      elseIfClauses = elseIfClauses->append(elseIfClause.getRaw());
   } else {
      elseIfClauses = RawSyntax::make(SyntaxKind::ElseIfList, {elseIfClause.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<IfStmtSyntax>(elseIfClauses, Cursor::ElseIfClauses);
}

///
/// WhileStmtSyntax
///
void WhileStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == WhileStmtSyntax::CHILDREN_COUNT);
#endif
}

std::optional<TokenSyntax> WhileStmtSyntax::getLabelName()
{
   RefCountPtr<SyntaxData> labelNameData = m_data->getChild(Cursor::LabelName);
   if (!labelNameData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, labelNameData.get()};
}

std::optional<TokenSyntax> WhileStmtSyntax::getLabelColon()
{
   RefCountPtr<SyntaxData> labelColonData = m_data->getChild(Cursor::LabelColon);
   if (!labelColonData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, labelColonData.get()};
}

TokenSyntax WhileStmtSyntax::getWhileKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::WhileKeyword).get()};
}

TokenSyntax WhileStmtSyntax::getLeftParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftParen).get()};
}

ConditionElementListSyntax WhileStmtSyntax::getConditions()
{
   return ConditionElementListSyntax{m_root, m_data->getChild(Cursor::Conditions).get()};
}

TokenSyntax WhileStmtSyntax::getRightParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::RightParen).get()};
}

CodeBlockSyntax WhileStmtSyntax::getBody()
{
   return CodeBlockSyntax{m_root, m_data->getChild(Cursor::Body).get()};
}

WhileStmtSyntax WhileStmtSyntax::withLabelName(std::optional<TokenSyntax> labelName)
{
   RefCountPtr<RawSyntax> rawLabelName;
   if (labelName.has_value()) {
      rawLabelName = labelName->getRaw();
   } else {
      rawLabelName = nullptr;
   }
   return m_data->replaceChild<WhileStmtSyntax>(rawLabelName, Cursor::LabelName);
}

WhileStmtSyntax WhileStmtSyntax::withLabelColon(std::optional<TokenSyntax> labelColon)
{
   RefCountPtr<RawSyntax> rawLabelColon;
   if (labelColon.has_value()) {
      rawLabelColon = labelColon->getRaw();
   } else {
      rawLabelColon = nullptr;
   }
   return m_data->replaceChild<WhileStmtSyntax>(rawLabelColon, Cursor::LabelColon);
}

WhileStmtSyntax WhileStmtSyntax::withWhileKeyword(std::optional<TokenSyntax> whileKeyword)
{
   RefCountPtr<RawSyntax> rawWhileKeyword;
   if (whileKeyword.has_value()) {
      rawWhileKeyword = whileKeyword->getRaw();
   } else {
      rawWhileKeyword = RawSyntax::missing(TokenKindType::T_WHILE,
                                           OwnedString::makeUnowned(get_token_text(TokenKindType::T_WHILE)));
   }
   return m_data->replaceChild<WhileStmtSyntax>(rawWhileKeyword, Cursor::WhileKeyword);
}

WhileStmtSyntax WhileStmtSyntax::withLeftParen(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> rawLeftParen;
   if (leftParen.has_value()) {
      rawLeftParen = leftParen->getRaw();
   } else {
      rawLeftParen = RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN)));
   }
   return m_data->replaceChild<WhileStmtSyntax>(rawLeftParen, Cursor::LeftParen);
}

WhileStmtSyntax WhileStmtSyntax::withConditions(std::optional<ConditionElementListSyntax> conditions)
{
   RefCountPtr<RawSyntax> rawConditions;
   if (conditions.has_value()) {
      rawConditions = conditions->getRaw();
   } else {
      rawConditions = RawSyntax::missing(SyntaxKind::ConditionElementList);
   }
   return m_data->replaceChild<WhileStmtSyntax>(rawConditions, Cursor::Conditions);
}

WhileStmtSyntax WhileStmtSyntax::withRightParen(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rawRightParen;
   if (rightParen.has_value()) {
      rawRightParen = rightParen->getRaw();
   } else {
      rawRightParen = RawSyntax::missing(TokenKindType::T_RIGHT_PAREN,
                                         OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN)));
   }
   return m_data->replaceChild<WhileStmtSyntax>(rawRightParen, Cursor::RightParen);
}

WhileStmtSyntax WhileStmtSyntax::withBody(std::optional<CodeBlockSyntax> body)
{
   RefCountPtr<RawSyntax> rawBody;
   if (body.has_value()) {
      rawBody = body->getRaw();
   } else {
      rawBody = RawSyntax::missing(SyntaxKind::CodeBlock);
   }
   return m_data->replaceChild<WhileStmtSyntax>(rawBody, Cursor::Body);
}

WhileStmtSyntax WhileStmtSyntax::addCondition(ConditionElementSyntax condition)
{
   RefCountPtr<RawSyntax> conditions = getRaw()->getChild(Cursor::Conditions);
   if (conditions) {
      conditions = conditions->append(condition.getRaw());
   } else {
      conditions = RawSyntax::make(SyntaxKind::ConditionElementList, {condition.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<WhileStmtSyntax>(conditions, Cursor::Conditions);
}

///
/// DoWhileStmtSyntax
///
void DoWhileStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == DoWhileStmtSyntax::CHILDREN_COUNT);
#endif
}

std::optional<TokenSyntax> DoWhileStmtSyntax::getLabelName()
{
   RefCountPtr<SyntaxData> labelNameData = m_data->getChild(Cursor::LabelName);
   if (!labelNameData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, labelNameData.get()};
}

std::optional<TokenSyntax> DoWhileStmtSyntax::getLabelColon()
{
   RefCountPtr<SyntaxData> labelColonData = m_data->getChild(Cursor::LabelColon);
   if (!labelColonData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, labelColonData.get()};
}

TokenSyntax DoWhileStmtSyntax::getDoKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::DoKeyword).get()};
}

CodeBlockSyntax DoWhileStmtSyntax::getBody()
{
   return CodeBlockSyntax{m_root, m_data->getChild(Cursor::Body).get()};
}

TokenSyntax DoWhileStmtSyntax::getWhileKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::WhileKeyword).get()};
}

TokenSyntax DoWhileStmtSyntax::getLeftParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftParen).get()};
}

ExprSyntax DoWhileStmtSyntax::getCondition()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Condition).get()};
}

TokenSyntax DoWhileStmtSyntax::getRightParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::RightParen).get()};
}

DoWhileStmtSyntax DoWhileStmtSyntax::withLabelName(std::optional<TokenSyntax> labelName)
{
   RefCountPtr<RawSyntax> rawLabelName;
   if (labelName.has_value()) {
      rawLabelName = labelName->getRaw();
   } else {
      rawLabelName = nullptr;
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawLabelName, Cursor::LabelName);
}

DoWhileStmtSyntax DoWhileStmtSyntax::withLabelColon(std::optional<TokenSyntax> labelColon)
{
   RefCountPtr<RawSyntax> rawLabelName;
   if (labelColon.has_value()) {
      rawLabelName = labelColon->getRaw();
   } else {
      rawLabelName = nullptr;
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawLabelName, Cursor::LabelColon);
}

DoWhileStmtSyntax DoWhileStmtSyntax::withDoKeyword(std::optional<TokenSyntax> doKeyword)
{
   RefCountPtr<RawSyntax> rawDoKeyword;
   if (doKeyword.has_value()) {
      rawDoKeyword = doKeyword->getRaw();
   } else {
      rawDoKeyword = RawSyntax::missing(TokenKindType::T_DO,
                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_DO)));
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawDoKeyword, Cursor::DoKeyword);
}

DoWhileStmtSyntax DoWhileStmtSyntax::withBody(std::optional<CodeBlockSyntax> body)
{
   RefCountPtr<RawSyntax> rawBody;
   if (body.has_value()) {
      rawBody = body->getRaw();
   } else {
      rawBody = RawSyntax::missing(SyntaxKind::CodeBlock);
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawBody, Cursor::Body);
}

DoWhileStmtSyntax DoWhileStmtSyntax::withWhileKeyword(std::optional<TokenSyntax> whileKeyword)
{
   RefCountPtr<RawSyntax> rawWhileKeyword;
   if (whileKeyword.has_value()) {
      rawWhileKeyword = whileKeyword->getRaw();
   } else {
      rawWhileKeyword = RawSyntax::missing(TokenKindType::T_WHILE,
                                           OwnedString::makeUnowned(get_token_text(TokenKindType::T_WHILE)));
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawWhileKeyword, Cursor::WhileKeyword);
}

DoWhileStmtSyntax DoWhileStmtSyntax::withLeftParen(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> rawLeftParen;
   if (leftParen.has_value()) {
      rawLeftParen = leftParen->getRaw();
   } else {
      rawLeftParen = RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN)));
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawLeftParen, Cursor::LeftParen);
}

DoWhileStmtSyntax DoWhileStmtSyntax::withCondition(std::optional<ExprSyntax> condition)
{
   RefCountPtr<RawSyntax> rawCondition;
   if (condition.has_value()) {
      rawCondition = condition->getRaw();
   } else {
      rawCondition = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawCondition, Cursor::Condition);
}

DoWhileStmtSyntax DoWhileStmtSyntax::withRightParen(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rawRightParen;
   if (rightParen.has_value()) {
      rawRightParen = rightParen->getRaw();
   } else {
      rawRightParen = RawSyntax::missing(TokenKindType::T_RIGHT_PAREN,
                                         OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN)));
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawRightParen, Cursor::RightParen);
}

///
/// SwitchDefaultLabelSyntax
///
void SwitchDefaultLabelSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SwitchDefaultLabelSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax SwitchDefaultLabelSyntax::getDefaultKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::DefaultKeyword).get()};
}

TokenSyntax SwitchDefaultLabelSyntax::getColon()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Colon).get()};
}

SwitchDefaultLabelSyntax SwitchDefaultLabelSyntax::withDefaultKeyword(std::optional<TokenSyntax> defaultKeyword)
{
   RefCountPtr<RawSyntax> rawDefaultKeyword;
   if (defaultKeyword.has_value()) {
      rawDefaultKeyword = defaultKeyword->getRaw();
   } else {
      rawDefaultKeyword = RawSyntax::missing(TokenKindType::T_DEFAULT,
                                             OwnedString::makeUnowned(get_token_text(TokenKindType::T_DEFAULT)));
   }
   return m_data->replaceChild<SwitchDefaultLabelSyntax>(rawDefaultKeyword, Cursor::DefaultKeyword);
}

SwitchDefaultLabelSyntax SwitchDefaultLabelSyntax::withColon(std::optional<TokenSyntax> colon)
{
   RefCountPtr<RawSyntax> rawColon;
   if (colon.has_value()) {
      rawColon = colon->getRaw();
   } else {
      rawColon = RawSyntax::missing(TokenKindType::T_COLON,
                                    OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON)));
   }
   return m_data->replaceChild<SwitchDefaultLabelSyntax>(rawColon, Cursor::Colon);
}

///
/// SwitchCaseLabelSyntax
///
void SwitchCaseLabelSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SwitchCaseLabelSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax SwitchCaseLabelSyntax::getCaseKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::CaseKeyword).get()};
}

ExprSyntax SwitchCaseLabelSyntax::getExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Expr).get()};
}

TokenSyntax SwitchCaseLabelSyntax::getColon()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Colon).get()};
}

SwitchCaseLabelSyntax SwitchCaseLabelSyntax::withCaseKeyword(std::optional<TokenSyntax> caseKeyword)
{
   RefCountPtr<RawSyntax> rawCaseKeyword;
   if (caseKeyword.has_value()) {
      rawCaseKeyword = caseKeyword->getRaw();
   } else {
      rawCaseKeyword = RawSyntax::missing(TokenKindType::T_CASE,
                                          OwnedString::makeUnowned(get_token_text(TokenKindType::T_CASE)));
   }
   return m_data->replaceChild<SwitchCaseLabelSyntax>(rawCaseKeyword, Cursor::CaseKeyword);
}

SwitchCaseLabelSyntax SwitchCaseLabelSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> rawExpr;
   if (expr.has_value()) {
      rawExpr = expr->getRaw();
   } else {
      rawExpr = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<SwitchCaseLabelSyntax>(rawExpr, Cursor::Expr);
}

SwitchCaseLabelSyntax SwitchCaseLabelSyntax::withColon(std::optional<TokenSyntax> colon)
{
   RefCountPtr<RawSyntax> rawColon;
   if (colon.has_value()) {
      rawColon = colon->getRaw();
   } else {
      rawColon = RawSyntax::missing(TokenKindType::T_COLON,
                                    OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON)));
   }
   return m_data->replaceChild<SwitchCaseLabelSyntax>(rawColon, Cursor::Colon);
}

///
/// SwitchCaseSyntax
///

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType SwitchCaseSyntax::CHILD_NODE_CHOICES
{
   {
      SwitchCaseSyntax::Cursor::Statements,{
         SyntaxKind::SwitchDefaultLabel,
               SyntaxKind::SwitchCaseLabel
      }
   }
};
#endif

void SwitchCaseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SwitchCaseSyntax::CHILDREN_COUNT);
   if (const RefCountPtr<RawSyntax> &statements = raw->getChild(Cursor::Statements)) {
      bool isDefaultLabel = statements->kindOf(SyntaxKind::SwitchDefaultLabel);
      bool isCaseLabel = statements->kindOf(SyntaxKind::SwitchCaseLabel);
      assert(isDefaultLabel || isCaseLabel);
   }
#endif
}

Syntax SwitchCaseSyntax::getLabel()
{
   return Syntax{m_root, m_data->getChild(Cursor::Label).get()};
}

CodeBlockItemListSyntax SwitchCaseSyntax::getStatements()
{
   return CodeBlockItemListSyntax{m_root, m_data->getChild(Cursor::Statements).get()};
}

SwitchCaseSyntax SwitchCaseSyntax::withLabel(std::optional<Syntax> label)
{
   RefCountPtr<RawSyntax> rawLabel;
   if (label.has_value()) {
      rawLabel = label->getRaw();
   } else {
      rawLabel = RawSyntax::missing(SyntaxKind::SwitchDefaultLabel);
   }
   return m_data->replaceChild<SwitchCaseSyntax>(rawLabel, Cursor::Label);
}

SwitchCaseSyntax SwitchCaseSyntax::withStatements(std::optional<CodeBlockItemListSyntax> statements)
{
   RefCountPtr<RawSyntax> rawStatements;
   if (statements.has_value()) {
      rawStatements = statements->getRaw();
   } else {
      rawStatements = RawSyntax::missing(SyntaxKind::CodeBlockItemList);
   }
   return m_data->replaceChild<SwitchCaseSyntax>(rawStatements, Cursor::Statements);
}

SwitchCaseSyntax SwitchCaseSyntax::addStatement(CodeBlockItemSyntax statement)
{
   RefCountPtr<RawSyntax> statements = getRaw()->getChild(Cursor::Statements);
   if (statements) {
      statements->append(statement.getRaw());
   } else {
      statements = RawSyntax::make(SyntaxKind::CodeBlockItemList, {statement.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<SwitchCaseSyntax>(statements, Cursor::Statements);
}

///
/// SwitchStmtSyntax
///
void SwitchStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SwitchStmtSyntax::CHILDREN_COUNT);
#endif
}

std::optional<TokenSyntax> SwitchStmtSyntax::getLabelName()
{
   RefCountPtr<SyntaxData> labelNameData = m_data->getChild(Cursor::LabelName);
   if (!labelNameData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, labelNameData.get()};
}

std::optional<TokenSyntax> SwitchStmtSyntax::getLabelColon()
{
   RefCountPtr<SyntaxData> labelColonData = m_data->getChild(Cursor::LabelColon);
   if (!labelColonData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, labelColonData.get()};
}

TokenSyntax SwitchStmtSyntax::getSwitchKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::SwitchKeyword).get()};
}

TokenSyntax SwitchStmtSyntax::getLeftParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftParen).get()};
}

ExprSyntax SwitchStmtSyntax::getConditionExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::ConditionExpr).get()};
}

TokenSyntax SwitchStmtSyntax::getRightParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::RightParen).get()};
}

TokenSyntax SwitchStmtSyntax::getLeftBrace()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftBrace).get()};
}

SwitchCaseListSyntax SwitchStmtSyntax::getCases()
{
   return SwitchCaseListSyntax{m_root, m_data->getChild(Cursor::Cases).get()};
}

TokenSyntax SwitchStmtSyntax::getRightBrace()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::RightBrace).get()};
}

SwitchStmtSyntax SwitchStmtSyntax::withLabelName(std::optional<TokenSyntax> labelName)
{
   RefCountPtr<RawSyntax> rawLabelName;
   if (labelName.has_value()) {
      rawLabelName = labelName->getRaw();
   } else {
      rawLabelName = nullptr;
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawLabelName, Cursor::LabelName);
}

SwitchStmtSyntax SwitchStmtSyntax::withLabelColon(std::optional<TokenSyntax> labelColon)
{
   RefCountPtr<RawSyntax> rawLabelColon;
   if (labelColon.has_value()) {
      rawLabelColon = labelColon->getRaw();
   } else {
      rawLabelColon = nullptr;
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawLabelColon, Cursor::LabelColon);
}

SwitchStmtSyntax SwitchStmtSyntax::withSwitchKeyword(std::optional<TokenSyntax> switchKeyword)
{
   RefCountPtr<RawSyntax> rawSwitchKeyword;
   if (switchKeyword.has_value()) {
      rawSwitchKeyword = switchKeyword->getRaw();
   } else {
      rawSwitchKeyword = RawSyntax::missing(TokenKindType::T_SWITCH,
                                            OwnedString::makeUnowned(get_token_text(TokenKindType::T_SWITCH)));
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawSwitchKeyword, Cursor::SwitchKeyword);
}

SwitchStmtSyntax SwitchStmtSyntax::withLeftParen(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> rawLeftParen;
   if (leftParen.has_value()) {
      rawLeftParen = leftParen->getRaw();
   } else {
      rawLeftParen = RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN)));
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawLeftParen, Cursor::LeftParen);
}

SwitchStmtSyntax SwitchStmtSyntax::withConditionExpr(std::optional<ExprSyntax> conditionExpr)
{
   RefCountPtr<RawSyntax> rawConditionExpr;
   if (conditionExpr.has_value()) {
      rawConditionExpr = conditionExpr->getRaw();
   } else {
      rawConditionExpr = RawSyntax::missing(SyntaxKind::UnknownExpr);
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawConditionExpr, Cursor::ConditionExpr);
}

SwitchStmtSyntax SwitchStmtSyntax::withRightParen(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rawRightParen;
   if (rightParen.has_value()) {
      rawRightParen = rightParen->getRaw();
   } else {
      rawRightParen = RawSyntax::missing(TokenKindType::T_RIGHT_PAREN,
                                         OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN)));
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawRightParen, Cursor::RightParen);
}

SwitchStmtSyntax SwitchStmtSyntax::withLeftBrace(std::optional<TokenSyntax> leftBrace)
{
   RefCountPtr<RawSyntax> rawLeftBrace;
   if (leftBrace.has_value()) {
      rawLeftBrace = leftBrace->getRaw();
   } else {
      rawLeftBrace = RawSyntax::missing(TokenKindType::T_LEFT_BRACE,
                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_BRACE)));
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawLeftBrace, Cursor::LeftBrace);
}

SwitchStmtSyntax SwitchStmtSyntax::withCases(std::optional<SwitchCaseListSyntax> cases)
{
   RefCountPtr<RawSyntax> rawCases;
   if (cases.has_value()) {
      rawCases = cases->getRaw();
   } else {
      rawCases = RawSyntax::missing(SyntaxKind::SwitchCaseList);
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawCases, Cursor::Cases);
}

SwitchStmtSyntax SwitchStmtSyntax::withRightBrace(std::optional<TokenSyntax> rightBrace)
{
   RefCountPtr<RawSyntax> rawRightBrace;
   if (rightBrace.has_value()) {
      rawRightBrace = rightBrace->getRaw();
   } else {
      rawRightBrace = RawSyntax::missing(TokenKindType::T_RIGHT_BRACE,
                                         OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_BRACE)));
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawRightBrace, Cursor::RightBrace);
}

SwitchStmtSyntax SwitchStmtSyntax::addCase(SwitchCaseSyntax switchCase)
{
   RefCountPtr<RawSyntax> cases = getRaw()->getChild(Cursor::Cases);
   if (cases) {
      cases = cases->append(switchCase.getRaw());
   } else {
      cases = RawSyntax::make(SyntaxKind::SwitchCaseList, {switchCase.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<SwitchStmtSyntax>(cases, Cursor::Cases);
}

///
/// DeferStmtSyntax
///
void DeferStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == DeferStmtSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax DeferStmtSyntax::getDeferKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::DeferKeyword).get()};
}

CodeBlockSyntax DeferStmtSyntax::getBody()
{
   return CodeBlockSyntax{m_root, m_data->getChild(Cursor::Body).get()};
}

DeferStmtSyntax DeferStmtSyntax::withDeferKeyword(std::optional<TokenSyntax> deferKeyword)
{
   RefCountPtr<RawSyntax> rawDeferKeyword;
   if (deferKeyword.has_value()) {
      rawDeferKeyword = deferKeyword->getRaw();
   } else {
      rawDeferKeyword = RawSyntax::missing(TokenKindType::T_DEFER,
                                           OwnedString::makeUnowned(get_token_text(TokenKindType::T_DEFER)));
   }
   return m_data->replaceChild<DeferStmtSyntax>(rawDeferKeyword, Cursor::DeferKeyword);
}

DeferStmtSyntax DeferStmtSyntax::withBody(std::optional<CodeBlockSyntax> body)
{
   RefCountPtr<RawSyntax> rawBody;
   if (body.has_value()) {
      rawBody = body->getRaw();
   } else {
      rawBody = RawSyntax::missing(SyntaxKind::CodeBlock);
   }
   return m_data->replaceChild<DeferStmtSyntax>(rawBody, Cursor::Body);
}

///
/// ExpressionStmtSyntax
///
void ExpressionStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ExpressionStmtSyntax::CHILDREN_COUNT);
#endif
}

ExprSyntax ExpressionStmtSyntax::getExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Expr).get()};
}

ExpressionStmtSyntax ExpressionStmtSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> rawExpr;
   if (expr.has_value()) {
      rawExpr = expr->getRaw();
   } else {
      rawExpr = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<ExpressionStmtSyntax>(rawExpr, Cursor::Expr);
}

///
/// ThrowStmtSyntax
///
void ThrowStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ThrowStmtSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax ThrowStmtSyntax::getThrowKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::ThrowKeyword).get()};
}

ExprSyntax ThrowStmtSyntax::getExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Expr).get()};
}

ThrowStmtSyntax ThrowStmtSyntax::withThrowKeyword(std::optional<TokenSyntax> throwKeyword)
{
   RefCountPtr<RawSyntax> rawThrowKeyword;
   if (throwKeyword.has_value()) {
      rawThrowKeyword = throwKeyword->getRaw();
   } else {
      rawThrowKeyword = RawSyntax::missing(TokenKindType::T_THROW,
                                           OwnedString::makeUnowned(get_token_text(TokenKindType::T_THROW)));
   }
   return m_data->replaceChild<ThrowStmtSyntax>(rawThrowKeyword, TokenKindType::T_THROW);
}

ThrowStmtSyntax ThrowStmtSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> rawExpr;
   if (expr.has_value()) {
      rawExpr = expr->getRaw();
   } else {
      rawExpr = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<ThrowStmtSyntax>(rawExpr, Cursor::Expr);
}

///
/// ReturnStmtSyntax
///
void ReturnStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ReturnStmtSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax ReturnStmtSyntax::getReturnKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::ReturnKeyword).get()};
}

ExprSyntax ReturnStmtSyntax::getExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Expr).get()};
}

ReturnStmtSyntax ReturnStmtSyntax::withReturnKeyword(std::optional<TokenSyntax> returnKeyword)
{
   RefCountPtr<RawSyntax> raw;
   if (returnKeyword.has_value()) {
      raw = returnKeyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_RETURN,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_RETURN)));
   }
   return m_data->replaceChild<ReturnStmtSyntax>(raw, Cursor::ReturnKeyword);
}

ReturnStmtSyntax ReturnStmtSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> rawExpr;
   if (expr.has_value()) {
      rawExpr = expr->getRaw();
   } else {
      rawExpr = nullptr;
   }
   return m_data->replaceChild<ReturnStmtSyntax>(rawExpr, Cursor::Expr);
}

} // polar::syntax
