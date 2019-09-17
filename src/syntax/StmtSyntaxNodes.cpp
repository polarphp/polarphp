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

#include "polarphp/syntax/syntaxnode/StmtSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/DeclSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/ExprSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"

namespace polar::syntax {

///
/// EmptyStmtSyntax
///
void EmptyStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == EmptyStmtSyntax::CHILDREN_COUNT);
   // check condition child node choices
   syntax_assert_child_token(raw, Semicolon, std::set {TokenKindType::T_SEMICOLON});
#endif
}

TokenSyntax EmptyStmtSyntax::getSemicolon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Semicolon).get()};
}

EmptyStmtSyntax EmptyStmtSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> semicolonRaw;
   if (semicolon.has_value()) {
      semicolonRaw = semicolon->getRaw();
   } else {
      semicolonRaw = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<EmptyStmtSyntax>(semicolonRaw, Cursor::Semicolon);
}

///
/// NestStmtSyntax
///
void NestStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == NestStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LeftBraceToken, std::set{TokenKindType::T_LEFT_BRACE});
   syntax_assert_child_kind(raw, Statements, std::set{SyntaxKind::InnerStmtList});
   syntax_assert_child_token(raw, RightBraceToken, std::set{TokenKindType::T_RIGHT_BRACE});
#endif
}

TokenSyntax NestStmtSyntax::getLeftBrace()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftBraceToken).get()};
}

InnerStmtListSyntax NestStmtSyntax::getStatements()
{
   return InnerStmtListSyntax {m_root, m_data->getChild(Cursor::Statements).get()};
}

TokenSyntax NestStmtSyntax::getRightBrace()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightBraceToken).get()};
}

NestStmtSyntax NestStmtSyntax::withLeftBrace(std::optional<TokenSyntax> leftBrace)
{
   RefCountPtr<RawSyntax> rawLeftBrace;
   if (leftBrace.has_value()) {
      rawLeftBrace = leftBrace->getRaw();
   } else {
      rawLeftBrace = make_missing_token(T_LEFT_BRACE);
   }
   return m_data->replaceChild<NestStmtSyntax>(rawLeftBrace, Cursor::LeftBraceToken);
}

NestStmtSyntax NestStmtSyntax::withStatements(std::optional<InnerStmtListSyntax> statements)
{
   RefCountPtr<RawSyntax> rawStatements;
   if (statements.has_value()) {
      rawStatements = statements->getRaw();
   } else {
      rawStatements = RawSyntax::missing(SyntaxKind::InnerStmtList);
   }
   return m_data->replaceChild<NestStmtSyntax>(rawStatements, Cursor::Statements);
}

NestStmtSyntax NestStmtSyntax::withRightBrace(std::optional<TokenSyntax> rightBrace)
{
   RefCountPtr<RawSyntax> rawRightBrace;
   if (rightBrace.has_value()) {
      rawRightBrace = rightBrace->getRaw();
   } else {
      rawRightBrace = make_missing_token(T_RIGHT_BRACE);
   }
   return m_data->replaceChild<NestStmtSyntax>(rawRightBrace, Cursor::RightBraceToken);
}

///
/// ExprStmtSyntax
///
void ExprStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ExprStmtSyntax::CHILDREN_COUNT);
   // check condition child node choices
   syntax_assert_child_kind(raw, Expr, std::set {SyntaxKind::Expr});
   syntax_assert_child_token(raw, Semicolon, std::set {TokenKindType::T_SEMICOLON});
#endif
}

ExprSyntax ExprStmtSyntax::getExpr()
{
   return ExprSyntax {m_root, m_data->getChild(Cursor::Expr).get()};
}

TokenSyntax ExprStmtSyntax::getSemicolon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Semicolon).get()};
}

ExprStmtSyntax ExprStmtSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> rawExpr;
   if (expr.has_value()) {
      rawExpr = expr->getRaw();
   } else {
      rawExpr = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<ExprStmtSyntax>(rawExpr, Cursor::Expr);
}

ExprStmtSyntax ExprStmtSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<ExprStmtSyntax>(rawSemicolon, Cursor::Semicolon);
}

///
/// InnerStmtSyntax
///

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType InnerStmtSyntax::CHILD_NODE_CHOICES
{
   {
      InnerStmtSyntax::Stmt, {
         SyntaxKind::Stmt, SyntaxKind::ClassDefinitionStmt,
               SyntaxKind::InterfaceDefinitionStmt, SyntaxKind::TraitDefinitionStmt,
               SyntaxKind::FunctionDefinitionStmt,
      }
   }
};
#endif

void InnerStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == InnerStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Stmt, CHILD_NODE_CHOICES.at(Cursor::Stmt));
#endif
}

StmtSyntax InnerStmtSyntax::getStmt()
{
   return StmtSyntax {m_root, m_data->getChild(Cursor::Stmt).get()};
}

InnerStmtSyntax InnerStmtSyntax::withStmt(std::optional<StmtSyntax> stmt)
{
   RefCountPtr<RawSyntax> rawStmt;
   if (stmt.has_value()) {
      rawStmt = stmt->getRaw();
   } else {
      rawStmt = RawSyntax::missing(SyntaxKind::Stmt);
   }
   return m_data->replaceChild<InnerStmtSyntax>(rawStmt, Cursor::Stmt);
}

///
/// InnerCodeBlockStmtSyntax
///
void InnerCodeBlockStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == InnerCodeBlockStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LeftBrace, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_kind(raw, Statements, std::set{SyntaxKind::InnerStmtList});
   syntax_assert_child_token(raw, RightBrace, std::set{TokenKindType::T_RIGHT_PAREN});
#endif
}

TokenSyntax InnerCodeBlockStmtSyntax::getLeftBrace()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftBrace).get()};
}

InnerStmtListSyntax InnerCodeBlockStmtSyntax::getStatements()
{
   return InnerStmtListSyntax{m_root, m_data->getChild(Cursor::Statements).get()};
}

TokenSyntax InnerCodeBlockStmtSyntax::getRightBrace()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::RightBrace).get()};
}

InnerCodeBlockStmtSyntax InnerCodeBlockStmtSyntax::addCodeBlockItem(InnerStmtSyntax stmt)
{
   RefCountPtr<RawSyntax> statements = getRaw()->getChild(Cursor::Statements);
   if (statements) {
      statements = statements->append(stmt.getRaw());
   } else {
      statements = RawSyntax::make(SyntaxKind::InnerStmtList, {stmt.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<InnerCodeBlockStmtSyntax>(statements, Cursor::Statements);
}

InnerCodeBlockStmtSyntax InnerCodeBlockStmtSyntax::withLeftBrace(std::optional<TokenSyntax> leftBrace)
{
   RefCountPtr<RawSyntax> rawLeftBrace;
   if (leftBrace.has_value()) {
      rawLeftBrace = leftBrace->getRaw();
   } else {
      rawLeftBrace = make_missing_token(T_LEFT_BRACE);
   }
   return m_data->replaceChild<InnerCodeBlockStmtSyntax>(rawLeftBrace, Cursor::LeftBrace);
}

InnerCodeBlockStmtSyntax InnerCodeBlockStmtSyntax::withStatements(std::optional<InnerStmtSyntax> statements)
{
   RefCountPtr<RawSyntax> rawStatements;
   if (statements.has_value()) {
      rawStatements = statements->getRaw();
   } else {
      rawStatements = RawSyntax::missing(SyntaxKind::InnerStmtList);
   }
   return m_data->replaceChild<InnerCodeBlockStmtSyntax>(rawStatements, Cursor::Statements);
}

InnerCodeBlockStmtSyntax InnerCodeBlockStmtSyntax::withRightBrace(std::optional<TokenSyntax> rightBrace)
{
   RefCountPtr<RawSyntax> rawRightBrace;
   if (rightBrace.has_value()) {
      rawRightBrace = rightBrace->getRaw();
   } else {
      rawRightBrace = make_missing_token(T_RIGHT_BRACE);
   }
   return m_data->replaceChild<InnerCodeBlockStmtSyntax>(rawRightBrace, Cursor::RightBrace);
}

///
/// TopStmtSyntax
///
#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType TopStmtSyntax::CHILD_NODE_CHOICES
{
   {
      TopStmtSyntax::Cursor::Stmt, {
         // Statement
         SyntaxKind::InnerCodeBlockStmt, SyntaxKind::IfStmt,
               SyntaxKind::WhileStmt, SyntaxKind::DoWhileStmt,
               SyntaxKind::ForStmt, SyntaxKind::SwitchStmt,
               SyntaxKind::BreakStmt, SyntaxKind::ContinueStmt,
               SyntaxKind::ReturnStmt, SyntaxKind::GlobalVariableDeclarationsStmt,
               SyntaxKind::StaticVariableDeclarationsStmt, SyntaxKind::EchoStmt,
               SyntaxKind::ExprStmt, SyntaxKind::UnsetStmt,
               SyntaxKind::ForeachStmt, SyntaxKind::DeclareStmt,
               SyntaxKind::EmptyStmt, SyntaxKind::TryStmt,
               SyntaxKind::ThrowStmt, SyntaxKind::GotoStmt,
               SyntaxKind::LabelStmt,

               SyntaxKind::FunctionDefinitionStmt,
               SyntaxKind::ClassDefinitionStmt, SyntaxKind::InterfaceDefinitionStmt,
               SyntaxKind::TraitDefinitionStmt, SyntaxKind::HaltCompilerStmt,
               SyntaxKind::NamespaceBlockStmt, SyntaxKind::NamespaceDefinitionStmt,
               SyntaxKind::NamespaceUseStmt, SyntaxKind::ConstDefinitionStmt,
      }
   }
};
#endif

void TopStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == TopStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Stmt, CHILD_NODE_CHOICES.at(Cursor::Stmt));
#endif
}

StmtSyntax TopStmtSyntax::getStmt()
{
   return StmtSyntax{m_root, m_data->getChild(Cursor::Stmt).get()};
}

TopStmtSyntax TopStmtSyntax::withStmt(std::optional<StmtSyntax> stmt)
{
   RefCountPtr<RawSyntax> rawStmt;
   if (stmt.has_value()) {
      rawStmt = stmt->getRaw();
   } else {
      rawStmt = RawSyntax::missing(SyntaxKind::Stmt);
   }
   return m_data->replaceChild<TopStmtSyntax>(rawStmt, Cursor::Stmt);
}

///
/// TopCodeBlockStmtSyntax
///
void TopCodeBlockStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == TopCodeBlockStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LeftBrace, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_kind(raw, Statements, std::set{SyntaxKind::TopCodeBlockStmt});
   syntax_assert_child_token(raw, RightBrace, std::set{TokenKindType::T_RIGHT_PAREN});
#endif
}

TokenSyntax TopCodeBlockStmtSyntax::getLeftBrace()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftBrace).get()};
}

TopStmtListSyntax TopCodeBlockStmtSyntax::getStatements()
{
   return TopStmtListSyntax{m_root, m_data->getChild(Cursor::Statements).get()};
}

TokenSyntax TopCodeBlockStmtSyntax::getRightBrace()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::RightBrace).get()};
}

TopCodeBlockStmtSyntax TopCodeBlockStmtSyntax::addCodeBlockItem(TopStmtSyntax stmt)
{
   RefCountPtr<RawSyntax> statements = getRaw()->getChild(Cursor::Statements);
   if (statements) {
      statements = statements->append(stmt.getRaw());
   } else {
      statements = RawSyntax::make(SyntaxKind::TopStmtList, {stmt.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<TopCodeBlockStmtSyntax>(statements, Cursor::Statements);
}

TopCodeBlockStmtSyntax TopCodeBlockStmtSyntax::withLeftBrace(std::optional<TokenSyntax> leftBrace)
{
   RefCountPtr<RawSyntax> rawLeftBrace;
   if (leftBrace.has_value()) {
      rawLeftBrace = leftBrace->getRaw();
   } else {
      rawLeftBrace = make_missing_token(T_LEFT_BRACE);
   }
   return m_data->replaceChild<TopCodeBlockStmtSyntax>(rawLeftBrace, Cursor::LeftBrace);
}

TopCodeBlockStmtSyntax TopCodeBlockStmtSyntax::withStatements(std::optional<TopStmtListSyntax> statements)
{
   RefCountPtr<RawSyntax> rawStatements;
   if (statements.has_value()) {
      rawStatements = statements->getRaw();
   } else {
      rawStatements = RawSyntax::missing(SyntaxKind::TopStmtList);
   }
   return m_data->replaceChild<TopCodeBlockStmtSyntax>(rawStatements, Cursor::Statements);
}

TopCodeBlockStmtSyntax TopCodeBlockStmtSyntax::withRightBrace(std::optional<TokenSyntax> rightBrace)
{
   RefCountPtr<RawSyntax> rawRightBrace;
   if (rightBrace.has_value()) {
      rawRightBrace = rightBrace->getRaw();
   } else {
      rawRightBrace = make_missing_token(T_RIGHT_BRACE);
   }
   return m_data->replaceChild<TopCodeBlockStmtSyntax>(rawRightBrace, Cursor::RightBrace);
}

///
/// DeclareStmtSyntax
///
void DeclareStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == DeclareStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, DeclareToken, std::set{TokenKindType::T_DECLARE});
   syntax_assert_child_token(raw, LeftParenToken, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_kind(raw, ConstList, std::set{SyntaxKind::ConstDeclareList});
   syntax_assert_child_token(raw, RightParenToken, std::set{TokenKindType::T_RIGHT_PAREN});
   syntax_assert_child_kind(raw, Stmt, std::set{SyntaxKind::Stmt});
#endif
}

TokenSyntax DeclareStmtSyntax::getDeclareToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::DeclareToken).get()};
}

TokenSyntax DeclareStmtSyntax::getLeftParenToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftParenToken).get()};
}

ConstDeclareListSyntax DeclareStmtSyntax::getConstList()
{
   return ConstDeclareListSyntax{m_root, m_data->getChild(Cursor::ConstList).get()};
}

TokenSyntax DeclareStmtSyntax::getRightParenToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::RightParenToken).get()};
}

StmtSyntax DeclareStmtSyntax::getStmt()
{
   return StmtSyntax{m_root, m_data->getChild(Cursor::Stmt).get()};
}

DeclareStmtSyntax
DeclareStmtSyntax::withDeclareToken(std::optional<TokenSyntax> declareToken)
{
   RefCountPtr<RawSyntax> rawDeclareToken;
   if (declareToken.has_value()) {
      rawDeclareToken = declareToken->getRaw();
   } else {
      rawDeclareToken = make_missing_token(T_DECLARE);
   }
   return m_data->replaceChild<DeclareStmtSyntax>(rawDeclareToken, Cursor::DeclareToken);
}

DeclareStmtSyntax
DeclareStmtSyntax::withLeftParenToken(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> rawLeftParen;
   if (leftParen.has_value()) {
      rawLeftParen = leftParen->getRaw();
   } else {
      rawLeftParen = make_missing_token(T_LEFT_PAREN);
   }
   return m_data->replaceChild<DeclareStmtSyntax>(rawLeftParen, Cursor::LeftParenToken);
}

DeclareStmtSyntax
DeclareStmtSyntax::withConstList(std::optional<ConstDeclareListSyntax> constList)
{
   RefCountPtr<RawSyntax> rawConstList;
   if (constList.has_value()) {
      rawConstList = constList->getRaw();
   } else {
      rawConstList = RawSyntax::missing(SyntaxKind::ConstDeclareList);
   }
   return m_data->replaceChild<DeclareStmtSyntax>(rawConstList, Cursor::ConstList);
}

DeclareStmtSyntax
DeclareStmtSyntax::withRightParenToken(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rawRightParen;
   if (rightParen.has_value()) {
      rawRightParen = rightParen->getRaw();
   } else {
      rawRightParen = make_missing_token(T_RIGHT_PAREN);
   }
   return m_data->replaceChild<DeclareStmtSyntax>(rawRightParen, Cursor::RightParenToken);
}

DeclareStmtSyntax
DeclareStmtSyntax::withStmt(std::optional<StmtSyntax> stmt)
{
   RefCountPtr<RawSyntax> rawStmt;
   if (stmt.has_value()) {
      rawStmt = stmt->getRaw();
   } else {
      rawStmt = RawSyntax::missing(SyntaxKind::Stmt);
   }
   return m_data->replaceChild<DeclareStmtSyntax>(rawStmt, Cursor::Stmt);
}

///
/// ConditionElementSyntax
///
void GotoStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == GotoStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, GotoToken, std::set{TokenKindType::T_GOTO});
   syntax_assert_child_token(raw, Target, std::set{TokenKindType::T_IDENTIFIER_STRING});
   syntax_assert_child_token(raw, Semicolon, std::set{TokenKindType::T_SEMICOLON});
#endif
}

TokenSyntax GotoStmtSyntax::getGotoToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::GotoToken).get()};
}

TokenSyntax GotoStmtSyntax::getTarget()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Target).get()};
}

TokenSyntax GotoStmtSyntax::getSemicolon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Semicolon).get()};
}

GotoStmtSyntax GotoStmtSyntax::withGotoToken(std::optional<TokenSyntax> gotoToken)
{
   RefCountPtr<RawSyntax> rawGotoToken;
   if (gotoToken.has_value()) {
      rawGotoToken = gotoToken->getRaw();
   } else {
      rawGotoToken = make_missing_token(T_GOTO);
   }
   return m_data->replaceChild<GotoStmtSyntax>(rawGotoToken, Cursor::GotoToken);
}

GotoStmtSyntax GotoStmtSyntax::withTarget(std::optional<TokenSyntax> target)
{
   RefCountPtr<RawSyntax> rawTarget;
   if (target.has_value()) {
      rawTarget = target->getRaw();
   } else {
      rawTarget = make_missing_token(T_IDENTIFIER_STRING);
   }
   return m_data->replaceChild<GotoStmtSyntax>(rawTarget, Cursor::Target);
}

GotoStmtSyntax GotoStmtSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<GotoStmtSyntax>(rawSemicolon, Cursor::Semicolon);
}

///
/// UnsetVariableSyntax
///

void UnsetVariableSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == UnsetVariableSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Variable, std::set{SyntaxKind::VariableExpr});
#endif
}

VariableExprSyntax UnsetVariableSyntax::getVariable()
{
   return VariableExprSyntax {m_root, m_data->getChild(Cursor::Variable).get()};
}

UnsetVariableSyntax UnsetVariableSyntax::withVariable(std::optional<VariableExprSyntax> variable)
{
   RefCountPtr<RawSyntax> rawVariable;
   if (variable.has_value()) {
      rawVariable = variable->getRaw();
   } else {
      rawVariable = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<UnsetVariableSyntax>(rawVariable, Cursor::Variable);
}

///
/// UnsetVariableListItemSyntax
///

void UnsetVariableListItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == UnsetVariableListItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, CommaToken, std::set{TokenKindType::T_COMMA});
   syntax_assert_child_kind(raw, Variable, std::set{SyntaxKind::UnsetVariable});
#endif
}

std::optional<TokenSyntax> UnsetVariableListItemSyntax::getComma()
{
   RefCountPtr<SyntaxData> commaData = m_data->getChild(Cursor::CommaToken);
   if (!commaData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, commaData.get()};
}

TokenSyntax UnsetVariableListItemSyntax::getVariable()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Variable).get()};
}

