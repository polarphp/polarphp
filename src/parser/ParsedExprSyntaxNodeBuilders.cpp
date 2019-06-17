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

} // polar::parser
