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

FallthroughStmtSyntaxBuilder &FallthroughStmtSyntaxBuilder::useFallthroughKeyword(TokenSyntax fallthroughKeyword)
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

///
/// FallthroughStmtSyntaxBuilder
///

ElseIfClauseSyntaxBuilder &ElseIfClauseSyntaxBuilder::useElseIfKeyword(TokenSyntax elseIfKeyword)
{
   m_layout[cursor_index(Cursor::ElseIfKeyword)] = elseIfKeyword.getRaw();
   return *this;
}

ElseIfClauseSyntaxBuilder &ElseIfClauseSyntaxBuilder::useLeftParen(TokenSyntax leftParen)
{
   m_layout[cursor_index(Cursor::LeftParen)] = leftParen.getRaw();
   return *this;
}

ElseIfClauseSyntaxBuilder &ElseIfClauseSyntaxBuilder::useCondition(ExprSyntax condition)
{
   m_layout[cursor_index(Cursor::Condition)] = condition.getRaw();
   return *this;
}

ElseIfClauseSyntaxBuilder &ElseIfClauseSyntaxBuilder::useRightParen(TokenSyntax rightParen)
{
   m_layout[cursor_index(Cursor::RightParen)] = rightParen.getRaw();
   return *this;
}

ElseIfClauseSyntaxBuilder &ElseIfClauseSyntaxBuilder::useBody(CodeBlockSyntax body)
{
   m_layout[cursor_index(Cursor::Body)] = body.getRaw();
   return *this;
}

ElseIfClauseSyntax ElseIfClauseSyntaxBuilder::build()
{
   CursorIndex elseIfKeywordIndex = cursor_index(Cursor::ElseIfKeyword);
   CursorIndex leftParenIndex = cursor_index(Cursor::LeftParen);
   CursorIndex conditionIndex = cursor_index(Cursor::Condition);
   CursorIndex rightParenIndex = cursor_index(Cursor::RightParen);
   CursorIndex bodyIndex = cursor_index(Cursor::Body);
   if (!m_layout[elseIfKeywordIndex]) {
      m_layout[elseIfKeywordIndex] = RawSyntax::missing(TokenKindType::T_ELSEIF,
                                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_ELSEIF)));
   }
   if (!m_layout[leftParenIndex]) {
      m_layout[leftParenIndex] = RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
                                                    OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN)));
   }
   if (!m_layout[conditionIndex]) {
      m_layout[conditionIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }
   if (!m_layout[rightParenIndex]) {
      m_layout[rightParenIndex] = RawSyntax::missing(TokenKindType::T_RIGHT_PAREN,
                                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN)));
   }
   if (!m_layout[bodyIndex]) {
      m_layout[bodyIndex] = RawSyntax::missing(SyntaxKind::CodeBlock);
   }
   RefCountPtr<RawSyntax> rawElseIfClauseSyntax = RawSyntax::make(SyntaxKind::ElseIfClause, m_layout, SourcePresence::Present,
                                                                  m_arena);
   return make<ElseIfClauseSyntax>(rawElseIfClauseSyntax);
}

///
/// IfStmtSyntaxBuilder
///

IfStmtSyntaxBuilder &IfStmtSyntaxBuilder::useLabelName(TokenSyntax labelName)
{
   m_layout[cursor_index(Cursor::LabelName)] = labelName.getRaw();
   return *this;
}

IfStmtSyntaxBuilder &IfStmtSyntaxBuilder::useLabelColon(TokenSyntax labelColon)
{
   m_layout[cursor_index(Cursor::LabelColon)] = labelColon.getRaw();
   return *this;
}

IfStmtSyntaxBuilder &IfStmtSyntaxBuilder::useIfKeyword(TokenSyntax ifKeyword)
{
   m_layout[cursor_index(Cursor::IfKeyword)] = ifKeyword.getRaw();
   return *this;
}

IfStmtSyntaxBuilder &IfStmtSyntaxBuilder::useLeftParen(TokenSyntax leftParen)
{
   m_layout[cursor_index(Cursor::LeftParen)] = leftParen.getRaw();
   return *this;
}

IfStmtSyntaxBuilder &IfStmtSyntaxBuilder::useCondition(ExprSyntax condition)
{
   m_layout[cursor_index(Cursor::Condition)] = condition.getRaw();
   return *this;
}

