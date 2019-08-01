// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/08/01.

#ifndef POLARPHP_SYNTAX_SYNTAX_NODE_COMMON_NODES_FWD_H
#define POLARPHP_SYNTAX_SYNTAX_NODE_COMMON_NODES_FWD_H

#include "polarphp/syntax/SyntaxCollection.h"

namespace polar::syntax {

class DeclSyntax;
class ExprSyntax;
class StmtSyntax;
class UnknownDeclSyntax;
class UnknownExprSyntax;
class UnknownStmtSyntax;
class UnknownTypeSyntax;
class CodeBlockItemSyntax;

/// type: SyntaxCollection
/// element type: CodeBlockItem
using CodeBlockItemListSyntax = SyntaxCollection<SyntaxKind::CodeBlockItemList, CodeBlockItemSyntax>;

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_COMMON_NODES_FWD_H
