// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/11.

#include "polarphp/syntax/SyntaxKind.h"
#include "polarphp/syntax/SyntaxNodes.h"


#define SYNTAX_TABLE_ENTRY(kind) {#kind , polar::as_integer<SyntaxKind>(SyntaxKind::kind), \
   kind##Syntax::CHILDREN_COUNT, kind##Syntax::REQUIRED_CHILDREN_COUNT}

namespace polar::syntax {

using SyntaxKindEntryType = std::tuple<std::string, std::uint32_t, std::uint32_t, std::uint32_t>;

/// name - serialization_code - children_count - required_children_count
static const std::map<SyntaxKind, SyntaxKindEntryType> scg_syntaxKindTable
{
   {SyntaxKind::Decl, SYNTAX_TABLE_ENTRY(Decl)},
   {SyntaxKind::Expr, SYNTAX_TABLE_ENTRY(Expr)},
   {SyntaxKind::Stmt, SYNTAX_TABLE_ENTRY(Stmt)},
   {SyntaxKind::Type, SYNTAX_TABLE_ENTRY(Type)},
   {SyntaxKind::Token, SYNTAX_TABLE_ENTRY(Token)},
   {SyntaxKind::Unknown, SYNTAX_TABLE_ENTRY(Unknown)},
   {SyntaxKind::CodeBlockItem, SYNTAX_TABLE_ENTRY(CodeBlockItem)},
   {SyntaxKind::CodeBlock, SYNTAX_TABLE_ENTRY(CodeBlock)},
   {SyntaxKind::TokenList, SYNTAX_TABLE_ENTRY(TokenList)},
   {SyntaxKind::NonEmptyTokenList, SYNTAX_TABLE_ENTRY(NonEmptyTokenList)},
   {SyntaxKind::CodeBlockItemList, SYNTAX_TABLE_ENTRY(CodeBlockItemList)}
};

StringRef retrieve_syntax_kind_text(SyntaxKind kind)
{
   auto iter = scg_syntaxKindTable.find(kind);
   if (iter == scg_syntaxKindTable.end()) {
      return StringRef();
   }
   return std::get<0>(iter->second);
}

int retrieve_syntax_kind_serialization_code(SyntaxKind kind)
{
   auto iter = scg_syntaxKindTable.find(kind);
   if (iter == scg_syntaxKindTable.end()) {
      return -1;
   }
   return std::get<1>(iter->second);
}

std::pair<std::uint32_t, std::uint32_t> retrieve_syntax_kind_child_count(SyntaxKind kind)
{
   auto iter = scg_syntaxKindTable.find(kind);
   if (iter == scg_syntaxKindTable.end()) {
      return {-1, -1};
   }
   return {std::get<2>(iter->second), std::get<3>(iter->second)};
}

} // polar::syntax
