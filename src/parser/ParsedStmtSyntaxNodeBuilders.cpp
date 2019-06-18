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

} // polar::parser
