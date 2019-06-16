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
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"

namespace polar::parser {
using namespace polar::syntax;

///
/// ParsedCodeBlockItemSyntax
///
ParsedSyntax ParsedCodeBlockItemSyntax::getDeferredItem()
{
   return ParsedSyntax{getRaw().getDeferredChildren()[CodeBlockItemSyntax::Cursor::Item]};
}

ParsedTokenSyntax ParsedCodeBlockItemSyntax::getDeferredSemicolon()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[CodeBlockItemSyntax::Cursor::Semicolon]};
}

std::optional<ParsedSyntax> ParsedCodeBlockItemSyntax::getgetDeferredErrorTokens()
{
   ParsedRawSyntaxNode rawChild = getRaw().getDeferredChildren()[CodeBlockItemSyntax::Cursor::ErrorTokens];
   if (rawChild.isNull()) {
      return std::nullopt;
   }
   return ParsedSyntax{rawChild};
}

///
/// ParsedCodeBlockSyntax
///
ParsedTokenSyntax ParsedCodeBlockSyntax::getDeferredLeftBrace()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[CodeBlockSyntax::Cursor::LeftBrace]};
}

ParsedCodeBlockItemListSyntax ParsedCodeBlockSyntax::getDeferredStatements()
{
   return ParsedCodeBlockItemListSyntax{getRaw().getDeferredChildren()[CodeBlockSyntax::Cursor::Statements]};
}

ParsedTokenSyntax ParsedCodeBlockSyntax::getDeferredRightBrace()
{
   return ParsedTokenSyntax{getRaw().getDeferredChildren()[CodeBlockSyntax::Cursor::RightBrace]};
}

} // polar::parser
