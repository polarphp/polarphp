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

#include "polarphp/parser/parsedbuilder/ParsedStmtSyntaxNodeBuilders.h"
#include "polarphp/parser/ParsedRawSyntaxRecorder.h"
#include "polarphp/parser/SyntaxParsingContext.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"

namespace polar::parser {

///
/// ParsedConditionElementSyntaxBuilder
///
ParsedConditionElementSyntaxBuilder
&ParsedConditionElementSyntaxBuilder::useCondition(ParsedSyntax condition)
{
   m_layout[cursor_index(Cursor::Condition)] = condition.getRaw();
   return *this;
}

ParsedConditionElementSyntaxBuilder
&ParsedConditionElementSyntaxBuilder::useTrailingComma(ParsedTokenSyntax trailingComma)
{
   m_layout[cursor_index(Cursor::TrailingComma)] = trailingComma.getRaw();
   return *this;
}

ParsedConditionElementSyntax ParsedConditionElementSyntaxBuilder::build()
{
   if (m_context.isBacktracking()){
      return makeDeferred();
   }
   return record();
}

ParsedConditionElementSyntax ParsedConditionElementSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::ConditionElement, m_layout, m_context);
   return ParsedConditionElementSyntax(std::move(rawNode));
}

ParsedConditionElementSyntax ParsedConditionElementSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::ConditionElement, m_layout);
   return ParsedConditionElementSyntax(std::move(rawNode));
}

void ParsedConditionElementSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void)recorder;
   (void)deferred;
   CursorIndex conditionIndex = cursor_index(Cursor::Condition);
   if (m_layout[conditionIndex].isNull()) {
      polar_unreachable("need missing non-token nodes ?");
   }
}

///
/// ParsedContinueStmtSyntaxBuilder
///
ParsedContinueStmtSyntaxBuilder
&ParsedContinueStmtSyntaxBuilder::useContinueKeyword(ParsedTokenSyntax continueKeyword)
{
   m_layout[cursor_index(Cursor::ContinueKeyword)] = continueKeyword.getRaw();
   return *this;
}

ParsedContinueStmtSyntaxBuilder
&ParsedContinueStmtSyntaxBuilder::useLNumberToken(ParsedTokenSyntax numberToken)
{
   m_layout[cursor_index(Cursor::LNumberToken)] = numberToken.getRaw();
   return *this;
}

ParsedContinueStmtSyntax ParsedContinueStmtSyntaxBuilder::build()
{
   if (m_context.isBacktracking()){
      return makeDeferred();
   }
   return record();
}

ParsedContinueStmtSyntax ParsedContinueStmtSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::ContinueStmt, m_layout, m_context);
   return ParsedContinueStmtSyntax(std::move(rawNode));
}

ParsedContinueStmtSyntax ParsedContinueStmtSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::ContinueStmt, m_layout);
   return ParsedContinueStmtSyntax(std::move(rawNode));
}

void ParsedContinueStmtSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex continueKeywordIndex = cursor_index(Cursor::ContinueKeyword);
   if (m_layout[continueKeywordIndex].isNull()) {
      if (deferred) {
         m_layout[continueKeywordIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_CONTINUE, SourceLoc());
      } else {
         m_layout[continueKeywordIndex] = recorder.recordMissingToken(TokenKindType::T_CONTINUE, SourceLoc());
      }
   }
}

///
/// ParsedBreakStmtSyntaxBuilder
///
ParsedBreakStmtSyntaxBuilder
&ParsedBreakStmtSyntaxBuilder::useBreakKeyword(ParsedTokenSyntax breakKeyword)
{
   m_layout[cursor_index(Cursor::BreakKeyword)] = breakKeyword.getRaw();
   return *this;
}

ParsedBreakStmtSyntaxBuilder
&ParsedBreakStmtSyntaxBuilder::useLNumberToken(ParsedTokenSyntax numberToken)
{
   m_layout[cursor_index(Cursor::LNumberToken)] = numberToken.getRaw();
   return *this;
}

ParsedBreakStmtSyntax ParsedBreakStmtSyntaxBuilder::build()
{
   if (m_context.isBacktracking()){
      return makeDeferred();
   }
   return record();
}

ParsedBreakStmtSyntax ParsedBreakStmtSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::BreakStmt, m_layout, m_context);
   return ParsedBreakStmtSyntax(std::move(rawNode));
}

