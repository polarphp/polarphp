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

bool encode_to_utf8(unsigned c,
                    SmallVectorImpl<char> &result)
{
   // Number of bits in the value, ignoring leading zeros.
   unsigned numBits = 32 - polar::utils::count_leading_zeros(c);
   // Handle the leading byte, based on the number of bits in the value.
   unsigned numTrailingBytes;
   if (numBits <= 5 + 6) {
      // Encoding is 0x110aaaaa 10bbbbbb
      result.push_back(char(0xC0 | (c >> 6)));
      numTrailingBytes = 1;
   } else if(numBits <= 4 + 6 + 6) {
      // Encoding is 0x1110aaaa 10bbbbbb 10cccccc
      result.push_back(char(0xE0 | (c >> (6 + 6))));
      numTrailingBytes = 2;

      // UTF-16 surrogate pair values are not valid code points.
      if (c >= 0xD800 && c <= 0xDFFF) {
         return false;
      }
      // U+FDD0...U+FDEF are also reserved
      if (c >= 0xFDD0 && c <= 0xFDEF) {
         return false;
      }
   } else if (numBits <= 3 + 6 + 6 + 6) {
      // Encoding is 0x11110aaa 10bbbbbb 10cccccc 10dddddd
      result.push_back(char(0xF0 | (c >> (6 + 6 + 6))));
      numTrailingBytes = 3;
      // Reject over-large code points.  These cannot be encoded as UTF-16
      // surrogate pairs, so UTF-32 doesn't allow them.
      if (c > 0x10FFFF) {
         return false;
      }
   } else {
      return false;  // UTF8 can encode these, but they aren't valid code points.
   }
   // Emit all of the trailing bytes.
   while (numTrailingBytes--) {
      result.push_back(char(0x80 | (0x3F & (c >> (numTrailingBytes * 6)))));
   }
   return true;
}

unsigned count_leading_ones(unsigned char c)
{
   return polar::utils::count_leading_ones(uint32_t(c) << 24);
}

/// isStartOfUTF8Character - Return true if this isn't a UTF8 continuation
/// character, which will be of the form 0b10XXXXXX
bool is_start_of_utf8_character(unsigned char c)
{
   // RFC 2279: The octet values FE and FF never appear.
   // RFC 3629: The octet values C0, C1, F5 to FF never appear.
   return c > 0x80 && (c < 0xC2 || c >= 0xF5);
}

void strip_underscores(unsigned char *str, int &length)
{
   unsigned char *src = str;
   unsigned char *dest = str;
   while (*src != '\0') {
      if (*src != '_') {
         *dest = *src;
         dest++;
      } else {
         --length;
      }
      src++;
   }
   *dest = '\0';
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

bool convert_double_quote_str_escape_sequences(std::string &filteredStr, char quoteType, const unsigned char *iter,
                                               const unsigned char *endMark, Lexer &lexer)
{
   size_t origLength = endMark - iter;
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
   filteredStr.append(reinterpret_cast<const char *>(iter), origLength);
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

namespace polar::parser {

using namespace internal;

/// validate_utf8_character_and_advance - Given a pointer to the starting byte of a
/// UTF8 character, validate it and advance the lexer past it.  This returns the
/// encoded character or ~0U if the encoding is invalid.
uint32_t validate_utf8_character_and_advance(const unsigned char *&ptr,
                                             const unsigned char *end)
{
   if (ptr >= end) {
      return ~0U;
   }
   unsigned char curByte = *ptr++;
   if (curByte < 0x80) {
      return curByte;
   }
   // Read the number of high bits set, which indicates the number of bytes in
   // the character.
   unsigned encodedBytes = count_leading_ones(curByte);
   // If this is 0b10XXXXXX, then it is a continuation character.
   if (encodedBytes == 1 ||
       is_start_of_utf8_character(curByte)) {
      // Skip until we get the start of another character.  This is guaranteed to
      // at least stop at the nul at the end of the buffer.
      while (ptr < end && is_start_of_utf8_character(*ptr)) {
         ++ptr;
      }
      return ~0U;
   }
   // Drop the high bits indicating the # bytes of the result.
   unsigned c = (unsigned char)(curByte << encodedBytes) >> encodedBytes;

   // Read and validate the continuation bytes.
   for (unsigned i = 1; i != encodedBytes; ++i) {
      if (ptr >= end) {
         return ~0U;
      }
      curByte = *ptr;
      // If the high bit isn't set or the second bit isn't clear, then this is not
      // a continuation byte!
      if (curByte < 0x80 || curByte >= 0xC0) {
         return ~0U;
      }
      // Accumulate our result.
      c <<= 6;
      c |= curByte & 0x3F;
      ++ptr;
   }

   // UTF-16 surrogate pair values are not valid code points.
   if (c >= 0xD800 && c <= 0xDFFF) {
      return ~0U;
   }

   // If we got here, we read the appropriate number of accumulated bytes.
   // Verify that the encoding was actually minimal.
   // Number of bits in the value, ignoring leading zeros.
   unsigned numBits = 32 - polar::utils::count_leading_zeros(c);
   if (numBits <= 5 + 6) {
      return encodedBytes == 2 ? c : ~0U;
   }
   if (numBits <= 4 + 6 + 6) {
      return encodedBytes == 3 ? c : ~0U;
   }
   return encodedBytes == 4 ? c : ~0U;
}
} // polar::parser
