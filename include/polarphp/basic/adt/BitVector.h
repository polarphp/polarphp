// This source file is part of the polarphp.org open source project
//
// copyright (c) 2017 - 2018 polarphp software foundation
// copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/22.

#ifndef POLARPHP_BASIC_ADT_BIT_VECTOR_H
#define POLARPHP_BASIC_ADT_BIT_VECTOR_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/IteratorRange.h"
#include "polarphp/utils/MathExtras.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <utility>

namespace polar {
namespace basic {

using polar::utils::count_population;
using polar::utils::mask_trailing_ones;
using polar::utils::mask_trailing_zeros;
using polar::utils::mask_leading_ones;
using polar::utils::mask_leading_zeros;
using polar::utils::count_leading_ones;
using polar::utils::count_leading_zeros;
using polar::utils::count_trailing_ones;
using polar::utils::count_trailing_zeros;
using polar::utils::align_to;

/// ForwardIterator for the bits that are set.
/// Iterators get invalidated when resize / reserve is called.
template <typename BitVectorType>
class ConstSetBitsIteratorImpl
{
   const BitVectorType &m_parent;
   int m_current = 0;

   void advance()
   {
      assert(m_current != -1 && "Trying to advance past end.");
      m_current = m_parent.findNext(m_current);
   }

public:
   ConstSetBitsIteratorImpl(const BitVectorType &parent, int current)
      : m_parent(parent), m_current(current)
   {}

   explicit ConstSetBitsIteratorImpl(const BitVectorType &parent)
      : ConstSetBitsIteratorImpl(parent, parent.findFirst())
   {}

   ConstSetBitsIteratorImpl(const ConstSetBitsIteratorImpl &) = default;

   ConstSetBitsIteratorImpl operator++(int)
   {
      auto prev = *this;
      advance();
      return prev;
   }

   ConstSetBitsIteratorImpl &operator++()
   {
      advance();
      return *this;
   }

   unsigned operator*() const
   {
      return m_current;
   }

   bool operator==(const ConstSetBitsIteratorImpl &other) const
   {
      assert(&m_parent == &other.m_parent &&
             "Comparing iterators from different BitVectors");
      return m_current == other.m_current;
   }

   bool operator!=(const ConstSetBitsIteratorImpl &other) const
   {
      assert(&m_parent == &other.m_parent &&
             "Comparing iterators from different BitVectors");
      return m_current != other.m_current;
   }
};

class BitVector
{
   typedef unsigned long BitWord;
   enum { BITWORD_SIZE = (unsigned)sizeof(BitWord) * CHAR_BIT };

   static_assert(BITWORD_SIZE == 64 || BITWORD_SIZE == 32,
                 "Unsupported word size");

   MutableArrayRef<BitWord> m_bits; // Actual bits.
   unsigned m_size;                 // m_size of bitvector in bits.

public:
   typedef unsigned size_type;
   // Encapsulation of a single bit.
   class Reference
   {
      friend class BitVector;

      BitWord *m_wordRef;
      unsigned m_bitPos;

   public:
      Reference(BitVector &vector, unsigned idx)
      {
         m_wordRef = &vector.m_bits[idx / BITWORD_SIZE];
         m_bitPos = idx % BITWORD_SIZE;
      }

      Reference() = delete;
      Reference(const Reference&) = default;

      Reference &operator=(Reference other) {
         *this = bool(other);
         return *this;
      }

      Reference& operator=(bool other)
      {
         if (other) {
            *m_wordRef |= BitWord(1) << m_bitPos;
         }  else{
            *m_wordRef &= ~(BitWord(1) << m_bitPos);
         }
         return *this;
      }

      operator bool() const
      {
         return ((*m_wordRef) & (BitWord(1) << m_bitPos)) != 0;
      }
   };

   typedef ConstSetBitsIteratorImpl<BitVector> ConstSetBitsIterator;
   typedef ConstSetBitsIterator set_iterator;

   ConstSetBitsIterator setBitsBegin() const
   {
      return ConstSetBitsIterator(*this);
   }

   ConstSetBitsIterator setBitsEnd() const
   {
      return ConstSetBitsIterator(*this, -1);
   }

   IteratorRange<ConstSetBitsIterator> setBits() const
   {
      return make_range(setBitsBegin(), setBitsEnd());
   }

   /// BitVector default ctor - Creates an empty bitvector.
   BitVector()
      : m_size(0)
   {}

