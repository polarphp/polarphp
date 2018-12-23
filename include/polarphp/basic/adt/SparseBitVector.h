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

#ifndef POLARPHP_BASIC_ADT_SPARSE_BIT_VECTOR_H
#define POLARPHP_BASIC_ADT_SPARSE_BIT_VECTOR_H

#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/MathExtras.h"
#include "polarphp/utils/RawOutStream.h"
#include <cassert>
#include <climits>
#include <cstring>
#include <iterator>
#include <list>

namespace polar {
namespace basic {

using polar::utils::count_population;
using polar::utils::count_trailing_zeros;
using polar::utils::count_trailing_ones;
using polar::utils::count_leading_ones;
using polar::utils::count_leading_zeros;
using polar::utils::RawOutStream;

/// SparseBitVector is an implementation of a bitvector that is sparse by only
/// storing the elements that have non-zero bits set.  In order to make this
/// fast for the most common cases, SparseBitVector is implemented as a linked
/// list of SparseBitVectorm_elements.  We maintain a pointer to the last
/// SparseBitVectorElement accessed (in the form of a list iterator), in order
/// to make multiple in-order test/set constant time after the first one is
/// executed.  Note that using vectors to store SparseBitVectorElement's does
/// not work out very well because it causes insertion in the middle to take
/// enormous amounts of time with a large amount of bits.  Other structures that
/// have better worst cases for insertion in the middle (various balanced trees,
/// etc) do not perform as well in practice as a linked list with this iterator
/// kept up to date.  They are also significantly more memory intensive.

template <unsigned ElementSize = 128>
struct SparseBitVectorElement
{
public:
   using BitWord = unsigned long;
   using size_type = unsigned;
   enum {
      BITWORD_SIZE = sizeof(BitWord) * CHAR_BIT,
      BITWORDS_PER_ELEMENT = (ElementSize + BITWORD_SIZE - 1) / BITWORD_SIZE,
      BITS_PER_ELEMENT = ElementSize
   };

private:
   // Index of Element in terms of where first bit starts.
   unsigned m_elementIndex;
   BitWord m_bits[BITWORDS_PER_ELEMENT];

   SparseBitVectorElement()
   {
      m_elementIndex = ~0U;
      memset(&m_bits[0], 0, sizeof (BitWord) * BITWORDS_PER_ELEMENT);
   }

public:
   explicit SparseBitVectorElement(unsigned idx)
   {
      m_elementIndex = idx;
      memset(&m_bits[0], 0, sizeof (BitWord) * BITWORDS_PER_ELEMENT);
   }

   // Comparison.
   bool operator==(const SparseBitVectorElement &other) const
   {
      if (m_elementIndex != other.m_elementIndex) {
         return false;
      }
      for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
         if (m_bits[i] != other.m_bits[i]) {
            return false;
         }
      }
      return true;
   }

   bool operator!=(const SparseBitVectorElement &other) const
   {
      return !(*this == other);
   }

   // Return the bits that make up word idx in our element.
   BitWord word(unsigned idx) const
   {
      assert(idx < BITWORDS_PER_ELEMENT);
      return m_bits[idx];
   }

   unsigned index() const
   {
      return m_elementIndex;
   }

   bool empty() const
   {
      for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
         if (m_bits[i]) {
            return false;
         }
      }
      return true;
   }

   void set(unsigned idx)
   {
      m_bits[idx / BITWORD_SIZE] |= 1L << (idx % BITWORD_SIZE);
   }

   bool testAndSet(unsigned idx)
   {
      bool old = test(idx);
      if (!old) {
         set(idx);
         return true;
      }
      return false;
   }

   void reset(unsigned idx)
   {
      m_bits[idx / BITWORD_SIZE] &= ~(1L << (idx % BITWORD_SIZE));
   }

   bool test(unsigned idx) const
   {
      return m_bits[idx / BITWORD_SIZE] & (1L << (idx % BITWORD_SIZE));
   }

