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
    * empty_stmt:
    *   ';'
    */
   [
      'kind' => 'EmptyStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken']
      ]
   ],
   /**
    * statement:
    *   '{' inner_statement_list '}'
    */
   [
      'kind' => 'NestStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'LeftBrace', 'kind' => 'LeftBraceToken'],
         ['name' => 'Statements', 'kind' => 'InnerStmtList', 'collectionElementName' => 'Statement'],
         ['name' => 'RightBrace', 'kind' => 'RightBraceToken']
      ]
   ],
   /**
    * expr_stmt:
    *   expr ';'
    */
   [
      'kind' => 'ExprStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'Expr', 'kind' => 'Expr'],
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken']
      ]
   ],
   /**
    * inner_statement:
    *   statement
    * | function_declaration_statement
    * | class_declaration_statement
    * | trait_declaration_statement
    * | interface_declaration_statement
    * | T_HALT_COMPILER '(' ')' ';'
    */
   [
      'kind' => 'InnerStmt',
      'baseKind' => 'Stmt',
      'omitWhenEmpty' => 'true',
      'children' => [
         [
            'name' => 'InnerStmt',
            'kind' => 'Stmt',
            'nodeChoices' => [
               ['name' => 'NormalStmt', 'kind' => 'Stmt'],
               ['name' => 'ClassDefStmt', 'kind' => 'ClassDefinitionStmt'],
               ['name' => 'InterfaceDefStmt', 'kind' => 'InterfaceDefinitionStmt'],
               ['name' => 'TraitDefStmt', 'kind' => 'TraitDefinitionStmt'],
               ['name' => 'FunctionDefStmt', 'kind' => 'FunctionDefinitionStmt']
            ]
         ]
      ]
   ],
   /**
    * inner_code_block_stmt:
    *   '{' inner_statement_list '}'
    */
   [
      'kind' => 'InnerCodeBlockStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'LeftBrace', 'kind' => 'LeftBraceToken'],
         ['name' => 'Statements', 'kind' => 'InnerStmtList', 'collectionElementName' => 'Statement'],
         ['name' => 'RightBrace', 'kind' => 'RightBraceToken']
      ]
   ],
   /**
    * top_statement:
    *    statement
    * |  function_declaration_statement
    * |  class_declaration_statement
    * |	trait_declaration_statement
    * |  interface_declaration_statement
    * |  T_HALT_COMPILER '(' ')' ';'
    * |  T_NAMESPACE namespace_name ';'
    * |  T_NAMESPACE namespace_name '{' top_statement_list '}'
    * |  T_NAMESPACE '{' top_statement_list '}'
    * |  T_USE mixed_group_use_declaration ';'
    * |  T_USE use_type group_use_declaration ';'
    * |  T_USE use_declarations ';'
    * |  T_USE use_type use_declarations ';'
    * |  T_CONST const_list ';'
    */
   [
      'kind' => 'TopStmt',
      'baseKind' => 'Stmt',
      'omitWhenEmpty' => true,
      'children' => [
         ['name' => 'TopStmt', 'kind' => 'Stmt']
      ]
   ],
   /**
    * top_code_block_stmt:
    *   '{' top_statement_list '}'
    */
   [
      'kind' => 'TopCodeBlockStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'LeftBrace', 'kind' => 'LeftBraceToken'],
         ['name' => 'Statements', 'kind' => 'TopStmtList', 'collectionElementName' => 'Statement'],
         ['name' => 'RightBrace', 'kind' => 'RightBraceToken'],
      ]
   ],
   /**
    * declare_stmt:
    *   T_DECLARE '(' const_list ')' declare_statement
    */
   [
      'kind' => 'DeclareStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'DeclareKeyword', 'kind' => 'DeclareKeyword'],
         ['name' => 'LeftParen', 'kind' => 'LeftParenToken'],
         ['name' => 'ConstDefs', 'kind' => 'ConstDeclareList', 'collectionElementName' => 'ConstDef'],
         ['name' => 'RightParen', 'kind' => 'RightParenToken'],
         ['name' => 'Stmt', 'kind' => 'Stmt']
      ]
   ],
   /**
    * goto_stmt:
    *   T_GOTO T_IDENTIFIER_STRING ';'
    */
   [
      'kind' => 'GotoStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'GotoKeyword', 'kind' => 'GotoKeyword'],
         ['name' => 'Target', 'kind' => 'IdentifierStringToken'],
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken']
      ]
   ],
   /**
    * unset_variable:
    *   variable
    */
   [
      'kind' => 'UnsetVariable',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Variable', 'kind' => 'VariableExpr']
      ]
   ],
   /**
    * unset_variable_list_item:
    *   ',' variable
    */
   [
      'kind' => 'UnsetVariableListItem',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Comma', 'kind' => 'CommaToken', 'isOptional' => true],
         ['name' => 'Variable', 'kind' => 'UnsetVariable']
      ]
   ],
   /**
    * unset_stmt:
    *   T_UNSET '(' unset_variables possible_comma ')' ';'
    */
   [
      'kind' => 'UnsetStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'UnsetKeyword', 'kind' => 'UnsetKeyword'],
         ['name' => 'LeftParen', 'kind' => 'LeftParenKeyword'],
         ['name' => 'UnsetVariables', 'kind' => 'UnsetVariableList', 'collectionElementName' => 'UnsetVariable'],
         ['name' => 'RightParen', 'kind' => 'LeftParenKeyword'],
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken']
      ]
   ],
   /**
    * label_stmt:
    *   T_IDENTIFIER_STRING ':'
    */
   [
      'kind' => 'LabelStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'Name', 'kind' => 'IdentifierStringToken'],
         ['name' => 'Colon', 'kind' => 'ColonKeyword']
      ]
   ],
   /**
    *  condition -> expression
    */
   [
      'kind' => 'ConditionElement',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Condition', 'kind' => 'Expr'],
         ['name' => 'TrailingComma', 'kind' => 'TrailingCommaToken']
      ]
   ],
   /**
    * continue_stmt:
    *   T_CONTINUE optional_expr ';'
    */
   [
      'kind' => 'ContinueStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'ContinueKeyword', 'kind' => 'ContinueKeyword'],
         ['name' => 'OptionalExpr', 'kind' => 'Expr', 'isOptional' => true],
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken']
      ]
   ],
   /**
    * break_stmt:
    *   T_BREAK optional_expr ';'
    */
   [
      'kind' => 'BreakStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'BreakKeyword', 'kind' => 'BreakKeyword'],
         ['name' => 'OptionalExpr', 'kind' => 'Expr', 'isOptional' => true],
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken']
      ]
   ],
   /**
    * fallthrough_stmt:
    *    fallthrough
    */
   [
      'kind' => 'FallthroughStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'FallthroughKeyword', 'kind' => 'FallthroughKeyword'],
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken']
      ]
   ],
   /**
    * if_stmt_without_else:
    *    T_IF '(' expr ')' statement
    * |  if_stmt_without_else T_ELSEIF '(' expr ')' statement
    */
   [
      'kind' => 'ElseIfClause',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'ElseIfKeyword', 'kind' => 'ElseIfKeyword'],
         ['name' => 'LeftParen', 'kind' => 'LeftParenToken'],
         ['name' => 'Condition', 'kind' => 'Expr'],
         ['name' => 'RightParen', 'kind' => 'RightParenToken'],
         ['name' => 'Body', 'kind' => 'Stmt']
      ]
   ],
   /**
    * if_stmt:
    *    if_stmt_without_else %prec T_NOELSE
    * |  if_stmt_without_else T_ELSE statement
    */
   [
      'kind' => 'IfStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'LabelName', 'kind' => 'IdentifierStringToken', 'isOptional' => true],
         ['name' => 'LabelColon', 'kind' => 'ColonToken', 'isOptional' => true],
         ['name' => 'IfKeyword', 'kind' => 'IfKeyword'],
         ['name' => 'LeftParen', 'kind' => 'LeftParenToken'],
         ['name' => 'Condition', 'kind' => 'Expr'],
         ['name' => 'RightParen', 'kind' => 'RightParenToken'],
         ['name' => 'Body', 'kind' => 'Stmt'],
         [
            'name' => 'ElseIfClauses',
            'kind' => 'ElseIfList',
            'collectionElementName' => 'ElseIfClause',
            'isOptional' => true
         ],
         ['name' => 'ElseKeyword', 'kind' => 'ElseKeyword', 'isOptional' => true],
         ['name' => 'ElseBody', 'kind' => 'Stmt', 'isOptional' => true]
      ]
   ],
   /**
    * while_stmt:
    *   T_WHILE '(' expr ')' while_statement
    */
   [
      'kind' => 'WhileStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'LabelName', 'kind' => 'IdentifierStringToken', 'isOptional' => true],
         ['name' => 'LabelColon', 'kind' => 'ColonToken', 'isOptional' => true],
         ['name' => 'WhileKeyword', 'kind' => 'WhileKeyword'],
         ['name' => 'ConditionsClause', 'kind' => 'ParenDecoratedExpr'],
         ['name' => 'Body', 'kind' => 'Stmt']
      ]
   ],
   /**
    * do_while_stmt:
    *   T_DO statement T_WHILE T_LEFT_PAREN expr T_RIGHT_PAREN T_SEMICOLON
    */
   [
      'kind' => 'DoWhileStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'LabelName', 'kind' => 'IdentifierStringToken', 'isOptional' => true],
         ['name' => 'LabelColon', 'kind' => 'ColonToken', 'isOptional' => true],
         ['name' => 'DoKeyword', 'kind' => 'DoKeyword'],
         ['name' => 'Body', 'kind' => 'Stmt'],
         ['name' => 'WhileKeyword', 'kind' => 'WhileKeyword'],
         ['name' => 'ConditionsClause', 'kind' => 'ParenDecoratedExpr'],
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken']
      ]
   ],

   /**
    * for_stmt:
    *   T_FOR '(' for_exprs ';' for_exprs ';' for_exprs ')' for_statement
    */
   [
      'kind' => 'ForStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'ForKeyword', 'kind' => 'ForKeyword'],
         ['name' => 'LeftParen', 'kind' => 'LeftParenToken'],
         ['name' => 'InitializedExprs', 'kind' => 'ExprList', 'collectionElementName' => 'InitializedExpr', 'isOptional' => true],
         ['name' => 'InitializedSemicolon', 'kind' => 'SemicolonToken'],
         ['name' => 'ConditionalExprs', 'kind' => 'ExprList', 'collectionElementName' => 'ConditionalExpr', 'isOptional' => true],
         ['name' => 'ConditionalSemicolon', 'kind' => 'SemicolonToken'],
         ['name' => 'OperationalExprs', 'kind' => 'ExprList', 'collectionElementName' => 'OperationalExpr', 'isOptional' => true],
         ['name' => 'RightParen', 'kind' => 'RightParenToken'],
         ['name' => 'Stmt', 'kind' => 'Stmt']
      ]
   ],
   /**
    * foreach_variable:
    *   variable
    * |	'&' variable
    * |	T_LIST '(' array_pair_list ')'
    * |	'[' array_pair_list ']'
    */
   [
      'kind' => 'ForeachVariable',
      'baseKind' => 'Syntax',
      'children' => [
         [
            'name' => 'Variable',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'Variable', 'kind' => 'VariableExpr'],
               ['name' => 'ReferencedVariable', 'kind' => 'ReferencedVariableExpr'],
               ['name' => 'ListStructureClause', 'kind' => 'ListStructureClause'],
               ['name' => 'SimplifiedArray', 'kind' => 'SimplifiedArrayCreateExpr']
            ]
         ]
      ]
   ],
   /**
    * foreach_stmt:
    *    T_FOREACH '(' expr T_AS foreach_variable ')' foreach_statement
    * |  T_FOREACH '(' expr T_AS foreach_variable T_DOUBLE_ARROW foreach_variable ')' foreach_statement
    */
   [
      'kind' => 'ForeachStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'Foreach', 'kind' => 'ForeachKeyword'],
         ['name' => 'LeftParen', 'kind' => 'LeftParenToken'],
         ['name' => 'IterableExpr', 'kind' => 'Expr'],
         ['name' => 'AsKeyword', 'kind' => 'AsKeyword'],
         ['name' => 'KeyVariable', 'kind' => 'ForeachVariable', 'isOptional' => true],
         ['name' => 'DoubleArrow', 'kind' => 'DoubleArrowToken', 'isOptional' => true],
         ['name' => 'ValueVariable', 'kind' => 'ForeachVariable'],
         ['name' => 'RightParen', 'kind' => 'RightParenToken'],
         ['name' => 'Stmt', 'kind' => 'Stmt']
      ]
   ],
   /**
    * switch-default-label -> 'default' ':'
    */
   [
      'kind' => 'SwitchDefaultLabel',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'DefaultKeyword', 'kind' => 'DefaultKeyword'],
         ['name' => 'Colon', 'kind' => 'ColonToken']
      ]
   ],
   /**
    * switch-case-label -> 'case' case-item-list ':'
    */
   [
      'kind' => 'SwitchCaseLabel',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'CaseKeyword', 'kind' => 'CaseKeyword'],
         ['name' => 'CondExpr', 'kind' => 'Expr'],
         ['name' => 'Colon', 'kind' => 'ColonToken']
      ]
   ],
   /**
    * switch_case_list_clause:
    * '{' case_list '}'
    */
   [
      'kind' => 'SwitchCaseListClause',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'LeftBrace', 'kind' => 'LeftBraceToken'],
         ['name' => 'SwitchCases', 'kind' => 'SwitchCaseList', 'collectionElementName' => 'SwitchCase'],
         ['name' => 'RightBrace', 'kind' =>  'RightBraceToken']
      ]
   ],
   /**
    * switch-case:
    *    switch-case-label stmt-list
    * |  switch-default-label stmt-list
    */
   [
      'kind' => 'SwitchCase',
      'baseKind' => 'Syntax',
      'children' => [
         [
            'name' => 'Label',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'DefaultLabel', 'kind' => 'SwitchDefaultLabel'],
               ['name' => 'CaseLabel', 'kind' => 'SwitchCaseLabel']
            ]
         ],
         ['name' => 'Statements', 'kind' => 'InnerStmtList', 'collectionElementName' => 'Stmt']
      ]
   ],
   /**
    * switch-stmt:
    *    identifier? ':'? 'switch' '(' expr ')' '{'switch-case-list '}'
    */
   [
      'kind' => 'SwitchStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'LabelName', 'kind' => 'IdentifierStringToken', 'isOptional' => true],
         ['name' => 'LabelColon', 'kind' => 'ColonToken', 'isOptional' => true],
         ['name' => 'SwitchKeyword', 'kind' => 'SwitchKeyword'],
         ['name' => 'LeftParen', 'kind' => 'LeftParenToken'],
         ['name' => 'Condition', 'kind' => 'Expr'],
         ['name' => 'RightParen', 'kind' => 'RightParenToken'],
         ['name' => 'Cases', 'kind' => 'SwitchCaseListClause']
      ]
   ],
   /**
    * defer-stmt -> 'defer' code-block
    */
   [
      'kind' => 'DeferStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'Defer', 'kind' => 'DeferKeyword'],
         ['name' => 'Body', 'kind' => 'InnerCodeBlockStmt']
      ]
   ],
   /**
    * throw_stmt:
    *   T_THROW expr ';'
    */
   [
      'kind' => 'ThrowStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'ThrowKeyword', 'kind' => 'ThrowKeyword'],
         ['name' => 'Expr', 'kind' => 'Expr'],
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken']
      ]
   ],
   /**
    * try_stmt:
    *   T_TRY '{' inner_statement_list '}' catch_list finally_statement
    */
   [
      'kind' => 'TryStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'TryKeyword', 'kind' => 'TryKeyword'],
         ['name' => 'CodeBlock', 'kind' => 'InnerCodeBlockStmt'],
         ['name' => 'Catches', 'kind' => 'CatchList', 'collectionElementName' => 'CatcheItem'],
         ['name' => 'FinallyClause', 'kind' => 'FinallyClause', 'isOptional' => true]
      ]
   ],
   /**
    * finally_statement:
    *   T_FINALLY '{' inner_statement_list '}'
    */
   [
      'kind' => 'FinallyClause',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Finally', 'kind' => 'FinallyKeyword'],
         ['name' => 'CodeBlock', 'kind' => 'InnerCodeBlockStmt']
      ]
   ],
   /**
    * catch_arg_type_hint_item:
    *   '|' name
    */
   [
      'kind' => 'CatchArgTypeHintItem',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Separator', 'kind' => 'VerticalBarToken', 'isOptional' => true],
         ['name' => 'TypeName', 'kind' => 'Name']
      ]
   ],
   /**
    * catch_list:
    *   catch_list T_CATCH '(' catch_name_list T_VARIABLE ')' '{' inner_statement_list '}'
    */
   [
      'kind' => 'CatchListItemClause',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'CatchKeyword', 'kind' => 'CatchKeyword'],
         ['name' => 'LeftParen', 'kind' => 'LeftParenToken'],
         ['name' => 'CatchArgTypeHints', 'kind' => 'CatchArgTypeHintList', 'collectionElementName' => 'CatchArgTypeHit'],
         ['name' => 'Variable', 'kind' => 'VariableToken'],
         ['name' => 'RightParen', 'kind' => 'RightParenToken'],
         ['name' => 'CodeBlock', 'kind' => 'InnerCodeBlockStmt']
      ]
   ],
   /**
    * return_stmt:
    *   T_RETURN optional_expr ';'
    */
   [
      'kind' => 'ReturnStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'ReturnKeyword', 'kind' => 'ReturnKeyword'],
         ['name' => 'ValueExpr', 'kind' => 'Expr', 'isOptional' => true],
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken']
      ]
   ],
   /**
    * echo_stmt:
    *   T_ECHO echo_expr_list ';'
    */
   [
      'kind' => 'EchoStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'Echo', 'kind' => 'EchoKeyword'],
         ['name' => 'Exprs', 'kind' => 'ExprList', 'collectionElementName' => 'Expr'],
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken']
      ]
   ],
   /**
    * halt_compiler_stmt:
    *   T_HALT_COMPILER '(' ')' ';'
    */
   [
      'kind' => 'HaltCompilerStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'HaltCompiler', 'kind' => 'HaltCompilerKeyword'],
         ['name' => 'LeftParen', 'kind' => 'LeftParenToken'],
         ['name' => 'RightParen', 'kind' => 'RightParenToken'],
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken'],
      ]
   ],
   /**
    * global_var:
    *   simple_variable
    */
   [
      'kind' => 'GlobalVariable',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Variable', 'kind' => 'SimpleVariableExpr']
      ]
   ],
   /**
    * global_var:
    *   simple_variable ','
    */
   [
      'kind' => 'GlobalVariableListItem',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Comma', 'kind' => 'CommaToken', 'isOptional' => true],
         ['name' => 'Variable', 'kind' => 'GlobalVariable']
      ]
   ],
   /**
    * global_variable_declarations_stmt:
    *   T_GLOBAL global_var_list ';'
    */
   [
      'kind' => 'GlobalVariableDeclarationsStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'Global', 'kind' => 'GlobalKeyword'],
         ['name' => 'Variables', 'kind' => 'GlobalVariableList', 'collectionElementName' => 'Variable'],
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken']
      ]
   ],
   /**
    * static_var:
    *   T_VARIABLE
    * | T_VARIABLE '=' expr
    */
   [
      'kind' => 'StaticVariableDeclare',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Variable', 'kind' => 'VariableToken'],
         ['name' => 'Equal', 'kind' => 'EqualToken', 'isOptional' => true],
         ['name' => 'ValueExpr', 'kind' => 'Expr', 'isOptional' => true]
      ]
   ],
   /**
    * static_variable_list_item:
    *   ',' static_var
    */
   [
      'kind' => 'StaticVariableListItem',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Comma', 'kind' => 'CommaToken', 'isOptional' => true],
         ['name' => 'Declaration', 'kind' => 'StaticVariableDeclare']
      ]
   ],
   /**
    * static_variable_declarations_stmt:
    *   T_STATIC static_var_list ';'
    */
   [
      'kind' => 'StaticVariableDeclarationsStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'StaticKeyword', 'kind' => 'StaticKeyword'],
         ['name' => 'Variables', 'kind' => 'StaticVariableList', 'collectionElementName' => 'Variable'],
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken']
      ]
   ],
   /**
    * use_type:
    *   T_FUNCTION
    * | T_CONST
    */
   [
      'kind' => 'NamespaceUseType',
      'baseKind' => 'Syntax',
      'children' => [
         [
            'name' => 'Type',
            'kind' => 'Token',
            'tokenChoices' => [
               'FunctionKeyword',
               'ConstKeyword'
            ]
         ]
      ]
   ],
   /**
    * unprefixed_use_declaration:
    *    namespace_name
    * |  namespace_name T_AS T_IDENTIFIER_STRING
    */
   [
      'kind' => 'NamespaceUnprefixedUseDeclaration',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'NamespaceName', 'kind' => 'NamespaceName'],
         ['name' => 'As', 'kind' => 'AsKeyword', 'isOptional' => true],
         ['name' => 'Identifier', 'kind' => 'IdentifierStringToken', 'isOptional' => true]
      ]
   ],
   /**
    * namespace_unprefixed_use_declaration_list_item:
    *    ',' unprefixed_use_declaration
    */
   [
      'kind' => 'NamespaceUnprefixedUseDeclarationListItem',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Comma', 'kind' => 'CommaToken', 'isOptional' => true],
         ['name' => 'Declaration', 'kind' => 'NamespaceUnprefixedUseDeclaration']
      ]
   ],
   /**
    * use_declaration:
    *    unprefixed_use_declaration
    * |  T_NS_SEPARATOR unprefixed_use_declaration
    */
   [
      'kind' => 'NamespaceUseDeclaration',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Separator', 'kind' => 'NamespaceSeparatorToken', 'isOptional' => true],
         ['name' => 'Declaration', 'kind' => 'NamespaceUnprefixedUseDeclaration']
      ]
   ],
   /**
    * namespace_use_declaration_list_item:
    *   ',' use_declaration
    */
   [
      'kind' => 'NamespaceUseDeclarationListItem',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Comma', 'kind' => 'CommaToken', 'isOptional' => true],
         ['name' => 'Declaration', 'kind' => 'NamespaceUseDeclaration']
      ]
   ],
   /**
    * inline_use_declaration:
    *    unprefixed_use_declaration
    * |  use_type unprefixed_use_declaration
    */
   [
      'kind' => 'NamespaceInlineUseDeclaration',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'UseType', 'kind' => 'NamespaceUseType', 'isOptional' => true],
         ['name' => 'Declaration', 'kind' => 'NamespaceUnprefixedUseDeclaration']
      ]
   ],
   /**
    * namespace_inline_use_declaration_list_item:
    *    ',' inline_use_declaration
    */
   [
      'kind' => 'NamespaceInlineUseDeclarationListItem',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Comma', 'kind' => 'CommaToken', 'isOptional' => true],
         ['name' => 'Declaration', 'kind' => 'NamespaceInlineUseDeclaration']
      ]
   ],
   /**
    * group_use_declaration:
    *    namespace_name T_NS_SEPARATOR '{' unprefixed_use_declarations possible_comma '}'
    * |  T_NS_SEPARATOR namespace_name T_NS_SEPARATOR '{' unprefixed_use_declarations possible_comma '}'
    */
   [
      'kind' => 'NamespaceGroupUseDeclaration',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'FirstNsSeparator', 'kind' => 'NamespaceSeparatorToken', 'isOptional' => true],
         ['name' => 'NamespaceName', 'kind' => 'NamespaceName'],
         ['name' => 'SecondNsSeparator', 'kind' => 'NamespaceSeparatorToken'],
         ['name' => 'LeftBrace', 'kind' => 'LeftBraceToken'],
         ['name' => 'Declarations', 'kind' => 'NamespaceUnprefixedUseDeclarationList', 'collectionElementName' => 'Declaration'],
         ['name' => 'Comma', 'kind' => 'CommaToken', 'isOptional' => true],
         ['name' => 'RightBrace', 'kind' => 'RightBraceToken']
      ]
   ],
   /**
    * mixed_group_use_declaration:
    *    namespace_name T_NS_SEPARATOR '{' inline_use_declarations possible_comma '}'
    * |  T_NS_SEPARATOR namespace_name T_NS_SEPARATOR '{' inline_use_declarations possible_comma '}'
    */
   [
      'kind' => 'NamespaceMixedGroupUseDeclaration',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'FirstNsSeparator', 'kind' => 'NamespaceSeparatorToken', 'isOptional' => true],
         ['name' => 'NamespaceName', 'kind' => 'NamespaceName'],
         ['name' => 'SecondNsSeparator', 'kind' => 'NamespaceSeparatorToken'],
         ['name' => 'LeftBrace', 'kind' => 'LeftBraceToken'],
         ['name' => 'Declarations', 'kind' => 'NamespaceInlineUseDeclarationList', 'collectionElementName' => 'Declaration'],
         ['name' => 'Comma', 'kind' => 'CommaToken', 'isOptional' => true],
         ['name' => 'RightBrace', 'kind' => 'RightBraceToken']
      ]
   ],
   /**
    * top_statement:
    *    T_USE mixed_group_use_declaration ';'
    * |  T_USE use_type group_use_declaration ';'
    * |  T_USE use_declarations ';'
    * |  T_USE use_type use_declarations ';'
    */
   [
      'kind' => 'NamespaceUseStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'Use', 'kind' => 'UseKeyword'],
         ['name' => 'UseType', 'kind' => 'NamespaceUseType', 'isOptional' => true],
         [
            'name' => 'Declarations',
            'kind' => 'Syntax',
            'nodeChoices' => [
               ['name' => 'MixedGroupUseDeclaration', 'kind' => 'NamespaceMixedGroupUseDeclaration'],
               ['name' => 'GroupUseDeclaration', 'kind' => 'NamespaceGroupUseDeclaration'],
               ['name' => 'UseDeclarationList', 'kind' => 'NamespaceUseDeclarationList']
            ]
         ],
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken']
      ]
   ],
   /**
    * namespace_definition_stmt:
    *   T_NAMESPACE namespace_name ';'
    */
   [
      'kind' => 'NamespaceDefinitionStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'NamespaceName', 'kind' => 'NamespaceKeyword'],
         ['name' => 'Name', 'kind' => 'NamespaceName'],
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken']
      ]
   ],
   /**
    * namespace_block_stmt:
    *    T_NAMESPACE namespace_name '{' top_statement_list '}'
    * |  T_NAMESPACE '{' top_statement_list '}'
    */
   [
      'kind' => 'NamespaceBlockStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'NamespaceName', 'kind' => 'NamespaceKeyword'],
         ['name' => 'Name', 'kind' => 'NamespaceName', 'isOptional' => true],
         ['name' => 'CodeBlock', 'kind' => 'TopCodeBlockStmt']
      ]
   ],
   /**
    * const_decl:
    *   T_IDENTIFIER_STRING '=' expr
    */
   [
      'kind' => 'ConstDeclare',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Name', 'kind' => 'IdentifierStringToken'],
         ['name' => 'Initializer', 'kind' => 'InitializerClause']
      ]
   ],
   /**
    * const_list_item:
    *   ',' const_decl
    */
   [
      'kind' => 'ConstListItem',
      'baseKind' => 'Syntax',
      'children' => [
         ['name' => 'Comma', 'kind' => 'CommaToken', 'isOptional' => true],
         ['name' => 'Declaration', 'kind' => 'ConstDeclare']
      ]
   ],
   /**
    * top_statement:
    *   T_CONST const_list ';'
    */
   [
      'kind' => 'ConstDefinitionStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'ConstKeyword', 'kind' => 'ConstKeyword'],
         ['name' => 'Declarations', 'kind' => 'ConstDeclareList', 'collectionElementName' => 'Declaration'],
         ['name' => 'Semicolon', 'kind' => 'SemicolonToken']
      ]
   ],
   /**
    * class_definition_stmt:
    *   class_definition_decl
    */
   [
      'kind' => 'ClassDefinitionStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'Definition', 'kind' => 'ClassDefinition']
      ]
   ],
   /**
    * interface_definition_stmt:
    *   interface_definition_decl
    */
   [
      'kind' => 'InterfaceDefinitionStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'Definition', 'kind' => 'InterfaceDefinition']
      ]
   ],
   /**
    * trait_definition_stmt:
    *   trait_definition_decl
    */
   [
      'kind' => 'TraitDefinitionStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'Definition', 'kind' => 'TraitDefinition']
      ]
   ],
   /**
    * function_definition_stmt:
    *   function_definition_decl
    */
   [
      'kind' => 'FunctionDefinitionStmt',
      'baseKind' => 'Stmt',
      'children' => [
         ['name' => 'Definition', 'kind' => 'FunctionDefinition']
      ]
   ]
);