IfStmtSyntaxBuilder &IfStmtSyntaxBuilder::useRightParen(TokenSyntax rightParen)
{
   m_layout[cursor_index(Cursor::RightParen)] = rightParen.getRaw();
   return *this;
}

IfStmtSyntaxBuilder &IfStmtSyntaxBuilder::useBody(CodeBlockSyntax body)
{
   m_layout[cursor_index(Cursor::Body)] = body.getRaw();
   return *this;
}

IfStmtSyntaxBuilder &IfStmtSyntaxBuilder::useElseIfClauses(ElseIfListSyntax elseIfClauses)
{
   m_layout[cursor_index(Cursor::ElseIfClauses)] = elseIfClauses.getRaw();
   return *this;
}

IfStmtSyntaxBuilder &IfStmtSyntaxBuilder::useElseKeyword(TokenSyntax elseKeyword)
{
   m_layout[cursor_index(Cursor::ElseKeyword)] = elseKeyword.getRaw();
   return *this;
}

IfStmtSyntaxBuilder &IfStmtSyntaxBuilder::useElseBody(Syntax elseBody)
{
   m_layout[cursor_index(Cursor::ElseBody)] = elseBody.getRaw();
   return *this;
}

IfStmtSyntaxBuilder &IfStmtSyntaxBuilder::addElseIfClause(ElseIfClauseSyntax elseIfClause)
{
   RefCountPtr<RawSyntax> clauses = m_layout[cursor_index(Cursor::ElseIfClauses)];
   if (!clauses) {
      clauses = RawSyntax::make(SyntaxKind::ElseIfList, {elseIfClause.getRaw()}, SourcePresence::Present,
                                m_arena);
   } else {
      clauses = clauses->append(elseIfClause.getRaw());
   }
   return *this;
}

IfStmtSyntax IfStmtSyntaxBuilder::build()
{
   CursorIndex labelNameIndex = cursor_index(Cursor::LabelName);
   CursorIndex labelColonIndex = cursor_index(Cursor::LabelColon);
   CursorIndex ifKeywordIndex = cursor_index(Cursor::IfKeyword);
   CursorIndex leftParenIndex = cursor_index(Cursor::LeftParen);
   CursorIndex conditionIndex = cursor_index(Cursor::Condition);
   CursorIndex rightParenIndex = cursor_index(Cursor::RightParen);
   CursorIndex bodyIndex = cursor_index(Cursor::Body);
   CursorIndex elseIfClausesIndex = cursor_index(Cursor::ElseIfClauses);
   CursorIndex elseKeywordIndex = cursor_index(Cursor::ElseKeyword);
   CursorIndex elseBodyIndex = cursor_index(Cursor::ElseBody);
   if (!m_layout[labelNameIndex]) {
      m_layout[labelNameIndex] = RawSyntax::missing(TokenKindType::T_STRING,
                                                    OwnedString::makeUnowned(""));
   }
   if (!m_layout[labelColonIndex]) {
      m_layout[labelColonIndex] = RawSyntax::missing(TokenKindType::T_COLON,
                                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON)));
   }
   if (!m_layout[ifKeywordIndex]) {
      m_layout[ifKeywordIndex] = RawSyntax::missing(TokenKindType::T_IF,
                                                    OwnedString::makeUnowned(get_token_text(TokenKindType::T_IF)));
   }
   if (!m_layout[leftParenIndex]) {
      m_layout[leftParenIndex] = RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
                                                    OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN)));
   }
   if (!m_layout[conditionIndex]) {
      m_layout[conditionIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }
   if (!m_layout[rightParenIndex]) {
      m_layout[rightParenIndex] = RawSyntax::missing(TokenKindType::T_RIGHT_PAREN,
                                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN)));
   }
   if (!m_layout[bodyIndex]) {
      m_layout[bodyIndex] = RawSyntax::missing(SyntaxKind::CodeBlock);
   }
   if (!m_layout[elseIfClausesIndex]) {
      m_layout[elseIfClausesIndex] = RawSyntax::missing(SyntaxKind::ElseIfList);
   }
   if (!m_layout[elseKeywordIndex]) {
      m_layout[elseKeywordIndex] = RawSyntax::missing(TokenKindType::T_ELSEIF,
                                                      OwnedString::makeUnowned(get_token_text(TokenKindType::T_ELSEIF)));
   }
   if (!m_layout[elseBodyIndex]) {
      m_layout[elseBodyIndex] = RawSyntax::missing(SyntaxKind::IfStmt);
   }
   RefCountPtr<RawSyntax> rawIfStmtSytax = RawSyntax::make(SyntaxKind::IfStmt, m_layout, SourcePresence::Present,
                                                           m_arena);
   return make<IfStmtSyntax>(rawIfStmtSytax);
}

