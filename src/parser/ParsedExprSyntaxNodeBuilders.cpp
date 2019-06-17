// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/17.

#include "polarphp/parser/parsedbuilder/ParsedExprSyntaxNodeBuilders.h"
#include "polarphp/parser/ParsedRawSyntaxRecorder.h"
#include "polarphp/parser/SyntaxParsingContext.h"
#include "polarphp/parser/parsedsyntaxnode/ParsedExprSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"

namespace polar::parser {

///
/// ParsedNullExprSyntaxBuilder
///
ParsedNullExprSyntaxBuilder &ParsedNullExprSyntaxBuilder::useNullKeyword(ParsedTokenSyntax nullKeyword)
{
   m_layout[cursor_index(Cursor::NullKeyword)] = nullKeyword.getRaw();
   return *this;
}

ParsedNullExprSyntax ParsedNullExprSyntaxBuilder::build()
{
   if (m_context.isBacktracking()) {
      return makeDeferred();
   }
   return record();
}

ParsedNullExprSyntax ParsedNullExprSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::NullExpr, m_layout, m_context);
   return ParsedNullExprSyntax(std::move(rawNode));
}

ParsedNullExprSyntax ParsedNullExprSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::NullExpr, m_layout);
   return ParsedNullExprSyntax(std::move(rawNode));
}

void ParsedNullExprSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex nullKeywordIndex = cursor_index(Cursor::NullKeyword);
   if (m_layout[nullKeywordIndex].isNull()) {
      if (deferred) {
         m_layout[nullKeywordIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_NULL, SourceLoc());
      } else {
         recorder.recordMissingToken(TokenKindType::T_NULL, SourceLoc());
      }
   }
}

///
/// ParsedClassRefParentExprSyntaxBuilder
///
ParsedClassRefParentExprSyntaxBuilder
&ParsedClassRefParentExprSyntaxBuilder::useParentKeyword(ParsedTokenSyntax parentKeyword)
{
   m_layout[cursor_index(Cursor::ParentKeyword)] = parentKeyword.getRaw();
   return *this;
}

ParsedClassRefParentExprSyntax ParsedClassRefParentExprSyntaxBuilder::build()
{
   if (m_context.isBacktracking()) {
      return makeDeferred();
   }
   return record();
}

ParsedClassRefParentExprSyntax ParsedClassRefParentExprSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::ClassRefParentExpr, m_layout, m_context);
   return ParsedClassRefParentExprSyntax(std::move(rawNode));
}

ParsedClassRefParentExprSyntax ParsedClassRefParentExprSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::ClassRefParentExpr, m_layout);
   return ParsedClassRefParentExprSyntax(std::move(rawNode));
}

void ParsedClassRefParentExprSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex parentExprIndex = cursor_index(Cursor::ParentKeyword);
   if (m_layout[parentExprIndex].isNull()) {
      if (deferred) {
         m_layout[parentExprIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_CLASS_REF_PARENT, SourceLoc());
      } else {
         m_layout[parentExprIndex] = recorder.recordMissingToken(TokenKindType::T_CLASS_REF_PARENT, SourceLoc());
      }
   }
}

///
/// ParsedClassRefSelfExprSyntaxBuilder
///
ParsedClassRefSelfExprSyntaxBuilder &ParsedClassRefSelfExprSyntaxBuilder::useSelfKeyword(ParsedTokenSyntax selfKeyword)
{
   m_layout[cursor_index(Cursor::SelfKeyword)] = selfKeyword.getRaw();
   return *this;
}

ParsedClassRefSelfExprSyntax ParsedClassRefSelfExprSyntaxBuilder::build()
{
   if (m_context.isBacktracking()) {
      return makeDeferred();
   }
   return record();
}

ParsedClassRefSelfExprSyntax ParsedClassRefSelfExprSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::ClassRefParentExpr, m_layout, m_context);
   return ParsedClassRefSelfExprSyntax(std::move(rawNode));
}
ParsedClassRefSelfExprSyntax ParsedClassRefSelfExprSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::ClassRefParentExpr, m_layout);
   return ParsedClassRefSelfExprSyntax(std::move(rawNode));
}

void ParsedClassRefSelfExprSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex selfExprIndex = cursor_index(Cursor::SelfKeyword);
   if (m_layout[selfExprIndex].isNull()) {
      if (deferred) {
         m_layout[selfExprIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_CLASS_REF_SELF, SourceLoc());
      } else {
         m_layout[selfExprIndex] = recorder.recordMissingToken(TokenKindType::T_CLASS_REF_SELF, SourceLoc());
      }
   }
}

///
/// ParsedClassRefStaticExprSyntaxBuilder
///
ParsedClassRefStaticExprSyntaxBuilder
&ParsedClassRefStaticExprSyntaxBuilder::useStaticKeyword(ParsedTokenSyntax staticKeyword)
{
   m_layout[cursor_index(Cursor::StaticKeyword)] = staticKeyword.getRaw();
   return *this;
}

ParsedClassRefStaticExprSyntax ParsedClassRefStaticExprSyntaxBuilder::build()
{
   if (m_context.isBacktracking()) {
      return makeDeferred();
   }
   return record();
}

ParsedClassRefStaticExprSyntax ParsedClassRefStaticExprSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::ClassRefStaticExpr, m_layout, m_context);
   return ParsedClassRefStaticExprSyntax(std::move(rawNode));
}

ParsedClassRefStaticExprSyntax ParsedClassRefStaticExprSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::ClassRefStaticExpr, m_layout);
   return ParsedClassRefStaticExprSyntax(std::move(rawNode));
}

void ParsedClassRefStaticExprSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex staticExprIndex = cursor_index(Cursor::StaticKeyword);
   if (m_layout[staticExprIndex].isNull()) {
      if (deferred) {
         m_layout[staticExprIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_STATIC, SourceLoc());
      } else {
         m_layout[staticExprIndex] = recorder.recordMissingToken(TokenKindType::T_STATIC, SourceLoc());
      }
   }
}

///
/// ParsedIntegerLiteralExprSyntaxBuilder
///
ParsedIntegerLiteralExprSyntaxBuilder
&ParsedIntegerLiteralExprSyntaxBuilder::useDigits(ParsedTokenSyntax digits)
{
   m_layout[cursor_index(Cursor::Digits)] = digits.getRaw();
   return *this;
}

ParsedIntegerLiteralExprSyntax ParsedIntegerLiteralExprSyntaxBuilder::build()
{
   if (m_context.isBacktracking()) {
      return makeDeferred();
   }
   return record();
}

ParsedIntegerLiteralExprSyntax ParsedIntegerLiteralExprSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::IntegerLiteralExpr, m_layout, m_context);
   return ParsedIntegerLiteralExprSyntax(std::move(rawNode));
}

ParsedIntegerLiteralExprSyntax ParsedIntegerLiteralExprSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::IntegerLiteralExpr, m_layout);
   return ParsedIntegerLiteralExprSyntax(std::move(rawNode));
}

void ParsedIntegerLiteralExprSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex digitsIndex = cursor_index(Cursor::Digits);
   if (m_layout[digitsIndex].isNull()) {
      if (deferred) {
         m_layout[digitsIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_LNUMBER, SourceLoc());
      } else {
         m_layout[digitsIndex] = recorder.recordMissingToken(TokenKindType::T_LNUMBER, SourceLoc());
      }
   }
}

///
/// FloatLiteralExprSyntax
///
ParsedFloatLiteralExprSyntaxBuilder
&ParsedFloatLiteralExprSyntaxBuilder::useDigits(ParsedTokenSyntax floatDigits)
{
   m_layout[cursor_index(Cursor::FloatDigits)] = floatDigits.getRaw();
   return *this;
}

ParsedFloatLiteralExprSyntax ParsedFloatLiteralExprSyntaxBuilder::build()
{
   if (m_context.isBacktracking()) {
      return makeDeferred();
   }
   return record();
}