UnsetVariableListItemSyntax
UnsetVariableListItemSyntax::withComma(std::optional<TokenSyntax> comma)
{
   RefCountPtr<RawSyntax> commaRaw;
   if (comma.has_value()) {
      commaRaw = comma->getRaw();
   } else {
      commaRaw = nullptr;
   }
   return m_data->replaceChild<UnsetVariableListItemSyntax>(commaRaw, Cursor::CommaToken);
}

UnsetVariableListItemSyntax
UnsetVariableListItemSyntax::withVariable(std::optional<UnsetVariableSyntax> variable)
{
   RefCountPtr<RawSyntax> rawVariable;
   if (variable.has_value()) {
      rawVariable = variable->getRaw();
   } else {
      rawVariable = make_missing_token(T_VARIABLE);
   }
   return m_data->replaceChild<UnsetVariableListItemSyntax>(rawVariable, Cursor::Variable);
}

///
/// UnsetStmtSyntax
///
void UnsetStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == UnsetVariableSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, UnsetToken, std::set{TokenKindType::T_UNSET});
   syntax_assert_child_token(raw, LeftParenToken, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_kind(raw, UnsetVariables, std::set{SyntaxKind::UnsetVariableList});
   syntax_assert_child_token(raw, RightParenToken, std::set{TokenKindType::T_RIGHT_PAREN});
   syntax_assert_child_token(raw, Semicolon, std::set{TokenKindType::T_COMMA});
#endif
}

TokenSyntax UnsetStmtSyntax::getUnsetToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::UnsetToken).get()};
}

TokenSyntax UnsetStmtSyntax::getLeftParenToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftParenToken).get()};
}

UnsetVariableListSyntax UnsetStmtSyntax::getUnsetVariables()
{
   return UnsetVariableListSyntax {m_root, m_data->getChild(Cursor::UnsetVariables).get()};
}

TokenSyntax UnsetStmtSyntax::getRightParenToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightParenToken).get()};
}

TokenSyntax UnsetStmtSyntax::getSemicolon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Semicolon).get()};
}

UnsetStmtSyntax
UnsetStmtSyntax::withUnsetToken(std::optional<TokenSyntax> unsetToken)
{
   RefCountPtr<RawSyntax> rawUnsetToken;
   if (unsetToken.has_value()) {
      rawUnsetToken = unsetToken->getRaw();
   } else {
      rawUnsetToken = make_missing_token(T_UNSET);
   }
   return m_data->replaceChild<UnsetStmtSyntax>(rawUnsetToken, Cursor::UnsetToken);
}

UnsetStmtSyntax
UnsetStmtSyntax::withLeftParenToken(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> rawLeftParen;
   if (leftParen.has_value()) {
      rawLeftParen = leftParen->getRaw();
   } else {
      rawLeftParen = make_missing_token(T_LEFT_PAREN);
   }
   return m_data->replaceChild<UnsetStmtSyntax>(rawLeftParen, Cursor::LeftParenToken);
}

UnsetStmtSyntax
UnsetStmtSyntax::withUnsetVariables(std::optional<UnsetVariableListSyntax> variables)
{
   RefCountPtr<RawSyntax> rawVariables;
   if (variables.has_value()) {
      rawVariables = variables->getRaw();
   } else {
      rawVariables = RawSyntax::missing(SyntaxKind::UnsetVariableList);
   }
   return m_data->replaceChild<UnsetStmtSyntax>(rawVariables, Cursor::UnsetVariables);
}

UnsetStmtSyntax
UnsetStmtSyntax::withRightParenToken(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rawRightParen;
   if (rightParen.has_value()) {
      rawRightParen = rightParen->getRaw();
   } else {
      rawRightParen = make_missing_token(T_RIGHT_PAREN);
   }
   return m_data->replaceChild<UnsetStmtSyntax>(rawRightParen, Cursor::RightParenToken);
}

UnsetStmtSyntax
UnsetStmtSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<UnsetStmtSyntax>(rawSemicolon, Cursor::Semicolon);
}

///
/// LabelStmtSyntax
///
void LabelStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == LabelStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, Name, std::set{TokenKindType::T_IDENTIFIER_STRING});
   syntax_assert_child_token(raw, Colon, std::set{TokenKindType::T_COLON});
#endif
}

TokenSyntax LabelStmtSyntax::getName()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Name).get()};
}

TokenSyntax LabelStmtSyntax::getColon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Colon).get()};
}

LabelStmtSyntax LabelStmtSyntax::withName(std::optional<TokenSyntax> name)
{
   RefCountPtr<RawSyntax> rawName;
   if (name.has_value()) {
      rawName = name->getRaw();
   } else {
      rawName = make_missing_token(T_IDENTIFIER_STRING);
   }
   return m_data->replaceChild<LabelStmtSyntax>(rawName, Cursor::Name);
}

LabelStmtSyntax LabelStmtSyntax::withColon(std::optional<TokenSyntax> colon)
{
   RefCountPtr<RawSyntax> rawColon;
   if (colon.has_value()) {
      rawColon = colon->getRaw();
   } else {
      rawColon = make_missing_token(T_COLON);
   }
   return m_data->replaceChild<LabelStmtSyntax>(rawColon, Cursor::Colon);
}

///
/// ConditionElementSyntax
///
const NodeChoicesType ConditionElementSyntax::CHILD_NODE_CHOICES
{
   {
      ConditionElementSyntax::Cursor::Condition, {
         SyntaxKind::Expr
      }
   }
};

void ConditionElementSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ConditionElementSyntax::CHILDREN_COUNT);
   // check condition child node choices
   syntax_assert_child_kind(raw, Condition, std::set{SyntaxKind::Expr});
#endif
}

ExprSyntax ConditionElementSyntax::getCondition()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Condition).get()};
}

std::optional<TokenSyntax> ConditionElementSyntax::getTrailingComma()
{
   RefCountPtr<SyntaxData> trailingCommaData = m_data->getChild(Cursor::TrailingComma);
   if (!trailingCommaData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, trailingCommaData.get()};
}

ConditionElementSyntax ConditionElementSyntax::withCondition(std::optional<ExprSyntax> condition)
{
   RefCountPtr<RawSyntax> rawCondition;
   if (condition.has_value()) {
      rawCondition = condition->getRaw();
   } else {
      rawCondition = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<ConditionElementSyntax>(rawCondition, Cursor::Condition);
}

ConditionElementSyntax ConditionElementSyntax::withTrailingComma(std::optional<TokenSyntax> trailingComma)
{
   RefCountPtr<RawSyntax> rawTrailingComma;
   if (trailingComma.has_value()) {
      rawTrailingComma = trailingComma->getRaw();
   } else {
      rawTrailingComma = nullptr;
   }
   return m_data->replaceChild<ConditionElementSyntax>(rawTrailingComma, Cursor::TrailingComma);
}

///
/// ContinueStmtSyntax
///

void ContinueStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ContinueStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ContinueKeyword, std::set{TokenKindType::T_CONTINUE});
   syntax_assert_child_kind(raw, Expr, std::set{SyntaxKind::Expr});
   syntax_assert_child_token(raw, Semicolon, std::set{TokenKindType::T_SEMICOLON});
#endif
}

TokenSyntax ContinueStmtSyntax::getContinueKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::ContinueKeyword).get()};
}

std::optional<ExprSyntax> ContinueStmtSyntax::getExpr()
{
   RefCountPtr<SyntaxData> exprData = m_data->getChild(Cursor::Expr);
   if (!exprData) {
      return std::nullopt;
   }
   return ExprSyntax{m_root, exprData.get()};
}

TokenSyntax ContinueStmtSyntax::getSemicolon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Semicolon).get()};
}

ContinueStmtSyntax ContinueStmtSyntax::withContinueKeyword(std::optional<TokenSyntax> continueKeyword)
{
   RefCountPtr<RawSyntax> rawContinueKeyword;
   if (continueKeyword.has_value()) {
      rawContinueKeyword = continueKeyword->getRaw();
   } else {
      rawContinueKeyword = make_missing_token(T_CONTINUE);
   }
   return m_data->replaceChild<ContinueStmtSyntax>(rawContinueKeyword, Cursor::ContinueKeyword);
}

ContinueStmtSyntax ContinueStmtSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> rawExpr;
   if (expr.has_value()) {
      rawExpr = expr->getRaw();
   } else {
      rawExpr = nullptr;
   }
   return m_data->replaceChild<ContinueStmtSyntax>(rawExpr, Cursor::Expr);
}

ContinueStmtSyntax ContinueStmtSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<ContinueStmtSyntax>(rawSemicolon, Cursor::Semicolon);
}

///
/// BreakStmtSyntax
///
void BreakStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == BreakStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, BreakKeyword, std::set{TokenKindType::T_BREAK});
   syntax_assert_child_kind(raw, Expr, std::set{SyntaxKind::Expr});
   syntax_assert_child_token(raw, Semicolon, std::set{TokenKindType::T_SEMICOLON});
#endif
}

TokenSyntax BreakStmtSyntax::getBreakKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::BreakKeyword).get()};
}

std::optional<ExprSyntax> BreakStmtSyntax::getExpr()
{
   RefCountPtr<SyntaxData> exprData = m_data->getChild(Cursor::Expr);
   if (!exprData) {
      return std::nullopt;
   }
   return ExprSyntax{m_root, exprData.get()};
}

TokenSyntax BreakStmtSyntax::getSemicolon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Semicolon).get()};
}

BreakStmtSyntax BreakStmtSyntax::withBreakKeyword(std::optional<TokenSyntax> breakKeyword)
{
   RefCountPtr<RawSyntax> rawBreakKeyword;
   if (breakKeyword.has_value()) {
      rawBreakKeyword = breakKeyword->getRaw();
   } else {
      rawBreakKeyword = make_missing_token(T_BREAK);
   }
   return m_data->replaceChild<BreakStmtSyntax>(rawBreakKeyword, Cursor::BreakKeyword);
}

BreakStmtSyntax BreakStmtSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> rawExpr;
   if (expr.has_value()) {
      rawExpr = expr->getRaw();
   } else {
      rawExpr = nullptr;
   }
   return m_data->replaceChild<BreakStmtSyntax>(rawExpr, Cursor::Expr);
}

BreakStmtSyntax BreakStmtSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<BreakStmtSyntax>(rawSemicolon, Cursor::Semicolon);
}

///
/// FallthroughStmtSyntax
///
void FallthroughStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if(isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == FallthroughStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, FallthroughKeyword, std::set{TokenKindType::T_FALLTHROUGH});
#endif
}

TokenSyntax FallthroughStmtSyntax::getFallthroughKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::FallthroughKeyword).get()};
}

TokenSyntax FallthroughStmtSyntax::getSemicolon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Semicolon).get()};
}

FallthroughStmtSyntax FallthroughStmtSyntax::withFallthroughStmtSyntax(std::optional<TokenSyntax> fallthroughKeyword)
{
   RefCountPtr<RawSyntax> rawFallthroughKeyword;
   if (fallthroughKeyword.has_value()) {
      rawFallthroughKeyword = fallthroughKeyword->getRaw();
   } else {
      rawFallthroughKeyword = make_missing_token(T_FALLTHROUGH);
   }
   return m_data->replaceChild<FallthroughStmtSyntax>(rawFallthroughKeyword, Cursor::FallthroughKeyword);
}

FallthroughStmtSyntax FallthroughStmtSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<FallthroughStmtSyntax>(rawSemicolon, Cursor::Semicolon);
}

///
/// ElseIfClauseSyntax
///
void ElseIfClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ElseIfClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ElseIfKeyword, std::set{TokenKindType::T_ELSEIF});
   syntax_assert_child_token(raw, LeftParen, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_token(raw, RightParen, std::set{TokenKindType::T_RIGHT_PAREN});
   syntax_assert_child_kind(raw, Condition, std::set{SyntaxKind::Expr});
   syntax_assert_child_kind(raw, Body, std::set{SyntaxKind::CodeBlock});
#endif
}

TokenSyntax ElseIfClauseSyntax::getElseIfKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::ElseIfKeyword).get()};
}

TokenSyntax ElseIfClauseSyntax::getLeftParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftParen).get()};
}

ExprSyntax ElseIfClauseSyntax::getCondition()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Condition).get()};
}

TokenSyntax ElseIfClauseSyntax::getRightParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::RightParen).get()};
}

StmtSyntax ElseIfClauseSyntax::getBody()
{
   return StmtSyntax{m_root, m_data->getChild(Cursor::Body).get()};
}

ElseIfClauseSyntax ElseIfClauseSyntax::withElseIfKeyword(std::optional<TokenSyntax> elseIfKeyword)
{
   RefCountPtr<RawSyntax> rawElseIfKeyword;
   if (elseIfKeyword.has_value()) {
      rawElseIfKeyword = elseIfKeyword->getRaw();
   } else {
      rawElseIfKeyword = make_missing_token(T_ELSEIF);
   }
   return m_data->replaceChild<ElseIfClauseSyntax>(rawElseIfKeyword, Cursor::ElseIfKeyword);
}

ElseIfClauseSyntax ElseIfClauseSyntax::withLeftParen(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> rawLeftParen;
   if (leftParen.has_value()) {
      rawLeftParen = leftParen->getRaw();
   } else {
      rawLeftParen = make_missing_token(T_LEFT_PAREN);
   }
   return m_data->replaceChild<ElseIfClauseSyntax>(rawLeftParen, Cursor::LeftParen);
}

ElseIfClauseSyntax ElseIfClauseSyntax::withCondition(std::optional<ExprSyntax> condition)
{
   RefCountPtr<RawSyntax> rawCondition;
   if (condition.has_value()) {
      rawCondition = condition->getRaw();
   } else {
      rawCondition = RawSyntax::missing(SyntaxKind::UnknownExpr);
   }
   return m_data->replaceChild<ElseIfClauseSyntax>(rawCondition, Cursor::Condition);
}

ElseIfClauseSyntax ElseIfClauseSyntax::withRightParen(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rawRightParen;
   if (rightParen.has_value()) {
      rawRightParen = rightParen->getRaw();
   } else {
      rawRightParen = make_missing_token(T_RIGHT_PAREN);
   }
   return m_data->replaceChild<ElseIfClauseSyntax>(rawRightParen, Cursor::RightParen);
}

ElseIfClauseSyntax ElseIfClauseSyntax::withBody(std::optional<StmtSyntax> body)
{
   RefCountPtr<RawSyntax> rawBody;
   if (body.has_value()) {
      rawBody = body->getRaw();
   } else {
      rawBody = RawSyntax::missing(SyntaxKind::Stmt);
   }
   return m_data->replaceChild<ElseIfClauseSyntax>(rawBody, Cursor::Body);
}

///
/// IfStmtSyntax
///

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType IfStmtSyntax::CHILD_NODE_CHOICES
{
   {IfStmtSyntax::ElseBody, {
         SyntaxKind::IfStmt,
               SyntaxKind::CodeBlock
      }}
};
#endif

void IfStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == IfStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LabelName, std::set{TokenKindType::T_IDENTIFIER_STRING});
   syntax_assert_child_token(raw, LabelColon, std::set{TokenKindType::T_COLON});
   syntax_assert_child_token(raw, IfKeyword, std::set{TokenKindType::T_IF});
   syntax_assert_child_token(raw, LeftParen, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_kind(raw, Condition, std::set{SyntaxKind::Expr});
   syntax_assert_child_token(raw, RightParen, std::set{TokenKindType::T_RIGHT_PAREN});
   syntax_assert_child_kind(raw, Body, std::set{SyntaxKind::CodeBlock});
   syntax_assert_child_kind(raw, ElseIfClauses, std::set{SyntaxKind::ElseIfList});
   syntax_assert_child_token(raw, ElseKeyword, std::set{TokenKindType::T_ELSE});
   if (const RefCountPtr<RawSyntax> &elseBody = raw->getChild(Cursor::ElseBody)) {
      bool isIfStmt = elseBody->kindOf(SyntaxKind::IfStmt);
      bool isCodeBlock = elseBody->kindOf(SyntaxKind::CodeBlock);
      assert(isIfStmt || isCodeBlock);
   }
#endif
}

std::optional<TokenSyntax> IfStmtSyntax::getLabelName()
{
   RefCountPtr<SyntaxData> labelNameData = m_data->getChild(Cursor::LabelName);
   if (!labelNameData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, labelNameData.get()};
}

std::optional<TokenSyntax> IfStmtSyntax::getLabelColon()
{
   RefCountPtr<SyntaxData> labelColonData = m_data->getChild(Cursor::LabelColon);
   if (!labelColonData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, labelColonData.get()};
}

TokenSyntax IfStmtSyntax::getIfKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::IfKeyword).get()};
}

TokenSyntax IfStmtSyntax::getLeftParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftParen).get()};
}

ExprSyntax IfStmtSyntax::getCondition()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Condition).get()};
}

TokenSyntax IfStmtSyntax::getRightParen()
{
   return TokenSyntax(m_root, m_data->getChild(Cursor::RightParen).get());
}

StmtSyntax IfStmtSyntax::getBody()
{
   return StmtSyntax{m_root, m_data->getChild(Cursor::Body).get()};
}

std::optional<ElseIfListSyntax> IfStmtSyntax::getElseIfClauses()
{
   RefCountPtr<SyntaxData> elseIfClausesData = m_data->getChild(Cursor::ElseIfClauses);
   if (!elseIfClausesData) {
      return std::nullopt;
   }
   return ElseIfListSyntax{m_root, elseIfClausesData.get()};
}

std::optional<TokenSyntax> IfStmtSyntax::getElseKeyword()
{
   RefCountPtr<SyntaxData> elseKeywordData = m_data->getChild(Cursor::ElseKeyword);
   if (!elseKeywordData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, elseKeywordData.get()};
}

std::optional<Syntax> IfStmtSyntax::getElseBody()
{
   RefCountPtr<SyntaxData> elseBodyData = m_data->getChild(Cursor::ElseBody);
   if (!elseBodyData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, elseBodyData.get()};
}

IfStmtSyntax IfStmtSyntax::withLabelName(std::optional<TokenSyntax> labelName)
{
   RefCountPtr<RawSyntax> rawLabelName;
   if (labelName.has_value()) {
      rawLabelName = labelName->getRaw();
   } else {
      rawLabelName = nullptr;
   }
   return m_data->replaceChild<IfStmtSyntax>(rawLabelName, Cursor::LabelName);
}

IfStmtSyntax IfStmtSyntax::withLabelColon(std::optional<TokenSyntax> labelColon)
{
   RefCountPtr<RawSyntax> rawLabelColon;
   if (labelColon.has_value()) {
      rawLabelColon = labelColon->getRaw();
   } else {
      rawLabelColon = nullptr;
   }
   return m_data->replaceChild<IfStmtSyntax>(rawLabelColon, Cursor::LabelColon);
}

IfStmtSyntax IfStmtSyntax::withIfKeyword(std::optional<TokenSyntax> ifKeyword)
{
   RefCountPtr<RawSyntax> rawIfKeyword;
   if (ifKeyword.has_value()) {
      rawIfKeyword = ifKeyword->getRaw();
   } else {
      rawIfKeyword = make_missing_token(T_IF);
   }
   return m_data->replaceChild<IfStmtSyntax>(rawIfKeyword, Cursor::IfKeyword);
}

IfStmtSyntax IfStmtSyntax::withLeftParen(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> rawLeftParen;
   if (leftParen.has_value()) {
      rawLeftParen = leftParen->getRaw();
   } else {
      rawLeftParen = make_missing_token(T_LEFT_PAREN);
   }
   return m_data->replaceChild<IfStmtSyntax>(rawLeftParen, Cursor::LeftParen);
}