///
/// WhileStmtSyntaxBuilder
///

WhileStmtSyntaxBuilder &WhileStmtSyntaxBuilder::useLabelName(TokenSyntax labelName)
{
   m_layout[cursor_index(Cursor::LabelName)] = labelName.getRaw();
   return *this;
}

WhileStmtSyntaxBuilder &WhileStmtSyntaxBuilder::useLabelColon(TokenSyntax labelColon)
{
   m_layout[cursor_index(Cursor::LabelColon)] = labelColon.getRaw();
   return *this;
}

WhileStmtSyntaxBuilder &WhileStmtSyntaxBuilder::useWhileKeyword(TokenSyntax whileKeyword)
{
   m_layout[cursor_index(Cursor::WhileKeyword)] = whileKeyword.getRaw();
   return *this;
}

WhileStmtSyntaxBuilder &WhileStmtSyntaxBuilder::useLeftParen(TokenSyntax leftParen)
{
   m_layout[cursor_index(Cursor::LeftParen)] = leftParen.getRaw();
   return *this;
}

WhileStmtSyntaxBuilder &WhileStmtSyntaxBuilder::useConditions(ConditionElementListSyntax conditions)
{
   m_layout[cursor_index(Cursor::Conditions)] = conditions.getRaw();
   return *this;
}

WhileStmtSyntaxBuilder &WhileStmtSyntaxBuilder::useRightParen(TokenSyntax rightParen)
{
   m_layout[cursor_index(Cursor::RightParen)] = rightParen.getRaw();
   return *this;
}

WhileStmtSyntaxBuilder &WhileStmtSyntaxBuilder::useBody(CodeBlockSyntax body)
{
   m_layout[cursor_index(Cursor::Body)] = body.getRaw();
   return *this;
}

WhileStmtSyntaxBuilder &WhileStmtSyntaxBuilder::addCondition(ConditionElementSyntax condition)
{
   RefCountPtr<RawSyntax> conditions = m_layout[cursor_index(Cursor::Conditions)];
   if (!conditions) {
      conditions = RawSyntax::make(SyntaxKind::ConditionElementList, {condition.getRaw()}, SourcePresence::Present,
                                   m_arena);
   } else {
      conditions = conditions->append(condition.getRaw());
   }
   return *this;
}

WhileStmtSyntax WhileStmtSyntaxBuilder::build()
{
   CursorIndex labelNameIndex = cursor_index(Cursor::LabelName);
   CursorIndex labelColonIndex = cursor_index(Cursor::LabelColon);
   CursorIndex whileKeywordIndex = cursor_index(Cursor::WhileKeyword);
   CursorIndex leftParenIndex = cursor_index(Cursor::LeftParen);
   CursorIndex conditionsIndex = cursor_index(Cursor::Conditions);
   CursorIndex rightParenIndex = cursor_index(Cursor::RightParen);
   CursorIndex bodyIndex = cursor_index(Cursor::Body);
   if (!m_layout[labelNameIndex]) {
      m_layout[labelNameIndex] = RawSyntax::missing(TokenKindType::T_STRING,
                                                    OwnedString::makeUnowned(""));
   }
   if (!m_layout[labelColonIndex]) {
      m_layout[labelColonIndex] = RawSyntax::missing(TokenKindType::T_COLON,
                                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON)));
   }
   if (!m_layout[whileKeywordIndex]) {
      m_layout[whileKeywordIndex] = RawSyntax::missing(TokenKindType::T_WHILE,
                                                       OwnedString::makeUnowned(get_token_text(TokenKindType::T_WHILE)));
   }
   if (!m_layout[leftParenIndex]) {
      m_layout[leftParenIndex] = RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
                                                    OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN)));
   }
   if (!m_layout[conditionsIndex]) {
      m_layout[conditionsIndex] = RawSyntax::missing(SyntaxKind::ConditionElementList);
   }
   if (!m_layout[rightParenIndex]) {
      m_layout[rightParenIndex] = RawSyntax::missing(TokenKindType::T_RIGHT_PAREN,
                                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN)));
   }
   if (!m_layout[bodyIndex]) {
      m_layout[bodyIndex] = RawSyntax::missing(SyntaxKind::CodeBlock);
   }
   RefCountPtr<RawSyntax> rawWhileStmt = RawSyntax::make(SyntaxKind::WhileStmt, m_layout, SourcePresence::Present,
                                                         m_arena);
   return make<WhileStmtSyntax>(rawWhileStmt);
}

