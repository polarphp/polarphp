%require "3.3"
%language "c++"
%expect 0
%locations

%define api.prefix {polar_yy_}
%define api.namespace{polar::parser::internal}
%define api.value.type variant
%define api.token.constructor false
%define api.parser.class {YYParser}
%define api.location.file "../../../include/polarphp/parser/internal/YYLocation.h"
%define api.location.include{"polarphp/parser/internal/YYLocation.h"}

%parse-param {polar::parser::Parser *parser}
%parse-param {polar::parser::Lexer *lexer}
%lex-param {polar::parser::Lexer *lexer}
%lex-param {polar::parser::Parser *parser}

%code top {
#include <cstdint>

#include "polarphp/syntax/SyntaxNodeFactories.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/DeclSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/ExprSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/StmtSyntaxNodes.h"
#include "polarphp/parser/Parser.h"
#include "polarphp/parser/internal/YYParserExtraDefs.h"
}

%code requires {

#include <memory>
#include "polarphp/syntax/Syntax.h"
#include "polarphp/syntax/References.h"

#define YYERROR_VERBOSE
#define polar_error polar::syntax::parse_error

namespace polar::parser {
class Parser;
class Lexer;
} // polar::parser

using polar::syntax::Syntax;
using polar::syntax::RefCountPtr;
using polar::syntax::RawSyntax;

}

%code provides {
#define polar_yy_lex polar::parser::internal::token_lex_wrapper
namespace polar::parser::internal {
using ParserSemantic = YYParser::semantic_type;
int token_lex_wrapper(ParserSemantic *value, location *loc, Lexer *lexer, Parser *parser);
} // polar::parser::internal
}

%code {
using namespace polar::syntax;
}

// %destructor { delete $$; } <RefCountPtr<RawSyntax>>
// %destructor { if ($$) delete $$; } <str>

%precedence PREC_ARROW_FUNCTION
%precedence T_INCLUDE T_INCLUDE_ONCE T_REQUIRE T_REQUIRE_ONCE
%left T_LOGICAL_OR
%left T_LOGICAL_XOR
%left T_LOGICAL_AND
%precedence T_PRINT
%precedence T_YIELD
%precedence T_DOUBLE_ARROW
%precedence T_YIELD_FROM
%precedence T_EQUAL T_PLUS_EQUAL T_MINUS_EQUAL T_MUL_EQUAL T_DIV_EQUAL T_STR_CONCAT_EQUAL T_MOD_EQUAL T_AND_EQUAL T_OR_EQUAL T_XOR_EQUAL T_SL_EQUAL T_SR_EQUAL T_POW_EQUAL T_COALESCE_EQUAL
%left T_QUESTION_MARK T_COLON
%right T_COALESCE
%left T_BOOLEAN_OR
%left T_BOOLEAN_AND
%left T_VBAR
%left T_CARET
%left T_AMPERSAND
%nonassoc T_IS_EQUAL T_IS_NOT_EQUAL T_IS_IDENTICAL T_IS_NOT_IDENTICAL T_SPACESHIP
%nonassoc T_IS_SMALLER T_IS_SMALLER_OR_EQUAL T_IS_GREATER T_IS_GREATER_OR_EQUAL
%left T_SL T_SR
%left T_PLUS_SIGN T_MINUS_SIGN T_STR_CONCAT
%left T_MUL_SIGN T_DIV_SIGN T_MOD_SIGN
%precedence T_EXCLAMATION_MARK
%precedence T_INSTANCEOF
%precedence T_TILDE T_INC T_DEC T_INT_CAST T_DOUBLE_CAST T_STRING_CAST T_ARRAY_CAST T_OBJECT_CAST T_BOOL_CAST T_UNSET_CAST T_ERROR_SUPPRESS_SIGN
%right T_POW
%precedence T_NEW T_CLONE

/* Resolve danging else conflict */
%precedence T_NOELSE
%precedence T_ELSEIF
%precedence T_ELSE

/* token define start */

%token END 0 "end of file (END)"

/* KEYWORD_MARK_START */
%token T_LINE            "__LINE__ (T_LINE)"
%token T_FILE            "__FILE__ (T_FILE)"
%token T_DIR             "__DIR__ (T_DIR)"
%token T_CLASS_CONST         "__CLASS__ (T_CLASS_CONST)"
%token T_TRAIT_CONST         "__TRAIT__ (T_TRAIT_CONST)"
%token T_METHOD_CONST        "__METHOD__ (T_METHOD_CONST)"
%token T_FUNC_CONST          "__FUNCTION__ (T_FUNC_CONST)"
%token T_NS_CONST            "__NAMESPACE__ (T_NS_CONST)"
/* KEYWORD_MARK_END */

/* DECL_KEYWORD_MARK_START */
%token T_NAMESPACE       "namespace (T_NAMESPACE)"
%token T_CLASS      "class (T_CLASS)"
%token T_TRAIT      "trait (T_TRAIT)"
%token T_INTERFACE  "interface (T_INTERFACE)"
%token T_EXTENDS    "extends (T_EXTENDS)"
%token T_IMPLEMENTS "implements (T_IMPLEMENTS)"
%token T_FUNCTION   "function (T_FUNCTION)"
%token T_FN         "fn (T_FN)"
%token T_CONST      "const (T_CONST)"
%token T_VAR        "var (T_VAR)"
%token T_USE        "use (T_USE)"
%token T_INSTEADOF  "insteadof (T_INSTEADOF)"
%token T_AS         "as (T_AS)"
%token T_GLOBAL     "global (T_GLOBAL)"
%token T_STATIC     "static (T_STATIC)"
%token T_ABSTRACT   "abstract (T_ABSTRACT)"
%token T_FINAL      "final (T_FINAL)"
%token T_PRIVATE    "private (T_PRIVATE)"
%token T_PROTECTED  "protected (T_PROTECTED)"
%token T_PUBLIC     "public (T_PUBLIC)"
%token T_LIST            "list (T_LIST)"
%token T_ARRAY           "array (T_ARRAY)"
%token T_CALLABLE        "callable (T_CALLABLE)"
%token T_THREAD_LOCAL    "thread_local (T_THREAD_LOCAL)"
%token T_MODULE "module (T_MODULE)"
%token T_PACKAGE "package (T_PACKAGE)"
%token T_ASYNC "async (T_ASYNC)"
%token T_EXPORT "export (T_EXPORT)"
/* DECL_KEYWORD_MARK_END */

/* STMT_KEYWORD_MARK_STRAT */
%token T_DEFER     "defer (T_DEFER)"
%token T_IF        "if (T_IF)"
%token T_ELSEIF    "elseif (T_ELSEIF)"
%token T_ELSE      "else (T_ELSE)"
%token T_ECHO       "echo (T_ECHO)"
%token T_DO         "do (T_DO)"
%token T_WHILE      "while (T_WHILE)"
%token T_FOR        "for (T_FOR)"
%token T_FOREACH    "foreach (T_FOREACH)"
%token T_SWITCH     "switch (T_SWITCH)"
%token T_CASE       "case (T_CASE)"
%token T_DEFAULT    "default (T_DEFAULT)"
%token T_BREAK      "break (T_BREAK)"
%token T_CONTINUE   "continue (T_CONTINUE)"
%token T_FALLTHROUGH "fallthrough (T_FALLTHROUGH)"
%token T_GOTO       "goto (T_GOTO)"
%token T_RETURN     "return (T_RETURN)"
%token T_TRY        "try (T_TRY)"
%token T_CATCH      "catch (T_CATCH)"
%token T_FINALLY    "finally (T_FINALLY)"
%token T_THROW      "throw (T_THROW)"
/* STMT_KEYWORD_MARK_END */

/* EXPR_KEYWORD_MARK_START */
%token T_UNSET      "unset (T_UNSET)"
%token T_ISSET      "isset (T_ISSET)"
%token T_EMPTY      "empty (T_EMPTY)"
%token T_HALT_COMPILER "__halt_compiler (T_HALT_COMPILER)"
%token T_EVAL         "eval (T_EVAL)"
%token T_INCLUDE      "include (T_INCLUDE)"
%token T_INCLUDE_ONCE "include_once (T_INCLUDE_ONCE)"
%token T_REQUIRE      "require (T_REQUIRE)"
%token T_REQUIRE_ONCE "require_once (T_REQUIRE_ONCE)"
%token T_LOGICAL_OR   "or (T_LOGICAL_OR)"
%token T_LOGICAL_XOR  "xor (T_LOGICAL_XOR)"
%token T_LOGICAL_AND  "and (T_LOGICAL_AND)"
%token T_PRINT        "print (T_PRINT)"
%token T_YIELD        "yield (T_YIELD)"
%token T_YIELD_FROM   "yield from (T_YIELD_FROM)"
%token T_INSTANCEOF  "instanceof (T_INSTANCEOF)"
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
%token T_DECLARE    "declare (T_DECLARE)"
%token T_CLASS_REF_STATIC "static (T_CLASS_REF_STATIC)"
%token T_CLASS_REF_SELF "self (T_CLASS_REF_SELF)"
%token T_CLASS_REF_PARENT "parent (T_CLASS_REF_PARENT)"
%token T_OBJ_REF "this (T_OBJ_REF)"
%token T_TRUE "true (T_TRUE)"
%token T_FALSE "false (T_FALSE)"
%token T_NULL "null (T_NULL)"
%token T_AWAIT "await (T_AWAIT)"
/* EXPR_KEYWORD_MARK_END */

