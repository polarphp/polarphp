// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//

#include "polarphp/utils/yaml/YamlParser.h"
#include "polarphp/basic/adt/AllocatorList.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/SourceLocation.h"
#include "polarphp/utils/SourceMgr.h"
#include "polarphp/utils/RawOutStream.h"
#include "polarphp/utils/Unicode.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

namespace polar {
namespace yaml {

using polar::basic::BumpPtrList;
using polar::basic::ArrayRef;
using polar::basic::SmallVector;
using polar::basic::SmallString;
using polar::utils::MemoryBuffer;
using polar::basic::utohexstr;
using polar::utils::report_fatal_error;

enum UnicodeEncodingForm
{
   UEF_UTF32_LE, ///< UTF-32 Little Endian
   UEF_UTF32_BE, ///< UTF-32 Big Endian
   UEF_UTF16_LE, ///< UTF-16 Little Endian
   UEF_UTF16_BE, ///< UTF-16 Big Endian
   UEF_UTF8,     ///< UTF-8 or ascii.
   UEF_Unknown   ///< Not a valid Unicode encoding.
};

/// EncodingInfo - Holds the encoding type and length of the byte order mark if
///                it exists. Length is in {0, 2, 3, 4}.
using EncodingInfo = std::pair<UnicodeEncodingForm, unsigned>;

/// get_unicode_encoding - Reads up to the first 4 bytes to determine the Unicode
///                      encoding form of \a input.
///
/// @param input A string of length 0 or more.
/// @returns An EncodingInfo indicating the Unicode encoding form of the input
///          and how long the byte order mark is if one exists.
static EncodingInfo get_unicode_encoding(StringRef input)
{
   if (input.empty()) {
      return std::make_pair(UEF_Unknown, 0);
   }
   switch (uint8_t(input[0])) {
   case 0x00:
      if (input.getSize() >= 4) {
         if (input[1] == 0
             && uint8_t(input[2]) == 0xFE
             && uint8_t(input[3]) == 0xFF) {
            return std::make_pair(UEF_UTF32_BE, 4);
         }
         if (input[1] == 0 && input[2] == 0 && input[3] != 0) {
            return std::make_pair(UEF_UTF32_BE, 0);
         }
      }
      if (input.getSize() >= 2 && input[1] != 0) {
         return std::make_pair(UEF_UTF16_BE, 0);
      }
      return std::make_pair(UEF_Unknown, 0);
   case 0xFF:
      if (input.getSize() >= 4
          && uint8_t(input[1]) == 0xFE
          && input[2] == 0
          && input[3] == 0) {
         return std::make_pair(UEF_UTF32_LE, 4);
      }

      if (input.getSize() >= 2 && uint8_t(input[1]) == 0xFE) {
         return std::make_pair(UEF_UTF16_LE, 2);
      }
      return std::make_pair(UEF_Unknown, 0);
   case 0xFE:
      if (input.getSize() >= 2 && uint8_t(input[1]) == 0xFF) {
         return std::make_pair(UEF_UTF16_BE, 2);
      }
      return std::make_pair(UEF_Unknown, 0);
   case 0xEF:
      if (input.getSize() >= 3
          && uint8_t(input[1]) == 0xBB
          && uint8_t(input[2]) == 0xBF) {
         return std::make_pair(UEF_UTF8, 3);
      }
      return std::make_pair(UEF_Unknown, 0);
   }

   // It could still be utf-32 or utf-16.
   if (input.getSize() >= 4 && input[1] == 0 && input[2] == 0 && input[3] == 0) {
      return std::make_pair(UEF_UTF32_LE, 0);
   }
   if (input.getSize() >= 2 && input[1] == 0) {
      return std::make_pair(UEF_UTF16_LE, 0);
   }

   return std::make_pair(UEF_UTF8, 0);
}

/// Pin the vtables to this file.
void Node::getAnchor()
{}
void NullNode::getAnchor()
{}
void ScalarNode::getAnchor()
{}
void BlockScalarNode::getAnchor()
{}
void KeyValueNode::getAnchor()
{}
void MappingNode::getAnchor()
{}
void SequenceNode::getAnchor()
{}
void AliasNode::getAnchor()
{}

/// Token - A single YAML token.
struct Token
{
   enum TokenKind
   {
      token_Error, // Uninitialized token.
      token_StreamStart,
      token_StreamEnd,
      token_VersionDirective,
      token_TagDirective,
      token_DocumentStart,
      token_DocumentEnd,
      token_BlockEntry,
      token_BlockEnd,
      token_BlockSequenceStart,
      token_BlockMappingStart,
      token_FlowEntry,
      token_FlowSequenceStart,
      token_FlowSequenceEnd,
      token_FlowMappingStart,
      token_FlowMappingEnd,
      token_m_key,
      token_Value,
      token_Scalar,
      token_BlockScalar,
      token_Alias,
      token_Anchor,
      token_tag
   } m_kind = token_Error;

   /// A string of length 0 or more whose begin() points to the logical location
   /// of the token in the input.
   StringRef m_range;

   /// The value of a block scalar node.
   std::string m_value;

   Token() = default;
};

using TokenQueueT = BumpPtrList<Token>;

namespace {

/// This struct is used to track simple keys.
///
/// Simple keys are handled by creating an entry in m_simpleKeys for each Token
/// which could legally be the start of a simple key. When peekNext is called,
/// if the Token To be returned is referenced by a SimpleKey, we continue
/// tokenizing until that potential simple key has either been found to not be
/// a simple key (we moved on to the next line or went further than 1024 chars).
/// Or when we run into a m_value, and then insert a m_key token (and possibly
/// others) before the SimpleKey's m_token.
struct SimpleKey
{
   bool m_isRequired;
   unsigned m_column;
   unsigned m_line;
   unsigned m_flowLevel;
   TokenQueueT::Iterator m_token;

   bool operator ==(const SimpleKey &other)
   {
      return m_token == other.m_token;
   }
};

} // end anonymous namespace

/// The Unicode scalar value of a UTF-8 minimal well-formed code unit
///        subsequence and the subsequence's length in code units (uint8_t).
///        A length of 0 represents an error.
using UTF8Decoded = std::pair<uint32_t, unsigned>;

static UTF8Decoded decode_utf8(StringRef range)
{
   StringRef::iterator position= range.begin();
   StringRef::iterator end = range.end();
   // 1 byte: [0x00, 0x7f]
   // Bit pattern: 0xxxxxxx
   if ((*position & 0x80) == 0) {
      return std::make_pair(*position, 1);
   }
   // 2 bytes: [0x80, 0x7ff]
   // Bit pattern: 110xxxxx 10xxxxxx
   if (position + 1 != end &&
       ((*position & 0xE0) == 0xC0) &&
       ((*(position + 1) & 0xC0) == 0x80)) {
      uint32_t codepoint = ((*position & 0x1F) << 6) |
            (*(position + 1) & 0x3F);
      if (codepoint >= 0x80) {
         return std::make_pair(codepoint, 2);
      }
   }
   // 3 bytes: [0x8000, 0xffff]
   // Bit pattern: 1110xxxx 10xxxxxx 10xxxxxx
   if (position + 2 != end &&
       ((*position & 0xF0) == 0xE0) &&
       ((*(position + 1) & 0xC0) == 0x80) &&
       ((*(position + 2) & 0xC0) == 0x80)) {
      uint32_t codepoint = ((*position & 0x0F) << 12) |
            ((*(position + 1) & 0x3F) << 6) |
            (*(position + 2) & 0x3F);
      // Codepoints between 0xD800 and 0xDFFF are invalid, as
      // they are high / low surrogate halves used by UTF-16.
      if (codepoint >= 0x800 &&
          (codepoint < 0xD800 || codepoint > 0xDFFF)) {
         return std::make_pair(codepoint, 3);
      }
   }
   // 4 bytes: [0x10000, 0x10FFFF]
   // Bit pattern: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
   if (position + 3 != end &&
       ((*position & 0xF8) == 0xF0) &&
       ((*(position + 1) & 0xC0) == 0x80) &&
       ((*(position + 2) & 0xC0) == 0x80) &&
       ((*(position + 3) & 0xC0) == 0x80)) {
      uint32_t codepoint = ((*position & 0x07) << 18) |
            ((*(position + 1) & 0x3F) << 12) |
            ((*(position + 2) & 0x3F) << 6) |
            (*(position + 3) & 0x3F);
      if (codepoint >= 0x10000 && codepoint <= 0x10FFFF) {
         return std::make_pair(codepoint, 4);
      }
   }
   return std::make_pair(0, 0);
}

/// Scans YAML tokens from a MemoryBuffer.
class Scanner
{
public:
   Scanner(StringRef input, SourceMgr &sourceMgr, bool showColors = true,
           std::error_code *errorCode = nullptr);
   Scanner(MemoryBufferRef buffer, SourceMgr &sourceMgr, bool showColors = true,
           std::error_code *errorCode = nullptr);

   /// Parse the next token and return it without popping it.
   Token &peekNext();

   /// Parse the next token and pop it from the queue.
   Token getNext();

   void printError(SMLocation location, SourceMgr::DiagKind kind, const Twine &message,
                   ArrayRef<SMRange> ranges = std::nullopt) {
      m_sourceMgr.printMessage(location, kind, message, ranges, /* FixIts= */ std::nullopt, m_showColors);
   }

   void setError(const Twine &message, StringRef::iterator)
   {
      if (m_current >= m_end) {
         m_current = m_end - 1;
      }
      // propagate the error if possible
      if (m_errorCode) {
         *m_errorCode = make_error_code(std::errc::invalid_argument);
      }

      // Don't print out more errors after the first one we encounter. The rest
      // are just the result of the first, and have no meaning.
      if (!m_failed) {
         printError(SMLocation::getFromPointer(m_current), SourceMgr::DK_Error, message);
      }
      m_failed = true;
   }

   void setError(const Twine &message)
   {
      setError(message, m_current);
   }

