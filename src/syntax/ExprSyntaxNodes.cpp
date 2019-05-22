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

#include "polarphp/syntax/syntaxnode/ExprSyntaxNodes.h"

namespace polar::syntax {

void NullExprSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == NullExprSyntax::CHILDREN_COUNT);
}

TokenSyntax NullExprSyntax::getNullKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::NulllKeyword).get()};
}

NullExprSyntax NullExprSyntax::withNullKeyword(std::optional<TokenSyntax> keyword)
{
   RefCountPtr<RawSyntax> raw;
   if (keyword.has_value()) {
      raw = keyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_NULL,
                               OwnedString::makeUnowned((get_token_text(TokenKindType::T_NULL))));
   }
   return m_data->replaceChild<NullExprSyntax>(raw, Cursor::NulllKeyword);
}

void ClassRefParentExprSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ClassRefParentExprSyntax::CHILDREN_COUNT);
}

TokenSyntax ClassRefParentExprSyntax::getParentKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::ParentKeyword).get()};
}

ClassRefParentExprSyntax ClassRefParentExprSyntax::withParentKeyword(std::optional<TokenSyntax> parentKeyword)
{
   RefCountPtr<RawSyntax> raw;
   if (parentKeyword.has_value()) {
      raw = parentKeyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_CLASS_REF_PARENT,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_PARENT)));
   }
   return m_data->replaceChild<ClassRefParentExprSyntax>(raw, Cursor::ParentKeyword);
}

void ClassRefSelfExprSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ClassRefSelfExprSyntax::CHILDREN_COUNT);
}

TokenSyntax ClassRefSelfExprSyntax::getSelfKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::SelfKeyword).get()};
}

ClassRefSelfExprSyntax ClassRefSelfExprSyntax::withSelfKeyword(std::optional<TokenSyntax> selfKeyword)
{
   RefCountPtr<RawSyntax> raw;
   if (selfKeyword.has_value()) {
      raw = selfKeyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_CLASS_REF_SELF,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_SELF)));
   }
   return m_data->replaceChild<ClassRefSelfExprSyntax>(raw, Cursor::SelfKeyword);
}

void ClassRefStaticExprSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ClassRefStaticExprSyntax::CHILDREN_COUNT);
}

TokenSyntax ClassRefStaticExprSyntax::getStaticKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::StaticKeyword).get()};
}

ClassRefStaticExprSyntax ClassRefStaticExprSyntax::withStaticKeyword(std::optional<TokenSyntax> staticKeyword)
{
   RefCountPtr<RawSyntax> raw;
   if (staticKeyword.has_value()) {
      raw = staticKeyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_CLASS_REF_STATIC,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_STATIC)));
   }
   return m_data->replaceChild<ClassRefStaticExprSyntax>(raw, Cursor::StaticKeyword);
}

void IntegerLiteralExprSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == IntegerLiteralExprSyntax::CHILDREN_COUNT);
}

TokenSyntax IntegerLiteralExprSyntax::getDigits()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Digits).get()};
}

IntegerLiteralExprSyntax IntegerLiteralExprSyntax::withDigits(std::optional<TokenSyntax> digits)
{
   RefCountPtr<RawSyntax> raw;
   if (digits.has_value()) {
      raw = digits->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_LNUMBER,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_LNUMBER)));
   }
   return m_data->replaceChild<IntegerLiteralExprSyntax>(raw, Cursor::Digits);
}

void FloatLiteralExprSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == FloatLiteralExprSyntax::CHILDREN_COUNT);
}

TokenSyntax FloatLiteralExprSyntax::getFloatDigits()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::FloatDigits).get()};
}

FloatLiteralExprSyntax FloatLiteralExprSyntax::withFloatDigits(std::optional<TokenSyntax> digits)
{
   RefCountPtr<RawSyntax> raw;
   if (digits.has_value()) {
      raw = digits->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_DNUMBER,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_DNUMBER)));
   }
   return m_data->replaceChild<FloatLiteralExprSyntax>(raw, Cursor::FloatDigits);
}

void StringLiteralExprSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == StringLiteralExprSyntax::CHILDREN_COUNT);
}

TokenSyntax StringLiteralExprSyntax::getString()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::String).get()};
}

