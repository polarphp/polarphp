// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/16.

//===----------------------------------------------------------------------===//
//
// This file implements a class to represent arbitrary precision floating
// point values and provide a variety of arithmetic operations on them.
//
//===----------------------------------------------------------------------===//

#include "polarphp/global/Global.h"
#include "polarphp/basic/adt/ApFloat.h"
#include "polarphp/basic/adt/ApSInt.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/FoldingSet.h"
#include "polarphp/basic/adt/Hashing.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/MathExtras.h"
#include "polarphp/utils/RawOutStream.h"
#include <cstring>
#include <limits.h>

namespace polar {
namespace basic {

using polar::utils::next_power_of_two;
using polar::debug_stream;

#define ApFloat_DISPATCH_ON_SEMANTICS(METHOD_CALL)                             \
   do {                                                                         \
   if (usesLayout<IEEEFloat>(getSemantics()))                                 \
   return m_storage.m_ieee.METHOD_CALL;                                               \
   if (usesLayout<DoubleApFloat>(getSemantics()))                             \
   return m_storage.m_dvalue.METHOD_CALL;                                             \
   polar_unreachable("Unexpected semantics");                              \
} while (false)


/// A macro used to combine two fcCategory enums into one key which can be used
/// in a switch statement to classify how the interaction of two ApFloat's
/// categories affects an operation.
///
/// TODO: If clang source code is ever allowed to use constexpr in its own
/// codebase, change this into a static inline function.
#define PackCategoriesIntoKey(_lhs, _rhs) ((polar::as_integer(_lhs)) * 4 + (polar::as_integer(_rhs)))

/* Assumed in hexadecimal significand parsing, and conversion to
   hexadecimal strings.  */
static_assert(ApFloatBase::sm_integerPartWidth % 4 == 0, "Part width must be divisible by 4!");

/* Represents floating point arithmetic semantics.  */
struct FltSemantics
{
   /* The largest E such that 2^E is representable; this matches the
       definition of IEEE 754.  */
   ApFloatBase::ExponentType m_maxExponent;

   /* The smallest E such that 2^E is a normalized number; this
       matches the definition of IEEE 754.  */
   ApFloatBase::ExponentType m_minExponent;

   /* Number of bits in the significand.  This includes the integer
       bit.  */
   unsigned int m_precision;

