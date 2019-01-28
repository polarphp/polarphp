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

#ifndef POLARPHP_BASIC_ADT_AP_INT_H
#define POLARPHP_BASIC_ADT_AP_INT_H

#include "polarphp/utils/MathExtras.h"
#include <cassert>
#include <climits>
#include <cstring>
#include <string>
#include <optional>

namespace polar {

// forward declare class with namespace
namespace utils {
class RawOutStream;
} // utils

namespace basic {

class HashCode;
class StringRef;
class FoldingSetNodeId;

using polar::utils::count_leading_zeros;
using polar::utils::count_leading_ones;
using polar::utils::count_trailing_ones;
using polar::utils::count_trailing_zeros;
using polar::utils::RawOutStream;
using polar::utils::count_population;
using polar::utils::is_power_of_two64;
using polar::utils::is_mask64;
using polar::utils::is_shifted_mask64;
using polar::utils::sign_extend64;
using polar::utils::bits_to_double;
using polar::utils::double_to_bits;
using polar::utils::float_to_bits;

template <typename T> class SmallVectorImpl;
template <typename T> class ArrayRef;

class ApInt;

inline ApInt operator-(ApInt);

//===----------------------------------------------------------------------===//
//                              ApInt Class
//===----------------------------------------------------------------------===//

/// Class for arbitrary precision integers.
///
/// ApInt is a functional replacement for common case unsigned integer type like
/// "unsigned", "unsigned long" or "uint64_t", but also allows non-byte-width
/// integer sizes and large integer value types such as 3-bits, 15-bits, or more
/// than 64-bits of precision. ApInt provides a variety of arithmetic operators
/// and methods to manipulate integer values of any bit-width. It supports both
/// the typical integer arithmetic and comparison operations as well as bitwise
/// manipulation.
///
/// The class has several invariants worth noting:
///   * All bit, byte, and word positions are zero-based.
///   * Once the bit width is set, it doesn't change except by the Truncate,
///     SignExtend, or ZeroExtend operations.
///   * All binary operators must be on ApInt instances of the same bit width.
///     Attempting to use these operators on instances with different bit
///     widths will yield an assertion.
///   * The value is stored canonically as an unsigned value. For operations
///     where it makes a difference, there are both signed and unsigned variants
///     of the operation. For example, sdiv and udiv. However, because the bit
///     widths must be the same, operations such as Mul and Add produce the same
///     results regardless of whether the values are interpreted as signed or
///     not.
///   * In general, the class tries to follow the style of computation other LLVM
///     uses in its IR. This simplifies its use for LLVM.
///
class POLAR_NODISCARD ApInt
{
public:
   typedef uint64_t WordType;

   /// This enum is used to hold the constants we needed for ApInt.
   enum : unsigned {
      /// Byte size of a word.
      APINT_WORD_SIZE = sizeof(WordType),
      /// Bits in a word.
      APINT_BITS_PER_WORD = APINT_WORD_SIZE * CHAR_BIT
   };

   enum class Rounding
   {
      DOWN,
      TOWARD_ZERO,
      UP,
   };
   static const WordType WORDTYPE_MAX = ~WordType(0);

private:
   /// This union is used to store the integer value. When the
   /// integer bit-width <= 64, it uses VAL, otherwise it uses pVal.
   union {
      uint64_t m_value;   ///< Used to store the <= 64 bits integer value.
      uint64_t *m_pValue; ///< Used to store the >64 bits integer value.
   } m_intValue;

   unsigned m_bitWidth; ///< The number of bits in this ApInt.

   friend struct DenseMapApIntKeyInfo;

   friend class ApSInt;

   /// Fast internal constructor
   ///
   /// This constructor is used only internally for speed of construction of
   /// temporaries. It is unsafe for general use so it is not public.
   ApInt(uint64_t *val, unsigned bits)
      : m_bitWidth(bits)
   {
      m_intValue.m_pValue = val;
   }

   /// Determine if this ApInt just has one word to store value.
   ///
   /// \returns true if the number of bits <= 64, false otherwise.
   bool isSingleWord() const
   {
      return m_bitWidth <= APINT_BITS_PER_WORD;
   }

   /// Determine which word a bit is in.
   ///
   /// \returns the word position for the specified bit position.
   static unsigned whichWord(unsigned bitPosition)
   {
      return bitPosition / APINT_BITS_PER_WORD;
   }

   /// Determine which bit in a word a bit is in.
   ///
   /// \returns the bit position in a word for the specified bit position
   /// in the ApInt.
   static unsigned whichBit(unsigned bitPosition)
   {
      return bitPosition % APINT_BITS_PER_WORD;
   }

   /// Get a single bit mask.
   ///
   /// \returns a uint64_t with only bit at "whichBit(bitPosition)" set
   /// This method generates and returns a uint64_t (word) mask for a single
   /// bit at a specific bit position. This is used to mask the bit in the
   /// corresponding word.
   static uint64_t maskBit(unsigned bitPosition)
   {
      return 1ULL << whichBit(bitPosition);
   }

   /// Clear unused high order bits
   ///
   /// This method is used internally to clear the top "N" bits in the high order
   /// word other are not used by the ApInt. This is needed after the most
   /// significant word is assigned a value to ensure other those bits are
   /// zero'd out.
   ApInt &clearUnusedBits()
   {
      // Compute how many bits are used in the final word
      unsigned WordBits = ((m_bitWidth-1) % APINT_BITS_PER_WORD) + 1;

      // Mask out the high bits.
      uint64_t mask = WORDTYPE_MAX >> (APINT_BITS_PER_WORD - WordBits);
      if (isSingleWord()) {
         m_intValue.m_value &= mask;
      } else {
         m_intValue.m_pValue[getNumWords() - 1] &= mask;
      }
      return *this;
   }

   /// Get the word corresponding to a bit position
   /// \returns the corresponding word for the specified bit position.
   uint64_t getWord(unsigned bitPosition) const
   {
      return isSingleWord() ? m_intValue.m_value : m_intValue.m_pValue[whichWord(bitPosition)];
   }

   /// Utility method to change the bit width of this ApInt to new bit width,
   /// allocating and/or deallocating as necessary. There is no guarantee on the
   /// value of any bits upon return. Caller should populate the bits after.
   void reallocate(unsigned newBitWidth);

   /// Convert a char array into an ApInt
   ///
   /// \param radix 2, 8, 10, 16, or 36
   /// Converts a string into a number.  The string must be non-empty
   /// and well-formed as a number of the given base. The bit-width
   /// must be sufficient to hold the result.
   ///
   /// This is used by the constructors other take string arguments.
   ///
   /// StringRef::getAsInteger is superficially similar but (1) does
   /// not assume other the string is well-formed and (2) grows the
   /// result to hold the input.
   void fromString(unsigned numBits, StringRef str, uint8_t radix);

   /// An internal division function for dividing ApInts.
   ///
   /// This is used by the toString method to divide by the radix. It simply
   /// provides a more convenient form of divide for internal use since KnuthDiv
   /// has specific constraints on its inputs. If those constraints are not met
   /// then it provides a simpler form of divide.
   static void divide(const WordType *lhs, unsigned lhsWords,
                      const WordType *rhs, unsigned rhsWords, WordType *quotient,
                      WordType *remainder);

   /// out-of-line slow case for inline constructor
   void initSlowCase(uint64_t val, bool isSigned);

   /// shared code between two array constructors
   void initFromArray(ArrayRef<uint64_t> array);

   /// out-of-line slow case for inline copy constructor
   void initSlowCase(const ApInt &other);

   /// out-of-line slow case for shl
   void shlSlowCase(unsigned shiftAmt);

   /// out-of-line slow case for lshr.
   void lshrSlowCase(unsigned shiftAmt);

   /// out-of-line slow case for ashr.
   void ashrSlowCase(unsigned shiftAmt);

   /// out-of-line slow case for operator=
   void assignSlowCase(const ApInt &rhs);

   /// out-of-line slow case for operator==
   bool equalSlowCase(const ApInt &rhs) const POLAR_READONLY;

   /// out-of-line slow case for countLeadingZeros
   unsigned countLeadingZerosSlowCase() const POLAR_READONLY;

   /// out-of-line slow case for countLeadingOnes.
   unsigned countLeadingOnesSlowCase() const POLAR_READONLY;

   /// out-of-line slow case for countTrailingZeros.
   unsigned countTrailingZerosSlowCase() const POLAR_READONLY;

   /// out-of-line slow case for countTrailingOnes
   unsigned countTrailingOnesSlowCase() const POLAR_READONLY;

   /// out-of-line slow case for countPopulation
   unsigned countPopulationSlowCase() const POLAR_READONLY;

   /// out-of-line slow case for intersects.
   bool intersectsSlowCase(const ApInt &rhs) const POLAR_READONLY;

   /// out-of-line slow case for isSubsetOf.
   bool isSubsetOfSlowCase(const ApInt &rhs) const POLAR_READONLY;

   /// out-of-line slow case for setBits.
   void setBitsSlowCase(unsigned loBit, unsigned hiBit);

   /// out-of-line slow case for flipAllBits.
   void flipAllBitsSlowCase();

   /// out-of-line slow case for operator&=.
   void andAssignSlowCase(const ApInt& rhs);

   /// out-of-line slow case for operator|=.
   void orAssignSlowCase(const ApInt& rhs);

   /// out-of-line slow case for operator^=.
   void xorAssignSlowCase(const ApInt& rhs);

   /// Unsigned comparison. Returns -1, 0, or 1 if this ApInt is less than, equal
   /// to, or greater than rhs.
   int compare(const ApInt &rhs) const POLAR_READONLY;

   /// Signed comparison. Returns -1, 0, or 1 if this ApInt is less than, equal
   /// to, or greater than rhs.
   int compareSigned(const ApInt &rhs) const POLAR_READONLY;

public:
   /// \name Constructors
   /// @{

