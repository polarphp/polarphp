// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
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

#include "StringExtras.h"
#include <ostream>

namespace polar {
namespace utils {

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

void print_escaped_string(std::string_view name, std::ostream &out)
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

void print_html_escaped(std::string_view str, std::ostream &out)
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

void print_lower_case(std::string_view str, std::ostream &out)
{
   for (const char c : str) {
      out << to_lower(c);
   }
}

} // utils
} // polar
