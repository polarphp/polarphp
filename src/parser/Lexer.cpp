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
#include "polarphp/utils/MathExtras.h"

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

Lexer::Lexer(Lexer &parent, State beginState, State endState)
   : Lexer(PrincipalTag(), parent.m_langOpts, parent.m_sourceMgr,
           parent.m_bufferId, parent.m_diags, parent.m_commentRetention,
           parent.m_triviaRetention)
{
   assert(m_bufferId == m_sourceMgr.findBufferContainingLoc(beginState.m_loc) &&
          "state for the wrong buffer");
   assert(m_bufferId == m_sourceMgr.findBufferContainingLoc(endState.m_loc) &&
          "state for the wrong buffer");

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
   lexer.restoreState(State(loc));
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

namespace {
void validate_multiline_indents(const Token &str, DiagnosticEngine *diags);
void diagnose_embedded_null(DiagnosticEngine *diags, const unsigned char *ptr);
bool advance_if_valid_start_of_identifier(const unsigned char *&ptr,
                                          const unsigned char *end);
bool advance_if_valid_start_of_operator(const unsigned char *&ptr,
                                        const unsigned char *end);
bool advance_if_valid_continuation_of_identifier(const unsigned char *&ptr,
                                                 const unsigned char *end);
} // anonymous namespace

void Lexer::formStringLiteralToken(const unsigned char *tokenStart, bool isMultilineString)
{
   formToken(TokenKindType::T_STRING, tokenStart);
   if (m_nextToken.is(TokenKindType::END)) {
      return;
   }
   m_nextToken.setStringLiteral(isMultilineString);

   if (isMultilineString && m_diags) {
      validate_multiline_indents(m_nextToken, m_diags);
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

Lexer::State Lexer::getStateForBeginningOfTokenLoc(SourceLoc sourceLoc) const
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
   return State(SourceLoc(polar::utils::SMLocation::getFromPointer(reinterpret_cast<const char *>(ptr))));
}

namespace {

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
                            const unsigned char *codeCompletionPtr = nullptr,
                            DiagnosticEngine *diags = nullptr) {
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

bool skip_to_end_of_slash_star_comment(const unsigned char *&m_yyCursor,
                                       const unsigned char *bufferEnd,
                                       const unsigned char *codeCompletionPtr = nullptr,
                                       DiagnosticEngine *diags = nullptr)
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

bool is_valid_identifier_continuation_code_point(uint32_t c)
{
   if (c < 0x80) {
      return polar::basic::is_identifier_body(c, true);
   }
   // N1518: Recommendations for extended identifier characters for C and C++
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

   // N1518: Recommendations for extended identifier characters for C and C++
   // Proposed Annex X.2: Ranges of characters disallowed initially
   if ((c >= 0x0300 && c <= 0x036F) ||
       (c >= 0x1DC0 && c <= 0x1DFF) ||
       (c >= 0x20D0 && c <= 0x20FF) ||
       (c >= 0xFE20 && c <= 0xFE2F)) {
      return false;
   }
   return true;
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
   return advance_if(ptr, end, Identifier::isOperatorStartCodePoint);
}

bool advance_if_valid_continuation_of_operator(const unsigned char *&ptr,
                                               const unsigned char *end)
{
   return advance_if(ptr, end, Identifier::isOperatorContinuationCodePoint);
}

} // anonymous namespace

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
      m_nextToken.setSemanticValue(std::move(numValue));
   } else {
      double numValue = 0;
      const char *tempPtr = reinterpret_cast<const char *>(bnumStrEnd);
      numValue = polar::utils::bstr_to_double(bnumStr, &tempPtr);
      /// errno isn't checked since we allow HUGE_VAL/INF overflow
      assert(bnumStrEnd == yytext + m_yyLength);
      formToken(TokenKindType::T_DNUMBER, m_yyText);
      m_nextToken.setSemanticValue(std::move(numValue));
   }
}

void Lexer::lexHexNumber()
{

}

void Lexer::lexLongNumber()
{

}

void Lexer::lexDoubleNumber()
{

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
            break;
         } else if (*yycursor++ == '\\' && yycursor < yylimit) {
            ++yycursor;
         }
      } else {
         m_yyLength = yylimit - yytext;
         /// Unclosed single quotes; treat similar to double quotes, but without a separate token
         /// for ' (unrecognized by parser), instead of old flex fallback to "Unexpected character..."
         /// rule, which continued in ST_IN_SCRIPTING state after the quote
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
      return;
   }
   strValue.append(reinterpret_cast<const char *>(m_yyText + bprefix + 1), m_yyLength - bprefix - 2);
   size_t filteredLength = convert_single_quote_str_escape_sequences(strValue.data(), strValue.data() + strValue.length(), * this);
   strValue.reserve(filteredLength);
   /// TODO
   /// add output filter support ?
   formToken(TokenKindType::T_CONSTANT_ENCAPSED_STRING, m_yyText);
   m_nextToken.setSemanticValue(std::move(strValue));
   return;
}

