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

return array(
   /**
    * paren_decorated_expr：
    * '(' expr ')'
    */
   [
      'kind' => 'ParenDecoratedExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'LeftParenToken', 'kind' => 'LeftParenToken'],
         ['name' => 'Expr', 'kind' => 'Expr'],
         ['name' => 'RightParenToken', 'kind' => 'RightParenToken'],
      ]
   ],
   /**
    * null_expr:
    *    T_NULL
    */
   [
      'kind' => 'NullExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'NullKeyword', 'kind' => 'NullKeyword']
      ]
   ],
   /**
    * optional_expr:
    *   %empty
    * | expr
    */
   [
      'kind' => 'OptionalExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'Expr', 'kind' => 'Expr']
      ]
   ],
   /**
    * expr_list_item:
    *    ',' expr
    * |  expr
    */
   [
      'kind' => 'ExprListItem',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'Comma', 'kind' => 'CommaToken', 'isOptional' => true],
         ['name' => 'Expr', 'kind' => 'Expr']
      ]
   ],
   /**
    * variable:
    *    callable_variable
    * |  static_member
    * |  dereferencable T_OBJECT_OPERATOR property_name
    */
   [
      'kind' => 'VariableExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'Var',
            'kind' => 'Expr',
            'nodeChoices' => [
               ['name' => 'Variable', 'kind' => 'CallableVariableExpr'],
               ['name' => 'StaticProperty', 'kind' => 'StaticPropertyExpr'],
               ['name' => 'InstanceProperty', 'kind' => 'InstancePropertyExpr']
            ]
         ]
      ]
   ],
   /**
    * referenced_variable_expr:
    *    '&' variable
    */
   [
      'kind' => 'ReferencedVariableExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'RefToken',
            'kind' => 'AmpersandToken'
         ]
      ]
   ]
);