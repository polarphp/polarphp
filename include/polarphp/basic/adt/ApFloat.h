// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/14.

#ifndef POLARPHP_BASIC_ADT_AP_FLOAT_H
#define POLARPHP_BASIC_ADT_AP_FLOAT_H

#include "polarphp/basic/adt/ApInt.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/utils/ErrorHandling.h"
#include <memory>

namespace polar {

// forward declare class with namespace
namespace utils {
class RawOutStream;
} // utils

namespace basic {

#define APFLOAT_DISPATCH_ON_SEMANTICS(METHOD_CALL)                             \
   do {                                                                         \
   if (usesLayout<IEEEFloat>(getSemantics()))                                 \
   return m_storage.m_ieee.METHOD_CALL;                                               \
   if (usesLayout<DoubleApFloat>(getSemantics()))                             \
   return m_storage.m_dvalue.METHOD_CALL;                                             \
   polar_unreachable("Unexpected semantics");                                 \
} while (false)

struct FltSemantics;
class ApSInt;
class StringRef;
class ApFloat;

using polar::utils::RawOutStream;

template <typename T>
class SmallVectorImpl;

/// Enum that represents what fraction of the LSB truncated bits of an fp number
/// represent.
///
/// This essentially combines the roles of guard and sticky bits.
enum class LostFraction
{ // Example of truncated bits:
   lfExactlyZero,    // 000000
   lfLessThanHalf,   // 0xxxxx  x's not all zero
   lfExactlyHalf,    // 100000
   lfMoreThanHalf    // 1xxxxx  x's not all zero
};

/// A self-contained host- and target-independent arbitrary-precision
/// floating-point software implementation.
///
/// ApFloat uses bignum integer arithmetic as provided by static functions in
/// the ApInt class.  The library will work with bignum integers whose parts are
/// any unsigned type at least 16 bits wide, but 64 bits is recommended.
///
/// Written for clarity rather than speed, in particular with a view to use in
/// the front-end of a cross compiler so that target arithmetic can be correctly
/// performed on the host.  Performance should nonetheless be reasonable,
/// particularly for its intended use.  It may be useful as a base
/// implementation for a run-time library during development of a faster
/// target-specific one.
///
/// All 5 rounding modes in the IEEE-754R draft are handled correctly for all
/// implemented operations.  Currently implemented operations are add, subtract,
/// multiply, divide, fused-multiply-add, conversion-to-float,
/// conversion-to-integer and conversion-from-integer.  New rounding modes
/// (e.g. away from zero) can be added with three or four lines of code.
///
/// Four formats are built-in: IEEE single precision, double precision,
/// quadruple precision, and x87 80-bit extended double (when operating with
/// full extended precision).  Adding a new format that obeys IEEE semantics
/// only requires adding two lines of code: a declaration and definition of the
/// format.
///
/// All operations return the status of that operation as an exception bit-mask,
/// so multiple operations can be done consecutively with their results or-ed
/// together.  The returned status can be useful for compiler diagnostics; e.g.,
/// inexact, underflow and overflow can be easily diagnosed on constant folding,
/// and compiler optimizers can determine what exceptions would be raised by
/// folding operations and optimize, or perhaps not optimize, accordingly.
///
/// At present, underflow tininess is detected after rounding; it should be
/// straight forward to add support for the before-rounding case too.
///
/// The library reads hexadecimal floating point numbers as per C99, and
/// correctly rounds if necessary according to the specified rounding mode.
/// Syntax is required to have been validated by the caller.  It also converts
/// floating point numbers to hexadecimal text as per the C99 %a and %A
/// conversions.  The output precision (or alternatively the natural minimal
/// precision) can be specified; if the requested precision is less than the
/// natural precision the output is correctly rounded for the specified rounding
/// mode.
///
/// It also reads decimal floating point numbers and correctly rounds according
/// to the specified rounding mode.
///
/// Conversion to decimal text is not currently implemented.
///
/// Non-zero finite numbers are represented internally as a sign bit, a 16-bit
/// signed exponent, and the significand as an array of integer parts.  After
/// normalization of a number of precision P the exponent is within the range of
/// the format, and if the number is not denormal the P-th bit of the
/// significand is set as an explicit integer bit.  For denormals the most
/// significant bit is shifted right so that the exponent is maintained at the
/// format's minimum, so that the smallest denormal has just the least
/// significant bit of the significand set.  The sign of zeroes and infinities
/// is significant; the exponent and significand of such numbers is not stored,
/// but has a known implicit (deterministic) value: 0 for the significands, 0
/// for zero exponent, all 1 bits for infinity exponent.  For NaNs the sign and
/// significand are deterministic, although not really meaningful, and preserved
/// in non-conversion operations.  The exponent is implicitly all 1 bits.
///
/// ApFloat does not provide any exception handling beyond default exception
/// handling. We represent Signaling NaNs via IEEE-754R 2008 6.2.1 should clause
/// by encoding Signaling NaNs with the first bit of its trailing significand as
/// 0.
///
/// TODO
/// ====
///
/// Some features that may or may not be worth adding:
///
/// Binary to decimal conversion (hard).
///
/// Optional ability to detect underflow tininess before rounding.
///
/// New formats: x87 in single and double precision mode (IEEE apart from
/// extended exponent range) (hard).
///
/// New operations: sqrt, IEEE remainder, C90 fmod, nexttoward.
///

// This is the common type definitions shared by ApFloat and its internal
// implementation classes. This struct should not define any non-static data
// members.
struct ApFloatBase
{
   typedef ApInt::WordType integerPart;
   static const unsigned sm_integerPartWidth = ApInt::APINT_BITS_PER_WORD;

   /// A signed type to represent a floating point numbers unbiased exponent.
   typedef signed short ExponentType;

   /// \name Floating Point Semantics.
   /// @{

   static const FltSemantics &getIEEEhalf() POLAR_READNONE;
   static const FltSemantics &getIEEEsingle() POLAR_READNONE;
   static const FltSemantics &getIEEEdouble() POLAR_READNONE;
   static const FltSemantics &getIEEEquad() POLAR_READNONE;
   static const FltSemantics &getPPCDoubleDouble() POLAR_READNONE;
   static const FltSemantics &getX87DoubleExtended() POLAR_READNONE;

   /// A Pseudo fltsemantic used to construct ApFloats that cannot conflict with
   /// anything real.
   static const FltSemantics &getBogus() POLAR_READNONE;

   /// @}

   /// IEEE-754R 5.11: Floating Point Comparison Relations.
   enum class CmpResult
   {
      cmpLessThan,
      cmpEqual,
      cmpGreaterThan,
      cmpUnordered
   };

