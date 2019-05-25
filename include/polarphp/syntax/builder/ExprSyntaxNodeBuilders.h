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

#ifndef POLARPHP_SYNTAX_BUILDER_EXPR_SYNTAX_NODE_BUILDERS_H
#define POLARPHP_SYNTAX_BUILDER_EXPR_SYNTAX_NODE_BUILDERS_H

#include "polarphp/syntax/RawSyntax.h"
#include "polarphp/syntax/syntaxnode/ExprSyntaxNodes.h"

namespace polar::syntax {

class SyntaxArena;

class NullExprSyntaxBuilder
{
public:
   using Cursor = NullExprSyntax::Cursor;
public:
   NullExprSyntaxBuilder() = default;
   NullExprSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   NullExprSyntaxBuilder &useNullKeyword(TokenSyntax nullKeyword);
   NullExprSyntax build();

private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[NullExprSyntax::CHILDREN_COUNT] = {
      nullptr
   };
};

class ClassRefParentExprSyntaxBuilder
{
public:
   using Cursor = ClassRefParentExprSyntax::Cursor;
public:
   ClassRefParentExprSyntaxBuilder() = default;
   ClassRefParentExprSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   ClassRefParentExprSyntaxBuilder &useParentKeyword(TokenSyntax parentKeyword);
   ClassRefParentExprSyntax build();
private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[ClassRefParentExprSyntax::CHILDREN_COUNT] = {
      nullptr
   };
};

class ClassRefSelfExprSyntaxBuilder
{
public:
   using Cursor = ClassRefSelfExprSyntax::Cursor;
public:
   ClassRefSelfExprSyntaxBuilder() = default;
   ClassRefSelfExprSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   ClassRefSelfExprSyntaxBuilder &useSelfKeyword(TokenSyntax selfKeyword);
   ClassRefSelfExprSyntax build();
private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[ClassRefSelfExprSyntax::CHILDREN_COUNT] = {
      nullptr
   };
};

class ClassRefStaticExprSyntaxBuilder
{
public:
   using Cursor = ClassRefStaticExprSyntax::Cursor;
public:
   ClassRefStaticExprSyntaxBuilder() = default;
   ClassRefStaticExprSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   ClassRefStaticExprSyntaxBuilder &useStaticKeyword(TokenSyntax staticKeyword);
   ClassRefStaticExprSyntax build();
private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[ClassRefStaticExprSyntax::CHILDREN_COUNT] = {
      nullptr
   };
};

class IntegerLiteralExprSyntaxBuilder
{
public:
   using Cursor = IntegerLiteralExprSyntax::Cursor;
public:
   IntegerLiteralExprSyntaxBuilder() = default;
   IntegerLiteralExprSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   IntegerLiteralExprSyntaxBuilder &useDigits(TokenSyntax digits);
   IntegerLiteralExprSyntax build();
private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[IntegerLiteralExprSyntax::CHILDREN_COUNT] = {
      nullptr
   };
};

class FloatLiteralExprSyntaxBuilder
{
public:
   using Cursor = FloatLiteralExprSyntax::Cursor;

public:
   FloatLiteralExprSyntaxBuilder() = default;
   FloatLiteralExprSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   FloatLiteralExprSyntaxBuilder &useFloatDigits(TokenSyntax digits);
   FloatLiteralExprSyntax build();

private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[FloatLiteralExprSyntax::CHILDREN_COUNT] = {
      nullptr
   };
};

class StringLiteralExprSyntaxBuilder
{
public:
   using Cursor = StringLiteralExprSyntax::Cursor;

public:
   StringLiteralExprSyntaxBuilder() = default;
   StringLiteralExprSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   StringLiteralExprSyntaxBuilder &useString(TokenSyntax str);
   StringLiteralExprSyntax build();

private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[StringLiteralExprSyntax::CHILDREN_COUNT] = {
      nullptr
   };
};

class BooleanLiteralExprSyntaxBuilder
{
public:
   using Cursor = BooleanLiteralExprSyntax::Cursor;

public:
   BooleanLiteralExprSyntaxBuilder() = default;
   BooleanLiteralExprSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   BooleanLiteralExprSyntaxBuilder &useBoolean(TokenSyntax booleanValue);
   BooleanLiteralExprSyntax build();

private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[BooleanLiteralExprSyntax::CHILDREN_COUNT] = {
      nullptr
   };
};

class TernaryExprSyntaxBuilder
{
public:
   using Cursor = TernaryExprSyntax::Cursor;

public:
   TernaryExprSyntaxBuilder() = default;
   TernaryExprSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   TernaryExprSyntaxBuilder &useConditionExpr(ExprSyntax conditionExpr);
   TernaryExprSyntaxBuilder &useQuestionMark(TokenSyntax questionMark);
   TernaryExprSyntaxBuilder &useFirstChoice(ExprSyntax firstChoice);
   TernaryExprSyntaxBuilder &useColonMark(TokenSyntax colonMark);
   TernaryExprSyntaxBuilder &useSecondChoice(ExprSyntax secondChoice);

   TernaryExprSyntax build();
private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[TernaryExprSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr, nullptr, nullptr, nullptr
   };
};

} // polar::syntax


#endif // POLARPHP_SYNTAX_BUILDER_EXPR_SYNTAX_NODE_BUILDERS_H