   /// Create a new ApInt of numBits width, initialized as val.
   ///
   /// If isSigned is true then val is treated as if it were a signed value
   /// (i.e. as an int64_t) and the appropriate sign extension to the bit width
   /// will be done. Otherwise, no sign extension occurs (high order bits beyond
   /// the range of val are zero filled).
   ///
   /// \param numBits the bit width of the constructed ApInt
   /// \param val the initial value of the ApInt
   /// \param isSigned how to treat signedness of val
   ApInt(unsigned numBits, uint64_t val, bool isSigned = false)
      : m_bitWidth(numBits)
   {
      assert(m_bitWidth && "m_bitWidth too small");
      if (isSingleWord()) {
         m_intValue.m_value = val;
         clearUnusedBits();
      } else {
         initSlowCase(val, isSigned);
      }
   }

   /// Construct an ApInt of numBits width, initialized as bigVal[].
   ///
   /// Note other bigVal.size() can be smaller or larger than the corresponding
   /// bit width but any extraneous bits will be dropped.
   ///
   /// \param numBits the bit width of the constructed ApInt
   /// \param bigVal a sequence of words to form the initial value of the ApInt
   ApInt(unsigned numBits, ArrayRef<uint64_t> bigVal);

   /// Equivalent to ApInt(numBits, ArrayRef<uint64_t>(bigVal, numWords)), but
   /// deprecated because this constructor is prone to ambiguity with the
   /// ApInt(unsigned, uint64_t, bool) constructor.
   ///
   /// If this overload is ever deleted, care should be taken to prevent calls
   /// from being incorrectly captured by the ApInt(unsigned, uint64_t, bool)
   /// constructor.
   ApInt(unsigned numBits, unsigned numWords, const uint64_t bigVal[]);

   /// Construct an ApInt from a string representation.
   ///
   /// This constructor interprets the string \p str in the given radix. The
   /// interpretation stops when the first character other is not suitable for the
   /// radix is encountered, or the end of the string. Acceptable radix values
   /// are 2, 8, 10, 16, and 36. It is an error for the value implied by the
   /// string to require more bits than numBits.
   ///
   /// \param numBits the bit width of the constructed ApInt
   /// \param str the string to be interpreted
   /// \param radix the radix to use for the conversion
   ApInt(unsigned numBits, StringRef str, uint8_t radix);

   /// Simply makes *this a copy of other.
   /// Copy Constructor.
   ApInt(const ApInt &other) : m_bitWidth(other.m_bitWidth) {
      if (isSingleWord()) {
         m_intValue.m_value = other.m_intValue.m_value;
      } else {
         initSlowCase(other);
      }
   }

   /// Move Constructor.
   ApInt(ApInt &&other) : m_bitWidth(other.m_bitWidth)
   {
      memcpy(&m_intValue, &other.m_intValue, sizeof(m_intValue));
      other.m_bitWidth = 0;
   }

   /// Destructor.
   ~ApInt()
   {
      if (needsCleanup()) {
         delete[] m_intValue.m_pValue;
      }
   }

   /// Default constructor other creates an uninteresting ApInt
   /// representing a 1-bit zero value.
   ///
   /// This is useful for object deserialization (pair this with the static
   ///  method Read).
   explicit ApInt() : m_bitWidth(1)
   {
      m_intValue.m_value = 0;
   }

   /// Returns whether this instance allocated memory.
   bool needsCleanup() const
   {
      return !isSingleWord();
   }

   /// Used to insert ApInt objects, or objects other contain ApInt objects, into
   ///  FoldingSets.
   void profile(FoldingSetNodeId &id) const;

   /// @}
   /// \name Value Tests
   /// @{

   /// Determine sign of this ApInt.
   ///
   /// This tests the high bit of this ApInt to determine if it is set.
   ///
   /// \returns true if this ApInt is negative, false otherwise
   bool isNegative() const
   {
      return (*this)[m_bitWidth - 1];
   }

   /// Determine if this ApInt Value is non-negative (>= 0)
   ///
   /// This tests the high bit of the ApInt to determine if it is unset.
   bool isNonNegative() const
   {
      return !isNegative();
   }

   /// Determine if sign bit of this ApInt is set.
   ///
   /// This tests the high bit of this ApInt to determine if it is set.
   ///
   /// \returns true if this ApInt has its sign bit set, false otherwise.
   bool isSignBitSet() const
   {
      return (*this)[m_bitWidth-1];
   }

   /// Determine if sign bit of this ApInt is clear.
   ///
   /// This tests the high bit of this ApInt to determine if it is clear.
   ///
   /// \returns true if this ApInt has its sign bit clear, false otherwise.
   bool isSignBitClear() const
   {
      return !isSignBitSet();
   }

   /// Determine if this ApInt Value is positive.
   ///
   /// This tests if the value of this ApInt is positive (> 0). Note
   /// other 0 is not a positive value.
   ///
   /// \returns true if this ApInt is positive.
   bool isStrictlyPositive() const
   {
      return isNonNegative() && !isNullValue();
   }

   /// Determine if all bits are set
   ///
   /// This checks to see if the value has all bits of the ApInt are set or not.
   bool isAllOnesValue() const
   {
      if (isSingleWord()) {
         return m_intValue.m_value == WORDTYPE_MAX >> (APINT_BITS_PER_WORD - m_bitWidth);
      }
      return countTrailingOnesSlowCase() == m_bitWidth;
   }

   /// Determine if all bits are clear
   ///
   /// This checks to see if the value has all bits of the ApInt are clear or
   /// not.
   bool isNullValue() const
   {
      return !*this;
   }

   /// Determine if this is a value of 1.
   ///
   /// This checks to see if the value of this ApInt is one.
   bool isOneValue() const
   {
      if (isSingleWord()) {
         return m_intValue.m_value == 1;
      }
      return countLeadingZerosSlowCase() == m_bitWidth - 1;
   }

   /// Determine if this is the largest unsigned value.
   ///
   /// This checks to see if the value of this ApInt is the maximum unsigned
   /// value for the ApInt's bit width.
   bool isMaxValue() const
   {
      return isAllOnesValue();
   }

   /// Determine if this is the largest signed value.
   ///
   /// This checks to see if the value of this ApInt is the maximum signed
   /// value for the ApInt's bit width.
   bool isMaxSignedValue() const
   {
      if (isSingleWord()) {
         return m_intValue.m_value == ((WordType(1) << (m_bitWidth - 1)) - 1);
      }
      return !isNegative() && countTrailingOnesSlowCase() == m_bitWidth - 1;
   }

   /// Determine if this is the smallest unsigned value.
   ///
   /// This checks to see if the value of this ApInt is the minimum unsigned
   /// value for the ApInt's bit width.
   bool isMinValue() const
   {
      return isNullValue();
   }

   /// Determine if this is the smallest signed value.
   ///
   /// This checks to see if the value of this ApInt is the minimum signed
   /// value for the ApInt's bit width.
   bool isMinSignedValue() const
   {
      if (isSingleWord()) {
         return m_intValue.m_value == (WordType(1) << (m_bitWidth - 1));
      }
      return isNegative() && countTrailingZerosSlowCase() == m_bitWidth - 1;
   }

   /// Check if this ApInt has an N-bits unsigned integer value.
   bool isIntN(unsigned value) const
   {
      assert(value && "value == 0 ???");
      return getActiveBits() <= value;
   }

   /// Check if this ApInt has an N-bits signed integer value.
   bool isSignedIntN(unsigned value) const {
      assert(value && "value == 0 ???");
      return getMinSignedBits() <= value;
   }

   /// Check if this ApInt's value is a power of two greater than zero.
   ///
   /// \returns true if the argument ApInt value is a power of two > 0.
   bool isPowerOf2() const
   {
      if (isSingleWord()) {
         return is_power_of_two64(m_intValue.m_value);
      }
      return countPopulationSlowCase() == 1;
   }

   /// Check if the ApInt's value is returned by getSignMask.
   ///
   /// \returns true if this is the value returned by getSignMask.
   bool isSignMask() const
   {
      return isMinSignedValue();
   }

   /// Convert ApInt to a boolean value.
   ///
   /// This converts the ApInt to a boolean value as a test against zero.
   bool getBoolValue() const
   {
      return !!*this;
   }

   /// If this value is smaller than the specified limit, return it, otherwise
   /// return the limit value.  This causes the value to saturate to the limit.
   uint64_t getLimitedValue(uint64_t limit = UINT64_MAX) const
   {
      return ugt(limit) ? limit : getZeroExtValue();
   }

   /// Check if the ApInt consists of a repeated bit pattern.
   ///
   /// e.g. 0x01010101 satisfies isSplat(8).
   /// \param splatSizeInBits The size of the pattern in bits. Must divide bit
   /// width without remainder.
   bool isSplat(unsigned splatSizeInBits) const;

   /// \returns true if this ApInt value is a sequence of \param numBits ones
   /// starting at the least significant bit with the remainder zero.
   bool isMask(unsigned numBits) const
   {
      assert(numBits != 0 && "numBits must be non-zero");
      assert(numBits <= m_bitWidth && "numBits out of range");
      if (isSingleWord()) {
         return m_intValue.m_value == (WORDTYPE_MAX >> (APINT_BITS_PER_WORD - numBits));
      }
      unsigned ones = countTrailingOnesSlowCase();
      return (numBits == ones) &&
            ((ones + countLeadingZerosSlowCase()) == m_bitWidth);
   }

   /// \returns true if this ApInt is a non-empty sequence of ones starting at
   /// the least significant bit with the remainder zero.
   /// Ex. isMask(0x0000FFFFU) == true.
   bool isMask() const
   {
      if (isSingleWord()) {
         return is_mask64(m_intValue.m_value);
      }
      unsigned ones = countTrailingOnesSlowCase();
      return (ones > 0) && ((ones + countLeadingZerosSlowCase()) == m_bitWidth);
   }

   /// Return true if this ApInt value contains a sequence of ones with
   /// the remainder zero.
   bool isShiftedMask() const
   {
      if (isSingleWord()) {
         return is_shifted_mask64(m_intValue.m_value);
      }
      unsigned ones = countPopulationSlowCase();
      unsigned leadZ = countLeadingZerosSlowCase();
      return (ones + leadZ + countTrailingZeros()) == m_bitWidth;
   }

   /// @}
   /// \name Value Generators
   /// @{