   /* Number of bits actually used in the semantics. */
   unsigned int m_sizeInBits;
};

static const FltSemantics sg_semIEEEhalf = {15, -14, 11, 16};
static const FltSemantics sg_semIEEEsingle = {127, -126, 24, 32};
static const FltSemantics sg_semIEEEdouble = {1023, -1022, 53, 64};
static const FltSemantics sg_semIEEEquad = {16383, -16382, 113, 128};
static const FltSemantics sg_semX87DoubleExtended = {16383, -16382, 64, 80};
static const FltSemantics sg_semBogus = {0, 0, 0, 0};

/* The IBM double-double semantics. Such a number consists of a pair of IEEE
     64-bit doubles (Hi, Lo), where |Hi| > |Lo|, and if normal,
     (double)(Hi + Lo) == Hi. The numeric value it's modeling is Hi + Lo.
     Therefore it has two 53-bit mantissa parts that aren't necessarily adjacent
     to each other, and two 11-bit exponents.
     Note: we need to make the value different from sg_semBogus as otherwise
     an unsafe optimization may collapse both values to a single address,
     and we heavily rely on them having distinct addresses.             */
static const FltSemantics sg_semPPCDoubleDouble = {-1, 0, 0, 0};

/* These are legacy semantics for the fallback, inaccrurate implementation of
     IBM double-double, if the accurate sg_semPPCDoubleDouble doesn't handle the
     operation. It's equivalent to having an IEEE number with consecutive 106
     bits of mantissa and 11 bits of exponent.
     It's not equivalent to IBM double-double. For example, a legit IBM
     double-double, 1 + epsilon:
       1 + epsilon = 1 + (1 >> 1076)
     is not representable by a consecutive 106 bits of mantissa.
     Currently, these semantics are used in the following way:
       sg_semPPCDoubleDouble -> (IEEEdouble, IEEEdouble) ->
       (64-bit APInt, 64-bit APInt) -> (128-bit APInt) ->
       sg_semPPCDoubleDoubleLegacy -> IEEE operations
     We use bitcastToApInt() to get the bit representation (in APInt) of the
     underlying IEEEdouble, then use the APInt constructor to construct the
     legacy IEEE float.
     TODO: Implement all operations in sg_semPPCDoubleDouble, and delete these
     semantics.  */
static const FltSemantics sg_semPPCDoubleDoubleLegacy = {1023, -1022 + 53,
                                                         53 + 53, 128};

const FltSemantics &ApFloatBase::getIEEEhalf()
{
   return sg_semIEEEhalf;
}

const FltSemantics &ApFloatBase::getIEEEsingle()
{
   return sg_semIEEEsingle;
}

const FltSemantics &ApFloatBase::getIEEEdouble()
{
   return sg_semIEEEdouble;
}

const FltSemantics &ApFloatBase::getIEEEquad()
{
   return sg_semIEEEquad;
}

const FltSemantics &ApFloatBase::getX87DoubleExtended()
{
   return sg_semX87DoubleExtended;
}

const FltSemantics &ApFloatBase::getBogus()
{
   return sg_semBogus;
}

const FltSemantics &ApFloatBase::getPPCDoubleDouble()
{
   return sg_semPPCDoubleDouble;
}

/* A tight upper bound on number of parts required to hold the value
     pow(5, power) is
       power * 815 / (351 * integerPartWidth) + 1
     However, whilst the result may require only this many parts,
     because we are multiplying two values to get it, the
     multiplication may require an extra part with the excess part
     being zero (consider the trivial case of 1 * 1, tcFullMultiply
     requires two parts to hold the single-part result).  So we add an
     extra one to guarantee enough space whilst multiplying.  */
const unsigned int g_maxExponent = 16383;
const unsigned int g_maxPrecision = 113;
const unsigned int g_maxPowerOfFiveExponent = g_maxExponent + g_maxPrecision - 1;
const unsigned int g_maxPowerOfFiveParts = 2 + ((g_maxPowerOfFiveExponent * 815) / (351 * ApFloatBase::sm_integerPartWidth));

unsigned int ApFloatBase::semanticsPrecision(const FltSemantics &semantics)
{
   return semantics.m_precision;
}

ApFloatBase::ExponentType
ApFloatBase::semanticsMaxExponent(const FltSemantics &semantics)
{
   return semantics.m_maxExponent;
}

ApFloatBase::ExponentType
ApFloatBase::semanticsMinExponent(const FltSemantics &semantics)
{
   return semantics.m_minExponent;
}

unsigned int ApFloatBase::semanticsSizeInBits(const FltSemantics &semantics)
{
   return semantics.m_sizeInBits;
}

unsigned ApFloatBase::getSizeInBits(const FltSemantics &semantics)
{
   return semantics.m_sizeInBits;
}

/* Zero at the end to avoid modular arithmetic when adding one; used
   when rounding up during hexadecimal output.  */
static const char sg_hexDigitsLower[] = "0123456789abcdef0";
static const char sg_hexDigitsUpper[] = "0123456789ABCDEF0";
static const char sg_infinityL[] = "infinity";
static const char sg_infinityU[] = "INFINITY";
static const char sg_NaNL[] = "nan";
static const char sg_NaNU[] = "NAN";

namespace {

/* A bunch of private, handy routines.  */

inline unsigned int part_count_for_bits(unsigned int bits)
{
   return ((bits) + ApFloatBase::sm_integerPartWidth - 1) / ApFloatBase::sm_integerPartWidth;
}

/* Returns 0U-9U.  Return values >= 10U are not digits.  */
inline unsigned int dec_digit_value(unsigned int c)
{
   return c - '0';
}

/* Return the value of a decimal exponent of the form
   [+-]ddddddd.
   If the exponent overflows, returns a large exponent with the
   appropriate sign.  */
int read_exponent(StringRef::iterator begin, StringRef::iterator end)
{
   bool isNegative;
   unsigned int absExponent;
   const unsigned int overlargeExponent = 24000;  /* FIXME.  */
   StringRef::iterator p = begin;

   assert(p != end && "Exponent has no digits");

   isNegative = (*p == '-');
   if (*p == '-' || *p == '+') {
      p++;
      assert(p != end && "Exponent has no digits");
   }

   absExponent = dec_digit_value(*p++);
   assert(absExponent < 10U && "Invalid character in exponent");

   for (; p != end; ++p) {
      unsigned int value;

      value = dec_digit_value(*p);
      assert(value < 10U && "Invalid character in exponent");

      value += absExponent * 10;
      if (absExponent >= overlargeExponent) {
         absExponent = overlargeExponent;
         p = end;  /* outwit assert below */
         break;
      }
      absExponent = value;
   }

   assert(p == end && "Invalid exponent in exponent");

   if (isNegative) {
      return -(int) absExponent;
   } else {
      return (int) absExponent;
   }
}

/* This is ugly and needs cleaning up, but I don't immediately see
   how whilst remaining safe.  */
int total_exponent(StringRef::iterator p, StringRef::iterator end,
                   int exponentAdjustment)
{
   int unsignedExponent;
   bool negative, overflow;
   int exponent = 0;

   assert(p != end && "Exponent has no digits");

   negative = *p == '-';
   if (*p == '-' || *p == '+') {
      p++;
      assert(p != end && "Exponent has no digits");
   }

   unsignedExponent = 0;
   overflow = false;
   for (; p != end; ++p) {
      unsigned int value;

      value = dec_digit_value(*p);
      assert(value < 10U && "Invalid character in exponent");

      unsignedExponent = unsignedExponent * 10 + value;
      if (unsignedExponent > 32767) {
         overflow = true;
         break;
      }
   }

   if (exponentAdjustment > 32767 || exponentAdjustment < -32768) {
      overflow = true;
   }
   if (!overflow) {
      exponent = unsignedExponent;
      if (negative) {
         exponent = -exponent;
      }

      exponent += exponentAdjustment;
      if (exponent > 32767 || exponent < -32768) {
         overflow = true;
      }
   }

   if (overflow) {
      exponent = negative ? -32768: 32767;
   }
   return exponent;
}

StringRef::iterator skip_leading_zeroes_and_any_dot(StringRef::iterator begin, StringRef::iterator end,
                                                    StringRef::iterator *dot)
{
   StringRef::iterator p = begin;
   *dot = end;
   while (p != end && *p == '0') {
      p++;
   }
   if (p != end && *p == '.') {
      *dot = p++;
      assert(end - begin != 1 && "Significand has no digits");
      while (p != end && *p == '0') {
         p++;
      }
   }
   return p;
}

/* Given a normal decimal floating point number of the form
     dddd.dddd[eE][+-]ddd
   where the decimal point and exponent are optional, fill out the
   structure D.  Exponent is appropriate if the significand is
   treated as an integer, and normalizedExponent if the significand
   is taken to have the decimal point after a single leading
   non-zero digit.
   If the value is zero, V->firstSigDigit points to a non-digit, and
   the return exponent is zero.
*/
struct DecimalInfo
{
   const char *m_firstSigDigit;
   const char *m_lastSigDigit;
   int m_exponent;
   int m_normalizedExponent;
};

void interpret_decimal(StringRef::iterator begin, StringRef::iterator end,
                       DecimalInfo *decimal)
{
   StringRef::iterator dot = end;
   StringRef::iterator p = skip_leading_zeroes_and_any_dot (begin, end, &dot);

   decimal->m_firstSigDigit = p;
   decimal->m_exponent = 0;
   decimal->m_normalizedExponent = 0;

   for (; p != end; ++p) {
      if (*p == '.') {
         assert(dot == end && "String contains multiple dots");
         dot = p++;
         if (p == end) {
            break;
         }

      }
      if (dec_digit_value(*p) >= 10U) {
         break;
      }
   }

   if (p != end) {
      assert((*p == 'e' || *p == 'E') && "Invalid character in significand");
      assert(p != begin && "Significand has no digits");
      assert((dot == end || p - begin != 1) && "Significand has no digits");

      /* p points to the first non-digit in the string */
      decimal->m_exponent = read_exponent(p + 1, end);

      /* Implied decimal point?  */
      if (dot == end) {
         dot = p;
      }
   }

   /* If number is all zeroes accept any exponent.  */
   if (p != decimal->m_firstSigDigit) {
      /* Drop insignificant trailing zeroes.  */
      if (p != begin) {
         do
         {
            do
            {
               p--;
            }
            while (p != begin && *p == '0');
         }
         while (p != begin && *p == '.');
      }

      /* Adjust the exponents for any decimal point.  */
      decimal->m_exponent += static_cast<ApFloat::ExponentType>((dot - p) - (dot > p));
      decimal->m_normalizedExponent = (decimal->m_exponent +
                                       static_cast<ApFloat::ExponentType>((p - decimal->m_firstSigDigit)
                                                                          - (dot > decimal->m_firstSigDigit && dot < p)));
   }

   decimal->m_lastSigDigit = p;
}

/* Return the trailing fraction of a hexadecimal number.
   DIGITVALUE is the first hex digit of the fraction, P points to
   the next digit.  */
LostFraction
trailing_hexa_decimal_fraction(StringRef::iterator p, StringRef::iterator end,
                               unsigned int digitValue)
{
   unsigned int hexDigit;

   /* If the first trailing digit isn't 0 or 8 we can work out the
     fraction immediately.  */
   if (digitValue > 8) {
      return LostFraction::lfMoreThanHalf;
   } else if (digitValue < 8 && digitValue > 0) {
      return LostFraction::lfLessThanHalf;
   }
   // Otherwise we need to find the first non-zero digit.
   while (p != end && (*p == '0' || *p == '.')) {
      p++;
   }
   assert(p != end && "Invalid trailing hexadecimal fraction!");
   hexDigit = hex_digit_value(*p);

   /* If we ran off the end it is exactly zero or one-half, otherwise
     a little more.  */
   if (hexDigit == -1U) {
      return digitValue == 0 ? LostFraction::lfExactlyZero: LostFraction::lfExactlyHalf;
   } else {
      return digitValue == 0 ? LostFraction::lfLessThanHalf: LostFraction::lfMoreThanHalf;
   }
}

/* Return the fraction lost were a bignum truncated losing the least
   significant BITS bits.  */
LostFraction lostfraction_through_truncation(const ApFloatBase::integerPart *parts,
                                             unsigned int partCount,
                                             unsigned int bits)
{
   unsigned int lsb;

   lsb = ApInt::tcLsb(parts, partCount);

   /* Note this is guaranteed true if bits == 0, or LSB == -1U.  */
   if (bits <= lsb) {
      return LostFraction::lfExactlyZero;
   }

   if (bits == lsb + 1) {
      return LostFraction::lfExactlyHalf;
   }

   if (bits <= partCount * ApFloatBase::sm_integerPartWidth &&
       ApInt::tcExtractBit(parts, bits - 1)) {
      return LostFraction::lfMoreThanHalf;
   }
   return LostFraction::lfLessThanHalf;
}

/* Shift DST right BITS bits noting lost fraction.  */
LostFraction shift_right(ApFloatBase::integerPart *dst, unsigned int parts, unsigned int bits)
{
   LostFraction lostFraction;
   lostFraction = lostfraction_through_truncation(dst, parts, bits);
   ApInt::tcShiftRight(dst, parts, bits);
   return lostFraction;
}

/* Combine the effect of two lost fractions.  */
LostFraction combine_lost_fractions(LostFraction moreSignificant,
                                    LostFraction lessSignificant)
{
   if (lessSignificant != LostFraction::lfExactlyZero) {
      if (moreSignificant == LostFraction::lfExactlyZero) {
         moreSignificant = LostFraction::lfLessThanHalf;
      } else if (moreSignificant == LostFraction::lfExactlyHalf) {
         moreSignificant = LostFraction::lfMoreThanHalf;
      }
   }
   return moreSignificant;
}

/* The error from the true value, in half-ulps, on multiplying two
   floating point numbers, which differ from the value they
   approximate by at most HUE1 and HUE2 half-ulps, is strictly less
   than the returned value.
   See "How to Read Floating Point Numbers Accurately" by William D
   Clinger.  */
unsigned int huerr_bound(bool inexactMultiply, unsigned int huerr1, unsigned int huerr2)
{
   assert(huerr1 < 2 || huerr2 < 2 || (huerr1 + huerr2 < 8));

   if (huerr1 + huerr2 == 0) {
      return inexactMultiply * 2;  /* <= inexactMultiply half-ulps.  */
   } else {
      return inexactMultiply + 2 * (huerr1 + huerr2);
   }

}

/* The number of ulps from the boundary (zero, or half if ISNEAREST)
   when the least significant BITS are truncated.  BITS cannot be
   zero.  */
ApFloatBase::integerPart ulps_from_boundary(const ApFloatBase::integerPart *parts, unsigned int bits,
                                            bool isNearest) {
   unsigned int count, partBits;
   ApFloatBase::integerPart part, boundary;

   assert(bits != 0);

   bits--;
   count = bits / ApFloatBase::sm_integerPartWidth;
   partBits = bits % ApFloatBase::sm_integerPartWidth + 1;

   part = parts[count] & (~(ApFloatBase::integerPart) 0 >> (ApFloatBase::sm_integerPartWidth - partBits));

   if (isNearest) {
      boundary = (ApFloatBase::integerPart) 1 << (partBits - 1);
   } else {
      boundary = 0;
   }

   if (count == 0) {
      if (part - boundary <= boundary - part) {
         return part - boundary;
      } else {
         return boundary - part;
      }

   }

   if (part == boundary) {
      while (--count)
         if (parts[count])
            return ~(ApFloatBase::integerPart) 0; /* A lot.  */

      return parts[0];
   } else if (part == boundary - 1) {
      while (--count) {
         if (~parts[count]) {
            return ~(ApFloatBase::integerPart) 0; /* A lot.  */
         }
      }
      return -parts[0];
   }

   return ~(ApFloatBase::integerPart) 0; /* A lot.  */
}

/* Place pow(5, power) in DST, and return the number of parts used.
   DST must be at least one part larger than size of the answer.  */
unsigned int powerOf5(ApFloatBase::integerPart *dst, unsigned int power)
{
   static const ApFloatBase::integerPart firstEightPowers[] = { 1, 5, 25, 125, 625, 3125, 15625, 78125 };
   ApFloatBase::integerPart pow5s[g_maxPowerOfFiveParts * 2 + 5];
   pow5s[0] = 78125 * 5;

   unsigned int partsCount[16] = { 1 };
   ApFloatBase::integerPart scratch[g_maxPowerOfFiveParts], *p1, *p2, *pow5;
   unsigned int result;
   assert(power <= g_maxExponent);

   p1 = dst;
   p2 = scratch;

   *p1 = firstEightPowers[power & 7];
   power >>= 3;

   result = 1;
   pow5 = pow5s;

   for (unsigned int n = 0; power; power >>= 1, n++) {
      unsigned int pc;

      pc = partsCount[n];

      /* Calculate pow(5,pow(2,n+3)) if we haven't yet.  */
      if (pc == 0) {
         pc = partsCount[n - 1];
         ApInt::tcFullMultiply(pow5, pow5 - pc, pow5 - pc, pc, pc);
         pc *= 2;
         if (pow5[pc - 1] == 0) {
            pc--;
         }
         partsCount[n] = pc;
      }

      if (power & 1) {
         ApFloatBase::integerPart *tmp;

         ApInt::tcFullMultiply(p2, p1, pow5, result, pc);
         result += pc;
         if (p2[result - 1] == 0) {
            result--;
         }
         /* Now result is in p1 with partsCount parts and p2 is scratch
         space.  */
         tmp = p1;
         p1 = p2;
         p2 = tmp;
      }

      pow5 += pc;
   }

   if (p1 != dst) {
      ApInt::tcAssign(dst, p1, result);
   }
   return result;
}

/* Write out an integerPart in hexadecimal, starting with the most
   significant nibble.  Write out exactly COUNT hexdigits, return
   COUNT.  */
unsigned int part_as_hex (char *dst, ApFloatBase::integerPart part, unsigned int count,
                          const char *hexDigitChars)
{
   unsigned int result = count;

   assert(count != 0 && count <= ApFloatBase::sm_integerPartWidth / 4);

   part >>= (ApFloatBase::sm_integerPartWidth - 4 * count);
   while (count--) {
      dst[count] = hexDigitChars[part & 0xf];
      part >>= 4;
   }

   return result;
}

/* Write out an unsigned decimal integer.  */
char *write_unsigned_decimal (char *dst, unsigned int n)
{
   char buff[40], *p;
   p = buff;
   do {
      *p++ = '0' + n % 10;
   } while (n /= 10);

   do {
      *dst++ = *--p;
   } while (p != buff);
   return dst;
}

/* Write out a signed decimal integer.  */
char *write_signed_decimal (char *dst, int value)
{
   if (value < 0) {
      *dst++ = '-';
      dst = write_unsigned_decimal(dst, -(unsigned) value);
   } else {
      dst = write_unsigned_decimal(dst, value);
   }
   return dst;
}

} // anonymous namespace

namespace internal {
/* Constructors.  */
void IEEEFloat::initialize(const FltSemantics *ourSemantics)
{
   unsigned int count;
   m_semantics = ourSemantics;
   count = getPartCount();
   if (count > 1) {
      m_significand.m_parts = new integerPart[count];
   }
}

void IEEEFloat::freeSignificand()
{
   if (needsCleanup()) {
      delete [] m_significand.m_parts;
   }
}

void IEEEFloat::assign(const IEEEFloat &other)
{
   assert(m_semantics == other.m_semantics);

   m_sign = other.m_sign;
   m_category = other.m_category;
   m_exponent = other.m_exponent;
   if (isFiniteNonZero() || m_category == FltCategory::fcNaN) {
      copySignificand(other);
   }
}

void IEEEFloat::copySignificand(const IEEEFloat &other)
{
   assert(isFiniteNonZero() || m_category == FltCategory::fcNaN);
   assert(other.getPartCount() >= getPartCount());
   ApInt::tcAssign(getSignificandParts(), other.getSignificandParts(),
                   getPartCount());
}

/* Make this number a NaN, with an arbitrary but deterministic value
   for the significand.  If double or longer, this is a signalling NaN,
   which may not be ideal.  If float, this is QNaN(0).  */
void IEEEFloat::makeNaN(bool snan, bool negative, const ApInt *fill)
{
   m_category = FltCategory::fcNaN;
   m_sign = negative;

   integerPart *significand = getSignificandParts();
   unsigned numParts = getPartCount();

   // Set the significand bits to the fill.
   if (!fill || fill->getNumWords() < numParts)
      ApInt::tcSet(significand, 0, numParts);
   if (fill) {
      ApInt::tcAssign(significand, fill->getRawData(),
                      std::min(fill->getNumWords(), numParts));

      // Zero out the excess bits of the significand.
      unsigned bitsToPreserve = m_semantics->m_precision - 1;
      unsigned part = bitsToPreserve / 64;
      bitsToPreserve %= 64;
      significand[part] &= ((1ULL << bitsToPreserve) - 1);
      for (part++; part != numParts; ++part) {
         significand[part] = 0;
      }
   }

   unsigned qNaNBit = m_semantics->m_precision - 2;

   if (snan) {
      // We always have to clear the QNaN bit to make it an SNaN.
      ApInt::tcClearBit(significand, qNaNBit);

      // If there are no bits set in the payload, we have to set
      // *something* to make it a NaN instead of an infinity;
      // conventionally, this is the next bit down from the QNaN bit.
      if (ApInt::tcIsZero(significand, numParts)) {
         ApInt::tcSetBit(significand, qNaNBit - 1);
      }
   } else {
      // We always have to set the QNaN bit to make it a QNaN.
      ApInt::tcSetBit(significand, qNaNBit);
   }

   // For x87 extended precision, we want to make a NaN, not a
   // pseudo-NaN.  Maybe we should expose the ability to make
   // pseudo-NaNs?
   if (m_semantics == &sg_semX87DoubleExtended) {
      ApInt::tcSetBit(significand, qNaNBit + 1);
   }

}

IEEEFloat &IEEEFloat::operator=(const IEEEFloat &other)
{
   if (this != &other) {
      if (m_semantics != other.m_semantics) {
         freeSignificand();
         initialize(other.m_semantics);
      }
      assign(other);
   }
   return *this;
}

IEEEFloat &IEEEFloat::operator=(IEEEFloat &&other)
{
   freeSignificand();
   m_semantics = other.m_semantics;
   m_significand = other.m_significand;
   m_exponent = other.m_exponent;
   m_category = other.m_category;
   m_sign = other.m_sign;

   other.m_semantics = &sg_semBogus;
   return *this;
}

bool IEEEFloat::isDenormal() const
{
   return isFiniteNonZero() && (m_exponent == m_semantics->m_minExponent) &&
         (ApInt::tcExtractBit(getSignificandParts(),
                              m_semantics->m_precision - 1) == 0);
}

bool IEEEFloat::isSmallest() const
{
   // The smallest number by magnitude in our format will be the smallest
   // denormal, i.e. the floating point number with exponent being minimum
   // exponent and significand bitwise equal to 1 (i.e. with MSB equal to 0).
   return isFiniteNonZero() && m_exponent == m_semantics->m_minExponent &&
         getSignificandMsb() == 0;
}

bool IEEEFloat::isSignificandAllOnes() const
{
   // Test if the significand excluding the integral bit is all ones. This allows
   // us to test for binade boundaries.
   const integerPart *parts = getSignificandParts();
   const unsigned partCount = getPartCount();
   for (unsigned i = 0; i < partCount - 1; i++) {
      if (~parts[i]) {
         return false;
      }
   }
   // Set the unused high bits to all ones when we compare.
   const unsigned numHighBits =
         partCount*sm_integerPartWidth - m_semantics->m_precision + 1;
   assert(numHighBits <= sm_integerPartWidth && "Can not have more high bits to "
                                                "fill than integerPartWidth");
   const integerPart highBitFill =
         ~integerPart(0) << (sm_integerPartWidth - numHighBits);
   if (~(parts[partCount - 1] | highBitFill)) {
      return false;
   }
   return true;
}

bool IEEEFloat::isSignificandAllZeros() const
{
   // Test if the significand excluding the integral bit is all zeros. This
   // allows us to test for binade boundaries.
   const integerPart *parts = getSignificandParts();
   const unsigned partCount = getPartCount();

   for (unsigned i = 0; i < partCount - 1; i++)
   {
      if (parts[i]) {
         return false;
      }
   }

   const unsigned numHighBits =
         partCount*sm_integerPartWidth - m_semantics->m_precision + 1;
   assert(numHighBits <= sm_integerPartWidth && "Can not have more high bits to "
                                                "clear than integerPartWidth");
   const integerPart highBitMask = ~integerPart(0) >> numHighBits;

   if (parts[partCount - 1] & highBitMask) {
      return false;
   }
   return true;
}

bool IEEEFloat::isLargest() const
{
   // The largest number by magnitude in our format will be the floating point
   // number with maximum exponent and with significand that is all ones.
   return isFiniteNonZero() && m_exponent == m_semantics->m_maxExponent
         && isSignificandAllOnes();
}

bool IEEEFloat::isInteger() const
{
   // This could be made more efficient; I'm going for obviously correct.
   if (!isFinite()) {
      return false;
   }
   IEEEFloat truncated = *this;
   truncated.roundToIntegral(RoundingMode::rmTowardZero);
   return compare(truncated) == CmpResult::cmpEqual;
}

bool IEEEFloat::bitwiseIsEqual(const IEEEFloat &other) const
{
   if (this == &other) {
      return true;
   }
   if (m_semantics != other.m_semantics ||
       m_category != other.m_category ||
       m_sign != other.m_sign) {
      return false;
   }
   if (m_category == FltCategory::fcZero || m_category == FltCategory::fcInfinity) {
      return true;
   }
   if (isFiniteNonZero() && m_exponent != other.m_exponent) {
      return false;
   }
   return std::equal(getSignificandParts(), getSignificandParts() + getPartCount(),
                     other.getSignificandParts());
}

IEEEFloat::IEEEFloat(const FltSemantics &ourSemantics, integerPart value)
{
   initialize(&ourSemantics);
   m_sign = 0;
   m_category = FltCategory::fcNormal;
   zeroSignificand();
   m_exponent = ourSemantics.m_precision - 1;
   getSignificandParts()[0] = value;
   normalize(RoundingMode::rmNearestTiesToEven, LostFraction::lfExactlyZero);
}

IEEEFloat::IEEEFloat(const FltSemantics &ourSemantics)
{
   initialize(&ourSemantics);
   m_category = FltCategory::fcZero;
   m_sign = false;
}

// Delegate to the previous constructor, because later copy constructor may
// actually inspects category, which can't be garbage.
IEEEFloat::IEEEFloat(const FltSemantics &ourSemantics, UninitializedTag tag)
   : IEEEFloat(ourSemantics)
{}

IEEEFloat::IEEEFloat(const IEEEFloat &other)
{
   initialize(other.m_semantics);
   assign(other);
}

IEEEFloat::IEEEFloat(IEEEFloat &&other)
   : m_semantics(&sg_semBogus)
{
   *this = std::move(other);
}

IEEEFloat::~IEEEFloat()
{
   freeSignificand();
}

unsigned int IEEEFloat::getPartCount() const
{
   return part_count_for_bits(m_semantics->m_precision + 1);
}

const IEEEFloat::integerPart *IEEEFloat::getSignificandParts() const
{
   return const_cast<IEEEFloat *>(this)->getSignificandParts();
}

IEEEFloat::integerPart *IEEEFloat::getSignificandParts()
{
   if (getPartCount() > 1) {
      return m_significand.m_parts;
   } else {
      return &m_significand.m_part;
   }
}

void IEEEFloat::zeroSignificand()
{
   ApInt::tcSet(getSignificandParts(), 0, getPartCount());
}

/* Increment an fcNormal floating point number's significand.  */
void IEEEFloat::incrementSignificand()
{
   integerPart carry;

   carry = ApInt::tcIncrement(getSignificandParts(), getPartCount());

   /* Our callers should never cause us to overflow.  */
   assert(carry == 0);
   (void)carry;
}

/* Add the significand of the RHS.  Returns the carry flag.  */
IEEEFloat::integerPart IEEEFloat::addSignificand(const IEEEFloat &other)
{
   integerPart *parts;
   parts = getSignificandParts();
   assert(m_semantics == other.m_semantics);
   assert(m_exponent == other.m_exponent);
   return ApInt::tcAdd(parts, other.getSignificandParts(), 0, getPartCount());
}

/* Subtract the significand of the RHS with a borrow flag.  Returns
   the borrow flag.  */
IEEEFloat::integerPart IEEEFloat::subtractSignificand(const IEEEFloat &other,
                                                      integerPart borrow)
{
   integerPart *parts;
   parts = getSignificandParts();
   assert(m_semantics == other.m_semantics);
   assert(m_exponent == other.m_exponent);
   return ApInt::tcSubtract(parts, other.getSignificandParts(), borrow,
                            getPartCount());
}

/* Multiply the significand of the RHS.  If ADDEND is non-NULL, add it
   on to the full-precision result of the multiplication.  Returns the
   lost fraction.  */
LostFraction IEEEFloat::multiplySignificand(const IEEEFloat &other,
                                            const IEEEFloat *addend)
{
   unsigned int omsb;        // One, not zero, based MSB.
   unsigned int partsCount, newPartsCount, precision;
   integerPart *lhsSignificand;
   integerPart scratch[4];
   integerPart *fullSignificand;
   LostFraction lostFraction;
   bool ignored;

   assert(m_semantics == other.m_semantics);

   precision = m_semantics->m_precision;

   // Allocate space for twice as many bits as the original significand, plus one
   // extra bit for the addition to overflow into.
   newPartsCount = part_count_for_bits(precision * 2 + 1);

   if (newPartsCount > 4) {
      fullSignificand = new integerPart[newPartsCount];
   } else {
      fullSignificand = scratch;
   }

   lhsSignificand = getSignificandParts();
   partsCount = getPartCount();

   ApInt::tcFullMultiply(fullSignificand, lhsSignificand,
                         other.getSignificandParts(), partsCount, partsCount);

   lostFraction = LostFraction::lfExactlyZero;
   omsb = ApInt::tcMsb(fullSignificand, newPartsCount) + 1;
   m_exponent += other.m_exponent;

   // Assume the operands involved in the multiplication are single-precision
   // FP, and the two multiplicants are:
   //   *this = a23 . a22 ... a0 * 2^e1
   //     rhs = b23 . b22 ... b0 * 2^e2
   // the result of multiplication is:
   //   *this = c48 c47 c46 . c45 ... c0 * 2^(e1+e2)
   // Note that there are three significant bits at the left-hand side of the
   // radix point: two for the multiplication, and an overflow bit for the
   // addition (that will always be zero at this point). Move the radix point
   // toward left by two bits, and adjust exponent accordingly.
   m_exponent += 2;

   if (addend && addend->isNonZero()) {
      // The intermediate result of the multiplication has "2 * precision"
      // signicant bit; adjust the addend to be consistent with mul result.
      //
      Significand savedSignificand = m_significand;
      const FltSemantics *savedSemantics = m_semantics;
      FltSemantics extendedSemantics;
      OpStatus status;
      unsigned int extendedPrecision;

      // Normalize our MSB to one below the top bit to allow for overflow.
      extendedPrecision = 2 * precision + 1;
      if (omsb != extendedPrecision - 1) {
         assert(extendedPrecision > omsb);
         ApInt::tcShiftLeft(fullSignificand, newPartsCount,
                            (extendedPrecision - 1) - omsb);
         m_exponent -= (extendedPrecision - 1) - omsb;
      }

      /* Create new semantics.  */
      extendedSemantics = *m_semantics;
      extendedSemantics.m_precision = extendedPrecision;

      if (newPartsCount == 1) {
         m_significand.m_part = fullSignificand[0];
      } else {
         m_significand.m_parts = fullSignificand;
      }

      m_semantics = &extendedSemantics;

      IEEEFloat extendedAddend(*addend);
      status = extendedAddend.convert(extendedSemantics, RoundingMode::rmTowardZero, &ignored);
      assert(status == OpStatus::opOK);
      (void)status;

      // Shift the significand of the addend right by one bit. This guarantees
      // that the high bit of the significand is zero (same as fullSignificand),
      // so the addition will overflow (if it does overflow at all) into the top bit.
      lostFraction = extendedAddend.shiftSignificandRight(1);
      assert(lostFraction == LostFraction::lfExactlyZero &&
             "Lost precision while shifting addend for fused-multiply-add.");

      lostFraction = addOrSubtractSignificand(extendedAddend, false);

      /* Restore our state.  */
      if (newPartsCount == 1) {
         fullSignificand[0] = m_significand.m_part;
      }

      m_significand = savedSignificand;
      m_semantics = savedSemantics;

      omsb = ApInt::tcMsb(fullSignificand, newPartsCount) + 1;
   }

   // Convert the result having "2 * precision" significant-bits back to the one
   // having "precision" significant-bits. First, move the radix point from
   // poision "2*precision - 1" to "precision - 1". The exponent need to be
   // adjusted by "2*precision - 1" - "precision - 1" = "precision".
   m_exponent -= precision + 1;

   // In case MSB resides at the left-hand side of radix point, shift the
   // mantissa right by some amount to make sure the MSB reside right before
   // the radix point (i.e. "MSB . rest-significant-bits").
   //
   // Note that the result is not normalized when "omsb < precision". So, the
   // caller needs to call IEEEFloat::normalize() if normalized value is
   // expected.
   if (omsb > precision) {
      unsigned int bits, significantParts;
      LostFraction lf;

      bits = omsb - precision;
      significantParts = part_count_for_bits(omsb);
      lf = shift_right(fullSignificand, significantParts, bits);
      lostFraction = combine_lost_fractions(lf, lostFraction);
      m_exponent += bits;
   }

   ApInt::tcAssign(lhsSignificand, fullSignificand, partsCount);

   if (newPartsCount > 4) {
      delete [] fullSignificand;
   }

   return lostFraction;
}

/* Multiply the significands of LHS and other to DST.  */
LostFraction IEEEFloat::divideSignificand(const IEEEFloat &other)
{
   unsigned int bit, i, partsCount;
   const integerPart *otherSignificand;
   integerPart *lhsSignificand, *dividend, *divisor;
   integerPart scratch[4];
   LostFraction lostFraction;

   assert(m_semantics == other.m_semantics);

   lhsSignificand = getSignificandParts();
   otherSignificand = other.getSignificandParts();
   partsCount = getPartCount();

   if (partsCount > 2) {
      dividend = new integerPart[partsCount * 2];
   } else {
      dividend = scratch;
   }
   divisor = dividend + partsCount;

   /* Copy the dividend and divisor as they will be modified in-place.  */
   for (i = 0; i < partsCount; i++) {
      dividend[i] = lhsSignificand[i];
      divisor[i] = otherSignificand[i];
      lhsSignificand[i] = 0;
   }

   m_exponent -= other.m_exponent;

   unsigned int precision = m_semantics->m_precision;

   /* Normalize the divisor.  */
   bit = precision - ApInt::tcMsb(divisor, partsCount) - 1;
   if (bit) {
      m_exponent += bit;
      ApInt::tcShiftLeft(divisor, partsCount, bit);
   }

   /* Normalize the dividend.  */
   bit = precision - ApInt::tcMsb(dividend, partsCount) - 1;
   if (bit) {
      m_exponent -= bit;
      ApInt::tcShiftLeft(dividend, partsCount, bit);
   }

   /* Ensure the dividend >= divisor initially for the loop below.
     Incidentally, this means that the division loop below is
     guaranteed to set the integer bit to one.  */
   if (ApInt::tcCompare(dividend, divisor, partsCount) < 0) {
      m_exponent--;
      ApInt::tcShiftLeft(dividend, partsCount, 1);
      assert(ApInt::tcCompare(dividend, divisor, partsCount) >= 0);
   }

   /* Long division.  */
   for (bit = precision; bit; bit -= 1) {
      if (ApInt::tcCompare(dividend, divisor, partsCount) >= 0) {
         ApInt::tcSubtract(dividend, divisor, 0, partsCount);
         ApInt::tcSetBit(lhsSignificand, bit - 1);
      }

      ApInt::tcShiftLeft(dividend, partsCount, 1);
   }

   /* Figure out the lost fraction.  */
   int cmp = ApInt::tcCompare(dividend, divisor, partsCount);

   if (cmp > 0) {
      lostFraction = LostFraction::lfMoreThanHalf;
   } else if (cmp == 0) {
      lostFraction = LostFraction::lfExactlyHalf;
   } else if (ApInt::tcIsZero(dividend, partsCount)) {
      lostFraction = LostFraction::lfExactlyZero;
   } else {
      lostFraction = LostFraction::lfLessThanHalf;
   }

   if (partsCount > 2) {
      delete [] dividend;
   }
   return lostFraction;
}

unsigned int IEEEFloat::getSignificandMsb() const
{
   return ApInt::tcMsb(getSignificandParts(), getPartCount());
}

unsigned int IEEEFloat::getSignificandLsb() const
{
   return ApInt::tcLsb(getSignificandParts(), getPartCount());
}

/* Note that a zero result is NOT normalized to fcZero.  */
LostFraction IEEEFloat::shiftSignificandRight(unsigned int bits)
{
   /* Our exponent should not overflow.  */
   assert((ExponentType) (m_exponent + bits) >= m_exponent);
   m_exponent += bits;
   return shift_right(getSignificandParts(), getPartCount(), bits);
}

/* Shift the significand left BITS bits, subtract BITS from its exponent.  */
void IEEEFloat::shiftSignificandLeft(unsigned int bits)
{
   assert(bits < m_semantics->m_precision);
   if (bits) {
      unsigned int partsCount = getPartCount();
      ApInt::tcShiftLeft(getSignificandParts(), partsCount, bits);
      m_exponent -= bits;
      assert(!ApInt::tcIsZero(getSignificandParts(), partsCount));
   }
}

IEEEFloat::CmpResult
IEEEFloat::compareAbsoluteValue(const IEEEFloat &other) const
{
   int compare;

   assert(m_semantics == other.m_semantics);
   assert(isFiniteNonZero());
   assert(other.isFiniteNonZero());

   compare = m_exponent - other.m_exponent;

   /* If exponents are equal, do an unsigned bignum comparison of the
     significands.  */
   if (compare == 0) {
      compare = ApInt::tcCompare(getSignificandParts(), other.getSignificandParts(),
                                 getPartCount());
   }

   if (compare > 0) {
      return CmpResult::cmpGreaterThan;
   } else if (compare < 0) {
      return CmpResult::cmpLessThan;
   } else {
      return CmpResult::cmpEqual;
   }
}

/* Handle overflow.  Sign is preserved.  We either become infinity or
   the largest finite number.  */
IEEEFloat::OpStatus IEEEFloat::handleOverflow(RoundingMode roundingMode)
{
   /* Infinity?  */
   if (roundingMode == RoundingMode::rmNearestTiesToEven ||
       roundingMode ==  RoundingMode::rmNearestTiesToAway ||
       (roundingMode ==  RoundingMode::rmTowardPositive && !m_sign) ||
       (roundingMode ==  RoundingMode::rmTowardNegative && m_sign)) {
      m_category = FltCategory::fcInfinity;
      return (OpStatus) (OpStatus::opOverflow | OpStatus::opInexact);
   }

   /* Otherwise we become the largest finite number.  */
   m_category = FltCategory::fcNormal;
   m_exponent = m_semantics->m_maxExponent;
   ApInt::tcSetLeastSignificantBits(getSignificandParts(), getPartCount(),
                                    m_semantics->m_precision);

   return opInexact;
}

/* Returns TRUE if, when truncating the current number, with BIT the
   new LSB, with the given lost fraction and rounding mode, the result
   would need to be rounded away from zero (i.e., by increasing the
   signficand).  This routine must work for fcZero of both signs, and
   fcNormal numbers.  */
bool IEEEFloat::roundAwayFromZero(RoundingMode roundingMode,
                                  LostFraction lostFraction,
                                  unsigned int bit) const
{
   /* NaNs and infinities should not have lost fractions.  */
   assert(isFiniteNonZero() || m_category == FltCategory::fcZero);

   /* Current callers never pass this so we don't handle it.  */
   assert(lostFraction != LostFraction::lfExactlyZero);

   switch (roundingMode) {
   case RoundingMode::rmNearestTiesToAway:
      return lostFraction == LostFraction::lfExactlyHalf || lostFraction == LostFraction::lfMoreThanHalf;

   case RoundingMode::rmNearestTiesToEven:
      if (lostFraction == LostFraction::lfMoreThanHalf)
         return true;

      /* Our zeroes don't have a significand to test.  */
      if (lostFraction == LostFraction::lfExactlyHalf && m_category != FltCategory::fcZero) {
         return ApInt::tcExtractBit(getSignificandParts(), bit);
      }
      return false;

   case RoundingMode::rmTowardZero:
      return false;

   case RoundingMode::rmTowardPositive:
      return !m_sign;

   case RoundingMode::rmTowardNegative:
      return m_sign;
   }
   polar_unreachable("Invalid rounding mode found");
}

IEEEFloat::OpStatus IEEEFloat::normalize(RoundingMode roundingMode,
                                         LostFraction lostFraction)
{
   unsigned int omsb;                /* One, not zero, based MSB.  */
   int exponentChange;

   if (!isFiniteNonZero()) {
      return opOK;
   }

   /* Before rounding normalize the exponent of fcNormal numbers.  */
   omsb = getSignificandMsb() + 1;

   if (omsb) {
      /* OMSB is numbered from 1.  We want to place it in the integer
       bit numbered PRECISION if possible, with a compensating change in
       the exponent.  */
      exponentChange = omsb - m_semantics->m_precision;

      /* If the resulting exponent is too high, overflow according to
       the rounding mode.  */
      if (m_exponent + exponentChange > m_semantics->m_maxExponent) {
         return handleOverflow(roundingMode);
      }
      /* Subnormal numbers have exponent minExponent, and their MSB
       is forced based on that.  */
      if (m_exponent + exponentChange < m_semantics->m_minExponent) {
         exponentChange = m_semantics->m_minExponent - m_exponent;
      }
      /* Shifting left is easy as we don't lose precision.  */
      if (exponentChange < 0) {
         assert(lostFraction == LostFraction::lfExactlyZero);
         shiftSignificandLeft(-exponentChange);
         return opOK;
      }

      if (exponentChange > 0) {
         LostFraction lf;

         /* Shift right and capture any new lost fraction.  */
         lf = shiftSignificandRight(exponentChange);

         lostFraction = combine_lost_fractions(lf, lostFraction);

         /* Keep OMSB up-to-date.  */
         if (omsb > (unsigned) exponentChange) {
            omsb -= exponentChange;
         } else {
            omsb = 0;
         }
      }
   }

   /* Now round the number according to roundingMode given the lost
     fraction.  */

   /* As specified in IEEE 754, since we do not trap we do not report
     underflow for exact results.  */
   if (lostFraction == LostFraction::lfExactlyZero) {
      /* Canonicalize zeroes.  */
      if (omsb == 0) {
         m_category = FltCategory::fcZero;
      }
      return opOK;
   }

   /* Increment the significand if we're rounding away from zero.  */
   if (roundAwayFromZero(roundingMode, lostFraction, 0)) {
      if (omsb == 0) {
         m_exponent = m_semantics->m_minExponent;
      }
      incrementSignificand();
      omsb = getSignificandMsb() + 1;

      /* Did the significand increment overflow?  */
      if (omsb == (unsigned) m_semantics->m_precision + 1) {
         /* Renormalize by incrementing the exponent and shifting our
         significand right one.  However if we already have the
         maximum exponent we overflow to infinity.  */
         if (m_exponent == m_semantics->m_maxExponent) {
            m_category = FltCategory::fcInfinity;
            return (OpStatus) (opOverflow | opInexact);
         }
         shiftSignificandRight(1);
         return opInexact;
      }
   }

   /* The normal case - we were and are not denormal, and any
     significand increment above didn't overflow.  */
   if (omsb == m_semantics->m_precision) {
      return opInexact;
   }

   /* We have a non-zero denormal.  */
   assert(omsb < m_semantics->m_precision);

   /* Canonicalize zeroes.  */
   if (omsb == 0) {
      m_category = FltCategory::fcZero;
   }

   /* The fcZero case is a denormal that underflowed to zero.  */
   return (OpStatus) (opUnderflow | opInexact);
}

IEEEFloat::OpStatus IEEEFloat::addOrSubtractSpecials(const IEEEFloat &other,
                                                     bool subtract)
{
   switch (PackCategoriesIntoKey(m_category, other.m_category)) {
   default:
      polar_unreachable(nullptr);

   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcZero):
   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcNormal):
   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcInfinity):
   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcNaN):
   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcZero):
   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcNormal):
   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcZero):
      return opOK;

   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcNaN):
   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcNaN):
   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcNaN):
      // We need to be sure to flip the sign here for subtraction because we
      // don't have a separate negate operation so -NaN becomes 0 - NaN here.
      m_sign = other.m_sign ^ subtract;
      m_category = FltCategory::fcNaN;
      copySignificand(other);
      return opOK;

   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcInfinity):
   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcInfinity):
      m_category = FltCategory::fcInfinity;
      m_sign = other.m_sign ^ subtract;
      return opOK;

   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcNormal):
      assign(other);
      m_sign = other.m_sign ^ subtract;
      return opOK;

   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcZero):
      /* Sign depends on rounding mode; handled by caller.  */
      return opOK;

   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcInfinity):
      /* Differently signed infinities can only be validly
       subtracted.  */
      if (((m_sign ^ other.m_sign)!=0) != subtract) {
         makeNaN();
         return opInvalidOp;
      }

      return opOK;

   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcNormal):
      return opDivByZero;
   }
}