IfStmtSyntax IfStmtSyntax::withCondition(std::optional<ExprSyntax> condition)
{
   RefCountPtr<RawSyntax> rawCondition;
   if (condition.has_value()) {
      rawCondition = condition->getRaw();
   } else {
      rawCondition = RawSyntax::missing(SyntaxKind::UnknownExpr);
   }
   return m_data->replaceChild<IfStmtSyntax>(rawCondition, Cursor::Condition);
}

IfStmtSyntax IfStmtSyntax::withRightParen(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rawRightParen;
   if (rightParen.has_value()) {
      rawRightParen = rightParen->getRaw();
   } else {
      rawRightParen = make_missing_token(T_RIGHT_PAREN);
   }
   return m_data->replaceChild<IfStmtSyntax>(rawRightParen, Cursor::RightParen);
}

IfStmtSyntax IfStmtSyntax::withBody(std::optional<StmtSyntax> body)
{
   RefCountPtr<RawSyntax> rawBody;
   if (body.has_value()) {
      rawBody = body->getRaw();
   } else {
      rawBody = RawSyntax::missing(SyntaxKind::Stmt);
   }
   return m_data->replaceChild<IfStmtSyntax>(rawBody, Cursor::Body);
}

IfStmtSyntax IfStmtSyntax::withElseIfClauses(std::optional<ElseIfListSyntax> elseIfClauses)
{
   RefCountPtr<RawSyntax> rawElseIfClauses;
   if (elseIfClauses.has_value()) {
      rawElseIfClauses = elseIfClauses->getRaw();
   } else {
      rawElseIfClauses = nullptr;
   }
   return m_data->replaceChild<IfStmtSyntax>(rawElseIfClauses, Cursor::ElseIfClauses);
}

IfStmtSyntax IfStmtSyntax::withElseKeyword(std::optional<TokenSyntax> elseKeyword)
{
   RefCountPtr<RawSyntax> rawElseKeyword;
   if (elseKeyword.has_value()) {
      rawElseKeyword = elseKeyword->getRaw();
   } else {
      rawElseKeyword = nullptr;
   }
   return m_data->replaceChild<IfStmtSyntax>(rawElseKeyword, Cursor::ElseKeyword);
}

IfStmtSyntax IfStmtSyntax::withElseBody(std::optional<Syntax> elseBody)
{
   RefCountPtr<RawSyntax> rawElseBody;
   if (elseBody.has_value()) {
      rawElseBody = elseBody->getRaw();
   } else {
      rawElseBody = nullptr;
   }
   return m_data->replaceChild<IfStmtSyntax>(rawElseBody, Cursor::ElseBody);
}

IfStmtSyntax IfStmtSyntax::addElseIfClause(ElseIfClauseSyntax elseIfClause)
{
   RefCountPtr<RawSyntax> elseIfClauses = getRaw()->getChild(Cursor::ElseIfClauses);
   if (elseIfClauses) {
      elseIfClauses = elseIfClauses->append(elseIfClause.getRaw());
   } else {
      elseIfClauses = RawSyntax::make(SyntaxKind::ElseIfList, {elseIfClause.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<IfStmtSyntax>(elseIfClauses, Cursor::ElseIfClauses);
}

///
/// WhileStmtSyntax
///
void WhileStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == WhileStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LabelName, std::set{TokenKindType::T_IDENTIFIER_STRING});
   syntax_assert_child_token(raw, LabelColon, std::set{TokenKindType::T_COLON});
   syntax_assert_child_token(raw, WhileKeyword, std::set{TokenKindType::T_WHILE});
   syntax_assert_child_token(raw, LeftParen, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_kind(raw, Conditions, std::set{SyntaxKind::ConditionElementList});
   syntax_assert_child_token(raw, RightParen, std::set{TokenKindType::T_RIGHT_PAREN});
   syntax_assert_child_kind(raw, Body, std::set{SyntaxKind::CodeBlock});
#endif
}

std::optional<TokenSyntax> WhileStmtSyntax::getLabelName()
{
   RefCountPtr<SyntaxData> labelNameData = m_data->getChild(Cursor::LabelName);
   if (!labelNameData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, labelNameData.get()};
}

std::optional<TokenSyntax> WhileStmtSyntax::getLabelColon()
{
   RefCountPtr<SyntaxData> labelColonData = m_data->getChild(Cursor::LabelColon);
   if (!labelColonData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, labelColonData.get()};
}

TokenSyntax WhileStmtSyntax::getWhileKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::WhileKeyword).get()};
}

TokenSyntax WhileStmtSyntax::getLeftParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftParen).get()};
}

ConditionElementListSyntax WhileStmtSyntax::getConditions()
{
   return ConditionElementListSyntax{m_root, m_data->getChild(Cursor::Conditions).get()};
}

TokenSyntax WhileStmtSyntax::getRightParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::RightParen).get()};
}

InnerCodeBlockStmtSyntax WhileStmtSyntax::getBody()
{
   return InnerCodeBlockStmtSyntax{m_root, m_data->getChild(Cursor::Body).get()};
}

WhileStmtSyntax WhileStmtSyntax::withLabelName(std::optional<TokenSyntax> labelName)
{
   RefCountPtr<RawSyntax> rawLabelName;
   if (labelName.has_value()) {
      rawLabelName = labelName->getRaw();
   } else {
      rawLabelName = nullptr;
   }
   return m_data->replaceChild<WhileStmtSyntax>(rawLabelName, Cursor::LabelName);
}

WhileStmtSyntax WhileStmtSyntax::withLabelColon(std::optional<TokenSyntax> labelColon)
{
   RefCountPtr<RawSyntax> rawLabelColon;
   if (labelColon.has_value()) {
      rawLabelColon = labelColon->getRaw();
   } else {
      rawLabelColon = nullptr;
   }
   return m_data->replaceChild<WhileStmtSyntax>(rawLabelColon, Cursor::LabelColon);
}

WhileStmtSyntax WhileStmtSyntax::withWhileKeyword(std::optional<TokenSyntax> whileKeyword)
{
   RefCountPtr<RawSyntax> rawWhileKeyword;
   if (whileKeyword.has_value()) {
      rawWhileKeyword = whileKeyword->getRaw();
   } else {
      rawWhileKeyword = make_missing_token(T_WHILE);
   }
   return m_data->replaceChild<WhileStmtSyntax>(rawWhileKeyword, Cursor::WhileKeyword);
}

WhileStmtSyntax WhileStmtSyntax::withLeftParen(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> rawLeftParen;
   if (leftParen.has_value()) {
      rawLeftParen = leftParen->getRaw();
   } else {
      rawLeftParen = make_missing_token(T_LEFT_PAREN);
   }
   return m_data->replaceChild<WhileStmtSyntax>(rawLeftParen, Cursor::LeftParen);
}

WhileStmtSyntax WhileStmtSyntax::withConditions(std::optional<ConditionElementListSyntax> conditions)
{
   RefCountPtr<RawSyntax> rawConditions;
   if (conditions.has_value()) {
      rawConditions = conditions->getRaw();
   } else {
      rawConditions = RawSyntax::missing(SyntaxKind::ConditionElementList);
   }
   return m_data->replaceChild<WhileStmtSyntax>(rawConditions, Cursor::Conditions);
}

WhileStmtSyntax WhileStmtSyntax::withRightParen(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rawRightParen;
   if (rightParen.has_value()) {
      rawRightParen = rightParen->getRaw();
   } else {
      rawRightParen = make_missing_token(T_RIGHT_PAREN);
   }
   return m_data->replaceChild<WhileStmtSyntax>(rawRightParen, Cursor::RightParen);
}

WhileStmtSyntax WhileStmtSyntax::withBody(std::optional<InnerCodeBlockStmtSyntax> body)
{
   RefCountPtr<RawSyntax> rawBody;
   if (body.has_value()) {
      rawBody = body->getRaw();
   } else {
      rawBody = RawSyntax::missing(SyntaxKind::CodeBlock);
   }
   return m_data->replaceChild<WhileStmtSyntax>(rawBody, Cursor::Body);
}

WhileStmtSyntax WhileStmtSyntax::addCondition(ConditionElementSyntax condition)
{
   RefCountPtr<RawSyntax> conditions = getRaw()->getChild(Cursor::Conditions);
   if (conditions) {
      conditions = conditions->append(condition.getRaw());
   } else {
      conditions = RawSyntax::make(SyntaxKind::ConditionElementList, {condition.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<WhileStmtSyntax>(conditions, Cursor::Conditions);
}

///
/// DoWhileStmtSyntax
///
void DoWhileStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == DoWhileStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LabelName, std::set{TokenKindType::T_IDENTIFIER_STRING});
   syntax_assert_child_token(raw, LabelColon, std::set{TokenKindType::T_COLON});
   syntax_assert_child_token(raw, DoKeyword, std::set{TokenKindType::T_DO});
   syntax_assert_child_kind(raw, Body, std::set{SyntaxKind::CodeBlock});
   syntax_assert_child_token(raw, WhileKeyword, std::set{TokenKindType::T_WHILE});
   syntax_assert_child_token(raw, LeftParen, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_kind(raw, Condition, std::set{SyntaxKind::Expr});
   syntax_assert_child_token(raw, RightParen, std::set{TokenKindType::T_RIGHT_PAREN});
#endif
}

std::optional<TokenSyntax> DoWhileStmtSyntax::getLabelName()
{
   RefCountPtr<SyntaxData> labelNameData = m_data->getChild(Cursor::LabelName);
   if (!labelNameData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, labelNameData.get()};
}

std::optional<TokenSyntax> DoWhileStmtSyntax::getLabelColon()
{
   RefCountPtr<SyntaxData> labelColonData = m_data->getChild(Cursor::LabelColon);
   if (!labelColonData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, labelColonData.get()};
}

TokenSyntax DoWhileStmtSyntax::getDoKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::DoKeyword).get()};
}

InnerCodeBlockStmtSyntax DoWhileStmtSyntax::getBody()
{
   return InnerCodeBlockStmtSyntax{m_root, m_data->getChild(Cursor::Body).get()};
}

TokenSyntax DoWhileStmtSyntax::getWhileKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::WhileKeyword).get()};
}

TokenSyntax DoWhileStmtSyntax::getLeftParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftParen).get()};
}

ExprSyntax DoWhileStmtSyntax::getCondition()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Condition).get()};
}

TokenSyntax DoWhileStmtSyntax::getRightParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::RightParen).get()};
}

TokenSyntax DoWhileStmtSyntax::getSemicolon()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Semicolon).get()};
}

DoWhileStmtSyntax DoWhileStmtSyntax::withLabelName(std::optional<TokenSyntax> labelName)
{
   RefCountPtr<RawSyntax> rawLabelName;
   if (labelName.has_value()) {
      rawLabelName = labelName->getRaw();
   } else {
      rawLabelName = nullptr;
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawLabelName, Cursor::LabelName);
}

DoWhileStmtSyntax DoWhileStmtSyntax::withLabelColon(std::optional<TokenSyntax> labelColon)
{
   RefCountPtr<RawSyntax> rawLabelName;
   if (labelColon.has_value()) {
      rawLabelName = labelColon->getRaw();
   } else {
      rawLabelName = nullptr;
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawLabelName, Cursor::LabelColon);
}

DoWhileStmtSyntax DoWhileStmtSyntax::withDoKeyword(std::optional<TokenSyntax> doKeyword)
{
   RefCountPtr<RawSyntax> rawDoKeyword;
   if (doKeyword.has_value()) {
      rawDoKeyword = doKeyword->getRaw();
   } else {
      rawDoKeyword = make_missing_token(T_DO);
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawDoKeyword, Cursor::DoKeyword);
}

DoWhileStmtSyntax DoWhileStmtSyntax::withBody(std::optional<InnerCodeBlockStmtSyntax> body)
{
   RefCountPtr<RawSyntax> rawBody;
   if (body.has_value()) {
      rawBody = body->getRaw();
   } else {
      rawBody = RawSyntax::missing(SyntaxKind::CodeBlock);
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawBody, Cursor::Body);
}

DoWhileStmtSyntax DoWhileStmtSyntax::withWhileKeyword(std::optional<TokenSyntax> whileKeyword)
{
   RefCountPtr<RawSyntax> rawWhileKeyword;
   if (whileKeyword.has_value()) {
      rawWhileKeyword = whileKeyword->getRaw();
   } else {
      rawWhileKeyword = make_missing_token(T_WHILE);
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawWhileKeyword, Cursor::WhileKeyword);
}

DoWhileStmtSyntax DoWhileStmtSyntax::withLeftParen(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> rawLeftParen;
   if (leftParen.has_value()) {
      rawLeftParen = leftParen->getRaw();
   } else {
      rawLeftParen = make_missing_token(T_LEFT_PAREN);
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawLeftParen, Cursor::LeftParen);
}

DoWhileStmtSyntax DoWhileStmtSyntax::withCondition(std::optional<ExprSyntax> condition)
{
   RefCountPtr<RawSyntax> rawCondition;
   if (condition.has_value()) {
      rawCondition = condition->getRaw();
   } else {
      rawCondition = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawCondition, Cursor::Condition);
}

DoWhileStmtSyntax DoWhileStmtSyntax::withRightParen(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rawRightParen;
   if (rightParen.has_value()) {
      rawRightParen = rightParen->getRaw();
   } else {
      rawRightParen = make_missing_token(T_RIGHT_PAREN);
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawRightParen, Cursor::RightParen);
}

DoWhileStmtSyntax DoWhileStmtSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<DoWhileStmtSyntax>(rawSemicolon, Cursor::RightParen);
}

///
/// ForStmtSyntax
///
void ForStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ForStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ForToken, std::set{TokenKindType::T_FOR});
   syntax_assert_child_token(raw, LeftParenToken, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_kind(raw, InitializedExprs, std::set{SyntaxKind::ExprList});
   syntax_assert_child_token(raw, InitializedSemicolonToken, std::set{TokenKindType::T_SEMICOLON});
   syntax_assert_child_kind(raw, ConditionalExprs, std::set{SyntaxKind::ExprList});
   syntax_assert_child_token(raw, ConditionalSemicolonToken, std::set{TokenKindType::T_SEMICOLON});
   syntax_assert_child_kind(raw, OperationalExprs, std::set{SyntaxKind::ExprList});
   syntax_assert_child_token(raw, RightParenToken, std::set{TokenKindType::T_RIGHT_PAREN});
   syntax_assert_child_kind(raw, Stmt, std::set{SyntaxKind::Stmt});
#endif
}

TokenSyntax ForStmtSyntax::getForToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::ForToken).get()};
}

TokenSyntax ForStmtSyntax::getLeftParenToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftParenToken).get()};
}

std::optional<ExprListSyntax> ForStmtSyntax::getInitializedExprs()
{
   return ExprListSyntax {m_root, m_data->getChild(Cursor::InitializedExprs).get()};
}

TokenSyntax ForStmtSyntax::getInitializedSemicolonToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::InitializedSemicolonToken).get()};
}

std::optional<ExprListSyntax> ForStmtSyntax::getConditionalExprs()
{
   return ExprListSyntax {m_root, m_data->getChild(Cursor::ConditionalExprs).get()};
}

TokenSyntax ForStmtSyntax::getConditionalSemicolonToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::ConditionalSemicolonToken).get()};
}

std::optional<ExprListSyntax> ForStmtSyntax::getOperationalExprs()
{
   return ExprListSyntax {m_root, m_data->getChild(Cursor::OperationalExprs).get()};
}

TokenSyntax ForStmtSyntax::getRightParenToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightParenToken).get()};
}

StmtSyntax ForStmtSyntax::getStmt()
{
   return StmtSyntax {m_root, m_data->getChild(Cursor::Stmt).get()};
}

ForStmtSyntax ForStmtSyntax::withForToken(std::optional<TokenSyntax> forToken)
{
   RefCountPtr<RawSyntax> rawForToken;
   if (!forToken.has_value()) {
      rawForToken = forToken->getRaw();
   } else {
      rawForToken = RawSyntax::missing(SyntaxKind::ForStmt);
   }
   return m_data->replaceChild<ForStmtSyntax>(rawForToken, Cursor::ForToken);
}

ForStmtSyntax ForStmtSyntax::withLeftParenToken(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> rawLeftParen;
   if (!leftParen.has_value()) {
      rawLeftParen = leftParen->getRaw();
   } else {
      rawLeftParen = make_missing_token(T_LEFT_PAREN);
   }
   return m_data->replaceChild<ForStmtSyntax>(rawLeftParen, Cursor::LeftParenToken);
}

ForStmtSyntax ForStmtSyntax::withInitializedExprs(std::optional<TokenSyntax> exprs)
{
   RefCountPtr<RawSyntax> rawExprs;
   if (!exprs.has_value()) {
      rawExprs = exprs->getRaw();
   } else {
      rawExprs = RawSyntax::missing(SyntaxKind::ExprList);
   }
   return m_data->replaceChild<ForStmtSyntax>(rawExprs, Cursor::InitializedExprs);
}

ForStmtSyntax ForStmtSyntax::withInitializedSemicolonToken(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (!semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<ForStmtSyntax>(rawSemicolon, Cursor::InitializedSemicolonToken);
}

ForStmtSyntax ForStmtSyntax::withConditionalExprs(std::optional<TokenSyntax> exprs)
{
   RefCountPtr<RawSyntax> rawExprs;
   if (!exprs.has_value()) {
      rawExprs = exprs->getRaw();
   } else {
      rawExprs = RawSyntax::missing(SyntaxKind::ExprList);
   }
   return m_data->replaceChild<ForStmtSyntax>(rawExprs, Cursor::ConditionalExprs);
}

ForStmtSyntax ForStmtSyntax::withConditionalSemicolonToken(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (!semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<ForStmtSyntax>(rawSemicolon, Cursor::ConditionalSemicolonToken);
}

ForStmtSyntax ForStmtSyntax::withOperationalExprs(std::optional<TokenSyntax> exprs)
{
   RefCountPtr<RawSyntax> rawExprs;
   if (!exprs.has_value()) {
      rawExprs = exprs->getRaw();
   } else {
      rawExprs = RawSyntax::missing(SyntaxKind::ExprList);
   }
   return m_data->replaceChild<ForStmtSyntax>(rawExprs, Cursor::OperationalExprs);
}

ForStmtSyntax ForStmtSyntax::withRightParenToken(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rawRightParen;
   if (!rightParen.has_value()) {
      rawRightParen = rightParen->getRaw();
   } else {
      rawRightParen = make_missing_token(T_RIGHT_PAREN);
   }
   return m_data->replaceChild<ForStmtSyntax>(rawRightParen, Cursor::RightParenToken);
}

ForStmtSyntax ForStmtSyntax::withStmt(std::optional<TokenSyntax> stmt)
{
   RefCountPtr<RawSyntax> rawStmt;
   if (!stmt.has_value()) {
      rawStmt = stmt->getRaw();
   } else {
      rawStmt = RawSyntax::missing(SyntaxKind::Stmt);
   }
   return m_data->replaceChild<ForStmtSyntax>(rawStmt, Cursor::Stmt);
}

///
/// ForeachVariableSyntax
///
#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType ForeachVariableSyntax::CHILD_NODE_CHOICES
{
   {
      ForeachVariableSyntax::Variable, {
         SyntaxKind::VariableExpr, SyntaxKind::ReferencedVariableExpr,
               SyntaxKind::ListStructureClause, SyntaxKind::SimplifiedArrayCreateExpr,
      }
   }
};
#endif

void ForeachVariableSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ForStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Variable, CHILD_NODE_CHOICES.at(Cursor::Variable));
#endif
}