   /// IEEE-754R 4.3: Rounding-direction attributes.
   enum class RoundingMode
   {
      rmNearestTiesToEven,
      rmTowardPositive,
      rmTowardNegative,
      rmTowardZero,
      rmNearestTiesToAway
   };

   /// IEEE-754R 7: Default exception handling.
   ///
   /// opUnderflow or opOverflow are always returned or-ed with opInexact.
   enum OpStatus
   {
      opOK = 0x00,
      opInvalidOp = 0x01,
      opDivByZero = 0x02,
      opOverflow = 0x04,
      opUnderflow = 0x08,
      opInexact = 0x10
   };

   /// Category of internally-represented number.
   enum FltCategory
   {
      fcInfinity,
      fcNaN,
      fcNormal,
      fcZero
   };

   /// Convenience enum used to construct an uninitialized ApFloat.
   enum UninitializedTag
   {
      uninitialized
   };

   /// Enumeration of \c ilogb error results.
   enum IlogbErrorKinds
   {
      IEK_Zero = INT_MIN + 1,
      IEK_NaN = INT_MIN,
      IEK_Inf = INT_MAX
   };

   static unsigned int semanticsPrecision(const FltSemantics &);
   static ExponentType semanticsMinExponent(const FltSemantics &);
   static ExponentType semanticsMaxExponent(const FltSemantics &);
   static unsigned int semanticsSizeInBits(const FltSemantics &);

   /// Returns the size of the floating point number (in bits) in the given
   /// semantics.
   static unsigned getSizeInBits(const FltSemantics &sem);
};

namespace internal {

class IEEEFloat final : public ApFloatBase
{
public:
   /// \name Constructors
   /// @{

   IEEEFloat(const FltSemantics &); // Default construct to 0.0
   IEEEFloat(const FltSemantics &, integerPart);
   IEEEFloat(const FltSemantics &, UninitializedTag);
   IEEEFloat(const FltSemantics &, const ApInt &);
   explicit IEEEFloat(double d);
   explicit IEEEFloat(float f);
   IEEEFloat(const IEEEFloat &);
   IEEEFloat(IEEEFloat &&);
   ~IEEEFloat();

   /// @}

   /// Returns whether this instance allocated memory.
   bool needsCleanup() const
   {
      return getPartCount() > 1;
   }

   /// \name Convenience "constructors"
   /// @{

   /// @}

   /// \name Arithmetic
   /// @{

   OpStatus add(const IEEEFloat &, RoundingMode);
   OpStatus subtract(const IEEEFloat &, RoundingMode);
   OpStatus multiply(const IEEEFloat &, RoundingMode);
   OpStatus divide(const IEEEFloat &, RoundingMode);
   /// IEEE remainder.
   OpStatus remainder(const IEEEFloat &);
   /// C fmod, or llvm frem.
   OpStatus mod(const IEEEFloat &);
   OpStatus fusedMultiplyAdd(const IEEEFloat &, const IEEEFloat &, RoundingMode);
   OpStatus roundToIntegral(RoundingMode);
   /// IEEE-754R 5.3.1: nextUp/nextDown.
   OpStatus next(bool nextDown);

   /// @}

   /// \name Sign operations.
   /// @{

   void changeSign();

   /// @}

   /// \name Conversions
   /// @{

   OpStatus convert(const FltSemantics &, RoundingMode, bool *);
   OpStatus convertToInteger(MutableArrayRef<integerPart>, unsigned int, bool,
                             RoundingMode, bool *) const;
   OpStatus convertFromApInt(const ApInt &, bool, RoundingMode);
   OpStatus convertFromSignExtendedInteger(const integerPart *, unsigned int,
                                           bool, RoundingMode);
   OpStatus convertFromZeroExtendedInteger(const integerPart *, unsigned int,
                                           bool, RoundingMode);
   OpStatus convertFromString(StringRef, RoundingMode);
   ApInt bitcastToApInt() const;
   double convertToDouble() const;
   float convertToFloat() const;

   /// @}

   /// The definition of equality is not straightforward for floating point, so
   /// we won't use operator==.  Use one of the following, or write whatever it
   /// is you really mean.
   bool operator==(const IEEEFloat &) const = delete;

   /// IEEE comparison with another floating point number (NaNs compare
   /// unordered, 0==-0).
   CmpResult compare(const IEEEFloat &) const;

   /// Bitwise comparison for equality (QNaNs compare equal, 0!=-0).
   bool bitwiseIsEqual(const IEEEFloat &) const;

   /// Write out a hexadecimal representation of the floating point value to DST,
   /// which must be of sufficient size, in the C99 form [-]0xh.hhhhp[+-]d.
   /// Return the number of characters written, excluding the terminating NUL.
   unsigned int convertToHexString(char *dst, unsigned int hexDigits,
                                   bool upperCase, RoundingMode) const;

   /// \name IEEE-754R 5.7.2 General operations.
   /// @{

   /// IEEE-754R isSignMinus: Returns true if and only if the current value is
   /// negative.
   ///
   /// This applies to zeros and NaNs as well.
   bool isNegative() const
   {
      return m_sign;
   }

   /// IEEE-754R isNormal: Returns true if and only if the current value is normal.
   ///
   /// This implies that the current value of the float is not zero, subnormal,
   /// infinite, or NaN following the definition of normality from IEEE-754R.
   bool isNormal() const
   {
      return !isDenormal() && isFiniteNonZero();
   }

   /// Returns true if and only if the current value is zero, subnormal, or
   /// normal.
   ///
   /// This means that the value is not infinite or NaN.
   bool isFinite() const
   {
      return !isNaN() && !isInfinity();
   }

   /// Returns true if and only if the float is plus or minus zero.
   bool isZero() const
   {
      return m_category == FltCategory::fcZero;
   }

   /// IEEE-754R isSubnormal(): Returns true if and only if the float is a
   /// denormal.
   bool isDenormal() const;

   /// IEEE-754R isInfinite(): Returns true if and only if the float is infinity.
   bool isInfinity() const
   {
      return m_category == FltCategory::fcInfinity;
   }

   /// Returns true if and only if the float is a quiet or signaling NaN.
   bool isNaN() const
   {
      return m_category == FltCategory::fcNaN;
   }

   /// Returns true if and only if the float is a signaling NaN.
   bool isSignaling() const;

   /// @}

   /// \name Simple Queries
   /// @{

   FltCategory getCategory() const
   {
      return m_category;
   }

   const FltSemantics &getSemantics() const
   {
      return *m_semantics;
   }

   bool isNonZero() const
   {
      return m_category != FltCategory::fcZero;
   }

   bool isFiniteNonZero() const
   {
      return isFinite() && !isZero();
   }

   bool isPosZero() const
   {
      return isZero() && !isNegative();
   }

