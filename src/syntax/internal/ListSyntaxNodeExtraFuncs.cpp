// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/15.

#include "polarphp/syntax/internal/ListSyntaxNodeExtraFuncs.h"

namespace polar::syntax::internal {
namespace canserveascollectionmemberraw {

bool check_code_block_item_list(SyntaxKind memberKind)
{
   return true;
}

bool check_token_list(SyntaxKind memberKind)
{
   return true;
}

bool check_non_empty_token_list(SyntaxKind memberKind)
{
   return true;
}

} // canserveascollectionmemberraw
} // polar::syntax::internal