/* Add or subtract two normal numbers.  */
LostFraction IEEEFloat::addOrSubtractSignificand(const IEEEFloat &other,
                                                 bool subtract)
{
   integerPart carry;
   LostFraction lostFraction;
   int bits;

   /* Determine if the operation on the absolute values is effectively
     an addition or subtraction.  */
   subtract ^= static_cast<bool>(m_sign ^ other.m_sign);

   /* Are we bigger exponent-wise than the other?  */
   bits = m_exponent - other.m_exponent;

   /* Subtraction is more subtle than one might naively expect.  */
   if (subtract) {
      IEEEFloat tempOther(other);
      bool reverse;

      if (bits == 0) {
         reverse = compareAbsoluteValue(tempOther) == CmpResult::cmpLessThan;
         lostFraction = LostFraction::lfExactlyZero;
      } else if (bits > 0) {
         lostFraction = tempOther.shiftSignificandRight(bits - 1);
         shiftSignificandLeft(1);
         reverse = false;
      } else {
         lostFraction = shiftSignificandRight(-bits - 1);
         tempOther.shiftSignificandLeft(1);
         reverse = true;
      }

      if (reverse) {
         carry = tempOther.subtractSignificand
               (*this, lostFraction != LostFraction::lfExactlyZero);
         copySignificand(tempOther);
         m_sign = !m_sign;
      } else {
         carry = subtractSignificand
               (tempOther, lostFraction != LostFraction::lfExactlyZero);
      }

      /* Invert the lost fraction - it was on the other and
       subtracted.  */
      if (lostFraction == LostFraction::lfLessThanHalf) {
         lostFraction = LostFraction::lfMoreThanHalf;
      } else if (lostFraction == LostFraction::lfMoreThanHalf) {
         lostFraction = LostFraction::lfLessThanHalf;
      }
      /* The code above is intended to ensure that no borrow is
       necessary.  */
      assert(!carry);
      (void)carry;
   } else {
      if (bits > 0) {
         IEEEFloat tempOther(other);

         lostFraction = tempOther.shiftSignificandRight(bits);
         carry = addSignificand(tempOther);
      } else {
         lostFraction = shiftSignificandRight(-bits);
         carry = addSignificand(other);
      }

      /* We have a guard bit; generating a carry cannot happen.  */
      assert(!carry);
      (void)carry;
   }

   return lostFraction;
}

IEEEFloat::OpStatus IEEEFloat::multiplySpecials(const IEEEFloat &other)
{
   switch (PackCategoriesIntoKey(m_category, other.m_category)) {
   default:
      polar_unreachable(nullptr);

   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcZero):
   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcNormal):
   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcInfinity):
   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcNaN):
      m_sign = false;
      return opOK;

   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcNaN):
   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcNaN):
   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcNaN):
      m_sign = false;
      m_category = FltCategory::fcNaN;
      copySignificand(other);
      return opOK;

   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcInfinity):
   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcNormal):
   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcInfinity):
      m_category = FltCategory::fcInfinity;
      return opOK;

   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcNormal):
   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcZero):
   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcZero):
      m_category = FltCategory::fcZero;
      return opOK;

   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcInfinity):
   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcZero):
      makeNaN();
      return opInvalidOp;

   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcNormal):
      return opOK;
   }
}

IEEEFloat::OpStatus IEEEFloat::divideSpecials(const IEEEFloat &other)
{
   switch (PackCategoriesIntoKey(m_category, other.m_category)) {
   default:
      polar_unreachable(nullptr);

   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcNaN):
   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcNaN):
   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcNaN):
      m_category = FltCategory::fcNaN;
      copySignificand(other);
      POLAR_FALLTHROUGH;
   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcZero):
   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcNormal):
   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcInfinity):
   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcNaN):
      m_sign = false;
      POLAR_FALLTHROUGH;
   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcZero):
   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcNormal):
   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcInfinity):
   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcNormal):
      return opOK;

   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcInfinity):
      m_category = FltCategory::fcZero;
      return opOK;

   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcZero):
      m_category = FltCategory::fcInfinity;
      return opDivByZero;

   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcInfinity):
   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcZero):
      makeNaN();
      return opInvalidOp;

   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcNormal):
      return opOK;
   }
}

IEEEFloat::OpStatus IEEEFloat::modSpecials(const IEEEFloat &other)
{
   switch (PackCategoriesIntoKey(m_category, other.m_category)) {
   default:
      polar_unreachable(nullptr);

   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcZero):
   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcNormal):
   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcInfinity):
   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcNaN):
   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcInfinity):
   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcNormal):
   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcInfinity):
      return opOK;

   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcNaN):
   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcNaN):
   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcNaN):
      m_sign = false;
      m_category = FltCategory::fcNaN;
      copySignificand(other);
      return opOK;

   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcZero):
   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcZero):
   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcNormal):
   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcInfinity):
   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcZero):
      makeNaN();
      return opInvalidOp;

   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcNormal):
      return opOK;
   }
}

/* Change sign.  */
void IEEEFloat::changeSign()
{
   /* Look mummy, this one's easy.  */
   m_sign = !m_sign;
}

/* Normalized addition or subtraction.  */
IEEEFloat::OpStatus IEEEFloat::addOrSubtract(const IEEEFloat &other,
                                             RoundingMode roundingMode,
                                             bool subtract) {
   OpStatus fs;

   fs = addOrSubtractSpecials(other, subtract);

   /* This return code means it was not a simple case.  */
   if (fs == opDivByZero) {
      LostFraction lostFraction;

      lostFraction = addOrSubtractSignificand(other, subtract);
      fs = normalize(roundingMode, lostFraction);

      /* Can only be zero if we lost no fraction.  */
      assert(m_category != FltCategory::fcZero || lostFraction == LostFraction::lfExactlyZero);
   }

   /* If two numbers add (exactly) to zero, IEEE 754 decrees it is a
     positive zero unless rounding to minus infinity, except that
     adding two like-signed zeroes gives that zero.  */
   if (m_category == FltCategory::fcZero) {
      if (other.m_category != FltCategory::fcZero || (m_sign == other.m_sign) == subtract) {
         m_sign = (roundingMode == RoundingMode::rmTowardNegative);
      }
   }
   return fs;
}

/* Normalized addition.  */
IEEEFloat::OpStatus IEEEFloat::add(const IEEEFloat &other,
                                   RoundingMode roundingMode)
{
   return addOrSubtract(other, roundingMode, false);
}

/* Normalized subtraction.  */
IEEEFloat::OpStatus IEEEFloat::subtract(const IEEEFloat &other,
                                        RoundingMode roundingMode)
{
   return addOrSubtract(other, roundingMode, true);
}

/* Normalized multiply.  */
IEEEFloat::OpStatus IEEEFloat::multiply(const IEEEFloat &other,
                                        RoundingMode roundingMode)
{
   OpStatus fs;

   m_sign ^= other.m_sign;
   fs = multiplySpecials(other);

   if (isFiniteNonZero()) {
      LostFraction lostFraction = multiplySignificand(other, nullptr);
      fs = normalize(roundingMode, lostFraction);
      if (lostFraction != LostFraction::lfExactlyZero) {
         fs = (OpStatus) (fs | opInexact);
      }
   }
   return fs;
}

/* Normalized divide.  */
IEEEFloat::OpStatus IEEEFloat::divide(const IEEEFloat &other,
                                      RoundingMode roundingMode)
{
   OpStatus fs;

   m_sign ^= other.m_sign;
   fs = divideSpecials(other);

   if (isFiniteNonZero()) {
      LostFraction lostFraction = divideSignificand(other);
      fs = normalize(roundingMode, lostFraction);
      if (lostFraction != LostFraction::lfExactlyZero) {
         fs = (OpStatus) (fs | opInexact);
      }
   }

   return fs;
}

/* Normalized remainder.  This is not currently correct in all cases.  */
IEEEFloat::OpStatus IEEEFloat::remainder(const IEEEFloat &other)
{
   OpStatus fs;
   IEEEFloat value = *this;
   unsigned int origSign = m_sign;

   fs = value.divide(other, RoundingMode::rmNearestTiesToEven);
   if (fs == opDivByZero) {
      return fs;
   }
   int parts = getPartCount();
   integerPart *x = new integerPart[parts];
   bool ignored;
   fs = value.convertToInteger(make_mutable_array_ref(x, parts),
                               parts * sm_integerPartWidth, true, RoundingMode::rmNearestTiesToEven,
                               &ignored);
   if (fs == opInvalidOp) {
      delete[] x;
      return fs;
   }

   fs = value.convertFromZeroExtendedInteger(x, parts * sm_integerPartWidth, true,
                                             RoundingMode::rmNearestTiesToEven);
   assert(fs == opOK);   // should always work
   fs = value.multiply(other, RoundingMode::rmNearestTiesToEven);
   assert(fs == opOK || fs == opInexact);   // should not overflow or underflow
   fs = subtract(value, RoundingMode::rmNearestTiesToEven);
   assert(fs == opOK || fs== opInexact);   // likewise
   if (isZero()) {
      m_sign = origSign;    // IEEE754 requires this
   }
   delete[] x;
   return fs;
}

/* Normalized llvm frem (C fmod). */
IEEEFloat::OpStatus IEEEFloat::mod(const IEEEFloat &other)
{
   OpStatus fs;
   fs = modSpecials(other);
   unsigned int origSign = m_sign;

   while (isFiniteNonZero() && other.isFiniteNonZero() &&
          compareAbsoluteValue(other) != CmpResult::cmpLessThan) {
      IEEEFloat value = scalbn(other, ilogb(*this) - ilogb(other), RoundingMode::rmNearestTiesToEven);
      if (compareAbsoluteValue(value) == CmpResult::cmpLessThan) {
         value = scalbn(value, -1, RoundingMode::rmNearestTiesToEven);
      }
      value.m_sign = m_sign;
      fs = subtract(value, RoundingMode::rmNearestTiesToEven);
      assert(fs == opOK);
   }
   if (isZero()) {
      m_sign = origSign; // fmod requires this
   }
   return fs;
}

/* Normalized fused-multiply-add.  */
IEEEFloat::OpStatus IEEEFloat::fusedMultiplyAdd(const IEEEFloat &multiplicand,
                                                const IEEEFloat &addend,
                                                RoundingMode roundingMode)
{
   OpStatus fs;

   /* Post-multiplication sign, before addition.  */
   m_sign ^= multiplicand.m_sign;

   /* If and only if all arguments are normal do we need to do an
     extended-precision calculation.  */
   if (isFiniteNonZero() &&
       multiplicand.isFiniteNonZero() &&
       addend.isFinite()) {
      LostFraction lostFraction;

      lostFraction = multiplySignificand(multiplicand, &addend);
      fs = normalize(roundingMode, lostFraction);
      if (lostFraction != LostFraction::lfExactlyZero) {
         fs = (OpStatus) (fs | opInexact);
      }
      /* If two numbers add (exactly) to zero, IEEE 754 decrees it is a
       positive zero unless rounding to minus infinity, except that
       adding two like-signed zeroes gives that zero.  */
      if (m_category == FltCategory::fcZero && !(fs & opUnderflow) && m_sign != addend.m_sign) {
         m_sign = (roundingMode == RoundingMode::rmTowardNegative);
      }

   } else {
      fs = multiplySpecials(multiplicand);

      /* FS can only be opOK or opInvalidOp.  There is no more work
       to do in the latter case.  The IEEE-754R standard says it is
       implementation-defined in this case whether, if ADDEND is a
       quiet NaN, we raise invalid op; this implementation does so.
       If we need to do the addition we can do so with normal
       precision.  */
      if (fs == opOK) {
         fs = addOrSubtract(addend, roundingMode, false);
      }
   }

   return fs;
}

/* Rounding-mode corrrect round to integral value.  */
IEEEFloat::OpStatus IEEEFloat::roundToIntegral(RoundingMode roundingMode)
{
   OpStatus fs;

   // If the exponent is large enough, we know that this value is already
   // integral, and the arithmetic below would potentially cause it to saturate
   // to +/-Inf.  Bail out early instead.
   if (isFiniteNonZero() && m_exponent+1 >= (int)semanticsPrecision(*m_semantics)) {
      return opOK;
   }
   // The algorithm here is quite simple: we add 2^(p-1), where p is the
   // precision of our format, and then subtract it back off again.  The choice
   // of rounding modes for the addition/subtraction determines the rounding mode
   // for our integral rounding as well.
   // NOTE: When the input value is negative, we do subtraction followed by
   // addition instead.
   ApInt integerConstant(next_power_of_two(semanticsPrecision(*m_semantics)), 1);
   integerConstant <<= semanticsPrecision(*m_semantics)-1;
   IEEEFloat magicConstant(*m_semantics);
   fs = magicConstant.convertFromApInt(integerConstant, false,
                                       RoundingMode::rmNearestTiesToEven);
   magicConstant.m_sign = m_sign;

   if (fs != opOK) {
      return fs;
   }

   // Preserve the input sign so that we can handle 0.0/-0.0 cases correctly.
   bool inputSign = isNegative();

   fs = add(magicConstant, roundingMode);
   if (fs != opOK && fs != opInexact) {
      return fs;
   }
   fs = subtract(magicConstant, roundingMode);

   // Restore the input sign.
   if (inputSign != isNegative()) {
      changeSign();
   }
   return fs;
}