/* PUNCTUATOR_MARK_START */
%token T_PLUS_SIGN "+ (T_PLUS_SIGN)"
%token T_MINUS_SIGN "- (T_MINUS_SIGN)"
%token T_MUL_SIGN "* (T_MUL_SIGN)"
%token T_DIV_SIGN "/ (T_DIV_SIGN)"
%token T_MOD_SIGN "% (T_MOD_SIGN)"
%token T_EQUAL        "= (T_EQUAL)"
%token T_STR_CONCAT   ". (T_STR_CONCAT)"
%token T_PLUS_EQUAL   "+= (T_PLUS_EQUAL)"
%token T_MINUS_EQUAL  "-= (T_MINUS_EQUAL)"
%token T_MUL_EQUAL    "*= (T_MUL_EQUAL)"
%token T_DIV_EQUAL    "/= (T_DIV_EQUAL)"
%token T_STR_CONCAT_EQUAL ".= (T_STR_CONCAT_EQUAL)"
%token T_MOD_EQUAL    "%= (T_MOD_EQUAL)"
%token T_AND_EQUAL    "&= (T_AND_EQUAL)"
%token T_OR_EQUAL     "|= (T_OR_EQUAL)"
%token T_XOR_EQUAL    "^= (T_XOR_EQUAL)"
%token T_SL_EQUAL     "<<= (T_SL_EQUAL)"
%token T_SR_EQUAL     ">>= (T_SR_EQUAL)"
%token T_COALESCE_EQUAL "\?\?= (T_COALESCE_EQUAL)"
%token T_BOOLEAN_OR   "|| (T_BOOLEAN_OR)"
%token T_BOOLEAN_AND  "&& (T_BOOLEAN_AND)"
%token T_IS_EQUAL     "== (T_IS_EQUAL)"
%token T_IS_NOT_EQUAL "!= (T_IS_NOT_EQUAL)"
%token T_IS_IDENTICAL "=== (T_IS_IDENTICAL)"
%token T_IS_NOT_IDENTICAL "!== (T_IS_NOT_IDENTICAL)"
%token T_IS_SMALLER "< (T_IS_SMALLER)"
%token T_IS_SMALLER_OR_EQUAL "<= (T_IS_SMALLER_OR_EQUAL)"
%token T_IS_GREATER_OR_EQUAL ">= (T_IS_GREATER_OR_EQUAL)"
%token T_IS_GREATER "> (T_IS_GREATER)"
%token T_SPACESHIP "<=> (T_SPACESHIP)"
%token T_SL "<< (T_SL)"
%token T_SR ">> (T_SR)"
%token T_INC "++ (T_INC)"
%token T_DEC "-- (T_DEC)"
%token T_NS_SEPARATOR    "\\ (T_NS_SEPARATOR)"
%token T_ELLIPSIS        "... (T_ELLIPSIS)"
%token T_COALESCE        "?? (T_COALESCE)"
%token T_POW             "** (T_POW)"
%token T_POW_EQUAL       "**= (T_POW_EQUAL)"
%token T_OBJECT_OPERATOR "-> (T_OBJECT_OPERATOR)"
%token T_DOUBLE_ARROW    "=> (T_DOUBLE_ARROW)"
%token T_DOLLAR_OPEN_CURLY_BRACES "${ (T_DOLLAR_OPEN_CURLY_BRACES)"
%token T_CURLY_OPEN      "{$ (T_CURLY_OPEN)"
%token T_PAAMAYIM_NEKUDOTAYIM ":: (T_PAAMAYIM_NEKUDOTAYIM)"
%token T_LEFT_PAREN "( (T_LEFT_PAREN)"
%token T_RIGHT_PAREN ") (T_RIGHT_PAREN)"
%token T_LEFT_BRACE "{ (T_LEFT_BRACE)"
%token T_RIGHT_BRACE "} (T_RIGHT_BRACE)"
%token T_LEFT_SQUARE_BRACKET "[ (T_LEFT_SQUARE_BRACKET)"
%token T_RIGHT_SQUARE_BRACKET "] (T_RIGHT_SQUARE_BRACKET)"
%token T_LEFT_ANGLE "< (T_LEFT_ANGLE)"
%token T_RIGHT_ANGLE "> (T_RIGHT_ANGLE)"
%token T_COMMA ", (T_COMMA)"
%token T_COLON ": (T_COLON)"
%token T_SEMICOLON "; (T_SEMICOLON)"
%token T_BACKTICK "` (T_BACKTICK)"
%token T_SINGLE_QUOTE "' (T_SINGLE_QUOTE)"
%token T_DOUBLE_QUOTE "\" (T_SINGLE_QUOTE)"
%token T_VBAR "| (T_VBAR)"
%token T_CARET "^ (T_CARET)"
%token T_EXCLAMATION_MARK "! (T_EXCLAMATION_MARK)"
%token T_TILDE "~ (T_TILDE)"
%token T_DOLLAR_SIGN "$ (T_DOLLAR_SIGN)"
%token T_QUESTION_MARK "? (T_QUESTION_MARK)"
%token T_ERROR_SUPPRESS_SIGN "@ (T_ERROR_SUPPRESS_SIGN)"
%token T_AMPERSAND "& (T_AMPERSAND)"
/* PUNCTUATOR_MARK_END */

/* MISC_MARK_START */
%token <std::int64_t> T_LNUMBER   "integer number (T_LNUMBER)"
%token <double> T_DNUMBER   "floating-point number (T_DNUMBER)"
%token <std::string> T_IDENTIFIER_STRING    "identifier (T_IDENTIFIER_STRING)"
%token <std::string> T_VARIABLE  "variable (T_VARIABLE)"
%token <std::string> T_ENCAPSED_AND_WHITESPACE  "quoted-string and whitespace (T_ENCAPSED_AND_WHITESPACE)"
%token <std::string> T_CONSTANT_ENCAPSED_STRING "quoted-string (T_CONSTANT_ENCAPSED_STRING)"
%token <std::string> T_STRING_VARNAME "variable name (T_STRING_VARNAME)"
%token <std::string> T_NUM_STRING "number (T_NUM_STRING)"

%token T_WHITESPACE      "whitespace (T_WHITESPACE)"
%token T_PREFIX_OPERATOR "prefix operator (T_PREFIX_OPERATOR)"
%token T_POSTFIX_OPERATOR "postfix operator (T_POSTFIX_OPERATOR)"
%token T_BINARY_OPERATOR "binary operator (T_BINARY_OPERATOR)"
%token T_COMMENT         "comment (T_COMMENT)"
%token T_DOC_COMMENT     "doc comment (T_DOC_COMMENT)"
%token T_OPEN_TAG        "open tag (T_OPEN_TAG)"
%token T_OPEN_TAG_WITH_ECHO "open tag with echo (T_OPEN_TAG_WITH_ECHO)"
%token T_CLOSE_TAG       "close tag (T_CLOSE_TAG)"
%token T_START_HEREDOC   "heredoc start (T_START_HEREDOC)"
%token T_END_HEREDOC     "heredoc end (T_END_HEREDOC)"

/* Token used to force a parse error from the lexer */
%token <std::string> T_ERROR          "error (T_ERROR)"
%token T_UNKNOWN_MARK "unknown token (T_UNKNOWN_MARK)"
/* MISC_MARK_END */
/* token define end */

