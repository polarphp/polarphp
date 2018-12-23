// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/04.

//===----------------------------------------------------------------------===//
//
// This file contains functions (and a class) useful for working with scaled
// numbers -- in particular, pairs of integers where one represents digits and
// another represents a scale.  The functions are helpers and live in the
// namespace ScaledNumbers.  The class ScaledNumber is useful for modelling
// certain cost metrics that need simple, integer-like semantics that are easy
// to reason about.
//
// These might remind you of soft-floats.  If you want one of those, you're in
// the wrong place.  Look at include/polarphp/basic/adt/ApFloat.h instead.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_SCALED_NUMBER_H
#define POLARPHP_UTILS_SCALED_NUMBER_H

#include "polarphp/utils/MathExtras.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <tuple>
#include <utility>

namespace polar {
namespace utils {
namespace scalednumbers {

/// \brief Maximum m_scale; same as APFloat for easy debug printing.
const int32_t MAX_SCALE = 16383;

/// \brief Maximum m_scale; same as APFloat for easy debug printing.
const int32_t MIN_SCALE = -16382;

/// \brief Get the m_width of a number.
template <typename DigitsT>
inline int get_width()
{
   return sizeof(DigitsT) * 8;
}

/// \brief Conditionally round up a scaled number.
///
/// Given \c m_digits and \c m_scale, round up iff \c shouldround is \c true.
/// Always returns \c m_scale unless there's an overflow, in which case it
/// returns \c 1+m_scale.
///
/// \pre adding 1 to \c m_scale will not overflow INT16_MAX.
template <typename DigitsT>
inline std::pair<DigitsT, int16_t>
get_rounded(DigitsT m_digits, int16_t m_scale, bool shouldround)
{
   static_assert(!std::numeric_limits<DigitsT>::is_signed, "expected unsigned");
   if (shouldround) {
      if (!++m_digits) {
         // Overflow.
         return std::make_pair(DigitsT(1) << (get_width<DigitsT>() - 1), m_scale + 1);
      }
   }
   return std::make_pair(m_digits, m_scale);
}

/// \brief Convenience helper for 32-bit rounding.
inline std::pair<uint32_t, int16_t> get_rounded32(uint32_t m_digits, int16_t m_scale,
                                                  bool shouldround)
{
   return get_rounded(m_digits, m_scale, shouldround);
}

/// \brief Convenience helper for 64-bit rounding.
inline std::pair<uint64_t, int16_t> get_rounded64(uint64_t m_digits, int16_t m_scale,
                                                  bool shouldround)
{
   return get_rounded(m_digits, m_scale, shouldround);
}

/// \brief Adjust a 64-bit scaled number down to the appropriate m_width.
///
/// \pre Adding 64 to \c m_scale will not overflow INT16_MAX.
template <typename DigitsT>
inline std::pair<DigitsT, int16_t> get_adjusted(uint64_t m_digits,
                                                int16_t m_scale = 0)
{
   static_assert(!std::numeric_limits<DigitsT>::is_signed, "expected unsigned");

   const int m_width = get_width<DigitsT>();
   if (m_width == 64 || m_digits <= std::numeric_limits<DigitsT>::max())
      return std::make_pair(m_digits, m_scale);

   // shift right and round.
   int shift = 64 - m_width - count_leading_zeros(m_digits);
   return get_rounded<DigitsT>(m_digits >> shift, m_scale + shift,
                               m_digits & (UINT64_C(1) << (shift - 1)));
}

/// \brief Convenience helper for adjusting to 32 bits.
inline std::pair<uint32_t, int16_t> get_adjusted32(uint64_t m_digits,
                                                   int16_t m_scale = 0)
{
   return get_adjusted<uint32_t>(m_digits, m_scale);
}

/// \brief Convenience helper for adjusting to 64 bits.
inline std::pair<uint64_t, int16_t> get_adjusted64(uint64_t m_digits,
                                                   int16_t m_scale = 0)
{
   return get_adjusted<uint64_t>(m_digits, m_scale);
}

/// \brief Multiply two 64-bit integers to create a 64-bit scaled number.
///
/// Implemented with four 64-bit integer multiplies.
std::pair<uint64_t, int16_t> multiply64(uint64_t lhs, uint64_t rhs);

/// \brief Multiply two 32-bit integers to create a 32-bit scaled number.
///
/// Implemented with one 64-bit integer multiply.
template <typename DigitsT>
inline std::pair<DigitsT, int16_t> getProduct(DigitsT lhs, DigitsT rhs)
{
   static_assert(!std::numeric_limits<DigitsT>::is_signed, "expected unsigned");

   if (get_width<DigitsT>() <= 32 || (lhs <= UINT32_MAX && rhs <= UINT32_MAX)) {
      return get_adjusted<DigitsT>(uint64_t(lhs) * rhs);
   }
   return multiply64(lhs, rhs);
}

/// \brief Convenience helper for 32-bit product.
inline std::pair<uint32_t, int16_t> get_product32(uint32_t lhs, uint32_t rhs)
{
   return getProduct(lhs, rhs);
}

/// \brief Convenience helper for 64-bit product.
inline std::pair<uint64_t, int16_t> get_product64(uint64_t lhs, uint64_t rhs)
{
   return getProduct(lhs, rhs);
}

/// \brief Divide two 64-bit integers to create a 64-bit scaled number.
///
/// Implemented with long division.
///
/// \pre \c dividend and \c divisor are non-zero.
std::pair<uint64_t, int16_t> divide64(uint64_t dividend, uint64_t divisor);

/// \brief Divide two 32-bit integers to create a 32-bit scaled number.
///
/// Implemented with one 64-bit integer divide/remainder pair.
///
/// \pre \c dividend and \c divisor are non-zero.
std::pair<uint32_t, int16_t> divide32(uint32_t dividend, uint32_t divisor);

/// \brief Divide two 32-bit numbers to create a 32-bit scaled number.
///
/// Implemented with one 64-bit integer divide/remainder pair.
///
/// Returns \c (DigitsT_MAX, MAX_SCALE) for divide-by-zero (0 for 0/0).
template <typename DigitsT>
std::pair<DigitsT, int16_t> getQuotient(DigitsT dividend, DigitsT divisor)
{
   static_assert(!std::numeric_limits<DigitsT>::is_signed, "expected unsigned");
   static_assert(sizeof(DigitsT) == 4 || sizeof(DigitsT) == 8,
                 "expected 32-bit or 64-bit m_digits");

   // Check for zero.
   if (!dividend) {
      return std::make_pair(0, 0);
   }
   if (!divisor) {
      return std::make_pair(std::numeric_limits<DigitsT>::max(), MAX_SCALE);
   }
   if (get_width<DigitsT>() == 64) {
      return divide64(dividend, divisor);
   }
   return divide32(dividend, divisor);
}

/// \brief Convenience helper for 32-bit quotient.
inline std::pair<uint32_t, int16_t> get_quotient32(uint32_t dividend,
                                                   uint32_t divisor)
{
   return getQuotient(dividend, divisor);
}

/// \brief Convenience helper for 64-bit quotient.
inline std::pair<uint64_t, int16_t> get_quotient64(uint64_t dividend,
                                                   uint64_t divisor)
{
   return getQuotient(dividend, divisor);
}

/// \brief Implementation of get_lg() and friends.
///
/// Returns the rounded lg of \c m_digits*2^m_scale and an int specifying whether
/// this was rounded up (1), down (-1), or exact (0).
///
/// Returns \c INT32_MIN when \c m_digits is zero.
template <typename DigitsT>
inline std::pair<int32_t, int> get_lg_impl(DigitsT m_digits, int16_t m_scale)
{
   static_assert(!std::numeric_limits<DigitsT>::is_signed, "expected unsigned");

   if (!m_digits) {
      return std::make_pair(INT32_MIN, 0);
   }

   // Get the floor of the lg of m_digits.
   int32_t localfloor = sizeof(m_digits) * 8 - count_leading_zeros(m_digits) - 1;
   // Get the actual floor.
   int32_t floor = m_scale + localfloor;
   if (m_digits == UINT64_C(1) << localfloor)
      return std::make_pair(floor, 0);

   // round based on the next digit.
   assert(localfloor >= 1);
   bool round = m_digits & UINT64_C(1) << (localfloor - 1);
   return std::make_pair(floor + round, round ? 1 : -1);
}

/// \brief Get the lg (rounded) of a scaled number.
///
/// Get the lg of \c m_digits*2^m_scale.
///
/// Returns \c INT32_MIN when \c m_digits is zero.
template <typename DigitsT>
int32_t get_lg(DigitsT m_digits, int16_t m_scale)
{
   return get_lg_impl(m_digits, m_scale).first;
}

/// \brief Get the lg floor of a scaled number.
///
/// Get the floor of the lg of \c m_digits*2^m_scale.
///
/// Returns \c INT32_MIN when \c m_digits is zero.
template <typename DigitsT>
int32_t get_lg_floor(DigitsT m_digits, int16_t m_scale)
{
   auto lg = get_lg_impl(m_digits, m_scale);
   return lg.first - (lg.second > 0);
}

/// \brief Get the lg ceiling of a scaled number.
///
/// Get the ceiling of the lg of \c m_digits*2^m_scale.
///
/// Returns \c INT32_MIN when \c m_digits is zero.
template <typename DigitsT>
int32_t get_lg_ceiling(DigitsT m_digits, int16_t m_scale)
{
   auto lg = get_lg_impl(m_digits, m_scale);
   return lg.first + (lg.second < 0);
}

/// \brief Implementation for comparing scaled numbers.
///
/// Compare two 64-bit numbers with different scales.  Given that the m_scale of
/// \c lhs is higher than that of \c rhs by \c scaleDiff, compare them.  Return -1,
/// 1, and 0 for less than, greater than, and equal, respectively.
///
/// \pre 0 <= scaleDiff < 64.
int compare_impl(uint64_t lhs, uint64_t rhs, int scaleDiff);

/// \brief Compare two scaled numbers.
///
/// Compare two scaled numbers.  Returns 0 for equal, -1 for less than, and 1
/// for greater than.
template <typename DigitsT>
int compare(DigitsT lhsDigits, int16_t lhsScale, DigitsT rhsDigits, int16_t rhsScale)
{
   static_assert(!std::numeric_limits<DigitsT>::is_signed, "expected unsigned");
   // Check for zero.
   if (!lhsDigits) {
      return rhsDigits ? -1 : 0;
   }
   if (!rhsDigits) {
      return 1;
   }
   // Check for the m_scale.  Use get_lg_floor to be sure that the m_scale difference
   // is always lower than 64.
   int32_t lgL = get_lg_floor(lhsDigits, lhsScale), lgR = get_lg_floor(rhsDigits, rhsScale);
   if (lgL != lgR) {
      return lgL < lgR ? -1 : 1;
   }

   // Compare m_digits.
   if (lhsScale < rhsScale) {
      return compare_impl(lhsDigits, rhsDigits, rhsScale - lhsScale);
   }
   return -compare_impl(rhsDigits, lhsDigits, lhsScale - rhsScale);
}

/// \brief Match scales of two numbers.
///
/// Given two scaled numbers, match up their scales.  Change the m_digits and
/// scales in place.  shift the m_digits as necessary to form equivalent numbers,
/// losing precision only when necessary.
///
/// If the output value of \c lhsDigits (\c rhsDigits) is \c 0, the output value of
/// \c lhsScale (\c rhsScale) is unspecified.
///
/// As a convenience, returns the matching m_scale.  If the output value of one
/// number is zero, returns the m_scale of the other.  If both are zero, which
/// m_scale is returned is unspecified.
template <typename DigitsT>
int16_t match_scales(DigitsT &lhsDigits, int16_t &lhsScale, DigitsT &rhsDigits,
                     int16_t &rhsScale)
{
   static_assert(!std::numeric_limits<DigitsT>::is_signed, "expected unsigned");

   if (lhsScale < rhsScale) {
      // Swap arguments.
      return match_scales(rhsDigits, rhsScale, lhsDigits, lhsScale);
   }

   if (!lhsDigits) {
      return rhsScale;
   }

   if (!rhsDigits || lhsScale == rhsScale) {
      return lhsScale;
   }
   // Now lhsScale > rhsScale.  Get the difference.
   int32_t scaleDiff = int32_t(lhsScale) - rhsScale;
   if (scaleDiff >= 2 * get_width<DigitsT>()) {
      // Don't bother shifting.  rhsDigits will get zero-ed out anyway.
      rhsDigits = 0;
      return lhsScale;
   }

   // shift lhsDigits left as much as possible, then shift rhsDigits right.
   int32_t shiftL = std::min<int32_t>(count_leading_zeros(lhsDigits), scaleDiff);
   assert(shiftL < get_width<DigitsT>() && "can't shift more than m_width");
   int32_t ShiftR = scaleDiff - shiftL;
   if (ShiftR >= get_width<DigitsT>()) {
      // Don't bother shifting.  rhsDigits will get zero-ed out anyway.
      rhsDigits = 0;
      return lhsScale;
   }

   lhsDigits <<= shiftL;
   rhsDigits >>= ShiftR;

   lhsScale -= shiftL;
   rhsScale += ShiftR;
   assert(lhsScale == rhsScale && "scales should match");
   return lhsScale;
}

/// \brief Get the sum of two scaled numbers.
///
/// Get the sum of two scaled numbers with as much precision as possible.
///
/// \pre Adding 1 to \c lhsScale (or \c rhsScale) will not overflow INT16_MAX.
template <typename DigitsT>
std::pair<DigitsT, int16_t> get_sum(DigitsT lhsDigits, int16_t lhsScale,
                                    DigitsT rhsDigits, int16_t rhsScale)
{
   static_assert(!std::numeric_limits<DigitsT>::is_signed, "expected unsigned");

   // Check inputs up front.  This is only relevant if addition overflows, but
   // testing here should catch more bugs.
   assert(lhsScale < INT16_MAX && "m_scale too large");
   assert(rhsScale < INT16_MAX && "m_scale too large");

   // Normalize m_digits to match scales.
   int16_t m_scale = match_scales(lhsDigits, lhsScale, rhsDigits, rhsScale);

   // Compute sum.
   DigitsT sum = lhsDigits + rhsDigits;
   if (sum >= rhsDigits) {
      return std::make_pair(sum, m_scale);
   }

   // Adjust sum after arithmetic overflow.
   DigitsT highBit = DigitsT(1) << (get_width<DigitsT>() - 1);
   return std::make_pair(highBit | sum >> 1, m_scale + 1);
}

/// \brief Convenience helper for 32-bit sum.
inline std::pair<uint32_t, int16_t>
get_sum32(uint32_t lhsDigits, int16_t lhsScale,
          uint32_t rhsDigits, int16_t rhsScale)
{
   return get_sum(lhsDigits, lhsScale, rhsDigits, rhsScale);
}

/// \brief Convenience helper for 64-bit sum.
inline std::pair<uint64_t, int16_t>
get_sum64(uint64_t lhsDigits, int16_t lhsScale,
          uint64_t rhsDigits, int16_t rhsScale)
{
   return get_sum(lhsDigits, lhsScale, rhsDigits, rhsScale);
}

/// \brief Get the difference of two scaled numbers.
///
/// Get lhs minus rhs with as much precision as possible.
///
/// Returns \c (0, 0) if the rhs is larger than the lhs.
template <typename DigitsT>
std::pair<DigitsT, int16_t> get_difference(DigitsT lhsDigits, int16_t lhsScale,
                                           DigitsT rhsDigits, int16_t rhsScale)
{
   static_assert(!std::numeric_limits<DigitsT>::is_signed, "expected unsigned");

   // Normalize m_digits to match scales.
   const DigitsT savedRDigits = rhsDigits;
   const int16_t savedRScale = rhsScale;
   match_scales(lhsDigits, lhsScale, rhsDigits, rhsScale);

   // Compute difference.
   if (lhsDigits <= rhsDigits) {
      return std::make_pair(0, 0);
   }

   if (rhsDigits || !savedRDigits) {
      return std::make_pair(lhsDigits - rhsDigits, lhsScale);
   }

   // Check if rhsDigits just barely lost its last bit.  E.g., for 32-bit:
   //
   //   1*2^32 - 1*2^0 == 0xffffffff != 1*2^32
   const auto rlgfloor = get_lg_floor(savedRDigits, savedRScale);
   if (!compare(lhsDigits, lhsScale, DigitsT(1), rlgfloor + get_width<DigitsT>())) {
      return std::make_pair(std::numeric_limits<DigitsT>::max(), rlgfloor);
   }
   return std::make_pair(lhsDigits, lhsScale);
}

/// \brief Convenience helper for 32-bit difference.
inline std::pair<uint32_t, int16_t> get_difference32(uint32_t lhsDigits,
                                                     int16_t lhsScale,
                                                     uint32_t rhsDigits,
                                                     int16_t rhsScale)
{
   return get_difference(lhsDigits, lhsScale, rhsDigits, rhsScale);
}

/// \brief Convenience helper for 64-bit difference.
inline std::pair<uint64_t, int16_t> get_difference64(uint64_t lhsDigits,
                                                     int16_t lhsScale,
                                                     uint64_t rhsDigits,
                                                     int16_t rhsScale)
{
   return get_difference(lhsDigits, lhsScale, rhsDigits, rhsScale);
}

} // end namespace scalednumbers

class RawOutStream;

class ScaledNumberBase
{
public:
   static const int sm_defaultPrecision = 10;

