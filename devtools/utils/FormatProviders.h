// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/11.

#ifndef POLAR_DEVLTOOLS_UTILS_FORMAT_PROVIDERS_H
#define POLAR_DEVLTOOLS_UTILS_FORMAT_PROVIDERS_H

#include "FormatVariadicDetail.h"
#include "StringExtras.h"
#include "NativeFormatting.h"

#include <optional>
#include <type_traits>
#include <vector>

namespace polar {
namespace utils {

namespace internal {
template <typename T>
struct UseIntegralFormatter
      : public std::integral_constant<
      bool, is_one_of<T, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
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
      is_one_of<T, char *, const char *>::value> {
};

template <typename T>
struct UseStringFormatter
      : public std::integral_constant<bool,
      std::is_convertible<T, std::string_view>::value>
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
   static std::optional<size_t> parseNumericPrecision(std::string_view str)
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

   static bool consumeHexStyle(std::string_view &str, HexPrintStyle &style)
   {
      if (!string_starts_with_lowercase(str, "x")) {
         return false;
      }
      if (string_consume_front(str, "x-")) {
         style = HexPrintStyle::Lower;
      } else if (string_consume_front(str, "X-")) {
         style = HexPrintStyle::Upper;
      } else if (string_consume_front(str, "x+") || string_consume_front(str, "x")) {
         style = HexPrintStyle::PrefixLower;
      } else if (string_consume_front(str, "X+") || string_consume_front(str, "X")) {
         style = HexPrintStyle::PrefixUpper;
      }
      return true;
   }

   static size_t consumeNumHexDigits(std::string_view &str, HexPrintStyle style,
                                     size_t defaultValue)
   {
      string_consume_integer(str, 10, defaultValue);
      if (isPrefixedHexStyle(style)) {
         defaultValue += 2;
      }
      return defaultValue;
   }
};
} // internal


} // utils
} // polar

#endif // POLAR_DEVLTOOLS_UTILS_FORMAT_PROVIDERS_H
