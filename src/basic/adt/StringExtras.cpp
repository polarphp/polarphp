// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/10.

//===-- StringExtras.cpp - Implement the StringExtras header --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the StringExtras.h header
//
//===----------------------------------------------------------------------===//

#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/utils/RawOutStream.h"
#include "polarphp/basic/adt/SmallVector.h"
#include <ostream>

namespace polar {
namespace basic {

namespace {
bool lowercase_compare(const char &lhs, const char &rhs)
{
   return to_lower(lhs) == to_lower(rhs);
}
} // anonymous namespace

/// StrInStrNoCase - Portable version of strcasestr.  Locates the first
/// occurrence of string 's1' in string 's2', ignoring case.  Returns
/// the offset of s2 in s1 or npos if s2 cannot be found.
std::string_view::size_type str_in_str_no_case(std::string_view s1, std::string_view s2)
{
   size_t N = s2.size(), M = s1.size();
   if (N > M) {
      return std::string_view::npos;
   }
   for (size_t i = 0, e = M - N + 1; i != e; ++i) {
      std::string_view cur = s1.substr(i, N);
      if (std::equal(cur.begin(), cur.end(),
                     s2.begin(), s2.end(),
                     lowercase_compare)) {
         return i;
      }
   }
   return std::string_view::npos;
}

/// get_token - This function extracts one token from source, ignoring any
/// leading characters that appear in the Delimiters string, and ending the
/// token at any of the characters that appear in the Delimiters string.  If
/// there are no tokens in the source string, an empty string is returned.
/// The function returns a pair containing the extracted token and the
/// remaining tail string.
std::pair<std::string_view , std::string_view> get_token(std::string_view source,
                                                         std::string_view delimiters)
{
   // Figure out where the token starts.
   std::string_view ::size_type start = source.find_first_not_of(delimiters);
   // Find the next occurrence of the delimiter.
   std::string_view ::size_type end = source.find_first_of(delimiters, start);
   return std::make_pair(source.substr(start, end - start), source.substr(end));
}

/// SplitString - Split up the specified string according to the specified
/// delimiters, appending the result fragments to the output list.
void split_string(std::string_view  source,
                  std::vector<std::string_view> &outFragments,
                  std::string_view  delimiters)
{
   std::pair<std::string_view , std::string_view> str = get_token(source, delimiters);
   while (!str.first.empty()) {
      outFragments.push_back(str.first);
      str = get_token(str.second, delimiters);
   }
}

void print_escaped_string(StringRef name, RawOutStream &out)
{
   for (unsigned i = 0, e = name.size(); i != e; ++i) {
      unsigned char c = name[i];
      if (is_print(c) && c != '\\' && c != '"') {
         out << c;
      } else {
         out << '\\' << hexdigit(c >> 4) << hexdigit(c & 0x0F);
      }
   }
}

void print_html_escaped(StringRef str, RawOutStream &out)
{
   for (char c : str) {
      if (c == '&') {
         out << "&amp;";
      } else if (c == '<') {
         out << "&lt;";
      } else if (c == '>') {
         out << "&gt;";
      } else if (c == '\"') {
         out << "&quot;";
      } else if (c == '\'') {
         out << "&apos;";
      } else {
         out << c;
      }
   }
}

void print_lower_case(StringRef str, RawOutStream &out)
{
   for (const char c : str) {
      out << to_lower(c);
   }
}

namespace {
int ascii_strncasecmp(const char *lhs, const char *rhs, size_t length)
{
   for (size_t index = 0; index < length; ++index) {
      unsigned char lhc = to_lower(lhs[index]);
      unsigned char rhc = to_lower(rhs[index]);
      if (lhc != rhc) {
         return lhc < rhc ? -1 : 1;
      }
   }
   return 0;
}

unsigned get_auto_sense_radix(std::string_view &str) {
   if (str.empty())
      return 10;

   if (string_starts_with(str, "0x") || string_starts_with(str, "0X")) {
      str = str.substr(2);
      return 16;
   }

   if (string_starts_with(str, "0b") || string_starts_with(str, "0B")) {
      str = str.substr(2);
      return 2;
   }

   if (string_starts_with(str, "0o")) {
      str = str.substr(2);
      return 8;
   }

   if (str[0] == '0' && str.size() > 1 && is_digit(str[1])) {
      str = str.substr(1);
      return 8;
   }

   return 10;
}

} // anonymous namespace

bool string_starts_with(std::string_view str, std::string_view prefix)
{
   return str.size() >= prefix.size() &&
         std::memcmp(str.data(), prefix.data(), prefix.size()) == 0;
}

bool string_starts_with(std::string_view str, std::string_view::value_type prefix)
{
   return str.size() >= 1 && str.front() == prefix;
}

bool string_starts_with_lowercase(std::string_view str, std::string_view prefix)
{
   return str.size() >= prefix.size() &&
         ascii_strncasecmp(str.data(), prefix.data(), prefix.size()) == 0;
}

bool string_starts_with_lowercase(std::string_view str, std::string_view::value_type prefix)
{
   return str.size() >= 1 && to_lower(str.front()) == to_lower(prefix);
}

bool string_ends_with(std::string_view str, std::string_view prefix)
{
   return str.size() >= prefix.size() &&
         std::memcmp(str.end() - prefix.size(), prefix.data(), prefix.size()) == 0;
}

bool string_ends_with(std::string_view str, std::string_view::value_type prefix)
{
   return str.size() >= 1 && str.back() == prefix;
}

bool string_ends_with_lowercase(std::string_view str, std::string_view prefix)
{
   return str.size() >= prefix.size() &&
         ascii_strncasecmp(str.end() - prefix.size(), prefix.data(), prefix.size()) == 0;
}

bool string_ends_with_lowercase(std::string_view str, std::string_view::value_type prefix)
{
   return str.size() >= 1 && to_lower(str.back()) == to_lower(prefix);
}

bool string_consume_signed_integer(std::string_view &str, unsigned radix,
                                   long long &result)
{
   unsigned long long ullValue;
   // Handle positive strings first.
   if (str.empty() || str.front() != '-') {
      if (string_consume_unsigned_integer(str, radix, ullValue) ||
          // Check for value so large it overflows a signed value.s
          (long long)ullValue < 0) {
         return true;
      }
      result = ullValue;
      return false;
   }
   // Get the positive part of the value.
   std::string_view str2 = string_drop_front(str, 1);
   if (string_consume_unsigned_integer(str2, radix, ullValue) ||
       // Reject values so large they'd overflow as negative signed, but allow
       // "-0".  This negates the unsigned so that the negative isn't undefined
       // on signed overflow.
       (long long)-ullValue > 0) {
      return true;
   }
   str = str2;
   result = -ullValue;
   return false;
}

bool string_consume_unsigned_integer(std::string_view &str, unsigned radix,
                                     unsigned long long &result)
{
   // Autosense radix if not specified.
   if (radix == 0) {
      radix = get_auto_sense_radix(str);
   }
   // Empty strings (after the radix autosense) are invalid.
   if (str.empty()) {
      return true;
   }
   // Parse all the bytes of the string given this radix.  Watch for overflow.
   std::string_view str2 = str;
   result = 0;
   while (!str2.empty()) {
      unsigned charVal;
      if (str2[0] >= '0' && str2[0] <= '9') {
         charVal = str2[0] - '0';
      } else if (str2[0] >= 'a' && str2[0] <= 'z') {
         charVal = str2[0] - 'a' + 10;
      } else if (str2[0] >= 'A' && str2[0] <= 'Z') {
         charVal = str2[0] - 'A' + 10;
      } else {
         break;
      }
      // If the parsed value is larger than the integer radix, we cannot
      // consume any more characters.
      if (charVal >= radix) {
         break;
      }
      // Add in this character.
      unsigned long long prevResult = result;
      result = result * radix + charVal;

      // Check for overflow by shifting back and seeing if bits were lost.
      if (result / radix < prevResult) {
         return true;
      }
      str2 = str2.substr(1);
   }

   // We consider the operation a failure if no characters were consumed
   // successfully.
   if (str.size() == str2.size()) {
      return true;
   }
   str = str2;
   return false;
}

/// str_in_str_no_case - Portable version of strcasestr.  Locates the first
/// occurrence of string 's1' in string 's2', ignoring case.  Returns
/// the offset of s2 in s1 or npos if s2 cannot be found.
StringRef::size_type str_in_str_no_case(StringRef lhs, StringRef rhs)
{
   size_t lhsSize = lhs.getSize();
   size_t rhsSize = rhs.getSize();
   if (rhsSize > lhsSize) {
      return StringRef::npos;
   }
   for (size_t i = 0, e = lhsSize - rhsSize + 1; i != e; ++i) {
      if (lhs.substr(i, rhsSize).equalsLower(rhs)) {
         return i;
      }
   }
   return StringRef::npos;
}

/// getToken - This function extracts one token from source, ignoring any
/// leading characters that appear in the Delimiters string, and ending the
/// token at any of the characters that appear in the Delimiters string.  If
/// there are no tokens in the source string, an empty string is returned.
/// The function returns a pair containing the extracted token and the
/// remaining tail string.
std::pair<StringRef, StringRef> get_token(StringRef source,
                                          StringRef delimiters)
{
   // Figure out where the token starts.
   StringRef::size_type start = source.findFirstNotOf(delimiters);
   // Find the next occurrence of the delimiter.
   StringRef::size_type end = source.findFirstOf(delimiters, start);
   return std::make_pair(source.slice(start, end), source.substr(end));
}

/// split_string - Split up the specified string according to the specified
/// delimiters, appending the result fragments to the output list.
void split_string(StringRef source,
                  SmallVectorImpl<StringRef> &outFragments,
                  StringRef delimiters)
{
   std::pair<StringRef, StringRef> pair = get_token(source, delimiters);
   while (!pair.first.empty()) {
      outFragments.push_back(pair.first);
      pair = get_token(pair.second, delimiters);
   }
}

} // basic
} // polar
