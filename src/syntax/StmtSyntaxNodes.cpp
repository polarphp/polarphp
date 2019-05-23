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
/// current only support expr condition
///
const std::map<SyntaxChildrenCountType, std::set<SyntaxKind>> ConditionElementSyntax::CHILD_NODE_CHOICES
{
   {
      ConditionElementSyntax::Cursor::Condition, {
         SyntaxKind::Expr
      }
   }
};

void ConditionElementSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ConditionElementSyntax::CHILDREN_COUNT);
   // check condition child node choices
   if (const RefCountPtr<RawSyntax> &condition = raw->getChild(Cursor::Condition)) {
      assert(condition->kindOf(SyntaxKind::Expr));
   }
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
   RefCountPtr<RawSyntax> raw;
   if (condition.has_value()) {
      raw = condition->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<ConditionElementSyntax>(raw, Cursor::Condition);
}

ConditionElementSyntax ConditionElementSyntax::withTrailingComma(std::optional<TokenSyntax> trailingComma)
{
   RefCountPtr<RawSyntax> raw;
   if (trailingComma.has_value()) {
      raw = trailingComma->getRaw();
   } else {
      raw = nullptr;
   }
   return m_data->replaceChild<ConditionElementSyntax>(raw, Cursor::TrailingComma);
}

void ContinueStmtSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ContinueStmtSyntax::CHILDREN_COUNT);
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
   RefCountPtr<RawSyntax> raw;
   if (continueKeyword.has_value()) {
      raw = continueKeyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_CONTINUE,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_CONTINUE)));
   }
   return m_data->replaceChild<ContinueStmtSyntax>(raw, Cursor::ContinueKeyword);
}

ContinueStmtSyntax ContinueStmtSyntax::withLNumberToken(std::optional<TokenSyntax> numberToken)
{
   RefCountPtr<RawSyntax> raw;
   if (numberToken.has_value()) {
      raw = numberToken->getRaw();
   } else {
      raw = nullptr;
   }
   return m_data->replaceChild<ContinueStmtSyntax>(raw, Cursor::LNumberToken);
}

void BreakStmtSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == BreakStmtSyntax::CHILDREN_COUNT);
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
   RefCountPtr<RawSyntax> raw;
   if (breakKeyword.has_value()) {
      raw = breakKeyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_BREAK,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_BREAK)));
   }
   return m_data->replaceChild<BreakStmtSyntax>(raw, Cursor::BreakKeyword);
}

BreakStmtSyntax BreakStmtSyntax::withLNumberToken(std::optional<TokenSyntax> numberToken)
{
   RefCountPtr<RawSyntax> raw;
   if (numberToken.has_value()) {
      raw = numberToken->getRaw();
   } else {
      raw = nullptr;
   }
   return m_data->replaceChild<BreakStmtSyntax>(raw, Cursor::LNumberToken);
}

void FallthroughStmtSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if(isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == FallthroughStmtSyntax::CHILDREN_COUNT);
}

TokenSyntax FallthroughStmtSyntax::getFallthroughKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::FallthroughKeyword).get()};
}

FallthroughStmtSyntax FallthroughStmtSyntax::withFallthroughStmtSyntax(std::optional<TokenSyntax> fallthroughKeyword)
{
   RefCountPtr<RawSyntax> raw;
   if (fallthroughKeyword.has_value()) {
      raw = fallthroughKeyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_FALLTHROUGH,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_FALLTHROUGH)));
   }
   return m_data->replaceChild<FallthroughStmtSyntax>(raw, TokenKindType::T_FALLTHROUGH);
}

void WhileStmtSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == WhileStmtSyntax::CHILDREN_COUNT);
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

ConditionElementListSyntax WhileStmtSyntax::getConditions()
{
   return ConditionElementListSyntax{m_root, m_data->getChild(Cursor::Conditions).get()};
}

CodeBlockSyntax WhileStmtSyntax::getBody()
{
   return CodeBlockSyntax{m_root, m_data->getChild(Cursor::Body).get()};
}