   size_type count() const
   {
      unsigned numBits = 0;
      for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
         numBits += count_population(m_bits[i]);
      }
      return numBits;
   }

   /// findFirst - Returns the index of the first set bit.
   int findFirst() const
   {
      for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
         if (m_bits[i] != 0) {
            return i * BITWORD_SIZE + count_trailing_zeros(m_bits[i]);
         }
      }
      polar_unreachable("Illegal empty element");
   }

   /// findLast - Returns the index of the last set bit.
   int findLast() const
   {
      for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
         unsigned idx = BITWORDS_PER_ELEMENT - i - 1;
         if (m_bits[idx] != 0) {
            return idx * BITWORD_SIZE + BITWORD_SIZE -
                  count_leading_zeros(m_bits[idx]) - 1;
         }
      }
      polar_unreachable("Illegal empty element");
   }

   /// findNext - Returns the index of the next set bit starting from the
   /// "Curr" bit. Returns -1 if the next set bit is not found.
   int findNext(unsigned curr) const
   {
      if (curr >= BITS_PER_ELEMENT) {
         return -1;
      }
      unsigned wordPos = curr / BITWORD_SIZE;
      unsigned bitPos = curr % BITWORD_SIZE;
      BitWord copy = m_bits[wordPos];
      assert(wordPos <= BITWORDS_PER_ELEMENT
             && "Word Position outside of element");

      // Mask off previous bits.
      copy &= ~0UL << bitPos;

      if (copy != 0) {
         return wordPos * BITWORD_SIZE + count_trailing_zeros(copy);
      }
      // Check subsequent words.
      for (unsigned i = wordPos+1; i < BITWORDS_PER_ELEMENT; ++i) {
         if (m_bits[i] != 0) {
            return i * BITWORD_SIZE + count_trailing_zeros(m_bits[i]);
         }
      }
      return -1;
   }

   // Union this element with other and return true if this one changed.
   bool unionWith(const SparseBitVectorElement &other)
   {
      bool changed = false;
      for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
         BitWord old = changed ? 0 : m_bits[i];
         m_bits[i] |= other.m_bits[i];
         if (!changed && old != m_bits[i]) {
            changed = true;
         }
      }
      return changed;
   }

   // Return true if we have any bits in common with other
   bool intersects(const SparseBitVectorElement &other) const
   {
      for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
         if (other.m_bits[i] & m_bits[i]) {
            return true;
         }
      }
      return false;
   }

   // Intersect this Element with other and return true if this one changed.
   // becameZero is set to true if this element became all-zero bits.
   bool intersectWith(const SparseBitVectorElement &other,
                      bool &becameZero)
   {
      bool changed = false;
      bool allzero = true;
      becameZero = false;
      for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
         BitWord old = changed ? 0 : m_bits[i];
         m_bits[i] &= other.m_bits[i];
         if (m_bits[i] != 0) {
            allzero = false;
         }
         if (!changed && old != m_bits[i]) {
            changed = true;
         }
      }
      becameZero = allzero;
      return changed;
   }

   // Intersect this Element with the complement of other and return true if this
   // one changed.  becameZero is set to true if this element became all-zero
   // bits.
   bool intersectWithComplement(const SparseBitVectorElement &other,
                                bool &becameZero)
   {
      bool changed = false;
      bool allzero = true;
      becameZero = false;
      for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
         BitWord old = changed ? 0 : m_bits[i];

         m_bits[i] &= ~other.m_bits[i];
         if (m_bits[i] != 0) {
            allzero = false;
         }
         if (!changed && old != m_bits[i]) {
            changed = true;
         }
      }
      becameZero = allzero;
      return changed;
   }

   // Three argument version of intersectWithComplement that intersects
   // lhs & ~rhs into this element
   void intersectWithComplement(const SparseBitVectorElement &lhs,
                                const SparseBitVectorElement &rhs,
                                bool &becameZero)
   {
      bool allzero = true;
      becameZero = false;
      for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
         m_bits[i] = lhs.m_bits[i] & ~rhs.m_bits[i];
         if (m_bits[i] != 0) {
            allzero = false;
         }
      }
      becameZero = allzero;
   }
};

