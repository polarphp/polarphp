// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/26.

#ifndef POLARPHP_BASIC_ADT_SMALL_BIT_VECTOR_H
#define POLARPHP_BASIC_ADT_SMALL_BIT_VECTOR_H

#include "polarphp/basic/adt/BitVector.h"
#include "polarphp/basic/adt/IteratorRange.h"
#include "polarphp/utils/MathExtras.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

namespace polar {
namespace basic {

/// This is a 'bitvector' (really, a variable-sized bit array), optimized for
/// the case when the array is small. It contains one pointer-sized field, which
/// is directly used as a plain collection of bits when possible, or as a
/// pointer to a larger heap-allocated array when necessary. This allows normal
/// "small" cases to be fast without losing generality for large inputs.
class SmallBitVector
{
   // TODO: In "large" mode, a pointer to a BitVector is used, leading to an
   // unnecessary level of indirection. It would be more efficient to use a
   // pointer to memory containing size, allocation size, and the array of bits.
   uintptr_t m_bitVector = 1;

   enum {
      // The number of bits in this class.
      NumBaseBits = sizeof(uintptr_t) * CHAR_BIT,

      // One bit is used to discriminate between small and large mode. The
      // remaining bits are used for the small-mode representation.
      SmallNumRawBits = NumBaseBits - 1,

      // A few more bits are used to store the size of the bit set in small mode.
      // Theoretically this is a ceil-log2. These bits are encoded in the most
      // significant bits of the raw bits.
      SmallNumSizeBits = (NumBaseBits == 32 ? 5 :
      NumBaseBits == 64 ? 6 :
      SmallNumRawBits),

      // The remaining bits are used to store the actual set in small mode.
      SmallNumDataBits = SmallNumRawBits - SmallNumSizeBits
   };

   static_assert(NumBaseBits == 64 || NumBaseBits == 32,
                 "Unsupported word size");

public:
   using size_type = unsigned;

   // Encapsulation of a single bit.
   class Reference
   {
      SmallBitVector &m_theVector;
      unsigned m_bitPos;

   public:
      Reference(SmallBitVector &bitVector, unsigned idx) : m_theVector(bitVector), m_bitPos(idx)
      {}

      Reference(const Reference&) = default;

      Reference& operator=(Reference value)
      {
         *this = bool(value);
         return *this;
      }

      Reference& operator=(bool value)
      {
         if (value) {
            m_theVector.set(m_bitPos);
         } else {
            m_theVector.reset(m_bitPos);
         }
         return *this;
      }

      operator bool() const
      {
         return const_cast<const SmallBitVector &>(m_theVector).operator[](m_bitPos);
      }
   };

private:
   bool isSmall() const
   {
      return m_bitVector & uintptr_t(1);
   }

   BitVector *getPointer() const
   {
      assert(!isSmall());
      return reinterpret_cast<BitVector *>(m_bitVector);
   }

   void switchToSmall(uintptr_t NewSmallBits, size_t NewSize)
   {
      m_bitVector = 1;
      setSmallSize(NewSize);
      setSmallBits(NewSmallBits);
   }

   void switchToLarge(BitVector *bitvector)
   {
      m_bitVector = reinterpret_cast<uintptr_t>(bitvector);
      assert(!isSmall() && "Tried to use an unaligned pointer");
   }

   // Return all the bits used for the "small" representation; this includes
   // bits for the size as well as the element bits.
   uintptr_t getSmallRawBits() const
   {
      assert(isSmall());
      return m_bitVector >> 1;
   }

   void setSmallRawBits(uintptr_t NewRawBits)
   {
      assert(isSmall());
      m_bitVector = (NewRawBits << 1) | uintptr_t(1);
   }

   // Return the size.
   size_t getSmallSize() const { return getSmallRawBits() >> SmallNumDataBits; }

   void setSmallSize(size_t Size) {
      setSmallRawBits(getSmallBits() | (Size << SmallNumDataBits));
   }

   // Return the element bits.
   uintptr_t getSmallBits() const
   {
      return getSmallRawBits() & ~(~uintptr_t(0) << getSmallSize());
   }

   void setSmallBits(uintptr_t NewBits)
   {
      setSmallRawBits((NewBits & ~(~uintptr_t(0) << getSmallSize())) |
                      (getSmallSize() << SmallNumDataBits));
   }

public:
   /// Creates an empty bitvector.
   SmallBitVector() = default;