   /// Returns true if an error occurred while parsing.
   bool failed()
   {
      return m_failed;
   }

private:
   void init(MemoryBufferRef buffer);

   StringRef currentInput()
   {
      return StringRef(m_current, m_end - m_current);
   }

   /// Decode a UTF-8 minimal well-formed code unit subsequence starting
   ///        at \a position.
   ///
   /// If the UTF-8 code units starting at position do not form a well-formed
   /// code unit subsequence, then the Unicode scalar value is 0, and the length
   /// is 0.
   UTF8Decoded decodeUtf8(StringRef::iterator position)
   {
      return decode_utf8(StringRef(position, m_end - position));
   }

   // The following functions are based on the gramar rules in the YAML spec. The
   // style of the function names it meant to closely match how they are written
   // in the spec. The number within the [] is the number of the grammar rule in
   // the spec.
   //
   // See 4.2 [Production Naming Conventions] for the meaning of the prefixes.
   //
   // c-
   //   A production starting and ending with a special character.
   // b-
   //   A production matching a single line break.
   // nb-
   //   A production starting and ending with a non-break character.
   // s-
   //   A production starting and ending with a white space character.
   // ns-
   //   A production starting and ending with a non-space character.
   // l-
   //   A production matching complete line(s).

   /// Skip a single nb-char[27] starting at position.
   ///
   /// A nb-char is 0x9 | [0x20-0x7E] | 0x85 | [0xA0-0xD7FF] | [0xE000-0xFEFE]
   ///                  | [0xFF00-0xFFFD] | [0x10000-0x10FFFF]
   ///
   /// @returns The code unit after the nb-char, or position if it's not an
   ///          nb-char.
   StringRef::iterator skipNbChar(StringRef::iterator position);

   /// Skip a single b-break[28] starting at position.
   ///
   /// A b-break is 0xD 0xA | 0xD | 0xA
   ///
   /// @returns The code unit after the b-break, or position if it's not a
   ///          b-break.
   StringRef::iterator skipBBreak(StringRef::iterator position);

   /// Skip a single s-space[31] starting at position.
   ///
   /// An s-space is 0x20
   ///
   /// @returns The code unit after the s-space, or position if it's not a
   ///          s-space.
   StringRef::iterator skipSSpace(StringRef::iterator position);

   /// Skip a single s-white[33] starting at position.
   ///
   /// A s-white is 0x20 | 0x9
   ///
   /// @returns The code unit after the s-white, or position if it's not a
   ///          s-white.
   StringRef::iterator skipSWhite(StringRef::iterator position);

   /// Skip a single ns-char[34] starting at position.
   ///
   /// A ns-char is nb-char - s-white
   ///
   /// @returns The code unit after the ns-char, or position if it's not a
   ///          ns-char.
   StringRef::iterator skipNsChar(StringRef::iterator position);

   using SkipWhileFunc = StringRef::iterator (Scanner::*)(StringRef::iterator);

   /// Skip minimal well-formed code unit subsequences until func
   ///        returns its input.
   ///
   /// @returns The code unit after the last minimal well-formed code unit
   ///          subsequence that func accepted.
   StringRef::iterator skipWhile( SkipWhileFunc func
                                  , StringRef::iterator position);

   /// Skip minimal well-formed code unit subsequences until func returns its
   /// input.
   void advanceWhile(SkipWhileFunc func);

   /// Scan ns-uri-char[39]s starting at Cur.
   ///
   /// This updates Cur and m_column while scanning.
   void scanNsUriChar();

   /// Consume a minimal well-formed code unit subsequence starting at
   ///        \a Cur. Return false if it is not the same Unicode scalar value as
   ///        \a expected. This updates \a m_column.
   bool consume(uint32_t expected);

   /// Skip \a distance UTF-8 code units. Updates \a Cur and \a m_column.
   void skip(uint32_t distance);

   /// Return true if the minimal well-formed code unit subsequence at
   ///        Pos is whitespace or a new line
   bool isBlankOrBreak(StringRef::iterator position);

   /// Consume a single b-break[28] if it's present at the current position.
   ///
   /// Return false if the code unit at the current position isn't a line break.
   bool consumeLineBreakIfPresent();

   /// If m_isSimpleKeyAllowed, create and push_back a new SimpleKey.
   void saveSimpleKeyCandidate(TokenQueueT::Iterator token,
                               unsigned atColumn,
                               bool isRequired);

   /// Remove simple keys that can no longer be valid simple keys.
   ///
   /// Invalid simple keys are not on the current line or are further than 1024
   /// columns back.
   void removeStaleSimpleKeyCandidates();

   /// Remove all simple keys on m_flowLevel \a level.
   void removeSimpleKeyCandidatesOnFlowLevel(unsigned level);

   /// Unroll indentation in \a m_indents back to \a Col. Creates BlockEnd
   ///        tokens if needed.
   bool unrollIndent(int toColumn);

   /// Increase indent to \a Col. Creates \a m_kind token at \a insertPoint
   ///        if needed.
   bool rollIndent(int toColumn,
                   Token::TokenKind kind,
                   TokenQueueT::Iterator insertPoint);

   /// Skip a single-line comment when the comment starts at the current
   /// position of the scanner.
   void skipComment();

   /// Skip whitespace and comments until the start of the next token.
   void scanToNextToken();

   /// Must be the first token generated.
   bool scanStreamStart();

   /// Generate tokens needed to close out the stream
   bool scanStreamEnd();

   /// Scan a %BLAH directive.
   bool scanDirective();

   /// Scan a ... or ---.
   bool scanDocumentIndicator(bool isStart);

   /// Scan a [ or { and generate the proper flow collection start token.
   bool scanFlowCollectionStart(bool isSequence);

   /// Scan a ] or } and generate the proper flow collection end token.
   bool scanFlowCollectionEnd(bool isSequence);

   /// Scan the , that separates entries in a flow collection.
   bool scanFlowEntry();

   /// Scan the - that starts block sequence entries.
   bool scanBlockEntry();

   /// Scan an explicit ? indicating a key.
   bool scanKey();

   /// Scan an explicit : indicating a value.
   bool scanValue();

   /// Scan a quoted scalar.
   bool scanFlowScalar(bool isDoubleQuoted);

   /// Scan an unquoted scalar.
   bool scanPlainScalar();

   /// Scan an Alias or m_anchor starting with * or &.
   bool scanAliasOrAnchor(bool isAlias);

   /// Scan a block scalar starting with | or >.
   bool scanBlockScalar(bool isLiteral);

   /// Scan a chomping indicator in a block scalar header.
   char scanBlockChompingIndicator();

   /// Scan an indentation indicator in a block scalar header.
   unsigned scanBlockIndentationIndicator();

   /// Scan a block scalar header.
   ///
   /// Return false if an error occurred.
   bool scanBlockScalarHeader(char &chompingIndicator, unsigned &indentIndicator,
                              bool &isDone);

   /// Look for the indentation level of a block scalar.
   ///
   /// Return false if an error occurred.
   bool findBlockScalarIndent(unsigned &blockIndent, unsigned blockExitIndent,
                              unsigned &lineBreaks, bool &isDone);

   /// Scan the indentation of a text line in a block scalar.
   ///
   /// Return false if an error occurred.
   bool scanBlockScalarIndent(unsigned blockIndent, unsigned blockExitIndent,
                              bool &isDone);

   /// Scan a tag of the form !stuff.
   bool scanTag();

   /// Dispatch to the next scanning function based on \a *Cur.
   bool fetchMoreTokens();

   /// The SourceMgr used for diagnostics and buffer management.
   SourceMgr &m_sourceMgr;

   /// The original input.
   MemoryBufferRef m_inputBuffer;

   /// The current position of the scanner.
   StringRef::iterator m_current;

   /// The end of the input (one past the last character).
   StringRef::iterator m_end;

   /// m_current YAML indentation level in spaces.
   int m_indent;

   /// m_current column number in Unicode code points.
   unsigned m_column;

   /// m_current line number.
   unsigned m_line;

   /// How deep we are in flow style containers. 0 Means at block level.
   unsigned m_flowLevel;

   /// Are we at the start of the stream?
   bool m_isStartOfStream;

   /// Can the next token be the start of a simple key?
   bool m_isSimpleKeyAllowed;

   /// True if an error has occurred.
   bool m_failed;

   /// Should colors be used when printing out the diagnostic messages?
   bool m_showColors;

   /// Queue of tokens. This is required to queue up tokens while looking
   ///        for the end of a simple key. And for cases where a single character
   ///        can produce multiple tokens (e.g. BlockEnd).
   TokenQueueT m_tokenQueue;

   /// Indentation levels.
   SmallVector<int, 4> m_indents;

   /// Potential simple keys.
   SmallVector<SimpleKey, 4> m_simpleKeys;

