// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/05/28.

#ifndef POLARPHP_BASIC_ADT_SMALL_STRING_H
#define POLARPHP_BASIC_ADT_SMALL_STRING_H

#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringRef.h"
#include <cstddef>
#include <ostream>

namespace polar {
namespace basic {

/// SmallString - A SmallString is just a SmallVector with methods and accessors
/// that make it work better as a string (e.g. operator+ etc).
template<unsigned InternalLen>
class SmallString : public SmallVector<char, InternalLen>
{
public:
   /// Default ctor - Initialize to empty.
   SmallString() = default;

   /// Initialize from a StringRef.
   SmallString(StringRef str)
      : SmallVector<char, InternalLen>(str.begin(), str.end())
   {}

   /// Initialize with a range.
   template<typename ItTy>
   SmallString(ItTy start, ItTy end)
      : SmallVector<char, InternalLen>(start, end)
   {}

   // Note that in order to add new overloads for append & assign, we have to
   // duplicate the inherited versions so as not to inadvertently hide them.

   /// @}
   /// @name String Assignment
   /// @{

   /// Assign from a repeated element.
   void assign(size_t numElts, char element)
   {
      SmallVectorImpl<char>::assign(numElts, element);
   }

   /// Assign from an iterator pair.
   template<typename InputIter>
   void assign(InputIter start, InputIter end)
   {
      this->clear();
      SmallVectorImpl<char>::append(start, end);
   }

   /// Assign from a StringRef.
   void assign(StringRef rhs)
   {
      this->clear();
      SmallVectorImpl<char>::append(rhs.begin(), rhs.end());
   }

   /// Assign from a SmallVector.
   void assign(const SmallVectorImpl<char> &rhs)
   {
      this->clear();
      SmallVectorImpl<char>::append(rhs.begin(), rhs.end());
   }

   /// @}
   /// @name String Concatenation
   /// @{

   /// Append from an iterator pair.
   template<typename InputIter>
   void append(InputIter start, InputIter end)
   {
      SmallVectorImpl<char>::append(start, end);
   }

   void append(size_t numInputs, char element)
   {
      SmallVectorImpl<char>::append(numInputs, element);
   }

   /// Append from a StringRef.
   void append(StringRef rhs)
   {
      SmallVectorImpl<char>::append(rhs.begin(), rhs.end());
   }

   /// Append from a SmallVector.
   void append(const SmallVectorImpl<char> &rhs)
   {
      SmallVectorImpl<char>::append(rhs.begin(), rhs.end());
   }

   /// @}
   /// @name String Comparison
   /// @{

   /// Check for string equality.  This is more efficient than compare() when
   /// the relative ordering of inequal strings isn't needed.
   bool equals(StringRef rhs) const
   {
      return getStr().equals(rhs);
   }

   /// Check for string equality, ignoring case.
   bool equalsLower(StringRef rhs) const
   {
      return getStr().equalsLower(rhs);
   }

   /// Compare two strings; the result is -1, 0, or 1 if this string is
   /// lexicographically less than, equal to, or greater than the \p RHS.
   int compare(StringRef rhs) const
   {
      return getStr().compare(rhs);
   }

   /// compare_lower - Compare two strings, ignoring case.
   int compareLower(StringRef rhs) const
   {
      return getStr().compareLower(rhs);
   }

   /// compare_numeric - Compare two strings, treating sequences of digits as
   /// numbers.
   int compareNumeric(StringRef rhs) const
   {
      return getStr().compareNumeric(rhs);
   }

   /// @}
   /// @name String Predicates
   /// @{

   /// startswith - Check if this string starts with the given \p Prefix.
   bool startsWith(StringRef prefix) const
   {
      return getStr().startsWith(prefix);
   }

   /// endswith - Check if this string ends with the given \p Suffix.
   bool endsWith(StringRef suffix) const
   {
      return getStr().endsWith(suffix);
   }

   /// @}
   /// @name String Searching
   /// @{

   /// find - Search for the first character \p C in the string.
   ///
   /// \return - The index of the first occurrence of \p C, or npos if not
   /// found.
   size_t find(char character, size_t from = 0) const
   {
      return getStr().find(character, from);
   }

