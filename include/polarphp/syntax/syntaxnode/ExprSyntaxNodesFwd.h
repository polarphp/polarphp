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
class ParenDecoratedExprSyntax;
class NullExprSyntax;
class OptionalExprSyntax;

class VariableExprSyntax;
class ClassConstIdentifierExprSyntax;
class ConstExprSyntax;
class StaticMemberExprSyntax;
class NewVariableClauseSyntax;
class CallableVariableExprSyntax;
class CallableFuncNameClauseSyntax;
class MemberNameClauseSyntax;
class PropertyNameClauseSyntax;
class InstancePropertyExprSyntax;
class StaticPropertyExprSyntax;

// argument clauses
class ArgumentSyntax;
class ArgumentListItemSyntax;
class ArgumentListClauseSyntax;

class DereferencableClauseSyntax;
class VariableClassNameClauseSyntax;
class ClassNameClauseSyntax;
class BraceDecoratedExprClauseSyntax;
class BraceDecoratedVariableExprSyntax;
class ArrayKeyValuePairItemSyntax;
class ArrayUnpackPairItemSyntax;
class ArrayPairItemSyntax;
class ListRecursivePairItemSyntax;
class ListPairItemSyntax;
class SimpleVariableExprSyntax;
class ArrayCreateExprSyntax;
class SimplifiedArrayCreateExprSyntax;
class ArrayAccessExprSyntax;
class BraceDecoratedArrayAccessExprSyntax;
class SimpleFunctionCallExprSyntax;
class FunctionCallExprSyntax;
class InstanceMethodCallExprSyntax;
class StaticMethodCallExprSyntax;
class DereferencableScalarExprSyntax;
class ScalarExprSyntax;
class ClassRefParentExprSyntax;
class ClassRefStaticExprSyntax;
class ClassRefSelfExprSyntax;
class IntegerLiteralExprSyntax;
class FloatLiteralExprSyntax;
class StringLiteralExprSyntax;

// for encaps var syntax
class EncapsVarOffsetSyntax;
class EncapsArrayVarSyntax;
class EncapsObjPropSyntax;
class EncapsDollarCurlyExprSyntax;
class EncapsDollarCurlyVarSyntax;
class EncapsDollarCurlyArraySyntax;
class EncapsCurlyVarSyntax;
class EncapsVarSyntax;
class EncapsListItemSyntax;

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

// Decl Syntax nodes forward declares
class NameSyntax;
class IdentifierSyntax;

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

///
/// type: SyntaxCollection
/// element type: EncapsListItemSyntax
///
/// encaps_list:
///   encaps_list encaps_var
/// | encaps_list T_ENCAPSED_AND_WHITESPACE
/// | encaps_var
/// | T_ENCAPSED_AND_WHITESPACE encaps_var
///
using EncapsItemListSyntax = SyntaxCollection<SyntaxKind::EncapsListItem, EncapsListItemSyntax>;

///
/// type: SyntaxCollection
/// element type: ArgumentSyntax
///
/// non_empty_argument_list:
///   argument
/// | non_empty_argument_list ',' argument
///
using ArgumentListSyntax = SyntaxCollection<SyntaxKind::ArgumentList, ArgumentListItemSyntax>;

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_EXPR_NODES_FWD_H