ExprSyntax ForeachVariableSyntax::getVariable()
{
   return ExprSyntax {m_root, m_data->getChild(Cursor::Variable).get()};
}

ForeachVariableSyntax ForeachVariableSyntax::withVariable(std::optional<ExprSyntax> variable)
{
   RefCountPtr<RawSyntax> rawVariable;
   if (variable.has_value()) {
      rawVariable = variable->getRaw();
   } else {
      rawVariable = RawSyntax::missing(SyntaxKind::VariableExpr);
   }
   return m_data->replaceChild<ForeachVariableSyntax>(rawVariable, Cursor::Variable);
}

///
/// ForeachStmtSyntax
///
void ForeachStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ForeachStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ForeachToken, std::set{TokenKindType::T_FOREACH});
   syntax_assert_child_token(raw, LeftParenToken, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_kind(raw, IterableExpr, std::set{SyntaxKind::Expr});
   syntax_assert_child_token(raw, AsToken, std::set{TokenKindType::T_AS});
   syntax_assert_child_kind(raw, KeyVariable, std::set{SyntaxKind::ForeachVariable});
   syntax_assert_child_token(raw, DoubleArrowToken, std::set{TokenKindType::T_DOUBLE_ARROW});
   syntax_assert_child_kind(raw, ValueVariable, std::set{SyntaxKind::ForeachVariable});
   syntax_assert_child_token(raw, RightParenToken, std::set{TokenKindType::T_RIGHT_PAREN});
   syntax_assert_child_kind(raw, Stmt, std::set{SyntaxKind::Stmt});
   if (raw->getChild(Cursor::KeyVariable)) {
      assert(raw->getChild(Cursor::DoubleArrowToken));
   }
#endif
}

TokenSyntax ForeachStmtSyntax::getForeachToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::ForeachToken).get()};
}

TokenSyntax ForeachStmtSyntax::getLeftParenToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftParenToken).get()};
}

ExprSyntax ForeachStmtSyntax::getIterableExpr()
{
   return ExprSyntax {m_root, m_data->getChild(Cursor::IterableExpr).get()};
}

TokenSyntax ForeachStmtSyntax::getAsToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::AsToken).get()};
}

std::optional<ForeachVariableSyntax> ForeachStmtSyntax::getKeyVariable()
{
   RefCountPtr<SyntaxData> keyData = m_data->getChild(Cursor::KeyVariable);
   if (!keyData) {
      return std::nullopt;
   }
   return ForeachVariableSyntax {m_root, keyData.get()};
}

std::optional<TokenSyntax> ForeachStmtSyntax::getDoubleArrowToken()
{
   RefCountPtr<SyntaxData> arrowData = m_data->getChild(Cursor::DoubleArrowToken);
   if (!arrowData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, arrowData.get()};
}

ForeachVariableSyntax ForeachStmtSyntax::getValueVariable()
{
   return ForeachVariableSyntax {m_root, m_data->getChild(Cursor::ValueVariable).get()};
}

TokenSyntax ForeachStmtSyntax::getRightParenToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightParenToken).get()};
}

StmtSyntax ForeachStmtSyntax::getStmt()
{
   return StmtSyntax {m_root, m_data->getChild(Cursor::Stmt).get()};
}

ForeachStmtSyntax
ForeachStmtSyntax::withForeachToken(std::optional<TokenSyntax> foreachToken)
{
   RefCountPtr<RawSyntax> rawForeachToken;
   if (foreachToken.has_value()) {
      rawForeachToken = foreachToken->getRaw();
   } else {
      rawForeachToken = make_missing_token(T_FOREACH);
   }
   return m_data->replaceChild<ForeachStmtSyntax>(rawForeachToken, Cursor::ForeachToken);
}

ForeachStmtSyntax
ForeachStmtSyntax::withLeftParenToken(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> rawLeftParen;
   if (leftParen.has_value()) {
      rawLeftParen = leftParen->getRaw();
   } else {
      rawLeftParen = make_missing_token(T_LEFT_PAREN);
   }
   return m_data->replaceChild<ForeachStmtSyntax>(rawLeftParen, Cursor::LeftParenToken);
}

ForeachStmtSyntax
ForeachStmtSyntax::withIterableExpr(std::optional<ExprSyntax> iterableExpr)
{
   RefCountPtr<RawSyntax> rawIterableExpr;
   if (iterableExpr.has_value()) {
      rawIterableExpr = iterableExpr->getRaw();
   } else {
      rawIterableExpr = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<ForeachStmtSyntax>(rawIterableExpr, Cursor::IterableExpr);
}

ForeachStmtSyntax
ForeachStmtSyntax::withAsToken(std::optional<TokenSyntax> asToken)
{
   RefCountPtr<RawSyntax> rawAsToken;
   if (asToken.has_value()) {
      rawAsToken = asToken->getRaw();
   } else {
      rawAsToken = make_missing_token(T_AS);
   }
   return m_data->replaceChild<ForeachStmtSyntax>(rawAsToken, Cursor::AsToken);
}

ForeachStmtSyntax
ForeachStmtSyntax::withKeyVariable(std::optional<ForeachVariableSyntax> key)
{
   RefCountPtr<RawSyntax> rawKey;
   if (key.has_value()) {
      rawKey = key->getRaw();
   } else {
      rawKey = RawSyntax::missing(SyntaxKind::ForeachVariable);
   }
   return m_data->replaceChild<ForeachStmtSyntax>(rawKey, Cursor::KeyVariable);
}

ForeachStmtSyntax
ForeachStmtSyntax::withDoubleArrowToken(std::optional<TokenSyntax> doubleArrow)
{
   RefCountPtr<RawSyntax> rawDoubleArrow;
   if (doubleArrow.has_value()) {
      rawDoubleArrow = doubleArrow->getRaw();
   } else {
      rawDoubleArrow = make_missing_token(T_DOUBLE_ARROW);
   }
   return m_data->replaceChild<ForeachStmtSyntax>(rawDoubleArrow, Cursor::DoubleArrowToken);
}

ForeachStmtSyntax
ForeachStmtSyntax::withValueVariable(std::optional<ForeachVariableSyntax> value)
{
   RefCountPtr<RawSyntax> rawValue;
   if (value.has_value()) {
      rawValue = value->getRaw();
   } else {
      rawValue = RawSyntax::missing(SyntaxKind::ForeachVariable);
   }
   return m_data->replaceChild<ForeachStmtSyntax>(rawValue, Cursor::ValueVariable);
}

ForeachStmtSyntax
ForeachStmtSyntax::withRightParenToken(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rawRightParen;
   if (rightParen.has_value()) {
      rawRightParen = rightParen->getRaw();
   } else {
      rawRightParen = make_missing_token(T_RIGHT_PAREN);
   }
   return m_data->replaceChild<ForeachStmtSyntax>(rawRightParen, Cursor::RightParenToken);
}

ForeachStmtSyntax
ForeachStmtSyntax::withStmt(std::optional<StmtSyntax> stmt)
{
   RefCountPtr<RawSyntax> rawStmt;
   if (stmt.has_value()) {
      rawStmt = stmt->getRaw();
   } else {
      rawStmt = RawSyntax::missing(SyntaxKind::Stmt);
   }
   return m_data->replaceChild<ForeachStmtSyntax>(rawStmt, Cursor::Stmt);
}

///
/// SwitchDefaultLabelSyntax
///
void SwitchDefaultLabelSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SwitchDefaultLabelSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, DefaultKeyword, std::set{TokenKindType::T_DEFAULT});
   syntax_assert_child_token(raw, Colon, std::set{TokenKindType::T_COLON});
#endif
}

TokenSyntax SwitchDefaultLabelSyntax::getDefaultKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::DefaultKeyword).get()};
}

TokenSyntax SwitchDefaultLabelSyntax::getColon()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Colon).get()};
}

SwitchDefaultLabelSyntax SwitchDefaultLabelSyntax::withDefaultKeyword(std::optional<TokenSyntax> defaultKeyword)
{
   RefCountPtr<RawSyntax> rawDefaultKeyword;
   if (defaultKeyword.has_value()) {
      rawDefaultKeyword = defaultKeyword->getRaw();
   } else {
      rawDefaultKeyword = make_missing_token(T_DEFAULT);
   }
   return m_data->replaceChild<SwitchDefaultLabelSyntax>(rawDefaultKeyword, Cursor::DefaultKeyword);
}

SwitchDefaultLabelSyntax SwitchDefaultLabelSyntax::withColon(std::optional<TokenSyntax> colon)
{
   RefCountPtr<RawSyntax> rawColon;
   if (colon.has_value()) {
      rawColon = colon->getRaw();
   } else {
      rawColon = make_missing_token(T_COLON);
   }
   return m_data->replaceChild<SwitchDefaultLabelSyntax>(rawColon, Cursor::Colon);
}

///
/// SwitchCaseLabelSyntax
///
void SwitchCaseLabelSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SwitchCaseLabelSyntax::CHILDREN_COUNT);

   syntax_assert_child_token(raw, CaseKeyword, std::set{TokenKindType::T_CASE});
   syntax_assert_child_kind(raw, Expr, std::set{SyntaxKind::Expr});
   syntax_assert_child_token(raw, Colon, std::set{TokenKindType::T_COLON});
#endif
}

TokenSyntax SwitchCaseLabelSyntax::getCaseKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::CaseKeyword).get()};
}

ExprSyntax SwitchCaseLabelSyntax::getExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Expr).get()};
}

TokenSyntax SwitchCaseLabelSyntax::getColon()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Colon).get()};
}

SwitchCaseLabelSyntax SwitchCaseLabelSyntax::withCaseKeyword(std::optional<TokenSyntax> caseKeyword)
{
   RefCountPtr<RawSyntax> rawCaseKeyword;
   if (caseKeyword.has_value()) {
      rawCaseKeyword = caseKeyword->getRaw();
   } else {
      rawCaseKeyword = make_missing_token(T_CASE);
   }
   return m_data->replaceChild<SwitchCaseLabelSyntax>(rawCaseKeyword, Cursor::CaseKeyword);
}

SwitchCaseLabelSyntax SwitchCaseLabelSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> rawExpr;
   if (expr.has_value()) {
      rawExpr = expr->getRaw();
   } else {
      rawExpr = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<SwitchCaseLabelSyntax>(rawExpr, Cursor::Expr);
}

SwitchCaseLabelSyntax SwitchCaseLabelSyntax::withColon(std::optional<TokenSyntax> colon)
{
   RefCountPtr<RawSyntax> rawColon;
   if (colon.has_value()) {
      rawColon = colon->getRaw();
   } else {
      rawColon = make_missing_token(T_COLON);
   }
   return m_data->replaceChild<SwitchCaseLabelSyntax>(rawColon, Cursor::Colon);
}

///
/// SwitchCaseSyntax
///

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType SwitchCaseSyntax::CHILD_NODE_CHOICES
{
   {
      SwitchCaseSyntax::Label,{
         SyntaxKind::SwitchDefaultLabel,
               SyntaxKind::SwitchCaseLabel
      }
   }
};
#endif

void SwitchCaseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SwitchCaseSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Label, CHILD_NODE_CHOICES.at(Cursor::Label));
   syntax_assert_child_kind(raw, Statements, std::set{SyntaxKind::CodeBlockItemList});
#endif
}

Syntax SwitchCaseSyntax::getLabel()
{
   return Syntax{m_root, m_data->getChild(Cursor::Label).get()};
}

InnerCodeBlockStmtSyntax SwitchCaseSyntax::getStatements()
{
   return InnerCodeBlockStmtSyntax {m_root, m_data->getChild(Cursor::Statements).get()};
}

SwitchCaseSyntax SwitchCaseSyntax::withLabel(std::optional<Syntax> label)
{
   RefCountPtr<RawSyntax> rawLabel;
   if (label.has_value()) {
      rawLabel = label->getRaw();
   } else {
      rawLabel = RawSyntax::missing(SyntaxKind::SwitchDefaultLabel);
   }
   return m_data->replaceChild<SwitchCaseSyntax>(rawLabel, Cursor::Label);
}

SwitchCaseSyntax SwitchCaseSyntax::withStatements(std::optional<InnerStmtListSyntax> statements)
{
   RefCountPtr<RawSyntax> rawStatements;
   if (statements.has_value()) {
      rawStatements = statements->getRaw();
   } else {
      rawStatements = RawSyntax::missing(SyntaxKind::InnerStmtList);
   }
   return m_data->replaceChild<SwitchCaseSyntax>(rawStatements, Cursor::Statements);
}

SwitchCaseSyntax SwitchCaseSyntax::addStatement(InnerStmtSyntax statement)
{
   RefCountPtr<RawSyntax> statements = getRaw()->getChild(Cursor::Statements);
   if (statements) {
      statements->append(statement.getRaw());
   } else {
      statements = RawSyntax::make(SyntaxKind::InnerStmtList, {statement.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<SwitchCaseSyntax>(statements, Cursor::Statements);
}

///
/// SwitchStmtSyntax
///
void SwitchStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SwitchStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LabelName, std::set{TokenKindType::T_IDENTIFIER_STRING});
   syntax_assert_child_token(raw, LabelColon, std::set{TokenKindType::T_COLON});
   syntax_assert_child_token(raw, SwitchKeyword, std::set{TokenKindType::T_SWITCH});
   syntax_assert_child_token(raw, LeftParen, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_kind(raw, ConditionExpr, std::set{SyntaxKind::Expr});
   syntax_assert_child_token(raw, RightParen, std::set{TokenKindType::T_RIGHT_PAREN});
   syntax_assert_child_token(raw, LeftBrace, std::set{TokenKindType::T_LEFT_BRACE});
   syntax_assert_child_kind(raw, Cases, std::set{SyntaxKind::SwitchCaseList});
   syntax_assert_child_token(raw, RightBrace, std::set{TokenKindType::T_RIGHT_BRACE});
#endif
}

std::optional<TokenSyntax> SwitchStmtSyntax::getLabelName()
{
   RefCountPtr<SyntaxData> labelNameData = m_data->getChild(Cursor::LabelName);
   if (!labelNameData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, labelNameData.get()};
}

std::optional<TokenSyntax> SwitchStmtSyntax::getLabelColon()
{
   RefCountPtr<SyntaxData> labelColonData = m_data->getChild(Cursor::LabelColon);
   if (!labelColonData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, labelColonData.get()};
}

TokenSyntax SwitchStmtSyntax::getSwitchKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::SwitchKeyword).get()};
}

TokenSyntax SwitchStmtSyntax::getLeftParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftParen).get()};
}

ExprSyntax SwitchStmtSyntax::getConditionExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::ConditionExpr).get()};
}

TokenSyntax SwitchStmtSyntax::getRightParen()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::RightParen).get()};
}

TokenSyntax SwitchStmtSyntax::getLeftBrace()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftBrace).get()};
}

SwitchCaseListSyntax SwitchStmtSyntax::getCases()
{
   return SwitchCaseListSyntax{m_root, m_data->getChild(Cursor::Cases).get()};
}

TokenSyntax SwitchStmtSyntax::getRightBrace()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::RightBrace).get()};
}

SwitchStmtSyntax SwitchStmtSyntax::withLabelName(std::optional<TokenSyntax> labelName)
{
   RefCountPtr<RawSyntax> rawLabelName;
   if (labelName.has_value()) {
      rawLabelName = labelName->getRaw();
   } else {
      rawLabelName = nullptr;
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawLabelName, Cursor::LabelName);
}

SwitchStmtSyntax SwitchStmtSyntax::withLabelColon(std::optional<TokenSyntax> labelColon)
{
   RefCountPtr<RawSyntax> rawLabelColon;
   if (labelColon.has_value()) {
      rawLabelColon = labelColon->getRaw();
   } else {
      rawLabelColon = nullptr;
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawLabelColon, Cursor::LabelColon);
}

SwitchStmtSyntax SwitchStmtSyntax::withSwitchKeyword(std::optional<TokenSyntax> switchKeyword)
{
   RefCountPtr<RawSyntax> rawSwitchKeyword;
   if (switchKeyword.has_value()) {
      rawSwitchKeyword = switchKeyword->getRaw();
   } else {
      rawSwitchKeyword = make_missing_token(T_SWITCH);
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawSwitchKeyword, Cursor::SwitchKeyword);
}

SwitchStmtSyntax SwitchStmtSyntax::withLeftParen(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> rawLeftParen;
   if (leftParen.has_value()) {
      rawLeftParen = leftParen->getRaw();
   } else {
      rawLeftParen = make_missing_token(T_LEFT_PAREN);
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawLeftParen, Cursor::LeftParen);
}

SwitchStmtSyntax SwitchStmtSyntax::withConditionExpr(std::optional<ExprSyntax> conditionExpr)
{
   RefCountPtr<RawSyntax> rawConditionExpr;
   if (conditionExpr.has_value()) {
      rawConditionExpr = conditionExpr->getRaw();
   } else {
      rawConditionExpr = RawSyntax::missing(SyntaxKind::UnknownExpr);
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawConditionExpr, Cursor::ConditionExpr);
}

SwitchStmtSyntax SwitchStmtSyntax::withRightParen(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rawRightParen;
   if (rightParen.has_value()) {
      rawRightParen = rightParen->getRaw();
   } else {
      rawRightParen = make_missing_token(T_RIGHT_PAREN);
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawRightParen, Cursor::RightParen);
}

SwitchStmtSyntax SwitchStmtSyntax::withLeftBrace(std::optional<TokenSyntax> leftBrace)
{
   RefCountPtr<RawSyntax> rawLeftBrace;
   if (leftBrace.has_value()) {
      rawLeftBrace = leftBrace->getRaw();
   } else {
      rawLeftBrace = make_missing_token(T_LEFT_BRACE);
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawLeftBrace, Cursor::LeftBrace);
}

SwitchStmtSyntax SwitchStmtSyntax::withCases(std::optional<SwitchCaseListSyntax> cases)
{
   RefCountPtr<RawSyntax> rawCases;
   if (cases.has_value()) {
      rawCases = cases->getRaw();
   } else {
      rawCases = RawSyntax::missing(SyntaxKind::SwitchCaseList);
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawCases, Cursor::Cases);
}

SwitchStmtSyntax SwitchStmtSyntax::withRightBrace(std::optional<TokenSyntax> rightBrace)
{
   RefCountPtr<RawSyntax> rawRightBrace;
   if (rightBrace.has_value()) {
      rawRightBrace = rightBrace->getRaw();
   } else {
      rawRightBrace = make_missing_token(T_RIGHT_BRACE);
   }
   return m_data->replaceChild<SwitchStmtSyntax>(rawRightBrace, Cursor::RightBrace);
}

SwitchStmtSyntax SwitchStmtSyntax::addCase(SwitchCaseSyntax switchCase)
{
   RefCountPtr<RawSyntax> cases = getRaw()->getChild(Cursor::Cases);
   if (cases) {
      cases = cases->append(switchCase.getRaw());
   } else {
      cases = RawSyntax::make(SyntaxKind::SwitchCaseList, {switchCase.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<SwitchStmtSyntax>(cases, Cursor::Cases);
}

void DeferStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == DeferStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, DeferKeyword, std::set{TokenKindType::T_DEFER});
   syntax_assert_child_kind(raw, Body, std::set{SyntaxKind::CodeBlock});
#endif
}

TokenSyntax DeferStmtSyntax::getDeferKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::DeferKeyword).get()};
}