   bool isNegZero() const
   {
      return isZero() && isNegative();
   }

   /// Returns true if and only if the number has the smallest possible non-zero
   /// magnitude in the current semantics.
   bool isSmallest() const;

   /// Returns true if and only if the number has the largest possible finite
   /// magnitude in the current semantics.
   bool isLargest() const;

   /// Returns true if and only if the number is an exact integer.
   bool isInteger() const;

   /// @}

   IEEEFloat &operator=(const IEEEFloat &);
   IEEEFloat &operator=(IEEEFloat &&);

   /// Overload to compute a hash code for an ApFloat value.
   ///
   /// Note that the use of hash codes for floating point values is in general
   /// frought with peril. Equality is hard to define for these values. For
   /// example, should negative and positive zero hash to different codes? Are
   /// they equal or not? This hash value implementation specifically
   /// emphasizes producing different codes for different inputs in order to
   /// be used in canonicalization and memoization. As such, equality is
   /// bitwiseIsEqual, and 0 != -0.
   friend HashCode hash_value(const IEEEFloat &arg);

   /// Converts this value into a decimal string.
   ///
   /// \param FormatPrecision The maximum number of digits of
   ///   precision to output.  If there are fewer digits available,
   ///   zero padding will not be used unless the value is
   ///   integral and small enough to be expressed in
   ///   FormatPrecision digits.  0 means to use the natural
   ///   precision of the number.
   /// \param FormatMaxPadding The maximum number of zeros to
   ///   consider inserting before falling back to scientific
   ///   notation.  0 means to always use scientific notation.
   ///
   /// \param TruncateZero Indicate whether to remove the trailing zero in
   ///   fraction part or not. Also setting this parameter to false forcing
   ///   producing of output more similar to default printf behavior.
   ///   Specifically the lower e is used as exponent delimiter and exponent
   ///   always contains no less than two digits.
   ///
   /// Number       Precision    MaxPadding      Result
   /// ------       ---------    ----------      ------
   /// 1.01E+4              5             2       10100
   /// 1.01E+4              4             2       1.01E+4
   /// 1.01E+4              5             1       1.01E+4
   /// 1.01E-2              5             2       0.0101
   /// 1.01E-2              4             2       0.0101
   /// 1.01E-2              4             1       1.01E-2
   void toString(SmallVectorImpl<char> &str, unsigned formatPrecision = 0,
                 unsigned formatMaxPadding = 3, bool truncateZero = true) const;

   /// If this value has an exact multiplicative inverse, store it in inv and
   /// return true.
   bool getExactInverse(ApFloat *inv) const;

   /// Returns the exponent of the internal representation of the ApFloat.
   ///
   /// Because the radix of ApFloat is 2, this is equivalent to floor(log2(x)).
   /// For special ApFloat values, this returns special error codes:
   ///
   ///   NaN -> \c IEK_NaN
   ///   0   -> \c IEK_Zero
   ///   Inf -> \c IEK_Inf
   ///
   friend int ilogb(const IEEEFloat &arg);

   /// Returns: X * 2^Exp for integral exponents.
   friend IEEEFloat scalbn(IEEEFloat X, int exp, RoundingMode);

   friend IEEEFloat frexp(const IEEEFloat &X, int &exp, RoundingMode);

   /// \name Special value setters.
   /// @{

   void makeLargest(bool neg = false);
   void makeSmallest(bool neg = false);
   void makeNaN(bool snan = false, bool neg = false,
                const ApInt *fill = nullptr);
   void makeInf(bool neg = false);
   void makeZero(bool neg = false);
   void makeQuiet();

   /// Returns the smallest (by magnitude) normalized finite number in the given
   /// semantics.
   ///
   /// \param Negative - True iff the number should be negative
   void makeSmallestNormalized(bool negative = false);

   /// @}

   CmpResult compareAbsoluteValue(const IEEEFloat &) const;

private:
   /// \name Simple Queries
   /// @{

   integerPart *getSignificandParts();
   const integerPart *getSignificandParts() const;
   unsigned int getPartCount() const;

   /// @}

   /// \name Significand operations.
   /// @{

   integerPart addSignificand(const IEEEFloat &);
   integerPart subtractSignificand(const IEEEFloat &, integerPart);
   LostFraction addOrSubtractSignificand(const IEEEFloat &, bool subtract);
   LostFraction multiplySignificand(const IEEEFloat &, const IEEEFloat *);
   LostFraction divideSignificand(const IEEEFloat &);
   void incrementSignificand();
   void initialize(const FltSemantics *);
   void shiftSignificandLeft(unsigned int);
   LostFraction shiftSignificandRight(unsigned int);
   unsigned int getSignificandLsb() const;
   unsigned int getSignificandMsb() const;
   void zeroSignificand();
   /// Return true if the significand excluding the integral bit is all ones.
   bool isSignificandAllOnes() const;
   /// Return true if the significand excluding the integral bit is all zeros.
   bool isSignificandAllZeros() const;

   /// @}

   /// \name Arithmetic on special values.
   /// @{

   OpStatus addOrSubtractSpecials(const IEEEFloat &, bool subtract);
   OpStatus divideSpecials(const IEEEFloat &);
   OpStatus multiplySpecials(const IEEEFloat &);
   OpStatus modSpecials(const IEEEFloat &);

   /// @}

   /// \name Miscellany
   /// @{

   bool convertFromStringSpecials(StringRef str);
   OpStatus normalize(RoundingMode, LostFraction);
   OpStatus addOrSubtract(const IEEEFloat &, RoundingMode, bool subtract);
   OpStatus handleOverflow(RoundingMode);
   bool roundAwayFromZero(RoundingMode, LostFraction, unsigned int) const;
   OpStatus convertToSignExtendedInteger(MutableArrayRef<integerPart>,
                                         unsigned int, bool, RoundingMode,
                                         bool *) const;
   OpStatus convertFromUnsignedParts(const integerPart *, unsigned int,
                                     RoundingMode);
   OpStatus convertFromHexadecimalString(StringRef, RoundingMode);
   OpStatus convertFromDecimalString(StringRef, RoundingMode);
   char *convertNormalToHexString(char *, unsigned int, bool,
                                  RoundingMode) const;
   OpStatus roundSignificandWithExponent(const integerPart *, unsigned int, int,
                                         RoundingMode);

   /// @}