   static void dump(uint64_t D, int16_t E, int m_width);
   static RawOutStream &print(RawOutStream &outstream, uint64_t D, int16_t E, int width,
                              unsigned precision);
   static std::string toString(uint64_t D, int16_t E, int width,
                               unsigned precision);

   static int countLeadingZeros32(uint32_t value)
   {
      return count_leading_zeros(value);
   }

   static int countLeadingZeros64(uint64_t value)
   {
      return count_leading_zeros(value);
   }

   static uint64_t getHalf(uint64_t value)
   {
      return (value >> 1) + (value & 1);
   }

   static std::pair<uint64_t, bool> splitSigned(int64_t value)
   {
      if (value >= 0) {
         return std::make_pair(value, false);
      }

      uint64_t unsignedValue = value == INT64_MIN ? UINT64_C(1) << 63 : uint64_t(-value);
      return std::make_pair(unsignedValue, true);
   }

   static int64_t joinSigned(uint64_t value, bool isNeg)
   {
      if (value > uint64_t(INT64_MAX)) {
         return isNeg ? INT64_MIN : INT64_MAX;
      }
      return isNeg ? -int64_t(value) : int64_t(value);
   }
};

/// Simple representation of a scaled number.
///
/// ScaledNumber is a number represented by m_digits and a m_scale.  It uses simple
/// saturation arithmetic and every operation is well-defined for every value.
/// It's somewhat similar in behaviour to a soft-float, but is *not* a
/// replacement for one.  If you're doing numerics, look at \a APFloat instead.
/// Nevertheless, we've found these semantics useful for modelling certain cost
/// metrics.
///
/// The number is split into a signed m_scale and unsigned m_digits.  The number
/// represented is \c getDigits()*2^getScale().  In this way, the m_digits are
/// much like the mantissa in the x87 long double, but there is no canonical
/// form so the same number can be represented by many bit representations.
///
/// ScaledNumber is templated on the underlying integer type for m_digits, which
/// is expected to be unsigned.
///
/// Unlike APFloat, ScaledNumber does not model architecture floating point
/// behaviour -- while this might make it a little faster and easier to reason
/// about, it certainly makes it more dangerous for general numerics.
///
/// ScaledNumber is totally ordered.  However, there is no canonical form, so
/// there are multiple representations of most scalars.  E.g.:
///
///     ScaledNumber(8u, 0) == ScaledNumber(4u, 1)
///     ScaledNumber(4u, 1) == ScaledNumber(2u, 2)
///     ScaledNumber(2u, 2) == ScaledNumber(1u, 3)
///
/// ScaledNumber implements most arithmetic operations.  precision is kept
/// where possible.  Uses simple saturation arithmetic, so that operations
/// saturate to 0.0 or getLargest() rather than under or overflowing.  It has
/// some extra arithmetic for unit inversion.  0.0/0.0 is defined to be 0.0.
/// Any other division by 0.0 is defined to be getLargest().
///
/// As a convenience for modifying the exponent, left and right shifting are
/// both implemented, and both interpret negative shifts as positive shifts in
/// the opposite direction.
///
/// scales are limited to the range accepted by x87 long double.  This makes
/// it trivial to add functionality to convert to APFloat (this is already
/// relied on for the implementation of printing).
///
/// Possible (and conflicting) future directions:
///
///  1. Turn this into a wrapper around \a APFloat.
///  2. Share the algorithm implementations with \a APFloat.
///  3. Allow \a ScaledNumber to represent a signed number.
template <typename DigitsT> class ScaledNumber : ScaledNumberBase
{
public:
   static_assert(!std::numeric_limits<DigitsT>::is_signed,
                 "only unsigned floats supported");

   typedef DigitsT DigitsType;

private:
   typedef std::numeric_limits<DigitsType> DigitsLimits;

   static const int m_width = sizeof(DigitsType) * 8;
   static_assert(m_width <= 64, "invalid integer width for digits");

private:
   DigitsType m_digits = 0;
   int16_t m_scale = 0;

public:
   ScaledNumber() = default;

   constexpr ScaledNumber(DigitsType digits, int16_t scale)
      : m_digits(digits), m_scale(scale)
   {}

private:
   ScaledNumber(const std::pair<DigitsT, int16_t> &value)
      : m_digits(value.first), m_scale(value.second)
   {}

public:
   static ScaledNumber getZero()
   {
      return ScaledNumber(0, 0);
   }

   static ScaledNumber getOne()
   {
      return ScaledNumber(1, 0);
   }

   static ScaledNumber getLargest()
   {
      return ScaledNumber(DigitsLimits::max(), scalednumbers::MAX_SCALE);
   }

   static ScaledNumber get(uint64_t value)
   {
      return adjustToWidth(value, 0);
   }

   static ScaledNumber getInverse(uint64_t value)
   {
      return get(value).invert();
   }

   static ScaledNumber getFraction(DigitsType numerator, DigitsType denominator)
   {
      return getQuotient(numerator, denominator);
   }

   int16_t getScale() const
   {
      return m_scale;
   }

   DigitsType getDigits() const
   {
      return m_digits;
   }

   /// \brief Convert to the given integer type.
   ///
   /// Convert to \c IntT using simple saturating arithmetic, truncating if
   /// necessary.
   template <typename IntT>
   IntT toInt() const;

   bool isZero() const
   {
      return !m_digits;
   }

   bool isLargest() const
   {
      return *this == getLargest();
   }

   bool isOne() const
   {
      if (m_scale > 0 || m_scale <= -m_width) {
         return false;
      }
      return m_digits == DigitsType(1) << -m_scale;
   }

   /// \brief The log base 2, rounded.
   ///
   /// Get the lg of the scalar.  lg 0 is defined to be INT32_MIN.
   int32_t lg() const
   {
      return scalednumbers::get_lg(m_digits, m_scale);
   }

   /// \brief The log base 2, rounded towards INT32_MIN.
   ///
   /// Get the lg floor.  lg 0 is defined to be INT32_MIN.
   int32_t lgfloor() const
   {
      return scalednumbers::get_lg_floor(m_digits, m_scale);
   }

   /// \brief The log base 2, rounded towards INT32_MAX.
   ///
   /// Get the lg ceiling.  lg 0 is defined to be INT32_MIN.
   int32_t lgCeiling() const
   {
      return scalednumbers::get_lg_ceiling(m_digits, m_scale);
   }

   bool operator==(const ScaledNumber &other) const
   {
      return compare(other) == 0;
   }

   bool operator<(const ScaledNumber &other) const
   {
      return compare(other) < 0;
   }

   bool operator!=(const ScaledNumber &other) const
   {
      return compare(other) != 0;
   }

   bool operator>(const ScaledNumber &other) const
   {
      return compare(other) > 0;
   }

   bool operator<=(const ScaledNumber &other) const
   {
      return compare(other) <= 0;
   }

   bool operator>=(const ScaledNumber &other) const
   {
      return compare(other) >= 0;
   }

   bool operator!() const
   {
      return isZero();
   }

   /// \brief Convert to a decimal representation in a string.
   ///
   /// Convert to a string.  Uses scientific notation for very large/small
   /// numbers.  Scientific notation is used roughly for numbers outside of the
   /// range 2^-64 through 2^64.
   ///
   /// \c precision indicates the number of decimal m_digits of precision to use;
   /// 0 requests the maximum available.
   ///
   /// As a special case to make debugging easier, if the number is small enough
   /// to convert without scientific notation and has more than \c precision
   /// m_digits before the decimal place, it's printed accurately to the first
   /// digit past zero.  E.g., assuming 10 m_digits of precision:
   ///
   ///     98765432198.7654... => 98765432198.8
   ///      8765432198.7654... =>  8765432198.8
   ///       765432198.7654... =>   765432198.8
   ///        65432198.7654... =>    65432198.77
   ///         5432198.7654... =>     5432198.765
   std::string toString(unsigned precision = sm_defaultPrecision)
   {
      return ScaledNumberBase::toString(m_digits, m_scale, m_width, precision);
   }

   /// \brief Print a decimal representation.
   ///
   /// Print a string.  See toString for documentation.
   RawOutStream &print(RawOutStream &outstream,
                       unsigned precision = sm_defaultPrecision) const
   {
      return ScaledNumberBase::print(outstream, m_digits, m_scale, m_width, precision);
   }

   void dump() const
   {
      return ScaledNumberBase::dump(m_digits, m_scale, m_width);
   }

   ScaledNumber &operator+=(const ScaledNumber &value)
   {
      std::tie(m_digits, m_scale) =
            scalednumbers::get_sum(m_digits, m_scale, value.m_digits, value.m_scale);
      // Check for exponent past MAX_SCALE.
      if (m_scale > scalednumbers::MAX_SCALE) {
         *this = getLargest();
      }
      return *this;
   }

   ScaledNumber &operator-=(const ScaledNumber &value)
   {
      std::tie(m_digits, m_scale) =
            scalednumbers::get_difference(m_digits, m_scale, value.m_digits, value.m_scale);
      return *this;
   }
   ScaledNumber &operator*=(const ScaledNumber &value);
   ScaledNumber &operator/=(const ScaledNumber &value);
   ScaledNumber &operator<<=(int16_t shift)
   {
      shiftLeft(shift);
      return *this;
   }

   ScaledNumber &operator>>=(int16_t shift)
   {
      shiftRight(shift);
      return *this;
   }

private:
   void shiftLeft(int32_t shift);
   void shiftRight(int32_t shift);

   /// \brief Adjust two floats to have matching exponents.
   ///
   /// Adjust \c this and \c value to have matching exponents.  Returns the new \c value
   /// by value.  Does nothing if \a isZero() for either.
   ///
   /// The value that compares smaller will lose precision, and possibly become
   /// \a isZero().
   ScaledNumber match_scales(ScaledNumber value)
   {
      scalednumbers::match_scales(m_digits, m_scale, value.m_digits, value.m_scale);
      return value;
   }

public:
   /// \brief m_scale a large number accurately.
   ///
   /// m_scale value (multiply it by this).  Uses full precision multiplication, even
   /// if m_width is smaller than 64, so information is not lost.
   uint64_t scale(uint64_t value) const;
   uint64_t scaleByInverse(uint64_t value) const
   {
      // TODO: implement directly, rather than relying on inverse.  Inverse is
      // expensive.
      return inverse().scale(value);
   }

   int64_t scale(int64_t value) const
   {
      std::pair<uint64_t, bool> unsignedValue = splitSigned(value);
      return joinSigned(scale(unsignedValue.first), unsignedValue.second);
   }

   int64_t scaleByInverse(int64_t value) const
   {
      std::pair<uint64_t, bool> unsignedValue = splitSigned(value);
      return joinSigned(scaleByInverse(unsignedValue.first), unsignedValue.second);
   }

   int compare(const ScaledNumber &value) const
   {
      return scalednumbers::compare(m_digits, m_scale, value.m_digits, value.m_scale);
   }

   int compareTo(uint64_t value) const
   {
      return scalednumbers::compare<uint64_t>(m_digits, m_scale, value, 0);
   }

   int compareTo(int64_t value) const
   {
      return value < 0 ? 1 : compareTo(uint64_t(value));
   }

   ScaledNumber &invert()
   {
      return *this = ScaledNumber::get(1) / *this;
   }

   ScaledNumber inverse() const
   {
      return ScaledNumber(*this).invert();
   }

private:
   static ScaledNumber getProduct(DigitsType lhs, DigitsType rhs)
   {
      return scalednumbers::getProduct(lhs, rhs);
   }

   static ScaledNumber getQuotient(DigitsType dividend, DigitsType divisor)
   {
      return scalednumbers::getQuotient(dividend, divisor);
   }

   static int countLeadingZerosWidth(DigitsType digits)
   {
      if (m_width == 64) {
         return countLeadingZeros64(digits);
      }

      if (m_width == 32) {
         return countLeadingZeros32(digits);
      }
      return countLeadingZeros32(digits) + m_width - 32;
   }

   /// \brief Adjust a number to m_width, rounding up if necessary.
   ///
   /// Should only be called for \c shift close to zero.
   ///
   /// \pre shift >= MIN_SCALE && shift + 64 <= MAX_SCALE.
   static ScaledNumber adjustToWidth(uint64_t value, int32_t shift)
   {
      assert(shift >= scalednumbers::MIN_SCALE && "shift should be close to 0");
      assert(shift <= scalednumbers::MAX_SCALE - 64 &&
             "shift should be close to 0");
      auto adjusted = scalednumbers::get_adjusted<DigitsT>(value, shift);
      return adjusted;
   }

   static ScaledNumber get_rounded(ScaledNumber snum, bool round)
   {
      // Saturate.
      if (snum.isLargest()) {
         return snum;
      }
      return scalednumbers::get_rounded(snum.m_digits, snum.m_scale, round);
   }
};

#define SCALED_NUMBER_BOP(op, base)                                            \
   template <typename DigitsT>                                                     \
   ScaledNumber<DigitsT> operator op(const ScaledNumber<DigitsT> &lhs,            \
   const ScaledNumber<DigitsT> &rhs) {          \
   return ScaledNumber<DigitsT>(lhs) base rhs;                                    \
}

SCALED_NUMBER_BOP(+, +=)
SCALED_NUMBER_BOP(-, -=)
SCALED_NUMBER_BOP(*, *=)
SCALED_NUMBER_BOP(/, /=)

#undef SCALED_NUMBER_BOP

template <typename DigitsT>
ScaledNumber<DigitsT> operator<<(const ScaledNumber<DigitsT> &lhs,
                                 int16_t shift)
{
   return ScaledNumber<DigitsT>(lhs) <<= shift;
}

template <typename DigitsT>
ScaledNumber<DigitsT> operator>>(const ScaledNumber<DigitsT> &lhs,
                                 int16_t shift)
{
   return ScaledNumber<DigitsT>(lhs) >>= shift;
}

template <typename DigitsT>
RawOutStream &operator<<(RawOutStream &outstream, const ScaledNumber<DigitsT> &value)
{
   return value.print(outstream, 10);
}

#define SCALED_NUMBER_COMPARE_TO_TYPE(op, T1, T2)                              \
   template <typename DigitsT>                                                     \
   bool operator op(const ScaledNumber<DigitsT> &lhs, T1 rhs) {                     \
   return lhs.compareTo(T2(rhs)) op 0;                                            \
}                                                                            \
   template <typename DigitsT>                                                     \
   bool operator op(T1 lhs, const ScaledNumber<DigitsT> &rhs) {                     \
   return 0 op rhs.compareTo(T2(lhs));                                            \
}
#define SCALED_NUMBER_COMPARE_TO(op)                                           \
   SCALED_NUMBER_COMPARE_TO_TYPE(op, uint64_t, uint64_t)                        \
   SCALED_NUMBER_COMPARE_TO_TYPE(op, uint32_t, uint64_t)                        \
   SCALED_NUMBER_COMPARE_TO_TYPE(op, int64_t, int64_t)                          \
   SCALED_NUMBER_COMPARE_TO_TYPE(op, int32_t, int64_t)
SCALED_NUMBER_COMPARE_TO(< )
SCALED_NUMBER_COMPARE_TO(> )
SCALED_NUMBER_COMPARE_TO(== )
SCALED_NUMBER_COMPARE_TO(!= )
SCALED_NUMBER_COMPARE_TO(<= )
SCALED_NUMBER_COMPARE_TO(>= )
#undef SCALED_NUMBER_COMPARE_TO
#undef SCALED_NUMBER_COMPARE_TO_TYPE

template <typename DigitsT>
uint64_t ScaledNumber<DigitsT>::scale(uint64_t value) const
{
   if (m_width == 64 || value <= DigitsLimits::max()) {
      return (get(value) * *this).template toInt<uint64_t>();
   }

   // Defer to the 64-bit version.
   return ScaledNumber<uint64_t>(m_digits, m_scale).scale(value);
}

template <typename DigitsT>
template <typename IntT>
IntT ScaledNumber<DigitsT>::toInt() const
{
   typedef std::numeric_limits<IntT> Limits;
   if (*this < 1) {
      return 0;
   }
   if (*this >= Limits::max()) {
      return Limits::max();
   }

   IntT value = m_digits;
   if (m_scale > 0) {
      assert(size_t(m_scale) < sizeof(IntT) * 8);
      return value << m_scale;
   }
   if (m_scale < 0) {
      assert(size_t(-m_scale) < sizeof(IntT) * 8);
      return value >> -m_scale;
   }
   return value;
}

template <typename DigitsT>
ScaledNumber<DigitsT> &ScaledNumber<DigitsT>::
operator*=(const ScaledNumber &value)
{
   if (isZero()) {
      return *this;
   }
   if (value.isZero()) {
      return *this = value;
   }
   // Save the exponents.
   int32_t scales = int32_t(m_scale) + int32_t(value.m_scale);
   // Get the raw product.
   *this = getProduct(m_digits, value.m_digits);
   // Combine with exponents.
   return *this <<= scales;
}

template <typename DigitsT>
ScaledNumber<DigitsT> &ScaledNumber<DigitsT>::
operator/=(const ScaledNumber &value)
{
   if (isZero()) {
      return *this;
   }
   if (value.isZero()) {
      return *this = getLargest();
   }
   // Save the exponents.
   int32_t scales = int32_t(m_scale) - int32_t(value.m_scale);

   // Get the raw quotient.
   *this = getQuotient(m_digits, value.m_digits);
   // Combine with exponents.
   return *this <<= scales;
}

template <typename DigitsT> void ScaledNumber<DigitsT>::shiftLeft(int32_t shift)
{
   if (!shift || isZero()) {
      return;
   }

   assert(shift != INT32_MIN);
   if (shift < 0) {
      shiftRight(-shift);
      return;
   }

   // shift as much as we can in the exponent.
   int32_t scaleShift = std::min(shift, scalednumbers::MAX_SCALE - m_scale);
   m_scale += scaleShift;
   if (scaleShift == shift) {
      return;
   }
   // Check this late, since it's rare.
   if (isLargest()) {
      return;
   }
   // shift the m_digits themselves.
   shift -= scaleShift;
   if (shift > countLeadingZerosWidth(m_digits)) {
      // Saturate.
      *this = getLargest();
      return;
   }
   m_digits <<= shift;
}

template <typename DigitsT>
void ScaledNumber<DigitsT>::shiftRight(int32_t shift)
{
   if (!shift || isZero()) {
      return;
   }

   assert(shift != INT32_MIN);
   if (shift < 0) {
      shiftLeft(-shift);
      return;
   }

   // shift as much as we can in the exponent.
   int32_t scaleShift = std::min(shift, m_scale - scalednumbers::MIN_SCALE);
   m_scale -= scaleShift;
   if (scaleShift == shift) {
      return;
   }
   // shift the m_digits themselves.
   shift -= scaleShift;
   if (shift >= m_width) {
      // Saturate.
      *this = getZero();
      return;
   }

   m_digits >>= shift;
}

template <typename T>
struct IsPodLike;
template <typename T>
struct IsPodLike<ScaledNumber<T>>
{
   static const bool value = true;
};

} // utils
} // polar

#endif // POLARPHP_UTILS_SCALED_NUMBER_H