%type <RefCountPtr<RawSyntax>> top_statement namespace_name name statement function_declaration_statement
%type <RefCountPtr<RawSyntax>> class_declaration_statement trait_declaration_statement
%type <RefCountPtr<RawSyntax>> interface_declaration_statement interface_extends_list
%type <RefCountPtr<RawSyntax>> group_use_declaration inline_use_declarations inline_use_declaration
%type <RefCountPtr<RawSyntax>> mixed_group_use_declaration use_declaration unprefixed_use_declaration
%type <RefCountPtr<RawSyntax>> unprefixed_use_declarations const_decl inner_statement
%type <RefCountPtr<RawSyntax>> expr optional_expr foreach_variable
%type <RefCountPtr<RawSyntax>> finally_statement unset_variable variable
%type <RefCountPtr<RawSyntax>> extends_from parameter optional_type argument global_var
%type <RefCountPtr<RawSyntax>> static_var class_statement trait_adaptation trait_precedence trait_alias
%type <RefCountPtr<RawSyntax>> absolute_trait_method_reference trait_method_reference property echo_expr
%type <RefCountPtr<RawSyntax>> new_expr anonymous_class class_name class_name_reference simple_variable
%type <RefCountPtr<RawSyntax>> internal_functions_in_bison
%type <RefCountPtr<RawSyntax>> exit_expr scalar backticks_expr lexical_var function_call member_name property_name
%type <RefCountPtr<RawSyntax>> variable_class_name dereferencable_scalar constant dereferencable
%type <RefCountPtr<RawSyntax>> callable_expr callable_variable static_member new_variable
%type <RefCountPtr<RawSyntax>> encaps_var encaps_var_offset isset_variables
%type <RefCountPtr<RawSyntax>> top_statement_list use_declarations const_list inner_statement_list if_stmt
%type <RefCountPtr<RawSyntax>> for_exprs switch_case_list global_var_list static_var_list
%type <RefCountPtr<RawSyntax>> echo_expr_list unset_variables catch_name_list catch_list parameter_list class_statement_list
%type <RefCountPtr<RawSyntax>> implements_list case_list if_stmt_without_else
%type <RefCountPtr<RawSyntax>> non_empty_parameter_list argument_list non_empty_argument_list property_list
%type <RefCountPtr<RawSyntax>> class_const_list class_const_decl name_list trait_adaptations method_body non_empty_for_exprs
%type <RefCountPtr<RawSyntax>> ctor_arguments trait_adaptation_list lexical_vars
%type <RefCountPtr<RawSyntax>> lexical_var_list encaps_list
%type <RefCountPtr<RawSyntax>> array_pair non_empty_array_pair_list array_pair_list possible_array_pair
%type <RefCountPtr<RawSyntax>> isset_variable type return_type type_expr
%type <RefCountPtr<RawSyntax>> identifier
%type <RefCountPtr<RawSyntax>> inline_function

%type <RefCountPtr<RawSyntax>> returns_ref function fn is_reference is_variadic variable_modifiers
%type <RefCountPtr<RawSyntax>> method_modifiers non_empty_member_modifiers member_modifier
%type <RefCountPtr<RawSyntax>> class_modifiers class_modifier use_type backup_fn_flags

%type <unsigned char *> backup_lex_pos
%type <std::string> backup_doc_comment

%% /* Rules */

start:
   top_statement_list {
      parser->setParsedAst($1);
   }
;

reserved_non_modifiers:
     T_INCLUDE | T_INCLUDE_ONCE | T_EVAL | T_REQUIRE | T_REQUIRE_ONCE | T_LOGICAL_OR | T_LOGICAL_XOR | T_LOGICAL_AND
   | T_INSTANCEOF | T_NEW | T_CLONE | T_EXIT | T_IF | T_ELSEIF | T_ELSE | T_ECHO | T_DO | T_WHILE
   | T_FOR | T_FOREACH | T_DECLARE | T_AS | T_TRY | T_CATCH | T_FINALLY
   | T_THROW | T_USE | T_INSTEADOF | T_GLOBAL | T_VAR | T_UNSET | T_ISSET | T_EMPTY | T_CONTINUE | T_GOTO
   | T_FUNCTION | T_CONST | T_RETURN | T_PRINT | T_YIELD | T_LIST | T_SWITCH | T_CASE | T_DEFAULT | T_BREAK
   | T_ARRAY | T_CALLABLE | T_EXTENDS | T_IMPLEMENTS | T_NAMESPACE | T_TRAIT | T_INTERFACE | T_CLASS
   | T_CLASS_CONST | T_TRAIT_CONST | T_FUNC_CONST | T_METHOD_CONST | T_LINE | T_FILE | T_DIR | T_NS_CONST | T_FN
;

semi_reserved:
      reserved_non_modifiers
   |  T_STATIC | T_ABSTRACT | T_FINAL | T_PRIVATE | T_PROTECTED | T_PUBLIC
;

identifier:
   T_IDENTIFIER_STRING {
      TokenSyntax identifierToken = make_token_with_text(IdentifierString, $1);
      IdentifierSyntax identifier = make_decl(Identifier, identifierToken);
      $$ = identifier.getRaw();
   }
|	semi_reserved {
      // TokenKindType kind = $<int>1;
      // TokenSyntax identifierToken = AbstractFactory::makeToken(kind, OwnedString::makeUnowned(get_token_text(kind)),
      //                                                          empty_trivia(), empty_trivia(), SourcePresence::Present);
      // IdentifierSyntax identifier = make_decl(Identifier, identifierToken);
      // $$ = identifier.getRaw();
   }
;

top_statement_list:
   top_statement_list top_statement {
      TopStmtListSyntax topStmtList = make<TopStmtListSyntax>($1);
      TopStmtSyntax stmt = make<TopStmtSyntax>($2);
      topStmtList.appending(stmt);
      $$ = topStmtList.getRaw();
   }
|  %empty {
      TopStmtListSyntax topStmtList = make_blank_stmt(TopStmtList);
      $$ = topStmtList.getRaw();
   }
;

namespace_name:
   T_IDENTIFIER_STRING {
      TokenSyntax identifierToken = make_token_with_text(IdentifierString, $1);
      NamespaceNameSyntax namespaceName = make_decl(NamespaceName, std::nullopt, std::nullopt, identifierToken);
      $$ = namespaceName.getRaw();
   }
|  namespace_name T_NS_SEPARATOR T_IDENTIFIER_STRING {
      NamespaceNameSyntax parentNs = make<NamespaceNameSyntax>($1);
      TokenSyntax separator = make_token(NamespaceSeparatorToken);
      TokenSyntax identifierToken = make_token_with_text(IdentifierString, $3);
      NamespaceNameSyntax newNs = make_decl(NamespaceName, parentNs, separator, identifierToken);
      $$ = newNs.getRaw();
   }
;

name:
   namespace_name {
      NamespaceNameSyntax ns = make<NamespaceNameSyntax>($1);
      NameSyntax name = make_decl(Name, std::nullopt, std::nullopt, ns);
      $$ = ns.getRaw();
   }
|  T_NAMESPACE T_NS_SEPARATOR namespace_name {
      TokenSyntax nsKeyword = make_token(NamespaceKeyword);
      TokenSyntax separator = make_token(NamespaceSeparatorToken);
      NamespaceNameSyntax ns = make<NamespaceNameSyntax>($3);
      NameSyntax name = make_decl(Name, nsKeyword, separator, ns);
      $$ = name.getRaw();
   }
|  T_NS_SEPARATOR namespace_name {
      TokenSyntax separator = make_token(NamespaceSeparatorToken);
      NamespaceNameSyntax ns = make<NamespaceNameSyntax>($2);
      NameSyntax name = make_decl(Name, std::nullopt, separator, ns);
      $$ = name.getRaw();
   }
;

top_statement:
   statement {
      StmtSyntax stmt = make<StmtSyntax>($1);
      TopStmtSyntax topStmt = make_stmt(TopStmt, stmt);
      $$ = topStmt.getRaw();
   }
|  function_declaration_statement {

   }
|  class_declaration_statement {

   }
|  trait_declaration_statement {

   }
|  interface_declaration_statement {

   }
|  T_HALT_COMPILER T_LEFT_PAREN T_RIGHT_PAREN T_SEMICOLON {
      TokenSyntax haltCompilerToken = make_token(HaltCompilerKeyword);
      TokenSyntax leftParenToken = make_token(LeftParenToken);
      TokenSyntax rightParenToken = make_token(RightParenToken);
      TokenSyntax SemicolonToken = make_token(SemicolonToken);
      HaltCompilerStmtSyntax stmt = make_stmt(HaltCompilerStmt, haltCompilerToken, leftParenToken, rightParenToken, SemicolonToken);
      $$ = stmt.getRaw();
   }
|  T_NAMESPACE namespace_name T_SEMICOLON {
      TokenSyntax namespaceKeyword = make_token(NamespaceKeyword);
      NamespaceNameSyntax namespaceName = make<NamespaceNameSyntax>($2);
      TokenSyntax SemicolonToken = make_token(SemicolonToken);
      NamespaceDefinitionStmtSyntax namespaceStmt = make_stmt(NamespaceDefinitionStmt, namespaceKeyword, namespaceName, SemicolonToken);
      $$ = namespaceStmt.getRaw();
      RESET_DOC_COMMENT();
   }
|  T_NAMESPACE namespace_name { RESET_DOC_COMMENT(); }
   T_LEFT_BRACE top_statement_list T_RIGHT_BRACE {
      TokenSyntax namespaceKeyword = make_token(NamespaceKeyword);
      NamespaceNameSyntax namespaceName = make<NamespaceNameSyntax>($2);
      TokenSyntax leftParenToken = make_token(LeftParenToken);
      TopStmtListSyntax topStmtList = make<TopStmtListSyntax>($5);
      TokenSyntax rightParenToken = make_token(RightParenToken);
      TopCodeBlockStmtSyntax codeblock = make_stmt(TopCodeBlockStmt, leftParenToken, topStmtList, rightParenToken);
      NamespaceBlockStmtSyntax namespaceBlockStmt = make_stmt(NamespaceBlockStmt, namespaceKeyword, namespaceName, codeblock);
      $$ = namespaceBlockStmt.getRaw();
   }
