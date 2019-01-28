// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/14.

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/ApFloat.h"
#include "polarphp/basic/adt/ApInt.h"
#include "polarphp/basic/adt/Hashing.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/basic/adt/EditDistance.h"
#include <bitset>

namespace polar {
namespace basic {

// MSVC emits references to this into the translation units which reference it.
#ifndef _MSC_VER
const size_t StringRef::npos;
#endif

namespace {

// strncasecmp() is not available on non-POSIX systems, so define an
// alternative function here.
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

} // anonymous namespace

/// compareLower - Compare strings, ignoring case.
int StringRef::compareLower(StringRef other) const
{
   if (int res = ascii_strncasecmp(m_data, other.m_data, std::min(m_length, other.m_length))) {
      return res;
   }
   if (m_length == other.m_length) {
      return 0;
   }
   return m_length < other.m_length ? -1 : 1;
}

/// Check if this string starts with the given \p prefix, ignoring case.
bool StringRef::startsWithLower(StringRef prefix) const
{
   return m_length >= prefix.m_length &&
         ascii_strncasecmp(m_data, prefix.m_data, prefix.m_length) == 0;
}

/// Check if this string ends with the given \p suffix, ignoring case.
bool StringRef::endsWithLower(StringRef suffix) const
{
   return m_length >= suffix.m_length &&
         ascii_strncasecmp(end() - suffix.m_length, suffix.m_data, suffix.m_length) == 0;
}

size_t StringRef::findLower(char c, size_t from) const
{
   char lower = to_lower(c);
   return findIf([lower](char dest) { return to_lower(dest) == lower; }, from);
}

/// compare_numeric - Compare strings, handle embedded numbers.
int StringRef::compareNumeric(StringRef other) const
{
   for (size_t index = 0, end = std::min(m_length, other.m_length); index != end; ++index) {
      // Check for sequences of digits.
      if (is_digit(m_data[index]) && is_digit(other.m_data[index])) {
         // The longer sequence of numbers is considered larger.
         // This doesn't really handle prefixed zeros well.
         size_t j;
         for (j = index + 1; j != end + 1; ++j) {
            bool ld = j < m_length && is_digit(m_data[j]);
            bool rd = j < other.m_length && is_digit(other.m_data[j]);
            if (ld != rd) {
               return rd ? -1 : 1;
            }
            if (!rd) {
               break;
            }
         }
         // The two number sequences have the same length (j-index), just memcmp them.
         if (int res = compareMemory(m_data + index, other.m_data + index, j - index)) {
            return res < 0 ? -1 : 1;
         }
         // Identical number sequences, continue search after the numbers.
         index = j - 1;
         continue;
      }
      if (m_data[index] != other.m_data[index]) {
         return (unsigned char)m_data[index] < (unsigned char)other.m_data[index] ? -1 : 1;
      }
   }
   if (m_length == other.m_length) {
      return 0;
   }
   return m_length < other.m_length ? -1 : 1;
}

// Compute the edit distance between the two given strings.
unsigned StringRef::editDistance(StringRef other,
                                 bool allowReplacements,
                                 unsigned maxEditDistance) const
{
   return compute_edit_distance(
            make_array_ref(getData(), getSize()),
            make_array_ref(other.getData(), other.getSize()),
            allowReplacements, maxEditDistance);
}

//===----------------------------------------------------------------------===//
// String Operations
//===----------------------------------------------------------------------===//

std::string StringRef::toLower() const
{
   std::string result(getSize(), char());
   for (size_type i = 0, end = getSize(); i != end; ++i) {
      result[i] = to_lower(m_data[i]);
   }
   return result;
}

std::string StringRef::toUpper() const
{
   std::string result(getSize(), char());
   for (size_type i = 0, end = getSize(); i != end; ++i) {
      result[i] = to_upper(m_data[i]);
   }
   return result;
}

//===----------------------------------------------------------------------===//
// String Searching
//===----------------------------------------------------------------------===//


/// find - Search for the first string \arg Str in the string.
///
/// \return - The index of the first occurrence of \arg Str, or npos if not
/// found.
size_t StringRef::find(StringRef str, size_t from) const
{
   if (from > m_length) {
      return npos;
   }
   const char *start = m_data + from;
   size_t size = m_length - from;

   const char *needle = str.getData();
   size_t N = str.getSize();
   if (N == 0) {
      return from;
   }
   if (size < N) {
      return npos;
   }
   if (N == 1) {
      const char *ptr = (const char *)::memchr(start, needle[0], size);
      return ptr == nullptr ? npos : ptr - m_data;
   }
   const char *stop = start + (size - N + 1);
   // For short haystacks or unsupported needles fall back to the naive algorithm
   if (size < 16 || N > 255) {
      do {
         if (std::memcmp(start, needle, N) == 0) {
            return start - m_data;
         }
         ++start;
      } while (start < stop);
      return npos;
   }

   // Build the bad char heuristic table, with uint8_t to reduce cache thrashing.
   uint8_t badCharSkip[256];
   std::memset(badCharSkip, N, 256);
   for (unsigned i = 0; i != N-1; ++i) {
      badCharSkip[(uint8_t)str[i]] = N-1-i;
   }
   do {
      uint8_t last = start[N - 1];
      if (POLAR_UNLIKELY(last == (uint8_t)needle[N - 1])) {
         if (std::memcmp(start, needle, N - 1) == 0) {
            return start - m_data;
         }
      }
      // Otherwise skip the appropriate number of bytes.
      start += badCharSkip[last];
   } while (start < stop);

   return npos;
}

size_t StringRef::findLower(StringRef str, size_t from) const
{
   StringRef self = substr(from);
   while (self.getSize() >= str.getSize()) {
      if (self.startsWithLower(str)) {
         return from;
      }
      self = self.dropFront();
      ++from;
   }
   return npos;
}

size_t StringRef::rfindLower(char c, size_t from) const
{
   from = std::min(from, m_length);
   size_t i = from;
   while (i != 0) {
      --i;
      if (to_lower(m_data[i]) == to_lower(c)) {
         return i;
      }
   }
   return npos;
}

/// rfind - Search for the last string \arg Str in the string.
///
/// \return - The index of the last occurrence of \arg Str, or npos if not
/// found.
size_t StringRef::rfind(StringRef str) const
{
   size_t N = str.getSize();
   if (N > m_length)
      return npos;
   for (size_t i = m_length - N + 1, end = 0; i != end;) {
      --i;
      if (substr(i, N).equals(str)) {
         return i;
      }
   }
   return npos;
}

size_t StringRef::rfindLower(StringRef str) const
{
   size_t N = str.getSize();
   if (N > m_length) {
      return npos;
   }
   for (size_t i = m_length - N + 1, end = 0; i != end;) {
      --i;
      if (substr(i, N).equalsLower(str)) {
         return i;
      }
   }
   return npos;
}

/// find_first_of - Find the first character in the string that is in \arg
/// Chars, or npos if not found.
///
/// Note: O(size() + Chars.size())
StringRef::size_type StringRef::findFirstOf(StringRef chars,
                                            size_t from) const
{
   std::bitset<1 << CHAR_BIT> charBits;
   for (size_type i = 0; i != chars.getSize(); ++i) {
      charBits.set((unsigned char)chars[i]);
   }

   for (size_type i = std::min(from, m_length), end = m_length; i != end; ++i) {
      if (charBits.test((unsigned char)m_data[i])) {
         return i;
      }
   }
   return npos;
}

/// find_first_not_of - Find the first character in the string that is not
/// \arg C or npos if not found.
StringRef::size_type StringRef::findFirstNotOf(char c, size_t from) const
{
   for (size_type i = std::min(from, m_length), end = m_length; i != end; ++i) {
      if (m_data[i] != c) {
         return i;
      }
   }
   return npos;
}

/// find_first_not_of - Find the first character in the string that is not
/// in the string \arg Chars, or npos if not found.
///
/// Note: O(size() + Chars.size())
StringRef::size_type StringRef::findFirstNotOf(StringRef chars,
                                               size_t from) const
{
   std::bitset<1 << CHAR_BIT> charBits;
   for (size_type i = 0; i != chars.getSize(); ++i) {
      charBits.set((unsigned char)chars[i]);
   }
   for (size_type i = std::min(from, m_length), end = m_length; i != end; ++i) {
      if (!charBits.test((unsigned char)m_data[i])) {
         return i;
      }
   }
   return npos;
}

/// find_last_of - Find the last character in the string that is in \arg C,
/// or npos if not found.
///
/// Note: O(size() + Chars.size())
StringRef::size_type StringRef::findLastOf(StringRef chars,
                                           size_t from) const
{
   std::bitset<1 << CHAR_BIT> charBits;
   for (size_type i = 0; i != chars.getSize(); ++i) {
      charBits.set((unsigned char)chars[i]);
   }
   for (size_type i = std::min(from, m_length) - 1, end = -1; i != end; --i) {
      if (charBits.test((unsigned char)m_data[i])) {
         return i;
      }
   }
   return npos;
}

/// findLastNotOf - Find the last character in the string that is not
/// \arg C, or npos if not found.
StringRef::size_type StringRef::findLastNotOf(char c, size_t from) const
{
   for (size_type i = std::min(from, m_length) - 1, end = -1; i != end; --i) {
      if (m_data[i] != c) {
         return i;
      }
   }
   return npos;
}

/// findLastNotOf - Find the last character in the string that is not in
/// \arg Chars, or npos if not found.
///
/// Note: O(size() + Chars.size())
StringRef::size_type StringRef::findLastNotOf(StringRef chars,
                                              size_t from) const
{
   std::bitset<1 << CHAR_BIT> charBits;
   for (size_type i = 0, end = chars.getSize(); i != end; ++i) {
      charBits.set((unsigned char)chars[i]);
   }
   for (size_type i = std::min(from, m_length) - 1, end = -1; i != end; --i) {
      if (!charBits.test((unsigned char)m_data[i])) {
         return i;
      }
   }
   return npos;
}

void StringRef::split(SmallVectorImpl<StringRef> &array,
                      StringRef separator, int maxSplit,
                      bool keepEmpty) const
{
   StringRef str = *this;

   // count down from maxSplit. When maxSplit is -1, this will just split
   // "forever". This doesn't support splitting more than 2^31 times
   // intentionally; if we ever want that we can make maxSplit a 64-bit integer
   // but that seems unlikely to be useful.
   while (maxSplit-- != 0) {
      size_t idx = str.find(separator);
      if (idx == npos) {
         break;
      }
      // Push this split.
      if (keepEmpty || idx > 0) {
         array.push_back(str.slice(0, idx));
      }
      // Jump forward.
      str = str.slice(idx + separator.getSize(), npos);
   }

   // Push the tail.
   if (keepEmpty || !str.empty()) {
      array.push_back(str);
   }
}

void StringRef::split(SmallVectorImpl<StringRef> &array, char separator,
                      int maxSplit, bool keepEmpty) const
{
   StringRef str = *this;

   // count down from maxSplit. When maxSplit is -1, this will just split
   // "forever". This doesn't support splitting more than 2^31 times
   // intentionally; if we ever want that we can make maxSplit a 64-bit integer
   // but that seems unlikely to be useful.
   while (maxSplit-- != 0) {
      size_t idx = str.find(separator);
      if (idx == npos) {
         break;
      }
      // Push this split.
      if (keepEmpty || idx > 0) {
         array.push_back(str.slice(0, idx));
      }
      // Jump forward.
      str = str.slice(idx + 1, npos);
   }
   // Push the tail.
   if (keepEmpty || !str.empty()) {
      array.push_back(str);
   }
}

//===----------------------------------------------------------------------===//
// Helpful Algorithms
//===----------------------------------------------------------------------===//

/// count - Return the number of non-overlapped occurrences of \arg Str in
/// the string.
size_t StringRef::count(StringRef str) const
{
   size_t count = 0;
   size_t N = str.getSize();
   if (N > m_length)
      return 0;
   for (size_t i = 0, end = m_length - N + 1; i != end; ++i) {
      if (substr(i, N).equals(str)) {
         ++count;
      }
   }
   return count;
}

namespace {

unsigned get_auto_sense_radix(StringRef &str)
{
   if (str.empty()) {
      return 10;
   }
   if (str.startsWith("0x") || str.startsWith("0X")) {
      str = str.substr(2);
      return 16;
   }
   if (str.startsWith("0b") || str.startsWith("0B")) {
      str = str.substr(2);
      return 2;
   }

   if (str.startsWith("0o")) {
      str = str.substr(2);
      return 8;
   }

   if (str[0] == '0' && str.getSize() > 1 && is_digit(str[1])) {
      str = str.substr(1);
      return 8;
   }
   return 10;
}

} // anonymous namespace

bool consume_unsigned_integer(StringRef &str, unsigned radix,
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
   StringRef str2 = str;
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
      unsigned long long prevresult = result;
      result = result * radix + charVal;

      // Check for overflow by shifting back and seeing if bits were lost.
      if (result / radix < prevresult) {
         return true;
      }
      str2 = str2.substr(1);
   }

