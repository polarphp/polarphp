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

#include "polarphp/parser/parsedsyntaxnode/ParsedCommonSyntaxNodes.h"
#include "polarphp/parser/parsedsyntaxnode/ParsedDeclSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/DeclSyntaxNodes.h"

namespace polar::parser {

using namespace polar::syntax;

ParsedCodeBlockItemListSyntax ParsedSourceFileSyntax::getDeferredStatements()
{
   return ParsedCodeBlockItemListSyntax{getRaw().getDeferredChildren()[SourceFileSyntax::Cursor::Statements]};
}

ParsedTokenSyntax ParsedSourceFileSyntax::getDeferredEofToken()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[SourceFileSyntax::Cursor::EOFToken]};
}

} // polar::parser