   ApInt convertHalfApFloatToApInt() const;
   ApInt convertFloatApFloatToApInt() const;
   ApInt convertDoubleApFloatToApInt() const;
   ApInt convertQuadrupleApFloatToApInt() const;
   ApInt convertF80LongDoubleApFloatToApInt() const;
   ApInt convertPPCDoubleDoubleApFloatToApInt() const;
   void initFromApInt(const FltSemantics *sem, const ApInt &api);
   void initFromHalfApInt(const ApInt &api);
   void initFromFloatApInt(const ApInt &api);
   void initFromDoubleApInt(const ApInt &api);
   void initFromQuadrupleApInt(const ApInt &api);
   void initFromF80LongDoubleApInt(const ApInt &api);
   void initFromPPCDoubleDoubleApInt(const ApInt &api);

   void assign(const IEEEFloat &);
   void copySignificand(const IEEEFloat &);
   void freeSignificand();

   /// Note: this must be the first data member.
   /// The semantics that this value obeys.
   const FltSemantics *m_semantics;

   /// A binary fraction with an explicit integer bit.
   ///
   /// The significand must be at least one bit wider than the target precision.
   union Significand {
      integerPart m_part;
      integerPart *m_parts;
   } m_significand;

   /// The signed unbiased exponent of the value.
   ExponentType m_exponent;

   /// What kind of floating point number this is.
   ///
   /// Only 2 bits are required, but VisualStudio incorrectly sign extends it.
   /// Using the extra bit keeps it from failing under VisualStudio.
   FltCategory m_category : 3;

   /// Sign bit of the number.
   unsigned int m_sign : 1;
};

HashCode hash_value(const IEEEFloat &arg);
int ilogb(const IEEEFloat &arg);
IEEEFloat scalbn(IEEEFloat x, int exp, IEEEFloat::RoundingMode roundingMode);
IEEEFloat frexp(const IEEEFloat &value, int &exp, IEEEFloat::RoundingMode rmode);

// This mode implements more precise float in terms of two ApFloats.
// The interface and layout is designed for arbitray underlying semantics,
// though currently only getPPCDoubleDouble semantics are supported, whose
// corresponding underlying semantics are getIEEEdouble.
class DoubleApFloat final : public ApFloatBase
{
   // Note: this must be the first data member.
   const FltSemantics *m_semantics;
   std::unique_ptr<ApFloat[]> m_floats;

   OpStatus addImpl(const ApFloat &a, const ApFloat &aa, const ApFloat &c,
                    const ApFloat &cc, RoundingMode rmode);

   OpStatus addWithSpecial(const DoubleApFloat &lhs, const DoubleApFloat &rhs,
                           DoubleApFloat &out, RoundingMode rmode);

public:
   DoubleApFloat(const FltSemantics &semantic);
   DoubleApFloat(const FltSemantics &semantic, UninitializedTag);
   DoubleApFloat(const FltSemantics &semantic, integerPart);
   DoubleApFloat(const FltSemantics &semantic, const ApInt &ivalue);
   DoubleApFloat(const FltSemantics &semantic, ApFloat &&first, ApFloat &&second);
   DoubleApFloat(const DoubleApFloat &other);
   DoubleApFloat(DoubleApFloat &&other);

   DoubleApFloat &operator=(const DoubleApFloat &other);

   DoubleApFloat &operator=(DoubleApFloat &&other)
   {
      if (this != &other) {
         this->~DoubleApFloat();
         new (this) DoubleApFloat(std::move(other));
      }
      return *this;
   }

   bool needsCleanup() const
   {
      return m_floats != nullptr;
   }

   ApFloat &getFirst()
   {
      return m_floats[0];
   }

   const ApFloat &getFirst() const
   {
      return m_floats[0];
   }

   ApFloat &getSecond()
   {
      return m_floats[1];
   }

   const ApFloat &getSecond() const
   {
      return m_floats[1];
   }

   OpStatus add(const DoubleApFloat &other, RoundingMode rmode);
   OpStatus subtract(const DoubleApFloat &other, RoundingMode rmode);
   OpStatus multiply(const DoubleApFloat &other, RoundingMode rmode);
   OpStatus divide(const DoubleApFloat &other, RoundingMode rmode);
   OpStatus remainder(const DoubleApFloat &other);
   OpStatus mod(const DoubleApFloat &other);
   OpStatus fusedMultiplyAdd(const DoubleApFloat &multiplicand,
                             const DoubleApFloat &addend, RoundingMode rmode);
   OpStatus roundToIntegral(RoundingMode rmode);
   void changeSign();
   CmpResult compareAbsoluteValue(const DoubleApFloat &other) const;

   FltCategory getCategory() const;
   bool isNegative() const;

   void makeInf(bool neg);
   void makeZero(bool neg);
   void makeLargest(bool neg);
   void makeSmallest(bool neg);
   void makeSmallestNormalized(bool neg);
   void makeNaN(bool snan, bool neg, const ApInt *fill);

   CmpResult compare(const DoubleApFloat &other) const;
   bool bitwiseIsEqual(const DoubleApFloat &other) const;
   ApInt bitcastToApInt() const;
   OpStatus convertFromString(StringRef, RoundingMode);
   OpStatus next(bool nextDown);

   OpStatus convertToInteger(MutableArrayRef<integerPart> input,
                             unsigned int width, bool isSigned, RoundingMode rmode,
                             bool *isExact) const;
   OpStatus convertFromApInt(const ApInt &input, bool isSigned, RoundingMode rmode);
   OpStatus convertFromSignExtendedInteger(const integerPart *input,
                                           unsigned int inputSize, bool isSigned,
                                           RoundingMode rmode);
   OpStatus convertFromZeroExtendedInteger(const integerPart *Input,
                                           unsigned int inputSize, bool isSigned,
                                           RoundingMode rmode);
   unsigned int convertToHexString(char *dest, unsigned int hexDigits,
                                   bool upperCase, RoundingMode rmode) const;

   bool isDenormal() const;
   bool isSmallest() const;
   bool isLargest() const;
   bool isInteger() const;

   void toString(SmallVectorImpl<char> &str, unsigned formatPrecision,
                 unsigned formatMaxPadding, bool truncateZero = true) const;

   bool getExactInverse(ApFloat *inv) const;

   friend int ilogb(const DoubleApFloat &arg);
   friend DoubleApFloat scalbn(DoubleApFloat value, int exp, RoundingMode);
   friend DoubleApFloat frexp(const DoubleApFloat &value, int &exp, RoundingMode);
   friend HashCode hash_value(const DoubleApFloat &arg);
};

HashCode hash_value(const DoubleApFloat &arg);

} // End internal namespace

// This is a interface class that is currently forwarding functionalities from
// internal::IEEEFloat.
class ApFloat : public ApFloatBase
{
   typedef internal::IEEEFloat IEEEFloat;
   typedef internal::DoubleApFloat DoubleApFloat;

   static_assert(std::is_standard_layout<IEEEFloat>::value, "");

