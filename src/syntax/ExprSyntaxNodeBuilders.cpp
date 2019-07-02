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

#include "polarphp/syntax/builder/ExprSyntaxNodeBuilders.h"

namespace polar::syntax {

///
/// NullExprSyntaxBuilder
///

NullExprSyntaxBuilder &NullExprSyntaxBuilder::useNullKeyword(TokenSyntax nullKeyword)
{
   m_layout[cursor_index(Cursor::NullKeyword)] = nullKeyword.getRaw();
   return *this;
}

NullExprSyntax NullExprSyntaxBuilder::build()
{
   CursorIndex nullKeywordIndex = cursor_index(Cursor::NullKeyword);
   if (!m_layout[nullKeywordIndex]) {
      m_layout[nullKeywordIndex] = RawSyntax::missing(TokenKindType::T_NULL,
                                                      OwnedString::makeUnowned(get_token_text(TokenKindType::T_NULL)));
   }
   RefCountPtr<RawSyntax> rawNullExpr = RawSyntax::make(SyntaxKind::NullExpr, m_layout,
                                                        SourcePresence::Present, m_arena);
   return make<NullExprSyntax>(rawNullExpr);
}

///
/// ClassRefParentExprSyntaxBuilder
///

ClassRefParentExprSyntaxBuilder &ClassRefParentExprSyntaxBuilder::useParentKeyword(TokenSyntax parentKeyword)
{
   m_layout[cursor_index(Cursor::ParentKeyword)] = parentKeyword.getRaw();
   return *this;
}

ClassRefParentExprSyntax ClassRefParentExprSyntaxBuilder::build()
{
   CursorIndex parentKeywordIndex = cursor_index(Cursor::ParentKeyword);
   if (!m_layout[parentKeywordIndex]) {
      m_layout[parentKeywordIndex] = RawSyntax::missing(TokenKindType::T_CLASS_REF_PARENT,
                                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_PARENT)));
   }
   RefCountPtr<RawSyntax> rawParentKeyword = RawSyntax::make(SyntaxKind::ClassRefParentExpr, m_layout, SourcePresence::Present,
                                                             m_arena);
   return make<ClassRefParentExprSyntax>(rawParentKeyword);
}

///
/// ClassRefSelfExprSyntaxBuilder
///
ClassRefSelfExprSyntaxBuilder &ClassRefSelfExprSyntaxBuilder::useSelfKeyword(TokenSyntax selfKeyword)
{
   m_layout[cursor_index(Cursor::SelfKeyword)] = selfKeyword.getRaw();
   return *this;
}

ClassRefSelfExprSyntax ClassRefSelfExprSyntaxBuilder::build()
{
   CursorIndex selfKeywordIndex = cursor_index(Cursor::SelfKeyword);
   if (!m_layout[selfKeywordIndex]) {
      m_layout[selfKeywordIndex] = RawSyntax::missing(TokenKindType::T_CLASS_REF_SELF,
                                                      OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_SELF)));
   }
   RefCountPtr<RawSyntax> rawParentKeyword = RawSyntax::make(SyntaxKind::ClassRefSelfExpr, m_layout, SourcePresence::Present,
                                                             m_arena);
   return make<ClassRefSelfExprSyntax>(rawParentKeyword);
}

///
/// ClassRefStaticExprSyntaxBuilder
///
ClassRefStaticExprSyntaxBuilder &ClassRefStaticExprSyntaxBuilder::useStaticKeyword(TokenSyntax staticKeyword)
{
   m_layout[cursor_index(Cursor::StaticKeyword)] = staticKeyword.getRaw();
   return *this;
}

ClassRefStaticExprSyntax ClassRefStaticExprSyntaxBuilder::build()
{
   CursorIndex staticKeywordIndex = cursor_index(Cursor::StaticKeyword);
   if (!m_layout[staticKeywordIndex]) {
      m_layout[staticKeywordIndex] = RawSyntax::missing(TokenKindType::T_CLASS_REF_STATIC,
                                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_STATIC)));
   }
   RefCountPtr<RawSyntax> rawParentKeyword = RawSyntax::make(SyntaxKind::ClassRefStaticExpr, m_layout, SourcePresence::Present,
                                                             m_arena);
   return make<ClassRefStaticExprSyntax>(rawParentKeyword);
}

///
/// IntegerLiteralExprSyntaxBuilder
///