   /// Creates a bitvector of specified number of bits. All bits are initialized
   /// to the specified value.
   explicit SmallBitVector(unsigned s, bool t = false)
   {
      if (s <= SmallNumDataBits)
         switchToSmall(t ? ~uintptr_t(0) : 0, s);
      else
         switchToLarge(new BitVector(s, t));
   }

   /// SmallBitVector copy ctor.
   SmallBitVector(const SmallBitVector &other)
   {
      if (other.isSmall()) {
         m_bitVector = other.m_bitVector;
      } else {
         switchToLarge(new BitVector(*other.getPointer()));
      }
   }

   SmallBitVector(SmallBitVector &&other) : m_bitVector(other.m_bitVector)
   {
      other.m_bitVector = 1;
   }

   ~SmallBitVector()
   {
      if (!isSmall())
         delete getPointer();
   }

   using const_setBits_iterator = ConstSetBitsIteratorImpl<SmallBitVector>;
   using set_iterator = const_setBits_iterator;

   const_setBits_iterator setBitsBegin() const
   {
      return const_setBits_iterator(*this);
   }

   const_setBits_iterator setBitsEnd() const
   {
      return const_setBits_iterator(*this, -1);
   }

   IteratorRange<const_setBits_iterator> setBits() const
   {
      return make_range(setBitsBegin(), setBitsEnd());
   }

   /// Tests whether there are no bits in this bitvector.
   bool empty() const
   {
      return isSmall() ? getSmallSize() == 0 : getPointer()->empty();
   }

   /// Returns the number of bits in this bitvector.
   size_t size() const
   {
      return isSmall() ? getSmallSize() : getPointer()->size();
   }

   /// Returns the number of bits which are set.
   size_type count() const
   {
      if (isSmall()) {
         uintptr_t bits = getSmallBits();
         return count_population(bits);
      }
      return getPointer()->count();
   }

   /// Returns true if any bit is set.
   bool any() const
   {
      if (isSmall()) {
         return getSmallBits() != 0;
      }
      return getPointer()->any();
   }

   /// Returns true if all bits are set.
   bool all() const
   {
      if (isSmall()) {
         return getSmallBits() == (uintptr_t(1) << getSmallSize()) - 1;
      }
      return getPointer()->all();
   }

   /// Returns true if none of the bits are set.
   bool none() const
   {
      if (isSmall()) {
         return getSmallBits() == 0;
      }
      return getPointer()->none();
   }

   /// Returns the index of the first set bit, -1 if none of the bits are set.
   int findFirst() const
   {
      if (isSmall()) {
         uintptr_t bits = getSmallBits();
         if (bits == 0) {
            return -1;
         }
         return count_trailing_zeros(bits);
      }
      return getPointer()->findFirst();
   }

   int findLast() const
   {
      if (isSmall()) {
         uintptr_t bits = getSmallBits();
         if (bits == 0) {
            return -1;
         }
         return NumBaseBits - count_leading_zeros(bits);
      }
      return getPointer()->findLast();
   }

   /// Returns the index of the first unset bit, -1 if all of the bits are set.
   int findFirstUnset() const
   {
      if (isSmall()) {
         if (count() == getSmallSize()) {
            return -1;
         }
         uintptr_t bits = getSmallBits();
         return count_trailing_ones(bits);
      }
      return getPointer()->findFirstUnset();
   }

   int findLastUnset() const
   {
      if (isSmall()) {
         if (count() == getSmallSize()) {
            return -1;
         }
         uintptr_t bits = getSmallBits();
         return NumBaseBits - count_leading_ones(bits);
      }
      return getPointer()->findLastUnset();
   }

   /// Returns the index of the next set bit following the "prev" bit.
   /// Returns -1 if the next set bit is not found.
   int findNext(unsigned prev) const
   {
      if (isSmall()) {
         uintptr_t bits = getSmallBits();
         // mask off previous bits.
         bits &= ~uintptr_t(0) << (prev + 1);
         if (bits == 0 || prev + 1 >= getSmallSize()) {
            return -1;
         }
         return count_trailing_zeros(bits);
      }
      return getPointer()->findNext(prev);
   }

   /// Returns the index of the next unset bit following the "prev" bit.
   /// Returns -1 if the next unset bit is not found.
   int findNextUnset(unsigned prev) const
   {
      if (isSmall()) {
         ++prev;
         uintptr_t bits = getSmallBits();
         // mask in previous bits.
         uintptr_t mask = (1 << prev) - 1;
         bits |= mask;
         if (bits == ~uintptr_t(0) || prev + 1 >= getSmallSize()) {
            return -1;
         }
         return count_trailing_ones(bits);
      }
      return getPointer()->findNextUnset(prev);
   }