/* Comparison requires normalized numbers.  */
IEEEFloat::CmpResult IEEEFloat::compare(const IEEEFloat &other) const
{
   CmpResult result;

   assert(m_semantics == other.m_semantics);

   switch (PackCategoriesIntoKey(m_category, other.m_category)) {
   default:
      polar_unreachable(nullptr);

   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcZero):
   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcNormal):
   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcInfinity):
   case PackCategoriesIntoKey(FltCategory::fcNaN, FltCategory::fcNaN):
   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcNaN):
   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcNaN):
   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcNaN):
      return CmpResult::cmpUnordered;

   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcNormal):
   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcZero):
   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcZero):
      if (m_sign) {
         return CmpResult::cmpLessThan;
      } else {
         return CmpResult::cmpGreaterThan;
      }

   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcInfinity):
   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcInfinity):
   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcNormal):
      if (other.m_sign) {
         return CmpResult::cmpGreaterThan;
      } else {
         return CmpResult::cmpLessThan;
      }

   case PackCategoriesIntoKey(FltCategory::fcInfinity, FltCategory::fcInfinity):
      if (m_sign == other.m_sign) {
         return CmpResult::cmpEqual;
      } else if (m_sign) {
         return CmpResult::cmpLessThan;
      } else {
         return CmpResult::cmpGreaterThan;
      }

   case PackCategoriesIntoKey(FltCategory::fcZero, FltCategory::fcZero):
      return CmpResult::cmpEqual;

   case PackCategoriesIntoKey(FltCategory::fcNormal, FltCategory::fcNormal):
      break;
   }

   /* Two normal numbers.  Do they have the same sign?  */
   if (m_sign != other.m_sign) {
      if (m_sign) {
         result = CmpResult::cmpLessThan;
      } else {
         result = CmpResult::cmpGreaterThan;
      }
   } else {
      /* Compare absolute values; invert result if negative.  */
      result = compareAbsoluteValue(other);

      if (m_sign) {
         if (result == CmpResult::cmpLessThan) {
            result = CmpResult::cmpGreaterThan;
         } else if (result == CmpResult::cmpGreaterThan) {
            result = CmpResult::cmpLessThan;
         }
      }
   }

   return result;
}

/// IEEEFloat::convert - convert a value of one floating point type to another.
/// The return value corresponds to the IEEE754 exceptions.  *losesInfo
/// records whether the transformation lost information, i.e. whether
/// converting the result back to the original type will produce the
/// original value (this is almost the same as return value==fsOK, but there
/// are edge cases where this is not so).

IEEEFloat::OpStatus IEEEFloat::convert(const FltSemantics &toSemantics,
                                       RoundingMode roundingMode,
                                       bool *losesInfo)
{
   LostFraction lostFraction;
   unsigned int newPartCount, oldPartCount;
   OpStatus fs;
   int shift;
   const FltSemantics &fromSemantics = *m_semantics;

   lostFraction = LostFraction::lfExactlyZero;
   newPartCount = part_count_for_bits(toSemantics.m_precision + 1);
   oldPartCount = getPartCount();
   shift = toSemantics.m_precision - fromSemantics.m_precision;

   bool x86SpecialNan = false;
   if (&fromSemantics == &sg_semX87DoubleExtended &&
       &toSemantics != &sg_semX87DoubleExtended && m_category == FltCategory::fcNaN &&
       (!(*getSignificandParts() & 0x8000000000000000ULL) ||
        !(*getSignificandParts() & 0x4000000000000000ULL))) {
      // x86 has some unusual NaNs which cannot be represented in any other
      // format; note them here.
      x86SpecialNan = true;
   }

   // If this is a truncation of a denormal number, and the target semantics
   // has larger exponent range than the source semantics (this can happen
   // when truncating from PowerPC double-double to double format), the
   // right shift could lose result mantissa bits.  Adjust exponent instead
   // of performing excessive shift.
   if (shift < 0 && isFiniteNonZero()) {
      int exponentChange = getSignificandMsb() + 1 - fromSemantics.m_precision;
      if (m_exponent + exponentChange < toSemantics.m_minExponent) {
         exponentChange = toSemantics.m_minExponent - m_exponent;
      }

      if (exponentChange < shift) {
         exponentChange = shift;
      }

      if (exponentChange < 0) {
         shift -= exponentChange;
         m_exponent += exponentChange;
      }
   }

   // If this is a truncation, perform the shift before we narrow the storage.
   if (shift < 0 && (isFiniteNonZero() || m_category == FltCategory::fcNaN)) {
      lostFraction = shift_right(getSignificandParts(), oldPartCount, -shift);
   }

   // Fix the storage so it can hold to new value.
   if (newPartCount > oldPartCount) {
      // The new type requires more storage; make it available.
      integerPart *newParts;
      newParts = new integerPart[newPartCount];
      ApInt::tcSet(newParts, 0, newPartCount);
      if (isFiniteNonZero() || m_category == FltCategory::fcNaN) {
         ApInt::tcAssign(newParts, getSignificandParts(), oldPartCount);
      }
      freeSignificand();
      m_significand.m_parts = newParts;
   } else if (newPartCount == 1 && oldPartCount != 1) {
      // Switch to built-in storage for a single part.
      integerPart newPart = 0;
      if (isFiniteNonZero() || m_category == FltCategory::fcNaN) {
         newPart = getSignificandParts()[0];
      }
      freeSignificand();
      m_significand.m_part = newPart;
   }

   // Now that we have the right storage, switch the semantics.
   m_semantics = &toSemantics;

   // If this is an extension, perform the shift now that the storage is
   // available.
   if (shift > 0 && (isFiniteNonZero() || m_category == FltCategory::fcNaN)) {
      ApInt::tcShiftLeft(getSignificandParts(), newPartCount, shift);
   }
   if (isFiniteNonZero()) {
      fs = normalize(roundingMode, lostFraction);
      *losesInfo = (fs != opOK);
   } else if (m_category == FltCategory::fcNaN) {
      *losesInfo = lostFraction != LostFraction::lfExactlyZero || x86SpecialNan;

      // For x87 extended precision, we want to make a NaN, not a special NaN if
      // the input wasn't special either.
      if (!x86SpecialNan && m_semantics == &sg_semX87DoubleExtended)
         ApInt::tcSetBit(getSignificandParts(), m_semantics->m_precision - 1);

      // gcc forces the Quiet bit on, which means (float)(double)(float_sNan)
      // does not give you back the same bits.  This is dubious, and we
      // don't currently do it.  You're really supposed to get
      // an invalid operation signal at runtime, but nobody does that.
      fs = opOK;
   } else {
      *losesInfo = false;
      fs = opOK;
   }

   return fs;
}

/* Convert a floating point number to an integer according to the
   rounding mode.  If the rounded integer value is out of range this
   returns an invalid operation exception and the contents of the
   destination parts are unspecified.  If the rounded value is in
   range but the floating point number is not the exact integer, the C
   standard doesn't require an inexact exception to be raised.  IEEE
   854 does require it so we do that.
   Note that for conversions to integer type the C standard requires
   round-to-zero to always be used.  */
IEEEFloat::OpStatus IEEEFloat::convertToSignExtendedInteger(
      MutableArrayRef<integerPart> parts, unsigned int width, bool isSigned,
      RoundingMode roundingMode, bool *isExact) const
{
   LostFraction lostFraction;
   const integerPart *src;
   unsigned int dstPartsCount, truncatedBits;

   *isExact = false;

   /* Handle the three special cases first.  */
   if (m_category == FltCategory::fcInfinity || m_category == FltCategory::fcNaN) {
      return opInvalidOp;
   }
   dstPartsCount = part_count_for_bits(width);
   assert(dstPartsCount <= parts.getSize() && "Integer too big");
   if (m_category == FltCategory::fcZero) {
      ApInt::tcSet(parts.getData(), 0, dstPartsCount);
      // Negative zero can't be represented as an int.
      *isExact = !m_sign;
      return opOK;
   }

   src = getSignificandParts();

   /* Step 1: place our absolute value, with any fraction truncated, in
     the destination.  */
   if (m_exponent < 0) {
      /* Our absolute value is less than one; truncate everything.  */
      ApInt::tcSet(parts.getData(), 0, dstPartsCount);
      /* For exponent -1 the integer bit represents .5, look at that.
       For smaller exponents leftmost truncated bit is 0. */
      truncatedBits = m_semantics->m_precision -1U - m_exponent;
   } else {
      /* We want the most significant (exponent + 1) bits; the rest are
       truncated.  */
      unsigned int bits = m_exponent + 1U;

      /* Hopelessly large in magnitude?  */
      if (bits > width)
         return opInvalidOp;

      if (bits < m_semantics->m_precision) {
         /* We truncate (semantics->precision - bits) bits.  */
         truncatedBits = m_semantics->m_precision - bits;
         ApInt::tcExtract(parts.getData(), dstPartsCount, src, bits, truncatedBits);
      } else {
         /* We want at least as many bits as are available.  */
         ApInt::tcExtract(parts.getData(), dstPartsCount, src, m_semantics->m_precision,
                          0);
         ApInt::tcShiftLeft(parts.getData(), dstPartsCount,
                            bits - m_semantics->m_precision);
         truncatedBits = 0;
      }
   }

   /* Step 2: work out any lost fraction, and increment the absolute
     value if we would round away from zero.  */
   if (truncatedBits) {
      lostFraction = lostfraction_through_truncation(src, getPartCount(),
                                                     truncatedBits);
      if (lostFraction != LostFraction::lfExactlyZero &&
          roundAwayFromZero(roundingMode, lostFraction, truncatedBits)) {
         if (ApInt::tcIncrement(parts.getData(), dstPartsCount)) {
            return opInvalidOp;     /* Overflow.  */
         }
      }
   } else {
      lostFraction = LostFraction::lfExactlyZero;
   }

   /* Step 3: check if we fit in the destination.  */
   unsigned int omsb = ApInt::tcMsb(parts.getData(), dstPartsCount) + 1;

   if (m_sign) {
      if (!isSigned) {
         /* Negative numbers cannot be represented as unsigned.  */
         if (omsb != 0) {
            return opInvalidOp;
         }
      } else {
         /* It takes omsb bits to represent the unsigned integer value.
         We lose a bit for the sign, but care is needed as the
         maximally negative integer is a special case.  */
         if (omsb == width &&
             ApInt::tcLsb(parts.getData(), dstPartsCount) + 1 != omsb) {
            return opInvalidOp;
         }

         /* This case can happen because of rounding.  */
         if (omsb > width) {
            return opInvalidOp;
         }

      }

      ApInt::tcNegate (parts.getData(), dstPartsCount);
   } else {
      if (omsb >= width + !isSigned) {
         return opInvalidOp;
      }
   }

   if (lostFraction == LostFraction::lfExactlyZero) {
      *isExact = true;
      return opOK;
   } else {
      return opInexact;
   }
}

/* Same as convertToSignExtendedInteger, except we provide
   deterministic values in case of an invalid operation exception,
   namely zero for NaNs and the minimal or maximal value respectively
   for underflow or overflow.
   The *isExact output tells whether the result is exact, in the sense
   that converting it back to the original floating point type produces
   the original value.  This is almost equivalent to result==opOK,
   except for negative zeroes.
*/
IEEEFloat::OpStatus
IEEEFloat::convertToInteger(MutableArrayRef<integerPart> parts,
                            unsigned int width, bool isSigned,
                            RoundingMode roundingMode, bool *isExact) const
{
   OpStatus fs;

   fs = convertToSignExtendedInteger(parts, width, isSigned, roundingMode,
                                     isExact);

   if (fs == opInvalidOp) {
      unsigned int bits, dstPartsCount;

      dstPartsCount = part_count_for_bits(width);
      assert(dstPartsCount <= parts.getSize() && "Integer too big");

      if (m_category == FltCategory::fcNaN) {
         bits = 0;
      } else if (m_sign) {
         bits = isSigned;
      } else {
         bits = width - isSigned;
      }
      ApInt::tcSetLeastSignificantBits(parts.getData(), dstPartsCount, bits);
      if (m_sign && isSigned) {
         ApInt::tcShiftLeft(parts.getData(), dstPartsCount, width - 1);
      }
   }
   return fs;
}

/* Convert an unsigned integer SRC to a floating point number,
   rounding according to roundingMode.  The sign of the floating
   point number is not modified.  */
IEEEFloat::OpStatus IEEEFloat::convertFromUnsignedParts(
      const integerPart *src, unsigned int srcCount, RoundingMode roundingMode)
{
   unsigned int omsb, precision, dstCount;
   integerPart *dst;
   LostFraction lostFraction;

   m_category = FltCategory::fcNormal;
   omsb = ApInt::tcMsb(src, srcCount) + 1;
   dst = getSignificandParts();
   dstCount = getPartCount();
   precision = m_semantics->m_precision;

   /* We want the most significant PRECISION bits of SRC.  There may not
     be that many; extract what we can.  */
   if (precision <= omsb) {
      m_exponent = omsb - 1;
      lostFraction = lostfraction_through_truncation(src, srcCount,
                                                     omsb - precision);
      ApInt::tcExtract(dst, dstCount, src, precision, omsb - precision);
   } else {
      m_exponent = precision - 1;
      lostFraction = LostFraction::lfExactlyZero;
      ApInt::tcExtract(dst, dstCount, src, omsb, 0);
   }
   return normalize(roundingMode, lostFraction);
}

IEEEFloat::OpStatus IEEEFloat::convertFromApInt(const ApInt &value, bool isSigned,
                                                RoundingMode roundingMode)
{
   unsigned int partCount = value.getNumWords();
   ApInt api = value;

   m_sign = false;
   if (isSigned && api.isNegative()) {
      m_sign = true;
      api = -api;
   }

   return convertFromUnsignedParts(api.getRawData(), partCount, roundingMode);
}

/* Convert a two's complement integer SRC to a floating point number,
   rounding according to roundingMode.  ISSIGNED is true if the
   integer is signed, in which case it must be sign-extended.  */
IEEEFloat::OpStatus
IEEEFloat::convertFromSignExtendedInteger(const integerPart *src,
                                          unsigned int srcCount, bool isSigned,
                                          RoundingMode roundingMode)
{
   OpStatus status;

   if (isSigned &&
       ApInt::tcExtractBit(src, srcCount * sm_integerPartWidth - 1)) {
      integerPart *copy;

      /* If we're signed and negative negate a copy.  */
      m_sign = true;
      copy = new integerPart[srcCount];
      ApInt::tcAssign(copy, src, srcCount);
      ApInt::tcNegate(copy, srcCount);
      status = convertFromUnsignedParts(copy, srcCount, roundingMode);
      delete [] copy;
   } else {
      m_sign = false;
      status = convertFromUnsignedParts(src, srcCount, roundingMode);
   }

   return status;
}

/* FIXME: should this just take a const APInt reference?  */
IEEEFloat::OpStatus
IEEEFloat::convertFromZeroExtendedInteger(const integerPart *parts,
                                          unsigned int width, bool isSigned,
                                          RoundingMode roundingMode)
{
   unsigned int partCount = part_count_for_bits(width);
   ApInt api = ApInt(width, make_array_ref(parts, partCount));

   m_sign = false;
   if (isSigned && ApInt::tcExtractBit(parts, width - 1)) {
      m_sign = true;
      api = -api;
   }

   return convertFromUnsignedParts(api.getRawData(), partCount, roundingMode);
}

IEEEFloat::OpStatus
IEEEFloat::convertFromHexadecimalString(StringRef s,
                                        RoundingMode roundingMode)
{
   LostFraction lostFraction = LostFraction::lfExactlyZero;

   m_category = FltCategory::fcNormal;
   zeroSignificand();
   m_exponent = 0;

   integerPart *significand = getSignificandParts();
   unsigned partsCount = getPartCount();
   unsigned bitPos = partsCount * sm_integerPartWidth;
   bool computedTrailingFraction = false;

   // Skip leading zeroes and any (hexa)decimal point.
   StringRef::iterator begin = s.begin();
   StringRef::iterator end = s.end();
   StringRef::iterator dot;
   StringRef::iterator p = skip_leading_zeroes_and_any_dot(begin, end, &dot);
   StringRef::iterator firstSignificantDigit = p;

   while (p != end) {
      integerPart hex_value;

      if (*p == '.') {
         assert(dot == end && "String contains multiple dots");
         dot = p++;
         continue;
      }

      hex_value = hex_digit_value(*p);
      if (hex_value == -1U) {
         break;
      }
      p++;

      // Store the number while we have space.
      if (bitPos) {
         bitPos -= 4;
         hex_value <<= bitPos % sm_integerPartWidth;
         significand[bitPos / sm_integerPartWidth] |= hex_value;
      } else if (!computedTrailingFraction) {
         lostFraction = trailing_hexa_decimal_fraction(p, end, hex_value);
         computedTrailingFraction = true;
      }
   }

   /* Hex floats require an exponent but not a hexadecimal point.  */
   assert(p != end && "Hex strings require an exponent");
   assert((*p == 'p' || *p == 'P') && "Invalid character in significand");
   assert(p != begin && "Significand has no digits");
   assert((dot == end || p - begin != 1) && "Significand has no digits");

   /* Ignore the exponent if we are zero.  */
   if (p != firstSignificantDigit) {
      int expAdjustment;

      /* Implicit hexadecimal point?  */
      if (dot == end) {
         dot = p;
      }

      /* Calculate the exponent adjustment implicit in the number of
       significant digits.  */
      expAdjustment = static_cast<int>(dot - firstSignificantDigit);
      if (expAdjustment < 0) {
         expAdjustment++;
      }

      expAdjustment = expAdjustment * 4 - 1;

      /* Adjust for writing the significand starting at the most
       significant nibble.  */
      expAdjustment += m_semantics->m_precision;
      expAdjustment -= partsCount * sm_integerPartWidth;

      /* Adjust for the given exponent.  */
      m_exponent = total_exponent(p + 1, end, expAdjustment);
   }

   return normalize(roundingMode, lostFraction);
}

IEEEFloat::OpStatus
IEEEFloat::roundSignificandWithExponent(const integerPart *decSigParts,
                                        unsigned sigPartCount, int exp,
                                        RoundingMode roundingMode)
{
   unsigned int parts, pow5PartCount;
   FltSemantics calcSemantics = { 32767, -32767, 0, 0 };
   integerPart pow5Parts[g_maxPowerOfFiveParts];
   bool isNearest;

   isNearest = (roundingMode == RoundingMode::rmNearestTiesToEven ||
                roundingMode == RoundingMode::rmNearestTiesToAway);

   parts = part_count_for_bits(m_semantics->m_precision + 11);

   /* Calculate pow(5, abs(exp)).  */
   pow5PartCount = powerOf5(pow5Parts, exp >= 0 ? exp: -exp);

   for (;; parts *= 2) {
      OpStatus sigStatus, powStatus;
      unsigned int excessPrecision, truncatedBits;

      calcSemantics.m_precision = parts * sm_integerPartWidth - 1;
      excessPrecision = calcSemantics.m_precision - m_semantics->m_precision;
      truncatedBits = excessPrecision;

      IEEEFloat decSig(calcSemantics, UninitializedTag::uninitialized);
      decSig.makeZero(m_sign);
      IEEEFloat pow5(calcSemantics);

      sigStatus = decSig.convertFromUnsignedParts(decSigParts, sigPartCount,
                                                  RoundingMode::rmNearestTiesToEven);
      powStatus = pow5.convertFromUnsignedParts(pow5Parts, pow5PartCount,
                                                RoundingMode::rmNearestTiesToEven);
      /* Add exp, as 10^n = 5^n * 2^n.  */
      decSig.m_exponent += exp;

      LostFraction calcLostFraction;
      integerPart HUerr, HUdistance;
      unsigned int powHUerr;

      if (exp >= 0) {
         /* multiplySignificand leaves the precision-th bit set to 1.  */
         calcLostFraction = decSig.multiplySignificand(pow5, nullptr);
         powHUerr = powStatus != opOK;
      } else {
         calcLostFraction = decSig.divideSignificand(pow5);
         /* Denormal numbers have less precision.  */
         if (decSig.m_exponent < m_semantics->m_minExponent) {
            excessPrecision += (m_semantics->m_minExponent - decSig.m_exponent);
            truncatedBits = excessPrecision;
            if (excessPrecision > calcSemantics.m_precision) {
               excessPrecision = calcSemantics.m_precision;
            }
         }
         /* Extra half-ulp lost in reciprocal of exponent.  */
         powHUerr = (powStatus == opOK && calcLostFraction == LostFraction::lfExactlyZero) ? 0:2;
      }

      /* Both multiplySignificand and divideSignificand return the
       result with the integer bit set.  */
      assert(ApInt::tcExtractBit
             (decSig.getSignificandParts(), calcSemantics.m_precision - 1) == 1);

      HUerr = huerr_bound(calcLostFraction != LostFraction::lfExactlyZero, sigStatus != opOK,
                          powHUerr);
      HUdistance = 2 * ulps_from_boundary(decSig.getSignificandParts(),
                                          excessPrecision, isNearest);

      /* Are we guaranteed to round correctly if we truncate?  */
      if (HUdistance >= HUerr) {
         ApInt::tcExtract(getSignificandParts(), getPartCount(), decSig.getSignificandParts(),
                          calcSemantics.m_precision - excessPrecision,
                          excessPrecision);
         /* Take the exponent of decSig.  If we tcExtract-ed less bits
         above we must adjust our exponent to compensate for the
         implicit right shift.  */
         m_exponent = (decSig.m_exponent + m_semantics->m_precision
                       - (calcSemantics.m_precision - excessPrecision));
         calcLostFraction = lostfraction_through_truncation(decSig.getSignificandParts(),
                                                            decSig.getPartCount(),
                                                            truncatedBits);
         return normalize(roundingMode, calcLostFraction);
      }
   }
}