template <unsigned ElementSize = 128>
class SparseBitVector
{
   using ElementList = std::list<SparseBitVectorElement<ElementSize>>;
   using ElementListIter = typename ElementList::iterator;
   using ElementListConstIter = typename ElementList::const_iterator;
   enum {
      BITWORD_SIZE = SparseBitVectorElement<ElementSize>::BITWORD_SIZE
   };

   ElementList m_elements;

   // Pointer to our current Element. This has no visible effect on the external
   // state of a SparseBitVector, it's just used to improve performance in the
   // common case of testing/modifying bits with similar indices.
   mutable ElementListIter m_currElementIter;

   // This is like std::lower_bound, except we do linear searching from the
   // current position.
   ElementListIter findLowerBoundImpl(unsigned elementIndex) const
   {
      // We cache a non-const iterator so we're forced to resort to const_cast to
      // get the begin/end in the case where 'this' is const. To avoid duplication
      // of code with the only difference being whether the const cast is present
      // 'this' is always const in this particular function and we sort out the
      // difference in FindLowerBound and FindLowerBoundConst.
      ElementListIter begin =
            const_cast<SparseBitVector<ElementSize> *>(this)->m_elements.begin();
      ElementListIter end =
            const_cast<SparseBitVector<ElementSize> *>(this)->m_elements.end();
      if (m_elements.empty()) {
         m_currElementIter = begin;
         return m_currElementIter;
      }
      // Make sure our current iterator is valid.
      if (m_currElementIter == end) {
         --m_currElementIter;
      }
      // Search from our current iterator, either backwards or forwards,
      // depending on what element we are looking for.
      ElementListIter elementIter = m_currElementIter;
      if (m_currElementIter->index() == elementIndex) {
         return elementIter;
      } else if (m_currElementIter->index() > elementIndex) {
         while (elementIter != begin
                && elementIter->index() > elementIndex) {
            --elementIter;
         }
      } else {
         while (elementIter != end &&
                elementIter->index() < elementIndex) {
            ++elementIter;
         }
      }
      m_currElementIter = elementIter;
      return elementIter;
   }

   ElementListConstIter findLowerBoundConst(unsigned elementIndex) const
   {
      return findLowerBoundImpl(elementIndex);
   }

   ElementListIter findLowerBound(unsigned elementIndex)
   {
      return findLowerBoundImpl(elementIndex);
   }

   // Iterator to walk set bits in the bitmap.  This iterator is a lot uglier
   // than it would be, in order to be efficient.
   class SparseBitVectorIterator
   {
   private:
      bool m_atEnd;
      const SparseBitVector<ElementSize> *m_bitVector = nullptr;
      // Current element inside of bitmap.
      ElementListConstIter m_iter;
      // Current bit number inside of our bitmap.
      unsigned m_bitNumber;

      // Current word number inside of our element.
      unsigned m_wordNumber;

      // Current bits from the element.
      typename SparseBitVectorElement<ElementSize>::BitWord m_bits;

      // Move our iterator to the first non-zero bit in the bitmap.
      void advanceToFirstNonZero()
      {
         if (m_atEnd)
            return;
         if (m_bitVector->m_elements.empty()) {
            m_atEnd = true;
            return;
         }
         m_iter = m_bitVector->m_elements.begin();
         m_bitNumber = m_iter->index() * ElementSize;
         unsigned bitPos = m_iter->findFirst();
         m_bitNumber += bitPos;
         m_wordNumber = (m_bitNumber % ElementSize) / BITWORD_SIZE;
         m_bits = m_iter->word(m_wordNumber);
         m_bits >>= bitPos % BITWORD_SIZE;
      }