ParsedFloatLiteralExprSyntax ParsedFloatLiteralExprSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::FloatLiteralExpr, m_layout, m_context);
   return ParsedFloatLiteralExprSyntax(std::move(rawNode));
}

ParsedFloatLiteralExprSyntax ParsedFloatLiteralExprSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::FloatLiteralExpr, m_layout);
   return ParsedFloatLiteralExprSyntax(std::move(rawNode));
}

void ParsedFloatLiteralExprSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex floatDigitsIndex = cursor_index(Cursor::FloatDigits);
   if (m_layout[floatDigitsIndex].isNull()) {
      if (deferred) {
         m_layout[floatDigitsIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_DNUMBER, SourceLoc());
      } else {
         m_layout[floatDigitsIndex] = recorder.recordMissingToken(TokenKindType::T_DNUMBER, SourceLoc());
      }
   }
}

///
/// ParsedStringLiteralExprSyntaxBuilder
///
ParsedStringLiteralExprSyntaxBuilder
&ParsedStringLiteralExprSyntaxBuilder::useString(ParsedTokenSyntax str)
{
   m_layout[cursor_index(Cursor::String)] = str.getRaw();
   return *this;
}

ParsedStringLiteralExprSyntax ParsedStringLiteralExprSyntaxBuilder::build()
{
   if (m_context.isBacktracking()) {
      return makeDeferred();
   }
   return record();
}

ParsedStringLiteralExprSyntax ParsedStringLiteralExprSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::StringLiteralExpr, m_layout, m_context);
   return ParsedStringLiteralExprSyntax(std::move(rawNode));
}

ParsedStringLiteralExprSyntax ParsedStringLiteralExprSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::StringLiteralExpr, m_layout);
   return ParsedStringLiteralExprSyntax(std::move(rawNode));
}

void ParsedStringLiteralExprSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex strIndex = cursor_index(Cursor::String);
   if (m_layout[strIndex].isNull()) {
      if (deferred) {
         m_layout[strIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_STRING, SourceLoc());
      } else {
         m_layout[strIndex] = recorder.recordMissingToken(TokenKindType::T_STRING, SourceLoc());
      }
   }
}

///
/// ParsedBooleanLiteralExprSyntaxBuilder
///

ParsedBooleanLiteralExprSyntaxBuilder
&ParsedBooleanLiteralExprSyntaxBuilder::useBoolean(ParsedTokenSyntax booleanToken)
{
   m_layout[cursor_index(Cursor::Boolean)] = booleanToken.getRaw();
   return *this;
}

ParsedBooleanLiteralExprSyntax ParsedBooleanLiteralExprSyntaxBuilder::build()
{
   if (m_context.isBacktracking()) {
      return makeDeferred();
   }
   return record();
}

ParsedBooleanLiteralExprSyntax ParsedBooleanLiteralExprSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::BooleanLiteralExpr, m_layout, m_context);
   return ParsedBooleanLiteralExprSyntax(std::move(rawNode));
}

ParsedBooleanLiteralExprSyntax ParsedBooleanLiteralExprSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::BooleanLiteralExpr, m_layout);
   return ParsedBooleanLiteralExprSyntax(std::move(rawNode));
}

void ParsedBooleanLiteralExprSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex booleanIndex = cursor_index(Cursor::Boolean);
   if (m_layout[booleanIndex].isNull()) {
      if (deferred) {
         m_layout[booleanIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_TRUE, SourceLoc());
      } else {
         m_layout[booleanIndex] = recorder.recordMissingToken(TokenKindType::T_TRUE, SourceLoc());
      }
   }
}

///
/// ParsedTernaryExprSyntaxBuilder
///

ParsedTernaryExprSyntaxBuilder
&ParsedTernaryExprSyntaxBuilder::useConditionExpr(ParsedExprSyntax conditionExpr)
{
   m_layout[cursor_index(Cursor::ConditionExpr)] = conditionExpr.getRaw();
   return *this;
}

ParsedTernaryExprSyntaxBuilder
&ParsedTernaryExprSyntaxBuilder::useQuestionMark(ParsedTokenSyntax questionMark)
{
   m_layout[cursor_index(Cursor::QuestionMark)] = questionMark.getRaw();
   return *this;
}