   std::error_code *m_errorCode;
};


/// encode_utf8 - Encode \a unicodeScalarValue in UTF-8 and append it to result.
static void encode_utf8( uint32_t unicodeScalarValue, SmallVectorImpl<char> &result)
{
   if (unicodeScalarValue <= 0x7F) {
      result.push_back(unicodeScalarValue & 0x7F);
   } else if (unicodeScalarValue <= 0x7FF) {
      uint8_t firstByte = 0xC0 | ((unicodeScalarValue & 0x7C0) >> 6);
      uint8_t secondByte = 0x80 | (unicodeScalarValue & 0x3F);
      result.push_back(firstByte);
      result.push_back(secondByte);
   } else if (unicodeScalarValue <= 0xFFFF) {
      uint8_t firstByte = 0xE0 | ((unicodeScalarValue & 0xF000) >> 12);
      uint8_t secondByte = 0x80 | ((unicodeScalarValue & 0xFC0) >> 6);
      uint8_t ThirdByte = 0x80 | (unicodeScalarValue & 0x3F);
      result.push_back(firstByte);
      result.push_back(secondByte);
      result.push_back(ThirdByte);
   } else if (unicodeScalarValue <= 0x10FFFF) {
      uint8_t firstByte = 0xF0 | ((unicodeScalarValue & 0x1F0000) >> 18);
      uint8_t secondByte = 0x80 | ((unicodeScalarValue & 0x3F000) >> 12);
      uint8_t ThirdByte = 0x80 | ((unicodeScalarValue & 0xFC0) >> 6);
      uint8_t FourthByte = 0x80 | (unicodeScalarValue & 0x3F);
      result.push_back(firstByte);
      result.push_back(secondByte);
      result.push_back(ThirdByte);
      result.push_back(FourthByte);
   }
}

bool dump_tokens(StringRef input, RawOutStream &outstream)
{
   SourceMgr sourceMgr;
   Scanner scanner(input, sourceMgr);
   while (true) {
      Token token = scanner.getNext();
      switch (token.m_kind) {
      case Token::token_StreamStart:
         outstream << "Stream-start: ";
         break;
      case Token::token_StreamEnd:
         outstream << "Stream-m_end: ";
         break;
      case Token::token_VersionDirective:
         outstream << "Version-Directive: ";
         break;
      case Token::token_TagDirective:
         outstream << "tag-Directive: ";
         break;
      case Token::token_DocumentStart:
         outstream << "Document-start: ";
         break;
      case Token::token_DocumentEnd:
         outstream << "Document-m_end: ";
         break;
      case Token::token_BlockEntry:
         outstream << "Block-Entry: ";
         break;
      case Token::token_BlockEnd:
         outstream << "Block-m_end: ";
         break;
      case Token::token_BlockSequenceStart:
         outstream << "Block-Sequence-start: ";
         break;
      case Token::token_BlockMappingStart:
         outstream << "Block-Mapping-start: ";
         break;
      case Token::token_FlowEntry:
         outstream << "Flow-Entry: ";
         break;
      case Token::token_FlowSequenceStart:
         outstream << "Flow-Sequence-start: ";
         break;
      case Token::token_FlowSequenceEnd:
         outstream << "Flow-Sequence-m_end: ";
         break;
      case Token::token_FlowMappingStart:
         outstream << "Flow-Mapping-start: ";
         break;
      case Token::token_FlowMappingEnd:
         outstream << "Flow-Mapping-m_end: ";
         break;
      case Token::token_m_key:
         outstream << "m_key: ";
         break;
      case Token::token_Value:
         outstream << "m_value: ";
         break;
      case Token::token_Scalar:
         outstream << "Scalar: ";
         break;
      case Token::token_BlockScalar:
         outstream << "Block Scalar: ";
         break;
      case Token::token_Alias:
         outstream << "Alias: ";
         break;
      case Token::token_Anchor:
         outstream << "m_anchor: ";
         break;
      case Token::token_tag:
         outstream << "tag: ";
         break;
      case Token::token_Error:
         break;
      }
      outstream << token.m_range << "\n";
      if (token.m_kind == Token::token_StreamEnd) {
         break;
      } else if (token.m_kind == Token::token_Error) {
         return false;
      }
   }
   return true;
}

bool scan_tokens(StringRef input)
{
   SourceMgr sourceMgr;
   Scanner scanner(input, sourceMgr);
   while (true) {
      Token token = scanner.getNext();
      if (token.m_kind == Token::token_StreamEnd)
         break;
      else if (token.m_kind == Token::token_Error)
         return false;
   }
   return true;
}

std::string escape(StringRef input, bool escapePrintable)
{
   std::string escapedInput;
   for (StringRef::iterator i = input.begin(), e = input.end(); i != e; ++i) {
      if (*i == '\\')
         escapedInput += "\\\\";
      else if (*i == '"')
         escapedInput += "\\\"";
      else if (*i == 0)
         escapedInput += "\\0";
      else if (*i == 0x07)
         escapedInput += "\\a";
      else if (*i == 0x08)
         escapedInput += "\\b";
      else if (*i == 0x09)
         escapedInput += "\\t";
      else if (*i == 0x0A)
         escapedInput += "\\n";
      else if (*i == 0x0B)
         escapedInput += "\\v";
      else if (*i == 0x0C)
         escapedInput += "\\f";
      else if (*i == 0x0D)
         escapedInput += "\\r";
      else if (*i == 0x1B)
         escapedInput += "\\e";
      else if ((unsigned char)*i < 0x20) { // Control characters not handled above.
         std::string hexStr = utohexstr(*i);
         escapedInput += "\\x" + std::string(2 - hexStr.size(), '0') + hexStr;
      } else if (*i & 0x80) { // UTF-8 multiple code unit subsequence.
         UTF8Decoded unicodeScalarValue
               = decode_utf8(StringRef(i, input.end() - i));
         if (unicodeScalarValue.second == 0) {
            // Found invalid char.
            SmallString<4> value;
            encode_utf8(0xFFFD, value);
            escapedInput.insert(escapedInput.end(), value.begin(), value.end());
            // FIXME: Error reporting.
            return escapedInput;
         }
         if (unicodeScalarValue.first == 0x85) {
            escapedInput += "\\N";
         } else if (unicodeScalarValue.first == 0xA0) {
            escapedInput += "\\_";
         } else if (unicodeScalarValue.first == 0x2028) {
            escapedInput += "\\L";
         } else if (unicodeScalarValue.first == 0x2029) {
            escapedInput += "\\P";
         } else if (!escapePrintable &&
                    polar::sys::unicode::is_printable(unicodeScalarValue.first)) {
            escapedInput += StringRef(i, unicodeScalarValue.second);
         } else {
            std::string hexStr = utohexstr(unicodeScalarValue.first);
            if (hexStr.size() <= 2) {
               escapedInput += "\\x" + std::string(2 - hexStr.size(), '0') + hexStr;
            } else if (hexStr.size() <= 4) {
               escapedInput += "\\u" + std::string(4 - hexStr.size(), '0') + hexStr;
            } else if (hexStr.size() <= 8) {
               escapedInput += "\\U" + std::string(8 - hexStr.size(), '0') + hexStr;
            }
         }
         i += unicodeScalarValue.second - 1;
      } else {
         escapedInput.push_back(*i);
      }
   }
   return escapedInput;
}

Scanner::Scanner(StringRef input, SourceMgr &sm, bool showColors,
                 std::error_code *errorCode)
   : m_sourceMgr(sm), m_showColors(showColors), m_errorCode(errorCode)
{
   init(MemoryBufferRef(input, "YAML"));
}

Scanner::Scanner(MemoryBufferRef buffer, SourceMgr &m_sourceMgr, bool showColors,
                 std::error_code *errorCode)
   : m_sourceMgr(m_sourceMgr), m_showColors(showColors), m_errorCode(errorCode)
{
   init(buffer);
}

void Scanner::init(MemoryBufferRef buffer)
{
   m_inputBuffer = buffer;
   m_current = m_inputBuffer.getBufferStart();
   m_end = m_inputBuffer.getBufferEnd();
   m_indent = -1;
   m_column = 0;
   m_line = 0;
   m_flowLevel = 0;
   m_isStartOfStream = true;
   m_isSimpleKeyAllowed = true;
   m_failed = false;
   std::unique_ptr<MemoryBuffer> inputBufferOwner =
         MemoryBuffer::getMemBuffer(buffer);
   m_sourceMgr.addNewSourceBuffer(std::move(inputBufferOwner), SMLocation());
}

Token &Scanner::peekNext()
{
   // If the current token is a possible simple key, keep parsing until we
   // can confirm.
   bool needMore = false;
   while (true) {
      if (m_tokenQueue.empty() || needMore) {
         if (!fetchMoreTokens()) {
            m_tokenQueue.clear();
            m_tokenQueue.push_back(Token());
            return m_tokenQueue.front();
         }
      }
      assert(!m_tokenQueue.empty() &&
             "fetchMoreTokens lied about getting tokens!");

      removeStaleSimpleKeyCandidates();
      SimpleKey skey;
      skey.m_token = m_tokenQueue.begin();
      if (!is_contained(m_simpleKeys, skey)) {
         break;
      } else {
         needMore = true;
      }
   }
   return m_tokenQueue.front();
}

Token Scanner::getNext()
{
   Token ret = peekNext();
   // m_tokenQueue can be empty if there was an error getting the next token.
   if (!m_tokenQueue.empty()) {
      m_tokenQueue.pop_front();
   }
   // There cannot be any referenced Token's if the m_tokenQueue is empty. So do a
   // quick deallocation of them all.
   if (m_tokenQueue.empty()) {
      m_tokenQueue.resetAlloc();
   }
   return ret;
}

StringRef::iterator Scanner::skipNbChar(StringRef::iterator position)
{
   if (position == m_end){
      return position;
   }

   // Check 7 bit c-printable - b-char.
   if (*position == 0x09
       || (*position >= 0x20 && *position <= 0x7E)) {
      return position + 1;
   }
   // Check for valid UTF-8.
   if (uint8_t(*position) & 0x80) {
      UTF8Decoded u8d = decodeUtf8(position);
      if (   u8d.second != 0
             && u8d.first != 0xFEFF
             && ( u8d.first == 0x85
                  || ( u8d.first >= 0xA0
                       && u8d.first <= 0xD7FF)
                  || ( u8d.first >= 0xE000
                       && u8d.first <= 0xFFFD)
                  || ( u8d.first >= 0x10000
                       && u8d.first <= 0x10FFFF))) {
         return position + u8d.second;
      }
   }
   return position;
}

StringRef::iterator Scanner::skipBBreak(StringRef::iterator position)
{
   if (position == m_end) {
      return position;
   }

   if (*position == 0x0D) {
      if (position + 1 != m_end && *(position + 1) == 0x0A) {
         return position + 2;
      }
      return position + 1;
   }

   if (*position == 0x0A) {
      return position + 1;
   }
   return position;
}

StringRef::iterator Scanner::skipSSpace(StringRef::iterator position)
{
   if (position == m_end) {
      return position;
   }
   if (*position == ' ') {
      return position + 1;
   }
   return position;
}

StringRef::iterator Scanner::skipSWhite(StringRef::iterator position)
{
   if (position == m_end) {
      return position;
   }
   if (*position == ' ' || *position == '\t') {
      return position + 1;
   }

   return position;
}

StringRef::iterator Scanner::skipNsChar(StringRef::iterator position)
{
   if (position == m_end) {
      return position;
   }
   if (*position == ' ' || *position == '\t') {
      return position;
   }
   return skipNbChar(position);
}

StringRef::iterator Scanner::skipWhile(SkipWhileFunc func, StringRef::iterator position)
{
   while (true) {
      StringRef::iterator i = (this->*func)(position);
      if (i == position) {
         break;
      }
      position = i;
   }
   return position;
}

void Scanner::advanceWhile(SkipWhileFunc func)
{
   auto finalValue = skipWhile(func, m_current);
   m_column += finalValue - m_current;
   m_current = finalValue;
}

static bool is_ns_hex_digit(const char c)
{
   return    (c >= '0' && c <= '9')
         || (c >= 'a' && c <= 'z')
         || (c >= 'A' && c <= 'Z');
}

static bool is_ns_word_char(const char c)
{
   return    c == '-'
         || (c >= 'a' && c <= 'z')
         || (c >= 'A' && c <= 'Z');
}

void Scanner::scanNsUriChar()
{
   while (true) {
      if (m_current == m_end) {
         break;
      }

      if ((*m_current == '%'
           && m_current + 2 < m_end
           && is_ns_hex_digit(*(m_current + 1))
           && is_ns_hex_digit(*(m_current + 2)))
          || is_ns_word_char(*m_current)
          || StringRef(m_current, 1).findFirstOf("#;/?:@&=+$,_.!~*'()[]")
          != StringRef::npos) {
         ++m_current;
         ++m_column;
      } else {
         break;
      }
   }
}

bool Scanner::consume(uint32_t expected)
{
   if (expected >= 0x80) {
      report_fatal_error("Not dealing with this yet");
   }
   if (m_current == m_end) {
      return false;
   }
   if (uint8_t(*m_current) >= 0x80) {
      report_fatal_error("Not dealing with this yet");
   }
   if (uint8_t(*m_current) == expected) {
      ++m_current;
      ++m_column;
      return true;
   }
   return false;
}

void Scanner::skip(uint32_t distance)
{
   m_current += distance;
   m_column += distance;
   assert(m_current <= m_end && "Skipped past the end");
}

bool Scanner::isBlankOrBreak(StringRef::iterator position)
{
   if (position == m_end) {
      return false;
   }
   return *position == ' ' || *position == '\t' || *position == '\r' ||
         *position == '\n';
}

bool Scanner::consumeLineBreakIfPresent()
{
   auto next = skipBBreak(m_current);
   if (next == m_current) {
      return false;
   }
   m_column = 0;
   ++m_line;
   m_current = next;
   return true;
}

void Scanner::saveSimpleKeyCandidate(TokenQueueT::Iterator token,
                                     unsigned atColumn,
                                     bool isRequired)
{
   if (m_isSimpleKeyAllowed) {
      SimpleKey skey;
      skey.m_token = token;
      skey.m_line = m_line;
      skey.m_column = atColumn;
      skey.m_isRequired = isRequired;
      skey.m_flowLevel = m_flowLevel;
      m_simpleKeys.push_back(skey);
   }
}

void Scanner::removeStaleSimpleKeyCandidates()
{
   for (SmallVectorImpl<SimpleKey>::iterator i = m_simpleKeys.begin();
        i != m_simpleKeys.end();) {
      if (i->m_line != m_line || i->m_column + 1024 < m_column) {
         if (i->m_isRequired)
            setError( "Could not find expected : for simple key"
                      , i->m_token->m_range.begin());
         i = m_simpleKeys.erase(i);
      } else {
         ++i;
      }
   }
}

void Scanner::removeSimpleKeyCandidatesOnFlowLevel(unsigned level)
{
   if (!m_simpleKeys.empty() && (m_simpleKeys.end() - 1)->m_flowLevel == level) {
      m_simpleKeys.pop_back();
   }
}

bool Scanner::unrollIndent(int toColumn)
{
   Token token;
   // Indentation is ignored in flow.
   if (m_flowLevel != 0) {
      return true;
   }
   while (m_indent > toColumn) {
      token.m_kind = Token::token_BlockEnd;
      token.m_range = StringRef(m_current, 1);
      m_tokenQueue.push_back(token);
      m_indent = m_indents.popBackValue();
   }
   return true;
}

bool Scanner::rollIndent(int toColumn,
                         Token::TokenKind m_kind,
                         TokenQueueT::Iterator insertPoint)
{
   if (m_flowLevel)
      return true;
   if (m_indent < toColumn) {
      m_indents.push_back(m_indent);
      m_indent = toColumn;

      Token token;
      token.m_kind = m_kind;
      token.m_range = StringRef(m_current, 0);
      m_tokenQueue.insert(insertPoint, token);
   }
   return true;
}

void Scanner::skipComment() {
   if (*m_current != '#')
      return;
   while (true) {
      // This may skip more than one byte, thus m_column is only incremented
      // for code points.
      StringRef::iterator iter = skipNbChar(m_current);
      if (iter == m_current) {
         break;
      }
      m_current = iter;
      ++m_column;
   }
}

void Scanner::scanToNextToken() {
   while (true) {
      while (*m_current == ' ' || *m_current == '\t') {
         skip(1);
      }

      skipComment();

      // Skip EOL.
      StringRef::iterator i = skipBBreak(m_current);
      if (i == m_current)
         break;
      m_current = i;
      ++m_line;
      m_column = 0;
      // New lines may start a simple key.
      if (!m_flowLevel)
         m_isSimpleKeyAllowed = true;
   }
}

bool Scanner::scanStreamStart() {
   m_isStartOfStream = false;

   EncodingInfo EI = get_unicode_encoding(currentInput());

   Token token;
   token.m_kind = Token::token_StreamStart;
   token.m_range = StringRef(m_current, EI.second);
   m_tokenQueue.push_back(token);
   m_current += EI.second;
   return true;
}

bool Scanner::scanStreamEnd() {
   // Force an ending new line if one isn't present.
   if (m_column != 0) {
      m_column = 0;
      ++m_line;
   }

   unrollIndent(-1);
   m_simpleKeys.clear();
   m_isSimpleKeyAllowed = false;

   Token token;
   token.m_kind = Token::token_StreamEnd;
   token.m_range = StringRef(m_current, 0);
   m_tokenQueue.push_back(token);
   return true;
}

bool Scanner::scanDirective() {
   // Reset the indentation level.
   unrollIndent(-1);
   m_simpleKeys.clear();
   m_isSimpleKeyAllowed = false;

   StringRef::iterator start = m_current;
   consume('%');
   StringRef::iterator NameStart = m_current;
   m_current = skipWhile(&Scanner::skipNsChar, m_current);
   StringRef Name(NameStart, m_current - NameStart);
   m_current = skipWhile(&Scanner::skipSWhite, m_current);

   Token token;
   if (Name == "YAML") {
      m_current = skipWhile(&Scanner::skipNsChar, m_current);
      token.m_kind = Token::token_VersionDirective;
      token.m_range = StringRef(start, m_current - start);
      m_tokenQueue.push_back(token);
      return true;
   } else if(Name == "TAG") {
      m_current = skipWhile(&Scanner::skipNsChar, m_current);
      m_current = skipWhile(&Scanner::skipSWhite, m_current);
      m_current = skipWhile(&Scanner::skipNsChar, m_current);
      token.m_kind = Token::token_TagDirective;
      token.m_range = StringRef(start, m_current - start);
      m_tokenQueue.push_back(token);
      return true;
   }
   return false;
}

bool Scanner::scanDocumentIndicator(bool isStart) {
   unrollIndent(-1);
   m_simpleKeys.clear();
   m_isSimpleKeyAllowed = false;

   Token token;
   token.m_kind = isStart ? Token::token_DocumentStart : Token::token_DocumentEnd;
   token.m_range = StringRef(m_current, 3);
   skip(3);
   m_tokenQueue.push_back(token);
   return true;
}

bool Scanner::scanFlowCollectionStart(bool isSequence) {
   Token token;
   token.m_kind = isSequence ? Token::token_FlowSequenceStart
                             : Token::token_FlowMappingStart;
   token.m_range = StringRef(m_current, 1);
   skip(1);
   m_tokenQueue.push_back(token);

   // [ and { may begin a simple key.
   saveSimpleKeyCandidate(--m_tokenQueue.end(), m_column - 1, false);

   // And may also be followed by a simple key.
   m_isSimpleKeyAllowed = true;
   ++m_flowLevel;
   return true;
}

bool Scanner::scanFlowCollectionEnd(bool isSequence) {
   removeSimpleKeyCandidatesOnFlowLevel(m_flowLevel);
   m_isSimpleKeyAllowed = false;
   Token token;
   token.m_kind = isSequence ? Token::token_FlowSequenceEnd
                             : Token::token_FlowMappingEnd;
   token.m_range = StringRef(m_current, 1);
   skip(1);
   m_tokenQueue.push_back(token);
   if (m_flowLevel)
      --m_flowLevel;
   return true;
}

bool Scanner::scanFlowEntry() {
   removeSimpleKeyCandidatesOnFlowLevel(m_flowLevel);
   m_isSimpleKeyAllowed = true;
   Token token;
   token.m_kind = Token::token_FlowEntry;
   token.m_range = StringRef(m_current, 1);
   skip(1);
   m_tokenQueue.push_back(token);
   return true;
}

bool Scanner::scanBlockEntry() {
   rollIndent(m_column, Token::token_BlockSequenceStart, m_tokenQueue.end());
   removeSimpleKeyCandidatesOnFlowLevel(m_flowLevel);
   m_isSimpleKeyAllowed = true;
   Token token;
   token.m_kind = Token::token_BlockEntry;
   token.m_range = StringRef(m_current, 1);
   skip(1);
   m_tokenQueue.push_back(token);
   return true;
}

bool Scanner::scanKey() {
   if (!m_flowLevel)
      rollIndent(m_column, Token::token_BlockMappingStart, m_tokenQueue.end());

   removeSimpleKeyCandidatesOnFlowLevel(m_flowLevel);
   m_isSimpleKeyAllowed = !m_flowLevel;

   Token token;
   token.m_kind = Token::token_m_key;
   token.m_range = StringRef(m_current, 1);
   skip(1);
   m_tokenQueue.push_back(token);
   return true;
}

bool Scanner::scanValue()
{
   // If the previous token could have been a simple key, insert the key token
   // into the token queue.
   if (!m_simpleKeys.empty()) {
      SimpleKey skey = m_simpleKeys.popBackValue();
      Token token;
      token.m_kind = Token::token_m_key;
      token.m_range = skey.m_token->m_range;
      TokenQueueT::Iterator i, e;
      for (i = m_tokenQueue.begin(), e = m_tokenQueue.end(); i != e; ++i) {
         if (i == skey.m_token) {
            break;
         }
      }
      assert(i != e && "SimpleKey not in token queue!");
      i = m_tokenQueue.insert(i, token);

      // We may also need to add a Block-Mapping-start token.
      rollIndent(skey.m_column, Token::token_BlockMappingStart, i);

      m_isSimpleKeyAllowed = false;
   } else {
      if (!m_flowLevel)
         rollIndent(m_column, Token::token_BlockMappingStart, m_tokenQueue.end());
      m_isSimpleKeyAllowed = !m_flowLevel;
   }

   Token token;
   token.m_kind = Token::token_Value;
   token.m_range = StringRef(m_current, 1);
   skip(1);
   m_tokenQueue.push_back(token);
   return true;
}

// Forbidding inlining improves performance by roughly 20%.
// FIXME: Remove once llvm optimizes this to the faster version without hints.
POLAR_ATTRIBUTE_NOINLINE static bool
wasEscaped(StringRef::iterator first, StringRef::iterator position);

// Returns whether a character at 'position' was escaped with a leading '\'.
// 'first' specifies the position of the first character in the string.
static bool wasEscaped(StringRef::iterator first,
                       StringRef::iterator position)
{
   assert(position - 1 >= first);
   StringRef::iterator iter = position - 1;
   // We calculate the number of consecutive '\'s before the current position
   // by iterating backwards through our string.
   while (iter >= first && *iter == '\\') --iter;
   // (position - 1 - iter) now contains the number of '\'s before the current
   // position. If it is odd, the character at 'position' was escaped.
   return (position - 1 - iter) % 2 == 1;
}

bool Scanner::scanFlowScalar(bool isDoubleQuoted)
{
   StringRef::iterator start = m_current;
   unsigned colStart = m_column;
   if (isDoubleQuoted) {
      do {
         ++m_current;
         while (m_current != m_end && *m_current != '"') {
            ++m_current;
         }
         // Repeat until the previous character was not a '\' or was an escaped
         // backslash.
      } while (m_current != m_end
               && *(m_current - 1) == '\\'
               && wasEscaped(start + 1, m_current));
   } else {
      skip(1);
      while (true) {
         // Skip a ' followed by another '.
         if (m_current + 1 < m_end && *m_current == '\'' && *(m_current + 1) == '\'') {
            skip(2);
            continue;
         } else if (*m_current == '\'')
            break;
         StringRef::iterator i = skipNbChar(m_current);
         if (i == m_current) {
            i = skipBBreak(m_current);
            if (i == m_current) {
               break;
            }
            m_current = i;
            m_column = 0;
            ++m_line;
         } else {
            if (i == m_end) {
               break;
            }
            m_current = i;
            ++m_column;
         }
      }
   }

   if (m_current == m_end) {
      setError("expected quote at end of scalar", m_current);
      return false;
   }

   skip(1); // Skip ending quote.
   Token token;
   token.m_kind = Token::token_Scalar;
   token.m_range = StringRef(start, m_current - start);
   m_tokenQueue.push_back(token);
   saveSimpleKeyCandidate(--m_tokenQueue.end(), colStart, false);
   m_isSimpleKeyAllowed = false;
   return true;
}

bool Scanner::scanPlainScalar()
{
   StringRef::iterator start = m_current;
   unsigned colStart = m_column;
   unsigned leadingBlanks = 0;
   assert(m_indent >= -1 && "m_indent must be >= -1 !");
   unsigned indent = static_cast<unsigned>(m_indent + 1);
   while (true) {
      if (*m_current == '#')
         break;

      while (!isBlankOrBreak(m_current)) {
         if (  m_flowLevel && *m_current == ':'
               && !(isBlankOrBreak(m_current + 1) || *(m_current + 1) == ',')) {
            setError("Found unexpected ':' while scanning a plain scalar", m_current);
            return false;
         }

         // Check for the end of the plain scalar.
         if (  (*m_current == ':' && isBlankOrBreak(m_current + 1))
               || (  m_flowLevel
                     && (StringRef(m_current, 1).findFirstOf(",:?[]{}")
                         != StringRef::npos))) {
            break;
         }

         StringRef::iterator i = skipNbChar(m_current);
         if (i == m_current) {
            break;
         }
         m_current = i;
         ++m_column;
      }

      // Are we at the end?
      if (!isBlankOrBreak(m_current)){
         break;
      }

      // Eat blanks.
      StringRef::iterator temp = m_current;
      while (isBlankOrBreak(temp)) {
         StringRef::iterator i = skipSWhite(temp);
         if (i != temp) {
            if (leadingBlanks && (m_column < indent) && *temp == '\t') {
               setError("Found invalid tab character in indentation", temp);
               return false;
            }
            temp = i;
            ++m_column;
         } else {
            i = skipBBreak(temp);
            if (!leadingBlanks) {
               leadingBlanks = 1;
            }
            temp = i;
            m_column = 0;
            ++m_line;
         }
      }

      if (!m_flowLevel && m_column < indent) {
         break;
      }
      m_current = temp;
   }
   if (start == m_current) {
      setError("Got empty plain scalar", start);
      return false;
   }
   Token token;
   token.m_kind = Token::token_Scalar;
   token.m_range = StringRef(start, m_current - start);
   m_tokenQueue.push_back(token);
   // Plain scalars can be simple keys.
   saveSimpleKeyCandidate(--m_tokenQueue.end(), colStart, false);
   m_isSimpleKeyAllowed = false;

   return true;
}

bool Scanner::scanAliasOrAnchor(bool isAlias)
{
   StringRef::iterator start = m_current;
   unsigned colStart = m_column;
   skip(1);
   while(true) {
      if (   *m_current == '[' || *m_current == ']'
             || *m_current == '{' || *m_current == '}'
             || *m_current == ','
             || *m_current == ':')
         break;
      StringRef::iterator i = skipNsChar(m_current);
      if (i == m_current) {
         break;
      }
      m_current = i;
      ++m_column;
   }

   if (start == m_current) {
      setError("Got empty alias or anchor", start);
      return false;
   }

   Token token;
   token.m_kind = isAlias ? Token::token_Alias : Token::token_Anchor;
   token.m_range = StringRef(start, m_current - start);
   m_tokenQueue.push_back(token);

   // Alias and anchors can be simple keys.
   saveSimpleKeyCandidate(--m_tokenQueue.end(), colStart, false);

   m_isSimpleKeyAllowed = false;

   return true;
}

char Scanner::scanBlockChompingIndicator()
{
   char indicator = ' ';
   if (m_current != m_end && (*m_current == '+' || *m_current == '-')) {
      indicator = *m_current;
      skip(1);
   }
   return indicator;
}

/// Get the number of line breaks after chomping.
///
/// Return the number of trailing line breaks to emit, depending on
/// \p chompingIndicator.
static unsigned getChompedLineBreaks(char chompingIndicator,
                                     unsigned lineBreaks, StringRef str)
{
   if (chompingIndicator == '-') {// Strip all line breaks.
      return 0;
   }
   if (chompingIndicator == '+') {// Keep all line breaks.
      return lineBreaks;
   }
   // Clip trailing lines.
   return str.empty() ? 0 : 1;
}

unsigned Scanner::scanBlockIndentationIndicator()
{
   unsigned m_indent = 0;
   if (m_current != m_end && (*m_current >= '1' && *m_current <= '9')) {
      m_indent = unsigned(*m_current - '0');
      skip(1);
   }
   return m_indent;
}

bool Scanner::scanBlockScalarHeader(char &chompingIndicator,
                                    unsigned &indentIndicator, bool &isDone)
{
   auto start = m_current;

   chompingIndicator = scanBlockChompingIndicator();
   indentIndicator = scanBlockIndentationIndicator();
   // Check for the chomping indicator once again.
   if (chompingIndicator == ' ') {
      chompingIndicator = scanBlockChompingIndicator();
   }
   m_current = skipWhile(&Scanner::skipSWhite, m_current);
   skipComment();

   if (m_current == m_end) { // EOF, we have an empty scalar.
      Token token;
      token.m_kind = Token::token_BlockScalar;
      token.m_range = StringRef(start, m_current - start);
      m_tokenQueue.push_back(token);
      isDone = true;
      return true;
   }

   if (!consumeLineBreakIfPresent()) {
      setError("expected a line break after block scalar header", m_current);
      return false;
   }
   return true;
}

bool Scanner::findBlockScalarIndent(unsigned &blockIndent,
                                    unsigned blockExitIndent,
                                    unsigned &lineBreaks, bool &isDone)
{
   unsigned maxAllSpaceLineCharacters = 0;
   StringRef::iterator longestAllSpaceLine;

   while (true) {
      advanceWhile(&Scanner::skipSSpace);
      if (skipNbChar(m_current) != m_current) {
         // This line isn't empty, so try and find the indentation.
         if (m_column <= blockExitIndent) { // m_end of the block literal.
            isDone = true;
            return true;
         }
         // We found the block's indentation.
         blockIndent = m_column;
         if (maxAllSpaceLineCharacters > blockIndent) {
            setError(
                     "Leading all-spaces line must be smaller than the block indent",
                     longestAllSpaceLine);
            return false;
         }
         return true;
      }
      if (skipBBreak(m_current) != m_current &&
          m_column > maxAllSpaceLineCharacters) {
         // Record the longest all-space line in case it's longer than the
         // discovered block indent.
         maxAllSpaceLineCharacters = m_column;
         longestAllSpaceLine = m_current;
      }

      // Check for EOF.
      if (m_current == m_end) {
         isDone = true;
         return true;
      }

      if (!consumeLineBreakIfPresent()) {
         isDone = true;
         return true;
      }
      ++lineBreaks;
   }
   return true;
}

bool Scanner::scanBlockScalarIndent(unsigned blockIndent,
                                    unsigned blockExitIndent, bool &isDone)
{
   // Skip the indentation.
   while (m_column < blockIndent) {
      auto iter = skipSSpace(m_current);
      if (iter == m_current)
         break;
      m_current = iter;
      ++m_column;
   }

   if (skipNbChar(m_current) == m_current)
      return true;

   if (m_column <= blockExitIndent) { // m_end of the block literal.
      isDone = true;
      return true;
   }

   if (m_column < blockIndent) {
      if (m_current != m_end && *m_current == '#') { // Trailing comment.
         isDone = true;
         return true;
      }
      setError("A text line is less indented than the block scalar", m_current);
      return false;
   }
   return true; // A normal text line.
}

bool Scanner::scanBlockScalar(bool isLiteral)
{
   // Eat '|' or '>'
   assert(*m_current == '|' || *m_current == '>');
   skip(1);

   char chompingIndicator;
   unsigned blockIndent;
   bool isDone = false;
   if (!scanBlockScalarHeader(chompingIndicator, blockIndent, isDone)) {
      return false;
   }

   if (isDone) {
      return true;
   }
   auto start = m_current;
   unsigned blockExitIndent = m_indent < 0 ? 0 : (unsigned)m_indent;
   unsigned lineBreaks = 0;
   if (blockIndent == 0) {
      if (!findBlockScalarIndent(blockIndent, blockExitIndent, lineBreaks,
                                 isDone)) {
         return false;
      }
   }
   // Scan the block's scalars body.
   SmallString<256> str;
   while (!isDone) {
      if (!scanBlockScalarIndent(blockIndent, blockExitIndent, isDone)) {
         return false;
      }
      if (isDone) {
         break;
      }
      // Parse the current line.
      auto lineStart = m_current;
      advanceWhile(&Scanner::skipNbChar);
      if (lineStart != m_current) {
         str.append(lineBreaks, '\n');
         str.append(StringRef(lineStart, m_current - lineStart));
         lineBreaks = 0;
      }
      // Check for EOF.
      if (m_current == m_end) {
         break;
      }
      if (!consumeLineBreakIfPresent()) {
         break;
      }
      ++lineBreaks;
   }

   if (m_current == m_end && !lineBreaks) {
      // Ensure that there is at least one line break before the end of file.
      lineBreaks = 1;
   }

   str.append(getChompedLineBreaks(chompingIndicator, lineBreaks, str), '\n');

   // New lines may start a simple key.
   if (!m_flowLevel)
      m_isSimpleKeyAllowed = true;

   Token token;
   token.m_kind = Token::token_BlockScalar;
   token.m_range = StringRef(start, m_current - start);
   token.m_value = str.getStr().getStr();
   m_tokenQueue.push_back(token);
   return true;
}

bool Scanner::scanTag()
{
   StringRef::iterator start = m_current;
   unsigned colStart = m_column;
   skip(1); // Eat !.
   if (m_current == m_end || isBlankOrBreak(m_current)); // An empty tag.
   else if (*m_current == '<') {
      skip(1);
      scanNsUriChar();
      if (!consume('>')) {
         return false;
      }
   } else {
      // FIXME: Actually parse the c-ns-shorthand-tag rule.
      m_current = skipWhile(&Scanner::skipNsChar, m_current);
   }

   Token token;
   token.m_kind = Token::token_tag;
   token.m_range = StringRef(start, m_current - start);
   m_tokenQueue.push_back(token);

   // Tags can be simple keys.
   saveSimpleKeyCandidate(--m_tokenQueue.end(), colStart, false);
   m_isSimpleKeyAllowed = false;
   return true;
}

bool Scanner::fetchMoreTokens()
{
   if (m_isStartOfStream) {
      return scanStreamStart();
   }
   scanToNextToken();
   if (m_current == m_end) {
      return scanStreamEnd();
   }
   removeStaleSimpleKeyCandidates();
   unrollIndent(m_column);
   if (m_column == 0 && *m_current == '%') {
      return scanDirective();
   }
   if (m_column == 0 && m_current + 4 <= m_end
       && *m_current == '-'
       && *(m_current + 1) == '-'
       && *(m_current + 2) == '-'
       && (m_current + 3 == m_end || isBlankOrBreak(m_current + 3))) {
      return scanDocumentIndicator(true);
   }
   if (m_column == 0 && m_current + 4 <= m_end
       && *m_current == '.'
       && *(m_current + 1) == '.'
       && *(m_current + 2) == '.'
       && (m_current + 3 == m_end || isBlankOrBreak(m_current + 3))) {
      return scanDocumentIndicator(false);
   }

   if (*m_current == '[') {
      return scanFlowCollectionStart(true);
   }
   if (*m_current == '{') {
      return scanFlowCollectionStart(false);
   }
   if (*m_current == ']') {
      return scanFlowCollectionEnd(true);
   }
   if (*m_current == '}') {
      return scanFlowCollectionEnd(false);
   }
   if (*m_current == ',') {
      return scanFlowEntry();
   }
   if (*m_current == '-' && isBlankOrBreak(m_current + 1)) {
      return scanBlockEntry();
   }
   if (*m_current == '?' && (m_flowLevel || isBlankOrBreak(m_current + 1))) {
      return scanKey();
   }
   if (*m_current == ':' && (m_flowLevel || isBlankOrBreak(m_current + 1))) {
      return scanValue();
   }
   if (*m_current == '*') {
      return scanAliasOrAnchor(true);
   }
   if (*m_current == '&') {
      return scanAliasOrAnchor(false);
   }
   if (*m_current == '!') {
      return scanTag();
   }
   if (*m_current == '|' && !m_flowLevel) {
      return scanBlockScalar(true);
   }
   if (*m_current == '>' && !m_flowLevel) {
      return scanBlockScalar(false);
   }
   if (*m_current == '\'') {
      return scanFlowScalar(false);
   }
   if (*m_current == '"') {
      return scanFlowScalar(true);
   }
   // Get a plain scalar.
   StringRef firstChar(m_current, 1);
   if (!(isBlankOrBreak(m_current)
         || firstChar.findFirstOf("-?:,[]{}#&*!|>'\"%@`") != StringRef::npos)
       || (*m_current == '-' && !isBlankOrBreak(m_current + 1))
       || (!m_flowLevel && (*m_current == '?' || *m_current == ':')
           && isBlankOrBreak(m_current + 1))
       || (!m_flowLevel && *m_current == ':'
           && m_current + 2 < m_end
           && *(m_current + 1) == ':'
           && !isBlankOrBreak(m_current + 2))) {
      return scanPlainScalar();
   }
   setError("Unrecognized character while tokenizing.");
   return false;
}

Stream::Stream(StringRef input, SourceMgr &sourceMgr, bool showColors,
               std::error_code *errorCode)
   : m_scanner(new Scanner(input, sourceMgr, showColors, errorCode)), m_currentDoc()
{}

Stream::Stream(MemoryBufferRef m_inputBuffer, SourceMgr &sourceMgr, bool showColors,
               std::error_code *errorCode)
   : m_scanner(new Scanner(m_inputBuffer, sourceMgr, showColors, errorCode)), m_currentDoc()
{}

Stream::~Stream()
{}

bool Stream::failed()
{
   return m_scanner->failed();
}

void Stream::printError(Node *node, const Twine &msg)
{
   m_scanner->printError(node->getSourceRange().m_start,
                         SourceMgr::DK_Error,
                         msg,
                         node->getSourceRange());
}

DocumentIterator Stream::begin()
{
   if (m_currentDoc) {
      report_fatal_error("Can only iterate over the stream once");
   }
   // Skip Stream-start.
   m_scanner->getNext();
   m_currentDoc.reset(new Document(*this));
   return DocumentIterator(m_currentDoc);
}

DocumentIterator Stream::end()
{
   return DocumentIterator();
}

void Stream::skip()
{
   for (DocumentIterator i = begin(), e = end(); i != e; ++i) {
      i->skip();
   }
}

Node::Node(unsigned int type, std::unique_ptr<Document> &doc, StringRef str,
           StringRef token)
   : m_doc(doc), m_typeId(type), m_anchor(str), m_tag(token)
{
   SMLocation start = SMLocation::getFromPointer(peekNext().m_range.begin());
   m_sourceRange = SMRange(start, start);
}

std::string Node::getVerbatimTag() const
{
   StringRef m_raw = getRawTag();
   if (!m_raw.empty() && m_raw != "!") {
      std::string ret;
      if (m_raw.findLastOf('!') == 0) {
         ret = m_doc->getTagMap().find("!")->second;
         ret += m_raw.substr(1);
         return ret;
      } else if (m_raw.startsWith("!!")) {
         ret = m_doc->getTagMap().find("!!")->second;
         ret += m_raw.substr(2);
         return ret;
      } else {
         StringRef tagHandle = m_raw.substr(0, m_raw.findLastOf('!') + 1);
         std::map<StringRef, StringRef>::const_iterator It =
               m_doc->getTagMap().find(tagHandle);
         if (It != m_doc->getTagMap().end())
            ret = It->second;
         else {
            Token token;
            token.m_kind = Token::token_tag;
            token.m_range = tagHandle;
            setError(Twine("Unknown tag handle ") + tagHandle, token);
         }
         ret += m_raw.substr(m_raw.findLastOf('!') + 1);
         return ret;
      }
   }

   switch (getType()) {
   case NK_Null:
      return "tag:yaml.org,2002:null";
   case NK_Scalar:
   case NK_BlockScalar:
      // TODO: tag resolution.
      return "tag:yaml.org,2002:str";
   case NK_Mapping:
      return "tag:yaml.org,2002:map";
   case NK_Sequence:
      return "tag:yaml.org,2002:seq";
   }

   return "";
}

Token &Node::peekNext()
{
   return m_doc->peekNext();
}

Token Node::getNext()
{
   return m_doc->getNext();
}

Node *Node::parseBlockNode()
{
   return m_doc->parseBlockNode();
}

BumpPtrAllocator &Node::getAllocator()
{
   return m_doc->m_nodeAllocator;
}

void Node::setError(const Twine &Msg, Token &m_token) const {
   m_doc->setError(Msg, m_token);
}

bool Node::failed() const {
   return m_doc->failed();
}

StringRef ScalarNode::getValue(SmallVectorImpl<char> &storage) const
{
   // TODO: Handle newlines properly. We need to remove leading whitespace.
   if (m_value[0] == '"') { // Double quoted.
      // Pull off the leading and trailing "s.
      StringRef unquotedValue = m_value.substr(1, m_value.getSize() - 2);
      // Search for characters that would require unescaping the value.
      StringRef::size_type i = unquotedValue.findFirstOf("\\\r\n");
      if (i != StringRef::npos) {
         return unescapeDoubleQuoted(unquotedValue, i, storage);
      }
      return unquotedValue;
   } else if (m_value[0] == '\'') { // Single quoted.
      // Pull off the leading and trailing 's.
      StringRef unquotedValue = m_value.substr(1, m_value.getSize() - 2);
      StringRef::size_type i = unquotedValue.find('\'');
      if (i != StringRef::npos) {
         // We're going to need storage.
         storage.clear();
         storage.reserve(unquotedValue.getSize());
         for (; i != StringRef::npos; i = unquotedValue.find('\'')) {
            StringRef valid(unquotedValue.begin(), i);
            storage.insert(storage.end(), valid.begin(), valid.end());
            storage.push_back('\'');
            unquotedValue = unquotedValue.substr(i + 2);
         }
         storage.insert(storage.end(), unquotedValue.begin(), unquotedValue.end());
         return StringRef(storage.begin(), storage.getSize());
      }
      return unquotedValue;
   }
   // Plain or block.
   return m_value.rtrim(' ');
}

StringRef ScalarNode::unescapeDoubleQuoted( StringRef unquotedValue,
                                            StringRef::size_type i,
                                            SmallVectorImpl<char> &storage) const
{
   // Use storage to build proper value.
   storage.clear();
   storage.reserve(unquotedValue.getSize());
   for (; i != StringRef::npos; i = unquotedValue.findFirstOf("\\\r\n")) {
      // Insert all previous chars into storage.
      StringRef valid(unquotedValue.begin(), i);
      storage.insert(storage.end(), valid.begin(), valid.end());
      // Chop off inserted chars.
      unquotedValue = unquotedValue.substr(i);
      assert(!unquotedValue.empty() && "Can't be empty!");
      // Parse escape or line break.
      switch (unquotedValue[0]) {
      case '\r':
      case '\n':
         storage.push_back('\n');
         if (unquotedValue.getSize() > 1
             && (unquotedValue[1] == '\r' || unquotedValue[1] == '\n')) {
            unquotedValue = unquotedValue.substr(1);
         }
         unquotedValue = unquotedValue.substr(1);
         break;
      default:
         if (unquotedValue.getSize() == 1) {
            // TODO: Report error.
            break;
         }
         unquotedValue = unquotedValue.substr(1);
         switch (unquotedValue[0]) {
         default: {
            Token token;
            token.m_range = StringRef(unquotedValue.begin(), 1);
            setError("Unrecognized escape code!", token);
            return "";
         }
         case '\r':
         case '\n':
            // Remove the new line.
            if (unquotedValue.getSize() > 1
                && (unquotedValue[1] == '\r' || unquotedValue[1] == '\n')) {
               unquotedValue = unquotedValue.substr(1);
            }
            // If this was just a single byte newline, it will get skipped
            // below.
            break;
         case '0':
            storage.push_back(0x00);
            break;
         case 'a':
            storage.push_back(0x07);
            break;
         case 'b':
            storage.push_back(0x08);
            break;
         case 't':
         case 0x09:
            storage.push_back(0x09);
            break;
         case 'n':
            storage.push_back(0x0A);
            break;
         case 'v':
            storage.push_back(0x0B);
            break;
         case 'f':
            storage.push_back(0x0C);
            break;
         case 'r':
            storage.push_back(0x0D);
            break;
         case 'e':
            storage.push_back(0x1B);
            break;
         case ' ':
            storage.push_back(0x20);
            break;
         case '"':
            storage.push_back(0x22);
            break;
         case '/':
            storage.push_back(0x2F);
            break;
         case '\\':
            storage.push_back(0x5C);
            break;
         case 'N':
            encode_utf8(0x85, storage);
            break;
         case '_':
            encode_utf8(0xA0, storage);
            break;
         case 'L':
            encode_utf8(0x2028, storage);
            break;
         case 'P':
            encode_utf8(0x2029, storage);
            break;
         case 'x': {
            if (unquotedValue.getSize() < 3) {
               // TODO: Report error.
               break;
            }

            unsigned int unicodeScalarValue;
            if (unquotedValue.substr(1, 2).getAsInteger(16, unicodeScalarValue)) {
               // TODO: Report error.
               unicodeScalarValue = 0xFFFD;
            }
            encode_utf8(unicodeScalarValue, storage);
            unquotedValue = unquotedValue.substr(2);
            break;
         }
         case 'u': {
            if (unquotedValue.getSize() < 5) {
               // TODO: Report error.
               break;
            }
            unsigned int unicodeScalarValue;
            if (unquotedValue.substr(1, 4).getAsInteger(16, unicodeScalarValue)) {
               // TODO: Report error.
               unicodeScalarValue = 0xFFFD;
            }
            encode_utf8(unicodeScalarValue, storage);
            unquotedValue = unquotedValue.substr(4);
            break;
         }
         case 'U': {
            if (unquotedValue.getSize() < 9)
               // TODO: Report error.
               break;
            unsigned int unicodeScalarValue;
            if (unquotedValue.substr(1, 8).getAsInteger(16, unicodeScalarValue)) {
               // TODO: Report error.
               unicodeScalarValue = 0xFFFD;
            }
            encode_utf8(unicodeScalarValue, storage);
            unquotedValue = unquotedValue.substr(8);
            break;
         }
         }
         unquotedValue = unquotedValue.substr(1);
      }
   }
   storage.insert(storage.end(), unquotedValue.begin(), unquotedValue.end());
   return StringRef(storage.begin(), storage.getSize());
}

Node *KeyValueNode::getKey()
{
   if (m_key)
      return m_key;
   // Handle implicit null keys.
   {
      Token &t = peekNext();
      if (t.m_kind == Token::token_BlockEnd
          || t.m_kind == Token::token_Value
          || t.m_kind == Token::token_Error) {
         return m_key = new (getAllocator()) NullNode(m_doc);
      }
      if (t.m_kind == Token::token_m_key) {
         getNext(); // skip token_m_key.
      }
   }

   // Handle explicit null keys.
   Token &t = peekNext();
   if (t.m_kind == Token::token_BlockEnd || t.m_kind == Token::token_Value) {
      return m_key = new (getAllocator()) NullNode(m_doc);
   }
   // We've got a normal key.
   return m_key = parseBlockNode();
}

Node *KeyValueNode::getValue()
{
   if (m_value) {
      return m_value;
   }
   getKey()->skip();
   if (failed()) {
      return m_value = new (getAllocator()) NullNode(m_doc);
   }

   // Handle implicit null values.
   {
      Token &t = peekNext();
      if (t.m_kind == Token::token_BlockEnd
          || t.m_kind == Token::token_FlowMappingEnd
          || t.m_kind == Token::token_m_key
          || t.m_kind == Token::token_FlowEntry
          || t.m_kind == Token::token_Error) {
         return m_value = new (getAllocator()) NullNode(m_doc);
      }

      if (t.m_kind != Token::token_Value) {
         setError("Unexpected token in m_key m_value.", t);
         return m_value = new (getAllocator()) NullNode(m_doc);
      }
      getNext(); // skip token_Value.
   }

   // Handle explicit null values.
   Token &t = peekNext();
   if (t.m_kind == Token::token_BlockEnd || t.m_kind == Token::token_m_key) {
      return m_value = new (getAllocator()) NullNode(m_doc);
   }

   // We got a normal value.
   return m_value = parseBlockNode();
}

void MappingNode::increment()
{
   if (failed()) {
      m_isAtEnd = true;
      m_currentEntry = nullptr;
      return;
   }
   if (m_currentEntry) {
      m_currentEntry->skip();
      if (m_type == MT_Inline) {
         m_isAtEnd = true;
         m_currentEntry = nullptr;
         return;
      }
   }
   Token token = peekNext();
   if (token.m_kind == Token::token_m_key || token.m_kind == Token::token_Scalar) {
      // KeyValueNode eats the token_m_key. That way it can detect null keys.
      m_currentEntry = new (getAllocator()) KeyValueNode(m_doc);
   } else if (m_type == MT_Block) {
      switch (token.m_kind) {
      case Token::token_BlockEnd:
         getNext();
         m_isAtEnd = true;
         m_currentEntry = nullptr;
         break;
      default:
         setError("Unexpected token. expected m_key or Block m_end", token);
         POLAR_FALLTHROUGH;
      case Token::token_Error:
         m_isAtEnd = true;
         m_currentEntry = nullptr;
      }
   } else {
      switch (token.m_kind) {
      case Token::token_FlowEntry:
         // Eat the flow entry and recurse.
         getNext();
         return increment();
      case Token::token_FlowMappingEnd:
         getNext();
         POLAR_FALLTHROUGH;
      case Token::token_Error:
         // Set this to end iterator.
         m_isAtEnd = true;
         m_currentEntry = nullptr;
         break;
      default:
         setError( "Unexpected token. expected m_key, Flow Entry, or Flow "
                   "Mapping m_end."
                   , token);
         m_isAtEnd = true;
         m_currentEntry = nullptr;
      }
   }
}

void SequenceNode::increment()
{
   if (failed()) {
      m_isAtEnd = true;
      m_currentEntry = nullptr;
      return;
   }
   if (m_currentEntry) {
      m_currentEntry->skip();
   }
   Token token = peekNext();
   if (m_seqType == ST_Block) {
      switch (token.m_kind) {
      case Token::token_BlockEntry:
         getNext();
         m_currentEntry = parseBlockNode();
         if (!m_currentEntry) { // An error occurred.
            m_isAtEnd = true;
            m_currentEntry = nullptr;
         }
         break;
      case Token::token_BlockEnd:
         getNext();
         m_isAtEnd = true;
         m_currentEntry = nullptr;
         break;

      default:
         setError( "Unexpected token. expected Block Entry or Block m_end."
                   , token);
         POLAR_FALLTHROUGH;
      case Token::token_Error:
         m_isAtEnd = true;
         m_currentEntry = nullptr;
      }
   } else if (m_seqType == ST_Indentless) {
      switch (token.m_kind) {
      case Token::token_BlockEntry:
         getNext();
         m_currentEntry = parseBlockNode();
         if (!m_currentEntry) { // An error occurred.
            m_isAtEnd = true;
            m_currentEntry = nullptr;
         }
         break;
      default:
      case Token::token_Error:
         m_isAtEnd = true;
         m_currentEntry = nullptr;
      }
   } else if (m_seqType == ST_Flow) {
      switch (token.m_kind) {
      case Token::token_FlowEntry:
         // Eat the flow entry and recurse.
         getNext();
         m_wasPreviousTokenFlowEntry = true;
         return increment();
      case Token::token_FlowSequenceEnd:
         getNext();
         POLAR_FALLTHROUGH;
      case Token::token_Error:
         // Set this to end iterator.
         m_isAtEnd = true;
         m_currentEntry = nullptr;
         break;
      case Token::token_StreamEnd:
      case Token::token_DocumentEnd:
      case Token::token_DocumentStart:
         setError("Could not find closing ]!", token);
         // Set this to end iterator.
         m_isAtEnd = true;
         m_currentEntry = nullptr;
         break;
      default:
         if (!m_wasPreviousTokenFlowEntry) {
            setError("expected , between entries!", token);
            m_isAtEnd = true;
            m_currentEntry = nullptr;
            break;
         }
         // Otherwise it must be a flow entry.
         m_currentEntry = parseBlockNode();
         if (!m_currentEntry) {
            m_isAtEnd = true;
         }
         m_wasPreviousTokenFlowEntry = false;
         break;
      }
   }
}

Document::Document(Stream &stream) : m_stream(stream), m_root(nullptr)
{
   // tag maps starts with two default mappings.
   m_tagMap["!"] = "!";
   m_tagMap["!!"] = "tag:yaml.org,2002:";

   if (parseDirectives()) {
      expectToken(Token::token_DocumentStart);
   }
   Token &token = peekNext();
   if (token.m_kind == Token::token_DocumentStart) {
      getNext();
   }
}

bool Document::skip()
{
   if (m_stream.m_scanner->failed()) {
      return false;
   }
   if (!m_root) {
      getRoot();
   }

   m_root->skip();
   Token &token = peekNext();
   if (token.m_kind == Token::token_StreamEnd) {
      return false;
   }

   if (token.m_kind == Token::token_DocumentEnd) {
      getNext();
      return skip();
   }
   return true;
}

Token &Document::peekNext()
{
   return m_stream.m_scanner->peekNext();
}

Token Document::getNext()
{
   return m_stream.m_scanner->getNext();
}

void Document::setError(const Twine &message, Token &location) const
{
   m_stream.m_scanner->setError(message, location.m_range.begin());
}

bool Document::failed() const
{
   return m_stream.m_scanner->failed();
}

Node *Document::parseBlockNode()
{
   Token token = peekNext();
   // Handle properties.
   Token anchorInfo;
   Token tagInfo;
parse_property:
   switch (token.m_kind) {
   case Token::token_Alias:
      getNext();
      return new (m_nodeAllocator) AliasNode(m_stream.m_currentDoc, token.m_range.substr(1));
   case Token::token_Anchor:
      if (anchorInfo.m_kind == Token::token_Anchor) {
         setError("Already encountered an anchor for this node!", token);
         return nullptr;
      }
      anchorInfo = getNext(); // Consume token_Anchor.
      token = peekNext();
      goto parse_property;
   case Token::token_tag:
      if (tagInfo.m_kind == Token::token_tag) {
         setError("Already encountered a tag for this node!", token);
         return nullptr;
      }
      tagInfo = getNext(); // Consume token_tag.
      token = peekNext();
      goto parse_property;
   default:
      break;
   }

   switch (token.m_kind) {
   case Token::token_BlockEntry:
      // We got an unindented BlockEntry sequence. This is not terminated with
      // a BlockEnd.
      // Don't eat the token_BlockEntry, SequenceNode needs it.
      return new (m_nodeAllocator) SequenceNode(m_stream.m_currentDoc,
                                                anchorInfo.m_range.substr(1),
                                                tagInfo.m_range,
                                                SequenceNode::ST_Indentless);
   case Token::token_BlockSequenceStart:
      getNext();
      return new (m_nodeAllocator)
            SequenceNode(m_stream.m_currentDoc,
                         anchorInfo.m_range.substr(1),
                         tagInfo.m_range,
                         SequenceNode::ST_Block);
   case Token::token_BlockMappingStart:
      getNext();
      return new (m_nodeAllocator)
            MappingNode(m_stream.m_currentDoc,
                        anchorInfo.m_range.substr(1),
                        tagInfo.m_range,
                        MappingNode::MT_Block);
   case Token::token_FlowSequenceStart:
      getNext();
      return new (m_nodeAllocator)
            SequenceNode(m_stream.m_currentDoc,
                         anchorInfo.m_range.substr(1),
                         tagInfo.m_range,
                         SequenceNode::ST_Flow);
   case Token::token_FlowMappingStart:
      getNext();
      return new (m_nodeAllocator)
            MappingNode(m_stream.m_currentDoc,
                        anchorInfo.m_range.substr(1),
                        tagInfo.m_range,
                        MappingNode::MT_Flow);
   case Token::token_Scalar:
      getNext();
      return new (m_nodeAllocator)
            ScalarNode(m_stream.m_currentDoc,
                       anchorInfo.m_range.substr(1),
                       tagInfo.m_range,
                       token.m_range);
   case Token::token_BlockScalar: {
      getNext();
      StringRef nullTerminatedStr(token.m_value.c_str(), token.m_value.length() + 1);
      StringRef strCopy = nullTerminatedStr.copy(m_nodeAllocator).dropBack();
      return new (m_nodeAllocator)
            BlockScalarNode(m_stream.m_currentDoc, anchorInfo.m_range.substr(1),
                            tagInfo.m_range, strCopy, token.m_range);
   }
   case Token::token_m_key:
      // Don't eat the token_m_key, KeyValueNode expects it.
      return new (m_nodeAllocator)
            MappingNode(m_stream.m_currentDoc,
                        anchorInfo.m_range.substr(1),
                        tagInfo.m_range,
                        MappingNode::MT_Inline);
   case Token::token_DocumentStart:
   case Token::token_DocumentEnd:
   case Token::token_StreamEnd:
   default:
      // TODO: Properly handle tags. "[!!str ]" should resolve to !!str "", not
      //       !!null null.
      return new (m_nodeAllocator) NullNode(m_stream.m_currentDoc);
   case Token::token_Error:
      return nullptr;
   }
   polar_unreachable("Control flow shouldn't reach here.");
   return nullptr;
}

bool Document::parseDirectives()
{
   bool isDirective = false;
   while (true) {
      Token token = peekNext();
      if (token.m_kind == Token::token_TagDirective) {
         parseTAGDirective();
         isDirective = true;
      } else if (token.m_kind == Token::token_VersionDirective) {
         parseYAMLDirective();
         isDirective = true;
      } else {
         break;
      }
   }
   return isDirective;
}

void Document::parseYAMLDirective()
{
   getNext(); // Eat %YAML <version>
}

void Document::parseTAGDirective()
{
   Token tag = getNext(); // %TAG <handle> <prefix>
   StringRef token = tag.m_range;
   // Strip %TAG
   token = token.substr(token.findFirstOf(" \t")).ltrim(" \t");
   std::size_t HandleEnd = token.findFirstOf(" \t");
   StringRef tagHandle = token.substr(0, HandleEnd);
   StringRef TagPrefix = token.substr(HandleEnd).ltrim(" \t");
   m_tagMap[tagHandle] = TagPrefix;
}

bool Document::expectToken(int tokenValue)
{
   Token token = getNext();
   if (token.m_kind != tokenValue) {
      setError("Unexpected token", token);
      return false;
   }
   return true;
}

} // yaml
} // polar
