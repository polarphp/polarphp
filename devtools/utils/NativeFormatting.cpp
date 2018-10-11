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

#include "NativeFormatting.h"
#include "StringExtras.h"
#include "ArrayExtras.h"
#include "Format.h"
#include "DataTypes.h"

#include <string>
#include <vector>
#include <float.h>
#include <optional>

namespace polar {
namespace utils {

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

void write_with_commas(std::ostream &out, std::vector<char> buffer)
{
   assert(!buffer.empty());
   std::vector<char> thisGroup;
   int initialDigits = ((buffer.size() - 1) % 3) + 1;
   thisGroup = vector_take_front(buffer, initialDigits);
   out.write(thisGroup.data(), thisGroup.size());
   buffer = vector_drop_front(buffer, initialDigits);
   assert(buffer.size() % 3 == 0);
   while (!buffer.empty()) {
      out << ',';
      thisGroup = vector_take_front(buffer, 3);
      out.write(thisGroup.data(), 3);
      buffer = vector_drop_front(buffer, 3);
   }
}

template <typename T>
void write_unsigned_impl(std::ostream &out, T N, size_t minDigits,
                         IntegerStyle style, bool isNegative)
{
   static_assert(std::is_unsigned<T>::value, "Value is not unsigned!");
   char numberBuffer[128];
   std::memset(numberBuffer, '0', sizeof(numberBuffer));
   size_t len = 0;
   len = format_to_buffer(N, numberBuffer);
   if (isNegative) {
      out << '-';
   }
   if (len < minDigits && style != IntegerStyle::Number) {
      for (size_t index = len; index < minDigits; ++index) {
         out << '0';
      }
   }

   if (style == IntegerStyle::Number) {
      write_with_commas(out, std::vector<char>(std::end(numberBuffer) - len, std::end(numberBuffer)));
   } else {
      out.write(std::end(numberBuffer) - len, len);
   }
}

template <typename T>
void write_unsigned(std::ostream &out, T N, size_t minDigits,
                    IntegerStyle style, bool isNegative = false)
{
   // Output using 32-bit div/mod if possible.
   if (N == static_cast<uint32_t>(N)) {
      write_unsigned_impl(out, static_cast<uint32_t>(N), minDigits, style,
                          isNegative);
   } else {
      write_unsigned_impl(out, N, minDigits, style, isNegative);
   }
}

template <typename T>
void write_signed(std::ostream &out, T N, size_t minDigits,
                  IntegerStyle style)
{
   static_assert(std::is_signed<T>::value, "Value is not signed!");
   using UnsignedT = typename std::make_unsigned<T>::type;
   if (N >= 0) {
      write_unsigned(out, static_cast<UnsignedT>(N), minDigits, style);
      return;
   }

   UnsignedT unsignedValue = -(UnsignedT)N;
   write_unsigned(out, unsignedValue, minDigits, style, true);
}
} // anonymous namespace

void write_integer(std::ostream &out, unsigned int N, size_t minDigits,
                   IntegerStyle style)
{
   write_unsigned(out, N, minDigits, style);
}

void write_integer(std::ostream &out, int N, size_t minDigits,
                   IntegerStyle style)
{
   write_signed(out, N, minDigits, style);
}

void write_integer(std::ostream &out, unsigned long N, size_t minDigits,
                   IntegerStyle style)
{
   write_unsigned(out, N, minDigits, style);
}

void write_integer(std::ostream &out, long N, size_t minDigits,
                   IntegerStyle style)
{
   write_signed(out, N, minDigits, style);
}

void write_integer(std::ostream &out, unsigned long long N, size_t minDigits,
                   IntegerStyle style)
{
   write_unsigned(out, N, minDigits, style);
}

void write_integer(std::ostream &out, long long N, size_t minDigits,
                   IntegerStyle style)
{
   write_signed(out, N, minDigits, style);
}

//void write_hex(std::ostream &out, uint64_t N, HexPrintStyle style,
//               std::optional<std::size_t> width)
//{
//   const size_t kMaxWidth = 128u;

//   size_t w = std::min(kMaxWidth, width.value_or(0u));

//   unsigned nibbles = (64 - countLeadingZeros(N) + 3) / 4;
//   bool prefix = (style == HexPrintStyle::PrefixLower ||
//                  style == HexPrintStyle::PrefixUpper);
//   bool upper =
//         (style == HexPrintStyle::Upper || style == HexPrintStyle::PrefixUpper);
//   unsigned prefixChars = prefix ? 2 : 0;
//   unsigned numChars =
//         std::max(static_cast<unsigned>(w), std::max(1u, nibbles) + prefixChars);

//   char numberBuffer[kMaxWidth];
//   ::memset(numberBuffer, '0', array_lengthof(numberBuffer));
//   if (prefix)
//      numberBuffer[1] = 'x';
//   char *endPtr = numberBuffer + numChars;
//   char *curPtr = endPtr;
//   while (N) {
//      unsigned char x = static_cast<unsigned char>(N) % 16;
//      *--curPtr = hexdigit(x, !upper);
//      N /= 16;
//   }
//   out.write(numberBuffer, numChars);
//}

//void write_double(std::ostream &out, double N, FloatStyle style,
//                  Optional<size_t> Precision) {
//   size_t Prec = Precision.getValueOr(getDefaultPrecision(style));

//   if (std::isnan(N)) {
//      out << "nan";
//      return;
//   } else if (std::isinf(N)) {
//      out << "INF";
//      return;
//   }

//   char Letter;
//   if (style == FloatStyle::Exponent)
//      Letter = 'e';
//   else if (style == FloatStyle::ExponentUpper)
//      Letter = 'E';
//   else
//      Letter = 'f';

//   SmallString<8> Spec;
//   llvm::raw_svector_ostream Out(Spec);
//   Out << "%." << Prec << Letter;

//   if (style == FloatStyle::Exponent || style == FloatStyle::ExponentUpper) {
//#ifdef _WIN32
//      // On MSVCRT and compatible, output of %e is incompatible to Posix
//      // by default. Number of exponent digits should be at least 2. "%+03d"
//      // FIXME: Implement our formatter to here or Support/Format.h!
//#if defined(__MINGW32__)
//      // FIXME: It should be generic to C++11.
//      if (N == 0.0 && std::signbit(N)) {
//         char NegativeZero[] = "-0.000000e+00";
//         if (style == FloatStyle::ExponentUpper)
//            NegativeZero[strlen(NegativeZero) - 4] = 'E';
//         out << NegativeZero;
//         return;
//      }
//#else
//      int fpcl = _fpclass(N);

//      // negative zero
//      if (fpcl == _FPCLASS_NZ) {
//         char NegativeZero[] = "-0.000000e+00";
//         if (style == FloatStyle::ExponentUpper)
//            NegativeZero[strlen(NegativeZero) - 4] = 'E';
//         out << NegativeZero;
//         return;
//      }
//#endif

//      char buf[32];
//      unsigned len;
//      len = format(Spec.c_str(), N).snprint(buf, sizeof(buf));
//      if (len <= sizeof(buf) - 2) {
//         if (len >= 5 && (buf[len - 5] == 'e' || buf[len - 5] == 'E') &&
//             buf[len - 3] == '0') {
//            int cs = buf[len - 4];
//            if (cs == '+' || cs == '-') {
//               int c1 = buf[len - 2];
//               int c0 = buf[len - 1];
//               if (isdigit(static_cast<unsigned char>(c1)) &&
//                   isdigit(static_cast<unsigned char>(c0))) {
//                  // Trim leading '0': "...e+012" -> "...e+12\0"
//                  buf[len - 3] = c1;
//                  buf[len - 2] = c0;
//                  buf[--len] = 0;
//               }
//            }
//         }
//         out << buf;
//         return;
//      }
//#endif
//   }

//   if (style == FloatStyle::Percent)
//      N *= 100.0;

//   char Buf[32];
//   format(Spec.c_str(), N).snprint(Buf, sizeof(Buf));
//   out << Buf;
//   if (style == FloatStyle::Percent)
//      out << '%';
//}

//bool isPrefixedHexStyle(HexPrintStyle out) {
//   return (out == HexPrintStyle::PrefixLower || out == HexPrintStyle::PrefixUpper);
//}

//size_t getDefaultPrecision(FloatStyle style) {
//   switch (style) {
//   case FloatStyle::Exponent:
//   case FloatStyle::ExponentUpper:
//      return 6; // Number of decimal places.
//   case FloatStyle::Fixed:
//   case FloatStyle::Percent:
//      return 2; // Number of decimal places.
//   }
//   POLAR_BUILTIN_UNREACHABLE;
//}

} // utils
} // polar