   union Storage
   {
      const FltSemantics *m_semantics;
      IEEEFloat m_ieee;
      DoubleApFloat m_dvalue;

      explicit Storage(IEEEFloat fvalue, const FltSemantics &semantic);
      explicit Storage(DoubleApFloat fvalue, const FltSemantics &semantic)
         : m_dvalue(std::move(fvalue)) {
         assert(&semantic == &getPPCDoubleDouble());
      }

      template <typename... ArgTypes>
      Storage(const FltSemantics &semantics, ArgTypes &&... args)
      {
         if (usesLayout<IEEEFloat>(semantics)) {
            new (&m_ieee) IEEEFloat(semantics, std::forward<ArgTypes>(args)...);
            return;
         }
         if (usesLayout<DoubleApFloat>(semantics)) {
            new (&m_dvalue) DoubleApFloat(semantics, std::forward<ArgTypes>(args)...);
            return;
         }
         polar_unreachable("Unexpected semantics");
      }

      ~Storage()
      {
         if (usesLayout<IEEEFloat>(*m_semantics))
         {
            m_ieee.~IEEEFloat();
            return;
         }
         if (usesLayout<DoubleApFloat>(*m_semantics)) {
            m_dvalue.~DoubleApFloat();
            return;
         }
         polar_unreachable("Unexpected semantics");
      }

      Storage(const Storage &other)
      {
         if (usesLayout<IEEEFloat>(*other.m_semantics)) {
            new (this) IEEEFloat(other.m_ieee);
            return;
         }
         if (usesLayout<DoubleApFloat>(*other.m_semantics)) {
            new (this) DoubleApFloat(other.m_dvalue);
            return;
         }
         polar_unreachable("Unexpected semantics");
      }

      Storage(Storage &&other)
      {
         if (usesLayout<IEEEFloat>(*other.m_semantics)) {
            new (this) IEEEFloat(std::move(other.m_ieee));
            return;
         }
         if (usesLayout<DoubleApFloat>(*other.m_semantics)) {
            new (this) DoubleApFloat(std::move(other.m_dvalue));
            return;
         }
         polar_unreachable("Unexpected semantics");
      }

      Storage &operator=(const Storage &other)
      {
         if (usesLayout<IEEEFloat>(*m_semantics) &&
             usesLayout<IEEEFloat>(*other.m_semantics)) {
            m_ieee = other.m_ieee;
         } else if (usesLayout<DoubleApFloat>(*m_semantics) &&
                    usesLayout<DoubleApFloat>(*other.m_semantics)) {
            m_dvalue = other.m_dvalue;
         } else if (this != &other) {
            this->~Storage();
            new (this) Storage(other);
         }
         return *this;
      }

      Storage &operator=(Storage &&other)
      {
         if (usesLayout<IEEEFloat>(*m_semantics) &&
             usesLayout<IEEEFloat>(*other.m_semantics)) {
            m_ieee = std::move(other.m_ieee);
         } else if (usesLayout<DoubleApFloat>(*m_semantics) &&
                    usesLayout<DoubleApFloat>(*other.m_semantics)) {
            m_dvalue = std::move(other.m_dvalue);
         } else if (this != &other) {
            this->~Storage();
            new (this) Storage(std::move(other));
         }
         return *this;
      }
   } m_storage;

   template <typename T> static bool usesLayout(const FltSemantics &semantics)
   {
      static_assert(std::is_same<T, IEEEFloat>::value ||
                    std::is_same<T, DoubleApFloat>::value, "");
      if (std::is_same<T, DoubleApFloat>::value) {
         return &semantics == &getPPCDoubleDouble();
      }
      return &semantics != &getPPCDoubleDouble();
   }

   IEEEFloat &getIEEE()
   {
      if (usesLayout<IEEEFloat>(*m_storage.m_semantics))
      {
         return m_storage.m_ieee;
      }

      if (usesLayout<DoubleApFloat>(*m_storage.m_semantics))
      {
         return m_storage.m_dvalue.getFirst().m_storage.m_ieee;
      }
      polar_unreachable("Unexpected semantics");
   }

   const IEEEFloat &getIEEE() const
   {
      if (usesLayout<IEEEFloat>(*m_storage.m_semantics)) {
         return m_storage.m_ieee;
      }
      if (usesLayout<DoubleApFloat>(*m_storage.m_semantics)) {
         return m_storage.m_dvalue.getFirst().m_storage.m_ieee;
      }
      polar_unreachable("Unexpected semantics");
   }

   void makeZero(bool neg)
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(makeZero(neg));
   }

   void makeInf(bool neg)
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(makeInf(neg));
   }

   void makeNaN(bool snan, bool neg, const ApInt *fill)
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(makeNaN(snan, neg, fill));
   }

   void makeLargest(bool neg)
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(makeLargest(neg));
   }

   void makeSmallest(bool neg)
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(makeSmallest(neg));
   }

   void makeSmallestNormalized(bool neg)
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(makeSmallestNormalized(neg));
   }

   // FIXME: This is due to clang 3.3 (or older version) always checks for the
   // default constructor in an array aggregate initialization, even if no
   // elements in the array is default initialized.
   ApFloat() : m_storage(getIEEEdouble())
   {
      polar_unreachable("This is a workaround for old clang.");
   }

   explicit ApFloat(IEEEFloat fvalue, const FltSemantics &semantic)
      : m_storage(std::move(fvalue), semantic)
   {}
   explicit ApFloat(DoubleApFloat fvalue, const FltSemantics &semantic)
      : m_storage(std::move(fvalue), semantic)
   {}

   CmpResult compareAbsoluteValue(const ApFloat &other) const
   {
      assert(&getSemantics() == &other.getSemantics() &&
             "Should only compare ApFloats with the same semantics");
      if (usesLayout<IEEEFloat>(getSemantics())) {
         return m_storage.m_ieee.compareAbsoluteValue(other.m_storage.m_ieee);
      }
      if (usesLayout<DoubleApFloat>(getSemantics())) {
         return m_storage.m_dvalue.compareAbsoluteValue(other.m_storage.m_dvalue);
      }
      polar_unreachable("Unexpected semantics");
   }

