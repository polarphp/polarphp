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
class VariableExprSyntax;
class BraceDecoratedExprClauseSyntax;
class BraceDecoratedVariableExprSyntax;
class ArrayKeyValuePairItemSyntax;
class ArrayUnpackPairItemSyntax;
class ArrayPairItemSyntax;
class ListRecursivePairItemSyntax;
class ListPairItemSyntax;
class SimpleVariableExprSyntax;
class ArrayExprSyntax;
class SimplifiedArrayExprSyntax;
class ClassRefParentExprSyntax;
class ClassRefStaticExprSyntax;
class ClassRefSelfExprSyntax;
class IntegerLiteralExprSyntax;
class FloatLiteralExprSyntax;
class StringLiteralExprSyntax;

/// for encaps var syntax
class EncapsVarOffsetSyntax;
class EncapsArrayVarSyntax;
class EncapsObjPropSyntax;
class EncapsDollarCurlyExprSyntax;
class EncapsDollarCurlyVarSyntax;
class EncapsDollarCurlyArraySyntax;
class EncapsCurlyVarSyntax;
class EncapsVarSyntax;

class HeredocExprSyntax;
class EncapsListStringExprSyntax;
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

///
/// type: SyntaxCollection
/// element type: ArrayPairItemSyntax
///
/// array_pair_item_list:
///   array_pair_item_list ',' array_pair_item
/// | array_pair_item
///
using ArrayPairItemListSyntax = SyntaxCollection<SyntaxKind::ArrayPairItemList, ArrayPairItemSyntax>;

///
/// type: SyntaxCollection
/// element type: ListPairItemSyntax
///
/// list_pair_item_list:
///   list_pair_item_list ',' array_pair_item
/// | list_pair_item_list
///
using ListPairItemListSyntax = SyntaxCollection<SyntaxKind::ListPairItemList, ListPairItemSyntax>;

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_EXPR_NODES_FWD_H
