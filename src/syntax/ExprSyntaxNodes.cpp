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
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == NullExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax NullExprSyntax::getNullKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::NullKeyword).get()};
}

NullExprSyntax NullExprSyntax::withNullKeyword(std::optional<TokenSyntax> keyword)
{
   RefCountPtr<RawSyntax> rawKeyword;
   if (keyword.has_value()) {
      rawKeyword = keyword->getRaw();
   } else {
      rawKeyword = RawSyntax::missing(TokenKindType::T_NULL,
                                      OwnedString::makeUnowned((get_token_text(TokenKindType::T_NULL))));
   }
   return m_data->replaceChild<NullExprSyntax>(rawKeyword, Cursor::NullKeyword);
}

void ClassRefParentExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ClassRefParentExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax ClassRefParentExprSyntax::getParentKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::ParentKeyword).get()};
}

ClassRefParentExprSyntax ClassRefParentExprSyntax::withParentKeyword(std::optional<TokenSyntax> parentKeyword)
{
   RefCountPtr<RawSyntax> rawParentKeyword;
   if (parentKeyword.has_value()) {
      rawParentKeyword = parentKeyword->getRaw();
   } else {
      rawParentKeyword = RawSyntax::missing(TokenKindType::T_CLASS_REF_PARENT,
                                            OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_PARENT)));
   }
   return m_data->replaceChild<ClassRefParentExprSyntax>(rawParentKeyword, Cursor::ParentKeyword);
}

void ClassRefSelfExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ClassRefSelfExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax ClassRefSelfExprSyntax::getSelfKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::SelfKeyword).get()};
}

ClassRefSelfExprSyntax ClassRefSelfExprSyntax::withSelfKeyword(std::optional<TokenSyntax> selfKeyword)
{
   RefCountPtr<RawSyntax> rawSelfKeyword;
   if (selfKeyword.has_value()) {
      rawSelfKeyword = selfKeyword->getRaw();
   } else {
      rawSelfKeyword = RawSyntax::missing(TokenKindType::T_CLASS_REF_SELF,
                                          OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_SELF)));
   }
   return m_data->replaceChild<ClassRefSelfExprSyntax>(rawSelfKeyword, Cursor::SelfKeyword);
}

void ClassRefStaticExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ClassRefStaticExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax ClassRefStaticExprSyntax::getStaticKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::StaticKeyword).get()};
}

ClassRefStaticExprSyntax ClassRefStaticExprSyntax::withStaticKeyword(std::optional<TokenSyntax> staticKeyword)
{
   RefCountPtr<RawSyntax> rawStaticKeyword;
   if (staticKeyword.has_value()) {
      rawStaticKeyword = staticKeyword->getRaw();
   } else {
      rawStaticKeyword = RawSyntax::missing(TokenKindType::T_CLASS_REF_STATIC,
                                            OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_STATIC)));
   }
   return m_data->replaceChild<ClassRefStaticExprSyntax>(rawStaticKeyword, Cursor::StaticKeyword);
}

void IntegerLiteralExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == IntegerLiteralExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax IntegerLiteralExprSyntax::getDigits()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Digits).get()};
}

IntegerLiteralExprSyntax IntegerLiteralExprSyntax::withDigits(std::optional<TokenSyntax> digits)
{
   RefCountPtr<RawSyntax> rawDigits;
   if (digits.has_value()) {
      rawDigits = digits->getRaw();
   } else {
      rawDigits = RawSyntax::missing(TokenKindType::T_LNUMBER,
                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_LNUMBER)));
   }
   return m_data->replaceChild<IntegerLiteralExprSyntax>(rawDigits, Cursor::Digits);
}

void FloatLiteralExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == FloatLiteralExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax FloatLiteralExprSyntax::getFloatDigits()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::FloatDigits).get()};
}

FloatLiteralExprSyntax FloatLiteralExprSyntax::withFloatDigits(std::optional<TokenSyntax> digits)
{
   RefCountPtr<RawSyntax> rawDigits;
   if (digits.has_value()) {
      rawDigits = digits->getRaw();
   } else {
      rawDigits = RawSyntax::missing(TokenKindType::T_DNUMBER,
                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_DNUMBER)));
   }
   return m_data->replaceChild<FloatLiteralExprSyntax>(rawDigits, Cursor::FloatDigits);
}

void StringLiteralExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == StringLiteralExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax StringLiteralExprSyntax::getString()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::String).get()};
}

StringLiteralExprSyntax StringLiteralExprSyntax::withString(std::optional<TokenSyntax> str)
{
   RefCountPtr<RawSyntax> rawStr;
   if (str.has_value()) {
      rawStr = str->getRaw();
   } else {
      rawStr = RawSyntax::missing(TokenKindType::T_IDENTIFIER_STRING,
                                  OwnedString::makeUnowned(get_token_text(TokenKindType::T_IDENTIFIER_STRING)));
   }
   return m_data->replaceChild<StringLiteralExprSyntax>(rawStr, Cursor::String);
}

