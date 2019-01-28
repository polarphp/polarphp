// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/05/27.

#ifndef POLARPHP_BASIC_ADT_STRINGREF_H
#define POLARPHP_BASIC_ADT_STRINGREF_H

#include "polarphp/global/CompilerFeature.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/IteratorRange.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <ostream>

namespace polar {
namespace basic {

class ApInt;
class HashCode;
template <typename T> class SmallVectorImpl;
class StringRef;

/// Helper functions for StringRef::getAsInteger.
bool get_as_unsigned_integer(StringRef str, unsigned radix,
                             unsigned long long &result);

bool get_as_signed_integer(StringRef str, unsigned radix, long long &result);

bool consume_unsigned_integer(StringRef &str, unsigned radix,
                              unsigned long long &result);
bool consume_signed_integer(StringRef &str, unsigned radix, long long &result);

/// StringRef - Represent a constant reference to a string, i.e. a character
/// array and a length, which need not be null terminated.
///
/// This class does not own the string data, it is expected to be used in
/// situations where the character data resides in some other buffer, whose
/// lifetime extends past that of the StringRef. For this reason, it is not in
/// general safe to store a StringRef.
class StringRef
{
public:
   static const size_t npos = ~size_t(0);

   using iterator = const char *;
   using const_iterator = const char *;
   using size_type = size_t;

private:
   /// The start of the string, in an external buffer.
   const char *m_data = nullptr;

   /// The length of the string.
   size_t m_length = 0;

   // Workaround memcmp issue with null pointers (undefined behavior)
   // by providing a specialized version
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   static int compareMemory(const char *lhs, const char *rhs, size_t length)
   {
      if (length == 0) {
         return 0;
      }
      return ::memcmp(lhs, rhs, length);
   }

public:
   /// @name Constructors
   /// @{

   /// Construct an empty string ref.
   /*implicit*/ StringRef() = default;

   /// Disable conversion from nullptr.  This prevents things like
   /// if (S == nullptr)
   StringRef(std::nullptr_t) = delete;

   /// Construct a string ref from a cstring.
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   /*implicit*/ StringRef(const char *str)
      : m_data(str), m_length(str ? ::strlen(str) : 0)
   {}

   /// Construct a string ref from a pointer and length.
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   /*implicit*/ constexpr StringRef(const char *data, size_t length)
      : m_data(data), m_length(length)
   {}

   /// Construct a string ref from an std::string.
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   /*implicit*/ StringRef(const std::string &str)
      : m_data(str.data()), m_length(str.length())
   {}

   static StringRef withNullAsEmpty(const char *data)
   {
      return StringRef(data ? data : "");
   }

   /// @}
   /// @name Iterators
   /// @{

   iterator begin() const
   {
      return m_data;
   }

   iterator end() const
   {
      return m_data + m_length;
   }

   const unsigned char *getBytesBegin() const
   {
      return reinterpret_cast<const unsigned char *>(begin());
   }

   const unsigned char *getBytesEnd() const
   {
      return reinterpret_cast<const unsigned char *>(end());
   }

   IteratorRange<const unsigned char *> getBytes() const
   {
      return make_range(getBytesBegin(), getBytesEnd());
   }

   /// @}
   /// @name String Operations
   /// @{

   /// data - Get a pointer to the start of the string (which may not be null
   /// terminated).
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   const char *getData() const
   {
      return m_data;
   }

   /// empty - Check if the string is empty.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   bool empty() const
   {
      return m_length == 0;
   }

   /// size - Get the string size.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   size_t getSize() const
   {
      return m_length;
   }

   /// size - Get the string size.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   size_t size() const
   {
      return m_length;
   }

   /// front - Get the first character in the string.
   POLAR_NODISCARD
   char getFront() const
   {
      assert(!empty());
      return m_data[0];
   }

   POLAR_NODISCARD
   inline char front() const
   {
      return getFront();
   }

   /// back - Get the last character in the string.
   POLAR_NODISCARD
   char getBack() const
   {
      assert(!empty());
      return m_data[m_length-1];
   }

   POLAR_NODISCARD
   inline char back() const
   {
      return getBack();
   }

   // copy - Allocate copy in Allocator and return StringRef to it.
   template <typename Allocator>
   POLAR_NODISCARD StringRef copy(Allocator &allocator) const
   {
      // Don't request a length 0 copy from the allocator.
      if (empty()) {
         return StringRef();
      }
      char *storage = allocator.template allocate<char>(m_length);
      std::copy(begin(), end(), storage);
      return StringRef(storage, m_length);
   }

