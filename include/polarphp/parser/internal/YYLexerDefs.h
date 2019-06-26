// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/26.

#ifndef POLARPHP_PARSER_INTERNAL_YY_LEXER_DEFS_H
#define POLARPHP_PARSER_INTERNAL_YY_LEXER_DEFS_H

#include "polarphp/parser/internal/YYLexerConditionDefs.h"
#include "polarphp/parser/internal/YYLocation.h"
#include "polarphp/parser/internal/YYParserDefs.h"
#include "polarphp/parser/internal/YYLexerDefs.h"

namespace polar::parser {
using YYLexerCondType = YYCONDTYPE;

namespace internal {
int token_lex(ParserSemantic *value, location *loc, Lexer *lexer);
} // internal
} // polar::parser

#if 0
# define YYDEBUG(s, c) printf("state: %d char: %c\n", s, c)
#else
# define YYDEBUG(s, c)
#endif

/// for utf8 encoding
#define YYCTYPE   unsigned char
#define YYCURSOR                   lexer->getYYCursor()
#define YYLIMIT                    lexer->getYYLimit()
#define YYMARKER                   lexer->getYYMarker()
#define YYGETCONDITION()           lexer->getYYCondition()
#define YYSETCONDITION(cond)      lexer->setYYCondition(cond)
#define YYFILL(n) { if ((YYCURSOR + n) >= (YYLIMIT)) { return 0; } }
#define COND_NAME(name)            yyc##name
#define polar_yy_push_state(name)  lexer->pushYYCondition(YYCONDTYPE::yyc##name)

#define BOM_UTF32_BE	"\x00\x00\xfe\xff"
#define BOM_UTF32_LE	"\xff\xfe\x00\x00"
#define BOM_UTF16_BE	"\xfe\xff"
#define BOM_UTF16_LE	"\xff\xfe"
#define BOM_UTF8		"\xef\xbb\xbf"

#endif // POLARPHP_PARSER_INTERNAL_YY_LEXER_DEFS_H