   /// BitVector ctor - Creates a bitvector of specified number of bits. All
   /// bits are initialized to the specified value.
   explicit BitVector(unsigned size, bool flag = false)
      : m_size(size)
   {
      size_t capacity = numBitWords(size);
      m_bits = allocate(capacity);
      initWords(m_bits, flag);
      if (flag) {
         clearUnusedBits();
      }
   }

   /// BitVector copy ctor.
   BitVector(const BitVector &rhs)
      : m_size(rhs.size())
   {
      if (m_size == 0) {
         m_bits = MutableArrayRef<BitWord>();
         return;
      }

      size_t capacity = numBitWords(rhs.size());
      m_bits = allocate(capacity);
      std::memcpy(m_bits.getData(), rhs.m_bits.getData(), capacity * sizeof(BitWord));
   }

   BitVector(BitVector &&rhs) : m_bits(rhs.m_bits), m_size(rhs.m_size)
   {
      rhs.m_bits = MutableArrayRef<BitWord>();
      rhs.m_size = 0;
   }

   ~BitVector()
   {
      std::free(m_bits.getData());
   }

   /// empty - Tests whether there are no bits in this bitvector.
   bool empty() const
   {
      return m_size == 0;
   }

   /// size - Returns the number of bits in this bitvector.
   size_type size() const
   {
      return m_size;
   }

   size_type getSize() const
   {
      return size();
   }

   /// count - Returns the number of bits which are set.
   size_type count() const
   {
      unsigned numBits = 0;
      for (unsigned i = 0; i < numBitWords(size()); ++i) {
         numBits += count_population(m_bits[i]);
      }
      return numBits;
   }

   /// any - Returns true if any bit is set.
   bool any() const
   {
      for (unsigned i = 0; i < numBitWords(size()); ++i) {
         if (m_bits[i] != 0) {
            return true;
         }
      }
      return false;
   }

   /// all - Returns true if all bits are set.
   bool all() const
   {
      for (unsigned i = 0; i < m_size / BITWORD_SIZE; ++i) {
         if (m_bits[i] != ~0UL) {
            return false;
         }
      }
      // If bits remain check that they are ones. The unused bits are always zero.
      if (unsigned remainder = m_size % BITWORD_SIZE) {
         return m_bits[m_size / BITWORD_SIZE] == (1UL << remainder) - 1;
      }
      return true;
   }

   /// none - Returns true if none of the bits are set.
   bool none() const
   {
      return !any();
   }

   /// findFirstIn - Returns the index of the first set bit in the range
   /// [begin, end).  Returns -1 if all bits in the range are unset.
   int findFirstIn(unsigned begin, unsigned end) const
   {
      assert(begin <= end && end <= m_size);
      if (begin == end) {
         return -1;
      }

      unsigned firstWord = begin / BITWORD_SIZE;
      unsigned lastWord = (end - 1) / BITWORD_SIZE;

      // Check subsequent words.
      for (unsigned i = firstWord; i <= lastWord; ++i) {
         BitWord copy = m_bits[i];
         if (i == firstWord) {
            unsigned firstBit = begin % BITWORD_SIZE;
            copy &= mask_trailing_zeros<BitWord>(firstBit);
         }
         if (i == lastWord) {
            unsigned lastBit = (end - 1) % BITWORD_SIZE;
            copy &= mask_trailing_ones<BitWord>(lastBit + 1);
         }
         if (copy != 0) {
            return i * BITWORD_SIZE + count_trailing_zeros(copy);
         }
      }
      return -1;
   }

   /// findLastIn - Returns the index of the last set bit in the range
   /// [begin, end).  Returns -1 if all bits in the range are unset.
   int findLastIn(unsigned begin, unsigned end) const
   {
      assert(begin <= end && end <= m_size);
      if (begin == end) {
         return -1;
      }

      unsigned lastWord = (end - 1) / BITWORD_SIZE;
      unsigned firstWord = begin / BITWORD_SIZE;

      for (unsigned i = lastWord + 1; i >= firstWord + 1; --i) {
         unsigned currentWord = i - 1;

         BitWord copy = m_bits[currentWord];
         if (currentWord == lastWord) {
            unsigned lastBit = (end - 1) % BITWORD_SIZE;
            copy &= mask_trailing_ones<BitWord>(lastBit + 1);
         }

         if (currentWord == firstWord) {
            unsigned firstBit = begin % BITWORD_SIZE;
            copy &= mask_trailing_zeros<BitWord>(firstBit);
         }

         if (copy != 0) {
            return (currentWord + 1) * BITWORD_SIZE - count_leading_zeros(copy) - 1;
         }
      }

      return -1;
   }

