// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/16.

#include "polarphp/parser/parsedsyntaxnode/ParsedStmtSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/StmtSyntaxNodes.h"

namespace polar::parser {

using namespace polar::syntax;

ParsedSyntax ParsedConditionElementSyntax::getDeferredCondition()
{
   return ParsedSyntax{getRaw().getDeferredChildren()[ConditionElementSyntax::Cursor::Condition]};
}

std::optional<ParsedTokenSyntax> ParsedConditionElementSyntax::getDeferredTrailingComma()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[ConditionElementSyntax::Cursor::TrailingComma];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedTokenSyntax{rawChild};
}

///
/// ParsedContinueStmtSyntax
///
ParsedTokenSyntax ParsedContinueStmtSyntax::getDeferredContinueKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[ContinueStmtSyntax::Cursor::ContinueKeyword]};
}

std::optional<ParsedTokenSyntax> ParsedContinueStmtSyntax::getDeferredLNumberToken()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[ContinueStmtSyntax::Cursor::LNumberToken];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedTokenSyntax{rawChild};
}

///
/// ParsedBreakStmtSyntax
///
ParsedTokenSyntax ParsedBreakStmtSyntax::getDeferredBreakKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[BreakStmtSyntax::Cursor::BreakKeyword]};
}

std::optional<ParsedTokenSyntax> ParsedBreakStmtSyntax::getDeferredLNumberToken()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[BreakStmtSyntax::Cursor::LNumberToken];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedTokenSyntax{rawChild};
}

///
/// ParsedFallthroughStmtSyntax
///
ParsedTokenSyntax ParsedFallthroughStmtSyntax::getDeferredFallthroughKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[FallthroughStmtSyntax::Cursor::FallthroughKeyword]};
}

///
/// ParsedElseIfClauseSyntax
///
ParsedTokenSyntax ParsedElseIfClauseSyntax::getDeferredElseIfKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[ElseIfClauseSyntax::Cursor::ElseIfKeyword]};
}

ParsedTokenSyntax ParsedElseIfClauseSyntax::getDeferredLeftParen()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[ElseIfClauseSyntax::Cursor::LeftParen]};
}

ParsedExprSyntax ParsedElseIfClauseSyntax::getDeferredCondition()
{
   return ParsedExprSyntax{getRaw().getDeferredChildren()[ElseIfClauseSyntax::Cursor::Condition]};
}

ParsedTokenSyntax ParsedElseIfClauseSyntax::getDeferredRightParen()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[ElseIfClauseSyntax::Cursor::RightParen]};
}

ParsedCodeBlockSyntax ParsedElseIfClauseSyntax::getDeferredBody()
{
   return ParsedCodeBlockSyntax{getRaw().getDeferredChildren()[ElseIfClauseSyntax::Cursor::Body]};
}

///
/// IfStmtSyntax
///
std::optional<ParsedTokenSyntax> ParsedIfStmtSyntax::getDeferredLabelName()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[IfStmtSyntax::Cursor::LabelName];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedTokenSyntax{rawChild};
}

std::optional<ParsedTokenSyntax> ParsedIfStmtSyntax::getDeferredLabelColon()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[IfStmtSyntax::Cursor::LabelColon];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedTokenSyntax{rawChild};
}

ParsedTokenSyntax ParsedIfStmtSyntax::getDeferredIfKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[IfStmtSyntax::Cursor::IfKeyword]};
}

ParsedTokenSyntax ParsedIfStmtSyntax::getDeferredLeftParen()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[IfStmtSyntax::Cursor::LeftParen]};
}

ParsedExprSyntax ParsedIfStmtSyntax::getDeferredCondition()
{
   return ParsedExprSyntax{getRaw().getDeferredChildren()[IfStmtSyntax::Cursor::Condition]};
}

ParsedTokenSyntax ParsedIfStmtSyntax::getDeferredRightParen()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[IfStmtSyntax::Cursor::RightParen]};
}

ParsedCodeBlockSyntax ParsedIfStmtSyntax::getDeferredBody()
{
   return ParsedCodeBlockSyntax{getRaw().getDeferredChildren()[IfStmtSyntax::Cursor::Body]};
}