WhileStmtSyntax WhileStmtSyntax::withLabelName(std::optional<TokenSyntax> labelName)
{
   RefCountPtr<RawSyntax> raw;
   if (labelName.has_value()) {
      raw = labelName->getRaw();
   } else {
      raw = nullptr;
   }
   return m_data->replaceChild<WhileStmtSyntax>(raw, Cursor::LabelName);
}

WhileStmtSyntax WhileStmtSyntax::withLabelColon(std::optional<TokenSyntax> labelColon)
{
   RefCountPtr<RawSyntax> raw;
   if (labelColon.has_value()) {
      raw = labelColon->getRaw();
   } else {
      raw = nullptr;
   }
   return m_data->replaceChild<WhileStmtSyntax>(raw, Cursor::LabelColon);
}

WhileStmtSyntax WhileStmtSyntax::withWhileKeyword(std::optional<TokenSyntax> whileKeyword)
{
   RefCountPtr<RawSyntax> raw;
   if (whileKeyword.has_value()) {
      raw = whileKeyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_WHILE,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_WHILE)));
   }
   return m_data->replaceChild<WhileStmtSyntax>(raw, Cursor::WhileKeyword);
}

WhileStmtSyntax WhileStmtSyntax::withConditions(std::optional<ConditionElementListSyntax> conditions)
{
   RefCountPtr<RawSyntax> raw;
   if (conditions.has_value()) {
      raw = conditions->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::ConditionElementList);
   }
   return m_data->replaceChild<WhileStmtSyntax>(raw, Cursor::Conditions);
}

WhileStmtSyntax WhileStmtSyntax::withBody(std::optional<CodeBlockSyntax> body)
{
   RefCountPtr<RawSyntax> raw;
   if (body.has_value()) {
      raw = body->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::CodeBlock);
   }
   return m_data->replaceChild<WhileStmtSyntax>(raw, Cursor::Body);
}

WhileStmtSyntax WhileStmtSyntax::addCondition(ConditionElementSyntax condition)
{
   RefCountPtr<RawSyntax> raw = getRaw()->getChild(Cursor::Conditions);
   if (raw) {
      raw->append(condition.getRaw());
   } else {
      raw = RawSyntax::make(SyntaxKind::ConditionElementList, {condition.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<WhileStmtSyntax>(raw, Cursor::Conditions);
}

TokenSyntax SwitchDefaultLabelSyntax::getDefaultKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::DefaultKeyword).get()};
}

TokenSyntax SwitchDefaultLabelSyntax::getColon()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Colon).get()};
}

void SwitchDefaultLabelSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SwitchDefaultLabelSyntax::CHILDREN_COUNT);
}

SwitchDefaultLabelSyntax SwitchDefaultLabelSyntax::withDefaultKeyword(std::optional<TokenSyntax> defaultKeyword)
{
   RefCountPtr<RawSyntax> raw;
   if (defaultKeyword.has_value()) {
      raw = defaultKeyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_DEFAULT,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_DEFAULT)));
   }
   return m_data->replaceChild<SwitchDefaultLabelSyntax>(raw, Cursor::DefaultKeyword);
}

SwitchDefaultLabelSyntax SwitchDefaultLabelSyntax::withColon(std::optional<TokenSyntax> colon)
{
   RefCountPtr<RawSyntax> raw;
   if (colon.has_value()) {
      raw = colon->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_COLON,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON)));
   }
   return m_data->replaceChild<SwitchDefaultLabelSyntax>(raw, Cursor::Colon);
}

void SwitchCaseLabelSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SwitchCaseLabelSyntax::CHILDREN_COUNT);
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
   RefCountPtr<RawSyntax> raw;
   if (caseKeyword.has_value()) {
      raw = caseKeyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_CASE,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_CASE)));
   }
   return m_data->replaceChild<SwitchCaseLabelSyntax>(raw, Cursor::CaseKeyword);
}

SwitchCaseLabelSyntax SwitchCaseLabelSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> raw;
   if (expr.has_value()) {
      raw = expr->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<SwitchCaseLabelSyntax>(raw, Cursor::Expr);
}

SwitchCaseLabelSyntax SwitchCaseLabelSyntax::withColon(std::optional<TokenSyntax> colon)
{
   RefCountPtr<RawSyntax> raw;
   if (colon.has_value()) {
      raw = colon->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_COLON,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON)));
   }
   return m_data->replaceChild<SwitchCaseLabelSyntax>(raw, Cursor::Colon);
}