public:
   ApFloat(const FltSemantics &semantics) : m_storage(semantics)
   {}
   ApFloat(const FltSemantics &semantics, StringRef str);
   ApFloat(const FltSemantics &semantics, integerPart ivalue) : m_storage(semantics, ivalue)
   {}
   // TODO: Remove this constructor. This isn't faster than the first one.
   ApFloat(const FltSemantics &semantics, UninitializedTag)
      : m_storage(semantics, UninitializedTag::uninitialized)
   {}

   ApFloat(const FltSemantics &semantics, const ApInt &ivalue) : m_storage(semantics, ivalue)
   {}
   explicit ApFloat(double dvalue) : m_storage(IEEEFloat(dvalue), getIEEEdouble())
   {}

   explicit ApFloat(float fvalue) : m_storage(IEEEFloat(fvalue), getIEEEsingle())
   {}

   ApFloat(const ApFloat &other) = default;
   ApFloat(ApFloat &&other) = default;

   ~ApFloat() = default;

   bool needsCleanup() const
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(needsCleanup());
   }

   /// Factory for Positive and Negative Zero.
   ///
   /// \param Negative True iff the number should be negative.
   static ApFloat getZero(const FltSemantics &semantic, bool negative = false)
   {
      ApFloat value(semantic, UninitializedTag::uninitialized);
      value.makeZero(negative);
      return value;
   }

   /// Factory for Positive and Negative Infinity.
   ///
   /// \param Negative True iff the number should be negative.
   static ApFloat getInf(const FltSemantics &semantic, bool negative = false)
   {
      ApFloat value(semantic, UninitializedTag::uninitialized);
      value.makeInf(negative);
      return value;
   }

   /// Factory for NaN values.
   ///
   /// \param Negative - True iff the NaN generated should be negative.
   /// \param type - The unspecified fill bits for creating the NaN, 0 by
   /// default.  The value is truncated as necessary.
   static ApFloat getNaN(const FltSemantics &semantic, bool negative = false,
                         unsigned type = 0)
   {
      if (type) {
         ApInt fill(64, type);
         return getQNaN(semantic, negative, &fill);
      } else {
         return getQNaN(semantic, negative, nullptr);
      }
   }

   /// Factory for QNaN values.
   static ApFloat getQNaN(const FltSemantics &semantic, bool negative = false,
                          const ApInt *payload = nullptr)
   {
      ApFloat value(semantic, UninitializedTag::uninitialized);
      value.makeNaN(false, negative, payload);
      return value;
   }

   /// Factory for SNaN values.
   static ApFloat getSNaN(const FltSemantics &semantic, bool negative = false,
                          const ApInt *payload = nullptr)
   {
      ApFloat value(semantic, UninitializedTag::uninitialized);
      value.makeNaN(true, negative, payload);
      return value;
   }

   /// Returns the largest finite number in the given semantics.
   ///
   /// \param Negative - True iff the number should be negative
   static ApFloat getLargest(const FltSemantics &semantic, bool negative = false)
   {
      ApFloat value(semantic, UninitializedTag::uninitialized);
      value.makeLargest(negative);
      return value;
   }

   /// Returns the smallest (by magnitude) finite number in the given semantics.
   /// Might be denormalized, which implies a relative loss of precision.
   ///
   /// \param Negative - True iff the number should be negative
   static ApFloat getSmallest(const FltSemantics &semantic, bool negative = false)
   {
      ApFloat value(semantic, UninitializedTag::uninitialized);
      value.makeSmallest(negative);
      return value;
   }

   /// Returns the smallest (by magnitude) normalized finite number in the given
   /// semantics.
   ///
   /// \param Negative - True iff the number should be negative
   static ApFloat getSmallestNormalized(const FltSemantics &semantic,
                                        bool negative = false)
   {
      ApFloat value(semantic, UninitializedTag::uninitialized);
      value.makeSmallestNormalized(negative);
      return value;
   }

   /// Returns a float which is bitcasted from an all one value int.
   ///
   /// \param BitWidth - Select float type
   /// \param isIEEE   - If 128 bit number, select between PPC and IEEE
   static ApFloat getAllOnesValue(unsigned bitWidth, bool isIEEE = false);

   /// Used to insert ApFloat objects, or objects that contain ApFloat objects,
   /// into FoldingSets.
   void profile(FoldingSetNodeId &nid) const;

   OpStatus add(const ApFloat &other, RoundingMode rmode)
   {
      assert(&getSemantics() == &other.getSemantics() &&
             "Should only call on two ApFloats with the same semantics");
      if (usesLayout<IEEEFloat>(getSemantics())) {
         return m_storage.m_ieee.add(other.m_storage.m_ieee, rmode);
      }

      if (usesLayout<DoubleApFloat>(getSemantics())) {
         return m_storage.m_dvalue.add(other.m_storage.m_dvalue, rmode);
      }
      polar_unreachable("Unexpected semantics");
   }

   OpStatus subtract(const ApFloat &other, RoundingMode rmode)
   {
      assert(&getSemantics() == &other.getSemantics() &&
             "Should only call on two ApFloats with the same semantics");
      if (usesLayout<IEEEFloat>(getSemantics())) {
         return m_storage.m_ieee.subtract(other.m_storage.m_ieee, rmode);
      }

      if (usesLayout<DoubleApFloat>(getSemantics())) {
         return m_storage.m_dvalue.subtract(other.m_storage.m_dvalue, rmode);
      }
      polar_unreachable("Unexpected semantics");
   }

   OpStatus multiply(const ApFloat &other, RoundingMode rmode)
   {
      assert(&getSemantics() == &other.getSemantics() &&
             "Should only call on two ApFloats with the same semantics");
      if (usesLayout<IEEEFloat>(getSemantics())) {
         return m_storage.m_ieee.multiply(other.m_storage.m_ieee, rmode);
      }
      if (usesLayout<DoubleApFloat>(getSemantics())) {
         return m_storage.m_dvalue.multiply(other.m_storage.m_dvalue, rmode);
      }
      polar_unreachable("Unexpected semantics");
   }
   OpStatus divide(const ApFloat &other, RoundingMode rmode)
   {
      assert(&getSemantics() == &other.getSemantics() &&
             "Should only call on two ApFloats with the same semantics");
      if (usesLayout<IEEEFloat>(getSemantics())) {
         return m_storage.m_ieee.divide(other.m_storage.m_ieee, rmode);
      }
      if (usesLayout<DoubleApFloat>(getSemantics())) {
         return m_storage.m_dvalue.divide(other.m_storage.m_dvalue, rmode);
      }
      polar_unreachable("Unexpected semantics");
   }

   OpStatus remainder(const ApFloat &other)
   {
      assert(&getSemantics() == &other.getSemantics() &&
             "Should only call on two ApFloats with the same semantics");
      if (usesLayout<IEEEFloat>(getSemantics())) {
         return m_storage.m_ieee.remainder(other.m_storage.m_ieee);
      }

      if (usesLayout<DoubleApFloat>(getSemantics())) {
         return m_storage.m_dvalue.remainder(other.m_storage.m_dvalue);
      }
      polar_unreachable("Unexpected semantics");
   }

   OpStatus mod(const ApFloat &other)
   {
      assert(&getSemantics() == &other.getSemantics() &&
             "Should only call on two ApFloats with the same semantics");
      if (usesLayout<IEEEFloat>(getSemantics())) {
         return m_storage.m_ieee.mod(other.m_storage.m_ieee);
      }

      if (usesLayout<DoubleApFloat>(getSemantics())) {
         return m_storage.m_dvalue.mod(other.m_storage.m_dvalue);
      }
      polar_unreachable("Unexpected semantics");
   }

   OpStatus fusedMultiplyAdd(const ApFloat &multiplicand, const ApFloat &addend,
                             RoundingMode rmode)
   {
      assert(&getSemantics() == &multiplicand.getSemantics() &&
             "Should only call on ApFloats with the same semantics");
      assert(&getSemantics() == &addend.getSemantics() &&
             "Should only call on ApFloats with the same semantics");
      if (usesLayout<IEEEFloat>(getSemantics())) {
         return m_storage.m_ieee.fusedMultiplyAdd(multiplicand.m_storage.m_ieee, addend.m_storage.m_ieee, rmode);
      }

      if (usesLayout<DoubleApFloat>(getSemantics())) {
         return m_storage.m_dvalue.fusedMultiplyAdd(multiplicand.m_storage.m_dvalue, addend.m_storage.m_dvalue,
                                                    rmode);
      }
      polar_unreachable("Unexpected semantics");
   }

   OpStatus roundToIntegral(RoundingMode rmode)
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(roundToIntegral(rmode));
   }

   // TODO: bool parameters are not readable and a source of bugs.
   // Do something.
   OpStatus next(bool nextDown)
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(next(nextDown));
   }

   /// Add two ApFloats, rounding ties to the nearest even.
   /// No error checking.
   ApFloat operator+(const ApFloat &other) const
   {
      ApFloat result(*this);
      (void)result.add(other, RoundingMode::rmNearestTiesToEven);
      return result;
   }

   /// Subtract two ApFloats, rounding ties to the nearest even.
   /// No error checking.
   ApFloat operator-(const ApFloat &other) const
   {
      ApFloat result(*this);
      (void)result.subtract(other, RoundingMode::rmNearestTiesToEven);
      return result;
   }

   /// Multiply two ApFloats, rounding ties to the nearest even.
   /// No error checking.
   ApFloat operator*(const ApFloat &other) const
   {
      ApFloat result(*this);
      (void)result.multiply(other, RoundingMode::rmNearestTiesToEven);
      return result;
   }

   /// Divide the first ApFloat by the second, rounding ties to the nearest even.
   /// No error checking.
   ApFloat operator/(const ApFloat &other) const
   {
      ApFloat result(*this);
      (void)result.divide(other, RoundingMode::rmNearestTiesToEven);
      return result;
   }

   void changeSign()
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(changeSign());
   }

   void clearSign()
   {
      if (isNegative()) {
         changeSign();
      }
   }

   void copySign(const ApFloat &other)
   {
      if (isNegative() != other.isNegative()) {
         changeSign();
      }
   }

   /// A static helper to produce a copy of an ApFloat value with its sign
   /// copied from some other ApFloat.
   static ApFloat copySign(ApFloat value, const ApFloat &sign)
   {
      value.copySign(sign);
      return value;
   }

   OpStatus convert(const FltSemantics &toSemantics, RoundingMode rmode,
                    bool *losesInfo);
   OpStatus convertToInteger(MutableArrayRef<integerPart> input,
                             unsigned int width, bool isSigned, RoundingMode rmode,
                             bool *isExact) const
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(
               convertToInteger(input, width, isSigned, rmode, isExact));
   }

   OpStatus convertToInteger(ApSInt &Result, RoundingMode rmode,
                             bool *isExact) const;

   OpStatus convertFromApInt(const ApInt &input, bool isSigned,
                             RoundingMode rmode)
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(convertFromApInt(input, isSigned, rmode));
   }

   OpStatus convertFromSignExtendedInteger(const integerPart *input,
                                           unsigned int inputSize, bool isSigned,
                                           RoundingMode rmode)
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(
               convertFromSignExtendedInteger(input, inputSize, isSigned, rmode));
   }

   OpStatus convertFromZeroExtendedInteger(const integerPart *input,
                                           unsigned int inputSize, bool isSigned,
                                           RoundingMode rmode)
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(
               convertFromZeroExtendedInteger(input, inputSize, isSigned, rmode));
   }

   OpStatus convertFromString(StringRef, RoundingMode);
   ApInt bitcastToApInt() const
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(bitcastToApInt());
   }

   double convertToDouble() const
   {
      return getIEEE().convertToDouble();
   }

   float convertToFloat() const
   {
      return getIEEE().convertToFloat();
   }

   bool operator==(const ApFloat &) const = delete;

   CmpResult compare(const ApFloat &other) const
   {
      assert(&getSemantics() == &other.getSemantics() &&
             "Should only compare ApFloats with the same semantics");
      if (usesLayout<IEEEFloat>(getSemantics())) {
         return m_storage.m_ieee.compare(other.m_storage.m_ieee);
      }

      if (usesLayout<DoubleApFloat>(getSemantics())) {
         return m_storage.m_dvalue.compare(other.m_storage.m_dvalue);
      }
      polar_unreachable("Unexpected semantics");
   }

   bool bitwiseIsEqual(const ApFloat &other) const
   {
      if (&getSemantics() != &other.getSemantics())
         return false;
      if (usesLayout<IEEEFloat>(getSemantics())) {
         return m_storage.m_ieee.bitwiseIsEqual(other.m_storage.m_ieee);
      }
      if (usesLayout<DoubleApFloat>(getSemantics())) {
         return m_storage.m_dvalue.bitwiseIsEqual(other.m_storage.m_dvalue);
      }
      polar_unreachable("Unexpected semantics");
   }

   /// We don't rely on operator== working on double values, as
   /// it returns true for things that are clearly not equal, like -0.0 and 0.0.
   /// As such, this method can be used to do an exact bit-for-bit comparison of
   /// two floating point values.
   ///
   /// We leave the version with the double argument here because it's just so
   /// convenient to write "2.0" and the like.  Without this function we'd
   /// have to duplicate its logic everywhere it's called.
   bool isExactlyValue(double dvalue) const
   {
      bool ignored;
      ApFloat temp(dvalue);
      temp.convert(getSemantics(), RoundingMode::rmNearestTiesToEven, &ignored);
      return bitwiseIsEqual(temp);
   }

   unsigned int convertToHexString(char *dest, unsigned int hexDigits,
                                   bool upperCase, RoundingMode rmode) const
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(
               convertToHexString(dest, hexDigits, upperCase, rmode));
   }

   bool isZero() const
   {
      return getCategory() == FltCategory::fcZero;
   }

   bool isInfinity() const
   {
      return getCategory() == FltCategory::fcInfinity;
   }

   bool isNaN() const
   {
      return getCategory() == FltCategory::fcNaN;
   }

   bool isNegative() const
   {
      return getIEEE().isNegative();
   }

   bool isDenormal() const
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(isDenormal());
   }

   bool isSignaling() const
   {
      return getIEEE().isSignaling();
   }

   bool isNormal() const
   {
      return !isDenormal() && isFiniteNonZero();
   }

   bool isFinite() const
   {
      return !isNaN() && !isInfinity();
   }

   FltCategory getCategory() const
   {
      return getIEEE().getCategory();
   }

   const FltSemantics &getSemantics() const
   {
      return *m_storage.m_semantics;
   }

   bool isNonZero() const
   {
      return !isZero();
   }

   bool isFiniteNonZero() const
   {
      return isFinite() && !isZero();
   }

   bool isPosZero() const
   {
      return isZero() && !isNegative();
   }

   bool isNegZero() const
   {
      return isZero() && isNegative();
   }

   bool isSmallest() const
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(isSmallest());
   }

   bool isLargest() const
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(isLargest());
   }

   bool isInteger() const
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(isInteger());
   }

   ApFloat &operator=(const ApFloat &other) = default;
   ApFloat &operator=(ApFloat &&other) = default;

   void toString(SmallVectorImpl<char> &str, unsigned formatPrecision = 0,
                 unsigned formatMaxPadding = 3, bool truncateZero = true) const
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(
               toString(str, formatPrecision, formatMaxPadding, truncateZero));
   }

   void print(RawOutStream &) const;
   void dump() const;

   bool getExactInverse(ApFloat *inv) const
   {
      APFLOAT_DISPATCH_ON_SEMANTICS(getExactInverse(inv));
   }

   friend HashCode hash_value(const ApFloat &arg);
   friend int ilogb(const ApFloat &arg) { return ilogb(arg.getIEEE()); }
   friend ApFloat scalbn(ApFloat fvalue, int exp, RoundingMode roundingMode);
   friend ApFloat frexp(const ApFloat &fvalue, int &exp, RoundingMode roundingMode);
   friend class internal::IEEEFloat;
   friend class internal::DoubleApFloat;
};