   /// findFirstUnsetIn - Returns the index of the first unset bit in the
   /// range [begin, end).  Returns -1 if all bits in the range are set.
   int findFirstUnsetIn(unsigned begin, unsigned end) const
   {
      assert(begin <= end && end <= m_size);
      if (begin == end) {
         return -1;
      }
      unsigned firstWord = begin / BITWORD_SIZE;
      unsigned lastWord = (end - 1) / BITWORD_SIZE;
      // Check subsequent words.
      for (unsigned i = firstWord; i <= lastWord; ++i) {
         BitWord copy = m_bits[i];
         if (i == firstWord) {
            unsigned firstBit = begin % BITWORD_SIZE;
            copy |= mask_trailing_ones<BitWord>(firstBit);
         }

         if (i == lastWord) {
            unsigned lastBit = (end - 1) % BITWORD_SIZE;
            copy |= mask_trailing_zeros<BitWord>(lastBit + 1);
         }
         if (copy != ~0UL) {
            unsigned result = i * BITWORD_SIZE + count_trailing_ones(copy);
            return result < size() ? result : -1;
         }
      }
      return -1;
   }

   /// findLastUnsetIn - Returns the index of the last unset bit in the
   /// range [begin, end).  Returns -1 if all bits in the range are set.
   int findLastUnsetIn(unsigned begin, unsigned end) const
   {
      assert(begin <= end && end <= m_size);
      if (begin == end) {
         return -1;
      }
      unsigned lastWord = (end - 1) / BITWORD_SIZE;
      unsigned firstWord = begin / BITWORD_SIZE;
      for (unsigned i = lastWord + 1; i >= firstWord + 1; --i) {
         unsigned currentWord = i - 1;
         BitWord copy = m_bits[currentWord];
         if (currentWord == lastWord) {
            unsigned lastBit = (end - 1) % BITWORD_SIZE;
            copy |= mask_trailing_zeros<BitWord>(lastBit + 1);
         }
         if (currentWord == firstWord) {
            unsigned firstBit = begin % BITWORD_SIZE;
            copy |= mask_trailing_ones<BitWord>(firstBit);
         }

         if (copy != ~0UL) {
            unsigned result =
                  (currentWord + 1) * BITWORD_SIZE - count_leading_ones(copy) - 1;
            return result < m_size ? result : -1;
         }
      }
      return -1;
   }

   /// findFirst - Returns the index of the first set bit, -1 if none
   /// of the bits are set.
   int findFirst() const
   {
      return findFirstIn(0, m_size);
   }

   /// findLast - Returns the index of the last set bit, -1 if none of the bits
   /// are set.
   int findLast() const
   {
      return findLastIn(0, m_size);
   }

   /// findNext - Returns the index of the next set bit following the
   /// "Prev" bit. Returns -1 if the next set bit is not found.
   int findNext(unsigned prev) const
   {
      return findFirstIn(prev + 1, m_size);
   }

   /// findPrev - Returns the index of the first set bit that precedes the
   /// the bit at \p PriorTo.  Returns -1 if all previous bits are unset.
   int findPrev(unsigned priorTo) const
   {
      return findLastIn(0, priorTo);
   }

   /// findFirstUnset - Returns the index of the first unset bit, -1 if all
   /// of the bits are set.
   int findFirstUnset() const
   {
      return findFirstUnsetIn(0, m_size);
   }

   /// findNextUnset - Returns the index of the next unset bit following the
   /// "Prev" bit.  Returns -1 if all remaining bits are set.
   int findNextUnset(unsigned prev) const
   {
      return findFirstUnsetIn(prev + 1, m_size);
   }

   /// findLastUnset - Returns the index of the last unset bit, -1 if all of
   /// the bits are set.
   int findLastUnset() const
   {
      return findLastUnsetIn(0, m_size);
   }

   /// findPrevUnset - Returns the index of the first unset bit that precedes
   /// the bit at \p PriorTo.  Returns -1 if all previous bits are set.
   int findPrevUnset(unsigned priorTo)
   {
      return findLastUnsetIn(0, priorTo);
   }

   /// clear - Removes all bits from the bitvector. Does not change capacity.
   void clear()
   {
      m_size = 0;
   }