InnerCodeBlockStmtSyntax DeferStmtSyntax::getBody()
{
   return InnerCodeBlockStmtSyntax {m_root, m_data->getChild(Cursor::Body).get()};
}

DeferStmtSyntax DeferStmtSyntax::withDeferKeyword(std::optional<TokenSyntax> deferKeyword)
{
   RefCountPtr<RawSyntax> rawDeferKeyword;
   if (deferKeyword.has_value()) {
      rawDeferKeyword = deferKeyword->getRaw();
   } else {
      rawDeferKeyword = make_missing_token(T_DEFER);
   }
   return m_data->replaceChild<DeferStmtSyntax>(rawDeferKeyword, Cursor::DeferKeyword);
}

DeferStmtSyntax DeferStmtSyntax::withBody(std::optional<InnerCodeBlockStmtSyntax> body)
{
   RefCountPtr<RawSyntax> rawBody;
   if (body.has_value()) {
      rawBody = body->getRaw();
   } else {
      rawBody = RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt);
   }
   return m_data->replaceChild<DeferStmtSyntax>(rawBody, Cursor::Body);
}

///
/// ThrowStmtSyntax
///
void ThrowStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ThrowStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ThrowKeyword, std::set{TokenKindType::T_THROW});
   syntax_assert_child_kind(raw, Expr, std::set{SyntaxKind::Expr});
   syntax_assert_child_token(raw, Semicolon, std::set{TokenKindType::T_SEMICOLON});
#endif
}

TokenSyntax ThrowStmtSyntax::getThrowKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::ThrowKeyword).get()};
}

ExprSyntax ThrowStmtSyntax::getExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Expr).get()};
}

TokenSyntax ThrowStmtSyntax::getSemicolon()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Semicolon).get()};
}

ThrowStmtSyntax ThrowStmtSyntax::withThrowKeyword(std::optional<TokenSyntax> throwKeyword)
{
   RefCountPtr<RawSyntax> rawThrowKeyword;
   if (throwKeyword.has_value()) {
      rawThrowKeyword = throwKeyword->getRaw();
   } else {
      rawThrowKeyword = make_missing_token(T_THROW);
   }
   return m_data->replaceChild<ThrowStmtSyntax>(rawThrowKeyword, Cursor::ThrowKeyword);
}

ThrowStmtSyntax ThrowStmtSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> rawExpr;
   if (expr.has_value()) {
      rawExpr = expr->getRaw();
   } else {
      rawExpr = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<ThrowStmtSyntax>(rawExpr, Cursor::Expr);
}

ThrowStmtSyntax ThrowStmtSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<ThrowStmtSyntax>(rawSemicolon, Cursor::Semicolon);
}

///
/// TryStmtSyntax
///
void TryStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == TryStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, TryToken, std::set{TokenKindType::T_TRY});
   syntax_assert_child_kind(raw, CodeBlock, std::set{SyntaxKind::InnerCodeBlockStmt});
   syntax_assert_child_kind(raw, CatchList, std::set{SyntaxKind::CatchList});
   syntax_assert_child_kind(raw, FinallyClause, std::set{SyntaxKind::FinallyClause});
#endif
}

TokenSyntax TryStmtSyntax::getTryToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::TryToken).get()};
}

InnerCodeBlockStmtSyntax TryStmtSyntax::getCodeBlock()
{
   return InnerCodeBlockStmtSyntax {m_root, m_data->getChild(Cursor::CodeBlock).get()};
}

std::optional<CatchListSyntax> TryStmtSyntax::getCatchList()
{
   return CatchListSyntax {m_root, m_data->getChild(Cursor::CatchList).get()};
}

std::optional<FinallyClauseSyntax> TryStmtSyntax::getFinallyClause()
{
   return FinallyClauseSyntax {m_root, m_data->getChild(Cursor::FinallyClause).get()};
}

TryStmtSyntax TryStmtSyntax::withTryToken(std::optional<TokenSyntax> tryToken)
{
   RefCountPtr<RawSyntax> rawTryToken;
   if (tryToken.has_value()) {
      rawTryToken = tryToken->getRaw();
   } else {
      rawTryToken = make_missing_token(T_TRY);
   }
   return m_data->replaceChild<TryStmtSyntax>(rawTryToken, Cursor::TryToken);
}

TryStmtSyntax TryStmtSyntax::withCodeBlock(std::optional<InnerCodeBlockStmtSyntax> codeBlock)
{
   RefCountPtr<RawSyntax> rawCodeBlock;
   if (codeBlock.has_value()) {
      rawCodeBlock = codeBlock->getRaw();
   } else {
      rawCodeBlock = RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt);
   }
   return m_data->replaceChild<TryStmtSyntax>(rawCodeBlock, Cursor::CodeBlock);
}

TryStmtSyntax TryStmtSyntax::withCatchList(std::optional<CatchListSyntax> catchList)
{
   RefCountPtr<RawSyntax> rawCatchList;
   if (catchList.has_value()) {
      rawCatchList = catchList->getRaw();
   } else {
      rawCatchList = nullptr;
   }
   return m_data->replaceChild<TryStmtSyntax>(rawCatchList, Cursor::CatchList);
}

TryStmtSyntax TryStmtSyntax::withFinallyClause(std::optional<FinallyClauseSyntax> finallyClause)
{
   RefCountPtr<RawSyntax> rawFinallyClause;
   if (finallyClause.has_value()) {
      rawFinallyClause = finallyClause->getRaw();
   } else {
      rawFinallyClause = nullptr;
   }
   return m_data->replaceChild<TryStmtSyntax>(rawFinallyClause, Cursor::FinallyClause);
}

///
/// FinallyClauseSyntax
///
void FinallyClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == FinallyClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, FinallyToken, std::set{TokenKindType::T_FINALLY});
   syntax_assert_child_kind(raw, CodeBlock, std::set{SyntaxKind::InnerCodeBlockStmt});
#endif
}

TokenSyntax FinallyClauseSyntax::getFinallyToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::FinallyToken).get()};
}

InnerCodeBlockStmtSyntax FinallyClauseSyntax::getCodeBlock()
{
   return InnerCodeBlockStmtSyntax {m_root, m_data->getChild(Cursor::CodeBlock).get()};
}

FinallyClauseSyntax
FinallyClauseSyntax::withFinallyToken(std::optional<TokenSyntax> finallyToken)
{
   RefCountPtr<RawSyntax> rawFinallyToken;
   if (finallyToken.has_value()) {
      rawFinallyToken = finallyToken->getRaw();
   } else {
      rawFinallyToken = make_missing_token(T_FINALLY);
   }
   return m_data->replaceChild<FinallyClauseSyntax>(rawFinallyToken, Cursor::FinallyToken);
}

FinallyClauseSyntax
FinallyClauseSyntax::withCodeBlock(std::optional<InnerCodeBlockStmtSyntax> codeBlock)
{
   RefCountPtr<RawSyntax> rawCodeBlock;
   if (codeBlock.has_value()) {
      rawCodeBlock = codeBlock->getRaw();
   } else {
      rawCodeBlock = RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt);
   }
   return m_data->replaceChild<FinallyClauseSyntax>(rawCodeBlock, Cursor::CodeBlock);
}

///
/// CatchArgTypeHintItemSyntax
///
void CatchArgTypeHintItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == CatchArgTypeHintItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, TypeName, std::set{SyntaxKind::Name});
   syntax_assert_child_token(raw, Separator, std::set{TokenKindType::T_VBAR});
#endif
}


std::optional<TokenSyntax> CatchArgTypeHintItemSyntax::getSeparator()
{
   RefCountPtr<SyntaxData> separatorData = m_data->getChild(Cursor::Separator);
   if (!separatorData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, separatorData.get()};
}

NameSyntax CatchArgTypeHintItemSyntax::getTypeName()
{
   return NameSyntax {m_root, m_data->getChild(Cursor::TypeName).get()};
}

CatchArgTypeHintItemSyntax
CatchArgTypeHintItemSyntax::withSeparator(std::optional<TokenSyntax> separator)
{
   RefCountPtr<RawSyntax> rawSeparator;
   if (separator.has_value()) {
      rawSeparator = separator->getRaw();
   } else {
      rawSeparator = nullptr;
   }
   return m_data->replaceChild<CatchArgTypeHintItemSyntax>(rawSeparator, Cursor::Separator);
}

CatchArgTypeHintItemSyntax
CatchArgTypeHintItemSyntax::withTypeName(std::optional<NameSyntax> typeName)
{
   RefCountPtr<RawSyntax> rawTypeName;
   if (typeName.has_value()) {
      rawTypeName = typeName->getRaw();
   } else {
      rawTypeName = RawSyntax::missing(SyntaxKind::Name);
   }
   return m_data->replaceChild<CatchArgTypeHintItemSyntax>(rawTypeName, Cursor::TypeName);
}

///
/// CatchListItemClauseSyntax
///
void CatchListItemClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == CatchListItemClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, CatchToken, std::set{TokenKindType::T_CATCH});
   syntax_assert_child_token(raw, LeftParenToken, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_kind(raw, CatchArgTypeHintList, std::set{SyntaxKind::CatchArgTypeHintList});
   syntax_assert_child_token(raw, Variable, std::set{TokenKindType::T_VARIABLE});
   syntax_assert_child_token(raw, RightParenToken, std::set{TokenKindType::T_RIGHT_PAREN});
   syntax_assert_child_kind(raw, CodeBlock, std::set{SyntaxKind::InnerCodeBlockStmt});
#endif
}

TokenSyntax CatchListItemClauseSyntax::getCatchToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::CatchToken).get()};
}

TokenSyntax CatchListItemClauseSyntax::getLeftParenToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftParenToken).get()};
}

CatchArgTypeHintListSyntax CatchListItemClauseSyntax::getCatchArgTypeHintList()
{
   return CatchArgTypeHintListSyntax {m_root, m_data->getChild(Cursor::CatchArgTypeHintList).get()};
}

TokenSyntax CatchListItemClauseSyntax::getVariable()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Variable).get()};
}

TokenSyntax CatchListItemClauseSyntax::getRightParenToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightParenToken).get()};
}

InnerCodeBlockStmtSyntax CatchListItemClauseSyntax::getCodeBlock()
{
   return InnerCodeBlockStmtSyntax {m_root, m_data->getChild(Cursor::CodeBlock).get()};
}

CatchListItemClauseSyntax
CatchListItemClauseSyntax::withCatchToken(std::optional<TokenSyntax> catchToken)
{
   RefCountPtr<RawSyntax> rawCatchToken;
   if (catchToken.has_value()) {
      rawCatchToken = catchToken->getRaw();
   } else {
      rawCatchToken = make_missing_token(T_CATCH);
   }
   return m_data->replaceChild<CatchListItemClauseSyntax>(rawCatchToken, Cursor::CatchToken);
}

CatchListItemClauseSyntax
CatchListItemClauseSyntax::withLeftParenToken(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> rawLeftParen;
   if (leftParen.has_value()) {
      rawLeftParen = leftParen->getRaw();
   } else {
      rawLeftParen = make_missing_token(T_LEFT_PAREN);
   }
   return m_data->replaceChild<CatchListItemClauseSyntax>(rawLeftParen, Cursor::LeftParenToken);
}

CatchListItemClauseSyntax
CatchListItemClauseSyntax::withCatchArgTypeHintList(std::optional<CatchArgTypeHintListSyntax> typeHints)
{
   RefCountPtr<RawSyntax> rawTypeHints;
   if (typeHints.has_value()) {
      rawTypeHints = typeHints->getRaw();
   } else {
      rawTypeHints = RawSyntax::missing(SyntaxKind::CatchArgTypeHintList);
   }
   return m_data->replaceChild<CatchListItemClauseSyntax>(rawTypeHints, Cursor::CatchArgTypeHintList);
}

CatchListItemClauseSyntax
CatchListItemClauseSyntax::withVariable(std::optional<TokenSyntax> variable)
{
   RefCountPtr<RawSyntax> rawVariable;
   if (variable.has_value()) {
      rawVariable = variable->getRaw();
   } else {
      rawVariable = make_missing_token(T_VARIABLE);
   }
   return m_data->replaceChild<CatchListItemClauseSyntax>(rawVariable, Cursor::Variable);
}

CatchListItemClauseSyntax
CatchListItemClauseSyntax::withRightParenToken(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rawRightParen;
   if (rightParen.has_value()) {
      rawRightParen = rightParen->getRaw();
   } else {
      rawRightParen = make_missing_token(T_RIGHT_PAREN);
   }
   return m_data->replaceChild<CatchListItemClauseSyntax>(rawRightParen, Cursor::RightParenToken);
}

CatchListItemClauseSyntax
CatchListItemClauseSyntax::withCodeBlock(std::optional<InnerCodeBlockStmtSyntax> codeBlock)
{
   RefCountPtr<RawSyntax> rawCodeBlock;
   if (codeBlock.has_value()) {
      rawCodeBlock = codeBlock->getRaw();
   } else {
      rawCodeBlock = RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt);
   }
   return m_data->replaceChild<CatchListItemClauseSyntax>(rawCodeBlock, Cursor::CodeBlock);
}

CatchListItemClauseSyntax
CatchListItemClauseSyntax::addCatchArgTypeHintList(InnerCodeBlockStmtSyntax typeHint)
{
   RefCountPtr<RawSyntax> rawTypeHints = getRaw()->getChild(Cursor::CatchArgTypeHintList);
   if (rawTypeHints) {
      rawTypeHints->append(typeHint.getRaw());
   } else {
      rawTypeHints = RawSyntax::make(SyntaxKind::CatchArgTypeHintList, {typeHint.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<CatchListItemClauseSyntax>(rawTypeHints, Cursor::CatchArgTypeHintList);
}

///
/// ReturnStmtSyntax
///
void ReturnStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ReturnStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ReturnKeyword, std::set{TokenKindType::T_RETURN});
   syntax_assert_child_kind(raw, Expr, std::set{SyntaxKind::Expr});
   syntax_assert_child_token(raw, Semicolon, std::set{TokenKindType::T_SEMICOLON});
#endif
}

TokenSyntax ReturnStmtSyntax::getReturnKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::ReturnKeyword).get()};
}

ExprSyntax ReturnStmtSyntax::getExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Expr).get()};
}

TokenSyntax ReturnStmtSyntax::getSemicolon()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Semicolon).get()};
}

ReturnStmtSyntax ReturnStmtSyntax::withReturnKeyword(std::optional<TokenSyntax> returnKeyword)
{
   RefCountPtr<RawSyntax> raw;
   if (returnKeyword.has_value()) {
      raw = returnKeyword->getRaw();
   } else {
      raw = make_missing_token(T_RETURN);
   }
   return m_data->replaceChild<ReturnStmtSyntax>(raw, Cursor::ReturnKeyword);
}

ReturnStmtSyntax ReturnStmtSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> rawExpr;
   if (expr.has_value()) {
      rawExpr = expr->getRaw();
   } else {
      rawExpr = nullptr;
   }
   return m_data->replaceChild<ReturnStmtSyntax>(rawExpr, Cursor::Expr);
}

ReturnStmtSyntax ReturnStmtSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<ReturnStmtSyntax>(rawSemicolon, Cursor::Semicolon);
}

///
/// EchoStmtSyntax
///
void EchoStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == EchoStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, EchoToken, std::set{TokenKindType::T_ECHO});
   syntax_assert_child_kind(raw, ExprListClause, std::set{SyntaxKind::ExprList});
#endif
}

TokenSyntax EchoStmtSyntax::getEchoToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::EchoToken).get()};
}

ExprListSyntax EchoStmtSyntax::getExprListClause()
{
   return ExprListSyntax {m_root, m_data->getChild(Cursor::ExprListClause).get()};
}

TokenSyntax EchoStmtSyntax::getSemicolon()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Semicolon).get()};
}

EchoStmtSyntax EchoStmtSyntax::withEchoToken(std::optional<TokenSyntax> echoToken)
{
   RefCountPtr<RawSyntax> rawEchoToken;
   if (echoToken.has_value()) {
      rawEchoToken = echoToken->getRaw();
   } else {
      rawEchoToken = make_missing_token(T_ECHO);
   }
   return m_data->replaceChild<EchoStmtSyntax>(rawEchoToken, Cursor::EchoToken);
}

EchoStmtSyntax EchoStmtSyntax::withExprListClause(std::optional<ExprListSyntax> exprListClause)
{
   RefCountPtr<RawSyntax> rawExprListClause;
   if (exprListClause.has_value()) {
      rawExprListClause = exprListClause->getRaw();
   } else {
      rawExprListClause = RawSyntax::missing(SyntaxKind::ExprList);
   }
   return m_data->replaceChild<EchoStmtSyntax>(rawExprListClause, Cursor::ExprListClause);
}

EchoStmtSyntax EchoStmtSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<EchoStmtSyntax>(rawSemicolon, Cursor::Semicolon);
}

///
/// HaltCompilerStmtSyntax
///
void HaltCompilerStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == HaltCompilerStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, HaltCompilerToken, std::set{TokenKindType::T_HALT_COMPILER});
   syntax_assert_child_token(raw, LeftParen, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_token(raw, RightParen, std::set{TokenKindType::T_RIGHT_PAREN});
   syntax_assert_child_token(raw, Semicolon, std::set{TokenKindType::T_SEMICOLON});
#endif
}

TokenSyntax HaltCompilerStmtSyntax::getHaltCompilerToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::HaltCompilerToken).get()};
}

TokenSyntax HaltCompilerStmtSyntax::getLeftParen()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftParen).get()};
}

TokenSyntax HaltCompilerStmtSyntax::getRightParen()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightParen).get()};
}

TokenSyntax HaltCompilerStmtSyntax::getSemicolon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Semicolon).get()};
}

HaltCompilerStmtSyntax HaltCompilerStmtSyntax::withHaltCompilerToken(std::optional<TokenSyntax> haltCompilerToken)
{
   RefCountPtr<RawSyntax> rawHaltCompilerToken;
   if (haltCompilerToken.has_value()) {
      rawHaltCompilerToken = haltCompilerToken->getRaw();
   } else {
      rawHaltCompilerToken = make_missing_token(T_HALT_COMPILER);
   }
   return m_data->replaceChild<HaltCompilerStmtSyntax>(rawHaltCompilerToken, Cursor::HaltCompilerToken);
}

HaltCompilerStmtSyntax HaltCompilerStmtSyntax::withLeftParen(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> rawLeftParen;
   if (leftParen.has_value()) {
      rawLeftParen = leftParen->getRaw();
   } else {
      rawLeftParen = make_missing_token(T_LEFT_PAREN);
   }
   return m_data->replaceChild<HaltCompilerStmtSyntax>(rawLeftParen, Cursor::LeftParen);
}

HaltCompilerStmtSyntax HaltCompilerStmtSyntax::withRightParen(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rawRightParen;
   if (rightParen.has_value()) {
      rawRightParen = rightParen->getRaw();
   } else {
      rawRightParen = make_missing_token(T_RIGHT_PAREN);
   }
   return m_data->replaceChild<HaltCompilerStmtSyntax>(rawRightParen, Cursor::RightParen);
}

HaltCompilerStmtSyntax HaltCompilerStmtSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<HaltCompilerStmtSyntax>(rawSemicolon, Cursor::Semicolon);
}

