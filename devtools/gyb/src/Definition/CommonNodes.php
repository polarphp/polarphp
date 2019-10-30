<?php

// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/10/29.

use Gyb\Syntax\Node;

return array(
   ['kind' => 'Decl', 'baseKind' => 'Syntax'],
   ['kind' => 'Expr', 'baseKind' => 'Syntax'],
   ['kind' => 'Stmt', 'baseKind' => 'Syntax'],
   ['kind' => 'UnknownDecl', 'baseKind' => 'Decl'],
   ['kind' => 'UnknownExpr', 'baseKind' => 'Expr'],
   ['kind' => 'UnknownStmt', 'baseKind' => 'Stmt']
);