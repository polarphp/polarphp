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

#ifndef POLARPHP_UTILS_FORMAT_PROVIDERS_H
#define POLARPHP_UTILS_FORMAT_PROVIDERS_H

#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/StringSwitch.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/utils/FormatVariadicDetail.h"
#include "polarphp/utils/NativeFormatting.h"

#include <type_traits>
#include <vector>

namespace polar {
namespace utils {

using polar::basic::StringRef;
using polar::basic::StringSwitch;
using polar::basic::Twine;

namespace internal {
template <typename T>
struct UseIntegralFormatter
      : public std::integral_constant<
      bool, polar::basic::is_one_of<T, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
      int64_t, uint64_t, int, unsigned, long, unsigned long,
      long long, unsigned long long>::value>
{};

template <typename T>
struct UseCharFormatter
      : public std::integral_constant<bool, std::is_same<T, char>::value>
{};

template <typename T>
struct IsCString
      : public std::integral_constant<bool,
      polar::basic::is_one_of<T, char *, const char *>::value>
{
};

template <typename T>
struct UseStringFormatter
      : public std::integral_constant<bool,
      std::is_convertible<T, StringRef>::value>
{};

template <typename T>
struct UsePointerFormatter
      : public std::integral_constant<bool, std::is_pointer<T>::value &&
      !IsCString<T>::value>
{};

template <typename T>
struct UseDoubleFormatter
      : public std::integral_constant<bool, std::is_floating_point<T>::value>
{};

class HelperFunctions
{
protected:
   static std::optional<size_t> parseNumericPrecision(StringRef str)
   {
      size_t prec;
      std::optional<size_t> result;
      if (str.empty()) {
         result = std::nullopt;
      } else if (str.getAsInteger(10, prec)) {
         assert(false && "Invalid precision specifier");
         result = std::nullopt;
      } else {
         assert(prec < 100 && "Precision out of range");
         result = std::min<size_t>(99u, prec);
      }
      return result;
   }

   static bool consumeHexStyle(StringRef &str, HexPrintStyle &style)
   {
      if (!str.startsWithLower("x")) {
         return false;
      }
      if (str.consumeFront("x-")) {
         style = HexPrintStyle::Lower;
      } else if (str.consumeFront("X-")) {
         style = HexPrintStyle::Upper;
      } else if (str.consumeFront("x+") || str.consumeFront("x")) {
         style = HexPrintStyle::PrefixLower;
      } else if (str.consumeFront("X+") || str.consumeFront("X")) {
         style = HexPrintStyle::PrefixUpper;
      }
      return true;
   }