ParsedBreakStmtSyntax ParsedBreakStmtSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::BreakStmt, m_layout);
   return ParsedBreakStmtSyntax(std::move(rawNode));
}

void ParsedBreakStmtSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex breakKeywordIndex = cursor_index(Cursor::BreakKeyword);
   if (m_layout[breakKeywordIndex].isNull()) {
      if (deferred) {
         m_layout[breakKeywordIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_BREAK, SourceLoc());
      } else {
         m_layout[breakKeywordIndex] = recorder.recordMissingToken(TokenKindType::T_BREAK, SourceLoc());
      }
   }
}

///
/// ParsedFallthroughStmtSyntaxBuilder
///
ParsedFallthroughStmtSyntaxBuilder
&ParsedFallthroughStmtSyntaxBuilder::useFallthroughKeyword(ParsedTokenSyntax fallthroughKeyword)
{
   m_layout[cursor_index(Cursor::FallthroughKeyword)] = fallthroughKeyword.getRaw();
   return *this;
}

ParsedFallthroughStmtSyntax ParsedFallthroughStmtSyntaxBuilder::build()
{
   if (m_context.isBacktracking()){
      return makeDeferred();
   }
   return record();
}

ParsedFallthroughStmtSyntax ParsedFallthroughStmtSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::ContinueStmt, m_layout, m_context);
   return ParsedFallthroughStmtSyntax(std::move(rawNode));
}

ParsedFallthroughStmtSyntax ParsedFallthroughStmtSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::ContinueStmt, m_layout);
   return ParsedFallthroughStmtSyntax(std::move(rawNode));
}

void ParsedFallthroughStmtSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex fallthroughKeywordIndex = cursor_index(Cursor::FallthroughKeyword);
   if (m_layout[fallthroughKeywordIndex].isNull()) {
      if (deferred) {
         m_layout[fallthroughKeywordIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_FALLTHROUGH, SourceLoc());
      } else {
         m_layout[fallthroughKeywordIndex] = recorder.recordMissingToken(TokenKindType::T_FALLTHROUGH, SourceLoc());
      }
   }
}

///
/// ParsedElseIfClauseSyntaxBuilder
///
ParsedElseIfClauseSyntaxBuilder
&ParsedElseIfClauseSyntaxBuilder::useElseIfKeyword(ParsedTokenSyntax elseIfKeyword)
{
   m_layout[cursor_index(Cursor::ElseIfKeyword)] = elseIfKeyword.getRaw();
   return *this;
}

ParsedElseIfClauseSyntaxBuilder
&ParsedElseIfClauseSyntaxBuilder::useLeftParen(ParsedTokenSyntax leftParen)
{
   m_layout[cursor_index(Cursor::LeftParen)] = leftParen.getRaw();
   return *this;
}

ParsedElseIfClauseSyntaxBuilder
&ParsedElseIfClauseSyntaxBuilder::useCondition(ParsedSyntax condition)
{
   m_layout[cursor_index(Cursor::Condition)] = condition.getRaw();
   return *this;
}

ParsedElseIfClauseSyntaxBuilder
&ParsedElseIfClauseSyntaxBuilder::useRightParen(ParsedTokenSyntax rightParen)
{
   m_layout[cursor_index(Cursor::RightParen)] = rightParen.getRaw();
   return *this;
}

ParsedElseIfClauseSyntaxBuilder
&ParsedElseIfClauseSyntaxBuilder::useBody(ParsedCodeBlockSyntax body)
{
   m_layout[cursor_index(Cursor::Body)] = body.getRaw();
   return *this;
}

ParsedElseIfClauseSyntax ParsedElseIfClauseSyntaxBuilder::build()
{
   if (m_context.isBacktracking()){
      return makeDeferred();
   }
   return record();
}

ParsedElseIfClauseSyntax ParsedElseIfClauseSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::ElseIfClause, m_layout, m_context);
   return ParsedElseIfClauseSyntax(std::move(rawNode));
}

ParsedElseIfClauseSyntax ParsedElseIfClauseSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::ElseIfClause, m_layout);
   return ParsedElseIfClauseSyntax(std::move(rawNode));
}

void ParsedElseIfClauseSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex elseIfIndex = cursor_index(Cursor::ElseIfKeyword);
   CursorIndex leftParenIndex = cursor_index(Cursor::LeftParen);
   CursorIndex conditionIndex = cursor_index(Cursor::Condition);
   CursorIndex rightParenIndex = cursor_index(Cursor::RightParen);
   CursorIndex bodyIndex = cursor_index(Cursor::Body);
   if (m_layout[elseIfIndex].isNull()) {
      if (deferred) {
         m_layout[elseIfIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_ELSEIF, SourceLoc());
      } else {
         m_layout[elseIfIndex] = recorder.recordMissingToken(TokenKindType::T_ELSEIF, SourceLoc());
      }
   }
   if (m_layout[leftParenIndex].isNull()) {
      if (deferred) {
         m_layout[leftParenIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_LEFT_PAREN, SourceLoc());
      } else {
         m_layout[leftParenIndex] = recorder.recordMissingToken(TokenKindType::T_LEFT_PAREN, SourceLoc());
      }
   }
   if (m_layout[conditionIndex].isNull()) {
      polar_unreachable("need missing non-token nodes ?");
   }
   if (m_layout[rightParenIndex].isNull()) {
      if (deferred) {
         m_layout[rightParenIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_RIGHT_PAREN, SourceLoc());
      } else {
         m_layout[rightParenIndex] = recorder.recordMissingToken(TokenKindType::T_RIGHT_PAREN, SourceLoc());
      }
   }
   if (m_layout[bodyIndex].isNull()) {
      polar_unreachable("need missing non-token nodes ?");
   }
}

///
/// ParsedIfStmtSyntaxBuilder
///
ParsedIfStmtSyntaxBuilder
&ParsedIfStmtSyntaxBuilder::useLabelName(ParsedTokenSyntax labelName)
{
   m_layout[Cursor::LabelName] = labelName.getRaw();
   return *this;
}

ParsedIfStmtSyntaxBuilder
&ParsedIfStmtSyntaxBuilder::useLabelColon(ParsedTokenSyntax labelColon)
{
   m_layout[Cursor::LabelColon] = labelColon.getRaw();
   return *this;
}

ParsedIfStmtSyntaxBuilder
&ParsedIfStmtSyntaxBuilder::useIfKeyword(ParsedTokenSyntax ifKeyword)
{
   m_layout[Cursor::IfKeyword] = ifKeyword.getRaw();
   return *this;
}

ParsedIfStmtSyntaxBuilder
&ParsedIfStmtSyntaxBuilder::useLeftParen(ParsedTokenSyntax leftParen)
{
   m_layout[Cursor::LeftParen] = leftParen.getRaw();
   return *this;
}

ParsedIfStmtSyntaxBuilder
&ParsedIfStmtSyntaxBuilder::useCondition(ParsedExprSyntax condition)
{
   m_layout[Cursor::Condition] = condition.getRaw();
   return *this;
}

ParsedIfStmtSyntaxBuilder
&ParsedIfStmtSyntaxBuilder::useRightParen(ParsedTokenSyntax rightParen)
{
   m_layout[Cursor::RightParen] = rightParen.getRaw();
   return *this;
}

ParsedIfStmtSyntaxBuilder
&ParsedIfStmtSyntaxBuilder::useBody(ParsedCodeBlockSyntax body)
{
   m_layout[Cursor::Body] = body.getRaw();
   return *this;
}

ParsedIfStmtSyntaxBuilder
&ParsedIfStmtSyntaxBuilder::useElseIfClauses(ParsedElseIfListSyntax elseIfClauses)
{
   assert(m_elseIfClausesMembers.empty() && "use either 'use' function or 'add', not both");
   m_layout[Cursor::ElseIfClauses] = elseIfClauses.getRaw();
   return *this;
}

ParsedIfStmtSyntaxBuilder
&ParsedIfStmtSyntaxBuilder::addElseIfClausesMember(ParsedElseIfClauseSyntax elseIfClause)
{
   assert(m_layout[cursor_index(Cursor::ElseIfClauses)].isNull() && "use either 'use' function or 'add', not both");
   m_elseIfClausesMembers.push_back(std::move(elseIfClause.getRaw()));
   return *this;
}

ParsedIfStmtSyntaxBuilder
&ParsedIfStmtSyntaxBuilder::useElseKeyword(ParsedTokenSyntax elseKeyword)
{
   m_layout[Cursor::ElseKeyword] = elseKeyword.getRaw();
   return *this;
}