   /// resize - Grow or shrink the bitvector.
   void resize(unsigned size, bool flag = false)
   {
      if (size > getBitCapacity()) {
         unsigned oldcapacity = m_bits.getSize();
         grow(size);
         initWords(m_bits.dropFront(oldcapacity), flag);
      }

      // Set any old unused bits that are now included in the BitVector. This
      // may set bits that are not included in the new vector, but we will clear
      // them back out below.
      if (size > m_size) {
         setUnusedBits(flag);
      }
      // Update the size, and clear out any bits that are now unused
      unsigned oldSize = m_size;
      m_size = size;
      if (flag || size < oldSize) {
         clearUnusedBits();
      }
   }

   void reserve(unsigned size)
   {
      if (size > getBitCapacity()) {
         grow(size);
      }
   }

   // Set, reset, flip
   BitVector &set()
   {
      initWords(m_bits, true);
      clearUnusedBits();
      return *this;
   }

   BitVector &set(unsigned idx)
   {
      assert(m_bits.getData() && "Bits never allocated");
      m_bits[idx / BITWORD_SIZE] |= BitWord(1) << (idx % BITWORD_SIZE);
      return *this;
   }

   /// set - Efficiently set a range of bits in [I, E)
   BitVector &set(unsigned iter, unsigned end) {
      assert(iter <= end && "Attempted to set backwards range!");
      assert(end <= size() && "Attempted to set out-of-bounds range!");

      if (iter == end) {
         return *this;
      }

      if (iter / BITWORD_SIZE == end / BITWORD_SIZE) {
         BitWord emask = 1UL << (end % BITWORD_SIZE);
         BitWord imask = 1UL << (iter % BITWORD_SIZE);
         BitWord mask = emask - imask;
         m_bits[iter / BITWORD_SIZE] |= mask;
         return *this;
      }

      BitWord prefixMask = ~0UL << (iter % BITWORD_SIZE);
      m_bits[iter / BITWORD_SIZE] |= prefixMask;
      iter = align_to(iter, BITWORD_SIZE);

      for (; iter + BITWORD_SIZE <= end; iter += BITWORD_SIZE) {
         m_bits[iter / BITWORD_SIZE] = ~0UL;
      }
      BitWord postfixMask = (1UL << (end % BITWORD_SIZE)) - 1;
      if (iter < end) {
         m_bits[iter / BITWORD_SIZE] |= postfixMask;
      }
      return *this;
   }

   BitVector &reset()
   {
      initWords(m_bits, false);
      return *this;
   }

   BitVector &reset(unsigned idx)
   {
      m_bits[idx / BITWORD_SIZE] &= ~(BitWord(1) << (idx % BITWORD_SIZE));
      return *this;
   }

   /// reset - Efficiently reset a range of bits in [I, E)
   BitVector &reset(unsigned iter, unsigned end)
   {
      assert(iter <= end && "Attempted to reset backwards range!");
      assert(end <= size() && "Attempted to reset out-of-bounds range!");

      if (iter == end) {
         return *this;
      }
      if (iter / BITWORD_SIZE == end / BITWORD_SIZE) {
         BitWord emask = 1UL << (end % BITWORD_SIZE);
         BitWord imask = 1UL << (iter % BITWORD_SIZE);
         BitWord mask = emask - imask;
         m_bits[iter / BITWORD_SIZE] &= ~mask;
         return *this;
      }

      BitWord prefixMask = ~0UL << (iter % BITWORD_SIZE);
      m_bits[iter / BITWORD_SIZE] &= ~prefixMask;
      iter = align_to(iter, BITWORD_SIZE);

      for (; iter + BITWORD_SIZE <= end; iter += BITWORD_SIZE) {
         m_bits[iter / BITWORD_SIZE] = 0UL;
      }

      BitWord postfixMask = (1UL << (end % BITWORD_SIZE)) - 1;
      if (iter < end) {
         m_bits[iter / BITWORD_SIZE] &= ~postfixMask;
      }
      return *this;
   }

   BitVector &flip()
   {
      for (unsigned i = 0; i < numBitWords(size()); ++i) {
         m_bits[i] = ~m_bits[i];
      }
      clearUnusedBits();
      return *this;
   }

   BitVector &flip(unsigned idx)
   {
      m_bits[idx / BITWORD_SIZE] ^= BitWord(1) << (idx % BITWORD_SIZE);
      return *this;
   }

   // Indexing.
   Reference operator[](unsigned idx)
   {
      assert (idx < m_size && "Out-of-bounds Bit access.");
      return Reference(*this, idx);
   }