std::optional<ParsedElseIfListSyntax> ParsedIfStmtSyntax::getDeferredElseIfClauses()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[IfStmtSyntax::Cursor::ElseIfClauses];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedElseIfListSyntax{rawChild};
}

std::optional<ParsedTokenSyntax> ParsedIfStmtSyntax::getDeferredElseKeyword()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[IfStmtSyntax::Cursor::ElseKeyword];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedTokenSyntax{rawChild};
}

std::optional<ParsedSyntax> ParsedIfStmtSyntax::getDeferredElseBody()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[IfStmtSyntax::Cursor::ElseBody];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedSyntax{rawChild};
}

///
/// ParsedWhileStmtSyntax
///

std::optional<ParsedTokenSyntax> ParsedWhileStmtSyntax::getDeferredLabelName()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[WhileStmtSyntax::Cursor::LabelName];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedTokenSyntax{rawChild};
}

std::optional<ParsedTokenSyntax> ParsedWhileStmtSyntax::getDeferredLabelColon()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[WhileStmtSyntax::Cursor::LabelColon];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedTokenSyntax{rawChild};
}

ParsedTokenSyntax ParsedWhileStmtSyntax::getDeferredWhileKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[WhileStmtSyntax::Cursor::WhileKeyword]};
}

ParsedConditionElementListSyntax ParsedWhileStmtSyntax::getDeferredConditions()
{
   return ParsedConditionElementListSyntax{getRaw().getDeferredChildren()[WhileStmtSyntax::Cursor::Conditions]};
}

ParsedCodeBlockSyntax ParsedWhileStmtSyntax::getDeferredBody()
{
   return ParsedCodeBlockSyntax{getRaw().getDeferredChildren()[WhileStmtSyntax::Cursor::Body]};
}

///
/// ParsedDoWhileStmtSyntax
///
std::optional<ParsedTokenSyntax> ParsedDoWhileStmtSyntax::getDeferredLabelName()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[DoWhileStmtSyntax::Cursor::LabelName];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedTokenSyntax{rawChild};
}

std::optional<ParsedTokenSyntax> ParsedDoWhileStmtSyntax::getDeferredLabelColon()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[DoWhileStmtSyntax::Cursor::LabelColon];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedTokenSyntax{rawChild};
}

ParsedTokenSyntax ParsedDoWhileStmtSyntax::getDeferredDoKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[DoWhileStmtSyntax::Cursor::DoKeyword]};
}

ParsedCodeBlockSyntax ParsedDoWhileStmtSyntax::getDeferredBody()
{
   return ParsedCodeBlockSyntax{getRaw().getDeferredChildren()[DoWhileStmtSyntax::Cursor::Body]};
}

ParsedTokenSyntax ParsedDoWhileStmtSyntax::getDeferredWhileKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[DoWhileStmtSyntax::Cursor::WhileKeyword]};
}

ParsedTokenSyntax ParsedDoWhileStmtSyntax::getDeferredLeftParen()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[DoWhileStmtSyntax::Cursor::LeftParen]};
}

ParsedExprSyntax ParsedDoWhileStmtSyntax::getDeferredCondition()
{
   return ParsedExprSyntax{getRaw().getDeferredChildren()[DoWhileStmtSyntax::Cursor::Condition]};
}

ParsedTokenSyntax ParsedDoWhileStmtSyntax::getDeferredRightParen()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[DoWhileStmtSyntax::Cursor::RightParen]};
}

///
/// ParsedSwitchDefaultLabelSyntax
///
ParsedTokenSyntax ParsedSwitchDefaultLabelSyntax::getDeferredDefaultKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[SwitchDefaultLabelSyntax::Cursor::DefaultKeyword]};
}

ParsedTokenSyntax ParsedSwitchDefaultLabelSyntax::getDeferredColon()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[SwitchDefaultLabelSyntax::Cursor::Colon]};
}

///
/// ParsedSwitchCaseLabelSyntax
///
ParsedTokenSyntax ParsedSwitchCaseLabelSyntax::getDeferredCaseKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[SwitchCaseLabelSyntax::Cursor::CaseKeyword]};
}

ParsedExprSyntax ParsedSwitchCaseLabelSyntax::getDeferredExpr()
{
   return ParsedExprSyntax{getRaw().getDeferredChildren()[SwitchCaseLabelSyntax::Cursor::Expr]};
}

