// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/03.

#ifndef POLARPHP_BASIC_ADT_STRING_SWITCH_H
#define POLARPHP_BASIC_ADT_STRING_SWITCH_H

#include "polarphp/basic/adt/StringRef.h"
#include <cassert>
#include <cstring>
#include <optional>

namespace polar {
namespace basic {

/// A switch()-like statement whose conds are string literals.
///
/// The StringSwitch class is a simple form of a switch() statement that
/// determines whether the given string matches one of the given string
/// literals. The template type parameter \p T is the type of the value that
/// will be returned from the string-switch expression. For example,
/// the following code switches on the name of a color in \c argv[i]:
///
/// \code
/// Color color = StringSwitch<Color>(argv[i])
///   .cond("red", Red)
///   .cond("orange", Orange)
///   .cond("yellow", Yellow)
///   .cond("green", Green)
///   .cond("blue", Blue)
///   .cond("indigo", Indigo)
///   .conds("violet", "purple", Violet)
///   .defaultCond(UnknownColor);
/// \endcode
template<typename T, typename R = T>
class StringSwitch
{
   /// \brief The string we are matching.
   StringRef m_str;

   /// \brief The pointer to the result of this switch statement, once known,
   /// null before that.
   std::optional<T> m_result;

public:
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   explicit StringSwitch(StringRef str)
      : m_str(str),
        m_result(std::nullopt)
   {}

   // StringSwitch is not copyable.
   StringSwitch(const StringSwitch &) = delete;
   void operator=(const StringSwitch &) = delete;
   StringSwitch &operator=(StringSwitch &&other) = delete;

   StringSwitch(StringSwitch &&other)
      : m_str(other.m_str),
        m_result(std::move(other.m_result))
   {}

   ~StringSwitch() = default;

   // cond-sensitive cond matchers
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch& cond(StringLiteral str, T value)
   {
      if (!m_result && m_str == str) {
         m_result = std::move(value);
      }
      return *this;
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &endsWith(StringLiteral str, T value)
   {
      if (!m_result && m_str.endsWith(str)) {
         m_result = std::move(value);
      }
      return *this;
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &startsWith(StringLiteral str, T value)
   {
      if (!m_result && m_str.startsWith(str)) {
         m_result = std::move(value);
      }
      return *this;
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &conds(StringLiteral str0, StringLiteral str1, T value)
   {
      return cond(str0, value).cond(str1, value);
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &conds(StringLiteral str0, StringLiteral str1,
                       StringLiteral str2, T value)
   {
      return cond(str0, value).conds(str1, str2, value);
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &conds(StringLiteral str0, StringLiteral str1,
                       StringLiteral str2, StringLiteral str3,
                       T value)
   {
      return cond(str0, value).conds(str1, str2, str3, value);
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &conds(StringLiteral str0, StringLiteral str1,
                       StringLiteral str2, StringLiteral str3,
                       StringLiteral str4, T value)
   {
      return cond(str0, value).conds(str1, str2, str3, str4, value);
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &conds(StringLiteral str0, StringLiteral str1,
                       StringLiteral str2, StringLiteral str3,
                       StringLiteral str4, StringLiteral str5,
                       T value)
   {
      return cond(str0, value).conds(str1, str2, str3, str4, str5, value);
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &conds(StringLiteral str0, StringLiteral str1,
                       StringLiteral str2, StringLiteral str3,
                       StringLiteral str4, StringLiteral str5,
                       StringLiteral str6, T value)
   {
      return cond(str0, value).conds(str1, str2, str3, str4, str5, str6, value);
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &conds(StringLiteral str0, StringLiteral str1,
                       StringLiteral str2, StringLiteral str3,
                       StringLiteral str4, StringLiteral str5,
                       StringLiteral str6, StringLiteral str7,
                       T value)
   {
      return cond(str0, value).conds(str1, str2, str3, str4, str5, str6, str7, value);
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &conds(StringLiteral str0, StringLiteral str1,
                       StringLiteral str2, StringLiteral str3,
                       StringLiteral str4, StringLiteral str5,
                       StringLiteral str6, StringLiteral str7,
                       StringLiteral str8, T value)
   {
      return cond(str0, value).conds(str1, str2, str3, str4, str5, str6, str7, str8, value);
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &conds(StringLiteral str0, StringLiteral str1,
                       StringLiteral str2, StringLiteral str3,
                       StringLiteral str4, StringLiteral str5,
                       StringLiteral str6, StringLiteral str7,
                       StringLiteral str8, StringLiteral str9,
                       T value)
   {
      return cond(str0, value).conds(str1, str2, str3, str4, str5, str6, str7, str8, str9, value);
   }

   // cond-insensitive cond matchers.
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &condLower(StringLiteral str, T value)
   {
      if (!m_result && m_str.equalsLower(str)) {
         m_result = std::move(value);
      }
      return *this;
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &endsWithLower(StringLiteral str, T value)
   {
      if (!m_result && m_str.endsWithLower(str)) {
         m_result = std::move(value);
      }
      return *this;
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &startsWithLower(StringLiteral str, T value)
   {
      if (!m_result && m_str.startsWithLower(str)) {
         m_result = std::move(value);
      }
      return *this;
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &condsLower(StringLiteral str0, StringLiteral str1, T value)
   {
      return condLower(str0, value).condLower(str1, value);
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &condsLower(StringLiteral str0, StringLiteral str1,
                            StringLiteral str2, T value)
   {
      return condLower(str0, value).condsLower(str1, str2, value);
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &condsLower(StringLiteral str0, StringLiteral str1,
                            StringLiteral str2, StringLiteral str3,
                            T value)
   {
      return condLower(str0, value).condsLower(str1, str2, str3, value);
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   StringSwitch &condsLower(StringLiteral str0, StringLiteral str1,
                            StringLiteral str2, StringLiteral str3,
                            StringLiteral str4, T value)
   {
      return condLower(str0, value).condsLower(str1, str2, str3, str4, value);
   }

   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   R defaultCond(T value) const
   {
      if (m_result) {
         return std::move(*m_result);
      }
      return value;
   }

   POLAR_NODISCARD
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   operator R() const
   {
      assert(m_result && "Fell off the end of a string-switch");
      return std::move(*m_result);
   }
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_STRING_SWITCH_H