/// See friend declarations above.
///
/// These additional declarations are required in order to compile LLVM with IBM
/// xlC compiler.
HashCode hash_value(const ApFloat &arg);
inline ApFloat scalbn(ApFloat x, int exp, ApFloat::RoundingMode rmode)
{
   if (ApFloat::usesLayout<internal::IEEEFloat>(x.getSemantics())) {
      return ApFloat(scalbn(x.m_storage.m_ieee, exp, rmode), x.getSemantics());
   }

   if (ApFloat::usesLayout<internal::DoubleApFloat>(x.getSemantics())) {
      return ApFloat(scalbn(x.m_storage.m_dvalue, exp, rmode), x.getSemantics());
   }
   polar_unreachable("Unexpected semantics");
}

/// Equivalent of C standard library function.
///
/// While the C standard says Exp is an unspecified value for infinity and nan,
/// this returns INT_MAX for infinities, and INT_MIN for NaNs.
inline ApFloat frexp(const ApFloat &fvalue, int &exp, ApFloat::RoundingMode roundingMode)
{
   if (ApFloat::usesLayout<internal::IEEEFloat>(fvalue.getSemantics())) {
      return ApFloat(frexp(fvalue.m_storage.m_ieee, exp, roundingMode), fvalue.getSemantics());
   }

   if (ApFloat::usesLayout<internal::DoubleApFloat>(fvalue.getSemantics())) {
      return ApFloat(frexp(fvalue.m_storage.m_dvalue, exp, roundingMode), fvalue.getSemantics());
   }
   polar_unreachable("Unexpected semantics");
}

