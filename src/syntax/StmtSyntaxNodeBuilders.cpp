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

#include "polarphp/syntax/builder/StmtSyntaxNodeBuilders.h"

namespace polar::syntax {

///
/// ConditionElementSyntaxBuilder
///
ConditionElementSyntaxBuilder &ConditionElementSyntaxBuilder::useCondition(Syntax condition)
{
   m_layout[cursor_index(Cursor::Condition)] = condition.getRaw();
   return *this;
}

ConditionElementSyntaxBuilder &ConditionElementSyntaxBuilder::useTrailingComma(TokenSyntax trailingComma)
{
   m_layout[cursor_index(Cursor::TrailingComma)] = trailingComma.getRaw();
   return *this;
}

ConditionElementSyntax ConditionElementSyntaxBuilder::build()
{
   CursorIndex conditionIndex = cursor_index(Cursor::Condition);
   CursorIndex trailingCommaIndex = cursor_index(Cursor::TrailingComma);
   if (!m_layout[conditionIndex]) {
      m_layout[conditionIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }
   if (!m_layout[trailingCommaIndex]) {
      m_layout[trailingCommaIndex] = RawSyntax::missing(TokenKindType::T_COMMA,
                                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_COMMA)));
   }
   RefCountPtr<RawSyntax> rawConditionElementSyntax = RawSyntax::make(SyntaxKind::ConditionElement, m_layout, SourcePresence::Present,
                                                                      m_arena);
   return make<ConditionElementSyntax>(rawConditionElementSyntax);
}


///
/// ConditionElementSyntaxBuilder
///

ContinueStmtSyntaxBuilder &ContinueStmtSyntaxBuilder::useContinueKeyword(TokenSyntax continueKeyword)
{
   m_layout[cursor_index(Cursor::ContinueKeyword)] = continueKeyword.getRaw();
   return *this;
}

ContinueStmtSyntaxBuilder &ContinueStmtSyntaxBuilder::useLNumberToken(TokenSyntax numberToken)
{
   m_layout[cursor_index(Cursor::LNumberToken)] = numberToken.getRaw();
   return *this;
}

ContinueStmtSyntax ContinueStmtSyntaxBuilder::build()
{
   CursorIndex continueKeywordIndex = cursor_index(Cursor::ContinueKeyword);
   CursorIndex numberTokenIndex = cursor_index(Cursor::LNumberToken);
   if (!m_layout[continueKeywordIndex]) {
      m_layout[continueKeywordIndex] = RawSyntax::missing(TokenKindType::T_CONTINUE,
                                                          OwnedString::makeUnowned(get_token_text(TokenKindType::T_CONTINUE)));
   }
   if (!m_layout[numberTokenIndex]) {
      m_layout[numberTokenIndex] = RawSyntax::missing(TokenKindType::T_LNUMBER,
                                                      OwnedString::makeUnowned(get_token_text(TokenKindType::T_LNUMBER)));
   }
   RefCountPtr<RawSyntax> rawContinueStmtSyntax = RawSyntax::make(SyntaxKind::ContinueStmt, m_layout, SourcePresence::Present,
                                                                  m_arena);
   return make<ContinueStmtSyntax>(rawContinueStmtSyntax);
}

///
/// BreakStmtSyntaxBuilder
///

BreakStmtSyntaxBuilder &BreakStmtSyntaxBuilder::useBreakKeyword(TokenSyntax breakKeyword)
{
   m_layout[cursor_index(Cursor::BreakKeyword)] = breakKeyword.getRaw();
   return *this;
}

BreakStmtSyntaxBuilder &BreakStmtSyntaxBuilder::useLNumberToken(TokenSyntax numberToken)
{
   m_layout[cursor_index(Cursor::LNumberToken)] = numberToken.getRaw();
   return *this;
}

BreakStmtSyntax BreakStmtSyntaxBuilder::build()
{
   CursorIndex breakKeywordIndex = cursor_index(Cursor::BreakKeyword);
   CursorIndex numberTokenIndex = cursor_index(Cursor::LNumberToken);
   if (!m_layout[breakKeywordIndex]) {
      m_layout[breakKeywordIndex] = RawSyntax::missing(TokenKindType::T_BREAK,
                                                       OwnedString::makeUnowned(get_token_text(TokenKindType::T_BREAK)));
   }
   if (!m_layout[numberTokenIndex]) {
      m_layout[numberTokenIndex] = RawSyntax::missing(TokenKindType::T_LNUMBER,
                                                      OwnedString::makeUnowned(get_token_text(TokenKindType::T_LNUMBER)));
   }
   RefCountPtr<RawSyntax> rawBreakStmtSyntax = RawSyntax::make(SyntaxKind::BreakStmt, m_layout, SourcePresence::Present,
                                                               m_arena);
   return make<BreakStmtSyntax>(rawBreakStmtSyntax);
}

///
/// FallthroughStmtSyntaxBuilder
///

FallthroughStmtSyntaxBuilder FallthroughStmtSyntaxBuilder::useFallthroughKeyword(TokenSyntax fallthroughKeyword)
{
   m_layout[cursor_index(Cursor::FallthroughKeyword)] = fallthroughKeyword.getRaw();
   return *this;
}

FallthroughStmtSyntax FallthroughStmtSyntaxBuilder::build()
{
   CursorIndex fallthroughKeywordIndex = cursor_index(Cursor::FallthroughKeyword);
   if (!m_layout[fallthroughKeywordIndex]) {
      m_layout[fallthroughKeywordIndex] = RawSyntax::missing(TokenKindType::T_FALLTHROUGH,
                                                             OwnedString::makeUnowned(get_token_text(TokenKindType::T_FALLTHROUGH)));
   }
   RefCountPtr<RawSyntax> rawFallthroughKeyword = RawSyntax::make(SyntaxKind::FallthroughStmt, m_layout, SourcePresence::Present,
                                                                  m_arena);
   return make<FallthroughStmtSyntax>(rawFallthroughKeyword);
}

} // polar::syntax