IntegerLiteralExprSyntaxBuilder &IntegerLiteralExprSyntaxBuilder::useDigits(TokenSyntax digits)
{
   m_layout[cursor_index(Cursor::Digits)] = digits.getRaw();
   return *this;
}

IntegerLiteralExprSyntax IntegerLiteralExprSyntaxBuilder::build()
{
   CursorIndex digitsIndex = cursor_index(Cursor::Digits);
   if (!m_layout[digitsIndex]) {
      m_layout[digitsIndex] = RawSyntax::missing(TokenKindType::T_LNUMBER,
                                                 OwnedString::makeUnowned(get_token_text(TokenKindType::T_LNUMBER)));
   }
   RefCountPtr<RawSyntax> rawDigits = RawSyntax::make(SyntaxKind::IntegerLiteralExpr, m_layout, SourcePresence::Present,
                                                      m_arena);
   return make<IntegerLiteralExprSyntax>(rawDigits);
}

///
/// FloatLiteralExprSyntaxBuilder
///

FloatLiteralExprSyntaxBuilder &FloatLiteralExprSyntaxBuilder::useFloatDigits(TokenSyntax digits)
{
   m_layout[cursor_index(Cursor::FloatDigits)] = digits.getRaw();
   return *this;
}

FloatLiteralExprSyntax FloatLiteralExprSyntaxBuilder::build()
{
   CursorIndex digitsIndex = cursor_index(Cursor::FloatDigits);
   if (!m_layout[digitsIndex]) {
      m_layout[digitsIndex] = RawSyntax::missing(TokenKindType::T_DNUMBER,
                                                 OwnedString::makeUnowned(get_token_text(TokenKindType::T_DNUMBER)));
   }
   RefCountPtr<RawSyntax> rawDigits = RawSyntax::make(SyntaxKind::FloatLiteralExpr, m_layout, SourcePresence::Present,
                                                      m_arena);
   return make<FloatLiteralExprSyntax>(rawDigits);
}

///
/// StringLiteralExprSyntaxBuilder
///
StringLiteralExprSyntaxBuilder &StringLiteralExprSyntaxBuilder::useString(TokenSyntax str)
{
   m_layout[cursor_index(Cursor::String)] = str.getRaw();
   return *this;
}

StringLiteralExprSyntax StringLiteralExprSyntaxBuilder::build()
{
   CursorIndex strIndex = cursor_index(Cursor::String);
   if (!m_layout[strIndex]) {
      m_layout[strIndex] = RawSyntax::missing(TokenKindType::T_STRING,
                                              OwnedString::makeUnowned(get_token_text(TokenKindType::T_STRING)));
   }
   RefCountPtr<RawSyntax> rawDigits = RawSyntax::make(SyntaxKind::StringLiteralExpr, m_layout, SourcePresence::Present,
                                                      m_arena);
   return make<StringLiteralExprSyntax>(rawDigits);
}

///
/// BooleanLiteralExprSyntaxBuilder
///
BooleanLiteralExprSyntaxBuilder &BooleanLiteralExprSyntaxBuilder::useBoolean(TokenSyntax booleanValue)
{
   m_layout[cursor_index(Cursor::Boolean)] = booleanValue.getRaw();
   return *this;
}

BooleanLiteralExprSyntax BooleanLiteralExprSyntaxBuilder::build()
{
   CursorIndex booleanIndex = cursor_index(Cursor::Boolean);
   if (!m_layout[booleanIndex]) {
      m_layout[booleanIndex] = RawSyntax::missing(TokenKindType::T_TRUE,
                                                  OwnedString::makeUnowned(get_token_text(TokenKindType::T_TRUE)));
   }
   RefCountPtr<RawSyntax> rawDigits = RawSyntax::make(SyntaxKind::BooleanLiteralExpr, m_layout, SourcePresence::Present,
                                                      m_arena);
   return make<BooleanLiteralExprSyntax>(rawDigits);
}

///
/// TernaryExprSyntaxBuilder
///

TernaryExprSyntaxBuilder &TernaryExprSyntaxBuilder::useConditionExpr(ExprSyntax conditionExpr)
{
   m_layout[cursor_index(Cursor::ConditionExpr)] = conditionExpr.getRaw();
   return *this;
}

TernaryExprSyntaxBuilder &TernaryExprSyntaxBuilder::useQuestionMark(TokenSyntax questionMark)
{
   m_layout[cursor_index(Cursor::QuestionMark)] = questionMark.getRaw();
   return *this;
}