ParsedTokenSyntax ParsedSwitchCaseLabelSyntax::getDeferredColon()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[SwitchCaseLabelSyntax::Cursor::Colon]};
}

///
/// ParsedSwitchCaseSyntax
///
ParsedSyntax ParsedSwitchCaseSyntax::getDeferredLabel()
{
   return ParsedSyntax{getRaw().getDeferredChildren()[SwitchCaseSyntax::Cursor::Label]};
}

ParsedCodeBlockItemListSyntax ParsedSwitchCaseSyntax::getDeferredStatements()
{
   return ParsedCodeBlockItemListSyntax{getRaw().getDeferredChildren()[SwitchCaseSyntax::Cursor::Statements]};
}

///
/// ParsedSwitchStmtSyntax
///
std::optional<ParsedTokenSyntax> ParsedSwitchStmtSyntax::getDeferredLabelName()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[SwitchStmtSyntax::Cursor::LabelName];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedTokenSyntax{rawChild};
}

std::optional<ParsedTokenSyntax> ParsedSwitchStmtSyntax::getDeferredLabelColon()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[SwitchStmtSyntax::Cursor::LabelColon];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedTokenSyntax{rawChild};
}

ParsedTokenSyntax ParsedSwitchStmtSyntax::getDeferredSwitchKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[SwitchStmtSyntax::Cursor::SwitchKeyword]};
}

ParsedTokenSyntax ParsedSwitchStmtSyntax::getDeferredLeftParen()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[SwitchStmtSyntax::Cursor::LeftParen]};
}

ParsedExprSyntax ParsedSwitchStmtSyntax::getDeferredConditionExpr()
{
   return ParsedExprSyntax{getRaw().getDeferredChildren()[SwitchStmtSyntax::Cursor::ConditionExpr]};
}

ParsedTokenSyntax ParsedSwitchStmtSyntax::getDeferredRightParen()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[SwitchStmtSyntax::Cursor::RightParen]};
}

ParsedTokenSyntax ParsedSwitchStmtSyntax::getDeferredLeftBrace()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[SwitchStmtSyntax::Cursor::LeftBrace]};
}

ParsedSwitchCaseListSyntax ParsedSwitchStmtSyntax::getDeferredCases()
{
   return ParsedSwitchCaseListSyntax{getRaw().getDeferredChildren()[SwitchStmtSyntax::Cursor::Cases]};
}

ParsedTokenSyntax ParsedSwitchStmtSyntax::getDeferredRightBrace()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[SwitchStmtSyntax::Cursor::RightBrace]};
}

///
/// ParsedDeferStmtSyntax
///

ParsedTokenSyntax ParsedDeferStmtSyntax::getDeferredDeferKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[DeferStmtSyntax::Cursor::DeferKeyword]};
}

ParsedCodeBlockSyntax ParsedDeferStmtSyntax::getDeferredBody()
{
   return ParsedCodeBlockSyntax{getRaw().getDeferredChildren()[DeferStmtSyntax::Cursor::Body]};
}

///
/// ParsedExpressionStmtSyntax
///
ParsedExprSyntax ParsedExpressionStmtSyntax::getDeferredExpr()
{
   return ParsedExprSyntax{getRaw().getDeferredChildren()[ExpressionStmtSyntax::Cursor::Expr]};
}

///
/// ParsedThrowStmtSyntax
///
ParsedTokenSyntax ParsedThrowStmtSyntax::getDeferredThrowKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[ThrowStmtSyntax::Cursor::ThrowKeyword]};
}

ParsedExprSyntax ParsedThrowStmtSyntax::getDeferredExpr()
{
   return ParsedExprSyntax{getRaw().getDeferredChildren()[ThrowStmtSyntax::Cursor::Expr]};
}

///
/// ParsedReturnStmtSyntax
///
ParsedTokenSyntax ParsedReturnStmtSyntax::getDeferredReturnKeyword()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[ReturnStmtSyntax::Cursor::ReturnKeyword]};
}

ParsedExprSyntax ParsedReturnStmtSyntax::getDeferredExpr()
{
   return ParsedExprSyntax{getRaw().getDeferredChildren()[ReturnStmtSyntax::Cursor::Expr]};
}

} // polar::parser
