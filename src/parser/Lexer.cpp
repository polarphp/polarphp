// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/09.

#include "polarphp/parser/Lexer.h"
#include "polarphp/parser/Parser.h"
#include "polarphp/parser/CommonDefs.h"
#include "polarphp/parser/internal/YYLexerDefs.h"
#include "polarphp/parser/internal/YYLexerExtras.h"
#include "polarphp/parser/Confusables.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/CharInfo.h"
#include "polarphp/syntax/Trivia.h"
#include "polarphp/utils/MathExtras.h"
#include "polarphp/kernel/LangOptions.h"
#include "polarphp/kernel/Exceptions.h"

#include <set>
#include <string>
#include <cstdint>

namespace polar::parser {

using polar::basic::SmallString;

using namespace internal;
using namespace polar::syntax;
using namespace polar::basic;

#define IS_LABEL_START(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || (c) == '_' || (c) >= 0x80)
#define HEREDOC_USING_SPACES 1
#define HEREDOC_USING_TABS 2
#define MAX_LENGTH_OF_INT64 19

Lexer::Lexer(const PrincipalTag &, const LangOptions &langOpts,
             const SourceManager &sourceMgr, unsigned bufferId,
             DiagnosticEngine *diags, CommentRetentionMode commentRetention,
             TriviaRetentionMode triviaRetention)
   : m_langOpts(langOpts),
     m_sourceMgr(sourceMgr),
     m_bufferId(bufferId),
     m_diags(diags),
     m_commentRetention(commentRetention),
     m_triviaRetention(triviaRetention)
{}

Lexer::Lexer(const LangOptions &options, const SourceManager &sourceMgr,
             unsigned bufferId, DiagnosticEngine *diags,
             CommentRetentionMode commentRetain, TriviaRetentionMode triviaRetention)
   : Lexer(PrincipalTag(), options, sourceMgr, bufferId, diags,
           commentRetain, triviaRetention)
{
   unsigned endOffset = sourceMgr.getRangeForBuffer(bufferId).getByteLength();
   initialize(/*offset=*/0, endOffset);
}

Lexer::Lexer(const LangOptions &options, const SourceManager &sourceMgr,
             unsigned bufferId, DiagnosticEngine *diags,
             CommentRetentionMode commentRetain, TriviaRetentionMode triviaRetention,
             unsigned offset, unsigned endOffset)
   : Lexer(PrincipalTag(), options, sourceMgr, bufferId, diags, commentRetain, triviaRetention)
{
   initialize(offset, endOffset);
}

Lexer::Lexer(Lexer &parent, LexerState beginState, LexerState endState)
   : Lexer(PrincipalTag(), parent.m_langOpts, parent.m_sourceMgr,
           parent.m_bufferId, parent.m_diags, parent.m_commentRetention,
           parent.m_triviaRetention)
{
   assert(m_bufferId == m_sourceMgr.findBufferContainingLoc(beginState.m_loc) &&
          "LexerState for the wrong buffer");
   assert(m_bufferId == m_sourceMgr.findBufferContainingLoc(endState.m_loc) &&
          "LexerState for the wrong buffer");

   unsigned offset = m_sourceMgr.getLocOffsetInBuffer(beginState.m_loc, m_bufferId);
   unsigned endOffset = m_sourceMgr.getLocOffsetInBuffer(endState.m_loc, m_bufferId);
   initialize(offset, endOffset);
}

void Lexer::initialize(unsigned offset, unsigned endOffset)
{
   assert(offset <= endOffset);
   // Initialize buffer pointers.
   StringRef contents =
         m_sourceMgr.extractText(m_sourceMgr.getRangeForBuffer(m_bufferId));
   m_bufferStart = reinterpret_cast<const unsigned char *>(contents.data());
   m_bufferEnd = reinterpret_cast<const unsigned char *>(contents.data() + contents.size());
   assert(*m_bufferEnd == 0);
   assert(m_bufferStart + offset <= m_bufferEnd);
   assert(m_bufferStart + endOffset <= m_bufferEnd);
   // Check for Unicode BOM at start of file (Only UTF-8 BOM supported now).
   size_t BOMLength = contents.startsWith("\xEF\xBB\xBF") ? 3 : 0;
   // Keep information about existance of UTF-8 BOM for transparency source code
   // editing with libSyntax.
   m_contentStart = m_bufferStart + BOMLength;
   // Initialize code completion.
   if (m_bufferId == m_sourceMgr.getCodeCompletionBufferID()) {
      const unsigned char *ptr = m_bufferStart + m_sourceMgr.getCodeCompletionOffset();
      if (ptr >= m_bufferStart && ptr <= m_bufferEnd) {
         m_codeCompletionPtr = ptr;
      }
   }
   m_artificialEof = m_bufferStart + endOffset;
   m_yyCursor = m_bufferStart + offset;

   assert(m_nextToken.is(TokenKindType::T_UNKOWN_MARK));
   lexImpl();
   assert((m_nextToken.isAtStartOfLine() || m_yyCursor != m_bufferStart) &&
          "The token should be at the beginning of the line, "
          "or we should be lexing from the middle of the buffer");
}

InFlightDiagnostic Lexer::diagnose(const unsigned char *loc, ast::Diagnostic diag)
{
   if (m_diags) {
      return m_diags->diagnose(getSourceLoc(loc), diag);
   }
   return InFlightDiagnostic();
}

Token Lexer::getTokenAt(SourceLoc loc)
{
   assert(m_bufferId == static_cast<unsigned>(
             m_sourceMgr.findBufferContainingLoc(loc)) &&
          "location from the wrong buffer");
   Lexer lexer(m_langOpts, m_sourceMgr, m_bufferId, m_diags,
               CommentRetentionMode::None,
               TriviaRetentionMode::WithoutTrivia);
   lexer.restoreState(LexerState(loc));
   return lexer.peekNextToken();
}

void Lexer::formToken(syntax::TokenKindType kind, const unsigned char *tokenStart)
{
   assert(m_yyCursor >= m_bufferStart &&
          m_yyCursor <= m_bufferEnd && "Current pointer out of range!");
   // When we are lexing a subrange from the middle of a file buffer, we will
   // run past the end of the range, but will stay within the file.  Check if
   // we are past the imaginary EOF, and synthesize a tok::eof in this case.
   if (kind != TokenKindType::END && tokenStart >= m_artificialEof) {
      kind = TokenKindType::END;
   }
   unsigned commentLength = 0;
   if (m_commentRetention == CommentRetentionMode::AttachToNextToken) {
      // 'CommentLength' here is the length from the *first* comment to the
      // token text (or its backtick if exist).
      auto iter = polar::basic::find_if(m_leadingTrivia, [](const ParsedTriviaPiece &piece) {
         return is_comment_trivia_kind(piece.getKind());
      });
      for (auto end = m_leadingTrivia.end(); iter != end; iter++) {
         if (iter->getKind() == TriviaKind::Backtick) {
            // Since Token::getCommentRange() doesn't take backtick into account,
            // we cannot include length of backtick.
            break;
         }
         commentLength += iter->getLength();
      }
   }

   StringRef tokenText { reinterpret_cast<const char *>(tokenStart), static_cast<size_t>(m_yyCursor - tokenStart) };
   if (m_triviaRetention == TriviaRetentionMode::WithTrivia) {
      lexTrivia(m_trailingTrivia, /* IsForTrailingTrivia */ true);
   }
   m_nextToken.setToken(kind, tokenText, commentLength);
}

void Lexer::formVariableToken(const unsigned char *tokenStart)
{
   formToken(TokenKindType::T_VARIABLE, tokenStart);
   m_nextToken.setValue(StringRef(reinterpret_cast<const char *>(tokenStart + 1), m_yyLength - 1));
}

void Lexer::formIdentifierToken(const unsigned char *tokenStart)
{
   formToken(TokenKindType::T_IDENTIFIER_STRING, tokenStart);
   m_nextToken.setValue(StringRef(reinterpret_cast<const char *>(tokenStart), m_yyLength));
}

void Lexer::lexTrivia(ParsedTrivia &trivia, bool isForTrailingTrivia)
{
restart:
   const unsigned char *triviaStart = m_yyCursor;
   switch (*m_yyCursor++) {
   case '\n':
      if (isForTrailingTrivia) {
         break;
      }
      m_nextToken.setAtStartOfLine(true);
      trivia.appendOrSquash(TriviaKind::Newline, 1);
      goto restart;
   case '\r':
      if (isForTrailingTrivia) {
         break;
      }
      m_nextToken.setAtStartOfLine(true);
      if (m_yyCursor[0] == '\n') {
         trivia.appendOrSquash(TriviaKind::CarriageReturnLineFeed, 2);
         ++m_yyCursor;
      } else {
         trivia.appendOrSquash(TriviaKind::CarriageReturn, 1);
      }
      goto restart;
   case ' ':
      trivia.appendOrSquash(TriviaKind::Space, 1);
      goto restart;
   case '\t':
      trivia.appendOrSquash(TriviaKind::Tab, 1);
      goto restart;
   case '\v':
      trivia.appendOrSquash(TriviaKind::VerticalTab, 1);
      goto restart;
   case '\f':
      trivia.appendOrSquash(TriviaKind::Formfeed, 1);
      goto restart;
   case '/':
      if (isForTrailingTrivia || isKeepingComments()) {
         // Don't lex comments as trailing trivias (for now).
         // Don't try to lex comments here if we are lexing comments as Tokens.
         break;
      } else if (*m_yyCursor == '/') {
         bool isDocComment = m_yyCursor[1] == '/';
         skipSlashSlashComment(/*EatNewline=*/false);
         size_t length = m_yyCursor - triviaStart;
         trivia.push_back(isDocComment ? TriviaKind::DocLineComment
                                       : TriviaKind::LineComment, length);
         goto restart;
      } else if (*m_yyCursor == '*') {
         // '/* ... */' comment.
         bool isDocComment = m_yyCursor[1] == '*';
         skipSlashStarComment();
         size_t length = m_yyCursor - triviaStart;
         trivia.push_back(isDocComment ? TriviaKind::DocBlockComment
                                       : TriviaKind::BlockComment, length);
         goto restart;
      }
      break;
   case '#':
      if (triviaStart == m_contentStart && *m_yyCursor == '!') {
         // Hashbang '#!/path/to/polarphp'.
         --m_yyCursor;
         skipHashbang(/*EatNewline=*/false);
         size_t length = m_yyCursor - triviaStart;
         trivia.push_back(TriviaKind::GarbageText, length);
         goto restart;
      }
      break;
   case 0:
      switch (getNullCharacterKind(m_yyCursor - 1)) {
      case NullCharacterKind::Embedded:
      {
         diagnose_embedded_null(m_diags, m_yyCursor - 1);
         size_t length = m_yyCursor - triviaStart;
         trivia.push_back(TriviaKind::GarbageText, length);
         goto restart;
      }
      case NullCharacterKind::CodeCompletion:
      case NullCharacterKind::BufferEnd:
         break;
      }
      break;
      // Start character of tokens.
   case '@': case '{': case '[': case '(': case '}': case ']': case ')':
   case ',': case ';': case ':': case '\\': case '$':
   case '0': case '1': case '2': case '3': case '4':
   case '5': case '6': case '7': case '8': case '9':
   case '"': case '\'': case '`':
      // Start of identifiers.
   case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
   case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
   case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
   case 'V': case 'W': case 'X': case 'Y': case 'Z':
   case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
   case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
   case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
   case 'v': case 'w': case 'x': case 'y': case 'z':
   case '_':
      // Start of operators.
   case '%': case '!': case '?': case '=':
   case '-': case '+': case '*':
   case '&': case '|': case '^': case '~': case '.':
   case '<': case '>':
      break;
   default:
      const unsigned char *temp = m_yyCursor - 1;
      if (advance_if_valid_start_of_identifier(temp, m_bufferEnd)) {
         break;
      }
      if (advance_if_valid_start_of_operator(temp, m_bufferEnd)) {
         break;
      }
      bool shouldTokenize = lexUnknown(/*EmitDiagnosticsIfToken=*/false);
      if (shouldTokenize) {
         m_yyCursor = temp;
         return;
      }
      size_t length = m_yyCursor - triviaStart;
      trivia.push_back(TriviaKind::GarbageText, length);
      goto restart;
   }
   // Reset the cursor.
   --m_yyCursor;
}

bool Lexer::lexUnknown(bool emitDiagnosticsIfToken)
{
   const unsigned char *temp = m_yyCursor - 1;
   if (advance_if_valid_continuation_of_identifier(temp, m_bufferEnd)) {
      // If this is a valid identifier continuation, but not a valid identifier
      // start, attempt to recover by eating more continuation characters.
      if (emitDiagnosticsIfToken) {
         //         diagnose(m_yyCursor - 1, diag::lex_invalid_identifier_start_character);
      }
      while (advance_if_valid_continuation_of_identifier(temp, m_bufferEnd));
      m_yyCursor = temp;
      return true;
   }
   // This character isn't allowed in polarphp source.
   uint32_t codepoint = validate_utf8_character_and_advance(temp, m_bufferEnd);
   if (codepoint == ~0U) {
      //      diagnose(m_yyCursor - 1, diag::lex_invalid_utf8)
      //            .fixItReplaceChars(getSourceLoc(m_yyCursor - 1), getSourceLoc(temp), " ");
      m_yyCursor = temp;
      return false;// Skip presumed whitespace.
   } else if (codepoint == 0x000000A0) {
      // Non-breaking whitespace (U+00A0)
      while (reinterpret_cast<const char *>(temp)[0] == '\xC2' &&
             reinterpret_cast<const char *>(temp)[1] == '\xA0') {
         temp += 2;
      }
      SmallString<8> spaces;
      spaces.assign((temp - m_yyCursor + 1) / 2, ' ');
      //      diagnose(m_yyCursor - 1, diag::lex_nonbreaking_space)
      //            .fixItReplaceChars(getSourceLoc(m_yyCursor - 1), getSourceLoc(temp),
      //                               spaces);
      m_yyCursor = temp;
      return false;// Skip presumed whitespace.
   } else if (codepoint == 0x0000201D) {
      // If this is an end curly quote, just diagnose it with a fixit hint.
      if (emitDiagnosticsIfToken) {
         //         diagnose(m_yyCursor - 1, diag::lex_invalid_curly_quote)
         //               .fixItReplaceChars(getSourceLoc(m_yyCursor - 1), getSourceLoc(temp), "\"");
      }
      m_yyCursor = temp;
      return true;
   }
   //   diagnose(m_yyCursor - 1, diag::lex_invalid_character)
   //         .fixItReplaceChars(getSourceLoc(m_yyCursor - 1), getSourceLoc(temp), " ");

   char expectedCodepoint;
   if ((expectedCodepoint =
        try_convert_confusable_character_to_ascii(codepoint))) {

      SmallString<4> confusedChar;
      encode_to_utf8(codepoint, confusedChar);
      SmallString<1> expectedChar;
      expectedChar += expectedCodepoint;
      //      diagnose(m_yyCursor - 1, diag::lex_confusable_character, confusedChar,
      //               expectedChar)
      //            .fixItReplaceChars(getSourceLoc(m_yyCursor - 1), getSourceLoc(temp),
      //                               expectedChar);
   }

   m_yyCursor = temp;
   return false; // Skip presumed whitespace.
}

Lexer::NullCharacterKind Lexer::getNullCharacterKind(const unsigned char *ptr) const
{
   assert(ptr != nullptr && *ptr == 0);
   if (ptr == m_codeCompletionPtr) {
      return NullCharacterKind::CodeCompletion;
   }
   if (ptr == m_bufferEnd) {
      return NullCharacterKind::BufferEnd;
   }
   return NullCharacterKind::Embedded;
}

void Lexer::notifyLexicalException(StringRef msg, int code)
{
   if (m_lexicalExceptionHandler != nullptr) {
      m_lexicalExceptionHandler(msg, code);
   }
}

LexerState Lexer::getStateForBeginningOfTokenLoc(SourceLoc sourceLoc) const
{
   const unsigned char *ptr = getBufferPtrForSourceLoc(sourceLoc);
   // Skip whitespace backwards until we hit a newline.  This is needed to
   // correctly lex the token if it is at the beginning of the line.
   while (ptr >= m_contentStart + 1) {
      char c = ptr[-1];
      if (c == ' ' || c == '\t') {
         --ptr;
         continue;
      }
      if (c == 0) {
         // A NUL character can be either whitespace we diagnose or a code
         // completion token.
         if (ptr - 1 == m_codeCompletionPtr) {
            break;
         }
         --ptr;
         continue;
      }
      if (c == '\n' || c == '\r') {
         --ptr;
         break;
      }
      break;
   }
   return LexerState(SourceLoc(polar::utils::SMLocation::getFromPointer(reinterpret_cast<const char *>(ptr))));
}

void Lexer::skipToEndOfLine(bool eatNewline)
{
   bool isEOL = advance_to_end_of_line(m_yyCursor, m_bufferEnd, m_codeCompletionPtr, m_diags);
   if (eatNewline && isEOL) {
      ++m_yyCursor;
      m_nextToken.setAtStartOfLine(true);
   }
}

void Lexer::skipSlashSlashComment(bool eatNewline)
{
   assert(m_yyCursor[-1] == '/' && m_yyCursor[0] == '/' && "Not a // comment");
   skipToEndOfLine(eatNewline);
}

/// skipSlashStarComment - /**/ comments are skipped (treated as whitespace).
/// Note that (unlike in C) block comments can be nested.
void Lexer::skipSlashStarComment()
{
   bool isMultiline =
         skip_to_end_of_slash_star_comment(m_yyCursor, m_bufferEnd, m_codeCompletionPtr, m_diags);
   if (isMultiline) {
      m_nextToken.setAtStartOfLine(true);
   }
}

void Lexer::skipHashbang(bool eatNewline)
{
   assert(m_yyCursor == m_contentStart && m_yyCursor[0] == '#' && m_yyCursor[1] == '!' &&
         "Not a hashbang");
   skipToEndOfLine(eatNewline);
}

void Lexer::lexBinaryNumber()
{
   /// The +/- 2 skips "0b"
   char *yytext = reinterpret_cast<char *>(const_cast<unsigned char *>(m_yyText));
   char *bnumStr = yytext + 2;
   char *bnumStrEnd = nullptr;
   size_t numLength = m_yyLength - 2;
   /// Skip any leading 0s
   while (*bnumStr == '0') {
      ++bnumStr;
      --numLength;
   }
   if (numLength < sizeof(std::int64_t) * CHAR_BIT) {
      std::int64_t numValue = 0;
      if (numLength > 0) {
         errno = 0;
         numValue = std::strtoll(bnumStr, &bnumStrEnd, 2);
         assert(!errno && bnumStrEnd == yytext + m_yyLength);
      }
      formToken(TokenKindType::T_LNUMBER, m_yyText);
      m_nextToken.setValue(numValue);
   } else {
      double numValue = 0;
      const char *tempPtr = reinterpret_cast<const char *>(bnumStrEnd);
      numValue = polar::utils::bstr_to_double(bnumStr, &tempPtr);
      /// errno isn't checked since we allow HUGE_VAL/INF overflow
      assert(tempPtr == yytext + m_yyLength);
      formToken(TokenKindType::T_DNUMBER, m_yyText);
      m_nextToken.setValue(numValue);
   }
}

void Lexer::lexHexNumber()
{
   /// Skip "0x"
   char *yytext = reinterpret_cast<char *>(const_cast<unsigned char *>(m_yyText));
   char *hexStr = yytext + 2;
   int length = m_yyLength - 2;
   char *end = nullptr;
   std::int64_t lvalue = 0;
   while (*hexStr == '0') {
      ++hexStr;
      --length;
   }
   constexpr int maxWidth = sizeof(std::int64_t) * 2;
   if (length < maxWidth || (length == maxWidth && *hexStr <= '7')) {
      if (length > 0) {
         errno = 0;
         lvalue = std::strtoll(yytext, &end, 16);
         assert(!errno && end == hexStr + length);
      }
      formToken(TokenKindType::T_LNUMBER, m_yyText);
      m_nextToken.setValue(lvalue);
   } else {
      const char *tempPtr = reinterpret_cast<const char *>(end);
      double dvalue = polar::utils::hexstr_to_double(yytext, &tempPtr);
      /// errno isn't checked since we allow HUGE_VAL/INF overflow
      assert(end == hexStr + length);
      formToken(TokenKindType::T_DNUMBER, m_yyText);
      m_nextToken.setValue(dvalue);
   }
}

void Lexer::lexLongNumber()
{
   char *end = nullptr;
   char *yytext = reinterpret_cast<char *>(const_cast<unsigned char *>(m_yyText));
   std::int64_t lvalue = 0;
   if (m_yyLength < MAX_LENGTH_OF_INT64) {
      /// Won't overflow
      errno = 0;
      /// base must be passed explicitly for correct parse error on Windows
      lvalue = std::strtoll(yytext, &end, yytext[0] == '0' ? 8 : 10);
      /// This isn't an assert, we need to ensure 019 isn't valid octal
      /// Because the lexing itself doesn't do that for us
      if (end != yytext + m_yyLength) {
         notifyLexicalException("Invalid numeric literal", 0);
         if (isInParseMode()) {
            formToken(TokenKindType::T_ERROR, m_yyText);
            return;
         }
         /// here we does not set semantic value
         formToken(TokenKindType::T_LNUMBER, m_yyText);
         return;
      }
   } else {
      errno = 0;
      lvalue = std::strtoll(yytext, &end, yytext[0] == '0' ? 8 : 10);
      if (errno == ERANGE) {
         double dvalue = 0.0;
         if (yytext[0] == '0') {
            /// octal overflow
            const char *tempPtr = reinterpret_cast<const char *>(end);
            dvalue = polar::utils::octstr_to_double(yytext, &tempPtr);
         } else {
            dvalue = std::strtod(yytext, &end);
         }
         /// handle double literal format error
         /// Also not an assert for the same reason
         if (end != yytext + m_yyLength) {
            notifyLexicalException("Invalid numeric literal", 0);
            if (isInParseMode()) {
               formToken(TokenKindType::T_ERROR, m_yyText);
               return;
            }
         }
         formToken(TokenKindType::T_DNUMBER, m_yyText);
         m_nextToken.setValue(dvalue);
         return;
      }
      /// handle integer literal format error
      if (end != yytext + m_yyLength) {
         notifyLexicalException("Invalid numeric literal", 0);
         if (isInParseMode()) {
            formToken(TokenKindType::T_ERROR, m_yyText);
            return;
         }
         formToken(TokenKindType::T_LNUMBER, m_yyText);
         return;
      }
   }
   assert(!errno);
   formToken(TokenKindType::T_LNUMBER, m_yyText);
   m_nextToken.setValue(lvalue);
}

void Lexer::lexDoubleNumber()
{
   char *end = nullptr;
   char *yytext = reinterpret_cast<char *>(const_cast<unsigned char *>(m_yyText));
   double dvalue = std::strtod(yytext, &end);
   /// errno isn't checked since we allow HUGE_VAL/INF overflow
   assert(end == yytext + m_yyLength);
   formToken(TokenKindType::T_DNUMBER, m_yyText);
   m_nextToken.setValue(dvalue);
}

void Lexer::lexSingleQuoteString()
{
   const unsigned char *yytext = m_yyText;
   const unsigned char *&yycursor = m_yyCursor;
   const unsigned char *yylimit = m_artificialEof;
   int bprefix = yytext[0] != '\'' ? 1 : 0;

   /// find full single quote string
   while (true) {
      if (yycursor < yylimit) {
         if (*yycursor == '\'') {
            ++yycursor;
            m_yyLength = yycursor - yytext;
            setLexingBinaryStrFlag(false);
            break;
         } else if (*yycursor++ == '\\' && yycursor < yylimit) {
            ++yycursor;
         }
      } else {
         m_yyLength = yylimit - yytext;
         /// Unclosed single quotes; treat similar to double quotes, but without a separate token
         /// for ' (unrecognized by parser), instead of old flex fallback to "Unexpected character..."
         /// rule, which continued in ST_IN_SCRIPTING LexerState after the quote
         formToken(TokenKindType::T_ENCAPSED_AND_WHITESPACE, m_yyText);
         return;
      }
   }
   std::string strValue;
   if (m_yyLength - bprefix - 2 <= 1) {
      if (m_yyLength - bprefix - 2 == 1) {
         unsigned char c = *(yytext + bprefix + 1);
         if (c == '\n' || c == '\r') {
            incLineNumber();
         }
         /// TODO
         /// ZVAL_INTERNED_STR(zendlval, ZSTR_CHAR(c));
         strValue.push_back(c);
      }
   } else {
      strValue.append(reinterpret_cast<const char *>(m_yyText + bprefix + 1), m_yyLength - bprefix - 2);
      size_t filteredLength = convert_single_quote_str_escape_sequences(strValue.data(), strValue.data() + strValue.length(), *this);
      strValue.reserve(filteredLength);
   }
   formToken(TokenKindType::T_CONSTANT_ENCAPSED_STRING, m_yyText);
   m_nextToken.setValue(strValue);
   return;
}

void Lexer::lexDoubleQuoteString()
{
   const unsigned char *&yycursor = m_yyCursor;
   const unsigned char *yytext = m_yyText;
   const unsigned char *yylimit = m_artificialEof;
   if (yycursor >= yylimit) {
      /// TODO
      /// need guard here ?
      yycursor = yylimit;
      formToken(TokenKindType::T_ERROR, yytext);
      return;
   }
   if (yytext[0] == '\\' && yycursor < yylimit) {
      ++yycursor;
   }
   while (yycursor < yylimit) {
      switch (*yycursor++) {
      case '"':
         break;
      case '$':
         if (IS_LABEL_START(*yycursor) || *yycursor == '{') {
            break;
         }
         continue;
      case '{':
         if (*yycursor == '$') {
            break;
         }
         continue;
      case '\\':
         if (yycursor < yylimit) {
            ++yycursor;
         }
         [[fallthrough]];
      default:
         continue;
      }
      --yycursor;
      break;
   }
   m_yyLength = yycursor - yytext;
   std::string filteredStr;
   if (convert_double_quote_str_escape_sequences(filteredStr, '"', yytext, yycursor, *this) ||
       !isInParseMode()) {
      formToken(TokenKindType::T_CONSTANT_ENCAPSED_STRING, yytext);
      m_nextToken.setValue(filteredStr);
   } else {
      formToken(TokenKindType::T_ERROR, yytext);
   }
   return;
}

void Lexer::lexBackquote()
{
   const unsigned char *yytext = m_yyText;
   const unsigned char *&yycursor = m_yyCursor;
   const unsigned char *yylimit = m_artificialEof;

   if (yycursor >= yylimit) {
      /// TODO
      /// really need this guard
      yycursor = yylimit;
      formToken(TokenKindType::END, yytext);
      return;
   }
   if (yytext[0] == '\\' && yycursor < yylimit) {
      ++yycursor;
   }
   while (yycursor < yylimit) {
      switch (*yycursor++) {
      case '`':
         break;
      case '$':
         if (IS_LABEL_START(*yycursor) || *yycursor == '{') {
            break;
         }
         continue;
      case '{':
         if (*yycursor == '$') {
            break;
         }
         continue;
      case '\\':
         if (yycursor < yylimit) {
            ++yycursor;
         }
         [[fallthrough]];
      default:
         continue;
      }
      --yycursor;
      break;
   }
   std::string filteredStr;
   m_yyLength = yycursor - yytext;
   if (convert_double_quote_str_escape_sequences(filteredStr, '`', yytext, yycursor, *this) ||
       !isInParseMode()) {
      formToken(TokenKindType::T_CONSTANT_ENCAPSED_STRING, yytext);
      m_nextToken.setValue(filteredStr);
   } else {
      formToken(TokenKindType::T_ERROR, yytext);
   }
}

void Lexer::lexHeredocHeader()
{
   const unsigned char *iter = nullptr;
   const unsigned char *savedCursor = nullptr;
   const unsigned char *yytext = m_yyText;
   const unsigned char *&yycursor = m_yyCursor;
   const unsigned char *yylimit = m_artificialEof;
   LexerFlags &flags = m_flags;
   int bprefix = 0;
   if (yytext[0] != '<') {
      setLexingBinaryStrFlag(true);
      bprefix = 1;
   }
   int spacing = 0;
   int indentation = 0;
   std::string heredocLabel;
   bool isHeredoc = true;
   incLineNumber();
   int hereDocLabelLength = m_yyLength - bprefix - 3 - 1 - (yytext[m_yyLength - 2] == '\r' ? 1 : 0);
   iter = yytext + bprefix + 3;
   /// trim leader spaces and tabs
   if ((*iter == ' ') || (*iter == '\t')) {
      ++iter;
      --hereDocLabelLength;
   }
   if (*iter == '\'') {
      ++iter;
      hereDocLabelLength -= 2;
      isHeredoc = false;
      m_yyCondition = COND_NAME(ST_NOWDOC);
   } else {
      if (*iter == '"') {
         ++iter;
         hereDocLabelLength -= 2;
      }
      m_yyCondition = COND_NAME(ST_HEREDOC);
   }
   heredocLabel.append(reinterpret_cast<const char *>(iter), hereDocLabelLength);
   std::shared_ptr<HereDocLabel> label = std::make_shared<HereDocLabel>();
   label->label = heredocLabel;
   label->indentation = 0;
   savedCursor = yycursor;
   m_heredocLabelStack.push(label);

   /// calculate indentation and space char type
   while (yycursor < yylimit && (*yycursor == ' ' || *yycursor == '\t')) {
      if (*yycursor == '\t') {
         spacing |= HEREDOC_USING_TABS;
      } else {
         spacing |= HEREDOC_USING_SPACES;
      }
      ++yycursor;
      ++indentation;
   }
   ///
   /// just empty heredoc
   if (yycursor == yylimit) {
      yycursor = savedCursor;
      formToken(TokenKindType::T_START_HEREDOC, yytext);
      return;
   }
   /// Check for ending label on the next line
   /// optimzed for empty heredoc
   if (hereDocLabelLength < yylimit - yycursor &&
       StringRef(reinterpret_cast<const char *>(yycursor), hereDocLabelLength) == heredocLabel) {
      if (!IS_LABEL_START(yycursor[hereDocLabelLength])) {
         ///
         /// detect heredoc end mark sequence
         if (spacing == (HEREDOC_USING_SPACES | HEREDOC_USING_TABS)) {
            notifyLexicalException("Invalid indentation - tabs and spaces cannot be mixed", 0);
         }
         yycursor = savedCursor;
         label->indentation = indentation;
         m_yyCondition = COND_NAME(ST_END_HEREDOC);
         formToken(TokenKindType::T_START_HEREDOC, yytext);
         return;
      }
   }

   yycursor = savedCursor;
   if (isHeredoc && !flags.isHeredocScanAhead()) {

   }
}

void Lexer::lexHeredocBody()
{

}

bool Lexer::isIdentifier(StringRef string)
{
   if (string.empty()) {
      return false;
   }
   const unsigned char *p = reinterpret_cast<const unsigned char *>(string.data());
   const unsigned char *end = reinterpret_cast<const unsigned char *>(string.end());
   if (!advance_if_valid_start_of_identifier(p, end)) {
      return false;
   }
   while (p < end && advance_if_valid_continuation_of_identifier(p, end));
   return p == end;
}

/// Determines if the given string is a valid operator identifier,
/// without escaping characters.
bool Lexer::isOperator(StringRef string)
{
   if (string.empty()) {
      return false;
   }
   const unsigned char *p = reinterpret_cast<const unsigned char *>(string.data());
   const unsigned char *end = reinterpret_cast<const unsigned char *>(string.end());
   if (!advance_if_valid_start_of_operator(p, end)) {
      return false;
   }
   while (p < end && advance_if_valid_continuation_of_operator(p, end));
   return p == end;
}

//===----------------------------------------------------------------------===//
// Main Lexer Loop
//===----------------------------------------------------------------------===//
void Lexer::lexImpl()
{
   assert(m_yyCursor >= m_bufferStart &&
          m_yyCursor <= m_bufferEnd && "Current pointer out of range!");
   m_leadingTrivia.clear();
   m_trailingTrivia.clear();
   if (m_yyCursor == m_bufferStart) {
      if (m_bufferStart < m_contentStart) {
         size_t BOMLen = m_contentStart - m_bufferStart;
         assert(BOMLen == 3 && "UTF-8 BOM is 3 bytes");
         // Add UTF-8 BOM to leadingTrivia.
         m_leadingTrivia.push_back(TriviaKind::GarbageText, BOMLen);
         m_yyCursor += BOMLen;
      }
      m_nextToken.setAtStartOfLine(true);
   } else {
      m_nextToken.setAtStartOfLine(false);
   }
   m_nextToken.resetValueType();

   /// here we want keep comment for next token
   m_yyText = m_yyCursor;
   lexTrivia(m_leadingTrivia, /* IsForTrailingTrivia */ false);

   // invoke yylexer
   if (m_flags.isIncrementLineNumber()) {
      incLineNumber();
   }
   internal::yy_token_lex(*this);
}

namespace {

// Find the start of the given line.
const unsigned char *find_start_of_line(const unsigned char *bufStart, const unsigned char *current)
{
   while (current != bufStart) {
      if (current[0] == '\n' || current[0] == '\r') {
         ++current;
         break;
      }
      --current;
   }
   return current;
}

SourceLoc get_loc_for_start_of_token_in_buffer(
      SourceManager &sourceMgr,
      unsigned bufferId,
      unsigned offset,
      unsigned bufferStart,
      unsigned bufferEnd)
{
   // Use fake language options; language options only affect validity
   // and the exact token produced.
   LangOptions fakeLangOptions;
   Lexer lexer(fakeLangOptions, sourceMgr, bufferId, nullptr, CommentRetentionMode::None,
               TriviaRetentionMode::WithoutTrivia, bufferStart, bufferEnd);

   // Lex tokens until we find the token that contains the source location.
   Token token;
   do {
      lexer.lex(token);
      unsigned tokenOffsets = sourceMgr.getLocOffsetInBuffer(token.getLoc(), bufferId);
      if (tokenOffsets > offset) {
         // We ended up skipping over the source location entirely, which means
         // that it points into whitespace. We are done here.
         break;
      }
      if (offset < tokenOffsets + token.getLength()) {
         // Current token encompasses our source location.
         if (token.is(TokenKindType::T_IDENTIFIER_STRING)) {
            //            SmallVector<Lexer::StringSegment, 4> Segments;
            //            Lexer::getStringLiteralSegments(token, Segments, /*diags=*/nullptr);
            //            for (auto &Seg : Segments) {
            //               unsigned SegOffs = sourceMgr.getLocOffsetInBuffer(Seg.loc, bufferId);
            //               unsigned SegEnd = SegOffs+Seg.Length;
            //               if (SegOffs > offset)
            //                  break;

            //               // If the offset is inside an interpolated expr segment, re-lex.
            //               if (Seg.Kind == Lexer::StringSegment::Expr && offset < SegEnd)
            //                  return get_loc_for_start_of_token_in_buffer(sourceMgr, bufferId, offset,
            //                                                    /*bufferStart=*/SegOffs,
            //                                                    /*bufferEnd=*/SegEnd);
            //            }
         }
         return token.getLoc();
      }
   } while (token.isNot(TokenKindType::END));
   // We've passed our source location; just return the original source location.
   return sourceMgr.getLocForOffset(bufferId, offset);
}

} // anonymous namespace

Lexer &Lexer::saveYYState()
{
   LexerState state;
   state.setYYLength(m_yyLength);
   state.setBufferStart(m_bufferStart);
   state.setBufferEnd(m_bufferEnd);
   state.setContentStart(m_contentStart);
   state.setYYText(m_yyText);
   state.setYYCursor(m_yyCursor);
   state.setYYMarker(m_yyMarker);
   state.setYYLimit(m_artificialEof);

   state.setCondition(m_yyCondition);
   state.setLineNumber(m_lineNumber);
   state.setLexerFlags(m_flags);
   state.setLexicalEventHandler(m_eventHandler);
   state.setLexicalExceptionHandler(m_lexicalExceptionHandler);

   std::stack<std::shared_ptr<HereDocLabel>> heredocLabelStack;
   m_heredocLabelStack.swap(heredocLabelStack);
   state.setHeredocLabelStack(std::move(heredocLabelStack));

   std::stack<YYLexerCondType> condState;
   m_yyConditionStack.swap(condState);
   state.setConditionStack(std::move(condState));

   m_yyStateStack.push(std::move(state));
   return *this;
}

Lexer &Lexer::restoreYYState()
{
   LexerState &state = m_yyStateStack.top();
   m_yyLength = state.getYYLength();
   m_bufferStart = state.getBufferStart();
   m_bufferEnd = state.getBufferEnd();
   m_contentStart = state.getContentStart();
   m_yyText = state.getYYText();
   m_yyCursor = state.getYYCursor();
   m_yyMarker = state.getYYMarker();
   m_artificialEof = state.getYYLimit();

   m_yyCondition = state.getCondition();
   m_lineNumber = state.getLineNumber();
   m_flags = state.getLexerFlags();
   m_eventHandler = state.getLexicalEventHandler();
   m_lexicalExceptionHandler = state.getLexicalExceptionHandler();

   std::stack<std::shared_ptr<HereDocLabel>> &heredocLabelStack = state.getHeredocLabelStack();
   m_heredocLabelStack.swap(heredocLabelStack);
   std::stack<YYLexerCondType> &condState = state.getConditionStack();
   m_yyConditionStack.swap(condState);

   m_yyStateStack.pop();

   return *this;
}

Token Lexer::getTokenAtLocation(const SourceManager &sourceMgr, SourceLoc loc)
{
   // Don't try to do anything with an invalid location.
   if (!loc.isValid()) {
      return Token();
   }

   // Figure out which buffer contains this location.
   int bufferId = sourceMgr.findBufferContainingLoc(loc);
   if (bufferId < 0) {
      return Token();
   }

   // Use fake language options; language options only affect validity
   // and the exact token produced.
   LangOptions fakeLangOpts;

   // Here we return comments as tokens because either the caller skipped
   // comments and normally we won't be at the beginning of a comment token
   // (making this option irrelevant), or the caller lexed comments and
   // we need to lex just the comment token.
   Lexer lexer(fakeLangOpts, sourceMgr, bufferId, nullptr, CommentRetentionMode::ReturnAsTokens);
   lexer.restoreState(LexerState(loc));
   return lexer.peekNextToken();
}

SourceLoc Lexer::getLocForEndOfToken(const SourceManager &sourceMgr, SourceLoc loc)
{
   return loc.getAdvancedLocOrInvalid(getTokenAtLocation(sourceMgr, loc).getLength());
}

SourceLoc Lexer::getLocForStartOfToken(SourceManager &sourceMgr, SourceLoc loc)
{
   if (!loc.isValid())
      return SourceLoc();
   unsigned BufferId = sourceMgr.findBufferContainingLoc(loc);
   return getLocForStartOfToken(sourceMgr, BufferId,
                                sourceMgr.getLocOffsetInBuffer(loc, BufferId));
}

SourceLoc Lexer::getLocForStartOfToken(SourceManager &sourceMgr, unsigned bufferId,
                                       unsigned offset)
{
   CharSourceRange entireRange = sourceMgr.getRangeForBuffer(bufferId);
   StringRef buffer = sourceMgr.extractText(entireRange);

   const unsigned char *bufStart = reinterpret_cast<const unsigned char *>(buffer.data());
   if (offset > buffer.size()) {
      return SourceLoc();
   }
   const unsigned char *strData = bufStart + offset;
   // If it points to whitespace return the SourceLoc for it.
   if (strData[0] == '\n' || strData[0] == '\r' ||
       strData[0] == ' ' || strData[0] == '\t') {
      return sourceMgr.getLocForOffset(bufferId, offset);
   }
   // Back up from the current location until we hit the beginning of a line
   // (or the buffer). We'll relex from that point.
   const unsigned char *lexStart = find_start_of_line(bufStart, strData);
   return get_loc_for_start_of_token_in_buffer(sourceMgr, bufferId, offset,
                                               /*bufferStart=*/lexStart - bufStart,
                                               /*bufferEnd=*/buffer.size());
}

SourceLoc Lexer::getLocForStartOfLine(SourceManager &sourceMgr, SourceLoc loc)
{
   // Don't try to do anything with an invalid location.
   if (loc.isInvalid()) {
      return loc;
   }
   // Figure out which buffer contains this location.
   int bufferId = sourceMgr.findBufferContainingLoc(loc);
   if (bufferId < 0) {
      return SourceLoc();
   }
   CharSourceRange entireRange = sourceMgr.getRangeForBuffer(bufferId);
   StringRef buffer = sourceMgr.extractText(entireRange);
   const unsigned char *bufStart = reinterpret_cast<const unsigned char *>(buffer.data());
   unsigned offset = sourceMgr.getLocOffsetInBuffer(loc, bufferId);
   const unsigned char *startOfLine = find_start_of_line(bufStart, bufStart + offset);
   return getSourceLoc(startOfLine);
}

SourceLoc Lexer::getLocForEndOfLine(SourceManager &sourceMgr, SourceLoc loc)
{
   // Don't try to do anything with an invalid location.
   if (loc.isInvalid()) {
      return loc;
   }

   // Figure out which buffer contains this location.
   int bufferId = sourceMgr.findBufferContainingLoc(loc);
   if (bufferId < 0) {
      return SourceLoc();
   }
   // Use fake language options; language options only affect validity
   // and the exact token produced.
   LangOptions fakeLangOpts;

   // Here we return comments as tokens because either the caller skipped
   // comments and normally we won't be at the beginning of a comment token
   // (making this option irrelevant), or the caller lexed comments and
   // we need to lex just the comment token.
   Lexer lexer(fakeLangOpts, sourceMgr, bufferId, nullptr, CommentRetentionMode::ReturnAsTokens);
   lexer.restoreState(LexerState(loc));
   lexer.skipToEndOfLine(/*EatNewline=*/true);
   return getSourceLoc(lexer.m_yyCursor);
}

StringRef Lexer::getIndentationForLine(SourceManager &sourceMgr, SourceLoc loc,
                                       StringRef *extraIndentation)
{
   // FIXME: do something more intelligent here.
   //
   // Four spaces is the typical indentation in Swift code, so for now just use
   // that directly here, but if someone was to do something better, updating
   // here will update everyone.

   if (extraIndentation) {
      *extraIndentation = "    ";
   }

   // Don't try to do anything with an invalid location.
   if (loc.isInvalid()) {
      return "";
   }
   // Figure out which buffer contains this location.
   int bufferId = sourceMgr.findBufferContainingLoc(loc);
   if (bufferId < 0) {
      return "";
   }

   CharSourceRange entireRange = sourceMgr.getRangeForBuffer(bufferId);
   StringRef buffer = sourceMgr.extractText(entireRange);

   const unsigned char *bufStart = reinterpret_cast<const unsigned char *>(buffer.data());
   unsigned offset = sourceMgr.getLocOffsetInBuffer(loc, bufferId);

   const unsigned char *startOfLine = find_start_of_line(bufStart, bufStart + offset);
   const unsigned char *endOfIndentation = startOfLine;
   while (*endOfIndentation && is_horizontal_whitespace(*endOfIndentation)) {
      ++endOfIndentation;
   }
   return StringRef(reinterpret_cast<const char *>(startOfLine), endOfIndentation - startOfLine);
}

ArrayRef<Token> slice_token_array(ArrayRef<Token> allTokens, SourceLoc startLoc, SourceLoc endLoc)
{
   assert(startLoc.isValid() && endLoc.isValid());
   auto startIt = token_lower_bound(allTokens, startLoc);
   auto endIt = token_lower_bound(allTokens, endLoc);
   assert(startIt->getLoc() == startLoc && endIt->getLoc() == endLoc);
   return allTokens.slice(startIt - allTokens.begin(), endIt - startIt + 1);
}

template <typename DF>
void tokenize(const LangOptions &langOpts, const SourceManager &sourceMgr,
              unsigned bufferId, unsigned offset, unsigned endOffset,
              DiagnosticEngine * diags,
              CommentRetentionMode commentRetention,
              TriviaRetentionMode triviaRetention, DF &&destFunc)
{
   assert(triviaRetention != TriviaRetentionMode::WithTrivia && "string interpolation with trivia is not implemented yet");

   if (offset == 0 && endOffset == 0) {
      endOffset = sourceMgr.getRangeForBuffer(bufferId).getByteLength();
   }

   Lexer lexer(langOpts, sourceMgr, bufferId, diags, commentRetention, triviaRetention, offset,
               endOffset);

   Token token;
   ParsedTrivia leadingTrivia;
   ParsedTrivia trailingTrivia;
   do {
      lexer.lex(token, leadingTrivia, trailingTrivia);
      destFunc(token, leadingTrivia, trailingTrivia);
   } while (token.getKind() != TokenKindType::END);
}

std::vector<Token> tokenize(const LangOptions &langOpts,
                            const SourceManager &sourceMgr, unsigned bufferId,
                            unsigned offset, unsigned endOffset,
                            DiagnosticEngine *diags,
                            bool keepComments)
{
   std::vector<Token> tokens;
   tokenize(langOpts, sourceMgr, bufferId, offset, endOffset,
            diags,
            keepComments ? CommentRetentionMode::ReturnAsTokens
                         : CommentRetentionMode::AttachToNextToken,
            TriviaRetentionMode::WithoutTrivia,
            [&](const Token &token, const ParsedTrivia &leadingTrivia,
            const ParsedTrivia &trailingTrivia) { tokens.push_back(token); });
   assert(tokens.back().is(TokenKindType::END));
   tokens.pop_back(); // Remove EOF.
   return tokens;
}

} // polar::parser
