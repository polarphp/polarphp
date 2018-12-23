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

#ifndef POLARPHP_BASIC_ADT_AP_SINT_H
#define POLARPHP_BASIC_ADT_AP_SINT_H

#include "polarphp/basic/adt/ApInt.h"
#include "polarphp/basic/adt/StringRef.h"

namespace polar {
namespace basic {

class POLAR_NODISCARD ApSInt : public ApInt
{
   bool m_isUnsigned;

public:
   /// Default constructor that creates an uninitialized ApInt.
   explicit ApSInt() : m_isUnsigned(false) {}

   /// ApSInt ctor - Create an ApSInt with the specified width, default to
   /// unsigned.
   explicit ApSInt(uint32_t bitWidth, bool m_isUnsigned = true)
      : ApInt(bitWidth, 0), m_isUnsigned(m_isUnsigned)
   {}

   explicit ApSInt(ApInt apint, bool m_isUnsigned = true)
      : ApInt(std::move(apint)),
        m_isUnsigned(m_isUnsigned)
   {}

   /// Construct an ApSInt from a string representation.
   ///
   /// This constructor interprets the string \p Str using the radix of 10.
   /// The interpretation stops at the end of the string. The bit width of the
   /// constructed ApSInt is determined automatically.
   ///
   /// \param Str the string to be interpreted.
   explicit ApSInt(StringRef str);

   ApSInt &operator=(ApInt other)
   {
      // Retain our current sign.
      ApInt::operator=(std::move(other));
      return *this;
   }

   ApSInt &operator=(uint64_t other)
   {
      // Retain our current sign.
      ApInt::operator=(other);
      return *this;
   }

   // Query sign information.
   bool isSigned() const
   {
      return !m_isUnsigned;
   }

   bool isUnsigned() const
   {
      return m_isUnsigned;
   }

   void setIsUnsigned(bool value)
   {
      m_isUnsigned = value;
   }

   void setIsSigned(bool value)
   {
      m_isUnsigned = !value;
   }

   /// toString - Append this ApSInt to the specified SmallString.
   void toString(SmallVectorImpl<char> &str, unsigned radix = 10) const
   {
      ApInt::toString(str, radix, isSigned());
   }
   /// toString - Converts an ApInt to a std::string.  This is an inefficient
   /// method; you should prefer passing in a SmallString instead.
   std::string toString(unsigned radix) const
   {
      return ApInt::toString(radix, isSigned());
   }

   using ApInt::toString;

   /// \brief Get the correctly-extended \c int64_t value.
   int64_t getExtValue() const
   {
      assert(getMinSignedBits() <= 64 && "Too many bits for int64_t");
      return isSigned() ? getSignExtValue() : getZeroExtValue();
   }

   ApSInt trunc(uint32_t width) const
   {
      return ApSInt(ApInt::trunc(width), m_isUnsigned);
   }

   ApSInt extend(uint32_t width) const
   {
      if (m_isUnsigned) {
         return ApSInt(zext(width), m_isUnsigned);
      } else {
         return ApSInt(sext(width), m_isUnsigned);
      }
   }

   ApSInt extOrTrunc(uint32_t width) const {
      if (m_isUnsigned) {
         return ApSInt(zextOrTrunc(width), m_isUnsigned);
      } else {
         return ApSInt(sextOrTrunc(width), m_isUnsigned);
      }
   }

   const ApSInt &operator%=(const ApSInt &other)
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      if (m_isUnsigned) {
         *this = urem(other);
      } else {
         *this = srem(other);
      }
      return *this;
   }

   const ApSInt &operator/=(const ApSInt &other)
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      if (m_isUnsigned)
         *this = udiv(other);
      else
         *this = sdiv(other);
      return *this;
   }