$definitions = array_merge($definitions, process_collection_items([
   /**
    * condition-list:
    *    condition
    * |  condition-list, condition ','?
    */
   ['kind' => 'ConditionElementList', 'elementKind' => 'ConditionElement'],
   /**
    * switch-case-list:
    *    switch-case
    * |  switch-case-list switch-case
    */
   ['kind' => 'SwitchCaseList', 'elementKind' => 'SwitchCase'],
   /**
    * elseif-list:
    *    elseif-clause
    * |  elseif-list elseif-clause
    */
   ['kind' => 'ElseIfList', 'elementKind' => 'ElseIfClause'],
   /**
    * inner_statement_list:
    *   inner_statement_list inner_statement
    */
   ['kind' => 'InnerStmtList', 'elementKind' => 'InnerStmt'],
   /**
    * top_statement_list:
    *   top_statement_list top_statement
    */
   ['kind' => 'TopStmtList', 'elementKind' => 'TopStmt'],
   /**
    * catch_list:
    *   catch_list T_CATCH '(' catch_name_list T_VARIABLE ')' '{' inner_statement_list '}'
    */
   ['kind' => 'CatchList', 'elementKind' => 'CatchListItemClause'],
   /**
    * catch_name_list:
    *    name
    * |  catch_name_list '|' name
    */
   ['kind' => 'CatchArgTypeHintList', 'elementKind' => 'CatchArgTypeHintItem'],
   /**
    * unset_variable_list:
    *    unset_variables:
    *    unset_variable
    * |  unset_variables ',' unset_variable
    */
   ['kind' => 'UnsetVariableList', 'elementKind' => 'UnsetVariableListItem'],
   /**
    * static_var_list:
    *    static_var_list ',' static_var
    * |  static_var
    */
   ['kind' => 'GlobalVariableList', 'elementKind' => 'GlobalVariableListItem'],
   /**
    * static_var_list:
    *    static_var_list ',' static_var
    * |  static_var
    */
   ['kind' => 'StaticVariableList', 'elementKind' => 'StaticVariableListItem'],
   /**
    * use_declarations:
    *    use_declarations ',' use_declaration
    * |  use_declaration
    */
   ['kind' => 'NamespaceUseDeclarationList', 'elementKind' => 'NamespaceUseDeclarationListItem'],
   /**
    * inline_use_declarations:
    *    inline_use_declarations ',' inline_use_declaration
    * |  inline_use_declaration
    */
   ['kind' => 'NamespaceInlineUseDeclarationList', 'elementKind' => 'NamespaceInlineUseDeclarationListItem'],
   /**
    * unprefixed_use_declarations:
    *    unprefixed_use_declarations ',' unprefixed_use_declaration
    * |  unprefixed_use_declaration
    */
   ['kind' => 'NamespaceUnprefixedUseDeclarationList', 'elementKind' => 'NamespaceUnprefixedUseDeclarationListItem'],
   /**
    * const_list:
    *    const_list ',' const_decl
    * |  const_decl
    */
   ['kind' => 'ConstDeclareList', 'elementKind' => 'ConstListItem']
]));

return $definitions;