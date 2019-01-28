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

#ifndef POLARPHP_SUPPORT_NATIVE_FORMATTING_H
#define POLARPHP_SUPPORT_NATIVE_FORMATTING_H

#include "polarphp/utils/RawOutStream.h"
#include <cstdint>
#include <optional>

namespace polar {
namespace utils {

enum class FloatStyle
{
   Exponent,
   ExponentUpper,
   Fixed,
   Percent
};

enum class IntegerStyle
{
   Integer,
   Number,
};

enum class HexPrintStyle
{
   Upper,
   Lower,
   PrefixUpper,
   PrefixLower
};

size_t get_default_precision(FloatStyle style);

bool is_prefixed_hex_style(HexPrintStyle style);

void write_integer(RawOutStream &outStream, unsigned int size, size_t minDigits,
                   IntegerStyle style);
void write_integer(RawOutStream &outStream, int size, size_t minDigits, IntegerStyle Style);
void write_integer(RawOutStream &outStream, unsigned long size, size_t minDigits,
                   IntegerStyle style);
void write_integer(RawOutStream &outStream, long size, size_t minDigits,
                   IntegerStyle style);
void write_integer(RawOutStream &outStream, unsigned long long size, size_t minDigits,
                   IntegerStyle style);
void write_integer(RawOutStream &outStream, long long size, size_t minDigits,
                   IntegerStyle style);

void write_hex(RawOutStream &outStream, uint64_t size, HexPrintStyle style,
               std::optional<size_t> width = std::nullopt);
void write_double(RawOutStream &outStream, double D, FloatStyle style,
                  std::optional<size_t> precision = std::nullopt);

} // utils
} // polar

#endif // POLARPHP_SUPPORT_NATIVE_FORMATTING_H
