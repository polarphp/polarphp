//===--- clang/Basic/CharInfo.h - Classifying ASCII Characters ------------===//
//
//                     The POLAR Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/26.

#ifndef POLARPHP_BASIC_CHARINFO_H
#define POLARPHP_BASIC_CHARINFO_H

#include "polarphp/global/DataTypes.h"
#include "polarphp/global/CompilerFeature.h"
#include "llvm/ADT/StringRef.h"

namespace polar::basic {

using llvm::StringRef;

namespace charinfo {
extern const uint16_t cg_infoTable[256];
enum {
   CHAR_HORZ_WS  = 0x0001,  // '\t', '\f', '\v'.  Note, no '\0'
   CHAR_VERT_WS  = 0x0002,  // '\r', '\n'
   CHAR_SPACE    = 0x0004,  // ' '
   CHAR_DIGIT    = 0x0008,  // 0-9
   CHAR_XLETTER  = 0x0010,  // a-f,A-F
   CHAR_UPPER    = 0x0020,  // A-Z
   CHAR_LOWER    = 0x0040,  // a-z
   CHAR_UNDER    = 0x0080,  // _
   CHAR_PERIOD   = 0x0100,  // .
   CHAR_RAWDEL   = 0x0200,  // {}[]#<>%:;?*+-/^&|~!=,"'
   CHAR_PUNCT    = 0x0400   // `$@()
};

enum {
   CHAR_XUPPER = CHAR_XLETTER | CHAR_UPPER,
   CHAR_XLOWER = CHAR_XLETTER | CHAR_LOWER
};
} // charinfo

/// Returns true if this is a valid first character of a C identifier,
/// which is [a-zA-Z_].
POLAR_READONLY inline bool is_identifier_head(unsigned char c,
                                              bool allowDollar = false)
{
   using namespace charinfo;
   if (cg_infoTable[c] & (CHAR_UPPER|CHAR_LOWER|CHAR_UNDER)) {
      return true;
   }
   return allowDollar && c == '$';
}

/// Returns true if this is a body character of a C identifier,
/// which is [a-zA-Z0-9_].
POLAR_READONLY inline bool is_identifier_body(unsigned char c,
                                              bool allowDollar = false)
{
   using namespace charinfo;
   if (cg_infoTable[c] & (CHAR_UPPER|CHAR_LOWER|CHAR_DIGIT|CHAR_UNDER)) {
      return true;
   }
   return allowDollar && c == '$';
}

/// Returns true if this character is horizontal ASCII whitespace:
/// ' ', '\\t', '\\f', '\\v'.
///
/// Note that this returns false for '\\0'.
POLAR_READONLY inline bool is_horizontal_whitespace(unsigned char c)
{
   using namespace charinfo;
   return (cg_infoTable[c] & (CHAR_HORZ_WS|CHAR_SPACE)) != 0;
}

/// Returns true if this character is vertical ASCII whitespace: '\\n', '\\r'.
///
/// Note that this returns false for '\\0'.
POLAR_READONLY inline bool is_vertical_whitespace(unsigned char c)
{
   using namespace charinfo;
   return (cg_infoTable[c] & CHAR_VERT_WS) != 0;
}

/// Return true if this character is horizontal or vertical ASCII whitespace:
/// ' ', '\\t', '\\f', '\\v', '\\n', '\\r'.
///
/// Note that this returns false for '\\0'.
POLAR_READONLY inline bool is_whitespace(unsigned char c)
{
   using namespace charinfo;
   return (cg_infoTable[c] & (CHAR_HORZ_WS|CHAR_VERT_WS|CHAR_SPACE)) != 0;
}

/// Return true if this character is an ASCII digit: [0-9]
POLAR_READONLY inline bool is_digit(unsigned char c)
{
   using namespace charinfo;
   return (cg_infoTable[c] & CHAR_DIGIT) != 0;
}

/// Return true if this character is a lowercase ASCII letter: [a-z]
POLAR_READONLY inline bool is_lowercase(unsigned char c)
{
   using namespace charinfo;
   return (cg_infoTable[c] & CHAR_LOWER) != 0;
}

/// Return true if this character is an uppercase ASCII letter: [A-Z]
POLAR_READONLY inline bool is_uppercase(unsigned char c)
{
   using namespace charinfo;
   return (cg_infoTable[c] & CHAR_UPPER) != 0;
}

/// Return true if this character is an ASCII letter: [a-zA-Z]
POLAR_READONLY inline bool is_letter(unsigned char c)
{
   using namespace charinfo;
   return (cg_infoTable[c] & (CHAR_UPPER|CHAR_LOWER)) != 0;
}

/// Return true if this character is an ASCII letter or digit: [a-zA-Z0-9]
POLAR_READONLY inline bool is_alphanumeric(unsigned char c)
{
   using namespace charinfo;
   return (cg_infoTable[c] & (CHAR_DIGIT|CHAR_UPPER|CHAR_LOWER)) != 0;
}

/// Return true if this character is an ASCII hex digit: [0-9a-fA-F]
POLAR_READONLY inline bool is_hex_digit(unsigned char c)
{
   using namespace charinfo;
   return (cg_infoTable[c] & (CHAR_DIGIT|CHAR_XLETTER)) != 0;
}

/// Return true if this character is an ASCII punctuation character.
///
/// Note that '_' is both a punctuation character and an identifier character!
POLAR_READONLY inline bool is_punctuation(unsigned char c)
{
   using namespace charinfo;
   return (cg_infoTable[c] & (CHAR_UNDER|CHAR_PERIOD|CHAR_RAWDEL|CHAR_PUNCT)) != 0;
}

/// Return true if this character is an ASCII printable character; that is, a
/// character that should take exactly one column to print in a fixed-width
/// terminal.
POLAR_READONLY inline bool is_printable(unsigned char c)
{
   using namespace charinfo;
   return (cg_infoTable[c] & (CHAR_UPPER|CHAR_LOWER|CHAR_PERIOD|CHAR_PUNCT|
                              CHAR_DIGIT|CHAR_UNDER|CHAR_RAWDEL|CHAR_SPACE)) != 0;
}

/// Return true if this is the body character of a C preprocessing number,
/// which is [a-zA-Z0-9_.].
POLAR_READONLY inline bool is_preprocessing_number_body(unsigned char c)
{
   using namespace charinfo;
   return (cg_infoTable[c] &
           (CHAR_UPPER|CHAR_LOWER|CHAR_DIGIT|CHAR_UNDER|CHAR_PERIOD)) != 0;
}

/// Return true if this is the body character of a C++ raw string delimiter.
POLAR_READONLY inline bool is_raw_string_delim_body(unsigned char c)
{
   using namespace charinfo;
   return (cg_infoTable[c] & (CHAR_UPPER|CHAR_LOWER|CHAR_PERIOD|
                              CHAR_DIGIT|CHAR_UNDER|CHAR_RAWDEL)) != 0;
}


/// Converts the given ASCII character to its lowercase equivalent.
///
/// If the character is not an uppercase character, it is returned as is.
POLAR_READONLY inline char to_lowercase(char c)
{
   if (is_uppercase(c)) {
      return c + 'a' - 'A';
   }
   return c;
}

/// Converts the given ASCII character to its uppercase equivalent.
///
/// If the character is not a lowercase character, it is returned as is.
POLAR_READONLY inline char to_uppercase(char c)
{
   if (is_lowercase(c)) {
      return c + 'A' - 'a';
   }
   return c;
}

/// Return true if this is a valid ASCII identifier.
///
/// Note that this is a very simple check; it does not accept UCNs as valid
/// identifier characters.
POLAR_READONLY inline bool is_valid_identifier(StringRef str,
                                               bool allowDollar = false)
{
   if (str.empty() || !is_identifier_head(str[0], allowDollar)) {
      return false;
   }
   for (StringRef::iterator iter = str.begin(), end = str.end(); iter != end; ++iter) {
      if (!is_identifier_body(*iter, allowDollar)) {
         return false;
      }
   }
   return true;
}

} // polar::basic

#endif // POLARPHP_BASIC_CHARINFO_H