#ifdef POLAR_DEBUG_BUILD
const TokenChoicesType BooleanLiteralExprSyntax::CHILD_TOKEN_CHOICES
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
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   ///
   /// check Boolean token choice
   ///
   syntax_assert_child_token(raw, Boolean, CHILD_TOKEN_CHOICES.at(Boolean));
   assert(raw->getLayout().size() == BooleanLiteralExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax BooleanLiteralExprSyntax::getBooleanValue()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Boolean).get()};
}

BooleanLiteralExprSyntax BooleanLiteralExprSyntax::withBooleanValue(std::optional<TokenSyntax> booleanValue)
{
   RefCountPtr<RawSyntax> rawBooleanValue;
   if (booleanValue.has_value()) {
      rawBooleanValue = booleanValue->getRaw();
   } else {
      rawBooleanValue = RawSyntax::missing(TokenKindType::T_TRUE,
                                           OwnedString::makeUnowned(get_token_text(TokenKindType::T_TRUE)));
   }
   return m_data->replaceChild<BooleanLiteralExprSyntax>(rawBooleanValue, Cursor::Boolean);
}

void TernaryExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == TernaryExprSyntax::CHILDREN_COUNT);
#endif
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
   RefCountPtr<RawSyntax> rawConditionExpr;
   if (conditionExpr.has_value()) {
      rawConditionExpr = conditionExpr->getRaw();
   } else {
      rawConditionExpr = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<TernaryExprSyntax>(rawConditionExpr, Cursor::ConditionExpr);
}

TernaryExprSyntax TernaryExprSyntax::withQuestionMark(std::optional<TokenSyntax> questionMark)
{
   RefCountPtr<RawSyntax> rawQuestionMark;
   if (questionMark.has_value()) {
      rawQuestionMark = questionMark->getRaw();
   } else {
      rawQuestionMark = RawSyntax::missing(TokenKindType::T_QUESTION_MARK,
                                           OwnedString::makeUnowned(get_token_text(TokenKindType::T_QUESTION_MARK)));
   }
   return m_data->replaceChild<TernaryExprSyntax>(rawQuestionMark, Cursor::QuestionMark);
}

TernaryExprSyntax TernaryExprSyntax::withFirstChoice(std::optional<ExprSyntax> firstChoice)
{
   RefCountPtr<RawSyntax> rawFirstChoice;
   if (firstChoice.has_value()) {
      rawFirstChoice = firstChoice->getRaw();
   } else {
      rawFirstChoice = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<TernaryExprSyntax>(rawFirstChoice, Cursor::FirstChoice);
}

TernaryExprSyntax TernaryExprSyntax::withColonMark(std::optional<TokenSyntax> colonMark)
{
   RefCountPtr<RawSyntax> rawColonMark;
   if (colonMark.has_value()) {
      rawColonMark = colonMark->getRaw();
   } else {
      rawColonMark = RawSyntax::missing(TokenKindType::T_COLON,
                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON)));
   }
   return m_data->replaceChild<TernaryExprSyntax>(rawColonMark, Cursor::ColonMark);
}

TernaryExprSyntax TernaryExprSyntax::withSecondChoice(std::optional<ExprSyntax> secondChoice)
{
   RefCountPtr<RawSyntax> rawSecondChoice;
   if (secondChoice.has_value()) {
      rawSecondChoice = secondChoice->getRaw();
   } else {
      rawSecondChoice = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<TernaryExprSyntax>(rawSecondChoice, Cursor::SecondChoice);
}

void AssignmentExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == AssignmentExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax AssignmentExprSyntax::getAssignToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::AssignToken).get()};
}

AssignmentExprSyntax AssignmentExprSyntax::withAssignToken(std::optional<TokenSyntax> assignToken)
{
   RefCountPtr<RawSyntax> rawAssignToken;
   if (assignToken.has_value()) {
      rawAssignToken = assignToken->getRaw();
   } else {
      rawAssignToken = RawSyntax::missing(TokenKindType::T_EQUAL,
                                          OwnedString::makeUnowned(get_token_text(TokenKindType::T_EQUAL)));
   }
   return m_data->replaceChild<AssignmentExprSyntax>(rawAssignToken, Cursor::AssignToken);
}

void SequenceExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SequenceExprSyntax::CHILDREN_COUNT);
#endif
}

ExprListSyntax SequenceExprSyntax::getElements()
{
   return ExprListSyntax{m_root, m_data->getChild(Cursor::Elements).get()};
}

SequenceExprSyntax SequenceExprSyntax::withElements(std::optional<ExprListSyntax> elements)
{
   RefCountPtr<RawSyntax> rawElements;
   if (elements.has_value()) {
      rawElements = elements->getRaw();
   } else {
      rawElements = RawSyntax::missing(SyntaxKind::ExprList);
   }
   return m_data->replaceChild<SequenceExprSyntax>(rawElements, Cursor::Elements);
}

