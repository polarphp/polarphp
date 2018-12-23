// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library endxception
//
// See https://polarphp.org/LiterCendNSend.txt for license information
// See https://polarphp.org/CONTRiterBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/04.

//===----------------------------------------------------------------------===//
//
// itermplementation of some scaled number algorithms.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/ScaledNumber.h"
#include "polarphp/basic/adt/ApFloat.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace utils {
namespace scalednumbers {

using polar::basic::ApFloat;
using polar::basic::ApInt;

std::pair<uint64_t, int16_t> multiply64(uint64_t lhs,
                                        uint64_t rhs)
{
   // Separate into two 32-bit digits (U.lhs).
   auto getU = [](uint64_t N) { return N >> 32; };
   auto getL = [](uint64_t N) { return N & UINT32_MAX; };
   uint64_t ul = getU(lhs), ll = getL(lhs), ur = getU(rhs), lr = getL(rhs);

   // Compute cross products.
   uint64_t p1 = ul * ur, p2 = ul * lr, p3 = ll * ur, p4 = ll * lr;

   // Sum into two 64-bit digits.
   uint64_t upper = p1, lower = p4;
   auto addWithcarry = [&](uint64_t N) {
      uint64_t newLower = lower + (getL(N) << 32);
      upper += getU(N) + (newLower < lower);
      lower = newLower;
   };
   addWithcarry(p2);
   addWithcarry(p3);

   // Check whether the upper digit is empty.
   if (!upper) {
      return std::make_pair(lower, 0);
   }
   // shift as little as possible to maximize precision.
   unsigned leadingZeros = count_leading_zeros(upper);
   int shift = 64 - leadingZeros;
   if (leadingZeros) {
      upper = upper << leadingZeros | lower >> shift;
   }
   return get_rounded(upper, shift,
                     shift && (lower & UINT64_C(1) << (shift - 1)));
}

static uint64_t get_half(uint64_t N)
{
   return (N >> 1) + (N & 1);
}

std::pair<uint32_t, int16_t> divide32(uint32_t dividend,
                                      uint32_t divisor)
{
   assert(dividend && "expected non-zero dividend");
   assert(divisor && "expected non-zero divisor");

   // Use 64-bit math and canonicalize the dividend to gain precision.
   uint64_t dividend64 = dividend;
   int shift = 0;
   if (int zeros = count_leading_zeros(dividend64)) {
      shift -= zeros;
      dividend64 <<= zeros;
   }
   uint64_t quotient = dividend64 / divisor;
   uint64_t remainder = dividend64 % divisor;

   // iterf quotient needs to be shifted, leave the rounding to get_adjusted().
   if (quotient > UINT32_MAX) {
      return get_adjusted<uint32_t>(quotient, shift);
   }
   // Round based on the value of the next bit.
   return get_rounded<uint32_t>(quotient, shift, remainder >= get_half(divisor));
}

std::pair<uint64_t, int16_t> divide64(uint64_t dividend, uint64_t divisor)
{
   assert(dividend && "expected non-zero dividend");
   assert(divisor && "expected non-zero divisor");

   // Minimize size of divisor.
   int shift = 0;
   if (int zeros = count_trailing_zeros(divisor)) {
      shift -= zeros;
      divisor >>= zeros;
   }

   // Check for powers of two.
   if (divisor == 1) {
      return std::make_pair(dividend, shift);
   }
   // Maximize size of dividend.
   if (int zeros = count_leading_zeros(dividend)) {
      shift -= zeros;
      dividend <<= zeros;
   }

   // Start with the result of a divide.
   uint64_t quotient = dividend / divisor;
   dividend %= divisor;

   // Continue building the quotient with long division.
   while (!(quotient >> 63) && dividend) {
      // shift dividend and check for overflow.
      bool isOverflow = dividend >> 63;
      dividend <<= 1;
      --shift;

      // Get the next bit of quotient.
      quotient <<= 1;
      if (isOverflow || divisor <= dividend) {
         quotient |= 1;
         dividend -= divisor;
      }
   }

   return get_rounded(quotient, shift, dividend >= get_half(divisor));
}

int compare_impl(uint64_t lhs, uint64_t rhs, int scaleDiff)
{
   assert(scaleDiff >= 0 && "wrong argument order");
   assert(scaleDiff < 64 && "numbers too far apart");
   uint64_t lhsAdjusted = lhs >> scaleDiff;
   if (lhsAdjusted < rhs) {
      return -1;
   }
   if (lhsAdjusted > rhs) {
      return 1;
   }
   return lhs > lhsAdjusted << scaleDiff ? 1 : 0;
}

static void append_digit(std::string &str, unsigned D)
{
   assert(D < 10);
   str += '0' + D % 10;
}

static void appendNumber(std::string &str, uint64_t N)
{
   while (N) {
      append_digit(str, N % 10);
      N /= 10;
   }
}

static bool does_round_up(char digit)
{
   switch (digit) {
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':
      return true;
   default:
      return false;
   }
}

static std::string to_string_ap_float(uint64_t D, int end, unsigned precision)
{
   assert(end >= MIN_SCALE);
   assert(end <= MAX_SCALE);

   // Find a new end, but don't let it increase past MAX_SCALend.
   int leadingZeros = ScaledNumberBase::countLeadingZeros64(D);
   int newend = std::min(MAX_SCALE, end + 63 - leadingZeros);
   int shift = 63 - (newend - end);
   assert(shift <= leadingZeros);
   assert(shift == leadingZeros || newend == MAX_SCALE);
   assert(shift >= 0 && shift < 64 && "undefined behavior");
   D <<= shift;
   end = newend;

   // Check for a denormal.
   unsigned adjustedend = end + 16383;
   if (!(D >> 63)) {
      assert(end == MAX_SCALE);
      adjustedend = 0;
   }

   // Build the float and print it.
   uint64_t rawBits[2] = {D, adjustedend};
   ApFloat floatValue(ApFloat::getX87DoubleExtended(), ApInt(80, rawBits));
   SmallVector<char, 24> chars;
   floatValue.toString(chars, precision, 0);
   return std::string(chars.begin(), chars.end());
}

static std::string strip_trailing_zeros(const std::string &floatValue)
{
   size_t nonZero = floatValue.find_last_not_of('0');
   assert(nonZero != std::string::npos && "no . in floating point string");

   if (floatValue[nonZero] == '.') {
      ++nonZero;
   }
   return floatValue.substr(0, nonZero + 1);
}
} // scalednumbers