   /// equals - Check for string equality, this is more efficient than
   /// compare() when the relative ordering of inequal strings isn't needed.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   bool equals(StringRef other) const
   {
      return (m_length == other.m_length &&
              compareMemory(m_data, other.m_data, other.m_length) == 0);
   }

   /// equals_lower - Check for string equality, ignoring case.
   POLAR_NODISCARD
   bool equalsLower(StringRef other) const
   {
      return m_length == other.m_length && compareLower(other) == 0;
   }

   /// compare - Compare two strings; the result is -1, 0, or 1 if this string
   /// is lexicographically less than, equal to, or greater than the \p RHS.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   int compare(StringRef other) const
   {
      // Check the prefix for a mismatch.
      if (int result = compareMemory(m_data, other.m_data, std::min(m_length, other.m_length))) {
         return result < 0 ? -1 : 1;
      }
      // Otherwise the prefixes match, so we only need to check the lengths.
      if (m_length == other.m_length) {
         return 0;
      }
      return m_length < other.m_length ? -1 : 1;
   }

   /// compareLower - Compare two strings, ignoring case.
   POLAR_NODISCARD
   int compareLower(StringRef other) const;

   /// compareNumeric - Compare two strings, treating sequences of digits as
   /// numbers.
   POLAR_NODISCARD
   int compareNumeric(StringRef other) const;

   /// \brief Determine the edit distance between this string and another
   /// string.
   ///
   /// \param other the string to compare this string against.
   ///
   /// \param allowReplacements whether to allow character
   /// replacements (change one character into another) as a single
   /// operation, rather than as two operations (an insertion and a
   /// removal).
   ///
   /// \param maxEditDistance If non-zero, the maximum edit distance that
   /// this routine is allowed to compute. If the edit distance will exceed
   /// that maximum, returns \c MaxEditDistance+1.
   ///
   /// \returns the minimum number of character insertions, removals,
   /// or (if \p allowReplacements is \c true) replacements needed to
   /// transform one of the given strings into the other. If zero,
   /// the strings are identical.
   POLAR_NODISCARD
   unsigned editDistance(StringRef other, bool allowReplacements = true,
                         unsigned maxEditDistance = 0) const;

   /// str - Get the contents as an std::string.
   POLAR_NODISCARD
   std::string getStr() const
   {
      if (!m_data) {
         return std::string();
      }
      return std::string(m_data, m_length);
   }

   /// @}
   /// @name Operator Overloads
   /// @{

   POLAR_NODISCARD
   char operator[](size_t index) const
   {
      assert(index < m_length && "Invalid index!");
      return m_data[index];
   }

   /// Disallow accidental assignment from a temporary std::string.
   ///
   /// The declaration here is extra complicated so that `stringRef = {}`
   /// and `stringRef = "abc"` continue to select the move assignment operator.
   template <typename T>
   typename std::enable_if<std::is_same<T, std::string>::value,
   StringRef>::type &
   operator=(T &&str) = delete;

   /// @}
   /// @name Type Conversions
   /// @{

   operator std::string() const
   {
      return getStr();
   }

   /// @}
   /// @name String Predicates
   /// @{

   /// Check if this string starts with the given \p Prefix.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   bool startsWith(StringRef prefix) const
   {
      return m_length >= prefix.m_length &&
            compareMemory(m_data, prefix.m_data, prefix.m_length) == 0;
   }

   /// Check if this string starts with the given \p Prefix, ignoring case.
   POLAR_NODISCARD
   bool startsWithLower(StringRef prefix) const;

   /// Check if this string ends with the given \p Suffix.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   bool endsWith(StringRef suffix) const
   {
      return m_length >= suffix.m_length &&
            compareMemory(end() - suffix.m_length, suffix.m_data, suffix.m_length) == 0;
   }

   /// Check if this string ends with the given \p Suffix, ignoring case.
   POLAR_NODISCARD
   bool endsWithLower(StringRef suffix) const;

   /// @}
   /// @name String Searching
   /// @{