/// Returns the absolute value of the argument.
inline ApFloat abs(ApFloat value)
{
   value.clearSign();
   return value;
}

/// \brief Returns the negated value of the argument.
inline ApFloat neg(ApFloat value)
{
   value.changeSign();
   return value;
}

/// Implements IEEE minNum semantics. Returns the smaller of the 2 arguments if
/// both are not NaN. If either argument is a NaN, returns the other argument.
POLAR_READONLY
inline ApFloat minnum(const ApFloat &lhs, const ApFloat &rhs)
{
   if (lhs.isNaN()) {
      return rhs;
   }

   if (rhs.isNaN()) {
      return lhs;
   }

   return (rhs.compare(lhs) == ApFloat::CmpResult::cmpLessThan) ? rhs : lhs;
}

/// Implements IEEE maxNum semantics. Returns the larger of the 2 arguments if
/// both are not NaN. If either argument is a NaN, returns the other argument.
POLAR_READONLY
inline ApFloat maxnum(const ApFloat &lhs, const ApFloat &rhs)
{
   if (lhs.isNaN()) {
      return rhs;
   }
   if (rhs.isNaN()) {
      return lhs;
   }

   return (lhs.compare(rhs) == ApFloat::CmpResult::cmpLessThan) ? rhs : lhs;
}


/// Implements IEEE 754-2018 minimum semantics. Returns the smaller of 2
/// arguments, propagating NaNs and treating -0 as less than +0.
POLAR_READONLY
inline ApFloat minimum(const ApFloat &lhs, const ApFloat &rhs)
{
   if (lhs.isNaN()) {
      return lhs;
   }
   if (rhs.isNaN()) {
      return rhs;
   }

   if (lhs.isZero() && rhs.isZero() && (lhs.isNegative() != rhs.isNegative())) {
      return lhs.isNegative() ? lhs : rhs;
   }

   return (rhs.compare(lhs) == ApFloat::CmpResult::cmpLessThan) ? rhs : lhs;
}

/// Implements IEEE 754-2018 maximum semantics. Returns the larger of 2
/// arguments, propagating NaNs and treating -0 as less than +0.
POLAR_READONLY
inline ApFloat maximum(const ApFloat &lhs, const ApFloat &rhs)
{
   if (lhs.isNaN()) {
      return lhs;
   }
   if (rhs.isNaN()) {
      return rhs;
   }
   if (lhs.isZero() && rhs.isZero() && (lhs.isNegative() != rhs.isNegative())) {
      return lhs.isNegative() ? rhs : lhs;
   }
   return (lhs.compare(rhs) == ApFloat::CmpResult::cmpLessThan) ? rhs : lhs;
}

} // basic
} // polar

#undef APFLOAT_DISPATCH_ON_SEMANTICS

#endif // POLARPHP_BASIC_ADT_AP_FLOAT_H