   // We consider the operation a failure if no characters were consumed
   // successfully.
   if (str.getSize() == str2.getSize()) {
      return true;
   }

   str = str2;
   return false;
}

bool consume_signed_integer(StringRef &str, unsigned radix,
                            long long &result)
{
   unsigned long long ullvalue;

   // Handle positive strings first.
   if (str.empty() || str.front() != '-') {
      if (consume_unsigned_integer(str, radix, ullvalue) ||
          // Check for value so large it overflows a signed value.
          (long long)ullvalue < 0) {
         return true;
      }
      result = ullvalue;
      return false;
   }

   // Get the positive part of the value.
   StringRef str2 = str.dropFront(1);
   if (consume_unsigned_integer(str2, radix, ullvalue) ||
       // Reject values so large they'd overflow as negative signed, but allow
       // "-0".  This negates the unsigned so that the negative isn't undefined
       // on signed overflow.
       (long long)-ullvalue > 0) {
      return true;
   }
   str = str2;
   result = -ullvalue;
   return false;
}

/// GetAsUnsignedInteger - Workhorse method that converts a integer character
/// sequence of radix up to 36 to an unsigned long long value.
bool get_as_unsigned_integer(StringRef str, unsigned radix,
                             unsigned long long &result)
{
   if (consume_unsigned_integer(str, radix, result)) {
      return true;
   }
   // For getAsUnsignedInteger, we require the whole string to be consumed or
   // else we consider it a failure.
   return !str.empty();
}

bool get_as_signed_integer(StringRef str, unsigned radix,
                           long long &result)
{
   if (consume_signed_integer(str, radix, result)) {
      return true;
   }
   // For getAsSignedInteger, we require the whole string to be consumed or else
   // we consider it a failure.
   return !str.empty();
}

bool StringRef::getAsInteger(unsigned radix, ApInt &result) const
{
   StringRef str = *this;

   // Autosense radix if not specified.
   if (radix == 0) {
      radix = get_auto_sense_radix(str);
   }
   assert(radix > 1 && radix <= 36);
   // Empty strings (after the radix autosense) are invalid.
   if (str.empty()) {
      return true;
   }

   // Skip leading zeroes.  This can be a significant improvement if
   // it means we don't need > 64 bits.
   while (!str.empty() && str.front() == '0') {
      str = str.substr(1);
   }
   // If it was nothing but zeroes....
   if (str.empty()) {
      result = ApInt(64, 0);
      return false;
   }

   // (Over-)estimate the required number of bits.
   unsigned log2Radix = 0;
   while ((1U << log2Radix) < radix) {
      log2Radix++;
   }
   bool isPowerOf2Radix = ((1U << log2Radix) == radix);

   unsigned bitWidth = log2Radix * str.getSize();
   if (bitWidth < result.getBitWidth()) {
      bitWidth = result.getBitWidth(); // don't shrink the result
   } else if (bitWidth > result.getBitWidth()) {
      result = result.zext(bitWidth);
   }
   ApInt radixAP, charAP; // unused unless !isPowerOf2Radix
   if (!isPowerOf2Radix) {
      // These must have the same bit-width as result.
      radixAP = ApInt(bitWidth, radix);
      charAP = ApInt(bitWidth, 0);
   }

   // Parse all the bytes of the string given this radix.
   result = 0;
   while (!str.empty()) {
      unsigned charVal;
      if (str[0] >= '0' && str[0] <= '9') {
         charVal = str[0]-'0';
      } else if (str[0] >= 'a' && str[0] <= 'z') {
         charVal = str[0]-'a'+10;
      } else if (str[0] >= 'A' && str[0] <= 'Z') {
         charVal = str[0]-'A'+10;
      } else {
         return true;
      }
      // If the parsed value is larger than the integer radix, the string is
      // invalid.
      if (charVal >= radix) {
         return true;
      }
      // Add in this character.
      if (isPowerOf2Radix) {
         result <<= log2Radix;
         result |= charVal;
      } else {
         result *= radixAP;
         charAP = charVal;
         result += charAP;
      }

      str = str.substr(1);
   }

   return false;
}

bool StringRef::getAsDouble(double &result, bool allowInexact) const
{
   ApFloat fvalue(0.0);
   ApFloat::OpStatus status =
         fvalue.convertFromString(*this, ApFloat::RoundingMode::rmNearestTiesToEven);
   if (status != ApFloat::opOK) {
      if (!allowInexact || !(status & ApFloat::opInexact)) {
         return true;
      }
   }
   result = fvalue.convertToDouble();
   return false;
}

// Implementation of StringRef hashing.
HashCode hash_value(StringRef str)
{
   return hash_combine_range(str.begin(), str.end());
}

} // basic
} // polar