TernaryExprSyntaxBuilder &TernaryExprSyntaxBuilder::useFirstChoice(ExprSyntax firstChoice)
{
   m_layout[cursor_index(Cursor::FirstChoice)] = firstChoice.getRaw();
   return *this;
}

TernaryExprSyntaxBuilder &TernaryExprSyntaxBuilder::useColonMark(TokenSyntax colonMark)
{
   m_layout[cursor_index(Cursor::ColonMark)] = colonMark.getRaw();
   return *this;
}

TernaryExprSyntaxBuilder &TernaryExprSyntaxBuilder::useSecondChoice(ExprSyntax secondChoice)
{
   m_layout[cursor_index(Cursor::SecondChoice)] = secondChoice.getRaw();
   return *this;
}

TernaryExprSyntax TernaryExprSyntaxBuilder::build()
{
   CursorIndex conditionExprIndex = cursor_index(Cursor::ConditionExpr);
   CursorIndex questionMarkIndex = cursor_index(Cursor::QuestionMark);
   CursorIndex firstChoiceIndex = cursor_index(Cursor::FirstChoice);
   CursorIndex colonMarkIndex = cursor_index(Cursor::ColonMark);
   CursorIndex secondChoiceIndex = cursor_index(Cursor::SecondChoice);
   if (!m_layout[conditionExprIndex]) {
      m_layout[conditionExprIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }
   if (!m_layout[questionMarkIndex]) {
      m_layout[questionMarkIndex] = RawSyntax::missing(TokenKindType::T_QUESTION_MARK,
                                                       OwnedString::makeUnowned(get_token_text(TokenKindType::T_QUESTION_MARK)));
   }
   if (!m_layout[firstChoiceIndex]) {
      m_layout[firstChoiceIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }
   if (!m_layout[colonMarkIndex]) {
      m_layout[colonMarkIndex] = RawSyntax::missing(TokenKindType::T_COLON,
                                                    OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON)));
   }
   if (!m_layout[secondChoiceIndex]) {
      m_layout[secondChoiceIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }
   RefCountPtr<RawSyntax> rawTernaryExprSyntax = RawSyntax::make(SyntaxKind::TernaryExpr, m_layout, SourcePresence::Present,
                                                                 m_arena);
   return make<TernaryExprSyntax>(rawTernaryExprSyntax);
}

///
/// AssignmentExprSyntax
///

AssignmentExprSyntaxBuilder &AssignmentExprSyntaxBuilder::useAssignToken(TokenSyntax assignToken)
{
   m_layout[cursor_index(Cursor::AssignToken)] = assignToken.getRaw();
   return *this;
}

AssignmentExprSyntax AssignmentExprSyntaxBuilder::build()
{
   CursorIndex assignTokenIndex = cursor_index(Cursor::AssignToken);
   if (!m_layout[assignTokenIndex]) {
      m_layout[assignTokenIndex] = RawSyntax::missing(TokenKindType::T_EQUAL,
                                                      OwnedString::makeUnowned(get_token_text(TokenKindType::T_EQUAL)));
   }
   RefCountPtr<RawSyntax> rawAssignTokenSyntax = RawSyntax::make(SyntaxKind::AssignmentExpr, m_layout, SourcePresence::Present,
                                                                 m_arena);
   return make<AssignmentExprSyntax>(rawAssignTokenSyntax);
}

///
/// SequenceExprSyntax
///

SequenceExprSyntaxBuilder &SequenceExprSyntaxBuilder::useElements(ExprListSyntax elements)
{
   m_layout[cursor_index(Cursor::Elements)] = elements.getRaw();
   return *this;
}

SequenceExprSyntaxBuilder &SequenceExprSyntaxBuilder::addElement(ExprSyntax element)
{
   RefCountPtr<RawSyntax> rawElements = m_layout[cursor_index(Cursor::Elements)];
   if (!rawElements) {
      rawElements = RawSyntax::make(SyntaxKind::ExprList, {element.getRaw()}, SourcePresence::Present, m_arena);
   } else {
      rawElements = rawElements->append(element.getRaw());
   }
   return *this;
}

SequenceExprSyntax SequenceExprSyntaxBuilder::build()
{
   CursorIndex elementsIndex = cursor_index(Cursor::Elements);
   if (!m_layout[elementsIndex]) {
      m_layout[elementsIndex] = RawSyntax::missing(SyntaxKind::ExprList);
   }
   RefCountPtr<RawSyntax> rawSequenceExprSyntax = RawSyntax::make(SyntaxKind::SequenceExpr, m_layout,
                                                                  SourcePresence::Present, m_arena);
   return make<SequenceExprSyntax>(rawSequenceExprSyntax);
}

///
/// PrefixOperatorExprSyntaxBuilder
///

PrefixOperatorExprSyntaxBuilder &PrefixOperatorExprSyntaxBuilder::useOperatorToken(TokenSyntax operatorToken)
{
   m_layout[cursor_index(Cursor::OperatorToken)] = operatorToken.getRaw();
   return *this;
}

PrefixOperatorExprSyntaxBuilder &PrefixOperatorExprSyntaxBuilder::useExpr(ExprSyntax expr)
{
   m_layout[cursor_index(Cursor::Expr)] = expr.getRaw();
   return *this;
}

PrefixOperatorExprSyntax PrefixOperatorExprSyntaxBuilder::build()
{
   CursorIndex operatorTokenIndex = cursor_index(Cursor::OperatorToken);
   CursorIndex exprIndex = cursor_index(Cursor::Expr);
   if (!m_layout[operatorTokenIndex]) {
      m_layout[operatorTokenIndex] = RawSyntax::missing(TokenKindType::T_PREFIX_OPERATOR,
                                                        OwnedString::makeUnowned(""));
   }
   if (!m_layout[exprIndex]) {
      m_layout[exprIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }
   RefCountPtr<RawSyntax> rawPrefixOperatorExpr = RawSyntax::make(SyntaxKind::PrefixOperatorExpr, m_layout, SourcePresence::Present,
                                                                  m_arena);
   return make<PrefixOperatorExprSyntax>(rawPrefixOperatorExpr);
}

///
/// PostfixOperatorExprSyntaxBuilder
///

PostfixOperatorExprSyntaxBuilder &PostfixOperatorExprSyntaxBuilder::useExpr(ExprSyntax expr)
{
   m_layout[cursor_index(Cursor::Expr)] = expr.getRaw();
   return *this;
}

PostfixOperatorExprSyntaxBuilder &PostfixOperatorExprSyntaxBuilder::useOperatorToken(TokenSyntax operatorToken)
{
   m_layout[cursor_index(Cursor::OperatorToken)] = operatorToken.getRaw();
   return *this;
}

PostfixOperatorExprSyntax PostfixOperatorExprSyntaxBuilder::build()
{
   CursorIndex operatorTokenIndex = cursor_index(Cursor::OperatorToken);
   CursorIndex exprIndex = cursor_index(Cursor::Expr);
   if (!m_layout[operatorTokenIndex]) {
      m_layout[operatorTokenIndex] = RawSyntax::missing(TokenKindType::T_POSTFIX_OPERATOR,
                                                        OwnedString::makeUnowned(""));
   }
   if (!m_layout[exprIndex]) {
      m_layout[exprIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }
   RefCountPtr<RawSyntax> rawPostfixOperatorExpr = RawSyntax::make(SyntaxKind::PostfixOperatorExpr, m_layout, SourcePresence::Present,
                                                                  m_arena);
   return make<PostfixOperatorExprSyntax>(rawPostfixOperatorExpr);
}

///
/// BinaryOperatorExprSyntaxBuilder
///

BinaryOperatorExprSyntaxBuilder &BinaryOperatorExprSyntaxBuilder::useOperatorToken(TokenSyntax operatorToken)
{
   m_layout[cursor_index(Cursor::OperatorToken)] = operatorToken.getRaw();
   return *this;
}

BinaryOperatorExprSyntax BinaryOperatorExprSyntaxBuilder::build()
{
   CursorIndex operatorTokenIndex = cursor_index(Cursor::OperatorToken);
   if (!m_layout[operatorTokenIndex]) {
      m_layout[operatorTokenIndex] = RawSyntax::missing(TokenKindType::T_BINARY_OPERATOR,
                                                        OwnedString::makeUnowned(""));
   }
   RefCountPtr<RawSyntax> rawBinaryOperatorExprSyntax = RawSyntax::make(SyntaxKind::BinaryOperatorExpr, m_layout, SourcePresence::Present,
                                                                        m_arena);
   return make<BinaryOperatorExprSyntax>(rawBinaryOperatorExprSyntax);
}

} // polar::syntax