IEEEFloat::OpStatus
IEEEFloat::convertFromDecimalString(StringRef str, RoundingMode roundingMode)
{
   DecimalInfo dvalue;
   OpStatus fs;

   /* Scan the text.  */
   StringRef::iterator p = str.begin();
   interpret_decimal(p, str.end(), &dvalue);

   /* Handle the quick cases.  First the case of no significant digits,
     i.e. zero, and then exponents that are obviously too large or too
     small.  Writing L for log 10 / log 2, a number d.ddddd*10^exp
     definitely overflows if
           (exp - 1) * L >= maxExponent
     and definitely underflows to zero where
           (exp + 1) * L <= minExponent - precision
     With integer arithmetic the tightest bounds for L are
           93/28 < L < 196/59            [ numerator <= 256 ]
           42039/12655 < L < 28738/8651  [ numerator <= 65536 ]
  */

   // Test if we have a zero number allowing for strings with no null terminators
   // and zero decimals with non-zero exponents.
   //
   // We computed firstSigDigit by ignoring all zeros and dots. Thus if
   // decimal->m_firstSigDigit equals str.end(), every digit must be a zero and there can
   // be at most one dot. On the other hand, if we have a zero with a non-zero
   // exponent, then we know that D.firstSigDigit will be non-numeric.
   if (dvalue.m_firstSigDigit == str.end() || dec_digit_value(*dvalue.m_firstSigDigit) >= 10U) {
      m_category = FltCategory::fcZero;
      fs = opOK;

      /* Check whether the normalized exponent is high enough to overflow
     max during the log-rebasing in the max-exponent check below. */
   } else if (dvalue.m_normalizedExponent - 1 > INT_MAX / 42039) {
      fs = handleOverflow(roundingMode);

      /* If it wasn't, then it also wasn't high enough to overflow max
     during the log-rebasing in the min-exponent check.  Check that it
     won't overflow min in either check, then perform the min-exponent
     check. */
   } else if (dvalue.m_normalizedExponent - 1 < INT_MIN / 42039 ||
              (dvalue.m_normalizedExponent + 1) * 28738 <=
              8651 * (m_semantics->m_minExponent - (int) m_semantics->m_precision)) {
      /* Underflow to zero and round.  */
      m_category = FltCategory::fcNormal;
      zeroSignificand();
      fs = normalize(roundingMode, LostFraction::lfLessThanHalf);

      /* We can finally safely perform the max-exponent check. */
   } else if ((dvalue.m_normalizedExponent - 1) * 42039
              >= 12655 * m_semantics->m_maxExponent) {
      /* Overflow and round.  */
      fs = handleOverflow(roundingMode);
   } else {
      integerPart *decSignificand;
      unsigned int partCount;

      /* A tight upper bound on number of bits required to hold an
       N-digit decimal integer is N * 196 / 59.  Allocate enough space
       to hold the full significand, and an extra part required by
       tcMultiplyPart.  */
      partCount = static_cast<unsigned int>(dvalue.m_lastSigDigit - dvalue.m_firstSigDigit) + 1;
      partCount = part_count_for_bits(1 + 196 * partCount / 59);
      decSignificand = new integerPart[partCount + 1];
      partCount = 0;

      /* Convert to binary efficiently - we do almost all multiplication
       in an integerPart.  When this would overflow do we do a single
       bignum multiplication, and then revert again to multiplication
       in an integerPart.  */
      do {
         integerPart decValue, val, multiplier;

         val = 0;
         multiplier = 1;

         do {
            if (*p == '.') {
               p++;
               if (p == str.end()) {
                  break;
               }
            }
            decValue = dec_digit_value(*p++);
            assert(decValue < 10U && "Invalid character in significand");
            multiplier *= 10;
            val = val * 10 + decValue;
            /* The maximum number that can be multiplied by ten with any
           digit added without overflowing an integerPart.  */
         } while (p <= dvalue.m_lastSigDigit && multiplier <= (~ (integerPart) 0 - 9) / 10);

         /* Multiply out the current part.  */
         ApInt::tcMultiplyPart(decSignificand, decSignificand, multiplier, val,
                               partCount, partCount + 1, false);

         /* If we used another part (likely but not guaranteed), increase
         the count.  */
         if (decSignificand[partCount]) {
            partCount++;
         }

      } while (p <= dvalue.m_lastSigDigit);

      m_category = FltCategory::fcNormal;
      fs = roundSignificandWithExponent(decSignificand, partCount,
                                        dvalue.m_exponent, roundingMode);

      delete [] decSignificand;
   }

   return fs;
}

bool IEEEFloat::convertFromStringSpecials(StringRef str)
{
   if (str.equals("inf") || str.equals("INFINITY") || str.equals("+Inf")) {
      makeInf(false);
      return true;
   }

   if (str.equals("-inf") || str.equals("-INFINITY") || str.equals("-Inf")) {
      makeInf(true);
      return true;
   }

   if (str.equals("nan") || str.equals("NaN")) {
      makeNaN(false, false);
      return true;
   }

   if (str.equals("-nan") || str.equals("-NaN")) {
      makeNaN(false, true);
      return true;
   }

   return false;
}

IEEEFloat::OpStatus IEEEFloat::convertFromString(StringRef str,
                                                 RoundingMode roundingMode)
{
   assert(!str.empty() && "Invalid string length");

   // Handle special cases.
   if (convertFromStringSpecials(str))
      return opOK;

   /* Handle a leading minus sign.  */
   StringRef::iterator p = str.begin();
   size_t slen = str.getSize();
   m_sign = *p == '-' ? 1 : 0;
   if (*p == '-' || *p == '+') {
      p++;
      slen--;
      assert(slen && "String has no digits");
   }

   if (slen >= 2 && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
      assert(slen - 2 && "Invalid string");
      return convertFromHexadecimalString(StringRef(p + 2, slen - 2),
                                          roundingMode);
   }

   return convertFromDecimalString(StringRef(p, slen), roundingMode);
}

/* Write out a hexadecimal representation of the floating point value
   to DST, which must be of sufficient size, in the C99 form
   [-]0xh.hhhhp[+-]d.  Return the number of characters written,
   excluding the terminating NUL.
   If UPPERCASE, the output is in upper case, otherwise in lower case.
   HEXDIGITS digits appear altogether, rounding the value if
   necessary.  If HEXDIGITS is 0, the minimal precision to display the
   number precisely is used instead.  If nothing would appear after
   the decimal point it is suppressed.
   The decimal exponent is always printed and has at least one digit.
   Zero values display an exponent of zero.  Infinities and NaNs
   appear as "infinity" or "nan" respectively.
   The above rules are as specified by C99.  There is ambiguity about
   what the leading hexadecimal digit should be.  This implementation
   uses whatever is necessary so that the exponent is displayed as
   stored.  This implies the exponent will fall within the IEEE format
   range, and the leading hexadecimal digit will be 0 (for denormals),
   1 (normal numbers) or 2 (normal numbers rounded-away-from-zero with
   any other digits zero).
*/
unsigned int IEEEFloat::convertToHexString(char *dst, unsigned int hexDigits,
                                           bool upperCase,
                                           RoundingMode roundingMode) const
{
   char *p;

   p = dst;
   if (m_sign) {
      *dst++ = '-';
   }
   switch (m_category) {
   case FltCategory::fcInfinity:
      memcpy (dst, upperCase ? sg_infinityU: sg_infinityL, sizeof sg_infinityU - 1);
      dst += sizeof sg_infinityL - 1;
      break;

   case FltCategory::fcNaN:
      memcpy (dst, upperCase ? sg_NaNU: sg_NaNL, sizeof sg_NaNU - 1);
      dst += sizeof sg_NaNU - 1;
      break;

   case FltCategory::fcZero:
      *dst++ = '0';
      *dst++ = upperCase ? 'X': 'x';
      *dst++ = '0';
      if (hexDigits > 1) {
         *dst++ = '.';
         memset (dst, '0', hexDigits - 1);
         dst += hexDigits - 1;
      }
      *dst++ = upperCase ? 'P': 'p';
      *dst++ = '0';
      break;

   case FltCategory::fcNormal:
      dst = convertNormalToHexString (dst, hexDigits, upperCase, roundingMode);
      break;
   }

   *dst = 0;

   return static_cast<unsigned int>(dst - p);
}

/* Does the hard work of outputting the correctly rounded hexadecimal
   form of a normal floating point number with the specified number of
   hexadecimal digits.  If HEXDIGITS is zero the minimum number of
   digits necessary to print the value precisely is output.  */
char *IEEEFloat::convertNormalToHexString(char *dst, unsigned int hexDigits,
                                          bool upperCase,
                                          RoundingMode roundingMode) const
{
   unsigned int count, valueBits, shift, partsCount, outputDigits;
   const char *hexDigitChars;
   const integerPart *significand;
   char *p;
   bool roundUp;

   *dst++ = '0';
   *dst++ = upperCase ? 'X': 'x';

   roundUp = false;
   hexDigitChars = upperCase ? sg_hexDigitsUpper: sg_hexDigitsLower;

   significand = getSignificandParts();
   partsCount = getPartCount();

   /* +3 because the first digit only uses the single integer bit, so
     we have 3 virtual zero most-significant-bits.  */
   valueBits = m_semantics->m_precision + 3;
   shift = sm_integerPartWidth - valueBits % sm_integerPartWidth;

   /* The natural number of digits required ignoring trailing
     insignificant zeroes.  */
   outputDigits = (valueBits - getSignificandLsb () + 3) / 4;

   /* hexDigits of zero means use the required number for the
     precision.  Otherwise, see if we are truncating.  If we are,
     find out if we need to round away from zero.  */
   if (hexDigits) {
      if (hexDigits < outputDigits) {
         /* We are dropping non-zero bits, so need to check how to round.
         "bits" is the number of dropped bits.  */
         unsigned int bits;
         LostFraction fraction;

         bits = valueBits - hexDigits * 4;
         fraction = lostfraction_through_truncation (significand, partsCount, bits);
         roundUp = roundAwayFromZero(roundingMode, fraction, bits);
      }
      outputDigits = hexDigits;
   }

   /* Write the digits consecutively, and start writing in the location
     of the hexadecimal point.  We move the most significant digit
     left and add the hexadecimal point later.  */
   p = ++dst;

   count = (valueBits + sm_integerPartWidth - 1) / sm_integerPartWidth;

   while (outputDigits && count) {
      integerPart part;

      /* Put the most significant integerPartWidth bits in "part".  */
      if (--count == partsCount) {
         part = 0;  /* An imaginary higher zero part.  */
      } else {
         part = significand[count] << shift;
      }
      if (count && shift) {
         part |= significand[count - 1] >> (sm_integerPartWidth - shift);
      }
      /* Convert as much of "part" to hexdigits as we can.  */
      unsigned int curDigits = sm_integerPartWidth / 4;

      if (curDigits > outputDigits) {
         curDigits = outputDigits;
      }

      dst += part_as_hex(dst, part, curDigits, hexDigitChars);
      outputDigits -= curDigits;
   }

   if (roundUp) {
      char *q = dst;

      /* Note that hexDigitChars has a trailing '0'.  */
      do {
         q--;
         *q = hexDigitChars[hex_digit_value (*q) + 1];
      } while (*q == '0');
      assert(q >= p);
   } else {
      /* Add trailing zeroes.  */
      memset (dst, '0', outputDigits);
      dst += outputDigits;
   }

   /* Move the most significant digit to before the point, and if there
     is something after the decimal point add it.  This must come
     after rounding above.  */
   p[-1] = p[0];
   if (dst -1 == p) {
      dst--;
   } else {
      p[0] = '.';
   }

   /* Finally output the exponent.  */
   *dst++ = upperCase ? 'P': 'p';

   return write_signed_decimal (dst, m_exponent);
}

HashCode hash_value(const IEEEFloat &arg)
{
   if (!arg.isFiniteNonZero())
      return hash_combine((uint8_t)arg.m_category,
                          // NaN has no sign, fix it at zero.
                          arg.isNaN() ? (uint8_t)0 : (uint8_t)arg.m_sign,
                          arg.m_semantics->m_precision);

   // Normal floats need their exponent and significand hashed.
   return hash_combine((uint8_t)arg.m_category, (uint8_t)arg.m_sign,
                       arg.m_semantics->m_precision, arg.m_exponent,
                       hash_combine_range(
                          arg.getSignificandParts(),
                          arg.getSignificandParts() + arg.getPartCount()));
}

// Conversion from ApFloat to/from host float/double.  It may eventually be
// possible to eliminate these and have everybody deal with ApFloats, but that
// will take a while.  This approach will not easily extend to long double.
// Current implementation requires integerPartWidth==64, which is correct at
// the moment but could be made more general.

// Denormals have exponent minExponent in ApFloat, but minExponent-1 in
// the actual IEEE respresentations.  We compensate for that here.

ApInt IEEEFloat::convertF80LongDoubleApFloatToApInt() const
{
   assert(m_semantics == (const FltSemantics*)&sg_semX87DoubleExtended);
   assert(getPartCount()==2);

   uint64_t myexponent, mysignificand;

   if (isFiniteNonZero()) {
      myexponent = m_exponent+16383; //bias
      mysignificand = getSignificandParts()[0];
      if (myexponent==1 && !(mysignificand & 0x8000000000000000ULL)) {
         myexponent = 0;   // denormal
      }
   } else if (m_category == FltCategory::fcZero) {
      myexponent = 0;
      mysignificand = 0;
   } else if (m_category == FltCategory::fcInfinity) {
      myexponent = 0x7fff;
      mysignificand = 0x8000000000000000ULL;
   } else {
      assert(m_category == FltCategory::fcNaN && "Unknown category");
      myexponent = 0x7fff;
      mysignificand = getSignificandParts()[0];
   }

   uint64_t words[2];
   words[0] = mysignificand;
   words[1] =  ((uint64_t)(m_sign & 1) << 15) |
         (myexponent & 0x7fffLL);
   return ApInt(80, words);
}

ApInt IEEEFloat::convertPPCDoubleDoubleApFloatToApInt() const
{
   assert(m_semantics == (const FltSemantics *)&sg_semPPCDoubleDoubleLegacy);
   assert(getPartCount()==2);

   uint64_t words[2];
   OpStatus fs;
   bool losesInfo;

   // Convert number to double.  To avoid spurious underflows, we re-
   // normalize against the "double" minExponent first, and only *then*
   // truncate the mantissa.  The result of that second conversion
   // may be inexact, but should never underflow.
   // Declare FltSemantics before ApFloat that uses it (and
   // saves pointer to it) to ensure correct destruction order.
   FltSemantics extendedSemantics = *m_semantics;
   extendedSemantics.m_minExponent = sg_semIEEEdouble.m_minExponent;
   IEEEFloat extended(*this);
   fs = extended.convert(extendedSemantics, RoundingMode::rmNearestTiesToEven, &losesInfo);
   assert(fs == opOK && !losesInfo);
   (void)fs;

   IEEEFloat u(extended);
   fs = u.convert(sg_semIEEEdouble, RoundingMode::rmNearestTiesToEven, &losesInfo);
   assert(fs == opOK || fs == opInexact);
   (void)fs;
   words[0] = *u.convertDoubleApFloatToApInt().getRawData();

   // If conversion was exact or resulted in a special case, we're done;
   // just set the second double to zero.  Otherwise, re-convert back to
   // the extended format and compute the difference.  This now should
   // convert exactly to double.
   if (u.isFiniteNonZero() && losesInfo) {
      fs = u.convert(extendedSemantics, RoundingMode::rmNearestTiesToEven, &losesInfo);
      assert(fs == opOK && !losesInfo);
      (void)fs;

      IEEEFloat v(extended);
      v.subtract(u,  RoundingMode::rmNearestTiesToEven);
      fs = v.convert(sg_semIEEEdouble,  RoundingMode::rmNearestTiesToEven, &losesInfo);
      assert(fs == opOK && !losesInfo);
      (void)fs;
      words[1] = *v.convertDoubleApFloatToApInt().getRawData();
   } else {
      words[1] = 0;
   }

   return ApInt(128, words);
}

ApInt IEEEFloat::convertQuadrupleApFloatToApInt() const
{
   assert(m_semantics == (const FltSemantics*)&sg_semIEEEquad);
   assert(getPartCount()==2);

   uint64_t myexponent, mysignificand, mysignificand2;

   if (isFiniteNonZero()) {
      myexponent = m_exponent+16383; //bias
      mysignificand = getSignificandParts()[0];
      mysignificand2 = getSignificandParts()[1];
      if (myexponent==1 && !(mysignificand2 & 0x1000000000000LL))
         myexponent = 0;   // denormal
   } else if (m_category== FltCategory::fcZero) {
      myexponent = 0;
      mysignificand = mysignificand2 = 0;
   } else if (m_category == FltCategory::fcInfinity) {
      myexponent = 0x7fff;
      mysignificand = mysignificand2 = 0;
   } else {
      assert(m_category == FltCategory::fcNaN && "Unknown category!");
      myexponent = 0x7fff;
      mysignificand = getSignificandParts()[0];
      mysignificand2 = getSignificandParts()[1];
   }

   uint64_t words[2];
   words[0] = mysignificand;
   words[1] = ((uint64_t)(m_sign & 1) << 63) |
         ((myexponent & 0x7fff) << 48) |
         (mysignificand2 & 0xffffffffffffLL);

   return ApInt(128, words);
}

ApInt IEEEFloat::convertDoubleApFloatToApInt() const
{
   assert(m_semantics == (const FltSemantics*)&sg_semIEEEdouble);
   assert(getPartCount()==1);

   uint64_t myexponent, mysignificand;

   if (isFiniteNonZero()) {
      myexponent = m_exponent+1023; //bias
      mysignificand = *getSignificandParts();
      if (myexponent==1 && !(mysignificand & 0x10000000000000LL)) {
         myexponent = 0;   // denormal
      }

   } else if (m_category == FltCategory::fcZero) {
      myexponent = 0;
      mysignificand = 0;
   } else if (m_category== FltCategory::fcInfinity) {
      myexponent = 0x7ff;
      mysignificand = 0;
   } else {
      assert(m_category == FltCategory::fcNaN && "Unknown category!");
      myexponent = 0x7ff;
      mysignificand = *getSignificandParts();
   }

   return ApInt(64, ((((uint64_t)(m_sign & 1) << 63) |
                      ((myexponent & 0x7ff) <<  52) |
                      (mysignificand & 0xfffffffffffffLL))));
}

ApInt IEEEFloat::convertFloatApFloatToApInt() const
{
   assert(m_semantics == (const FltSemantics*)&sg_semIEEEsingle);
   assert(getPartCount()==1);

   uint32_t myexponent, mysignificand;

   if (isFiniteNonZero()) {
      myexponent = m_exponent+127; //bias
      mysignificand = (uint32_t)*getSignificandParts();
      if (myexponent == 1 && !(mysignificand & 0x800000)) {
         myexponent = 0;   // denormal
      }
   } else if (m_category == FltCategory::fcZero) {
      myexponent = 0;
      mysignificand = 0;
   } else if (m_category == FltCategory::fcInfinity) {
      myexponent = 0xff;
      mysignificand = 0;
   } else {
      assert(m_category == FltCategory::fcNaN && "Unknown category!");
      myexponent = 0xff;
      mysignificand = (uint32_t)*getSignificandParts();
   }

   return ApInt(32, (((m_sign&1) << 31) | ((myexponent&0xff) << 23) |
                     (mysignificand & 0x7fffff)));
}