   /// Search for the first character \p C in the string.
   ///
   /// \returns The index of the first occurrence of \p C, or npos if not
   /// found.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   size_t find(char character, size_t from = 0) const
   {
      size_t findBegin = std::min(from, m_length);
      if (findBegin < m_length) { // Avoid calling memchr with nullptr.
         // Just forward to memchr, which is faster than a hand-rolled loop.
         if (const void *pos = ::memchr(m_data + findBegin, character, m_length - findBegin)) {
            return static_cast<const char *>(pos) - m_data;
         }
      }
      return npos;
   }

   /// Search for the first character \p C in the string, ignoring case.
   ///
   /// \returns The index of the first occurrence of \p C, or npos if not
   /// found.
   POLAR_NODISCARD
   size_t findLower(char character, size_t from = 0) const;

   /// Search for the first character satisfying the predicate \p F
   ///
   /// \returns The index of the first character satisfying \p F starting from
   /// \p From, or npos if not found.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   size_t findIf(FunctionRef<bool(char)> func, size_t from = 0) const
   {
      StringRef str = dropFront(from);
      while (!str.empty()) {
         if (func(str.getFront())) {
            return getSize() - str.getSize();
         }
         str = str.dropFront();
      }
      return npos;
   }

   /// Search for the first character not satisfying the predicate \p F
   ///
   /// \returns The index of the first character not satisfying \p F starting
   /// from \p From, or npos if not found.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   size_t findIfNot(FunctionRef<bool(char)> func, size_t from = 0) const
   {
      return findIf([func](char c) { return !func(c); }, from);
   }

   /// Search for the first string \p str in the string.
   ///
   /// \returns The index of the first occurrence of \p str, or npos if not
   /// found.
   POLAR_NODISCARD
   size_t find(StringRef str, size_t from = 0) const;

   /// Search for the first string \p str in the string, ignoring case.
   ///
   /// \returns The index of the first occurrence of \p str, or npos if not
   /// found.
   POLAR_NODISCARD
   size_t findLower(StringRef str, size_t from = 0) const;

   /// Search for the last character \p C in the string.
   ///
   /// \returns The index of the last occurrence of \p C, or npos if not
   /// found.
   POLAR_NODISCARD
   size_t rfind(char character, size_t from = npos) const {
      from = std::min(from, m_length);
      size_t i = from;
      while (i != 0) {
         --i;
         if (m_data[i] == character) {
            return i;
         }
      }
      return npos;
   }

   /// Search for the last character \p C in the string, ignoring case.
   ///
   /// \returns The index of the last occurrence of \p C, or npos if not
   /// found.
   POLAR_NODISCARD
   size_t rfindLower(char character, size_t from = npos) const;

   /// Search for the last string \p str in the string.
   ///
   /// \returns The index of the last occurrence of \p str, or npos if not
   /// found.
   POLAR_NODISCARD
   size_t rfind(StringRef str) const;

   /// Search for the last string \p str in the string, ignoring case.
   ///
   /// \returns The index of the last occurrence of \p str, or npos if not
   /// found.
   POLAR_NODISCARD
   size_t rfindLower(StringRef str) const;

   /// Find the first character in the string that is \p C, or npos if not
   /// found. Same as find.
   POLAR_NODISCARD
   size_t findFirstOf(char character, size_t from = 0) const
   {
      return find(character, from);
   }

   /// Find the first character in the string that is in \p Chars, or npos if
   /// not found.
   ///
   /// Complexity: O(size() + Chars.size())
   POLAR_NODISCARD
   size_t findFirstOf(StringRef chars, size_t from = 0) const;

   /// Find the first character in the string that is not \p C or npos if not
   /// found.
   POLAR_NODISCARD
   size_t findFirstNotOf(char character, size_t from = 0) const;

   /// Find the first character in the string that is not in the string
   /// \p Chars, or npos if not found.
   ///
   /// Complexity: O(size() + Chars.size())
   POLAR_NODISCARD
   size_t findFirstNotOf(StringRef chars, size_t from = 0) const;

   /// Find the last character in the string that is \p C, or npos if not
   /// found.
   POLAR_NODISCARD
   size_t findLastOf(char character, size_t from = npos) const
   {
      return rfind(character, from);
   }

   /// Find the last character in the string that is in \p C, or npos if not
   /// found.
   ///
   /// Complexity: O(size() + Chars.size())
   POLAR_NODISCARD
   size_t findLastOf(StringRef chars, size_t from = npos) const;

   /// Find the last character in the string that is not \p C, or npos if not
   /// found.
   POLAR_NODISCARD
   size_t findLastNotOf(char character, size_t from = npos) const;