|  T_NAMESPACE { RESET_DOC_COMMENT(); }
   T_LEFT_BRACE top_statement_list T_RIGHT_BRACE {
      TokenSyntax namespaceKeyword = make_token(NamespaceKeyword);
      TokenSyntax leftParenToken = make_token(LeftParenToken);
      TopStmtListSyntax topStmtList = make<TopStmtListSyntax>($4);
      TokenSyntax rightParenToken = make_token(RightParenToken);
      TopCodeBlockStmtSyntax codeblock = make_stmt(TopCodeBlockStmt, leftParenToken, topStmtList, rightParenToken);
      NamespaceBlockStmtSyntax namespaceBlockStmt = make_stmt(NamespaceBlockStmt, namespaceKeyword, std::nullopt, codeblock);
      $$ = namespaceBlockStmt.getRaw();
   }
|  T_USE mixed_group_use_declaration T_SEMICOLON {
      TokenSyntax useKeyword = make_token(UseKeyword);
      NamespaceMixedGroupUseDeclarationSyntax groupDecl = make<NamespaceMixedGroupUseDeclarationSyntax>($2);
      TokenSyntax semicolonToken = make_token(SemicolonToken);
      NamespaceUseStmtSyntax declStmt = make_stmt(NamespaceUseStmt, useKeyword, std::nullopt, groupDecl, semicolonToken);
      $$ = declStmt.getRaw();
   }
|  T_USE use_type group_use_declaration T_SEMICOLON {
      TokenSyntax useKeyword = make_token(UseKeyword);
      NamespaceUseTypeSyntax useType = make<NamespaceUseTypeSyntax>($2);
      NamespaceGroupUseDeclarationSyntax groupDecl = make<NamespaceGroupUseDeclarationSyntax>($3);
      TokenSyntax semicolonToken = make_token(SemicolonToken);
      NamespaceUseStmtSyntax declStmt = make_stmt(NamespaceUseStmt, useKeyword, useType, groupDecl, semicolonToken);
      $$ = declStmt.getRaw();
   }
|  T_USE use_declarations T_SEMICOLON {
      TokenSyntax useKeyword = make_token(UseKeyword);
      NamespaceUseDeclarationListSyntax declarations = make<NamespaceUseDeclarationListSyntax>($2);
      TokenSyntax semicolonToken = make_token(SemicolonToken);
      NamespaceUseStmtSyntax declStmt = make_stmt(NamespaceUseStmt, useKeyword, std::nullopt, declarations, semicolonToken);
      $$ = declStmt.getRaw();
   }
|  T_USE use_type use_declarations T_SEMICOLON {
      TokenSyntax useKeyword = make_token(UseKeyword);
      NamespaceUseTypeSyntax useType = make<NamespaceUseTypeSyntax>($2);
      NamespaceUseDeclarationListSyntax declarations = make<NamespaceUseDeclarationListSyntax>($3);
      TokenSyntax semicolonToken = make_token(SemicolonToken);
      NamespaceUseStmtSyntax declStmt = make_stmt(NamespaceUseStmt, useKeyword, useType, declarations, semicolonToken);
      $$ = declStmt.getRaw();
   }
|  T_CONST const_list T_SEMICOLON {
      TokenSyntax constKeyword = make_token(ConstKeyword);
      ConstDeclareListSyntax constList = make<ConstDeclareListSyntax>($2);
      TokenSyntax semicolon = make_token(SemicolonToken);
      ConstDefinitionStmtSyntax constDeclStmt = make_stmt(ConstDefinitionStmt, constKeyword, constList, semicolon);
      $$ = constDeclStmt.getRaw();
   }
;

use_type:
   T_FUNCTION {
      TokenSyntax funcKeyword = make_token(FunctionKeyword);
      NamespaceUseTypeSyntax useType = make_stmt(NamespaceUseType, funcKeyword);
      $$ = useType.getRaw();
   }
|  T_CONST {
      TokenSyntax constKeyword = make_token(ConstKeyword);
      NamespaceUseTypeSyntax useType = make_stmt(NamespaceUseType, constKeyword);
      $$ = useType.getRaw();
   }
;

group_use_declaration:
   namespace_name T_NS_SEPARATOR T_LEFT_BRACE unprefixed_use_declarations possible_comma T_RIGHT_BRACE {
      NamespaceNameSyntax ns = make<NamespaceNameSyntax>($1);
      TokenSyntax secondNsSeparator = make_token(NamespaceSeparatorToken);
      TokenSyntax leftParen = make_token(LeftParenToken);
      NamespaceUnprefixedUseDeclarationListSyntax declarations = make<NamespaceUnprefixedUseDeclarationListSyntax>($4);
      TokenSyntax rightParen = make_token(RightParenToken);
      NamespaceGroupUseDeclarationSyntax groupUseDeclaration = make_stmt(NamespaceGroupUseDeclaration,
         std::nullopt, ns, secondNsSeparator, leftParen, declarations, std::nullopt, rightParen
      );
      $$ = groupUseDeclaration.getRaw();
   }
|  T_NS_SEPARATOR namespace_name T_NS_SEPARATOR T_LEFT_BRACE unprefixed_use_declarations possible_comma T_RIGHT_BRACE {
      TokenSyntax firstNsSeparator = make_token(NamespaceSeparatorToken);
      NamespaceNameSyntax ns = make<NamespaceNameSyntax>($2);
      TokenSyntax secondNsSeparator = make_token(NamespaceSeparatorToken);
      TokenSyntax leftParen = make_token(LeftParenToken);
      NamespaceUnprefixedUseDeclarationListSyntax declarations = make<NamespaceUnprefixedUseDeclarationListSyntax>($5);
      TokenSyntax rightParen = make_token(RightParenToken);
      NamespaceGroupUseDeclarationSyntax groupUseDeclaration = make_stmt(NamespaceGroupUseDeclaration,
         firstNsSeparator, ns, secondNsSeparator, leftParen, declarations, std::nullopt, rightParen
      );
      $$ = groupUseDeclaration.getRaw();
   }
;

mixed_group_use_declaration:
   namespace_name T_NS_SEPARATOR T_LEFT_BRACE inline_use_declarations possible_comma T_RIGHT_BRACE {
      NamespaceNameSyntax ns = make<NamespaceNameSyntax>($1);
      TokenSyntax secondNsSeparator = make_token(NamespaceSeparatorToken);
      TokenSyntax leftParen = make_token(LeftParenToken);
      NamespaceInlineUseDeclarationListSyntax declarations = make<NamespaceInlineUseDeclarationListSyntax>($4);
      TokenSyntax rightParen = make_token(RightParenToken);
      NamespaceMixedGroupUseDeclarationSyntax groupUseDeclaration = make_stmt(NamespaceMixedGroupUseDeclaration,
         std::nullopt, ns, secondNsSeparator, leftParen, declarations, std::nullopt, rightParen
      );
      $$ = groupUseDeclaration.getRaw();
   }
|  T_NS_SEPARATOR namespace_name T_NS_SEPARATOR T_LEFT_BRACE inline_use_declarations possible_comma T_RIGHT_BRACE {
      TokenSyntax firstNsSeparator = make_token(NamespaceSeparatorToken);
      NamespaceNameSyntax ns = make<NamespaceNameSyntax>($2);
      TokenSyntax secondNsSeparator = make_token(NamespaceSeparatorToken);
      TokenSyntax leftParen = make_token(LeftParenToken);
      NamespaceInlineUseDeclarationListSyntax declarations = make<NamespaceInlineUseDeclarationListSyntax>($5);
      TokenSyntax rightParen = make_token(RightParenToken);
      NamespaceMixedGroupUseDeclarationSyntax groupUseDeclaration = make_stmt(NamespaceMixedGroupUseDeclaration,
         firstNsSeparator, ns, secondNsSeparator, leftParen, declarations, std::nullopt, rightParen
      );
      $$ = groupUseDeclaration.getRaw();
   }
;

possible_comma:
   %empty
|  T_COMMA
;

inline_use_declarations:
   inline_use_declarations T_COMMA inline_use_declaration {
      NamespaceInlineUseDeclarationListSyntax list = make<NamespaceInlineUseDeclarationListSyntax>($1);
      TokenSyntax comma = make_token(CommaToken);
      NamespaceInlineUseDeclarationSyntax useDecl = make<NamespaceInlineUseDeclarationSyntax>($3);
      NamespaceInlineUseDeclarationListItemSyntax useDeclListItem = make_stmt(NamespaceInlineUseDeclarationListItem, comma, useDecl);
      list.appending(useDeclListItem);
      $$ = list.getRaw();
   }