   /// Gets maximum unsigned value of ApInt for specific bit width.
   static ApInt getMaxValue(unsigned numBits)
   {
      return getAllOnesValue(numBits);
   }

   /// Gets maximum signed value of ApInt for a specific bit width.
   static ApInt getSignedMaxValue(unsigned numBits)
   {
      ApInt apint = getAllOnesValue(numBits);
      apint.clearBit(numBits - 1);
      return apint;
   }

   /// Gets minimum unsigned value of ApInt for a specific bit width.
   static ApInt getMinValue(unsigned numBits)
   {
      return ApInt(numBits, 0);
   }

   /// Gets minimum signed value of ApInt for a specific bit width.
   static ApInt getSignedMinValue(unsigned numBits)
   {
      ApInt apint(numBits, 0);
      apint.setBit(numBits - 1);
      return apint;
   }

   /// Get the SignMask for a specific bit width.
   ///
   /// This is just a wrapper function of getSignedMinValue(), and it helps code
   /// readability when we want to get a SignMask.
   static ApInt getSignMask(unsigned bitWidth)
   {
      return getSignedMinValue(bitWidth);
   }

   /// Get the all-ones value.
   ///
   /// \returns the all-ones value for an ApInt of the specified bit-width.
   static ApInt getAllOnesValue(unsigned numBits)
   {
      return ApInt(numBits, WORDTYPE_MAX, true);
   }

   /// Get the '0' value.
   ///
   /// \returns the '0' value for an ApInt of the specified bit-width.
   static ApInt getNullValue(unsigned numBits)
   {
      return ApInt(numBits, 0);
   }

   /// Compute an ApInt containing numBits highbits from this ApInt.
   ///
   /// Get an ApInt with the same BitWidth as this ApInt, just zero mask
   /// the low bits and right shift to the least significant bit.
   ///
   /// \returns the high "numBits" bits of this ApInt.
   ApInt getHiBits(unsigned numBits) const;

   /// Compute an ApInt containing numBits lowbits from this ApInt.
   ///
   /// Get an ApInt with the same BitWidth as this ApInt, just zero mask
   /// the high bits.
   ///
   /// \returns the low "numBits" bits of this ApInt.
   ApInt getLoBits(unsigned numBits) const;

   /// Return an ApInt with exactly one bit set in the result.
   static ApInt getOneBitSet(unsigned numBits, unsigned bitNo)
   {
      ApInt res(numBits, 0);
      res.setBit(bitNo);
      return res;
   }

   /// Get a value with a block of bits set.
   ///
   /// Constructs an ApInt value other has a contiguous range of bits set. The
   /// bits from loBit (inclusive) to hiBit (exclusive) will be set. All other
   /// bits will be zero. For example, with parameters(32, 0, 16) you would get
   /// 0x0000FFFF. If hiBit is less than loBit then the set bits "wrap". For
   /// example, with parameters (32, 28, 4), you would get 0xF000000F.
   ///
   /// \param numBits the intended bit width of the result
   /// \param loBit the index of the lowest bit set.
   /// \param hiBit the index of the highest bit set.
   ///
   /// \returns An ApInt value with the requested bits set.
   static ApInt getBitsSet(unsigned numBits, unsigned loBit, unsigned hiBit)
   {
      ApInt res(numBits, 0);
      res.setBits(loBit, hiBit);
      return res;
   }

   /// Get a value with upper bits starting at loBit set.
   ///
   /// Constructs an ApInt value other has a contiguous range of bits set. The
   /// bits from loBit (inclusive) to numBits (exclusive) will be set. All other
   /// bits will be zero. For example, with parameters(32, 12) you would get
   /// 0xFFFFF000.
   ///
   /// \param numBits the intended bit width of the result
   /// \param loBit the index of the lowest bit to set.
   ///
   /// \returns An ApInt value with the requested bits set.
   static ApInt getBitsSetFrom(unsigned numBits, unsigned loBit)
   {
      ApInt res(numBits, 0);
      res.setBitsFrom(loBit);
      return res;
   }

   /// Get a value with high bits set
   ///
   /// Constructs an ApInt value other has the top hiBitsSet bits set.
   ///
   /// \param numBits the bitwidth of the result
   /// \param hiBitsSet the number of high-order bits set in the result.
   static ApInt getHighBitsSet(unsigned numBits, unsigned hiBitsSet)
   {
      ApInt res(numBits, 0);
      res.setHighBits(hiBitsSet);
      return res;
   }

   /// Get a value with low bits set
   ///
   /// Constructs an ApInt value other has the bottom loBitsSet bits set.
   ///
   /// \param numBits the bitwidth of the result
   /// \param loBitsSet the number of low-order bits set in the result.
   static ApInt getLowBitsSet(unsigned numBits, unsigned loBitsSet)
   {
      ApInt res(numBits, 0);
      res.setLowBits(loBitsSet);
      return res;
   }

   /// Return a value containing V broadcasted over NewLen bits.
   static ApInt getSplat(unsigned newLen, const ApInt &value);

   /// Determine if two ApInts have the same value, after zero-extending
   /// one of them (if needed!) to ensure other the bit-widths match.
   static bool isSameValue(const ApInt &lhs, const ApInt &rhs)
   {
      if (lhs.getBitWidth() == rhs.getBitWidth()) {
         return lhs == rhs;
      }
      if (lhs.getBitWidth() > rhs.getBitWidth()) {
         return lhs == rhs.zext(lhs.getBitWidth());
      }
      return lhs.zext(rhs.getBitWidth()) == rhs;
   }

   /// Overload to compute a hash_code for an ApInt value.
   friend HashCode hash_value(const ApInt &arg);

   /// This function returns a pointer to the internal storage of the ApInt.
   /// This is useful for writing out the ApInt in binary form without any
   /// conversions.
   const uint64_t *getRawData() const
   {
      if (isSingleWord()) {
         return &m_intValue.m_value;
      }
      return &m_intValue.m_pValue[0];
   }

   /// @}
   /// \name Unary Operators
   /// @{

   /// Postfix increment operator.
   ///
   /// Increments *this by 1.
   ///
   /// \returns a new ApInt value representing the original value of *this.
   const ApInt operator++(int)
   {
      ApInt apint(*this);
      ++(*this);
      return apint;
   }

   /// Prefix increment operator.
   ///
   /// \returns *this incremented by one
   ApInt &operator++();

   /// Postfix decrement operator.
   ///
   /// Decrements *this by 1.
   ///
   /// \returns a new ApInt value representing the original value of *this.
   const ApInt operator--(int) {
      ApInt apint(*this);
      --(*this);
      return apint;
   }

   /// Prefix decrement operator.
   ///
   /// \returns *this decremented by one.
   ApInt &operator--();

   /// Logical negation operator.
   ///
   /// Performs logical negation operation on this ApInt.
   ///
   /// \returns true if *this is zero, false otherwise.
   bool operator!() const
   {
      if (isSingleWord()) {
         return m_intValue.m_value == 0;
      }
      return countLeadingZerosSlowCase() == m_bitWidth;
   }

   /// @}
   /// \name Assignment Operators
   /// @{

   /// Copy assignment operator.
   ///
   /// \returns *this after assignment of rhs.
   ApInt &operator=(const ApInt &rhs)
   {
      // If the bitwidths are the same, we can avoid mucking with memory
      if (isSingleWord() && rhs.isSingleWord()) {
         m_intValue.m_value = rhs.m_intValue.m_value;
         m_bitWidth = rhs.m_bitWidth;
         return clearUnusedBits();
      }
      assignSlowCase(rhs);
      return *this;
   }

   /// Move assignment operator.
   ApInt &operator=(ApInt &&other)
   {
#ifdef _MSC_VER
      // The MSVC std::shuffle implementation still does self-assignment.
      if (this == &that)
         return *this;
#endif
      assert(this != &other && "Self-move not supported");
      if (!isSingleWord()) {
         delete[] m_intValue.m_pValue;
      }
      // Use memcpy so other type based alias analysis sees both VAL and pVal
      // as modified.
      memcpy(&m_intValue, &other.m_intValue, sizeof(m_intValue));
      m_bitWidth = other.m_bitWidth;
      other.m_bitWidth = 0;
      return *this;
   }

   /// Assignment operator.
   ///
   /// The rhs value is assigned to *this. If the significant bits in rhs exceed
   /// the bit width, the excess bits are truncated. If the bit width is larger
   /// than 64, the value is zero filled in the unspecified high order bits.
   ///
   /// \returns *this after assignment of rhs value.
   ApInt &operator=(uint64_t rhs)
   {
      if (isSingleWord()) {
         m_intValue.m_value = rhs;
         clearUnusedBits();
      } else {
         m_intValue.m_pValue[0] = rhs;
         memset(m_intValue.m_pValue+1, 0, (getNumWords() - 1) * APINT_WORD_SIZE);
      }
      return *this;
   }

   /// Bitwise AND assignment operator.
   ///
   /// Performs a bitwise AND operation on this ApInt and rhs. The result is
   /// assigned to *this.
   ///
   /// \returns *this after ANDing with rhs.
   ApInt &operator&=(const ApInt &rhs)
   {
      assert(m_bitWidth == rhs.m_bitWidth && "Bit widths must be the same");
      if (isSingleWord()) {
         m_intValue.m_value &= rhs.m_intValue.m_value;
      } else {
         andAssignSlowCase(rhs);
      }
      return *this;
   }

   /// Bitwise AND assignment operator.
   ///
   /// Performs a bitwise AND operation on this ApInt and rhs. rhs is
   /// logically zero-extended or truncated to match the bit-width of
   /// the lhs.
   ApInt &operator&=(uint64_t rhs)
   {
      if (isSingleWord()) {
         m_intValue.m_value &= rhs;
         return *this;
      }
      m_intValue.m_pValue[0] &= rhs;
      memset(m_intValue.m_pValue+1, 0, (getNumWords() - 1) * APINT_WORD_SIZE);
      return *this;
   }