   bool operator[](unsigned idx) const
   {
      assert (idx < m_size && "Out-of-bounds Bit access.");
      BitWord mask = BitWord(1) << (idx % BITWORD_SIZE);
      return (m_bits[idx / BITWORD_SIZE] & mask) != 0;
   }

   bool test(unsigned idx) const
   {
      return (*this)[idx];
   }

   // Push single bit to end of vector.
   void pushBack(bool value)
   {
      unsigned oldSize = m_size;
      unsigned newSize = m_size + 1;

      // Resize, which will insert zeros.
      // If we already fit then the unused bits will be already zero.
      if (newSize > getBitCapacity()) {
         resize(newSize, false);
      } else {
         m_size = newSize;
      }

      // If true, set single bit.
      if (value) {
          set(oldSize);
      }
   }

   inline void push_back(bool value)
   {
      pushBack(value);
   }

   /// Test if any common bits are set.
   bool anyCommon(const BitVector &rhs) const
   {
      unsigned thisWords = numBitWords(size());
      unsigned rhsWords  = numBitWords(rhs.size());
      for (unsigned i = 0, e = std::min(thisWords, rhsWords); i != e; ++i) {
         if (m_bits[i] & rhs.m_bits[i]) {
            return true;
         }
      }
      return false;
   }

   // Comparison operators.
   bool operator==(const BitVector &rhs) const
   {
      unsigned thisWords = numBitWords(size());
      unsigned rhsWords  = numBitWords(rhs.size());
      unsigned i;
      for (i = 0; i != std::min(thisWords, rhsWords); ++i) {
         if (m_bits[i] != rhs.m_bits[i]) {
            return false;
         }
      }

      // Verify that any extra words are all zeros.
      if (i != thisWords) {
         for (; i != thisWords; ++i) {
            if (m_bits[i]) {
               return false;
            }
         }

      } else if (i != rhsWords) {
         for (; i != rhsWords; ++i) {
            if (rhs.m_bits[i]) {
               return false;
            }
         }
      }
      return true;
   }

   bool operator!=(const BitVector &rhs) const
   {
      return !(*this == rhs);
   }

   /// Intersection, union, disjoint union.
   BitVector &operator&=(const BitVector &rhs)
   {
      unsigned thisWords = numBitWords(size());
      unsigned rhsWords  = numBitWords(rhs.size());
      unsigned i;
      for (i = 0; i != std::min(thisWords, rhsWords); ++i) {
         m_bits[i] &= rhs.m_bits[i];
      }
      // Any bits that are just in this bitvector become zero, because they aren't
      // in the rhs bit vector.  Any words only in rhs are ignored because they
      // are already zero in the LHS.
      for (; i != thisWords; ++i) {
         m_bits[i] = 0;
      }
      return *this;
   }

   /// reset - Reset bits that are set in rhs. Same as *this &= ~rhs.
   BitVector &reset(const BitVector &rhs)
   {
      unsigned thisWords = numBitWords(size());
      unsigned rhsWords  = numBitWords(rhs.size());
      unsigned i;
      for (i = 0; i != std::min(thisWords, rhsWords); ++i) {
         m_bits[i] &= ~rhs.m_bits[i];
      }
      return *this;
   }

   /// test - Check if (This - rhs) is zero.
   /// This is the same as reset(rhs) and any().
   bool test(const BitVector &rhs) const
   {
      unsigned thisWords = numBitWords(size());
      unsigned rhsWords  = numBitWords(rhs.size());
      unsigned i;
      for (i = 0; i != std::min(thisWords, rhsWords); ++i) {
         if ((m_bits[i] & ~rhs.m_bits[i]) != 0) {
            return true;
         }
      }
      for (; i != thisWords ; ++i) {
         if (m_bits[i] != 0) {
            return true;
         }
      }
      return false;
   }

   BitVector &operator|=(const BitVector &rhs)
   {
      if (size() < rhs.size()) {
         resize(rhs.size());
      }
      for (size_t i = 0, e = numBitWords(rhs.size()); i != e; ++i) {
         m_bits[i] |= rhs.m_bits[i];
      }
      return *this;
   }

   BitVector &operator^=(const BitVector &rhs)
   {
      if (size() < rhs.size()) {
         resize(rhs.size());
      }
      for (size_t i = 0, e = numBitWords(rhs.size()); i != e; ++i) {
         m_bits[i] ^= rhs.m_bits[i];
      }
      return *this;
   }

