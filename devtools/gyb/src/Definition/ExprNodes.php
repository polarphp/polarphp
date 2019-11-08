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

use function Gyb\Utils\process_collection_items;

$definitions = array(
   /**
    * paren_decorated_expr：
    * '(' expr ')'
    */
   [
      'kind' => 'ParenDecoratedExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'LeftParen', 'kind' => 'LeftParenToken'],
         ['name' => 'Expr', 'kind' => 'Expr'],
         ['name' => 'RightParen', 'kind' => 'RightParenToken'],
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
         ['name' => 'RefToken', 'kind' => 'AmpersandToken'],
         ['name' => 'Variable', 'kind' => 'VariableExpr'],
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
         [
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
               ['name' => 'NewVariableInstance', 'kind' => 'NewVariableClause'],
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
         ['name' => 'Ellipsis', 'kind' => 'EllipsisToken', 'isOptional' => true],
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
         ['name' => 'Comma', 'kind' => 'CommaToken', 'isOptional' => true],
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
         ['name' => 'LeftParen', 'kind' => 'LeftParenToken'],
         [
            'name' => 'Arguments',
            'kind' => 'ArgumentList',
            'collectionElementName' => 'Argument',
            'isOptional' => true
         ],
         ['name' => 'RightParen', 'kind' => 'RightParenToken']
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
   ],
   /**
    * class_name:
    *    T_STATIC
    * |  name
    */
   [
      'kind' => 'ClassNameClause',
      'baseKind' => 'Syntax',
      'children' => [
         [
            'name' => 'Name',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'StaticClassName', 'kind' => 'StaticKeyword'],
               ['name' => 'NameExprClassName', 'kind' => 'Name']
            ]
         ]
      ]
   ],
   /**
    * class_name_reference:
    *    class_name
    * |  new_variable
    */
   [
      'kind' => 'ClassNameRefClause',
      'baseKind' => 'Syntax',
      'children' => [
         [
            'name' => 'NameRef',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'ClassName', 'kind' => 'ClassNameClause'],
               ['name' => 'NewVariableClassName', 'kind' => 'NewVariableClause']
            ]
         ]
      ]
   ],
   /**
    * brace_decorated_expr_clause:
    *    '{' expr '}'
    */
   [
      'kind' => 'BraceDecoratedExprClause',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'LeftBrace', 'kind' => 'LeftBraceToken'],
         ['name' => 'Expr', 'kind' => 'Expr'],
         ['name' => 'RightBrace', 'kind' => 'RightBraceToken']
      ]
   ],
   /**
    * expr:
    *    '$' '{' expr '}'
    */
   [
      'kind' => 'BraceDecoratedVariableExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'DollarSign', 'kind' => 'DollarToken'],
         ['name' => 'DecoratedExpr', 'kind' => 'BraceDecoratedExprClause']
      ]
   ],
   /**
    * array_key_value_pair_item:
    *    expr T_DOUBLE_ARROW expr
    * |  expr
    * |  expr T_DOUBLE_ARROW '&' variable
    * |  '&' variable
    */
   [
      'kind' => 'ArrayKeyValuePairItem',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'KeyExpr', 'kind' => 'Expr', 'isOptional' => true],
         ['name' => 'DoubleArrow', 'kind' => 'DoubleArrowToken', 'isOptional' => true],
         [
            'name' => 'Value',
            'kind' => 'Expr',
            'nodeChoices' => [
               ['name' => 'ReferencedVariableValue', 'kind' => 'ReferencedVariableExpr'],
               ['name' => 'ExprValue', 'kind' => 'Expr']
            ]
         ]
      ]
   ],
   /**
    * array_unpack_pair_item:
    *    T_ELLIPSIS expr
    */
   [
      'kind' => 'ArrayUnpackPairItem',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Ellipsis', 'kind' => 'EllipsisToken'],
         ['name' => 'UnpackExpr', 'kind' => 'Expr']
      ]
   ],
   /**
    * array_pair:
    *    array_key_value_pair_item
    * |  array_unpack_pair_item
    */
   [
      'kind' => 'ArrayPair',
      'baseKind' => 'Syntax',
      'children' => [
         [
            'name' => 'Item',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'ArrayKeyValuePairItem', 'kind' => 'ArrayKeyValuePairItem'],
               ['name' => 'ArrayUnpackPairItem', 'kind' => 'ArrayUnpackPairItem']
            ]
         ]
      ]
   ],
   /**
    * array_pair_list_item:
    *    ',' array_pair
    */
   [
      'kind' => 'ArrayPairListItem',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Comma', 'kind' => 'CommaToken', 'isOptional' => true],
         [
            'name' => 'ArrayPair',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'ArrayPair', 'kind' => 'ArrayPair'],
               ['name' => 'ListRecursivePairItem', 'kind' => 'ListRecursivePairItem']
            ],
            'isOptional' => true
         ]
      ]
   ],
   /**
    * list_recursive_pair_item:
    *    expr T_DOUBLE_ARROW T_LIST '(' list_pair_item_list ')'
    * |  T_LIST '(' list_pair_item_list ')'
    */
   [
      'kind' => 'ListRecursivePairItem',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'KeyExpr', 'kind' => 'Expr', 'isOptional' => true],
         ['name' => 'DoubleArrow', 'kind' => 'DoubleArrowToken', 'isOptional' => true],
         ['name' => 'ListToken', 'kind' => 'ListToken'],
         ['name' => 'LeftParen', 'kind' => 'LeftParenToken'],
         ['name' => 'Items', 'kind' => 'ArrayPairList', 'collectionElementName' => 'PairItem'],
         ['name' => 'RightParen', 'kind' => 'RightParenToken']
      ]
   ],
   /**
    * simple_variable:
    *    T_VARIABLE
    * |  brace_decorated_variable_expr
    * |  '$' simple_variable
    */
   [
      'kind' => 'SimpleVariableExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'DollarSign', 'kind' => 'DollarToken', 'isOptional' => true],
         [
            'name' => 'Variable',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'Variable', 'kind' => 'VariableToken'],
               ['name' => 'BraceDecoratedVariable', 'kind' => 'BraceDecoratedVariableExpr'],
               ['name' => 'SimpleVariable', 'kind' => 'SimpleVariableExpr'],
            ]
         ]
      ]
   ],
   /**
    * array_expr:
    *    T_ARRAY '(' array_pair_list ')'
    */
   [
      'kind' => 'ArrayCreateExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'Array', 'kind' => 'ArrayToken'],
         ['name' => 'LeftParen', 'kind' => 'LeftParenToken'],
         ['name' => 'Items', 'kind' => 'ArrayPairList', 'collectionElementName' => 'PairItem'],
         ['name' => 'RightParen', 'kind' => 'RightParenToken']
      ]
   ],
   /**
    * simplified_array_expr:
    *    '[' array_pair_list ']'
    */
   [
      'kind' => 'SimplifiedArrayCreateExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'LeftSquareBracket', 'kind' => 'LeftSquareBracketToken'],
         ['name' => 'Items', 'kind' => 'ArrayPairList', 'collectionElementName' => 'PairItem'],
         ['name' => 'RightSquareBracket', 'kind' => 'RightSquareBracketToken']
      ]
   ],
   /**
    * array_access:
    *    T_VARIABLE '[' encaps_var_offset ']'
    * |  new_variable '[' optional_expr ']'
    * |  constant '[' optional_expr ']'
    * |  dereferencable '[' optional_expr ']'
    */
   [
      'kind' => 'ArrayAccessExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'ArrayRef',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'VariableArrayRef', 'kind' => 'VariableToken'],
               ['name' => 'NewVariableArrayRef', 'kind' => 'NewVariableClause'],
               ['name' => 'DereferencableArrayRef', 'kind' => 'DereferencableClause'],
               ['name' => 'ConstExprArrayRef', 'kind' => 'ConstExpr']
            ]
         ],
         ['name' => 'LeftSquareBracket', 'kind' => 'LeftSquareBracketToken'],
         [
            'name' => 'Offset',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'EncapsVariableOffset', 'kind' => 'EncapsVariableOffset'],
               ['name' => 'OptionalExprOffset', 'kind' => 'OptionalExpr']
            ]
         ],
         ['name' => 'RightSquareBracket', 'kind' => 'RightSquareBracketToken']
      ]
   ],
   /**
    * brace_decorated_array_access:
    *    new_variable '{' expr '}'
    * |  dereferencable '{' expr '}'
    */
   [
      'kind' => 'BraceDecoratedArrayAccessExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'ArrayRef',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'NewVariableArrayRef', 'kind' => 'NewVariableClause'],
               ['name' => 'DereferencableArrayRef', 'kind' => 'DereferencableClause']
            ]
         ],
         ['name' => 'Offset', 'kind' => 'BraceDecoratedExprClause']
      ]
   ],
   /**
    * simple_function_call:
    *    name argument_list
    * |  callable_expr argument_list
    */
   [
      'kind' => 'SimpleFunctionCallExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'FuncName',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'NameFuncName', 'kind' => 'Name'],
               ['name' => 'CallableFuncName', 'kind' => 'CallableFuncNameClause']
            ]
         ],
         [
            'name' => 'ArgumentsClause',
            'kind' => 'ArgumentListClause'
         ]
      ]
   ],
   /**
    * function_call:
    *    name argument_list
    * |  class_name T_PAAMAYIM_NEKUDOTAYIM member_name argument_list
    * |  variable_class_name T_PAAMAYIM_NEKUDOTAYIM member_name argument_list
    * |  callable_expr argument_list
    */
   [
      'kind' => 'FunctionCallExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'Callable',
            'kind' => 'Expr',
            'nodeChoices' => [
               ['name' => 'SimpleFunctionCall', 'kind' => 'SimpleFunctionCallExpr'],
               ['name' => 'StaticMethodCall', 'kind' => 'StaticMethodCallExpr']
            ]
         ]
      ]
   ],
   /**
    * instance_method_call:
    *    dereferencable T_OBJECT_OPERATOR property_name argument_list
    */
   [
      'kind' => 'InstanceMethodCallExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'QualifiedMethodName', 'kind' => 'InstancePropertyExpr'],
         ['name' => 'Arguments', 'kind' => 'ArgumentListClause']
      ]
   ],
   /**
    * static_method_call:
    *    class_name T_PAAMAYIM_NEKUDOTAYIM member_name argument_list
    * |  variable_class_name T_PAAMAYIM_NEKUDOTAYIM member_name argument_list
    */
   [
      'kind' => 'StaticMethodCallExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'ClassName',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'ClassNameRef', 'kind' => 'ClassNameClause'],
               ['name' => 'VariableClassNameRef', 'kind' => 'VariableClassNameClause']
            ]
         ],
         ['name' => 'Separator', 'kind' => 'PaamayimNekudotayimToken'],
         ['name' => 'MethodName', 'kind' => 'MemberNameClause'],
         ['name' => 'Arguments', 'kind' => 'ArgumentListClause']
      ]
   ],
   /**
    * dereferencable_scalar:
    *    T_ARRAY '(' array_pair_list ')'
    * |  '[' array_pair_list ']'
    * |  T_CONSTANT_ENCAPSED_STRING
    */
   [
      'kind' => 'DereferencableScalarExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'ScalarValue',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'ArrayCreateExpr', 'kind' => 'ArrayCreateExpr'],
               ['name' => 'SimplifiedArrayCreateExpr', 'kind' => 'SimplifiedArrayCreateExpr'],
               ['name' => 'ConstExpr', 'kind' => 'ConstantEncapsedStringToken']
            ]
         ]
      ]
   ],
   /**
    * anonymous_class:
    *   T_CLASS ctor_arguments
    *   extends_from implements_list backup_doc_comment '{' class_statement_list '}'
    */
   [
      'kind' => 'AnonymousClassDefinitionClause',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'ClassKeyword', 'kind' => 'ClassKeyword'],
         ['name' => 'CtorArguments', 'kind' => 'ArgumentListClause', 'isOptional' => true],
         ['name' => 'ExtendsFrom', 'kind' => 'ExtendsFromClause', 'isOptional' => true],
         ['name' => 'Implements', 'kind' => 'ImplementsClause', 'isOptional' => true],
         ['name' => 'Members', 'kind' => 'MemberDeclBlock']
      ]
   ],
   /**
    * simple_instance_create_expr:
    *   T_NEW class_name_reference ctor_arguments
    */
   [
      'kind' => 'SimpleInstanceCreateExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'NewKeyword', 'kind' => 'NewKeyword'],
         ['name' => 'ClassName', 'kind' => 'ClassNameRefClause'],
         ['name' => 'CtorArgsClause', 'kind' => 'ArgumentListClause', 'isOptional' => true]
      ]
   ],
   /**
    * anonymous_instance_create:
    *    T_NEW anonymous_class
    */
   [
      'kind' => 'AnonymousInstanceCreateExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'NewKeyword', 'kind' => 'NewKeyword'],
         ['name' => 'AnonymousClassDef', 'kind' => 'AnonymousClassDefinitionClause']
      ]
   ],
   /**
    * inline_function:
    *   function returns_ref backup_doc_comment '(' parameter_list ')' lexical_vars return_type
    *   backup_fn_flags '{' inner_statement_list '}' backup_fn_flags
    */
   [
      'kind' => 'ClassicLambdaExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'FuncKeyword', 'kind' => 'FunctionKeyword'],
         ['name' => 'ReturnRef', 'kind' => 'AmpersandToken', 'isOptional' => true],
         ['name' => 'Parameters', 'kind' => 'ParameterClause'],
         ['name' => 'LexicalVars', 'kind' => 'UseLexicalVariableClause', 'isOptional' => true],
         ['name' => 'ReturnType', 'kind' => 'ReturnTypeClause', 'isOptional' => true],
         ['name' => 'Body', 'kind' => 'InnerCodeBlockStmt']
      ]
   ],
   /**
    * inline_function:
    *   fn returns_ref '(' parameter_list ')' return_type backup_doc_comment T_DOUBLE_ARROW backup_fn_flags backup_lex_pos expr backup_fn_flags
    */
   [
      'kind' => 'SimplifiedLambdaExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'FnKeyword', 'kind' => 'FnKeyword'],
         ['name' => 'ReturnRef', 'kind' => 'AmpersandToken', 'isOptional' => true],
         ['name' => 'Parameters', 'kind' => 'ParameterClause'],
         ['name' => 'ReturnType', 'kind' => 'ReturnTypeClause', 'isOptional' => true],
         ['name' => 'DoubleArrow', 'kind' => 'DoubleArrowToken'],
         ['name' => 'Body', 'kind' => 'Expr']
      ]
   ],
   /**
    * lambda_expr:
    *    function returns_ref backup_doc_comment '(' parameter_list ')' lexical_vars return_type
    *    backup_fn_flags '{' inner_statement_list '}' backup_fn_flags
    * |  fn returns_ref '(' parameter_list ')' return_type backup_doc_comment T_DOUBLE_ARROW backup_fn_flags backup_lex_pos expr backup_fn_flags
    */
   [
      'kind' => 'LambdaExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'StaticKeyword', 'kind' => 'StaticKeyword', 'isOptional' => true],
         [
            'name' => 'LambdaExpr',
            'kind' => 'Expr',
            'nodeChoices' => [
               ['name' => 'ClassicLambda', 'kind' => 'ClassicLambdaExpr'],
               ['name' => 'SimplifiedLambda', 'kind' => 'SimplifiedLambdaExpr']
            ]
         ]
      ]
   ],
   /**
    * new_expr:
    *    T_NEW class_name_reference ctor_arguments
    * |  T_NEW anonymous_class
    */
   [
      'kind' => 'InstanceCreateExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'CreateExpr',
            'kind' => 'Expr',
            'nodeChoices' => [
               ['name' => 'AnonymousInstanceExpr', 'kind' => 'AnonymousInstanceCreateExpr'],
               ['name' => 'SimpleInstanceExpr', 'kind' => 'SimpleInstanceCreateExpr']
            ]
         ]
      ]
   ],
   /**
    * scalar:
    *    T_LNUMBER
    * |  T_DNUMBER
    * |  T_LINE
    * |  T_FILE
    * |  T_DIR
    * |  T_TRAIT_CONST
    * |  T_METHOD_CONST
    * |  T_FUNC_CONST
    * |  T_NS_CONST
    * |  T_CLASS_CONST
    * |  T_START_HEREDOC T_ENCAPSED_AND_WHITESPACE T_END_HEREDOC
    * |  T_START_HEREDOC encaps_list T_END_HEREDOC
    * |  T_START_HEREDOC T_END_HEREDOC
    * |  '"' encaps_list '"'
    * |  dereferencable_scalar
    * |  constant
    */
   [
      'kind' => 'ScalarExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'ScalarValue',
            'kind' => 'Syntax',
            'nodeChoices' => [
               [
                  'name' => 'ScalarToken',
                  'kind' => 'Token',
                  'tokenChoices' => [
                     'LNumberToken',
                     'DNumberToken',
                     'LineKeyword',
                     'FileKeyword',
                     'DirKeyword',
                     'TraitConstKeyword',
                     'MethodConstKeyword',
                     'FuncConstKeyword',
                     'NamespaceConstKeyword',
                     'ClassConstKeyword'
                  ]
               ],
               ['name' => 'HeredocExpr', 'kind' => 'HeredocExpr'],
               ['name' => 'EncapsListString', 'kind' => 'EncapsListStringExpr'],
               ['name' => 'DereferencableScalar', 'kind' => 'DereferencableScalarExpr'],
               ['name' => 'ConstExpr', 'kind' => 'ConstExpr']
            ]
         ]
      ]
   ],
   /**
    * class_ref_parent:
    *    ClassRefParent
    */
   [
      'kind' => 'ClassRefParentExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'ParentKeyword', 'kind' => 'ClassRefParentKeyword']
      ]
   ],
   /**
    * class_self_parent:
    *    SelfKeyword
    */
   [
      'kind' => 'ClassRefSelfExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'SelfKeyword', 'kind' => 'ClassRefSelfKeyword']
      ]
   ],
   /**
    * class_static_parent:
    *    StaticKeyword
    */
   [
      'kind' => 'ClassRefStaticExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'StaticKeyword', 'kind' => 'ClassReStaticKeyword']
      ]
   ],
   /**
    * integer_literal_expr:
    *    digits
    */
   [
      'kind' => 'IntegerLiteralExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'Digits', 'kind' => 'LNumberToken']
      ]
   ],
   /**
    * float_literal_expr:
    *    float_digits
    */
   [
      'kind' => 'FloatLiteralExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'FloatDigits', 'kind' => 'DNumberToken']
      ]
   ],
   /**
    * string_literal:
    *   '"' T_CONSTANT_ENCAPSED_STRING '"'
    * | '\'' T_CONSTANT_ENCAPSED_STRING '\''
    */
   [
      'kind' => 'StringLiteralExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'LeftQuote',
            'kind' => 'Token',
            'tokenChoices' => [
               'SingleStrQuoteToken',
               'DoubleStrQuoteToken'
            ]
         ],
         ['name' => 'Text', 'kind' => 'ConstantEncapsedStringToken'],
         [
            'name' => 'RightQuote',
            'kind' => 'Token',
            'tokenChoices' => [
               'SingleStrQuoteToken',
               'DoubleStrQuoteToken'
            ]
         ],
      ]
   ],
   /**
    * boolean_literal_expr:
    *    TrueKeyword
    * |  FalseKeyword
    */
   [
      'kind' => 'BooleanLiteralExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'Value',
            'kind' => 'Token',
            'tokenChoices' => [
               'TrueKeyword',
               'FalseKeyword'
            ]
         ]
      ]
   ],
   /**
    * isset_variable:
    *   expr
    */
   [
      'kind' => 'IssetVariable',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'variableExpr', 'kind' => 'Expr']
      ]
   ],
   /**
    * isset_var_item:
    *    ','  expr
    */
   [
      'kind' => 'IssetVariableListItem',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Comma', 'kind' => 'CommaToken', 'isOptional' => true],
         ['name' => 'Variable', 'kind' => 'IssetVariable']
      ]
   ],
   /**
    * isset_vars_clause:
    *   '(' isset_variables_list ')'
    */
   [
      'kind' => 'IssetVariablesClause',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'LeftParen', 'kind' => 'LeftParenToken'],
         ['name' => 'Variables', 'kind' => 'IssetVariablesList', 'collectionElementName' => 'Variable'],
         ['name' => 'RightParen', 'kind' => 'RightParenToken']
      ]
   ],
   /**
    * isset_expr:
    *   T_ISSET '(' isset_variables possible_comma ')'
    */
   [
      'kind' => 'IssetFuncExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'IssetKeyword', 'kind' => 'IssetKeyword'],
         ['name' => 'VariablesClause', 'kind' => 'IssetVariablesClause']
      ]
   ],
   /**
    * empty_expr:
    *   T_EMPTY '(' expr ')'
    */
   [
      'kind' => 'EmptyFuncExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'EmptyKeyword', 'kind' => 'EmptyKeyword'],
         ['name' => 'Arguments', 'kind' => 'ParenDecoratedExpr']
      ]
   ],
   /**
    * include_expr:
    *    T_INCLUDE expr
    * |  T_INCLUDE_ONCE expr
    */
   [
      'kind' => 'IncludeExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'IncludeKeyword',
            'kind' => 'Token',
            'tokenChoices' => [
               'IncludeKeyword',
               'IncludeOnceKeyword'
            ]
         ],
         ['name' => 'PathExpr', 'kind' => 'Expr']
      ]
   ],
   /**
    *    T_REQUIRE expr
    * |  T_REQUIRE_ONCE expr
    */
   [
      'kind' => 'RequireExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'RequireKeyword',
            'kind' => 'Token',
            'tokenChoices' => [
               'RequireKeyword',
               'RequireOnceKeyword'
            ]
         ],
         ['name' => 'PathExpr', 'kind' => 'Expr']
      ]
   ],
   /**
    * eval_expr:
    *    T_EVAL '(' expr ')'
    */
   [
      'kind' => 'EvalFuncExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'EvalKeyword', 'kind' => 'EvalKeyword'],
         ['name' => 'Arguments', 'kind' => 'ParenDecoratedExpr']
      ]
   ],
   /**
    * print_func:
    *   T_PRINT expr
    */
   [
      'kind' => 'PrintFuncExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'PrintKeyword', 'kind' => 'PrintKeyword'],
         ['name' => 'Args', 'kind' => 'Expr']
      ]
   ],
   /**
    * func_like_expr:
    *    T_ISSET '(' isset_variables possible_comma ')'
    * |  T_EMPTY '(' expr ')'
    * |  T_INCLUDE expr
    * |  T_INCLUDE_ONCE expr
    * |  T_EVAL '(' expr ')'
    * |  T_REQUIRE expr
    * |  T_REQUIRE_ONCE expr
    */
   [
      'kind' => 'FuncLikeExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'FuncLikeExpr',
            'kind' => 'Expr',
            'nodeChoices' => [
               ['name' => 'IssetFunc', 'kind' => 'IssetFuncExpr'],
               ['name' => 'EmptyFunc', 'kind' => 'EmptyFuncExpr'],
               ['name' => 'IncludeExpr', 'kind' => 'IncludeExpr'],
               ['name' => 'RequireExpr', 'kind' => 'RequireExpr'],
               ['name' => 'EvalFunc', 'kind' => 'EvalFuncExpr']
            ]
         ]
      ]
   ],
   /**
    * array_structure_assignment:
    *   '[' array_pair_list ']' '=' expr
    */
   [
      'kind' => 'ArrayStructureAssignmentExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'ArrayStructure', 'kind' => 'SimplifiedArrayCreateExpr'],
         ['name' => 'EqualKeyword', 'kind' => 'EqualKeyword'],
         ['name' => 'ValueExpr', 'kind' => 'Expr']
      ]
   ],
   /**
    * list_structure_clause:
    *   T_LIST '(' array_pair_list ')'
    */
   [
      'kind' => 'ListStructureClause',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'ListKeyword', 'kind' => 'ListKeyword'],
         ['name' => 'LeftParen', 'kind' => 'LeftParenToken'],
         ['name' => 'PairItems', 'kind' => 'ArrayPairList', 'collectionElementName' => 'pairItem'],
         ['name' => 'RightParen', 'kind' => 'RightParenToken']
      ]
   ],
   /**
    * list_structure_assignment_expr:
    *   T_LIST '(' array_pair_list ')' = expr
    */
   [
      'kind' => 'ListStructureAssignmentExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'ListStrcuture', 'kind' => 'ListStructureClause'],
         ['name' => 'EqualKeyword', 'kind' => 'EqualKeyword'],
         ['name' => 'Value', 'kind' => 'Expr']
      ]
   ],
   /**
    * assign_expr:
    *    variable '=' expr
    * |  variable '=' '&' variable
    */
   [
      'kind' => 'AssignmentExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'Target', 'kind' => 'VariableExpr'],
         ['name' => 'AssignSign', 'kind' => 'EqualToken'],
         [
            'name' => 'Value',
            'kind' => 'Expr',
            'nodeChoices' => [
               ['name' => 'Expr', 'kind' => 'Expr'],
               ['name' => 'ReferencedVariable', 'kind' => 'ReferencedVariableExpr']
            ]
         ]
      ]
   ],
   /**
    * compound_assign_expr:
    *    variable T_PLUS_EQUAL expr
    * |  variable T_MINUS_EQUAL expr
    * |  variable T_MUL_EQUAL expr
    * |  variable T_POW_EQUAL expr
    * |  variable T_DIV_EQUAL expr
    * |  variable T_CONCAT_EQUAL expr
    * |  variable T_MOD_EQUAL expr
    * |  variable T_AND_EQUAL expr
    * |  variable T_OR_EQUAL expr
    * |  variable T_XOR_EQUAL expr
    * |  variable T_SL_EQUAL expr
    * |  variable T_SR_EQUAL expr
    * |  variable T_COALESCE_EQUAL expr
    */
   [
      'kind' => 'CompoundAssignmentExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'Target', 'kind' => 'VariableExpr'],
         [
            'name' => 'CompoundAssignSign',
            'kind' => 'Token',
            'tokenChoices' => [
               'PlusEqualToken',
               'MinusEqualToken',
               'MulEqualToken',
               'PowEqualToken',
               'DivEqualToken',
               'StrConcatEqualToken',
               'ModEqualToken',
               'AndEqualToken',
               'OrEqualToken',
               'XorEqualToken',
               'ShiftLeftEqualToken',
               'ShiftRightEqualToken',
               'CoalesceEqualToken'
            ]
         ],
         ['name' => 'ValueExpr', 'kind' => 'Expr']
      ]
   ],
   /**
    * logical_expr:
    *    expr T_BOOLEAN_OR expr
    * |  expr T_BOOLEAN_AND expr
    * |  expr T_LOGICAL_OR expr
    * |  expr T_LOGICAL_AND expr
    * |  expr T_LOGICAL_XOR expr
    */
   [
      'kind' => 'LogicalExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'Lhs', 'kind' => 'Expr'],
         [
            'name' => 'LogicalOperator',
            'kind' => 'Token',
            'tokenChoices' => [
               'BooleanOrToken',
               'BooleanAndToken',
               'LogicOrKeyword',
               'LogicAndKeyword',
               'LogicXorKeyword'
            ]
         ],
         ['name' => 'Rhs', 'kind' => 'Expr'],
      ]
   ],
   /**
    * bit_logical_expr:
    *   expr '|' expr
    * | expr '&' expr
    * | expr '^' expr
    */
   [
      'kind' => 'BitLogicalExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'Lhs', 'kind' => 'Expr'],
         [
            'name' => 'BitLogicalOperator',
            'kind' => 'Token',
            'tokenChoices' => [
               'VerticalBarToken',
               'AmpersandToken',
               'CaretToken'
            ]
         ],
         ['name' => 'Rhs', 'kind' => 'Expr']
      ]
   ],
   /**
    * relation_expr:
    *    expr T_IS_IDENTICAL expr
    * |  expr T_IS_NOT_IDENTICAL expr
    * |  expr T_IS_EQUAL expr
    * |  expr T_IS_NOT_EQUAL expr
    * |  expr '<' expr
    * |  expr T_IS_SMALLER_OR_EQUAL expr
    * |  expr '>' expr
    * |  expr T_IS_GREATER_OR_EQUAL expr
    * |  expr T_SPACESHIP expr
    */
   [
      'kind' => 'RelationExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'Lhs', 'kind' => 'Expr'],
         [
            'name' => 'RelationOperator',
            'kind' => 'Token',
            'tokenChoices' => [
               'IsIdenticalToken',
               'IsNotIdenticalToken',
               'IsEqualToken',
               'IsNotEqualToken',
               'LeftAngleToken',
               'IsSmallerOrEqualToken',
               'RightAngleToken',
               'IsGreaterOrEqualToken',
               'SpaceshipToken'
            ]
         ],
         ['name' => 'Rhs', 'kind' => 'Expr'],
      ]
   ],
   /**
    * cast_expr:
    *    T_INT_CAST expr
    * |  T_DOUBLE_CAST expr
    * |  T_STRING_CAST expr
    * |  T_ARRAY_CAST expr
    * |  T_OBJECT_CAST expr
    * |  T_BOOL_CAST expr
    * |  T_UNSET_CAST expr
    */
   [
      'kind' => 'CastExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'CastOperator',
            'kind' => 'Token',
            'tokenChoices' => [
               'IntCastKeyword',
               'DoubleCastKeyword',
               'StringCastKeyword',
               'ArrayCastKeyword',
               'ObjectCastKeyword',
               'BoolCastKeyword',
               'UnsetCastKeyword'
            ]
         ],
         ['name' => 'Value', 'kind' => 'Expr']
      ]
   ],
   /**
    * exit_expr_clause:
    *   '(' optional_expr ')'
    */
   [
      'kind' => 'ExitExprArgClause',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'LeftParen', 'kind' => 'LeftParenToken'],
         ['name' => 'Expr', 'kind' => 'Expr', 'isOptional' => true],
         ['name' => 'RightParen', 'kind' => 'RightParenToken'],
      ]
   ],
   /**
    * exit_expr:
    *   T_EXIT exit_expr
    */
   [
      'kind' => 'ExitExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'ExitKeyword', 'kind' => 'ExitKeyword'],
         ['name' => 'ArgClause', 'kind' => 'ExitExprArgClause', 'isOptional' => true]
      ]
   ],
   /**
    * yield_expr:
    *    T_YIELD
    * |  T_YIELD expr
    * |  T_YIELD expr T_DOUBLE_ARROW expr
    */
   [
      'kind' => 'YieldExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'YieldKeyword', 'kind' => 'YieldKeyword'],
         ['name' => 'KeyExpr', 'kind' => 'Expr', 'isOptional' => true],
         ['name' => 'DoubleArrow', 'kind' => 'DoubleArrowToken', 'isOptional' => true],
         ['name' => 'ValueExpr', 'kind' => 'Expr', 'isOptional' => true]
      ]
   ],
   /**
    * yield_from_expr:
    *   T_YIELD_FROM expr
    */
   [
      'kind' => 'YieldFromExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'YieldFromKeyword', 'kind' => 'YieldFromKeyword'],
         ['name' => 'ValueExpr', 'kind' => 'Expr']
      ]
   ],
   /**
    * clone_expr:
    *   T_CLONE expr
    */
   [
      'kind' => 'CloneExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'Clone', 'kind' => 'CloneKeyword'],
         ['name' => 'ObjectExpr', 'kind' => 'Expr']
      ]
   ],
   /**
    * encaps_var_offset:
    *    T_IDENTIFIER_STRING
    * |  T_NUM_STRING
    * |  '-' T_NUM_STRING
    * |  T_VARIABLE
    */
   [
      'kind' => 'EncapsVariableOffset',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'MinusSign', 'kind' => 'MinusSignToken'],
         [
            'name' => 'Offset',
            'kind' => 'Token',
            'tokenChoices' => [
               'IdentifierStringToken',
               'NumStringToken',
               'VariableToken'
            ]
         ]
      ]
   ],
   /**
    * encaps_var:
    *   T_VARIABLE '[' encaps_var_offset ']'
    */
   [
      'kind' => 'EncapsArrayVar',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'VarKeyword', 'kind' => 'VariableToken'],
         ['name' => 'LeftSquareBracket', 'kind' => 'LeftSquareBracketToken'],
         ['name' => 'Offset', 'kind' => 'EncapsVariableOffset'],
         ['name' => 'RightSquareBracket', 'kind' => 'RightSquareBracketToken']
      ]
   ],
   /**
    * encaps_var:
    *   T_VARIABLE T_OBJECT_OPERATOR T_IDENTIFIER_STRING
    */
   [
      'kind' => 'EncapsObjProp',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'VarKeyword', 'kind' => 'VariableToken'],
         ['name' => 'ObjOperator', 'kind' => 'ObjectOperatorToken'],
         ['name' => 'Identifier', 'kind' => 'IdentifierStringToken']
      ]
   ],
   /**
    * encaps_var:
    *   T_DOLLAR_OPEN_CURLY_BRACES expr '}'
    */
   [
      'kind' => 'EncapsDollarCurlyExpr',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'DollarOpenCurly', 'kind' => 'DollarOpenCurlyBracesToken'],
         ['name' => 'ValueExpr', 'kind' => 'Expr'],
         ['name' => 'CloseCurly', 'kind' => 'RightBraceToken']
      ]
   ],
   /**
    * encaps_var:
    *   T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME '}'
    */
   [
      'kind' => 'EncapsDollarCurlyVariable',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'DollarOpenCurly', 'kind' => 'DollarOpenCurlyBracesToken'],
         ['name' => 'VarName', 'kind' => 'IdentifierStringToken'],
         ['name' => 'CloseCurly', 'kind' => 'RightBraceToken']
      ]
   ],
   /**
    * encaps_var:
    *   T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME '[' expr ']' '}'
    */
   [
      'kind' => 'EncapsDollarCurlyArray',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'DollarOpenCurly', 'kind' => 'DollarOpenCurlyBracesToken'],
         ['name' => 'VarName', 'kind' => 'IdentifierStringToken'],
         ['name' => 'LeftSquareBracket', 'kind' => 'LeftSquareBracketToken'],
         ['name' => 'Index', 'kind' => 'Expr'],
         ['name' => 'RightSquareBracket', 'kind' => 'RightSquareBracketToken'],
         ['name' => 'CloseCurly', 'kind' => 'RightBraceToken']
      ]
   ],
   /**
    * encaps_var:
    *   T_CURLY_OPEN variable '}'
    */
   [
      'kind' => 'EncapsCurlyVariable',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'CurlyOpen', 'kind' => 'CurlyOpenToken'],
         ['name' => 'Variable', 'kind' => 'VariableExpr'],
         ['name' => 'CloseCurly', 'kind' => 'RightBraceToken']
      ]
   ],
   /**
    * encaps_var:
    *    T_VARIABLE
    * |  T_VARIABLE '[' encaps_var_offset ']'
    * |  T_VARIABLE T_OBJECT_OPERATOR T_STRING
    * |  T_DOLLAR_OPEN_CURLY_BRACES expr '}'
    * |  T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME '}'
    * |  T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME '[' expr ']' '}'
    * |  T_CURLY_OPEN variable '}'
    */
   [
      'kind' => 'EncapsVariable',
      'baseKind' => 'Syntax',
      'children' => [
         [
            'name' => 'Var',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'Variable', 'kind' => 'VariableToken'],
               ['name' => 'EncapsArrayVar', 'kind' => 'EncapsArrayVar'],
               ['name' => 'EncapsObjPropVar', 'kind' => 'EncapsObjProp'],
               ['name' => 'EncapsDollarCurlyExprVar', 'kind' => 'EncapsDollarCurlyExpr'],
               ['name' => 'EncapsDollarCurlyVar', 'kind' => 'EncapsDollarCurlyVariable'],
               ['name' => 'EncapsDollarCurlyArrayVar', 'kind' => 'EncapsDollarCurlyArray'],
               ['name' => 'EncapsCurlyVariable', 'kind' => 'EncapsCurlyVariable']
            ]
         ]
      ]
   ],
   /**
    * encaps_list_item:
    *    encaps_var
    * |  T_ENCAPSED_AND_WHITESPACE
    * |  T_ENCAPSED_AND_WHITESPACE encaps_var
    */
   [
      'kind' => 'EncapsListItem',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'StrLiteral', 'kind' => 'EncapsedAndWhitespaceToken', 'isOptional' => true],
         ['name' => 'EncapsVariable', 'kind' => 'EncapsVariable', 'isOptional' => true]
      ]
   ],
   /**
    * backticks_expr:
    *   T_ENCAPSED_AND_WHITESPACE
    * | encaps_list
    */
   [
      'kind' => 'BackticksClause',
      'baseKind' => 'Syntax',
      'children' => [
         [
            'name' => 'Backticks',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'BacktickStr', 'kind' => 'EncapsedAndWhitespaceToken'],
               ['name' => 'EncapsItems', 'kind' => 'EncapsItemList']
            ]
         ]
      ]
   ],
   /**
    * heredoc_expr:
    *    T_START_HEREDOC T_ENCAPSED_AND_WHITESPACE T_END_HEREDOC
    * |  T_START_HEREDOC encaps_list T_END_HEREDOC
    * |  T_START_HEREDOC T_END_HEREDOC
    */
   [
      'kind' => 'HeredocExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'StartHeredoc', 'kind' => 'StartHeredocToken'],
         [
            'name' => 'Text',
            'kind' => 'Syntax',
            'children' => [
               ['name' => 'EncapsedStr', 'kind' => 'EncapsedAndWhitespaceToken'],
               ['name' => 'EncapsItems', 'kind' => 'EncapsItemList'],
            ],
            'isOptional' => true
         ],
         ['name' => 'EndHeredoc', 'kind' => 'EndHereDocToken']
      ]
   ],
   /**
    * encaps_list_str:
    *   '"' encaps_list '"'
    */
   [
      'kind' => 'EncapsListStringExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'LeftQuote', 'kind' => 'DoubleStrQuoteToken'],
         ['name' => 'EncapsItems', 'kind' => 'EncapsItemList', 'collectionElementName' => 'EncapsItem'],
         ['name' => 'RightQuote', 'kind' => 'DoubleStrQuoteToken']
      ]
   ],
   /**
    * ternary_expr:
    *    expr '?' expr ':' expr
    * |  expr '?' ':' expr
    */
   [
      'kind' => 'TernaryExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'Condition', 'kind' => 'Expr'],
         ['name' => 'QuestionMark', 'kind' => 'QuestionMarkToken'],
         ['name' => 'FirstChoice', 'kind' => 'Expr', 'isOptional' => true],
         ['name' => 'ColonMark', 'kind' => 'ColonToken'],
         ['name' => 'SecondChoice', 'kind' => 'Expr']
      ]
   ],
   /**
    * sequence_expr:
    *    expr_list
    */
   [
      'kind' => 'SequenceExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'Elements', 'kind' => 'ExprList', 'collectionElementName' => 'Element']
      ]
   ],
   /**
    * prefix_operator_expr:
    *    '+' expr %prec T_INC
    * |  '-' expr %prec T_INC
    * |  '!' expr
    * |  '~' expr
    * |  '@' expr
    * |  T_INC variable
    * |  T_DEC variable
    */
   [
      'kind' => 'PrefixOperatorExpr',
      'baseKind' => 'Expr',
      'children' => [
         [
            'name' => 'PrefixOperator',
            'kind' => 'Token',
            'tokenChoices' => [
               'PlusSignToken',
               'MinusSignToken',
               'ExclamationMarkToken',
               'TildeToken',
               'ErrorSuppressSignToken',
               'IncToken',
               'DecToken'
            ]
         ],
         ['name' => 'ValueRef', 'kind' => 'Expr']
      ]
   ],
   /**
    * postfix_operator_expr:
    *    variable T_INC
    * |  variable T_DEC
    */
   [
      'kind' => 'PostfixOperatorExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'ValueExpr', 'kind' => 'Expr'],
         [
            'name' => 'PostfixOperator',
            'kind' => 'Token',
            'tokenChoices' => [
               'IncToken',
               'DecToken',
            ]
         ]
      ]
   ],
   /**
    * binary_expr:
    *    expr '.' expr
    * |	expr '+' expr
    * |  expr '-' expr
    * |  expr '*' expr
    * |  expr T_POW expr
    * |  expr '/' expr
    * |  expr '%' expr
    * |  expr T_SL expr
    * |  expr T_SR expr
    * |  expr T_COALESCE expr
    */
   [
      'kind' => 'BinaryOperatorExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'Lhs', 'kind' => 'Expr'],
         [
            'name' => 'BinaryOperator',
            'kind' => 'Token',
            'tokenChoices' => [
               'StrConcatToken',
               'PlusSignToken',
               'MinusSignToken',
               'MulSignToken',
               'DivSignToken',
               'PowToken',
               'ModSignToken',
               'ShiftLeftToken',
               'ShiftRightToken',
               'CoalesceToken'
            ]
         ],
         ['name' => 'Rhs', 'kind' => 'Expr']
      ]
   ],
   /**
    * instanceof_expr:
    *   expr T_INSTANCEOF class_name_reference
    */
   [
      'kind' => 'InstanceofExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'Instance', 'kind' => 'Expr'],
         ['name' => 'InstanceofKeyword', 'kind' => 'InstanceofKeyword'],
         ['name' => 'ClassNameRef', 'kind' => 'ClassNameRefClause']
      ]
   ],
   /**
    * shell_cmd_expr:
    *   '`' backticks_expr '`'
    */
   [
      'kind' => 'ShellCmdExpr',
      'baseKind' => 'Expr',
      'children' => [
         ['name' => 'LeftBacktick', 'kind' => 'BacktickToken'],
         ['name' => 'Expr', 'kind' => 'BackticksClause', 'isOptional' => true],
         ['name' => 'RightBacktick', 'kind' => 'BacktickToken']
      ]
   ],
   /**
    * lexical_vars:
    *    %empty
    * | T_USE '(' lexical_var_list ')'
    */
   [
      'kind' => 'UseLexicalVariableClause',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'UseKeyword', 'kind' => 'UseKeyword'],
         ['name' => 'LeftParen', 'kind' => 'LeftParenToken'],
         ['name' => 'LexicalVars', 'kind' => 'LexicalVariableList', 'collectionElementName' => 'LexicalVar'],
         ['name' => 'RightParen', 'kind' => 'LeftParenToken']
      ]
   ],
   /**
    * lexical_var:
    *   T_VARIABLE
    * | '&' T_VARIABLE
    */
   [
      'kind' => 'LexicalVariable',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'ReferenceSign', 'kind' => 'AmpersandToken', 'isOptional' => true],
         ['name' => 'Variable', 'kind' => 'VariableToken']
      ]
   ],
   /**
    * lexical_variable_list_item:
    *   lexical_variable
    * | ',' lexical_variable
    */
   [
      'kind' => 'LexicalVariableListItem',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Comma', 'kind' => 'CommaToken', 'isOptional' => true],
         ['name' => 'LexicalVariable', 'kind' => 'LexicalVariable']
      ]
   ]
);