StringLiteralExprSyntax StringLiteralExprSyntax::withString(std::optional<TokenSyntax> str)
{
   RefCountPtr<RawSyntax> raw;
   if (str.has_value()) {
      raw = str->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_STRING,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_STRING)));
   }
   return m_data->replaceChild<StringLiteralExprSyntax>(raw, Cursor::String);
}

#ifdef POLAR_DEBUG_BUILD
const std::map<SyntaxChildrenCountType, std::set<TokenKindType>> BooleanLiteralExprSyntax::CHILD_TOKEN_CHOICES
{
   {
      BooleanLiteralExprSyntax::Boolean, {
         TokenKindType::T_FALSE,
               TokenKindType::T_TRUE
      }
   }
};
#endif

void BooleanLiteralExprSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   ///
   /// check Boolean token choice
   ///
   syntax_assert_child_token(raw, Boolean, CHILD_TOKEN_CHOICES.at(Boolean));
   assert(raw->getLayout().size() == BooleanLiteralExprSyntax::CHILDREN_COUNT);
}

TokenSyntax BooleanLiteralExprSyntax::getBooleanValue()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Boolean).get()};
}

BooleanLiteralExprSyntax BooleanLiteralExprSyntax::withBooleanValue(std::optional<TokenSyntax> booleanValue)
{
   RefCountPtr<RawSyntax> raw;
   if (booleanValue.has_value()) {
      raw = booleanValue->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_TRUE,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_TRUE)));
   }
   return m_data->replaceChild<BooleanLiteralExprSyntax>(raw, Cursor::Boolean);
}

void TernaryExprSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == TernaryExprSyntax::CHILDREN_COUNT);
}

ExprSyntax TernaryExprSyntax::getConditionExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::ConditionExpr).get()};
}

TokenSyntax TernaryExprSyntax::getQuestionMark()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::QuestionMark).get()};
}

ExprSyntax TernaryExprSyntax::getFirstChoice()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::FirstChoice).get()};
}

TokenSyntax TernaryExprSyntax::getColonMark()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::ColonMark).get()};
}

ExprSyntax TernaryExprSyntax::getSecondChoice()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::SecondChoice).get()};
}

TernaryExprSyntax TernaryExprSyntax::withConditionExpr(std::optional<ExprSyntax> conditionExpr)
{
   RefCountPtr<RawSyntax> raw;
   if (conditionExpr.has_value()) {
      raw = conditionExpr->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<TernaryExprSyntax>(raw, Cursor::ConditionExpr);
}

TernaryExprSyntax TernaryExprSyntax::withQuestionMark(std::optional<TokenSyntax> questionMark)
{
   RefCountPtr<RawSyntax> raw;
   if (questionMark.has_value()) {
      raw = questionMark->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_INFIX_QUESTION_MARK,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_INFIX_QUESTION_MARK)));
   }
   return m_data->replaceChild<TernaryExprSyntax>(raw, Cursor::QuestionMark);
}

TernaryExprSyntax TernaryExprSyntax::withFirstChoice(std::optional<ExprSyntax> firstChoice)
{
   RefCountPtr<RawSyntax> raw;
   if (firstChoice.has_value()) {
      raw = firstChoice->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<TernaryExprSyntax>(raw, Cursor::FirstChoice);
}

TernaryExprSyntax TernaryExprSyntax::withColonMark(std::optional<TokenSyntax> colonMark)
{
   RefCountPtr<RawSyntax> raw;
   if (colonMark.has_value()) {
      raw = colonMark->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_COLON,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON)));
   }
   return m_data->replaceChild<TernaryExprSyntax>(raw, Cursor::ColonMark);
}

TernaryExprSyntax TernaryExprSyntax::withSecondChoice(std::optional<ExprSyntax> secondChoice)
{
   RefCountPtr<RawSyntax> raw;
   if (secondChoice.has_value()) {
      raw = secondChoice->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<TernaryExprSyntax>(raw, Cursor::SecondChoice);
}

void AssignmentExprSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == AssignmentExprSyntax::CHILDREN_COUNT);
}

TokenSyntax AssignmentExprSyntax::getAssignToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::AssignToken).get()};
}