///
/// GlobalVariableSyntax
///
void GlobalVariableSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == GlobalVariableSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, Variable, std::set{TokenKindType::T_VARIABLE});
#endif
}

SimpleVariableExprSyntax
GlobalVariableSyntax::getVariable()
{
   return SimpleVariableExprSyntax {m_root, m_data->getChild(Cursor::Variable).get()};
}

GlobalVariableSyntax
GlobalVariableSyntax::withVariable(std::optional<TokenSyntax> variable)
{
   RefCountPtr<RawSyntax> rawVariable;
   if (variable.has_value()) {
      rawVariable = variable->getRaw();
   } else {
      rawVariable = make_missing_token(T_VARIABLE);
   }
   return m_data->replaceChild<GlobalVariableSyntax>(rawVariable, Cursor::Variable);
}

///
/// GlobalVariableListItemSyntax
///
void GlobalVariableListItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == GlobalVariableListItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, Comma, std::set{TokenKindType::T_COMMA});
   syntax_assert_child_token(raw, Variable, std::set{TokenKindType::T_VARIABLE});
#endif
}

std::optional<TokenSyntax> GlobalVariableListItemSyntax::getComma()
{
   RefCountPtr<SyntaxData> commaData = m_data->getChild(Cursor::Comma);
   if (!commaData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, commaData.get()};
}

GlobalVariableSyntax GlobalVariableListItemSyntax::getVariable()
{
   return GlobalVariableSyntax {m_root, m_data->getChild(Cursor::Variable).get()};
}

GlobalVariableListItemSyntax
GlobalVariableListItemSyntax::withComma(std::optional<TokenSyntax> comma)
{
   RefCountPtr<RawSyntax> rawComma;
   if (comma.has_value()) {
      rawComma = comma->getRaw();
   } else {
      rawComma = nullptr;
   }
   return m_data->replaceChild<GlobalVariableListItemSyntax>(rawComma, Cursor::Comma);
}

GlobalVariableListItemSyntax
GlobalVariableListItemSyntax::withVariable(std::optional<GlobalVariableSyntax> variable)
{
   RefCountPtr<RawSyntax> rawVariable;
   if (variable.has_value()) {
      rawVariable = variable->getRaw();
   } else {
      rawVariable = RawSyntax::missing(SyntaxKind::GlobalVariable);
   }
   return m_data->replaceChild<GlobalVariableListItemSyntax>(rawVariable, Cursor::Variable);
}

///
/// GlobalVariableDeclarationsStmtSyntax
///
void GlobalVariableDeclarationsStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == GlobalVariableDeclarationsStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, GlobalToken, std::set{TokenKindType::T_GLOBAL});
   syntax_assert_child_kind(raw, Variables, std::set{SyntaxKind::GlobalVariableList});
   syntax_assert_child_token(raw, Semicolon, std::set{TokenKindType::T_SEMICOLON});
#endif
}

TokenSyntax GlobalVariableDeclarationsStmtSyntax::getGlobalToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::GlobalToken).get()};
}

GlobalVariableListSyntax GlobalVariableDeclarationsStmtSyntax::getVariables()
{
   return GlobalVariableListSyntax {m_root, m_data->getChild(Cursor::Variables).get()};
}

TokenSyntax GlobalVariableDeclarationsStmtSyntax::getSemicolon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Semicolon).get()};
}

GlobalVariableDeclarationsStmtSyntax
GlobalVariableDeclarationsStmtSyntax::withGlobalToken(std::optional<TokenSyntax> globalToken)
{
   RefCountPtr<RawSyntax> rawGlobalToken;
   if (globalToken.has_value()) {
      rawGlobalToken = globalToken->getRaw();
   } else {
      rawGlobalToken = make_missing_token(T_GLOBAL);
   }
   return m_data->replaceChild<GlobalVariableDeclarationsStmtSyntax>(rawGlobalToken, Cursor::GlobalToken);
}

GlobalVariableDeclarationsStmtSyntax
GlobalVariableDeclarationsStmtSyntax::withVariables(std::optional<GlobalVariableListSyntax> variables)
{
   RefCountPtr<RawSyntax> rawVariables;
   if (variables.has_value()) {
      rawVariables = variables->getRaw();
   } else {
      rawVariables = RawSyntax::missing(SyntaxKind::GlobalVariableList);
   }
   return m_data->replaceChild<GlobalVariableDeclarationsStmtSyntax>(rawVariables, Cursor::Variables);
}

GlobalVariableDeclarationsStmtSyntax
GlobalVariableDeclarationsStmtSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<GlobalVariableDeclarationsStmtSyntax>(rawSemicolon, Cursor::Semicolon);
}

///
/// StaticVariableDeclareSyntax
///
void StaticVariableDeclareSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == StaticVariableDeclareSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, Variable, std::set{TokenKindType::T_VARIABLE});
   syntax_assert_child_token(raw, EqualToken, std::set{TokenKindType::T_EQUAL});
   syntax_assert_child_kind(raw, ValueExpr, std::set{SyntaxKind::Expr});
   if (raw->getChild(Cursor::EqualToken)) {
      assert(raw->getChild(Cursor::ValueExpr));
   } else if (raw->getChild(Cursor::ValueExpr)) {
      assert(raw->getChild(Cursor::EqualToken));
   }
#endif
}

TokenSyntax StaticVariableDeclareSyntax::getVariable()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Variable).get()};
}

std::optional<TokenSyntax> StaticVariableDeclareSyntax::getEqualToken()
{
   RefCountPtr<SyntaxData> equalTokenData = m_data->getChild(Cursor::EqualToken);
   if (!equalTokenData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, equalTokenData.get()};
}

std::optional<ExprSyntax> StaticVariableDeclareSyntax::getValueExpr()
{
   RefCountPtr<SyntaxData> valueExprData = m_data->getChild(Cursor::ValueExpr);
   if (!valueExprData) {
      return std::nullopt;
   }
   return ExprSyntax {m_root, valueExprData.get()};
}

StaticVariableDeclareSyntax
StaticVariableDeclareSyntax::withVariable(std::optional<TokenSyntax> variable)
{
   RefCountPtr<RawSyntax> rawVariable;
   if (variable.has_value()) {
      rawVariable = variable->getRaw();
   } else {
      // TODO not good
      rawVariable = make_missing_token(T_VARIABLE);
   }
   return m_data->replaceChild<StaticVariableDeclareSyntax>(rawVariable, Cursor::Variable);
}

StaticVariableDeclareSyntax
StaticVariableDeclareSyntax::withEqualToken(std::optional<TokenSyntax> equalToken)
{
   RefCountPtr<RawSyntax> rawEqualToken;
   if (equalToken.has_value()) {
      rawEqualToken = equalToken->getRaw();
   } else {
      rawEqualToken = nullptr;
   }
   return m_data->replaceChild<StaticVariableDeclareSyntax>(rawEqualToken, Cursor::EqualToken);
}

StaticVariableDeclareSyntax
StaticVariableDeclareSyntax::withValueExpr(std::optional<ExprSyntax> valueExpr)
{
   RefCountPtr<RawSyntax> rawValueExpr;
   if (valueExpr.has_value()) {
      rawValueExpr = valueExpr->getRaw();
   } else {
      rawValueExpr = nullptr;
   }
   return m_data->replaceChild<StaticVariableDeclareSyntax>(rawValueExpr, Cursor::ValueExpr);
}

///
/// StaticVariableListItemSyntax
///
void StaticVariableListItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == StaticVariableListItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, Comma, std::set{TokenKindType::T_COMMA});
   syntax_assert_child_kind(raw, Declaration, std::set{SyntaxKind::StaticVariableDeclare});
#endif
}

std::optional<TokenSyntax> StaticVariableListItemSyntax::getComma()
{
   RefCountPtr<SyntaxData> commaData = m_data->getChild(Cursor::Comma);
   if (!commaData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, commaData.get()};
}

StaticVariableDeclareSyntax StaticVariableListItemSyntax::getDeclaration()
{
   return StaticVariableDeclareSyntax {m_root, m_data->getChild(Cursor::Declaration).get()};
}

StaticVariableListItemSyntax
StaticVariableListItemSyntax::withComma(std::optional<TokenSyntax> comma)
{
   RefCountPtr<RawSyntax> commaExpr;
   if (comma.has_value()) {
      commaExpr = comma->getRaw();
   } else {
      commaExpr = nullptr;
   }
   return m_data->replaceChild<StaticVariableListItemSyntax>(commaExpr, Cursor::Comma);
}

StaticVariableListItemSyntax
StaticVariableListItemSyntax::withDeclaration(std::optional<TokenSyntax> declaration)
{
   RefCountPtr<RawSyntax> declarationExpr;
   if (declaration.has_value()) {
      declarationExpr = declaration->getRaw();
   } else {
      declarationExpr = RawSyntax::missing(SyntaxKind::StaticVariableDeclare);
   }
   return m_data->replaceChild<StaticVariableListItemSyntax>(declarationExpr, Cursor::Declaration);
}

///
/// StaticVariableDeclarationsStmtSyntax
///
void StaticVariableDeclarationsStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == StaticVariableDeclarationsStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, StaticToken, std::set{TokenKindType::T_STATIC});
   syntax_assert_child_kind(raw, Variables, std::set{SyntaxKind::StaticVariableList});
   syntax_assert_child_token(raw, Semicolon, std::set{TokenKindType::T_SEMICOLON});
#endif
}

TokenSyntax StaticVariableDeclarationsStmtSyntax::getStaticToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::StaticToken).get()};
}

GlobalVariableListSyntax StaticVariableDeclarationsStmtSyntax::getVariables()
{
   return GlobalVariableListSyntax {m_root, m_data->getChild(Cursor::Variables).get()};
}

TokenSyntax StaticVariableDeclarationsStmtSyntax::getSemicolon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Semicolon).get()};
}

StaticVariableDeclarationsStmtSyntax
StaticVariableDeclarationsStmtSyntax::withStaticToken(std::optional<TokenSyntax> staticToken)
{
   RefCountPtr<RawSyntax> rawStaticToken;
   if (staticToken.has_value()) {
      rawStaticToken = staticToken->getRaw();
   } else {
      rawStaticToken = make_missing_token(T_STATIC);
   }
   return m_data->replaceChild<StaticVariableDeclarationsStmtSyntax>(rawStaticToken, Cursor::StaticToken);
}

StaticVariableDeclarationsStmtSyntax
StaticVariableDeclarationsStmtSyntax::withVariables(std::optional<GlobalVariableListSyntax> variables)
{
   RefCountPtr<RawSyntax> rawVariables;
   if (variables.has_value()) {
      rawVariables = variables->getRaw();
   } else {
      rawVariables = RawSyntax::missing(SyntaxKind::StaticVariableList);
   }
   return m_data->replaceChild<StaticVariableDeclarationsStmtSyntax>(rawVariables, Cursor::Variables);
}

StaticVariableDeclarationsStmtSyntax
StaticVariableDeclarationsStmtSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<StaticVariableDeclarationsStmtSyntax>(rawSemicolon, Cursor::Semicolon);
}

///
/// NamespaceUseTypeSyntax
///
#ifdef POLAR_DEBUG_BUILD
const TokenChoicesType NamespaceUseTypeSyntax::CHILD_TOKEN_CHOICES
{
   {
      NamespaceUseTypeSyntax::TypeToken, {
         TokenKindType::T_FUNCTION, TokenKindType::T_CONST
      }
   }
};
#endif

void NamespaceUseTypeSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == NamespaceUseTypeSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, TypeToken, CHILD_TOKEN_CHOICES.at(Cursor::TypeToken));
#endif
}

TokenSyntax NamespaceUseTypeSyntax::getTypeToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::TypeToken).get()};
}

NamespaceUseTypeSyntax NamespaceUseTypeSyntax::withTypeToken(std::optional<TokenSyntax> typeToken)
{
   RefCountPtr<RawSyntax> typeTokenRaw;
   if (typeToken.has_value()) {
      typeTokenRaw = typeToken->getRaw();
   } else {
      typeTokenRaw = make_missing_token(T_FUNCTION);
   }
   return m_data->replaceChild<NamespaceUseTypeSyntax>(typeTokenRaw, Cursor::TypeToken);
}

///
/// NamespaceUnprefixedUseDeclarationSyntax
///
void NamespaceUnprefixedUseDeclarationSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == NamespaceUnprefixedUseDeclarationSyntax::CHILDREN_COUNT);
   if (const RefCountPtr<RawSyntax> &nsChild = raw->getChild(Cursor::Namespace)) {
      assert(nsChild->kindOf(SyntaxKind::NamespaceName));
   }
   syntax_assert_child_token(raw, AsToken, std::set{TokenKindType::T_AS});
   syntax_assert_child_token(raw, IdentifierToken, std::set{TokenKindType::T_IDENTIFIER_STRING});
#endif
}

NamespaceNameSyntax NamespaceUnprefixedUseDeclarationSyntax::getNamespace()
{
   return NamespaceNameSyntax{m_root, m_data->getChild(Cursor::Namespace).get()};
}

std::optional<TokenSyntax> NamespaceUnprefixedUseDeclarationSyntax::getAsToken()
{
   RefCountPtr<SyntaxData> asTokenData = m_data->getChild(Cursor::AsToken);
   if (!asTokenData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, asTokenData.get()};
}

std::optional<TokenSyntax> NamespaceUnprefixedUseDeclarationSyntax::getIdentifierToken()
{
   RefCountPtr<SyntaxData> identifierData = m_data->getChild(Cursor::IdentifierToken);
   if (!identifierData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, identifierData.get()};
}

NamespaceUnprefixedUseDeclarationSyntax
NamespaceUnprefixedUseDeclarationSyntax::withNamespace(std::optional<NamespaceNameSyntax> ns)
{
   RefCountPtr<RawSyntax> nsRaw;
   if (ns.has_value()) {
      nsRaw = ns->getRaw();
   } else {
      nsRaw = RawSyntax::missing(SyntaxKind::NamespaceName);
   }
   return m_data->replaceChild<NamespaceUnprefixedUseDeclarationSyntax>(nsRaw, Cursor::Namespace);
}

NamespaceUnprefixedUseDeclarationSyntax
NamespaceUnprefixedUseDeclarationSyntax::withAsToken(std::optional<TokenSyntax> asToken)
{
   RefCountPtr<RawSyntax> asTokenRaw;
   if (asToken.has_value()) {
      asTokenRaw = asToken->getRaw();
   } else {
      asTokenRaw = nullptr;
   }
   return m_data->replaceChild<NamespaceUnprefixedUseDeclarationSyntax>(asTokenRaw, Cursor::AsToken);
}

NamespaceUnprefixedUseDeclarationSyntax
NamespaceUnprefixedUseDeclarationSyntax::withIdentifierToken(std::optional<TokenSyntax> identifierToken)
{
   RefCountPtr<RawSyntax> identifierTokenRaw;
   if (identifierToken.has_value()) {
      identifierTokenRaw = identifierToken->getRaw();
   } else {
      identifierTokenRaw = nullptr;
   }
   return m_data->replaceChild<NamespaceUnprefixedUseDeclarationSyntax>(identifierTokenRaw, Cursor::IdentifierToken);
}

///
/// NamespaceUnprefixedUseDeclarationListItemSyntax
///

void NamespaceUnprefixedUseDeclarationListItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == NamespaceUnprefixedUseDeclarationListItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, CommaToken, std::set{TokenKindType::T_COMMA});
   syntax_assert_child_kind(raw, NamespaceUseDeclaration, std::set{SyntaxKind::NamespaceUseDeclaration});
#endif
}

std::optional<TokenSyntax> NamespaceUnprefixedUseDeclarationListItemSyntax::getCommaToken()
{
   RefCountPtr<SyntaxData> commaData = m_data->getChild(Cursor::CommaToken);
   if (!commaData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, commaData.get()};
}

NamespaceUnprefixedUseDeclarationSyntax
NamespaceUnprefixedUseDeclarationListItemSyntax::getNamespaceUseDeclaration()
{
   return NamespaceUnprefixedUseDeclarationSyntax {m_root, m_data->getChild(Cursor::NamespaceUseDeclaration).get()};
}

NamespaceUnprefixedUseDeclarationListItemSyntax
NamespaceUnprefixedUseDeclarationListItemSyntax::withCommaToken(std::optional<TokenSyntax> comma)
{
   RefCountPtr<RawSyntax> commaRaw;
   if (comma.has_value()) {
      commaRaw = comma->getRaw();
   } else {
      commaRaw = nullptr;
   }
   return m_data->replaceChild<NamespaceUnprefixedUseDeclarationListItemSyntax>(commaRaw, Cursor::CommaToken);
}

NamespaceUnprefixedUseDeclarationListItemSyntax
NamespaceUnprefixedUseDeclarationListItemSyntax::withNamespaceUseDeclaration(std::optional<NamespaceUnprefixedUseDeclarationSyntax> declaration)
{
   RefCountPtr<RawSyntax> declarationRaw;
   if (declaration.has_value()) {
      declarationRaw = declaration->getRaw();
   } else {
      declarationRaw = RawSyntax::missing(SyntaxKind::NamespaceUnprefixedUseDeclaration);
   }
   return m_data->replaceChild<NamespaceUnprefixedUseDeclarationListItemSyntax>(declarationRaw, Cursor::NamespaceUseDeclaration);
}

///
/// NamespaceUseDeclarationSyntax
///
void NamespaceUseDeclarationSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == NamespaceUseDeclarationSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, NsSeparator, std::set{TokenKindType::T_NS_SEPARATOR});
   if (const RefCountPtr<RawSyntax> &declarationChild = raw->getChild(Cursor::UnprefixedUseDeclaration)) {
      assert(declarationChild->kindOf(SyntaxKind::NamespaceUnprefixedUseDeclaration));
   }
#endif
}

std::optional<TokenSyntax> NamespaceUseDeclarationSyntax::getNsSeparator()
{
   RefCountPtr<SyntaxData> separatorData = m_data->getChild(Cursor::NsSeparator);
   if (!separatorData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, separatorData.get()};
}

NamespaceUnprefixedUseDeclarationSyntax
NamespaceUseDeclarationSyntax::getUnprefixedUseDeclaration()
{
   return NamespaceUnprefixedUseDeclarationSyntax{m_root, m_data->getChild(Cursor::UnprefixedUseDeclaration).get()};
}

NamespaceUseDeclarationSyntax
NamespaceUseDeclarationSyntax::withNsSeparator(std::optional<TokenSyntax> nsSeparator)
{
   RefCountPtr<RawSyntax> nsSeparatorRaw;
   if (nsSeparator.has_value()) {
      nsSeparatorRaw = nsSeparator->getRaw();
   } else {
      nsSeparatorRaw = nullptr;
   }
   return m_data->replaceChild<NamespaceUseDeclarationSyntax>(nsSeparatorRaw, Cursor::NsSeparator);
}

NamespaceUseDeclarationSyntax
NamespaceUseDeclarationSyntax::withUnprefixedUseDeclaration(std::optional<NamespaceUnprefixedUseDeclarationSyntax> declaration)
{
   RefCountPtr<RawSyntax> declarationRaw;
   if (declaration.has_value()) {
      declarationRaw = declaration->getRaw();
   } else {
      declarationRaw = RawSyntax::missing(SyntaxKind::NamespaceUnprefixedUseDeclaration);
   }
   return m_data->replaceChild<NamespaceUseDeclarationSyntax>(declarationRaw, Cursor::UnprefixedUseDeclaration);
}

