// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/14.

#ifndef POLARPHP_SYNTAX_BUILDER_STMT_SYNTAX_NODE_BUILDERS_H
#define POLARPHP_SYNTAX_BUILDER_STMT_SYNTAX_NODE_BUILDERS_H

#include "polarphp/syntax/RawSyntax.h"
#include "polarphp/syntax/syntaxnode/StmtSyntaxNodes.h"

namespace polar::syntax {

class SyntaxArena;

class ConditionElementSyntaxBuilder
{
public:
   using Cursor = ConditionElementSyntax::Cursor;
public:
   ConditionElementSyntaxBuilder() = default;
   ConditionElementSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   ConditionElementSyntaxBuilder &useCondition(Syntax condition);
   ConditionElementSyntaxBuilder &useTrailingComma(TokenSyntax trailingComma);
   ConditionElementSyntax build();
private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[ConditionElementSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr, nullptr
   };
};

class ContinueStmtSyntaxBuilder
{
public:
   using Cursor = ContinueStmtSyntax::Cursor;
public:
   ContinueStmtSyntaxBuilder() = default;
   ContinueStmtSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   ContinueStmtSyntaxBuilder &useContinueKeyword(TokenSyntax continueKeyword);
   ContinueStmtSyntaxBuilder &useExpr(ExprSyntax expr);
   ContinueStmtSyntaxBuilder &useSemicolon(TokenSyntax semicolon);

   ContinueStmtSyntax build();
private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[ContinueStmtSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr, nullptr
   };
};

class BreakStmtSyntaxBuilder
{
public:
   using Cursor = BreakStmtSyntax::Cursor;

public:
   BreakStmtSyntaxBuilder() = default;
   BreakStmtSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   BreakStmtSyntaxBuilder &useBreakKeyword(TokenSyntax breakKeyword);
   BreakStmtSyntaxBuilder &useExpr(ExprSyntax expr);
   BreakStmtSyntaxBuilder &useSemicolon(TokenSyntax semicolon);

   BreakStmtSyntax build();

private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[BreakStmtSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr
   };
};

class FallthroughStmtSyntaxBuilder
{
public:
   using Cursor =  FallthroughStmtSyntax::Cursor;

public:
   FallthroughStmtSyntaxBuilder() = default;
   FallthroughStmtSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   FallthroughStmtSyntaxBuilder &useFallthroughKeyword(TokenSyntax fallthroughKeyword);
   FallthroughStmtSyntax build();

private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[FallthroughStmtSyntax::CHILDREN_COUNT] = {
      nullptr
   };
};

class ElseIfClauseSyntaxBuilder
{
public:
   using Cursor =  ElseIfClauseSyntax::Cursor;

public:
   ElseIfClauseSyntaxBuilder() = default;
   ElseIfClauseSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   ElseIfClauseSyntaxBuilder &useElseIfKeyword(TokenSyntax elseIfKeyword);
   ElseIfClauseSyntaxBuilder &useLeftParen(TokenSyntax leftParen);
   ElseIfClauseSyntaxBuilder &useCondition(ExprSyntax condition);
   ElseIfClauseSyntaxBuilder &useRightParen(TokenSyntax rightParen);
   ElseIfClauseSyntaxBuilder &useBody(StmtSyntax body);

   ElseIfClauseSyntax build();

private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[ElseIfClauseSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr, nullptr, nullptr, nullptr,
   };
};

class IfStmtSyntaxBuilder
{
public:
   using Cursor =  IfStmtSyntax::Cursor;
public:
   IfStmtSyntaxBuilder() = default;
   IfStmtSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   IfStmtSyntaxBuilder &useLabelName(TokenSyntax labelName);
   IfStmtSyntaxBuilder &useLabelColon(TokenSyntax labelColon);
   IfStmtSyntaxBuilder &useIfKeyword(TokenSyntax ifKeyword);
   IfStmtSyntaxBuilder &useLeftParen(TokenSyntax leftParen);
   IfStmtSyntaxBuilder &useCondition(ExprSyntax condition);
   IfStmtSyntaxBuilder &useRightParen(TokenSyntax rightParen);
   IfStmtSyntaxBuilder &useBody(StmtSyntax body);
   IfStmtSyntaxBuilder &useElseIfClauses(ElseIfListSyntax elseIfClauses);
   IfStmtSyntaxBuilder &useElseKeyword(TokenSyntax elseKeyword);
   IfStmtSyntaxBuilder &useElseBody(Syntax elseBody);

   IfStmtSyntaxBuilder &addElseIfClause(ElseIfClauseSyntax elseIfClause);

   IfStmtSyntax build();
private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[IfStmtSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr, nullptr, nullptr,
   };
};

class WhileStmtSyntaxBuilder
{
public:
   using Cursor =  WhileStmtSyntax::Cursor;
public:
   WhileStmtSyntaxBuilder() = default;
   WhileStmtSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   WhileStmtSyntaxBuilder &useLabelName(TokenSyntax labelName);
   WhileStmtSyntaxBuilder &useLabelColon(TokenSyntax labelColon);
   WhileStmtSyntaxBuilder &useWhileKeyword(TokenSyntax whileKeyword);
   WhileStmtSyntaxBuilder &useLeftParen(TokenSyntax leftParen);
   WhileStmtSyntaxBuilder &useConditions(ConditionElementListSyntax conditions);
   WhileStmtSyntaxBuilder &useRightParen(TokenSyntax rightParen);
   WhileStmtSyntaxBuilder &useBody(StmtSyntax body);
   WhileStmtSyntaxBuilder &addCondition(ConditionElementSyntax condition);