   BitVector &operator>>=(unsigned N)
   {
      assert(N <= m_size);
      if (POLAR_UNLIKELY(empty() || N == 0))
         return *this;

      unsigned numWords = numBitWords(m_size);
      assert(numWords >= 1);

      wordShr(N / BITWORD_SIZE);

      unsigned bitDistance = N % BITWORD_SIZE;
      if (bitDistance == 0) {
         return *this;
      }
      // When the shift size is not a multiple of the word size, then we have
      // a tricky situation where each word in succession needs to extract some
      // of the bits from the next word and or them into this word while
      // shifting this word to make room for the new bits.  This has to be done
      // for every word in the array.

      // Since we're shifting each word right, some bits will fall off the end
      // of each word to the right, and empty space will be created on the left.
      // The final word in the array will lose bits permanently, so starting at
      // the beginning, work forwards shifting each word to the right, and
      // OR'ing in the bits from the end of the next word to the beginning of
      // the current word.

      // Example:
      //   Starting with {0xAABBCCDD, 0xEEFF0011, 0x22334455} and shifting right
      //   by 4 bits.
      // Step 1: Word[0] >>= 4           ; 0x0ABBCCDD
      // Step 2: Word[0] |= 0x10000000   ; 0x1ABBCCDD
      // Step 3: Word[1] >>= 4           ; 0x0EEFF001
      // Step 4: Word[1] |= 0x50000000   ; 0x5EEFF001
      // Step 5: Word[2] >>= 4           ; 0x02334455
      // Result: { 0x1ABBCCDD, 0x5EEFF001, 0x02334455 }
      const BitWord mask = mask_trailing_ones<BitWord>(bitDistance);
      const unsigned lsh = BITWORD_SIZE - bitDistance;

      for (unsigned idx = 0; idx < numWords - 1; ++idx) {
         m_bits[idx] >>= bitDistance;
         m_bits[idx] |= (m_bits[idx + 1] & mask) << lsh;
      }

      m_bits[numWords - 1] >>= bitDistance;
      return *this;
   }

   BitVector &operator<<=(unsigned N)
   {
      assert(N <= m_size);
      if (POLAR_UNLIKELY(empty() || N == 0)) {
         return *this;
      }

      unsigned numWords = numBitWords(m_size);
      assert(numWords >= 1);

      wordShl(N / BITWORD_SIZE);

      unsigned bitDistance = N % BITWORD_SIZE;
      if (bitDistance == 0) {
         return *this;
      }
      // When the shift size is not a multiple of the word size, then we have
      // a tricky situation where each word in succession needs to extract some
      // of the bits from the previous word and or them into this word while
      // shifting this word to make room for the new bits.  This has to be done
      // for every word in the array.  This is similar to the algorithm outlined
      // in operator>>=, but backwards.

      // Since we're shifting each word left, some bits will fall off the end
      // of each word to the left, and empty space will be created on the right.
      // The first word in the array will lose bits permanently, so starting at
      // the end, work backwards shifting each word to the left, and OR'ing
      // in the bits from the end of the next word to the beginning of the
      // current word.

      // Example:
      //   Starting with {0xAABBCCDD, 0xEEFF0011, 0x22334455} and shifting left
      //   by 4 bits.
      // Step 1: Word[2] <<= 4           ; 0x23344550
      // Step 2: Word[2] |= 0x0000000E   ; 0x2334455E
      // Step 3: Word[1] <<= 4           ; 0xEFF00110
      // Step 4: Word[1] |= 0x0000000A   ; 0xEFF0011A
      // Step 5: Word[0] <<= 4           ; 0xABBCCDD0
      // Result: { 0xABBCCDD0, 0xEFF0011A, 0x2334455E }
      const BitWord mask = mask_leading_ones<BitWord>(bitDistance);
      const unsigned rsh = BITWORD_SIZE - bitDistance;

      for (int idx = numWords - 1; idx > 0; --idx) {
         m_bits[idx] <<= bitDistance;
         m_bits[idx] |= (m_bits[idx - 1] & mask) >> rsh;
      }
      m_bits[0] <<= bitDistance;
      clearUnusedBits();

      return *this;
   }

   // Assignment operator.
   const BitVector &operator=(const BitVector &rhs)
   {
      if (this == &rhs) {
         return *this;
      }

      m_size = rhs.size();
      unsigned rhsWords = numBitWords(m_size);
      if (m_size <= getBitCapacity()) {
         if (m_size) {
            std::memcpy(m_bits.getData(), rhs.m_bits.getData(), rhsWords * sizeof(BitWord));
         }
         clearUnusedBits();
         return *this;
      }

      // Grow the bitvector to have enough elements.
      unsigned newcapacity = rhsWords;
      assert(newcapacity > 0 && "negative capacity?");
      auto newBits = allocate(newcapacity);
      std::memcpy(newBits.getData(), rhs.m_bits.getData(), newcapacity * sizeof(BitWord));
      // Destroy the old bits.
      std::free(m_bits.getData());
      m_bits = newBits;
      return *this;
   }