///
/// NamespaceUseDeclarationListItemSyntax
///

void NamespaceUseDeclarationListItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == NamespaceUseDeclarationListItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, CommaToken, std::set{TokenKindType::T_COMMA});
   syntax_assert_child_kind(raw, NamespaceUseDeclaration, std::set{SyntaxKind::NamespaceUseDeclaration});
#endif
}

std::optional<TokenSyntax> NamespaceUseDeclarationListItemSyntax::getComma()
{
   RefCountPtr<SyntaxData> commaData = m_data->getChild(Cursor::CommaToken);
   if (!commaData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, commaData.get()};
}

NamespaceUseDeclarationSyntax
NamespaceUseDeclarationListItemSyntax::getNamespaceUseDeclaration()
{
   return NamespaceUseDeclarationSyntax {m_root, m_data->getChild(Cursor::NamespaceUseDeclaration).get()};
}

NamespaceUseDeclarationListItemSyntax
NamespaceUseDeclarationListItemSyntax::withComma(std::optional<TokenSyntax> comma)
{
   RefCountPtr<RawSyntax> commaRaw;
   if (comma.has_value()) {
      commaRaw = comma->getRaw();
   } else {
      commaRaw = nullptr;
   }
   return m_data->replaceChild<NamespaceUseDeclarationListItemSyntax>(commaRaw, Cursor::CommaToken);
}

NamespaceUseDeclarationListItemSyntax
NamespaceUseDeclarationListItemSyntax::withNamespaceUseDeclaration(std::optional<NamespaceUseDeclarationSyntax> declaration)
{
   RefCountPtr<RawSyntax> declarationRaw;
   if (declaration.has_value()) {
      declarationRaw = declaration->getRaw();
   } else {
      declarationRaw = nullptr;
   }
   return m_data->replaceChild<NamespaceUseDeclarationListItemSyntax>(declarationRaw, Cursor::NamespaceUseDeclaration);
}

///
/// NamespaceInlineUseDeclarationSyntax
///
void NamespaceInlineUseDeclarationSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == NamespaceInlineUseDeclarationSyntax::CHILDREN_COUNT);
   if (const RefCountPtr<RawSyntax> &useTypeChild = raw->getChild(Cursor::UseType)) {
      assert(useTypeChild->kindOf(SyntaxKind::NamespaceUseType));
   }
   if (const RefCountPtr<RawSyntax> &declarationChild = raw->getChild(Cursor::UnprefixedUseDeclaration)) {
      assert(declarationChild->kindOf(SyntaxKind::NamespaceUnprefixedUseDeclaration));
   }
#endif
}

std::optional<NamespaceUseTypeSyntax> NamespaceInlineUseDeclarationSyntax::getUseType()
{
   RefCountPtr<SyntaxData> useTypeData = m_data->getChild(Cursor::UseType);
   if (!useTypeData) {
      return std::nullopt;
   }
   return NamespaceUseTypeSyntax{m_root, useTypeData.get()};
}

NamespaceUnprefixedUseDeclarationSyntax
NamespaceInlineUseDeclarationSyntax::getUnprefixedUseDeclaration()
{
   return NamespaceUnprefixedUseDeclarationSyntax{m_root, m_data->getChild(Cursor::UnprefixedUseDeclaration).get()};
}

NamespaceInlineUseDeclarationSyntax
NamespaceInlineUseDeclarationSyntax::withUseType(std::optional<NamespaceUseTypeSyntax> useType)
{
   RefCountPtr<RawSyntax> useTypeRaw;
   if (useType.has_value()) {
      useTypeRaw = useType->getRaw();
   } else {
      useTypeRaw = nullptr;
   }
   return m_data->replaceChild<NamespaceInlineUseDeclarationSyntax>(useTypeRaw, Cursor::UseType);
}

NamespaceInlineUseDeclarationSyntax
NamespaceInlineUseDeclarationSyntax::withUnprefixedUseDeclaration(std::optional<NamespaceUnprefixedUseDeclarationSyntax> declaration)
{
   RefCountPtr<RawSyntax> declarationRaw;
   if (declaration.has_value()) {
      declarationRaw = declaration->getRaw();
   } else {
      declarationRaw = RawSyntax::missing(SyntaxKind::NamespaceUnprefixedUseDeclaration);
   }
   return m_data->replaceChild<NamespaceInlineUseDeclarationSyntax>(declarationRaw, Cursor::UnprefixedUseDeclaration);
}

///
/// NamespaceInlineUseDeclarationListItemSyntax
///

void NamespaceInlineUseDeclarationListItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == NamespaceInlineUseDeclarationListItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, CommaToken, std::set{TokenKindType::T_COMMA});
   syntax_assert_child_kind(raw, NamespaceUseDeclaration, std::set{SyntaxKind::NamespaceUseDeclaration});
#endif
}

std::optional<TokenSyntax> NamespaceInlineUseDeclarationListItemSyntax::getCommaToken()
{
   RefCountPtr<SyntaxData> commaData = m_data->getChild(Cursor::CommaToken);
   if (!commaData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, commaData.get()};
}

NamespaceInlineUseDeclarationSyntax
NamespaceInlineUseDeclarationListItemSyntax::getNamespaceUseDeclaration()
{
   return NamespaceInlineUseDeclarationSyntax {m_root, m_data->getChild(Cursor::NamespaceUseDeclaration).get()};
}

NamespaceInlineUseDeclarationListItemSyntax
NamespaceInlineUseDeclarationListItemSyntax::withCommaToken(std::optional<TokenSyntax> comma)
{
   RefCountPtr<RawSyntax> commaRaw;
   if (comma.has_value()) {
      commaRaw = comma->getRaw();
   } else {
      commaRaw = nullptr;
   }
   return m_data->replaceChild<NamespaceInlineUseDeclarationListItemSyntax>(commaRaw, Cursor::CommaToken);
}

NamespaceInlineUseDeclarationListItemSyntax
NamespaceInlineUseDeclarationListItemSyntax::withNamespaceUseDeclaration(std::optional<NamespaceInlineUseDeclarationSyntax> declaration)
{
   RefCountPtr<RawSyntax> declarationRaw;
   if (declaration.has_value()) {
      declarationRaw = declaration->getRaw();
   } else {
      declarationRaw = RawSyntax::missing(SyntaxKind::NamespaceInlineUseDeclaration);
   }
   return m_data->replaceChild<NamespaceInlineUseDeclarationListItemSyntax>(declarationRaw, Cursor::NamespaceUseDeclaration);
}

///
/// NamespaceGroupUseDeclarationSyntax
///
void NamespaceGroupUseDeclarationSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == NamespaceGroupUseDeclarationSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, FirstNsSeparator, std::set{TokenKindType::T_NS_SEPARATOR});
   syntax_assert_child_token(raw, SecondNsSeparator, std::set{TokenKindType::T_NS_SEPARATOR});
   syntax_assert_child_token(raw, LeftBrace, std::set{TokenKindType::T_LEFT_BRACE});
   syntax_assert_child_token(raw, LeftBrace, std::set{TokenKindType::T_RIGHT_BRACE});
   syntax_assert_child_token(raw, CommaToken, std::set{TokenKindType::T_COMMA});
   if (const RefCountPtr<RawSyntax> &namespaceChild = raw->getChild(Cursor::Namespace)) {
      assert(namespaceChild->kindOf(SyntaxKind::NamespaceName));
   }
   if (const RefCountPtr<RawSyntax> &declarationsChild = raw->getChild(Cursor::UnprefixedUseDeclarations)) {
      assert(declarationsChild->kindOf(SyntaxKind::NamespaceUnprefixedUseDeclarationList));
   }
#endif
}

std::optional<TokenSyntax> NamespaceGroupUseDeclarationSyntax::getFirstNsSeparator()
{
   RefCountPtr<SyntaxData> separatorData = m_data->getChild(Cursor::FirstNsSeparator);
   if (!separatorData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, separatorData.get()};
}

NamespaceNameSyntax NamespaceGroupUseDeclarationSyntax::getNamespace()
{
   return NamespaceNameSyntax{m_root, m_data->getChild(Cursor::Namespace).get()};
}

TokenSyntax NamespaceGroupUseDeclarationSyntax::getSecondNsSeparator()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::SecondNsSeparator).get()};
}

TokenSyntax NamespaceGroupUseDeclarationSyntax::getLeftBrace()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftBrace).get()};
}

NamespaceUnprefixedUseDeclarationListSyntax
NamespaceGroupUseDeclarationSyntax::getUnprefixedUseDeclarations()
{
   return NamespaceUnprefixedUseDeclarationListSyntax {m_root, m_data->getChild(Cursor::UnprefixedUseDeclarations).get()};
}

std::optional<TokenSyntax> NamespaceGroupUseDeclarationSyntax::getCommaToken()
{
   RefCountPtr<SyntaxData> commaData = m_data->getChild(Cursor::CommaToken);
   if (!commaData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, commaData.get()};
}

TokenSyntax NamespaceGroupUseDeclarationSyntax::getRightBrace()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::LeftBrace).get()};
}

NamespaceGroupUseDeclarationSyntax
NamespaceGroupUseDeclarationSyntax::withFirstNsSeparator(std::optional<TokenSyntax> separator)
{
   RefCountPtr<RawSyntax> separatorRaw;
   if (separator.has_value()) {
      separatorRaw = separator->getRaw();
   } else {
      separatorRaw = nullptr;
   }
   return m_data->replaceChild<NamespaceGroupUseDeclarationSyntax>(separatorRaw, Cursor::FirstNsSeparator);
}

NamespaceGroupUseDeclarationSyntax
NamespaceGroupUseDeclarationSyntax::withNamespace(std::optional<NamespaceNameSyntax> ns)
{
   RefCountPtr<RawSyntax> nsRaw;
   if (ns.has_value()) {
      nsRaw = ns->getRaw();
   } else {
      nsRaw = RawSyntax::missing(SyntaxKind::NamespaceName);
   }
   return m_data->replaceChild<NamespaceGroupUseDeclarationSyntax>(nsRaw, Cursor::Namespace);
}

NamespaceGroupUseDeclarationSyntax
NamespaceGroupUseDeclarationSyntax::withSecondNsSeparator(std::optional<TokenSyntax> separator)
{
   RefCountPtr<RawSyntax> separatorRaw;
   if (separator.has_value()) {
      separatorRaw = separator->getRaw();
   } else {
      separatorRaw = make_missing_token(T_NS_SEPARATOR);
   }
   return m_data->replaceChild<NamespaceGroupUseDeclarationSyntax>(separatorRaw, Cursor::SecondNsSeparator);
}

NamespaceGroupUseDeclarationSyntax
NamespaceGroupUseDeclarationSyntax::withLeftBrace(std::optional<TokenSyntax> leftBrace)
{
   RefCountPtr<RawSyntax> leftBraceRaw;
   if (leftBrace.has_value()) {
      leftBraceRaw = leftBrace->getRaw();
   } else {
      leftBraceRaw = make_missing_token(T_LEFT_BRACE);
   }
   return m_data->replaceChild<NamespaceGroupUseDeclarationSyntax>(leftBraceRaw, Cursor::LeftBrace);
}

NamespaceGroupUseDeclarationSyntax
NamespaceGroupUseDeclarationSyntax::withUnprefixedUseDeclarations(std::optional<NamespaceUnprefixedUseDeclarationListSyntax> declarations)
{
   RefCountPtr<RawSyntax> declarationsRaw;
   if (declarations.has_value()) {
      declarationsRaw = declarations->getRaw();
   } else {
      declarationsRaw = RawSyntax::missing(SyntaxKind::NamespaceUnprefixedUseDeclarationList);
   }
   return m_data->replaceChild<NamespaceGroupUseDeclarationSyntax>(declarationsRaw, Cursor::UnprefixedUseDeclarations);
}

NamespaceGroupUseDeclarationSyntax
NamespaceGroupUseDeclarationSyntax::withCommaToken(std::optional<TokenSyntax> comma)
{
   RefCountPtr<RawSyntax> commaRaw;
   if (comma.has_value()) {
      commaRaw = comma->getRaw();
   } else {
      commaRaw = make_missing_token(T_COMMA);
   }
   return m_data->replaceChild<NamespaceGroupUseDeclarationSyntax>(commaRaw, Cursor::CommaToken);
}

NamespaceGroupUseDeclarationSyntax
NamespaceGroupUseDeclarationSyntax::withRightBrace(std::optional<TokenSyntax> rightBrace)
{
   RefCountPtr<RawSyntax> rightBraceRaw;
   if (rightBrace.has_value()) {
      rightBraceRaw = rightBrace->getRaw();
   } else {
      rightBraceRaw = make_missing_token(T_RIGHT_BRACE);
   }
   return m_data->replaceChild<NamespaceGroupUseDeclarationSyntax>(rightBraceRaw, Cursor::RightBrace);
}

NamespaceGroupUseDeclarationSyntax
NamespaceGroupUseDeclarationSyntax::addUnprefixedUseDeclaration(NamespaceUnprefixedUseDeclarationSyntax declaration)
{
   RefCountPtr<RawSyntax> declarationsRaw = getRaw()->getChild(Cursor::UnprefixedUseDeclarations);
   if (declarationsRaw) {
      declarationsRaw = declarationsRaw->append(declaration.getRaw());
   } else {
      declarationsRaw = RawSyntax::make(SyntaxKind::NamespaceUnprefixedUseDeclarationList, {
                                           declaration.getRaw()
                                        }, SourcePresence::Present);
   }
   return m_data->replaceChild<NamespaceGroupUseDeclarationSyntax>(declarationsRaw, Cursor::UnprefixedUseDeclarations);
}

///
/// NamespaceMixedGroupUseDeclarationSyntax
///
void NamespaceMixedGroupUseDeclarationSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == NamespaceMixedGroupUseDeclarationSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, FirstNsSeparator, std::set{TokenKindType::T_NS_SEPARATOR});
   syntax_assert_child_token(raw, SecondNsSeparator, std::set{TokenKindType::T_NS_SEPARATOR});
   syntax_assert_child_token(raw, LeftBrace, std::set{TokenKindType::T_LEFT_BRACE});
   syntax_assert_child_token(raw, LeftBrace, std::set{TokenKindType::T_RIGHT_BRACE});
   syntax_assert_child_token(raw, CommaToken, std::set{TokenKindType::T_COMMA});
   if (const RefCountPtr<RawSyntax> &namespaceChild = raw->getChild(Cursor::Namespace)) {
      assert(namespaceChild->kindOf(SyntaxKind::NamespaceName));
   }
   if (const RefCountPtr<RawSyntax> &declarationsChild = raw->getChild(Cursor::InlineUseDeclarations)) {
      assert(declarationsChild->kindOf(SyntaxKind::NamespaceInlineUseDeclarationList));
   }
#endif
}

std::optional<TokenSyntax> NamespaceMixedGroupUseDeclarationSyntax::getFirstNsSeparator()
{
   RefCountPtr<SyntaxData> separatorData = m_data->getChild(Cursor::FirstNsSeparator);
   if (!separatorData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, separatorData.get()};
}

NamespaceNameSyntax NamespaceMixedGroupUseDeclarationSyntax::getNamespace()
{
   return NamespaceNameSyntax{m_root, m_data->getChild(Cursor::Namespace).get()};
}

TokenSyntax NamespaceMixedGroupUseDeclarationSyntax::getSecondNsSeparator()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::SecondNsSeparator).get()};
}

TokenSyntax NamespaceMixedGroupUseDeclarationSyntax::getLeftBrace()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftBrace).get()};
}

NamespaceInlineUseDeclarationListSyntax
NamespaceMixedGroupUseDeclarationSyntax::getInlineUseDeclarations()
{
   return NamespaceInlineUseDeclarationListSyntax {m_root, m_data->getChild(Cursor::InlineUseDeclarations).get()};
}

std::optional<TokenSyntax> NamespaceMixedGroupUseDeclarationSyntax::getCommaToken()
{
   RefCountPtr<SyntaxData> commaData = m_data->getChild(Cursor::CommaToken);
   if (!commaData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, commaData.get()};
}

TokenSyntax NamespaceMixedGroupUseDeclarationSyntax::getRightBrace()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftBrace).get()};
}

NamespaceMixedGroupUseDeclarationSyntax
NamespaceMixedGroupUseDeclarationSyntax::withFirstNsSeparator(std::optional<TokenSyntax> separator)
{
   RefCountPtr<RawSyntax> separatorRaw;
   if (separator.has_value()) {
      separatorRaw = separator->getRaw();
   } else {
      separatorRaw = nullptr;
   }
   return m_data->replaceChild<NamespaceMixedGroupUseDeclarationSyntax>(separatorRaw, Cursor::FirstNsSeparator);
}

NamespaceMixedGroupUseDeclarationSyntax
NamespaceMixedGroupUseDeclarationSyntax::withNamespace(std::optional<NamespaceNameSyntax> ns)
{
   RefCountPtr<RawSyntax> nsRaw;
   if (ns.has_value()) {
      nsRaw = ns->getRaw();
   } else {
      nsRaw = RawSyntax::missing(SyntaxKind::NamespaceName);
   }
   return m_data->replaceChild<NamespaceMixedGroupUseDeclarationSyntax>(nsRaw, Cursor::Namespace);
}

NamespaceMixedGroupUseDeclarationSyntax
NamespaceMixedGroupUseDeclarationSyntax::withSecondNsSeparator(std::optional<TokenSyntax> separator)
{
   RefCountPtr<RawSyntax> separatorRaw;
   if (separator.has_value()) {
      separatorRaw = separator->getRaw();
   } else {
      separatorRaw = make_missing_token(T_NS_SEPARATOR);
   }
   return m_data->replaceChild<NamespaceMixedGroupUseDeclarationSyntax>(separatorRaw, Cursor::SecondNsSeparator);
}

NamespaceMixedGroupUseDeclarationSyntax
NamespaceMixedGroupUseDeclarationSyntax::withLeftBrace(std::optional<TokenSyntax> leftBrace)
{
   RefCountPtr<RawSyntax> leftBraceRaw;
   if (leftBrace.has_value()) {
      leftBraceRaw = leftBrace->getRaw();
   } else {
      leftBraceRaw = make_missing_token(T_LEFT_BRACE);
   }
   return m_data->replaceChild<NamespaceMixedGroupUseDeclarationSyntax>(leftBraceRaw, Cursor::LeftBrace);
}

NamespaceMixedGroupUseDeclarationSyntax
NamespaceMixedGroupUseDeclarationSyntax::withInlineUseDeclarations(std::optional<NamespaceInlineUseDeclarationListSyntax> declarations)
{
   RefCountPtr<RawSyntax> declarationsRaw;
   if (declarations.has_value()) {
      declarationsRaw = declarations->getRaw();
   } else {
      declarationsRaw = RawSyntax::missing(SyntaxKind::NamespaceInlineUseDeclarationList);
   }
   return m_data->replaceChild<NamespaceMixedGroupUseDeclarationSyntax>(declarationsRaw, Cursor::InlineUseDeclarations);
}

NamespaceMixedGroupUseDeclarationSyntax
NamespaceMixedGroupUseDeclarationSyntax::withCommaToken(std::optional<TokenSyntax> comma)
{
   RefCountPtr<RawSyntax> commaRaw;
   if (comma.has_value()) {
      commaRaw = comma->getRaw();
   } else {
      commaRaw = make_missing_token(T_COMMA);
   }
   return m_data->replaceChild<NamespaceMixedGroupUseDeclarationSyntax>(commaRaw, Cursor::CommaToken);
}

