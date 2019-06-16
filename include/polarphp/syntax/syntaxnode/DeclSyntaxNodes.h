// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/19.

#ifndef POLARPHP_SYNTAX_SYNTAX_NODE_DECL_NODES_H
#define POLARPHP_SYNTAX_SYNTAX_NODE_DECL_NODES_H

#include "polarphp/syntax/Syntax.h"
#include "polarphp/syntax/SyntaxCollection.h"
#include "polarphp/syntax/TokenSyntax.h"
#include "polarphp/syntax/UnknownSyntax.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"

namespace polar::syntax {

class SourceFileSyntax;

class SourceFileSyntax : public Syntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 2;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: CodeBlockItemListSyntax
      /// optional: false
      Statements,
      /// type: TokenSyntax
      /// optional: false
      EOFToken
   };

public:
   SourceFileSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getEofToken();
   CodeBlockItemListSyntax getStatements();
   SourceFileSyntax withStatements(std::optional<CodeBlockItemListSyntax> statements);
   SourceFileSyntax addStatement(CodeBlockItemSyntax statement);
   SourceFileSyntax withEofToken(std::optional<TokenSyntax> eofToken);

private:
   friend class SourceFileSyntaxBuilder;
   void validate();
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_DECL_NODES_H
