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
#include "polarphp/parser/Parser.h"
#include "polarphp/parser/Lexer.h"

#include <stdarg.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

/*!max:re2c */

namespace polar::parser::internal {
int token_lex(ParserSemantic *value, location *loc, Lexer *lexer)
{
/*!re2c
re2c:yyfill:check = 0;
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
//<!*> := yyleng = YYCURSOR - SCNG(yy_text);

<ST_IN_SCRIPTING>"exit" {

}

<ST_IN_SCRIPTING>"die" {
}

<ST_IN_SCRIPTING>"function" {
}

<ST_IN_SCRIPTING>"const" {
}

<ST_IN_SCRIPTING>"return" {
}

<ST_IN_SCRIPTING>"yield"{WHITESPACE}"from"[^a-zA-Z0-9_\x80-\xff] {
}

<ST_IN_SCRIPTING>"yield" {
}

<ST_IN_SCRIPTING>"try" {
}

<ST_IN_SCRIPTING>"catch" {

}

<ST_IN_SCRIPTING>"finally" {
}

<ST_IN_SCRIPTING>"throw" {
}

<ST_IN_SCRIPTING>"if" {
}

<ST_IN_SCRIPTING>"elseif" {
}

<ST_IN_SCRIPTING>"endif" {

}

<ST_IN_SCRIPTING>"else" {

}

<ST_IN_SCRIPTING>"while" {

}

<ST_IN_SCRIPTING>"endwhile" {

}

<ST_IN_SCRIPTING>"do" {

}

<ST_IN_SCRIPTING>"for" {

}

<ST_IN_SCRIPTING>"endfor" {

}

<ST_IN_SCRIPTING>"foreach" {

}

<ST_IN_SCRIPTING>"endforeach" {

}

<ST_IN_SCRIPTING>"declare" {

}

<ST_IN_SCRIPTING>"enddeclare" {

}

<ST_IN_SCRIPTING>"instanceof" {

}

<ST_IN_SCRIPTING>"as" {

}

<ST_IN_SCRIPTING>"switch" {

}

<ST_IN_SCRIPTING>"endswitch" {

}

<ST_IN_SCRIPTING>"case" {

}

<ST_IN_SCRIPTING>"default" {

}

<ST_IN_SCRIPTING>"break" {

}

<ST_IN_SCRIPTING>"continue" {

}

<ST_IN_SCRIPTING>"goto" {

}

<ST_IN_SCRIPTING>"echo" {

}

<ST_IN_SCRIPTING>"print" {

}

<ST_IN_SCRIPTING>"class" {

}

<ST_IN_SCRIPTING>"interface" {

}

<ST_IN_SCRIPTING>"trait" {

}

<ST_IN_SCRIPTING>"extends" {

}

<ST_IN_SCRIPTING>"implements" {

}

<ST_IN_SCRIPTING>"->" {

}

<ST_IN_SCRIPTING,ST_LOOKING_FOR_PROPERTY>{WHITESPACE}+ {

}

<ST_LOOKING_FOR_PROPERTY>"->" {

}

<ST_LOOKING_FOR_PROPERTY>{LABEL} {

}

<ST_LOOKING_FOR_PROPERTY>{ANY_CHAR} {

}

<ST_IN_SCRIPTING>"::" {

}

<ST_IN_SCRIPTING>"\\" {

}

<ST_IN_SCRIPTING>"..." {

}

<ST_IN_SCRIPTING>"??" {

}

<ST_IN_SCRIPTING>"new" {

}

<ST_IN_SCRIPTING>"clone" {

}

<ST_IN_SCRIPTING>"var" {

}

<ST_IN_SCRIPTING>"("{TABS_AND_SPACES}("int"|"integer"){TABS_AND_SPACES}")" {

}

<ST_IN_SCRIPTING>"("{TABS_AND_SPACES}("real"|"double"|"float"){TABS_AND_SPACES}")" {

}

<ST_IN_SCRIPTING>"("{TABS_AND_SPACES}("string"|"binary"){TABS_AND_SPACES}")" {

}

<ST_IN_SCRIPTING>"("{TABS_AND_SPACES}"array"{TABS_AND_SPACES}")" {

}

<ST_IN_SCRIPTING>"("{TABS_AND_SPACES}"object"{TABS_AND_SPACES}")" {

}

<ST_IN_SCRIPTING>"("{TABS_AND_SPACES}("bool"|"boolean"){TABS_AND_SPACES}")" {

}

<ST_IN_SCRIPTING>"("{TABS_AND_SPACES}("unset"){TABS_AND_SPACES}")" {

}

<ST_IN_SCRIPTING>"eval" {

}

<ST_IN_SCRIPTING>"include" {

}

<ST_IN_SCRIPTING>"include_once" {

}

<ST_IN_SCRIPTING>"require" {

}

<ST_IN_SCRIPTING>"require_once" {

}

<ST_IN_SCRIPTING>"namespace" {

}

<ST_IN_SCRIPTING>"use" {

}

<ST_IN_SCRIPTING>"insteadof" {

}

<ST_IN_SCRIPTING>"global" {

}

<ST_IN_SCRIPTING>"isset" {

}

<ST_IN_SCRIPTING>"empty" {

}

<ST_IN_SCRIPTING>"__halt_compiler" {

}

<ST_IN_SCRIPTING>"static" {

}

<ST_IN_SCRIPTING>"abstract" {

}

<ST_IN_SCRIPTING>"final" {

}

<ST_IN_SCRIPTING>"private" {

}

<ST_IN_SCRIPTING>"protected" {

}

<ST_IN_SCRIPTING>"public" {

}

<ST_IN_SCRIPTING>"unset" {

}

<ST_IN_SCRIPTING>"=>" {

}

<ST_IN_SCRIPTING>"list" {

}

<ST_IN_SCRIPTING>"array" {

}

<ST_IN_SCRIPTING>"callable" {

}

<ST_IN_SCRIPTING>"++" {

}

<ST_IN_SCRIPTING>"--" {

}

<ST_IN_SCRIPTING>"===" {

}

<ST_IN_SCRIPTING>"!==" {

}

<ST_IN_SCRIPTING>"==" {

}

<ST_IN_SCRIPTING>"!="|"<>" {

}

<ST_IN_SCRIPTING>"<=>" {

}

<ST_IN_SCRIPTING>"<=" {

}

<ST_IN_SCRIPTING>">=" {

}

<ST_IN_SCRIPTING>"+=" {

}

<ST_IN_SCRIPTING>"-=" {

}

<ST_IN_SCRIPTING>"*=" {

}

<ST_IN_SCRIPTING>"*\*" {

}

<ST_IN_SCRIPTING>"*\*=" {

}

<ST_IN_SCRIPTING>"/=" {

}

<ST_IN_SCRIPTING>".=" {

}

<ST_IN_SCRIPTING>"%=" {

}

<ST_IN_SCRIPTING>"<<=" {

}

<ST_IN_SCRIPTING>">>=" {

}

<ST_IN_SCRIPTING>"&=" {

}

<ST_IN_SCRIPTING>"|=" {

}

<ST_IN_SCRIPTING>"^=" {

}

<ST_IN_SCRIPTING>"??=" {

}

<ST_IN_SCRIPTING>"||" {

}

<ST_IN_SCRIPTING>"&&" {

}

<ST_IN_SCRIPTING>"OR" {

}

<ST_IN_SCRIPTING>"AND" {

}

<ST_IN_SCRIPTING>"XOR" {

}

<ST_IN_SCRIPTING>"<<" {

}

<ST_IN_SCRIPTING>">>" {

}

<ST_IN_SCRIPTING>{TOKENS} {

}


<ST_IN_SCRIPTING>"{" {

}


<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC>"${" {

}


<ST_IN_SCRIPTING>"}" {

}


<ST_LOOKING_FOR_VARNAME>{LABEL}[[}] {

}


<ST_LOOKING_FOR_VARNAME>{ANY_CHAR} {

}

<ST_IN_SCRIPTING>{BNUM} {
}

<ST_IN_SCRIPTING>{LNUM} {

}

<ST_IN_SCRIPTING>{HNUM} {

}

<ST_VAR_OFFSET>[0]|([1-9][0-9]*) { /* Offset could be treated as a long */

}

<ST_VAR_OFFSET>{LNUM}|{HNUM}|{BNUM} { /* Offset must be treated as a string */

}

<ST_IN_SCRIPTING>{DNUM}|{EXPONENT_DNUM} {

}

<ST_IN_SCRIPTING>"__CLASS__" {

}

<ST_IN_SCRIPTING>"__TRAIT__" {

}

<ST_IN_SCRIPTING>"__FUNCTION__" {

}

<ST_IN_SCRIPTING>"__METHOD__" {

}

<ST_IN_SCRIPTING>"__LINE__" {

}

<ST_IN_SCRIPTING>"__FILE__" {

}

<ST_IN_SCRIPTING>"__DIR__" {

}

<ST_IN_SCRIPTING>"__NAMESPACE__" {

}

/* Make sure a label character follows "->", otherwise there is no property
 * and "->" will be taken literally
 */
<ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE>"$"{LABEL}"->"[a-zA-Z_\x80-\xff] {

}

/* A [ always designates a variable offset, regardless of what follows
 */
<ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE>"$"{LABEL}"[" {

}

<ST_IN_SCRIPTING,ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE,ST_VAR_OFFSET>"$"{LABEL} {

}

<ST_VAR_OFFSET>"]" {

}

<ST_VAR_OFFSET>{TOKENS}|[{}"`] {

}

<ST_VAR_OFFSET>[ \n\r\t\\'#] {

}

<ST_IN_SCRIPTING,ST_VAR_OFFSET>{LABEL} {

}


<ST_IN_SCRIPTING>"#"|"//" {

}

<ST_IN_SCRIPTING>"/*"|"/**"{WHITESPACE} {

}

<ST_IN_SCRIPTING>"?>"{NEWLINE}? {

}


<ST_IN_SCRIPTING>b?['] {

}


<ST_IN_SCRIPTING>b?["] {

}


<ST_IN_SCRIPTING>b?"<<<"{TABS_AND_SPACES}({LABEL}|([']{LABEL}['])|(["]{LABEL}["])){NEWLINE} {

}


<ST_IN_SCRIPTING>[`] {

}


<ST_END_HEREDOC>{ANY_CHAR} {

}


<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC>"{$" {

}


<ST_DOUBLE_QUOTES>["] {

}

<ST_BACKQUOTE>[`] {

}


<ST_DOUBLE_QUOTES>{ANY_CHAR} {

}


<ST_BACKQUOTE>{ANY_CHAR} {

}


<ST_HEREDOC>{ANY_CHAR} {

}


<ST_NOWDOC>{ANY_CHAR} {

}


<ST_IN_SCRIPTING,ST_VAR_OFFSET>{ANY_CHAR} {

}

*/
}
}