ParsedIfStmtSyntaxBuilder
&ParsedIfStmtSyntaxBuilder::useElseBody(ParsedSyntax elseBody)
{
   m_layout[Cursor::ElseBody] = elseBody.getRaw();
   return *this;
}

ParsedIfStmtSyntax ParsedIfStmtSyntaxBuilder::build()
{
   if (m_context.isBacktracking()){
      return makeDeferred();
   }
   return record();
}

ParsedIfStmtSyntax ParsedIfStmtSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::IfStmt, m_layout, m_context);
   return ParsedIfStmtSyntax(std::move(rawNode));
}

ParsedIfStmtSyntax ParsedIfStmtSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::IfStmt, m_layout);
   return ParsedIfStmtSyntax(std::move(rawNode));
}

void ParsedIfStmtSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex ifKeywordIndex = cursor_index(Cursor::IfKeyword);
   CursorIndex leftParenIndex = cursor_index(Cursor::LeftParen);
   CursorIndex conditionIndex = cursor_index(Cursor::Condition);
   CursorIndex rightParenIndex = cursor_index(Cursor::RightParen);
   CursorIndex bodyIndex = cursor_index(Cursor::Body);
   CursorIndex elseIfClausesIndex = cursor_index(Cursor::ElseIfClauses);
   CursorIndex elseKeywordIndex = cursor_index(Cursor::ElseKeyword);
   CursorIndex elseBodyIndex = cursor_index(Cursor::ElseBody);

   if (!m_elseIfClausesMembers.empty()) {
      if (deferred) {
         m_layout[elseIfClausesIndex] = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::ElseIfList, m_layout, m_context);
      } else {
         m_layout[elseIfClausesIndex] = recorder.recordRawSyntax(SyntaxKind::ElseIfList, m_layout);
      }
   }

   if (m_layout[ifKeywordIndex].isNull()) {
      if (deferred) {
         m_layout[ifKeywordIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_IF, SourceLoc());
      } else {
         m_layout[ifKeywordIndex] = recorder.recordMissingToken(TokenKindType::T_IF, SourceLoc());
      }
   }
   if (m_layout[leftParenIndex].isNull()) {
      if (deferred) {
         m_layout[leftParenIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_LEFT_PAREN, SourceLoc());
      } else {
         m_layout[leftParenIndex] = recorder.recordMissingToken(TokenKindType::T_LEFT_PAREN, SourceLoc());
      }
   }
   if (m_layout[conditionIndex].isNull()) {
      polar_unreachable("need missing non-token nodes ?");
   }
   if (m_layout[rightParenIndex].isNull()) {
      if (deferred) {
         m_layout[rightParenIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_RIGHT_PAREN, SourceLoc());
      } else {
         m_layout[rightParenIndex] = recorder.recordMissingToken(TokenKindType::T_RIGHT_PAREN, SourceLoc());
      }
   }
   if (m_layout[bodyIndex].isNull()) {
      polar_unreachable("need missing non-token nodes ?");
   }

   if (m_layout[elseIfClausesIndex].isNull()) {
      if (deferred) {
         m_layout[elseIfClausesIndex] = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::ElseIfList, {}, m_context);
      } else {
         m_layout[elseIfClausesIndex] = recorder.recordRawSyntax(SyntaxKind::ElseIfList, {});
      }
   }

   if (m_layout[elseKeywordIndex].isNull()) {
      if (deferred) {
         m_layout[elseKeywordIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_ELSE, SourceLoc());
      } else {
         m_layout[elseKeywordIndex] = recorder.recordMissingToken(TokenKindType::T_ELSE, SourceLoc());
      }
   }
   if (m_layout[elseBodyIndex].isNull()) {
      polar_unreachable("need missing non-token nodes ?");
   }
}

ParsedWhileStmtSyntaxBuilder
&ParsedWhileStmtSyntaxBuilder::useLabelName(ParsedTokenSyntax labelName)
{
   m_layout[cursor_index(Cursor::LabelName)] = labelName.getRaw();
   return *this;
}

ParsedWhileStmtSyntaxBuilder
&ParsedWhileStmtSyntaxBuilder::useLabelColon(ParsedTokenSyntax labelColon)
{
   m_layout[cursor_index(Cursor::LabelColon)] = labelColon.getRaw();
   return *this;
}