   /// Find the last character in the string that is not in \p Chars, or
   /// npos if not found.
   ///
   /// Complexity: O(size() + Chars.size())
   POLAR_NODISCARD
   size_t findLastNotOf(StringRef chars, size_t from = npos) const;

   /// Return true if the given string is a substring of *this, and false
   /// otherwise.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   bool contains(StringRef other) const
   {
      return find(other) != npos;
   }

   /// Return true if the given character is contained in *this, and false
   /// otherwise.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   bool contains(char character) const
   {
      return findFirstOf(character) != npos;
   }

   /// Return true if the given string is a substring of *this, and false
   /// otherwise.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   bool containsLower(StringRef other) const
   {
      return findLower(other) != npos;
   }

   /// Return true if the given character is contained in *this, and false
   /// otherwise.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   bool containsLower(char character) const
   {
      return findLower(character) != npos;
   }

   /// @}
   /// @name Helpful Algorithms
   /// @{

   /// Return the number of occurrences of \p C in the string.
   POLAR_NODISCARD
   size_t count(char character) const
   {
      size_t count = 0;
      for (size_t i = 0, e = m_length; i != e; ++i) {
         if (m_data[i] == character) {
            ++count;
         }
      }
      return count;
   }

   /// Return the number of non-overlapped occurrences of \p str in
   /// the string.
   size_t count(StringRef str) const;

   /// Parse the current string as an integer of the specified radix.  If
   /// \p radix is specified as zero, this does radix autosensing using
   /// extended C rules: 0 is octal, 0x is hex, 0b is binary.
   ///
   /// If the string is invalid or if only a subset of the string is valid,
   /// this returns true to signify the error.  The string is considered
   /// erroneous if empty or if it overflows T.
   template <typename T>
   typename std::enable_if<std::numeric_limits<T>::is_signed, bool>::type
   getAsInteger(unsigned radix, T &result) const
   {
      long long llVal;
      if (get_as_signed_integer(*this, radix, llVal) ||
          static_cast<T>(llVal) != llVal) {
         return true;
      }
      result = llVal;
      return false;
   }

   template <typename T>
   typename std::enable_if<!std::numeric_limits<T>::is_signed, bool>::type
   getAsInteger(unsigned radix, T &result) const
   {
      unsigned long long ullVal;
      // The additional cast to unsigned long long is required to avoid the
      // Visual C++ warning C4805: '!=' : unsafe mix of type 'bool' and type
      // 'unsigned __int64' when instantiating getAsInteger with T = bool.
      if (get_as_unsigned_integer(*this, radix, ullVal) ||
          static_cast<unsigned long long>(static_cast<T>(ullVal)) != ullVal) {
         return true;
      }
      result = ullVal;
      return false;
   }

   /// Parse the current string as an integer of the specified radix.  If
   /// \p radix is specified as zero, this does radix autosensing using
   /// extended C rules: 0 is octal, 0x is hex, 0b is binary.
   ///
   /// If the string does not begin with a number of the specified radix,
   /// this returns true to signify the error. The string is considered
   /// erroneous if empty or if it overflows T.
   /// The portion of the string representing the discovered numeric value
   /// is removed from the beginning of the string.
   template <typename T>
   typename std::enable_if<std::numeric_limits<T>::is_signed, bool>::type
   consumeInteger(unsigned radix, T &result)
   {
      long long llVal;
      if (consume_signed_integer(*this, radix, llVal) ||
          static_cast<long long>(static_cast<T>(llVal)) != llVal) {
         return true;
      }
      result = llVal;
      return false;
   }

   template <typename T>
   typename std::enable_if<!std::numeric_limits<T>::is_signed, bool>::type
   consumeInteger(unsigned radix, T &result)
   {
      unsigned long long ullVal;
      if (consume_unsigned_integer(*this, radix, ullVal) ||
          static_cast<unsigned long long>(static_cast<T>(ullVal)) != ullVal) {
         return true;
      }
      result = ullVal;
      return false;
   }

   /// Parse the current string as an integer of the specified \p radix, or of
   /// an autosensed radix if the \p radix given is 0.  The current value in
   /// \p result is discarded, and the storage is changed to be wide enough to
   /// store the parsed integer.
   ///
   /// \returns true if the string does not solely consist of a valid
   /// non-empty number in the appropriate base.
   ///
   /// APInt::fromString is superficially similar but assumes the
   /// string is well-formed in the given radix.
   bool getAsInteger(unsigned radix, ApInt &result) const;

