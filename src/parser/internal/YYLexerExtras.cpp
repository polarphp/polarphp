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
#include "clang/Basic/CharInfo.h"
#include "polarphp/parser/Token.h"
#include "polarphp/parser/Lexer.h"
#include "polarphp/parser/Parser.h"
#include "polarphp/utils/MathExtras.h"

#include <string>

namespace polar::parser::internal {

using namespace polar;

#define POLAR_IS_OCT(c)  ((c)>='0' && (c)<='7')

int token_lex_wrapper(ParserSemantic *value, YYLocation *loc, Lexer *lexer, Parser *parser)
{
   Token token;
   lexer->setSemanticValueContainer(value);
   lexer->lex(token);
   // setup values that parser need
   Token::ValueType valueType = token.getValueType();
   if (valueType == Token::ValueType::LongLong) {
      value->emplace<std::int64_t>(token.getValue<std::int64_t>());
   } else if (valueType == Token::ValueType::Double) {
      value->emplace<double>(token.getValue<double>());
   } else if (valueType == Token::ValueType::String) {
      value->emplace<std::string>(token.getValue<std::string>());
   }
   parser->m_token = token;
   return token.getKind();
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

void diagnose_embedded_null(DiagnosticEngine *diags, const unsigned char *ptr)
{
   assert(ptr && "invalid source location");
   assert(*ptr == '\0' && "not an embedded null");

   if (!diags) {
      return;
   }

   SourceLoc nullLoc = Lexer::getSourceLoc(ptr);
   SourceLoc nullEndLoc = Lexer::getSourceLoc(ptr+1);
   //   diags->diagnose(nullLoc, diag::lex_nul_character)
   //         .fixItRemoveChars(nullLoc, nullEndLoc);
}

/// Advance \p m_yyCursor to the end of line or the end of file. Returns \c true
/// if it stopped at the end of line, \c false if it stopped at the end of file.
bool advance_to_end_of_line(const unsigned char *&m_yyCursor, const unsigned char *bufferEnd,
                            const unsigned char *codeCompletionPtr, DiagnosticEngine *diags) {
   while (1) {
      switch (*m_yyCursor++) {
      case '\n':
      case '\r':
         --m_yyCursor;
         return true; // If we found the end of the line, return.
      default:
         // If this is a "high" UTF-8 character, validate it.
         if (diags && (signed char)(m_yyCursor[-1]) < 0) {
            --m_yyCursor;
            const unsigned char *charStart = m_yyCursor;
            if (validate_utf8_character_and_advance(m_yyCursor, bufferEnd) == ~0U) {
               //               diags->diagnose(Lexer::getSourceLoc(charStart),
               //                               diag::lex_invalid_utf8);
            }

         }
         break;   // Otherwise, eat other characters.
      case 0:
         if (m_yyCursor - 1 != bufferEnd) {
            if (diags && m_yyCursor - 1 != codeCompletionPtr) {
               // If this is a random nul character in the middle of a buffer, skip
               // it as whitespace.
               diagnose_embedded_null(diags, m_yyCursor - 1);
            }
            continue;
         }
         // Otherwise, the last line of the file does not have a newline.
         --m_yyCursor;
         return false;
      }
   }
}

bool skip_to_end_of_slash_star_comment(const unsigned char *&m_yyCursor, const unsigned char *bufferEnd,
                                       const unsigned char *codeCompletionPtr, DiagnosticEngine *diags)
{
   const unsigned char *startPtr = m_yyCursor - 1;
   assert(m_yyCursor[-1] == '/' && m_yyCursor[0] == '*' && "Not a /* comment");
   // Make sure to advance over the * so that we don't incorrectly handle /*/ as
   // the beginning and end of the comment.
   ++m_yyCursor;

   // /**/ comments can be nested, keep track of how deep we've gone.
   unsigned depth = 1;
   bool isMultiline = false;

   while (1) {
      switch (*m_yyCursor++) {
      case '*':
         // Check for a '*/'
         if (*m_yyCursor == '/') {
            ++m_yyCursor;
            if (--depth == 0)
               return isMultiline;
         }
         break;
      case '/':
         // Check for a '/*'
         if (*m_yyCursor == '*') {
            ++m_yyCursor;
            ++depth;
         }
         break;

      case '\n':
      case '\r':
         isMultiline = true;
         break;

      default:
         // If this is a "high" UTF-8 character, validate it.
         if (diags && (signed char)(m_yyCursor[-1]) < 0) {
            --m_yyCursor;
            const unsigned char *charStart = m_yyCursor;
            if (validate_utf8_character_and_advance(m_yyCursor, bufferEnd) == ~0U) {
               //               diags->diagnose(Lexer::getSourceLoc(charStart),
               //                               diag::lex_invalid_utf8);
            }
         }

         break;   // Otherwise, eat other characters.
      case 0:
         if (m_yyCursor - 1 != bufferEnd) {
            if (diags && m_yyCursor - 1 != codeCompletionPtr) {
               // If this is a random nul character in the middle of a buffer, skip
               // it as whitespace.
               diagnose_embedded_null(diags, m_yyCursor - 1);
            }
            continue;
         }
         // Otherwise, we have an unterminated /* comment.
         --m_yyCursor;

         if (diags) {
            // Count how many levels deep we are.
            SmallString<8> terminator("*/");
            while (--depth != 0)
               terminator += "*/";
            const unsigned char *EOL = (m_yyCursor[-1] == '\n') ? (m_yyCursor - 1) : m_yyCursor;
            //            diags
            //                  ->diagnose(Lexer::getSourceLoc(EOL),
            //                             diag::lex_unterminated_block_comment)
            //                  .fixItInsert(Lexer::getSourceLoc(EOL), terminator);
            //            diags->diagnose(Lexer::getSourceLoc(StartPtr), diag::lex_comment_start);
         }
         return isMultiline;
      }
   }
}

bool advance_if(const unsigned char *&ptr, const unsigned char *end,
                bool (*predicate)(uint32_t))
{
   const unsigned char *next = ptr;
   uint32_t c = validate_utf8_character_and_advance(next, end);
   if (c == ~0U) {
      return false;
   }
   if (predicate(c)) {
      ptr = next;
      return true;
   }
   return false;
}

const char *next_newline(const char *str, const char *end, size_t &newlineLen)
{
   for (; str < end; ++str) {
      if (*str == '\r') {
         newlineLen = str + 1 < end && *(str + 1) == '\n' ? 2 : 1;
      } else if (*str == '\n') {
         newlineLen = 1;
         return str;
      }
   }
   newlineLen = 0;
   return nullptr;
}

bool strip_multiline_string_indentation(Lexer &lexer, std::string &str, int indentation, bool usingSpaces,
                                        bool newlineAtStart, bool newlineAtEnd)
{
   const char *cursor = str.data();
   const char *end = cursor + str.size();
   char *copy = str.data();
   int newLineCount = 0;
   size_t newlineLen;
   const char *newLine;
   if (!newlineAtStart) {
      newLine = next_newline(cursor, end, newlineLen);
      if (nullptr == newLine) {
         return true;
      }
      cursor = newLine + newlineLen;
      copy = const_cast<char *>(newLine) + newlineLen;
      ++newLineCount;
   } else {
      newLine = cursor;
   }
   // intentional
   while (cursor <= end && newLine) {
      int skip;
      newLine = next_newline(cursor, end, newlineLen);
      if (nullptr == newLine && newlineAtEnd) {
         newLine = end;
      }
      // Try to skip indentation
      for (skip = 0; skip < indentation; skip++, cursor++) {
         if (cursor == newLine) {
            // Don't require full indentation on whitespace-only lines
            break;
         }
         if (cursor == end || (*cursor != ' ' && *cursor != '\t')) {
            lexer.notifyLexicalException(0, "Invalid body indentation level (expecting an indentation level of at least %d)",
                                         indentation);
            return false;
         }
         if ((!usingSpaces && *cursor == ' ') || (usingSpaces && *cursor == '\t')) {
            lexer.notifyLexicalException("Invalid indentation - tabs and spaces cannot be mixed", 0);
            return false;
         }
      }
      if (cursor == end) {
         break;
      }
      size_t len = newLine ? (newLine - cursor + newlineLen) : (end - cursor);
      std::memmove(copy, cursor, len);
      cursor += len;
      copy += len;
      ++newLineCount;
   }
   str.resize(copy - str.data());
   return true;
}

void strip_underscores(std::string &str, size_t &len)
{
   std::string::iterator src = str.begin();
   std::string::iterator dest = str.begin();
   while (*src != '\0') {
      if (*src != '_') {
         *dest = *src;
         dest++;
      } else {
         --len;
      }
      src++;
   }
   str.resize(len);
}

TokenKindType get_token_kind_by_char(unsigned char c)
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
   case '.':
      token = TokenKindType::T_STR_CONCAT;
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
      token = TokenKindType::T_UNKNOWN_MARK;
      break;
   }
   return token;
}