void Lexer::lexDoubleQuoteStringStart()
{
   const unsigned char *yytext = m_yyText;
   const unsigned char *&yycursor = m_yyCursor;
   const unsigned char *yylimit = m_artificialEof;
   int bprefix = yytext[0] != '\'' ? 1 : 0;
   std::string filteredStr;
   while (yycursor < yylimit) {
      switch (*yycursor++) {
      case '"':
         m_yyLength = yycursor - m_yyText;
         if (convert_double_quote_str_escape_sequences(filteredStr, '"', yytext + bprefix + 1, yycursor - 1, *this) ||
             !isInParseMode()) {
            formToken(TokenKindType::T_CONSTANT_ENCAPSED_STRING, m_yyText);
            m_nextToken.setSemanticValue(std::move(filteredStr));
         } else {
            formToken(TokenKindType::T_ERROR, m_yyText);
         }
         return;
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
      yycursor--;
      break;
   }
   /// Remember how much was scanned to save rescanning
   m_scannedStringLength = yycursor - yytext - m_yyLength;
   yycursor = yytext + m_yyLength;
   m_yyCondition = COND_NAME(ST_DOUBLE_QUOTES);
   formToken(TokenKindType::T_DOUBLE_STR_QUOTE, yytext);
}

void Lexer::lexDoubleQuoteStringBody()
{
   const unsigned char *&yycursor = m_yyCursor;
   const unsigned char *yytext = m_yyText;
   const unsigned char *yylimit = m_artificialEof;
   if (m_scannedStringLength) {
      yycursor += m_scannedStringLength - 1;
      m_scannedStringLength = 0;
   } else {
      if (yycursor > yylimit) {
         formToken(TokenKindType::T_ERROR, yytext);
         return;
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
   }
   m_yyLength = yycursor - yytext;
   std::string filteredStr;
   if (convert_double_quote_str_escape_sequences(filteredStr, '"', yytext, yycursor, *this) ||
       !isInParseMode()) {
      formToken(TokenKindType::T_CONSTANT_ENCAPSED_STRING, yytext);
      m_nextToken.setSemanticValue(std::move(filteredStr));
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

   if (yycursor > yylimit) {
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
      m_nextToken.setSemanticValue(std::move(filteredStr));
   } else {
      formToken(TokenKindType::T_ERROR, yytext);
   }
}

void Lexer::lexHeredocStart()
{
   const unsigned char *iter = nullptr;
   const unsigned char *savedCursor = nullptr;
   const unsigned char *yytext = m_yyText;
   const unsigned char *&yycursor = m_yyCursor;
   const unsigned char *yylimit = m_artificialEof;
   int bprefix = (yytext[0] != '<') ? 1 : 0;
   int spacing = 0;
   int indentation = 0;
   std::string heredocLabel;
   bool isHeredoc = true;
   incLineNumber();
   int hereDocLabelLength = m_yyLength - bprefix - 3 - 1 - (yytext[m_yyLength - 2] == '\r' ? 1 : 0);
   iter = yytext + bprefix + 3;
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
      yycursor = savedCursor;
      formToken(TokenKindType::T_START_HEREDOC, yytext);
      return;
   }
   /// Check for ending label on the next line
   /// optimzed for empty heredoc
   if (hereDocLabelLength < yylimit - yycursor &&
       StringRef(reinterpret_cast<const char *>(yycursor), hereDocLabelLength) == heredocLabel) {
      if (!IS_LABEL_START(yycursor[hereDocLabelLength])) {
         if (spacing == (HEREDOC_USING_SPACES | HEREDOC_USING_TABS)) {
            /// TODO
   //         zend_throw_exception(zend_ce_parse_error, "Invalid indentation - tabs and spaces cannot be mixed", 0);
         }
         yycursor = savedCursor;
         label->indentation = indentation;
         m_yyCondition = COND_NAME(ST_END_HEREDOC);
         formToken(TokenKindType::T_START_HEREDOC, yytext);
         return;
      }
   }

   yycursor = savedCursor;
   if (isHeredoc && !m_heredocScanAhead) {

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

namespace {
void validate_multiline_indents(const Token &str,
                                DiagnosticEngine *diags)
{

}
} // anonymous namespace

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

   /// here we want keep comment for next token
   m_yyText = m_yyCursor;
   lexTrivia(m_leadingTrivia, /* IsForTrailingTrivia */ false);

   // invoke yylexer
   if (m_incrementLineNumber) {
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
         if (token.is(TokenKindType::T_STRING)) {
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

}

Lexer &Lexer::restoreYYState()
{

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
   lexer.restoreState(State(loc));
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
   lexer.restoreState(State(loc));
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
