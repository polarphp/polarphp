// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/03.

#include "polarphp/utils/GlobPattern.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/BitVector.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/ErrorCode.h"

namespace polar {
namespace utils {

using polar::basic::BitVector;

namespace {

bool has_wildcard(StringRef str)
{
   return str.findFirstOf("?*[") != StringRef::npos;
}

// Expands character ranges and returns a bitmap.
// For example, "a-cf-hz" is expanded to "abcfghz".
Expected<BitVector> expand(StringRef str, StringRef original)
{
   BitVector bitVector(256, false);
   // Expand X-Y.
   for (;;) {
      if (str.getSize() < 3) {
         break;
      }
      uint8_t start = str[0];
      uint8_t end = str[2];
      // If it doesn't start with something like X-Y,
      // consume the first character and proceed.
      if (str[1] != '-') {
         bitVector[start] = true;
         str = str.substr(1);
         continue;
      }
      // It must be in the form of X-Y.
      // Validate it and then interpret the range.
      if (start > end) {
         return make_error<StringError>("invalid glob pattern: " + original,
                                        ErrorCode::invalid_argument);
      }
      for (int c = start; c <= end; ++c) {
         bitVector[(uint8_t)c] = true;
      }
      str = str.substr(3);
   }
   for (char c : str) {
      bitVector[(uint8_t)c] = true;
   }
   return bitVector;
}

// This is a scanner for the glob pattern.
// A glob pattern token is one of "*", "?", "[<chars>]", "[^<chars>]"
// (which is a negative form of "[<chars>]"), or a non-meta character.
// This function returns the first token in str.
Expected<BitVector> scan(StringRef &str, StringRef original)
{
   switch (str[0]) {
   case '*':
      str = str.substr(1);
      // '*' is represented by an empty bitvector.
      // All other bitvectors are 256-bit long.
      return BitVector();
   case '?':
      str = str.substr(1);
      return BitVector(256, true);
   case '[': {
      size_t end = str.find(']', 1);
      if (end == StringRef::npos) {
         return make_error<StringError>("invalid glob pattern: " + original,
                                        ErrorCode::invalid_argument);
      }

      StringRef chars = str.substr(1, end - 1);
      str = str.substr(end + 1);
      if (chars.startsWith("^")) {
         Expected<BitVector> bitVector = expand(chars.substr(1), original);
         if (!bitVector) {
            return bitVector.takeError();
         }
         return bitVector->flip();
      }
      return expand(chars, original);
   }
   default:
      BitVector bitVector(256, false);
      bitVector[(uint8_t)str[0]] = true;
      str = str.substr(1);
      return bitVector;
   }
}

} // anonymous namespace

Expected<GlobPattern> GlobPattern::create(StringRef str)
{
   GlobPattern pattern;
   // str doesn't contain any metacharacter,
   // so the regular string comparison should work.
   if (!has_wildcard(str)) {
      pattern.m_exact = str;
      return pattern;
   }
   // str is something like "foo*". We can use startsWith().
   if (str.endsWith("*") && !has_wildcard(str.dropBack())) {
      pattern.m_prefix = str.dropBack();
      return pattern;
   }

   // str is something like "*foo". We can use endsWith().
   if (str.startsWith("*") && !has_wildcard(str.dropFront())) {
      pattern.m_suffix = str.dropFront();
      return pattern;
   }

   // Otherwise, we need to do real glob pattern matching.
   // Parse the pattern now.
   StringRef original = str;
   while (!str.empty()) {
      Expected<BitVector> bitVector = scan(str, original);
      if (!bitVector) {
         return bitVector.takeError();
      }
      pattern.m_tokens.push_back(*bitVector);
   }
   return pattern;
}

bool GlobPattern::match(StringRef str) const
{
   if (m_exact) {
      return str == *m_exact;
   }
   if (m_prefix) {
      return str.startsWith(*m_prefix);
   }
   if (m_suffix) {
      return str.endsWith(*m_suffix);
   }
   return matchOne(m_tokens, str);
}

// Runs glob pattern pattern against string str.
bool GlobPattern::matchOne(ArrayRef<BitVector> pattern, StringRef str) const
{
   for (;;) {
      if (pattern.empty()) {
         return str.empty();
      }
      // If pattern[0] is '*', try to match pattern[1..] against all possible
      // tail strings of str to see at least one pattern succeeds.
      if (pattern[0].getSize() == 0) {
         pattern = pattern.slice(1);
         if (pattern.empty()) {
            // Fast path. If a pattern is '*', it matches anything.
            return true;
         }
         for (size_t i = 0, e = str.getSize(); i < e; ++i) {
            if (matchOne(pattern, str.substr(i))) {
               return true;
            }
         }
         return false;
      }

      // If pattern[0] is not '*', it must consume one character.
      if (str.empty() || !pattern[0][(uint8_t)str[0]]) {
         return false;
      }
      pattern = pattern.slice(1);
      str = str.substr(1);
   }
}

} // utils
} // polar
