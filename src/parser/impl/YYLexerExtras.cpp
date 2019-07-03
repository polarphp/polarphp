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
#include "polarphp/basic/CharInfo.h"
#include "polarphp/parser/Token.h"
#include "polarphp/parser/Lexer.h"

#include <string>

namespace polar::parser::internal {

using polar::basic::is_hex_digit;

#define POLAR_IS_OCT(c)  ((c)>='0' && (c)<='7')

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

size_t convert_single_quote_str_escape_sequences(char *iter, char *endMark, Lexer &lexer)
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
   *targetStr = 0;
   return iter - origIter;
}

bool convert_double_quote_str_escape_sequences(std::string &filteredStr, char quoteType, char *iter,
                                               char *endMark, Lexer &lexer)
{
   char *origIter = iter;
   size_t origLength = endMark - origIter;
   if (origLength <= 1) {
      if (origLength == 1) {
         char c = *iter;
         if (c == '\n' || c == '\r') {
            lexer.incLineNumber();
         }
         /// TODO
         /// ZVAL_INTERNED_STR(zendlval, ZSTR_CHAR(c));
         filteredStr.push_back(c);
      }
      return true;
   }
   filteredStr.append(iter, origLength);
   /// convert escape sequences
   auto fiter = filteredStr.begin();
   auto fendMark = filteredStr.end();
   /// find first '\'
   while (true) {
      if (*fiter == '\\') {
         break;
      }
      if (*iter == '\n' || (*fiter == '\r' && (*(fiter + 1) != '\n'))) {
         lexer.incLineNumber();
      }
      ++fiter;
      if (fiter == fendMark) {
         return true;
      }
   }
   auto targetIter = fiter;
   while (fiter != fendMark) {
      if (*fiter == '\\') {
         ++fiter;
         if (fiter == fendMark) {
            *targetIter++ = '\\';
            break;
         }
         switch (*fiter) {
         case 'n':
            *targetIter++ = '\n';
            break;
         case 'r':
            *targetIter++ = '\r';
            break;
         case 't':
            *targetIter++ = '\t';
            break;
         case 'f':
            *targetIter++ = '\f';
            break;
         case 'v':
            *targetIter++ = '\v';
            break;
         case 'e':
            *targetIter++ = '\e';
            break;
         case '"':
         case '`':
            if (*fiter != quoteType) {
               *targetIter++ = '\\';
               *targetIter++ = *fiter;
               break;
            }
         case '\\':
         case '$':
            *targetIter++ = *fiter;
            break;
         case 'x':
         case 'X':
            if (is_hex_digit(*(fiter + 1))) {
               char hexBuf[3] = { 0, 0, 0 };
               hexBuf[0] = *(++fiter);
               if (is_hex_digit(*(fiter + 1))) {
                  hexBuf[1] = *(++fiter);
               }
               *targetIter++ = static_cast<char>(std::strtol(hexBuf, nullptr, 16));
            } else {
               *targetIter++ = '\\';
               *targetIter++ = *fiter;
            }
            break;
            /* UTF-8 codepoint escape, format: /\\u\{\x+\}/ */
         case 'u':
         {
            /// cache where we started so we can parse after validating
            auto start = fiter + 1;
            size_t length = 0;
            bool valid = true;
            unsigned long codePoint;
            if (*start != '{') {
               /// we silently let this pass to avoid breaking code
               /// with JSON in string literals (e.g. "\"\u202e\""
               *targetIter++ = '\\';
               *targetIter++ = 'u';
               break;
            } else {
               /// on the other hand, invalid \u{blah} errors
               fiter += 2;
               ++length;
               while (*fiter != '}') {
                  if (!is_hex_digit(*fiter)) {
                     valid = false;
                     break;
                  } else {
                     ++length;
                  }
               }
               if (*fiter == '}') {
                  valid = true;
                  ++length;
               }
            }
            /* \u{} is invalid */
            if (length <= 2) {
               valid = false;
            }
            if (!valid) {
               // zend_throw_exception(zend_ce_parse_error,
               //                      "Invalid UTF-8 codepoint escape sequence", 0);
               return false;
            }
            errno = 0;
            StringRef codePointStr(filteredStr.data() + (start - filteredStr.begin()), fiter - start);
            codePoint = strtoul(codePointStr.getData(), nullptr, 16);
            /// per RFC 3629, UTF-8 can only represent 21 bits
            if (codePoint > 0x10FFFF || errno) {
               //               zend_throw_exception(zend_ce_parse_error,
               //                                    "Invalid UTF-8 codepoint escape sequence: Codepoint too large", 0);
               return false;
            }
            /// based on https://en.wikipedia.org/wiki/UTF-8#Sample_code
            if (codePoint < 0x80) {
               *targetIter++ = codePoint;
            } else if (codePoint <= 0x7FF) {
               *targetIter++ = (codePoint >> 6) + 0xC0;
               *targetIter++ = (codePoint & 0x3F) + 0x80;
            } else if (codePoint <= 0xFFFF) {
               *targetIter++ = (codePoint >> 12) + 0xE0;
               *targetIter++ = ((codePoint >> 6) & 0x3F) + 0x80;
               *targetIter++ = (codePoint & 0x3F) + 0x80;
            } else if (codePoint <= 0x10FFFF) {
               *targetIter++ = (codePoint >> 18) + 0xF0;
               *targetIter++ = ((codePoint >> 12) & 0x3F) + 0x80;
               *targetIter++ = ((codePoint >> 6) & 0x3F) + 0x80;
               *targetIter++ = (codePoint & 0x3F) + 0x80;
            }
         }
            break;
         default:
            /// check for an octal
            if (POLAR_IS_OCT(*fiter)) {
               char octalBuf[4] = { 0, 0, 0, 0 };
               octalBuf[0] = *fiter;
               if (POLAR_IS_OCT(*(fiter + 1))) {
                  octalBuf[1] = *(++fiter);
                  if (POLAR_IS_OCT(*(fiter + 1))) {
                     octalBuf[2] = *(++fiter);
                  }
               }
               if (octalBuf[2] && (octalBuf[0] > '3')) {
                  /// 3 octit values must not overflow 0xFF (\377)
                  /// zend_error(E_COMPILE_WARNING, "Octal escape sequence overflow \\%s is greater than \\377", octal_buf);
               }
               *targetIter++ = static_cast<char>(std::strtol(octalBuf, nullptr, 8));
            } else {
               *targetIter++ = '\\';
               *targetIter++ = *fiter;
            }
            break;
         }
      } else {
         *targetIter++ = *fiter;
      }
      if (*fiter == '\n' || (*fiter == '\r' && (*(fiter + 1) != '\n'))) {
         lexer.incLineNumber();
      }
      ++fiter;
   }
   filteredStr.reserve(targetIter - filteredStr.begin());
   /// TODO
   /// output filtered
   return true;
}

} // polar::parser::internal