ApInt IEEEFloat::convertHalfApFloatToApInt() const
{
   assert(m_semantics == (const FltSemantics*)&sg_semIEEEhalf);
   assert(getPartCount()==1);

   uint32_t myexponent, mysignificand;

   if (isFiniteNonZero()) {
      myexponent = m_exponent+15; //bias
      mysignificand = (uint32_t)*getSignificandParts();
      if (myexponent == 1 && !(mysignificand & 0x400)) {
         myexponent = 0;   // denormal
      }

   } else if (m_category==FltCategory::fcZero) {
      myexponent = 0;
      mysignificand = 0;
   } else if (m_category==FltCategory::fcInfinity) {
      myexponent = 0x1f;
      mysignificand = 0;
   } else {
      assert(m_category == FltCategory::fcNaN && "Unknown category!");
      myexponent = 0x1f;
      mysignificand = (uint32_t)*getSignificandParts();
   }

   return ApInt(16, (((m_sign&1) << 15) | ((myexponent&0x1f) << 10) |
                     (mysignificand & 0x3ff)));
}

// This function creates an APInt that is just a bit map of the floating
// point constant as it would appear in memory.  It is not a conversion,
// and treating the result as a normal integer is unlikely to be useful.

ApInt IEEEFloat::bitcastToApInt() const
{
   if (m_semantics == (const FltSemantics*)&sg_semIEEEhalf) {
      return convertHalfApFloatToApInt();
   }

   if (m_semantics == (const FltSemantics*)&sg_semIEEEsingle) {
      return convertFloatApFloatToApInt();
   }

   if (m_semantics == (const FltSemantics*)&sg_semIEEEdouble) {
      return convertDoubleApFloatToApInt();
   }

   if (m_semantics == (const FltSemantics*)&sg_semIEEEquad) {
      return convertQuadrupleApFloatToApInt();
   }

   if (m_semantics == (const FltSemantics *)&sg_semPPCDoubleDoubleLegacy) {
      return convertPPCDoubleDoubleApFloatToApInt();
   }

   assert(m_semantics == (const FltSemantics*)&sg_semX87DoubleExtended &&
          "unknown format!");
   return convertF80LongDoubleApFloatToApInt();
}

float IEEEFloat::convertToFloat() const
{
   assert(m_semantics == (const FltSemantics*)&sg_semIEEEsingle &&
          "Float semantics are not IEEEsingle");
   ApInt api = bitcastToApInt();
   return api.bitsToFloat();
}

double IEEEFloat::convertToDouble() const
{
   assert(m_semantics == (const FltSemantics*)&sg_semIEEEdouble &&
          "Float semantics are not IEEEdouble");
   ApInt api = bitcastToApInt();
   return api.bitsToDouble();
}

/// Integer bit is explicit in this format.  Intel hardware (387 and later)
/// does not support these bit patterns:
///  exponent = all 1's, integer bit 0, significand 0 ("pseudoinfinity")
///  exponent = all 1's, integer bit 0, significand nonzero ("pseudoNaN")
///  exponent = 0, integer bit 1 ("pseudodenormal")
///  exponent!=0 nor all 1's, integer bit 0 ("unnormal")
/// At the moment, the first two are treated as NaNs, the second two as Normal.
void IEEEFloat::initFromF80LongDoubleApInt(const ApInt &apint) {
   assert(apint.getBitWidth()==80);
   uint64_t i1 = apint.getRawData()[0];
   uint64_t i2 = apint.getRawData()[1];
   uint64_t myexponent = (i2 & 0x7fff);
   uint64_t mysignificand = i1;
   uint8_t myintegerbit = mysignificand >> 63;

   initialize(&sg_semX87DoubleExtended);
   assert(getPartCount()==2);

   m_sign = static_cast<unsigned int>(i2>>15);
   if (myexponent==0 && mysignificand==0) {
      // exponent, significand meaningless
      m_category = FltCategory::fcZero;
   } else if (myexponent==0x7fff && mysignificand == 0x8000000000000000ULL) {
      // exponent, significand meaningless
      m_category = FltCategory::fcInfinity;
   } else if ((myexponent == 0x7fff && mysignificand != 0x8000000000000000ULL) ||
              (myexponent != 0x7fff && myexponent != 0 && myintegerbit == 0)) {
      // exponent meaningless
      m_category = FltCategory::fcNaN;
      getSignificandParts()[0] = mysignificand;
      getSignificandParts()[1] = 0;
   } else {
      m_category = FltCategory::fcNormal;
      m_exponent = myexponent - 16383;
      getSignificandParts()[0] = mysignificand;
      getSignificandParts()[1] = 0;
      if (myexponent==0) {         // denormal
         m_exponent = -16382;
      }
   }
}

void IEEEFloat::initFromPPCDoubleDoubleApInt(const ApInt &apint)
{
   assert(apint.getBitWidth()==128);
   uint64_t i1 = apint.getRawData()[0];
   uint64_t i2 = apint.getRawData()[1];
   OpStatus fs;
   bool losesInfo;

   // Get the first double and convert to our format.
   initFromDoubleApInt(ApInt(64, i1));
   fs = convert(sg_semPPCDoubleDoubleLegacy, RoundingMode::rmNearestTiesToEven, &losesInfo);
   assert(fs == opOK && !losesInfo);
   (void)fs;

   // Unless we have a special case, add in second double.
   if (isFiniteNonZero()) {
      IEEEFloat v(sg_semIEEEdouble, ApInt(64, i2));
      fs = v.convert(sg_semPPCDoubleDoubleLegacy, RoundingMode::rmNearestTiesToEven, &losesInfo);
      assert(fs == opOK && !losesInfo);
      (void)fs;

      add(v, RoundingMode::rmNearestTiesToEven);
   }
}

void IEEEFloat::initFromQuadrupleApInt(const ApInt &apint)
{
   assert(apint.getBitWidth()==128);
   uint64_t i1 = apint.getRawData()[0];
   uint64_t i2 = apint.getRawData()[1];
   uint64_t myexponent = (i2 >> 48) & 0x7fff;
   uint64_t mysignificand  = i1;
   uint64_t mysignificand2 = i2 & 0xffffffffffffLL;

   initialize(&sg_semIEEEquad);
   assert(getPartCount()==2);

   m_sign = static_cast<unsigned int>(i2>>63);
   if (myexponent==0 &&
       (mysignificand==0 && mysignificand2==0)) {
      // exponent, significand meaningless
      m_category = FltCategory::fcZero;
   } else if (myexponent==0x7fff &&
              (mysignificand==0 && mysignificand2==0)) {
      // exponent, significand meaningless
      m_category = FltCategory::fcInfinity;
   } else if (myexponent==0x7fff &&
              (mysignificand!=0 || mysignificand2 !=0)) {
      // exponent meaningless
      m_category = FltCategory::fcNaN;
      getSignificandParts()[0] = mysignificand;
      getSignificandParts()[1] = mysignificand2;
   } else {
      m_category = FltCategory::fcNormal;
      m_exponent = myexponent - 16383;
      getSignificandParts()[0] = mysignificand;
      getSignificandParts()[1] = mysignificand2;
      if (myexponent==0) {         // denormal
         m_exponent = -16382;
      } else {
         getSignificandParts()[1] |= 0x1000000000000LL;  // integer bit
      }
   }
}

void IEEEFloat::initFromDoubleApInt(const ApInt &apint)
{
   assert(apint.getBitWidth()==64);
   uint64_t i = *apint.getRawData();
   uint64_t myexponent = (i >> 52) & 0x7ff;
   uint64_t mysignificand = i & 0xfffffffffffffLL;

   initialize(&sg_semIEEEdouble);
   assert(getPartCount()==1);

   m_sign = static_cast<unsigned int>(i>>63);
   if (myexponent==0 && mysignificand==0) {
      // exponent, significand meaningless
      m_category = FltCategory::fcZero;
   } else if (myexponent==0x7ff && mysignificand==0) {
      // exponent, significand meaningless
      m_category = FltCategory::fcInfinity;
   } else if (myexponent==0x7ff && mysignificand!=0) {
      // exponent meaningless
      m_category = FltCategory::fcNaN;
      *getSignificandParts() = mysignificand;
   } else {
      m_category = FltCategory::fcNormal;
      m_exponent = myexponent - 1023;
      *getSignificandParts() = mysignificand;
      if (myexponent == 0) {      // denormal
         m_exponent = -1022;
      } else {
         *getSignificandParts() |= 0x10000000000000LL;  // integer bit
      }
   }
}

void IEEEFloat::initFromFloatApInt(const ApInt &api)
{
   assert(api.getBitWidth()==32);
   uint32_t i = (uint32_t)*api.getRawData();
   uint32_t myexponent = (i >> 23) & 0xff;
   uint32_t mysignificand = i & 0x7fffff;

   initialize(&sg_semIEEEsingle);
   assert(getPartCount()==1);

   m_sign = i >> 31;
   if (myexponent==0 && mysignificand==0) {
      // exponent, significand meaningless
      m_category = FltCategory::fcZero;
   } else if (myexponent==0xff && mysignificand==0) {
      // exponent, significand meaningless
      m_category = FltCategory::fcInfinity;
   } else if (myexponent==0xff && mysignificand!=0) {
      // sign, exponent, significand meaningless
      m_category = FltCategory::fcNaN;
      *getSignificandParts() = mysignificand;
   } else {
      m_category = FltCategory::fcNormal;
      m_exponent = myexponent - 127;  //bias
      *getSignificandParts() = mysignificand;
      if (myexponent==0) {  // denormal
         m_exponent = -126;
      } else {
         *getSignificandParts() |= 0x800000; // integer bit
      }
   }
}

void IEEEFloat::initFromHalfApInt(const ApInt &apint)
{
   assert(apint.getBitWidth()==16);
   uint32_t i = (uint32_t)*apint.getRawData();
   uint32_t myexponent = (i >> 10) & 0x1f;
   uint32_t mysignificand = i & 0x3ff;

   initialize(&sg_semIEEEhalf);
   assert(getPartCount()==1);

   m_sign = i >> 15;
   if (myexponent==0 && mysignificand==0) {
      // exponent, significand meaningless
      m_category = FltCategory::fcZero;
   } else if (myexponent==0x1f && mysignificand==0) {
      // exponent, significand meaningless
      m_category = FltCategory::fcInfinity;
   } else if (myexponent==0x1f && mysignificand!=0) {
      // sign, exponent, significand meaningless
      m_category = FltCategory::fcNaN;
      *getSignificandParts() = mysignificand;
   } else {
      m_category = FltCategory::fcNormal;
      m_exponent = myexponent - 15;  //bias
      *getSignificandParts() = mysignificand;
      if (myexponent == 0) {   // denormal
         m_exponent = -14;
      } else {
         *getSignificandParts() |= 0x400; // integer bit
      }
   }
}

/// Treat api as containing the bits of a floating point number.  Currently
/// we infer the floating point type from the size of the APInt.  The
/// isIEEE argument distinguishes between PPC128 and IEEE128 (not meaningful
/// when the size is anything else).
void IEEEFloat::initFromApInt(const FltSemantics *semantics, const ApInt &apint)
{
   if (semantics == &sg_semIEEEhalf) {
      return initFromHalfApInt(apint);
   }

   if (semantics == &sg_semIEEEsingle) {
      return initFromFloatApInt(apint);
   }

   if (semantics == &sg_semIEEEdouble) {
      return initFromDoubleApInt(apint);
   }

   if (semantics == &sg_semX87DoubleExtended) {
      return initFromF80LongDoubleApInt(apint);
   }

   if (semantics == &sg_semIEEEquad) {
      return initFromQuadrupleApInt(apint);
   }

   if (semantics == &sg_semPPCDoubleDoubleLegacy) {
      return initFromPPCDoubleDoubleApInt(apint);
   }
   polar_unreachable(nullptr);
}

/// Make this number the largest magnitude normal number in the given
/// semantics.
void IEEEFloat::makeLargest(bool negative)
{
   // We want (in interchange format):
   //   sign = {Negative}
   //   exponent = 1..10
   //   significand = 1..1
   m_category = FltCategory::fcNormal;
   m_sign = negative;
   m_exponent = m_semantics->m_maxExponent;

   // Use memset to set all but the highest integerPart to all ones.
   integerPart *significand = getSignificandParts();
   unsigned partCount = getPartCount();
   memset(significand, 0xFF, sizeof(integerPart)*(partCount - 1));

   // Set the high integerPart especially setting all unused top bits for
   // internal consistency.
   const unsigned numUnusedHighBits =
         partCount * sm_integerPartWidth - m_semantics->m_precision;
   significand[partCount - 1] = (numUnusedHighBits < sm_integerPartWidth)
         ? (~integerPart(0) >> numUnusedHighBits)
         : 0;
}

/// Make this number the smallest magnitude denormal number in the given
/// semantics.
void IEEEFloat::makeSmallest(bool negative)
{
   // We want (in interchange format):
   //   sign = {Negative}
   //   exponent = 0..0
   //   significand = 0..01
   m_category = FltCategory::fcNormal;
   m_sign = negative;
   m_exponent = m_semantics->m_minExponent;
   ApInt::tcSet(getSignificandParts(), 1, getPartCount());
}

void IEEEFloat::makeSmallestNormalized(bool negative)
{
   // We want (in interchange format):
   //   sign = {Negative}
   //   exponent = 0..0
   //   significand = 10..0

   m_category = FltCategory::fcNormal;
   zeroSignificand();
   m_sign = negative;
   m_exponent = m_semantics->m_minExponent;
   getSignificandParts()[part_count_for_bits(m_semantics->m_precision) - 1] |=
         (((integerPart)1) << ((m_semantics->m_precision - 1) % sm_integerPartWidth));
}

IEEEFloat::IEEEFloat(const FltSemantics &semantics, const ApInt &apint)
{
   initFromApInt(&semantics, apint);
}

IEEEFloat::IEEEFloat(float fvalue)
{
   initFromApInt(&sg_semIEEEsingle, ApInt::floatToBits(fvalue));
}

IEEEFloat::IEEEFloat(double dvalue)
{
   initFromApInt(&sg_semIEEEdouble, ApInt::doubleToBits(dvalue));
}

namespace
{
void append(SmallVectorImpl<char> &buffer, StringRef str)
{
   buffer.append(str.begin(), str.end());
}

/// Removes data from the given significand until it is no more
/// precise than is required for the desired precision.
void adjust_to_precision(ApInt &significand,
                         int &exp, unsigned formatPrecision)
{
   unsigned bits = significand.getActiveBits();

   // 196/59 is a very slight overestimate of lg_2(10).
   unsigned bitsRequired = (formatPrecision * 196 + 58) / 59;

   if (bits <= bitsRequired) return;

   unsigned tensRemovable = (bits - bitsRequired) * 59 / 196;
   if (!tensRemovable) return;

   exp += tensRemovable;

   ApInt divisor(significand.getBitWidth(), 1);
   ApInt powten(significand.getBitWidth(), 10);
   while (true) {
      if (tensRemovable & 1) {
         divisor *= powten;
      }
      tensRemovable >>= 1;
      if (!tensRemovable){
         break;
      }
      powten *= powten;
   }

   significand = significand.udiv(divisor);

   // Truncate the significand down to its active bit count.
   significand = significand.trunc(significand.getActiveBits());
}


void adjust_to_precision(SmallVectorImpl<char> &buffer,
                         int &exp, unsigned formatPrecision)
{
   unsigned N = buffer.getSize();
   if (N <= formatPrecision) {
      return;
   }

   // The most significant figures are the last ones in the buffer.
   unsigned firstSignificant = N - formatPrecision;

   // Round.
   // FIXME: this probably shouldn't use 'round half up'.

   // Rounding down is just a truncation, except we also want to drop
   // trailing zeros from the new result.
   if (buffer[firstSignificant - 1] < '5') {
      while (firstSignificant < N && buffer[firstSignificant] == '0') {
         firstSignificant++;
      }
      exp += firstSignificant;
      buffer.erase(&buffer[0], &buffer[firstSignificant]);
      return;
   }

   // Rounding up requires a decimal add-with-carry.  If we continue
   // the carry, the newly-introduced zeros will just be truncated.
   for (unsigned index = firstSignificant; index != N; ++index) {
      if (buffer[index] == '9') {
         firstSignificant++;
      } else {
         buffer[index]++;
         break;
      }
   }

   // If we carried through, we have exactly one digit of precision.
   if (firstSignificant == N) {
      exp += firstSignificant;
      buffer.clear();
      buffer.push_back('1');
      return;
   }

   exp += firstSignificant;
   buffer.erase(&buffer[0], &buffer[firstSignificant]);
}
} // anonymous namespace

