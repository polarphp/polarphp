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
%type <RefCountPtr<RawSyntax>> encaps_var isset_variables
%type <RefCountPtr<RawSyntax>> top_statement_list use_declarations const_list inner_statement_list if_stmt
%type <RefCountPtr<RawSyntax>> for_exprs switch_case_list global_var_list static_var_list
%type <RefCountPtr<RawSyntax>> echo_expr_list unset_variables catch_name_list catch_list parameter_list class_statement_list
%type <RefCountPtr<RawSyntax>> implements_list case_list if_stmt_without_else case_separator
%type <RefCountPtr<RawSyntax>> non_empty_parameter_list argument_list non_empty_argument_list property_list
%type <RefCountPtr<RawSyntax>> class_const_list class_const_decl name_list trait_adaptations method_body non_empty_for_exprs
%type <RefCountPtr<RawSyntax>> ctor_arguments trait_adaptation_list lexical_vars
%type <RefCountPtr<RawSyntax>> lexical_var_list encaps_list
%type <RefCountPtr<RawSyntax>> array_pair non_empty_array_pair_list array_pair_list possible_array_pair
%type <RefCountPtr<RawSyntax>> isset_variable type return_type type_expr
%type <RefCountPtr<RawSyntax>> identifier reserved_non_modifiers semi_reserved reserved_non_modifiers_token semi_reserved_token
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

reserved_non_modifiers_token:
     T_INCLUDE { $$ = make_reserved_keyword(Include); }
   | T_INCLUDE_ONCE { $$ = make_reserved_keyword(IncludeOnce); }
   | T_EVAL { $$ = make_reserved_keyword(Eval); }
   | T_REQUIRE { $$ = make_reserved_keyword(Require); }
   | T_REQUIRE_ONCE { $$ = make_reserved_keyword(RequireOnce); }
   | T_LOGICAL_OR { $$ = make_reserved_keyword(LogicOr); }
   | T_LOGICAL_XOR { $$ = make_reserved_keyword(LogicXor); }
   | T_LOGICAL_AND { $$ = make_reserved_keyword(LogicAnd); }
   | T_INSTANCEOF { $$ = make_reserved_keyword(Instanceof); }
   | T_NEW { $$ = make_reserved_keyword(New); }
   | T_CLONE { $$ = make_reserved_keyword(Clone); }
   | T_EXIT { $$ = make_reserved_keyword(Exit); }
   | T_IF { $$ = make_reserved_keyword(If); }
   | T_ELSEIF { $$ = make_reserved_keyword(ElseIf); }
   | T_ELSE { $$ = make_reserved_keyword(Else); }
   | T_ECHO { $$ = make_reserved_keyword(Echo); }
   | T_DO { $$ = make_reserved_keyword(Do); }
   | T_WHILE { $$ = make_reserved_keyword(While); }
   | T_FOR { $$ = make_reserved_keyword(For); }
   | T_FOREACH { $$ = make_reserved_keyword(Foreach); }
   | T_DECLARE { $$ = make_reserved_keyword(Declare); }
   | T_AS { $$ = make_reserved_keyword(As); }
   | T_TRY { $$ = make_reserved_keyword(Try); }
   | T_CATCH { $$ = make_reserved_keyword(Catch); }
   | T_FINALLY { $$ = make_reserved_keyword(Finally); }
   | T_THROW { $$ = make_reserved_keyword(Throw); }
   | T_USE { $$ = make_reserved_keyword(Use); }
   | T_INSTEADOF { $$ = make_reserved_keyword(Insteadof); }
   | T_GLOBAL { $$ = make_reserved_keyword(Global); }
   | T_VAR { $$ = make_reserved_keyword(Var); }
   | T_UNSET { $$ = make_reserved_keyword(Unset); }
   | T_ISSET { $$ = make_reserved_keyword(Isset); }
   | T_EMPTY { $$ = make_reserved_keyword(Empty); }
   | T_CONTINUE { $$ = make_reserved_keyword(Continue); }
   | T_GOTO { $$ = make_reserved_keyword(Goto); }
   | T_FUNCTION { $$ = make_reserved_keyword(Function); }
   | T_CONST { $$ = make_reserved_keyword(Const); }
   | T_RETURN { $$ = make_reserved_keyword(Return); }
   | T_PRINT { $$ = make_reserved_keyword(Print); }
   | T_YIELD { $$ = make_reserved_keyword(Yield); }
   | T_LIST { $$ = make_reserved_keyword(List); }
   | T_SWITCH { $$ = make_reserved_keyword(Switch); }
   | T_CASE { $$ = make_reserved_keyword(Case); }
   | T_DEFAULT { $$ = make_reserved_keyword(Default); }
   | T_BREAK { $$ = make_reserved_keyword(Break); }
   | T_ARRAY { $$ = make_reserved_keyword(Array); }
   | T_CALLABLE { $$ = make_reserved_keyword(Callable); }
   | T_EXTENDS { $$ = make_reserved_keyword(Extends); }
   | T_IMPLEMENTS { $$ = make_reserved_keyword(Implements); }
   | T_NAMESPACE { $$ = make_reserved_keyword(Namespace); }
   | T_TRAIT { $$ = make_reserved_keyword(Trait); }
   | T_INTERFACE { $$ = make_reserved_keyword(Interface); }
   | T_CLASS { $$ = make_reserved_keyword(Class); }
   | T_CLASS_CONST { $$ = make_reserved_keyword(ClassConst); }
   | T_TRAIT_CONST { $$ = make_reserved_keyword(TraitConst); }
   | T_FUNC_CONST { $$ = make_reserved_keyword(FuncConst); }
   | T_METHOD_CONST { $$ = make_reserved_keyword(MethodConst); }
   | T_LINE { $$ = make_reserved_keyword(Line); }
   | T_FILE { $$ = make_reserved_keyword(File); }
   | T_DIR { $$ = make_reserved_keyword(Dir); }
   | T_NS_CONST { $$ = make_reserved_keyword(NamespaceConst); }
   | T_FN { $$ = make_reserved_keyword(Fn); }
;

reserved_non_modifiers:
   reserved_non_modifiers_token {
      TokenSyntax modifierKeyword = make<TokenSyntax>($1);
      ReservedNonModifierSyntax modifier = make_decl(ReservedNonModifier, modifierKeyword);
      $$ = modifier.getRaw();
   }
;

semi_reserved_token:
   reserved_non_modifiers_token { $$ = $1; }
   |  T_STATIC { $$ = make_reserved_keyword(Static); }
   | T_ABSTRACT { $$ = make_reserved_keyword(Abstract); }
   | T_FINAL { $$ = make_reserved_keyword(Final); }
   | T_PRIVATE { $$ = make_reserved_keyword(Private); }
   | T_PROTECTED { $$ = make_reserved_keyword(Protected); }
   | T_PUBLIC { $$ = make_reserved_keyword(Public); }
;

semi_reserved:
   semi_reserved_token {
      TokenSyntax reservedKeyword = make<TokenSyntax>($1);
      SemiReservedSytnax semiReserved = make_decl(SemiReserved, reservedKeyword);
      $$ = semiReserved.getRaw();
   }
;

identifier:
   T_IDENTIFIER_STRING {
      TokenSyntax identifierToken = make_token_with_text(IdentifierString, $1);
      IdentifierSyntax identifier = make_decl(Identifier, identifierToken);
      $$ = identifier.getRaw();
   }
|	semi_reserved {
      SemiReservedSytnax reserved = make<SemiReservedSytnax>($1);
      IdentifierSyntax identifier = make_decl(Identifier, reserved);
      $$ = identifier.getRaw();
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
      TokenSyntax semicolonToken = make_token(SemicolonToken);
      HaltCompilerStmtSyntax stmt = make_stmt(HaltCompilerStmt, haltCompilerToken, leftParenToken, rightParenToken, semicolonToken);
      $$ = stmt.getRaw();
   }
;

statement:
   T_LEFT_BRACE inner_statement_list T_RIGHT_BRACE {

   }
|  if_stmt {
      $$ = $1;
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
      TokenSyntax tryKeyword = make_token(TryKeyword);
      TokenSyntax leftBrace = make_token(LeftBraceToken);
      InnerStmtListSyntax innerStmts = make<InnerStmtListSyntax>($3);
      TokenSyntax rightBrace = make_token(RightBraceToken);
      InnerCodeBlockStmtSyntax tryCodeBlcok = make_stmt(InnerCodeBlockStmt, leftBrace, innerStmts, rightBrace);
      CatchListSyntax catchList = make<CatchListSyntax>($5);
      std::optional<FinallyClauseSyntax> finallyClause = $6 ? std::optional(make<FinallyClauseSyntax>($6)) : std::nullopt;
      TryStmtSyntax tryStmt = make_stmt(TryStmt, tryKeyword, tryCodeBlcok, catchList, finallyClause);
      $$ = tryStmt.getRaw();
   }
|  T_THROW expr T_SEMICOLON {
      TokenSyntax throwKeyword = make_token(ThrowKeyword);
      ExprSyntax expr = make<ExprSyntax>($2);
      TokenSyntax colonToken = make_token(ColonToken);
      ThrowStmtSyntax throwStmt = make_stmt(ThrowStmt, throwKeyword, expr, colonToken);
      $$ = throwStmt.getRaw();
   }
|  T_GOTO T_IDENTIFIER_STRING T_SEMICOLON {
      TokenSyntax gotoKeyword = make_token(GotoKeyword);
      TokenSyntax identifierStr = make_token_with_text(IdentifierString, $2);
      TokenSyntax colonToken = make_token(ColonToken);
      GotoStmtSyntax gotoStmt = make_stmt(GotoStmt, gotoKeyword, identifierStr, colonToken);
      $$ = gotoStmt.getRaw();
   }
|  T_IDENTIFIER_STRING T_COLON {
      TokenSyntax identifierStr = make_token_with_text(IdentifierString, $1);
      TokenSyntax colonToken = make_token(ColonToken);
      LabelStmtSyntax labelStmt = make_stmt(LabelStmt, identifierStr, colonToken);
      $$ = labelStmt.getRaw();
   }
;

catch_list:
   %empty {
      CatchListSyntax list = make_blank_stmt(CatchList);
      $$ = list.getRaw();
   }
|  catch_list T_CATCH T_LEFT_PAREN catch_name_list T_VARIABLE T_RIGHT_PAREN T_LEFT_BRACE inner_statement_list T_RIGHT_BRACE {
      CatchListSyntax list = make<CatchListSyntax>($1);
      TokenSyntax catchKeyword = make_token(CatchKeyword);
      TokenSyntax leftParen = make_token(LeftParenToken);
      CatchArgTypeHintListSyntax typeHints = make<CatchArgTypeHintListSyntax>($4);
      TokenSyntax variableToken = make_token_with_text(Variable, $5);
      TokenSyntax rightParen = make_token(RightParenToken);
      TokenSyntax leftBrace = make_token(LeftBraceToken);
      InnerStmtListSyntax innerStmts = make<InnerStmtListSyntax>($8);
      TokenSyntax rightBrace = make_token(RightBraceToken);
      InnerCodeBlockStmtSyntax catchHandlerCodeBlock = make_stmt(InnerCodeBlockStmt, leftBrace, innerStmts, rightBrace);
      CatchListItemClauseSyntax catchClause = make_stmt(CatchListItemClause, 
         catchKeyword, leftParen, typeHints, variableToken, rightParen, catchHandlerCodeBlock
      );
      list.appending(catchClause);
      $$ = list.getRaw();
   }
;

catch_name_list:
   name {
      NameSyntax name = make<NameSyntax>($1);
      CatchArgTypeHintItemSyntax typeHintItem = make_stmt(CatchArgTypeHintItem, std::nullopt, name);
      std::vector<CatchArgTypeHintItemSyntax> hits{typeHintItem};
      CatchArgTypeHintListSyntax list = make_stmt(CatchArgTypeHintList, hits);
      $$ = list.getRaw();
   }
|  catch_name_list T_VBAR name {
      CatchArgTypeHintListSyntax list = make<CatchArgTypeHintListSyntax>($1);
      TokenSyntax vbarToken = make_token(VerticalBarToken);
      NameSyntax name = make<NameSyntax>($3);
      CatchArgTypeHintItemSyntax typeHintItem = make_stmt(CatchArgTypeHintItem, vbarToken, name);
      list.appending(typeHintItem);
      $$ = list.getRaw();
   }
;

finally_statement:
   %empty {
      $$ = nullptr;
   }
|  T_FINALLY T_LEFT_BRACE inner_statement_list T_RIGHT_BRACE {
      TokenSyntax finallyKeyword = make_token(FinallyKeyword);
      TokenSyntax leftBrace = make_token(LeftBraceToken);
      InnerStmtListSyntax innerStmts = make<InnerStmtListSyntax>($3);
      TokenSyntax rightBrace = make_token(RightBraceToken);
      InnerCodeBlockStmtSyntax catchHandlerCodeBlock = make_stmt(InnerCodeBlockStmt, leftBrace, innerStmts, rightBrace);
      FinallyClauseSyntax finallyClause = make_stmt(FinallyClause, finallyKeyword, catchHandlerCodeBlock);
      $$ = finallyClause.getRaw();
   }
;

unset_variables:
   unset_variable {
      UnsetVariableSyntax unsetVar = make<UnsetVariableSyntax>($1);
      UnsetVariableListItemSyntax listItem = make_stmt(UnsetVariableListItem, std::nullopt, unsetVar);
      std::vector<UnsetVariableListItemSyntax> vars{listItem};
      UnsetVariableListSyntax list = make_stmt(UnsetVariableList, vars);
      $$ = list.getRaw();
   }
|  unset_variables T_COMMA unset_variable {
      UnsetVariableListSyntax list = make<UnsetVariableListSyntax>($1);
      TokenSyntax comma = make_token(CommaToken);
      UnsetVariableSyntax unsetVar = make<UnsetVariableSyntax>($3);
      UnsetVariableListItemSyntax listItem = make_stmt(UnsetVariableListItem, comma, unsetVar);
      list.appending(listItem);
      $$ = list.getRaw();
   }
;

unset_variable:
   variable {
      VariableExprSyntax variable = make<VariableExprSyntax>($1);
      UnsetVariableSyntax unsetVar = make_stmt(UnsetVariable, variable);
      $$ = unsetVar.getRaw();
   }
;

function_declaration_statement:
   function returns_ref T_IDENTIFIER_STRING backup_doc_comment T_LEFT_PAREN parameter_list T_RIGHT_PAREN return_type
   backup_fn_flags T_LEFT_BRACE inner_statement_list T_RIGHT_BRACE backup_fn_flags {

   }
;

is_reference:
   %empty {
      $$ = nullptr;
   }
|  T_AMPERSAND {
      TokenSyntax ampersand = make_token(AmpersandToken);
      $$ = ampersand.getRaw();
   }
;

is_variadic:
   %empty {
      $$ = nullptr;
   }
|  T_ELLIPSIS {
      TokenSyntax ellipsis = make_token(EllipsisToken);
      $$ = ellipsis.getRaw();
   }
;

class_declaration_statement:
   class_modifiers T_CLASS  { }
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
      $$ = nullptr;
   }
|  T_EXTENDS name {
      TokenSyntax extendsKeyword = make_token(ExtendsKeyword);
      NameSyntax name = make<NameSyntax>($2);
      ExtendsFromClauseSyntax extendClause = make_decl(ExtendsFromClause, extendsKeyword, name);
      $$ = extendClause.getRaw();
   }
;

interface_extends_list:
   %empty {
      $$ = nullptr;
   }
