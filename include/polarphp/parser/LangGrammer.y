%{
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/09.

#include "polarphp/parser/Parser.h"
#include "polarphp/parser/Lexer.h"

#define YYERROR_VERBOSE
#define YYSTYPE polar::parser::ParserStackElement
#define polar_error polar::parser::parse_error

%}
%pure-parser
%expect 0

%code requires {
}

%destructor { delete $$; } <ast>
%destructor { if ($$) delete $$; } <str>

%precedence T_INCLUDE T_INCLUDE_ONCE T_REQUIRE T_REQUIRE_ONCE
%left T_LOGICAL_OR
%left T_LOGICAL_XOR
%left T_LOGICAL_AND
%precedence T_PRINT
%precedence T_YIELD
%precedence T_DOUBLE_ARROW
%precedence T_YIELD_FROM
%precedence '=' T_PLUS_EQUAL T_MINUS_EQUAL T_MUL_EQUAL T_DIV_EQUAL T_CONCAT_EQUAL T_MOD_EQUAL T_AND_EQUAL T_OR_EQUAL T_XOR_EQUAL T_SL_EQUAL T_SR_EQUAL T_POW_EQUAL T_COALESCE_EQUAL
%left '?' ':'
%right T_COALESCE
%left T_BOOLEAN_OR
%left T_BOOLEAN_AND
%left '|'
%left '^'
%left '&'
%nonassoc T_IS_EQUAL T_IS_NOT_EQUAL T_IS_IDENTICAL T_IS_NOT_IDENTICAL T_SPACESHIP
%nonassoc '<' T_IS_SMALLER_OR_EQUAL '>' T_IS_GREATER_OR_EQUAL
%left T_SL T_SR
%left '+' '-' '.'
%left '*' '/' '%'
%precedence '!'
%precedence T_INSTANCEOF
%precedence '~' T_INC T_DEC T_INT_CAST T_DOUBLE_CAST T_STRING_CAST T_ARRAY_CAST T_OBJECT_CAST T_BOOL_CAST T_UNSET_CAST '@'
%right T_POW
%precedence T_NEW T_CLONE

/* Resolve danging else conflict */
%precedence T_NOELSE
%precedence T_ELSEIF
%precedence T_ELSE

%token <ast> T_LNUMBER   "integer number (T_LNUMBER)"
%token <ast> T_DNUMBER   "floating-point number (T_DNUMBER)"
%token <ast> T_STRING    "identifier (T_STRING)"
%token <ast> T_VARIABLE  "variable (T_VARIABLE)"
%token <ast> T_INLINE_HTML
%token <ast> T_ENCAPSED_AND_WHITESPACE  "quoted-string and whitespace (T_ENCAPSED_AND_WHITESPACE)"
%token <ast> T_CONSTANT_ENCAPSED_STRING "quoted-string (T_CONSTANT_ENCAPSED_STRING)"
%token <ast> T_STRING_VARNAME "variable name (T_STRING_VARNAME)"
%token <ast> T_NUM_STRING "number (T_NUM_STRING)"