   const BitVector &operator=(BitVector &&rhs)
   {
      if (this == &rhs) {
         return *this;
      }

      std::free(m_bits.getData());
      m_bits = rhs.m_bits;
      m_size = rhs.m_size;

      rhs.m_bits = MutableArrayRef<BitWord>();
      rhs.m_size = 0;

      return *this;
   }

   void swap(BitVector &rhs)
   {
      std::swap(m_bits, rhs.m_bits);
      std::swap(m_size, rhs.m_size);
   }

   //===--------------------------------------------------------------------===//
   // Portable bit mask operations.
   //===--------------------------------------------------------------------===//
   //
   // These methods all operate on arrays of uint32_t, each holding 32 bits. The
   // fixed word size makes it easier to work with literal bit vector constants
   // in portable code.
   //
   // The LSB in each word is the lowest numbered bit.  The size of a portable
   // bit mask is always a whole multiple of 32 bits.  If no bit mask size is
   // given, the bit mask is assumed to cover the entire BitVector.

   /// setBitsInMask - Add '1' bits from Mask to this vector. Don't resize.
   /// This computes "*this |= Mask".
   void setBitsInMask(const uint32_t *mask, unsigned maskWords = ~0u)
   {
      applyMask<true, false>(mask, maskWords);
   }

   /// clearBitsInMask - Clear any bits in this vector that are set in Mask.
   /// Don't resize. This computes "*this &= ~Mask".
   void clearBitsInMask(const uint32_t *mask, unsigned maskWords = ~0u)
   {
      applyMask<false, false>(mask, maskWords);
   }

   /// setBitsNotInMask - Add a bit to this vector for every '0' bit in Mask.
   /// Don't resize.  This computes "*this |= ~Mask".
   void setBitsNotInMask(const uint32_t *mask, unsigned maskWords = ~0u)
   {
      applyMask<true, true>(mask, maskWords);
   }

   /// clearBitsNotInMask - Clear a bit in this vector for every '0' bit in Mask.
   /// Don't resize.  This computes "*this &= Mask".
   void clearBitsNotInMask(const uint32_t *mask, unsigned maskWords = ~0u)
   {
      applyMask<false, true>(mask, maskWords);
   }

private:
   /// \brief Perform a logical left shift of \p Count words by moving everything
   /// \p Count words to the right in memory.
   ///
   /// While confusing, words are stored from least significant at m_bits[0] to
   /// most significant at m_bits[numWords-1].  A logical shift left, however,
   /// moves the current least significant bit to a higher logical index, and
   /// fills the previous least significant bits with 0.  Thus, we actually
   /// need to move the bytes of the memory to the right, not to the left.
   /// Example:
   ///   Words = [0xBBBBAAAA, 0xDDDDFFFF, 0x00000000, 0xDDDD0000]
   /// represents a BitVector where 0xBBBBAAAA contain the least significant
   /// bits.  So if we want to shift the BitVector left by 2 words, we need to
   /// turn this into 0x00000000 0x00000000 0xBBBBAAAA 0xDDDDFFFF by using a
   /// memmove which moves right, not left.
   void wordShl(uint32_t count)
   {
      if (count == 0) {
         return;
      }

      uint32_t numWords = numBitWords(m_size);

      auto src = m_bits.takeFront(numWords).dropBack(count);
      auto dest = m_bits.takeFront(numWords).dropFront(count);

      // Since we always move Word-sized chunks of data with src and dest both
      // aligned to a word-boundary, we don't need to worry about endianness
      // here.
      std::memmove(dest.begin(), src.begin(), dest.getSize() * sizeof(BitWord));
      std::memset(m_bits.getData(), 0, count * sizeof(BitWord));
      clearUnusedBits();
   }

   /// \brief Perform a logical right shift of \p Count words by moving those
   /// words to the left in memory.  See wordShl for more information.
   ///
   void wordShr(uint32_t count)
   {
      if (count == 0) {
         return;
      }
      uint32_t numWords = numBitWords(m_size);

      auto src = m_bits.takeFront(numWords).dropFront(count);
      auto dest = m_bits.takeFront(numWords).dropBack(count);
      assert(dest.getSize() == src.getSize());

      std::memmove(dest.begin(), src.begin(), dest.getSize() * sizeof(BitWord));
      std::memset(dest.end(), 0, count * sizeof(BitWord));
   }