void IEEEFloat::toString(SmallVectorImpl<char> &str, unsigned formatPrecision,
                         unsigned formatMaxPadding, bool truncateZero) const
{
   switch (m_category) {
   case FltCategory::fcInfinity:
      if (isNegative()) {
         return append(str, "-Inf");
      } else {
         return append(str, "+Inf");
      }
   case FltCategory::fcNaN: return append(str, "NaN");

   case FltCategory::fcZero:
      if (isNegative()) {
         str.push_back('-');
      }
      if (!formatMaxPadding) {
         if (truncateZero) {
            append(str, "0.0E+0");
         } else {
            append(str, "0.0");
            if (formatPrecision > 1) {
               str.append(formatPrecision - 1, '0');
            }
            append(str, "e+00");
         }
      } else {
         str.push_back('0');
      }

      return;

   case FltCategory::fcNormal:
      break;
   }
   if (isNegative()) {
      str.push_back('-');
   }

   // Decompose the number into an APInt and an exponent.
   int exp = m_exponent - ((int) m_semantics->m_precision - 1);
   ApInt significand(m_semantics->m_precision,
                     make_array_ref(getSignificandParts(),
                                    part_count_for_bits(m_semantics->m_precision)));

   // Set FormatPrecision if zero.  We want to do this before we
   // truncate trailing zeros, as those are part of the precision.
   if (!formatPrecision) {
      // We use enough digits so the number can be round-tripped back to an
      // ApFloat. The formula comes from "How to Print Floating-Point Numbers
      // Accurately" by Steele and White.
      // FIXME: Using a formula based purely on the precision is conservative;
      // we can print fewer digits depending on the actual value being printed.

      // FormatPrecision = 2 + floor(significandBits / lg_2(10))
      formatPrecision = 2 + m_semantics->m_precision * 59 / 196;
   }

   // Ignore trailing binary zeros.
   int trailingZeros = significand.countTrailingZeros();
   exp += trailingZeros;
   significand.lshrInPlace(trailingZeros);

   // Change the exponent from 2^e to 10^e.
   if (exp == 0) {
      // Nothing to do.
   } else if (exp > 0) {
      // Just shift left.
      significand = significand.zext(m_semantics->m_precision + exp);
      significand <<= exp;
      exp = 0;
   } else { /* exp < 0 */
      int texp = -exp;

      // We transform this using the identity:
      //   (N)(2^-e) == (N)(5^e)(10^-e)
      // This means we have to multiply N (the significand) by 5^e.
      // To avoid overflow, we have to operate on numbers large
      // enough to store N * 5^e:
      //   log2(N * 5^e) == log2(N) + e * log2(5)
      //                 <= semantics->precision + e * 137 / 59
      //   (log_2(5) ~ 2.321928 < 2.322034 ~ 137/59)

      unsigned precision = m_semantics->m_precision + (137 * texp + 136) / 59;

      // Multiply significand by 5^e.
      //   N * 5^0101 == N * 5^(1*1) * 5^(0*2) * 5^(1*4) * 5^(0*8)
      significand = significand.zext(precision);
      ApInt five_to_the_i(precision, 5);
      while (true) {
         if (texp & 1) {
            significand *= five_to_the_i;
         }

         texp >>= 1;
         if (!texp) {
            break;
         }
         five_to_the_i *= five_to_the_i;
      }
   }

   adjust_to_precision(significand, exp, formatPrecision);

   SmallVector<char, 256> buffer;

   // Fill the buffer.
   unsigned precision = significand.getBitWidth();
   ApInt ten(precision, 10);
   ApInt digit(precision, 0);

   bool inTrail = true;
   while (significand != 0) {
      // digit <- significand % 10
      // significand <- significand / 10
      ApInt::udivrem(significand, ten, significand, digit);

      unsigned d = digit.getZeroExtValue();

      // Drop trailing zeros.
      if (inTrail && !d) {
         exp++;
      }
      else {
         buffer.push_back((char) ('0' + d));
         inTrail = false;
      }
   }

   assert(!buffer.empty() && "no characters in buffer!");

   // Drop down to FormatPrecision.
   // TODO: don't do more precise calculations above than are required.
   adjust_to_precision(buffer, exp, formatPrecision);

   unsigned NDigits = buffer.getSize();

   // Check whether we should use scientific notation.
   bool formatScientific;
   if (!formatMaxPadding) {
      formatScientific = true;
   } else {
      if (exp >= 0) {
         // 765e3 --> 765000
         //              ^^^
         // But we shouldn't make the number look more precise than it is.
         formatScientific = ((unsigned) exp > formatMaxPadding ||
                             NDigits + (unsigned) exp > formatPrecision);
      } else {
         // Power of the most significant digit.
         int msd = exp + (int) (NDigits - 1);
         if (msd >= 0) {
            // 765e-2 == 7.65
            formatScientific = false;
         } else {
            // 765e-5 == 0.00765
            //           ^ ^^
            formatScientific = ((unsigned) -msd) > formatMaxPadding;
         }
      }
   }

   // Scientific formatting is pretty straightforward.
   if (formatScientific) {
      exp += (NDigits - 1);

      str.push_back(buffer[NDigits-1]);
      str.push_back('.');
      if (NDigits == 1 && truncateZero) {
         str.push_back('0');
      } else {
         for (unsigned index = 1; index != NDigits; ++index) {
            str.push_back(buffer[NDigits-1-index]);
         }
      }

      // Fill with zeros up to FormatPrecision.
      if (!truncateZero && formatPrecision > NDigits - 1) {
         str.append(formatPrecision - NDigits + 1, '0');
      }

      // For !TruncateZero we use lower 'e'.
      str.push_back(truncateZero ? 'E' : 'e');

      str.push_back(exp >= 0 ? '+' : '-');
      if (exp < 0) {
         exp = -exp;
      }
      SmallVector<char, 6> expbuf;
      do {
         expbuf.push_back((char) ('0' + (exp % 10)));
         exp /= 10;
      } while (exp);
      // Exponent always at least two digits if we do not truncate zeros.
      if (!truncateZero && expbuf.getSize() < 2) {
         expbuf.push_back('0');
      }

      for (unsigned index = 0, end = expbuf.getSize(); index != end; ++index) {
         str.push_back(expbuf[end - 1 - index]);
      }

      return;
   }

   // Non-scientific, positive exponents.
   if (exp >= 0) {
      for (unsigned index = 0; index != NDigits; ++index) {
         str.push_back(buffer[NDigits-1-index]);
      }

      for (unsigned index = 0; index != (unsigned) exp; ++index) {
         str.push_back('0');
      }

      return;
   }

   // Non-scientific, negative exponents.

   // The number of digits to the left of the decimal point.
   int nWholeDigits = exp + (int) NDigits;

   unsigned index = 0;
   if (nWholeDigits > 0) {
      for (; index != (unsigned) nWholeDigits; ++index) {
         str.push_back(buffer[NDigits-index-1]);
      }
      str.push_back('.');
   } else {
      unsigned nZeros = 1 + (unsigned) - nWholeDigits;

      str.push_back('0');
      str.push_back('.');
      for (unsigned zero = 1; zero != nZeros; ++zero) {
         str.push_back('0');
      }
   }

   for (; index != NDigits; ++index) {
      str.push_back(buffer[NDigits - index - 1]);
   }
}

bool IEEEFloat::getExactInverse(ApFloat *inv) const
{
   // Special floats and denormals have no exact inverse.
   if (!isFiniteNonZero()) {
      return false;
   }
   // Check that the number is a power of two by making sure that only the
   // integer bit is set in the significand.
   if (getSignificandLsb() != m_semantics->m_precision - 1) {
      return false;
   }
   // Get the inverse.
   IEEEFloat reciprocal(*m_semantics, 1ULL);
   if (reciprocal.divide(*this, RoundingMode::rmNearestTiesToEven) != opOK) {
      return false;
   }
   // Avoid multiplication with a denormal, it is not safe on all platforms and
   // may be slower than a normal division.
   if (reciprocal.isDenormal()) {
      return false;
   }
   assert(reciprocal.isFiniteNonZero() &&
          reciprocal.getSignificandLsb() == reciprocal.m_semantics->m_precision - 1);

   if (inv) {
      *inv = ApFloat(reciprocal, *m_semantics);
   }
   return true;
}

bool IEEEFloat::isSignaling() const
{
   if (!isNaN()) {
      return false;
   }
   // IEEE-754R 2008 6.2.1: A signaling NaN bit string should be encoded with the
   // first bit of the trailing significand being 0.
   return !ApInt::tcExtractBit(getSignificandParts(), m_semantics->m_precision - 2);
}

/// IEEE-754R 2008 5.3.1: nextUp/nextDown.
///
/// *NOTE* since nextDown(x) = -nextUp(-x), we only implement nextUp with
/// appropriate sign switching before/after the computation.
IEEEFloat::OpStatus IEEEFloat::next(bool nextDown)
{
   // If we are performing nextDown, swap sign so we have -x.
   if (nextDown) {
      changeSign();
   }
   // Compute nextUp(x)
   OpStatus result = opOK;

   // Handle each float category separately.
   switch (m_category) {
   case FltCategory::fcInfinity:
      // nextUp(+inf) = +inf
      if (!isNegative()) {
         break;
      }
      // nextUp(-inf) = -getLargest()
      makeLargest(true);
      break;
   case FltCategory::fcNaN:
      // IEEE-754R 2008 6.2 Par 2: nextUp(sNaN) = qNaN. Set Invalid flag.
      // IEEE-754R 2008 6.2: nextUp(qNaN) = qNaN. Must be identity so we do not
      //                     change the payload.
      if (isSignaling()) {
         result = opInvalidOp;
         // For consistency, propagate the sign of the sNaN to the qNaN.
         makeNaN(false, isNegative(), nullptr);
      }
      break;
   case FltCategory::fcZero:
      // nextUp(pm 0) = +getSmallest()
      makeSmallest(false);
      break;
   case FltCategory::fcNormal:
      // nextUp(-getSmallest()) = -0
      if (isSmallest() && isNegative()) {
         ApInt::tcSet(getSignificandParts(), 0, getPartCount());
         m_category = FltCategory::fcZero;
         m_exponent = 0;
         break;
      }

      // nextUp(getLargest()) == INFINITY
      if (isLargest() && !isNegative()) {
         ApInt::tcSet(getSignificandParts(), 0, getPartCount());
         m_category = FltCategory::fcInfinity;
         m_exponent = m_semantics->m_maxExponent + 1;
         break;
      }

      // nextUp(normal) == normal + inc.
      if (isNegative()) {
         // If we are negative, we need to decrement the significand.

         // We only cross a binade boundary that requires adjusting the exponent
         // if:
         //   1. exponent != semantics->minExponent. This implies we are not in the
         //   smallest binade or are dealing with denormals.
         //   2. Our significand excluding the integral bit is all zeros.
         bool WillCrossBinadeBoundary =
               m_exponent != m_semantics->m_minExponent && isSignificandAllZeros();

         // Decrement the significand.
         //
         // We always do this since:
         //   1. If we are dealing with a non-binade decrement, by definition we
         //   just decrement the significand.
         //   2. If we are dealing with a normal -> normal binade decrement, since
         //   we have an explicit integral bit the fact that all bits but the
         //   integral bit are zero implies that subtracting one will yield a
         //   significand with 0 integral bit and 1 in all other spots. Thus we
         //   must just adjust the exponent and set the integral bit to 1.
         //   3. If we are dealing with a normal -> denormal binade decrement,
         //   since we set the integral bit to 0 when we represent denormals, we
         //   just decrement the significand.
         integerPart *parts = getSignificandParts();
         ApInt::tcDecrement(parts, getPartCount());

         if (WillCrossBinadeBoundary) {
            // Our result is a normal number. Do the following:
            // 1. Set the integral bit to 1.
            // 2. Decrement the exponent.
            ApInt::tcSetBit(parts, m_semantics->m_precision - 1);
            m_exponent--;
         }
      } else {
         // If we are positive, we need to increment the significand.

         // We only cross a binade boundary that requires adjusting the exponent if
         // the input is not a denormal and all of said input's significand bits
         // are set. If all of said conditions are true: clear the significand, set
         // the integral bit to 1, and increment the exponent. If we have a
         // denormal always increment since moving denormals and the numbers in the
         // smallest normal binade have the same exponent in our representation.
         bool WillCrossBinadeBoundary = !isDenormal() && isSignificandAllOnes();

         if (WillCrossBinadeBoundary) {
            integerPart *parts = getSignificandParts();
            ApInt::tcSet(parts, 0, getPartCount());
            ApInt::tcSetBit(parts, m_semantics->m_precision - 1);
            assert(m_exponent != m_semantics->m_maxExponent &&
                  "We can not increment an exponent beyond the maxExponent allowed"
                  " by the given floating point semantics.");
            m_exponent++;
         } else {
            incrementSignificand();
         }
      }
      break;
   }

   // If we are performing nextDown, swap sign so we have -nextUp(-x)
   if (nextDown) {
      changeSign();
   }
   return result;
}

void IEEEFloat::makeInf(bool negative)
{
   m_category = FltCategory::fcInfinity;
   m_sign = negative;
   m_exponent = m_semantics->m_maxExponent + 1;
   ApInt::tcSet(getSignificandParts(), 0, getPartCount());
}

void IEEEFloat::makeZero(bool negative)
{
   m_category = FltCategory::fcZero;
   m_sign = negative;
   m_exponent = m_semantics->m_minExponent - 1;
   ApInt::tcSet(getSignificandParts(), 0, getPartCount());
}

void IEEEFloat::makeQuiet()
{
   assert(isNaN());
   ApInt::tcSetBit(getSignificandParts(), m_semantics->m_precision - 2);
}

int ilogb(const IEEEFloat &arg)
{
   if (arg.isNaN()) {
      return IEEEFloat::IEK_NaN;
   }
   if (arg.isZero()) {
      return IEEEFloat::IEK_Zero;
   }
   if (arg.isInfinity()) {
      return IEEEFloat::IEK_Inf;
   }

   if (!arg.isDenormal()) {
      return arg.m_exponent;
   }

   IEEEFloat normalized(arg);
   int significandBits = arg.getSemantics().m_precision - 1;

   normalized.m_exponent += significandBits;
   normalized.normalize(IEEEFloat::RoundingMode::rmNearestTiesToEven, LostFraction::lfExactlyZero);
   return normalized.m_exponent - significandBits;
}

IEEEFloat scalbn(IEEEFloat value, int exp, IEEEFloat::RoundingMode roundingMode)
{
   auto maxExp = value.getSemantics().m_maxExponent;
   auto minExp = value.getSemantics().m_minExponent;

   // If Exp is wildly out-of-scale, simply adding it to X.exponent will
   // overflow; clamp it to a safe range before adding, but ensure that the range
   // is large enough that the clamp does not change the result. The range we
   // need to support is the difference between the largest possible exponent and
   // the normalized exponent of half the smallest denormal.

   int significandBits = value.getSemantics().m_precision - 1;
   int maxIncrement = maxExp - (minExp - significandBits) + 1;

   // Clamp to one past the range ends to let normalize handle overlflow.
   value.m_exponent += std::min(std::max(exp, -maxIncrement - 1), maxIncrement);
   value.normalize(roundingMode, LostFraction::lfExactlyZero);
   if (value.isNaN()) {
      value.makeQuiet();
   }
   return value;
}

IEEEFloat frexp(const IEEEFloat &value, int &exp, IEEEFloat::RoundingMode roundingMode)
{
   exp = ilogb(value);

   // Quiet signalling nans.
   if (exp == IEEEFloat::IEK_NaN) {
      IEEEFloat quiet(value);
      quiet.makeQuiet();
      return quiet;
   }

   if (exp == IEEEFloat::IEK_Inf) {
      return value;
   }

   // 1 is added because frexp is defined to return a normalized fraction in
   // +/-[0.5, 1.0), rather than the usual +/-[1.0, 2.0).
   exp = exp == IEEEFloat::IEK_Zero ? 0 : exp + 1;
   return scalbn(value, -exp, roundingMode);
}

DoubleApFloat::DoubleApFloat(const FltSemantics &semantics)
   : m_semantics(&semantics),
     m_floats(new ApFloat[2]{ApFloat(sg_semIEEEdouble), ApFloat(sg_semIEEEdouble)})
{
   assert(m_semantics == &sg_semPPCDoubleDouble);
}

DoubleApFloat::DoubleApFloat(const FltSemantics &semantics, UninitializedTag)
   : m_semantics(&semantics),
     m_floats(new ApFloat[2]{ApFloat(sg_semIEEEdouble, UninitializedTag::uninitialized),
                             ApFloat(sg_semIEEEdouble, UninitializedTag::uninitialized)}) {
   assert(m_semantics == &sg_semPPCDoubleDouble);
}

DoubleApFloat::DoubleApFloat(const FltSemantics &semantics, integerPart ivalue)
   : m_semantics(&semantics),
     m_floats(new ApFloat[2]{ApFloat(sg_semIEEEdouble, ivalue), ApFloat(sg_semIEEEdouble)})
{
   assert(m_semantics == &sg_semPPCDoubleDouble);
}

DoubleApFloat::DoubleApFloat(const FltSemantics &semantics, const ApInt &ivalue)
   : m_semantics(&semantics),
     m_floats(new ApFloat[2]{
        ApFloat(sg_semIEEEdouble, ApInt(64, ivalue.getRawData()[0])),
     ApFloat(sg_semIEEEdouble, ApInt(64, ivalue.getRawData()[1]))})
{
   assert(m_semantics == &sg_semPPCDoubleDouble);
}

DoubleApFloat::DoubleApFloat(const FltSemantics &semantics, ApFloat &&first,
                             ApFloat &&second)
   : m_semantics(&semantics),
     m_floats(new ApFloat[2]{std::move(first), std::move(second)})
{
   assert(m_semantics == &sg_semPPCDoubleDouble);
   assert(&m_floats[0].getSemantics() == &sg_semIEEEdouble);
   assert(&m_floats[1].getSemantics() == &sg_semIEEEdouble);
}

DoubleApFloat::DoubleApFloat(const DoubleApFloat &other)
   : m_semantics(other.m_semantics),
     m_floats(other.m_floats ? new ApFloat[2]{ApFloat(other.m_floats[0]),
     ApFloat(other.m_floats[1])}
   : nullptr)
{
   assert(m_semantics == &sg_semPPCDoubleDouble);
}

DoubleApFloat::DoubleApFloat(DoubleApFloat &&other)
   : m_semantics(other.m_semantics), m_floats(std::move(other.m_floats))
{
   other.m_semantics = &sg_semBogus;
   assert(m_semantics == &sg_semPPCDoubleDouble);
}

DoubleApFloat &DoubleApFloat::operator=(const DoubleApFloat &other)
{
   if (m_semantics == other.m_semantics && other.m_floats) {
      m_floats[0] = other.m_floats[0];
      m_floats[1] = other.m_floats[1];
   } else if (this != &other) {
      this->~DoubleApFloat();
      new (this) DoubleApFloat(other);
   }
   return *this;
}

// Implement addition, subtraction, multiplication and division based on:
// "Software for Doubled-Precision Floating-Point Computations",
// by Seppo Linnainmaa, ACM TOMS vol 7 no 3, September 1981, pages 272-283.
ApFloat::OpStatus DoubleApFloat::addImpl(const ApFloat &a, const ApFloat &aa,
                                         const ApFloat &c, const ApFloat &cc,
                                         RoundingMode roundingMode)
{
   int status = opOK;
   ApFloat z = a;
   status |= z.add(c, roundingMode);
   if (!z.isFinite()) {
      if (!z.isInfinity()) {
         m_floats[0] = std::move(z);
         m_floats[1].makeZero(/* Neg = */ false);
         return (OpStatus)status;
      }
      status = opOK;
      auto aComparedToC = a.compareAbsoluteValue(c);
      z = cc;
      status |= z.add(aa, roundingMode);
      if (aComparedToC == ApFloat::CmpResult::cmpGreaterThan) {
         // z = cc + aa + c + a;
         status |= z.add(c, roundingMode);
         status |= z.add(a, roundingMode);
      } else {
         // z = cc + aa + a + c;
         status |= z.add(a, roundingMode);
         status |= z.add(c, roundingMode);
      }
      if (!z.isFinite()) {
         m_floats[0] = std::move(z);
         m_floats[1].makeZero(/* Neg = */ false);
         return (OpStatus)status;
      }
      m_floats[0] = z;
      ApFloat zz = aa;
      status |= zz.add(cc, roundingMode);
      if (aComparedToC == ApFloat::CmpResult::cmpGreaterThan) {
         // m_floats[1] = a - z + c + zz;
         m_floats[1] = a;
         status |= m_floats[1].subtract(z, roundingMode);
         status |= m_floats[1].add(c, roundingMode);
         status |= m_floats[1].add(zz, roundingMode);
      } else {
         // m_floats[1] = c - z + a + zz;
         m_floats[1] = c;
         status |= m_floats[1].subtract(z, roundingMode);
         status |= m_floats[1].add(a, roundingMode);
         status |= m_floats[1].add(zz, roundingMode);
      }
   } else {
      // q = a - z;
      ApFloat q = a;
      status |= q.subtract(z, roundingMode);

      // zz = q + c + (a - (q + z)) + aa + cc;
      // Compute a - (q + z) as -((q + z) - a) to avoid temporary copies.
      auto zz = q;
      status |= zz.add(c, roundingMode);
      status |= q.add(z, roundingMode);
      status |= q.subtract(a, roundingMode);
      q.changeSign();
      status |= zz.add(q, roundingMode);
      status |= zz.add(aa, roundingMode);
      status |= zz.add(cc, roundingMode);
      if (zz.isZero() && !zz.isNegative()) {
         m_floats[0] = std::move(z);
         m_floats[1].makeZero(/* Neg = */ false);
         return opOK;
      }
      m_floats[0] = z;
      status |= m_floats[0].add(zz, roundingMode);
      if (!m_floats[0].isFinite()) {
         m_floats[1].makeZero(/* Neg = */ false);
         return (OpStatus)status;
      }
      m_floats[1] = std::move(z);
      status |= m_floats[1].subtract(m_floats[0], roundingMode);
      status |= m_floats[1].add(zz, roundingMode);
   }
   return (OpStatus)status;
}

ApFloat::OpStatus DoubleApFloat::addWithSpecial(const DoubleApFloat &lhs,
                                                const DoubleApFloat &rhs,
                                                DoubleApFloat &out,
                                                RoundingMode roundingMode)
{
   if (lhs.getCategory() == FltCategory::fcNaN) {
      out = lhs;
      return opOK;
   }
   if (rhs.getCategory() == FltCategory::fcNaN) {
      out = rhs;
      return opOK;
   }
   if (lhs.getCategory() == FltCategory::fcZero) {
      out = rhs;
      return opOK;
   }
   if (rhs.getCategory() == FltCategory::fcZero) {
      out = lhs;
      return opOK;
   }
   if (lhs.getCategory() == FltCategory::fcInfinity && rhs.getCategory() == FltCategory::fcInfinity &&
       lhs.isNegative() != rhs.isNegative())
   {
      out.makeNaN(false, out.isNegative(), nullptr);
      return opInvalidOp;
   }
   if (lhs.getCategory() == FltCategory::fcInfinity) {
      out = lhs;
      return opOK;
   }
   if (rhs.getCategory() == FltCategory::fcInfinity) {
      out = rhs;
      return opOK;
   }
   assert(lhs.getCategory() == FltCategory::fcNormal && rhs.getCategory() == FltCategory::fcNormal);

   ApFloat A(lhs.m_floats[0]), AA(lhs.m_floats[1]), C(rhs.m_floats[0]),
         CC(rhs.m_floats[1]);
   assert(&A.getSemantics() == &sg_semIEEEdouble);
   assert(&AA.getSemantics() == &sg_semIEEEdouble);
   assert(&C.getSemantics() == &sg_semIEEEdouble);
   assert(&CC.getSemantics() == &sg_semIEEEdouble);
   assert(&out.m_floats[0].getSemantics() == &sg_semIEEEdouble);
   assert(&out.m_floats[1].getSemantics() == &sg_semIEEEdouble);
   return out.addImpl(A, AA, C, CC, roundingMode);
}