      // Move our iterator to the next non-zero bit.
      void advanceToNextNonZero()
      {
         if (m_atEnd) {
            return;
         }
         while (m_bits && !(m_bits & 1)) {
            m_bits >>= 1;
            m_bitNumber += 1;
         }
         // See if we ran out of Bits in this word.
         if (!m_bits) {
            int nextSetBitNumber = m_iter->findNext(m_bitNumber % ElementSize) ;
            // If we ran out of set bits in this element, move to next element.
            if (nextSetBitNumber == -1 || (m_bitNumber % ElementSize == 0)) {
               ++m_iter;
               m_wordNumber = 0;

               // We may run out of elements in the bitmap.
               if (m_iter == m_bitVector->m_elements.end()) {
                  m_atEnd = true;
                  return;
               }
               // Set up for next non-zero word in bitmap.
               m_bitNumber = m_iter->index() * ElementSize;
               nextSetBitNumber = m_iter->findFirst();
               m_bitNumber += nextSetBitNumber;
               m_wordNumber = (m_bitNumber % ElementSize) / BITWORD_SIZE;
               m_bits = m_iter->word(m_wordNumber);
               m_bits >>= nextSetBitNumber % BITWORD_SIZE;
            } else {
               m_wordNumber = (nextSetBitNumber % ElementSize) / BITWORD_SIZE;
               m_bits = m_iter->word(m_wordNumber);
               m_bits >>= nextSetBitNumber % BITWORD_SIZE;
               m_bitNumber = m_iter->index() * ElementSize;
               m_bitNumber += nextSetBitNumber;
            }
         }
      }

   public:
      SparseBitVectorIterator() = default;
      SparseBitVectorIterator(const SparseBitVector<ElementSize> *other,
                              bool end = false):m_bitVector(other)
      {
         m_iter = m_bitVector->m_elements.begin();
         m_bitNumber = 0;
         m_bits = 0;
         m_wordNumber = ~0;
         m_atEnd = end;
         advanceToFirstNonZero();
      }

      // Preincrement.
      inline SparseBitVectorIterator& operator++()
      {
         ++m_bitNumber;
         m_bits >>= 1;
         advanceToNextNonZero();
         return *this;
      }

      // Postincrement.
      inline SparseBitVectorIterator operator++(int)
      {
         SparseBitVectorIterator temp = *this;
         ++*this;
         return temp;
      }

      // Return the current set bit number.
      unsigned operator*() const
      {
         return m_bitNumber;
      }

      bool operator==(const SparseBitVectorIterator &other) const
      {
         // If they are both at the end, ignore the rest of the fields.
         if (m_atEnd && other.m_atEnd)
            return true;
         // Otherwise they are the same if they have the same bit number and
         // bitmap.
         return m_atEnd == other.m_atEnd && other.m_bitNumber == m_bitNumber;
      }

      bool operator!=(const SparseBitVectorIterator &other) const
      {
         return !(*this == other);
      }
   };

public:
   using iterator = SparseBitVectorIterator;

   SparseBitVector()
      : m_elements(),
        m_currElementIter(m_elements.begin())
   {
   }

   SparseBitVector(const SparseBitVector &other)
      : m_elements(other.m_elements),
        m_currElementIter(m_elements.begin())
   {}

   SparseBitVector(SparseBitVector &&other)
      : m_elements(std::move(other.m_elements)),
        m_currElementIter(m_elements.begin())
   {}

   // Clear.
   void clear()
   {
      m_elements.clear();
   }

   // Assignment
   SparseBitVector& operator=(const SparseBitVector& other)
   {
      if (this == &other) {
         return *this;
      }
      m_elements = other.m_elements;
      m_currElementIter = m_elements.begin();
      return *this;
   }

   SparseBitVector &operator=(SparseBitVector &&other)
   {
      m_elements = std::move(other.m_elements);
      m_currElementIter = m_elements.begin();
      return *this;
   }

   // Test, Reset, and Set a bit in the bitmap.
   bool test(unsigned idx) const
   {
      if (m_elements.empty()) {
         return false;
      }
      unsigned elementIndex = idx / ElementSize;
      ElementListConstIter elementIter = findLowerBoundConst(elementIndex);

      // If we can't find an element that is supposed to contain this bit, there
      // is nothing more to do.
      if (elementIter == m_elements.end() ||
          elementIter->index() != elementIndex) {
         return false;
      }
      return elementIter->test(idx % ElementSize);
   }