   /// findPrev - Returns the index of the first set bit that precedes the
   /// the bit at \p priorTo.  Returns -1 if all previous bits are unset.
   int findPrev(unsigned priorTo) const
   {
      if (isSmall()) {
         if (priorTo == 0) {
            return -1;
         }
         --priorTo;
         uintptr_t bits = getSmallBits();
         bits &= mask_trailing_ones<uintptr_t>(priorTo + 1);
         if (bits == 0) {
            return -1;
         }
         return NumBaseBits - count_leading_zeros(bits) - 1;
      }
      return getPointer()->findPrev(priorTo);
   }

   /// Clear all bits.
   void clear()
   {
      if (!isSmall()) {
         delete getPointer();
      }
      switchToSmall(0, 0);
   }

   /// Grow or shrink the bitvector.
   void resize(unsigned N, bool t = false)
   {
      if (!isSmall()) {
         getPointer()->resize(N, t);
      } else if (SmallNumDataBits >= N) {
         uintptr_t NewBits = t ? ~uintptr_t(0) << getSmallSize() : 0;
         setSmallSize(N);
         setSmallBits(NewBits | getSmallBits());
      } else {
         BitVector *bitvector = new BitVector(N, t);
         uintptr_t OldBits = getSmallBits();
         for (size_t i = 0, e = getSmallSize(); i != e; ++i)
            (*bitvector)[i] = (OldBits >> i) & 1;
         switchToLarge(bitvector);
      }
   }

   void reserve(unsigned N)
   {
      if (isSmall()) {
         if (N > SmallNumDataBits) {
            uintptr_t OldBits = getSmallRawBits();
            size_t SmallSize = getSmallSize();
            BitVector *bitvector = new BitVector(SmallSize);
            for (size_t i = 0; i < SmallSize; ++i) {
               if ((OldBits >> i) & 1) {
                  bitvector->set(i);
               }
            }
            bitvector->reserve(N);
            switchToLarge(bitvector);
         }
      } else {
         getPointer()->reserve(N);
      }
   }

   // Set, reset, flip
   SmallBitVector &set()
   {
      if (isSmall()) {
         setSmallBits(~uintptr_t(0));
      } else {
         getPointer()->set();
      }
      return *this;
   }

   SmallBitVector &set(unsigned idx)
   {
      if (isSmall()) {
         assert(idx <= static_cast<unsigned>(
                   std::numeric_limits<uintptr_t>::digits) &&
                "undefined behavior");
         setSmallBits(getSmallBits() | (uintptr_t(1) << idx));
      } else {
         getPointer()->set(idx);
      }

      return *this;
   }

   /// Efficiently set a range of bits in [start, end)
   SmallBitVector &set(unsigned start, unsigned end) {
      assert(start <= end && "Attempted to set backwards range!");
      assert(end <= size() && "Attempted to set out-of-bounds range!");
      if (start == end) return *this;
      if (isSmall()) {
         uintptr_t EMask = ((uintptr_t)1) << end;
         uintptr_t IMask = ((uintptr_t)1) << start;
         uintptr_t mask = EMask - IMask;
         setSmallBits(getSmallBits() | mask);
      } else
         getPointer()->set(start, end);
      return *this;
   }

   SmallBitVector &reset()
   {
      if (isSmall())
         setSmallBits(0);
      else
         getPointer()->reset();
      return *this;
   }

   SmallBitVector &reset(unsigned idx)
   {
      if (isSmall()) {
         setSmallBits(getSmallBits() & ~(uintptr_t(1) << idx));
      } else {
         getPointer()->reset(idx);
      }
      return *this;
   }

   /// Efficiently reset a range of bits in [start, end)
   SmallBitVector &reset(unsigned start, unsigned end)
   {
      assert(start <= end && "Attempted to reset backwards range!");
      assert(end <= size() && "Attempted to reset out-of-bounds range!");
      if (start == end) return *this;
      if (isSmall()) {
         uintptr_t EMask = ((uintptr_t)1) << end;
         uintptr_t IMask = ((uintptr_t)1) << start;
         uintptr_t mask = EMask - IMask;
         setSmallBits(getSmallBits() & ~mask);
      } else {
         getPointer()->reset(start, end);
      }
      return *this;
   }

