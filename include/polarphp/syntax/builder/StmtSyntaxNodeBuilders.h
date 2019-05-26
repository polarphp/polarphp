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

} // polar::syntax

#endif // POLARPHP_SYNTAX_BUILDER_STMT_SYNTAX_NODE_BUILDERS_H
