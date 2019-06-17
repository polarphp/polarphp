// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/18.

#ifndef POLARPHP_SYNTAX_BUILDER_DECL_SYNTAX_NODE_BUILDERS_H
#define POLARPHP_SYNTAX_BUILDER_DECL_SYNTAX_NODE_BUILDERS_H

#include "polarphp/syntax/RawSyntax.h"
#include "polarphp/syntax/syntaxnode/DeclSyntaxNodes.h"

namespace polar::syntax {

class SourceFileSyntaxBuilder
{
public:
   using Cursor = SourceFileSyntax::Cursor;
public:
   SourceFileSyntaxBuilder() = default;
   SourceFileSyntaxBuilder(const RefCountPtr<SyntaxArena> &arena)
      : m_arena(arena)
   {}

   SourceFileSyntaxBuilder &useStatements(CodeBlockItemListSyntax statements);
   SourceFileSyntaxBuilder &addStatement(CodeBlockItemSyntax statement);
   SourceFileSyntaxBuilder &useEofToken(TokenSyntax eofToken);
   SourceFileSyntax build();
private:
   RefCountPtr<SyntaxArena> m_arena = nullptr;
   RefCountPtr<RawSyntax> m_layout[SourceFileSyntax::CHILDREN_COUNT] = {
      nullptr, nullptr
   };
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_BUILDER_DECL_SYNTAX_NODE_BUILDERS_H
