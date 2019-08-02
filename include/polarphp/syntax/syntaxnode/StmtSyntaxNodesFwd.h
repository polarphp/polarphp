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
class CommonStmtSyntax;
class ConditionElementSyntax;
class ContinueStmtSyntax;
class BreakStmtSyntax;
class FallthroughStmtSyntax;
class ElseIfClauseSyntax;
class IfStmtSyntax;
class WhileStmtSyntax;
class DoWhileStmtSyntax;
class SwitchCaseSyntax;
class SwitchDefaultLabelSyntax;
class SwitchCaseLabelSyntax;
class SwitchStmtSyntax;
class DeferStmtSyntax;
class ExpressionStmtSyntax;
class ThrowStmtSyntax;
class ReturnStmtSyntax;

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

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_STMT_NODES_FWD_H
