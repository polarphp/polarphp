// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/01.

#ifndef POLARPHP_UTILS_KONWN_BITS_H
#define POLARPHP_UTILS_KONWN_BITS_H

#include "polarphp/basic/adt/ApInt.h"

namespace polar::utils {

using polar::basic::ApInt;

// Struct for tracking the known zeros and ones of a value.
struct KnownBits
{
   ApInt zero;
   ApInt one;

private:
   // Internal constructor for creating a KnownBits from two APInts.
   KnownBits(ApInt z, ApInt o)
      : zero(std::move(z)),
        one(std::move(o))
   {}

public:
   // Default construct zero and one.
   KnownBits()
   {}

   /// Create a known bits object of bitWidth bits initialized to unknown.
   KnownBits(unsigned bitWidth)
      : zero(bitWidth, 0),
        one(bitWidth, 0)
   {}

   /// Get the bit width of this value.
   unsigned getBitWidth() const
   {
      assert(zero.getBitWidth() == one.getBitWidth() &&
             "zero and one should have the same width!");
      return zero.getBitWidth();
   }

   /// Returns true if there is conflicting information.
   bool hasConflict() const
   {
      return zero.intersects(one);
   }

   /// Returns true if we know the value of all bits.
   bool isConstant() const
   {
      assert(!hasConflict() && "KnownBits conflict!");
      return zero.countPopulation() + one.countPopulation() == getBitWidth();
   }

   /// Returns the value when all bits have a known value. This just returns one
   /// with a protective assertion.
   const ApInt &getConstant() const
   {
      assert(isConstant() && "Can only get value when all bits are known");
      return one;
   }

   /// Returns true if we don't know any bits.
   bool isUnknown() const
   {
      return zero.isNullValue() && one.isNullValue();
   }

   /// Resets the known state of all bits.
   void resetAll()
   {
      zero.clearAllBits();
      one.clearAllBits();
   }

   /// Returns true if value is all zero.
   bool isZero() const
   {
      assert(!hasConflict() && "KnownBits conflict!");
      return zero.isAllOnesValue();
   }

   /// Returns true if value is all one bits.
   bool isAllOnes() const
   {
      assert(!hasConflict() && "KnownBits conflict!");
      return one.isAllOnesValue();
   }

   /// Make all bits known to be zero and discard any previous information.
   void setAllZero()
   {
      zero.setAllBits();
      one.clearAllBits();
   }

   /// Make all bits known to be one and discard any previous information.
   void setAllOnes()
   {
      zero.clearAllBits();
      one.setAllBits();
   }

   /// Returns true if this value is known to be negative.
   bool isNegative() const
   {
      return one.isSignBitSet();
   }

   /// Returns true if this value is known to be non-negative.
   bool isNonNegative() const
   {
      return zero.isSignBitSet();
   }

   /// Make this value negative.
   void makeNegative()
   {
      one.setSignBit();
   }

   /// Make this value negative.
   void makeNonNegative()
   {
      zero.setSignBit();
   }

   /// Truncate the underlying known zero and one bits. This is equivalent
   /// to truncating the value we're tracking.
   KnownBits trunc(unsigned bitWidth) const
   {
      return KnownBits(zero.trunc(bitWidth), one.trunc(bitWidth));
   }

   /// Extends the underlying known Zero and One bits.
   /// By setting ExtendedBitsAreKnownZero=true this will be equivalent to
   /// zero extending the value we're tracking.
   /// With ExtendedBitsAreKnownZero=false the extended bits are set to unknown.
   KnownBits zext(unsigned bitWidth, bool extendedBitsAreKnownZero) const
   {
      unsigned oldBitWidth = getBitWidth();
      ApInt newZero = zero.zext(bitWidth);
      if (extendedBitsAreKnownZero) {
         newZero.setBitsFrom(oldBitWidth);
      }
      return KnownBits(newZero, one.zext(bitWidth));
   }

   /// Sign extends the underlying known zero and one bits. This is equivalent
   /// to sign extending the value we're tracking.
   KnownBits sext(unsigned bitWidth) const
   {
      return KnownBits(zero.sext(bitWidth), one.sext(bitWidth));
   }

   /// Extends or truncates the underlying known Zero and One bits. When
   /// extending the extended bits can either be set as known zero (if
   /// ExtendedBitsAreKnownZero=true) or as unknown (if
   /// ExtendedBitsAreKnownZero=false).
   KnownBits zextOrTrunc(unsigned bitWidth, bool extendedBitsAreKnownZero) const
   {
      if (bitWidth > getBitWidth()) {
         return zext(bitWidth, extendedBitsAreKnownZero);
      }
      return KnownBits(zero.zextOrTrunc(bitWidth), one.zextOrTrunc(bitWidth));
   }

   /// Returns the minimum number of trailing zero bits.
   unsigned countMinTrailingZeros() const
   {
      return zero.countTrailingOnes();
   }

   /// Returns the minimum number of trailing one bits.
   unsigned countMinTrailingOnes() const
   {
      return one.countTrailingOnes();
   }

   /// Returns the minimum number of leading zero bits.
   unsigned countMinLeadingZeros() const
   {
      return zero.countLeadingOnes();
   }

   /// Returns the minimum number of leading one bits.
   unsigned countMinLeadingOnes() const
   {
      return one.countLeadingOnes();
   }

   /// Returns the number of times the sign bit is replicated into the other
   /// bits.
   unsigned countMinSignBits() const
   {
      if (isNonNegative()) {
         return countMinLeadingZeros();
      }
      if (isNegative()) {
         return countMinLeadingOnes();
      }
      return 0;
   }

   /// Returns the maximum number of trailing zero bits possible.
   unsigned countMaxTrailingZeros() const
   {
      return one.countTrailingZeros();
   }

   /// Returns the maximum number of trailing one bits possible.
   unsigned countMaxTrailingOnes() const
   {
      return zero.countTrailingZeros();
   }

   /// Returns the maximum number of leading zero bits possible.
   unsigned countMaxLeadingZeros() const
   {
      return one.countLeadingZeros();
   }

   /// Returns the maximum number of leading one bits possible.
   unsigned countMaxLeadingOnes() const
   {
      return zero.countLeadingZeros();
   }

   /// Returns the number of bits known to be one.
   unsigned countMinPopulation() const
   {
      return one.countPopulation();
   }

   /// Returns the maximum number of bits that could be one.
   unsigned countMaxPopulation() const
   {
      return getBitWidth() - zero.countPopulation();
   }

   /// Compute known bits resulting from adding LHS, RHS and a 1-bit Carry.
   static KnownBits computeForAddCarry(
         const KnownBits &lhs, const KnownBits &rhs, const KnownBits &carry);

   /// Compute known bits resulting from adding LHS and RHS.
   static KnownBits computeForAddSub(bool add, bool nsw, const KnownBits &lhs,
                                     KnownBits rhs);
};

} // polar::utils

#endif // POLARPHP_UTILS_KONWN_BITS_H
