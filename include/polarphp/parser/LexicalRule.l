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
#include "polarphp/parser/internal/YYParserExtraDefs.h"
#include "polarphp/parser/Parser.h"
#include "polarphp/parser/Lexer.h"

#include <stdarg.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define RESTART_LEX() goto restart_lex

/*!max:re2c */

namespace polar::parser::internal {

void yy_token_lex(Lexer &lexer)
{
restart_lex:
/*!re2c
   re2c:yyfill:check = 0;
   re2c:yyfill:enable = 0;
   LNUM	[0-9]+(_[0-9]+)*
   DNUM	({LNUM}?"."{LNUM})|({LNUM}"."{LNUM}?)
   EXPONENT_DNUM	(({LNUM}|{DNUM})[eE][+-]?{LNUM})
   HNUM	"0x"[0-9a-fA-F]+(_[0-9a-fA-F]+)*
   BNUM	"0b"[01]+(_[01]+)*
   LABEL	[a-zA-Z_\x80-\xff][a-zA-Z0-9_\x80-\xff]*
   WHITESPACE [ \n\r\t]+
   TABS_AND_SPACES [ \t]*
   TOKENS [;:,.\[\]()|^&+-/*=%!~$<>?@]
   NEWLINE ("\r"|"\n"|"\r\n")

/* compute yyleng before each rule */
<!*> := lexer.setYYLength(lexer.getYYCursor() - lexer.getYYText());

<ST_IN_SCRIPTING> "true" {
   lexer.formToken(TokenKindType::T_TRUE);
   return;
}

<ST_IN_SCRIPTING> "false" {
   lexer.formToken(TokenKindType::T_FALSE);
   return;
}

<ST_IN_SCRIPTING> "exit" {
   lexer.formToken(TokenKindType::T_EXIT);
   return;
}

<ST_IN_SCRIPTING> "die" {
   lexer.formToken(TokenKindType::T_EXIT);
   return;
}

<ST_IN_SCRIPTING> "fn" {
   lexer.formToken(TokenKindType::T_FN);
   return;
}

<ST_IN_SCRIPTING> "function" {
   lexer.formToken(TokenKindType::T_FUNCTION);
   return;
}

<ST_IN_SCRIPTING> "const" {
   lexer.formToken(TokenKindType::T_CONST);
   return;
}

<ST_IN_SCRIPTING> "return" {
   lexer.formToken(TokenKindType::T_RETURN);
   return;
}

<ST_IN_SCRIPTING> "await" {
   lexer.formToken(TokenKindType::T_AWAIT);
   return;
}

<ST_IN_SCRIPTING> "yield"{WHITESPACE}"from" / [^a-zA-Z0-9_\x80-\xff] {
   lexer.formToken(TokenKindType::T_YIELD_FROM);
   return;
}

<ST_IN_SCRIPTING> "yield" {
   lexer.formToken(TokenKindType::T_YIELD);
   return;
}

<ST_IN_SCRIPTING> "try" {
   lexer.formToken(TokenKindType::T_TRY);
   return;
}

<ST_IN_SCRIPTING> "catch" {
   lexer.formToken(TokenKindType::T_CATCH);
   return;
}

<ST_IN_SCRIPTING> "finally" {
   lexer.formToken(TokenKindType::T_FINALLY);
   return;
}

<ST_IN_SCRIPTING> "throw" {
   lexer.formToken(TokenKindType::T_THROW);
   return;
}

<ST_IN_SCRIPTING> "if" {
   lexer.formToken(TokenKindType::T_IF);
   return;
}

<ST_IN_SCRIPTING> "elseif" {
   lexer.formToken(TokenKindType::T_ELSEIF);
   return;
}

<ST_IN_SCRIPTING> "else" {
   lexer.formToken(TokenKindType::T_ELSE);
   return;
}

<ST_IN_SCRIPTING> "while" {
   lexer.formToken(TokenKindType::T_WHILE);
   return;
}

<ST_IN_SCRIPTING> "do" {
   lexer.formToken(TokenKindType::T_DO);
   return;
}

<ST_IN_SCRIPTING> "for" {
   lexer.formToken(TokenKindType::T_FOR);
   return;
}

<ST_IN_SCRIPTING> "foreach" {
   lexer.formToken(TokenKindType::T_FOREACH);
   return;
}

<ST_IN_SCRIPTING> "declare" {
   lexer.formToken(TokenKindType::T_DECLARE);
   return;
}

<ST_IN_SCRIPTING> "instanceof" {
   lexer.formToken(TokenKindType::T_INSTANCEOF);
   return;
}

<ST_IN_SCRIPTING> "as" {
   lexer.formToken(TokenKindType::T_AS);
   return;
}

<ST_IN_SCRIPTING> "switch" {
   lexer.formToken(TokenKindType::T_SWITCH);
   return;
}

<ST_IN_SCRIPTING> "case" {
   lexer.formToken(TokenKindType::T_CASE);
   return;
}

<ST_IN_SCRIPTING> "default" {
   lexer.formToken(TokenKindType::T_DEFAULT);
   return;
}

<ST_IN_SCRIPTING> "break" {
   lexer.formToken(TokenKindType::T_BREAK);
   return;
}

<ST_IN_SCRIPTING> "continue" {
   lexer.formToken(TokenKindType::T_CONTINUE);
   return;
}

<ST_IN_SCRIPTING> "goto" {
   lexer.formToken(TokenKindType::T_GOTO);
   return;
}

<ST_IN_SCRIPTING> "fallthrough" {
   lexer.formToken(TokenKindType::T_FALLTHROUGH);
   return;
}

<ST_IN_SCRIPTING> "echo" {
   lexer.formToken(TokenKindType::T_ECHO);
   return;
}

<ST_IN_SCRIPTING> "print" {
   lexer.formToken(TokenKindType::T_PRINT);
   return;
}

<ST_IN_SCRIPTING> "class" {
   lexer.formToken(TokenKindType::T_CLASS);
   return;
}

<ST_IN_SCRIPTING> "interface" {
   lexer.formToken(TokenKindType::T_INTERFACE);
   return;
}

<ST_IN_SCRIPTING> "trait" {
   lexer.formToken(TokenKindType::T_TRAIT);
   return;
}

<ST_IN_SCRIPTING> "extends" {
   lexer.formToken(TokenKindType::T_EXTENDS);
   return;
}

<ST_IN_SCRIPTING> "implements" {
   lexer.formToken(TokenKindType::T_IMPLEMENTS);
   return;
}

<ST_IN_SCRIPTING> "->" {
   polar_yy_push_condition(ST_LOOKING_FOR_PROPERTY);
   lexer.formToken(TokenKindType::T_OBJECT_OPERATOR);
   return;
}

<ST_LOOKING_FOR_PROPERTY> "->" {
   lexer.formToken(TokenKindType::T_OBJECT_OPERATOR);
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
}

<ST_IN_SCRIPTING> "::" {
   lexer.formToken(TokenKindType::T_PAAMAYIM_NEKUDOTAYIM);
   return;
}

<ST_IN_SCRIPTING> "\\" {
   lexer.formToken(TokenKindType::T_NS_SEPARATOR);
   return;
}

<ST_IN_SCRIPTING> "..." {
   lexer.formToken(TokenKindType::T_ELLIPSIS);
   return;
}

<ST_IN_SCRIPTING> "??" {
   lexer.formToken(TokenKindType::T_COALESCE);
   return;
}

<ST_IN_SCRIPTING> "new" {
   lexer.formToken(TokenKindType::T_NEW);
   return;
}

<ST_IN_SCRIPTING> "null" {
   lexer.formToken(TokenKindType::T_NULL);
   return;
}

<ST_IN_SCRIPTING> "clone" {
   lexer.formToken(TokenKindType::T_CLONE);
   return;
}

<ST_IN_SCRIPTING> "var" {
   lexer.formToken(TokenKindType::T_VAR);
   return;
}

<ST_IN_SCRIPTING> "("{TABS_AND_SPACES}("int"|"integer"){TABS_AND_SPACES}")" {
   lexer.formToken(TokenKindType::T_INT_CAST);
   return;
}

<ST_IN_SCRIPTING> "("{TABS_AND_SPACES}("double"|"float"){TABS_AND_SPACES}")" {
   lexer.formToken(TokenKindType::T_DOUBLE_CAST);
   return;
}

<ST_IN_SCRIPTING> "("{TABS_AND_SPACES}("string"|"binary"){TABS_AND_SPACES}")" {
   lexer.formToken(TokenKindType::T_STRING_CAST);
   return;
}

<ST_IN_SCRIPTING> "("{TABS_AND_SPACES}"array"{TABS_AND_SPACES}")" {
   lexer.formToken(TokenKindType::T_ARRAY_CAST);
   return;
}

<ST_IN_SCRIPTING> "("{TABS_AND_SPACES}"object"{TABS_AND_SPACES}")" {
   lexer.formToken(TokenKindType::T_OBJECT_CAST);
   return;
}

<ST_IN_SCRIPTING> "("{TABS_AND_SPACES}("bool"|"boolean"){TABS_AND_SPACES}")" {
   lexer.formToken(TokenKindType::T_BOOL_CAST);
   return;
}

<ST_IN_SCRIPTING> "("{TABS_AND_SPACES}("unset"){TABS_AND_SPACES}")" {
   lexer.formToken(TokenKindType::T_UNSET_CAST);
   return;
}

<ST_IN_SCRIPTING> "eval" {
   lexer.formToken(TokenKindType::T_EVAL);
   return;
}

<ST_IN_SCRIPTING> "include" {
   lexer.formToken(TokenKindType::T_INCLUDE);
   return;
}

<ST_IN_SCRIPTING> "include_once" {
   lexer.formToken(TokenKindType::T_INCLUDE_ONCE);
   return;
}

<ST_IN_SCRIPTING> "require" {
   lexer.formToken(TokenKindType::T_REQUIRE);
   return;
}

<ST_IN_SCRIPTING> "require_once" {
   lexer.formToken(TokenKindType::T_REQUIRE_ONCE);
   return;
}

<ST_IN_SCRIPTING> "namespace" {
   lexer.formToken(TokenKindType::T_NAMESPACE);
   return;
}

<ST_IN_SCRIPTING> "use" {
   lexer.formToken(TokenKindType::T_USE);
   return;
}

<ST_IN_SCRIPTING> "insteadof" {
   lexer.formToken(TokenKindType::T_INSTEADOF);
   return;
}

<ST_IN_SCRIPTING> "global" {
   lexer.formToken(TokenKindType::T_GLOBAL);
   return;
}

<ST_IN_SCRIPTING> "isset" {
   lexer.formToken(TokenKindType::T_ISSET);
   return;
}

<ST_IN_SCRIPTING> "empty" {
   lexer.formToken(TokenKindType::T_EMPTY);
   return;
}

<ST_IN_SCRIPTING> "__halt_compiler" {
   lexer.formToken(TokenKindType::T_HALT_COMPILER);
   return;
}

<ST_IN_SCRIPTING> "static" {
   lexer.formToken(TokenKindType::T_STATIC);
   return;
}

<ST_IN_SCRIPTING> "abstract" {
   lexer.formToken(TokenKindType::T_ABSTRACT);
   return;
}

<ST_IN_SCRIPTING> "final" {
   lexer.formToken(TokenKindType::T_FINAL);
   return;
}

<ST_IN_SCRIPTING> "private" {
   lexer.formToken(TokenKindType::T_PRIVATE);
   return;
}

<ST_IN_SCRIPTING> "protected" {
   lexer.formToken(TokenKindType::T_PROTECTED);
   return;
}

<ST_IN_SCRIPTING> "public" {
   lexer.formToken(TokenKindType::T_PUBLIC);
   return;
}

<ST_IN_SCRIPTING> "this" {
   lexer.formToken(TokenKindType::T_OBJ_REF);
   return;
}

<ST_IN_SCRIPTING> "self" {
   lexer.formToken(TokenKindType::T_CLASS_REF_SELF);
   return;
}

<ST_IN_SCRIPTING> "static" / "::" {
   lexer.formToken(TokenKindType::T_CLASS_REF_STATIC);
   return;
}

<ST_IN_SCRIPTING> "parent" {
   lexer.formToken(TokenKindType::T_CLASS_REF_PARENT);
   return;
}

<ST_IN_SCRIPTING> "unset" {
   lexer.formToken(TokenKindType::T_UNSET);
   return;
}

<ST_IN_SCRIPTING> "list" {
   lexer.formToken(TokenKindType::T_LIST);
   return;
}

<ST_IN_SCRIPTING> "array" {
   lexer.formToken(TokenKindType::T_ARRAY);
   return;
}

<ST_IN_SCRIPTING> "callable" {
   lexer.formToken(TokenKindType::T_CALLABLE);
   return;
}

<ST_IN_SCRIPTING> "thread_local" {
   lexer.formToken(TokenKindType::T_THREAD_LOCAL);
   return;
}

<ST_IN_SCRIPTING> "module" {
   lexer.formToken(TokenKindType::T_MODULE);
   return;
}

<ST_IN_SCRIPTING> "package" {
   lexer.formToken(TokenKindType::T_PACKAGE);
   return;
}

<ST_IN_SCRIPTING> "async" {
   lexer.formToken(TokenKindType::T_ASYNC);
   return;
}

<ST_IN_SCRIPTING> "export" {
   lexer.formToken(TokenKindType::T_EXPORT);
   return;
}

<ST_IN_SCRIPTING> "defer" {
   lexer.formToken(TokenKindType::T_DEFER);
   return;
}

<ST_IN_SCRIPTING> "__CLASS__" {
   lexer.formToken(TokenKindType::T_CLASS_CONST);
   return;
}

<ST_IN_SCRIPTING> "__TRAIT__" {
   lexer.formToken(TokenKindType::T_TRAIT_CONST);
   return;
}

<ST_IN_SCRIPTING> "__FUNCTION__" {
   lexer.formToken(TokenKindType::T_FUNC_CONST);
   return;
}

<ST_IN_SCRIPTING> "__METHOD__" {
   lexer.formToken(TokenKindType::T_METHOD_CONST);
   return;
}

<ST_IN_SCRIPTING> "__LINE__" {
   lexer.formToken(TokenKindType::T_LINE);
   return;
}

<ST_IN_SCRIPTING> "__FILE__" {
   lexer.formToken(TokenKindType::T_FILE);
   return;
}

<ST_IN_SCRIPTING> "__DIR__" {
   lexer.formToken(TokenKindType::T_DIR);
   return;
}

<ST_IN_SCRIPTING> "__NAMESPACE__" {
   lexer.formToken(TokenKindType::T_NS_CONST);
   return;
}

<ST_IN_SCRIPTING> "=>" {
   lexer.formToken(TokenKindType::T_DOUBLE_ARROW);
   return;
}

<ST_IN_SCRIPTING> "++" {
   lexer.formToken(TokenKindType::T_INC);
   return;
}

<ST_IN_SCRIPTING> "--" {
   lexer.formToken(TokenKindType::T_DEC);
   return;
}

<ST_IN_SCRIPTING> "===" {
   lexer.formToken(TokenKindType::T_IS_IDENTICAL);
   return;
}

<ST_IN_SCRIPTING> "!==" {
   lexer.formToken(TokenKindType::T_IS_NOT_IDENTICAL);
   return;
}

<ST_IN_SCRIPTING> "==" {
   lexer.formToken(TokenKindType::T_IS_EQUAL);
   return;
}

<ST_IN_SCRIPTING> "!="|"<>" {
   lexer.formToken(TokenKindType::T_IS_NOT_EQUAL);
   return;
}

<ST_IN_SCRIPTING> "<=>" {
   lexer.formToken(TokenKindType::T_SPACESHIP);
   return;
}

<ST_IN_SCRIPTING> "<=" {
   lexer.formToken(TokenKindType::T_IS_SMALLER_OR_EQUAL);
   return;
}

<ST_IN_SCRIPTING> ">=" {
   lexer.formToken(TokenKindType::T_IS_GREATER_OR_EQUAL);
   return;
}

<ST_IN_SCRIPTING> "+=" {
   lexer.formToken(TokenKindType::T_PLUS_EQUAL);
   return;
}

<ST_IN_SCRIPTING> "-=" {
   lexer.formToken(TokenKindType::T_MINUS_EQUAL);
   return;
}

<ST_IN_SCRIPTING> "*=" {
   lexer.formToken(TokenKindType::T_MUL_EQUAL);
   return;
}

<ST_IN_SCRIPTING> "*\*" {
   lexer.formToken(TokenKindType::T_POW);
   return;
}

<ST_IN_SCRIPTING> "*\*=" {
   lexer.formToken(TokenKindType::T_POW_EQUAL);
   return;
}

<ST_IN_SCRIPTING> "/=" {
   lexer.formToken(TokenKindType::T_DIV_EQUAL);
   return;
}

<ST_IN_SCRIPTING> ".=" {
   lexer.formToken(TokenKindType::T_STR_CONCAT_EQUAL);
   return;
}

<ST_IN_SCRIPTING> "%=" {
   lexer.formToken(TokenKindType::T_MOD_EQUAL);
   return;
}

<ST_IN_SCRIPTING> "<<=" {
   lexer.formToken(TokenKindType::T_SL_EQUAL);
   return;
}

<ST_IN_SCRIPTING> ">>=" {
   lexer.formToken(TokenKindType::T_SR_EQUAL);
   return;
}

<ST_IN_SCRIPTING> "&=" {
   lexer.formToken(TokenKindType::T_AND_EQUAL);
   return;
}

<ST_IN_SCRIPTING> "|=" {
   lexer.formToken(TokenKindType::T_OR_EQUAL);
   return;
}

<ST_IN_SCRIPTING> "^=" {
   lexer.formToken(TokenKindType::T_XOR_EQUAL);
   return;
}

<ST_IN_SCRIPTING> "??=" {
   lexer.formToken(TokenKindType::T_COALESCE_EQUAL);
   return;
}

<ST_IN_SCRIPTING> "||" {
   lexer.formToken(TokenKindType::T_BOOLEAN_OR);
   return;
}

<ST_IN_SCRIPTING> "&&" {
   lexer.formToken(TokenKindType::T_BOOLEAN_AND);
   return;
}

<ST_IN_SCRIPTING> "OR" {
   lexer.formToken(TokenKindType::T_LOGICAL_OR);
   return;
}

<ST_IN_SCRIPTING> "AND" {
   lexer.formToken(TokenKindType::T_LOGICAL_AND);
   return;
}

<ST_IN_SCRIPTING> "XOR" {
   lexer.formToken(TokenKindType::T_LOGICAL_XOR);
   return;
}

<ST_IN_SCRIPTING> "<<" {
   lexer.formToken(TokenKindType::T_SL);
   return;
}

<ST_IN_SCRIPTING> ">>" {
   lexer.formToken(TokenKindType::T_SR);
   return;
}

<ST_IN_SCRIPTING> {TOKENS} {
   lexer.formToken(get_token_kind_by_char(lexer.getYYText()[0]));
   return;
}

<ST_IN_SCRIPTING> "{" {
   polar_yy_push_condition(ST_IN_SCRIPTING);
   lexer.formToken(TokenKindType::T_LEFT_BRACE);
   return;
}

<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC> "${" {
   polar_yy_push_condition(ST_LOOKING_FOR_VARNAME);
   lexer.formToken(TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES);
   return;
}

<ST_IN_SCRIPTING> "}" {
   RESET_DOC_COMMENT();
   if (!lexer.yyConditonStackEmpty()) {
      lexer.popYYCondtion();
   }
   if (lexer.getYYCondition() == COND_NAME(ST_HEREDOC) && lexer.nextLineHasHeredocEndMarker()) {
      /// check next line is heredoc endmarker
      lexer.m_flags.setReserveHeredocSpaces(true);
   }
   lexer.formToken(TokenKindType::T_RIGHT_BRACE);
   return;
}

<ST_LOOKING_FOR_VARNAME> {LABEL} / [[}] {
   lexer.popYYCondtion();
   polar_yy_push_condition(ST_IN_SCRIPTING);
   lexer.formStringVariableToken();
   return;
}

<ST_LOOKING_FOR_VARNAME> * {
   lexer.setYYCursor(lexer.getYYText());
   lexer.setYYLength(0);
   lexer.popYYCondtion();
   polar_yy_push_condition(ST_IN_SCRIPTING);
   RESTART_LEX();
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
   lexer.formVariableToken();
   return;
}

<ST_IN_SCRIPTING,ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE> "$"{LABEL} {
   lexer.formVariableToken();
   return;
}

<ST_IN_SCRIPTING> {LABEL} {
   lexer.formIdentifierToken();
   return;
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
   lexer.formToken(TokenKindType::T_DOUBLE_QUOTE);
   return;
}

<ST_IN_SCRIPTING> b?"<<<"{TABS_AND_SPACES}({LABEL}|([']{LABEL}['])|(["]{LABEL}["])){NEWLINE} {
   lexer.lexHeredocHeader();
   return;
}

<ST_IN_SCRIPTING> [`] {
   GOTO_CONDITION(ST_BACKQUOTE);
   lexer.formToken(TokenKindType::T_BACKTICK);
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
   lexer.formToken(TokenKindType::T_CURLY_OPEN);
   return;
}

<ST_DOUBLE_QUOTES> ["] {
   if (lexer.m_nextToken.getKind() == TokenKindType::T_DOUBLE_QUOTE) {
      lexer.setYYLength(0);
      lexer.formToken(TokenKindType::T_CONSTANT_ENCAPSED_STRING, lexer.getYYText() - 1);
      /// use some flag to represent empty string
      lexer.m_nextToken.setValue("");
      lexer.setYYCursor(lexer.getYYText());
      return;
   }
   GOTO_CONDITION(ST_IN_SCRIPTING);
   lexer.setLexingBinaryStrFlag(false);
   lexer.formToken(TokenKindType::T_DOUBLE_QUOTE);
   return;
}

<ST_DOUBLE_QUOTES> * {
   lexer.lexDoubleQuoteString();
   return;
}

<ST_BACKQUOTE> [`] {
   if (lexer.m_nextToken.getKind() == TokenKindType::T_BACKTICK) {
      /// check whether is empty string
      lexer.setYYLength(0);
      lexer.formToken(TokenKindType::T_CONSTANT_ENCAPSED_STRING, lexer.getYYText() - 1);
      /// TODO
      /// use some flag to represent empty string
      lexer.m_nextToken.setValue("");
      lexer.setYYCursor(lexer.getYYText());
      return;
   }
   GOTO_CONDITION(ST_IN_SCRIPTING);
   lexer.formToken(TokenKindType::T_BACKTICK);
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
      lexer.formToken(TokenKindType::END);
      return;
   }
   /// zend_error(E_COMPILE_WARNING,"Unexpected character in input:  '%c' (ASCII=%d) state=%d", yytext[0], yytext[0], YYSTATE);
   /// lexing recursive
   RESTART_LEX();
}

<INITIAL>"<?php"([ \t]|{NEWLINE}) {
   lexer.setYYText(lexer.getYYText() + lexer.getYYLength());
   GOTO_CONDITION(ST_IN_SCRIPTING);
   RESTART_LEX();
}

<INITIAL>"<?php" {
   lexer.setYYText(lexer.getYYText() + lexer.getYYLength());
   GOTO_CONDITION(ST_IN_SCRIPTING);
   RESTART_LEX();
}

<INITIAL>"<?polar"([ \t]|{NEWLINE}) {
   lexer.setYYText(lexer.getYYText() + lexer.getYYLength());
   GOTO_CONDITION(ST_IN_SCRIPTING);
   RESTART_LEX();
}

<INITIAL>"<?polar" {
   lexer.setYYText(lexer.getYYText() + lexer.getYYLength());
   GOTO_CONDITION(ST_IN_SCRIPTING);
   RESTART_LEX();
}

<INITIAL> * {
   lexer.setYYCursor(--lexer.getYYCursor());
   GOTO_CONDITION(ST_IN_SCRIPTING);
   RESTART_LEX();
}

*/
}
} // polar::parser::internal
