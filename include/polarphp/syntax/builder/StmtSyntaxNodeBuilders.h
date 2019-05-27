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
#include "polarphp/syntax/SyntaxNodes.h"

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
      nullptr, nullptr
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
   ContinueStmtSyntaxBuilder &useLNumberToken(TokenSyntax numberToken);

   ContinueStmtSyntax build();
private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[ContinueStmtSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr
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
   BreakStmtSyntaxBuilder &useLNumberToken(TokenSyntax numberToken);

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
   ElseIfClauseSyntaxBuilder &useBody(CodeBlockSyntax body);

   ElseIfClauseSyntax build();

private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[ElseIfClauseSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr, nullptr, nullptr, nullptr
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
   IfStmtSyntaxBuilder &useBody(CodeBlockSyntax body);
   IfStmtSyntaxBuilder &useElseIfClauses(ElseIfListSyntax elseIfClauses);
   IfStmtSyntaxBuilder &useElseKeyword(TokenSyntax elseKeyword);
   IfStmtSyntaxBuilder &useElseBody(Syntax elseBody);

   IfStmtSyntaxBuilder &addElseIfClause(ElseIfClauseSyntax elseIfClause);

   IfStmtSyntax build();
private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[IfStmtSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr, nullptr, nullptr, nullptr,
      nullptr, nullptr, nullptr, nullptr, nullptr
   };
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_BUILDER_STMT_SYNTAX_NODE_BUILDERS_H
