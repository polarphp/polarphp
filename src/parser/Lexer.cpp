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
#include "clang/Basic/CharInfo.h"
#include "polarphp/syntax/Trivia.h"
#include "polarphp/utils/MathExtras.h"
#include "polarphp/kernel/LangOptions.h"
#include "polarphp/kernel/Exceptions.h"
#include "llvm/ADT/SmallString.h"

#include <set>
#include <string>
#include <cstdint>
#include <iostream>

namespace polar::parser {

using polar::SmallString;

using namespace internal;
using namespace polar::syntax;
using namespace polar;

#define HEREDOC_USING_SPACES 1
#define HEREDOC_USING_TABS 2

#if POLAR_POINTER_SIZE == 8
#  define MAX_LENGTH_OF_INT64 20
#else
#  define MAX_LENGTH_OF_INT64 11
#endif

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
   size_t BOMLength = contents.startswith("\xEF\xBB\xBF") ? 3 : 0;
   // Keep information about existance of UTF-8 BOM for transparency source code
   // editing with libSyntax.
   m_contentStart = m_bufferStart + BOMLength;
   // Initialize code completion.
   if (m_bufferId == m_sourceMgr.getCodeCompletionBufferId()) {
      const unsigned char *ptr = m_bufferStart + m_sourceMgr.getCodeCompletionOffset();
      if (ptr >= m_bufferStart && ptr <= m_bufferEnd) {
         m_codeCompletionPtr = ptr;
      }
   }
   m_artificialEof = m_bufferStart + endOffset;
   m_yyCursor = m_bufferStart + offset;

   assert(m_nextToken.is(TokenKindType::T_UNKNOWN_MARK));
}

