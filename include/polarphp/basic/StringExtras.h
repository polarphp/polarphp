//===- llvm/ADT/STLExtras.h - Useful STL related functions ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
// Created by polarboy on 2018/10/10.

#ifndef POLARPHP_BASIC_STRING_EXTRAS_H
#define POLARPHP_BASIC_STRING_EXTRAS_H

#include "polarphp/global/CompilerFeature.h"
#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Twine.h"
#include "llvm/ADT/ArrayRef.h"
#include "polarphp/basic/OptionSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/Allocator.h"
#include <iterator>
#include <string>


#include <cassert>
#include <cstring>
#include <string>
#include <utility>
#include <vector>
#include <array>

namespace llvm {
// forward declare with namespace
class raw_ostream;
} // llvm

namespace polar {

using llvm::raw_ostream;
using llvm::StringRef;
using llvm::ArrayRef;
using llvm::Twine;
using llvm::SmallString;
using llvm::SmallVectorImpl;
using llvm::BumpPtrAllocator;

/// hexdigit - Return the hexadecimal character for the
/// given number \p value (which should be less than 16).
inline char hexdigit(unsigned value, bool lowerCase = false)
{
   const char hexChar = lowerCase ? 'a' : 'A';
   return value < 10 ? '0' + value : hexChar + value - 10;
}


/// Construct a string ref from a boolean.
inline StringRef to_string_ref(bool value)
{
   return StringRef(value ? "true" : "false");
}

/// Construct a string ref from an array ref of unsigned chars.
inline StringRef to_string_ref(ArrayRef<uint8_t> value)
{
   return StringRef(reinterpret_cast<const char *>(value.begin()), value.size());
}

/// Construct a string ref from an array ref of unsigned chars.
inline ArrayRef<uint8_t> arrayref_from_stringref(StringRef value)
{
   return {value.bytes_begin(), value.bytes_end()};
}

/// Given an array of c-style strings terminated by a null pointer, construct
/// a vector of std::string_view representing the same strings without the terminating
/// null string.
inline std::vector<std::string_view> to_stringview_array(const char *const *strings)
{
   std::vector<std::string_view> result;
   while (*strings) {
      result.push_back(*strings++);
   }
   return result;
}

/// Construct a string ref from a boolean.
inline std::string_view to_stringview(bool flag)
{
   return std::string_view(flag ? "true" : "false");
}

/// Interpret the given character \p C as a hexadecimal digit and return its
/// value.
///
/// If \p C is not a valid hex digit, -1U is returned.
inline unsigned hex_digit_value(char c)
{
   if (c >= '0' && c <= '9') return c-'0';
   if (c >= 'a' && c <= 'f') return c-'a'+10U;
   if (c >= 'A' && c <= 'F') return c-'A'+10U;
   return -1U;
}

/// Checks if character \p C is one of the 10 decimal digits.
inline bool is_digit(char c)
{
   return c >= '0' && c <= '9';
}

/// Checks if character \p C is a hexadecimal numeric character.
inline bool is_hex_digit(char c)
{
   return hex_digit_value(c) != -1U;
}

/// Checks if character \p C is a valid letter as classified by "C" locale.
inline bool is_alpha(char c)
{
   return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

/// Checks whether character \p C is either a decimal digit or an uppercase or
/// lowercase letter as classified by "C" locale.
inline bool is_alnum(char c)
{
   return is_alpha(c) || is_digit(c);
}

/// Checks whether character \p C is valid ASCII (high bit is zero).
inline bool is_ascii(char c)
{
   return static_cast<unsigned char>(c) <= 127;
}

/// Checks whether all characters in S are ASCII.
inline bool is_ascii(std::string_view str)
{
   for (char c : str) {
      if (POLAR_UNLIKELY(!is_ascii(c))) {
         return false;
      }
   }
   return true;
}

/// Checks whether character \p C is printable.
///
/// Locale-independent version of the C standard library isprint whose results
/// may differ on different platforms.
inline bool is_print(char c)
{
   unsigned char uc = static_cast<unsigned char>(c);
   return (0x20 <= uc) && (uc <= 0x7E);
}

/// Returns the corresponding lowercase character if \p x is uppercase.
inline char to_lower(char c)
{
   if (c >= 'A' && c <= 'Z') {
      return c - 'A' + 'a';
   }
   return c;
}

/// Returns the corresponding uppercase character if \p x is lowercase.
inline char to_upper(char c)
{
   if (c >= 'a' && c <= 'z') {
      return c - 'a' + 'A';
   }
   return c;
}

inline std::string utohexstr(uint64_t c, bool lowerCase = false)
{
   char buffer[17];
   char *bufPtr = std::end(buffer);
   if (c == 0) {
      *--bufPtr = '0';
   }
   while (c) {
      unsigned char mod = static_cast<unsigned char>(c) & 15;
      *--bufPtr = hexdigit(mod, lowerCase);
      c >>= 4;
   }
   return std::string(bufPtr, std::end(buffer));
}

/// Convert buffer \p input to its hexadecimal representation.
/// The returned string is double the size of \p input.
inline std::string to_hex(std::string_view input)
{
   static const char *const lut = "0123456789ABCDEF";
   size_t length = input.size();

   std::string output;
   output.reserve(2 * length);
   for (size_t i = 0; i < length; ++i) {
      const unsigned char c = input[i];
      output.push_back(lut[c >> 4]);
      output.push_back(lut[c & 15]);
   }
   return output;
}

/// Convert buffer \p Input to its hexadecimal representation.
/// The returned string is double the size of \p Input.
inline std::string to_hex(StringRef input, bool lowerCase = false)
{
   static const char *const lut = "0123456789ABCDEF";
   const uint8_t offset = lowerCase ? 32 : 0;
   size_t length = input.size();
   std::string output;
   output.reserve(2 * length);
   for (size_t i = 0; i < length; ++i) {
      const unsigned char c = input[i];
      output.push_back(lut[c >> 4] | offset);
      output.push_back(lut[c & 15] | offset);
   }
   return output;
}

inline std::string to_hex(ArrayRef<uint8_t> input, bool lowerCase = false)
{
   return to_hex(to_string_ref(input), lowerCase);
}

inline uint8_t hex_from_nibbles(char msb, char lsb)
{
   unsigned u1 = hex_digit_value(msb);
   unsigned u2 = hex_digit_value(lsb);
   assert(u1 != -1U && u2 != -1U);

   return static_cast<uint8_t>((u1 << 4) | u2);
}


/// Convert hexadecimal string \p Input to its binary representation.
/// The return string is half the size of \p input.
inline std::string from_hex(StringRef input)
{
   if (input.empty()) {
      return std::string();
   }
   std::string output;
   output.reserve((input.size() + 1) / 2);
   if (input.size() % 2 == 1) {
      output.push_back(hex_from_nibbles('0', input.front()));
      input = input.drop_front();
   }

   assert(input.size() % 2 == 0);
   while (!input.empty()) {
      uint8_t hex = hex_from_nibbles(input[0], input[1]);
      output.push_back(hex);
      input = input.drop_front(2);
   }
   return output;
}

/// \brief Convert the string \p S to an integer of the specified type using
/// the radix \p Base.  If \p Base is 0, auto-detects the radix.
/// Returns true if the number was successfully converted, false otherwise.
template <typename N>
bool to_integer(StringRef str, N &num, unsigned base = 0)
{
   return !str.getAsInteger(base, num);
}

namespace internal {
template <typename N>
inline bool to_float(const Twine &twine, N &num, N (*strToFunc)(const char *, char **))
{
   SmallString<32> storage;
   StringRef str = twine.toNullTerminatedStringRef(storage);
   char *end;
   N temp = strToFunc(str.data(), &end);
   if (*end != '\0') {
      return false;
   }
   num = temp;
   return true;
}
}

inline bool to_float(const Twine &twine, float &num)
{
   return internal::to_float(twine, num, strtof);
}

inline bool to_float(const Twine &twine, double &num)
{
   return internal::to_float(twine, num, strtod);
}

inline bool to_float(const Twine &twine, long double &num)
{
   return internal::to_float(twine, num, strtold);
}

inline std::string utostr(uint64_t value, bool isNeg = false)
{
   char buffer[21];
   char *bufPtr = std::end(buffer);

   if (value == 0) {
      *--bufPtr = '0';  // Handle special case...
   }

   while (value) {
      *--bufPtr = '0' + char(value % 10);
      value /= 10;
   }

   if (isNeg) {
      *--bufPtr = '-';   // Add negative sign...
   }
   return std::string(bufPtr, std::end(buffer));
}

inline std::string itostr(int64_t value)
{
   if (value < 0) {
      return utostr(static_cast<uint64_t>(-value), true);
   } else {
      return utostr(static_cast<uint64_t>(value));
   }
}

/// StrInStrNoCase - Portable version of strcasestr.  Locates the first
/// occurrence of string 's1' in string 's2', ignoring case.  Returns
/// the offset of s2 in s1 or npos if s2 cannot be found.
std::string_view::size_type str_in_str_no_case(std::string_view s1, std::string_view s2);

/// getToken - This function extracts one token from source, ignoring any
/// leading characters that appear in the Delimiters string, and ending the
/// token at any of the characters that appear in the Delimiters string.  If
/// there are no tokens in the source string, an empty string is returned.
/// The function returns a pair containing the extracted token and the
/// remaining tail string.
std::pair<std::string_view, std::string_view> get_token(std::string_view source,
                                                        std::string_view delimiters = " \t\n\v\f\r");

/// StrInStrNoCase - Portable version of strcasestr.  Locates the first
/// occurrence of string 's1' in string 's2', ignoring case.  Returns
/// the offset of s2 in s1 or npos if s2 cannot be found.
StringRef::size_type str_in_str_nocase(StringRef s1, StringRef s2);

/// getToken - This function extracts one token from source, ignoring any
/// leading characters that appear in the Delimiters string, and ending the
/// token at any of the characters that appear in the Delimiters string.  If
/// there are no tokens in the source string, an empty string is returned.
/// The function returns a pair containing the extracted token and the
/// remaining tail string.
std::pair<StringRef, StringRef> get_token(StringRef source,
                                          StringRef delimiters = " \t\n\v\f\r");

/// SplitString - Split up the specified string according to the specified
/// delimiters, appending the result fragments to the output list.
void split_string(std::string_view source,
                  std::vector<std::string_view> &outFragments,
                  std::string_view delimiters = " \t\n\v\f\r");

/// SplitString - Split up the specified string according to the specified
/// delimiters, appending the result fragments to the output list.
void split_string(StringRef source,
                  SmallVectorImpl<StringRef> &outFragments,
                  StringRef delimiters = " \t\n\v\f\r");

/// HashString - Hash function for strings.
///
/// This is the Bernstein hash function.
//
// FIXME: Investigate whether a modified bernstein hash function performs
// better: http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx
//   X*33+c -> X*33^c
inline unsigned hash_string(StringRef str, unsigned result = 0)
{
   for (StringRef::size_type i = 0, e = str.size(); i != e; ++i) {
      result = result * 33 + (unsigned char)str[i];
   }
   return result;
}

/// Returns the English suffix for an ordinal integer (-st, -nd, -rd, -th).
inline StringRef get_ordinal_suffix(unsigned value)
{
   // It is critically important that we do this perfectly for
   // user-written sequences with over 100 elements.
   switch (value % 100) {
      case 11:
      case 12:
      case 13:
         return "th";
      default:
         switch (value % 10) {
            case 1: return "st";
            case 2: return "nd";
            case 3: return "rd";
            default: return "th";
         }
   }
}

/// PrintEscapedString - Print each character of the specified string, escaping
/// it if it is not printable or if it is an escape char.
void print_escaped_string(StringRef name, raw_ostream &out);

/// printLowerCase - Print each character as lowercase if it is uppercase.
void print_lower_case(StringRef string, raw_ostream &out);

namespace internal {

template <typename IteratorType>
inline std::string join_impl(IteratorType begin, IteratorType end,
                             StringRef separator, std::input_iterator_tag) {
   std::string str;
   if (begin == end) {
      return str;
   }
   str += (*begin);
   while (++begin != end) {
      str += separator;
      str += (*begin);
   }
   return str;
}

template <typename IteratorType>
inline std::string join_impl(IteratorType begin, IteratorType end,
                             StringRef separator, std::forward_iterator_tag)
{
   std::string str;
   if (begin == end) {
      return str;
   }
   size_t length = (std::distance(begin, end) - 1) * separator.size();
   for (IteratorType iter = begin; iter != end; ++iter) {
      length += (*begin).size();
   }
   str.reserve(length);
   str += (*begin);
   while (++begin != end) {
      str += separator;
      str += (*begin);
   }
   return str;
}

template <typename Sep>
inline void join_items_impl(std::string &result, Sep separator)
{}

template <typename Sep, typename Arg>
inline void join_items_impl(std::string &result, Sep separator,
                            const Arg &item)
{
   result += item;
}

template <typename Sep, typename Arg1, typename... Args>
inline void join_items_impl(std::string &result, Sep separator, const Arg1 &a1,
                            Args &&... items)
{
   result += a1;
   result += separator;
   join_items_impl(result, separator, std::forward<Args>(items)...);
}

inline size_t join_one_item_size(char character)
{
   return 1;
}

inline size_t join_one_item_size(const char *str)
{
   return str ? ::strlen(str) : 0;
}

template <typename T>
inline size_t join_one_item_size(const T &str)
{
   return str.size();
}

inline size_t join_items_size()
{
   return 0;
}

template <typename T>
inline size_t join_items_size(const T &value)
{
   return join_one_item_size(value);
}

template <typename T, typename... Args>
inline size_t join_items_size(const T &value, Args &&... items)
{
   return join_one_item_size(value) + join_items_size(std::forward<Args>(items)...);
}

} // end namespace internal

/// Joins the strings in the range [begin, end), adding separator between
/// the elements.
template <typename IteratorType>
inline std::string join(IteratorType begin, IteratorType end, StringRef separator)
{
   using tag = typename std::iterator_traits<IteratorType>::iterator_category;
   return internal::join_impl(begin, end, separator, tag());
}

/// Joins the strings in the range [R.begin(), R.end()), adding separator
/// between the elements.
template <typename Range>
inline std::string join(Range &&range, StringRef separator)
{
   return join(range.begin(), range.end(), separator);
}

/// Joins the strings in the parameter pack \p Items, adding \p separator
/// between the elements.  All arguments must be implicitly convertible to
/// std::string, or there should be an overload of std::string::operator+=()
/// that accepts the argument explicitly.
template <typename Sep, typename... Args>
inline std::string join_items(Sep separator, Args &&... items)
{
   std::string result;
   if (sizeof...(items) == 0) {
      return result;
   }
   size_t NS = internal::join_one_item_size(separator);
   size_t NI = internal::join_items_size(std::forward<Args>(items)...);
   result.reserve(NI + (sizeof...(items) - 1) * NS + 1);
   internal::join_items_impl(result, separator, std::forward<Args>(items)...);
   return result;
}


/// Print each character of the specified string, escaping it if it is not
/// printable or if it is an escape char.
void print_escaped_string(StringRef name, raw_ostream &out);

/// Print each character of the specified string, escaping HTML special
/// characters.
void print_html_escaped(StringRef string, raw_ostream &out);

/// print_lower_case - Print each character as lowercase if it is uppercase.
void print_lower_case(StringRef string, raw_ostream &out);

template <typename... ArgTypes>
std::string format_string(const std::string &format, ArgTypes&&...args)
{
   char buffer[512];
   int size = std::snprintf(buffer, 512, format.c_str(), std::forward<ArgTypes>(args)...);
   return std::string(buffer, size);
}

bool string_starts_with(std::string_view str, std::string_view prefix);
bool string_starts_with(std::string_view str, std::string_view::value_type prefix);
bool string_starts_with_lowercase(std::string_view str, std::string_view prefix);
bool string_starts_with_lowercase(std::string_view str, std::string_view::value_type prefix);

bool string_ends_with(std::string_view str, std::string_view prefix);
bool string_ends_with(std::string_view str, std::string_view::value_type prefix);
bool string_ends_with_lowercase(std::string_view str, std::string_view prefix);
bool string_ends_with_lowercase(std::string_view str, std::string_view::value_type prefix);

inline bool string_consume_front(std::string_view &str, std::string_view prefix)
{
   if (!string_starts_with(str, prefix)) {
      return false;
   }
   assert(str.size() > prefix.size() && "Dropping more elements than exist");
   str = str.substr(prefix.size());
   return true;
}

inline bool string_consume_back(std::string_view &str, std::string_view suffix)
{
   if (!string_ends_with(str, suffix)) {
      return false;
   }
   assert(str.size() > suffix.size() && "Dropping more elements than exist");
   str = str.substr(0, str.size() - suffix.size());
   return true;
}

inline std::string_view string_drop_front(std::string_view str, std::size_t size)
{
   assert(str.size() >= size && "Dropping more elements than exist");
   return str.substr(size);
}

inline std::string_view string_drop_back(std::string_view str, std::size_t size)
{
   assert(str.size() >= size && "Dropping more elements than exist");
   return str.substr(0, str.size() - size);
}

bool string_consume_signed_integer(std::string_view &str, unsigned radix,
                                   long long &result);
bool string_consume_unsigned_integer(std::string_view &str, unsigned radix,
                                     unsigned long long &result);

/// Parse the current string as an integer of the specified radix.  If
/// \p Radix is specified as zero, this does radix autosensing using
/// extended C rules: 0 is octal, 0x is hex, 0b is binary.
///
/// If the string does not begin with a number of the specified radix,
/// this returns true to signify the error. The string is considered
/// erroneous if empty or if it overflows T.
/// The portion of the string representing the discovered numeric value
/// is removed from the beginning of the string.
template <typename T>
typename std::enable_if<std::numeric_limits<T>::is_signed, bool>::type
string_consume_integer(std::string_view &str, unsigned radix, T &result)
{
   long long llval;
   if (string_consume_signed_integer(str, radix, llval) ||
       static_cast<long long>(static_cast<T>(llval)) != llval) {
      return true;
   }
   result = llval;
   return false;
}

template <typename T>
typename std::enable_if<!std::numeric_limits<T>::is_signed, bool>::type
string_consume_integer(std::string_view &str, unsigned radix, T &result)
{
   unsigned long long ullval;
   if (string_consume_unsigned_integer(str, radix, ullval) ||
       static_cast<unsigned long long>(static_cast<T>(ullval)) != ullval) {
      return true;
   }
   result = ullval;
   return false;
}

template <typename T,
   typename = std::enable_if_t<std::is_arithmetic_v<T>>>
const T &sprintable(const T &value)
{
   return value;
}

inline const char *sprintable(StringRef str)
{
   return str.data();
}

/// Determine whether the given string can be an argument label.
///
/// \seealso Token::canBeArgumentLabel()
bool canBeArgumentLabel(StringRef identifier);

/// Determine whether the given string can be the name of a member.
bool canBeMemberName(StringRef identifier);

/// Returns true if the given word is one of Swift's known prepositions.
///
/// This can be faster than getPartOfSpeech(StringRef).
bool isPreposition(StringRef word);

/// Describes the part of speech of a particular word.
enum class PartOfSpeech {
   Unknown,
   Preposition,
   Verb,
   Gerund,
};

/// Determine the part of speech for the given word.
PartOfSpeech getPartOfSpeech(StringRef word);

/// Scratch space used for returning a set of StringRefs.
class StringScratchSpace {
   llvm::BumpPtrAllocator Allocator;

public:
   StringRef copyString(StringRef string);

   llvm::BumpPtrAllocator &getAllocator() { return Allocator; }
};

namespace camel_case {
class WordIterator;

/// A bidirectional iterator that walks through the words in a camelCase
/// string.
///
/// Note that this iterator is not technically conforming bidirectional
/// iterator, because it's reference type is not a true reference. But it
/// quacks like a duck.
class WordIterator {
   StringRef String;
   unsigned Position;
   mutable unsigned NextPosition : 31;
   mutable unsigned NextPositionValid : 1;
   mutable unsigned PrevPosition : 31;
   mutable unsigned PrevPositionValid : 1;

   void computeNextPosition() const;

   void computePrevPosition() const;

   /// Proxy used for the arrow operator of the word iterator.
   class ArrowProxy {
      StringRef String;

   public:
      explicit ArrowProxy(StringRef string) : String(string) {}

      const StringRef *operator->() const {
         return &String;
      }
   };

public:
   using value_type = StringRef;
   using reference = StringRef;
   using pointer = ArrowProxy;
   using difference_type = int;
   using iterator_category = std::bidirectional_iterator_tag;

   WordIterator(StringRef string, unsigned position)
      : String(string), Position(position) {
      assert(!string.empty());
      NextPositionValid = false;
      PrevPositionValid = false;
   }

   StringRef operator*() const {
      if (!NextPositionValid)
         computeNextPosition();

      return String.slice(Position, NextPosition);
   }

   ArrowProxy operator->() const {
      return ArrowProxy(**this);
   }

   WordIterator &operator++() {
      if (!NextPositionValid)
         computeNextPosition();

      // Save the previous position.
      PrevPosition = Position;
      PrevPositionValid = true;

      // Move to the next position.
      Position = NextPosition;

      // We don't know what lies ahead.
      NextPositionValid = false;
      return *this;
   }

   WordIterator operator++(int) {
      WordIterator tmp(*this);
      ++(*this);
      return tmp;
   }

   WordIterator &operator--() {
      if (!PrevPositionValid)
         computePrevPosition();

      // Save the next position.
      NextPosition = Position;
      NextPositionValid = true;

      // Move to the previous position.
      Position = PrevPosition;

      // We don't know what lies behind.
      PrevPositionValid = false;

      return *this;
   }

   WordIterator operator--(int) {
      WordIterator tmp(*this);
      --(*this);
      return tmp;
   }

   friend bool operator==(const WordIterator &x, const WordIterator &y) {
      assert(x.String.data() == y.String.data() &&
             x.String.size() == y.String.size() &&
             "comparing word iterators from different strings");
      return x.Position == y.Position;
   }

   friend bool operator!=(const WordIterator &x, const WordIterator &y) {
      return !(x == y);
   }

   /// Retrieve the position of this iterator within the underlying
   /// string.
   unsigned getPosition() const {
      return Position;
   }

   /// Retrieve the string up until this iterator
   StringRef getPriorStr() const {
      return String.slice(0, Position);
   }

   /// Retrieve the rest of the string (including this position)
   StringRef getRestOfStr() const {
      return String.slice(Position, String.size());
   }
};

/// Find the first camelCase word in the given string.
StringRef getFirstWord(StringRef string);

/// Find the last camelCase word in the given string.
StringRef getLastWord(StringRef string);

/// A wrapper that treats a string as a container of camelCase words.
class Words {
   StringRef String;

public:
   using iterator = WordIterator;
   using const_iterator = WordIterator;
   using reverse_iterator = std::reverse_iterator<WordIterator>;
   using const_reverse_iterator = std::reverse_iterator<WordIterator>;

   explicit Words(StringRef string) : String(string) {}

   bool empty() const { return String.empty(); }

   iterator begin() const { return WordIterator(String, 0); }

   iterator end() const { return WordIterator(String, String.size()); }

   reverse_iterator rbegin() const { return reverse_iterator(end()); }

   reverse_iterator rend() const { return reverse_iterator(begin()); }
};

/// Retrieve the camelCase words in the given string.
inline Words getWords(StringRef string) { return Words(string); }

/// Check whether the two words are the same, ignoring the case of the
/// first letter.
bool sameWordIgnoreFirstCase(StringRef word1, StringRef word2);

/// Check whether the first word starts with the second word, ignoring the
/// case of the first letter.
bool startsWithIgnoreFirstCase(StringRef word1, StringRef word2);

/// Lowercase the first word within the given camelCase string.
///
/// \param string The string to lowercase.
/// \param scratch Scratch buffer used to form the resulting string.
///
/// \returns the string with the first word lowercased. When the
/// first word is an acronym, the string will be returned
/// unchanged.
StringRef toLowercaseWord(StringRef string, SmallVectorImpl<char> &scratch);

/// Lowercase the first word within the given camelCase string.
///
/// \param string The string to lowercase.
/// \param scratch Scratch buffer used to form the resulting string.
///
/// \returns the string with the first word lowercased. When the
/// first word is an acronym, the string will be returned
/// unchanged.
StringRef toLowercaseWord(StringRef string, StringScratchSpace &scratch);

/// Lowercase the first word within the given camelCase string.
///
/// \param string The string to lowercase.
/// \param scratch Scratch buffer used to form the resulting string.
///
/// \returns the string with the first word lowercased, including
/// initialisms.
StringRef toLowercaseInitialisms(StringRef string,
                                 StringScratchSpace &scratch);

/// Lowercase the first word within the given camelCase string.
///
/// \param string The string to lowercase.
/// \param scratch Scratch buffer used to form the resulting string.
///
/// \returns the string with the first word lowercased, including
/// initialisms.
StringRef toLowercaseInitialisms(StringRef string,
                                 SmallVectorImpl<char> &scratch);

/// Sentence-case the given camelCase string by turning the first
/// letter into an uppercase letter.
///
/// \param string The string to sentence-case.
/// \param scratch Scratch buffer used to form the resulting string.
///
/// \returns the string in sentence case.
StringRef toSentencecase(StringRef string, SmallVectorImpl<char> &scratch);

/// Drop the class prefix (i..e, a 2-3 character acronym) from the front
/// of the given string.
///
/// \param string The string whose prefix will be dropped.
///
/// \returns the result of dropping the prefix from \p string, or the
/// whole string if it has no prefix.
StringRef dropPrefix(StringRef string);

/// Append the given string to the given buffer, sentence-casing the string
/// so that the result reads as separate camelCase words.
///
/// \param buffer The buffer to append to.
/// \param string The new string to append, which will be sentence-cased.
///
/// \returns the contents of the buffer after appending.
StringRef appendSentenceCase(SmallVectorImpl<char> &buffer,
                             StringRef string);

/// Search the given camelCase string for the first occurrence of
/// the second string as a complete word.
///
/// \param string The string to search.
/// \param word The string to search for; must be a single Title word
/// \returns the index of the start of the match, or String::npos if
///   it was not found
size_t findWord(StringRef string, StringRef word);
} // end namespace camel_case

/// Flags used by \c OmissionTypeName to describe the input type.
enum class OmissionTypeFlags {
   /// Whether the parameter with this type has a default argument.
      DefaultArgument = 0x01,

   /// Whether this parameter is of some Boolean type.
      Boolean = 0x02,

   /// Whether this parameter is of some function/block type.
      Function = 0x04,
};

/// Options that described omitted types.
using OmissionTypeOptions = OptionSet<OmissionTypeFlags>;

/// Describes the name of a type as is used for omitting needless
/// words.
struct OmissionTypeName {
   /// The name of the type.
   StringRef Name;

   /// For a collection type, the name of the element type.
   StringRef CollectionElement;

   /// Options that describe this type.
   OmissionTypeOptions Options;

   /// Construct a type name.
   OmissionTypeName(StringRef name = StringRef(),
                    OmissionTypeOptions options = None,
                    StringRef collectionElement = StringRef())
      : Name(name), CollectionElement(collectionElement),
        Options(options) {}

   /// Construct a type name.
   OmissionTypeName(const char *name, OmissionTypeOptions options = None,
                    StringRef collectionElement = StringRef())
      : Name(name), CollectionElement(collectionElement),
        Options(options) {}

   /// Produce a new type name for omission with a default argument.
   OmissionTypeName withDefaultArgument(bool defaultArgument = true) {
      OmissionTypeName result(*this);
      if (defaultArgument)
         result.Options |= OmissionTypeFlags::DefaultArgument;
      else
         result.Options -= OmissionTypeFlags::DefaultArgument;
      return result;
   }

   /// Determine whether the parameter corresponding to this type has a default
   /// argument.
   bool hasDefaultArgument() const {
      return Options.contains(OmissionTypeFlags::DefaultArgument);
   }

   /// Whether this type is a Boolean type.
   bool isBoolean() const {
      return Options.contains(OmissionTypeFlags::Boolean);
   }

   /// Whether this type is a function/block type.
   bool isFunction() const {
      return Options.contains(OmissionTypeFlags::Function);
   }

   /// Determine whether the type name is empty.
   bool empty() const { return Name.empty(); }

   friend bool operator==(const OmissionTypeName &lhs,
                          const OmissionTypeName &rhs) {
      return lhs.Name == rhs.Name &&
             (lhs.CollectionElement.empty() ||
              rhs.CollectionElement.empty() ||
              lhs.CollectionElement == rhs.CollectionElement);
   }

   friend bool operator!=(const OmissionTypeName &lhs,
                          const OmissionTypeName &rhs) {
      return !(lhs == rhs);
   }
};

/// Match the given type name at the beginning of the given name,
/// returning the remainder of the name.
///
/// For example, matching "stringByAppendingString" to the type "NSString"
/// would produce "ByAppendingString".
StringRef matchLeadingTypeName(StringRef name, OmissionTypeName typeName);

/// Describes a set of names with an inheritance relationship.
class InheritedNameSet {
   const InheritedNameSet *Parent;
   llvm::StringSet<llvm::BumpPtrAllocator &> Names;

public:
   /// Construct a new inherited name set with the given parent.
   InheritedNameSet(const InheritedNameSet *parent,
                    llvm::BumpPtrAllocator &allocator)
      : Parent(parent), Names(allocator) {}

   // Add a new name to the set.
   void add(StringRef name);

   /// Determine whether this set includes the given name.
   bool contains(StringRef name) const;
};

/// Omit needless words for a declaration.
///
/// \param baseName The base name of the declaration. This value may be
/// changed if any words are removed.
///
/// \param argNames The names of the arguments to the function, or empty if
/// the declaration is not a function. The values in this array may be changed if any words are removed.
///
/// \param firstParamName The name of the first parameter.
///
/// \param resultType The name of the result type.
///
/// \param contextType The name of the type of the enclosing context,
/// e.g., the class name.
///
/// \param paramTypes The names of the parameter types for the
/// function, or empty if the declaration is not a function.
///
/// \param returnsSelf Whether the result of the declaration is 'Self'
/// (in Swift) or 'instancetype' (in Objective-C).
///
/// \param isProperty Whether this is the name of a property.
///
/// \param allPropertyNames The set of property names in the enclosing context.
///
/// \param scratch Scratch space that will be used for modifications beyond
/// just chopping names.
///
/// \returns true if any words were omitted, false otherwise.
bool omitNeedlessWords(StringRef &baseName,
                       MutableArrayRef<StringRef> argNames,
                       StringRef firstParamName,
                       OmissionTypeName resultType,
                       OmissionTypeName contextType,
                       ArrayRef<OmissionTypeName> paramTypes,
                       bool returnsSelf,
                       bool isProperty,
                       const InheritedNameSet *allPropertyNames,
                       StringScratchSpace &scratch);
} // polar

#endif // POLARPHP_BASIC_STRING_EXTRAS_H