|  T_EXTENDS name_list {
      TokenSyntax extendsKeyword = make_token(ExtendsKeyword);
      NameListSyntax names = make<NameListSyntax>($2);
      InterfaceExtendsClauseSyntax interfaceExtendsClause = make_decl(InterfaceExtendsClause, extendsKeyword, names);
      $$ = interfaceExtendsClause.getRaw();
   }
;

implements_list:
   %empty {
      $$ = nullptr;
   }
|  T_IMPLEMENTS name_list {
      TokenSyntax implementsKeyword = make_token(ImplementsKeyword);
      NameListSyntax names = make<NameListSyntax>($2);
      ImplementsClauseSyntax implementsClause = make_decl(ImplementsClause, implementsKeyword, names);
      $$ = implementsClause.getRaw();
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
;

case_list:
   %empty {
      SwitchCaseListSyntax list = make_blank_stmt(SwitchCaseList);
      $$ = list.getRaw();
   }
|  case_list T_CASE expr case_separator inner_statement_list {
      SwitchCaseListSyntax list = make<SwitchCaseListSyntax>($1);
      TokenSyntax caseKeyword = make_token(CaseKeyword);
      ExprSyntax expr = make<ExprSyntax>($3);
      TokenSyntax separator = make<TokenSyntax>($4);
      InnerStmtListSyntax stmts = make<InnerStmtListSyntax>($5);
      SwitchCaseLabelSyntax label = make_stmt(SwitchCaseLabel, caseKeyword, expr, separator);
      SwitchCaseSyntax caseItem = make_stmt(SwitchCase, label, stmts);
      list.appending(caseItem);
      $$ = list.getRaw();
   }
|  case_list T_DEFAULT case_separator inner_statement_list {
      SwitchCaseListSyntax list = make<SwitchCaseListSyntax>($1);
      TokenSyntax caseKeyword = make_token(DefaultKeyword);
      TokenSyntax separator = make<TokenSyntax>($3);
      InnerStmtListSyntax stmts = make<InnerStmtListSyntax>($4);
      SwitchDefaultLabelSyntax label = make_stmt(SwitchDefaultLabel, caseKeyword, separator);
      SwitchCaseSyntax caseItem = make_stmt(SwitchCase, label, stmts);
      list.appending(caseItem);
      $$ = list.getRaw();
   }
;

case_separator:
   T_COLON {
      TokenSyntax colon = make_token(ColonToken);
      $$ = colon.getRaw();
   }
;

if_stmt_without_else:
   T_IF T_LEFT_PAREN expr T_RIGHT_PAREN statement {
      TokenSyntax ifKeyword = make_token(IfKeyword);
      TokenSyntax leftParen = make_token(LeftParenToken);
      ExprSyntax expr = make<ExprSyntax>($3);
      TokenSyntax rightParen = make_token(RightParenToken);
      StmtSyntax stmt = make<StmtSyntax>($5);
      ElseIfListSyntax elseIfList = make_blank_stmt(ElseIfList);
      IfStmtSyntax ifStmt = make_stmt(
         IfStmt, std::nullopt, std::nullopt, ifKeyword, leftParen, expr, rightParen, 
         stmt, elseIfList, std::nullopt, std::nullopt
      );
      $$ = ifStmt.getRaw();
   }
|  if_stmt_without_else T_ELSEIF T_LEFT_PAREN expr T_RIGHT_PAREN statement {
      IfStmtSyntax ifStmt = make<IfStmtSyntax>($1);
      TokenSyntax elseIfKeyword = make_token(ElseIfKeyword);
      TokenSyntax leftParen = make_token(LeftParenToken);
      ExprSyntax expr = make<ExprSyntax>($4);
      TokenSyntax rightParen = make_token(RightParenToken);
      StmtSyntax stmt = make<StmtSyntax>($6);
      ElseIfClauseSyntax elseIfClause = make_stmt(
         ElseIfClause, elseIfKeyword, leftParen, expr, rightParen, stmt
      );
      ifStmt.addElseIfClause(elseIfClause);
      $$ = ifStmt.getRaw();
   }
;

if_stmt:
   if_stmt_without_else %prec T_NOELSE {
      $$ = $1;
   }
|  if_stmt_without_else T_ELSE statement {
      IfStmtSyntax ifStmt = make<IfStmtSyntax>($1);
      TokenSyntax elseKeyword = make_token(ElseKeyword);
      StmtSyntax stmt = make<StmtSyntax>($3);
      ifStmt.withElseKeyword(elseKeyword);
      ifStmt.withElseBody(stmt);
      $$ = ifStmt.getRaw();
   }
;

parameter_list:
   non_empty_parameter_list {
      $$ = $1;
   }
|  %empty {
      $$ = nullptr;
   }
;

non_empty_parameter_list:
   parameter {
      ParameterSyntax param = make<ParameterSyntax>($1);
      ParameterListItemSyntax paramListItem = make_decl(ParameterListItem, std::nullopt, param);
      std::vector<ParameterListItemSyntax> items{paramListItem};
      ParameterListSyntax list = make_decl(ParameterList, items);
      $$ = list.getRaw();
   }
|  non_empty_parameter_list T_COMMA parameter {
      ParameterListSyntax list = make<ParameterListSyntax>($1);
      TokenSyntax comma = make_token(CommaToken);
      ParameterSyntax param = make<ParameterSyntax>($3);
      ParameterListItemSyntax paramListItem = make_decl(ParameterListItem, comma, param);
      list.appending(paramListItem);
      $$ = list.getRaw();
   }
;

parameter:
   optional_type is_reference is_variadic T_VARIABLE {
      std::optional<TypeExprClauseSyntax> optionalType = $1 ? std::optional(make<TypeExprClauseSyntax>($1)) : std::nullopt;
      std::optional<TokenSyntax> refToken = $2 ? std::optional(make<TokenSyntax>($2)) : std::nullopt;
      std::optional<TokenSyntax> variadicToken = $3 ? std::optional(make<TokenSyntax>($3)) : std::nullopt;
      TokenSyntax variable = make_token_with_text(Variable, $4);
      ParameterSyntax parameterDecl = make_decl(Parameter, optionalType, refToken, variadicToken, variable, std::nullopt);
      $$ = parameterDecl.getRaw();
   }
|  optional_type is_reference is_variadic T_VARIABLE T_EQUAL expr {
      std::optional<TypeExprClauseSyntax> optionalType = $1 ? std::optional(make<TypeExprClauseSyntax>($1)) : std::nullopt;
      std::optional<TokenSyntax> refToken = $2 ? std::optional(make<TokenSyntax>($2)) : std::nullopt;
      std::optional<TokenSyntax> variadicToken = $3 ? std::optional(make<TokenSyntax>($3)) : std::nullopt;
      TokenSyntax variable = make_token_with_text(Variable, $4);
      TokenSyntax equal = make_token(EqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($6);
      InitializerClauseSyntax initializer = make_decl(InitializerClause, equal, valueExpr);
      ParameterSyntax parameterDecl = make_decl(Parameter, optionalType, refToken, variadicToken, variable, initializer);
      $$ = parameterDecl.getRaw();
   }
;

optional_type:
   %empty {
      $$ = nullptr;
   }
|  type_expr {
      TypeExprClauseSyntax typeExpr = make<TypeExprClauseSyntax>($1);
      $$ = typeExpr.getRaw();
   }
;

type_expr:
   type {
      TypeClauseSyntax type = make<TypeClauseSyntax>($1);
      TypeExprClauseSyntax typeExpr = make_decl(TypeExprClause, std::nullopt, type);
      $$ = typeExpr.getRaw();
   }
|  T_QUESTION_MARK type {
      TokenSyntax questionMark = make_token(QuestionMarkToken);
      TypeClauseSyntax type = make<TypeClauseSyntax>($2);
      TypeExprClauseSyntax typeExpr = make_decl(TypeExprClause, questionMark, type);
      $$ = typeExpr.getRaw();
   }
;

type:
   T_ARRAY {
      TokenSyntax arrayToken = make_token(ArrayKeyword);
      TypeClauseSyntax type = make_decl(TypeClause, arrayToken);
      $$ = type.getRaw();
   }
|  T_CALLABLE {
      TokenSyntax callableToken = make_token(CallableKeyword);
      TypeClauseSyntax type = make_decl(TypeClause, callableToken);
      $$ = type.getRaw();
   }
|  name {
      NameSyntax name = make<NameSyntax>($1);
      TypeClauseSyntax type = make_decl(TypeClause, name);
      $$ = type.getRaw();
   }
;

return_type:
   %empty {
      $$ = nullptr;
   }
|  T_COLON type_expr {
      TokenSyntax colon = make_token(ColonToken);
      TypeExprClauseSyntax typeExpr = make<TypeExprClauseSyntax>($2);
      ReturnTypeClauseSyntax returnType = make_decl(ReturnTypeClause, colon, typeExpr);
      $$ = returnType.getRaw();
   }
;

argument_list:
   T_LEFT_PAREN T_RIGHT_PAREN {
      TokenSyntax leftParen = make_token(LeftParenToken);
      TokenSyntax rightParen = make_token(RightParenToken);
      ArgumentListClauseSyntax argumentListClause = make_expr(ArgumentListClause, leftParen, std::nullopt, rightParen);
      $$ = argumentListClause.getRaw();
   }
|  T_LEFT_PAREN non_empty_argument_list possible_comma T_RIGHT_PAREN {
      TokenSyntax leftParen = make_token(LeftParenToken);
      ArgumentListSyntax args = make<ArgumentListSyntax>($2);
      TokenSyntax rightParen = make_token(RightParenToken);
      ArgumentListClauseSyntax argumentListClause = make_expr(ArgumentListClause, leftParen, args, rightParen);
      $$ = argumentListClause.getRaw();
   }
;

non_empty_argument_list:
   argument {
      ArgumentSyntax argument = make<ArgumentSyntax>($1);
      ArgumentListItemSyntax argumnetListItem = make_expr(ArgumentListItem, std::nullopt, argument);
      std::vector<ArgumentListItemSyntax> items{argumnetListItem};
      ArgumentListSyntax list = make_expr(ArgumentList, items);
      $$ = list.getRaw();
   }
|  non_empty_argument_list T_COMMA argument {
      ArgumentListSyntax list = make<ArgumentListSyntax>($1);
      TokenSyntax comma = make_token(CommaToken);
      ArgumentSyntax argument = make<ArgumentSyntax>($3);
      ArgumentListItemSyntax argumnetListItem = make_expr(ArgumentListItem, comma, argument);
      list.appending(argumnetListItem);
      $$ = list.getRaw();
   }
;

argument:
   expr {
      ExprSyntax expr = make<ExprSyntax>($1);
      ArgumentSyntax argument = make_expr(Argument, std::nullopt, expr);
      $$ = argument.getRaw();
   }
|  T_ELLIPSIS expr {
      TokenSyntax ellipsisToken = make_token(EllipsisToken);
      ExprSyntax expr = make<ExprSyntax>($2);
      ArgumentSyntax argument = make_expr(Argument, ellipsisToken, expr);
      $$ = argument.getRaw();
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
      MemberDeclListSyntax list = make<MemberDeclListSyntax>($1);
      MemberDeclListItemSyntax classStmt = make<MemberDeclListItemSyntax>($2);
      list.appending(classStmt);
      $$ = list.getRaw();
   }
|  %empty {
      MemberDeclListSyntax list = make_blank_decl(MemberDeclList);
      $$ = list.getRaw();
   }
;

class_statement:
   variable_modifiers optional_type property_list T_SEMICOLON {
      MemberModifierListSyntax modifiers = make<MemberModifierListSyntax>($1);
      std::optional<TypeExprClauseSyntax> optionalType = $2 ? std::optional(make<TypeExprClauseSyntax>($1)) : std::nullopt;
      ClassPropertyListSyntax propList = make<ClassPropertyListSyntax>($3);
      ClassPropertyDeclSyntax decl = make_decl(
         ClassPropertyDecl, modifiers, optionalType, propList
      );
      TokenSyntax semicolon = make_token(SemicolonToken);
      MemberDeclListItemSyntax declStmt = make_decl(MemberDeclListItem, decl, semicolon);
      $$ = declStmt.getRaw();
   }
|  method_modifiers T_CONST class_const_list T_SEMICOLON {
      MemberModifierListSyntax modifiers = make<MemberModifierListSyntax>($1);
      TokenSyntax constKeyword = make_token(ConstKeyword);
      ClassConstListSyntax list = make<ClassConstListSyntax>($1);
      ClassConstDeclSyntax decl = make_decl(ClassConstDecl, modifiers, constKeyword, list);
      TokenSyntax semicolon = make_token(SemicolonToken);
      MemberDeclListItemSyntax declStmt = make_decl(MemberDeclListItem, decl, semicolon);
      $$ = declStmt.getRaw();
   }
|  T_USE name_list trait_adaptations {
      TokenSyntax useKeyword = make_token(UseKeyword);
      NameListSyntax names = make<NameListSyntax>($2);
      RefCountPtr<RawSyntax> rawSyntax = $3;
      RefCountPtr<RawSyntax> rawSemicolon = nullptr;
      RefCountPtr<RawSyntax> rawAdaptations = nullptr;
      if (rawSyntax->kindOf(SyntaxKind::ClassTraitAdaptationBlock)) {
         rawAdaptations = rawSyntax;
      } else if (rawSyntax->isToken()) {
         rawSemicolon = rawSyntax;
      }
      ClassTraitDeclSyntax decl = make_decl(
         ClassTraitDecl, useKeyword, names, rawAdaptations ? std::optional(make<ClassTraitAdaptationBlockSyntax>(rawSyntax)) : std::nullopt);
      MemberDeclListItemSyntax declStmt = make_decl(
         MemberDeclListItem, decl, rawSemicolon ? std::optional(make<TokenSyntax>(rawSemicolon)) : std::nullopt);
      $$ = declStmt.getRaw();
   }
|  method_modifiers function returns_ref identifier backup_doc_comment T_LEFT_PAREN parameter_list T_RIGHT_PAREN
   return_type backup_fn_flags method_body backup_fn_flags {
      MemberModifierListSyntax modifiers = make<MemberModifierListSyntax>($1);
      TokenSyntax funcKeyword = make<TokenSyntax>($2);
      std::optional<TokenSyntax> returnRef = $3 ? std::optional(make<TokenSyntax>($3)) : std::nullopt;
      IdentifierSyntax identifier = make<IdentifierSyntax>($4);
      TokenSyntax leftParen = make_token(LeftParenToken);
      ParameterListSyntax params = make<ParameterListSyntax>($7);
      TokenSyntax rightParen = make_token(RightParenToken);
      ParameterClauseSyntax paramsClause = make_decl(ParameterClause, leftParen, params, rightParen);
      std::optional<ReturnTypeClauseSyntax> returnType = $9 ? std::optional(make<ReturnTypeClauseSyntax>($9)) : std::nullopt;
      RefCountPtr<RawSyntax> rawSyntax = $11;
      RefCountPtr<RawSyntax> rawSemicolon = nullptr;
      RefCountPtr<RawSyntax> rawBody = nullptr;
      if (rawSyntax->kindOf(SyntaxKind::InnerCodeBlockStmt)) {
         rawBody = rawSyntax;
      } else if (rawSyntax->isToken()) {
         rawSemicolon = rawSyntax;
      }
      ClassMethodDeclSyntax methodDecl = make_decl(
         ClassMethodDecl, modifiers, funcKeyword, returnRef, identifier, 
         paramsClause, returnType, rawBody ? std::optional(make<InnerCodeBlockStmtSyntax>(rawBody)) : std::nullopt
      );
      MemberDeclListItemSyntax declStmt = make_decl(
         MemberDeclListItem, methodDecl, rawSemicolon ? std::optional(make<TokenSyntax>(rawSemicolon)) : std::nullopt);
      $$ = declStmt.getRaw();
   }
;

name_list:
   name {
      NameSyntax name = make<NameSyntax>($1);
      NameListItemSyntax listItem = make_decl(NameListItem, std::nullopt, name);
      std::vector<NameListItemSyntax> names{listItem};
      NameListSyntax list = make_decl(NameList, names);
      $$ = list.getRaw();
   }
|  name_list T_COMMA name {
      NameListSyntax list = make<NameListSyntax>($1);
      TokenSyntax comma = make_token(CommaToken);
      NameSyntax name = make<NameSyntax>($3);
      NameListItemSyntax listItem = make_decl(NameListItem, comma, name);
      list.appending(listItem);
      $$ = list.getRaw();
   }
;

trait_adaptations:
   T_SEMICOLON {
      TokenSyntax semicolon = make_token(SemicolonToken);
      $$ = semicolon.getRaw();
   }
|  T_LEFT_BRACE T_RIGHT_BRACE {
      TokenSyntax leftParen = make_token(LeftBraceToken);
      TokenSyntax rightParen = make_token(RightBraceToken);
      ClassTraitAdaptationBlockSyntax adaptationBlock = make_decl(
         ClassTraitAdaptationBlock, leftParen, std::nullopt, rightParen
      );
      $$ = adaptationBlock.getRaw();
   }
|  T_LEFT_BRACE trait_adaptation_list T_RIGHT_BRACE {
      TokenSyntax leftParen = make_token(LeftBraceToken);
      ClassTraitAdaptationListSyntax list = make<ClassTraitAdaptationListSyntax>($2);
      TokenSyntax rightParen = make_token(RightBraceToken);
      ClassTraitAdaptationBlockSyntax adaptationBlock = make_decl(
         ClassTraitAdaptationBlock, leftParen, list, rightParen
      );
      $$ = adaptationBlock.getRaw();
   }
;

trait_adaptation_list:
   trait_adaptation {
      ClassTraitAdaptationSyntax adaptation = make<ClassTraitAdaptationSyntax>($1);
      std::vector<ClassTraitAdaptationSyntax> items{adaptation};
      ClassTraitAdaptationListSyntax list = make_decl(
         ClassTraitAdaptationList, items
      );
      $$ = list.getRaw();
   }
|  trait_adaptation_list trait_adaptation {
      ClassTraitAdaptationListSyntax list = make<ClassTraitAdaptationListSyntax>($1);
      ClassTraitAdaptationSyntax adaptation = make<ClassTraitAdaptationSyntax>($2);
      list.appending(adaptation);
      $$ = list.getRaw();
   }
;

trait_adaptation:
   trait_precedence T_SEMICOLON {
      ClassTraitPrecedenceSyntax traitPrecedence = make<ClassTraitPrecedenceSyntax>($1);
      TokenSyntax semicolon = make_token(SemicolonToken);
      ClassTraitAdaptationSyntax adaptation = make_decl(
         ClassTraitAdaptation, traitPrecedence, semicolon);
      $$ = adaptation.getRaw();
   }
|  trait_alias T_SEMICOLON {
      ClassTraitAliasSyntax traitAlias = make<ClassTraitAliasSyntax>($1);
      TokenSyntax semicolon = make_token(SemicolonToken);
      ClassTraitAdaptationSyntax adaptation = make_decl(
         ClassTraitAdaptation, traitAlias, semicolon);
      $$ = adaptation.getRaw();
   }
;

trait_precedence:
   absolute_trait_method_reference T_INSTEADOF name_list {
      ClassAbsoluteTraitMethodReferenceSyntax absoluteMethodRef = make<ClassAbsoluteTraitMethodReferenceSyntax>($1);
      TokenSyntax insteadofKeyword = make_token(InsteadofKeyword);
      NameListSyntax names = make<NameListSyntax>($3);
      ClassTraitPrecedenceSyntax traitPrecedence = make_decl(
         ClassTraitPrecedence, absoluteMethodRef, insteadofKeyword, names
      );
      $$ = traitPrecedence.getRaw();
   }
;

trait_alias:
   trait_method_reference T_AS T_IDENTIFIER_STRING {
      ClassTraitMethodReferenceSyntax methodRef = make<ClassTraitMethodReferenceSyntax>($1);
      TokenSyntax asKeyword = make_token(AsKeyword);
      TokenSyntax aliasName = make_token_with_text(IdentifierString, $3);
      ClassTraitAliasSyntax traitAlias = make_decl(
         ClassTraitAlias, methodRef, asKeyword, std::nullopt, aliasName
      );
      $$ = traitAlias.getRaw();
   }
|  trait_method_reference T_AS reserved_non_modifiers {
      ClassTraitMethodReferenceSyntax methodRef = make<ClassTraitMethodReferenceSyntax>($1);
      TokenSyntax asKeyword = make_token(AsKeyword);
      ReservedNonModifierSyntax aliasName = make<ReservedNonModifierSyntax>($3);
      ClassTraitAliasSyntax traitAlias = make_decl(
         ClassTraitAlias, methodRef, asKeyword, std::nullopt, aliasName
      );
      $$ = traitAlias.getRaw();
   }
|  trait_method_reference T_AS member_modifier identifier {
      ClassTraitMethodReferenceSyntax methodRef = make<ClassTraitMethodReferenceSyntax>($1);
      TokenSyntax asKeyword = make_token(AsKeyword);
      IdentifierSyntax aliasName = make<IdentifierSyntax>($4);
      MemberModifierSyntax modifier = make<MemberModifierSyntax>($3);
      ClassTraitAliasSyntax traitAlias = make_decl(
         ClassTraitAlias, methodRef, asKeyword, modifier, aliasName
      );
      $$ = traitAlias.getRaw();
   }
|  trait_method_reference T_AS member_modifier {
      ClassTraitMethodReferenceSyntax methodRef = make<ClassTraitMethodReferenceSyntax>($1);
      TokenSyntax asKeyword = make_token(AsKeyword);
      MemberModifierSyntax modifier = make<MemberModifierSyntax>($3);
      ClassTraitAliasSyntax traitAlias = make_decl(
         ClassTraitAlias, methodRef, asKeyword, modifier, std::nullopt
      );
      $$ = traitAlias.getRaw();
   }
;

trait_method_reference:
   identifier {
      IdentifierSyntax identifier = make<IdentifierSyntax>($1);
      ClassTraitMethodReferenceSyntax methodRef = make_decl(
         ClassTraitMethodReference, identifier
      );
      $$ = methodRef.getRaw();
   }
|  absolute_trait_method_reference {
      ClassAbsoluteTraitMethodReferenceSyntax absoluteMethodRef = make<ClassAbsoluteTraitMethodReferenceSyntax>($1);
      ClassTraitMethodReferenceSyntax methodRef = make_decl(
         ClassTraitMethodReference, absoluteMethodRef
      );
      $$ = methodRef.getRaw();
   }
;

absolute_trait_method_reference:
   name T_PAAMAYIM_NEKUDOTAYIM identifier {
      NameSyntax name = make<NameSyntax>($1);
      TokenSyntax paamayimNekudotayimToken = make_token(PaamayimNekudotayimToken);
      IdentifierSyntax identifier = make<IdentifierSyntax>($3);
      ClassAbsoluteTraitMethodReferenceSyntax traitMethodRef = make_decl(
         ClassAbsoluteTraitMethodReference, name, paamayimNekudotayimToken, identifier
      );
      $$ = traitMethodRef.getRaw();
   }
;

method_body:
   T_SEMICOLON {
      TokenSyntax semicolon = make_token(SemicolonToken);
      $$ = semicolon.getRaw();
   }
|  T_LEFT_BRACE inner_statement_list T_RIGHT_BRACE {
      TokenSyntax leftBrace = make_token(LeftBraceToken);
      InnerStmtListSyntax stmts = make<InnerStmtListSyntax>($2);
      TokenSyntax rightBrace = make_token(RightBraceToken);
      InnerCodeBlockStmtSyntax innerStmtCodeBlock = make_stmt(InnerCodeBlockStmt, leftBrace, stmts, rightBrace);
      $$ = innerStmtCodeBlock.getRaw();
   }
;

variable_modifiers:
   non_empty_member_modifiers {
      $$ = $1;
   }
|  T_VAR {
      TokenSyntax modifierKeyword = make_token(PublicKeyword);
      MemberModifierSyntax memberModifier = make_decl(MemberModifier, modifierKeyword);
      std::vector<MemberModifierSyntax> items{memberModifier};
      MemberModifierListSyntax list = make_decl(MemberModifierList, items);
      $$ = list.getRaw();
   }
;

method_modifiers:
   %empty {
      TokenSyntax modifierKeyword = make_token(PublicKeyword);
      MemberModifierSyntax memberModifier = make_decl(MemberModifier, modifierKeyword);
      std::vector<MemberModifierSyntax> items{memberModifier};
      MemberModifierListSyntax list = make_decl(MemberModifierList, items);
      $$ = list.getRaw();
   }
|  non_empty_member_modifiers {
      $$ = $1;
   }
;

non_empty_member_modifiers:
   member_modifier {
      MemberModifierSyntax modifier = make<MemberModifierSyntax>($1);
      std::vector<MemberModifierSyntax> items{modifier};
      MemberModifierListSyntax list = make_decl(MemberModifierList, items);
      $$ = list.getRaw();
   }
|  non_empty_member_modifiers member_modifier {
      MemberModifierListSyntax list = make<MemberModifierListSyntax>($1);
      MemberModifierSyntax modifier = make<MemberModifierSyntax>($2);
      list.appending(modifier);
      $$ = list.getRaw();
   }
;

member_modifier:
   T_PUBLIC {
      TokenSyntax modifierKeyword = make_token(PublicKeyword);
      MemberModifierSyntax memberModifier = make_decl(MemberModifier, modifierKeyword);
      $$ = memberModifier.getRaw();
   }
|  T_PROTECTED {
      TokenSyntax modifierKeyword = make_token(ProtectedKeyword);
      MemberModifierSyntax memberModifier = make_decl(MemberModifier, modifierKeyword);
      $$ = memberModifier.getRaw();
   }
|  T_PRIVATE {
      TokenSyntax modifierKeyword = make_token(PrivateKeyword);
      MemberModifierSyntax memberModifier = make_decl(MemberModifier, modifierKeyword);
      $$ = memberModifier.getRaw();
   }
|  T_STATIC {
      TokenSyntax modifierKeyword = make_token(StaticKeyword);
      MemberModifierSyntax memberModifier = make_decl(MemberModifier, modifierKeyword);
      $$ = memberModifier.getRaw();
   }
|  T_ABSTRACT {
      TokenSyntax modifierKeyword = make_token(AbstractKeyword);
      MemberModifierSyntax memberModifier = make_decl(MemberModifier, modifierKeyword);
      $$ = memberModifier.getRaw();
   }
|  T_FINAL {
      TokenSyntax modifierKeyword = make_token(FinalKeyword);
      MemberModifierSyntax memberModifier = make_decl(MemberModifier, modifierKeyword);
      $$ = memberModifier.getRaw();
   }
;

property_list:
   property_list T_COMMA property {
      ClassPropertyListSyntax list = make<ClassPropertyListSyntax>($1);
      TokenSyntax comma = make_token(CommaToken);
      ClassPropertyClauseSyntax property = make<ClassPropertyClauseSyntax>($3);
      ClassPropertyListItemSyntax propertyListItem = make_decl(
         ClassPropertyListItem, comma, property);
      list.appending(propertyListItem);
      $$ = list.getRaw();
   }
|  property {
      ClassPropertyClauseSyntax property = make<ClassPropertyClauseSyntax>($1);
      ClassPropertyListItemSyntax propertyListItem = make_decl(
         ClassPropertyListItem, std::nullopt, property);
      std::vector<ClassPropertyListItemSyntax> items{propertyListItem};
      ClassPropertyListSyntax list = make_decl(ClassPropertyList, items);
      $$ = list.getRaw();
   }
;

property:
   T_VARIABLE backup_doc_comment {
      TokenSyntax variable = make_token_with_text(Variable, $1);
      ClassPropertyClauseSyntax prop = make_decl(ClassPropertyClause, variable, std::nullopt);
      $$ = prop.getRaw();
   }
|  T_VARIABLE T_EQUAL expr backup_doc_comment {
      TokenSyntax variable = make_token_with_text(Variable, $1);
      TokenSyntax equalToken = make_token(EqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      InitializerClauseSyntax initializer = make_decl(InitializerClause, equalToken, valueExpr);
      ClassPropertyClauseSyntax prop = make_decl(ClassPropertyClause, variable, initializer);
      $$ = prop.getRaw();
   }
;

class_const_list:
   class_const_list T_COMMA class_const_decl {
      ClassConstListSyntax list = make<ClassConstListSyntax>($1);
      TokenSyntax comma = make_token(CommaToken);
      ClassConstClauseSyntax constDecl = make<ClassConstClauseSyntax>($3);
      ClassConstListItemSyntax listItem = make_decl(ClassConstListItem, comma, constDecl);
      list.appending(listItem);
      $$ = list.getRaw();
   }
|  class_const_decl {
      ClassConstClauseSyntax constDecl = make<ClassConstClauseSyntax>($1);
      ClassConstListItemSyntax listItem = make_decl(ClassConstListItem, std::nullopt, constDecl);
      std::vector<ClassConstListItemSyntax> items{listItem};
      ClassConstListSyntax list = make_decl(ClassConstList, items);
      $$ = list.getRaw();
   }
;

class_const_decl:
   identifier T_EQUAL expr backup_doc_comment {
      IdentifierSyntax identifier = make<IdentifierSyntax>($1);
      TokenSyntax equalToken = make_token(EqualToken);
      ExprSyntax expr = make<ExprSyntax>($3);
      InitializerClauseSyntax initializer = make_decl(InitializerClause, equalToken, expr);
      ClassConstClauseSyntax classConstClause = make_decl(
         ClassConstClause, identifier, initializer
      );
      $$ = classConstClause.getRaw();
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
      ExprListSyntax list = make<ExprListSyntax>($1);
      TokenSyntax comma = make_token(CommaToken);
      ExprSyntax echoExpr = make<ExprSyntax>($3);
      ExprListItemSyntax exprListItem = make_expr(ExprListItem, comma, echoExpr);
      list.appending(exprListItem);
      $$ = list.getRaw();
   }
|  echo_expr {
      ExprSyntax echoExpr = make<ExprSyntax>($1);
      ExprListItemSyntax exprListItem = make_expr(ExprListItem, std::nullopt, echoExpr);
      std::vector<ExprListItemSyntax> items{exprListItem};
      ExprListSyntax list = make_expr(ExprList, items);
      $$ = list.getRaw();
   }
;

echo_expr:
   expr {
      $$ = $1;
   }
;

for_exprs:
   %empty {
      $$ = nullptr;
   }
|  non_empty_for_exprs {
      $$ = $1;
   }
;

non_empty_for_exprs:
   non_empty_for_exprs T_COMMA expr {
      ExprListSyntax list = make<ExprListSyntax>($1);
      TokenSyntax comma = make_token(CommaToken);
      ExprSyntax expr = make<ExprSyntax>($3);
      ExprListItemSyntax exprListItem = make_expr(ExprListItem, comma, expr);
      list.appending(exprListItem);
      $$ = list.getRaw();
   }
|  expr {
      ExprSyntax expr = make<ExprSyntax>($1);
      ExprListItemSyntax exprListItem = make_expr(ExprListItem, std::nullopt, expr);
      std::vector<ExprListItemSyntax> items{exprListItem};
      ExprListSyntax list = make_expr(ExprList, items);
      $$ = list.getRaw();
   }
;

anonymous_class:
   T_CLASS {} ctor_arguments
   extends_from implements_list backup_doc_comment T_LEFT_BRACE class_statement_list T_RIGHT_BRACE {
      TokenSyntax classKeyword = make_token(ClassKeyword);
      std::optional<ArgumentListClauseSyntax> argsClause = $3 ? std::optional(make<ArgumentListClauseSyntax>($3)) : std::nullopt;
      std::optional<ExtendsFromClauseSyntax> extendsFrom = $4 ? std::optional(make<ExtendsFromClauseSyntax>($4)) : std::nullopt;
      std::optional<ImplementsClauseSyntax> implementsFrom = $5 ? std::optional(make<ImplementsClauseSyntax>($5)) : std::nullopt;
      TokenSyntax leftBrace = make_token(LeftBraceToken);
      MemberDeclListSyntax stmts = make<MemberDeclListSyntax>($8);
      TokenSyntax rightBrace = make_token(RightBraceToken);
      MemberDeclBlockSyntax classDefCodeBlock = make_decl(MemberDeclBlock, leftBrace, stmts, rightBrace);
      AnonymousClassDefinitionClauseSyntax classDef = make_expr(
         AnonymousClassDefinitionClause, classKeyword, argsClause, extendsFrom, implementsFrom, classDefCodeBlock);
      $$ = classDef.getRaw();
   }
;

new_expr:
   T_NEW class_name_reference ctor_arguments {
      TokenSyntax newToken = make_token(NewKeyword);
      ClassNameRefClauseSyntax classNameRef = make<ClassNameRefClauseSyntax>($2);
      std::optional<ArgumentListClauseSyntax> argsClause = $3 ? std::optional(make<ArgumentListClauseSyntax>($3)) : std::nullopt;
      SimpleInstanceCreateExprSyntax simpleInstanceCreateExpr = make_expr(SimpleInstanceCreateExpr, newToken, classNameRef, argsClause);
      InstanceCreateExprSyntax instanceCreateExpr = make_expr(InstanceCreateExpr, simpleInstanceCreateExpr);
      $$ = instanceCreateExpr.getRaw();
   }
|  T_NEW anonymous_class {
      TokenSyntax newToken = make_token(NewKeyword);
      AnonymousClassDefinitionClauseSyntax anonymousClass = make<AnonymousClassDefinitionClauseSyntax>($2);
      AnonymousInstanceCreateExprSyntax anonymousClassInstanceCreateExpr = make_expr(
         AnonymousInstanceCreateExpr, newToken, anonymousClass
      );
      InstanceCreateExprSyntax instanceCreateExpr = make_expr(InstanceCreateExpr, anonymousClassInstanceCreateExpr);
      $$ = instanceCreateExpr.getRaw();
   }
;

expr:
   variable {
      VariableExprSyntax var = make<VariableExprSyntax>($1);
      $$ = var.getRaw();
   }
|  T_LIST T_LEFT_PAREN array_pair_list T_RIGHT_PAREN T_EQUAL expr {
      TokenSyntax listKeyword = make_token(ListKeyword);
      TokenSyntax leftParen = make_token(LeftParenToken);
      ArrayPairListSyntax arrayList = make<ArrayPairListSyntax>($3);
      TokenSyntax rightParen = make_token(RightParenToken);
      TokenSyntax equalToken = make_token(EqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($6);
      ListStructureClauseSyntax listStructureClause = make_expr(
         ListStructureClause, listKeyword, leftParen, arrayList, rightParen
      );
      ListStructureAssignmentExprSyntax listAssignmentExpr = make_expr(
         ListStructureAssignmentExpr, listStructureClause, equalToken, valueExpr
      );
      $$ = listAssignmentExpr.getRaw();
   }
|  T_LEFT_SQUARE_BRACKET array_pair_list T_RIGHT_SQUARE_BRACKET T_EQUAL expr {
      TokenSyntax leftSquareBracket = make_token(LeftSquareBracketToken);
      ArrayPairListSyntax arrayList = make<ArrayPairListSyntax>($2);
      TokenSyntax rightSquareBracket = make_token(RightSquareBracketToken);
      TokenSyntax equalToken = make_token(EqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($5);
      SimplifiedArrayCreateExprSyntax simplifiedArrayCreateExpr = make_expr(
         SimplifiedArrayCreateExpr, leftSquareBracket, arrayList, rightSquareBracket
      );
      ArrayStructureAssignmentExprSyntax arrayAssignmentExpr = make_expr(
         ArrayStructureAssignmentExpr, simplifiedArrayCreateExpr, equalToken, valueExpr
      );
      $$ = arrayAssignmentExpr.getRaw();
   }
|  variable T_EQUAL expr {
      VariableExprSyntax var = make<VariableExprSyntax>($1);
      TokenSyntax equalToken = make_token(EqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      AssignmentExprSyntax assignmentExpr = make_expr(AssignmentExpr, var, equalToken, valueExpr);
      $$ = assignmentExpr.getRaw();
   }
|  variable T_EQUAL T_AMPERSAND variable {
      VariableExprSyntax var = make<VariableExprSyntax>($1);
      TokenSyntax equalToken = make_token(EqualToken);
      TokenSyntax ampersand = make_token(AmpersandToken);
      VariableExprSyntax valueExpr = make<VariableExprSyntax>($4);
      ReferencedVariableExprSyntax referencedVar = make_expr(ReferencedVariableExpr, ampersand, valueExpr);
      AssignmentExprSyntax assignmentExpr = make_expr(AssignmentExpr, var, equalToken, referencedVar);
      $$ = assignmentExpr.getRaw();
   }
|  T_CLONE expr {
      TokenSyntax cloneKeyword = make_token(CloneKeyword);
      ExprSyntax expr = make<ExprSyntax>($2);
      CloneExprSyntax cloneExpr = make_expr(CloneExpr, cloneKeyword, expr);
      $$ = cloneExpr.getRaw();
   }
|  variable T_PLUS_EQUAL expr {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($1);
      TokenSyntax plusEqual = make_token(PlusEqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      CompoundAssignmentExprSyntax assignment = make_expr(CompoundAssignmentExpr, varExpr, plusEqual, valueExpr);
      $$ = assignment.getRaw();
   }
|  variable T_MINUS_EQUAL expr {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($1);
      TokenSyntax minusEqual = make_token(MinusEqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      CompoundAssignmentExprSyntax assignment = make_expr(CompoundAssignmentExpr, varExpr, minusEqual, valueExpr);
      $$ = assignment.getRaw();
   }
|  variable T_MUL_EQUAL expr {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($1);
      TokenSyntax mulEqual = make_token(MulEqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      CompoundAssignmentExprSyntax assignment = make_expr(CompoundAssignmentExpr, varExpr, mulEqual, valueExpr);
      $$ = assignment.getRaw();
   }
|  variable T_POW_EQUAL expr {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($1);
      TokenSyntax powEqual = make_token(PowEqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      CompoundAssignmentExprSyntax assignment = make_expr(CompoundAssignmentExpr, varExpr, powEqual, valueExpr);
      $$ = assignment.getRaw();
   }
|  variable T_DIV_EQUAL expr {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($1);
      TokenSyntax divEqual = make_token(DivEqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      CompoundAssignmentExprSyntax assignment = make_expr(CompoundAssignmentExpr, varExpr, divEqual, valueExpr);
      $$ = assignment.getRaw();
   }
|  variable T_STR_CONCAT_EQUAL expr {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($1);
      TokenSyntax strConcatEqual = make_token(StrConcatEqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      CompoundAssignmentExprSyntax assignment = make_expr(CompoundAssignmentExpr, varExpr, strConcatEqual, valueExpr);
      $$ = assignment.getRaw();
   }
|  variable T_MOD_EQUAL expr {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($1);
      TokenSyntax modEqual = make_token(ModEqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      CompoundAssignmentExprSyntax assignment = make_expr(CompoundAssignmentExpr, varExpr, modEqual, valueExpr);
      $$ = assignment.getRaw();
   }
|  variable T_AND_EQUAL expr {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($1);
      TokenSyntax andEqual = make_token(AndEqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      CompoundAssignmentExprSyntax assignment = make_expr(CompoundAssignmentExpr, varExpr, andEqual, valueExpr);
      $$ = assignment.getRaw();
   }
|  variable T_OR_EQUAL expr {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($1);
      TokenSyntax orEqual = make_token(OrEqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      CompoundAssignmentExprSyntax assignment = make_expr(CompoundAssignmentExpr, varExpr, orEqual, valueExpr);
      $$ = assignment.getRaw();
   }
|  variable T_XOR_EQUAL expr {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($1);
      TokenSyntax xorEqual = make_token(XorEqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      CompoundAssignmentExprSyntax assignment = make_expr(CompoundAssignmentExpr, varExpr, xorEqual, valueExpr);
      $$ = assignment.getRaw();
   }
|  variable T_SL_EQUAL expr {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($1);
      TokenSyntax shiftLeftEqual = make_token(ShiftLeftEqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      CompoundAssignmentExprSyntax assignment = make_expr(CompoundAssignmentExpr, varExpr, shiftLeftEqual, valueExpr);
      $$ = assignment.getRaw();
   }
|  variable T_SR_EQUAL expr {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($1);
      TokenSyntax shiftRightEqual = make_token(ShiftRightEqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      CompoundAssignmentExprSyntax assignment = make_expr(CompoundAssignmentExpr, varExpr, shiftRightEqual, valueExpr);
      $$ = assignment.getRaw();
   }
|  variable T_COALESCE_EQUAL expr {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($1);
      TokenSyntax coalesceEqualToken = make_token(CoalesceEqualToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      CompoundAssignmentExprSyntax assignment = make_expr(CompoundAssignmentExpr, varExpr, coalesceEqualToken, valueExpr);
      $$ = assignment.getRaw();
   }
|  variable T_INC {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($1);
      TokenSyntax incToken = make_token(IncToken);
      PostfixOperatorExprSyntax postfixExpr = make_expr(PostfixOperatorExpr, varExpr, incToken);
      $$ = postfixExpr.getRaw();
   }
|  T_INC variable {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($2);
      TokenSyntax incToken = make_token(IncToken);
      PrefixOperatorExprSyntax prefixExpr = make_expr(PrefixOperatorExpr, incToken, varExpr);
      $$ = prefixExpr.getRaw();
   }
|  variable T_DEC {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($1);
      TokenSyntax decToken = make_token(DecToken);
      PostfixOperatorExprSyntax postfixExpr = make_expr(PostfixOperatorExpr, varExpr, decToken);
      $$ = postfixExpr.getRaw();
   }
|  T_DEC variable {
      VariableExprSyntax varExpr = make<VariableExprSyntax>($2);
      TokenSyntax decToken = make_token(DecToken);
      PrefixOperatorExprSyntax prefixExpr = make_expr(PrefixOperatorExpr, decToken, varExpr);
      $$ = prefixExpr.getRaw();
   }
|  expr T_BOOLEAN_OR expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(BooleanOrToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      LogicalExprSyntax loginalExpr = make_expr(LogicalExpr, lhs, operatorToken, rhs);
      $$ = loginalExpr.getRaw();
      
   }
|  expr T_BOOLEAN_AND expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(BooleanAndToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      LogicalExprSyntax loginalExpr = make_expr(LogicalExpr, lhs, operatorToken, rhs);
      $$ = loginalExpr.getRaw();
   }
|  expr T_LOGICAL_OR expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(LogicOrKeyword);
      ExprSyntax rhs = make<ExprSyntax>($3);
      LogicalExprSyntax loginalExpr = make_expr(LogicalExpr, lhs, operatorToken, rhs);
      $$ = loginalExpr.getRaw();
   }
|  expr T_LOGICAL_AND expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(LogicAndKeyword);
      ExprSyntax rhs = make<ExprSyntax>($3);
      LogicalExprSyntax loginalExpr = make_expr(LogicalExpr, lhs, operatorToken, rhs);
      $$ = loginalExpr.getRaw();
   }
|  expr T_LOGICAL_XOR expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(LogicXorKeyword);
      ExprSyntax rhs = make<ExprSyntax>($3);
      LogicalExprSyntax loginalExpr = make_expr(LogicalExpr, lhs, operatorToken, rhs);
      $$ = loginalExpr.getRaw();
   }
|  expr T_VBAR expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(VerticalBarToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      BitLogicalExprSyntax bitLogicExpr = make_expr(BitLogicalExpr, lhs, operatorToken, rhs);
      $$ = bitLogicExpr.getRaw();
   }
|  expr T_AMPERSAND expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(AmpersandToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      BitLogicalExprSyntax bitLogicExpr = make_expr(BitLogicalExpr, lhs, operatorToken, rhs);
      $$ = bitLogicExpr.getRaw();
   }
|  expr T_CARET expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(CaretToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      BitLogicalExprSyntax bitLogicExpr = make_expr(BitLogicalExpr, lhs, operatorToken, rhs);
      $$ = bitLogicExpr.getRaw();
   }
|  expr T_STR_CONCAT expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(StrConcatToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      BinaryOperatorExprSyntax binaryExpr = make_expr(BinaryOperatorExpr, lhs, operatorToken, rhs);
      $$ = binaryExpr.getRaw();
   }
|  expr T_PLUS_SIGN expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(PlusSignToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      BinaryOperatorExprSyntax binaryExpr = make_expr(BinaryOperatorExpr, lhs, operatorToken, rhs);
      $$ = binaryExpr.getRaw();
   }
|  expr T_MINUS_SIGN expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(MinusSignToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      BinaryOperatorExprSyntax binaryExpr = make_expr(BinaryOperatorExpr, lhs, operatorToken, rhs);
      $$ = binaryExpr.getRaw();
   }
|  expr T_MUL_SIGN expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(MulSignToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      BinaryOperatorExprSyntax binaryExpr = make_expr(BinaryOperatorExpr, lhs, operatorToken, rhs);
      $$ = binaryExpr.getRaw();
   }
|  expr T_POW expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(PowToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      BinaryOperatorExprSyntax binaryExpr = make_expr(BinaryOperatorExpr, lhs, operatorToken, rhs);
      $$ = binaryExpr.getRaw();
   }
|  expr T_DIV_SIGN expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(DivSignToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      BinaryOperatorExprSyntax binaryExpr = make_expr(BinaryOperatorExpr, lhs, operatorToken, rhs);
      $$ = binaryExpr.getRaw();
   }
|  expr T_MOD_SIGN expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(ModSignToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      BinaryOperatorExprSyntax binaryExpr = make_expr(BinaryOperatorExpr, lhs, operatorToken, rhs);
      $$ = binaryExpr.getRaw();
   }
|  expr T_SL expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(ShiftLeftToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      BinaryOperatorExprSyntax binaryExpr = make_expr(BinaryOperatorExpr, lhs, operatorToken, rhs);
      $$ = binaryExpr.getRaw();
   }
|  expr T_SR expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(ShiftRightToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      BinaryOperatorExprSyntax binaryExpr = make_expr(BinaryOperatorExpr, lhs, operatorToken, rhs);
      $$ = binaryExpr.getRaw();
   }
|  T_PLUS_SIGN expr %prec T_INC {
      TokenSyntax plusSignToken = make_token(PlusSignToken);
      ExprSyntax varExpr = make<ExprSyntax>($2);
      PrefixOperatorExprSyntax prefixExpr = make_expr(PrefixOperatorExpr, plusSignToken, varExpr);
      $$ = prefixExpr.getRaw();
   }
|  T_MINUS_SIGN expr %prec T_INC {
      TokenSyntax minusSignToken = make_token(MinusSignToken);
      ExprSyntax varExpr = make<ExprSyntax>($2);
      PrefixOperatorExprSyntax prefixExpr = make_expr(PrefixOperatorExpr, minusSignToken, varExpr);
      $$ = prefixExpr.getRaw();
   }
|  T_EXCLAMATION_MARK expr {
      TokenSyntax exclamationMarkToken = make_token(ExclamationMarkToken);
      ExprSyntax varExpr = make<ExprSyntax>($2);
      PrefixOperatorExprSyntax prefixExpr = make_expr(PrefixOperatorExpr, exclamationMarkToken, varExpr);
      $$ = prefixExpr.getRaw();
   }
|  T_TILDE expr {
      TokenSyntax tildeToken = make_token(TildeToken);
      ExprSyntax varExpr = make<ExprSyntax>($2);
      PrefixOperatorExprSyntax prefixExpr = make_expr(PrefixOperatorExpr, tildeToken, varExpr);
      $$ = prefixExpr.getRaw();
   }
|  expr T_IS_IDENTICAL expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(IsIdenticalToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      RelationExprSyntax relationExpr = make_expr(RelationExpr, lhs, operatorToken, rhs);
      $$ = relationExpr.getRaw();
   }
|  expr T_IS_NOT_IDENTICAL expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(IsNotIdenticalToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      RelationExprSyntax relationExpr = make_expr(RelationExpr, lhs, operatorToken, rhs);
      $$ = relationExpr.getRaw();
   }
|  expr T_IS_EQUAL expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(IsEqualToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      RelationExprSyntax relationExpr = make_expr(RelationExpr, lhs, operatorToken, rhs);
      $$ = relationExpr.getRaw();
   }
|  expr T_IS_NOT_EQUAL expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(IsNotEqualToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      RelationExprSyntax relationExpr = make_expr(RelationExpr, lhs, operatorToken, rhs);
      $$ = relationExpr.getRaw();
   }
|  expr T_IS_SMALLER expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(IsSmallerToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      RelationExprSyntax relationExpr = make_expr(RelationExpr, lhs, operatorToken, rhs);
      $$ = relationExpr.getRaw();
   }
|  expr T_IS_SMALLER_OR_EQUAL expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(IsSmallerOrEqualToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      RelationExprSyntax relationExpr = make_expr(RelationExpr, lhs, operatorToken, rhs);
      $$ = relationExpr.getRaw();
   }
|  expr T_IS_GREATER expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(IsGreaterToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      RelationExprSyntax relationExpr = make_expr(RelationExpr, lhs, operatorToken, rhs);
      $$ = relationExpr.getRaw();
   }
|  expr T_IS_GREATER_OR_EQUAL expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(IsGreaterOrEqualToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      RelationExprSyntax relationExpr = make_expr(RelationExpr, lhs, operatorToken, rhs);
      $$ = relationExpr.getRaw();
   }
|  expr T_SPACESHIP expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(SpaceshipToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      RelationExprSyntax relationExpr = make_expr(RelationExpr, lhs, operatorToken, rhs);
      $$ = relationExpr.getRaw();
   }
|  expr T_INSTANCEOF class_name_reference {
      ExprSyntax instance = make<ExprSyntax>($1);
      TokenSyntax instanceof = make_token(InstanceofKeyword);
      ClassNameRefClauseSyntax classNameRef = make<ClassNameRefClauseSyntax>($3);
      InstanceofExprSyntax instanceofExpr = make_expr(InstanceofExpr, instance, instanceof, classNameRef);
      $$ = instanceofExpr.getRaw();
   }
|  T_LEFT_PAREN expr T_RIGHT_PAREN {
      TokenSyntax leftParen = make_token(LeftParenToken);
      ExprSyntax expr = make<ExprSyntax>($2);
      TokenSyntax rightParen = make_token(RightParenToken);
      ParenDecoratedExprSyntax decoatedExpr = make_expr(ParenDecoratedExpr, leftParen, expr, rightParen);
      $$ = decoatedExpr.getRaw();
   }
|  new_expr {
      $$ = $1;
   }
|  expr T_QUESTION_MARK expr T_COLON expr { 
      ExprSyntax conditionExpr = make<ExprSyntax>($1);
      TokenSyntax questionMark = make_token(QuestionMarkToken);
      ExprSyntax firstChoice = make<ExprSyntax>($3);
      TokenSyntax colon = make_token(ColonToken);
      ExprSyntax secondChoice = make<ExprSyntax>($5);
      TernaryExprSyntax ternaryExpr = make_expr(TernaryExpr, conditionExpr, questionMark, firstChoice, colon, secondChoice);
      $$ = ternaryExpr.getRaw();
   }
|  expr T_QUESTION_MARK T_COLON expr {
      ExprSyntax conditionExpr = make<ExprSyntax>($1);
      TokenSyntax questionMark = make_token(QuestionMarkToken);
      TokenSyntax colon = make_token(ColonToken);
      ExprSyntax secondChoice = make<ExprSyntax>($4);
      TernaryExprSyntax ternaryExpr = make_expr(TernaryExpr, conditionExpr, questionMark, std::nullopt, colon, secondChoice);
      $$ = ternaryExpr.getRaw();
   }
|  expr T_COALESCE expr {
      ExprSyntax lhs = make<ExprSyntax>($1);
      TokenSyntax operatorToken = make_token(CoalesceToken);
      ExprSyntax rhs = make<ExprSyntax>($3);
      BinaryOperatorExprSyntax binaryExpr = make_expr(BinaryOperatorExpr, lhs, operatorToken, rhs);
      $$ = binaryExpr.getRaw();
   }
|  internal_functions_in_bison {
      $$ = $1;
   }
|  T_INT_CAST expr {
      TokenSyntax castType = make_token(IntCastKeyword);
      ExprSyntax sourceExpr = make<ExprSyntax>($2);
      CastExprSyntax castExpr = make_expr(CastExpr, castType, sourceExpr);
      $$ = castExpr.getRaw();
   }
|  T_DOUBLE_CAST expr {
      TokenSyntax castType = make_token(DoubleCastKeyword);
      ExprSyntax sourceExpr = make<ExprSyntax>($2);
      CastExprSyntax castExpr = make_expr(CastExpr, castType, sourceExpr);
      $$ = castExpr.getRaw();
   }
|  T_STRING_CAST expr {
      TokenSyntax castType = make_token(StringCastKeyword);
      ExprSyntax sourceExpr = make<ExprSyntax>($2);
      CastExprSyntax castExpr = make_expr(CastExpr, castType, sourceExpr);
      $$ = castExpr.getRaw();
   }
|  T_ARRAY_CAST expr {
      TokenSyntax castType = make_token(ArrayCastKeyword);
      ExprSyntax sourceExpr = make<ExprSyntax>($2);
      CastExprSyntax castExpr = make_expr(CastExpr, castType, sourceExpr);
      $$ = castExpr.getRaw();
   }
|  T_OBJECT_CAST expr {
      TokenSyntax castType = make_token(ObjectCastKeyword);
      ExprSyntax sourceExpr = make<ExprSyntax>($2);
      CastExprSyntax castExpr = make_expr(CastExpr, castType, sourceExpr);
      $$ = castExpr.getRaw();
   }
|  T_BOOL_CAST expr {
      TokenSyntax castType = make_token(BoolCastKeyword);
      ExprSyntax sourceExpr = make<ExprSyntax>($2);
      CastExprSyntax castExpr = make_expr(CastExpr, castType, sourceExpr);
      $$ = castExpr.getRaw();
   }
|  T_UNSET_CAST expr {
      TokenSyntax castType = make_token(UnsetCastKeyword);
      ExprSyntax sourceExpr = make<ExprSyntax>($2);
      CastExprSyntax castExpr = make_expr(CastExpr, castType, sourceExpr);
      $$ = castExpr.getRaw();
   }
|  T_EXIT exit_expr {
      TokenSyntax exitToken = make_token(ExitKeyword);
      std::optional<ExitExprArgClauseSyntax> exitArgClause = $2 ? std::optional(make<ExitExprArgClauseSyntax>($2)) : std::nullopt;
      ExitExprSyntax exitExpr = make_expr(ExitExpr, exitToken, exitArgClause);
      $$ = exitExpr.getRaw();
   }
|  T_ERROR_SUPPRESS_SIGN expr {
      TokenSyntax errorSuppressToken = make_token(ErrorSuppressSignToken);
      ExprSyntax varExpr = make<ExprSyntax>($2);
      PrefixOperatorExprSyntax prefixExpr = make_expr(PrefixOperatorExpr, errorSuppressToken, varExpr);
      $$ = prefixExpr.getRaw();
   }
|  scalar {
      Syntax scalarValue = make<Syntax>($1);
      ScalarExprSyntax scalar = make_expr(ScalarExpr, scalarValue);
      $$ = scalar.getRaw();
   }
|  T_BACKTICK backticks_expr T_BACKTICK {
      TokenSyntax backtickToken = make_token(BacktickToken);
      BackticksClauseSyntax backticksExpr = make<BackticksClauseSyntax>($2);
      ShellCmdExprSyntax shellCmd = make_expr(ShellCmdExpr, backtickToken, backticksExpr, backtickToken);
      $$ = shellCmd.getRaw();
   }
|  T_PRINT expr {
      TokenSyntax printKeyword = make_token(PrintKeyword);
      ExprSyntax valueExpr = make<ExprSyntax>($2);
      PrintFuncExprSyntax printExpr = make_expr(PrintFuncExpr, printKeyword, valueExpr);
      $$ = printExpr.getRaw();
   }
|  T_YIELD {
      TokenSyntax yieldKeyword = make_token(YieldKeyword);
      YieldExprSyntax yieldExpr = make_expr(YieldExpr, yieldKeyword, std::nullopt, std::nullopt, std::nullopt);
      $$ = yieldExpr.getRaw();
   }
|  T_YIELD expr {
      TokenSyntax yieldKeyword = make_token(YieldKeyword);
      ExprSyntax valueExpr = make<ExprSyntax>($2);
      YieldExprSyntax yieldExpr = make_expr(YieldExpr, yieldKeyword, std::nullopt, std::nullopt, valueExpr);
      $$ = yieldExpr.getRaw();
   }
|  T_YIELD expr T_DOUBLE_ARROW expr {
      TokenSyntax yieldKeyword = make_token(YieldKeyword);
      ExprSyntax keyExpr = make<ExprSyntax>($2);
      TokenSyntax doubleArrowToken = make_token(DoubleArrowToken);
      ExprSyntax valueExpr = make<ExprSyntax>($4);
      YieldExprSyntax yieldExpr = make_expr(YieldExpr, yieldKeyword, keyExpr, doubleArrowToken, valueExpr);
      $$ = yieldExpr.getRaw();
   }
|  T_YIELD_FROM expr {
      TokenSyntax yieldFromKeyword = make_token(YieldFromKeyword);
      ExprSyntax valueExpr = make<ExprSyntax>($2);
      YieldFromExprSyntax yieldFromExpr = make_expr(YieldFromExpr, yieldFromKeyword, valueExpr);
      $$ = yieldFromExpr.getRaw();
   }
|  inline_function {
      LambdaExprSyntax lambdaExpr = make_expr(LambdaExpr, std::nullopt, make<ExprSyntax>($1));
      $$ = lambdaExpr.getRaw();
   }
|  T_STATIC inline_function {
      TokenSyntax staticKeyword = make_token(StaticKeyword);
      LambdaExprSyntax lambdaExpr = make_expr(LambdaExpr, staticKeyword, make<ExprSyntax>($2));
      $$ = lambdaExpr.getRaw();
   }
;

inline_function:
   function returns_ref backup_doc_comment T_LEFT_PAREN parameter_list T_RIGHT_PAREN lexical_vars return_type
   backup_fn_flags T_LEFT_BRACE inner_statement_list T_RIGHT_BRACE backup_fn_flags {
      TokenSyntax functionKeyword = make<TokenSyntax>($1);
      std::optional<TokenSyntax> returnRef = $2 ? std::optional(make<TokenSyntax>($2)) : std::nullopt;
      TokenSyntax leftParenToken = make_token(LeftParenToken);
      std::optional<ParameterListSyntax> params = $5 ? std::optional(make<ParameterListSyntax>($5)) : std::nullopt;
      TokenSyntax rightParenToken = make_token(RightParenToken);
      ParameterClauseSyntax paramsClause = make_decl(ParameterClause, leftParenToken, params, rightParenToken);
      std::optional<UseLexicalVariableClauseSyntax> lexicalVarClause = $7 ? std::optional(make<UseLexicalVariableClauseSyntax>($7)) : std::nullopt;
      std::optional<ReturnTypeClauseSyntax> returnType = $8 ? std::optional(make<ReturnTypeClauseSyntax>($8)) : std::nullopt;
      TokenSyntax leftBrace = make_token(LeftBraceToken);
      InnerStmtListSyntax innerStmtClause = make<InnerStmtListSyntax>($11);
      TokenSyntax rightBrace = make_token(RightBraceToken);
      InnerCodeBlockStmtSyntax innerClodeBlock = make_stmt(InnerCodeBlockStmt, leftBrace, innerStmtClause, rightBrace);
      ClassicLambdaExprSyntax lambdaExpr = make_expr(
         ClassicLambdaExpr, functionKeyword, returnRef, paramsClause, lexicalVarClause, returnType, innerClodeBlock);
      $$ = lambdaExpr.getRaw();
   }
|  fn returns_ref T_LEFT_PAREN parameter_list T_RIGHT_PAREN return_type backup_doc_comment T_DOUBLE_ARROW
   backup_fn_flags backup_lex_pos expr backup_fn_flags {
      TokenSyntax fnKeyword = make<TokenSyntax>($1);
      std::optional<TokenSyntax> returnRef = $2 ? std::optional(make<TokenSyntax>($2)) : std::nullopt;
      TokenSyntax leftParenToken = make_token(LeftParenToken);
      std::optional<ParameterListSyntax> params = $4 ? std::optional(make<ParameterListSyntax>($4)) : std::nullopt;
      TokenSyntax rightParenToken = make_token(RightParenToken);
      ParameterClauseSyntax paramsClause = make_decl(ParameterClause, leftParenToken, params, rightParenToken);
      std::optional<ReturnTypeClauseSyntax> returnType = $6 ? std::optional(make<ReturnTypeClauseSyntax>($4)) : std::nullopt;
      TokenSyntax doubleArrowToken = make_token(DoubleArrowToken);
      ExprSyntax expr = make<ExprSyntax>($11);
      SimplifiedLambdaExprSyntax lambdaExpr = make_expr(SimplifiedLambdaExpr, fnKeyword, returnRef, paramsClause, returnType, doubleArrowToken, expr);
      $$ = lambdaExpr.getRaw();
   }
;

fn:
   T_FN {
      TokenSyntax fnKeyword = make_token(FnKeyword);
      $$ = fnKeyword.getRaw();
   }
;

function:
   T_FUNCTION {
      TokenSyntax funcKeyword = make_token(FunctionKeyword);
      $$ = funcKeyword.getRaw();
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
      $$ = nullptr;
   }
|  T_AMPERSAND {
      TokenSyntax ampersand = make_token(AmpersandToken);
      $$ = ampersand.getRaw();
   }
;

lexical_vars:
   %empty {
      $$ = nullptr;
   }
|  T_USE T_LEFT_PAREN lexical_var_list T_RIGHT_PAREN {
      TokenSyntax useKeyword = make_token(UseKeyword);
      TokenSyntax leftParen = make_token(LeftParenToken);
      LexicalVariableListSyntax lexicalVarList = make<LexicalVariableListSyntax>($3);
      TokenSyntax rightParen = make_token(RightParenToken);
      UseLexicalVariableClauseSyntax useLexicalVarsClause = make_expr(
         UseLexicalVariableClause, useKeyword, leftParen, lexicalVarList, rightParen
      );
      $$ = useLexicalVarsClause.getRaw();
   }
;

lexical_var_list:
   lexical_var_list T_COMMA lexical_var {
      LexicalVariableListSyntax list = make<LexicalVariableListSyntax>($1);
      TokenSyntax comma = make_token(CommaToken);
      LexicalVariableSyntax lexicalVar = make<LexicalVariableSyntax>($3);
      LexicalVariableListItemSyntax lexicalVarListItem = make_expr(
         LexicalVariableListItem, comma, lexicalVar
      );
      list.appending(lexicalVarListItem);
      $$ = list.getRaw();
   }
|  lexical_var {
      LexicalVariableSyntax lexicalVar = make<LexicalVariableSyntax>($1);
      LexicalVariableListItemSyntax lexicalVarListItem = make_expr(
         LexicalVariableListItem, std::nullopt, lexicalVar
      );
      std::vector<LexicalVariableListItemSyntax> items{lexicalVarListItem};
      LexicalVariableListSyntax list = make_expr(LexicalVariableList, items);
      $$ = list.getRaw();
   }
;

lexical_var:
   T_VARIABLE {
      TokenSyntax variableToken = make_token_with_text(Variable, $1);
      LexicalVariableSyntax lexicalVar = make_expr(LexicalVariable, std::nullopt, variableToken);
      $$ = lexicalVar.getRaw();
   }
|  T_AMPERSAND T_VARIABLE {
      TokenSyntax ampersand = make_token(AmpersandToken);
      TokenSyntax variableToken = make_token_with_text(Variable, $2);
      LexicalVariableSyntax lexicalVar = make_expr(LexicalVariable, ampersand, variableToken);
      $$ = lexicalVar.getRaw();
   }
;

function_call:
   name argument_list {
      NameSyntax funcName = make<NameSyntax>($1);
      ArgumentListClauseSyntax argsClause = make<ArgumentListClauseSyntax>($2);
      SimpleFunctionCallExprSyntax simpleFuncCallExpr = make_expr(SimpleFunctionCallExpr, funcName, argsClause);
      FunctionCallExprSyntax funcCallExpr = make_expr(FunctionCallExpr, simpleFuncCallExpr);
      $$ = funcCallExpr.getRaw();
   }
|  class_name T_PAAMAYIM_NEKUDOTAYIM member_name argument_list {
      ClassNameClauseSyntax className = make<ClassNameClauseSyntax>($1);
      TokenSyntax paamayimNekudotayimToken = make_token(PaamayimNekudotayimToken);
      MemberNameClauseSyntax memberName = make<MemberNameClauseSyntax>($3);
      ArgumentListClauseSyntax argsClause = make<ArgumentListClauseSyntax>($4);
      StaticMethodCallExprSyntax staticMemthodCallExpr = make_expr(
         StaticMethodCallExpr, className, paamayimNekudotayimToken, memberName, argsClause);
      FunctionCallExprSyntax funcCallExpr = make_expr(FunctionCallExpr, staticMemthodCallExpr);
      $$ = funcCallExpr.getRaw();
   }
|  variable_class_name T_PAAMAYIM_NEKUDOTAYIM member_name argument_list {
      VariableClassNameClauseSyntax className = make<VariableClassNameClauseSyntax>($1);
      TokenSyntax paamayimNekudotayimToken = make_token(PaamayimNekudotayimToken);
      MemberNameClauseSyntax memberName = make<MemberNameClauseSyntax>($3);
      ArgumentListClauseSyntax argsClause = make<ArgumentListClauseSyntax>($4);
      StaticMethodCallExprSyntax staticMemthodCallExpr = make_expr(
         StaticMethodCallExpr, className, paamayimNekudotayimToken, memberName, argsClause);
      FunctionCallExprSyntax funcCallExpr = make_expr(FunctionCallExpr, staticMemthodCallExpr);
      $$ = funcCallExpr.getRaw();
   }
|  callable_expr argument_list {
      CallableFuncNameClauseSyntax funcName = make<CallableFuncNameClauseSyntax>($1);
      ArgumentListClauseSyntax argsClause = make<ArgumentListClauseSyntax>($2);
      SimpleFunctionCallExprSyntax simpleFuncCallExpr = make_expr(SimpleFunctionCallExpr, funcName, argsClause);
      $$ = simpleFuncCallExpr.getRaw();
   }
;

class_name:
   T_STATIC {
      TokenSyntax staticToken = make_token(StaticKeyword);
      ClassNameClauseSyntax className = make_expr(ClassNameClause, staticToken);
      $$ = className.getRaw();
   }
|  name {
      NameSyntax name = make<NameSyntax>($1);
      ClassNameClauseSyntax className = make_expr(ClassNameClause, name);
      $$ = className.getRaw();
   }
;

class_name_reference:
   class_name {
      ClassNameClauseSyntax className = make<ClassNameClauseSyntax>($1);
      ClassNameRefClauseSyntax classNameRef = make_expr(ClassNameRefClause, className);
      $$ = classNameRef.getRaw();
   }
|  new_variable {
      NewVariableClauseSyntax newVar = make<NewVariableClauseSyntax>($1);
      ClassNameRefClauseSyntax classNameRef = make_expr(ClassNameRefClause, newVar);
      $$ = classNameRef.getRaw();
   }
;

exit_expr:
   %empty {
      $$ = nullptr;
   }
|  T_LEFT_PAREN optional_expr T_RIGHT_PAREN {
      TokenSyntax leftParen = make_token(LeftParenToken);
      OptionalExprSyntax optExpr = make<OptionalExprSyntax>($2);
      TokenSyntax rightParen = make_token(RightParenToken);
      ExitExprArgClauseSyntax exitArgsClause = make_expr(ExitExprArgClause, leftParen, optExpr, rightParen);
      $$ = exitArgsClause.getRaw();
   }
;

backticks_expr:
   %empty {
      $$ = nullptr;
   }
|  T_ENCAPSED_AND_WHITESPACE {
      TokenSyntax str = make_token_with_text(EncapsedAndWhitespace, $1);
      BackticksClauseSyntax expr = make_expr(BackticksClause, str);
      $$ = expr.getRaw();
   }
|  encaps_list {
      EncapsItemListSyntax encapsList = make<EncapsItemListSyntax>($1);
      BackticksClauseSyntax expr = make_expr(BackticksClause, encapsList);
      $$ = expr.getRaw();
   }
;

ctor_arguments:
   %empty {
      $$ = nullptr;
   }
|  argument_list {
      $$ = $1;
   }
;

dereferencable_scalar:
   T_ARRAY T_LEFT_PAREN array_pair_list T_RIGHT_PAREN {
      TokenSyntax arrayToken = make_token(ArrayKeyword);
      TokenSyntax leftParen = make_token(LeftParenToken);
      ArrayPairListSyntax arrayPairList = make<ArrayPairListSyntax>($3);
      TokenSyntax rightParen = make_token(RightParenToken);
      ArrayCreateExprSyntax arrayCreateExpr = make_expr(ArrayCreateExpr, arrayToken, leftParen, arrayPairList, rightParen);
      DereferencableScalarExprSyntax scalar = make_expr(DereferencableScalarExpr, arrayCreateExpr);
      $$ = scalar.getRaw();
   }
|  T_LEFT_SQUARE_BRACKET array_pair_list T_RIGHT_SQUARE_BRACKET {
      TokenSyntax leftSquareBracket = make_token(LeftSquareBracketToken);
      ArrayPairListSyntax arrayPairList = make<ArrayPairListSyntax>($2);
      TokenSyntax rightSquareBracket = make_token(RightSquareBracketToken);
      SimplifiedArrayCreateExprSyntax simpleArrayCreateExpr = make_expr(SimplifiedArrayCreateExpr, leftSquareBracket, arrayPairList, rightSquareBracket);
      DereferencableScalarExprSyntax scalar = make_expr(DereferencableScalarExpr, simpleArrayCreateExpr);
      $$ = scalar.getRaw();
   }
|  T_DOUBLE_QUOTE T_CONSTANT_ENCAPSED_STRING T_DOUBLE_QUOTE {
      TokenSyntax doubleQuote = make_token(DoubleStrQuoteToken);
      TokenSyntax strToken = make_token_with_text(ConstantEncapsedString, $2);
      StringLiteralExprSyntax str = make_expr(StringLiteralExpr, doubleQuote, strToken, doubleQuote);
      DereferencableScalarExprSyntax scalar = make_expr(DereferencableScalarExpr, str);
      $$ = scalar.getRaw();
   }
|  T_SINGLE_QUOTE T_CONSTANT_ENCAPSED_STRING T_SINGLE_QUOTE {
      TokenSyntax singleQuote = make_token(SingleStrQuoteToken);
      TokenSyntax strToken = make_token_with_text(ConstantEncapsedString, $2);
      StringLiteralExprSyntax str = make_expr(StringLiteralExpr, singleQuote, strToken, singleQuote);
      DereferencableScalarExprSyntax scalar = make_expr(DereferencableScalarExpr, str);
      $$ = scalar.getRaw();
   }
;

scalar:
   T_LNUMBER {
      TokenSyntax lval = make_lnumber_token($1);
      ScalarExprSyntax scalarValue = make_expr(ScalarExpr, lval);
      $$ = scalarValue.getRaw();
   }
|  T_DNUMBER {
      TokenSyntax dval = make_dnumber_token($1);
      ScalarExprSyntax scalarValue = make_expr(ScalarExpr, dval);
      $$ = scalarValue.getRaw();
   }
|  T_LINE {
      TokenSyntax lineKeyword = make_token(LineKeyword);
      ScalarExprSyntax scalarValue = make_expr(ScalarExpr, lineKeyword);
      $$ = scalarValue.getRaw();
   }
|  T_FILE {
      TokenSyntax fileKeyword = make_token(FileKeyword);
      ScalarExprSyntax scalarValue = make_expr(ScalarExpr, fileKeyword);
      $$ = scalarValue.getRaw();
   }
|  T_DIR {
      TokenSyntax dirKeyword = make_token(DirKeyword);
      ScalarExprSyntax scalarValue = make_expr(ScalarExpr, dirKeyword);
      $$ = scalarValue.getRaw();
   }
|  T_TRAIT_CONST {
      TokenSyntax traitConstKeyword = make_token(TraitConstKeyword);
      ScalarExprSyntax scalarValue = make_expr(ScalarExpr, traitConstKeyword);
      $$ = scalarValue.getRaw();
   }
|  T_METHOD_CONST {
      TokenSyntax methodConstKeyword = make_token(MethodConstKeyword);
      ScalarExprSyntax scalarValue = make_expr(ScalarExpr, methodConstKeyword);
      $$ = scalarValue.getRaw();
   }
|  T_FUNC_CONST {
      TokenSyntax funcConstKeyword = make_token(FuncConstKeyword);
      ScalarExprSyntax scalarValue = make_expr(ScalarExpr, funcConstKeyword);
      $$ = scalarValue.getRaw();
   }
|  T_NS_CONST {
      TokenSyntax namespaceConstKeyword = make_token(NamespaceConstKeyword);
      ScalarExprSyntax scalarValue = make_expr(ScalarExpr, namespaceConstKeyword);
      $$ = scalarValue.getRaw();
   }
|  T_CLASS_CONST {
      TokenSyntax classConstKeyword = make_token(ClassConstKeyword);
      ScalarExprSyntax scalarValue = make_expr(ScalarExpr, classConstKeyword);
      $$ = scalarValue.getRaw();
   }
|  T_START_HEREDOC T_ENCAPSED_AND_WHITESPACE T_END_HEREDOC {
      TokenSyntax startHeredoc = make_token(StartHereDoc);
      TokenSyntax encapsStr = make_token_with_text(EncapsedAndWhitespace, $2);
      TokenSyntax endHeredoc = make_token(EndHereDoc);
      HeredocExprSyntax heredoc = make_expr(HeredocExpr, startHeredoc, encapsStr, endHeredoc);
      ScalarExprSyntax scalarValue = make_expr(ScalarExpr, heredoc);
      $$ = scalarValue.getRaw();
   }
|  T_START_HEREDOC T_END_HEREDOC {
      TokenSyntax startHeredoc = make_token(StartHereDoc);
      TokenSyntax endHeredoc = make_token(EndHereDoc);
      HeredocExprSyntax heredoc = make_expr(HeredocExpr, startHeredoc, std::nullopt, endHeredoc);
      ScalarExprSyntax scalarValue = make_expr(ScalarExpr, heredoc);
      $$ = scalarValue.getRaw();
   }
|  T_DOUBLE_QUOTE encaps_list T_DOUBLE_QUOTE {
      TokenSyntax quote = make_token(DoubleStrQuoteToken);
      EncapsItemListSyntax encapsList = make<EncapsItemListSyntax>($2);
      EncapsListStringExprSyntax str = make_expr(EncapsListStringExpr, quote, encapsList, quote);
      ScalarExprSyntax scalarValue = make_expr(ScalarExpr, str);
      $$ = scalarValue.getRaw();
   }
|  T_START_HEREDOC encaps_list T_END_HEREDOC {
      TokenSyntax startHeredoc = make_token(StartHereDoc);
      EncapsItemListSyntax encapsList = make<EncapsItemListSyntax>($2);
      TokenSyntax endHeredoc = make_token(EndHereDoc);
      HeredocExprSyntax heredoc = make_expr(HeredocExpr, startHeredoc, encapsList, endHeredoc);
      ScalarExprSyntax scalarValue = make_expr(ScalarExpr, heredoc);
      $$ = scalarValue.getRaw();
   }
|  dereferencable_scalar {
      DereferencableScalarExprSyntax dscalar = make<DereferencableScalarExprSyntax>($1);
      ScalarExprSyntax scalarValue = make_expr(ScalarExpr, dscalar);
      $$ = scalarValue.getRaw();
   }
|  constant {
      ConstExprSyntax constant = make<ConstExprSyntax>($1);
      ScalarExprSyntax scalarValue = make_expr(ScalarExpr, constant);
      $$ = scalarValue.getRaw();
   }
;

constant:
   name {
      NameSyntax name = make<NameSyntax>($1);
      ConstExprSyntax constant = make_expr(ConstExpr, name);
      $$ = constant.getRaw();
   }
|  class_name T_PAAMAYIM_NEKUDOTAYIM identifier {
      ClassNameClauseSyntax className = make<ClassNameClauseSyntax>($1);
      TokenSyntax paamayimNekudotayimToken = make_token(PaamayimNekudotayimToken);
      IdentifierSyntax identifier = make<IdentifierSyntax>($3);
      ClassConstIdentifierExprSyntax classConst = make_expr(ClassConstIdentifierExpr, className, paamayimNekudotayimToken, identifier);
      ConstExprSyntax constant = make_expr(ConstExpr, classConst);
      $$ = constant.getRaw();
   }
|  variable_class_name T_PAAMAYIM_NEKUDOTAYIM identifier {
      VariableClassNameClauseSyntax className = make<VariableClassNameClauseSyntax>($1);
      TokenSyntax paamayimNekudotayimToken = make_token(PaamayimNekudotayimToken);
      IdentifierSyntax identifier = make<IdentifierSyntax>($3);
      ClassConstIdentifierExprSyntax classConst = make_expr(ClassConstIdentifierExpr, className, paamayimNekudotayimToken, identifier);
      ConstExprSyntax constant = make_expr(ConstExpr, classConst);
      $$ = constant.getRaw();
   }
;

optional_expr:
   %empty {
      $$ = nullptr;
   }
|  expr {
      ExprSyntax expr = make<ExprSyntax>($1);
      $$ = expr.getRaw();
   }
;

variable_class_name:
   dereferencable {
      DereferencableClauseSyntax dereferencable = make<DereferencableClauseSyntax>($1);
      VariableClassNameClauseSyntax className = make_expr(VariableClassNameClause, dereferencable);
      $$ = className.getRaw();
   }
;

dereferencable:
   variable {
      VariableExprSyntax variable = make<VariableExprSyntax>($1);
      DereferencableClauseSyntax dereferencable = make_expr(DereferencableClause, variable);
      $$ = dereferencable.getRaw();
   }
|  T_LEFT_PAREN expr T_RIGHT_PAREN {
      TokenSyntax leftParen = make_token(LeftParenToken);
      ExprSyntax expr = make<ExprSyntax>($2);
      TokenSyntax rightParen = make_token(RightParenToken);
      ParenDecoratedExprSyntax decoratedExpr = make_expr(ParenDecoratedExpr, leftParen, expr, rightParen);
      DereferencableClauseSyntax dereferencable = make_expr(DereferencableClause, decoratedExpr);
      $$ = dereferencable.getRaw();
   }
|  dereferencable_scalar {
      DereferencableScalarExprSyntax scalar = make<DereferencableScalarExprSyntax>($1);
      DereferencableClauseSyntax dereferencable = make_expr(DereferencableClause, scalar);
      $$ = dereferencable.getRaw();
   }
;

callable_expr:
   callable_variable {
      CallableVariableExprSyntax callableVar = make<CallableVariableExprSyntax>($1);
      CallableFuncNameClauseSyntax callableExpr = make_expr(CallableFuncNameClause, callableVar);
      $$ = callableExpr.getRaw();
   }
|  T_LEFT_PAREN expr T_RIGHT_PAREN {
      TokenSyntax leftParen = make_token(LeftParenToken);
      ExprSyntax expr = make<ExprSyntax>($2);
      TokenSyntax rightParen = make_token(RightParenToken);
      ParenDecoratedExprSyntax decoratedExpr = make_expr(ParenDecoratedExpr, leftParen, expr, rightParen);
      CallableFuncNameClauseSyntax callableExpr = make_expr(CallableFuncNameClause, decoratedExpr);
      $$ = callableExpr.getRaw();
   }
|  dereferencable_scalar {
      DereferencableScalarExprSyntax dereferencable = make<DereferencableScalarExprSyntax>($1);
      CallableFuncNameClauseSyntax callableExpr = make_expr(CallableFuncNameClause, dereferencable);
      $$ = callableExpr.getRaw();
   }
;

callable_variable:
   simple_variable {
      SimpleVariableExprSyntax simpleVar = make<SimpleVariableExprSyntax>($1);
      CallableVariableExprSyntax callableVar = make_expr(CallableVariableExpr, simpleVar);
      $$ = callableVar.getRaw();
   }
|  dereferencable T_LEFT_SQUARE_BRACKET optional_expr T_RIGHT_SQUARE_BRACKET {
      DereferencableClauseSyntax dereferencable = make<DereferencableClauseSyntax>($1);
      TokenSyntax leftSquareBracket = make_token(LeftSquareBracketToken);
      OptionalExprSyntax expr = make<OptionalExprSyntax>($3);
      TokenSyntax rightSquareBracket = make_token(RightSquareBracketToken);
      ArrayAccessExprSyntax arrayAccess = make_expr(ArrayAccessExpr, dereferencable, leftSquareBracket, expr, rightSquareBracket);
      CallableVariableExprSyntax callableVar = make_expr(CallableVariableExpr, arrayAccess);
      $$ = callableVar.getRaw();
   }
|  constant T_LEFT_SQUARE_BRACKET optional_expr T_RIGHT_SQUARE_BRACKET {
      ConstExprSyntax constant = make<ConstExprSyntax>($1);
      TokenSyntax leftSquareBracket = make_token(LeftSquareBracketToken);
      OptionalExprSyntax expr = make<OptionalExprSyntax>($3);
      TokenSyntax rightSquareBracket = make_token(RightSquareBracketToken);
      ArrayAccessExprSyntax arrayAccess = make_expr(ArrayAccessExpr, constant, leftSquareBracket, expr, rightSquareBracket);
      CallableVariableExprSyntax callableVar = make_expr(CallableVariableExpr, arrayAccess);
      $$ = callableVar.getRaw();
   }
|  dereferencable T_LEFT_BRACE expr T_RIGHT_BRACE {
      DereferencableClauseSyntax dereferencable = make<DereferencableClauseSyntax>($1);
      TokenSyntax leftBrace = make_token(LeftBraceToken);
      ExprSyntax expr = make<ExprSyntax>($3);
      TokenSyntax rightBrace = make_token(RightBraceToken);
      BraceDecoratedExprClauseSyntax decoratedExpr = make_expr(BraceDecoratedExprClause, leftBrace, expr, rightBrace);
      BraceDecoratedArrayAccessExprSyntax arrayAccess = make_expr(BraceDecoratedArrayAccessExpr, dereferencable, decoratedExpr);
      CallableVariableExprSyntax callableVar = make_expr(CallableVariableExpr, arrayAccess);
      $$ = callableVar.getRaw();
   }
|  dereferencable T_OBJECT_OPERATOR property_name argument_list {
      DereferencableClauseSyntax dereferencable = make<DereferencableClauseSyntax>($1);
      TokenSyntax objOperator = make_token(ObjectOperatorToken);
      PropertyNameClauseSyntax propName = make<PropertyNameClauseSyntax>($3);
      InstancePropertyExprSyntax methodName = make_expr(InstancePropertyExpr, dereferencable, objOperator, propName);
      ArgumentListClauseSyntax args = make<ArgumentListClauseSyntax>($4);
      InstanceMethodCallExprSyntax methodCallExpr = make_expr(InstanceMethodCallExpr, methodName, args);
      CallableVariableExprSyntax callableVar = make_expr(CallableVariableExpr, methodCallExpr);
      $$ = callableVar.getRaw();
   }
|  function_call {
      FunctionCallExprSyntax functionCallExpr = make<FunctionCallExprSyntax>($1);
      CallableVariableExprSyntax callableVar = make_expr(CallableVariableExpr, functionCallExpr);
      $$ = callableVar.getRaw();
   }
;

variable:
   callable_variable {
      CallableVariableExprSyntax callableVar = make<CallableVariableExprSyntax>($1);
      VariableExprSyntax var = make_expr(VariableExpr, callableVar);
      $$ = var.getRaw();
   }
|  static_member {
      StaticPropertyExprSyntax staticMember = make<StaticPropertyExprSyntax>($1);
      VariableExprSyntax var = make_expr(VariableExpr, staticMember);
      $$ = var.getRaw();
   }
|  dereferencable T_OBJECT_OPERATOR property_name {
      DereferencableScalarExprSyntax dereferencableExpr = make<DereferencableScalarExprSyntax>($1);
      TokenSyntax objOperator = make_token(ObjectOperatorToken);
      PropertyNameClauseSyntax propName = make<PropertyNameClauseSyntax>($3);
      InstancePropertyExprSyntax propExpr = make_expr(InstancePropertyExpr, dereferencableExpr, objOperator, propName);
      VariableExprSyntax var = make_expr(VariableExpr, propExpr);
      $$ = var.getRaw();
   }
;

simple_variable:
   T_VARIABLE {
      TokenSyntax variableToken = make_token_with_text(Variable, $1);
      SimpleVariableExprSyntax simpleVariable = make_expr(SimpleVariableExpr, std::nullopt, variableToken);
      $$ = simpleVariable.getRaw();
   }
|  T_DOLLAR_SIGN T_LEFT_BRACE expr T_RIGHT_BRACE {
      TokenSyntax dollarToken = make_token(DollarToken);
      TokenSyntax leftParen = make_token(LeftParenToken);
      ExprSyntax expr = make<ExprSyntax>($3);
      TokenSyntax rightParen = make_token(RightParenToken);
      BraceDecoratedExprClauseSyntax decoratedExpr = make_expr(BraceDecoratedExprClause, leftParen, expr, rightParen);
      BraceDecoratedVariableExprSyntax bracedVarExpr = make_expr(BraceDecoratedVariableExpr, dollarToken, decoratedExpr);
      SimpleVariableExprSyntax simpleVariable = make_expr(SimpleVariableExpr, std::nullopt, bracedVarExpr);
      $$ = simpleVariable.getRaw();
   }
|  T_DOLLAR_SIGN simple_variable {
      TokenSyntax dollarToken = make_token(DollarToken);
      SimpleVariableExprSyntax parentVar = make<SimpleVariableExprSyntax>($2);
      SimpleVariableExprSyntax simpleVariable = make_expr(SimpleVariableExpr, dollarToken, parentVar);
      $$ = simpleVariable.getRaw();
   }
;

static_member:
      class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable {
      ClassNameClauseSyntax className = make<ClassNameClauseSyntax>($1);
      TokenSyntax paamayimNekudotayimToken = make_token(PaamayimNekudotayimToken);
      SimpleVariableExprSyntax simpleVar = make<SimpleVariableExprSyntax>($3);
      StaticPropertyExprSyntax staticMember = make_expr(StaticPropertyExpr, className, paamayimNekudotayimToken, simpleVar);
      $$ = staticMember.getRaw();
   }
|     variable_class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable {
      VariableClassNameClauseSyntax className = make<VariableClassNameClauseSyntax>($1);
      TokenSyntax paamayimNekudotayimToken = make_token(PaamayimNekudotayimToken);
      SimpleVariableExprSyntax simpleVar = make<SimpleVariableExprSyntax>($3);
      StaticPropertyExprSyntax staticMember = make_expr(StaticPropertyExpr, className, paamayimNekudotayimToken, simpleVar);
      $$ = staticMember.getRaw();
   }
;

new_variable:
   simple_variable {
      SimpleVariableExprSyntax simpleVar = make<SimpleVariableExprSyntax>($1);
      NewVariableClauseSyntax newVar = make_expr(NewVariableClause, simpleVar);
      $$ = newVar.getRaw();
   }
|  new_variable T_LEFT_SQUARE_BRACKET optional_expr T_RIGHT_SQUARE_BRACKET {
      NewVariableClauseSyntax var = make<NewVariableClauseSyntax>($1);
      TokenSyntax leftBracket = make_token(LeftSquareBracketToken);
      OptionalExprSyntax expr = make<OptionalExprSyntax>($3);
      TokenSyntax rightBracket = make_token(RightSquareBracketToken);
      ArrayAccessExprSyntax arrayAccess = make_expr(ArrayAccessExpr, var, leftBracket, expr, rightBracket);
      $$ = arrayAccess.getRaw();
   }
|  new_variable T_LEFT_BRACE expr T_RIGHT_BRACE {
      NewVariableClauseSyntax var = make<NewVariableClauseSyntax>($1);
      TokenSyntax leftBrace = make_token(LeftBraceToken);
      ExprSyntax expr = make<ExprSyntax>($3);
      TokenSyntax rightBrace = make_token(RightBraceToken);
      BraceDecoratedExprClauseSyntax decoratedExpr = make_expr(BraceDecoratedExprClause,
         leftBrace, expr, rightBrace);
      BraceDecoratedArrayAccessExprSyntax arrayAccess = make_expr(
         BraceDecoratedArrayAccessExpr, var, decoratedExpr);
      $$ = arrayAccess.getRaw();
   }
|  new_variable T_OBJECT_OPERATOR property_name {
      NewVariableClauseSyntax var = make<NewVariableClauseSyntax>($1);
      TokenSyntax objOperator = make_token(ObjectOperatorToken);
      PropertyNameClauseSyntax propertyName = make<PropertyNameClauseSyntax>($3);
      InstancePropertyExprSyntax propExpr = make_expr(InstancePropertyExpr, var, objOperator, propertyName);
      $$ = propExpr.getRaw();
   }
|  class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable {
      ClassNameClauseSyntax className = make<ClassNameClauseSyntax>($1);
      TokenSyntax paamayimNekudotayimToken = make_token(PaamayimNekudotayimToken);
      SimpleVariableExprSyntax simpleVar = make<SimpleVariableExprSyntax>($3);
      StaticPropertyExprSyntax staticPropExpr = make_expr(StaticPropertyExpr, className, paamayimNekudotayimToken, simpleVar);
      $$ = staticPropExpr.getRaw();
   }
|  new_variable T_PAAMAYIM_NEKUDOTAYIM simple_variable {
      NewVariableClauseSyntax newVar = make<NewVariableClauseSyntax>($1);
      TokenSyntax paamayimNekudotayimToken = make_token(PaamayimNekudotayimToken);
      SimpleVariableExprSyntax simpleVar = make<SimpleVariableExprSyntax>($3);
      StaticPropertyExprSyntax staticPropExpr = make_expr(StaticPropertyExpr, newVar, paamayimNekudotayimToken, simpleVar);
      $$ = staticPropExpr.getRaw();
   }
;

member_name:
   identifier {
      IdentifierSyntax identifier = make<IdentifierSyntax>($1);
      MemberNameClauseSyntax memberName = make_expr(MemberNameClause, identifier);
      $$ = memberName.getRaw();
   }
|  T_LEFT_BRACE expr T_RIGHT_BRACE {
      TokenSyntax leftBrace = make_token(LeftBraceToken);
      ExprSyntax expr = make<ExprSyntax>($2);
      TokenSyntax rightBrace = make_token(RightBraceToken);
      BraceDecoratedExprClauseSyntax decoratedExpr = make_expr(BraceDecoratedExprClause, leftBrace, expr, rightBrace);
      MemberNameClauseSyntax memberName = make_expr(MemberNameClause, decoratedExpr);
      $$ = memberName.getRaw();
   }
|  simple_variable {
      SimpleVariableExprSyntax simple = make<SimpleVariableExprSyntax>($1);
      MemberNameClauseSyntax memberName = make_expr(MemberNameClause, simple);
      $$ = memberName.getRaw();
   }
;

property_name:
   T_IDENTIFIER_STRING {
      TokenSyntax identifierStr = make_token_with_text(IdentifierString, $1);
      PropertyNameClauseSyntax propertyName = make_expr(PropertyNameClause, identifierStr);
      $$ = propertyName.getRaw();
   }
|  T_LEFT_BRACE expr T_RIGHT_BRACE {
      TokenSyntax leftBrace = make_token(LeftBraceToken);
      ExprSyntax expr = make<ExprSyntax>($2);
      TokenSyntax rightBrace = make_token(RightBraceToken);
      BraceDecoratedExprClauseSyntax decoratedExpr = make_expr(BraceDecoratedExprClause, leftBrace, expr, rightBrace);
      PropertyNameClauseSyntax propertyName = make_expr(PropertyNameClause, decoratedExpr);
      $$ = propertyName.getRaw();
   }
|  simple_variable {
      SimpleVariableExprSyntax simpleVar = make<SimpleVariableExprSyntax>($1);
      PropertyNameClauseSyntax propertyName = make_expr(PropertyNameClause, simpleVar);
      $$ = propertyName.getRaw();
   }
;

array_pair_list:
   non_empty_array_pair_list {
      $$ = $1;
   }
;

possible_array_pair:
   %empty {
      $$ = nullptr;
   }
|  array_pair {
      $$ = $1;
   }
;

non_empty_array_pair_list:
   non_empty_array_pair_list T_COMMA possible_array_pair {
      ArrayPairListSyntax list = make<ArrayPairListSyntax>($1);
      TokenSyntax comma = make_token(CommaToken);
      ArrayPairListItemSyntax listItem = make<ArrayPairListItemSyntax>($3);
      list.appending(listItem);
      $$ = list.getRaw();
   }
|  possible_array_pair {
      RefCountPtr<RawSyntax> rawArrayPair = $1;
      ArrayPairListItemSyntax listItem = make_expr(ArrayPairListItem, std::nullopt, 
         rawArrayPair ? std::optional(make<Syntax>(rawArrayPair)) : std::nullopt);
      std::vector<ArrayPairListItemSyntax> items{listItem};
      ArrayPairListSyntax list = make_expr(ArrayPairList, items);
      $$ = list.getRaw();
   }
;

array_pair:
   expr T_DOUBLE_ARROW expr {
      ExprSyntax keyExpr = make<ExprSyntax>($1);
      TokenSyntax arrow = make_token(DoubleArrowToken);
      ExprSyntax valueExpr = make<ExprSyntax>($3);
      ArrayKeyValuePairItemSyntax keyValuePair = make_expr(ArrayKeyValuePairItem, keyExpr, arrow, valueExpr);
      ArrayPairSyntax arrayPair = make_expr(ArrayPair, keyValuePair);
      $$ = arrayPair.getRaw();
   }
|  expr {
      ExprSyntax valueExpr = make<ExprSyntax>($1);
      ArrayKeyValuePairItemSyntax keyValuePair = make_expr(ArrayKeyValuePairItem, std::nullopt, std::nullopt, valueExpr);
      ArrayPairSyntax arrayPair = make_expr(ArrayPair, keyValuePair);
      $$ = arrayPair.getRaw();
   }
|  expr T_DOUBLE_ARROW T_AMPERSAND variable {
      ExprSyntax keyExpr = make<ExprSyntax>($1);
      TokenSyntax arrow = make_token(DoubleArrowToken);
      TokenSyntax refToken = make_token(AmpersandToken);
      VariableExprSyntax variable = make<VariableExprSyntax>($4);
      ReferencedVariableExprSyntax refVariable = make_expr(ReferencedVariableExpr, refToken, variable);
      ArrayKeyValuePairItemSyntax keyValuePair = make_expr(ArrayKeyValuePairItem, keyExpr, arrow, refVariable);
      ArrayPairSyntax arrayPair = make_expr(ArrayPair, keyValuePair);
      $$ = arrayPair.getRaw();
   }
|  T_AMPERSAND variable {
      TokenSyntax refToken = make_token(AmpersandToken);
      VariableExprSyntax variable = make<VariableExprSyntax>($2);
      ReferencedVariableExprSyntax refVariable = make_expr(ReferencedVariableExpr, refToken, variable);
      ArrayKeyValuePairItemSyntax keyValuePair = make_expr(ArrayKeyValuePairItem, std::nullopt, std::nullopt, refVariable);
      ArrayPairSyntax arrayPair = make_expr(ArrayPair, keyValuePair);
      $$ = arrayPair.getRaw();
   }
|  T_ELLIPSIS expr {
      TokenSyntax ellipsisToken = make_token(EllipsisToken);
      ExprSyntax expr = make<ExprSyntax>($2);
      ArrayUnpackPairItemSyntax unpackPair = make_expr(ArrayUnpackPairItem, ellipsisToken, expr);
      ArrayPairSyntax arrayPair = make_expr(ArrayPair, unpackPair);
      $$ = arrayPair.getRaw();
   }
|  expr T_DOUBLE_ARROW T_LIST T_LEFT_PAREN array_pair_list T_RIGHT_PAREN {
      ExprSyntax keyExpr = make<ExprSyntax>($1);
      TokenSyntax arrow = make_token(DoubleArrowToken);
      TokenSyntax listKeyword = make_token(ListKeyword);
      TokenSyntax leftParen = make_token(LeftParenToken);
      ArrayPairListSyntax arrayPairList = make<ArrayPairListSyntax>($5);
      TokenSyntax rightParen = make_token(LeftParenToken);
      ListRecursivePairItemSyntax listRecursivePair = make_expr(ListRecursivePairItem, keyExpr, arrow, listKeyword, 
         leftParen, arrayPairList, rightParen);
      $$ = listRecursivePair.getRaw();
   }
|  T_LIST T_LEFT_PAREN array_pair_list T_RIGHT_PAREN {
      TokenSyntax listKeyword = make_token(ListKeyword);
      TokenSyntax leftParen = make_token(LeftParenToken);
      ArrayPairListSyntax arrayPairList = make<ArrayPairListSyntax>($3);
      TokenSyntax rightParen = make_token(LeftParenToken);
      ListRecursivePairItemSyntax listRecursivePair = make_expr(ListRecursivePairItem, std::nullopt, std::nullopt, listKeyword, 
         leftParen, arrayPairList, rightParen);
      $$ = listRecursivePair.getRaw();
   }
;

encaps_list:
   encaps_list encaps_var {
      EncapsItemListSyntax list = make<EncapsItemListSyntax>($1);
      EncapsVariableSyntax encapsVar = make<EncapsVariableSyntax>($2);
      EncapsListItemSyntax listItem = make_expr(EncapsListItem, std::nullopt, encapsVar);
      list.appending(listItem);
      $$ = list.getRaw();
   }
|  encaps_list T_ENCAPSED_AND_WHITESPACE {
      EncapsItemListSyntax list = make<EncapsItemListSyntax>($1);
      TokenSyntax encapsStr = make_token_with_text(EncapsedAndWhitespace, $2);
      EncapsListItemSyntax listItem = make_expr(EncapsListItem, encapsStr, std::nullopt);
      list.appending(listItem);
      $$ = list.getRaw();
   }
|  encaps_var {
      EncapsVariableSyntax encapsVar = make<EncapsVariableSyntax>($1);
      EncapsListItemSyntax listItem = make_expr(EncapsListItem, std::nullopt, encapsVar);
      std::vector<EncapsListItemSyntax> items{listItem};
      EncapsItemListSyntax list = make_expr(EncapsItemList, items);
      $$ = list.getRaw();
   }
|  T_ENCAPSED_AND_WHITESPACE encaps_var {
      TokenSyntax encapsStr = make_token_with_text(EncapsedAndWhitespace, $1);
      EncapsVariableSyntax encapsVar = make<EncapsVariableSyntax>($2);
      EncapsListItemSyntax strListItem = make_expr(EncapsListItem, encapsStr, std::nullopt);
      EncapsListItemSyntax varListItem = make_expr(EncapsListItem, std::nullopt, encapsVar);
      std::vector<EncapsListItemSyntax> items{strListItem, varListItem};
      EncapsItemListSyntax list = make_expr(EncapsItemList, items);
      $$ = list.getRaw();
   }
;

encaps_var:
   T_VARIABLE {
      TokenSyntax variableToken = make_token_with_text(Variable, $1);
      EncapsVariableSyntax enscapVar = make_expr(EncapsVariable, variableToken);
      $$ = enscapVar.getRaw();
   }
|  T_VARIABLE T_OBJECT_OPERATOR T_IDENTIFIER_STRING {
      TokenSyntax variableToken = make_token_with_text(Variable, $1);
      TokenSyntax objOperator = make_token(ObjectOperatorToken);
      TokenSyntax identifierStr = make_token_with_text(IdentifierString, $3);
      EncapsObjPropSyntax objPropVar = make_expr(EncapsObjProp, variableToken, objOperator, identifierStr);
      EncapsVariableSyntax enscapVar = make_expr(EncapsVariable, objPropVar);
      $$ = enscapVar.getRaw();
   }
|  T_DOLLAR_OPEN_CURLY_BRACES expr T_RIGHT_BRACE {
      TokenSyntax curlyOpenToken = make_token(DollarOpenCurlyBracesToken);
      ExprSyntax expr = make<ExprSyntax>($2);
      TokenSyntax rightBraceToken = make_token(RightBraceToken);
      EncapsDollarCurlyExprSyntax curlyExpr = make_expr(EncapsDollarCurlyExpr, curlyOpenToken, expr, rightBraceToken);
      EncapsVariableSyntax enscapVar = make_expr(EncapsVariable, curlyExpr);
      $$ = enscapVar.getRaw();
   }
|  T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME T_RIGHT_BRACE {
      TokenSyntax curlyOpenToken = make_token(DollarOpenCurlyBracesToken);
      TokenSyntax varname = make_token_with_text(StringVarName, $2);
      TokenSyntax rightBraceToken = make_token(RightBraceToken);
      EncapsDollarCurlyVarSyntax curlyVar = make_expr(EncapsDollarCurlyVariable, curlyOpenToken, varname, rightBraceToken);
      EncapsVariableSyntax enscapVar = make_expr(EncapsVariable, curlyVar);
      $$ = enscapVar.getRaw();
   }
|  T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME T_LEFT_SQUARE_BRACKET expr T_RIGHT_SQUARE_BRACKET T_RIGHT_BRACE {
      TokenSyntax curlyOpenToken = make_token(DollarOpenCurlyBracesToken);
      TokenSyntax varname = make_token_with_text(StringVarName, $2);
      TokenSyntax leftSquareBracketToken = make_token(LeftSquareBracketToken);
      ExprSyntax expr = make<ExprSyntax>($4);
      TokenSyntax rightSquareBracketToken = make_token(RightSquareBracketToken);
      TokenSyntax rightBraceToken = make_token(RightBraceToken);
      EncapsDollarCurlyArraySyntax curlyArrayVar = make_expr(EncapsDollarCurlyArray, curlyOpenToken, varname, leftSquareBracketToken,
         expr, rightSquareBracketToken, rightBraceToken
      );
      EncapsVariableSyntax enscapVar = make_expr(EncapsVariable, curlyArrayVar);
      $$ = enscapVar.getRaw();
   }
|  T_CURLY_OPEN variable T_RIGHT_BRACE {
      TokenSyntax curlyOpenToken = make_token(CurlyOpenToken);
      VariableExprSyntax var = make<VariableExprSyntax>($2);
      TokenSyntax rightBraceToken = make_token(RightBraceToken);
      EncapsCurlyVariableSyntax curlyVar = make_expr(EncapsCurlyVariable, curlyOpenToken, var, rightBraceToken);
      EncapsVariableSyntax enscapVar = make_expr(EncapsVariable, curlyVar);
      $$ = enscapVar.getRaw();
   }
;

internal_functions_in_bison:
   T_ISSET T_LEFT_PAREN isset_variables possible_comma T_RIGHT_PAREN {
      TokenSyntax issetKeyword = make_token(IssetKeyword);
      TokenSyntax leftParen = make_token(LeftParenToken);
      IssetVariablesListSyntax vars = make<IssetVariablesListSyntax>($3);
      TokenSyntax rightParen = make_token(RightParenToken);
      IssetVariablesClauseSyntax issetClause = make_expr(IssetVariablesClause, leftParen, vars, rightParen);
   }
|  T_EMPTY T_LEFT_PAREN expr T_RIGHT_PAREN {
      TokenSyntax emptyKeyword = make_token(EmptyKeyword);
      TokenSyntax leftParen = make_token(LeftParenToken);
      ExprSyntax expr = make<ExprSyntax>($3);
      TokenSyntax rightParen = make_token(RightParenToken);
      ParenDecoratedExprSyntax argsClause = make_expr(ParenDecoratedExpr, leftParen, expr, rightParen);
      EmptyFuncExprSyntax emptyFunc = make_expr(EmptyFuncExpr, emptyKeyword, argsClause);
      $$ = emptyFunc.getRaw();
   }
|  T_INCLUDE expr {
      TokenSyntax includeKeyword = make_token(IncludeKeyword);
      ExprSyntax expr = make<ExprSyntax>($2);
      IncludeExprSyntax includeExpr = make_expr(IncludeExpr, includeKeyword, expr);
      $$ = includeExpr.getRaw();
   }
|  T_INCLUDE_ONCE expr {
      TokenSyntax includeOnceKeywork = make_token(IncludeOnceKeyword);
      ExprSyntax expr = make<ExprSyntax>($2);
      IncludeExprSyntax includeExpr = make_expr(IncludeExpr, includeOnceKeywork, expr);
      $$ = includeExpr.getRaw();
   }
|  T_EVAL T_LEFT_PAREN expr T_RIGHT_PAREN {
      TokenSyntax evalKeyword = make_token(EvalKeyword);
      TokenSyntax leftParen = make_token(LeftParenToken);
      ExprSyntax expr = make<ExprSyntax>($3);
      TokenSyntax rightParen = make_token(RightParenToken);
      ParenDecoratedExprSyntax argsClause = make_expr(ParenDecoratedExpr, leftParen, expr, rightParen);
      EvalFuncExprSyntax evalFunc = make_expr(EvalFuncExpr, evalKeyword, argsClause);
      $$ = evalFunc.getRaw();
   }
|  T_REQUIRE expr {
      TokenSyntax requireKeyword = make_token(RequireKeyword);
      ExprSyntax expr = make<ExprSyntax>($2);
      RequireExprSyntax requireExpr = make_expr(RequireExpr, requireKeyword, expr);
      $$ = requireExpr.getRaw();
   }
|  T_REQUIRE_ONCE expr {
      TokenSyntax requireOnceKeyword = make_token(RequireOnceKeyword);
      ExprSyntax expr = make<ExprSyntax>($2);
      RequireExprSyntax requireExpr = make_expr(RequireExpr, requireOnceKeyword, expr);
      $$ = requireExpr.getRaw();
   }
;

isset_variables:
   isset_variable {
      IssetVariableSyntax issetVar = make<IssetVariableSyntax>($1);
      IssetVariableListItemSyntax issetListItem = make_expr(IssetVariableListItem, std::nullopt, issetVar);
      std::vector<IssetVariableListItemSyntax> issetItems{issetListItem};
      IssetVariablesListSyntax list = make_expr(IssetVariablesList, issetItems);
      $$ = list.getRaw();
   }
|  isset_variables T_COMMA isset_variable {
      IssetVariablesListSyntax list = make<IssetVariablesListSyntax>($1);
      TokenSyntax comma = make_token(CommaToken);
      IssetVariableSyntax issetVar = make<IssetVariableSyntax>($3);
      IssetVariableListItemSyntax issetListItem = make_expr(IssetVariableListItem, comma, issetVar);
      list.appending(issetListItem);
      $$ = list.getRaw();
   }
;

isset_variable:
   expr {
      ExprSyntax expr = make<ExprSyntax>($1);
      IssetVariableSyntax issetVariable = make_expr(IssetVariable, expr);
      $$ = issetVariable.getRaw();
   }
;
%%