void Lexer::lex(Token &result, ParsedTrivia &leadingTriviaResult, ParsedTrivia &trailingTrivialResult)
{
   lexImpl();
   assert((m_nextToken.isAtStartOfLine() || m_yyCursor != m_bufferStart) &&
          "The token should be at the beginning of the line, "
          "or we should be lexing from the middle of the buffer");
   result = m_nextToken;
   if (m_triviaRetention == TriviaRetentionMode::WithTrivia) {
      leadingTriviaResult = {m_leadingTrivia};
      trailingTrivialResult = {m_trailingTrivia};
   }
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
      auto iter = llvm::find_if(m_leadingTrivia, [](const ParsedTriviaPiece &piece) {
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

void Lexer::formStringVariableToken(const unsigned char *tokenStart)
{
   formToken(TokenKindType::T_STRING_VARNAME, tokenStart);
   m_nextToken.setValue(StringRef(reinterpret_cast<const char *>(tokenStart), m_yyLength));
}

void Lexer::formErrorToken(const unsigned char *tokenStart)
{
   formToken(TokenKindType::T_ERROR, tokenStart);
   if (!m_currentExceptionMsg.empty()) {
      m_nextToken.setValue(m_currentExceptionMsg);
   }
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

bool Lexer::nextLineHasHeredocEndMarker()
{
   if (m_heredocLabelStack.empty()) {
      return false;
   }
   const unsigned char *savedCursor = m_yyCursor;
   const unsigned char *&cursor = m_yyCursor;
   const unsigned char *yylimit = m_artificialEof;
   std::shared_ptr<HereDocLabel> label = m_heredocLabelStack.top();
   /// trim
   while (cursor < yylimit) {
      switch (*cursor++) {
      case '\n':
      case '\t':
      case ' ':
         continue;
      default:
         --cursor;
         break;
      }
      break;
   }
   bool found = false;
   if (isFoundHeredocEndMarker(label) && !isLabelStart(cursor[label->name.size()])) {
      found = true;
   }
   cursor = savedCursor;
   return found;
}

bool Lexer::isFoundHeredocEndMarker(std::shared_ptr<HereDocLabel> label) const
{
   size_t labelLength = label->name.size();
   return isLabelStart(*m_yyCursor) &&
         (labelLength < m_artificialEof - m_yyCursor) &&
         (StringRef(reinterpret_cast<const char *>(m_yyCursor), labelLength) == label->name);
}

void Lexer::notifyLexicalException(StringRef msg, int code)
{
   m_flags.setLexExceptionOccurred(true);
   m_currentExceptionMsg = msg.str();
   if (m_lexicalExceptionHandler) {
      m_lexicalExceptionHandler(msg, code);
   }
}

LexerState Lexer::getStateForBeginningOfTokenLoc(SourceLoc sourceLoc) const
{
   const unsigned char *ptr = getBufferPtrForSourceLoc(sourceLoc);
   // Skip whitespace backwards until we hit a newline.  This is needed to
   // correctly lex the token if it is at the beginning of the line.
   while (ptr >= m_contentStart + 1) {
      unsigned char c = ptr[-1];
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
   return LexerState(SourceLoc(SMLoc::getFromPointer(reinterpret_cast<const char *>(ptr))));
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
   char *bnumYYStr = yytext + 2;
   size_t numLength = m_yyLength - 2;
   char *end = nullptr;
   /// Skip any leading 0s
   while (numLength > 0 && (*bnumYYStr == '0' || *bnumYYStr == '_')) {
      ++bnumYYStr;
      --numLength;
   }
   std::string bnumStr(bnumYYStr, numLength);
   bool containsUnderscores = bnumStr.find('_') != std::string::npos;
   if (containsUnderscores) {
      strip_underscores(bnumStr, numLength);
   }
   if (numLength < sizeof(std::int64_t) * CHAR_BIT) {
      std::int64_t numValue = 0;
      if (numLength > 0) {
         errno = 0;
         numValue = std::strtoll(bnumStr.c_str(), &end, 2);
         assert(!errno && end == bnumStr.c_str() + numLength);
      }
      formToken(TokenKindType::T_LNUMBER, m_yyText);
      m_nextToken.setValue(numValue);
   } else {
      double numValue = 0;
      numValue = polar::utils::bstr_to_double(bnumStr.c_str(), const_cast<const char **>(&end));
      /// errno isn't checked since we allow HUGE_VAL/INF overflow
      assert(end == bnumStr.c_str() + numLength);
      formToken(TokenKindType::T_DNUMBER);
      m_nextToken.setValue(numValue);
   }
}

void Lexer::lexHexNumber()
{
   /// Skip "0x"
   char *yytext = reinterpret_cast<char *>(const_cast<unsigned char *>(m_yyText));
   char *hexYYStr = yytext + 2;
   char *end = nullptr;
   std::size_t length = m_yyLength - 2;
   std::int64_t lvalue = 0;
   while (*hexYYStr == '0') {
      ++hexYYStr;
      --length;
   }
   std::string hexStr(hexYYStr, length);
   bool containsUnderscores = hexStr.find('_') != std::string::npos;
   if (containsUnderscores) {
      strip_underscores(hexStr, length);
   }
   constexpr int maxWidth = POLAR_POINTER_SIZE * 2;
   if (length < maxWidth || (length == maxWidth && hexStr.at(0) <= '7')) {

      if (length > 0) {
         errno = 0;
         lvalue = std::strtoll(hexStr.c_str(), &end, 16);
         assert(!errno && end == hexStr.c_str() + length);
      }
      formToken(TokenKindType::T_LNUMBER);
      m_nextToken.setValue(lvalue);
   } else {
      /// overflow
      bool needCorrectOverflow = false;
      if (m_nextToken.getKind() == TokenKindType::T_MINUS_SIGN) {
         char buff[maxWidth];
         std::sprintf(buff, "%llx\n ", std::numeric_limits<std::int64_t>::min());
         if (StringRef(buff, maxWidth) == StringRef(hexStr)) {
            needCorrectOverflow = true;
         }
      }
      double dvalue = polar::utils::hexstr_to_double(hexStr.c_str(), const_cast<const char **>(&end));
      /// errno isn't checked since we allow HUGE_VAL/INF overflow
      assert(end == hexStr.c_str() + length);
      formToken(TokenKindType::T_DNUMBER, m_yyText);
      m_nextToken.setNeedCorrectLNumberOverflow(needCorrectOverflow);
      m_nextToken.setValue(dvalue);
   }
}

void Lexer::lexLongNumber()
{
   char *end = nullptr;
   size_t length = m_yyLength;
   std::string lnumStr(reinterpret_cast<char *>(const_cast<unsigned char *>(m_yyText)), length);
   std::int64_t lvalue = 0;
   bool isOctal =  lnumStr.at(0) == '0';
   bool containsUnderscores = lnumStr.find('_') != std::string::npos;
   if (containsUnderscores) {
      strip_underscores(lnumStr, length);
   }

   // Digits 8 and 9 are illegal in octal literals.
   if (isOctal) {
      for(size_t i = 0; i < length; ++i) {
         if (lnumStr.at(i) == 8 || lnumStr.at(i) == 9) {
            notifyLexicalException("Invalid numeric literal", 0);
            if (isInParseMode()) {
               formToken(TokenKindType::T_ERROR);
            }
            // Continue in order to determine if this is T_LNUMBER or T_DNUMBER.
            length = i;
            break;
         }
      }
   }

   if (length < MAX_LENGTH_OF_INT64 - 1) {
      /// Won't overflow
      errno = 0;
      /// base must be passed explicitly for correct parse error on Windows
      lvalue = std::strtoll(lnumStr.c_str(), &end, isOctal ? 8 : 10);
      assert(end == lnumStr.c_str() + length);
   } else {
      errno = 0;
      lvalue = std::strtoll(lnumStr.c_str(), &end, 0);
      /// overflow is undefined behavior
      /// we just handle -9223372036854775808
      if (errno == ERANGE) {
         // Overflow
         errno = 0;
         bool needCorrectOverflow = false;
         if (m_nextToken.getKind() == TokenKindType::T_MINUS_SIGN) {
            std::string minStr;
            if (isOctal) {
               char buff[24];
               std::sprintf(buff, "-0%llo", std::numeric_limits<std::int64_t>::min());
               minStr.append(buff, 24);
            } else {
               minStr = std::to_string(std::numeric_limits<std::int64_t>::min());
            }
            if (StringRef(minStr.data() + 1, minStr.size() - 1) == StringRef(lnumStr)) {
               needCorrectOverflow = true;
            }
         }
         double dvalue = 0.0;
         if (isOctal) {
            /// octal overflow
            dvalue = polar::utils::octstr_to_double(lnumStr.c_str(), const_cast<const char **>(&end));
         } else {
            dvalue = std::strtod(lnumStr.c_str(), &end);
         }
         assert(end == lnumStr.c_str() + length);
         formToken(TokenKindType::T_DNUMBER);
         m_nextToken.setValue(dvalue);
         m_nextToken.setNeedCorrectLNumberOverflow(needCorrectOverflow);
         return;
      }
      assert(end == lnumStr.c_str() + length);
   }
   assert(!errno);
   formToken(TokenKindType::T_LNUMBER);
   m_nextToken.setValue(lvalue);
}

void Lexer::lexDoubleNumber()
{
   char *end = nullptr;
   char *yytext = reinterpret_cast<char *>(const_cast<unsigned char *>(m_yyText));
   size_t length = m_yyLength;
   std::string dnumStr(yytext, m_yyLength);
   bool containsUnderscores = dnumStr.find('_') != std::string::npos;
   if (containsUnderscores) {
      strip_underscores(dnumStr, length);
   }
   double dvalue = std::strtod(dnumStr.c_str(), &end);
   /// errno isn't checked since we allow HUGE_VAL/INF overflow
   assert(end == dnumStr.c_str() + length);
   formToken(TokenKindType::T_DNUMBER);
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
         formToken(TokenKindType::T_ENCAPSED_AND_WHITESPACE);
         return;
      }
   }
   std::string strValue;
   // '0b', '\'' and '\''
   if (m_yyLength - bprefix - 2 <= 1) {
      if (m_yyLength - bprefix - 2 == 1) {
         unsigned char c = *(yytext + bprefix + 1);
         strValue.push_back(c);
      }
   } else {
      strValue.append(reinterpret_cast<const char *>(m_yyText + bprefix + 1), m_yyLength - bprefix - 2);
      long filteredLength = convert_single_quote_str_escape_sequences(strValue.begin(), strValue.end(), *this);
      strValue.resize(filteredLength);
   }
   formToken(TokenKindType::T_CONSTANT_ENCAPSED_STRING);
   m_nextToken.setValue(strValue);
   return;
}

void Lexer::lexDoubleQuoteString()
{
   const unsigned char *&yycursor = m_yyCursor;
   const unsigned char *yytext = m_yyText;
   const unsigned char *yylimit = m_artificialEof;
   if (yycursor >= yylimit) {
      yycursor = yylimit;
      formToken(TokenKindType::T_ERROR);
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
         if (isLabelStart(*yycursor) || *yycursor == '{') {
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
   std::string filteredStr(reinterpret_cast<const char *>(yytext), m_yyLength);
   if (convert_double_quote_str_escape_sequences(filteredStr, '"', filteredStr.begin(),
                                                 filteredStr.end(), *this) ||
       !isInParseMode()) {
      formToken(TokenKindType::T_CONSTANT_ENCAPSED_STRING);
      m_nextToken.setValue(filteredStr);
   } else {
      formToken(TokenKindType::T_ERROR);
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
         if (isLabelStart(*yycursor) || *yycursor == '{') {
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
   std::string filteredStr(reinterpret_cast<const char *>(yytext), m_yyLength);
   if (convert_double_quote_str_escape_sequences(filteredStr, '`', filteredStr.begin(), filteredStr.end(), *this) ||
       !isInParseMode()) {
      formToken(TokenKindType::T_ENCAPSED_AND_WHITESPACE, yytext);
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
   int newlineLength = 1;
   if (yytext[m_yyLength - 2] == 'r') {
      // \r\n
      ++newlineLength;
   }
   std::size_t hereDocLabelLength = m_yyLength - bprefix - 3 - newlineLength;
   iter = yytext + bprefix + 3;
   /// trim leader spaces and tabs
   /// recalculate header length
   ///
   if ((*iter == ' ') || (*iter == '\t')) {
      ++iter;
      --hereDocLabelLength;
   }
   if (*iter == '\'') {
      ++iter;
      hereDocLabelLength -= 2;
      isHeredoc = false;
      setYYCondition(COND_NAME(ST_NOWDOC));
   } else {
      if (*iter == '"') {
         ++iter;
         hereDocLabelLength -= 2;
      }
      setYYCondition(COND_NAME(ST_HEREDOC));
   }
   heredocLabel.append(reinterpret_cast<const char *>(iter), hereDocLabelLength);
   heredocLabel.resize(hereDocLabelLength);
   std::shared_ptr<HereDocLabel> label = std::make_shared<HereDocLabel>();
   label->name = heredocLabel;
   label->indentation = 0;
   /// current position is first column of first line after header
   /// if we found end marker, we use this point to restore yycursor
   /// and goto ST_END_HEREDOC condition
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
      /// precalculate indentation for empty heredoc
      ++indentation;
   }

   /// just empty heredoc with no end mark because it reach eof
   if (yycursor == yylimit) {
      yycursor = savedCursor;
      formToken(TokenKindType::T_START_HEREDOC);
      return;
   }
   /// Check for ending label on the next line
   /// optimzed for empty heredoc
   if (isFoundHeredocEndMarker(label)) {
      if (!isLabelStart(yycursor[hereDocLabelLength])) {
         /// detect heredoc end mark sequence
         if (spacing == (HEREDOC_USING_SPACES | HEREDOC_USING_TABS)) {
            notifyLexicalException("Invalid indentation - tabs and spaces cannot be mixed", 0);
         }
         yycursor = savedCursor;
         label->indentation = indentation;
         m_yyCondition = COND_NAME(ST_END_HEREDOC);
         m_flags.setReserveHeredocSpaces(true);
         formToken(TokenKindType::T_START_HEREDOC);
         return;
      }
   }

   yycursor = savedCursor;

   /// scan ahead to get right intentation
   /// because at lex stage heredoc can nest in {$xxx = <<<XXX
   ///  XXX;
   /// } something like this
   ///
   /// but we only need scan ahead at top level
   ///
   /// note when we scan ahead, we don't care about intentation
   ///
   if (isHeredoc && !flags.isHeredocScanAhead()) {
      /// recursive lex heredoc
      /// at lex stage recursive heredoc is legal，but at parse stage it is illegal
      int heredocNestingLevel = 1;
      TokenKindType firstToken = TokenKindType::T_UNKNOWN_MARK;
      saveYYState();
      m_flags.setHeredocScanAhead(true);
      m_flags.setHeredocIndentationUsesSpaces(false);
      m_heredocIndentation = 0;
      m_eventHandler = nullptr;
      // zend_exception_save();
      while (heredocNestingLevel > 0) {
         Token token;
         lex(token);
         TokenKindType tokenKind = token.getKind();
         if (isLexExceptionOccurred()) {
            clearExceptionFlag();
            break;
         }
         if (firstToken == TokenKindType::T_UNKNOWN_MARK) {
            firstToken = tokenKind;
         }
         switch (tokenKind) {
         case TokenKindType::T_START_HEREDOC:
            ++heredocNestingLevel;
            break;
         case TokenKindType::T_END_HEREDOC:
            --heredocNestingLevel;
            break;
         case TokenKindType::END:
            heredocNestingLevel = 0;
            break;
         default:
            continue;
         }
      }
      // zend_exception_restore();
      if ((firstToken == TokenKindType::T_VARIABLE ||
           firstToken == TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES ||
           firstToken == TokenKindType::T_CURLY_OPEN) && m_heredocIndentation) {
         notifyLexicalException(0, "Invalid body indentation level (expecting an indentation level of at least %d)",
                                m_heredocIndentation);
      }

      label->indentation = m_heredocIndentation;
      label->intentationUseSpaces = m_flags.isHeredocIndentationUsesSpaces();
      restoreYYState();
      m_flags.setHeredocScanAhead(false);
      m_flags.setIncrementLineNumber(false);
   }
   formToken(TokenKindType::T_START_HEREDOC);
   /// reserve spaces after header
   m_flags.setReserveHeredocSpaces(true);
}

void Lexer::lexHeredocBody()
{
   const unsigned char *yytext = m_yyText;
   const unsigned char *&yycursor = m_yyCursor;
   const unsigned char *yylimit = m_artificialEof;
   std::size_t &yylength = m_yyLength;
   std::shared_ptr<HereDocLabel> label = m_heredocLabelStack.top();
   int newlineLength = 0;
   std::size_t indentation = 0;
   int spacing = 0;
   bool foundEndMarker = false;
   /// lex until we meet end mark or '${' or '{$'
   if (yycursor > yylimit) {
      yycursor = yylimit;
      formToken(TokenKindType::END);
      return;
   }
   /// before control get here, re2c already increment yycursor
   --yycursor;
   while (yycursor < yylimit) {
      switch (*yycursor++) {
      case '\r':
         if (*yycursor == '\n') {
            ++yycursor;
         }
         [[fallthrough]];
      case '\n':
         /// check wehther this line have end marker
         indentation = spacing = 0;
         while (yycursor < yylimit && (*yycursor == ' ' || *yycursor == '\t')) {
            if (*yycursor == ' ') {
               spacing |= HEREDOC_USING_SPACES;
            } else {
               spacing |= HEREDOC_USING_TABS;
            }
            ++yycursor;
            ++indentation;
         }

         if (yycursor == yylimit) {
            yylength = yycursor - yytext;
            formToken(TokenKindType::T_ENCAPSED_AND_WHITESPACE);
            /// save unclosed string into token
            m_nextToken.setValue(StringRef(reinterpret_cast<const char *>(yytext), yylength));
            return;
         }
         /// Check for ending label on the next line
         if (isFoundHeredocEndMarker(label)) {
            if (isLabelStart(yycursor[label->name.size()])) {
               /// just normal string
               continue;
            }
            if (spacing == (HEREDOC_USING_SPACES | HEREDOC_USING_TABS)) {
               notifyLexicalException("Invalid indentation - tabs and spaces cannot be mixed", 0);
            }
            /// newline before label will be subtracted from returned text, but
            /// yyleng/yytext will include it
            if (yycursor[-indentation - 2] == '\r' && yycursor[-indentation - 1] == '\n') {
               newlineLength = 2;
            } else {
               newlineLength = 1;
            }
            /// For newline before label
            m_flags.setIncrementLineNumber(true);
            if (m_flags.isHeredocScanAhead()) {
               /// in scan ahead mode, we don't care about indentation
               /// just record it
               m_heredocIndentation = indentation;
               m_flags.setHeredocIndentationUsesSpaces(spacing == HEREDOC_USING_SPACES);
            } else {
               yycursor -= indentation;
            }
            m_flags.setReserveHeredocSpaces(true);
            setYYCondition(COND_NAME(ST_END_HEREDOC));
            foundEndMarker = true;
            break;
         }
         continue;
      case '$':
         if (isLabelStart(*yycursor) || *yycursor == '{') {
            break;
         }
         continue;
      case '{':
         if (*yycursor == '$') {
            break;
         }
         continue;
      case '\\':
         if (yycursor < yylimit && *yycursor != '\n' && *yycursor != '\r') {
            ++yycursor;
         }
         [[fallthrough]];
      default:
         continue;
      }
      if (!foundEndMarker) {
         --yycursor;
      }
      break;
   }
   yylength = yycursor - yytext;
   /// scan ahead and normal mode both need exclude newline
   std::string filteredStr(reinterpret_cast<const char *>(yytext), yylength - newlineLength);
   if (!m_flags.isHeredocScanAhead() && !m_flags.isLexExceptionOccurred() && (isInParseMode() || m_flags.isCheckHeredocIndentation())) {
      /// TODO
      /// need review here
      bool newlineAtStart = *(yytext - 1) == '\n' || *(yytext - 1) == '\r';
      if (!strip_multiline_string_indentation(*this, filteredStr, label->indentation, label->intentationUseSpaces,
                                              newlineAtStart, newlineLength != 0)) {
         formErrorToken(yytext);
         return;
      }
      if (!convert_double_quote_str_escape_sequences(filteredStr, 0, filteredStr.begin(),
                                                     filteredStr.end(), *this)) {
         formToken(TokenKindType::T_ERROR);
         return;
      }
   }
   formToken(TokenKindType::T_ENCAPSED_AND_WHITESPACE);
   m_nextToken.setValue(filteredStr);
}

void Lexer::lexNowdocBody()
{
   const unsigned char *yytext = m_yyText;
   const unsigned char *&yycursor = m_yyCursor;
   const unsigned char *yylimit = m_artificialEof;
   std::size_t &yylength = m_yyLength;
   std::shared_ptr<HereDocLabel> label = m_heredocLabelStack.top();
   int newlineLength = 0;
   std::size_t indentation = 0;
   int spacing = 0;
   bool foundEndMarker = false;
   if (yycursor > yylimit) {
      yycursor = yylimit;
      formToken(TokenKindType::END);
      return;
   }
   --yycursor;
   while (yycursor < yylimit) {
      switch (*yycursor++) {
      case '\r':
         if (*yycursor == '\n') {
            ++yycursor;
         }
         [[fallthrough]];
      case '\n':
         indentation = spacing = 0;
         while (yycursor < yylimit && (*yycursor == ' ' || *yycursor == '\t')) {
            if (*yycursor == '\t') {
               spacing |= HEREDOC_USING_TABS;
            } else {
               spacing |= HEREDOC_USING_SPACES;
            }
            ++yycursor;
            ++indentation;
         }
         if (yycursor == yylimit) {
            m_yyLength = yycursor - yytext;
            formToken(TokenKindType::T_ENCAPSED_AND_WHITESPACE);
            m_nextToken.setValue(StringRef(reinterpret_cast<const char *>(yytext), yylength));
            return;
         }
         /// Check for ending label on the next line
         if (isFoundHeredocEndMarker(label)) {
            if (isLabelStart(yycursor[label->name.size()])) {
               continue;
            }
            if (spacing == (HEREDOC_USING_SPACES | HEREDOC_USING_TABS)) {
               notifyLexicalException(0, "Invalid indentation - tabs and spaces cannot be mixed");
            }
            /// newline before label will be subtracted from returned text, but
            /// yyleng/yytext will include it
            if (yycursor[-indentation - 2] == '\r' && yycursor[-indentation - 1] == '\n') {
               newlineLength = 2;
            } else {
               newlineLength = 1;
            }
            /// For newline before label
            m_flags.setIncrementLineNumber(true);
            m_flags.setReserveHeredocSpaces(true);
            yycursor -= indentation;
            label->indentation = indentation;
            setYYCondition(COND_NAME(ST_END_HEREDOC));
            foundEndMarker = true;
            break;
         }
         [[fallthrough]];
      default:
         continue;
      }
      if (foundEndMarker) {
         break;
      }
   }
   yylength = yycursor - yytext;
   std::string filteredStr(reinterpret_cast<const char *>(yytext), yylength - newlineLength);
   if (!m_flags.isLexExceptionOccurred() && spacing != 0 && (isInParseMode() || m_flags.isCheckHeredocIndentation())) {
      bool newlineAtStart = *(yytext - 1) == '\n' || *(yytext - 1) == '\r';
      if (!strip_multiline_string_indentation(*this, filteredStr, indentation, spacing == HEREDOC_USING_SPACES,
                                              newlineAtStart, newlineLength != 0)) {
         formErrorToken(yytext);
         return;
      }
   }
   formToken(TokenKindType::T_ENCAPSED_AND_WHITESPACE, yytext);
   m_nextToken.setValue(filteredStr);
}

void Lexer::lexHereAndNowDocEnd()
{
   /// handle empty nowdoc and heredoc
   if (m_nextToken.getKind() == TokenKindType::T_START_HEREDOC) {
      setYYLength(0);
      formToken(TokenKindType::T_ENCAPSED_AND_WHITESPACE, getYYText() - 1);
      m_nextToken.setValue("");
      setYYCursor(getYYText());
      return;
   }
   std::shared_ptr<HereDocLabel> label = m_heredocLabelStack.top();
   m_heredocLabelStack.pop();
   m_yyLength = label->indentation + label->name.size();
   m_yyCursor += m_yyLength - 1;
   m_yyCondition = COND_NAME(ST_IN_SCRIPTING);
   formToken(TokenKindType::T_END_HEREDOC, m_yyText);
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
         std::size_t BOMLen = m_contentStart - m_bufferStart;
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
   if (!m_flags.isReserveHeredocSpaces()) {
      lexTrivia(m_leadingTrivia, /* IsForTrailingTrivia */ false);
   } else {
      m_flags.setReserveHeredocSpaces(false);
   }
   m_yyText = m_yyCursor;
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
      if (offset < tokenOffsets + token.getLexicalLength()) {
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
   state.setLexerFlags(m_flags);
   state.setLexicalEventHandler(m_eventHandler);
   state.setLexicalExceptionHandler(m_lexicalExceptionHandler);

   state.setHeredocLabelStack(m_heredocLabelStack);

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
   return loc.getAdvancedLocOrInvalid(getTokenAtLocation(sourceMgr, loc).getLexicalLength());
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
            [&](const Lexer &lexer, const Token &token, const ParsedTrivia &leadingTrivia,
            const ParsedTrivia &trailingTrivia) { tokens.push_back(token); });
   assert(tokens.back().is(TokenKindType::END));
   tokens.pop_back(); // Remove EOF.
   return tokens;
}

} // polar::parser