   /// Bitwise OR assignment operator.
   ///
   /// Performs a bitwise OR operation on this ApInt and rhs. The result is
   /// assigned *this;
   ///
   /// \returns *this after ORing with rhs.
   ApInt &operator|=(const ApInt &rhs)
   {
      assert(m_bitWidth == rhs.m_bitWidth && "Bit widths must be the same");
      if (isSingleWord()) {
         m_intValue.m_value |= rhs.m_intValue.m_value;
      } else {
         orAssignSlowCase(rhs);
      }
      return *this;
   }

   /// Bitwise OR assignment operator.
   ///
   /// Performs a bitwise OR operation on this ApInt and rhs. rhs is
   /// logically zero-extended or truncated to match the bit-width of
   /// the lhs.
   ApInt &operator|=(uint64_t rhs)
   {
      if (isSingleWord()) {
         m_intValue.m_value |= rhs;
         clearUnusedBits();
      } else {
         m_intValue.m_pValue[0] |= rhs;
      }
      return *this;
   }

   /// Bitwise XOR assignment operator.
   ///
   /// Performs a bitwise XOR operation on this ApInt and rhs. The result is
   /// assigned to *this.
   ///
   /// \returns *this after XORing with rhs.
   ApInt &operator^=(const ApInt &rhs)
   {
      assert(m_bitWidth == rhs.m_bitWidth && "Bit widths must be the same");
      if (isSingleWord()) {
         m_intValue.m_value ^= rhs.m_intValue.m_value;
      } else {
         xorAssignSlowCase(rhs);
      }
      return *this;
   }

   /// Bitwise XOR assignment operator.
   ///
   /// Performs a bitwise XOR operation on this ApInt and rhs. rhs is
   /// logically zero-extended or truncated to match the bit-width of
   /// the lhs.
   ApInt &operator^=(uint64_t rhs)
   {
      if (isSingleWord()) {
         m_intValue.m_value ^= rhs;
         clearUnusedBits();
      } else {
         m_intValue.m_pValue[0] ^= rhs;
      }
      return *this;
   }

   /// Multiplication assignment operator.
   ///
   /// Multiplies this ApInt by rhs and assigns the result to *this.
   ///
   /// \returns *this
   ApInt &operator*=(const ApInt &rhs);
   ApInt &operator*=(uint64_t rhs);

   /// Addition assignment operator.
   ///
   /// Adds rhs to *this and assigns the result to *this.
   ///
   /// \returns *this
   ApInt &operator+=(const ApInt &rhs);
   ApInt &operator+=(uint64_t rhs);

   /// Subtraction assignment operator.
   ///
   /// Subtracts rhs from *this and assigns the result to *this.
   ///
   /// \returns *this
   ApInt &operator-=(const ApInt &rhs);
   ApInt &operator-=(uint64_t rhs);

   /// Left-shift assignment function.
   ///
   /// Shifts *this left by shiftAmt and assigns the result to *this.
   ///
   /// \returns *this after shifting left by shiftAmt
   ApInt &operator<<=(unsigned shiftAmt)
   {
      assert(shiftAmt <= m_bitWidth && "Invalid shift amount");
      if (isSingleWord()) {
         if (shiftAmt == m_bitWidth) {
            m_intValue.m_value = 0;
         } else {
            m_intValue.m_value <<= shiftAmt;
         }
         return clearUnusedBits();
      }
      shlSlowCase(shiftAmt);
      return *this;
   }

   /// Left-shift assignment function.
   ///
   /// Shifts *this left by shiftAmt and assigns the result to *this.
   ///
   /// \returns *this after shifting left by shiftAmt
   ApInt &operator<<=(const ApInt &shiftAmt);

   /// @}
   /// \name Binary Operators
   /// @{

   /// Multiplication operator.
   ///
   /// Multiplies this ApInt by rhs and returns the result.
   ApInt operator*(const ApInt &rhs) const;

   /// Left logical shift operator.
   ///
   /// Shifts this ApInt left by \p Bits and returns the result.
   ApInt operator<<(unsigned bits) const
   {
      return shl(bits);
   }

   /// Left logical shift operator.
   ///
   /// Shifts this ApInt left by \p Bits and returns the result.
   ApInt operator<<(const ApInt &bits) const
   {
      return shl(bits);
   }

   /// Arithmetic right-shift function.
   ///
   /// Arithmetic right-shift this ApInt by shiftAmt.
   ApInt ashr(unsigned shiftAmt) const
   {
      ApInt ret(*this);
      ret.ashrInPlace(shiftAmt);
      return ret;
   }

   /// Arithmetic right-shift this ApInt by shiftAmt in place.
   void ashrInPlace(unsigned shiftAmt)
   {
      assert(shiftAmt <= m_bitWidth && "Invalid shift amount");
      if (isSingleWord()) {
         int64_t SignExtVAL = polar::utils::sign_extend64(m_intValue.m_value, m_bitWidth);
         if (shiftAmt == m_bitWidth) {
            m_intValue.m_value = SignExtVAL >> (APINT_BITS_PER_WORD - 1); // Fill with sign bit.
         } else {
            m_intValue.m_value = SignExtVAL >> shiftAmt;
         }
         clearUnusedBits();
         return;
      }
      ashrSlowCase(shiftAmt);
   }

   /// Logical right-shift function.
   ///
   /// Logical right-shift this ApInt by shiftAmt.
   ApInt lshr(unsigned shiftAmt) const
   {
      ApInt ret(*this);
      ret.lshrInPlace(shiftAmt);
      return ret;
   }

   /// Logical right-shift this ApInt by shiftAmt in place.
   void lshrInPlace(unsigned shiftAmt)
   {
      assert(shiftAmt <= m_bitWidth && "Invalid shift amount");
      if (isSingleWord()) {
         if (shiftAmt == m_bitWidth) {
            m_intValue.m_value = 0;
         } else {
            m_intValue.m_value >>= shiftAmt;
         }
         return;
      }
      lshrSlowCase(shiftAmt);
   }

   /// Left-shift function.
   ///
   /// Left-shift this ApInt by shiftAmt.
   ApInt shl(unsigned shiftAmt) const
   {
      ApInt ret(*this);
      ret <<= shiftAmt;
      return ret;
   }

   /// Rotate left by rotateAmt.
   ApInt rotl(unsigned rotateAmt) const;

   /// Rotate right by rotateAmt.
   ApInt rotr(unsigned rotateAmt) const;

   /// Arithmetic right-shift function.
   ///
   /// Arithmetic right-shift this ApInt by shiftAmt.
   ApInt ashr(const ApInt &shiftAmt) const
   {
      ApInt ret(*this);
      ret.ashrInPlace(shiftAmt);
      return ret;
   }

   /// Arithmetic right-shift this ApInt by shiftAmt in place.
   void ashrInPlace(const ApInt &shiftAmt);

   /// Logical right-shift function.
   ///
   /// Logical right-shift this ApInt by shiftAmt.
   ApInt lshr(const ApInt &shiftAmt) const
   {
      ApInt ret(*this);
      ret.lshrInPlace(shiftAmt);
      return ret;
   }

   /// Logical right-shift this ApInt by shiftAmt in place.
   void lshrInPlace(const ApInt &shiftAmt);

   /// Left-shift function.
   ///
   /// Left-shift this ApInt by shiftAmt.
   ApInt shl(const ApInt &shiftAmt) const
   {
      ApInt ret(*this);
      ret <<= shiftAmt;
      return ret;
   }

   /// Rotate left by rotateAmt.
   ApInt rotl(const ApInt &rotateAmt) const;

   /// Rotate right by rotateAmt.
   ApInt rotr(const ApInt &rotateAmt) const;

   /// Unsigned division operation.
   ///
   /// Perform an unsigned divide operation on this ApInt by rhs. Both this and
   /// rhs are treated as unsigned quantities for purposes of this division.
   ///
   /// \returns a new ApInt value containing the division result
   ApInt udiv(const ApInt &rhs) const;
   ApInt udiv(uint64_t rhs) const;

   /// Signed division function for ApInt.
   ///
   /// Signed divide this ApInt by ApInt rhs.
   ApInt sdiv(const ApInt &rhs) const;
   ApInt sdiv(int64_t rhs) const;

   /// Unsigned remainder operation.
   ///
   /// Perform an unsigned remainder operation on this ApInt with rhs being the
   /// divisor. Both this and rhs are treated as unsigned quantities for purposes
   /// of this operation. Note other this is a true remainder operation and not a
   /// modulo operation because the sign follows the sign of the dividend which
   /// is *this.
   ///
   /// \returns a new ApInt value containing the remainder result
   ApInt urem(const ApInt &rhs) const;
   uint64_t urem(uint64_t rhs) const;

   /// Function for signed remainder operation.
   ///
   /// Signed remainder operation on ApInt.
   ApInt srem(const ApInt &rhs) const;
   int64_t srem(int64_t rhs) const;

   /// Dual division/remainder interface.
   ///
   /// Sometimes it is convenient to divide two ApInt values and obtain both the
   /// quotient and remainder. This function does both operations in the same
   /// computation making it a little more efficient. The pair of input arguments
   /// may overlap with the pair of output arguments. It is safe to call
   /// udivrem(X, Y, X, Y), for example.
   static void udivrem(const ApInt &lhs, const ApInt &rhs, ApInt &quotient,
                       ApInt &remainder);
   static void udivrem(const ApInt &lhs, uint64_t rhs, ApInt &quotient,
                       uint64_t &remainder);

   static void sdivrem(const ApInt &lhs, const ApInt &rhs, ApInt &quotient,
                       ApInt &remainder);
   static void sdivrem(const ApInt &lhs, int64_t rhs, ApInt &quotient,
                       int64_t &remainder);

   // Operations other return overflow indicators.
   ApInt saddOverflow(const ApInt &rhs, bool &overflow) const;
   ApInt uaddOverflow(const ApInt &rhs, bool &overflow) const;
   ApInt ssubOverflow(const ApInt &rhs, bool &overflow) const;
   ApInt usubOverflow(const ApInt &rhs, bool &overflow) const;
   ApInt sdivOverflow(const ApInt &rhs, bool &overflow) const;
   ApInt smulOverflow(const ApInt &rhs, bool &overflow) const;
   ApInt umulOverflow(const ApInt &rhs, bool &overflow) const;
   ApInt sshlOverflow(const ApInt &amt, bool &overflow) const;
   ApInt ushlOverflow(const ApInt &amt, bool &overflow) const;