///
/// DoWhileStmtSyntaxBuilder
///
DoWhileStmtSyntaxBuilder &DoWhileStmtSyntaxBuilder::useLabelName(TokenSyntax labelName)
{
   m_layout[cursor_index(Cursor::LabelName)] = labelName.getRaw();
   return *this;
}

DoWhileStmtSyntaxBuilder &DoWhileStmtSyntaxBuilder::useLabelColon(TokenSyntax labelColon)
{
   m_layout[cursor_index(Cursor::LabelColon)] = labelColon.getRaw();
   return *this;
}

DoWhileStmtSyntaxBuilder &DoWhileStmtSyntaxBuilder::useDoKeyword(TokenSyntax doKeyword)
{
   m_layout[cursor_index(Cursor::DoKeyword)] = doKeyword.getRaw();
   return *this;
}

DoWhileStmtSyntaxBuilder &DoWhileStmtSyntaxBuilder::useBody(CodeBlockSyntax body)
{
   m_layout[cursor_index(Cursor::Body)] = body.getRaw();
   return *this;
}

DoWhileStmtSyntaxBuilder &DoWhileStmtSyntaxBuilder::useWhileKeyword(TokenSyntax whileKeyword)
{
   m_layout[cursor_index(Cursor::WhileKeyword)] = whileKeyword.getRaw();
   return *this;
}

DoWhileStmtSyntaxBuilder &DoWhileStmtSyntaxBuilder::useLeftParen(TokenSyntax leftParen)
{
   m_layout[cursor_index(Cursor::LeftParen)] = leftParen.getRaw();
   return *this;
}

DoWhileStmtSyntaxBuilder &DoWhileStmtSyntaxBuilder::useCondition(ExprSyntax condition)
{
   m_layout[cursor_index(Cursor::Condition)] = condition.getRaw();
   return *this;
}

DoWhileStmtSyntaxBuilder &DoWhileStmtSyntaxBuilder::useRightParen(TokenSyntax rightParen)
{
   m_layout[cursor_index(Cursor::RightParen)] = rightParen.getRaw();
   return *this;
}

DoWhileStmtSyntax DoWhileStmtSyntaxBuilder::build()
{
   CursorIndex labelColonIndex = cursor_index(Cursor::LabelColon);
   CursorIndex labelName = cursor_index(Cursor::LabelName);
   CursorIndex doKeywordIndex = cursor_index(Cursor::DoKeyword);
   CursorIndex bodyIndex = cursor_index(Cursor::Body);
   CursorIndex whileKeywordIndex = cursor_index(Cursor::WhileKeyword);
   CursorIndex leftParenIndex = cursor_index(Cursor::LeftParen);
   CursorIndex conditionIndex = cursor_index(Cursor::Condition);
   CursorIndex rightParenIndex =  cursor_index(Cursor::RightParen);
   if (!m_layout[labelColonIndex]) {
      m_layout[labelColonIndex] = RawSyntax::missing(TokenKindType::T_COLON,
                                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON)));
   }
   if (!m_layout[labelName]) {
      m_layout[labelName] = RawSyntax::missing(TokenKindType::T_STRING,
                                               OwnedString::makeUnowned(""));
   }
   if (!m_layout[doKeywordIndex]) {
      m_layout[doKeywordIndex] = RawSyntax::missing(TokenKindType::T_DO,
                                                    OwnedString::makeUnowned(get_token_text(TokenKindType::T_DO)));
   }
   if (!m_layout[bodyIndex]) {
      m_layout[bodyIndex] = RawSyntax::missing(SyntaxKind::CodeBlock);
   }
   if (!m_layout[whileKeywordIndex]) {
      m_layout[whileKeywordIndex] = RawSyntax::missing(TokenKindType::T_WHILE,
                                                       OwnedString::makeUnowned(get_token_text(TokenKindType::T_WHILE)));
   }

   if (!m_layout[leftParenIndex]) {
      m_layout[leftParenIndex] = RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
                                                    OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN)));
   }

   if (!m_layout[conditionIndex]) {
      m_layout[conditionIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }

   if (!m_layout[rightParenIndex]) {
      m_layout[rightParenIndex] = RawSyntax::missing(TokenKindType::T_RIGHT_PAREN,
                                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN)));
   }
   RefCountPtr<RawSyntax> rawDoWhileStmtSyntax = RawSyntax::make(SyntaxKind::DoWhileStmt, m_layout, SourcePresence::Present,
                                                                 m_arena);
   return make<DoWhileStmtSyntax>(rawDoWhileStmtSyntax);
}