   /// Parse the current string as an IEEE double-precision floating
   /// point value.  The string must be a well-formed double.
   ///
   /// If \p allowInexact is false, the function will fail if the string
   /// cannot be represented exactly.  Otherwise, the function only fails
   /// in case of an overflow or underflow.
   bool getAsDouble(double &result, bool allowInexact = true) const;

   /// @}
   /// @name String Operations
   /// @{

   // Convert the given ASCII string to lowercase.
   POLAR_NODISCARD
   std::string toLower() const;

   /// Convert the given ASCII string to uppercase.
   POLAR_NODISCARD
   std::string toUpper() const;

   /// @}
   /// @name Substring Operations
   /// @{

   /// Return a reference to the substring from [Start, Start + N).
   ///
   /// \param Start The index of the starting character in the substring; if
   /// the index is npos or greater than the length of the string then the
   /// empty substring will be returned.
   ///
   /// \param N The number of characters to included in the substring. If N
   /// exceeds the number of characters remaining in the string, the string
   /// suffix (starting with \p Start) will be returned.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringRef substr(size_t start, size_t size = npos) const
   {
      start = std::min(start, m_length);
      return StringRef(m_data + start, std::min(size, m_length - start));
   }

   /// Return a StringRef equal to 'this' but with only the first \p N
   /// elements remaining.  If \p N is greater than the length of the
   /// string, the entire string is returned.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringRef takeFront(size_t size = 1) const
   {
      if (size >= getSize()) {
         return *this;
      }
      return dropBack(getSize() - size);
   }

   /// Return a StringRef equal to 'this' but with only the last \p N
   /// elements remaining.  If \p N is greater than the length of the
   /// string, the entire string is returned.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringRef takeBack(size_t size = 1) const
   {
      if (size >= getSize()) {
         return *this;
      }
      return dropFront(getSize() - size);
   }

   /// Return the longest prefix of 'this' such that every character
   /// in the prefix satisfies the given predicate.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringRef takeWhile(FunctionRef<bool(char)> func) const
   {
      return substr(0, findIfNot(func));
   }

   /// Return the longest prefix of 'this' such that no character in
   /// the prefix satisfies the given predicate.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringRef takeUntil(FunctionRef<bool(char)> func) const
   {
      return substr(0, findIf(func));
   }

   /// Return a StringRef equal to 'this' but with the first \p N elements
   /// dropped.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringRef dropFront(size_t size = 1) const
   {
      assert(getSize() >= size && "Dropping more elements than exist");
      return substr(size);
   }

   /// Return a StringRef equal to 'this' but with the last \p N elements
   /// dropped.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringRef dropBack(size_t size = 1) const
   {
      assert(getSize() >= size && "Dropping more elements than exist");
      return substr(0, getSize() - size);
   }

   /// Return a StringRef equal to 'this', but with all characters satisfying
   /// the given predicate dropped from the beginning of the string.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringRef dropWhile(FunctionRef<bool(char)> func) const
   {
      return substr(findIfNot(func));
   }

   /// Return a StringRef equal to 'this', but with all characters not
   /// satisfying the given predicate dropped from the beginning of the string.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringRef dropUntil(FunctionRef<bool(char)> func) const
   {
      return substr(findIf(func));
   }

   /// Returns true if this StringRef has the given prefix and removes that
   /// prefix.
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   bool consumeFront(StringRef prefix)
   {
      if (!startsWith(prefix)) {
         return false;
      }
      *this = dropFront(prefix.getSize());
      return true;
   }

   /// Returns true if this StringRef has the given suffix and removes that
   /// suffix.
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   bool consumeBack(StringRef suffix) {
      if (!endsWith(suffix)) {
         return false;
      }
      *this = dropBack(suffix.getSize());
      return true;
   }

   /// Return a reference to the substring from [Start, End).
   ///
   /// \param Start The index of the starting character in the substring; if
   /// the index is npos or greater than the length of the string then the
   /// empty substring will be returned.
   ///
   /// \param End The index following the last character to include in the
   /// substring. If this is npos or exceeds the number of characters
   /// remaining in the string, the string suffix (starting with \p Start)
   /// will be returned. If this is less than \p Start, an empty string will
   /// be returned.
   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringRef slice(size_t start, size_t end) const
   {
      start = std::min(start, m_length);
      end = std::min(std::max(start, end), m_length);
      return StringRef(m_data + start, end - start);
   }