%token END 0 "end of file"
%token T_INCLUDE      "include (T_INCLUDE)"
%token T_INCLUDE_ONCE "include_once (T_INCLUDE_ONCE)"
%token T_EVAL         "eval (T_EVAL)"
%token T_REQUIRE      "require (T_REQUIRE)"
%token T_REQUIRE_ONCE "require_once (T_REQUIRE_ONCE)"
%token T_LOGICAL_OR   "or (T_LOGICAL_OR)"
%token T_LOGICAL_XOR  "xor (T_LOGICAL_XOR)"
%token T_LOGICAL_AND  "and (T_LOGICAL_AND)"
%token T_PRINT        "print (T_PRINT)"
%token T_YIELD        "yield (T_YIELD)"
%token T_YIELD_FROM   "yield from (T_YIELD_FROM)"
%token T_PLUS_EQUAL   "+= (T_PLUS_EQUAL)"
%token T_MINUS_EQUAL  "-= (T_MINUS_EQUAL)"
%token T_MUL_EQUAL    "*= (T_MUL_EQUAL)"
%token T_DIV_EQUAL    "/= (T_DIV_EQUAL)"
%token T_CONCAT_EQUAL ".= (T_CONCAT_EQUAL)"
%token T_MOD_EQUAL    "%= (T_MOD_EQUAL)"
%token T_AND_EQUAL    "&= (T_AND_EQUAL)"
%token T_OR_EQUAL     "|= (T_OR_EQUAL)"
%token T_XOR_EQUAL    "^= (T_XOR_EQUAL)"
%token T_SL_EQUAL     "<<= (T_SL_EQUAL)"
%token T_SR_EQUAL     ">>= (T_SR_EQUAL)"
%token T_COALESCE_EQUAL "??= (T_COALESCE_EQUAL)"
%token T_BOOLEAN_OR   "|| (T_BOOLEAN_OR)"
%token T_BOOLEAN_AND  "&& (T_BOOLEAN_AND)"
%token T_IS_EQUAL     "== (T_IS_EQUAL)"
%token T_IS_NOT_EQUAL "!= (T_IS_NOT_EQUAL)"
%token T_IS_IDENTICAL "=== (T_IS_IDENTICAL)"
%token T_IS_NOT_IDENTICAL "!== (T_IS_NOT_IDENTICAL)"
%token T_IS_SMALLER_OR_EQUAL "<= (T_IS_SMALLER_OR_EQUAL)"
%token T_IS_GREATER_OR_EQUAL ">= (T_IS_GREATER_OR_EQUAL)"
%token T_SPACESHIP "<=> (T_SPACESHIP)"
%token T_SL "<< (T_SL)"
%token T_SR ">> (T_SR)"
%token T_INSTANCEOF  "instanceof (T_INSTANCEOF)"
%token T_INC "++ (T_INC)"
%token T_DEC "-- (T_DEC)"
%token T_INT_CAST    "(int) (T_INT_CAST)"
%token T_DOUBLE_CAST "(double) (T_DOUBLE_CAST)"
%token T_STRING_CAST "(string) (T_STRING_CAST)"
%token T_ARRAY_CAST  "(array) (T_ARRAY_CAST)"
%token T_OBJECT_CAST "(object) (T_OBJECT_CAST)"
%token T_BOOL_CAST   "(bool) (T_BOOL_CAST)"
%token T_UNSET_CAST  "(unset) (T_UNSET_CAST)"
%token T_NEW       "new (T_NEW)"
%token T_CLONE     "clone (T_CLONE)"
%token T_EXIT      "exit (T_EXIT)"
%token T_IF        "if (T_IF)"
%token T_ELSEIF    "elseif (T_ELSEIF)"
%token T_ELSE      "else (T_ELSE)"
%token T_ENDIF     "endif (T_ENDIF)"
%token T_ECHO       "echo (T_ECHO)"
%token T_DO         "do (T_DO)"
%token T_WHILE      "while (T_WHILE)"
%token T_ENDWHILE   "endwhile (T_ENDWHILE)"
%token T_FOR        "for (T_FOR)"
%token T_ENDFOR     "endfor (T_ENDFOR)"
%token T_FOREACH    "foreach (T_FOREACH)"
%token T_ENDFOREACH "endforeach (T_ENDFOREACH)"
%token T_DECLARE    "declare (T_DECLARE)"
%token T_ENDDECLARE "enddeclare (T_ENDDECLARE)"
%token T_AS         "as (T_AS)"
%token T_SWITCH     "switch (T_SWITCH)"
%token T_ENDSWITCH  "endswitch (T_ENDSWITCH)"
%token T_CASE       "case (T_CASE)"
%token T_DEFAULT    "default (T_DEFAULT)"
%token T_BREAK      "break (T_BREAK)"
%token T_CONTINUE   "continue (T_CONTINUE)"
%token T_GOTO       "goto (T_GOTO)"
%token T_FUNCTION   "function (T_FUNCTION)"
%token T_CONST      "const (T_CONST)"
%token T_RETURN     "return (T_RETURN)"
%token T_TRY        "try (T_TRY)"
%token T_CATCH      "catch (T_CATCH)"
%token T_FINALLY    "finally (T_FINALLY)"
%token T_THROW      "throw (T_THROW)"
%token T_USE        "use (T_USE)"
%token T_INSTEADOF  "insteadof (T_INSTEADOF)"
%token T_GLOBAL     "global (T_GLOBAL)"
%token T_STATIC     "static (T_STATIC)"
%token T_ABSTRACT   "abstract (T_ABSTRACT)"
%token T_FINAL      "final (T_FINAL)"
%token T_PRIVATE    "private (T_PRIVATE)"
%token T_PROTECTED  "protected (T_PROTECTED)"
%token T_PUBLIC     "public (T_PUBLIC)"
%token T_VAR        "var (T_VAR)"
%token T_UNSET      "unset (T_UNSET)"
%token T_ISSET      "isset (T_ISSET)"
%token T_EMPTY      "empty (T_EMPTY)"
%token T_HALT_COMPILER "__halt_compiler (T_HALT_COMPILER)"
%token T_CLASS      "class (T_CLASS)"
%token T_TRAIT      "trait (T_TRAIT)"
%token T_INTERFACE  "interface (T_INTERFACE)"
%token T_EXTENDS    "extends (T_EXTENDS)"
%token T_IMPLEMENTS "implements (T_IMPLEMENTS)"
%token T_OBJECT_OPERATOR "-> (T_OBJECT_OPERATOR)"
%token T_DOUBLE_ARROW    "=> (T_DOUBLE_ARROW)"
%token T_LIST            "list (T_LIST)"
%token T_ARRAY           "array (T_ARRAY)"
%token T_CALLABLE        "callable (T_CALLABLE)"
%token T_LINE            "__LINE__ (T_LINE)"
%token T_FILE            "__FILE__ (T_FILE)"
%token T_DIR             "__DIR__ (T_DIR)"
%token T_CLASS_C         "__CLASS__ (T_CLASS_C)"
%token T_TRAIT_C         "__TRAIT__ (T_TRAIT_C)"
%token T_METHOD_C        "__METHOD__ (T_METHOD_C)"
%token T_FUNC_C          "__FUNCTION__ (T_FUNC_C)"
%token T_COMMENT         "comment (T_COMMENT)"
%token T_DOC_COMMENT     "doc comment (T_DOC_COMMENT)"
%token T_OPEN_TAG        "open tag (T_OPEN_TAG)"
%token T_OPEN_TAG_WITH_ECHO "open tag with echo (T_OPEN_TAG_WITH_ECHO)"
%token T_CLOSE_TAG       "close tag (T_CLOSE_TAG)"
%token T_WHITESPACE      "whitespace (T_WHITESPACE)"
%token T_START_HEREDOC   "heredoc start (T_START_HEREDOC)"
%token T_END_HEREDOC     "heredoc end (T_END_HEREDOC)"
%token T_DOLLAR_OPEN_CURLY_BRACES "${ (T_DOLLAR_OPEN_CURLY_BRACES)"
%token T_CURLY_OPEN      "{$ (T_CURLY_OPEN)"
%token T_PAAMAYIM_NEKUDOTAYIM ":: (T_PAAMAYIM_NEKUDOTAYIM)"
%token T_NAMESPACE       "namespace (T_NAMESPACE)"
%token T_NS_C            "__NAMESPACE__ (T_NS_C)"
%token T_NS_SEPARATOR    "\\ (T_NS_SEPARATOR)"
%token T_ELLIPSIS        "... (T_ELLIPSIS)"
%token T_COALESCE        "?? (T_COALESCE)"
%token T_POW             "** (T_POW)"
%token T_POW_EQUAL       "**= (T_POW_EQUAL)"

