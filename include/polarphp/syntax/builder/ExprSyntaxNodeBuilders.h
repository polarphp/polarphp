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

} // polar::syntax


#endif // POLARPHP_SYNTAX_BUILDER_EXPR_SYNTAX_NODE_BUILDERS_H