   WhileStmtSyntax build();

private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[WhileStmtSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
   };
};

class DoWhileStmtSyntaxBuilder
{
public:
   using Cursor =  DoWhileStmtSyntax::Cursor;
public:
   DoWhileStmtSyntaxBuilder() = default;
   DoWhileStmtSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   DoWhileStmtSyntaxBuilder &useLabelName(TokenSyntax labelName);
   DoWhileStmtSyntaxBuilder &useLabelColon(TokenSyntax labelColon);
   DoWhileStmtSyntaxBuilder &useDoKeyword(TokenSyntax doKeyword);
   DoWhileStmtSyntaxBuilder &useBody(StmtSyntax body);
   DoWhileStmtSyntaxBuilder &useWhileKeyword(TokenSyntax whileKeyword);
   DoWhileStmtSyntaxBuilder &useLeftParen(TokenSyntax leftParen);
   DoWhileStmtSyntaxBuilder &useCondition(ExprSyntax condition);
   DoWhileStmtSyntaxBuilder &useRightParen(TokenSyntax rightParen);

   DoWhileStmtSyntax build();

private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[DoWhileStmtSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr, nullptr,
   };
};

class SwitchDefaultLabelSyntaxBuilder
{
public:
   using Cursor =  SwitchDefaultLabelSyntax::Cursor;

public:
   SwitchDefaultLabelSyntaxBuilder() = default;
   SwitchDefaultLabelSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   SwitchDefaultLabelSyntaxBuilder &useDefaultKeyword(TokenSyntax defaultKeyword);
   SwitchDefaultLabelSyntaxBuilder &useColon(TokenSyntax colon);
   SwitchDefaultLabelSyntax build();
private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[SwitchDefaultLabelSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr,
   };
};

class SwitchCaseLabelSyntaxBuilder
{
public:
   using Cursor =  SwitchCaseLabelSyntax::Cursor;

public:
   SwitchCaseLabelSyntaxBuilder() = default;
   SwitchCaseLabelSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   SwitchCaseLabelSyntaxBuilder &useCaseKeyword(TokenSyntax caseKeyword);
   SwitchCaseLabelSyntaxBuilder &useExpr(ExprSyntax expr);
   SwitchCaseLabelSyntaxBuilder &useColon(TokenSyntax colon);
   SwitchCaseLabelSyntax build();

private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[SwitchCaseLabelSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr, nullptr
   };
};

class SwitchCaseSyntaxBuilder
{
public:
   using Cursor =  SwitchCaseSyntax::Cursor;

public:
   SwitchCaseSyntaxBuilder() = default;
   SwitchCaseSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   SwitchCaseSyntaxBuilder &useLabel(Syntax label);
   SwitchCaseSyntaxBuilder &useStatements(InnerStmtListSyntax statements);
   SwitchCaseSyntax build();

private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[SwitchCaseSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr
   };
};

class SwitchStmtSyntaxBuilder
{
public:
   using Cursor =  SwitchStmtSyntax::Cursor;

public:
   SwitchStmtSyntaxBuilder() = default;
   SwitchStmtSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   SwitchStmtSyntaxBuilder &useLabelName(TokenSyntax labelName);
   SwitchStmtSyntaxBuilder &useLabelColon(TokenSyntax colon);
   SwitchStmtSyntaxBuilder &useSwitchKeyword(TokenSyntax switchKeyword);
   SwitchStmtSyntaxBuilder &useLeftParen(TokenSyntax leftParen);
   SwitchStmtSyntaxBuilder &useConditionExpr(ExprSyntax condition);
   SwitchStmtSyntaxBuilder &useRightParen(TokenSyntax rightParen);
   SwitchStmtSyntaxBuilder &useLeftBrace(TokenSyntax leftBrace);
   SwitchStmtSyntaxBuilder &useCases(SwitchCaseListSyntax cases);
   SwitchStmtSyntaxBuilder &useRightBrace(TokenSyntax rightBrace);

   SwitchStmtSyntaxBuilder &addCase(SwitchCaseSyntax caseItem);
   SwitchStmtSyntax build();

private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[SwitchStmtSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr,
   };
};

class DeferStmtSyntaxBuilder
{
public:
   using Cursor = DeferStmtSyntax::Cursor;

public:
   DeferStmtSyntaxBuilder() = default;
   DeferStmtSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   DeferStmtSyntaxBuilder &useDeferKeyword(TokenSyntax deferKeyword);
   DeferStmtSyntaxBuilder &useBody(InnerCodeBlockStmtSyntax body);
   DeferStmtSyntax build();

private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[DeferStmtSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr,
   };
};

class ThrowStmtSyntaxBuilder
{
public:
   using Cursor = ThrowStmtSyntax::Cursor;
public:
   ThrowStmtSyntaxBuilder() = default;
   ThrowStmtSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   ThrowStmtSyntaxBuilder &useThrowKeyword(TokenSyntax throwKeyword);
   ThrowStmtSyntaxBuilder &useExpr(ExprSyntax expr);
   ThrowStmtSyntax build();

private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[ThrowStmtSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr,
   };
};

class ReturnStmtSyntaxBuilder
{
public:
   using Cursor = ReturnStmtSyntax::Cursor;

public:
   ReturnStmtSyntaxBuilder() = default;
   ReturnStmtSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   ReturnStmtSyntaxBuilder &useReturnKeyword(TokenSyntax returnKeyword);
   ReturnStmtSyntaxBuilder &useExpr(ExprSyntax expr);
   ReturnStmtSyntax build();

private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[ReturnStmtSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr,
   };
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_BUILDER_STMT_SYNTAX_NODE_BUILDERS_H
