// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/07.

//===----------------------------------------------------------------------===//
//
//  This file defines the Token interface.
//
//===----------------------------------------------------------------------===//

#ifndef POLAR_SYNTAX_TOKEN_H
#define POLAR_SYNTAX_TOKEN_H

#include "polarphp/basic/adt/StringRef.h"

namespace polar::syntax {

#define POLAR_DEFAULT_TOKEN_ID -1

using TokenKindType = int;
using polar::basic::StringRef;

/// Token - This structure provides full information about a lexed token.
/// It is not intended to be space efficient, it is intended to return as much
/// information as possible about each returned token.  This is expected to be
/// compressed into a smaller form if memory footprint is important.
///
class Token
{
public:

private:
   /// Kind - The actual flavor of token this is.
   TokenKindType m_kind;

   /// Whether this token is the first token on the line.
   unsigned m_atStartOfLine : 1;

   /// Whether this token is an escaped `identifier` token.
   unsigned m_escapedIdentifier : 1;

   /// Modifiers for string literals
   unsigned m_multilineString : 1;

   /// Length of custom delimiter of "raw" string literals
   unsigned m_customDelimiterLen : 8;

   // Padding bits == 32 - 11;

   /// The length of the comment that precedes the token.
   unsigned m_commentLength;

   /// Text - The actual string covered by the token in the source buffer.
   StringRef m_text;

   StringRef trimComment() const
   {
      assert(hasComment() && "Has no comment to trim.");
      StringRef raw(m_text.begin() - m_commentLength, m_commentLength);
      return raw.trim();
   }

public:
   Token(TokenKindType kind, StringRef text, unsigned commentLength = 0)
      : m_kind(kind),
        m_atStartOfLine(false),
        m_escapedIdentifier(false),
        m_multilineString(false),
        m_customDelimiterLen(0),
        m_commentLength(commentLength),
        m_text(text)
   {}

   Token()
      : Token(POLAR_DEFAULT_TOKEN_ID, {}, 0)
   {}

   TokenKindType getKind() const
   {
      return m_kind;
   }

   void setKind(TokenKindType kind)
   {
      m_kind = kind;
   }

   void clearCommentLegth()
   {
      m_commentLength = 0;
   }

   bool is(TokenKindType kind)
   {
      return m_kind = kind;
   }

   bool isNot(TokenKindType kind)
   {
      return m_kind != kind;
   }

   // Predicates to check to see if the token is any of a list of tokens.
   bool isAny(TokenKindType kind)
   {
      return is(kind);
   }

   template <typename ...T>
   bool isAny(TokenKindType token1, TokenKindType token2, T... tokens) const
   {
      if (is(token1)) {
         return true;
      }
      return isAny(token2, tokens...);
   }

   template <typename ...T>
   bool isNot(TokenKindType token1, T... tokens) const
   {
      return isAny(token1, tokens...);
   }
};

} // polar::parser

#endif // POLAR_SYNTAX_TOKEN_H