   MutableArrayRef<BitWord> allocate(size_t numWords)
   {
      BitWord *rawBits = (BitWord *)std::malloc(numWords * sizeof(BitWord));
      return MutableArrayRef<BitWord>(rawBits, numWords);
   }

   int nextUnsetInWord(int wordIndex, BitWord word) const
   {
      unsigned result = wordIndex * BITWORD_SIZE + count_trailing_ones(word);
      return result < size() ? result : -1;
   }

   unsigned numBitWords(unsigned svalue) const
   {
      return (svalue + BITWORD_SIZE-1) / BITWORD_SIZE;
   }

   // Set the unused bits in the high words.
   void setUnusedBits(bool flag = true)
   {
      //  Set high words first.
      unsigned uedWords = numBitWords(m_size);
      if (m_bits.getSize() > uedWords) {
         initWords(m_bits.dropFront(uedWords), flag);
      }
      //  Then set any stray high bits of the last used word.
      unsigned extraBits = m_size % BITWORD_SIZE;
      if (extraBits) {
         BitWord extraBitMask = ~0UL << extraBits;
         if (flag) {
            m_bits[uedWords-1] |= extraBitMask;
         } else {
            m_bits[uedWords-1] &= ~extraBitMask;
         }
      }
   }

   // Clear the unused bits in the high words.
   void clearUnusedBits()
   {
      setUnusedBits(false);
   }

   void grow(unsigned newSize)
   {
      size_t newcapacity = std::max<size_t>(numBitWords(newSize), m_bits.getSize() * 2);
      assert(newcapacity > 0 && "realloc-ing zero space");
      BitWord *newBits =
            (BitWord *)std::realloc(m_bits.getData(), newcapacity * sizeof(BitWord));
      m_bits = MutableArrayRef<BitWord>(newBits, newcapacity);
      clearUnusedBits();
   }

   void initWords(MutableArrayRef<BitWord> bits, bool flag)
   {
      if (bits.getSize() > 0) {
         memset(bits.getData(), 0 - (int)flag, bits.getSize() * sizeof(BitWord));
      }
   }

   template<bool AddBits, bool InvertMask>
   void applyMask(const uint32_t *maskPtr, unsigned maskWords)
   {
      static_assert(BITWORD_SIZE % 32 == 0, "Unsupported BitWord size.");
      maskWords = std::min(maskWords, (size() + 31) / 32);
      const unsigned scale = BITWORD_SIZE / 32;
      unsigned i;
      for (i = 0; maskWords >= scale; ++i, maskWords -= scale) {
         BitWord bitword = m_bits[i];
         // This inner loop should unroll completely when BITWORD_SIZE > 32.
         for (unsigned b = 0; b != BITWORD_SIZE; b += 32) {
            uint32_t mask = *maskPtr++;
            if (InvertMask) {
               mask = ~mask;
            }
            if (AddBits) {
               bitword |=   BitWord(mask) << b;
            } else {
               bitword &= ~(BitWord(mask) << b);
            }
         }
         m_bits[i] = bitword;
      }
      for (unsigned b = 0; maskWords; b += 32, --maskWords) {
         uint32_t mask = *maskPtr++;
         if (InvertMask) {
            mask = ~mask;
         }
         if (AddBits) {
            m_bits[i] |=   BitWord(mask) << b;
         } else {
            m_bits[i] &= ~(BitWord(mask) << b);
         }
      }
      if (AddBits) {
         clearUnusedBits();
      }
   }

public:
   /// Return the size (in bytes) of the bit vector.
   size_t getMemorySize() const
   {
      return m_bits.getSize() * sizeof(BitWord);
   }

   size_t getBitCapacity() const
   {
      return m_bits.getSize() * BITWORD_SIZE;
   }
};

inline size_t capacity_in_bytes(const BitVector &vector)
{
   return vector.getMemorySize();
}

} // basic
} // polar

namespace std {
/// Implement std::swap in terms of BitVector swap.
inline void swap(polar::basic::BitVector &lhs, polar::basic::BitVector &rhs)
{
   lhs.swap(rhs);
}
} // end namespace std

#endif // POLARPHP_BASIC_ADT_BIT_VECTOR_H
