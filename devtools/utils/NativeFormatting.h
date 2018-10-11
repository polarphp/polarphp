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

#ifndef POLAR_DEVLTOOLS_UTILS_NATIVE_FORMATTING_H
#define POLAR_DEVLTOOLS_UTILS_NATIVE_FORMATTING_H

#include <optional>
#include <ostream>
#include <cstdint>

namespace polar {
namespace utils {

enum class FloatStyle { Exponent, ExponentUpper, Fixed, Percent };
enum class IntegerStyle {
   Integer,
   Number,
};
enum class HexPrintStyle { Upper, Lower, PrefixUpper, PrefixLower };

size_t get_default_precision(FloatStyle style);

bool is_prefixed_hex_style(HexPrintStyle style);

void write_integer(std::ostream &out, unsigned int value, size_t minDigits,
                   IntegerStyle style);
void write_integer(std::ostream &out, int value, size_t minDigits, IntegerStyle style);
void write_integer(std::ostream &out, unsigned long value, size_t minDigits,
                   IntegerStyle style);
void write_integer(std::ostream &out, long value, size_t minDigits,
                   IntegerStyle style);
void write_integer(std::ostream &out, unsigned long long value, size_t minDigits,
                   IntegerStyle style);
void write_integer(std::ostream &out, long long value, size_t minDigits,
                   IntegerStyle style);

void write_hex(std::ostream &out, uint64_t value, HexPrintStyle style,
               std::optional<size_t> width = std::nullopt);
void write_double(std::ostream &out, double value, FloatStyle style,
                  std::optional<size_t> precision = std::nullopt);

} // utils
} // polar

#endif // POLAR_DEVLTOOLS_UTILS_NATIVE_FORMATTING_H