long convert_single_quote_str_escape_sequences(std::string::iterator iter, std::string::iterator endMark, Lexer &lexer)
{
   std::string::iterator origIter = iter;
   while (true) {
      if (*iter == '\\') {
         break;
      }
      ++iter;
      if (iter == endMark) {
         return iter - origIter;
      }
   }
   std::string::iterator targetIter = iter;
   while (iter != endMark) {
      if (*iter == '\\') {
         ++iter;
         if (*iter == '\\' || *iter == '\'') {
            *targetIter = *iter;
         } else {
            *targetIter++ = '\\';
            *targetIter++ = *iter;
         }
      } else {
         *targetIter++ = *iter;
      }
      ++targetIter;
   }
   return targetIter - origIter;
}

bool convert_double_quote_str_escape_sequences(std::string &filteredStr, char quoteType, std::string::iterator iter,
                                               std::string::iterator endMark, Lexer &lexer)
{
   size_t origLength = endMark - iter;
   if (origLength <= 1) {
      return true;
   }
   /// convert escape sequences
   auto fiter = filteredStr.begin();
   auto fendMark = filteredStr.end();
   /// find first '\'
   while (true) {
      if (*fiter == '\\') {
         break;
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
            //         case 'e':
            //            *targetIter++ = '\e';
            //            break;
         case '"':
         case '`':
            if (*fiter != quoteType) {
               *targetIter++ = '\\';
               *targetIter++ = *fiter;
               break;
            }
            [[fallthrough]];
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
               lexer.notifyLexicalException("Invalid UTF-8 codepoint escape sequence", 0);
               return false;
            }
            errno = 0;
            StringRef codePointStr(filteredStr.data() + (start - filteredStr.begin()), fiter - start);
            codePoint = strtoul(codePointStr.data(), nullptr, 16);
            /// per RFC 3629, UTF-8 can only represent 21 bits
            if (codePoint > 0x10FFFF || errno) {
               lexer.notifyLexicalException("Invalid UTF-8 codepoint escape sequence: Codepoint too large", 0);
               return false;
            }
            /// based on https://en.wikipedia.org/wiki/UTF-8#Sample_code
            if (codePoint < 0x80) {
               *targetIter++ = static_cast<std::string::value_type>(codePoint);
            } else if (codePoint <= 0x7FF) {
               *targetIter++ = static_cast<std::string::value_type>((codePoint >> 6) + 0xC0);
               *targetIter++ = static_cast<std::string::value_type>((codePoint & 0x3F) + 0x80);
            } else if (codePoint <= 0xFFFF) {
               *targetIter++ = static_cast<std::string::value_type>((codePoint >> 12) + 0xE0);
               *targetIter++ = static_cast<std::string::value_type>(((codePoint >> 6) & 0x3F) + 0x80);
               *targetIter++ = static_cast<std::string::value_type>((codePoint & 0x3F) + 0x80);
            } else if (codePoint <= 0x10FFFF) {
               *targetIter++ = static_cast<std::string::value_type>((codePoint >> 18) + 0xF0);
               *targetIter++ = static_cast<std::string::value_type>(((codePoint >> 12) & 0x3F) + 0x80);
               *targetIter++ = static_cast<std::string::value_type>(((codePoint >> 6) & 0x3F) + 0x80);
               *targetIter++ = static_cast<std::string::value_type>((codePoint & 0x3F) + 0x80);
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
                  lexer.notifyLexicalException(0, "Octal escape sequence overflow \\%s is greater than \\377", octalBuf);
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
      ++fiter;
   }
   filteredStr.resize(targetIter - filteredStr.begin());
   return true;
}

namespace {
bool is_valid_identifier_continuation_code_point(uint32_t c)
{
   if (c < 0x80) {
      return clang::isIdentifierBody(static_cast<unsigned char>(c), true);
   }
   // N1518: Recommendations for extended identifier characters for c and c++
   // Proposed Annex X.1: Ranges of characters allowed
   return c == 0x00A8 || c == 0x00AA || c == 0x00AD || c == 0x00AF
         || (c >= 0x00B2 && c <= 0x00B5) || (c >= 0x00B7 && c <= 0x00BA)
         || (c >= 0x00BC && c <= 0x00BE) || (c >= 0x00C0 && c <= 0x00D6)
         || (c >= 0x00D8 && c <= 0x00F6) || (c >= 0x00F8 && c <= 0x00FF)

         || (c >= 0x0100 && c <= 0x167F)
         || (c >= 0x1681 && c <= 0x180D)
         || (c >= 0x180F && c <= 0x1FFF)

         || (c >= 0x200B && c <= 0x200D)
         || (c >= 0x202A && c <= 0x202E)
         || (c >= 0x203F && c <= 0x2040)
         || c == 0x2054
         || (c >= 0x2060 && c <= 0x206F)

         || (c >= 0x2070 && c <= 0x218F)
         || (c >= 0x2460 && c <= 0x24FF)
         || (c >= 0x2776 && c <= 0x2793)
         || (c >= 0x2C00 && c <= 0x2DFF)
         || (c >= 0x2E80 && c <= 0x2FFF)

         || (c >= 0x3004 && c <= 0x3007)
         || (c >= 0x3021 && c <= 0x302F)
         || (c >= 0x3031 && c <= 0x303F)

         || (c >= 0x3040 && c <= 0xD7FF)

         || (c >= 0xF900 && c <= 0xFD3D)
         || (c >= 0xFD40 && c <= 0xFDCF)
         || (c >= 0xFDF0 && c <= 0xFE44)
         || (c >= 0xFE47 && c <= 0xFFF8)

         || (c >= 0x10000 && c <= 0x1FFFD)
         || (c >= 0x20000 && c <= 0x2FFFD)
         || (c >= 0x30000 && c <= 0x3FFFD)
         || (c >= 0x40000 && c <= 0x4FFFD)
         || (c >= 0x50000 && c <= 0x5FFFD)
         || (c >= 0x60000 && c <= 0x6FFFD)
         || (c >= 0x70000 && c <= 0x7FFFD)
         || (c >= 0x80000 && c <= 0x8FFFD)
         || (c >= 0x90000 && c <= 0x9FFFD)
         || (c >= 0xA0000 && c <= 0xAFFFD)
         || (c >= 0xB0000 && c <= 0xBFFFD)
         || (c >= 0xC0000 && c <= 0xCFFFD)
         || (c >= 0xD0000 && c <= 0xDFFFD)
         || (c >= 0xE0000 && c <= 0xEFFFD);
}

bool is_valid_identifier_start_code_point(uint32_t c)
{
   if (!is_valid_identifier_continuation_code_point(c)) {
      return false;
   }

   if (c < 0x80 && (is_digit(static_cast<unsigned char>(c)) || c == '$')) {
      return false;
   }

   // N1518: Recommendations for extended identifier characters for c and c++
   // Proposed Annex X.2: Ranges of characters disallowed initially
   if ((c >= 0x0300 && c <= 0x036F) ||
       (c >= 0x1DC0 && c <= 0x1DFF) ||
       (c >= 0x20D0 && c <= 0x20FF) ||
       (c >= 0xFE20 && c <= 0xFE2F)) {
      return false;
   }
   return true;
}

bool is_operator_start_code_point(uint32_t c)
{
   // ASCII operator chars.
   static const char opChars[] = "/=-+*%<>!&|^~.?";
   if (c < 0x80) {
      return std::memchr(opChars, c, sizeof(opChars) - 1) != nullptr;
   }
   // Unicode math, symbol, arrow, dingbat, and line/box drawing chars.
   return (c >= 0x00A1 && c <= 0x00A7)
         || c == 0x00A9 || c == 0x00AB || c == 0x00AC || c == 0x00AE
         || c == 0x00B0 || c == 0x00B1 || c == 0x00B6 || c == 0x00BB
         || c == 0x00BF || c == 0x00D7 || c == 0x00F7
         || c == 0x2016 || c == 0x2017 || (c >= 0x2020 && c <= 0x2027)
         || (c >= 0x2030 && c <= 0x203E) || (c >= 0x2041 && c <= 0x2053)
         || (c >= 0x2055 && c <= 0x205E) || (c >= 0x2190 && c <= 0x23FF)
         || (c >= 0x2500 && c <= 0x2775) || (c >= 0x2794 && c <= 0x2BFF)
         || (c >= 0x2E00 && c <= 0x2E7F) || (c >= 0x3001 && c <= 0x3003)
         || (c >= 0x3008 && c <= 0x3030);
}

/// is_operator_continuation_code_point - Return true if the specified code point
/// is a valid operator code point.
bool is_operator_continuation_code_point(uint32_t c)
{
   if (is_operator_start_code_point(c)) {
      return true;
   }
   // Unicode combining characters and variation selectors.
   return (c >= 0x0300 && c <= 0x036F)
         || (c >= 0x1DC0 && c <= 0x1DFF)
         || (c >= 0x20D0 && c <= 0x20FF)
         || (c >= 0xFE00 && c <= 0xFE0F)
         || (c >= 0xFE20 && c <= 0xFE2F)
         || (c >= 0xE0100 && c <= 0xE01EF);
}
} // anonymous namespace

bool encode_to_utf8(unsigned c, SmallVectorImpl<char> &result)
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

bool advance_if_valid_start_of_identifier(const unsigned char *&ptr,
                                          const unsigned char *end)
{
   return advance_if(ptr, end, is_valid_identifier_start_code_point);
}

bool advance_if_valid_continuation_of_identifier(const unsigned char *&ptr,
                                                 const unsigned char *end)
{
   return advance_if(ptr, end, is_valid_identifier_continuation_code_point);
}

bool advance_if_valid_start_of_operator(const unsigned char *&ptr,
                                        const unsigned char *end)
{
   return advance_if(ptr, end, is_operator_start_code_point);
}

bool advance_if_valid_continuation_of_operator(const unsigned char *&ptr,
                                               const unsigned char *end)
{
   return advance_if(ptr, end, is_operator_continuation_code_point);
}

} // polar::parser::internal