   void reset(unsigned idx)
   {
      if (m_elements.empty()) {
         return;
      }
      unsigned elementIndex = idx / ElementSize;
      ElementListIter elementIter = findLowerBound(elementIndex);

      // If we can't find an element that is supposed to contain this bit, there
      // is nothing more to do.
      if (elementIter == m_elements.end() ||
          elementIter->index() != elementIndex) {
         return;
      }
      elementIter->reset(idx % ElementSize);
      // When the element is zeroed out, delete it.
      if (elementIter->empty()) {
         ++m_currElementIter;
         m_elements.erase(elementIter);
      }
   }

   void set(unsigned idx)
   {
      unsigned elementIndex = idx / ElementSize;
      ElementListIter elementIter;
      if (m_elements.empty()) {
         elementIter = m_elements.emplace(m_elements.end(), elementIndex);
      } else {
         elementIter = findLowerBound(elementIndex);
         if (elementIter == m_elements.end() ||
             elementIter->index() != elementIndex) {
            // We may have hit the beginning of our SparseBitVector, in which case,
            // we may need to insert right after this element, which requires moving
            // the current iterator forward one, because insert does insert before.
            if (elementIter != m_elements.end() &&
                elementIter->index() < elementIndex) {
               ++elementIter;
            }
            elementIter = m_elements.emplace(elementIter, elementIndex);
         }
      }
      m_currElementIter = elementIter;
      elementIter->set(idx % ElementSize);
   }

   bool testAndSet(unsigned idx)
   {
      bool old = test(idx);
      if (!old) {
         set(idx);
         return true;
      }
      return false;
   }

   bool operator!=(const SparseBitVector &other) const
   {
      return !(*this == other);
   }

   bool operator==(const SparseBitVector &other) const
   {
      ElementListConstIter iter1 = m_elements.begin();
      ElementListConstIter iter2 = other.m_elements.begin();
      for (; iter1 != m_elements.end() && iter2 != other.m_elements.end();
           ++iter1, ++iter2) {
         if (*iter1 != *iter2)
            return false;
      }
      return iter1 == m_elements.end() && iter2 == other.m_elements.end();
   }

   // Union our bitmap with the other and return true if we changed.
   bool operator|=(const SparseBitVector &other)
   {
      if (this == &other) {
         return false;
      }
      bool changed = false;
      ElementListIter iter1 = m_elements.begin();
      ElementListConstIter iter2 = other.m_elements.begin();
      // If other is empty, we are done
      if (other.m_elements.empty()) {
         return false;
      }
      while (iter2 != other.m_elements.end()) {
         if (iter1 == m_elements.end() || iter1->index() > iter2->index()) {
            m_elements.insert(iter1, *iter2);
            ++iter2;
            changed = true;
         } else if (iter1->index() == iter2->index()) {
            changed |= iter1->unionWith(*iter2);
            ++iter1;
            ++iter2;
         } else {
            ++iter1;
         }
      }
      m_currElementIter = m_elements.begin();
      return changed;
   }

   // Intersect our bitmap with the other and return true if ours changed.
   bool operator&=(const SparseBitVector &other)
   {
      if (this == &other) {
         return false;
      }
      bool changed = false;
      ElementListIter iter1 = m_elements.begin();
      ElementListConstIter iter2 = other.m_elements.begin();
      // Check if both bitmaps are empty.
      if (m_elements.empty() && other.m_elements.empty()) {
         return false;
      }
      // Loop through, intersecting as we go, erasing elements when necessary.
      while (iter2 != other.m_elements.end()) {
         if (iter1 == m_elements.end()) {
            m_currElementIter = m_elements.begin();
            return changed;
         }

         if (iter1->index() > iter2->index()) {
            ++iter2;
         } else if (iter1->index() == iter2->index()) {
            bool becameZero;
            changed |= iter1->intersectWith(*iter2, becameZero);
            if (becameZero) {
               ElementListIter IterTmp = iter1;
               ++iter1;
               m_elements.erase(IterTmp);
            } else {
               ++iter1;
            }
            ++iter2;
         } else {
            ElementListIter IterTmp = iter1;
            ++iter1;
            m_elements.erase(IterTmp);
            changed = true;
         }
      }
      if (iter1 != m_elements.end()) {
         m_elements.erase(iter1, m_elements.end());
         changed = true;
      }
      m_currElementIter = m_elements.begin();
      return changed;
   }

