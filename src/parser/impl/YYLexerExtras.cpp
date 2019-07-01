// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/06.

#include "polarphp/parser/internal/YYLexerExtras.h"
#include "polarphp/parser/internal/YYLexerDefs.h"
#include "polarphp/parser/Token.h"
#include "polarphp/parser/Lexer.h"

namespace polar::parser::internal {


int token_lex_wrapper(ParserSemantic *value, YYLocation *loc, Lexer *lexer)
{
   Token token;
   lexer->setSemanticValueContainer(value);
   lexer->lex(token);
   // setup values that parser need
   return token.getKind();
}

size_t count_str_newline(const unsigned char *str, size_t length)
{
   const unsigned char *p = str;
   const unsigned char *boundary = p + length;
   size_t count = 0;
   while (p < boundary) {
      if (*p == '\n' || (*p == '\r' && (*(p+1) != '\n'))) {
         ++count;
      }
      p++;
   }
   return count;
}

void handle_newlines(Lexer &lexer, const unsigned char *str, size_t length)
{
   size_t count = count_str_newline(str, length);
   lexer.incLineNumber(count);
}

void handle_newline(Lexer &lexer, unsigned char c)
{
   if (c == '\n' || c == '\r') {
      lexer.incLineNumber();
   }
}

} // polar::parser::internal