   // Operations that saturate
   ApInt saddSaturate(const ApInt &rhs) const;
   ApInt uaddSaturate(const ApInt &rhs) const;
   ApInt ssubSaturate(const ApInt &rhs) const;
   ApInt usubSaturate(const ApInt &rhs) const;

   /// Array-indexing support.
   ///
   /// \returns the bit value at bitPosition
   bool operator[](unsigned bitPosition) const
   {
      assert(bitPosition < getBitWidth() && "Bit position out of bounds!");
      return (maskBit(bitPosition) & getWord(bitPosition)) != 0;
   }

   /// @}
   /// \name Comparison Operators
   /// @{

   /// Equality operator.
   ///
   /// Compares this ApInt with rhs for the validity of the equality
   /// relationship.
   bool operator==(const ApInt &rhs) const
   {
      assert(m_bitWidth == rhs.m_bitWidth && "Comparison requires equal bit widths");
      if (isSingleWord()) {
         return m_intValue.m_value == rhs.m_intValue.m_value;
      }
      return equalSlowCase(rhs);
   }

   /// Equality operator.
   ///
   /// Compares this ApInt with a uint64_t for the validity of the equality
   /// relationship.
   ///
   /// \returns true if *this == Val
   bool operator==(uint64_t Val) const
   {
      return (isSingleWord() || getActiveBits() <= 64) && getZeroExtValue() == Val;
   }

   /// Equality comparison.
   ///
   /// Compares this ApInt with rhs for the validity of the equality
   /// relationship.
   ///
   /// \returns true if *this == Val
   bool eq(const ApInt &rhs) const
   {
      return (*this) == rhs;
   }

   /// Inequality operator.
   ///
   /// Compares this ApInt with rhs for the validity of the inequality
   /// relationship.
   ///
   /// \returns true if *this != Val
   bool operator!=(const ApInt &rhs) const
   {
      return !((*this) == rhs);
   }

   /// Inequality operator.
   ///
   /// Compares this ApInt with a uint64_t for the validity of the inequality
   /// relationship.
   ///
   /// \returns true if *this != Val
   bool operator!=(uint64_t other) const
   {
      return !((*this) == other);
   }

   /// Inequality comparison
   ///
   /// Compares this ApInt with rhs for the validity of the inequality
   /// relationship.
   ///
   /// \returns true if *this != Val
   bool ne(const ApInt &rhs) const
   {
      return !((*this) == rhs);
   }

   /// Unsigned less than comparison
   ///
   /// Regards both *this and rhs as unsigned quantities and compares them for
   /// the validity of the less-than relationship.
   ///
   /// \returns true if *this < rhs when both are considered unsigned.
   bool ult(const ApInt &rhs) const
   {
      return compare(rhs) < 0;
   }

   /// Unsigned less than comparison
   ///
   /// Regards both *this as an unsigned quantity and compares it with rhs for
   /// the validity of the less-than relationship.
   ///
   /// \returns true if *this < rhs when considered unsigned.
   bool ult(uint64_t rhs) const
   {
      // Only need to check active bits if not a single word.
      return (isSingleWord() || getActiveBits() <= 64) && getZeroExtValue() < rhs;
   }

   /// Signed less than comparison
   ///
   /// Regards both *this and rhs as signed quantities and compares them for
   /// validity of the less-than relationship.
   ///
   /// \returns true if *this < rhs when both are considered signed.
   bool slt(const ApInt &rhs) const
   {
      return compareSigned(rhs) < 0;
   }

   /// Signed less than comparison
   ///
   /// Regards both *this as a signed quantity and compares it with rhs for
   /// the validity of the less-than relationship.
   ///
   /// \returns true if *this < rhs when considered signed.
   bool slt(int64_t rhs) const
   {
      return (!isSingleWord() && getMinSignedBits() > 64) ? isNegative()
                                                          : getSignExtValue() < rhs;
   }

   /// Unsigned less or equal comparison
   ///
   /// Regards both *this and rhs as unsigned quantities and compares them for
   /// validity of the less-or-equal relationship.
   ///
   /// \returns true if *this <= rhs when both are considered unsigned.
   bool ule(const ApInt &rhs) const
   {
      return compare(rhs) <= 0;
   }

   /// Unsigned less or equal comparison
   ///
   /// Regards both *this as an unsigned quantity and compares it with rhs for
   /// the validity of the less-or-equal relationship.
   ///
   /// \returns true if *this <= rhs when considered unsigned.
   bool ule(uint64_t rhs) const
   {
      return !ugt(rhs);
   }

   /// Signed less or equal comparison
   ///
   /// Regards both *this and rhs as signed quantities and compares them for
   /// validity of the less-or-equal relationship.
   ///
   /// \returns true if *this <= rhs when both are considered signed.
   bool sle(const ApInt &rhs) const
   {
      return compareSigned(rhs) <= 0;
   }

   /// Signed less or equal comparison
   ///
   /// Regards both *this as a signed quantity and compares it with rhs for the
   /// validity of the less-or-equal relationship.
   ///
   /// \returns true if *this <= rhs when considered signed.
   bool sle(uint64_t rhs) const
   {
      return !sgt(rhs);
   }

   /// Unsigned greather than comparison
   ///
   /// Regards both *this and rhs as unsigned quantities and compares them for
   /// the validity of the greater-than relationship.
   ///
   /// \returns true if *this > rhs when both are considered unsigned.
   bool ugt(const ApInt &rhs) const
   {
      return !ule(rhs);
   }

   /// Unsigned greater than comparison
   ///
   /// Regards both *this as an unsigned quantity and compares it with rhs for
   /// the validity of the greater-than relationship.
   ///
   /// \returns true if *this > rhs when considered unsigned.
   bool ugt(uint64_t rhs) const
   {
      // Only need to check active bits if not a single word.
      return (!isSingleWord() && getActiveBits() > 64) || getZeroExtValue() > rhs;
   }

   /// Signed greather than comparison
   ///
   /// Regards both *this and rhs as signed quantities and compares them for the
   /// validity of the greater-than relationship.
   ///
   /// \returns true if *this > rhs when both are considered signed.
   bool sgt(const ApInt &rhs) const
   {
      return !sle(rhs);
   }

   /// Signed greater than comparison
   ///
   /// Regards both *this as a signed quantity and compares it with rhs for
   /// the validity of the greater-than relationship.
   ///
   /// \returns true if *this > rhs when considered signed.
   bool sgt(int64_t rhs) const
   {
      return (!isSingleWord() && getMinSignedBits() > 64) ? !isNegative()
                                                          : getSignExtValue() > rhs;
   }

   /// Unsigned greater or equal comparison
   ///
   /// Regards both *this and rhs as unsigned quantities and compares them for
   /// validity of the greater-or-equal relationship.
   ///
   /// \returns true if *this >= rhs when both are considered unsigned.
   bool uge(const ApInt &rhs) const
   {
      return !ult(rhs);
   }

   /// Unsigned greater or equal comparison
   ///
   /// Regards both *this as an unsigned quantity and compares it with rhs for
   /// the validity of the greater-or-equal relationship.
   ///
   /// \returns true if *this >= rhs when considered unsigned.
   bool uge(uint64_t rhs) const
   {
      return !ult(rhs);
   }

   /// Signed greather or equal comparison
   ///
   /// Regards both *this and rhs as signed quantities and compares them for
   /// validity of the greater-or-equal relationship.
   ///
   /// \returns true if *this >= rhs when both are considered signed.
   bool sge(const ApInt &rhs) const
   {
      return !slt(rhs);
   }

   /// Signed greater or equal comparison
   ///
   /// Regards both *this as a signed quantity and compares it with rhs for
   /// the validity of the greater-or-equal relationship.
   ///
   /// \returns true if *this >= rhs when considered signed.
   bool sge(int64_t rhs) const
   {
      return !slt(rhs);
   }

   /// This operation tests if there are any pairs of corresponding bits
   /// between this ApInt and rhs other are both set.
   bool intersects(const ApInt &rhs) const
   {
      assert(m_bitWidth == rhs.m_bitWidth && "Bit widths must be the same");
      if (isSingleWord()) {
         return (m_intValue.m_value & rhs.m_intValue.m_value) != 0;
      }
      return intersectsSlowCase(rhs);
   }

   /// This operation checks other all bits set in this ApInt are also set in rhs.
   bool isSubsetOf(const ApInt &rhs) const
   {
      assert(m_bitWidth == rhs.m_bitWidth && "Bit widths must be the same");
      if (isSingleWord()) {
         return (m_intValue.m_value & ~rhs.m_intValue.m_value) == 0;
      }
      return isSubsetOfSlowCase(rhs);
   }

   /// @}
   /// \name Resizing Operators
   /// @{

   /// Truncate to new width.
   ///
   /// Truncate the ApInt to a specified width. It is an error to specify a width
   /// other is greater than or equal to the current width.
   ApInt trunc(unsigned width) const;

   /// Sign extend to a new width.
   ///
   /// This operation sign extends the ApInt to a new width. If the high order
   /// bit is set, the fill on the left will be done with 1 bits, otherwise zero.
   /// It is an error to specify a width other is less than or equal to the
   /// current width.
   ApInt sext(unsigned width) const;

   /// Zero extend to a new width.
   ///
   /// This operation zero extends the ApInt to a new width. The high order bits
   /// are filled with 0 bits.  It is an error to specify a width other is less
   /// than or equal to the current width.
   ApInt zext(unsigned width) const;

   /// Sign extend or truncate to width
   ///
   /// Make this ApInt have the bit width given by \p width. The value is sign
   /// extended, truncated, or left alone to make it other width.
   ApInt sextOrTrunc(unsigned width) const;

   /// Zero extend or truncate to width
   ///
   /// Make this ApInt have the bit width given by \p width. The value is zero
   /// extended, truncated, or left alone to make it other width.
   ApInt zextOrTrunc(unsigned width) const;