///
/// SwitchDefaultLabelSyntaxBuilder
///

SwitchDefaultLabelSyntaxBuilder &SwitchDefaultLabelSyntaxBuilder::useDefaultKeyword(TokenSyntax defaultKeyword)
{
   m_layout[cursor_index(Cursor::DefaultKeyword)] = defaultKeyword.getRaw();
   return *this;
}

SwitchDefaultLabelSyntaxBuilder &SwitchDefaultLabelSyntaxBuilder::useColon(TokenSyntax colon)
{
   m_layout[cursor_index(Cursor::Colon)] = colon.getRaw();
   return *this;
}

SwitchDefaultLabelSyntax SwitchDefaultLabelSyntaxBuilder::build()
{
   CursorIndex defaultKeywordIndex = cursor_index(Cursor::DefaultKeyword);
   CursorIndex colonIndex = cursor_index(Cursor::Colon);
   if (!m_layout[defaultKeywordIndex]) {
      m_layout[defaultKeywordIndex] = RawSyntax::missing(TokenKindType::T_DEFAULT,
                                                         OwnedString::makeUnowned(get_token_text(TokenKindType::T_DEFAULT)));
   }
   if (!m_layout[colonIndex]) {
      m_layout[colonIndex] = RawSyntax::missing(TokenKindType::T_COLON,
                                                OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON)));
   }
   RefCountPtr<RawSyntax> rawSwitchDefaultLabelSyntax = RawSyntax::make(SyntaxKind::SwitchDefaultLabel, m_layout, SourcePresence::Present,
                                                                        m_arena);
   return make<SwitchDefaultLabelSyntax>(rawSwitchDefaultLabelSyntax);
}

///
/// SwitchDefaultLabelSyntaxBuilder
///

SwitchCaseLabelSyntaxBuilder &SwitchCaseLabelSyntaxBuilder::useCaseKeyword(TokenSyntax caseKeyword)
{
   m_layout[cursor_index(Cursor::CaseKeyword)] = caseKeyword.getRaw();
   return *this;
}

SwitchCaseLabelSyntaxBuilder &SwitchCaseLabelSyntaxBuilder::useExpr(ExprSyntax expr)
{
   m_layout[cursor_index(Cursor::Expr)] = expr.getRaw();
   return *this;
}

SwitchCaseLabelSyntaxBuilder &SwitchCaseLabelSyntaxBuilder::useColon(TokenSyntax colon)
{
   m_layout[cursor_index(Cursor::Colon)] = colon.getRaw();
   return *this;
}

