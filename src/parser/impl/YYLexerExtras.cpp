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

TokenKindType token_kind_map(unsigned char c)
{
   TokenKindType token;
   switch (c) {
   case ';':
      token = TokenKindType::T_SEMICOLON;
      break;
   case ':':
      token = TokenKindType::T_COLON;
      break;
   case ',':
      token = TokenKindType::T_COMMA;
      break;
   case '[':
      token = TokenKindType::T_LEFT_SQUARE_BRACKET;
      break;
   case ']':
      token = TokenKindType::T_RIGHT_SQUARE_BRACKET;
      break;
   case '(':
      token = TokenKindType::T_LEFT_PAREN;
      break;
   case ')':
      token = TokenKindType::T_RIGHT_PAREN;
      break;
   case '|':
      token = TokenKindType::T_VBAR;
      break;
   case '^':
      token = TokenKindType::T_CARET;
      break;
   case '&':
      token = TokenKindType::T_AMPERSAND;
      break;
   case '+':
      token = TokenKindType::T_PLUS_SIGN;
      break;
   case '-':
      token = TokenKindType::T_MINUS_SIGN;
      break;
   case '/':
      token = TokenKindType::T_DIV_SIGN;
      break;
   case '*':
      token = TokenKindType::T_MUL_SIGN;
      break;
   case '=':
      token = TokenKindType::T_EQUAL;
      break;
   case '%':
      token = TokenKindType::T_MOD_SIGN;
      break;
   case '!':
      token = TokenKindType::T_EXCLAMATION_MARK;
      break;
   case '~':
      token = TokenKindType::T_TILDE;
      break;
   case '$':
      token = TokenKindType::T_DOLLAR_SIGN;
      break;
   case '<':
      token = TokenKindType::T_LEFT_ANGLE;
      break;
   case '>':
      token = TokenKindType::T_RIGHT_ANGLE;
      break;
   case '?':
      token = TokenKindType::T_QUESTION_MARK;
      break;
   case '@':
      token = TokenKindType::T_ERROR_SUPPRESS_SIGN;
      break;
   default:
      token = TokenKindType::T_UNKOWN_MARK;
      break;
   }
   return token;
}

size_t convert_single_escape_sequences(char *iter, char *endMark, Lexer &lexer)
{
   char *origIter = iter;
   /// find first '\'
   while(true) {
      if (*iter == '\\') {
         break;
      }
      if (*iter == '\n' || (*iter == '\r' && (*(iter + 1) != '\n'))) {
         lexer.incLineNumber();
      }
      ++iter;
      if (iter == endMark) {
         return iter - origIter;
      }
   }
   char *targetStr = iter;
   while (targetStr < endMark) {
      if (*iter == '\\') {
         ++iter;
         if (*iter == '\\' || *iter == '\'') {
            *targetStr++ = *iter;
         } else {
            *targetStr++ = '\\';
            *targetStr++ = *iter;
         }
      } else {
         *targetStr = *iter;
      }
      if (*iter == '\n' || (*iter == '\r' && (*(iter + 1) != '\n'))) {
         lexer.incLineNumber();
      }
      ++iter;
   }
   return iter - origIter;
}

} // polar::parser::internal
