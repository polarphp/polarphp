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

#ifndef POLARPHP_INTERNAL_LIST_SYNTAX_NODE_EXTRA_FUNCS_H
#define POLARPHP_INTERNAL_LIST_SYNTAX_NODE_EXTRA_FUNCS_H

#include <cstdint>
#include <set>
#include "polarphp/syntax/References.h"

namespace polar::syntax {
enum class SyntaxKind : std::uint32_t;
class RawSyntax;
class SyntaxArena;
} // polar::syntax

namespace polar::basic {
template<typename T>
class ArrayRef;
} // polar::basic

namespace polar::syntax::internal {

std::set<SyntaxKind> retrive_collection_syntax_element_type_choices(SyntaxKind kind);

using polar::basic::ArrayRef;

namespace canserveascollectionmemberraw {

bool check_token_list(SyntaxKind memberKind);
bool check_non_empty_token_list(SyntaxKind memberKind);

} // canserveascollectionmemberraw

namespace abstractfactorycreateraw {
bool need_invoke_create_raw_func(SyntaxKind kind);
/// common syntax nodes
RefCountPtr<RawSyntax> create_code_block_item_list_raw(ArrayRef<RefCountPtr<RawSyntax>> elements,
                                                       RefCountPtr<SyntaxArena> arena);
RefCountPtr<RawSyntax> create_non_empty_token_list_raw(ArrayRef<RefCountPtr<RawSyntax>> elements,
                                                       RefCountPtr<SyntaxArena> arena);
RefCountPtr<RawSyntax> create_token_list_raw(ArrayRef<RefCountPtr<RawSyntax>> elements,
                                             RefCountPtr<SyntaxArena> arena);
RefCountPtr<RawSyntax> create_code_block_item_raw(ArrayRef<RefCountPtr<RawSyntax>> elements,
                                                  RefCountPtr<SyntaxArena> arena);

} // abstractfactorycreateraw

} // polar::syntax::internal

#endif // POLARPHP_INTERNAL_LIST_SYNTAX_NODE_EXTRA_FUNCS_H
