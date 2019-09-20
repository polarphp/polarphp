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
#include "polarphp/syntax/syntaxnode/ExprSyntaxNodes.h"

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
      m_layout[trailingCommaIndex] = make_missing_token(T_COMMA);
   }
   RefCountPtr<RawSyntax> rawConditionElementSyntax = RawSyntax::make(
            SyntaxKind::ConditionElement, m_layout, SourcePresence::Present,
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

ContinueStmtSyntaxBuilder &ContinueStmtSyntaxBuilder::useExpr(ExprSyntax expr)
{
   m_layout[cursor_index(Cursor::Expr)] = expr.getRaw();
   return *this;
}

ContinueStmtSyntaxBuilder &ContinueStmtSyntaxBuilder::useSemicolon(TokenSyntax semicolon)
{
   m_layout[cursor_index(Cursor::Semicolon)] = semicolon.getRaw();
   return *this;
}

ContinueStmtSyntax ContinueStmtSyntaxBuilder::build()
{
   CursorIndex continueKeywordIndex = cursor_index(Cursor::ContinueKeyword);
   CursorIndex exprIndex = cursor_index(Cursor::Expr);
   CursorIndex semicolonIndex = cursor_index(Cursor::Semicolon);
   if (!m_layout[continueKeywordIndex]) {
      m_layout[continueKeywordIndex] = make_missing_token(T_CONTINUE);
   }
   if (!m_layout[exprIndex]) {
      m_layout[exprIndex] = RawSyntax::missing(SyntaxKind::UnknownExpr);
   }
   if (!m_layout[semicolonIndex]) {
      m_layout[semicolonIndex] = make_missing_token(T_SEMICOLON);
   }
   RefCountPtr<RawSyntax> rawContinueStmtSyntax = RawSyntax::make(
            SyntaxKind::ContinueStmt, m_layout, SourcePresence::Present,
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

BreakStmtSyntaxBuilder &BreakStmtSyntaxBuilder::useExpr(ExprSyntax expr)
{
   m_layout[cursor_index(Cursor::Expr)] = expr.getRaw();
   return *this;
}

BreakStmtSyntaxBuilder &BreakStmtSyntaxBuilder::useSemicolon(TokenSyntax semicolon)
{
   m_layout[cursor_index(Cursor::Semicolon)] = semicolon.getRaw();
   return *this;
}

BreakStmtSyntax BreakStmtSyntaxBuilder::build()
{
   CursorIndex breakKeywordIndex = cursor_index(Cursor::BreakKeyword);
   CursorIndex semicolonIndex = cursor_index(Cursor::Semicolon);
   if (!m_layout[breakKeywordIndex]) {
      m_layout[breakKeywordIndex] = make_missing_token(T_BREAK);
   }
   if (!m_layout[semicolonIndex]) {
      m_layout[semicolonIndex] = make_missing_token(T_SEMICOLON);
   }
   RefCountPtr<RawSyntax> rawBreakStmtSyntax = RawSyntax::make(
            SyntaxKind::BreakStmt, m_layout, SourcePresence::Present,
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
      m_layout[fallthroughKeywordIndex] = make_missing_token(T_FALLTHROUGH);
   }
   RefCountPtr<RawSyntax> rawFallthroughKeyword = RawSyntax::make(
            SyntaxKind::FallthroughStmt, m_layout, SourcePresence::Present,
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

ElseIfClauseSyntaxBuilder &ElseIfClauseSyntaxBuilder::useBody(StmtSyntax body)
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
      m_layout[elseIfKeywordIndex] = make_missing_token(T_ELSEIF);
   }
   if (!m_layout[leftParenIndex]) {
      m_layout[leftParenIndex] = make_missing_token(T_LEFT_PAREN);
   }
   if (!m_layout[conditionIndex]) {
      m_layout[conditionIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }
   if (!m_layout[rightParenIndex]) {
      m_layout[rightParenIndex] = make_missing_token(T_RIGHT_PAREN);
   }
   if (!m_layout[bodyIndex]) {
      m_layout[bodyIndex] = RawSyntax::missing(SyntaxKind::Stmt);
   }
   RefCountPtr<RawSyntax> rawElseIfClauseSyntax = RawSyntax::make(
            SyntaxKind::ElseIfClause, m_layout, SourcePresence::Present,
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

IfStmtSyntaxBuilder &IfStmtSyntaxBuilder::useBody(StmtSyntax body)
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
      clauses = RawSyntax::make(
               SyntaxKind::ElseIfList, {elseIfClause.getRaw()}, SourcePresence::Present,
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
      m_layout[labelNameIndex] = RawSyntax::missing(TokenKindType::T_IDENTIFIER_STRING,
                                                    OwnedString::makeUnowned(""));
   }
   if (!m_layout[labelColonIndex]) {
      m_layout[labelColonIndex] = make_missing_token(T_COLON);
   }
   if (!m_layout[ifKeywordIndex]) {
      m_layout[ifKeywordIndex] = make_missing_token(T_IF);
   }
   if (!m_layout[leftParenIndex]) {
      m_layout[leftParenIndex] = make_missing_token(T_LEFT_PAREN);
   }
   if (!m_layout[conditionIndex]) {
      m_layout[conditionIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }
   if (!m_layout[rightParenIndex]) {
      m_layout[rightParenIndex] = make_missing_token(T_RIGHT_PAREN);
   }
   if (!m_layout[bodyIndex]) {
      m_layout[bodyIndex] = RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt);
   }
   if (!m_layout[elseIfClausesIndex]) {
      m_layout[elseIfClausesIndex] = RawSyntax::missing(SyntaxKind::ElseIfList);
   }
   if (!m_layout[elseKeywordIndex]) {
      m_layout[elseKeywordIndex] = make_missing_token(T_ELSEIF);
   }
   if (!m_layout[elseBodyIndex]) {
      m_layout[elseBodyIndex] = RawSyntax::missing(SyntaxKind::IfStmt);
   }
   RefCountPtr<RawSyntax> rawIfStmtSytax = RawSyntax::make(
            SyntaxKind::IfStmt, m_layout, SourcePresence::Present,
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

WhileStmtSyntaxBuilder &WhileStmtSyntaxBuilder::useConditions(ParenDecoratedExprSyntax conditions)
{
   m_layout[cursor_index(Cursor::ConditionsClause)] = conditions.getRaw();
   return *this;
}

WhileStmtSyntaxBuilder &WhileStmtSyntaxBuilder::useBody(StmtSyntax body)
{
   m_layout[cursor_index(Cursor::Body)] = body.getRaw();
   return *this;
}

WhileStmtSyntaxBuilder &WhileStmtSyntaxBuilder::addCondition(ConditionElementSyntax condition)
{
   RefCountPtr<RawSyntax> conditions = m_layout[cursor_index(Cursor::ConditionsClause)];
   if (!conditions) {
      conditions = RawSyntax::make(
               SyntaxKind::ConditionElementList, {condition.getRaw()}, SourcePresence::Present,
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
   CursorIndex conditionsIndex = cursor_index(Cursor::ConditionsClause);
   CursorIndex bodyIndex = cursor_index(Cursor::Body);
   if (!m_layout[labelNameIndex]) {
      m_layout[labelNameIndex] = RawSyntax::missing(TokenKindType::T_IDENTIFIER_STRING,
                                                    OwnedString::makeUnowned(""));
   }
   if (!m_layout[labelColonIndex]) {
      m_layout[labelColonIndex] = make_missing_token(T_COLON);
   }
   if (!m_layout[whileKeywordIndex]) {
      m_layout[whileKeywordIndex] = make_missing_token(T_WHILE);
   }
   if (!m_layout[conditionsIndex]) {
      m_layout[conditionsIndex] = RawSyntax::missing(SyntaxKind::ParenDecoratedExpr);
   }
   if (!m_layout[bodyIndex]) {
      m_layout[bodyIndex] = RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt);
   }
   RefCountPtr<RawSyntax> rawWhileStmt = RawSyntax::make(
            SyntaxKind::WhileStmt, m_layout, SourcePresence::Present,
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

DoWhileStmtSyntaxBuilder &DoWhileStmtSyntaxBuilder::useBody(StmtSyntax body)
{
   m_layout[cursor_index(Cursor::Body)] = body.getRaw();
   return *this;
}

DoWhileStmtSyntaxBuilder &DoWhileStmtSyntaxBuilder::useWhileKeyword(TokenSyntax whileKeyword)
{
   m_layout[cursor_index(Cursor::WhileKeyword)] = whileKeyword.getRaw();
   return *this;
}

DoWhileStmtSyntaxBuilder &DoWhileStmtSyntaxBuilder::useConditionsClause(ParenDecoratedExprSyntax condition)
{
   m_layout[cursor_index(Cursor::ConditionsClause)] = condition.getRaw();
   return *this;
}

DoWhileStmtSyntax DoWhileStmtSyntaxBuilder::build()
{
   CursorIndex labelColonIndex = cursor_index(Cursor::LabelColon);
   CursorIndex labelName = cursor_index(Cursor::LabelName);
   CursorIndex doKeywordIndex = cursor_index(Cursor::DoKeyword);
   CursorIndex bodyIndex = cursor_index(Cursor::Body);
   CursorIndex whileKeywordIndex = cursor_index(Cursor::WhileKeyword);
   CursorIndex conditionIndex = cursor_index(Cursor::ConditionsClause);
   if (!m_layout[labelColonIndex]) {
      m_layout[labelColonIndex] = make_missing_token(T_COLON);
   }
   if (!m_layout[labelName]) {
      m_layout[labelName] = RawSyntax::missing(TokenKindType::T_IDENTIFIER_STRING,
                                               OwnedString::makeUnowned(""));
   }
   if (!m_layout[doKeywordIndex]) {
      m_layout[doKeywordIndex] = make_missing_token(T_DO);
   }
   if (!m_layout[bodyIndex]) {
      m_layout[bodyIndex] = RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt);
   }
   if (!m_layout[whileKeywordIndex]) {
      m_layout[whileKeywordIndex] = make_missing_token(T_WHILE);
   }
   if (!m_layout[conditionIndex]) {
      m_layout[conditionIndex] = RawSyntax::missing(SyntaxKind::ParenDecoratedExpr);
   }
   RefCountPtr<RawSyntax> rawDoWhileStmtSyntax = RawSyntax::make(
            SyntaxKind::DoWhileStmt, m_layout, SourcePresence::Present,
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
      m_layout[defaultKeywordIndex] = make_missing_token(T_DEFAULT);
   }
   if (!m_layout[colonIndex]) {
      m_layout[colonIndex] = make_missing_token(T_COLON);
   }
   RefCountPtr<RawSyntax> rawSwitchDefaultLabelSyntax = RawSyntax::make(
            SyntaxKind::SwitchDefaultLabel, m_layout, SourcePresence::Present,
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
      m_layout[caseKeywordIndex] = make_missing_token(T_CASE);
   }
   if (!m_layout[exprIndex]) {
      m_layout[exprIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }
   if (!m_layout[colonIndex]) {
      m_layout[colonIndex] = make_missing_token(T_COLON);
   }
   RefCountPtr<RawSyntax> rawSwitchCaseLabelSyntax = RawSyntax::make(
            SyntaxKind::SwitchCaseLabel, m_layout, SourcePresence::Present,
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

SwitchCaseSyntaxBuilder &SwitchCaseSyntaxBuilder::useStatements(InnerStmtListSyntax statements)
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
      m_layout[statementsIndex] = RawSyntax::missing(SyntaxKind::InnerStmtList);
   }
   RefCountPtr<RawSyntax> rawSwitchCaseSyntax = RawSyntax::make(
            SyntaxKind::SwitchCase, m_layout, SourcePresence::Present,
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

SwitchStmtSyntaxBuilder &SwitchStmtSyntaxBuilder::useSwitchCaseListCaluse(SwitchCaseListClauseSyntax switchCaseListCaluse)
{
   m_layout[cursor_index(Cursor::SwitchCaseListClause)] = switchCaseListCaluse.getRaw();
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
   CursorIndex casesIndex = cursor_index(Cursor::SwitchCaseListClause);
   if (!m_layout[labelNameIndex]) {
      m_layout[labelNameIndex] = RawSyntax::missing(TokenKindType::T_IDENTIFIER_STRING,
                                                    OwnedString::makeUnowned(""));
   }

   if (!m_layout[labelColonIndex]) {
      m_layout[labelColonIndex] = make_missing_token(T_COLON);
   }

   if (!m_layout[switchKeywordIndex]) {
      m_layout[switchKeywordIndex] = make_missing_token(T_SWITCH);
   }

   if (!m_layout[leftParenIndex]) {
      m_layout[leftParenIndex] = make_missing_token(T_LEFT_PAREN);
   }

   if (!m_layout[conditionExprIndex]) {
      m_layout[conditionExprIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }

   if (!m_layout[rightParenIndex]) {
      m_layout[rightParenIndex] = make_missing_token(T_RIGHT_PAREN);
   }

   if (!m_layout[casesIndex]) {
      m_layout[casesIndex] = RawSyntax::missing(SyntaxKind::SwitchCaseListClause);
   }

   RefCountPtr<RawSyntax> rawSwitchStmtSyntax = RawSyntax::make(
            SyntaxKind::SwitchStmt, m_layout, SourcePresence::Present,
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

DeferStmtSyntaxBuilder &DeferStmtSyntaxBuilder::useBody(InnerCodeBlockStmtSyntax body)
{
   m_layout[cursor_index(Cursor::Body)] = body.getRaw();
   return *this;
}

DeferStmtSyntax DeferStmtSyntaxBuilder::build()
{
   CursorIndex deferKeywordIndex = cursor_index(Cursor::DeferKeyword);
   CursorIndex bodyIndex = cursor_index(Cursor::Body);
   if (!m_layout[deferKeywordIndex]) {
      m_layout[deferKeywordIndex] = make_missing_token(T_DEFER);
   }
   if (!m_layout[bodyIndex]) {
      m_layout[bodyIndex] = RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt);
   }
   RefCountPtr<RawSyntax> rawDeferStmtSyntax = RawSyntax::make(
            SyntaxKind::DeferStmt, m_layout, SourcePresence::Present,
            m_arena);
   return make<DeferStmtSyntax>(rawDeferStmtSyntax);
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
      m_layout[throwKeywordIndex] = make_missing_token(T_THROW);
   }

   if (!m_layout[exprIndex]) {
      m_layout[exprIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }

   RefCountPtr<RawSyntax> rawThrowStmtSyntax = RawSyntax::make(
            SyntaxKind::ThrowStmt, m_layout, SourcePresence::Present,
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
      m_layout[returnKeywordIndex] = make_missing_token(T_RETURN);
   }

   if (!m_layout[exprIndex]) {
      m_layout[exprIndex] = RawSyntax::missing(SyntaxKind::Expr);
   }
   RefCountPtr<RawSyntax> rawReturnStmtSyntax = RawSyntax::make(
            SyntaxKind::ThrowStmt, m_layout, SourcePresence::Present,
            m_arena);
   return make<ReturnStmtSyntax>(rawReturnStmtSyntax);
}

} // polar::syntax