ParsedWhileStmtSyntaxBuilder
&ParsedWhileStmtSyntaxBuilder::useWhileKeyword(ParsedTokenSyntax whileKeyword)
{
   m_layout[cursor_index(Cursor::WhileKeyword)] = whileKeyword.getRaw();
   return *this;
}

ParsedWhileStmtSyntaxBuilder
&ParsedWhileStmtSyntaxBuilder::useLeftParen(ParsedTokenSyntax leftParen)
{
   m_layout[cursor_index(Cursor::LeftParen)] = leftParen.getRaw();
   return *this;
}

ParsedWhileStmtSyntaxBuilder
&ParsedWhileStmtSyntaxBuilder::useConditions(ParsedConditionElementListSyntax conditions)
{
   assert(m_conditionsMembers.empty() && "use either 'use' function or 'add', not both");
   m_layout[cursor_index(Cursor::Conditions)] = conditions.getRaw();
   return *this;
}

ParsedWhileStmtSyntaxBuilder
&ParsedWhileStmtSyntaxBuilder::useRightParen(ParsedTokenSyntax rightParen)
{
   m_layout[cursor_index(Cursor::RightParen)] = rightParen.getRaw();
   return *this;
}

ParsedWhileStmtSyntaxBuilder
&ParsedWhileStmtSyntaxBuilder::addConditionsMember(ParsedConditionElementSyntax condition)
{
   assert(m_layout[cursor_index(Cursor::Conditions)].isNull() && "use either 'use' function or 'add', not both");
   m_conditionsMembers.push_back(std::move(condition.getRaw()));
   return *this;
}

ParsedWhileStmtSyntaxBuilder
&ParsedWhileStmtSyntaxBuilder::useBody(ParsedCodeBlockSyntax body)
{
   m_layout[cursor_index(Cursor::Body)] = body.getRaw();
   return *this;
}

ParsedWhileStmtSyntax ParsedWhileStmtSyntaxBuilder::build()
{
   if (m_context.isBacktracking()){
      return makeDeferred();
   }
   return record();
}

ParsedWhileStmtSyntax ParsedWhileStmtSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::IfStmt, m_layout, m_context);
   return ParsedWhileStmtSyntax(std::move(rawNode));
}

ParsedWhileStmtSyntax ParsedWhileStmtSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::IfStmt, m_layout);
   return ParsedWhileStmtSyntax(std::move(rawNode));
}

void ParsedWhileStmtSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex whileKeywordIndex = cursor_index(Cursor::WhileKeyword);
   CursorIndex leftParenIndex = cursor_index(Cursor::LeftParen);
   CursorIndex conditionsIndex = cursor_index(Cursor::Conditions);
   CursorIndex rightParenIndex = cursor_index(Cursor::RightParen);
   CursorIndex bodyIndex = cursor_index(Cursor::Body);
   if (!m_conditionsMembers.empty()) {
      if (deferred) {
         m_layout[conditionsIndex] = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::ConditionElementList, m_layout, m_context);
      } else {
         m_layout[conditionsIndex] = recorder.recordRawSyntax(SyntaxKind::ConditionElementList, m_layout);
      }
   }

   if (m_layout[whileKeywordIndex].isNull()) {
      if (deferred) {
         m_layout[whileKeywordIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_WHILE, SourceLoc());
      } else {
         m_layout[whileKeywordIndex] = recorder.recordMissingToken(TokenKindType::T_WHILE, SourceLoc());
      }
   }

   if (m_layout[leftParenIndex].isNull()) {
      if (deferred) {
         m_layout[leftParenIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_LEFT_PAREN, SourceLoc());
      } else {
         m_layout[leftParenIndex] = recorder.recordMissingToken(TokenKindType::T_LEFT_PAREN, SourceLoc());
      }
   }

   if (!m_conditionsMembers.empty()) {
      if (deferred) {
         m_layout[conditionsIndex] = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::ConditionElementList, {}, m_context);
      } else {
         m_layout[conditionsIndex] = recorder.recordRawSyntax(SyntaxKind::ConditionElementList, {});
      }
   }

   if (m_layout[rightParenIndex].isNull()) {
      if (deferred) {
         m_layout[rightParenIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_RIGHT_PAREN, SourceLoc());
      } else {
         m_layout[rightParenIndex] = recorder.recordMissingToken(TokenKindType::T_RIGHT_PAREN, SourceLoc());
      }
   }

   if (m_layout[bodyIndex].isNull()) {
      polar_unreachable("need missing non-token nodes ?");
   }
}

