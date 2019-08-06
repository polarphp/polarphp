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

#ifndef POLARPHP_SYNTAX_SYNTAX_NODE_EXPR_NODES_FWD_H
#define POLARPHP_SYNTAX_SYNTAX_NODE_EXPR_NODES_FWD_H

#include "polarphp/syntax/SyntaxCollection.h"

namespace polar::syntax {

class ExprSyntax;
class NullExprSyntax;
class BraceDecoratedExprClauseSyntax;
class ClassRefParentExprSyntax;
class ClassRefStaticExprSyntax;
class ClassRefSelfExprSyntax;
class IntegerLiteralExprSyntax;
class FloatLiteralExprSyntax;
class StringLiteralExprSyntax;
class BooleanLiteralExprSyntax;
class TernaryExprSyntax;
class AssignmentExprSyntax;
class SequenceExprSyntax;
class PrefixOperatorExprSyntax;
class PostfixOperatorExprSyntax;
class BinaryOperatorExprSyntax;
class UseLexicalVarClauseSyntax;
class LexicalVarItemSyntax;

/// type: SyntaxCollection
/// element type: ExprSyntax
using ExprListSyntax = SyntaxCollection<SyntaxKind::ExprList, ExprSyntax>;

///
/// type: SyntaxCollection
/// element type: LexicalVarItemSyntax
///
/// lexical_var_list:
///   lexical_var_list ',' lexical_var
/// | lexical_var
///
using LexicalVarListSyntax = SyntaxCollection<SyntaxKind::LexicalVarList, LexicalVarItemSyntax>;

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_EXPR_NODES_FWD_H
