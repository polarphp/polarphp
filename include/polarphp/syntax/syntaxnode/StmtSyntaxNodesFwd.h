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
class StaticVariableDeclaresStmtSyntax;
class GlobalVariableDeclaresStmtSyntax;

// stmt wrapper for decl syntax nodes
class ClassDefinitionStmtSyntax;
class InterfaceDefinitionStmtSyntax;
class TraitDefinitionStmtSyntax;
class FunctionDefinitionStmtSyntax;

// expr syntax nodes forward declares
class ExprListItemSyntax;

// decl syntax nodes forward declares
class ClassDefinitionSyntax;
class InterfaceDefinitionSyntax;
class TraitDefinitionSyntax;
class FunctionDefinitionSyntax;
class NameSyntax;

///
/// type: SyntaxCollection
/// element type: ExprSyntax
///
using ExprListSyntax = SyntaxCollection<SyntaxKind::ExprList, ExprListItemSyntax>;

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

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_STMT_NODES_FWD_H