///
/// ParsedDoWhileStmtSyntaxBuilder
///
ParsedDoWhileStmtSyntaxBuilder
&ParsedDoWhileStmtSyntaxBuilder::useLabelName(ParsedTokenSyntax labelName)
{
   m_layout[cursor_index(Cursor::LabelName)] = labelName.getRaw();
   return *this;
}

ParsedDoWhileStmtSyntaxBuilder
&ParsedDoWhileStmtSyntaxBuilder::useLabelColon(ParsedTokenSyntax labelColon)
{
   m_layout[cursor_index(Cursor::LabelColon)] = labelColon.getRaw();
   return *this;
}

ParsedDoWhileStmtSyntaxBuilder
&ParsedDoWhileStmtSyntaxBuilder::useDoKeyword(ParsedTokenSyntax doKeyword)
{
   m_layout[cursor_index(Cursor::DoKeyword)] = doKeyword.getRaw();
   return *this;
}

ParsedDoWhileStmtSyntaxBuilder
&ParsedDoWhileStmtSyntaxBuilder::useBody(ParsedCodeBlockSyntax body)
{
   m_layout[cursor_index(Cursor::Body)] = body.getRaw();
   return *this;
}

ParsedDoWhileStmtSyntaxBuilder
&ParsedDoWhileStmtSyntaxBuilder::useWhileKeyword(ParsedTokenSyntax whileKeyword)
{
   m_layout[cursor_index(Cursor::WhileKeyword)] = whileKeyword.getRaw();
   return *this;
}

ParsedDoWhileStmtSyntaxBuilder
&ParsedDoWhileStmtSyntaxBuilder::useLeftParen(ParsedTokenSyntax leftParen)
{
   m_layout[cursor_index(Cursor::LeftParen)] = leftParen.getRaw();
   return *this;
}

ParsedDoWhileStmtSyntaxBuilder
&ParsedDoWhileStmtSyntaxBuilder::useCondition(ParsedExprSyntax condition)
{
   m_layout[cursor_index(Cursor::Condition)] = condition.getRaw();
   return *this;
}

ParsedDoWhileStmtSyntaxBuilder
&ParsedDoWhileStmtSyntaxBuilder::useRightParen(ParsedTokenSyntax rightParen)
{
   m_layout[cursor_index(Cursor::RightParen)] = rightParen.getRaw();
   return *this;
}

ParsedDoWhileStmtSyntax ParsedDoWhileStmtSyntaxBuilder::build()
{
   if (m_context.isBacktracking()){
      return makeDeferred();
   }
   return record();
}

ParsedDoWhileStmtSyntax ParsedDoWhileStmtSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::DoWhileStmt, m_layout, m_context);
   return ParsedDoWhileStmtSyntax(std::move(rawNode));
}

ParsedDoWhileStmtSyntax ParsedDoWhileStmtSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::DoWhileStmt, m_layout);
   return ParsedDoWhileStmtSyntax(std::move(rawNode));
}

void ParsedDoWhileStmtSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex doKeywordIndex = cursor_index(Cursor::DoKeyword);
   CursorIndex bodyIndex = cursor_index(Cursor::Body);
   CursorIndex whileKeywordIndex = cursor_index(Cursor::WhileKeyword);
   CursorIndex leftParenIndex = cursor_index(Cursor::LeftParen);
   CursorIndex conditionsIndex = cursor_index(Cursor::Condition);
   CursorIndex rightParenIndex = cursor_index(Cursor::RightParen);

   if (m_layout[doKeywordIndex].isNull()) {
      if (deferred) {
         m_layout[doKeywordIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_DO, SourceLoc());
      } else {
         m_layout[doKeywordIndex] = recorder.recordMissingToken(TokenKindType::T_DO, SourceLoc());
      }
   }

   if (m_layout[bodyIndex].isNull()) {
      polar_unreachable("need missing non-token nodes ?");
   }

   if (m_layout[whileKeywordIndex].isNull()) {
      if (deferred) {
         m_layout[whileKeywordIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_WHILE, SourceLoc());
      } else {
         m_layout[whileKeywordIndex] = recorder.recordMissingToken(TokenKindType::T_WHILE, SourceLoc());
      }
   }

   if (m_layout[leftParenIndex].isNull()) {
      if (deferred) {
         m_layout[leftParenIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_LEFT_PAREN, SourceLoc());
      } else {
         m_layout[leftParenIndex] = recorder.recordMissingToken(TokenKindType::T_LEFT_PAREN, SourceLoc());
      }
   }

   if (m_layout[conditionsIndex].isNull()) {
      polar_unreachable("need missing non-token nodes ?");
   }

   if (m_layout[rightParenIndex].isNull()) {
      if (deferred) {
         m_layout[rightParenIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_RIGHT_PAREN, SourceLoc());
      } else {
         m_layout[rightParenIndex] = recorder.recordMissingToken(TokenKindType::T_RIGHT_PAREN, SourceLoc());
      }
   }
}