   // Intersect our bitmap with the complement of the other and return true
   // if ours changed.
   bool intersectWithComplement(const SparseBitVector &other)
   {
      if (this == &other) {
         if (!empty()) {
            clear();
            return true;
         }
         return false;
      }

      bool changed = false;
      ElementListIter iter1 = m_elements.begin();
      ElementListConstIter iter2 = other.m_elements.begin();

      // If either our bitmap or other is empty, we are done
      if (m_elements.empty() || other.m_elements.empty())
         return false;

      // Loop through, intersecting as we go, erasing elements when necessary.
      while (iter2 != other.m_elements.end()) {
         if (iter1 == m_elements.end()) {
            m_currElementIter = m_elements.begin();
            return changed;
         }

         if (iter1->index() > iter2->index()) {
            ++iter2;
         } else if (iter1->index() == iter2->index()) {
            bool becameZero;
            changed |= iter1->intersectWithComplement(*iter2, becameZero);
            if (becameZero) {
               ElementListIter IterTmp = iter1;
               ++iter1;
               m_elements.erase(IterTmp);
            } else {
               ++iter1;
            }
            ++iter2;
         } else {
            ++iter1;
         }
      }
      m_currElementIter = m_elements.begin();
      return changed;
   }

   bool intersectWithComplement(const SparseBitVector<ElementSize> *other) const
   {
      return intersectWithComplement(*other);
   }

   //  Three argument version of intersectWithComplement.
   //  Result of lhs & ~rhs is stored into this bitmap.
   void intersectWithComplement(const SparseBitVector<ElementSize> &lhs,
                                const SparseBitVector<ElementSize> &rhs)
   {
      if (this == &lhs) {
         intersectWithComplement(rhs);
         return;
      } else if (this == &rhs) {
         SparseBitVector rhsCopy(rhs);
         intersectWithComplement(lhs, rhsCopy);
         return;
      }

      m_elements.clear();
      m_currElementIter = m_elements.begin();
      ElementListConstIter iter1 = lhs.m_elements.begin();
      ElementListConstIter iter2 = rhs.m_elements.begin();

      // If lhs is empty, we are done
      // If rhs is empty, we still have to copy lhs
      if (lhs.m_elements.empty())
         return;

      // Loop through, intersecting as we go, erasing elements when necessary.
      while (iter2 != rhs.m_elements.end()) {
         if (iter1 == lhs.m_elements.end()) {
            return;
         }
         if (iter1->index() > iter2->index()) {
            ++iter2;
         } else if (iter1->index() == iter2->index()) {
            bool becameZero = false;
            m_elements.emplace_back(iter1->index());
            m_elements.back().intersectWithComplement(*iter1, *iter2, becameZero);
            if (becameZero) {
               m_elements.pop_back();
            }
            ++iter1;
            ++iter2;
         } else {
            m_elements.push_back(*iter1++);
         }
      }

      // copy the remaining elements
      std::copy(iter1, lhs.m_elements.end(), std::back_inserter(m_elements));
   }

   void intersectWithComplement(const SparseBitVector<ElementSize> *lhs,
                                const SparseBitVector<ElementSize> *rhs)
   {
      intersectWithComplement(*lhs, *rhs);
   }

   bool intersects(const SparseBitVector<ElementSize> *other) const
   {
      return intersects(*other);
   }

   // Return true if we share any bits in common with other
   bool intersects(const SparseBitVector<ElementSize> &other) const
   {
      ElementListConstIter iter1 = m_elements.begin();
      ElementListConstIter iter2 = other.m_elements.begin();

      // Check if both bitmaps are empty.
      if (m_elements.empty() && other.m_elements.empty())
         return false;

      // Loop through, intersecting stopping when we hit bits in common.
      while (iter2 != other.m_elements.end()) {
         if (iter1 == m_elements.end()) {
            return false;
         }

         if (iter1->index() > iter2->index()) {
            ++iter2;
         } else if (iter1->index() == iter2->index()) {
            if (iter1->intersects(*iter2)) {
               return true;
            }
            ++iter1;
            ++iter2;
         } else {
            ++iter1;
         }
      }
      return false;
   }