SequenceExprSyntax SequenceExprSyntax::addElement(ExprSyntax expr)
{
   RefCountPtr<RawSyntax> elements = getRaw()->getChild(Cursor::Elements);
   if (elements) {
      elements = elements->append(expr.getRaw());
   } else {
      elements = RawSyntax::make(SyntaxKind::ExprList, {expr.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<SequenceExprSyntax>(elements, Cursor::Elements);
}

void PrefixOperatorExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == PrefixOperatorExprSyntax::CHILDREN_COUNT);
#endif
}

std::optional<TokenSyntax> PrefixOperatorExprSyntax::getOperatorToken()
{
   RefCountPtr<SyntaxData> operatorTokenData = m_data->getChild(Cursor::OperatorToken);
   if (!operatorTokenData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, operatorTokenData.get()};
}

ExprSyntax PrefixOperatorExprSyntax::getExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Expr).get()};
}

PrefixOperatorExprSyntax PrefixOperatorExprSyntax::withOperatorToken(std::optional<TokenSyntax> operatorToken)
{
   RefCountPtr<RawSyntax> rawOperatorToken;
   if (operatorToken.has_value()) {
      rawOperatorToken = operatorToken->getRaw();
   } else {
      rawOperatorToken = nullptr;
   }
   return m_data->replaceChild<PrefixOperatorExprSyntax>(rawOperatorToken, Cursor::OperatorToken);
}

PrefixOperatorExprSyntax PrefixOperatorExprSyntax::withExpr(std::optional<TokenSyntax> expr)
{
   RefCountPtr<RawSyntax> rawExpr;
   if (expr.has_value()) {
      rawExpr = expr->getRaw();
   } else {
      rawExpr = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<PrefixOperatorExprSyntax>(rawExpr, Cursor::Expr);
}

void PostfixOperatorExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == PostfixOperatorExprSyntax::CHILDREN_COUNT);
#endif
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
   RefCountPtr<RawSyntax> rawExpr;
   if (expr.has_value()) {
      rawExpr = expr->getRaw();
   } else {
      rawExpr = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<PostfixOperatorExprSyntax>(rawExpr, Cursor::Expr);
}

PostfixOperatorExprSyntax PostfixOperatorExprSyntax::withOperatorToken(std::optional<TokenSyntax> operatorToken)
{
   RefCountPtr<RawSyntax> rawOperatorToken;
   if (operatorToken.has_value()) {
      rawOperatorToken = operatorToken->getRaw();
   } else {
      rawOperatorToken = RawSyntax::missing(TokenKindType::T_POSTFIX_OPERATOR,
                                            OwnedString::makeUnowned(""));
   }
   return m_data->replaceChild<PostfixOperatorExprSyntax>(rawOperatorToken, Cursor::OperatorToken);
}

void BinaryOperatorExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == BinaryOperatorExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax BinaryOperatorExprSyntax::getOperatorToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::OperatorToken).get()};
}

BinaryOperatorExprSyntax BinaryOperatorExprSyntax::withOperatorToken(std::optional<TokenSyntax> operatorToken)
{
   RefCountPtr<RawSyntax> rawOperatorToken;
   if (operatorToken.has_value()) {
      rawOperatorToken = operatorToken->getRaw();
   } else {
      rawOperatorToken = RawSyntax::missing(TokenKindType::T_BINARY_OPERATOR,
                                            OwnedString::makeUnowned(""));
   }
   return m_data->replaceChild<BinaryOperatorExprSyntax>(rawOperatorToken, TokenKindType::T_BINARY_OPERATOR);
}

///
/// LexicalVarItemSyntax
///
void LexicalVarItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == LexicalVarItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ReferenceToken, std::set{TokenKindType::T_AMPERSAND});
   syntax_assert_child_token(raw, Variable, std::set{TokenKindType::T_VARIABLE});
#endif
}

std::optional<TokenSyntax> LexicalVarItemSyntax::getReferenceToken()
{
   RefCountPtr<SyntaxData> refData = m_data->getChild(Cursor::ReferenceToken);
   if (!refData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, refData.get()};
}

TokenSyntax LexicalVarItemSyntax::getVariable()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Variable).get()};
}

LexicalVarItemSyntax LexicalVarItemSyntax::withReferenceToken(std::optional<TokenSyntax> referenceToken)
{
   RefCountPtr<RawSyntax> referenceTokenRaw;
   if (referenceToken.has_value()) {
      referenceTokenRaw = referenceToken->getRaw();
   } else {
      referenceTokenRaw = nullptr;
   }
   return m_data->replaceChild<LexicalVarItemSyntax>(referenceTokenRaw, Cursor::ReferenceToken);
}

LexicalVarItemSyntax LexicalVarItemSyntax::withVariable(std::optional<TokenSyntax> variable)
{
   RefCountPtr<RawSyntax> variableRaw;
   if (variable.has_value()) {
      variableRaw = variable->getRaw();
   } else {
      variableRaw = RawSyntax::missing(TokenKindType::T_VARIABLE,
                                       OwnedString::makeUnowned(get_token_text(TokenKindType::T_VARIABLE)));
   }
   return m_data->replaceChild<LexicalVarItemSyntax>(variableRaw, Cursor::Variable);
}

} // polar::syntax