AssignmentExprSyntax AssignmentExprSyntax::withAssignToken(std::optional<TokenSyntax> assignToken)
{
   RefCountPtr<RawSyntax> raw;
   if (assignToken.has_value()) {
      raw = assignToken->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_EQUAL,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_EQUAL)));
   }
   return m_data->replaceChild<AssignmentExprSyntax>(raw, Cursor::AssignToken);
}

void SequenceExprSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SequenceExprSyntax::CHILDREN_COUNT);
}

ExprListSyntax SequenceExprSyntax::getElements()
{
   return ExprListSyntax{m_root, m_data->getChild(Cursor::Elements).get()};
}

SequenceExprSyntax SequenceExprSyntax::withElements(std::optional<ExprListSyntax> elements)
{
   RefCountPtr<RawSyntax> raw;
   if (elements.has_value()) {
      raw = elements->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::ExprList);
   }
   return m_data->replaceChild<SequenceExprSyntax>(raw, Cursor::Elements);
}

SequenceExprSyntax SequenceExprSyntax::addElement(ExprSyntax expr)
{
   RefCountPtr<RawSyntax> raw = getRaw()->getChild(Cursor::Elements);
   if (raw) {
      raw->append(expr.getRaw());
   } else {
      raw = RawSyntax::make(SyntaxKind::ExprList, {expr.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<SequenceExprSyntax>(raw, Cursor::Elements);
}

void PrefixOperatorExprSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == PrefixOperatorExprSyntax::CHILDREN_COUNT);
}

std::optional<TokenSyntax> PrefixOperatorExprSyntax::getOperatorToken()
{
   auto childData = m_data->getChild(Cursor::OperatorToken);
   if (!childData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, childData.get()};
}

ExprSyntax PrefixOperatorExprSyntax::getExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Expr).get()};
}

PrefixOperatorExprSyntax PrefixOperatorExprSyntax::withOperatorToken(std::optional<TokenSyntax> operatorToken)
{
   RefCountPtr<RawSyntax> raw;
   if (operatorToken.has_value()) {
      raw = operatorToken->getRaw();
   } else {
      raw = nullptr;
   }
   return m_data->replaceChild<PrefixOperatorExprSyntax>(raw, Cursor::OperatorToken);
}

PrefixOperatorExprSyntax PrefixOperatorExprSyntax::withExpr(std::optional<TokenSyntax> expr)
{
   RefCountPtr<RawSyntax> raw;
   if (expr.has_value()) {
      raw = expr->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<PrefixOperatorExprSyntax>(raw, Cursor::Expr);
}

void PostfixOperatorExprSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == PostfixOperatorExprSyntax::CHILDREN_COUNT);
}

ExprSyntax PostfixOperatorExprSyntax::getExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Expr).get()};
}

TokenSyntax PostfixOperatorExprSyntax::getOperatorToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::OperatorToken).get()};
}

PostfixOperatorExprSyntax PostfixOperatorExprSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> raw;
   if (expr.has_value()) {
      raw = expr->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<PostfixOperatorExprSyntax>(raw, Cursor::Expr);
}

PostfixOperatorExprSyntax PostfixOperatorExprSyntax::withOperatorToken(std::optional<TokenSyntax> operatorToken)
{
   RefCountPtr<RawSyntax> raw;
   if (operatorToken.has_value()) {
      raw = operatorToken->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_POSTFIX_OPERATOR,
                               OwnedString::makeUnowned(""));
   }
   return m_data->replaceChild<PostfixOperatorExprSyntax>(raw, Cursor::OperatorToken);
}

void BinaryOperatorExprSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == BinaryOperatorExprSyntax::CHILDREN_COUNT);
}

TokenSyntax BinaryOperatorExprSyntax::getOperatorToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::OperatorToken).get()};
}

BinaryOperatorExprSyntax BinaryOperatorExprSyntax::withOperatorToken(std::optional<TokenSyntax> operatorToken)
{
   RefCountPtr<RawSyntax> raw;
   if (operatorToken.has_value()) {
      raw = operatorToken->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_BINARY_OPERATOR,
                               OwnedString::makeUnowned(""));
   }
   return m_data->replaceChild<BinaryOperatorExprSyntax>(raw, TokenKindType::T_BINARY_OPERATOR);
}

} // polar::syntax
