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

#include "polarphp/parser/internal/YYLexerDefs.h"
#include "polarphp/parser/internal/YYLexerExtras.h"
#include "polarphp/parser/Parser.h"
#include "polarphp/parser/Lexer.h"

#include <stdarg.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define RESTART_LEX() goto restart_lex
#define HANDLE_NEWLINES() handle_newlines(lexer, lexer.getYYText(), lexer.getYYLength())

/*!max:re2c */

namespace polar::parser::internal {

void yy_token_lex(Lexer &lexer)
{
restart_lex:
/*!re2c
   re2c:yyfill:check = 0;
   re2c:yyfill:enable = 0;
   LNUM	[0-9]+
   DNUM	([0-9]*"."[0-9]+)|([0-9]+"."[0-9]*)
   EXPONENT_DNUM	(({LNUM}|{DNUM})[eE][+-]?{LNUM})
   HNUM	"0x"[0-9a-fA-F]+
   BNUM	"0b"[01]+
   LABEL	[a-zA-Z_\x80-\xff][a-zA-Z0-9_\x80-\xff]*
   WHITESPACE [ \n\r\t]+
   TABS_AND_SPACES [ \t]*
   TOKENS [;:,.\[\]()|^&+-/*=%!~$<>?@]
   ANY_CHAR [^]
   NEWLINE ("\r"|"\n"|"\r\n")

/* compute yyleng before each rule */
<!*> := lexer.setYYLength(lexer.getYYCursor() - lexer.getYYText());

<ST_IN_SCRIPTING> "true" {
   lexer.formToken(TokenKindType::T_TRUE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "false" {
   lexer.formToken(TokenKindType::T_FALSE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "exit" {
   lexer.formToken(TokenKindType::T_EXIT, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "die" {
   lexer.formToken(TokenKindType::T_EXIT, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "function" {
   lexer.formToken(TokenKindType::T_FUNCTION, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "await" {
   lexer.formToken(TokenKindType::T_AWAIT, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "const" {
   lexer.formToken(TokenKindType::T_CONST, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "return" {
   lexer.formToken(TokenKindType::T_RETURN, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "yield"{WHITESPACE}"from" / [^a-zA-Z0-9_\x80-\xff] {
   HANDLE_NEWLINES();
   lexer.formToken(TokenKindType::T_YIELD_FROM, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "yield" {
   lexer.formToken(TokenKindType::T_YIELD, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "try" {
   lexer.formToken(TokenKindType::T_TRY, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "catch" {
   lexer.formToken(TokenKindType::T_CATCH, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "finally" {
   lexer.formToken(TokenKindType::T_FINALLY, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "throw" {
   lexer.formToken(TokenKindType::T_THROW, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "if" {
   lexer.formToken(TokenKindType::T_IF, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "elseif" {
   lexer.formToken(TokenKindType::T_ELSEIF, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "else" {
   lexer.formToken(TokenKindType::T_ELSE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "while" {
   lexer.formToken(TokenKindType::T_WHILE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "do" {
   lexer.formToken(TokenKindType::T_DO, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "for" {
   lexer.formToken(TokenKindType::T_FOR, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "foreach" {
   lexer.formToken(TokenKindType::T_FOREACH, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "declare" {
   lexer.formToken(TokenKindType::T_DECLARE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "enddeclare" {
   lexer.formToken(TokenKindType::T_ENDDECLARE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "instanceof" {
   lexer.formToken(TokenKindType::T_INSTANCEOF, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "as" {
   lexer.formToken(TokenKindType::T_AS, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "switch" {
   lexer.formToken(TokenKindType::T_SWITCH, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "case" {
   lexer.formToken(TokenKindType::T_CASE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "default" {
   lexer.formToken(TokenKindType::T_DEFAULT, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "break" {
   lexer.formToken(TokenKindType::T_BREAK, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "continue" {
   lexer.formToken(TokenKindType::T_CONTINUE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "fallthrough" {
   lexer.formToken(TokenKindType::T_FALLTHROUGH, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "goto" {
   lexer.formToken(TokenKindType::T_GOTO, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "echo" {
   lexer.formToken(TokenKindType::T_ECHO, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "print" {
   lexer.formToken(TokenKindType::T_PRINT, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "class" {
   lexer.formToken(TokenKindType::T_CLASS, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "interface" {
   lexer.formToken(TokenKindType::T_INTERFACE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "trait" {
   lexer.formToken(TokenKindType::T_TRAIT, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "extends" {
   lexer.formToken(TokenKindType::T_EXTENDS, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "implements" {
   lexer.formToken(TokenKindType::T_IMPLEMENTS, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "->" {
   polar_yy_push_condition(ST_LOOKING_FOR_PROPERTY);
   lexer.formToken(TokenKindType::T_OBJECT_OPERATOR, lexer.getYYText());
   return;
}

<ST_LOOKING_FOR_PROPERTY> "->" {
   lexer.formToken(TokenKindType::T_OBJECT_OPERATOR, lexer.getYYText());
   return;
}

<ST_LOOKING_FOR_PROPERTY> {LABEL} {
   lexer.popYYCondtion();
   lexer.formIdentifierToken(lexer.getYYText());
   return;
}

<ST_LOOKING_FOR_PROPERTY> * {
   lexer.setYYCursor(lexer.getYYText());
   lexer.setYYLength(0);
   lexer.popYYCondtion();
   RESTART_LEX();
   return;
}

<ST_IN_SCRIPTING> "::" {
   lexer.formToken(TokenKindType::T_PAAMAYIM_NEKUDOTAYIM, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "\\" {
   lexer.formToken(TokenKindType::T_NS_SEPARATOR, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "..." {
   lexer.formToken(TokenKindType::T_ELLIPSIS, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "??" {
   lexer.formToken(TokenKindType::T_COALESCE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "new" {
   lexer.formToken(TokenKindType::T_NEW, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "null" {
   lexer.formToken(TokenKindType::T_NULL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "clone" {
   lexer.formToken(TokenKindType::T_CLONE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "var" {
   lexer.formToken(TokenKindType::T_VAR, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "("{TABS_AND_SPACES}("int"|"integer"){TABS_AND_SPACES}")" {
   lexer.formToken(TokenKindType::T_INT_CAST, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "("{TABS_AND_SPACES}("real"|"double"|"float"){TABS_AND_SPACES}")" {
   lexer.formToken(TokenKindType::T_DOUBLE_CAST, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "("{TABS_AND_SPACES}("string"|"binary"){TABS_AND_SPACES}")" {
   lexer.formToken(TokenKindType::T_STRING_CAST, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "("{TABS_AND_SPACES}"array"{TABS_AND_SPACES}")" {
   lexer.formToken(TokenKindType::T_ARRAY_CAST, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "("{TABS_AND_SPACES}"object"{TABS_AND_SPACES}")" {
   lexer.formToken(TokenKindType::T_OBJECT_CAST, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "("{TABS_AND_SPACES}("bool"|"boolean"){TABS_AND_SPACES}")" {
   lexer.formToken(TokenKindType::T_BOOL_CAST, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "("{TABS_AND_SPACES}("unset"){TABS_AND_SPACES}")" {
   lexer.formToken(TokenKindType::T_UNSET_CAST, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "eval" {
   lexer.formToken(TokenKindType::T_EVAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "include" {
   lexer.formToken(TokenKindType::T_INCLUDE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "include_once" {
   lexer.formToken(TokenKindType::T_INCLUDE_ONCE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "require" {
   lexer.formToken(TokenKindType::T_REQUIRE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "require_once" {
   lexer.formToken(TokenKindType::T_REQUIRE_ONCE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "namespace" {
   lexer.formToken(TokenKindType::T_NAMESPACE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "use" {
   lexer.formToken(TokenKindType::T_USE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "insteadof" {
   lexer.formToken(TokenKindType::T_INSTEADOF, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "global" {
   lexer.formToken(TokenKindType::T_GLOBAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "isset" {
   lexer.formToken(TokenKindType::T_ISSET, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "empty" {
   lexer.formToken(TokenKindType::T_EMPTY, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "__halt_compiler" {
   lexer.formToken(TokenKindType::T_HALT_COMPILER, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "static" {
   lexer.formToken(TokenKindType::T_STATIC, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "abstract" {
   lexer.formToken(TokenKindType::T_ABSTRACT, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "final" {
   lexer.formToken(TokenKindType::T_FINAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "private" {
   lexer.formToken(TokenKindType::T_PRIVATE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "protected" {
   lexer.formToken(TokenKindType::T_PROTECTED, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "public" {
   lexer.formToken(TokenKindType::T_PUBLIC, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "this" {
   lexer.formToken(TokenKindType::T_OBJ_REF, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "self" {
   lexer.formToken(TokenKindType::T_CLASS_REF_SELF, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "static" / "::" {
   lexer.formToken(TokenKindType::T_CLASS_REF_STATIC, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "parent" {
   lexer.formToken(TokenKindType::T_CLASS_REF_PARENT, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "unset" {
   lexer.formToken(TokenKindType::T_UNSET, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "list" {
   lexer.formToken(TokenKindType::T_LIST, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "array" {
   lexer.formToken(TokenKindType::T_ARRAY, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "callable" {
   lexer.formToken(TokenKindType::T_CALLABLE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "thread_local" {
   lexer.formToken(TokenKindType::T_THREAD_LOCAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "module" {
   lexer.formToken(TokenKindType::T_MODULE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "package" {
   lexer.formToken(TokenKindType::T_PACKAGE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "async" {
   lexer.formToken(TokenKindType::T_ASYNC, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "export" {
   lexer.formToken(TokenKindType::T_EXPORT, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "defer" {
   lexer.formToken(TokenKindType::T_DEFER, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "__CLASS__" {
   lexer.formToken(TokenKindType::T_CLASS_CONST, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "__TRAIT__" {
   lexer.formToken(TokenKindType::T_TRAIT_CONST, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "__FUNCTION__" {
   lexer.formToken(TokenKindType::T_FUNC_CONST, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "__METHOD__" {
   lexer.formToken(TokenKindType::T_METHOD_CONST, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "__LINE__" {
   lexer.formToken(TokenKindType::T_LINE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "__FILE__" {
   lexer.formToken(TokenKindType::T_FILE, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "__DIR__" {
   lexer.formToken(TokenKindType::T_DIR, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "__NAMESPACE__" {
   lexer.formToken(TokenKindType::T_NS_CONST, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "=>" {
   lexer.formToken(TokenKindType::T_DOUBLE_ARROW, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "++" {
   lexer.formToken(TokenKindType::T_INC, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "--" {
   lexer.formToken(TokenKindType::T_DEC, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "===" {
   lexer.formToken(TokenKindType::T_IS_IDENTICAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "!==" {
   lexer.formToken(TokenKindType::T_IS_NOT_IDENTICAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "==" {
   lexer.formToken(TokenKindType::T_IS_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "!="|"<>" {
   lexer.formToken(TokenKindType::T_IS_NOT_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "<=>" {
   lexer.formToken(TokenKindType::T_SPACESHIP, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "<=" {
   lexer.formToken(TokenKindType::T_IS_SMALLER_OR_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> ">=" {
   lexer.formToken(TokenKindType::T_IS_GREATER_OR_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "+=" {
   lexer.formToken(TokenKindType::T_PLUS_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "-=" {
   lexer.formToken(TokenKindType::T_MINUS_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "*=" {
   lexer.formToken(TokenKindType::T_MUL_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "*\*" {
   lexer.formToken(TokenKindType::T_POW, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "*\*=" {
   lexer.formToken(TokenKindType::T_POW_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "/=" {
   lexer.formToken(TokenKindType::T_DIV_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> ".=" {
   lexer.formToken(TokenKindType::T_STR_CONCAT_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "%=" {
   lexer.formToken(TokenKindType::T_MOD_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "<<=" {
   lexer.formToken(TokenKindType::T_SL_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> ">>=" {
   lexer.formToken(TokenKindType::T_SR_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "&=" {
   lexer.formToken(TokenKindType::T_AND_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "|=" {
   lexer.formToken(TokenKindType::T_OR_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "^=" {
   lexer.formToken(TokenKindType::T_XOR_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "??=" {
   lexer.formToken(TokenKindType::T_COALESCE_EQUAL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "||" {
   lexer.formToken(TokenKindType::T_BOOLEAN_OR, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "&&" {
   lexer.formToken(TokenKindType::T_BOOLEAN_AND, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "OR" {
   lexer.formToken(TokenKindType::T_LOGICAL_OR, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "AND" {
   lexer.formToken(TokenKindType::T_LOGICAL_AND, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "XOR" {
   lexer.formToken(TokenKindType::T_LOGICAL_XOR, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "<<" {
   lexer.formToken(TokenKindType::T_SL, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> ">>" {
   lexer.formToken(TokenKindType::T_SR, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> {TOKENS} {
   lexer.formToken(token_kind_map(lexer.getYYText()[0]), lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "{" {
   polar_yy_push_condition(ST_IN_SCRIPTING);
   lexer.formToken(TokenKindType::T_LEFT_BRACE, lexer.getYYText());
   return;
}

<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC> "${" {
   polar_yy_push_condition(ST_LOOKING_FOR_VARNAME);
   lexer.formToken(TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "}" {
   /// RESET_DOC_COMMENT();
   if (!lexer.yyConditonStackEmpty()) {
      lexer.popYYCondtion();
   }
   lexer.formToken(TokenKindType::T_RIGHT_BRACE, lexer.getYYText());
   return;
}

<ST_LOOKING_FOR_VARNAME> {LABEL} / [[}] {
   lexer.formToken(TokenKindType::T_STRING_VARNAME, lexer.getYYText());
   return;
}

<ST_LOOKING_FOR_VARNAME> * {
   lexer.setYYCursor(lexer.getYYText());
   lexer.setYYLength(0);
   lexer.popYYCondtion();
   polar_yy_push_condition(ST_IN_SCRIPTING);
   RESTART_LEX();
   return;
}

<ST_IN_SCRIPTING> {BNUM} {
   lexer.lexBinaryNumber();
   return;
}

<ST_IN_SCRIPTING> {LNUM} {
   lexer.lexLongNumber();
   return;
}

<ST_IN_SCRIPTING> {HNUM} {
   lexer.lexHexNumber();
   return;
}

<ST_IN_SCRIPTING> {DNUM}|{EXPONENT_DNUM} {
   lexer.lexDoubleNumber();
   return;
}

/* Make sure a label character follows "->", otherwise there is no property
 * and "->" will be taken literally
 */
<ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE> "$"{LABEL} / "->"[a-zA-Z_\x80-\xff] {
   polar_yy_push_condition(ST_LOOKING_FOR_PROPERTY);
   lexer.formVariableToken(lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING,ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE> "$"{LABEL} {
   lexer.formVariableToken(lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> {LABEL} {
   lexer.formIdentifierToken(lexer.getYYText());
   return;
}

<ST_IN_SCRIPTING> "#"|"//" {

}

<ST_IN_SCRIPTING> "/*"|"/**"{WHITESPACE} {

}

<ST_IN_SCRIPTING> b?['] {
   if (lexer.getYYText()[0] == 'b') {
      lexer.setLexingBinaryStrFlag(true);
   }
   lexer.lexSingleQuoteString();
   return;
}

<ST_IN_SCRIPTING> b?["] {
   const unsigned char *yytext = lexer.getYYText();
   if (yytext[0] == 'b') {
      lexer.setLexingBinaryStrFlag(true);
   }
   GOTO_CONDITION(ST_DOUBLE_QUOTES);
   lexer.formToken(TokenKindType::T_DOUBLE_STR_QUOTE, yytext);
   return;
}

<ST_IN_SCRIPTING> b?"<<<"{TABS_AND_SPACES}({LABEL}|([']{LABEL}['])|(["]{LABEL}["])){NEWLINE} {
   lexer.lexHeredocHeader();
   return;
}

<ST_IN_SCRIPTING> [`] {
   GOTO_CONDITION(ST_BACKQUOTE);
   lexer.formToken(TokenKindType::T_BACKTICK, lexer.getYYText());
   return;
}

<ST_END_HEREDOC> * {
   lexer.lexHereAndNowDocEnd();
   return;
}

<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC> "{$" {
   polar_yy_push_condition(ST_IN_SCRIPTING);
   lexer.setYYCursor(lexer.getYYText() + 1);
   lexer.setYYLength(1);
   lexer.formToken(TokenKindType::T_CURLY_OPEN, lexer.getYYText());
   return;
}

<ST_DOUBLE_QUOTES> ["] {
   GOTO_CONDITION(ST_IN_SCRIPTING);
   lexer.setLexingBinaryStrFlag(false);
   lexer.formToken(TokenKindType::T_DOUBLE_STR_QUOTE, lexer.getYYText());
   return;
}

<ST_BACKQUOTE> [`] {
   GOTO_CONDITION(ST_IN_SCRIPTING);
   lexer.formToken(TokenKindType::T_BACKTICK, lexer.getYYText());
   return;
}

<ST_DOUBLE_QUOTES> * {
   lexer.lexDoubleQuoteString();
   return;
}

<ST_BACKQUOTE> * {
   lexer.lexBackquote();
   return;
}

<ST_HEREDOC> * {
   lexer.lexHeredocBody();
   return;
}

<ST_NOWDOC> * {
   lexer.lexNowdocBody();
   return;
}

<ST_IN_SCRIPTING> * {
   if (YYCURSOR >= YYLIMIT) {
      lexer.setYYCursor(lexer.getYYLimit());
      lexer.formToken(TokenKindType::END, lexer.getYYText());
      return;
   }
   /// zend_error(E_COMPILE_WARNING,"Unexpected character in input:  '%c' (ASCII=%d) state=%d", yytext[0], yytext[0], YYSTATE);
   /// lexing recursive
   RESTART_LEX();
   return;
}
*/
}
} // polar::parser::internal