/**
 * collection syntax node definitions
 */
$definitions = array_merge($definitions, process_collection_items([
   ['kind' => 'ExprList', 'elementKind' => 'ExprListItem'],
   /**
    * lexical_var_list:
    *    lexical_var_list ',' lexical_var
    * |  lexical_var
    */
   ['kind' => 'LexicalVariableList', 'elementKind' => 'LexicalVariableListItem'],
   /**
    * array_pair_item_list:
    *    array_pair_item_list ',' array_pair_item
    * |  array_pair_item
    */
   ['kind' => 'ArrayPairList', 'elementKind' => 'ArrayPairListItem'],
   /**
    * encaps_list:
    *    encaps_list encaps_var
    * |  encaps_list T_ENCAPSED_AND_WHITESPACE
    * |  encaps_var
    * |  T_ENCAPSED_AND_WHITESPACE encaps_var
    */
   ['kind' => 'EncapsItemList', 'elementKind' => 'EncapsListItem'],
   /**
    * non_empty_argument_list:
    *    argument
    * |  non_empty_argument_list ',' argument
    */
   ['kind' => 'ArgumentList', 'elementKind' => 'ArgumentListItem'],
   /**
    * isset_variables:
    *    isset_variable
    * |  isset_variables ',' isset_variable
    */
   ['kind' => 'IssetVariablesList', 'elementKind' => 'IssetVariableListItem']
]));

return $definitions;