ApFloat::OpStatus DoubleApFloat::add(const DoubleApFloat &rhs,
                                     RoundingMode roundingMode)
{
   return addWithSpecial(*this, rhs, *this, roundingMode);
}

ApFloat::OpStatus DoubleApFloat::subtract(const DoubleApFloat &rhs,
                                          RoundingMode roundingMode)
{
   changeSign();
   auto ret = add(rhs, roundingMode);
   changeSign();
   return ret;
}

ApFloat::OpStatus DoubleApFloat::multiply(const DoubleApFloat &other,
                                          ApFloat::RoundingMode roundingMode)
{
   const auto &lhs = *this;
   auto &out = *this;
   /* Interesting observation: For special categories, finding the lowest
     common ancestor of the following layered graph gives the correct
     return category:
        NaN
       /   \
     Zero  Inf
       \   /
       Normal
     e.g. NaN * NaN = NaN
          Zero * Inf = NaN
          Normal * Zero = Zero
          Normal * Inf = Inf
  */
   if (lhs.getCategory() == FltCategory::fcNaN) {
      out = lhs;
      return opOK;
   }
   if (other.getCategory() == FltCategory::fcNaN) {
      out = other;
      return opOK;
   }
   if ((lhs.getCategory() == FltCategory::fcZero && other.getCategory() == FltCategory::fcInfinity) ||
       (lhs.getCategory() == FltCategory::fcInfinity && other.getCategory() == FltCategory::fcZero)) {
      out.makeNaN(false, false, nullptr);
      return opOK;
   }
   if (lhs.getCategory() == FltCategory::fcZero || lhs.getCategory() == FltCategory::fcInfinity) {
      out = lhs;
      return opOK;
   }
   if (other.getCategory() == FltCategory::fcZero || other.getCategory() == FltCategory::fcInfinity) {
      out = other;
      return opOK;
   }
   assert(lhs.getCategory() == FltCategory::fcNormal && other.getCategory() == FltCategory::fcNormal &&
          "Special cases not handled exhaustively");

   int status = opOK;
   ApFloat A = m_floats[0], B = m_floats[1], C = other.m_floats[0], D = other.m_floats[1];
   // t = a * c
   ApFloat T = A;
   status |= T.multiply(C, roundingMode);
   if (!T.isFiniteNonZero()) {
      m_floats[0] = T;
      m_floats[1].makeZero(/* Neg = */ false);
      return (OpStatus)status;
   }

   // tau = fmsub(a, c, t), that is -fmadd(-a, c, t).
   ApFloat Tau = A;
   T.changeSign();
   status |= Tau.fusedMultiplyAdd(C, T, roundingMode);
   T.changeSign();
   {
      // v = a * d
      ApFloat V = A;
      status |= V.multiply(D, roundingMode);
      // w = b * c
      ApFloat W = B;
      status |= W.multiply(C, roundingMode);
      status |= V.add(W, roundingMode);
      // tau += v + w
      status |= Tau.add(V, roundingMode);
   }
   // u = t + tau
   ApFloat U = T;
   status |= U.add(Tau, roundingMode);

   m_floats[0] = U;
   if (!U.isFinite()) {
      m_floats[1].makeZero(/* Neg = */ false);
   } else {
      // m_floats[1] = (t - u) + tau
      status |= T.subtract(U, roundingMode);
      status |= T.add(Tau, roundingMode);
      m_floats[1] = T;
   }
   return (OpStatus)status;
}

ApFloat::OpStatus DoubleApFloat::divide(const DoubleApFloat &other,
                                        ApFloat::RoundingMode roundingMode)
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   ApFloat temp(sg_semPPCDoubleDoubleLegacy, bitcastToApInt());
   auto ret =
         temp.divide(ApFloat(sg_semPPCDoubleDoubleLegacy, other.bitcastToApInt()), roundingMode);
   *this = DoubleApFloat(sg_semPPCDoubleDouble, temp.bitcastToApInt());
   return ret;
}

ApFloat::OpStatus DoubleApFloat::remainder(const DoubleApFloat &other)
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   ApFloat temp(sg_semPPCDoubleDoubleLegacy, bitcastToApInt());
   auto ret =
         temp.remainder(ApFloat(sg_semPPCDoubleDoubleLegacy, other.bitcastToApInt()));
   *this = DoubleApFloat(sg_semPPCDoubleDouble, temp.bitcastToApInt());
   return ret;
}

ApFloat::OpStatus DoubleApFloat::mod(const DoubleApFloat &other)
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   ApFloat temp(sg_semPPCDoubleDoubleLegacy, bitcastToApInt());
   auto ret = temp.mod(ApFloat(sg_semPPCDoubleDoubleLegacy, other.bitcastToApInt()));
   *this = DoubleApFloat(sg_semPPCDoubleDouble, temp.bitcastToApInt());
   return ret;
}

ApFloat::OpStatus
DoubleApFloat::fusedMultiplyAdd(const DoubleApFloat &multiplicand,
                                const DoubleApFloat &addend,
                                ApFloat::RoundingMode roundingMode)
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   ApFloat temp(sg_semPPCDoubleDoubleLegacy, bitcastToApInt());
   auto ret = temp.fusedMultiplyAdd(
            ApFloat(sg_semPPCDoubleDoubleLegacy, multiplicand.bitcastToApInt()),
            ApFloat(sg_semPPCDoubleDoubleLegacy, addend.bitcastToApInt()), roundingMode);
   *this = DoubleApFloat(sg_semPPCDoubleDouble, temp.bitcastToApInt());
   return ret;
}

ApFloat::OpStatus DoubleApFloat::roundToIntegral(ApFloat::RoundingMode roundingMode)
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   ApFloat temp(sg_semPPCDoubleDoubleLegacy, bitcastToApInt());
   auto ret = temp.roundToIntegral(roundingMode);
   *this = DoubleApFloat(sg_semPPCDoubleDouble, temp.bitcastToApInt());
   return ret;
}

void DoubleApFloat::changeSign()
{
   m_floats[0].changeSign();
   m_floats[1].changeSign();
}

ApFloat::CmpResult
DoubleApFloat::compareAbsoluteValue(const DoubleApFloat &other) const
{
   auto result = m_floats[0].compareAbsoluteValue(other.m_floats[0]);
   if (result != CmpResult::cmpEqual)
      return result;
   result = m_floats[1].compareAbsoluteValue(other.m_floats[1]);
   if (result == CmpResult::cmpLessThan || result == CmpResult::cmpGreaterThan) {
      auto against = m_floats[0].isNegative() ^ m_floats[1].isNegative();
      auto otherAgainst = other.m_floats[0].isNegative() ^ other.m_floats[1].isNegative();
      if (against && !otherAgainst) {
         return CmpResult::cmpLessThan;
      }
      if (!against && otherAgainst) {
         return CmpResult::cmpGreaterThan;
      }
      if (!against && !otherAgainst) {
         return result;
      }
      if (against && otherAgainst) {
         return (CmpResult)(polar::as_integer(CmpResult::cmpLessThan) +
                            polar::as_integer(CmpResult::cmpGreaterThan) -
                            polar::as_integer(result));
      }
   }
   return result;
}

ApFloat::FltCategory DoubleApFloat::getCategory() const
{
   return m_floats[0].getCategory();
}

bool DoubleApFloat::isNegative() const
{
   return m_floats[0].isNegative();
}

void DoubleApFloat::makeInf(bool neg)
{
   m_floats[0].makeInf(neg);
   m_floats[1].makeZero(/* neg = */ false);
}

void DoubleApFloat::makeZero(bool neg)
{
   m_floats[0].makeZero(neg);
   m_floats[1].makeZero(/* neg = */ false);
}

void DoubleApFloat::makeLargest(bool neg)
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   m_floats[0] = ApFloat(sg_semIEEEdouble, ApInt(64, 0x7fefffffffffffffull));
   m_floats[1] = ApFloat(sg_semIEEEdouble, ApInt(64, 0x7c8ffffffffffffeull));
   if (neg) {
      changeSign();
   }
}

void DoubleApFloat::makeSmallest(bool neg)
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   m_floats[0].makeSmallest(neg);
   m_floats[1].makeZero(/* neg = */ false);
}

void DoubleApFloat::makeSmallestNormalized(bool neg) {
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   m_floats[0] = ApFloat(sg_semIEEEdouble, ApInt(64, 0x0360000000000000ull));
   if (neg) {
      m_floats[0].changeSign();
   }
   m_floats[1].makeZero(/* neg = */ false);
}

void DoubleApFloat::makeNaN(bool snan, bool neg, const ApInt *fill)
{
   m_floats[0].makeNaN(snan, neg, fill);
   m_floats[1].makeZero(/* neg = */ false);
}

ApFloat::CmpResult DoubleApFloat::compare(const DoubleApFloat &other) const
{
   auto result = m_floats[0].compare(other.m_floats[0]);
   // |Float[0]| > |Float[1]|
   if (result == CmpResult::cmpEqual) {
      return m_floats[1].compare(other.m_floats[1]);
   }

   return result;
}

bool DoubleApFloat::bitwiseIsEqual(const DoubleApFloat &other) const
{
   return m_floats[0].bitwiseIsEqual(other.m_floats[0]) &&
         m_floats[1].bitwiseIsEqual(other.m_floats[1]);
}

HashCode hash_value(const DoubleApFloat &arg)
{
   if (arg.m_floats) {
      return hash_combine(hash_value(arg.m_floats[0]), hash_value(arg.m_floats[1]));
   }
   return hash_combine(arg.m_semantics);
}

ApInt DoubleApFloat::bitcastToApInt() const
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   uint64_t data[] = {
      m_floats[0].bitcastToApInt().getRawData()[0],
      m_floats[1].bitcastToApInt().getRawData()[0],
   };
   return ApInt(128, 2, data);
}

ApFloat::OpStatus DoubleApFloat::convertFromString(StringRef str,
                                                   RoundingMode roundingMode)
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   ApFloat temp(sg_semPPCDoubleDoubleLegacy);
   auto ret = temp.convertFromString(str, roundingMode);
   *this = DoubleApFloat(sg_semPPCDoubleDouble, temp.bitcastToApInt());
   return ret;
}

ApFloat::OpStatus DoubleApFloat::next(bool nextDown)
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   ApFloat temp(sg_semPPCDoubleDoubleLegacy, bitcastToApInt());
   auto ret = temp.next(nextDown);
   *this = DoubleApFloat(sg_semPPCDoubleDouble, temp.bitcastToApInt());
   return ret;
}

ApFloat::OpStatus
DoubleApFloat::convertToInteger(MutableArrayRef<integerPart> input,
                                unsigned int width, bool isSigned,
                                RoundingMode roundingMode, bool *isExact) const
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   return ApFloat(sg_semPPCDoubleDoubleLegacy, bitcastToApInt())
         .convertToInteger(input, width, isSigned, roundingMode, isExact);
}

ApFloat::OpStatus DoubleApFloat::convertFromApInt(const ApInt &input,
                                                  bool isSigned,
                                                  RoundingMode roundingMode)
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   ApFloat temp(sg_semPPCDoubleDoubleLegacy);
   auto ret = temp.convertFromApInt(input, isSigned, roundingMode);
   *this = DoubleApFloat(sg_semPPCDoubleDouble, temp.bitcastToApInt());
   return ret;
}

ApFloat::OpStatus
DoubleApFloat::convertFromSignExtendedInteger(const integerPart *input,
                                              unsigned int inputSize,
                                              bool isSigned, RoundingMode roundingMode)
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   ApFloat temp(sg_semPPCDoubleDoubleLegacy);
   auto ret = temp.convertFromSignExtendedInteger(input, inputSize, isSigned, roundingMode);
   *this = DoubleApFloat(sg_semPPCDoubleDouble, temp.bitcastToApInt());
   return ret;
}

ApFloat::OpStatus
DoubleApFloat::convertFromZeroExtendedInteger(const integerPart *input,
                                              unsigned int inputSize,
                                              bool isSigned, RoundingMode roundingMode)
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   ApFloat temp(sg_semPPCDoubleDoubleLegacy);
   auto ret = temp.convertFromZeroExtendedInteger(input, inputSize, isSigned, roundingMode);
   *this = DoubleApFloat(sg_semPPCDoubleDouble, temp.bitcastToApInt());
   return ret;
}

unsigned int DoubleApFloat::convertToHexString(char *dest,
                                               unsigned int hexDigits,
                                               bool upperCase,
                                               RoundingMode roundingMode) const
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   return ApFloat(sg_semPPCDoubleDoubleLegacy, bitcastToApInt())
         .convertToHexString(dest, hexDigits, upperCase, roundingMode);
}

bool DoubleApFloat::isDenormal() const
{
   return getCategory() == FltCategory::fcNormal &&
         (m_floats[0].isDenormal() || m_floats[1].isDenormal() ||
         // (double)(Hi + Lo) == Hi defines a normal number.
         m_floats[0].compare(m_floats[0] + m_floats[1]) != CmpResult::cmpEqual);
}

bool DoubleApFloat::isSmallest() const
{
   if (getCategory() != FltCategory::fcNormal) {
      return false;
   }
   DoubleApFloat temp(*this);
   temp.makeSmallest(this->isNegative());
   return temp.compare(*this) == CmpResult::cmpEqual;
}

bool DoubleApFloat::isLargest() const
{
   if (getCategory() != FltCategory::fcNormal) {
      return false;
   }

   DoubleApFloat temp(*this);
   temp.makeLargest(this->isNegative());
   return temp.compare(*this) == CmpResult::cmpEqual;
}

bool DoubleApFloat::isInteger() const
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   return m_floats[0].isInteger() && m_floats[1].isInteger();
}

void DoubleApFloat::toString(SmallVectorImpl<char> &str,
                             unsigned formatPrecision,
                             unsigned formatMaxPadding,
                             bool truncateZero) const
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   ApFloat(sg_semPPCDoubleDoubleLegacy, bitcastToApInt())
         .toString(str, formatPrecision, formatMaxPadding, truncateZero);
}

bool DoubleApFloat::getExactInverse(ApFloat *fvalue) const
{
   assert(m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   ApFloat temp(sg_semPPCDoubleDoubleLegacy, bitcastToApInt());
   if (!fvalue) {
      return temp.getExactInverse(nullptr);
   }
   ApFloat tempFvalue(sg_semPPCDoubleDoubleLegacy);
   auto ret = temp.getExactInverse(&tempFvalue);
   *fvalue = ApFloat(sg_semPPCDoubleDouble, tempFvalue.bitcastToApInt());
   return ret;
}

DoubleApFloat scalbn(DoubleApFloat arg, int exp, ApFloat::RoundingMode roundingMode)
{
   assert(arg.m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   return DoubleApFloat(sg_semPPCDoubleDouble, scalbn(arg.m_floats[0], exp, roundingMode),
         scalbn(arg.m_floats[1], exp, roundingMode));
}

DoubleApFloat frexp(const DoubleApFloat &arg, int &exp,
                    ApFloat::RoundingMode roundingMode)
{
   assert(arg.m_semantics == &sg_semPPCDoubleDouble && "Unexpected Semantics");
   ApFloat first = frexp(arg.m_floats[0], exp, roundingMode);
   ApFloat second = arg.m_floats[1];
   if (arg.getCategory() == ApFloat::FltCategory::fcNormal) {
      second = scalbn(second, -exp, roundingMode);
   }
   return DoubleApFloat(sg_semPPCDoubleDouble, std::move(first), std::move(second));
}

} // End internal namespace

ApFloat::Storage::Storage(IEEEFloat fvalue, const FltSemantics &semantics)
{
   if (usesLayout<IEEEFloat>(semantics)) {
      new (&m_ieee) IEEEFloat(std::move(fvalue));
      return;
   }
   if (usesLayout<DoubleApFloat>(semantics)) {
      new (&m_dvalue)
            DoubleApFloat(semantics, ApFloat(std::move(fvalue), fvalue.getSemantics()),
                          ApFloat(sg_semIEEEdouble));
      return;
   }
   polar_unreachable("Unexpected semantics");
}

ApFloat::OpStatus ApFloat::convertFromString(StringRef str, RoundingMode roundingMode)
{
   ApFloat_DISPATCH_ON_SEMANTICS(convertFromString(str, roundingMode));
}

HashCode hash_value(const ApFloat &arg)
{
   if (ApFloat::usesLayout<internal::IEEEFloat>(arg.getSemantics())) {
      return hash_value(arg.m_storage.m_ieee);
   }
   if (ApFloat::usesLayout<internal::DoubleApFloat>(arg.getSemantics())) {
      return hash_value(arg.m_storage.m_dvalue);
   }
   polar_unreachable("Unexpected semantics");
}

ApFloat::ApFloat(const FltSemantics &semantics, StringRef str)
   : ApFloat(semantics)
{
   convertFromString(str, RoundingMode::rmNearestTiesToEven);
}

ApFloat::OpStatus ApFloat::convert(const FltSemantics &toSemantics,
                                   RoundingMode roundingMode, bool *losesInfo)
{
   if (&getSemantics() == &toSemantics) {
      *losesInfo = false;
      return opOK;
   }

   if (usesLayout<IEEEFloat>(getSemantics()) &&
       usesLayout<IEEEFloat>(toSemantics)) {
      return m_storage.m_ieee.convert(toSemantics, roundingMode, losesInfo);
   }

   if (usesLayout<IEEEFloat>(getSemantics()) &&
       usesLayout<DoubleApFloat>(toSemantics)) {
      assert(&toSemantics == &sg_semPPCDoubleDouble);
      auto ret = m_storage.m_ieee.convert(sg_semPPCDoubleDoubleLegacy, roundingMode, losesInfo);
      *this = ApFloat(toSemantics, m_storage.m_ieee.bitcastToApInt());
      return ret;
   }
   if (usesLayout<DoubleApFloat>(getSemantics()) &&
       usesLayout<IEEEFloat>(toSemantics)) {
      auto ret = getIEEE().convert(toSemantics, roundingMode, losesInfo);
      *this = ApFloat(std::move(getIEEE()), toSemantics);
      return ret;
   }
   polar_unreachable("Unexpected semantics");
}

ApFloat ApFloat::getAllOnesValue(unsigned bitWidth, bool isIEEE) {
   if (isIEEE) {
      switch (bitWidth) {
      case 16:
         return ApFloat(sg_semIEEEhalf, ApInt::getAllOnesValue(bitWidth));
      case 32:
         return ApFloat(sg_semIEEEsingle, ApInt::getAllOnesValue(bitWidth));
      case 64:
         return ApFloat(sg_semIEEEdouble, ApInt::getAllOnesValue(bitWidth));
      case 80:
         return ApFloat(sg_semX87DoubleExtended, ApInt::getAllOnesValue(bitWidth));
      case 128:
         return ApFloat(sg_semIEEEquad, ApInt::getAllOnesValue(bitWidth));
      default:
         polar_unreachable("Unknown floating bit width");
         break;
      }
   } else {
      assert(bitWidth == 128);
      return ApFloat(sg_semPPCDoubleDouble, ApInt::getAllOnesValue(bitWidth));
   }
}

void ApFloat::print(RawOutStream &outstream) const
{
   SmallVector<char, 16> buffer;
   toString(buffer);
   outstream << buffer << "\n";
}

#if !defined(NDEBUG) || defined(POLAR_ENABLE_DUMP)
POLAR_DUMP_METHOD void ApFloat::dump() const
{
   print(debug_stream());
}
#endif

void ApFloat::profile(FoldingSetNodeId &nid) const
{
   nid.add(bitcastToApInt());
}

/* Same as convertToInteger(integerPart*, ...), except the result is returned in
   an APSInt, whose initial bit-width and signed-ness are used to determine the
   precision of the conversion.
 */
ApFloat::OpStatus ApFloat::convertToInteger(ApSInt &result,
                                            RoundingMode roundingMode,
                                            bool *isExact) const
{
   unsigned bitWidth = result.getBitWidth();
   SmallVector<uint64_t, 4> parts(result.getNumWords());
   OpStatus status = convertToInteger(parts, bitWidth, result.isSigned(),
                                      roundingMode, isExact);
   // Keeps the original signed-ness.
   result = ApInt(bitWidth, parts);
   return status;
}

} // basic
} // polar