ParsedTernaryExprSyntaxBuilder
&ParsedTernaryExprSyntaxBuilder::useFirstChoice(ParsedExprSyntax firstChoice)
{
   m_layout[cursor_index(Cursor::FirstChoice)] = firstChoice.getRaw();
   return *this;
}

ParsedTernaryExprSyntaxBuilder
&ParsedTernaryExprSyntaxBuilder::useColonMark(ParsedTokenSyntax colonMark)
{
   m_layout[cursor_index(Cursor::ColonMark)] = colonMark.getRaw();
   return *this;
}

ParsedTernaryExprSyntaxBuilder
&ParsedTernaryExprSyntaxBuilder::useSecondChoice(ParsedExprSyntax secondChoice)
{
   m_layout[cursor_index(Cursor::SecondChoice)] = secondChoice.getRaw();
   return *this;
}

ParsedTernaryExprSyntax ParsedTernaryExprSyntaxBuilder::build()
{
   if (m_context.isBacktracking()) {
      return makeDeferred();
   }
   return record();
}

ParsedTernaryExprSyntax ParsedTernaryExprSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::TernaryExpr, m_layout, m_context);
   return ParsedTernaryExprSyntax(std::move(rawNode));
}

ParsedTernaryExprSyntax ParsedTernaryExprSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::TernaryExpr, m_layout);
   return ParsedTernaryExprSyntax(std::move(rawNode));
}

void ParsedTernaryExprSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex conditionExprIndex = cursor_index(Cursor::ConditionExpr);
   CursorIndex questionMarkIndex = cursor_index(Cursor::QuestionMark);
   CursorIndex firstChoiceIndex = cursor_index(Cursor::FirstChoice);
   CursorIndex colonMarkIndex = cursor_index(Cursor::ColonMark);
   CursorIndex secondChoiceIndex = cursor_index(Cursor::SecondChoice);
   if (m_layout[conditionExprIndex].isNull()) {
      polar_unreachable("need missing non-token nodes ?");
   }
   if (m_layout[questionMarkIndex].isNull()) {
      if (deferred) {
         m_layout[questionMarkIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_INFIX_QUESTION_MARK, SourceLoc());
      } else {
         m_layout[questionMarkIndex] = recorder.recordMissingToken(TokenKindType::T_INFIX_QUESTION_MARK, SourceLoc());
      }
   }
   if (m_layout[firstChoiceIndex].isNull()) {
      polar_unreachable("need missing non-token nodes ?");
   }
   if (m_layout[colonMarkIndex].isNull()) {
      if (deferred) {
         m_layout[colonMarkIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_COLON, SourceLoc());
      } else {
         m_layout[colonMarkIndex] = recorder.recordMissingToken(TokenKindType::T_COLON, SourceLoc());
      }
   }
   if (m_layout[secondChoiceIndex].isNull()) {
      polar_unreachable("need missing non-token nodes ?");
   }
}

///
/// ParsedAssignmentExprSyntaxBuilder
///
ParsedAssignmentExprSyntaxBuilder
&ParsedAssignmentExprSyntaxBuilder::useAssignToken(ParsedTokenSyntax assignToken)
{
   m_layout[cursor_index(Cursor::AssignToken)] = assignToken.getRaw();
   return *this;
}

ParsedAssignmentExprSyntax ParsedAssignmentExprSyntaxBuilder::build()
{
   if (m_context.isBacktracking()) {
      return makeDeferred();
   }
   return record();
}

ParsedAssignmentExprSyntax ParsedAssignmentExprSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::AssignmentExpr, m_layout, m_context);
   return ParsedAssignmentExprSyntax(std::move(rawNode));
}

ParsedAssignmentExprSyntax ParsedAssignmentExprSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::AssignmentExpr, m_layout);
   return ParsedAssignmentExprSyntax(std::move(rawNode));
}

void ParsedAssignmentExprSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex assignmentExprIndex = cursor_index(Cursor::AssignToken);
   if (m_layout[assignmentExprIndex].isNull()) {
      if (deferred) {
         m_layout[assignmentExprIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_EQUAL, SourceLoc());
      } else {
         m_layout[assignmentExprIndex] = recorder.recordMissingToken(TokenKindType::T_EQUAL, SourceLoc());
      }
   }
}

///
/// ParsedSequenceExprSyntaxBuilder
///
ParsedSequenceExprSyntaxBuilder
&ParsedSequenceExprSyntaxBuilder::useElemenets(ParsedExprListSyntax elements)
{
   assert(m_statementMembers.empty() && "use either 'use' function or 'add', not both");
   m_layout[cursor_index(Cursor::Elements)] = elements.getRaw();
   return *this;
}

ParsedSequenceExprSyntaxBuilder
&ParsedSequenceExprSyntaxBuilder::addElementsMember(ParsedExprSyntax statement)
{
   assert(m_layout[cursor_index(Cursor::Elements)].isNull() &&
         "use either 'use' function or 'add', not both");
   m_statementMembers.push_back(std::move(statement.getRaw()));
   return *this;
}

ParsedSequenceExprSyntax ParsedSequenceExprSyntaxBuilder::build()
{
   if (m_context.isBacktracking()){
      return makeDeferred();
   }
   return record();
}

ParsedSequenceExprSyntax ParsedSequenceExprSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::SequenceExpr, m_layout, m_context);
   return ParsedSequenceExprSyntax(std::move(rawNode));
}

ParsedSequenceExprSyntax ParsedSequenceExprSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::SequenceExpr, m_layout);
   return ParsedSequenceExprSyntax(std::move(rawNode));
}

void ParsedSequenceExprSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex sequenceExprIndex = cursor_index(Cursor::Elements);
   if (!m_statementMembers.empty()) {
      if (deferred) {
         m_layout[sequenceExprIndex] = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::SequenceExpr, m_statementMembers, m_context);
      } else {
         m_layout[sequenceExprIndex] = recorder.recordRawSyntax(SyntaxKind::SequenceExpr, m_statementMembers);
      }
   }
   if (m_layout[sequenceExprIndex].isNull()) {
      if (deferred) {
         m_layout[sequenceExprIndex] = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::SequenceExpr, {}, m_context);
      } else {
         m_layout[sequenceExprIndex] = recorder.recordRawSyntax(SyntaxKind::SequenceExpr, {});
      }
   }
}

///
/// ParsedPrefixOperatorExprSyntaxBuilder
///
ParsedPrefixOperatorExprSyntaxBuilder
&ParsedPrefixOperatorExprSyntaxBuilder::useOperatorToken(ParsedTokenSyntax operatorToken)
{
   m_layout[cursor_index(Cursor::OperatorToken)] = operatorToken.getRaw();
   return *this;
}

ParsedPrefixOperatorExprSyntaxBuilder
&ParsedPrefixOperatorExprSyntaxBuilder::useExpr(ParsedExprSyntax expr)
{
   m_layout[cursor_index(Cursor::Expr)] = expr.getRaw();
   return *this;
}

ParsedPrefixOperatorExprSyntax ParsedPrefixOperatorExprSyntaxBuilder::build()
{
   if (m_context.isBacktracking()){
      return makeDeferred();
   }
   return record();
}

ParsedPrefixOperatorExprSyntax ParsedPrefixOperatorExprSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::PrefixOperatorExpr, m_layout, m_context);
   return ParsedPrefixOperatorExprSyntax(std::move(rawNode));
}

ParsedPrefixOperatorExprSyntax ParsedPrefixOperatorExprSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::PrefixOperatorExpr, m_layout);
   return ParsedPrefixOperatorExprSyntax(std::move(rawNode));
}

void ParsedPrefixOperatorExprSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   (void) deferred;
   CursorIndex exprIndex = cursor_index(Cursor::Expr);
   if (m_layout[exprIndex].isNull()) {
      polar_unreachable("need missing non-token nodes ?");
   }
}

} // polar::parser
