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
   RefCountPtr<RawSyntax> raw = getRaw();
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
   RefCountPtr<SyntaxData> childData = m_data->getChild(Cursor::TrailingComma);
   if (!childData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, childData.get()};
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
   RefCountPtr<RawSyntax> raw = getRaw();
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
   RefCountPtr<SyntaxData> childData = m_data->getChild(Cursor::LNumberToken);
   if (!childData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, childData.get()};
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

void WhileStmtSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == WhileStmtSyntax::CHILDREN_COUNT);
}

std::optional<TokenSyntax> WhileStmtSyntax::getLabelName()
{
   RefCountPtr<SyntaxData> childData = m_data->getChild(Cursor::LabelName);
   if (!childData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, childData.get()};
}

std::optional<TokenSyntax> WhileStmtSyntax::getLabelColon()
{
   RefCountPtr<SyntaxData> childData = m_data->getChild(Cursor::LabelColon);
   if (!childData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, childData.get()};
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

void ThrowStmtSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = getRaw();
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
   RefCountPtr<RawSyntax> raw = getRaw();
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