|  inline_use_declaration {
      NamespaceInlineUseDeclarationSyntax inlineUseDecl = make<NamespaceInlineUseDeclarationSyntax>($1);
      NamespaceInlineUseDeclarationListItemSyntax useDeclListItem = make_stmt(NamespaceInlineUseDeclarationListItem, std::nullopt, inlineUseDecl);
      std::vector<NamespaceInlineUseDeclarationListItemSyntax> declarations{useDeclListItem};
      NamespaceInlineUseDeclarationListSyntax list = make_stmt(NamespaceInlineUseDeclarationList, declarations);
      $$ = list.getRaw();
   }
;

unprefixed_use_declarations:
   unprefixed_use_declarations T_COMMA unprefixed_use_declaration {
      NamespaceUnprefixedUseDeclarationListSyntax list = make<NamespaceUnprefixedUseDeclarationListSyntax>($1);
      TokenSyntax comma = make_token(CommaToken);
      NamespaceUnprefixedUseDeclarationSyntax unprefixedUseDecl = make<NamespaceUnprefixedUseDeclarationSyntax>($3);
      NamespaceUnprefixedUseDeclarationListItemSyntax useDeclListItem = make_stmt(NamespaceUnprefixedUseDeclarationListItem, comma, unprefixedUseDecl);
      list.appending(useDeclListItem);
      $$ = list.getRaw();
   }
|  unprefixed_use_declaration {
      NamespaceUnprefixedUseDeclarationSyntax unprefixedUseDecl = make<NamespaceUnprefixedUseDeclarationSyntax>($1);
      NamespaceUnprefixedUseDeclarationListItemSyntax useDeclListItem = make_stmt(NamespaceUnprefixedUseDeclarationListItem, std::nullopt, unprefixedUseDecl);
      std::vector<NamespaceUnprefixedUseDeclarationListItemSyntax> declarations{useDeclListItem};
      NamespaceUnprefixedUseDeclarationListSyntax list = make_stmt(NamespaceUnprefixedUseDeclarationList, declarations);
      $$ = list.getRaw();
   }
;

use_declarations:
   use_declarations T_COMMA use_declaration {
      NamespaceUseDeclarationListSyntax list = make<NamespaceUseDeclarationListSyntax>($1);
      TokenSyntax comma = make_token(CommaToken);
      NamespaceUseDeclarationSyntax useDecl = make<NamespaceUseDeclarationSyntax>($3);
      NamespaceUseDeclarationListItemSyntax useDeclListItem = make_stmt(NamespaceUseDeclarationListItem, comma, useDecl);
      list.appending(useDeclListItem);
      $$ = list.getRaw();
   }
|  use_declaration {
      NamespaceUseDeclarationSyntax declaration = make<NamespaceUseDeclarationSyntax>($1);
      NamespaceUseDeclarationListItemSyntax declarationListItem = make_stmt(NamespaceUseDeclarationListItem, std::nullopt, declaration);
      std::vector<NamespaceUseDeclarationListItemSyntax> declarations{declarationListItem};
      NamespaceUseDeclarationListSyntax list = make_stmt(NamespaceUseDeclarationList, declarations);
      $$ = list.getRaw();
   }
;

inline_use_declaration:
   unprefixed_use_declaration {
      NamespaceUnprefixedUseDeclarationSyntax unprefixedUseDecl = make<NamespaceUnprefixedUseDeclarationSyntax>($1);
      NamespaceInlineUseDeclarationSyntax inlineUseDecl = make_stmt(NamespaceInlineUseDeclaration, std::nullopt, unprefixedUseDecl);
      $$ = inlineUseDecl.getRaw();
   }
|  use_type unprefixed_use_declaration {
      NamespaceUseTypeSyntax useType = make<NamespaceUseTypeSyntax>($1);
      NamespaceUnprefixedUseDeclarationSyntax unprefixedUseDecl = make<NamespaceUnprefixedUseDeclarationSyntax>($2);
      NamespaceInlineUseDeclarationSyntax inlineUseDecl = make_stmt(NamespaceInlineUseDeclaration, useType, unprefixedUseDecl);
      $$ = inlineUseDecl.getRaw();
   }
;

unprefixed_use_declaration:
   namespace_name {
      NamespaceNameSyntax ns = make<NamespaceNameSyntax>($1);
      NamespaceUnprefixedUseDeclarationSyntax declaration = make_stmt(NamespaceUnprefixedUseDeclaration, ns, std::nullopt, std::nullopt);
      $$ = declaration.getRaw();
   }
|  namespace_name T_AS T_IDENTIFIER_STRING {
      NamespaceNameSyntax ns = make<NamespaceNameSyntax>($1);
      TokenSyntax asToken = make_token(AsKeyword);
      TokenSyntax identifierStr = make_token_with_text(IdentifierString, $3);
      NamespaceUnprefixedUseDeclarationSyntax declaration = make_stmt(NamespaceUnprefixedUseDeclaration, ns, asToken, identifierStr);
      $$ = declaration.getRaw();
   }
;

use_declaration:
   unprefixed_use_declaration {
      NamespaceUnprefixedUseDeclarationSyntax unprefixedUseDecl = make<NamespaceUnprefixedUseDeclarationSyntax>($1);
      NamespaceUseDeclarationSyntax useDecl = make_stmt(NamespaceUseDeclaration, std::nullopt, unprefixedUseDecl);
      $$ = useDecl.getRaw();
   }
|  T_NS_SEPARATOR unprefixed_use_declaration {
      TokenSyntax nsSeparator = make_token(NamespaceSeparatorToken);
      NamespaceUnprefixedUseDeclarationSyntax unprefixedUseDecl = make<NamespaceUnprefixedUseDeclarationSyntax>($2);
      NamespaceUseDeclarationSyntax useDecl = make_stmt(NamespaceUseDeclaration, nsSeparator, unprefixedUseDecl);
      $$ = useDecl.getRaw();
   }
;

const_list:
   const_list T_COMMA const_decl {
      ConstDeclareListSyntax list = make<ConstDeclareListSyntax>($1);
      ConstDeclareSyntax constDecl = make<ConstDeclareSyntax>($3);
      TokenSyntax comma = make_token(CommaToken);
      ConstListItemSyntax constListItem = make_stmt(ConstListItem, comma, constDecl);
      list.appending(constListItem);
      $$ = list.getRaw();
   }
|  const_decl {
      ConstDeclareSyntax constDecl = make<ConstDeclareSyntax>($1);
      ConstListItemSyntax constListItem = make_stmt(ConstListItem, std::nullopt, constDecl);
      std::vector<ConstListItemSyntax> declarations{constListItem};
      ConstDeclareListSyntax list = make_stmt(ConstDeclareList, declarations);
      $$ = list.getRaw();
   }
;

inner_statement_list:
   inner_statement_list inner_statement {
      InnerStmtListSyntax stmtList = make<InnerStmtListSyntax>($1);
      InnerStmtSyntax stmt = make<InnerStmtSyntax>($2);
      stmtList.appending(stmt);
      $$ = stmtList.getRaw();
   }
|  %empty {
      InnerStmtListSyntax innerStmtList = make_blank_stmt(InnerStmtList);
      $$ = innerStmtList.getRaw();
   }
;

inner_statement:
   statement {
      $$ = $1;
   }
|  function_declaration_statement {

   }
|  class_declaration_statement {

   }
|  trait_declaration_statement {

   }
|  interface_declaration_statement {

   }
|  T_HALT_COMPILER T_LEFT_PAREN T_RIGHT_PAREN T_SEMICOLON {
      TokenSyntax haltCompilerToken = make_token(HaltCompilerKeyword);
      TokenSyntax leftParenToken = make_token(LeftParenToken);
      TokenSyntax rightParenToken = make_token(RightParenToken);
      TokenSyntax SemicolonToken = make_token(SemicolonToken);
      HaltCompilerStmtSyntax stmt = make_stmt(HaltCompilerStmt, haltCompilerToken, leftParenToken, rightParenToken, SemicolonToken);
      $$ = stmt.getRaw();
   }
;

statement:
   T_LEFT_BRACE inner_statement_list T_RIGHT_BRACE {

   }
|  if_stmt {

   }
|  T_WHILE T_LEFT_PAREN expr T_RIGHT_PAREN statement {

   }
|  T_DO statement T_WHILE T_LEFT_PAREN expr T_RIGHT_PAREN T_SEMICOLON {

   }
|  T_FOR T_LEFT_PAREN for_exprs T_SEMICOLON for_exprs T_SEMICOLON for_exprs T_RIGHT_PAREN statement {

   }
|  T_SWITCH T_LEFT_PAREN expr T_RIGHT_PAREN switch_case_list {

   }
|  T_BREAK optional_expr T_SEMICOLON {

   }
|  T_CONTINUE optional_expr T_SEMICOLON {

   }
|  T_FALLTHROUGH T_SEMICOLON {

   }
|  T_RETURN optional_expr T_SEMICOLON {

   }
|  T_GLOBAL global_var_list T_SEMICOLON {

   }
|  T_STATIC static_var_list T_SEMICOLON {

   }