///
/// ParsedSwitchDefaultLabelSyntaxBuilder
///
ParsedSwitchDefaultLabelSyntaxBuilder
&ParsedSwitchDefaultLabelSyntaxBuilder::useDefaultKeyword(ParsedTokenSyntax defaultKeyword)
{
   m_layout[cursor_index(Cursor::DefaultKeyword)] = defaultKeyword.getRaw();
   return *this;
}

ParsedSwitchDefaultLabelSyntaxBuilder
&ParsedSwitchDefaultLabelSyntaxBuilder::useColon(ParsedTokenSyntax colon)
{
   m_layout[cursor_index(Cursor::Colon)] = colon.getRaw();
   return *this;
}

ParsedSwitchDefaultLabelSyntax ParsedSwitchDefaultLabelSyntaxBuilder::build()
{
   if (m_context.isBacktracking()){
      return makeDeferred();
   }
   return record();
}

ParsedSwitchDefaultLabelSyntax ParsedSwitchDefaultLabelSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::SwitchDefaultLabel, m_layout, m_context);
   return ParsedSwitchDefaultLabelSyntax(std::move(rawNode));
}

ParsedSwitchDefaultLabelSyntax ParsedSwitchDefaultLabelSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::SwitchDefaultLabel, m_layout);
   return ParsedSwitchDefaultLabelSyntax(std::move(rawNode));
}

void ParsedSwitchDefaultLabelSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex defaultKeywordIndex = cursor_index(Cursor::DefaultKeyword);
   CursorIndex colonIndex = cursor_index(Cursor::Colon);
   if (m_layout[defaultKeywordIndex].isNull()) {
      if (deferred) {
         m_layout[defaultKeywordIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_DEFAULT, SourceLoc());
      } else {
         m_layout[defaultKeywordIndex] = recorder.recordMissingToken(TokenKindType::T_DEFAULT, SourceLoc());
      }
   }
   if (m_layout[colonIndex].isNull()) {
      if (deferred) {
         m_layout[colonIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_COLON, SourceLoc());
      } else {
         m_layout[colonIndex] = recorder.recordMissingToken(TokenKindType::T_COLON, SourceLoc());
      }
   }
}

///
/// ParsedSwitchCaseLabelSyntaxBuilder
///

ParsedSwitchCaseLabelSyntaxBuilder
&ParsedSwitchCaseLabelSyntaxBuilder::useCaseKeyword(ParsedTokenSyntax caseKeyword)
{
   m_layout[cursor_index(Cursor::CaseKeyword)] = caseKeyword.getRaw();
   return *this;
}

ParsedSwitchCaseLabelSyntaxBuilder
&ParsedSwitchCaseLabelSyntaxBuilder::useExpr(ParsedExprSyntax expr)
{
   m_layout[cursor_index(Cursor::Expr)] = expr.getRaw();
   return *this;
}

ParsedSwitchCaseLabelSyntaxBuilder
&ParsedSwitchCaseLabelSyntaxBuilder::useColon(ParsedTokenSyntax colon)
{
   m_layout[cursor_index(Cursor::Colon)] = colon.getRaw();
   return *this;
}

ParsedSwitchCaseLabelSyntax
ParsedSwitchCaseLabelSyntaxBuilder::build()
{
   if (m_context.isBacktracking()){
      return makeDeferred();
   }
   return record();
}

ParsedSwitchCaseLabelSyntax
ParsedSwitchCaseLabelSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::SwitchCaseLabel, m_layout, m_context);
   return ParsedSwitchCaseLabelSyntax(std::move(rawNode));
}