   /// Search for the first string \p Str in the string.
   ///
   /// \returns The index of the first occurrence of \p Str, or npos if not
   /// found.
   size_t find(StringRef str, size_t from = 0) const
   {
      return getStr().find(str, from);
   }

   /// Search for the last character \p C in the string.
   ///
   /// \returns The index of the last occurrence of \p C, or npos if not
   /// found.
   size_t rfind(char character, size_t from = StringRef::npos) const
   {
      return getStr().rfind(character, from);
   }

   /// Search for the last string \p Str in the string.
   ///
   /// \returns The index of the last occurrence of \p Str, or npos if not
   /// found.
   size_t rfind(StringRef str) const
   {
      return getStr().rfind(str);
   }

   /// Find the first character in the string that is \p C, or npos if not
   /// found. Same as find.
   size_t findFirstOf(char character, size_t from = 0) const
   {
      return getStr().findFirstOf(character, from);
   }

   /// Find the first character in the string that is in \p Chars, or npos if
   /// not found.
   ///
   /// Complexity: O(size() + Chars.size())
   size_t findFirstOf(StringRef chars, size_t from = 0) const
   {
      return getStr().findFirstOf(chars, from);
   }

   /// Find the first character in the string that is not \p C or npos if not
   /// found.
   size_t findFirstNotOf(char character, size_t from = 0) const
   {
      return getStr().findFirstNotOf(character, from);
   }

   /// Find the first character in the string that is not in the string
   /// \p Chars, or npos if not found.
   ///
   /// Complexity: O(size() + Chars.size())
   size_t findFirstNotOf(StringRef chars, size_t from = 0) const
   {
      return getStr().findFirstNotOf(chars, from);
   }

   /// Find the last character in the string that is \p C, or npos if not
   /// found.
   size_t findLastOf(char character, size_t from = StringRef::npos) const
   {
      return getStr().findLastOf(character, from);
   }

   /// Find the last character in the string that is in \p C, or npos if not
   /// found.
   ///
   /// Complexity: O(size() + Chars.size())
   size_t findLastOf(StringRef chars, size_t from = StringRef::npos) const
   {
      return getStr().findLastOf(chars, from);
   }

   /// @}
   /// @name Helpful Algorithms
   /// @{

   /// Return the number of occurrences of \p C in the string.
   size_t count(char character) const
   {
      return getStr().count(character);
   }

   /// Return the number of non-overlapped occurrences of \p Str in the
   /// string.
   size_t count(StringRef str) const
   {
      return getStr().count(str);
   }

   /// @}
   /// @name Substring Operations
   /// @{

   /// Return a reference to the substring from [Start, Start + N).
   ///
   /// \param Start The index of the starting character in the substring; if
   /// the index is npos or greater than the length of the string then the
   /// empty substring will be returned.
   ///
   /// \param N The number of characters to included in the substring. If \p N
   /// exceeds the number of characters remaining in the string, the string
   /// suffix (starting with \p Start) will be returned.
   StringRef substr(size_t start, size_t size = StringRef::npos) const
   {
      return getStr().substr(start, size);
   }

   /// Return a reference to the substring from [Start, End).
   ///
   /// \param Start The index of the starting character in the substring; if
   /// the index is npos or greater than the length of the string then the
   /// empty substring will be returned.
   ///
   /// \param End The index following the last character to include in the
   /// substring. If this is npos, or less than \p Start, or exceeds the
   /// number of characters remaining in the string, the string suffix
   /// (starting with \p Start) will be returned.
   StringRef slice(size_t start, size_t end) const
   {
      return getStr().slice(start, end);
   }

   // Extra methods.

   /// Explicit conversion to StringRef.
   StringRef getStr() const
   {
      return StringRef(this->begin(), this->getSize());
   }

   // TODO: Make this const, if it's safe...
   const char* getCStr()
   {
      this->pushBack(0);
      this->popBack();
      return this->getData();
   }

   /// Implicit conversion to StringRef.
   operator StringRef() const
   {
      return getStr();
   }

   // Extra operators.
   const SmallString &operator=(StringRef rhs)
   {
      this->clear();
      return *this += rhs;
   }

   SmallString &operator+=(StringRef rhs)
   {
      append(rhs.begin(), rhs.end());
      return *this;
   }

   SmallString &operator+=(char character)
   {
      this->pushBack(character);
      return *this;
   }
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_SMALL_STRING_H