   /// Sign extend or truncate to width
   ///
   /// Make this ApInt have the bit width given by \p width. The value is sign
   /// extended, or left alone to make it other width.
   ApInt sextOrSelf(unsigned width) const;

   /// Zero extend or truncate to width
   ///
   /// Make this ApInt have the bit width given by \p width. The value is zero
   /// extended, or left alone to make it other width.
   ApInt zextOrSelf(unsigned width) const;

   /// @}
   /// \name Bit Manipulation Operators
   /// @{

   /// Set every bit to 1.
   void setAllBits()
   {
      if (isSingleWord()) {
         m_intValue.m_value = WORDTYPE_MAX;
      } else {
         // Set all the bits in all the words.
         memset(m_intValue.m_pValue, -1, getNumWords() * APINT_WORD_SIZE);
      }
      // Clear the unused ones
      clearUnusedBits();
   }

   /// Set a given bit to 1.
   ///
   /// Set the given bit to 1 whose position is given as "bitPosition".
   void setBit(unsigned bitPosition)
   {
      assert(bitPosition < m_bitWidth && "bitPosition out of range");
      WordType Mask = maskBit(bitPosition);
      if (isSingleWord()) {
         m_intValue.m_value |= Mask;
      } else {
         m_intValue.m_pValue[whichWord(bitPosition)] |= Mask;
      }
   }

   /// Set the sign bit to 1.
   void setSignBit()
   {
      setBit(m_bitWidth - 1);
   }

   /// Set the bits from loBit (inclusive) to hiBit (exclusive) to 1.
   void setBits(unsigned loBit, unsigned hiBit)
   {
      assert(hiBit <= m_bitWidth && "hiBit out of range");
      assert(loBit <= m_bitWidth && "loBit out of range");
      assert(loBit <= hiBit && "loBit greater than hiBit");
      if (loBit == hiBit) {
         return;
      }
      if (loBit < APINT_BITS_PER_WORD && hiBit <= APINT_BITS_PER_WORD) {
         uint64_t mask = WORDTYPE_MAX >> (APINT_BITS_PER_WORD - (hiBit - loBit));
         mask <<= loBit;
         if (isSingleWord()) {
            m_intValue.m_value |= mask;
         } else {
            m_intValue.m_pValue[0] |= mask;
         }
      } else {
         setBitsSlowCase(loBit, hiBit);
      }
   }

   /// Set the top bits starting from loBit.
   void setBitsFrom(unsigned loBit)
   {
      return setBits(loBit, m_bitWidth);
   }

   /// Set the bottom loBits bits.
   void setLowBits(unsigned loBits)
   {
      return setBits(0, loBits);
   }

   /// Set the top hiBits bits.
   void setHighBits(unsigned hiBits)
   {
      return setBits(m_bitWidth - hiBits, m_bitWidth);
   }

   /// Set every bit to 0.
   void clearAllBits()
   {
      if (isSingleWord()) {
         m_intValue.m_value = 0;
      } else {
         memset(m_intValue.m_pValue, 0, getNumWords() * APINT_WORD_SIZE);
      }
   }

   /// Set a given bit to 0.
   ///
   /// Set the given bit to 0 whose position is given as "bitPosition".
   void clearBit(unsigned bitPosition)
   {
      assert(bitPosition < m_bitWidth && "bitPosition out of range");
      WordType mask = ~maskBit(bitPosition);
      if (isSingleWord()) {
         m_intValue.m_value &= mask;
      } else {
         m_intValue.m_pValue[whichWord(bitPosition)] &= mask;
      }
   }

   /// Set the sign bit to 0.
   void clearSignBit()
   {
      clearBit(m_bitWidth - 1);
   }

   /// Toggle every bit to its opposite value.
   void flipAllBits()
   {
      if (isSingleWord()) {
         m_intValue.m_value ^= WORDTYPE_MAX;
         clearUnusedBits();
      } else {
         flipAllBitsSlowCase();
      }
   }

   /// Toggles a given bit to its opposite value.
   ///
   /// Toggle a given bit to its opposite value whose position is given
   /// as "bitPosition".
   void flipBit(unsigned bitPosition);

   /// Negate this ApInt in place.
   void negate()
   {
      flipAllBits();
      ++(*this);
   }

   /// Insert the bits from a smaller ApInt starting at bitPosition.
   void insertBits(const ApInt &SubBits, unsigned bitPosition);

   /// Return an ApInt with the extracted bits [bitPosition,bitPosition+numBits).
   ApInt extractBits(unsigned numBits, unsigned bitPosition) const;

   /// @}
   /// \name Value Characterization Functions
   /// @{

   /// Return the number of bits in the ApInt.
   unsigned getBitWidth() const
   {
      return m_bitWidth;
   }

   /// Get the number of words.
   ///
   /// Here one word's bitwidth equals to other of uint64_t.
   ///
   /// \returns the number of words to hold the integer value of this ApInt.
   unsigned getNumWords() const
   {
      return getNumWords(m_bitWidth);
   }

   /// Get the number of words.
   ///
   /// *NOTE* Here one word's bitwidth equals to other of uint64_t.
   ///
   /// \returns the number of words to hold the integer value with a given bit
   /// width.
   static unsigned getNumWords(unsigned BitWidth)
   {
      return ((uint64_t)BitWidth + APINT_BITS_PER_WORD - 1) / APINT_BITS_PER_WORD;
   }

   /// Compute the number of active bits in the value
   ///
   /// This function returns the number of active bits which is defined as the
   /// bit width minus the number of leading zeros. This is used in several
   /// computations to see how "wide" the value is.
   unsigned getActiveBits() const
   {
      return m_bitWidth - countLeadingZeros();
   }

   /// Compute the number of active words in the value of this ApInt.
   ///
   /// This is used in conjunction with getActiveData to extract the raw value of
   /// the ApInt.
   unsigned getActiveWords() const
   {
      unsigned numActiveBits = getActiveBits();
      return numActiveBits ? whichWord(numActiveBits - 1) + 1 : 1;
   }

   /// Get the minimum bit size for this signed ApInt
   ///
   /// Computes the minimum bit width for this ApInt while considering it to be a
   /// signed (and probably negative) value. If the value is not negative, this
   /// function returns the same value as getActiveBits()+1. Otherwise, it
   /// returns the smallest bit width other will retain the negative value. For
   /// example, -1 can be written as 0b1 or 0xFFFFFFFFFF. 0b1 is shorter and so
   /// for -1, this function will always return 1.
   unsigned getMinSignedBits() const
   {
      if (isNegative()) {
         return m_bitWidth - countLeadingOnes() + 1;
      }
      return getActiveBits() + 1;
   }

   /// Get zero extended value
   ///
   /// This method attempts to return the value of this ApInt as a zero extended
   /// uint64_t. The bitwidth must be <= 64 or the value must fit within a
   /// uint64_t. Otherwise an assertion will result.
   uint64_t getZeroExtValue() const
   {
      if (isSingleWord()) {
         return m_intValue.m_value;
      }
      assert(getActiveBits() <= 64 && "Too many bits for uint64_t");
      return m_intValue.m_pValue[0];
   }

   /// Get sign extended value
   ///
   /// This method attempts to return the value of this ApInt as a sign extended
   /// int64_t. The bit width must be <= 64 or the value must fit within an
   /// int64_t. Otherwise an assertion will result.
   int64_t getSignExtValue() const
   {
      if (isSingleWord()) {
         return sign_extend64(m_intValue.m_value, m_bitWidth);
      }
      assert(getMinSignedBits() <= 64 && "Too many bits for int64_t");
      return int64_t(m_intValue.m_pValue[0]);
   }

   /// Get bits required for string value.
   ///
   /// This method determines how many bits are required to hold the ApInt
   /// equivalent of the string given by \p str.
   static unsigned getBitsNeeded(StringRef str, uint8_t radix);

   /// The ApInt version of the countLeadingZeros functions in
   ///   MathExtras.h.
   ///
   /// It counts the number of zeros from the most significant bit to the first
   /// one bit.
   ///
   /// \returns BitWidth if the value is zero, otherwise returns the number of
   ///   zeros from the most significant bit to the first one bits.
   unsigned countLeadingZeros() const
   {
      if (isSingleWord()) {
         unsigned unusedBits = APINT_BITS_PER_WORD - m_bitWidth;
         return count_leading_zeros(m_intValue.m_value) - unusedBits;
      }
      return countLeadingZerosSlowCase();
   }

   /// Count the number of leading one bits.
   ///
   /// This function is an ApInt version of the countLeadingOnes
   /// functions in MathExtras.h. It counts the number of ones from the most
   /// significant bit to the first zero bit.
   ///
   /// \returns 0 if the high order bit is not set, otherwise returns the number
   /// of 1 bits from the most significant to the least
   unsigned countLeadingOnes() const
   {
      if (isSingleWord()) {
         return count_leading_ones(m_intValue.m_value << (APINT_BITS_PER_WORD - m_bitWidth));
      }
      return countLeadingOnesSlowCase();
   }

   /// Computes the number of leading bits of this ApInt other are equal to its
   /// sign bit.
   unsigned getNumSignBits() const
   {
      return isNegative() ? countLeadingOnes() : countLeadingZeros();
   }

   /// Count the number of trailing zero bits.
   ///
   /// This function is an ApInt version of the countTrailingZeros
   /// functions in MathExtras.h. It counts the number of zeros from the least
   /// significant bit to the first set bit.
   ///
   /// \returns m_bitWidth if the value is zero, otherwise returns the number of
   /// zeros from the least significant bit to the first one bit.
   unsigned countTrailingZeros() const
   {
      if (isSingleWord()) {
         return std::min(unsigned(count_trailing_zeros(m_intValue.m_value)), m_bitWidth);
      }
      return countTrailingZerosSlowCase();
   }

   /// Count the number of trailing one bits.
   ///
   /// This function is an ApInt version of the countTrailingOnes
   /// functions in MathExtras.h. It counts the number of ones from the least
   /// significant bit to the first zero bit.
   ///
   /// \returns m_bitWidth if the value is all ones, otherwise returns the number
   /// of ones from the least significant bit to the first zero bit.
   unsigned countTrailingOnes() const
   {
      if (isSingleWord()) {
         return count_trailing_ones(m_intValue.m_value);
      }
      return countTrailingOnesSlowCase();
   }