   /// Split into two substrings around the first occurrence of a separator
   /// character.
   ///
   /// If \p Separator is in the string, then the result is a pair (LHS, RHS)
   /// such that (*this == LHS + Separator + RHS) is true and RHS is
   /// maximal. If \p Separator is not in the string, then the result is a
   /// pair (LHS, RHS) where (*this == LHS) and (RHS == "").
   ///
   /// \param Separator The character to split on.
   /// \returns The split substrings.
   POLAR_NODISCARD
   std::pair<StringRef, StringRef> split(char separator) const
   {
      return split(StringRef(&separator, 1));
   }

   /// Split into two substrings around the first occurrence of a separator
   /// string.
   ///
   /// If \p Separator is in the string, then the result is a pair (LHS, RHS)
   /// such that (*this == LHS + Separator + RHS) is true and RHS is
   /// maximal. If \p Separator is not in the string, then the result is a
   /// pair (LHS, RHS) where (*this == LHS) and (RHS == "").
   ///
   /// \param Separator - The string to split on.
   /// \return - The split substrings.
   POLAR_NODISCARD
   std::pair<StringRef, StringRef> split(StringRef separator) const
   {
      size_t idx = find(separator);
      if (idx == npos) {
         return std::make_pair(*this, StringRef());
      }
      return std::make_pair(slice(0, idx), slice(idx + separator.getSize(), npos));
   }

   /// Split into two substrings around the last occurrence of a separator
   /// string.
   ///
   /// If \p Separator is in the string, then the result is a pair (LHS, RHS)
   /// such that (*this == LHS + Separator + RHS) is true and RHS is
   /// minimal. If \p Separator is not in the string, then the result is a
   /// pair (LHS, RHS) where (*this == LHS) and (RHS == "").
   ///
   /// \param Separator - The string to split on.
   /// \return - The split substrings.
   POLAR_NODISCARD
   std::pair<StringRef, StringRef> rsplit(StringRef separator) const
   {
      size_t idx = rfind(separator);
      if (idx == npos) {
          return std::make_pair(*this, StringRef());
      }
      return std::make_pair(slice(0, idx), slice(idx + separator.size(), npos));
   }


   /// Split into substrings around the occurrences of a separator string.
   ///
   /// Each substring is stored in \p retArray. If \p MaxSplit is >= 0, at most
   /// \p maxSplit splits are done and consequently <= \p MaxSplit + 1
   /// elements are added to A.
   /// If \p keepEmpty is false, empty strings are not added to \p A. They
   /// still count when considering \p MaxSplit
   /// An useful invariant is that
   /// Separator.join(A) == *this if MaxSplit == -1 and KeepEmpty == true
   ///
   /// \param A - Where to put the substrings.
   /// \param Separator - The string to split on.
   /// \param MaxSplit - The maximum number of times the string is split.
   /// \param KeepEmpty - True if empty substring should be added.
   void split(SmallVectorImpl<StringRef> &retArray,
              StringRef separator, int maxSplit = -1,
              bool keepEmpty = true) const;

   /// Split into substrings around the occurrences of a separator character.
   ///
   /// Each substring is stored in \p A. If \p MaxSplit is >= 0, at most
   /// \p MaxSplit splits are done and consequently <= \p MaxSplit + 1
   /// elements are added to A.
   /// If \p KeepEmpty is false, empty strings are not added to \p A. They
   /// still count when considering \p MaxSplit
   /// An useful invariant is that
   /// Separator.join(A) == *this if MaxSplit == -1 and KeepEmpty == true
   ///
   /// \param A - Where to put the substrings.
   /// \param Separator - The string to split on.
   /// \param MaxSplit - The maximum number of times the string is split.
   /// \param KeepEmpty - True if empty substring should be added.
   void split(SmallVectorImpl<StringRef> &retArray, char separator, int maxSplit = -1,
              bool keepEmpty = true) const;

   /// Split into two substrings around the last occurrence of a separator
   /// character.
   ///
   /// If \p Separator is in the string, then the result is a pair (LHS, RHS)
   /// such that (*this == LHS + Separator + RHS) is true and RHS is
   /// minimal. If \p Separator is not in the string, then the result is a
   /// pair (LHS, RHS) where (*this == LHS) and (RHS == "").
   ///
   /// \param Separator - The character to split on.
   /// \return - The split substrings.
   POLAR_NODISCARD
   std::pair<StringRef, StringRef> rsplit(char separator) const
   {
      return rsplit(StringRef(&separator, 1));
   }