   ApSInt operator%(const ApSInt &other) const
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      return m_isUnsigned ? ApSInt(urem(other), true) : ApSInt(srem(other), false);
   }

   ApSInt operator/(const ApSInt &other) const
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      return m_isUnsigned ? ApSInt(udiv(other), true) : ApSInt(sdiv(other), false);
   }

   ApSInt operator>>(unsigned amt) const
   {
      return m_isUnsigned ? ApSInt(lshr(amt), true) : ApSInt(ashr(amt), false);
   }

   ApSInt& operator>>=(unsigned amt)
   {
      if (m_isUnsigned) {
         lshrInPlace(amt);
      } else {
         ashrInPlace(amt);
      }
      return *this;
   }

   inline bool operator<(const ApSInt& other) const
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      return m_isUnsigned ? ult(other) : slt(other);
   }

   inline bool operator>(const ApSInt& other) const
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      return m_isUnsigned ? ugt(other) : sgt(other);
   }

   inline bool operator<=(const ApSInt& other) const
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      return m_isUnsigned ? ule(other) : sle(other);
   }

   inline bool operator>=(const ApSInt& other) const
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      return m_isUnsigned ? uge(other) : sge(other);
   }

   inline bool operator==(const ApSInt& other) const
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      return eq(other);
   }

   inline bool operator!=(const ApSInt& other) const
   {
      return !((*this) == other);
   }

   bool operator==(int64_t other) const
   {
      return compareValues(*this, get(other)) == 0;
   }

   bool operator!=(int64_t other) const
   {
      return compareValues(*this, get(other)) != 0;
   }

   bool operator<=(int64_t other) const
   {
      return compareValues(*this, get(other)) <= 0;
   }

   bool operator>=(int64_t other) const
   {
      return compareValues(*this, get(other)) >= 0;
   }

   bool operator<(int64_t other) const
   {
      return compareValues(*this, get(other)) < 0;
   }

   bool operator>(int64_t other) const
   {
      return compareValues(*this, get(other)) > 0;
   }

   // The remaining operators just wrap the logic of ApInt, but retain the
   // signedness information.

   ApSInt operator<<(unsigned bits) const
   {
      return ApSInt(static_cast<const ApInt&>(*this) << bits, m_isUnsigned);
   }

   ApSInt& operator<<=(unsigned amt)
   {
      static_cast<ApInt&>(*this) <<= amt;
      return *this;
   }

   ApSInt& operator++()
   {
      ++(static_cast<ApInt&>(*this));
      return *this;
   }
   ApSInt& operator--() {
      --(static_cast<ApInt&>(*this));
      return *this;
   }

   ApSInt operator++(int)
   {
      return ApSInt(++static_cast<ApInt&>(*this), m_isUnsigned);
   }

   ApSInt operator--(int)
   {
      return ApSInt(--static_cast<ApInt&>(*this), m_isUnsigned);
   }

   ApSInt operator-() const
   {
      return ApSInt(-static_cast<const ApInt&>(*this), m_isUnsigned);
   }

   ApSInt& operator+=(const ApSInt& other)
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      static_cast<ApInt&>(*this) += other;
      return *this;
   }

   ApSInt& operator-=(const ApSInt& other)
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      static_cast<ApInt&>(*this) -= other;
      return *this;
   }

   ApSInt& operator*=(const ApSInt& other)
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      static_cast<ApInt&>(*this) *= other;
      return *this;
   }

   ApSInt& operator&=(const ApSInt& other)
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      static_cast<ApInt&>(*this) &= other;
      return *this;
   }

   ApSInt& operator|=(const ApSInt& other)
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      static_cast<ApInt&>(*this) |= other;
      return *this;
   }

   ApSInt& operator^=(const ApSInt& other)
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      static_cast<ApInt&>(*this) ^= other;
      return *this;
   }

   ApSInt operator&(const ApSInt& other) const
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      return ApSInt(static_cast<const ApInt&>(*this) & other, m_isUnsigned);
   }

   ApSInt operator|(const ApSInt& other) const
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      return ApSInt(static_cast<const ApInt&>(*this) | other, m_isUnsigned);
   }

   ApSInt operator^(const ApSInt &other) const
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      return ApSInt(static_cast<const ApInt&>(*this) ^ other, m_isUnsigned);
   }

   ApSInt operator*(const ApSInt& other) const
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      return ApSInt(static_cast<const ApInt&>(*this) * other, m_isUnsigned);
   }

   ApSInt operator+(const ApSInt& other) const
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      return ApSInt(static_cast<const ApInt&>(*this) + other, m_isUnsigned);
   }

   ApSInt operator-(const ApSInt& other) const
   {
      assert(m_isUnsigned == other.m_isUnsigned && "Signedness mismatch!");
      return ApSInt(static_cast<const ApInt&>(*this) - other, m_isUnsigned);
   }

   ApSInt operator~() const
   {
      return ApSInt(~static_cast<const ApInt&>(*this), m_isUnsigned);
   }

   /// getMaxValue - Return the ApSInt representing the maximum integer value
   ///  with the given bit width and signedness.
   static ApSInt getMaxValue(uint32_t numBits, bool isUnsigned)
   {
      return ApSInt(isUnsigned ? ApInt::getMaxValue(numBits)
                               : ApInt::getSignedMaxValue(numBits), isUnsigned);
   }

   /// getMinValue - Return the ApSInt representing the minimum integer value
   ///  with the given bit width and signedness.
   static ApSInt getMinValue(uint32_t numBits, bool isUnsigned)
   {
      return ApSInt(isUnsigned ? ApInt::getMinValue(numBits)
                               : ApInt::getSignedMinValue(numBits), isUnsigned);
   }

   /// \brief Determine if two ApSInts have the same value, zero- or
   /// sign-extending as needed.
   static bool isSameValue(const ApSInt &lhs, const ApSInt &rhs)
   {
      return !compareValues(lhs, rhs);
   }

   /// \brief Compare underlying values of two numbers.
   static int compareValues(const ApSInt &lhs, const ApSInt &rhs)
   {
      if (lhs.getBitWidth() == rhs.getBitWidth() && lhs.isSigned() == rhs.isSigned()) {
         return lhs.m_isUnsigned ? lhs.compare(rhs) : lhs.compareSigned(rhs);
      }
      // Check for a bit-width mismatch.
      if (lhs.getBitWidth() > rhs.getBitWidth()) {
         return compareValues(lhs, rhs.extend(lhs.getBitWidth()));
      }

      if (rhs.getBitWidth() > lhs.getBitWidth()) {
         return compareValues(lhs.extend(rhs.getBitWidth()), rhs);
      }

      // We have a signedness mismatch. Check for negative values and do an
      // unsigned compare if both are positive.
      if (lhs.isSigned()) {
         assert(!rhs.isSigned() && "Expected signed mismatch");
         if (lhs.isNegative()) {
            return -1;
         }
      } else {
         assert(rhs.isSigned() && "Expected signed mismatch");
         if (rhs.isNegative()) {
            return 1;
         }
      }
      return lhs.compare(rhs);
   }

   static ApSInt get(int64_t value)
   {
      return ApSInt(ApInt(64, value), false);
   }

   static ApSInt getUnsigned(uint64_t value)
   {
      return ApSInt(ApInt(64, value), true);
   }

   /// profile - Used to insert ApSInt objects, or objects that contain ApSInt
   ///  objects, into FoldingSets.
   void profile(FoldingSetNodeId& id) const;
};

inline bool operator==(int64_t lhs, const ApSInt &rhs) { return rhs == lhs; }
inline bool operator!=(int64_t lhs, const ApSInt &rhs) { return rhs != lhs; }
inline bool operator<=(int64_t lhs, const ApSInt &rhs) { return rhs >= lhs; }
inline bool operator>=(int64_t lhs, const ApSInt &rhs) { return rhs <= lhs; }
inline bool operator<(int64_t lhs, const ApSInt &rhs) { return rhs > lhs; }
inline bool operator>(int64_t lhs, const ApSInt &rhs) { return rhs < lhs; }

inline RawOutStream &operator<<(RawOutStream &outstream, const ApSInt &ivalue)
{
   ivalue.print(outstream, ivalue.isSigned());
   return outstream;
}

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_AP_SINT_H