NamespaceMixedGroupUseDeclarationSyntax
NamespaceMixedGroupUseDeclarationSyntax::withRightBrace(std::optional<TokenSyntax> rightBrace)
{
   RefCountPtr<RawSyntax> rightBraceRaw;
   if (rightBrace.has_value()) {
      rightBraceRaw = rightBrace->getRaw();
   } else {
      rightBraceRaw = make_missing_token(T_RIGHT_BRACE);
   }
   return m_data->replaceChild<NamespaceMixedGroupUseDeclarationSyntax>(rightBraceRaw, Cursor::RightBrace);
}

NamespaceMixedGroupUseDeclarationSyntax
NamespaceMixedGroupUseDeclarationSyntax::addInlineUseDeclaration(NamespaceInlineUseDeclarationSyntax declaration)
{
   RefCountPtr<RawSyntax> declarationsRaw = getRaw()->getChild(Cursor::InlineUseDeclarations);
   if (declarationsRaw) {
      declarationsRaw = declarationsRaw->append(declaration.getRaw());
   } else {
      declarationsRaw = RawSyntax::make(SyntaxKind::NamespaceUnprefixedUseDeclarationList, {
                                           declaration.getRaw()
                                        }, SourcePresence::Present);
   }
   return m_data->replaceChild<NamespaceMixedGroupUseDeclarationSyntax>(declarationsRaw, Cursor::InlineUseDeclarations);
}

///
/// NamespaceUseStmtSyntax
///
void NamespaceUseStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == NamespaceUseStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, UseToken, std::set{TokenKindType::T_USE});
   syntax_assert_child_token(raw, SemicolonToken, std::set{TokenKindType::T_SEMICOLON});
   if (const RefCountPtr<RawSyntax> &useTypeChild = raw->getChild(Cursor::UseType)) {
      assert(useTypeChild->kindOf(SyntaxKind::NamespaceUseType));
   }
   if (const RefCountPtr<RawSyntax> &declarationsChild = raw->getChild(Cursor::Declarations)) {
      bool isMixGroupUseDeclarations = declarationsChild->kindOf(SyntaxKind::NamespaceMixedGroupUseDeclaration);
      bool isGroupUseDeclarations = declarationsChild->kindOf(SyntaxKind::NamespaceGroupUseDeclaration);
      bool isUseDeclarations = declarationsChild->kindOf(SyntaxKind::NamespaceUseDeclarationList);
      assert(isMixGroupUseDeclarations || isGroupUseDeclarations || isUseDeclarations);
   }
#endif
}

TokenSyntax NamespaceUseStmtSyntax::getUseToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::UseToken).get()};
}

std::optional<NamespaceUseTypeSyntax>
NamespaceUseStmtSyntax::getUseType()
{
   RefCountPtr<SyntaxData> useTypeData = m_data->getChild(Cursor::UseType);
   if (!useTypeData) {
      return std::nullopt;
   }
   return NamespaceUseTypeSyntax{m_root, useTypeData.get()};
}

Syntax NamespaceUseStmtSyntax::getDeclarations()
{
   return Syntax {m_root, m_data->getChild(Cursor::Declarations).get()};
}

TokenSyntax NamespaceUseStmtSyntax::getSemicolon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::SemicolonToken).get()};
}

NamespaceUseStmtSyntax
NamespaceUseStmtSyntax::withUseToken(std::optional<TokenSyntax> useToken)
{
   RefCountPtr<RawSyntax> useTokenRaw;
   if (useToken.has_value()) {
      useTokenRaw = useToken->getRaw();
   } else {
      useTokenRaw = make_missing_token(T_USE);
   }
   return m_data->replaceChild<NamespaceUseStmtSyntax>(useTokenRaw, Cursor::UseToken);
}

NamespaceUseStmtSyntax
NamespaceUseStmtSyntax::withUseType(std::optional<NamespaceUseTypeSyntax> useType)
{
   RefCountPtr<RawSyntax> useTypeRaw;
   if (useType.has_value()) {
      useTypeRaw = useType->getRaw();
   } else {
      useTypeRaw = nullptr;
   }
   return m_data->replaceChild<NamespaceUseStmtSyntax>(useTypeRaw, Cursor::UseType);
}

NamespaceUseStmtSyntax
NamespaceUseStmtSyntax::withDeclarations(std::optional<Syntax> declarations)
{
   RefCountPtr<RawSyntax> declarationsRaw;
   if (declarations.has_value()) {
      declarationsRaw = declarations->getRaw();
   } else {
      declarationsRaw = RawSyntax::missing(SyntaxKind::NamespaceUseDeclarationList);
   }
   return m_data->replaceChild<NamespaceUseStmtSyntax>(declarationsRaw, Cursor::Declarations);
}

NamespaceUseStmtSyntax
NamespaceUseStmtSyntax::withSemicolonToken(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> tokenRaw;
   if (semicolon.has_value()) {
      tokenRaw = semicolon->getRaw();
   } else {
      tokenRaw = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<NamespaceUseStmtSyntax>(tokenRaw, Cursor::SemicolonToken);
}

///
/// NamespaceDefinitionStmtSyntax
///
void NamespaceDefinitionStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == NamespaceDefinitionStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, NamespaceToken, std::set{TokenKindType::T_NAMESPACE});
   syntax_assert_child_kind(raw, NamespaceName, std::set{SyntaxKind::NamespaceName});
   syntax_assert_child_token(raw, SemicolonToken, std::set{TokenKindType::T_SEMICOLON});
#endif
}

TokenSyntax NamespaceDefinitionStmtSyntax::getNamespaceToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::NamespaceToken).get()};
}

NamespaceNameSyntax NamespaceDefinitionStmtSyntax::getNamespaceName()
{
   return NamespaceNameSyntax{m_root, m_data->getChild(Cursor::NamespaceName).get()};
}

TokenSyntax NamespaceDefinitionStmtSyntax::getSemicolonToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::SemicolonToken).get()};
}

NamespaceDefinitionStmtSyntax
NamespaceDefinitionStmtSyntax::withNamespaceToken(std::optional<TokenSyntax> namespaceToken)
{
   RefCountPtr<RawSyntax> rawNamespaceToken;
   if (namespaceToken.has_value()) {
      rawNamespaceToken = namespaceToken->getRaw();
   } else {
      rawNamespaceToken = make_missing_token(T_NAMESPACE);
   }
   return m_data->replaceChild<NamespaceDefinitionStmtSyntax>(rawNamespaceToken, Cursor::NamespaceToken);
}

NamespaceDefinitionStmtSyntax
NamespaceDefinitionStmtSyntax::withNamespaceName(std::optional<NamespaceNameSyntax> name)
{
   RefCountPtr<RawSyntax> rawName;
   if (name.has_value()) {
      rawName = name->getRaw();
   } else {
      rawName = RawSyntax::missing(SyntaxKind::NamespaceName);
   }
   return m_data->replaceChild<NamespaceDefinitionStmtSyntax>(rawName, Cursor::NamespaceName);
}

NamespaceDefinitionStmtSyntax
NamespaceDefinitionStmtSyntax::withSemicolonToken(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> rawSemicolon;
   if (semicolon.has_value()) {
      rawSemicolon = semicolon->getRaw();
   } else {
      rawSemicolon = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<NamespaceDefinitionStmtSyntax>(rawSemicolon, Cursor::SemicolonToken);
}

///
/// NamespaceBlockStmtSyntax
///
void NamespaceBlockStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == NamespaceBlockStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, NamespaceToken, std::set{TokenKindType::T_NAMESPACE});
   syntax_assert_child_kind(raw, NamespaceName, std::set{SyntaxKind::NamespaceName});
   syntax_assert_child_kind(raw, CodeBlock, std::set{SyntaxKind::TopCodeBlockStmt});
#endif
}

TokenSyntax NamespaceBlockStmtSyntax::getNamespaceToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::NamespaceToken).get()};
}

std::optional<NamespaceNameSyntax> NamespaceBlockStmtSyntax::getNamespaceName()
{
   RefCountPtr<SyntaxData> nsNameData = m_data->getChild(Cursor::NamespaceName);
   if (!nsNameData) {
      return std::nullopt;
   }
   return NamespaceNameSyntax{m_root, nsNameData.get()};
}

TopCodeBlockStmtSyntax
NamespaceBlockStmtSyntax::getCodeBlock()
{
   return TopCodeBlockStmtSyntax{m_root, m_data->getChild(Cursor::CodeBlock).get()};
}

NamespaceBlockStmtSyntax
NamespaceBlockStmtSyntax::withNamespaceToken(std::optional<TokenSyntax> namespaceToken)
{
   RefCountPtr<RawSyntax> rawNamespaceToken;
   if (namespaceToken.has_value()) {
      rawNamespaceToken = namespaceToken->getRaw();
   } else {
      rawNamespaceToken = make_missing_token(T_NAMESPACE);
   }
   return m_data->replaceChild<NamespaceBlockStmtSyntax>(rawNamespaceToken, Cursor::NamespaceToken);
}

NamespaceBlockStmtSyntax
NamespaceBlockStmtSyntax::withNamespaceName(std::optional<NamespaceNameSyntax> name)
{
   RefCountPtr<RawSyntax> rawName;
   if (name.has_value()) {
      rawName = name->getRaw();
   } else {
      rawName = nullptr;
   }
   return m_data->replaceChild<NamespaceBlockStmtSyntax>(rawName, Cursor::NamespaceName);
}

NamespaceBlockStmtSyntax
NamespaceBlockStmtSyntax::withCodeBlock(std::optional<TopCodeBlockStmtSyntax> codeBlock)
{
   RefCountPtr<RawSyntax> rawCodeBlock;
   if (codeBlock.has_value()) {
      rawCodeBlock = codeBlock->getRaw();
   } else {
      rawCodeBlock = RawSyntax::missing(SyntaxKind::TopCodeBlockStmt);
   }
   return m_data->replaceChild<NamespaceBlockStmtSyntax>(rawCodeBlock, Cursor::CodeBlock);
}

///
/// ConstDeclareSyntax
///
void ConstDeclareSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ConstDeclareSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, Name, std::set{TokenKindType::T_IDENTIFIER_STRING});
   if (const RefCountPtr<RawSyntax> &initializerChild = raw->getChild(Cursor::InitializerClause)) {
      initializerChild->kindOf(SyntaxKind::InitializerClause);
   }
#endif
}

TokenSyntax ConstDeclareSyntax::getName()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Name).get()};
}

InitializerClauseSyntax ConstDeclareSyntax::getInitializer()
{
   return InitializerClauseSyntax {m_root, m_data->getChild(Cursor::InitializerClause).get()};
}

ConstDeclareSyntax ConstDeclareSyntax::withName(std::optional<TokenSyntax> name)
{
   RefCountPtr<RawSyntax> nameRaw;
   if (name.has_value()) {
      nameRaw = name->getRaw();
   } else {
      nameRaw = make_missing_token(T_IDENTIFIER_STRING);
   }
   return m_data->replaceChild<ConstDeclareSyntax>(nameRaw, Cursor::Name);
}

ConstDeclareSyntax ConstDeclareSyntax::withIntializer(std::optional<InitializerClauseSyntax> initializer)
{
   RefCountPtr<RawSyntax> initializerRaw;
   if (initializer.has_value()) {
      initializerRaw = initializer->getRaw();
   } else {
      initializerRaw = RawSyntax::missing(SyntaxKind::InitializerClause);
   }
   return m_data->replaceChild<ConstDeclareSyntax>(initializerRaw, Cursor::InitializerClause);
}

///
/// ConstListItemSyntax
///
void ConstListItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ConstListItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, CommaToken, std::set{TokenKindType::T_COMMA});
   syntax_assert_child_kind(raw, Declaration, std::set{SyntaxKind::ConstDeclare});
#endif
}

std::optional<TokenSyntax> ConstListItemSyntax::getComma()
{
   RefCountPtr<SyntaxData> commaData = m_data->getChild(Cursor::CommaToken);
   if (!commaData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, commaData.get()};
}

ConstDeclareSyntax ConstListItemSyntax::getDeclaration()
{
   return ConstDeclareSyntax {m_root, m_data->getChild(Cursor::Declaration).get()};
}

ConstListItemSyntax ConstListItemSyntax::withComma(std::optional<TokenSyntax> comma)
{
   RefCountPtr<RawSyntax> commaRaw;
   if (comma.has_value()) {
      commaRaw = comma->getRaw();
   } else {
      commaRaw = nullptr;
   }
   return m_data->replaceChild<ConstListItemSyntax>(commaRaw, Cursor::CommaToken);
}

ConstListItemSyntax ConstListItemSyntax::withDeclaration(std::optional<ConstDeclareSyntax> declaration)
{
   RefCountPtr<RawSyntax> declarationRaw;
   if (declaration.has_value()) {
      declarationRaw = declaration->getRaw();
   } else {
      declarationRaw = RawSyntax::missing(SyntaxKind::ConstDeclare);
   }
   return m_data->replaceChild<ConstListItemSyntax>(declarationRaw, Cursor::Declaration);
}

///
/// ConstDefinitionStmtSyntax
///
void ConstDefinitionStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ConstDefinitionStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ConstToken, std::set{TokenKindType::T_CONST});
   syntax_assert_child_token(raw, Semicolon, std::set{TokenKindType::T_SEMICOLON});
   if (const RefCountPtr<RawSyntax> &declarations = raw->getChild(Cursor::Declarations)) {
      assert(declarations->kindOf(SyntaxKind::ConstDeclareList));
   }
#endif
}

TokenSyntax ConstDefinitionStmtSyntax::getConstToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::ConstToken).get()};
}

ConstDeclareListSyntax ConstDefinitionStmtSyntax::getDeclarations()
{
   return ConstDeclareListSyntax {m_root, m_data->getChild(Cursor::Declarations).get()};
}

TokenSyntax ConstDefinitionStmtSyntax::getSemicolon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Semicolon).get()};
}

ConstDefinitionStmtSyntax ConstDefinitionStmtSyntax::withConstToken(std::optional<TokenSyntax> constToken)
{
   RefCountPtr<RawSyntax> constTokenRaw;
   if (constToken.has_value()) {
      constTokenRaw = constToken->getRaw();
   } else {
      constTokenRaw = make_missing_token(T_CONST);
   }
   return m_data->replaceChild<ConstDefinitionStmtSyntax>(constTokenRaw, Cursor::ConstToken);
}

ConstDefinitionStmtSyntax ConstDefinitionStmtSyntax::withDeclarations(std::optional<ConstDeclareListSyntax> declarations)
{
   RefCountPtr<RawSyntax> declarationsRaw;
   if (declarations.has_value()) {
      declarationsRaw = declarations->getRaw();
   } else {
      declarationsRaw = RawSyntax::missing(SyntaxKind::ConstDeclareList);
   }
   return m_data->replaceChild<ConstDefinitionStmtSyntax>(declarationsRaw, Cursor::Declarations);
}

ConstDefinitionStmtSyntax ConstDefinitionStmtSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> semicolonRaw;
   if (semicolon.has_value()) {
      semicolonRaw = semicolon->getRaw();
   } else {
      semicolonRaw = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<ConstDefinitionStmtSyntax>(semicolonRaw, Cursor::Semicolon);
}

///
/// ClassDefinitionStmtSyntax
///
void ClassDefinitionStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ClassDefinitionStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, ClassDefinition, std::set{SyntaxKind::ClassDefinition});
#endif
}

ClassDefinitionSyntax ClassDefinitionStmtSyntax::getClassDefinition()
{
   return ClassDefinitionSyntax {m_root, m_data->getChild(Cursor::ClassDefinition).get()};
}

ClassDefinitionStmtSyntax
ClassDefinitionStmtSyntax::withClassDefinition(std::optional<ClassDefinitionSyntax> classDefinition)
{
   RefCountPtr<RawSyntax> rawClassDefinition;
   if (classDefinition.has_value()) {
      rawClassDefinition = classDefinition->getRaw();
   } else {
      rawClassDefinition = RawSyntax::missing(SyntaxKind::ClassDefinition);
   }
   return m_data->replaceChild<ClassDefinitionStmtSyntax>(rawClassDefinition, Cursor::ClassDefinition);
}

///
/// InterfaceDefinitionStmtSyntax
///
void InterfaceDefinitionStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == InterfaceDefinitionStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, InterfaceDefinition, std::set{SyntaxKind::InterfaceDefinition});
#endif
}

InterfaceDefinitionSyntax InterfaceDefinitionStmtSyntax::getInterfaceDefinition()
{
   return InterfaceDefinitionSyntax {m_root, m_data->getChild(Cursor::InterfaceDefinition).get()};
}

InterfaceDefinitionStmtSyntax
InterfaceDefinitionStmtSyntax::withInterfaceDefinition(std::optional<InterfaceDefinitionSyntax> interfaceDefinition)
{
   RefCountPtr<RawSyntax> rawInterfaceDefinition;
   if (interfaceDefinition.has_value()) {
      rawInterfaceDefinition = interfaceDefinition->getRaw();
   } else {
      rawInterfaceDefinition = RawSyntax::missing(SyntaxKind::InterfaceDefinition);
   }
   return m_data->replaceChild<InterfaceDefinitionStmtSyntax>(rawInterfaceDefinition, Cursor::InterfaceDefinition);
}

///
/// TraitDefinitionStmtSyntax
///
void TraitDefinitionStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == TraitDefinitionStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, TraitDefinition, std::set{SyntaxKind::TraitDefinition});
#endif
}

TraitDefinitionSyntax TraitDefinitionStmtSyntax::getTraitDefinition()
{
   return TraitDefinitionSyntax {m_root, m_data->getChild(Cursor::TraitDefinition).get()};
}

TraitDefinitionStmtSyntax
TraitDefinitionStmtSyntax::withTraitDefinition(std::optional<TraitDefinitionSyntax> traitDefinition)
{
   RefCountPtr<RawSyntax> rawTraitDefinition;
   if (traitDefinition.has_value()) {
      rawTraitDefinition = traitDefinition->getRaw();
   } else {
      rawTraitDefinition = RawSyntax::missing(SyntaxKind::TraitDefinition);
   }
   return m_data->replaceChild<TraitDefinitionStmtSyntax>(rawTraitDefinition, Cursor::TraitDefinition);
}

///
/// FunctionDefinitionStmtSyntax
///
void FunctionDefinitionStmtSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == FunctionDefinitionStmtSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, FunctionDefinition, std::set{SyntaxKind::FunctionDefinition});
#endif
}

FunctionDefinitionSyntax FunctionDefinitionStmtSyntax::getFunctionDefinition()
{
   return FunctionDefinitionSyntax {m_root, m_data->getChild(Cursor::FunctionDefinition).get()};
}

FunctionDefinitionStmtSyntax
FunctionDefinitionStmtSyntax::withFunctionDefinition(std::optional<FunctionDefinitionSyntax> functionDefinition)
{
   RefCountPtr<RawSyntax> rawFunctionDefinition;
   if (functionDefinition.has_value()) {
      rawFunctionDefinition = functionDefinition->getRaw();
   } else {
      rawFunctionDefinition = RawSyntax::missing(SyntaxKind::FunctionDefinition);
   }
   return m_data->replaceChild<FunctionDefinitionStmtSyntax>(rawFunctionDefinition, Cursor::FunctionDefinition);
}

} // polar::syntax