   /// Return string with consecutive \p Char characters starting from the
   /// the left removed.
   POLAR_NODISCARD
   StringRef ltrim(char character) const
   {
      return dropFront(std::min(m_length, findFirstNotOf(character)));
   }

   /// Return string with consecutive characters in \p Chars starting from
   /// the left removed.
   POLAR_NODISCARD
   StringRef ltrim(StringRef chars = " \t\n\v\f\r") const
   {
      return dropFront(std::min(m_length, findFirstNotOf(chars)));
   }

   /// Return string with consecutive \p Char characters starting from the
   /// right removed.
   POLAR_NODISCARD
   StringRef rtrim(char character) const
   {
      return dropBack(m_length - std::min(m_length, findLastNotOf(character) + 1));
   }

   /// Return string with consecutive characters in \p Chars starting from
   /// the right removed.
   POLAR_NODISCARD
   StringRef rtrim(StringRef chars = " \t\n\v\f\r") const
   {
      return dropBack(m_length - std::min(m_length, findLastNotOf(chars) + 1));
   }

   /// Return string with consecutive \p Char characters starting from the
   /// left and right removed.
   POLAR_NODISCARD
   StringRef trim(char character) const
   {
      return ltrim(character).rtrim(character);
   }

   /// Return string with consecutive characters in \p Chars starting from
   /// the left and right removed.
   POLAR_NODISCARD
   StringRef trim(StringRef chars = " \t\n\v\f\r") const
   {
      return ltrim(chars).rtrim(chars);
   }

   /// @}
};

/// A wrapper around a string literal that serves as a proxy for constructing
/// global tables of StringRefs with the length computed at compile time.
/// In order to avoid the invocation of a global constructor, StringLiteral
/// should *only* be used in a constexpr context, as such:
///
/// constexpr StringLiteral S("test");
///
class StringLiteral : public StringRef
{
private:
   constexpr StringLiteral(const char *str, size_t N)
      : StringRef(str, N)
   {}

public:
   template <size_t N>
   constexpr StringLiteral(const char (&str)[N])
#if defined(__clang__) && __has_attribute(enable_if)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgcc-compat"
   __attribute((enable_if(__builtin_strlen(str) == N - 1,
                          "invalid string literal")))
#pragma clang diagnostic pop
#endif
      : StringRef(str, N - 1) {
   }

   // Explicit construction for strings like "foo\0bar".
   template <size_t N>
   static constexpr StringLiteral withInnerNUL(const char (&str)[N])
   {
      return StringLiteral(str, N - 1);
   }
};

/// @name StringRef Comparison Operators
/// @{

POLAR_ATTRIBUTE_ALWAYS_INLINE
inline bool operator==(StringRef lhs, StringRef rhs)
{
   return lhs.equals(rhs);
}

POLAR_ATTRIBUTE_ALWAYS_INLINE
inline bool operator!=(StringRef lhs, StringRef rhs)
{
   return !(lhs == rhs);
}

inline bool operator<(StringRef lhs, StringRef rhs)
{
   return lhs.compare(rhs) == -1;
}

inline bool operator<=(StringRef lhs, StringRef rhs)
{
   return lhs.compare(rhs) != 1;
}

inline bool operator>(StringRef lhs, StringRef rhs)
{
   return lhs.compare(rhs) == 1;
}

inline bool operator>=(StringRef lhs, StringRef rhs)
{
   return lhs.compare(rhs) != -1;
}

inline std::string &operator+=(std::string &buffer, StringRef string)
{
   return buffer.append(string.getData(), string.getSize());
}

inline std::ostream &operator <<(std::ostream &out, const StringRef str)
{
   out << str.getStr();
   return out;
}

/// @}

/// \brief Compute a hash_code for a StringRef.
POLAR_NODISCARD
HashCode hash_value(StringRef str);

} // basic

namespace utils {
// StringRefs can be treated like a POD type.
template <typename T> struct IsPodLike;
template <>
struct IsPodLike<polar::basic::StringRef>
{
   static const bool value = true;
};
} // utils

} // polar

#endif // POLARPHP_BASIC_ADT_STRINGREF_H
