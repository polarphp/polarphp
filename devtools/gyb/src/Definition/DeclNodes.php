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
   ],
   /**
    * class_const_identifier:
    *    class_name T_PAAMAYIM_NEKUDOTAYIM identifier
    * |  variable_class_name T_PAAMAYIM_NEKUDOTAYIM identifier
    */
   [
      'kind' => 'ClassConstIdentifierExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'ClassName',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'VariableClassName', 'kind' => 'VariableClassNameClause'],
               ['name' => 'ClassName', 'kind' => 'ClassNameClause']
            ]
         ],
         ['name' => 'Separator', 'kind' => 'PaamayimNekudotayimToken'],
         ['name' => 'Identifier', 'kind' => 'Identifier']
      ]
   ],
   /**
    * constant:
    *    name
    * |  class_name T_PAAMAYIM_NEKUDOTAYIM identifier
    * |  variable_class_name T_PAAMAYIM_NEKUDOTAYIM identifier
    */
   [
      'kind' => 'ConstExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'Identifier',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'NameIdentifer', 'kind' => 'Name'],
               ['name' => 'ClassConstIdentifier', 'kind' => 'ClassConstIdentifierExpr']
            ]
         ]
      ]
   ],
   /**
    * new_variable:
    *    simple_variable
    * |  new_variable '[' optional_expr ']'
    * |  new_variable '{' expr '}'
    * |  new_variable T_OBJECT_OPERATOR property_name
    * |  class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable
    * |  new_variable T_PAAMAYIM_NEKUDOTAYIM simple_variable
    */
   [
      'kind' => 'NewVariableClause',
      'baseKind' => 'Syntax',
      'children' => [
         'name' => 'Variable',
         'kind' => 'Expr',
         'nodeChoices' => [
            ['name' => 'SimpleVariable', 'kind' => 'SimpleVariableExpr'],
            ['name' => 'ArrayItemVariable', 'kind' => 'ArrayAccessExpr'],
            ['name' => 'BraceDecoratedArrayItemVariable', 'kind' => 'ArrayAccessExpr'],
            ['name' => 'InstanceProperty', 'kind' => 'InstancePropertyExpr'],
            ['name' => 'StaticProperty', 'kind' => 'StaticPropertyExpr']
         ]
      ]
   ],
   /**
    * callable_variable:
    *    simple_variable
    * |  dereferencable '[' optional_expr ']'
    * |  constant '[' optional_expr ']'
    * |  dereferencable '{' expr '}'
    * |  dereferencable T_OBJECT_OPERATOR property_name argument_list
    * |  function_call
    */
   [
      'kind' => 'CallableVariableExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'Variable',
            'kind' => 'Expr',
            'nodeChoices' => [
               ['name' => 'SimpleVariable', 'kind' => 'SimpleVariableExpr'],
               ['name' => 'ArrayItemVariable', 'kind' => 'ArrayAccessExpr'],
               ['name' => 'BraceDecoratedArrayItemVariable', 'kind' => 'BraceDecoratedArrayAccessExpr'],
               ['name' => 'InstanceMethodCall', 'kind' => 'InstanceMethodCallExpr'],
               ['name' => 'FunctionCall', 'kind' => 'FunctionCallExpr']
            ]
         ]
      ]
   ],
   /**
    * callable_expr:
    *    callable_variable
    * |  '(' expr ')'
    * |  dereferencable_scalar
    */
   [
      'kind' => 'CallableFuncNameClause',
      'baseKind' => 'Syntax',
      'children' => [
         [
            'name' => 'FuncName',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'CallableVariableFuncName', 'kind' => 'CallableVariableExpr'],
               ['name' => 'ParenDecoratedExprFuncName', 'kind' => 'ParenDecoratedExpr'],
               ['name' => 'DereferencableScalarFuncName', 'kind' => 'DereferencableScalarExpr']
            ]
         ]
      ]
   ],
   /**
    * member_name:
    *    identifier
    * |  '{' expr '}'
    * |  simple_variable
    */
   [
      'kind' => 'MemberNameClause',
      'baseKind' => 'Syntax',
      'children' => [
         [
            'name' => 'Name',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'IdentifierName', 'kind' => 'Identifier'],
               ['name' => 'BraceDecoratedExprClauseName', 'kind' => 'BraceDecoratedExprClause'],
               ['name' => 'SimpleVariableExprName', 'kind' => 'SimpleVariableExpr']
            ]
         ]
      ]
   ],
   /**
    * property_name:
    *    T_IDENTIFIER_STRING
    * |  '{' expr '}'
    * |  simple_variable
    */
   [
      'kind' => 'PropertyNameClause',
      'baseKind' => 'Syntax',
      'children' => [
         [
            'name' => 'Name',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'TokenPropertyName', 'kind' => 'IdentifierStringToken'],
               ['name' => 'BraceDecoratedExprPropertyName', 'kind' => 'BraceDecoratedExprClause'],
               ['name' => 'SimpleVariableExprPropertyName', 'kind' => 'SimpleVariableExpr'],
            ]
         ]
      ]
   ],
   /**
    * object_property_access:
    *    new_variable T_OBJECT_OPERATOR property_name
    * |  dereferencable T_OBJECT_OPERATOR property_name
    * |  T_VARIABLE T_OBJECT_OPERATOR T_IDENTIFIER_STRING
    */
   [
      'kind' => 'InstancePropertyExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'ObjectRef',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'TokenInstance', 'kind' => 'VariableToken'],
               ['name' => 'NewVariableInstance', 'kind' => 'NewVariable'],
               ['name' => 'DereferencableInstance', 'kind' => 'DereferencableClause']
            ]
         ],
         [
            'name' => 'Separator', 'kind' => 'ObjectOperatorToken'
         ],
         [
            'name' => 'PropertyName',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'TokenProperty', 'kind' => 'IdentifierStringToken'],
               ['name' => 'PropertyNameClause', 'kind' => 'PropertyNameClause']
            ]
         ]
      ]
   ],
   /**
    * static_member:
    *    class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable
    * |  variable_class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable
    * |  new_variable T_PAAMAYIM_NEKUDOTAYIM simple_variable
    */
   [
      'kind' => 'StaticPropertyExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'ClassName',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'ClassName', 'kind' => 'ClassNameClause'],
               ['name' => 'VariableClassName', 'kind' => 'VariableClassNameClause'],
               ['name' => 'NewVariableClassName', 'kind' => 'NewVariableClause']
            ]
         ],
         ['name' => 'Separator', 'kind' => 'PaamayimNekudotayimToken'],
         ['name' => 'MemberName', 'kind' => 'SimpleVariableExpr']
      ]
   ],
   /**
    * argument:
    *    expr
    * |  T_ELLIPSIS expr
    */
   [
      'kind' => 'Argument',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'EllipsisToken', 'kind' => 'EllipsisToken', 'isOptional' => true],
         ['name' => 'Expr', 'kind' => 'Expr']
      ]
   ],
   /**
    * argument_list_item:
    *    argument ','
    */
   [
      'kind' => 'ArgumentListItem',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'CommaToken', 'kind' => 'CommaToken', 'isOptional' => true],
         ['name' => 'Argument', 'kind' => 'Argument']
      ]
   ],
   /**
    * argument_list:
    *    '(' ')'
    * |  '(' non_empty_argument_list possible_comma ')'
    */
   [
      'kind' => 'ArgumentListClause',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'LeftParenToken', 'kind' => 'LeftParenToken'],
         ['name' => 'Arguments', 'kind' => 'ArgumentList'],
         ['name' => 'RightParenToken', 'kind' => 'RightParenToken']
      ]
   ],
   /**
    * dereferencable_clause:
    *    variable
    * |  '(' expr ')'
    * |  dereferencable_scalar
    */
   [
      'kind' => 'DereferencableClause',
      'baseKind' => 'Syntax',
      'children' => [
         [
            'name' => 'DereferencableExpr',
            'kind' => 'Expr',
            'nodeChoices' => [
               ['name' => 'VariableExpr', 'kind' => 'VariableExpr'],
               ['name' => 'ParenDecoratedExpr', 'kind' => 'ParenDecoratedExpr'],
               ['name' => 'DereferencableScalarExpr', 'kind' => 'DereferencableScalarExpr']
            ]
         ]
      ]
   ],
   /**
    * variable_class_name:
    *    dereferencable
    */
   [
      'kind' => 'VariableClassNameClause',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'DereferencableClassName', 'kind' => 'DereferencableClause']
      ]
   ]
);