/* Token used to force a parse error from the lexer */
%token T_ERROR

%type <ast> top_statement namespace_name name statement function_declaration_statement
%type <ast> class_declaration_statement trait_declaration_statement
%type <ast> interface_declaration_statement interface_extends_list
%type <ast> group_use_declaration inline_use_declarations inline_use_declaration
%type <ast> mixed_group_use_declaration use_declaration unprefixed_use_declaration
%type <ast> unprefixed_use_declarations const_decl inner_statement
%type <ast> expr optional_expr while_statement for_statement foreach_variable
%type <ast> foreach_statement declare_statement finally_statement unset_variable variable
%type <ast> extends_from parameter optional_type argument global_var
%type <ast> static_var class_statement trait_adaptation trait_precedence trait_alias
%type <ast> absolute_trait_method_reference trait_method_reference property echo_expr
%type <ast> new_expr anonymous_class class_name class_name_reference simple_variable
%type <ast> internal_functions_in_yacc
%type <ast> exit_expr scalar backticks_expr lexical_var function_call member_name property_name
%type <ast> variable_class_name dereferencable_scalar constant dereferencable
%type <ast> callable_expr callable_variable static_member new_variable
%type <ast> encaps_var encaps_var_offset isset_variables
%type <ast> top_statement_list use_declarations const_list inner_statement_list if_stmt
%type <ast> alt_if_stmt for_exprs switch_case_list global_var_list static_var_list
%type <ast> echo_expr_list unset_variables catch_name_list catch_list parameter_list class_statement_list
%type <ast> implements_list case_list if_stmt_without_else
%type <ast> non_empty_parameter_list argument_list non_empty_argument_list property_list
%type <ast> class_const_list class_const_decl name_list trait_adaptations method_body non_empty_for_exprs
%type <ast> ctor_arguments alt_if_stmt_without_else trait_adaptation_list lexical_vars
%type <ast> lexical_var_list encaps_list
%type <ast> array_pair non_empty_array_pair_list array_pair_list possible_array_pair
%type <ast> isset_variable type return_type type_expr
%type <ast> identifier

%type <num> returns_ref function is_reference is_variadic variable_modifiers
%type <num> method_modifiers non_empty_member_modifiers member_modifier
%type <num> class_modifiers class_modifier use_type backup_fn_flags

%type <str> backup_doc_comment

%% /* Rules */

start:
      %empty
;

%%