#ifdef POLAR_DEBUG_BUILD
const std::map<SyntaxChildrenCountType, std::set<SyntaxKind>> SwitchCaseSyntax::CHILD_NODE_CHOICES
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
   RefCountPtr<RawSyntax> raw;
   if (label.has_value()) {
      raw = label->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::SwitchDefaultLabel);
   }
   return m_data->replaceChild<SwitchCaseSyntax>(raw, Cursor::Label);
}

SwitchCaseSyntax SwitchCaseSyntax::withStatements(std::optional<CodeBlockItemListSyntax> statements)
{
   RefCountPtr<RawSyntax> raw;
   if (statements.has_value()) {
      raw = statements->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::CodeBlockItemList);
   }
   return m_data->replaceChild<SwitchCaseSyntax>(raw, Cursor::Statements);
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

void SwitchStmtSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SwitchStmtSyntax::CHILDREN_COUNT);
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

}

SwitchStmtSyntax SwitchStmtSyntax::withSwitchKeyword(std::optional<TokenSyntax> switchKeyword)
{

}

SwitchStmtSyntax SwitchStmtSyntax::withLeftParen(std::optional<TokenSyntax> leftParen)
{

}

SwitchStmtSyntax SwitchStmtSyntax::withConditionExpr(std::optional<ExprSyntax> conditionExpr)
{

}

SwitchStmtSyntax SwitchStmtSyntax::withRightParen(std::optional<TokenSyntax> rightParen)
{

}

SwitchStmtSyntax SwitchStmtSyntax::withLeftBrace(std::optional<TokenSyntax> leftBrace)
{

}

SwitchStmtSyntax SwitchStmtSyntax::withCases(std::optional<SwitchCaseListSyntax> cases)
{

}

SwitchStmtSyntax SwitchStmtSyntax::withRightBrace(std::optional<TokenSyntax> rightBrace)
{

}

SwitchStmtSyntax SwitchStmtSyntax::addCase(SwitchCaseSyntax switchCase)
{

}

void DeferStmtSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == DeferStmtSyntax::CHILDREN_COUNT);
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
   RefCountPtr<RawSyntax> raw;
   if (deferKeyword.has_value()) {
      raw = deferKeyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_DEFER,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_DEFER)));
   }
   return m_data->replaceChild<DeferStmtSyntax>(raw, Cursor::DeferKeyword);
}

DeferStmtSyntax DeferStmtSyntax::withBody(std::optional<CodeBlockSyntax> body)
{
   RefCountPtr<RawSyntax> raw;
   if (body.has_value()) {
      raw = body->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::CodeBlock);
   }
   return m_data->replaceChild<DeferStmtSyntax>(raw, Cursor::Body);
}

void ExpressionStmtSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ExpressionStmtSyntax::CHILDREN_COUNT);
}

ExprSyntax ExpressionStmtSyntax::getExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Expr).get()};
}

ExpressionStmtSyntax ExpressionStmtSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> raw;
   if (expr.has_value()) {
      raw = expr->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<ExpressionStmtSyntax>(raw, Cursor::Expr);
}

void ThrowStmtSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ThrowStmtSyntax::CHILDREN_COUNT);
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
   RefCountPtr<RawSyntax> raw;
   if (throwKeyword.has_value()) {
      raw = throwKeyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_THROW,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_THROW)));
   }
   return m_data->replaceChild<ThrowStmtSyntax>(raw, TokenKindType::T_THROW);
}

ThrowStmtSyntax ThrowStmtSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> raw;
   if (expr.has_value()) {
      raw = expr->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<ThrowStmtSyntax>(raw, Cursor::Expr);
}

void ReturnStmtSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ReturnStmtSyntax::CHILDREN_COUNT);
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
   RefCountPtr<RawSyntax> raw;
   if (expr.has_value()) {
      raw = expr->getRaw();
   } else {
      raw = nullptr;
   }
   return m_data->replaceChild<ReturnStmtSyntax>(raw, Cursor::Expr);
}

} // polar::syntax