SwitchCaseLabelSyntax SwitchCaseLabelSyntaxBuilder::build()
{
   CursorIndex caseKeywordIndex = cursor_index(Cursor::CaseKeyword);
   CursorIndex exprIndex = cursor_index(Cursor::Expr);
   CursorIndex colonIndex = cursor_index(Cursor::Colon);
   if (!m_layout[caseKeywordIndex]) {
      m_layout[caseKeywordIndex] = RawSyntax::missing(TokenKindType::T_CASE,
                                                      OwnedString::makeUnowned(get_token_text(TokenKindType::T_CASE)));
   }
   if (!m_layout[exprIndex]) {
      m_layout[exprIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }
   if (!m_layout[colonIndex]) {
      m_layout[colonIndex] = RawSyntax::missing(TokenKindType::T_COLON,
                                                OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON)));
   }
   RefCountPtr<RawSyntax> rawSwitchCaseLabelSyntax = RawSyntax::make(SyntaxKind::SwitchCaseLabel, m_layout, SourcePresence::Present,
                                                                     m_arena);
   return make<SwitchCaseLabelSyntax>(rawSwitchCaseLabelSyntax);
}

///
/// SwitchDefaultLabelSyntaxBuilder
///
SwitchCaseSyntaxBuilder &SwitchCaseSyntaxBuilder::useLabel(Syntax label)
{
   m_layout[cursor_index(Cursor::Label)] = label.getRaw();
   return *this;
}

SwitchCaseSyntaxBuilder &SwitchCaseSyntaxBuilder::useStatements(CodeBlockItemListSyntax statements)
{
   m_layout[cursor_index(Cursor::Statements)] = statements.getRaw();
   return *this;
}

SwitchCaseSyntax SwitchCaseSyntaxBuilder::build()
{
   CursorIndex labelIndex = cursor_index(Cursor::Label);
   CursorIndex statementsIndex = cursor_index(Cursor::Statements);
   if (!m_layout[labelIndex]) {
      m_layout[labelIndex] = RawSyntax::missing(SyntaxKind::SwitchDefaultLabel);
   }
   if (!m_layout[statementsIndex]) {
      m_layout[statementsIndex] = RawSyntax::missing(SyntaxKind::CodeBlockItemList);
   }
   RefCountPtr<RawSyntax> rawSwitchCaseSyntax = RawSyntax::make(SyntaxKind::SwitchCase, m_layout, SourcePresence::Present,
                                                                m_arena);
   return make<SwitchCaseSyntax>(rawSwitchCaseSyntax);
}

///
/// SwitchStmtSyntaxBuilder
///

SwitchStmtSyntaxBuilder &SwitchStmtSyntaxBuilder::useLabelName(TokenSyntax labelName)
{
   m_layout[cursor_index(Cursor::LabelName)] = labelName.getRaw();
   return *this;
}

SwitchStmtSyntaxBuilder &SwitchStmtSyntaxBuilder::useLabelColon(TokenSyntax colon)
{
   m_layout[cursor_index(Cursor::LabelColon)] = colon.getRaw();
   return *this;
}

SwitchStmtSyntaxBuilder &SwitchStmtSyntaxBuilder::useSwitchKeyword(TokenSyntax switchKeyword)
{
   m_layout[cursor_index(Cursor::SwitchKeyword)] = switchKeyword.getRaw();
   return *this;
}

SwitchStmtSyntaxBuilder &SwitchStmtSyntaxBuilder::useLeftParen(TokenSyntax leftParen)
{
   m_layout[cursor_index(Cursor::LeftParen)] = leftParen.getRaw();
   return *this;
}

SwitchStmtSyntaxBuilder &SwitchStmtSyntaxBuilder::useConditionExpr(ExprSyntax condition)
{
   m_layout[cursor_index(Cursor::ConditionExpr)] = condition.getRaw();
   return *this;
}

SwitchStmtSyntaxBuilder &SwitchStmtSyntaxBuilder::useRightParen(TokenSyntax rightParen)
{
   m_layout[cursor_index(Cursor::RightParen)] = rightParen.getRaw();
   return *this;
}

SwitchStmtSyntaxBuilder &SwitchStmtSyntaxBuilder::useLeftBrace(TokenSyntax leftBrace)
{
   m_layout[cursor_index(Cursor::LeftBrace)] = leftBrace.getRaw();
   return *this;
}

SwitchStmtSyntaxBuilder &SwitchStmtSyntaxBuilder::useCases(SwitchCaseListSyntax cases)
{
   m_layout[cursor_index(Cursor::Cases)] = cases.getRaw();
   return *this;
}