   SmallBitVector &flip()
   {
      if (isSmall()) {
         setSmallBits(~getSmallBits());
      } else {
         getPointer()->flip();
      }
      return *this;
   }

   SmallBitVector &flip(unsigned idx)
   {
      if (isSmall()) {
         setSmallBits(getSmallBits() ^ (uintptr_t(1) << idx));
      } else {
         getPointer()->flip(idx);
      }
      return *this;
   }

   // No argument flip.
   SmallBitVector operator~() const
   {
      return SmallBitVector(*this).flip();
   }

   // Indexing.
   Reference operator[](unsigned idx)
   {
      assert(idx < size() && "Out-of-bounds Bit access.");
      return Reference(*this, idx);
   }

   bool operator[](unsigned idx) const
   {
      assert(idx < size() && "Out-of-bounds Bit access.");
      if (isSmall()) {
         return ((getSmallBits() >> idx) & 1) != 0;
      }

      return getPointer()->operator[](idx);
   }

   bool test(unsigned idx) const
   {
      return (*this)[idx];
   }


   // Push single bit to end of vector.
   void push_back(bool value)
   {
      resize(size() + 1, value);
   }

   // Push single bit to end of vector.
   void pushBack(bool value)
   {
      resize(size() + 1, value);
   }

   /// Test if any common bits are set.
   bool anyCommon(const SmallBitVector &other) const
   {
      if (isSmall() && other.isSmall()) {
         return (getSmallBits() & other.getSmallBits()) != 0;
      }
      if (!isSmall() && !other.isSmall()) {
         return getPointer()->anyCommon(*other.getPointer());
      }
      for (unsigned i = 0, e = std::min(size(), other.size()); i != e; ++i) {
         if (test(i) && other.test(i)) {
            return true;
         }
      }
      return false;
   }

   // Comparison operators.
   bool operator==(const SmallBitVector &other) const
   {
      if (size() != other.size()) {
         return false;
      }

      if (isSmall()) {
         return getSmallBits() == other.getSmallBits();
      } else {
         return *getPointer() == *other.getPointer();
      }
   }

   bool operator!=(const SmallBitVector &other) const
   {
      return !(*this == other);
   }

   // Intersection, union, disjoint union.
   SmallBitVector &operator&=(const SmallBitVector &other)
   {
      resize(std::max(size(), other.size()));
      if (isSmall()) {
         setSmallBits(getSmallBits() & other.getSmallBits());
      } else if (!other.isSmall()) {
         getPointer()->operator&=(*other.getPointer());
      } else {
         SmallBitVector copy = other;
         copy.resize(size());
         getPointer()->operator&=(*copy.getPointer());
      }
      return *this;
   }

   /// Reset bits that are set in other. Same as *this &= ~other.
   SmallBitVector &reset(const SmallBitVector &other)
   {
      if (isSmall() && other.isSmall()) {
         setSmallBits(getSmallBits() & ~other.getSmallBits());
      } else if (!isSmall() && !other.isSmall()) {
         getPointer()->reset(*other.getPointer());
      } else {
         for (unsigned i = 0, e = std::min(size(), other.size()); i != e; ++i) {
            if (other.test(i)) {
               reset(i);
            }
         }
      }

      return *this;
   }

   /// Check if (This - other) is zero. This is the same as reset(other) and any().
   bool test(const SmallBitVector &other) const
   {
      if (isSmall() && other.isSmall()) {
         return (getSmallBits() & ~other.getSmallBits()) != 0;
      }
      if (!isSmall() && !other.isSmall()) {
         return getPointer()->test(*other.getPointer());
      }

      unsigned i, e;
      for (i = 0, e = std::min(size(), other.size()); i != e; ++i) {
         if (test(i) && !other.test(i)) {
            return true;
         }
      }
      for (e = size(); i != e; ++i) {
         if (test(i)) {
            return true;
         }
      }
      return false;
   }

   SmallBitVector &operator|=(const SmallBitVector &other)
   {
      resize(std::max(size(), other.size()));
      if (isSmall()) {
         setSmallBits(getSmallBits() | other.getSmallBits());
      } else if (!other.isSmall()) {
         getPointer()->operator|=(*other.getPointer());
      } else {
         SmallBitVector copy = other;
         copy.resize(size());
         getPointer()->operator|=(*copy.getPointer());
      }
      return *this;
   }