|  T_ECHO echo_expr_list T_SEMICOLON {

   }
|  expr T_SEMICOLON {
      ExprSyntax expr = make<ExprSyntax>($1);
      TokenSyntax simicolon = make_token(SemicolonToken);
      ExprStmtSyntax exprStmt = make_stmt(ExprStmt, expr, simicolon);
      $$ = exprStmt.getRaw();
   }
|  T_UNSET T_LEFT_PAREN unset_variables possible_comma T_RIGHT_PAREN T_SEMICOLON {

   }
|  T_FOREACH T_LEFT_PAREN expr T_AS foreach_variable T_RIGHT_PAREN statement {

   }
|  T_FOREACH T_LEFT_PAREN expr T_AS foreach_variable T_DOUBLE_ARROW foreach_variable T_RIGHT_PAREN statement {

   }
|  T_DECLARE T_LEFT_PAREN const_list T_RIGHT_PAREN {}
   statement {

   }
|  T_SEMICOLON {
      // make empty stmt
      TokenSyntax semicolon = make_token(SemicolonToken);
      EmptyStmtSyntax emptyStmt = make_stmt(EmptyStmt, semicolon);
      $$ = emptyStmt.getRaw();
   }
|  T_TRY T_LEFT_BRACE inner_statement_list T_RIGHT_BRACE catch_list finally_statement {

   }
|  T_THROW expr T_SEMICOLON {

   }
|  T_GOTO T_IDENTIFIER_STRING T_SEMICOLON {

   }
|  T_IDENTIFIER_STRING T_COLON {

   }
;

catch_list:
   %empty {

   }
|  catch_list T_CATCH T_LEFT_PAREN catch_name_list T_VARIABLE T_RIGHT_PAREN T_LEFT_BRACE inner_statement_list T_RIGHT_BRACE {

   }
;

catch_name_list:
   name {

   }
|  catch_name_list T_VBAR name {

   }
;

finally_statement:
   %empty {

   }
|  T_FINALLY T_LEFT_BRACE inner_statement_list T_RIGHT_BRACE {

   }
;

unset_variables:
   unset_variable {

   }
|  unset_variables T_COMMA unset_variable {

   }
;

unset_variable:
   variable {

   }
;

function_declaration_statement:
   function returns_ref T_IDENTIFIER_STRING backup_doc_comment T_LEFT_PAREN parameter_list T_RIGHT_PAREN return_type
   backup_fn_flags T_LEFT_BRACE inner_statement_list T_RIGHT_BRACE backup_fn_flags {

   }
;

is_reference:
   %empty {

   }
|  T_AMPERSAND {

   }
;

is_variadic:
   %empty {

   }
|  T_ELLIPSIS {

   }
;

class_declaration_statement:
   class_modifiers T_CLASS  {}
   T_IDENTIFIER_STRING extends_from implements_list backup_doc_comment T_LEFT_BRACE class_statement_list T_RIGHT_BRACE {

   }
|  T_CLASS {}
   T_IDENTIFIER_STRING extends_from implements_list backup_doc_comment T_LEFT_BRACE class_statement_list T_RIGHT_BRACE {

   }
;

class_modifiers:
   class_modifier {

   }
|  class_modifiers class_modifier {

   }
;

class_modifier:
   T_ABSTRACT {

   }
|  T_FINAL {

   }
;

trait_declaration_statement:
   T_TRAIT {}
   T_IDENTIFIER_STRING interface_extends_list backup_doc_comment T_LEFT_BRACE class_statement_list T_RIGHT_BRACE {

   }
;

interface_declaration_statement:
   T_INTERFACE {}
   T_IDENTIFIER_STRING interface_extends_list backup_doc_comment T_LEFT_BRACE class_statement_list T_RIGHT_BRACE {

   }
;

extends_from:
   %empty {

   }
|  T_EXTENDS name {

   }
;

interface_extends_list:
   %empty {

   }
|  T_EXTENDS name_list {

   }
;

implements_list:
   %empty {

   }
|  T_IMPLEMENTS name_list {

   }
;

foreach_variable:
   variable {

   }
|  T_AMPERSAND variable {

   }
|  T_LIST T_LEFT_PAREN array_pair_list T_RIGHT_PAREN {

   }
|  T_LEFT_SQUARE_BRACKET array_pair_list T_RIGHT_SQUARE_BRACKET {

   }
;

switch_case_list:
   T_LEFT_BRACE case_list T_RIGHT_BRACE {

   }
|  T_LEFT_BRACE T_SEMICOLON case_list T_RIGHT_BRACE {

   }
;

case_list:
   %empty {

   }
|  case_list T_CASE expr case_separator inner_statement_list {

   }
|  case_list T_DEFAULT case_separator inner_statement_list {

   }
;

case_separator:
   T_COLON
;

if_stmt_without_else:
   T_IF T_LEFT_PAREN expr T_RIGHT_PAREN statement {

   }
|  if_stmt_without_else T_ELSEIF T_LEFT_PAREN expr T_RIGHT_PAREN statement {

   }
;

if_stmt:
   if_stmt_without_else %prec T_NOELSE {

   }
|  if_stmt_without_else T_ELSE statement {

   }
;

parameter_list:
   non_empty_parameter_list {

   }
|  %empty {

   }
;

non_empty_parameter_list:
   parameter {

   }
|  non_empty_parameter_list T_COMMA parameter {

   }
;

parameter:
   optional_type is_reference is_variadic T_VARIABLE {

   }
|  optional_type is_reference is_variadic T_VARIABLE T_EQUAL expr {

   }
;

optional_type:
   %empty {

   }
|  type_expr {

   }
;

type_expr:
   type {

   }
|  T_QUESTION_MARK type {

   }
;

type:
   T_ARRAY {

   }
|  T_CALLABLE {

   }
|  name {

   }
;

return_type:
   %empty {

   }
|  T_COLON type_expr {

   }
;

argument_list:
   T_LEFT_PAREN T_RIGHT_PAREN {

   }
|  T_LEFT_PAREN non_empty_argument_list possible_comma T_RIGHT_PAREN {

   }
;

non_empty_argument_list:
   argument {

   }
|  non_empty_argument_list T_COMMA argument {

   }
;

argument:
   expr {

   }
|  T_ELLIPSIS expr {

   }
;

global_var_list:
   global_var_list T_COMMA global_var {
      GlobalVariableListSyntax list = make<GlobalVariableListSyntax>($1);
      TokenSyntax comma = make_token(CommaToken);
      GlobalVariableListItemSyntax gvar = make<GlobalVariableListItemSyntax>($3);
      list.appending(gvar);
      $$ = list.getRaw();
   }
|  global_var {
      GlobalVariableSyntax gvar = make<GlobalVariableSyntax>($1);
      GlobalVariableListItemSyntax gvarItem = make_stmt(GlobalVariableListItem, std::nullopt, gvar);
      std::vector<GlobalVariableListItemSyntax> vars{gvarItem};
      GlobalVariableListSyntax list = make_stmt(GlobalVariableList, vars);
      $$ = list.getRaw();
   }
;

global_var:
   simple_variable {
      SimpleVariableExprSyntax simpleVar = make<SimpleVariableExprSyntax>($1);
      GlobalVariableSyntax gvar = make_stmt(GlobalVariable, simpleVar);
      $$ = gvar.getRaw();
   }
;

static_var_list:
   static_var_list T_COMMA static_var {
      StaticVariableListSyntax list = make<StaticVariableListSyntax>($1);
      TokenSyntax comma = make_token(CommaToken);
      StaticVariableDeclareSyntax staticVar = make<StaticVariableDeclareSyntax>($3);
      StaticVariableListItemSyntax listItem = make_stmt(StaticVariableListItem, comma, staticVar);
      list.appending(listItem);
      $$ = list.getRaw();
   }
|  static_var {
      StaticVariableDeclareSyntax staticVar = make<StaticVariableDeclareSyntax>($1);
      StaticVariableListItemSyntax listItem = make_stmt(StaticVariableListItem, std::nullopt, staticVar);
      std::vector<StaticVariableListItemSyntax> items{listItem};
      StaticVariableListSyntax list = make_stmt(StaticVariableList, items);
      $$ = list.getRaw();
   }
;

static_var:
   T_VARIABLE {
      TokenSyntax variableToken = make_token_with_text(Variable, $1);
      StaticVariableDeclareSyntax staticVar = make_stmt(StaticVariableDeclare, variableToken, std::nullopt, std::nullopt);
      $$ = staticVar.getRaw();
   }