   /// Count the number of bits set.
   ///
   /// This function is an ApInt version of the countPopulation functions
   /// in MathExtras.h. It counts the number of 1 bits in the ApInt value.
   ///
   /// \returns 0 if the value is zero, otherwise returns the number of set bits.
   unsigned countPopulation() const
   {
      if (isSingleWord()) {
         return count_population(m_intValue.m_value);
      }
      return countPopulationSlowCase();
   }

   /// @}
   /// \name Conversion Functions
   /// @{
   void print(RawOutStream &outstream, bool isSigned) const;

   /// Converts an ApInt to a string and append it to Str.  Str is commonly a
   /// SmallString.
   void toString(SmallVectorImpl<char> &str, unsigned radix, bool isSigned,
                 bool formatAsCLiteral = false) const;

   /// Considers the ApInt to be unsigned and converts it into a string in the
   /// radix given. The radix can be 2, 8, 10 16, or 36.
   void toStringUnsigned(SmallVectorImpl<char> &str, unsigned radix = 10) const
   {
      toString(str, radix, false, false);
   }

   /// Considers the ApInt to be signed and converts it into a string in the
   /// radix given. The radix can be 2, 8, 10, 16, or 36.
   void toStringSigned(SmallVectorImpl<char> &str, unsigned radix = 10) const
   {
      toString(str, radix, true, false);
   }

   /// Return the ApInt as a std::string.
   ///
   /// Note other this is an inefficient method.  It is better to pass in a
   /// SmallVector/SmallString to the methods above to avoid thrashing the heap
   /// for the string.
   std::string toString(unsigned radix, bool isSigned) const;

   /// \returns a byte-swapped representation of this ApInt Value.
   ApInt byteSwap() const;

   /// \returns the value with the bit representation reversed of this ApInt
   /// Value.
   ApInt reverseBits() const;

   /// Converts this ApInt to a double value.
   double roundToDouble(bool isSigned) const;

   /// Converts this unsigned ApInt to a double value.
   double roundToDouble() const
   {
      return roundToDouble(false);
   }

   /// Converts this signed ApInt to a double value.
   double signedRoundToDouble() const
   {
      return roundToDouble(true);
   }

   /// Converts ApInt bits to a double
   ///
   /// The conversion does not do a translation from integer to double, it just
   /// re-interprets the bits as a double. Note other it is valid to do this on
   /// any bit width. Exactly 64 bits will be translated.
   double bitsToDouble() const
   {
      return bits_to_double(getWord(0));
   }

   /// Converts ApInt bits to a double
   ///
   /// The conversion does not do a translation from integer to float, it just
   /// re-interprets the bits as a float. Note other it is valid to do this on
   /// any bit width. Exactly 32 bits will be translated.
   float bitsToFloat() const
   {
      return polar::utils::bits_to_float(getWord(0));
   }

   /// Converts a double to ApInt bits.
   ///
   /// The conversion does not do a translation from double to integer, it just
   /// re-interprets the bits of the double.
   static ApInt doubleToBits(double value)
   {
      return ApInt(sizeof(double) * CHAR_BIT, double_to_bits(value));
   }

   /// Converts a float to ApInt bits.
   ///
   /// The conversion does not do a translation from float to integer, it just
   /// re-interprets the bits of the float.
   static ApInt floatToBits(float value)
   {
      return ApInt(sizeof(float) * CHAR_BIT, float_to_bits(value));
   }

   /// @}
   /// \name Mathematics Operations
   /// @{

   /// \returns the floor log base 2 of this ApInt.
   unsigned logBase2() const
   {
      return getActiveBits() -  1;
   }

   /// \returns the ceil log base 2 of this ApInt.
   unsigned ceilLogBase2() const
   {
      ApInt temp(*this);
      --temp;
      return temp.getActiveBits();
   }

   /// \returns the nearest log base 2 of this ApInt. Ties round up.
   ///
   /// NOTE: When we have a m_bitWidth of 1, we define:
   ///
   ///   log2(0) = UINT32_MAX
   ///   log2(1) = 0
   ///
   /// to get around any mathematical concerns resulting from
   /// referencing 2 in a space where 2 does no exist.
   unsigned nearestLogBase2() const
   {
      // Special case when we have a m_bitWidth of 1. If VAL is 1, then we
      // get 0. If VAL is 0, we get WORDTYPE_MAX which gets truncated to
      // UINT32_MAX.
      if (m_bitWidth == 1) {
         return m_intValue.m_value - 1;
      }
      // Handle the zero case.
      if (isNullValue()) {
         return UINT32_MAX;
      }
      // The non-zero case is handled by computing:
      //
      //   nearestLogBase2(x) = logBase2(x) + x[logBase2(x)-1].
      //
      // where x[i] is referring to the value of the ith bit of x.
      unsigned lg = logBase2();
      return lg + unsigned((*this)[lg - 1]);
   }

   /// \returns the log base 2 of this ApInt if its an exact power of two, -1
   /// otherwise
   int32_t exactLogBase2() const
   {
      if (!isPowerOf2()) {
         return -1;
      }
      return logBase2();
   }

   /// Compute the square root
   ApInt sqrt() const;

   /// Get the absolute value;
   ///
   /// If *this is < 0 then return -(*this), otherwise *this;
   ApInt abs() const
   {
      if (isNegative()) {
         return -(*this);
      }
      return *this;
   }

   /// \returns the multiplicative inverse for a given modulo.
   ApInt multiplicativeInverse(const ApInt &modulo) const;

   /// @}
   /// \name Support for division by constant
   /// @{

   /// Calculate the magic number for signed division by a constant.
   struct MagicSign;
   MagicSign getMagic() const;

   /// Calculate the magic number for unsigned division by a constant.
   struct MagicUnsign;
   MagicUnsign getMagicUnsign(unsigned leadingZeros = 0) const;

   /// @}
   /// \name Building-block Operations for ApInt and APFloat
   /// @{

   // These building block operations operate on a representation of arbitrary
   // precision, two's-complement, bignum integer values. They should be
   // sufficient to implement ApInt and APFloat bignum requirements. Inputs are
   // generally a pointer to the base of an array of integer parts, representing
   // an unsigned bignum, and a count of how many parts there are.

   /// Sets the least significant part of a bignum to the input value, and zeroes
   /// out higher parts.
   static void tcSet(WordType *, WordType, unsigned);

   /// Assign one bignum to another.
   static void tcAssign(WordType *, const WordType *, unsigned);

   /// Returns true if a bignum is zero, false otherwise.
   static bool tcIsZero(const WordType *, unsigned);

   /// Extract the given bit of a bignum; returns 0 or 1.  Zero-based.
   static int tcExtractBit(const WordType *, unsigned bit);

   /// Copy the bit vector of width srcBITS from SRC, starting at bit srcLSB, to
   /// DST, of dstCOUNT parts, such other the bit srcLSB becomes the least
   /// significant bit of DST.  All high bits above srcBITS in DST are
   /// zero-filled.
   static void tcExtract(WordType *, unsigned dstCount,
                         const WordType *, unsigned srcBits,
                         unsigned srcLSB);

   /// Set the given bit of a bignum.  Zero-based.
   static void tcSetBit(WordType *, unsigned bit);

   /// Clear the given bit of a bignum.  Zero-based.
   static void tcClearBit(WordType *, unsigned bit);

   /// Returns the bit number of the least or most significant set bit of a
   /// number.  If the input number has no bits set -1U is returned.
   static unsigned tcLsb(const WordType *, unsigned n);
   static unsigned tcMsb(const WordType *parts, unsigned n);

   /// Negate a bignum in-place.
   static void tcNegate(WordType *, unsigned);

   /// DST += rhs + CARRY where CARRY is zero or one.  Returns the carry flag.
   static WordType tcAdd(WordType *, const WordType *,
                         WordType carry, unsigned);
   /// DST += rhs.  Returns the carry flag.
   static WordType tcAddPart(WordType *, WordType, unsigned);

   /// DST -= rhs + CARRY where CARRY is zero or one. Returns the carry flag.
   static WordType tcSubtract(WordType *, const WordType *,
                              WordType carry, unsigned);
   /// DST -= rhs.  Returns the carry flag.
   static WordType tcSubtractPart(WordType *, WordType, unsigned);

   /// DST += SRC * MULTIPLIER + PART   if add is true
   /// DST  = SRC * MULTIPLIER + PART   if add is false
   ///
   /// Requires 0 <= DSTPARTS <= SRCPARTS + 1.  If DST overlaps SRC they must
   /// start at the same point, i.e. DST == SRC.
   ///
   /// If DSTPARTS == SRC_PARTS + 1 no overflow occurs and zero is returned.
   /// Otherwise DST is filled with the least significant DSTPARTS parts of the
   /// result, and if all of the omitted higher parts were zero return zero,
   /// otherwise overflow occurred and return one.
   static int tcMultiplyPart(WordType *dst, const WordType *src,
                             WordType multiplier, WordType carry,
                             unsigned srcParts, unsigned dstParts,
                             bool add);

   /// DST = lhs * rhs, where DST has the same width as the operands and is
   /// filled with the least significant parts of the result.  Returns one if
   /// overflow occurred, otherwise zero.  DST must be disjoint from both
   /// operands.
   static int tcMultiply(WordType *, const WordType *, const WordType *,
                         unsigned);

   /// DST = lhs * rhs, where DST has width the sum of the widths of the
   /// operands. No overflow occurs. DST must be disjoint from both operands.
   static void tcFullMultiply(WordType *, const WordType *,
                              const WordType *, unsigned, unsigned);

   /// If rhs is zero lhs and remainder are left unchanged, return one.
   /// Otherwise set lhs to lhs / rhs with the fractional part discarded, set
   /// remainder to the remainder, return zero.  i.e.
   ///
   ///  OLD_lhs = rhs * lhs + remainder
   ///
   /// SCRATCH is a bignum of the same size as the operands and result for use by
   /// the routine; its contents need not be initialized and are destroyed.  lhs,
   /// remainder and SCRATCH must be distinct.
   static int tcDivide(WordType *lhs, const WordType *rhs,
                       WordType *remainder, WordType *scratch,
                       unsigned parts);