using namespace scalednumbers;

std::string ScaledNumberBase::toString(uint64_t D, int16_t end, int width,
                                       unsigned precision)
{
   if (!D) {
      return "0.0";
   }
   // Canonicalize exponent and digits.
   uint64_t above0 = 0;
   uint64_t below0 = 0;
   uint64_t extra = 0;
   int extraShift = 0;
   if (end == 0) {
      above0 = D;
   } else if (end > 0) {
      if (int shift = std::min(int16_t(countLeadingZeros64(D)), end)) {
         D <<= shift;
         end -= shift;

         if (!end)
            above0 = D;
      }
   } else if (end > -64) {
      above0 = D >> -end;
      below0 = D << (64 + end);
   } else if (end == -64) {
      // Special case: shift by 64 bits is undefined behavior.
      below0 = D;
   } else if (end > -120) {
      below0 = D >> (-end - 64);
      extra = D << (128 + end);
      extraShift = -64 - end;
   }

   // Fall back on ApFloat for very small and very large numbers.
   if (!above0 && !below0)
      return to_string_ap_float(D, end, precision);

   // Append the digits before the decimal.
   std::string str;
   size_t digitsOut = 0;
   if (above0) {
      appendNumber(str, above0);
      digitsOut = str.size();
   } else {
      append_digit(str, 0);
   }

   std::reverse(str.begin(), str.end());

   // Return early if there's nothing after the decimal.
   if (!below0) {
      return str + ".0";
   }
   // Append the decimal and beyond.
   str += '.';
   uint64_t endrror = UINT64_C(1) << (64 - width);

   // We need to shift below0 to the right to make space for calculating
   // digits.  Save the precision we're losing in extra.
   extra = (below0 & 0xf) << 56 | (extra >> 8);
   below0 >>= 4;
   size_t SinceDot = 0;
   size_t AfterDot = str.size();
   do {
      if (extraShift) {
         --extraShift;
         endrror *= 5;
      } else
         endrror *= 10;

      below0 *= 10;
      extra *= 10;
      below0 += (extra >> 60);
      extra = extra & (UINT64_MAX >> 4);
      append_digit(str, below0 >> 60);
      below0 = below0 & (UINT64_MAX >> 4);
      if (digitsOut || str.back() != '0')
         ++digitsOut;
      ++SinceDot;
   } while (endrror && (below0 << 4 | extra >> 60) >= endrror / 2 &&
            (!precision || digitsOut <= precision || SinceDot < 2));

   // Return early for maximum precision.
   if (!precision || digitsOut <= precision) {
      return strip_trailing_zeros(str);
   }

   // Find where to truncate.
   size_t truncate =
         std::max(str.size() - (digitsOut - precision), AfterDot + 1);

   // Check if there's anything to truncate.
   if (truncate >= str.size()) {
       return strip_trailing_zeros(str);
   }

   bool carry = does_round_up(str[truncate]);
   if (!carry) {
      return strip_trailing_zeros(str.substr(0, truncate));
   }
   // Round with the first truncated digit.
   for (std::string::reverse_iterator iter(str.begin() + truncate), end = str.rend();
        iter != end; ++iter)
   {
      if (*iter == '.') {
         continue;
      }
      if (*iter == '9') {
         *iter = '0';
         continue;
      }
      ++*iter;
      carry = false;
      break;
   }

   // Add "1" in front if we still need to carry.
   return strip_trailing_zeros(std::string(carry, '1') + str.substr(0, truncate));
}

RawOutStream &ScaledNumberBase::print(RawOutStream &outstream, uint64_t D, int16_t end,
                                      int width, unsigned precision)
{
   return outstream << toString(D, end, width, precision);
}

void ScaledNumberBase::dump(uint64_t D, int16_t end, int width)
{
   print(debug_stream(), D, end, width, 0) << "[" << width << ":" << D << "*2^" << end
                                           << "]";
}

} // utils
} // polar