   SmallBitVector &operator^=(const SmallBitVector &other)
   {
      resize(std::max(size(), other.size()));
      if (isSmall())
         setSmallBits(getSmallBits() ^ other.getSmallBits());
      else if (!other.isSmall())
         getPointer()->operator^=(*other.getPointer());
      else {
         SmallBitVector copy = other;
         copy.resize(size());
         getPointer()->operator^=(*copy.getPointer());
      }
      return *this;
   }

   SmallBitVector &operator<<=(unsigned N)
   {
      if (isSmall()) {
         setSmallBits(getSmallBits() << N);
      } else {
         getPointer()->operator<<=(N);
      }
      return *this;
   }

   SmallBitVector &operator>>=(unsigned N)
   {
      if (isSmall()) {
         setSmallBits(getSmallBits() >> N);
      } else {
         getPointer()->operator>>=(N);
      }
      return *this;
   }

   // Assignment operator.
   const SmallBitVector &operator=(const SmallBitVector &other)
   {
      if (isSmall()) {
         if (other.isSmall()) {
            m_bitVector = other.m_bitVector;
         } else {
            switchToLarge(new BitVector(*other.getPointer()));
         }
      } else {
         if (!other.isSmall()) {
            *getPointer() = *other.getPointer();
         } else {
            delete getPointer();
            m_bitVector = other.m_bitVector;
         }
      }
      return *this;
   }

   const SmallBitVector &operator=(SmallBitVector &&other)
   {
      if (this != &other) {
         clear();
         swap(other);
      }
      return *this;
   }

   void swap(SmallBitVector &other)
   {
      std::swap(m_bitVector, other.m_bitVector);
   }

   /// Add '1' bits from mask to this vector. Don't resize.
   /// This computes "*this |= mask".
   void setBitsInMask(const uint32_t *mask, unsigned maskWords = ~0u)
   {
      if (isSmall()) {
         applyMask<true, false>(mask, maskWords);
      } else {
         getPointer()->setBitsInMask(mask, maskWords);
      }
   }

   /// Clear any bits in this vector that are set in mask. Don't resize.
   /// This computes "*this &= ~mask".
   void clearBitsInMask(const uint32_t *mask, unsigned maskWords = ~0u)
   {
      if (isSmall()) {
         applyMask<false, false>(mask, maskWords);
      } else {
         getPointer()->clearBitsInMask(mask, maskWords);
      }
   }

   /// Add a bit to this vector for every '0' bit in mask. Don't resize.
   /// This computes "*this |= ~mask".
   void setBitsNotInMask(const uint32_t *mask, unsigned maskWords = ~0u)
   {
      if (isSmall()) {
         applyMask<true, true>(mask, maskWords);
      } else {
         getPointer()->setBitsNotInMask(mask, maskWords);
      }
   }

   /// Clear a bit in this vector for every '0' bit in mask. Don't resize.
   /// This computes "*this &= mask".
   void clearBitsNotInMask(const uint32_t *mask, unsigned maskWords = ~0u)
   {
      if (isSmall()) {
         applyMask<false, true>(mask, maskWords);
      } else {
         getPointer()->clearBitsNotInMask(mask, maskWords);
      }
   }

private:
   template <bool AddBits, bool InvertMask>
   void applyMask(const uint32_t *mask, unsigned maskWords)
   {
      assert(maskWords <= sizeof(uintptr_t) && "mask is larger than base!");
      uintptr_t M = mask[0];
      if (NumBaseBits == 64) {
         M |= uint64_t(mask[1]) << 32;
      }
      if (InvertMask) {
         M = ~M;
      }
      if (AddBits) {
         setSmallBits(getSmallBits() | M);
      } else {
         setSmallBits(getSmallBits() & ~M);
      }
   }
};

inline SmallBitVector
operator&(const SmallBitVector &lhs, const SmallBitVector &other)
{
   SmallBitVector result(lhs);
   result &= other;
   return result;
}

inline SmallBitVector
operator|(const SmallBitVector &lhs, const SmallBitVector &other)
{
   SmallBitVector result(lhs);
   result |= other;
   return result;
}

inline SmallBitVector
operator^(const SmallBitVector &lhs, const SmallBitVector &other)
{
   SmallBitVector result(lhs);
   result ^= other;
   return result;
}

} // basic
} // polar

namespace std {

/// Implement std::swap in terms of BitVector swap.
inline void
swap(polar::basic::SmallBitVector &lhs, polar::basic::SmallBitVector &rhs)
{
   lhs.swap(rhs);
}

} // end namespace std

#endif // POLARPHP_BASIC_ADT_SMALL_BIT_VECTOR_H
