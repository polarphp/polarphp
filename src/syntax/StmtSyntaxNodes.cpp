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