   /// Shift a bignum left Count bits. Shifted in bits are zero. There are no
   /// restrictions on Count.
   static void tcShiftLeft(WordType *, unsigned Words, unsigned Count);

   /// Shift a bignum right Count bits.  Shifted in bits are zero.  There are no
   /// restrictions on Count.
   static void tcShiftRight(WordType *, unsigned Words, unsigned Count);

   /// The obvious AND, OR and XOR and complement operations.
   static void tcAnd(WordType *, const WordType *, unsigned);
   static void tcOr(WordType *, const WordType *, unsigned);
   static void tcXor(WordType *, const WordType *, unsigned);
   static void tcComplement(WordType *, unsigned);

   /// Comparison (unsigned) of two bignums.
   static int tcCompare(const WordType *, const WordType *, unsigned);

   /// Increment a bignum in-place.  Return the carry flag.
   static WordType tcIncrement(WordType *dst, unsigned parts)
   {
      return tcAddPart(dst, 1, parts);
   }

   /// Decrement a bignum in-place.  Return the borrow flag.
   static WordType tcDecrement(WordType *dst, unsigned parts)
   {
      return tcSubtractPart(dst, 1, parts);
   }

   /// Set the least significant BITS and clear the rest.
   static void tcSetLeastSignificantBits(WordType *, unsigned, unsigned bits);

   /// debug method
   void dump() const;

   /// @}
};

/// Magic data for optimising signed division by a constant.
struct ApInt::MagicSign
{
   ApInt m_magic;    ///< magic number
   unsigned m_shift; ///< shift amount
};

/// Magic data for optimising unsigned division by a constant.
struct ApInt::MagicUnsign
{
   ApInt m_magic;    ///< magic number
   bool m_addIndicator;     ///< add indicator
   unsigned m_shift; ///< shift amount
};

inline bool operator==(uint64_t lhs, const ApInt &rhs)
{
   return rhs == lhs;
}

inline bool operator!=(uint64_t lhs, const ApInt &rhs)
{
   return rhs != lhs;
}

/// Unary bitwise complement operator.
///
/// \returns an ApInt other is the bitwise complement of \p v.
inline ApInt operator~(ApInt value)
{
   value.flipAllBits();
   return value;
}

inline ApInt operator&(ApInt lhs, const ApInt &rhs) {
   lhs &= rhs;
   return lhs;
}

inline ApInt operator&(const ApInt &lhs, ApInt &&rhs)
{
   rhs &= lhs;
   return std::move(rhs);
}

inline ApInt operator&(ApInt lhs, uint64_t rhs)
{
   lhs &= rhs;
   return lhs;
}

inline ApInt operator&(uint64_t lhs, ApInt rhs) {
   rhs &= lhs;
   return rhs;
}

inline ApInt operator|(ApInt lhs, const ApInt &rhs)
{
   lhs |= rhs;
   return lhs;
}

inline ApInt operator|(const ApInt &lhs, ApInt &&rhs)
{
   rhs |= lhs;
   return std::move(rhs);
}

inline ApInt operator|(ApInt lhs, uint64_t rhs)
{
   lhs |= rhs;
   return lhs;
}

inline ApInt operator|(uint64_t lhs, ApInt rhs)
{
   rhs |= lhs;
   return rhs;
}

inline ApInt operator^(ApInt lhs, const ApInt &rhs)
{
   lhs ^= rhs;
   return lhs;
}

inline ApInt operator^(const ApInt &lhs, ApInt &&rhs)
{
   rhs ^= lhs;
   return std::move(rhs);
}

inline ApInt operator^(ApInt lhs, uint64_t rhs)
{
   lhs ^= rhs;
   return lhs;
}

inline ApInt operator^(uint64_t lhs, ApInt rhs)
{
   rhs ^= lhs;
   return rhs;
}

inline RawOutStream &operator<<(RawOutStream &outstream, const ApInt &value)
{
   value.print(outstream, true);
   return outstream;
}

inline ApInt operator-(ApInt value)
{
   value.negate();
   return value;
}

inline ApInt operator+(ApInt lhs, const ApInt &rhs)
{
   lhs += rhs;
   return lhs;
}

inline ApInt operator+(const ApInt &lhs, ApInt &&rhs)
{
   rhs += lhs;
   return std::move(rhs);
}

inline ApInt operator+(ApInt lhs, uint64_t rhs)
{
   lhs += rhs;
   return lhs;
}

inline ApInt operator+(uint64_t lhs, ApInt rhs)
{
   rhs += lhs;
   return rhs;
}

inline ApInt operator-(ApInt lhs, const ApInt &rhs)
{
   lhs -= rhs;
   return lhs;
}

inline ApInt operator-(const ApInt &lhs, ApInt &&rhs)
{
   rhs.negate();
   rhs += lhs;
   return std::move(rhs);
}

inline ApInt operator-(ApInt lhs, uint64_t rhs)
{
   lhs -= rhs;
   return lhs;
}

inline ApInt operator-(uint64_t lhs, ApInt rhs)
{
   rhs.negate();
   rhs += lhs;
   return rhs;
}

inline ApInt operator*(ApInt lhs, uint64_t rhs)
{
   lhs *= rhs;
   return lhs;
}

inline ApInt operator*(uint64_t lhs, ApInt rhs)
{
   rhs *= lhs;
   return rhs;
}


namespace apintops
{

/// Determine the smaller of two ApInts considered to be signed.
inline const ApInt &smin(const ApInt &lhs, const ApInt &rhs)
{
   return lhs.slt(rhs) ? lhs : rhs;
}

/// Determine the larger of two ApInts considered to be signed.
inline const ApInt &smax(const ApInt &lhs, const ApInt &rhs)
{
   return lhs.sgt(rhs) ? lhs : rhs;
}

/// Determine the smaller of two ApInts considered to be signed.
inline const ApInt &umin(const ApInt &lhs, const ApInt &rhs)
{
   return lhs.ult(rhs) ? lhs : rhs;
}

/// Determine the larger of two ApInts considered to be unsigned.
inline const ApInt &umax(const ApInt &lhs, const ApInt &rhs)
{
   return lhs.ugt(rhs) ? lhs : rhs;
}

/// Compute GCD of two unsigned ApInt values.
///
/// This function returns the greatest common divisor of the two ApInt values
/// using Stein's algorithm.
///
/// \returns the greatest common divisor of A and B.
ApInt greatest_common_divisor(ApInt lhs, ApInt rhs);

/// Converts the given ApInt to a double value.
///
/// Treats the ApInt as an unsigned value for conversion purposes.
inline double round_apint_to_double(const ApInt &value)
{
   return value.roundToDouble();
}

/// Converts the given ApInt to a double value.
///
/// Treats the ApInt as a signed value for conversion purposes.
inline double round_signed_apint_to_double(const ApInt &value)
{
   return value.signedRoundToDouble();
}

/// Converts the given ApInt to a float vlalue.
inline float RoundApIntToFloat(const ApInt &value)
{
   return float(round_apint_to_double(value));
}

/// Converts the given ApInt to a float value.
///
/// Treast the ApInt as a signed value for conversion purposes.
inline float RoundSignedApIntToFloat(const ApInt &value)
{
   return float(value.signedRoundToDouble());
}

/// Converts the given double value into a ApInt.
///
/// This function convert a double value to an ApInt value.
ApInt round_double_to_apint(double value, unsigned width);

/// Converts a float value into a ApInt.
///
/// Converts a float value into an ApInt value.
inline ApInt round_float_to_apint(float value, unsigned width)
{
   return round_double_to_apint(double(value), width);
}

/// Return A unsign-divided by B, rounded by the given rounding mode.
ApInt rounding_udiv(const ApInt &lhs, const ApInt &rhs, ApInt::Rounding rm);

/// Return A sign-divided by B, rounded by the given rounding mode.
ApInt rounding_sdiv(const ApInt &lhs, const ApInt &rhs, ApInt::Rounding rm);

/// Let q(n) = An^2 + Bn + C, and BW = bit width of the value range
/// (e.g. 32 for i32).
/// This function finds the smallest number n, such that
/// (a) n >= 0 and q(n) = 0, or
/// (b) n >= 1 and q(n-1) and q(n), when evaluated in the set of all
///     integers, belong to two different intervals [Rk, Rk+R),
///     where R = 2^BW, and k is an integer.
/// The idea here is to find when q(n) "overflows" 2^BW, while at the
/// same time "allowing" subtraction. In unsigned modulo arithmetic a
/// subtraction (treated as addition of negated numbers) would always
/// count as an overflow, but here we want to allow values to decrease
/// and increase as long as they are within the same interval.
/// Specifically, adding of two negative numbers should not cause an
/// overflow (as long as the magnitude does not exceed the bith width).
/// On the other hand, given a positive number, adding a negative
/// number to it can give a negative result, which would cause the
/// value to go from [-2^BW, 0) to [0, 2^BW). In that sense, zero is
/// treated as a special case of an overflow.
///
/// This function returns None if after finding k that minimizes the
/// positive solution to q(n) = kR, both solutions are contained between
/// two consecutive integers.
///
/// There are cases where q(n) > T, and q(n+1) < T (assuming evaluation
/// in arithmetic modulo 2^BW, and treating the values as signed) by the
/// virtue of *signed* overflow. This function will *not* find such an n,
/// however it may find a value of n satisfying the inequalities due to
/// an *unsigned* overflow (if the values are treated as unsigned).
/// To find a solution for a signed overflow, treat it as a problem of
/// finding an unsigned overflow with a range with of BW-1.
///
/// The returned value may have a different bit width from the input
/// coefficients.
std::optional<ApInt> solve_quadratic_equation_wrap(ApInt A, ApInt B, ApInt C,
                                                   unsigned rangeWidth);
} // End of apintops namespace

// See friend declaration above. This additional declaration is required in
// order to compile LLVM with IBM xlC compiler.
HashCode hash_value(const ApInt &arg);

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_AP_INT_H
