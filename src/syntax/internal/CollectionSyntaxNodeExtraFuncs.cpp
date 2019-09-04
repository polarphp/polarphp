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

#include "polarphp/syntax/internal/CollectionSyntaxNodeExtraFuncs.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/syntax/SyntaxNodes.h"
#include "polarphp/syntax/SyntaxKind.h"
#include "polarphp/syntax/SyntaxArena.h"
#include "polarphp/syntax/RawSyntax.h"


namespace polar::syntax::internal {
namespace canserveascollectionmemberraw {

bool check_token_list(SyntaxKind memberKind)
{
   return TokenSyntax::kindOf(memberKind);
}

bool check_non_empty_token_list(SyntaxKind memberKind)
{
   return TokenSyntax::kindOf(memberKind);
}

} // canserveascollectionmemberraw

namespace abstractfactorycreateraw {

bool need_invoke_create_raw_func(SyntaxKind kind)
{
//   switch (kind) {
//   case SyntaxKind::CodeBlockItemList:
//   case SyntaxKind::TokenList:
//   case SyntaxKind::NonEmptyTokenList:
//   case SyntaxKind::CodeBlockItem:
//      return true;
//   default:
//      return false;
//   }
}

RefCountPtr<RawSyntax> create_code_block_item_list_raw(ArrayRef<RefCountPtr<RawSyntax>> elements,
                                                       RefCountPtr<SyntaxArena> arena)
{

}

RefCountPtr<RawSyntax> create_non_empty_token_list_raw(ArrayRef<RefCountPtr<RawSyntax>> elements,
                                                       RefCountPtr<SyntaxArena> arena)
{

}

RefCountPtr<RawSyntax> create_token_list_raw(ArrayRef<RefCountPtr<RawSyntax>> elements,
                                             RefCountPtr<SyntaxArena> arena)
{

}

RefCountPtr<RawSyntax> create_code_block_item_raw(ArrayRef<RefCountPtr<RawSyntax>> elements,
                                                  RefCountPtr<SyntaxArena> arena)
{

}

} // abstractfactorycreateraw
} // polar::syntax::internal
