// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/09.

#include "polarphp/syntax/SyntaxKind.h"

namespace polar::syntax {

void dump_syntax_kind(RawOutStream &outStream, const SyntaxKind kind)
{
   outStream << retrieve_syntax_kind_text(kind);
}

bool is_collection_kind(SyntaxKind kind)
{
   switch (kind) {
   default:
      return false;
   }
}

bool is_decl_kind(SyntaxKind kind)
{
   return kind >= SyntaxKind::FirstDecl && kind <= SyntaxKind::LastDecl;
}

bool is_type_kind(SyntaxKind kind)
{
   return kind >= SyntaxKind::FirstType && kind <= SyntaxKind::LastType;
}

bool is_stmt_kind(SyntaxKind kind)
{
   return kind >= SyntaxKind::FirstStmt && kind <= SyntaxKind::LastStmt;
}

bool is_expr_kind(SyntaxKind kind)
{
   return kind >= SyntaxKind::FirstExpr && kind <= SyntaxKind::LastExpr;
}

bool is_token_kind(SyntaxKind kind)
{
   return kind == SyntaxKind::Token;
}

bool is_unknown_kind(SyntaxKind kind)
{
   return kind == SyntaxKind::Unknown ||
         kind == SyntaxKind::UnknownDecl ||
         kind == SyntaxKind::UnknownExpr ||
         kind == SyntaxKind::UnknownStmt ||
         kind == SyntaxKind::UnknownType;
}

SyntaxKind get_unknown_kind(SyntaxKind kind)
{
   if (is_expr_kind(kind)) {
      return SyntaxKind::UnknownExpr;
   }
   if (is_stmt_kind(kind)) {
      return SyntaxKind::UnknownStmt;
   }
   if (is_decl_kind(kind)) {
      return SyntaxKind::UnknownDecl;
   }
   if (is_type_kind(kind)) {
      return SyntaxKind::UnknownType;
   }
   return SyntaxKind::Unknown;
}

bool parser_shall_omit_when_no_children(SyntaxKind kind)
{
   switch (kind) {
   default:
      return false;
   }
}

} // polar::syntax

namespace polar::utils {
RawOutStream &operator<<(RawOutStream &outStream, polar::syntax::SyntaxKind kind)
{
   outStream << polar::syntax::retrieve_syntax_kind_text(kind);
   return outStream;
}
} // polar::utils
