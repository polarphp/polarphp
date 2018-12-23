// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/01.

#ifndef POLARPHP_UTILS_KONWN_BITS_H
#define POLARPHP_UTILS_KONWN_BITS_H

#include "polarphp/basic/adt/ApInt.h"

namespace polar {
namespace utils {

using polar::basic::ApInt;

// Struct for tracking the known zeros and ones of a value.
struct KnownBits
{
   ApInt m_zero;
   ApInt m_one;

private:
   // Internal constructor for creating a KnownBits from two APInts.
   KnownBits(ApInt zero, ApInt one)
      : m_zero(std::move(zero)), m_one(std::move(one))
   {}

public:
   // Default construct m_zero and m_one.
   KnownBits()
   {}

   /// Create a known bits object of bitWidth bits initialized to unknown.
   KnownBits(unsigned bitWidth) : m_zero(bitWidth, 0), m_one(bitWidth, 0)
   {}

   /// Get the bit width of this value.
   unsigned getBitWidth() const
   {
      assert(m_zero.getBitWidth() == m_one.getBitWidth() &&
             "m_zero and m_one should have the same width!");
      return m_zero.getBitWidth();
   }

   /// Returns true if there is conflicting information.
   bool hasConflict() const
   {
      return m_zero.intersects(m_one);
   }

   /// Returns true if we know the value of all bits.
   bool isConstant() const
   {
      assert(!hasConflict() && "KnownBits conflict!");
      return m_zero.countPopulation() + m_one.countPopulation() == getBitWidth();
   }

   /// Returns the value when all bits have a known value. This just returns m_one
   /// with a protective assertion.
   const ApInt &getConstant() const
   {
      assert(isConstant() && "Can only get value when all bits are known");
      return m_one;
   }

   /// Returns true if we don't know any bits.
   bool isUnknown() const
   {
      return m_zero.isNullValue() && m_one.isNullValue();
   }

   /// Resets the known state of all bits.
   void resetAll()
   {
      m_zero.clearAllBits();
      m_one.clearAllBits();
   }

   /// Returns true if value is all zero.
   bool isZero() const
   {
      assert(!hasConflict() && "KnownBits conflict!");
      return m_zero.isAllOnesValue();
   }

   /// Returns true if value is all one bits.
   bool isAllOnes() const
   {
      assert(!hasConflict() && "KnownBits conflict!");
      return m_one.isAllOnesValue();
   }

   /// Make all bits known to be zero and discard any previous information.
   void setAllZero()
   {
      m_zero.setAllBits();
      m_one.clearAllBits();
   }

   /// Make all bits known to be one and discard any previous information.
   void setAllOnes()
   {
      m_zero.clearAllBits();
      m_one.setAllBits();
   }

   /// Returns true if this value is known to be negative.
   bool isNegative() const
   {
      return m_one.isSignBitSet();
   }

   /// Returns true if this value is known to be non-negative.
   bool isNonNegative() const
   {
      return m_zero.isSignBitSet();
   }

   /// Make this value negative.
   void makeNegative()
   {
      m_one.setSignBit();
   }

   /// Make this value negative.
   void makeNonNegative()
   {
      m_zero.setSignBit();
   }

   /// Truncate the underlying known m_zero and m_one bits. This is equivalent
   /// to truncating the value we're tracking.
   KnownBits trunc(unsigned bitWidth)
   {
      return KnownBits(m_zero.trunc(bitWidth), m_one.trunc(bitWidth));
   }

   /// m_zero extends the underlying known m_zero and m_one bits. This is equivalent
   /// to zero extending the value we're tracking.
   KnownBits zext(unsigned bitWidth)
   {
      return KnownBits(m_zero.zext(bitWidth), m_one.zext(bitWidth));
   }

   /// Sign extends the underlying known m_zero and m_one bits. This is equivalent
   /// to sign extending the value we're tracking.
   KnownBits sext(unsigned bitWidth)
   {
      return KnownBits(m_zero.sext(bitWidth), m_one.sext(bitWidth));
   }

   /// m_zero extends or truncates the underlying known m_zero and m_one bits. This is
   /// equivalent to zero extending or truncating the value we're tracking.
   KnownBits zextOrTrunc(unsigned bitWidth)
   {
      return KnownBits(m_zero.zextOrTrunc(bitWidth), m_one.zextOrTrunc(bitWidth));
   }

   /// Returns the minimum number of trailing zero bits.
   unsigned countMinTrailingZeros() const
   {
      return m_zero.countTrailingOnes();
   }

   /// Returns the minimum number of trailing one bits.
   unsigned countMinTrailingOnes() const
   {
      return m_one.countTrailingOnes();
   }

   /// Returns the minimum number of leading zero bits.
   unsigned countMinLeadingZeros() const
   {
      return m_zero.countLeadingOnes();
   }

   /// Returns the minimum number of leading one bits.
   unsigned countMinLeadingOnes() const
   {
      return m_one.countLeadingOnes();
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
      return m_one.countTrailingZeros();
   }

   /// Returns the maximum number of trailing one bits possible.
   unsigned countMaxTrailingOnes() const
   {
      return m_zero.countTrailingZeros();
   }

   /// Returns the maximum number of leading zero bits possible.
   unsigned countMaxLeadingZeros() const
   {
      return m_one.countLeadingZeros();
   }

   /// Returns the maximum number of leading one bits possible.
   unsigned countMaxLeadingOnes() const
   {
      return m_zero.countLeadingZeros();
   }

   /// Returns the number of bits known to be one.
   unsigned countMinPopulation() const
   {
      return m_one.countPopulation();
   }

   /// Returns the maximum number of bits that could be one.
   unsigned countMaxPopulation() const
   {
      return getBitWidth() - m_zero.countPopulation();
   }

   /// Compute known bits resulting from adding LHS and RHS.
   static KnownBits computeForAddSub(bool add, bool nsw, const KnownBits &lhs,
                                     KnownBits rhs);
};

} // utils
} // polar

#endif // POLARPHP_UTILS_KONWN_BITS_H
