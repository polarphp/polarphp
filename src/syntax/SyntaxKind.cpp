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

bool is_collection_kind(SyntaxKind kind)
{
   return false;
}

bool is_decl_kind(SyntaxKind kind)
{
   return false;
}

bool is_type_kind(SyntaxKind kind)
{
   return false;
}

bool is_stmt_kind(SyntaxKind kind)
{
   return false;
}

bool is_expr_kind(SyntaxKind kind)
{
   return false;
}

bool is_token_kind(SyntaxKind kind)
{
   return false;
}

bool is_unknown_kind(SyntaxKind kind)
{
   return false;
}

SyntaxKind get_unknown_kind(SyntaxKind kind)
{
   return kind;
}

bool parser_shall_omit_when_no_children(SyntaxKind kind)
{
   return false;
}

} // polar::syntax