SwitchStmtSyntaxBuilder &SwitchStmtSyntaxBuilder::addCase(SwitchCaseSyntax caseItem)
{
   RefCountPtr<RawSyntax> cases = m_layout[Cursor::Cases];
   if (!cases) {
      cases = RawSyntax::make(SyntaxKind::SwitchCaseList, {caseItem.getRaw()}, SourcePresence::Present,
                              m_arena);
   } else {
      cases = cases->append(caseItem.getRaw());
   }
   return *this;
}

SwitchStmtSyntaxBuilder &SwitchStmtSyntaxBuilder::useRightBrace(TokenSyntax rightBrace)
{
   m_layout[cursor_index(Cursor::RightBrace)] = rightBrace.getRaw();
   return *this;
}

SwitchStmtSyntax SwitchStmtSyntaxBuilder::build()
{
   CursorIndex labelNameIndex = cursor_index(Cursor::LabelName);
   CursorIndex labelColonIndex = cursor_index(Cursor::LabelColon);
   CursorIndex switchKeywordIndex = cursor_index(Cursor::SwitchKeyword);
   CursorIndex leftParenIndex = cursor_index(Cursor::LeftParen);
   CursorIndex conditionExprIndex = cursor_index(Cursor::ConditionExpr);
   CursorIndex rightParenIndex = cursor_index(Cursor::RightParen);
   CursorIndex leftBraceIndex = cursor_index(Cursor::LeftBrace);
   CursorIndex casesIndex = cursor_index(Cursor::Cases);
   CursorIndex rightBraceIndex = cursor_index(Cursor::RightBrace);
   if (!m_layout[labelNameIndex]) {
      m_layout[labelNameIndex] = RawSyntax::missing(TokenKindType::T_STRING,
                                                    OwnedString::makeUnowned(""));
   }

   if (!m_layout[labelColonIndex]) {
      m_layout[labelColonIndex] = RawSyntax::missing(TokenKindType::T_COLON,
                                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON)));
   }

   if (!m_layout[switchKeywordIndex]) {
      m_layout[switchKeywordIndex] = RawSyntax::missing(TokenKindType::T_SWITCH,
                                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_SWITCH)));
   }

   if (!m_layout[leftParenIndex]) {
      m_layout[leftParenIndex] = RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
                                                    OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN)));
   }

   if (!m_layout[conditionExprIndex]) {
      m_layout[conditionExprIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }

   if (!m_layout[rightParenIndex]) {
      m_layout[rightParenIndex] = RawSyntax::missing(TokenKindType::T_RIGHT_PAREN,
                                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN)));
   }

   if (!m_layout[leftBraceIndex]) {
      m_layout[leftBraceIndex] = RawSyntax::missing(TokenKindType::T_LEFT_BRACE,
                                                    OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_BRACE)));
   }

   if (!m_layout[casesIndex]) {
      m_layout[casesIndex] = RawSyntax::missing(SyntaxKind::SwitchCaseList);
   }

   if (!m_layout[rightBraceIndex]) {
      m_layout[rightBraceIndex] = RawSyntax::missing(TokenKindType::T_RIGHT_BRACE,
                                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_BRACE)));
   }

   RefCountPtr<RawSyntax> rawSwitchStmtSyntax = RawSyntax::make(SyntaxKind::SwitchStmt, m_layout, SourcePresence::Present,
                                                                m_arena);
   return make<SwitchStmtSyntax>(rawSwitchStmtSyntax);
}

///
/// DeferStmtSyntaxBuilder
///

DeferStmtSyntaxBuilder &DeferStmtSyntaxBuilder::useDeferKeyword(TokenSyntax deferKeyword)
{
   m_layout[cursor_index(Cursor::DeferKeyword)] = deferKeyword.getRaw();
   return *this;
}

DeferStmtSyntaxBuilder &DeferStmtSyntaxBuilder::useBody(CodeBlockSyntax body)
{
   m_layout[cursor_index(Cursor::Body)] = body.getRaw();
   return *this;
}

