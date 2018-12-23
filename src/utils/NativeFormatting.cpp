// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/10.

#include "polarphp/utils/NativeFormatting.h"

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/utils/Format.h"

#include <float.h>

namespace polar {
namespace utils {

using polar::basic::SmallString;
using polar::basic::ArrayRef;

namespace {

template<typename T, std::size_t N>
int format_to_buffer(T value, char (&buffer)[N])
{
   char *endPtr = std::end(buffer);
   char *curPtr = endPtr;

   do {
      *--curPtr = '0' + char(value % 10);
      value /= 10;
   } while (value);
   return endPtr - curPtr;
}

void writeWithCommas(RawOutStream &outStream, ArrayRef<char> buffer)
{
   assert(!buffer.empty());

   ArrayRef<char> thisGroup;
   int initialDigits = ((buffer.getSize() - 1) % 3) + 1;
   thisGroup = buffer.takeFront(initialDigits);
   outStream.write(thisGroup.getData(), thisGroup.getSize());
   buffer = buffer.dropFront(initialDigits);
   assert(buffer.getSize() % 3 == 0);
   while (!buffer.empty()) {
      outStream << ',';
      thisGroup = buffer.takeFront(3);
      outStream.write(thisGroup.getData(), 3);
      buffer = buffer.dropFront(3);
   }
}

template <typename T>
void write_unsigned_impl(RawOutStream &outStream, T N, size_t minDigits,
                         IntegerStyle style, bool isNegative) {
   static_assert(std::is_unsigned<T>::value, "Value is not unsigned!");

   char numberBuffer[128];
   std::memset(numberBuffer, '0', sizeof(numberBuffer));

   size_t len = 0;
   len = format_to_buffer(N, numberBuffer);
   if (isNegative) {
      outStream << '-';
   }
   if (len < minDigits && style != IntegerStyle::Number) {
      for (size_t index = len; index < minDigits; ++index) {
         outStream << '0';
      }
   }

   if (style == IntegerStyle::Number) {
      writeWithCommas(outStream, ArrayRef<char>(std::end(numberBuffer) - len, len));
   } else {
      outStream.write(std::end(numberBuffer) - len, len);
   }
}

template <typename T>
void write_unsigned(RawOutStream &outStream, T N, size_t minDigits,
                    IntegerStyle style, bool isNegative = false)
{
   // Output using 32-bit div/mod if possible.
   if (N == static_cast<uint32_t>(N)) {
      write_unsigned_impl(outStream, static_cast<uint32_t>(N), minDigits, style,
                          isNegative);
   } else {
      write_unsigned_impl(outStream, N, minDigits, style, isNegative);
   }
}

template <typename T>
void write_signed(RawOutStream &outStream, T N, size_t minDigits,
                  IntegerStyle style)
{
   static_assert(std::is_signed<T>::value, "Value is not signed!");
   using UnsignedType = typename std::make_unsigned<T>::type;
   if (N >= 0) {
      write_unsigned(outStream, static_cast<UnsignedType>(N), minDigits, style);
      return;
   }
   UnsignedType UN = -(UnsignedType)N;
   write_unsigned(outStream, UN, minDigits, style, true);
}

} // anonymous namespace

void write_integer(RawOutStream &outStream, unsigned int N, size_t minDigits,
                   IntegerStyle style)
{
   write_unsigned(outStream, N, minDigits, style);
}

void write_integer(RawOutStream &outStream, int N, size_t minDigits,
                   IntegerStyle style)
{
   write_signed(outStream, N, minDigits, style);
}

void write_integer(RawOutStream &outStream, unsigned long N, size_t minDigits,
                   IntegerStyle style)
{
   write_unsigned(outStream, N, minDigits, style);
}

void write_integer(RawOutStream &outStream, long N, size_t minDigits,
                   IntegerStyle style)
{
   write_signed(outStream, N, minDigits, style);
}

void write_integer(RawOutStream &outStream, unsigned long long N, size_t minDigits,
                   IntegerStyle style)
{
   write_unsigned(outStream, N, minDigits, style);
}

void write_integer(RawOutStream &outStream, long long N, size_t minDigits,
                   IntegerStyle style)
{
   write_signed(outStream, N, minDigits, style);
}

void write_hex(RawOutStream &outStream, uint64_t N, HexPrintStyle style,
               std::optional<size_t> width)
{
   const size_t kMaxWidth = 128u;

   size_t w = std::min(kMaxWidth, width.value_or(0u));

   unsigned nibbles = (64 - count_leading_zeros(N) + 3) / 4;
   bool prefix = (style == HexPrintStyle::PrefixLower ||
                  style == HexPrintStyle::PrefixUpper);
   bool upper =
         (style == HexPrintStyle::Upper || style == HexPrintStyle::PrefixUpper);
   unsigned prefixChars = prefix ? 2 : 0;
   unsigned numChars =
         std::max(static_cast<unsigned>(w), std::max(1u, nibbles) + prefixChars);

   char numberBuffer[kMaxWidth];
   ::memset(numberBuffer, '0', basic::array_lengthof(numberBuffer));
   if (prefix) {
      numberBuffer[1] = 'x';
   }
   char *endPtr = numberBuffer + numChars;
   char *curPtr = endPtr;
   while (N) {
      unsigned char x = static_cast<unsigned char>(N) % 16;
      *--curPtr = basic::hexdigit(x, !upper);
      N /= 16;
   }
   outStream.write(numberBuffer, numChars);
}

void write_double(RawOutStream &outStream, double N, FloatStyle style,
                  std::optional<size_t> precision)
{
   size_t prec = precision.value_or(get_default_precision(style));

   if (std::isnan(N)) {
      outStream << "nan";
      return;
   } else if (std::isinf(N)) {
      outStream << "INF";
      return;
   }

   char letter;
   if (style == FloatStyle::Exponent) {
      letter = 'e';
   } else if (style == FloatStyle::ExponentUpper) {
      letter = 'E';
   } else {
      letter = 'f';
   }

   SmallString<8> spec;
   RawSvectorOutStream out(spec);
   out << "%." << prec << letter;

   if (style == FloatStyle::Exponent || style == FloatStyle::ExponentUpper) {
#ifdef _WIN32
      // On MSVCRT and compatible, output of %e is incompatible to Posix
      // by default. Number of exponent digits should be at least 2. "%+03d"
      // FIXME: Implement our formatter to here or Support/Format.h!
#if defined(__MINGW32__)
      // FIXME: It should be generic to C++11.
      if (N == 0.0 && std::signbit(N)) {
         char negativeZero[] = "-0.000000e+00";
         if (Style == FloatStyle::ExponentUpper)
            negativeZero[strlen(negativeZero) - 4] = 'E';
         S << negativeZero;
         return;
      }
#else
      int fpcl = _fpclass(N);

      // negative zero
      if (fpcl == _FPCLASS_NZ) {
         char negativeZero[] = "-0.000000e+00";
         if (Style == FloatStyle::ExponentUpper)
            negativeZero[strlen(negativeZero) - 4] = 'E';
         S << negativeZero;
         return;
      }
#endif

      char buf[32];
      unsigned len;
      len = format(spec.c_str(), N).snprint(buf, sizeof(buf));
      if (len <= sizeof(buf) - 2) {
         if (len >= 5 && (buf[len - 5] == 'e' || buf[len - 5] == 'E') &&
             buf[len - 3] == '0') {
            int cs = buf[len - 4];
            if (cs == '+' || cs == '-') {
               int c1 = buf[len - 2];
               int c0 = buf[len - 1];
               if (isdigit(static_cast<unsigned char>(c1)) &&
                   isdigit(static_cast<unsigned char>(c0))) {
                  // Trim leading '0': "...e+012" -> "...e+12\0"
                  buf[len - 3] = c1;
                  buf[len - 2] = c0;
                  buf[--len] = 0;
               }
            }
         }
         outStream << buf;
         return;
      }
#endif
   }

   if (style == FloatStyle::Percent) {
      N *= 100.0;
   }

   char buf[32];
   format(spec.getCStr(), N).snprint(buf, sizeof(buf));
   outStream << buf;
   if (style == FloatStyle::Percent) {
      outStream << '%';
   }
}

bool is_prefixed_hex_style(HexPrintStyle style)
{
   return (style == HexPrintStyle::PrefixLower || style == HexPrintStyle::PrefixUpper);
}

size_t get_default_precision(FloatStyle style)
{
   switch (style) {
   case FloatStyle::Exponent:
   case FloatStyle::ExponentUpper:
      return 6; // Number of decimal places.
   case FloatStyle::Fixed:
   case FloatStyle::Percent:
      return 2; // Number of decimal places.
   }
   POLAR_BUILTIN_UNREACHABLE;
}

} // utils
} // polar