   // Return true iff all bits set in this SparseBitVector are
   // also set in other.
   bool contains(const SparseBitVector<ElementSize> &other) const
   {
      SparseBitVector<ElementSize> result(*this);
      result &= other;
      return (result == other);
   }

   // Return the first set bit in the bitmap.  Return -1 if no bits are set.
   int findFirst() const
   {
      if (m_elements.empty()) {
         return -1;
      }
      const SparseBitVectorElement<ElementSize> &first = *(m_elements.begin());
      return (first.index() * ElementSize) + first.findFirst();
   }

   // Return the last set bit in the bitmap.  Return -1 if no bits are set.
   int findLast() const
   {
      if (m_elements.empty()) {
         return -1;
      }
      const SparseBitVectorElement<ElementSize> &Last = *(m_elements.rbegin());
      return (Last.index() * ElementSize) + Last.findLast();
   }

   // Return true if the SparseBitVector is empty
   bool empty() const
   {
      return m_elements.empty();
   }

   unsigned count() const
   {
      unsigned bitCount = 0;
      for (ElementListConstIter iter = m_elements.begin(); iter != m_elements.end(); ++iter) {
         bitCount += iter->count();
      }
      return bitCount;
   }

   iterator begin() const
   {
      return iterator(this);
   }

   iterator end() const
   {
      return iterator(this, true);
   }
};

// Convenience functions to allow Or and And without dereferencing in the user
// code.

template <unsigned ElementSize>
inline bool operator |=(SparseBitVector<ElementSize> &lhs,
                        const SparseBitVector<ElementSize> *rhs)
{
   return lhs |= *rhs;
}

template <unsigned ElementSize>
inline bool operator |=(SparseBitVector<ElementSize> *lhs,
                        const SparseBitVector<ElementSize> &rhs)
{
   return lhs->operator|=(rhs);
}

template <unsigned ElementSize>
inline bool operator &=(SparseBitVector<ElementSize> *lhs,
                        const SparseBitVector<ElementSize> &rhs)
{
   return lhs->operator&=(rhs);
}

template <unsigned ElementSize>
inline bool operator &=(SparseBitVector<ElementSize> &lhs,
                        const SparseBitVector<ElementSize> *rhs)
{
   return lhs &= *rhs;
}

// Convenience functions for infix union, intersection, difference operators.

template <unsigned ElementSize>
inline SparseBitVector<ElementSize>
operator|(const SparseBitVector<ElementSize> &lhs,
          const SparseBitVector<ElementSize> &rhs)
{
   SparseBitVector<ElementSize> result(lhs);
   result |= rhs;
   return result;
}

template <unsigned ElementSize>
inline SparseBitVector<ElementSize>
operator&(const SparseBitVector<ElementSize> &lhs,
          const SparseBitVector<ElementSize> &rhs)
{

   SparseBitVector<ElementSize> result(lhs);
   result &= rhs;
   return result;
}

template <unsigned ElementSize>
inline SparseBitVector<ElementSize>
operator-(const SparseBitVector<ElementSize> &lhs,
          const SparseBitVector<ElementSize> &rhs)
{
   SparseBitVector<ElementSize> result;
   result.intersectWithComplement(lhs, rhs);
   return result;
}

// Dump a SparseBitVector to a stream
template <unsigned ElementSize>
void dump(const SparseBitVector<ElementSize> &vector, RawOutStream &out)
{
   out << "[";

   typename SparseBitVector<ElementSize>::iterator bi = vector.begin(),
         be = vector.end();
   if (bi != be) {
      out << *bi;
      for (++bi; bi != be; ++bi) {
         out << " " << *bi;
      }
   }
   out << "]\n";
}

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_SPARSE_BIT_VECTOR_H