DeferStmtSyntax DeferStmtSyntaxBuilder::build()
{
   CursorIndex deferKeywordIndex = cursor_index(Cursor::DeferKeyword);
   CursorIndex bodyIndex = cursor_index(Cursor::Body);
   if (!m_layout[deferKeywordIndex]) {
      m_layout[deferKeywordIndex] = RawSyntax::missing(TokenKindType::T_DEFER,
                                                       OwnedString::makeUnowned(get_token_text(TokenKindType::T_DEFER)));
   }
   if (!m_layout[bodyIndex]) {
      m_layout[bodyIndex] = RawSyntax::missing(SyntaxKind::CodeBlock);
   }
   RefCountPtr<RawSyntax> rawDeferStmtSyntax = RawSyntax::make(SyntaxKind::DeferStmt, m_layout, SourcePresence::Present,
                                                               m_arena);
   return make<DeferStmtSyntax>(rawDeferStmtSyntax);
}

///
/// ExpressionStmtSyntaxBuilder
///
ExpressionStmtSyntaxBuilder &ExpressionStmtSyntaxBuilder::useExpr(ExprSyntax expr)
{
   m_layout[cursor_index(Cursor::Expr)] = expr.getRaw();
   return *this;
}

ExpressionStmtSyntax ExpressionStmtSyntaxBuilder::build()
{
   CursorIndex exprIndex = cursor_index(Cursor::Expr);
   if (!m_layout[exprIndex]) {
      m_layout[exprIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }
   RefCountPtr<RawSyntax> rawExpressionStmtSyntax = RawSyntax::make(SyntaxKind::ExpressionStmt, m_layout, SourcePresence::Present,
                                                                    m_arena);
   return make<ExpressionStmtSyntax>(rawExpressionStmtSyntax);
}

///
/// ThrowStmtSyntaxBuilder
///

ThrowStmtSyntaxBuilder &ThrowStmtSyntaxBuilder::useThrowKeyword(TokenSyntax throwKeyword)
{
   m_layout[cursor_index(Cursor::ThrowKeyword)] = throwKeyword.getRaw();
   return *this;
}

ThrowStmtSyntaxBuilder &ThrowStmtSyntaxBuilder::useExpr(ExprSyntax expr)
{
   m_layout[cursor_index(Cursor::Expr)] = expr.getRaw();
   return *this;
}

ThrowStmtSyntax ThrowStmtSyntaxBuilder::build()
{
   CursorIndex throwKeywordIndex = cursor_index(Cursor::ThrowKeyword);
   CursorIndex exprIndex = cursor_index(Cursor::Expr);

   if (!m_layout[throwKeywordIndex]) {
      m_layout[throwKeywordIndex] = RawSyntax::missing(TokenKindType::T_THROW,
                                                       OwnedString::makeUnowned(get_token_text(TokenKindType::T_THROW)));
   }

   if (!m_layout[exprIndex]) {
      m_layout[exprIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }

   RefCountPtr<RawSyntax> rawThrowStmtSyntax = RawSyntax::make(SyntaxKind::ThrowStmt, m_layout, SourcePresence::Present,
                                                               m_arena);
   return make<ThrowStmtSyntax>(rawThrowStmtSyntax);
}

///
/// ReturnStmtSyntaxBuilder
///
ReturnStmtSyntaxBuilder &ReturnStmtSyntaxBuilder::useReturnKeyword(TokenSyntax returnKeyword)
{
   m_layout[cursor_index(Cursor::ReturnKeyword)] = returnKeyword.getRaw();
   return *this;
}

ReturnStmtSyntaxBuilder &ReturnStmtSyntaxBuilder::useExpr(ExprSyntax expr)
{
   m_layout[cursor_index(Cursor::Expr)] = expr.getRaw();
   return *this;
}

ReturnStmtSyntax ReturnStmtSyntaxBuilder::build()
{
   CursorIndex returnKeywordIndex = cursor_index(Cursor::ReturnKeyword);
   CursorIndex exprIndex = cursor_index(Cursor::Expr);

   if (!m_layout[returnKeywordIndex]) {
      m_layout[returnKeywordIndex] = RawSyntax::missing(TokenKindType::T_RETURN,
                                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_RETURN)));
   }

   if (!m_layout[exprIndex]) {
      m_layout[exprIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }
   RefCountPtr<RawSyntax> rawReturnStmtSyntax = RawSyntax::make(SyntaxKind::ThrowStmt, m_layout, SourcePresence::Present,
                                                                m_arena);
   return make<ReturnStmtSyntax>(rawReturnStmtSyntax);
}

} // polar::syntax
