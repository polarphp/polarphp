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

#ifndef POLARPHP_SYNTAX_SYNTAX_NODE_STMT_NODES_FWD_H
#define POLARPHP_SYNTAX_SYNTAX_NODE_STMT_NODES_FWD_H

#include "polarphp/syntax/SyntaxCollection.h"

namespace polar::syntax {

class EmptyStmtSyntax;
class NestStmtSyntax;
class ExprStmtSyntax;
class InnerStmtSyntax;
class InnerCodeBlockStmtSyntax;
class TopStmtSyntax;
class DeclareStmtSyntax;
class GotoStmtSyntax;
class UnsetVariableSyntax;
class UnsetStmtSyntax;
class LabelStmtSyntax;
class ConditionElementSyntax;
class ContinueStmtSyntax;
class BreakStmtSyntax;
class FallthroughStmtSyntax;
class ElseIfClauseSyntax;
class IfStmtSyntax;
class WhileStmtSyntax;
class DoWhileStmtSyntax;
class ForStmtSyntax;
class ForeachVariableSyntax;
class ForeachStmtSyntax;
class SwitchCaseSyntax;
class SwitchDefaultLabelSyntax;
class SwitchCaseLabelSyntax;
class SwitchStmtSyntax;
class DeferStmtSyntax;
class ThrowStmtSyntax;
class TryStmtSyntax;
class FinallyClauseSyntax;
class CatchArgTypeHintItemSyntax;
class CatchListItemClauseSyntax;
class ReturnStmtSyntax;
class EchoStmtSyntax;
class HaltCompilerStmtSyntax;
class GlobalVariableListItemSyntax;
class GlobalVariableDeclarationsStmtSyntax;
class StaticVariableListItemSyntax;
class StaticVariableDeclarationsStmtSyntax;

class NamespaceUseTypeSyntax;
class NamespaceUnprefixedUseDeclarationSyntax;
class NamespaceUseDeclarationSyntax;
class NamespaceInlineUseDeclarationSyntax;
class NamespaceGroupUseDeclarationSyntax;
class NamespaceMixedGroupUseDeclarationSyntax;
class NamespaceUseStmtSyntax;

class NamespaceDefinitionStmtSyntax;
class NamespaceBlockStmtSyntax;

// stmt wrapper for decl syntax nodes
class ClassDefinitionStmtSyntax;
class InterfaceDefinitionStmtSyntax;
class TraitDefinitionStmtSyntax;
class FunctionDefinitionStmtSyntax;

// expr syntax nodes forward declares
class ExprListItemSyntax;
class SimpleVariableExprSyntax;
class VariableExprSyntax;
class ReferencedVariableExprSyntax;
class ListStructureClauseSyntax;
class SimplifiedArrayCreateExprSyntax;
using ExprListSyntax = SyntaxCollection<SyntaxKind::ExprList, ExprListItemSyntax>;

// decl syntax nodes forward declares
class ClassDefinitionSyntax;
class InterfaceDefinitionSyntax;
class TraitDefinitionSyntax;
class FunctionDefinitionSyntax;
class NameSyntax;
class NamespacePartSyntax;
class ConstDeclareItemSyntax;
using NamespacePartListSyntax = SyntaxCollection<SyntaxKind::NamespacePartList, NamespacePartSyntax>;
using ConstDeclareItemListSyntax = SyntaxCollection<SyntaxKind::ConstDeclareItemList, ConstDeclareItemSyntax>;

///
/// type: SyntaxCollection
/// element type: ConditionElementSyntax
///
/// condition-list -> condition
///                 | condition-list, condition ','?
using ConditionElementListSyntax = SyntaxCollection<SyntaxKind::ConditionElementList, ConditionElementSyntax>;

///
/// type: SyntaxCollection
/// element type: SwitchCaseSyntax
///
/// switch-case-list ->  switch-case
///                    | switch-case-list switch-case
using SwitchCaseListSyntax = SyntaxCollection<SyntaxKind::SwitchCaseList, SwitchCaseSyntax>;

///
/// type: SyntaxCollection
/// element type: ElseIfClauseSyntax
///
/// elseif-list ->  elseif-clause
///                | elseif-list elseif-clause
using ElseIfListSyntax = SyntaxCollection<SyntaxKind::ElseIfList, ElseIfClauseSyntax>;

///
/// inner_statement_list:
///   inner_statement_list inner_statement
///
using InnerStmtListSyntax = SyntaxCollection<SyntaxKind::InnerStmtList, InnerStmtSyntax>;

///
/// catch_list:
///   catch_list T_CATCH '(' catch_name_list T_VARIABLE ')' '{' inner_statement_list '}'
///
using CatchListSyntax = SyntaxCollection<SyntaxKind::CatchList, CatchListItemClauseSyntax>;

///
/// catch_name_list:
///   name
/// | catch_name_list '|' name
///
using CatchArgTypeHintListSyntax = SyntaxCollection<SyntaxKind::CatchArgTypeHintList, CatchArgTypeHintItemSyntax>;

///
/// unset_variable_list:
///   unset_variables:
///   unset_variable
/// | unset_variables ',' unset_variable
///
using UnsetVariableListSyntax = SyntaxCollection<SyntaxKind::UnsetVariableList, UnsetVariableSyntax>;

///
/// static_var_list:
///   static_var_list ',' static_var
/// | static_var
///
using GlobalVariableListSyntax = SyntaxCollection<SyntaxKind::GlobalVariableList, GlobalVariableListItemSyntax>;

///
/// static_var_list:
///   static_var_list ',' static_var
/// | static_var
///
using StaticVariableListSyntax = SyntaxCollection<SyntaxKind::StaticVariableList, StaticVariableListItemSyntax>;

///
/// type: SyntaxCollection
/// element type: NamespaceUseDeclarationSyntax
///
/// use_declarations:
///   use_declarations ',' use_declaration
/// | use_declaration
///
using NamespaceUseDeclarationListSyntax = SyntaxCollection<SyntaxKind::NamespaceUseDeclarationList, NamespaceUseDeclarationSyntax>;

///
/// type: SyntaxCollection
/// element type: NamespaceInlineUseDeclarationSyntax
///
/// inline_use_declarations:
///   inline_use_declarations ',' inline_use_declaration
/// | inline_use_declaration
///
using NamespaceInlineUseDeclarationListSyntax = SyntaxCollection<SyntaxKind::NamespaceInlineUseDeclarationList, NamespaceInlineUseDeclarationSyntax>;

///
/// type: SyntaxCollection
/// element type: NamespaceUnprefixedUseDeclarationSyntax
///
/// unprefixed_use_declarations:
///   unprefixed_use_declarations ',' unprefixed_use_declaration
/// | unprefixed_use_declaration
///
using NamespaceUnprefixedUseDeclarationListSyntax = SyntaxCollection<SyntaxKind::NamespaceUnprefixedUseDeclarationList, NamespaceUnprefixedUseDeclarationSyntax>;


} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_STMT_NODES_FWD_H