   static size_t consumeNumHexDigits(StringRef &str, HexPrintStyle style,
                                     size_t defaultValue) {
      str.consumeInteger(10, defaultValue);
      if (is_prefixed_hex_style(style)) {
         defaultValue += 2;
      }
      return defaultValue;
   }
};
} // internal

/// Implementation of FormatProvider<T> for integral arithmetic types.
///
/// The options string of an integral type has the grammar:
///
///   integer_options   :: [style][digits]
///   style             :: <see table below>
///   digits            :: <non-negative integer> 0-99
///
///   ==========================================================================
///   |  style  |     Meaning          |      Example     | Digits Meaning     |
///   --------------------------------------------------------------------------
///   |         |                      |  Input |  Output |                    |
///   ==========================================================================
///   |   x-    | Hex no prefix, lower |   42   |    2a   | Minimum # digits   |
///   |   X-    | Hex no prefix, upper |   42   |    2A   | Minimum # digits   |
///   | x+ / x  | Hex + prefix, lower  |   42   |   0x2a  | Minimum # digits   |
///   | X+ / X  | Hex + prefix, upper  |   42   |   0x2A  | Minimum # digits   |
///   | N / n   | Digit grouped number | 123456 | 123,456 | Ignored            |
///   | D / d   | Integer              | 100000 | 100000  | Ignored            |
///   | (empty) | Same as D / d        |        |         |                    |
///   ==========================================================================
///

template <typename T>
struct FormatProvider<
      T, typename std::enable_if<internal::UseIntegralFormatter<T>::value>::type>
      : public internal::HelperFunctions
{
private:
public:
   static void format(const T &value, RawOutStream &stream, StringRef style)
   {
      HexPrintStyle hs;
      size_t digits = 0;
      if (consumeHexStyle(style, hs)) {
         digits = consumeNumHexDigits(style, hs, 0);
         write_hex(stream, value, hs, digits);
         return;
      }

      IntegerStyle is = IntegerStyle::Integer;
      if (style.consumeFront("N") || style.consumeFront("n")) {
         is = IntegerStyle::Number;
      } else if (style.consumeFront("D") || style.consumeFront("d")) {
         is = IntegerStyle::Integer;
      }
      style.consumeInteger(10, digits);
      assert(style.empty() && "Invalid integral format style!");
      write_integer(stream, value, digits, is);
   }
};

/// Implementation of FormatProvider<T> for integral pointer types.
///
/// The options string of a pointer type has the grammar:
///
///   pointer_options   :: [style][precision]
///   style             :: <see table below>
///   digits            :: <non-negative integer> 0-sizeof(void*)
///
///   ==========================================================================
///   |   S     |     Meaning          |                Example                |
///   --------------------------------------------------------------------------
///   |         |                      |       Input       |      Output       |
///   ==========================================================================
///   |   x-    | Hex no prefix, lower |    0xDEADBEEF     |     deadbeef      |
///   |   X-    | Hex no prefix, upper |    0xDEADBEEF     |     DEADBEEF      |
///   | x+ / x  | Hex + prefix, lower  |    0xDEADBEEF     |    0xdeadbeef     |
///   | X+ / X  | Hex + prefix, upper  |    0xDEADBEEF     |    0xDEADBEEF     |
///   | (empty) | Same as X+ / X       |                   |                   |
///   ==========================================================================
///
/// The default precision is the number of nibbles in a machine word, and in all
/// cases indicates the minimum number of nibbles to print.
template <typename T>
struct FormatProvider<
      T, typename std::enable_if<internal::UsePointerFormatter<T>::value>::type>
      : public internal::HelperFunctions
{
private:
public:
   static void format(const T &value, RawOutStream &stream, StringRef style)
   {
      HexPrintStyle hs = HexPrintStyle::PrefixUpper;
      consumeHexStyle(style, hs);
      size_t digits = consumeNumHexDigits(style, hs, sizeof(void *) * 2);
      write_hex(stream, reinterpret_cast<std::uintptr_t>(value), hs, digits);
   }
};

/// Implementation of FormatProvider<T> for c-style strings and string
/// objects such as std::string and polar::basic::StringRef.
///
/// The options string of a string type has the grammar:
///
///   string_options :: [length]
///
/// where `length` is an optional integer specifying the maximum number of
/// characters in the string to print.  If `length` is omitted, the string is
/// printed up to the null terminator.

template <typename T>
struct FormatProvider<
      T, typename std::enable_if<internal::UseStringFormatter<T>::value>::type>
{
   static void format(const T &value, RawOutStream &stream, StringRef style)
   {
      size_t N = StringRef::npos;
      if (!style.empty() && style.getAsInteger(10, N)) {
         assert(false && "Style is not a valid integer");
      }
      StringRef str = value;
      stream << str.substr(0, N);
   }
};

/// Implementation of FormatProvider<T> for polar::basic::Twine.
///
/// This follows the same rules as the string formatter.

template <>
struct FormatProvider<Twine>
{
   static void format(const Twine &value, RawOutStream &stream,
                      StringRef style)
   {
      FormatProvider<std::string>::format(value.getStr(), stream, style);
   }
};

/// Implementation of FormatProvider<T> for characters.
///
/// The options string of a character type has the grammar:
///
///   char_options :: (empty) | [integer_options]
///
/// If `char_options` is empty, the character is displayed as an ASCII
/// character.  Otherwise, it is treated as an integer options string.
///
template <typename T>
struct FormatProvider<
      T, typename std::enable_if<internal::UseCharFormatter<T>::value>::type>
{
   static void format(const char &value, RawOutStream &stream,
                      StringRef style)
   {
      if (style.empty()) {
         stream << value;
      } else {
         int x = static_cast<int>(value);
         FormatProvider<int>::format(x, stream, style);
      }
   }
};

/// Implementation of FormatProvider<T> for type `bool`
///
/// The options string of a boolean type has the grammar:
///
///   bool_options :: "" | "Y" | "y" | "D" | "d" | "T" | "t"
///
///   ==================================
///   |    C    |     Meaning          |
///   ==================================
///   |    Y    |       YES / NO       |
///   |    y    |       yes / no       |
///   |  D / d  |    Integer 0 or 1    |
///   |    T    |     TRUE / FALSE     |
///   |    t    |     true / false     |
///   | (empty) |   Equivalent to 't'  |
///   ==================================
template <>
struct FormatProvider<bool>
{
   static void format(const bool &bvalue, RawOutStream &stream,
                      StringRef style) {
      stream << StringSwitch<const char *>(style)
                .cond("Y", bvalue ? "YES" : "NO")
                .cond("y", bvalue ? "yes" : "no")
                .condLower("D", bvalue ? "1" : "0")
                .cond("T", bvalue ? "TRUE" : "FALSE")
                .conds("t", "", bvalue ? "true" : "false")
                .defaultCond(bvalue ? "1" : "0");
   }
};

/// Implementation of FormatProvider<T> for floating point types.
///
/// The options string of a floating point type has the format:
///
///   float_options   :: [style][precision]
///   style           :: <see table below>
///   precision       :: <non-negative integer> 0-99
///
///   =====================================================
///   |  style  |     Meaning          |      Example     |
///   -----------------------------------------------------
///   |         |                      |  Input |  Output |
///   =====================================================
///   | P / p   | Percentage           |  0.05  |  5.00%  |
///   | F / f   | Fixed point          |   1.0  |  1.00   |
///   |   E     | Exponential with E   | 100000 | 1.0E+05 |
///   |   e     | Exponential with e   | 100000 | 1.0e+05 |
///   | (empty) | Same as F / f        |        |         |
///   =====================================================
///
/// The default precision is 6 for exponential (E / e) and 2 for everything
/// else.

template <typename T>
struct FormatProvider<
      T, typename std::enable_if<internal::UseDoubleFormatter<T>::value>::type>
      : public internal::HelperFunctions
{
   static void format(const T &value, RawOutStream &stream, StringRef style)
   {
      FloatStyle fstyle;
      if (style.consumeFront("P") || style.consumeFront("p")) {
         fstyle = FloatStyle::Percent;
      } else if (style.consumeFront("F") || style.consumeFront("f")) {
         fstyle = FloatStyle::Fixed;
      } else if (style.consumeFront("E")) {
         fstyle = FloatStyle::ExponentUpper;
      } else if (style.consumeFront("e")) {
         fstyle = FloatStyle::Exponent;
      } else {
         fstyle = FloatStyle::Fixed;
      }
      std::optional<size_t> precision = parseNumericPrecision(style);
      if (!precision.has_value()) {
         precision = get_default_precision(fstyle);
      }
      write_double(stream, static_cast<double>(value), fstyle, precision);
   }
};

namespace internal {
template <typename IterT>
using IterValue = typename std::iterator_traits<IterT>::value_type;

template <typename IterT>
struct range_item_has_provider
      : public std::integral_constant<
      bool, !UsesMissingProvider<IterValue<IterT>>::value>
{};
} // internal

/// Implementation of FormatProvider<T> for ranges.
///
/// This will print an arbitrary range as a delimited sequence of items.
///
/// The options string of a range type has the grammar:
///
///   range_style       ::= [separator] [element_style]
///   separator         ::= "$" delimeted_expr
///   element_style     ::= "@" delimeted_expr
///   delimeted_expr    ::= "[" expr "]" | "(" expr ")" | "<" expr ">"
///   expr              ::= <any string not containing delimeter>
///
/// where the separator expression is the string to insert between consecutive
/// items in the range and the argument expression is the Style specification to
/// be used when formatting the underlying type.  The default separator if
/// unspecified is ' ' (space).  The syntax of the argument expression follows
/// whatever grammar is dictated by the format provider or format adapter used
/// to format the value type.
///
/// Note that attempting to format an `iterator_range<T>` where no format
/// provider can be found for T will result in a compile error.
///

template <typename IterT> class FormatProvider<polar::basic::IteratorRange<IterT>>
{
   using value = typename std::iterator_traits<IterT>::value_type;
   using reference = typename std::iterator_traits<IterT>::reference;

   static StringRef consumeOneOption(StringRef &style, char indicator,
                                     StringRef defaultValue)
   {
      if (style.empty()) {
         return defaultValue;
      }
      if (style.front() != indicator) {
         return defaultValue;
      }
      style = style.dropFront();
      if (style.empty()) {
         assert(false && "Invalid range style");
         return defaultValue;
      }

      for (const char *d : {"[]", "<>", "()"}) {
         if (style.front() != d[0]) {
            continue;
         }
         size_t end = style.findFirstOf(d[1]);
         if (end == StringRef::npos) {
            assert(false && "Missing range option end delimeter!");
            return defaultValue;
         }
         StringRef result = style.slice(1, end);
         style = style.dropFront(end + 1);
         return result;
      }
      assert(false && "Invalid range style!");
      return defaultValue;
   }

   static std::pair<StringRef, StringRef> parseOptions(StringRef style)
   {
      StringRef sep = consumeOneOption(style, '$', ", ");
      StringRef args = consumeOneOption(style, '@', "");
      assert(style.empty() && "Unexpected text in range option string!");
      return std::make_pair(sep, args);
   }

public:
   static_assert(internal::range_item_has_provider<IterT>::value,
                 "Range value_type does not have a format provider!");
   static void format(const polar::basic::IteratorRange<IterT> &value,
                      RawOutStream &stream, StringRef style) {
      StringRef sep;
      StringRef argStyle;
      std::tie(sep, argStyle) = parseOptions(style);
      auto begin = value.begin();
      auto end = value.end();
      if (begin != end) {
         auto adapter =
               internal::build_format_adapter(std::forward<reference>(*begin));
         adapter.format(stream, argStyle);
         ++begin;
      }
      while (begin != end) {
         stream << sep;
         auto adapter =
               internal::build_format_adapter(std::forward<reference>(*begin));
         adapter.format(stream, argStyle);
         ++begin;
      }
   }
};

} // utils
} // polar

#endif // POLARPHP_UTILS_FORMAT_PROVIDERS_H