|  T_VARIABLE T_EQUAL expr {
      TokenSyntax variableToken = make_token_with_text(Variable, $1);
      TokenSyntax equalToken = make_token(EqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      StaticVariableDeclareSyntax staticVar = make_stmt(StaticVariableDeclare, variableToken, equalToken, valueExpr);
      $$ = staticVar.getRaw();
   }
;

class_statement_list:
   class_statement_list class_statement {

   }
|  %empty {

   }
;

class_statement:
   variable_modifiers optional_type property_list T_SEMICOLON {

   }
|  method_modifiers T_CONST class_const_list T_SEMICOLON {

   }
|  T_USE name_list trait_adaptations {

   }
|  method_modifiers function returns_ref identifier backup_doc_comment T_LEFT_PAREN parameter_list T_RIGHT_PAREN
   return_type backup_fn_flags method_body backup_fn_flags {

   }
;

name_list:
   name {

   }
|  name_list T_COMMA name {

   }
;

trait_adaptations:
   T_SEMICOLON {

   }
|  T_LEFT_BRACE T_RIGHT_BRACE {

   }
|  T_LEFT_BRACE trait_adaptation_list T_RIGHT_BRACE {

   }
;

trait_adaptation_list:
   trait_adaptation {

   }
|  trait_adaptation_list trait_adaptation {

   }
;

trait_adaptation:
   trait_precedence T_SEMICOLON {

   }
|  trait_alias T_SEMICOLON {

   }
;

trait_precedence:
   absolute_trait_method_reference T_INSTEADOF name_list {

   }
;

trait_alias:
   trait_method_reference T_AS T_IDENTIFIER_STRING {

   }
|  trait_method_reference T_AS reserved_non_modifiers {

   }
|  trait_method_reference T_AS member_modifier identifier {

   }
|  trait_method_reference T_AS member_modifier {

   }
;

trait_method_reference:
   identifier {

   }
|  absolute_trait_method_reference {
   }
;

absolute_trait_method_reference:
   name T_PAAMAYIM_NEKUDOTAYIM identifier {

   }
;

method_body:
   T_SEMICOLON {

   }
|  T_LEFT_BRACE inner_statement_list T_RIGHT_BRACE {

   }
;

variable_modifiers:
   non_empty_member_modifiers {

   }
|  T_VAR {

   }
;

method_modifiers:
   %empty {

   }
|  non_empty_member_modifiers {

   }
;

non_empty_member_modifiers:
   member_modifier {

   }
|  non_empty_member_modifiers member_modifier {

   }
;

member_modifier:
   T_PUBLIC {

   }
|  T_PROTECTED {

   }
|  T_PRIVATE {

   }
|  T_STATIC {

   }
|  T_ABSTRACT {

   }
|  T_FINAL {

   }
;

property_list:
   property_list T_COMMA property {

   }
|  property {

   }
;

property:
   T_VARIABLE backup_doc_comment {

   }
|  T_VARIABLE T_EQUAL expr backup_doc_comment {

   }
;

class_const_list:
   class_const_list T_COMMA class_const_decl {

   }
|  class_const_decl {

   }
;

class_const_decl:
   identifier T_EQUAL expr backup_doc_comment {

   }
;

const_decl:
   T_IDENTIFIER_STRING T_EQUAL expr backup_doc_comment {
      TokenSyntax identifierToken = make_token_with_text(IdentifierString, $1);
      TokenSyntax equalToken = make_token(EqualToken);
      ExprSyntax expr = make<ExprSyntax>($3);
      InitializerClauseSyntax initializer = make_decl(InitializerClause, equalToken, expr);
      ConstDeclareSyntax constDecl = make_stmt(ConstDeclare, identifierToken, initializer);
      $$ = constDecl.getRaw();
   }
;

echo_expr_list:
   echo_expr_list T_COMMA echo_expr {

   }
|  echo_expr {

   }
;

echo_expr:
   expr {

   }
;

for_exprs:
   %empty {
   }
|  non_empty_for_exprs {

   }
;

non_empty_for_exprs:
   non_empty_for_exprs T_COMMA expr {

   }
|  expr {

   }
;

anonymous_class:
   T_CLASS {} ctor_arguments
   extends_from implements_list backup_doc_comment T_LEFT_BRACE class_statement_list T_RIGHT_BRACE {

   }
;

new_expr:
   T_NEW class_name_reference ctor_arguments {

   }
|  T_NEW anonymous_class {

   }
;

expr:
   variable {

   }
|  T_LIST T_LEFT_PAREN array_pair_list T_RIGHT_PAREN T_EQUAL expr {

   }
|  T_LEFT_SQUARE_BRACKET array_pair_list T_RIGHT_SQUARE_BRACKET T_EQUAL expr {

   }
|  variable T_EQUAL expr {

   }
|  variable T_EQUAL T_AMPERSAND variable {

   }
|  T_CLONE expr {

   }
|  variable T_PLUS_EQUAL expr {

   }
|  variable T_MINUS_EQUAL expr {

   }
|  variable T_MUL_EQUAL expr {

   }
|  variable T_POW_EQUAL expr {

   }
|  variable T_DIV_EQUAL expr {

   }
|  variable T_STR_CONCAT_EQUAL expr {

   }
|  variable T_MOD_EQUAL expr {

   }
|  variable T_AND_EQUAL expr {

   }
|  variable T_OR_EQUAL expr {

   }
|  variable T_XOR_EQUAL expr {

   }
|  variable T_SL_EQUAL expr {

   }
|  variable T_SR_EQUAL expr {

   }
|  variable T_COALESCE_EQUAL expr {

   }
|  variable T_INC {

   }
|  T_INC variable {

   }
|  variable T_DEC {

   }
|  T_DEC variable {

   }
|  expr T_BOOLEAN_OR expr {

   }
|  expr T_BOOLEAN_AND expr {

   }
|  expr T_LOGICAL_OR expr {

   }
|  expr T_LOGICAL_AND expr {

   }
|  expr T_LOGICAL_XOR expr {

   }
|  expr T_VBAR expr {

   }
|  expr T_AMPERSAND expr {

   }
|  expr T_CARET expr {

   }
|  expr T_STR_CONCAT expr {

   }
|  expr T_PLUS_SIGN expr {

   }
|  expr T_MINUS_SIGN expr {

   }
|  expr T_MUL_SIGN expr {

   }
|  expr T_POW expr {

   }
|  expr T_DIV_SIGN expr {

   }
|  expr T_MOD_SIGN expr {

   }
|  expr T_SL expr {

   }
|  expr T_SR expr {

   }
|  T_PLUS_SIGN expr %prec T_INC {

   }
|  T_MINUS_SIGN expr %prec T_INC {

   }
|  T_EXCLAMATION_MARK expr {

   }
|  T_TILDE expr {

   }
|  expr T_IS_IDENTICAL expr {

   }
|  expr T_IS_NOT_IDENTICAL expr {

   }
|  expr T_IS_EQUAL expr {

   }
|  expr T_IS_NOT_EQUAL expr {

   }
|  expr T_IS_SMALLER expr {

   }
|  expr T_IS_SMALLER_OR_EQUAL expr {

   }
|  expr T_IS_GREATER expr {

   }
|  expr T_IS_GREATER_OR_EQUAL expr {

   }
|  expr T_SPACESHIP expr {

   }
|  expr T_INSTANCEOF class_name_reference {

   }
|  T_LEFT_PAREN expr T_RIGHT_PAREN {

   }
|  new_expr {

   }
|  expr T_QUESTION_MARK expr T_COLON expr {

   }
|  expr T_QUESTION_MARK T_COLON expr {

   }
|  expr T_COALESCE expr {

   }
|  internal_functions_in_bison {

   }
|  T_INT_CAST expr {

   }
|  T_DOUBLE_CAST expr {

   }
|  T_STRING_CAST expr {

   }
|  T_ARRAY_CAST expr {

   }
|  T_OBJECT_CAST expr {

   }
|  T_BOOL_CAST expr {

   }
|  T_UNSET_CAST expr {

   }
|  T_EXIT exit_expr {

   }
|  T_ERROR_SUPPRESS_SIGN expr {

   }
|  scalar {
      Syntax scalarValue = make<Syntax>($1);
      ScalarExprSyntax scalar = make_expr(ScalarExpr, scalarValue);
      $$ = scalar.getRaw();
   }
|  T_BACKTICK backticks_expr T_BACKTICK {

   }
|  T_PRINT expr {

   }
|  T_YIELD {

   }
|  T_YIELD expr {

   }
|  T_YIELD expr T_DOUBLE_ARROW expr {

   }
|  T_YIELD_FROM expr {

   }
|  inline_function {

   }
|  T_STATIC inline_function {

   }
;

inline_function:
   function returns_ref backup_doc_comment T_LEFT_PAREN parameter_list T_RIGHT_PAREN lexical_vars return_type
   backup_fn_flags T_LEFT_BRACE inner_statement_list T_RIGHT_BRACE backup_fn_flags {

   }
|  fn returns_ref T_LEFT_PAREN parameter_list T_RIGHT_PAREN return_type backup_doc_comment T_DOUBLE_ARROW
   backup_fn_flags backup_lex_pos expr backup_fn_flags {

   }
;

fn:
   T_FN {

   }
;

function:
   T_FUNCTION {

   }
;

backup_doc_comment:
   %empty {

   }
;

backup_fn_flags:
   %empty %prec PREC_ARROW_FUNCTION {

   }
;

backup_lex_pos:
   %empty {

   }
;

returns_ref:
   %empty {

   }
|  T_AMPERSAND {

   }
;

lexical_vars:
   %empty {

   }
|  T_USE T_LEFT_PAREN lexical_var_list T_RIGHT_PAREN {

   }
;

lexical_var_list:
   lexical_var_list T_COMMA lexical_var {

   }
|  lexical_var {

   }
;

lexical_var:
   T_VARIABLE {

   }
|  T_AMPERSAND T_VARIABLE {

   }
;

function_call:
   name argument_list {

   }
|  class_name T_PAAMAYIM_NEKUDOTAYIM member_name argument_list {

   }
|  variable_class_name T_PAAMAYIM_NEKUDOTAYIM member_name argument_list {

   }
|  callable_expr argument_list {

   }
;

class_name:
   T_STATIC {

   }
|  name {

   }
;

class_name_reference:
   class_name {

   }
|  new_variable {

   }
;

exit_expr:
   %empty {

   }
|  T_LEFT_PAREN optional_expr T_RIGHT_PAREN {

   }
;

backticks_expr:
   %empty {

   }
|  T_ENCAPSED_AND_WHITESPACE {

   }
|  encaps_list {

   }
;

ctor_arguments:
   %empty {

   }
|  argument_list {

   }
;

dereferencable_scalar:
   T_ARRAY T_LEFT_PAREN array_pair_list T_RIGHT_PAREN {

   }
|  T_LEFT_SQUARE_BRACKET array_pair_list T_RIGHT_SQUARE_BRACKET {

   }
|  T_DOUBLE_QUOTE T_CONSTANT_ENCAPSED_STRING T_DOUBLE_QUOTE {

   }
|  T_SINGLE_QUOTE T_CONSTANT_ENCAPSED_STRING T_SINGLE_QUOTE {

   }
;

scalar:
   T_LNUMBER {
      TokenSyntax lval = make_lnumber_token($1);
      $$ = lval.getRaw();
   }
|  T_DNUMBER {
      TokenSyntax dval = make_dnumber_token($1);
      $$ = dval.getRaw();
   }
|  T_LINE {
      TokenSyntax lineKeyword = make_token(LineKeyword);
      $$ = lineKeyword.getRaw();
   }
|  T_FILE {
      TokenSyntax fileKeyword = make_token(FileKeyword);
      $$ = fileKeyword.getRaw();
   }
|  T_DIR {
      TokenSyntax dirKeyword = make_token(DirKeyword);
      $$ = dirKeyword.getRaw();
   }
|  T_TRAIT_CONST {
      TokenSyntax traitConstKeyword = make_token(TraitConstKeyword);
      $$ = traitConstKeyword.getRaw();
   }
|  T_METHOD_CONST {
      TokenSyntax methodConstKeyword = make_token(MethodConstKeyword);
      $$ = methodConstKeyword.getRaw();
   }
|  T_FUNC_CONST {
      TokenSyntax funcConstKeyword = make_token(FuncConstKeyword);
      $$ = funcConstKeyword.getRaw();
   }
|  T_NS_CONST {
      TokenSyntax namespaceConstKeyword = make_token(NamespaceConstKeyword);
      $$ = namespaceConstKeyword.getRaw();
   }
|  T_CLASS_CONST {
      TokenSyntax classConstKeyword = make_token(ClassConstKeyword);
      $$ = classConstKeyword.getRaw();
   }
|  T_START_HEREDOC T_ENCAPSED_AND_WHITESPACE T_END_HEREDOC {

   }
|  T_START_HEREDOC T_END_HEREDOC {

   }
|  T_DOUBLE_QUOTE encaps_list T_DOUBLE_QUOTE {

   }
|  T_START_HEREDOC encaps_list T_END_HEREDOC {

   }
|  dereferencable_scalar {

   }
|  constant {

   }
;

constant:
   name {

   }
|  class_name T_PAAMAYIM_NEKUDOTAYIM identifier {

   }
|  variable_class_name T_PAAMAYIM_NEKUDOTAYIM identifier {

   }
;

optional_expr:
   %empty {

   }
|  expr {

   }
;

variable_class_name:
   dereferencable {

   }
;

dereferencable:
   variable {

   }
|  T_LEFT_PAREN expr T_RIGHT_PAREN {

   }
|  dereferencable_scalar {

   }
;

callable_expr:
   callable_variable {

   }
|  T_LEFT_PAREN expr T_RIGHT_PAREN {

   }
|  dereferencable_scalar {

   }
;

callable_variable:
   simple_variable {

   }
|  dereferencable T_LEFT_SQUARE_BRACKET optional_expr T_RIGHT_SQUARE_BRACKET {

   }
|  constant T_LEFT_SQUARE_BRACKET optional_expr T_RIGHT_SQUARE_BRACKET {

   }
|  dereferencable T_LEFT_BRACE expr T_RIGHT_BRACE {

   }
|  dereferencable T_OBJECT_OPERATOR property_name argument_list {

   }
|  function_call {

   }
;

variable:
   callable_variable {

   }
|  static_member {

   }
|  dereferencable T_OBJECT_OPERATOR property_name {

   }
;

simple_variable:
   T_VARIABLE {

   }
|  T_DOLLAR_SIGN T_LEFT_BRACE expr T_RIGHT_BRACE {

   }
|  T_DOLLAR_SIGN simple_variable {

   }
;

static_member:
   class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable {

   }
|  variable_class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable {

   }
;

new_variable:
   simple_variable {

   }
|  new_variable T_LEFT_SQUARE_BRACKET optional_expr T_RIGHT_SQUARE_BRACKET {

   }
|  new_variable T_LEFT_BRACE expr T_RIGHT_BRACE {

   }
|  new_variable T_OBJECT_OPERATOR property_name {

   }
|  class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable {

   }
|  new_variable T_PAAMAYIM_NEKUDOTAYIM simple_variable {

   }
;

member_name:
   identifier {

   }
|  T_LEFT_BRACE expr T_RIGHT_BRACE {

   }
|  simple_variable {

   }
;

property_name:
   T_IDENTIFIER_STRING {

   }
|  T_LEFT_BRACE expr T_RIGHT_BRACE {

   }
|  simple_variable {

   }
;

array_pair_list:
   non_empty_array_pair_list {

   }
;

possible_array_pair:
   %empty {

   }
|  array_pair {

}
;

non_empty_array_pair_list:
   non_empty_array_pair_list T_COMMA possible_array_pair {

   }
|  possible_array_pair {

   }
;

array_pair:
   expr T_DOUBLE_ARROW expr {

   }
|  expr {

   }
|  expr T_DOUBLE_ARROW T_AMPERSAND variable {

   }
|  T_AMPERSAND variable {

   }
|  T_ELLIPSIS expr {

   }
|  expr T_DOUBLE_ARROW T_LIST T_LEFT_PAREN array_pair_list T_RIGHT_PAREN {

   }
|  T_LIST T_LEFT_PAREN array_pair_list T_RIGHT_PAREN {

   }
;

encaps_list:
   encaps_list encaps_var {

   }
|  encaps_list T_ENCAPSED_AND_WHITESPACE {

   }
|  encaps_var {

   }
|  T_ENCAPSED_AND_WHITESPACE encaps_var {

   }
;

encaps_var:
   T_VARIABLE {

   }
|  T_VARIABLE T_LEFT_SQUARE_BRACKET encaps_var_offset T_RIGHT_SQUARE_BRACKET {

   }
|  T_VARIABLE T_OBJECT_OPERATOR T_IDENTIFIER_STRING {

   }
|  T_DOLLAR_OPEN_CURLY_BRACES expr T_RIGHT_BRACE {

   }
|  T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME T_RIGHT_BRACE {

   }
|  T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME T_LEFT_SQUARE_BRACKET expr T_RIGHT_SQUARE_BRACKET T_RIGHT_BRACE {

   }
|  T_CURLY_OPEN variable T_RIGHT_BRACE {

   }
;

encaps_var_offset:
   T_STRING_VARNAME {

   }
|  T_NUM_STRING {

   }
|  T_MINUS_SIGN T_NUM_STRING {

   }
|  T_VARIABLE {

   }
;

internal_functions_in_bison:
   T_ISSET T_LEFT_PAREN isset_variables possible_comma T_RIGHT_PAREN {

   }
|  T_EMPTY T_LEFT_PAREN expr T_RIGHT_PAREN {

   }
|  T_INCLUDE expr {

   }
|  T_INCLUDE_ONCE expr {

   }
|  T_EVAL T_LEFT_PAREN expr T_RIGHT_PAREN {

   }
|  T_REQUIRE expr {

   }
|  T_REQUIRE_ONCE expr {

   }
;

isset_variables:
   isset_variable {

   }
|  isset_variables T_COMMA isset_variable {

   }
;

isset_variable:
   expr {

   }
;
%%