ParsedSwitchCaseLabelSyntax ParsedSwitchCaseLabelSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::SwitchCaseLabel, m_layout);
   return ParsedSwitchCaseLabelSyntax(std::move(rawNode));
}

void ParsedSwitchCaseLabelSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex defaultCaseIndex = cursor_index(Cursor::CaseKeyword);
   CursorIndex exprInex = cursor_index(Cursor::Expr);
   CursorIndex colonIndex = cursor_index(Cursor::Colon);
   if (m_layout[defaultCaseIndex].isNull()) {
      if (deferred) {
         m_layout[defaultCaseIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_CASE, SourceLoc());
      } else {
         m_layout[defaultCaseIndex] = recorder.recordMissingToken(TokenKindType::T_CASE, SourceLoc());
      }
   }

   if (m_layout[exprInex].isNull()) {
      polar_unreachable("need missing non-token nodes ?");
   }

   if (m_layout[colonIndex].isNull()) {
      if (deferred) {
         m_layout[colonIndex] = ParsedRawSyntaxNode::makeDeferredMissing(TokenKindType::T_COLON, SourceLoc());
      } else {
         m_layout[colonIndex] = recorder.recordMissingToken(TokenKindType::T_COLON, SourceLoc());
      }
   }
}

///
/// ParsedSwitchCaseSyntaxBuilder
///
ParsedSwitchCaseSyntaxBuilder
&ParsedSwitchCaseSyntaxBuilder::useLabel(ParsedSyntax label)
{
   m_layout[cursor_index(Cursor::Label)] = label.getRaw();
   return *this;
}

ParsedSwitchCaseSyntaxBuilder
&ParsedSwitchCaseSyntaxBuilder::useStatements(ParsedCodeBlockItemListSyntax statements)
{
   assert(m_statementsMembers.empty() && "use either 'use' function or 'add', not both");
   m_layout[cursor_index(Cursor::Statements)] = statements.getRaw();
   return *this;
}

ParsedSwitchCaseSyntaxBuilder
&ParsedSwitchCaseSyntaxBuilder::addStatement(ParsedCodeBlockItemSyntax statement)
{
   assert(m_layout[cursor_index(Cursor::Statements)].isNull() && "use either 'use' function or 'add', not both");
   m_statementsMembers.push_back(std::move(statement.getRaw()));
   return *this;
}

ParsedSwitchCaseSyntax ParsedSwitchCaseSyntaxBuilder::build()
{
   if (m_context.isBacktracking()){
      return makeDeferred();
   }
   return record();
}

ParsedSwitchCaseSyntax ParsedSwitchCaseSyntaxBuilder::makeDeferred()
{
   finishLayout(true);
   ParsedRawSyntaxNode rawNode = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::SwitchCase, m_layout, m_context);
   return ParsedSwitchCaseSyntax(std::move(rawNode));
}

ParsedSwitchCaseSyntax ParsedSwitchCaseSyntaxBuilder::record()
{
   finishLayout(false);
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   ParsedRawSyntaxNode rawNode = recorder.recordRawSyntax(SyntaxKind::SwitchCase, m_layout);
   return ParsedSwitchCaseSyntax(std::move(rawNode));
}

void ParsedSwitchCaseSyntaxBuilder::finishLayout(bool deferred)
{
   ParsedRawSyntaxRecorder &recorder = m_context.getRecorder();
   (void) recorder;
   CursorIndex labelIndex = cursor_index(Cursor::Label);
   CursorIndex statementsIndex = cursor_index(Cursor::Statements);
   if (!m_statementsMembers.empty()) {
      if (deferred) {
         m_layout[statementsIndex] = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::CodeBlockItemList, m_layout, m_context);
      } else {
         m_layout[statementsIndex] = recorder.recordRawSyntax(SyntaxKind::CodeBlockItemList, m_layout);
      }
   }
   if (m_layout[labelIndex].isNull()) {
      polar_unreachable("need missing non-token nodes ?");
   }
   if (m_layout[statementsIndex].isNull()) {
      if (deferred) {
         m_layout[statementsIndex] = ParsedRawSyntaxNode::makeDeferred(SyntaxKind::CodeBlockItemList, {}, m_context);
      } else {
         m_layout[statementsIndex] = recorder.recordRawSyntax(SyntaxKind::CodeBlockItemList, {});
      }
   }
}

} // polar::parser
