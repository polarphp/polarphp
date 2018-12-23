// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/15.

#include "polarphp/basic/adt/ApInt.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/FoldingSet.h"
#include "polarphp/basic/adt/Hashing.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/ApInt.h"
#include "polarphp/basic/adt/Bit.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/MathExtras.h"
#include "polarphp/utils/RawOutStream.h"

#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <optional>

namespace polar {
namespace basic {

#define DEBUG_TYPE "ApInt"
using polar::utils::byte_swap16;
using polar::utils::byte_swap32;
using polar::utils::byte_swap64;
using polar::utils::reverse_bits;
using polar::utils::sign_extend64;
using polar::debug_stream;
using polar::utils::low32;
using polar::utils::high32;
using polar::utils::make64;
using polar::utils::find_last_set;
using polar::utils::find_first_set;
using polar::utils::ZeroBehavior;

namespace {
/// A utility function for allocating memory, checking for allocation failures,
/// and ensuring the contents are zeroed.
inline uint64_t *get_cleared_memory(unsigned numWords)
{
   uint64_t *result = new uint64_t[numWords];
   memset(result, 0, numWords * sizeof(uint64_t));
   return result;
}
/// A utility function for allocating memory and checking for allocation
/// failure.  The content is not zeroed.
inline uint64_t *get_memory(unsigned numWords)
{
   return new uint64_t[numWords];
}
/// A utility function that converts a character to a digit.
inline unsigned get_digit(char cdigit, uint8_t radix)
{
   unsigned r;
   if (radix == 16 || radix == 36) {
      r = cdigit - '0';
      if (r <= 9) {
         return r;
      }
      r = cdigit - 'A';
      if (r <= radix - 11U) {
         return r + 10;
      }
      r = cdigit - 'a';
      if (r <= radix - 11U) {
         return r + 10;
      }
      radix = 10;
   }
   r = cdigit - '0';
   if (r < radix) {
      return r;
   }
   return -1U;
}
} // anonymous namespace
void ApInt::initSlowCase(uint64_t value, bool isSigned)
{
   m_intValue.m_pValue = get_cleared_memory(getNumWords());
   m_intValue.m_pValue[0] = value;
   if (isSigned && int64_t(value) < 0) {
      for (unsigned i = 1; i < getNumWords(); ++i) {
         m_intValue.m_pValue[i] = WORDTYPE_MAX;
      }
   }
   clearUnusedBits();
}
void ApInt::initSlowCase(const ApInt& other)
{
   m_intValue.m_pValue = get_memory(getNumWords());
   memcpy(m_intValue.m_pValue, other.m_intValue.m_pValue, getNumWords() * APINT_WORD_SIZE);
}
void ApInt::initFromArray(ArrayRef<uint64_t> bigVal)
{
   assert(m_bitWidth && "Bitwidth too small");
   assert(bigVal.getData() && "Null pointer detected!");
   if (isSingleWord()) {
      m_intValue.m_value = bigVal[0];
   } else {
      // Get memory, cleared to 0
      m_intValue.m_pValue = get_cleared_memory(getNumWords());
      // Calculate the number of words to copy
      unsigned words = std::min<unsigned>(bigVal.getSize(), getNumWords());
      // Copy the words from bigVal to pVal
      memcpy(m_intValue.m_pValue, bigVal.getData(), words * APINT_WORD_SIZE);
   }
   // Make sure unused high bits are cleared
   clearUnusedBits();
}
ApInt::ApInt(unsigned numBits, ArrayRef<uint64_t> bigVal)
   : m_bitWidth(numBits)
{
   initFromArray(bigVal);
}
ApInt::ApInt(unsigned numBits, unsigned numWords, const uint64_t bigVal[])
   : m_bitWidth(numBits)
{
   initFromArray(make_array_ref(bigVal, numWords));
}
ApInt::ApInt(unsigned numbits, StringRef str, uint8_t radix)
   : m_bitWidth(numbits)
{
   assert(m_bitWidth && "Bitwidth too small");
   fromString(numbits, str, radix);
}
void ApInt::reallocate(unsigned newm_bitWidth)
{
   // If the number of words is the same we can just change the width and stop.
   if (getNumWords() == getNumWords(newm_bitWidth)) {
      m_bitWidth = newm_bitWidth;
      return;
   }
   // If we have an allocation, delete it.
   if (!isSingleWord()) {
      delete [] m_intValue.m_pValue;
   }
   // Update m_bitWidth.
   m_bitWidth = newm_bitWidth;
   // If we are supposed to have an allocation, create it.
   if (!isSingleWord()) {
      m_intValue.m_pValue = get_memory(getNumWords());
   }
}
void ApInt::assignSlowCase(const ApInt& other)
{
   // Don't do anything for X = X
   if (this == &other) {
      return;
   }
   // Adjust the bit width and handle allocations as necessary.
   reallocate(other.getBitWidth());
   // Copy the data.
   if (isSingleWord()) {
      m_intValue.m_value = other.m_intValue.m_value;
   } else {
      memcpy(m_intValue.m_pValue, other.m_intValue.m_pValue, getNumWords() * APINT_WORD_SIZE);
   }
}
/// This method 'profiles' an ApInt for use with FoldingSet.
void ApInt::profile(FoldingSetNodeId& id) const
{
   id.addInteger(m_bitWidth);
   if (isSingleWord()) {
      id.addInteger(m_intValue.m_value);
      return;
   }
   unsigned numWords = getNumWords();
   for (unsigned i = 0; i < numWords; ++i) {
      id.addInteger(m_intValue.m_pValue[i]);
   }
}
/// @brief Prefix increment operator. Increments the ApInt by one.
ApInt& ApInt::operator++()
{
   if (isSingleWord()) {
      ++m_intValue.m_value;
   } else {
      tcIncrement(m_intValue.m_pValue, getNumWords());
   }
   return clearUnusedBits();
}
/// @brief Prefix decrement operator. Decrements the ApInt by one.
ApInt& ApInt::operator--()
{
   if (isSingleWord()) {
      --m_intValue.m_value;
   } else {
      tcDecrement(m_intValue.m_pValue, getNumWords());
   }
   return clearUnusedBits();
}
/// Adds the rhs ApInt to this ApInt.
/// @returns this, after addition of rhs.
/// @brief Addition assignment operator.
ApInt& ApInt::operator+=(const ApInt& other)
{
   assert(m_bitWidth == other.m_bitWidth && "Bit widths must be the same");
   if (isSingleWord()) {
      m_intValue.m_value += other.m_intValue.m_value;
   } else {
      tcAdd(m_intValue.m_pValue, other.m_intValue.m_pValue, 0, getNumWords());
   }
   return clearUnusedBits();
}
ApInt& ApInt::operator+=(uint64_t other)
{
   if (isSingleWord()) {
      m_intValue.m_value += other;
   } else {
      tcAddPart(m_intValue.m_pValue, other, getNumWords());
   }
   return clearUnusedBits();
}
/// Subtracts the rhs ApInt from this ApInt
/// @returns this, after subtraction
/// @brief Subtraction assignment operator.
ApInt& ApInt::operator-=(const ApInt& other)
{
   assert(m_bitWidth == other.m_bitWidth && "Bit widths must be the same");
   if (isSingleWord()) {
      m_intValue.m_value -= other.m_intValue.m_value;
   } else {
      tcSubtract(m_intValue.m_pValue, other.m_intValue.m_pValue, 0, getNumWords());
   }
   return clearUnusedBits();
}
ApInt& ApInt::operator-=(uint64_t other)
{
   if (isSingleWord()) {
      m_intValue.m_value -= other;
   } else {
      tcSubtractPart(m_intValue.m_pValue, other, getNumWords());
   }
   return clearUnusedBits();
}
ApInt ApInt::operator*(const ApInt& other) const
{
   assert(m_bitWidth == other.m_bitWidth && "Bit widths must be the same");
   if (isSingleWord()) {
      return ApInt(m_bitWidth, m_intValue.m_value * other.m_intValue.m_value);
   }
   ApInt result(get_memory(getNumWords()), getBitWidth());
   tcMultiply(result.m_intValue.m_pValue, m_intValue.m_pValue, other.m_intValue.m_pValue, getNumWords());
   result.clearUnusedBits();
   return result;
}
void ApInt::andAssignSlowCase(const ApInt &other)
{
   tcAnd(m_intValue.m_pValue, other.m_intValue.m_pValue, getNumWords());
}
void ApInt::orAssignSlowCase(const ApInt &other)
{
   tcOr(m_intValue.m_pValue, other.m_intValue.m_pValue, getNumWords());
}
void ApInt::xorAssignSlowCase(const ApInt &other)
{
   tcXor(m_intValue.m_pValue, other.m_intValue.m_pValue, getNumWords());
}
ApInt& ApInt::operator*=(const ApInt& other)
{
   assert(m_bitWidth == other.m_bitWidth && "Bit widths must be the same");
   *this = *this * other;
   return *this;
}
ApInt& ApInt::operator*=(uint64_t other)
{
   if (isSingleWord()) {
      m_intValue.m_value *= other;
   } else {
      unsigned numWords = getNumWords();
      tcMultiplyPart(m_intValue.m_pValue, m_intValue.m_pValue, other, 0, numWords, numWords, false);
   }
   return clearUnusedBits();
}
bool ApInt::equalSlowCase(const ApInt& other) const
{
   return std::equal(m_intValue.m_pValue, m_intValue.m_pValue + getNumWords(), other.m_intValue.m_pValue);
}
int ApInt::compare(const ApInt &other) const
{
   assert(m_bitWidth == other.m_bitWidth && "Bit widths must be same for comparison");
   if (isSingleWord()) {
      return m_intValue.m_value < other.m_intValue.m_value ? -1 : m_intValue.m_value > other.m_intValue.m_value;
   }
   return tcCompare(m_intValue.m_pValue, other.m_intValue.m_pValue, getNumWords());
}
int ApInt::compareSigned(const ApInt& other) const
{
   assert(m_bitWidth == other.m_bitWidth && "Bit widths must be same for comparison");
   if (isSingleWord()) {
      int64_t lhsSext = sign_extend64(m_intValue.m_value, m_bitWidth);
      int64_t rhsSext = sign_extend64(other.m_intValue.m_value, m_bitWidth);
      return lhsSext < rhsSext ? -1 : lhsSext > rhsSext;
   }
   bool lhsNeg = isNegative();
   bool rhsNeg = other.isNegative();
   // If the sign bits don't match, then (LHS < rhs) if LHS is negative
   if (lhsNeg != rhsNeg) {
      return lhsNeg ? -1 : 1;
   }
   // Otherwise we can just use an unsigned comparison, because even negative
   // numbers compare correctly this way if both have the same signed-ness.
   return tcCompare(m_intValue.m_pValue, other.m_intValue.m_pValue, getNumWords());
}
void ApInt::setBitsSlowCase(unsigned loBit, unsigned hiBit)
{
   unsigned loWord = whichWord(loBit);
   unsigned hiWord = whichWord(hiBit);
   // Create an initial mask for the low word with zeros below loBit.
   uint64_t loMask = WORDTYPE_MAX << whichBit(loBit);
   // If hiBit is not aligned, we need a high mask.
   unsigned hiShiftAmt = whichBit(hiBit);
   if (hiShiftAmt != 0) {
      // Create a high mask with zeros above hiBit.
      uint64_t hiMask = WORDTYPE_MAX >> (APINT_BITS_PER_WORD - hiShiftAmt);
      // If loWord and hiWord are equal, then we combine the masks. Otherwise,
      // set the bits in hiWord.
      if (hiWord == loWord) {
         loMask &= hiMask;
      } else {
         m_intValue.m_pValue[hiWord] |= hiMask;
      }
   }
   // Apply the mask to the low word.
   m_intValue.m_pValue[loWord] |= loMask;
   // Fill any words between loWord and hiWord with all ones.
   for (unsigned word = loWord + 1; word < hiWord; ++word) {
      m_intValue.m_pValue[word] = WORDTYPE_MAX;
   }
}
/// @brief Toggle every bit to its opposite value.
void ApInt::flipAllBitsSlowCase()
{
   tcComplement(m_intValue.m_pValue, getNumWords());
   clearUnusedBits();
}
/// Toggle a given bit to its opposite value whose position is given
/// as "bitPosition".
/// @brief Toggles a given bit to its opposite value.
void ApInt::flipBit(unsigned bitPosition)
{
   assert(bitPosition < m_bitWidth && "Out of the bit-width range!");
   if ((*this)[bitPosition]) {
      clearBit(bitPosition);
   }
   else setBit(bitPosition);
}
void ApInt::insertBits(const ApInt &subBits, unsigned bitPosition)
{
   unsigned subm_bitWidth = subBits.getBitWidth();
   assert(0 < subm_bitWidth && (subm_bitWidth + bitPosition) <= m_bitWidth &&
          "Illegal bit insertion");
   // Insertion is a direct copy.
   if (subm_bitWidth == m_bitWidth) {
      *this = subBits;
      return;
   }
   // Single word result can be done as a direct bitmask.
   if (isSingleWord()) {
      uint64_t mask = WORDTYPE_MAX >> (APINT_BITS_PER_WORD - subm_bitWidth);
      m_intValue.m_value &= ~(mask << bitPosition);
      m_intValue.m_value |= (subBits.m_intValue.m_value << bitPosition);
      return;
   }
   unsigned loBit = whichBit(bitPosition);
   unsigned loWord = whichWord(bitPosition);
   unsigned hi1Word = whichWord(bitPosition + subm_bitWidth - 1);
   // Insertion within a single word can be done as a direct bitmask.
   if (loWord == hi1Word) {
      uint64_t mask = WORDTYPE_MAX >> (APINT_BITS_PER_WORD - subm_bitWidth);
      m_intValue.m_pValue[loWord] &= ~(mask << loBit);
      m_intValue.m_pValue[loWord] |= (subBits.m_intValue.m_value << loBit);
      return;
   }
   // Insert on word boundaries.
   if (loBit == 0) {
      // Direct copy whole words.
      unsigned numWholeSubWords = subm_bitWidth / APINT_BITS_PER_WORD;
      memcpy(m_intValue.m_pValue + loWord, subBits.getRawData(),
             numWholeSubWords * APINT_WORD_SIZE);
      // Mask+insert remaining bits.
      unsigned remainingBits = subm_bitWidth % APINT_BITS_PER_WORD;
      if (remainingBits != 0) {
         uint64_t mask = WORDTYPE_MAX >> (APINT_BITS_PER_WORD - remainingBits);
         m_intValue.m_pValue[hi1Word] &= ~mask;
         m_intValue.m_pValue[hi1Word] |= subBits.getWord(subm_bitWidth - 1);
      }
      return;
   }
   // General case - set/clear individual bits in dst based on src.
   // TODO - there is scope for optimization here, but at the moment this code
   // path is barely used so prefer readability over performance.
   for (unsigned i = 0; i != subm_bitWidth; ++i) {
      if (subBits[i]) {
         setBit(bitPosition + i);
      } else {
         clearBit(bitPosition + i);
      }
   }
}
ApInt ApInt::extractBits(unsigned numBits, unsigned bitPosition) const
{
   assert(numBits > 0 && "Can't extract zero bits");
   assert(bitPosition < m_bitWidth && (numBits + bitPosition) <= m_bitWidth &&
          "Illegal bit extraction");
   if (isSingleWord()) {
      return ApInt(numBits, m_intValue.m_value >> bitPosition);
   }
   unsigned loBit = whichBit(bitPosition);
   unsigned loWord = whichWord(bitPosition);
   unsigned hiWord = whichWord(bitPosition + numBits - 1);
   // Single word result extracting bits from a single word source.
   if (loWord == hiWord) {
      return ApInt(numBits, m_intValue.m_pValue[loWord] >> loBit);
   }
   // Extracting bits that start on a source word boundary can be done
   // as a fast memory copy.
   if (loBit == 0) {
      return ApInt(numBits, make_array_ref(m_intValue.m_pValue + loWord, 1 + hiWord - loWord));
   }
   // General case - shift + copy source words directly into place.
   ApInt result(numBits, 0);
   unsigned numSrcWords = getNumWords();
   unsigned numDstWords = result.getNumWords();
   uint64_t *destPtr = result.isSingleWord() ? &result.m_intValue.m_value : result.m_intValue.m_pValue;
   for (unsigned word = 0; word < numDstWords; ++word) {
      uint64_t w0 = m_intValue.m_pValue[loWord + word];
      uint64_t w1 =
            (loWord + word + 1) < numSrcWords ? m_intValue.m_pValue[loWord + word + 1] : 0;
      destPtr[word] = (w0 >> loBit) | (w1 << (APINT_BITS_PER_WORD - loBit));
   }
   return result.clearUnusedBits();
}
unsigned ApInt::getBitsNeeded(StringRef str, uint8_t radix)
{
   assert(!str.empty() && "Invalid string length");
   assert((radix == 10 || radix == 8 || radix == 16 || radix == 2 ||
           radix == 36) &&
          "Radix should be 2, 8, 10, 16, or 36!");
   size_t slen = str.getSize();
   // Each computation below needs to know if it's negative.
   StringRef::iterator p = str.begin();
   unsigned isNegative = *p == '-';
   if (*p == '-' || *p == '+') {
      p++;
      slen--;
      assert(slen && "String is only a sign, needs a value.");
   }
   // For radixes of power-of-two values, the bits required is accurately and
   // easily computed
   if (radix == 2) {
      return slen + isNegative;
   }
   if (radix == 8) {
      return slen * 3 + isNegative;
   }
   if (radix == 16) {
      return slen * 4 + isNegative;
   }
   // FIXME: base 36
   // This is grossly inefficient but accurate. We could probably do something
   // with a computation of roughly slen*64/20 and then adjust by the value of
   // the first few digits. But, I'm not sure how accurate that could be.
   // Compute a sufficient number of bits that is always large enough but might
   // be too large. This avoids the assertion in the constructor. This
   // calculation doesn't work appropriately for the numbers 0-9, so just use 4
   // bits in that case.
   unsigned sufficient
         = radix == 10? (slen == 1 ? 4 : slen * 64/18)
                      : (slen == 1 ? 7 : slen * 16/3);
   // Convert to the actual binary value.
   ApInt tmp(sufficient, StringRef(p, slen), radix);
   // Compute how many bits are required. If the log is infinite, assume we need
   // just bit.
   unsigned log = tmp.logBase2();
   if (log == (unsigned)-1) {
      return isNegative + 1;
   } else {
      return isNegative + log + 1;
   }
}
HashCode hash_value(const ApInt &arg)
{
   if (arg.isSingleWord()) {
      return hash_combine(arg.m_intValue.m_value);
   }
   return hash_combine_range(arg.m_intValue.m_pValue, arg.m_intValue.m_pValue + arg.getNumWords());
}
bool ApInt::isSplat(unsigned splatSizeInBits) const
{
   assert(getBitWidth() % splatSizeInBits == 0 &&
          "splatSizeInBits must divide width!");
   // We can check that all parts of an integer are equal by making use of a
   // little trick: rotate and check if it's still the same value.
   return *this == rotl(splatSizeInBits);
}
/// This function returns the high "numBits" bits of this ApInt.
ApInt ApInt::getHiBits(unsigned numBits) const
{
   return this->lshr(m_bitWidth - numBits);
}
/// This function returns the low "numBits" bits of this ApInt.
ApInt ApInt::getLoBits(unsigned numBits) const
{
   ApInt result(getLowBitsSet(m_bitWidth, numBits));
   result &= *this;
   return result;
}
/// Return a value containing V broadcasted over newLen bits.
ApInt ApInt::getSplat(unsigned newLen, const ApInt &value)
{
   assert(newLen >= value.getBitWidth() && "Can't splat to smaller bit width!");
   ApInt retValue = value.zextOrSelf(newLen);
   for (unsigned index = value.getBitWidth(); index < newLen; index <<= 1) {
      retValue |= retValue << index;
   }
   return retValue;
}
unsigned ApInt::countLeadingZerosSlowCase() const
{
   unsigned count = 0;
   for (int i = getNumWords()-1; i >= 0; --i) {
      uint64_t value = m_intValue.m_pValue[i];
      if (value == 0) {
         count += APINT_BITS_PER_WORD;
      } else {
         count += count_leading_zeros(value);
         break;
      }
   }
   // Adjust for unused bits in the most significant word (they are zero).
   unsigned mod = m_bitWidth % APINT_BITS_PER_WORD;
   count -= mod > 0 ? APINT_BITS_PER_WORD - mod : 0;
   return count;
}
unsigned ApInt::countLeadingOnesSlowCase() const
{
   unsigned highWordBits = m_bitWidth % APINT_BITS_PER_WORD;
   unsigned shift;
   if (!highWordBits) {
      highWordBits = APINT_BITS_PER_WORD;
      shift = 0;
   } else {
      shift = APINT_BITS_PER_WORD - highWordBits;
   }
   int i = getNumWords() - 1;
   unsigned count = count_leading_ones(m_intValue.m_pValue[i] << shift);
   if (count == highWordBits) {
      for (i--; i >= 0; --i) {
         if (m_intValue.m_pValue[i] == WORDTYPE_MAX) {
            count += APINT_BITS_PER_WORD;
         } else {
            count += count_leading_ones(m_intValue.m_pValue[i]);
            break;
         }
      }
   }
   return count;
}
unsigned ApInt::countTrailingZerosSlowCase() const
{
   unsigned count = 0;
   unsigned i = 0;
   for (; i < getNumWords() && m_intValue.m_pValue[i] == 0; ++i) {
      count += APINT_BITS_PER_WORD;
   }
   if (i < getNumWords()) {
      count += count_trailing_zeros(m_intValue.m_pValue[i]);
   }
   return std::min(count, m_bitWidth);
}
unsigned ApInt::countTrailingOnesSlowCase() const
{
   unsigned count = 0;
   unsigned i = 0;
   for (; i < getNumWords() && m_intValue.m_pValue[i] == WORDTYPE_MAX; ++i) {
      count += APINT_BITS_PER_WORD;
   }
   if (i < getNumWords()) {
      count += count_trailing_ones(m_intValue.m_pValue[i]);
   }
   assert(count <= m_bitWidth);
   return count;
}
unsigned ApInt::countPopulationSlowCase() const
{
   unsigned count = 0;
   for (unsigned i = 0; i < getNumWords(); ++i) {
      count += count_population(m_intValue.m_pValue[i]);
   }
   return count;
}
bool ApInt::intersectsSlowCase(const ApInt &other) const
{
   for (unsigned i = 0, e = getNumWords(); i != e; ++i) {
      if ((m_intValue.m_pValue[i] & other.m_intValue.m_pValue[i]) != 0) {
         return true;
      }
   }
   return false;
}
bool ApInt::isSubsetOfSlowCase(const ApInt &other) const
{
   for (unsigned i = 0, e = getNumWords(); i != e; ++i) {
      if ((m_intValue.m_pValue[i] & ~other.m_intValue.m_pValue[i]) != 0) {
         return false;
      }
   }
   return true;
}
ApInt ApInt::byteSwap() const
{
   assert(m_bitWidth >= 16 && m_bitWidth % 16 == 0 && "Cannot byteswap!");
   if (m_bitWidth == 16) {
      return ApInt(m_bitWidth, byte_swap16(uint16_t(m_intValue.m_value)));
   }
   if (m_bitWidth == 32) {
      return ApInt(m_bitWidth, byte_swap32(unsigned(m_intValue.m_value)));
   }
   if (m_bitWidth == 48) {
      unsigned temp1 = unsigned(m_intValue.m_value >> 16);
      temp1 = byte_swap32(temp1);
      uint16_t temp2 = uint16_t(m_intValue.m_value);
      temp2 = byte_swap16(temp2);
      return ApInt(m_bitWidth, (uint64_t(temp2) << 32) | temp1);
   }
   if (m_bitWidth == 64) {
      return ApInt(m_bitWidth, byte_swap64(m_intValue.m_value));
   }
   ApInt result(getNumWords() * APINT_BITS_PER_WORD, 0);
   for (unsigned index = 0, num = getNumWords(); index != num; ++index) {
      result.m_intValue.m_pValue[index] = byte_swap64(m_intValue.m_pValue[num - index - 1]);
   }
   if (result.m_bitWidth != m_bitWidth) {
      result.lshrInPlace(result.m_bitWidth - m_bitWidth);
      result.m_bitWidth = m_bitWidth;
   }
   return result;
}
ApInt ApInt::reverseBits() const
{
   switch (m_bitWidth) {
   case 64:
      return ApInt(m_bitWidth, reverse_bits<uint64_t>(m_intValue.m_value));
   case 32:
      return ApInt(m_bitWidth, reverse_bits<uint32_t>(m_intValue.m_value));
   case 16:
      return ApInt(m_bitWidth, reverse_bits<uint16_t>(m_intValue.m_value));
   case 8:
      return ApInt(m_bitWidth, reverse_bits<uint8_t>(m_intValue.m_value));
   default:
      break;
   }
   ApInt val(*this);
   ApInt reversed(m_bitWidth, 0);
   unsigned s = m_bitWidth;
   for (; val != 0; val.lshrInPlace(1)) {
      reversed <<= 1;
      reversed |= val[0];
      --s;
   }
   reversed <<= s;
   return reversed;
}
ApInt apintops::greatest_common_divisor(ApInt lhs, ApInt rhs) {
   // Fast-path a common case.
   if (lhs == rhs) {
      return lhs;
   }
   // Corner cases: if either operand is zero, the other is the gcd.
   if (!lhs) {
      return rhs;
   }
   if (!rhs) {
      return lhs;
   }
   // Count common powers of 2 and remove all other powers of 2.
   unsigned pow2;
   {
      unsigned lhsPow2 = lhs.countTrailingZeros();
      unsigned rhsPow2 = rhs.countTrailingZeros();
      if (lhsPow2 > rhsPow2) {
         lhs.lshrInPlace(lhsPow2 - rhsPow2);
         pow2 = rhsPow2;
      } else if (rhsPow2 > lhsPow2) {
         rhs.lshrInPlace(rhsPow2 - lhsPow2);
         pow2 = lhsPow2;
      } else {
         pow2 = lhsPow2;
      }
   }
   // Both operands are odd multiples of 2^Pow_2:
   //
   //   gcd(a, b) = gcd(|a - b| / 2^i, min(a, b))
   //
   // This is a modified version of Stein's algorithm, taking advantage of
   // efficient countTrailingZeros().
   while (lhs != rhs) {
      if (lhs.ugt(rhs)) {
         lhs -= rhs;
         lhs.lshrInPlace(lhs.countTrailingZeros() - pow2);
      } else {
         rhs -= lhs;
         rhs.lshrInPlace(rhs.countTrailingZeros() - pow2);
      }
   }
   return lhs;
}
ApInt apintops::round_double_to_apint(double doubleValue, unsigned width)
{
   uint64_t temp = bit_cast<uint64_t>(doubleValue);
   // Get the sign bit from the highest order bit
   bool isNeg = temp >> 63;
   // Get the 11-bit exponent and adjust for the 1023 bit bias
   int64_t exp = ((temp >> 52) & 0x7ff) - 1023;
   // If the exponent is negative, the value is < 0 so just return 0.
   if (exp < 0) {
      return ApInt(width, 0u);
   }
   // Extract the mantissa by clearing the top 12 bits (sign + exponent).
   uint64_t mantissa = (temp & (~0ULL >> 12)) | 1ULL << 52;
   // If the exponent doesn't shift all bits out of the mantissa
   if (exp < 52) {
      return isNeg ? -ApInt(width, mantissa >> (52 - exp)) :
                     ApInt(width, mantissa >> (52 - exp));
   }
   // If the client didn't provide enough bits for us to shift the mantissa into
   // then the result is undefined, just return 0
   if (width <= exp - 52) {
      return ApInt(width, 0);
   }
   // Otherwise, we have to shift the mantissa bits up to the right location
   ApInt ret(width, mantissa);
   ret <<= (unsigned)exp - 52;
   return isNeg ? -ret : ret;
}
/// This function converts this ApInt to a double.
/// The layout for double is as following (IEEE Standard 754):
///  --------------------------------------
/// |  Sign    Exponent    Fraction    Bias |
/// |-------------------------------------- |
/// |  1[63]   11[62-52]   52[51-00]   1023 |
///  --------------------------------------
double ApInt::roundToDouble(bool isSigned) const
{
   // Handle the simple case where the value is contained in one uint64_t.
   // It is wrong to optimize getWord(0) to VAL; there might be more than one word.
   if (isSingleWord() || getActiveBits() <= APINT_BITS_PER_WORD) {
      if (isSigned) {
         int64_t sext = sign_extend64(getWord(0), m_bitWidth);
         return double(sext);
      } else
         return double(getWord(0));
   }
   // Determine if the value is negative.
   bool isNeg = isSigned ? (*this)[m_bitWidth-1] : false;
   // Construct the absolute value if we're negative.
   ApInt temp(isNeg ? -(*this) : (*this));
   // Figure out how many bits we're using.
   unsigned n = temp.getActiveBits();
   // The exponent (without bias normalization) is just the number of bits
   // we are using. Note that the sign bit is gone since we constructed the
   // absolute value.
   uint64_t exp = n;
   // Return infinity for exponent overflow
   if (exp > 1023) {
      if (!isSigned || !isNeg)
         return std::numeric_limits<double>::infinity();
      else
         return -std::numeric_limits<double>::infinity();
   }
   exp += 1023; // Increment for 1023 bias
   // Number of bits in mantissa is 52. To obtain the mantissa value, we must
   // extract the high 52 bits from the correct words in pVal.
   uint64_t mantissa;
   unsigned hiWord = whichWord(n-1);
   if (hiWord == 0) {
      mantissa = temp.m_intValue.m_pValue[0];
      if (n > 52)
         mantissa >>= n - 52; // shift down, we want the top 52 bits.
   } else {
      assert(hiWord > 0 && "huh?");
      uint64_t hibits = temp.m_intValue.m_pValue[hiWord] << (52 - n % APINT_BITS_PER_WORD);
      uint64_t lobits = temp.m_intValue.m_pValue[hiWord-1] >> (11 + n % APINT_BITS_PER_WORD);
      mantissa = hibits | lobits;
   }
   // The leading bit of mantissa is implicit, so get rid of it.
   uint64_t sign = isNeg ? (1ULL << (APINT_BITS_PER_WORD - 1)) : 0;

   uint64_t ret = sign | (exp << 52) | mantissa;
   return bit_cast<double>(ret);
}

// Truncate to new width.
ApInt ApInt::trunc(unsigned width) const
{
   assert(width < m_bitWidth && "Invalid ApInt Truncate request");
   assert(width && "Can't truncate to 0 bits");
   if (width <= APINT_BITS_PER_WORD) {
      return ApInt(width, getRawData()[0]);
   }
   ApInt result(get_memory(getNumWords(width)), width);
   // Copy full words.
   unsigned i;
   for (i = 0; i != width / APINT_BITS_PER_WORD; i++) {
      result.m_intValue.m_pValue[i] = m_intValue.m_pValue[i];
   }
   // Truncate and copy any partial word.
   unsigned bits = (0 - width) % APINT_BITS_PER_WORD;
   if (bits != 0) {
      result.m_intValue.m_pValue[i] = m_intValue.m_pValue[i] << bits >> bits;
   }
   return result;
}
// Sign extend to a new width.
ApInt ApInt::sext(unsigned width) const
{
   assert(width > m_bitWidth && "Invalid ApInt SignExtend request");
   if (width <= APINT_BITS_PER_WORD) {
      return ApInt(width, sign_extend64(m_intValue.m_value, m_bitWidth));
   }
   ApInt result(get_memory(getNumWords(width)), width);
   // Copy words.
   std::memcpy(result.m_intValue.m_pValue, getRawData(), getNumWords() * APINT_WORD_SIZE);
   // Sign extend the last word since there may be unused bits in the input.
   result.m_intValue.m_pValue[getNumWords() - 1] =
         sign_extend64(result.m_intValue.m_pValue[getNumWords() - 1],
         ((m_bitWidth - 1) % APINT_BITS_PER_WORD) + 1);
   // Fill with sign bits.
   std::memset(result.m_intValue.m_pValue + getNumWords(), isNegative() ? -1 : 0,
               (result.getNumWords() - getNumWords()) * APINT_WORD_SIZE);
   result.clearUnusedBits();
   return result;
}
//  Zero extend to a new width.
ApInt ApInt::zext(unsigned width) const
{
   assert(width > m_bitWidth && "Invalid ApInt ZeroExtend request");
   if (width <= APINT_BITS_PER_WORD) {
      return ApInt(width, m_intValue.m_value);
   }
   ApInt result(get_memory(getNumWords(width)), width);
   // Copy words.
   std::memcpy(result.m_intValue.m_pValue, getRawData(), getNumWords() * APINT_WORD_SIZE);
   // Zero remaining words.
   std::memset(result.m_intValue.m_pValue + getNumWords(), 0,
               (result.getNumWords() - getNumWords()) * APINT_WORD_SIZE);
   return result;
}
ApInt ApInt::zextOrTrunc(unsigned width) const
{
   if (m_bitWidth < width) {
      return zext(width);
   }
   if (m_bitWidth > width) {
      return trunc(width);
   }
   return *this;
}
ApInt ApInt::sextOrTrunc(unsigned width) const
{
   if (m_bitWidth < width) {
      return sext(width);
   }
   if (m_bitWidth > width) {
      return trunc(width);
   }
   return *this;
}
ApInt ApInt::zextOrSelf(unsigned width) const
{
   if (m_bitWidth < width) {
      return zext(width);
   }
   return *this;
}
ApInt ApInt::sextOrSelf(unsigned width) const
{
   if (m_bitWidth < width) {
      return sext(width);
   }
   return *this;
}
/// Arithmetic right-shift this ApInt by shiftAmt.
/// @brief Arithmetic right-shift function.
void ApInt::ashrInPlace(const ApInt &shiftAmt)
{
   ashrInPlace((unsigned)shiftAmt.getLimitedValue(m_bitWidth));
}
/// Arithmetic right-shift this ApInt by shiftAmt.
/// @brief Arithmetic right-shift function.
void ApInt::ashrSlowCase(unsigned shiftAmt)
{
   // Don't bother performing a no-op shift.
   if (!shiftAmt) {
      return;
   }
   // Save the original sign bit for later.
   bool negative = isNegative();
   // wordShift is the inter-part shift; BitShift is is intra-part shift.
   unsigned wordShift = shiftAmt / APINT_BITS_PER_WORD;
   unsigned bitShift = shiftAmt % APINT_BITS_PER_WORD;
   unsigned wordsToMove = getNumWords() - wordShift;
   if (wordsToMove != 0) {
      // Sign extend the last word to fill in the unused bits.
      m_intValue.m_pValue[getNumWords() - 1] = sign_extend64(
               m_intValue.m_pValue[getNumWords() - 1], ((m_bitWidth - 1) % APINT_BITS_PER_WORD) + 1);
      // Fastpath for moving by whole words.
      if (bitShift == 0) {
         std::memmove(m_intValue.m_pValue, m_intValue.m_pValue + wordShift, wordsToMove * APINT_WORD_SIZE);
      } else {
         // Move the words containing significant bits.
         for (unsigned i = 0; i != wordsToMove - 1; ++i)
            m_intValue.m_pValue[i] = (m_intValue.m_pValue[i + wordShift] >> bitShift) |
                  (m_intValue.m_pValue[i + wordShift + 1] << (APINT_BITS_PER_WORD - bitShift));
         // Handle the last word which has no high bits to copy.
         m_intValue.m_pValue[wordsToMove - 1] = m_intValue.m_pValue[wordShift + wordsToMove - 1] >> bitShift;
         // Sign extend one more time.
         m_intValue.m_pValue[wordsToMove - 1] =
               sign_extend64(m_intValue.m_pValue[wordsToMove - 1], APINT_BITS_PER_WORD - bitShift);
      }
   }
   // Fill in the remainder based on the original sign.
   std::memset(m_intValue.m_pValue + wordsToMove, negative ? -1 : 0,
               wordShift * APINT_WORD_SIZE);
   clearUnusedBits();
}
/// Logical right-shift this ApInt by shiftAmt.
/// @brief Logical right-shift function.
void ApInt::lshrInPlace(const ApInt &shiftAmt)
{
   lshrInPlace((unsigned)shiftAmt.getLimitedValue(m_bitWidth));
}
/// Logical right-shift this ApInt by shiftAmt.
/// @brief Logical right-shift function.
void ApInt::lshrSlowCase(unsigned shiftAmt)
{
   tcShiftRight(m_intValue.m_pValue, getNumWords(), shiftAmt);
}
/// Left-shift this ApInt by shiftAmt.
/// @brief Left-shift function.
ApInt &ApInt::operator<<=(const ApInt &shiftAmt)
{
   // It's undefined behavior in C to shift by m_bitWidth or greater.
   *this <<= (unsigned)shiftAmt.getLimitedValue(m_bitWidth);
   return *this;
}
void ApInt::shlSlowCase(unsigned shiftAmt)
{
   tcShiftLeft(m_intValue.m_pValue, getNumWords(), shiftAmt);
   clearUnusedBits();
}
namespace {
// Calculate the rotate amount modulo the bit width.
unsigned rotate_modulo(unsigned bitWidth, const ApInt &rotateAmt)
{
   unsigned rotm_bitWidth = rotateAmt.getBitWidth();
   ApInt rot = rotateAmt;
   if (rotm_bitWidth < bitWidth) {
      // Extend the rotate ApInt, so that the urem doesn't divide by 0.
      // e.g. ApInt(1, 32) would give ApInt(1, 0).
      rot = rotateAmt.zext(bitWidth);
   }
   rot = rot.urem(ApInt(rot.getBitWidth(), bitWidth));
   return rot.getLimitedValue(bitWidth);
}
} // anonymous namespace
ApInt ApInt::rotl(const ApInt &rotateAmt) const
{
   return rotl(rotate_modulo(m_bitWidth, rotateAmt));
}
ApInt ApInt::rotl(unsigned rotateAmt) const
{
   rotateAmt %= m_bitWidth;
   if (rotateAmt == 0) {
      return *this;
   }
   return shl(rotateAmt) | lshr(m_bitWidth - rotateAmt);
}
ApInt ApInt::rotr(const ApInt &rotateAmt) const
{
   return rotr(rotate_modulo(m_bitWidth, rotateAmt));
}
ApInt ApInt::rotr(unsigned rotateAmt) const
{
   rotateAmt %= m_bitWidth;
   if (rotateAmt == 0) {
      return *this;
   }
   return lshr(rotateAmt) | shl(m_bitWidth - rotateAmt);
}
// Square Root - this method computes and returns the square root of "this".
// Three mechanisms are used for computation. For small values (<= 5 bits),
// a table lookup is done. This gets some performance for common cases. For
// values using less than 52 bits, the value is converted to double and then
// the libc sqrt function is called. The result is rounded and then converted
// back to a uint64_t which is then used to construct the result. Finally,
// the Babylonian method for computing square roots is used.
ApInt ApInt::sqrt() const
{
   // Determine the magnitude of the value.
   unsigned magnitude = getActiveBits();
   // Use a fast table for some small values. This also gets rid of some
   // rounding errors in libc sqrt for small values.
   if (magnitude <= 5) {
      static const uint8_t results[32] = {
         /*     0 */ 0,
         /*  1- 2 */ 1, 1,
         /*  3- 6 */ 2, 2, 2, 2,
         /*  7-12 */ 3, 3, 3, 3, 3, 3,
         /* 13-20 */ 4, 4, 4, 4, 4, 4, 4, 4,
         /* 21-30 */ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
         /*    31 */ 6
      };
      return ApInt(m_bitWidth, results[ (isSingleWord() ? m_intValue.m_value : m_intValue.m_pValue[0]) ]);
   }
   // If the magnitude of the value fits in less than 52 bits (the precision of
   // an IEEE double precision floating point value), then we can use the
   // libc sqrt function which will probably use a hardware sqrt computation.
   // This should be faster than the algorithm below.
   if (magnitude < 52) {
      return ApInt(m_bitWidth,
                   uint64_t(::round(::sqrt(double(isSingleWord() ? m_intValue.m_value
                                                                 : m_intValue.m_pValue[0])))));
   }
   // Okay, all the short cuts are exhausted. We must compute it. The following
   // is a classical Babylonian method for computing the square root. This code
   // was adapted to ApInt from a wikipedia article on such computations.
   // See http://www.wikipedia.org/ and go to the page named
   // Calculate_an_integer_square_root.
   unsigned nbits = m_bitWidth, i = 4;
   ApInt testy(m_bitWidth, 16);
   ApInt x_old(m_bitWidth, 1);
   ApInt x_new(m_bitWidth, 0);
   ApInt two(m_bitWidth, 2);
   // Select a good starting value using binary logarithms.
   for (;; i += 2, testy = testy.shl(2))
      if (i >= nbits || this->ule(testy)) {
         x_old = x_old.shl(i / 2);
         break;
      }
   // Use the Babylonian method to arrive at the integer square root:
   for (;;) {
      x_new = (this->udiv(x_old) + x_old).udiv(two);
      if (x_old.ule(x_new)) {
         break;
      }
      x_old = x_new;
   }
   // Make sure we return the closest approximation
   // NOTE: The rounding calculation below is correct. It will produce an
   // off-by-one discrepancy with results from pari/gp. That discrepancy has been
   // determined to be a rounding issue with pari/gp as it begins to use a
   // floating point representation after 192 bits. There are no discrepancies
   // between this algorithm and pari/gp for bit widths < 192 bits.
   ApInt square(x_old * x_old);
   ApInt nextSquare((x_old + 1) * (x_old +1));
   if (this->ult(square)) {
      return x_old;
   }
   assert(this->ule(nextSquare) && "Error in ApInt::sqrt computation");
   ApInt midpoint((nextSquare - square).udiv(two));
   ApInt offset(*this - square);
   if (offset.ult(midpoint)) {
      return x_old;
   }
   return x_old + 1;
}
/// Computes the multiplicative inverse of this ApInt for a given modulo. The
/// iterative extended Euclidean algorithm is used to solve for this value,
/// however we simplify it to speed up calculating only the inverse, and take
/// advantage of div+rem calculations. We also use some tricks to avoid copying
/// (potentially large) ApInts around.
ApInt ApInt::multiplicativeInverse(const ApInt& modulo) const
{
   assert(ult(modulo) && "This ApInt must be smaller than the modulo");
   // Using the properties listed at the following web page (accessed 06/21/08):
   //   http://www.numbertheory.org/php/euclid.html
   // (especially the properties numbered 3, 4 and 9) it can be proved that
   // m_bitWidth bits suffice for all the computations in the algorithm implemented
   // below. More precisely, this number of bits suffice if the multiplicative
   // inverse exists, but may not suffice for the general extended Euclidean
   // algorithm.
   ApInt r[2] = { modulo, *this };
   ApInt t[2] = { ApInt(m_bitWidth, 0), ApInt(m_bitWidth, 1) };
   ApInt q(m_bitWidth, 0);
   unsigned i;
   for (i = 0; r[i^1] != 0; i ^= 1) {
      // An overview of the math without the confusing bit-flipping:
      // q = r[i-2] / r[i-1]
      // r[i] = r[i-2] % r[i-1]
      // t[i] = t[i-2] - t[i-1] * q
      udivrem(r[i], r[i^1], q, r[i]);
      t[i] -= t[i^1] * q;
   }
   // If this ApInt and the modulo are not coprime, there is no multiplicative
   // inverse, so return 0. We check this by looking at the next-to-last
   // remainder, which is the gcd(*this,modulo) as calculated by the Euclidean
   // algorithm.
   if (r[i] != 1) {
      return ApInt(m_bitWidth, 0);
   }
   // The next-to-last t is the multiplicative inverse.  However, we are
   // interested in a positive inverse. Calculate a positive one from a negative
   // one if necessary. A simple addition of the modulo suffices because
   // abs(t[i]) is known to be less than *this/2 (see the link above).
   if (t[i].isNegative()) {
      t[i] += modulo;
   }
   return std::move(t[i]);
}
/// Calculate the magic numbers required to implement a signed integer division
/// by a constant as a sequence of multiplies, adds and shifts.  Requires that
/// the divisor not be 0, 1, or -1.  Taken from "Hacker's Delight", Henry S.
/// Warren, Jr., chapter 10.
ApInt::MagicSign ApInt::getMagic() const
{
   const ApInt& d = *this;
   unsigned p;
   ApInt ad, anc, delta, q1, r1, q2, r2, t;
   ApInt signedMin = ApInt::getSignedMinValue(d.getBitWidth());
   struct MagicSign mag;
   ad = d.abs();
   t = signedMin + (d.lshr(d.getBitWidth() - 1));
   anc = t - 1 - t.urem(ad);   // absolute value of nc
   p = d.getBitWidth() - 1;    // initialize p
   q1 = signedMin.udiv(anc);   // initialize q1 = 2p/abs(nc)
   r1 = signedMin - q1*anc;    // initialize r1 = rem(2p,abs(nc))
   q2 = signedMin.udiv(ad);    // initialize q2 = 2p/abs(d)
   r2 = signedMin - q2*ad;     // initialize r2 = rem(2p,abs(d))
   do {
      p = p + 1;
      q1 = q1<<1;          // update q1 = 2p/abs(nc)
      r1 = r1<<1;          // update r1 = rem(2p/abs(nc))
      if (r1.uge(anc)) {  // must be unsigned comparison
         q1 = q1 + 1;
         r1 = r1 - anc;
      }
      q2 = q2<<1;          // update q2 = 2p/abs(d)
      r2 = r2<<1;          // update r2 = rem(2p/abs(d))
      if (r2.uge(ad)) {   // must be unsigned comparison
         q2 = q2 + 1;
         r2 = r2 - ad;
      }
      delta = ad - r2;
   } while (q1.ult(delta) || (q1 == delta && r1 == 0));
   mag.m_magic = q2 + 1;
   if (d.isNegative()) {
      mag.m_magic = -mag.m_magic;   // resulting magic number
   }
   mag.m_shift = p - d.getBitWidth();          // resulting shift
   return mag;
}
/// Calculate the magic numbers required to implement an unsigned integer
/// division by a constant as a sequence of multiplies, adds and shifts.
/// Requires that the divisor not be 0.  Taken from "Hacker's Delight", Henry
/// S. Warren, Jr., chapter 10.
/// LeadingZeros can be used to simplify the calculation if the upper bits
/// of the divided value are known zero.
ApInt::MagicUnsign ApInt::getMagicUnsign(unsigned leadingZeros) const
{
   const ApInt& d = *this;
   unsigned p;
   ApInt nc, delta, q1, r1, q2, r2;
   struct MagicUnsign magu;
   magu.m_addIndicator = 0;               // initialize "add" indicator
   ApInt allOnes = ApInt::getAllOnesValue(d.getBitWidth()).lshr(leadingZeros);
   ApInt signedMin = ApInt::getSignedMinValue(d.getBitWidth());
   ApInt signedMax = ApInt::getSignedMaxValue(d.getBitWidth());
   nc = allOnes - (allOnes - d).urem(d);
   p = d.getBitWidth() - 1;  // initialize p
   q1 = signedMin.udiv(nc);  // initialize q1 = 2p/nc
   r1 = signedMin - q1*nc;   // initialize r1 = rem(2p,nc)
   q2 = signedMax.udiv(d);   // initialize q2 = (2p-1)/d
   r2 = signedMax - q2*d;    // initialize r2 = rem((2p-1),d)
   do {
      p = p + 1;
      if (r1.uge(nc - r1)) {
         q1 = q1 + q1 + 1;  // update q1
         r1 = r1 + r1 - nc; // update r1
      }
      else {
         q1 = q1+q1; // update q1
         r1 = r1+r1; // update r1
      }
      if ((r2 + 1).uge(d - r2)) {
         if (q2.uge(signedMax)) magu.m_addIndicator = 1;
         q2 = q2+q2 + 1;     // update q2
         r2 = r2+r2 + 1 - d; // update r2
      }
      else {
         if (q2.uge(signedMin)) magu.m_addIndicator = 1;
         q2 = q2+q2;     // update q2
         r2 = r2+r2 + 1; // update r2
      }
      delta = d - 1 - r2;
   } while (p < d.getBitWidth()*2 &&
            (q1.ult(delta) || (q1 == delta && r1 == 0)));
   magu.m_magic = q2 + 1; // resulting magic number
   magu.m_shift = p - d.getBitWidth();  // resulting shift
   return magu;
}

namespace {
/// Implementation of Knuth's Algorithm D (Division of nonnegative integers)
/// from "Art of Computer Programming, Volume 2", section 4.3.1, p. 272. The
/// variables here have the same names as in the algorithm. Comments explain
/// the algorithm and any deviation from it.
void knuth_div(uint32_t *u, uint32_t *v, uint32_t *q, uint32_t* r,
               unsigned m, unsigned n)
{
   assert(u && "Must provide dividend");
   assert(v && "Must provide divisor");
   assert(q && "Must provide quotient");
   assert(u != v && u != q && v != q && "Must use different memory");
   assert(n>1 && "n must be > 1");
   // b denotes the base of the number system. In our case b is 2^32.
   const uint64_t b = uint64_t(1) << 32;
   // The DEBUG macros here tend to be spam in the debug output if you're not
   // debugging this code. Disable them unless KNUTH_DEBUG is defined.
#ifdef KNUTH_DEBUG
#define DEBUG_KNUTH(X) POLAR_DEBUG(X)
#else
#define DEBUG_KNUTH(X) do {} while(false)
#endif
   DEBUG_KNUTH(debug_stream() << "knuth_div: m=" << m << " n=" << n << '\n');
   DEBUG_KNUTH(debug_stream() << "knuth_div: original:");
   DEBUG_KNUTH(for (int i = m+n; i >=0; i--) debug_stream() << " " << u[i]);
   DEBUG_KNUTH(debug_stream() << " by");
   DEBUG_KNUTH(for (int i = n; i >0; i--) debug_stream() << " " << v[i-1]);
   DEBUG_KNUTH(debug_stream() << '\n');
   // D1. [Normalize.] Set d = b / (v[n-1] + 1) and multiply all the digits of
   // u and v by d. Note that we have taken Knuth's advice here to use a power
   // of 2 value for d such that d * v[n-1] >= b/2 (b is the base). A power of
   // 2 allows us to shift instead of multiply and it is easy to determine the
   // shift amount from the leading zeros.  We are basically normalizing the u
   // and v so that its high bits are shifted to the top of v's range without
   // overflow. Note that this can require an extra word in u so that u must
   // be of length m+n+1.
   unsigned shift = count_leading_zeros(v[n-1]);
   uint32_t v_carry = 0;
   uint32_t u_carry = 0;
   if (shift) {
      for (unsigned i = 0; i < m+n; ++i) {
         uint32_t u_tmp = u[i] >> (32 - shift);
         u[i] = (u[i] << shift) | u_carry;
         u_carry = u_tmp;
      }
      for (unsigned i = 0; i < n; ++i) {
         uint32_t v_tmp = v[i] >> (32 - shift);
         v[i] = (v[i] << shift) | v_carry;
         v_carry = v_tmp;
      }
   }
   u[m+n] = u_carry;
   DEBUG_KNUTH(debug_stream() << "knuth_div:   normal:");
   DEBUG_KNUTH(for (int i = m+n; i >=0; i--) debug_stream() << " " << u[i]);
   DEBUG_KNUTH(debug_stream() << " by");
   DEBUG_KNUTH(for (int i = n; i >0; i--) debug_stream() << " " << v[i-1]);
   DEBUG_KNUTH(debug_stream() << '\n');
   // D2. [Initialize j.]  Set j to m. This is the loop counter over the places.
   int j = m;
   do {
      DEBUG_KNUTH(debug_stream() << "knuth_div: quotient digit #" << j << '\n');
      // D3. [Calculate q'.].
      //     Set qp = (u[j+n]*b + u[j+n-1]) / v[n-1]. (qp=qprime=q')
      //     Set rp = (u[j+n]*b + u[j+n-1]) % v[n-1]. (rp=rprime=r')
      // Now test if qp == b or qp*v[n-2] > b*rp + u[j+n-2]; if so, decrease
      // qp by 1, increase rp by v[n-1], and repeat this test if rp < b. The test
      // on v[n-2] determines at high speed most of the cases in which the trial
      // value qp is one too large, and it eliminates all cases where qp is two
      // too large.
      uint64_t dividend = make64(u[j+n], u[j+n-1]);
      DEBUG_KNUTH(debug_stream() << "knuth_div: dividend == " << dividend << '\n');
      uint64_t qp = dividend / v[n-1];
      uint64_t rp = dividend % v[n-1];
      if (qp == b || qp*v[n-2] > b*rp + u[j+n-2]) {
         qp--;
         rp += v[n-1];
         if (rp < b && (qp == b || qp*v[n-2] > b*rp + u[j+n-2]))
            qp--;
      }
      DEBUG_KNUTH(debug_stream() << "knuth_div: qp == " << qp << ", rp == " << rp << '\n');
      // D4. [Multiply and subtract.] Replace (u[j+n]u[j+n-1]...u[j]) with
      // (u[j+n]u[j+n-1]..u[j]) - qp * (v[n-1]...v[1]v[0]). This computation
      // consists of a simple multiplication by a one-place number, combined with
      // a subtraction.
      // The digits (u[j+n]...u[j]) should be kept positive; if the result of
      // this step is actually negative, (u[j+n]...u[j]) should be left as the
      // true value plus b**(n+1), namely as the b's complement of
      // the true value, and a "borrow" to the left should be remembered.
      int64_t borrow = 0;
      for (unsigned i = 0; i < n; ++i) {
         uint64_t p = uint64_t(qp) * uint64_t(v[i]);
         int64_t subres = int64_t(u[j+i]) - borrow - low32(p);
         u[j+i] = low32(subres);
         borrow = high32(p) - high32(subres);
         DEBUG_KNUTH(debug_stream() << "knuth_div: u[j+i] = " << u[j+i]
               << ", borrow = " << borrow << '\n');
      }
      bool isNeg = u[j+n] < borrow;
      u[j+n] -= low32(borrow);
      DEBUG_KNUTH(debug_stream() << "knuth_div: after subtraction:");
      DEBUG_KNUTH(for (int i = m+n; i >=0; i--) debug_stream() << " " << u[i]);
      DEBUG_KNUTH(debug_stream() << '\n');
      // D5. [Test remainder.] Set q[j] = qp. If the result of step D4 was
      // negative, go to step D6; otherwise go on to step D7.
      q[j] = low32(qp);
      if (isNeg) {
         // D6. [Add back]. The probability that this step is necessary is very
         // small, on the order of only 2/b. Make sure that test data accounts for
         // this possibility. Decrease q[j] by 1
         q[j]--;
         // and add (0v[n-1]...v[1]v[0]) to (u[j+n]u[j+n-1]...u[j+1]u[j]).
         // A carry will occur to the left of u[j+n], and it should be ignored
         // since it cancels with the borrow that occurred in D4.
         bool carry = false;
         for (unsigned i = 0; i < n; i++) {
            uint32_t limit = std::min(u[j+i],v[i]);
            u[j+i] += v[i] + carry;
            carry = u[j+i] < limit || (carry && u[j+i] == limit);
         }
         u[j+n] += carry;
      }
      DEBUG_KNUTH(debug_stream() << "knuth_div: after correction:");
      DEBUG_KNUTH(for (int i = m+n; i >=0; i--) debug_stream() << " " << u[i]);
      DEBUG_KNUTH(debug_stream() << "\nknuth_div: digit result = " << q[j] << '\n');
      // D7. [Loop on j.]  Decrease j by one. Now if j >= 0, go back to D3.
   } while (--j >= 0);
   DEBUG_KNUTH(debug_stream() << "knuth_div: quotient:");
   DEBUG_KNUTH(for (int i = m; i >=0; i--) debug_stream() <<" " << q[i]);
   DEBUG_KNUTH(debug_stream() << '\n');
   // D8. [Unnormalize]. Now q[...] is the desired quotient, and the desired
   // remainder may be obtained by dividing u[...] by d. If r is non-null we
   // compute the remainder (urem uses this).
   if (r) {
      // The value d is expressed by the "shift" value above since we avoided
      // multiplication by d by using a shift left. So, all we have to do is
      // shift right here.
      if (shift) {
         uint32_t carry = 0;
         DEBUG_KNUTH(debug_stream() << "knuth_div: remainder:");
         for (int i = n-1; i >= 0; i--) {
            r[i] = (u[i] >> shift) | carry;
            carry = u[i] << (32 - shift);
            DEBUG_KNUTH(debug_stream() << " " << r[i]);
         }
      } else {
         for (int i = n-1; i >= 0; i--) {
            r[i] = u[i];
            DEBUG_KNUTH(debug_stream() << " " << r[i]);
         }
      }
      DEBUG_KNUTH(debug_stream() << '\n');
   }
   DEBUG_KNUTH(debug_stream() << '\n');
}
} // anonymous namespace

void ApInt::divide(const WordType *lhs, unsigned lhsWords, const WordType *rhs,
                   unsigned rhsWords, WordType *quotient, WordType *remainder)
{
   assert(lhsWords >= rhsWords && "Fractional result");
   // First, compose the values into an array of 32-bit words instead of
   // 64-bit words. This is a necessity of both the "short division" algorithm
   // and the Knuth "classical algorithm" which requires there to be native
   // operations for +, -, and * on an m bit value with an m*2 bit result. We
   // can't use 64-bit operands here because we don't have native results of
   // 128-bits. Furthermore, casting the 64-bit values to 32-bit values won't
   // work on large-endian machines.
   unsigned n = rhsWords * 2;
   unsigned m = (lhsWords * 2) - n;
   // Allocate space for the temporary values we need either on the stack, if
   // it will fit, or on the heap if it won't.
   uint32_t space[128];
   uint32_t *U = nullptr;
   uint32_t *V = nullptr;
   uint32_t *Q = nullptr;
   uint32_t *R = nullptr;
   if ((remainder?4:3)*n+2*m+1 <= 128) {
      U = &space[0];
      V = &space[m+n+1];
      Q = &space[(m+n+1) + n];
      if (remainder)
         R = &space[(m+n+1) + n + (m+n)];
   } else {
      U = new uint32_t[m + n + 1];
      V = new uint32_t[n];
      Q = new uint32_t[m+n];
      if (remainder)
         R = new uint32_t[n];
   }
   // Initialize the dividend
   memset(U, 0, (m+n+1)*sizeof(uint32_t));
   for (unsigned i = 0; i < lhsWords; ++i) {
      uint64_t tmp = lhs[i];
      U[i * 2] = low32(tmp);
      U[i * 2 + 1] = high32(tmp);
   }
   U[m+n] = 0; // this extra word is for "spill" in the Knuth algorithm.
   // Initialize the divisor
   memset(V, 0, (n)*sizeof(uint32_t));
   for (unsigned i = 0; i < rhsWords; ++i) {
      uint64_t tmp = rhs[i];
      V[i * 2] = low32(tmp);
      V[i * 2 + 1] = high32(tmp);
   }
   // initialize the quotient and remainder
   memset(Q, 0, (m+n) * sizeof(uint32_t));
   if (remainder) {
      memset(R, 0, n * sizeof(uint32_t));
   }
   // Now, adjust m and n for the Knuth division. n is the number of words in
   // the divisor. m is the number of words by which the dividend exceeds the
   // divisor (i.e. m+n is the length of the dividend). These sizes must not
   // contain any zero words or the Knuth algorithm fails.
   for (unsigned i = n; i > 0 && V[i-1] == 0; i--) {
      n--;
      m++;
   }
   for (unsigned i = m+n; i > 0 && U[i-1] == 0; i--) {
      m--;
   }
   // If we're left with only a single word for the divisor, Knuth doesn't work
   // so we implement the short division algorithm here. This is much simpler
   // and faster because we are certain that we can divide a 64-bit quantity
   // by a 32-bit quantity at hardware speed and short division is simply a
   // series of such operations. This is just like doing short division but we
   // are using base 2^32 instead of base 10.
   assert(n != 0 && "Divide by zero?");
   if (n == 1) {
      uint32_t divisor = V[0];
      uint32_t remainder = 0;
      for (int i = m; i >= 0; i--) {
         uint64_t partial_dividend = make64(remainder, U[i]);
         if (partial_dividend == 0) {
            Q[i] = 0;
            remainder = 0;
         } else if (partial_dividend < divisor) {
            Q[i] = 0;
            remainder = low32(partial_dividend);
         } else if (partial_dividend == divisor) {
            Q[i] = 1;
            remainder = 0;
         } else {
            Q[i] = low32(partial_dividend / divisor);
            remainder = low32(partial_dividend - (Q[i] * divisor));
         }
      }
      if (R) {
         R[0] = remainder;
      }
   } else {
      // Now we're ready to invoke the Knuth classical divide algorithm. In this
      // case n > 1.
      knuth_div(U, V, Q, R, m, n);
   }
   // If the caller wants the quotient
   if (quotient) {
      for (unsigned i = 0; i < lhsWords; ++i) {
         quotient[i] = make64(Q[i*2+1], Q[i*2]);
      }
   }
   // If the caller wants the remainder
   if (remainder) {
      for (unsigned i = 0; i < rhsWords; ++i) {
         remainder[i] = make64(R[i*2+1], R[i*2]);
      }
   }
   // Clean up the memory we allocated.
   if (U != &space[0]) {
      delete [] U;
      delete [] V;
      delete [] Q;
      delete [] R;
   }
}
ApInt ApInt::udiv(const ApInt &other) const
{
   assert(m_bitWidth == other.m_bitWidth && "Bit widths must be the same");
   // First, deal with the easy case
   if (isSingleWord()) {
      assert(other.m_intValue.m_value != 0 && "Divide by zero?");
      return ApInt(m_bitWidth, m_intValue.m_value / other.m_intValue.m_value);
   }
   // Get some facts about the LHS and rhs number of bits and words
   unsigned lhsWords = getNumWords(getActiveBits());
   unsigned rhsBits  = other.getActiveBits();
   unsigned rhsWords = getNumWords(rhsBits);
   assert(rhsWords && "Divided by zero???");
   // Deal with some degenerate cases
   if (!lhsWords) {
      // 0 / X ===> 0
      return ApInt(m_bitWidth, 0);
   }
   if (rhsBits == 1) {
      // X / 1 ===> X
      return *this;
   }
   if (lhsWords < rhsWords || this->ult(other)) {
      // X / Y ===> 0, iff X < Y
      return ApInt(m_bitWidth, 0);
   }
   if (*this == other) {
      // X / X ===> 1
      return ApInt(m_bitWidth, 1);
   }
   if (lhsWords == 1) {// rhsWords is 1 if lhsWords is 1.
      // All high words are zero, just use native divide
      return ApInt(m_bitWidth, this->m_intValue.m_pValue[0] / other.m_intValue.m_pValue[0]);
   }
   // We have to compute it the hard way. Invoke the Knuth divide algorithm.
   ApInt quotient(m_bitWidth, 0); // to hold result.
   divide(m_intValue.m_pValue, lhsWords, other.m_intValue.m_pValue, rhsWords, quotient.m_intValue.m_pValue, nullptr);
   return quotient;
}
ApInt ApInt::udiv(uint64_t other) const
{
   assert(other != 0 && "Divide by zero?");
   // First, deal with the easy case
   if (isSingleWord()) {
      return ApInt(m_bitWidth, m_intValue.m_value / other);
   }
   // Get some facts about the LHS words.
   unsigned lhsWords = getNumWords(getActiveBits());
   // Deal with some degenerate cases
   if (!lhsWords) {
      // 0 / X ===> 0
      return ApInt(m_bitWidth, 0);
   }
   if (other == 1) {
      // X / 1 ===> X
      return *this;
   }
   if (this->ult(other)) {
      // X / Y ===> 0, iff X < Y
      return ApInt(m_bitWidth, 0);
   }
   if (*this == other) {
      // X / X ===> 1
      return ApInt(m_bitWidth, 1);
   }
   if (lhsWords == 1) { // rhsWords is 1 if lhsWords is 1.
      // All high words are zero, just use native divide
      return ApInt(m_bitWidth, this->m_intValue.m_pValue[0] / other);
   }
   // We have to compute it the hard way. Invoke the Knuth divide algorithm.
   ApInt quotient(m_bitWidth, 0); // to hold result.
   divide(m_intValue.m_pValue, lhsWords, &other, 1, quotient.m_intValue.m_pValue, nullptr);
   return quotient;
}
ApInt ApInt::sdiv(const ApInt &other) const
{
   if (isNegative()) {
      if (other.isNegative()) {
         return (-(*this)).udiv(-other);
      }
      return -((-(*this)).udiv(other));
   }
   if (other.isNegative()) {
      return -(this->udiv(-other));
   }
   return this->udiv(other);
}
ApInt ApInt::sdiv(int64_t other) const
{
   if (isNegative()) {
      if (other < 0) {
         return (-(*this)).udiv(-other);
      }
      return -((-(*this)).udiv(other));
   }
   if (other < 0) {
      return -(this->udiv(-other));
   }
   return this->udiv(other);
}
ApInt ApInt::urem(const ApInt &other) const
{
   assert(m_bitWidth == other.m_bitWidth && "Bit widths must be the same");
   if (isSingleWord()) {
      assert(other.m_intValue.m_value != 0 && "remainder by zero?");
      return ApInt(m_bitWidth, m_intValue.m_value % other.m_intValue.m_value);
   }
   // Get some facts about the LHS
   unsigned lhsWords = getNumWords(getActiveBits());
   // Get some facts about the rhs
   unsigned rhsBits = other.getActiveBits();
   unsigned rhsWords = getNumWords(rhsBits);
   assert(rhsWords && "Performing remainder operation by zero ???");
   // Check the degenerate cases
   if (lhsWords == 0) {
      // 0 % Y ===> 0
      return ApInt(m_bitWidth, 0);
   }
   if (rhsBits == 1) {
      // X % 1 ===> 0
      return ApInt(m_bitWidth, 0);
   }
   if (lhsWords < rhsWords || this->ult(other)) {
      // X % Y ===> X, iff X < Y
      return *this;
   }
   if (*this == other) {
      // X % X == 0;
      return ApInt(m_bitWidth, 0);
   }
   if (lhsWords == 1) {
      // All high words are zero, just use native remainder
      return ApInt(m_bitWidth, m_intValue.m_pValue[0] % other.m_intValue.m_pValue[0]);
   }
   // We have to compute it the hard way. Invoke the Knuth divide algorithm.
   ApInt remainder(m_bitWidth, 0);
   divide(m_intValue.m_pValue, lhsWords, other.m_intValue.m_pValue, rhsWords, nullptr, remainder.m_intValue.m_pValue);
   return remainder;
}
uint64_t ApInt::urem(uint64_t rhs) const
{
   assert(rhs != 0 && "remainder by zero?");
   if (isSingleWord()) {
      return m_intValue.m_value % rhs;
   }
   // Get some facts about the lhs
   unsigned lhsWords = getNumWords(getActiveBits());
   // Check the degenerate cases
   if (lhsWords == 0) {
      // 0 % Y ===> 0
      return 0;
   }
   if (rhs == 1) {
      // X % 1 ===> 0
      return 0;
   }
   if (this->ult(rhs)) {
      // X % Y ===> X, iff X < Y
      return getZeroExtValue();
   }
   if (*this == rhs) {
      // X % X == 0;
      return 0;
   }
   if (lhsWords == 1) {
      // All high words are zero, just use native remainder
      return m_intValue.m_pValue[0] % rhs;
   }
   // We have to compute it the hard way. Invoke the Knuth divide algorithm.
   uint64_t remainder;
   divide(m_intValue.m_pValue, lhsWords, &rhs, 1, nullptr, &remainder);
   return remainder;
}
ApInt ApInt::srem(const ApInt &rhs) const
{
   if (isNegative()) {
      if (rhs.isNegative()) {
         return -((-(*this)).urem(-rhs));
      }
      return -((-(*this)).urem(rhs));
   }
   if (rhs.isNegative()) {
      return this->urem(-rhs);
   }
   return this->urem(rhs);
}
int64_t ApInt::srem(int64_t rhs) const
{
   if (isNegative()) {
      if (rhs < 0) {
         return -((-(*this)).urem(-rhs));
      }
      return -((-(*this)).urem(rhs));
   }
   if (rhs < 0) {
      return this->urem(-rhs);
   }
   return this->urem(rhs);
}
void ApInt::udivrem(const ApInt &lhs, const ApInt &rhs,
                    ApInt &quotient, ApInt &remainder)
{
   assert(lhs.m_bitWidth == rhs.m_bitWidth && "Bit widths must be the same");
   unsigned bitWidth = lhs.m_bitWidth;
   // First, deal with the easy case
   if (lhs.isSingleWord()) {
      assert(rhs.m_intValue.m_value != 0 && "Divide by zero?");
      uint64_t quotVal = lhs.m_intValue.m_value / rhs.m_intValue.m_value;
      uint64_t remVal = lhs.m_intValue.m_value % rhs.m_intValue.m_value;
      quotient = ApInt(bitWidth, quotVal);
      remainder = ApInt(bitWidth, remVal);
      return;
   }
   // Get some size facts about the dividend and divisor
   unsigned lhsWords = getNumWords(lhs.getActiveBits());
   unsigned rhsBits  = rhs.getActiveBits();
   unsigned rhsWords = getNumWords(rhsBits);
   assert(rhsWords && "Performing divrem operation by zero ???");
   // Check the degenerate cases
   if (lhsWords == 0) {
      quotient = ApInt(bitWidth, 0);                // 0 / Y ===> 0
      remainder = ApInt(bitWidth, 0);               // 0 % Y ===> 0
      return;
   }
   if (rhsBits == 1) {
      quotient = lhs;             // X / 1 ===> X
      remainder = ApInt(bitWidth, 0);              // X % 1 ===> 0
   }
   if (lhsWords < rhsWords || lhs.ult(rhs)) {
      remainder = lhs;            // X % Y ===> X, iff X < Y
      quotient = ApInt(bitWidth, 0);               // X / Y ===> 0, iff X < Y
      return;
   }
   if (lhs == rhs) {
      quotient  = ApInt(bitWidth, 1);              // X / X ===> 1
      remainder = ApInt(bitWidth, 0);              // X % X ===> 0;
      return;
   }
   // Make sure there is enough space to hold the results.
   // NOTE: This assumes that reallocate won't affect any bits if it doesn't
   // change the size. This is necessary if quotient or remainder is aliased
   // with lhs or rhs.
   quotient.reallocate(bitWidth);
   remainder.reallocate(bitWidth);
   if (lhsWords == 1) { // rhsWords is 1 if lhsWords is 1.
      // There is only one word to consider so use the native versions.
      uint64_t lhsValue = lhs.m_intValue.m_pValue[0];
      uint64_t rhsValue = rhs.m_intValue.m_pValue[0];
      quotient = lhsValue / rhsValue;
      remainder = lhsValue % rhsValue;
      return;
   }
   // Okay, lets do it the long way
   divide(lhs.m_intValue.m_pValue, lhsWords, rhs.m_intValue.m_pValue, rhsWords, quotient.m_intValue.m_pValue,
          remainder.m_intValue.m_pValue);
   // Clear the rest of the quotient and remainder.
   std::memset(quotient.m_intValue.m_pValue + lhsWords, 0,
               (getNumWords(bitWidth) - lhsWords) * APINT_WORD_SIZE);
   std::memset(remainder.m_intValue.m_pValue + rhsWords, 0,
               (getNumWords(bitWidth) - rhsWords) * APINT_WORD_SIZE);
}

void ApInt::udivrem(const ApInt &lhs, uint64_t rhs, ApInt &quotient,
                    uint64_t &remainder) {
   assert(rhs != 0 && "Divide by zero?");
   unsigned bitWidth = lhs.m_bitWidth;
   // First, deal with the easy case
   if (lhs.isSingleWord()) {
      uint64_t quotVal = lhs.m_intValue.m_value / rhs;
      remainder = lhs.m_intValue.m_value % rhs;
      quotient = ApInt(bitWidth, quotVal);
      return;
   }
   // Get some size facts about the dividend and divisor
   unsigned lhsWords = getNumWords(lhs.getActiveBits());
   // Check the degenerate cases
   if (lhsWords == 0) {
      quotient = ApInt(bitWidth, 0);                // 0 / Y ===> 0
      remainder = 0;               // 0 % Y ===> 0
      return;
   }
   if (rhs == 1) {
      quotient = lhs;             // X / 1 ===> X
      remainder = 0;              // X % 1 ===> 0
   }
   if (lhs.ult(rhs)) {
      remainder = lhs.getZeroExtValue(); // X % Y ===> X, iff X < Y
      quotient = ApInt(bitWidth, 0);                   // X / Y ===> 0, iff X < Y
      return;
   }
   if (lhs == rhs) {
      quotient  = ApInt(bitWidth, 1);              // X / X ===> 1
      remainder = 0;              // X % X ===> 0;
      return;
   }
   // Make sure there is enough space to hold the results.
   // NOTE: This assumes that reallocate won't affect any bits if it doesn't
   // change the size. This is necessary if quotient is aliased with lhs.
   quotient.reallocate(bitWidth);
   if (lhsWords == 1) { // rhsWords is 1 if lhsWords is 1.
      // There is only one word to consider so use the native versions.
      uint64_t lhsValue = lhs.m_intValue.m_pValue[0];
      quotient = lhsValue / rhs;
      remainder = lhsValue % rhs;
      return;
   }
   // Okay, lets do it the long way
   divide(lhs.m_intValue.m_pValue, lhsWords, &rhs, 1, quotient.m_intValue.m_pValue, &remainder);
   // Clear the rest of the quotient.
   std::memset(quotient.m_intValue.m_pValue + lhsWords, 0,
               (getNumWords(bitWidth) - lhsWords) * APINT_WORD_SIZE);
}
void ApInt::sdivrem(const ApInt &lhs, const ApInt &rhs,
                    ApInt &quotient, ApInt &remainder)
{
   if (lhs.isNegative()) {
      if (rhs.isNegative())
         ApInt::udivrem(-lhs, -rhs, quotient, remainder);
      else {
         ApInt::udivrem(-lhs, rhs, quotient, remainder);
         quotient.negate();
      }
      remainder.negate();
   } else if (rhs.isNegative()) {
      ApInt::udivrem(lhs, -rhs, quotient, remainder);
      quotient.negate();
   } else {
      ApInt::udivrem(lhs, rhs, quotient, remainder);
   }
}
void ApInt::sdivrem(const ApInt &lhs, int64_t rhs,
                    ApInt &quotient, int64_t &remainder)
{
   uint64_t ret = remainder;
   if (lhs.isNegative()) {
      if (rhs < 0)
         ApInt::udivrem(-lhs, -rhs, quotient, ret);
      else {
         ApInt::udivrem(-lhs, rhs, quotient, ret);
         quotient.negate();
      }
      ret = -ret;
   } else if (rhs < 0) {
      ApInt::udivrem(lhs, -rhs, quotient, ret);
      quotient.negate();
   } else {
      ApInt::udivrem(lhs, rhs, quotient, ret);
   }
   remainder = ret;
}
ApInt ApInt::saddOverflow(const ApInt &rhs, bool &overflow) const
{
   ApInt res = *this+rhs;
   overflow = isNonNegative() == rhs.isNonNegative() &&
         res.isNonNegative() != isNonNegative();
   return res;
}
ApInt ApInt::uaddOverflow(const ApInt &rhs, bool &overflow) const
{
   ApInt res = *this+rhs;
   overflow = res.ult(rhs);
   return res;
}
ApInt ApInt::ssubOverflow(const ApInt &rhs, bool &overflow) const
{
   ApInt res = *this - rhs;
   overflow = isNonNegative() != rhs.isNonNegative() &&
         res.isNonNegative() != isNonNegative();
   return res;
}
ApInt ApInt::usubOverflow(const ApInt &rhs, bool &overflow) const
{
   ApInt res = *this-rhs;
   overflow = res.ugt(*this);
   return res;
}
ApInt ApInt::sdivOverflow(const ApInt &rhs, bool &overflow) const
{
   // MININT/-1  -->  overflow.
   overflow = isMinSignedValue() && rhs.isAllOnesValue();
   return sdiv(rhs);
}
ApInt ApInt::smulOverflow(const ApInt &rhs, bool &overflow) const
{
   ApInt res = *this * rhs;
   if (*this != 0 && rhs != 0) {
      overflow = res.sdiv(rhs) != *this || res.sdiv(*this) != rhs;
   } else {
      overflow = false;
   }
   return res;
}
ApInt ApInt::umulOverflow(const ApInt &rhs, bool &overflow) const
{
   ApInt res = *this * rhs;
   if (*this != 0 && rhs != 0) {
      overflow = res.udiv(rhs) != *this || res.udiv(*this) != rhs;
   } else {
      overflow = false;
   }
   return res;
}
ApInt ApInt::sshlOverflow(const ApInt &shAmt, bool &overflow) const
{
   overflow = shAmt.uge(getBitWidth());
   if (overflow) {
      return ApInt(m_bitWidth, 0);
   }
   if (isNonNegative()) {// Don't allow sign change.
      overflow = shAmt.uge(countLeadingZeros());
   } else {
      overflow = shAmt.uge(countLeadingOnes());
   }
   return *this << shAmt;
}

ApInt ApInt::ushlOverflow(const ApInt &shAmt, bool &overflow) const
{
   overflow = shAmt.uge(getBitWidth());
   if (overflow) {
      return ApInt(m_bitWidth, 0);
   }
   overflow = shAmt.ugt(countLeadingZeros());
   return *this << shAmt;
}


ApInt ApInt::saddSaturate(const ApInt &rhs) const
{
   bool overflow;
   ApInt result = saddOverflow(rhs, overflow);
   if (!overflow) {
      return result;
   }


   return isNegative() ? ApInt::getSignedMinValue(m_bitWidth)
                       : ApInt::getSignedMaxValue(m_bitWidth);
}

ApInt ApInt::uaddSaturate(const ApInt &rhs) const
{
   bool overflow;
   ApInt result = uaddOverflow(rhs, overflow);
   if (!overflow) {
      return result;
   }
   return ApInt::getMaxValue(m_bitWidth);
}

ApInt ApInt::ssubSaturate(const ApInt &rhs) const
{
   bool overflow;
   ApInt result = ssubOverflow(rhs, overflow);
   if (!overflow) {
      return result;
   }
   return isNegative() ? ApInt::getSignedMinValue(m_bitWidth)
                       : ApInt::getSignedMaxValue(m_bitWidth);
}

ApInt ApInt::usubSaturate(const ApInt &rhs) const
{
   bool overflow;
   ApInt result = usubOverflow(rhs, overflow);
   if (!overflow) {
      return result;
   }
   return ApInt(m_bitWidth, 0);
}

void ApInt::fromString(unsigned numbits, StringRef str, uint8_t radix)
{
   // Check our assumptions here
   assert(!str.empty() && "Invalid string length");
   assert((radix == 10 || radix == 8 || radix == 16 || radix == 2 ||
           radix == 36) &&
          "Radix should be 2, 8, 10, 16, or 36!");
   StringRef::iterator p = str.begin();
   size_t slen = str.getSize();
   bool isNeg = *p == '-';
   if (*p == '-' || *p == '+') {
      p++;
      slen--;
      assert(slen && "String is only a sign, needs a value.");
   }
   assert((slen <= numbits || radix != 2) && "Insufficient bit width");
   assert(((slen-1)*3 <= numbits || radix != 8) && "Insufficient bit width");
   assert(((slen-1)*4 <= numbits || radix != 16) && "Insufficient bit width");
   assert((((slen-1)*64)/22 <= numbits || radix != 10) &&
          "Insufficient bit width");
   // Allocate memory if needed
   if (isSingleWord()) {
      m_intValue.m_value = 0;
   } else {
      m_intValue.m_pValue = get_cleared_memory(getNumWords());
   }
   // Figure out if we can shift instead of multiply
   unsigned shift = (radix == 16 ? 4 : radix == 8 ? 3 : radix == 2 ? 1 : 0);
   // Enter digit traversal loop
   for (StringRef::iterator e = str.end(); p != e; ++p) {
      unsigned digit = get_digit(*p, radix);
      assert(digit < radix && "Invalid character in digit string");
      // Shift or multiply the value by the radix
      if (slen > 1) {
         if (shift) {
            *this <<= shift;
         } else {
            *this *= radix;
         }
      }
      // Add in the digit we just interpreted
      *this += digit;
   }
   // If its negative, put it in two's complement form
   if (isNeg) {
      this->negate();
   }
}
void ApInt::toString(SmallVectorImpl<char> &str, unsigned radix,
                     bool isSigned, bool formatAsCLiteral) const
{
   assert((radix == 10 || radix == 8 || radix == 16 || radix == 2 ||
           radix == 36) &&
          "Radix should be 2, 8, 10, 16, or 36!");
   const char *prefix = "";
   if (formatAsCLiteral) {
      switch (radix) {
      case 2:
         // Binary literals are a non-standard extension added in gcc 4.3:
         // http://gcc.gnu.org/onlinedocs/gcc-4.3.0/gcc/Binary-constants.html
         prefix = "0b";
         break;
      case 8:
         prefix = "0";
         break;
      case 10:
         break; // No prefix
      case 16:
         prefix = "0x";
         break;
      default:
         polar_unreachable("Invalid radix!");
      }
   }
   // First, check for a zero value and just short circuit the logic below.
   if (*this == 0) {
      while (*prefix) {
         str.push_back(*prefix);
         ++prefix;
      };
      str.push_back('0');
      return;
   }
   static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
   if (isSingleWord()) {
      char buffer[65];
      char *bufPtr = std::end(buffer);
      uint64_t num;
      if (!isSigned) {
         num = getZeroExtValue();
      } else {
         int64_t intValue = getSignExtValue();
         if (intValue >= 0) {
            num = intValue;
         } else {
            str.push_back('-');
            num = -(uint64_t)intValue;
         }
      }
      while (*prefix) {
         str.push_back(*prefix);
         ++prefix;
      };
      while (num) {
         *--bufPtr = digits[num % radix];
         num /= radix;
      }
      str.append(bufPtr, std::end(buffer));
      return;
   }
   ApInt temp(*this);
   if (isSigned && isNegative()) {
      // They want to print the signed version and it is a negative value
      // Flip the bits and add one to turn it into the equivalent positive
      // value and put a '-' in the result.
      temp.negate();
      str.push_back('-');
   }
   while (*prefix) {
      str.push_back(*prefix);
      ++prefix;
   };
   // We insert the digits backward, then reverse them to get the right order.
   unsigned startDig = str.getSize();
   // For the 2, 8 and 16 bit cases, we can just shift instead of divide
   // because the number of bits per digit (1, 3 and 4 respectively) divides
   // equally.  We just shift until the value is zero.
   if (radix == 2 || radix == 8 || radix == 16) {
      // Just shift tmp right for each digit width until it becomes zero
      unsigned shiftAmt = (radix == 16 ? 4 : (radix == 8 ? 3 : 1));
      unsigned maskAmt = radix - 1;
      while (temp.getBoolValue()) {
         unsigned digit = unsigned(temp.getRawData()[0]) & maskAmt;
         str.push_back(digits[digit]);
         temp.lshrInPlace(shiftAmt);
      }
   } else {
      while (temp.getBoolValue()) {
         uint64_t digit;
         udivrem(temp, radix, temp, digit);
         assert(digit < radix && "divide failed");
         str.push_back(digits[digit]);
      }
   }
   // Reverse the digits before returning.
   std::reverse(str.begin()+startDig, str.end());
}
/// Returns the ApInt as a std::string. Note that this is an inefficient method.
/// It is better to pass in a SmallVector/SmallString to the methods above.
std::string ApInt::toString(unsigned radix = 10, bool isSigned = true) const
{
   SmallString<40> str;
   toString(str, radix, isSigned, /* formatAsCLiteral = */false);
   return str.getStr();
}
#if !defined(NDEBUG) || defined(POLAR_ENABLE_DUMP)
POLAR_DUMP_METHOD void ApInt::dump() const
{
   SmallString<40> str, unsigedStr;
   this->toStringUnsigned(unsigedStr);
   this->toStringSigned(str);
   debug_stream() << "ApInt(" << m_bitWidth << "b, "
                  << unsigedStr << "u " << str << "s)\n";
}
#endif
void ApInt::print(RawOutStream &outstream, bool isSigned) const
{
   SmallString<40> str;
   this->toString(str, 10, isSigned, /* formatAsCLiteral = */false);
   outstream << str;
}
namespace  {
// This implements a variety of operations on a representation of
// arbitrary precision, two's-complement, bignum integer values.
// Assumed by low_half, high_half, part_msb and part_lsb.  A fairly safe
// and unrestricting assumption.
static_assert(ApInt::APINT_BITS_PER_WORD % 2 == 0,
              "Part width must be divisible by 2!");
/* Some handy functions local to this file.  */
/* Returns the integer part with the least significant BITS set.
   BITS cannot be zero.  */
inline ApInt::WordType low_bit_mask(unsigned bits)
{
   assert(bits != 0 && bits <= ApInt::APINT_BITS_PER_WORD);
   return ~(ApInt::WordType) 0 >> (ApInt::APINT_BITS_PER_WORD - bits);
}
/* Returns the value of the lower half of PART.  */
inline ApInt::WordType low_half(ApInt::WordType part)
{
   return part & low_bit_mask(ApInt::APINT_BITS_PER_WORD / 2);
}
/* Returns the value of the upper half of PART.  */
inline ApInt::WordType high_half(ApInt::WordType part)
{
   return part >> (ApInt::APINT_BITS_PER_WORD / 2);
}
/* Returns the bit number of the most significant set bit of a part.
   If the input number has no bits set -1U is returned.  */
unsigned part_msb(ApInt::WordType value)
{
   return find_last_set(value, ZeroBehavior::ZB_Max);
}
/* Returns the bit number of the least significant set bit of a
   part.  If the input number has no bits set -1U is returned.  */
unsigned part_lsb(ApInt::WordType value)
{
   return find_first_set(value, ZeroBehavior::ZB_Max);
}
} // anonymous namespace
/* Sets the least significant part of a bignum to the input value, and
   zeroes out higher parts.  */
void ApInt::tcSet(WordType *dst, WordType part, unsigned parts)
{
   assert(parts > 0);
   dst[0] = part;
   for (unsigned i = 1; i < parts; i++) {
      dst[i] = 0;
   }
}
/* Assign one bignum to another.  */
void ApInt::tcAssign(WordType *dst, const WordType *src, unsigned parts)
{
   for (unsigned i = 0; i < parts; i++) {
      dst[i] = src[i];
   }
}
/* Returns true if a bignum is zero, false otherwise.  */
bool ApInt::tcIsZero(const WordType *src, unsigned parts)
{
   for (unsigned i = 0; i < parts; i++) {
      if (src[i]) {
         return false;
      }
   }
   return true;
}
/* Extract the given bit of a bignum; returns 0 or 1.  */
int ApInt::tcExtractBit(const WordType *parts, unsigned bit)
{
   return (parts[whichWord(bit)] & maskBit(bit)) != 0;
}
/* Set the given bit of a bignum. */
void ApInt::tcSetBit(WordType *parts, unsigned bit)
{
   parts[whichWord(bit)] |= maskBit(bit);
}
/* Clears the given bit of a bignum. */
void ApInt::tcClearBit(WordType *parts, unsigned bit)
{
   parts[whichWord(bit)] &= ~maskBit(bit);
}
/* Returns the bit number of the least significant set bit of a
   number.  If the input number has no bits set -1U is returned.  */
unsigned ApInt::tcLsb(const WordType *parts, unsigned n)
{
   for (unsigned i = 0; i < n; i++) {
      if (parts[i] != 0) {
         unsigned lsb = part_lsb(parts[i]);
         return lsb + i * APINT_BITS_PER_WORD;
      }
   }
   return -1U;
}
/* Returns the bit number of the most significant set bit of a number.
   If the input number has no bits set -1U is returned.  */
unsigned ApInt::tcMsb(const WordType *parts, unsigned n)
{
   do {
      --n;
      if (parts[n] != 0) {
         unsigned msb = part_msb(parts[n]);
         return msb + n * APINT_BITS_PER_WORD;
      }
   } while (n);
   return -1U;
}
/* Copy the bit vector of width srcBITS from SRC, starting at bit
   srcLSB, to DST, of dstCOUNT parts, such that the bit srcLSB becomes
   the least significant bit of DST.  All high bits above srcBITS in
   DST are zero-filled.  */
void
ApInt::tcExtract(WordType *dst, unsigned dstCount, const WordType *src,
                 unsigned srcBits, unsigned srcLSB)
{
   unsigned dstParts = (srcBits + APINT_BITS_PER_WORD - 1) / APINT_BITS_PER_WORD;
   assert(dstParts <= dstCount);
   unsigned firstSrcPart = srcLSB / APINT_BITS_PER_WORD;
   tcAssign (dst, src + firstSrcPart, dstParts);
   unsigned shift = srcLSB % APINT_BITS_PER_WORD;
   tcShiftRight (dst, dstParts, shift);
   /* We now have (dstParts * APINT_BITS_PER_WORD - shift) bits from SRC
     in DST.  If this is less that srcBits, append the rest, else
     clear the high bits.  */
   unsigned n = dstParts * APINT_BITS_PER_WORD - shift;
   if (n < srcBits) {
      WordType mask = low_bit_mask(srcBits - n);
      dst[dstParts - 1] |= ((src[firstSrcPart + dstParts] & mask)
            << n % APINT_BITS_PER_WORD);
   } else if (n > srcBits) {
      if (srcBits % APINT_BITS_PER_WORD) {
         dst[dstParts - 1] &= low_bit_mask(srcBits % APINT_BITS_PER_WORD);
      }
   }
   /* Clear high parts.  */
   while (dstParts < dstCount) {
      dst[dstParts++] = 0;
   }
}
/* DST += rhs + C where C is zero or one.  Returns the carry flag.  */
ApInt::WordType ApInt::tcAdd(WordType *dst, const WordType *rhs,
                             WordType c, unsigned parts)
{
   assert(c <= 1);
   for (unsigned i = 0; i < parts; i++) {
      WordType l = dst[i];
      if (c) {
         dst[i] += rhs[i] + 1;
         c = (dst[i] <= l);
      } else {
         dst[i] += rhs[i];
         c = (dst[i] < l);
      }
   }
   return c;
}
/// This function adds a single "word" integer, src, to the multiple
/// "word" integer array, dst[]. dst[] is modified to reflect the addition and
/// 1 is returned if there is a carry out, otherwise 0 is returned.
/// @returns the carry of the addition.
ApInt::WordType ApInt::tcAddPart(WordType *dst, WordType src,
                                 unsigned parts)
{
   for (unsigned i = 0; i < parts; ++i) {
      dst[i] += src;
      if (dst[i] >= src) {
         return 0; // No need to carry so exit early.
      }
      src = 1; // Carry one to next digit.
   }
   return 1;
}
/* DST -= rhs + C where C is zero or one.  Returns the carry flag.  */
ApInt::WordType ApInt::tcSubtract(WordType *dst, const WordType *rhs,
                                  WordType c, unsigned parts)
{
   assert(c <= 1);
   for (unsigned i = 0; i < parts; i++) {
      WordType l = dst[i];
      if (c) {
         dst[i] -= rhs[i] + 1;
         c = (dst[i] >= l);
      } else {
         dst[i] -= rhs[i];
         c = (dst[i] > l);
      }
   }
   return c;
}
/// This function subtracts a single "word" (64-bit word), src, from
/// the multi-word integer array, dst[], propagating the borrowed 1 value until
/// no further borrowing is needed or it runs out of "words" in dst.  The result
/// is 1 if "borrowing" exhausted the digits in dst, or 0 if dst was not
/// exhausted. In other words, if src > dst then this function returns 1,
/// otherwise 0.
/// @returns the borrow out of the subtraction
ApInt::WordType ApInt::tcSubtractPart(WordType *dest, WordType src,
                                      unsigned parts)
{
   for (unsigned i = 0; i < parts; ++i) {
      WordType item = dest[i];
      dest[i] -= src;
      if (src <= item) {
         return 0; // No need to borrow so exit early.
      }
      src = 1; // We have to "borrow 1" from next "word"
   }
   return 1;
}
/* Negate a bignum in-place.  */
void ApInt::tcNegate(WordType *dst, unsigned parts)
{
   tcComplement(dst, parts);
   tcIncrement(dst, parts);
}
/*  DST += SRC * MULTIPLIER + CARRY   if add is true
    DST  = SRC * MULTIPLIER + CARRY   if add is false
    Requires 0 <= DSTPARTS <= SRCPARTS + 1.  If DST overlaps SRC
    they must start at the same point, i.e. DST == SRC.
    If DSTPARTS == SRCPARTS + 1 no overflow occurs and zero is
    returned.  Otherwise DST is filled with the least significant
    DSTPARTS parts of the result, and if all of the omitted higher
    parts were zero return zero, otherwise overflow occurred and
    return one.  */
int ApInt::tcMultiplyPart(WordType *dst, const WordType *src,
                          WordType multiplier, WordType carry,
                          unsigned srcParts, unsigned dstParts,
                          bool add)
{
   /* Otherwise our writes of DST kill our later reads of SRC.  */
   assert(dst <= src || dst >= src + srcParts);
   assert(dstParts <= srcParts + 1);
   /* N loops; minimum of dstParts and srcParts.  */
   unsigned n = std::min(dstParts, srcParts);
   for (unsigned i = 0; i < n; i++) {
      WordType low, mid, high, srcPart;
      /* [ LOW, HIGH ] = MULTIPLIER * SRC[i] + DST[i] + CARRY.
         This cannot overflow, because
         (n - 1) * (n - 1) + 2 (n - 1) = (n - 1) * (n + 1)
         which is less than n^2.  */
      srcPart = src[i];
      if (multiplier == 0 || srcPart == 0) {
         low = carry;
         high = 0;
      } else {
         low = low_half(srcPart) * low_half(multiplier);
         high = high_half(srcPart) * high_half(multiplier);
         mid = low_half(srcPart) * high_half(multiplier);
         high += high_half(mid);
         mid <<= APINT_BITS_PER_WORD / 2;
         if (low + mid < low)
            high++;
         low += mid;
         mid = high_half(srcPart) * low_half(multiplier);
         high += high_half(mid);
         mid <<= APINT_BITS_PER_WORD / 2;
         if (low + mid < low) {
            high++;
         }
         low += mid;
         /* Now add carry.  */
         if (low + carry < low) {
            high++;
         }
         low += carry;
      }
      if (add) {
         /* And now DST[i], and store the new low part there.  */
         if (low + dst[i] < low) {
            high++;
         }
         dst[i] += low;
      } else {
         dst[i] = low;
      }
      carry = high;
   }
   if (srcParts < dstParts) {
      /* Full multiplication, there is no overflow.  */
      assert(srcParts + 1 == dstParts);
      dst[srcParts] = carry;
      return 0;
   }
   /* We overflowed if there is carry.  */
   if (carry)
      return 1;
   /* We would overflow if any significant unwritten parts would be
     non-zero.  This is true if any remaining src parts are non-zero
     and the multiplier is non-zero.  */
   if (multiplier) {
      for (unsigned i = dstParts; i < srcParts; i++) {
         if (src[i]) {
            return 1;
         }
      }
   }
   /* We fitted in the narrow destination.  */
   return 0;
}
/* DST = lhs * rhs, where DST has the same width as the operands and
   is filled with the least significant parts of the result.  Returns
   one if overflow occurred, otherwise zero.  DST must be disjoint
   from both operands.  */
int ApInt::tcMultiply(WordType *dst, const WordType *lhs,
                      const WordType *rhs, unsigned parts)
{
   assert(dst != lhs && dst != rhs);
   int overflow = 0;
   tcSet(dst, 0, parts);
   for (unsigned i = 0; i < parts; i++) {
      overflow |= tcMultiplyPart(&dst[i], lhs, rhs[i], 0, parts,
                                 parts - i, true);
   }
   return overflow;
}
/// DST = lhs * rhs, where DST has width the sum of the widths of the
/// operands. No overflow occurs. DST must be disjoint from both operands.
void ApInt::tcFullMultiply(WordType *dst, const WordType *lhs,
                           const WordType *rhs, unsigned lhsParts,
                           unsigned rhsParts)
{
   /* Put the narrower number on the lhs for less loops below.  */
   if (lhsParts > rhsParts) {
      return tcFullMultiply (dst, rhs, lhs, rhsParts, lhsParts);
   }
   assert(dst != lhs && dst != rhs);
   tcSet(dst, 0, rhsParts);
   for (unsigned i = 0; i < lhsParts; i++) {
      tcMultiplyPart(&dst[i], rhs, lhs[i], 0, rhsParts, rhsParts + 1, true);
   }
}
/* If rhs is zero lhs and remainder are left unchanged, return one.
   Otherwise set lhs to lhs / rhs with the fractional part discarded,
   set remainder to the remainder, return zero.  i.e.
   OLD_lhs = rhs * lhs + remainder
   SCRATCH is a bignum of the same size as the operands and result for
   use by the routine; its contents need not be initialized and are
   destroyed.  lhs, remainder and SCRATCH must be distinct.
*/
int ApInt::tcDivide(WordType *lhs, const WordType *rhs,
                    WordType *remainder, WordType *scratch,
                    unsigned parts)
{
   assert(lhs != remainder && lhs != scratch && remainder != scratch);
   unsigned shiftCount = tcMsb(rhs, parts) + 1;
   if (shiftCount == 0) {
      return true;
   }
   shiftCount = parts * APINT_BITS_PER_WORD - shiftCount;
   unsigned n = shiftCount / APINT_BITS_PER_WORD;
   WordType mask = (WordType) 1 << (shiftCount % APINT_BITS_PER_WORD);
   tcAssign(scratch, rhs, parts);
   tcShiftLeft(scratch, parts, shiftCount);
   tcAssign(remainder, lhs, parts);
   tcSet(lhs, 0, parts);
   /* Loop, subtracting scratch if remainder is greater and adding that to
     the total.  */
   for (;;) {
      int compare = tcCompare(remainder, scratch, parts);
      if (compare >= 0) {
         tcSubtract(remainder, scratch, 0, parts);
         lhs[n] |= mask;
      }
      if (shiftCount == 0) {
         break;
      }
      shiftCount--;
      tcShiftRight(scratch, parts, 1);
      if ((mask >>= 1) == 0) {
         mask = (WordType) 1 << (APINT_BITS_PER_WORD - 1);
         n--;
      }
   }
   return false;
}
/// Shift a bignum left Cound bits in-place. Shifted in bits are zero. There are
/// no restrictions on count.
void ApInt::tcShiftLeft(WordType *dst, unsigned words, unsigned count)
{
   // Don't bother performing a no-op shift.
   if (!count) {
      return;
   }
   // wordshift is the inter-part shift; BitShift is the intra-part shift.
   unsigned wordshift = std::min(count / APINT_BITS_PER_WORD, words);
   unsigned bitShift = count % APINT_BITS_PER_WORD;
   // Fastpath for moving by whole words.
   if (bitShift == 0) {
      std::memmove(dst + wordshift, dst, (words - wordshift) * APINT_WORD_SIZE);
   } else {
      while (words-- > wordshift) {
         dst[words] = dst[words - wordshift] << bitShift;
         if (words > wordshift) {
            dst[words] |=
                  dst[words - wordshift - 1] >> (APINT_BITS_PER_WORD - bitShift);
         }
      }
   }
   // Fill in the remainder with 0s.
   std::memset(dst, 0, wordshift * APINT_WORD_SIZE);
}
/// Shift a bignum right count bits in-place. Shifted in bits are zero. There
/// are no restrictions on count.
void ApInt::tcShiftRight(WordType *dest, unsigned words, unsigned count)
{
   // Don't bother performing a no-op shift.
   if (!count) {
      return;
   }
   // wordshift is the inter-part shift; BitShift is the intra-part shift.
   unsigned wordshift = std::min(count / APINT_BITS_PER_WORD, words);
   unsigned bitShift = count % APINT_BITS_PER_WORD;
   unsigned wordsToMove = words - wordshift;
   // Fastpath for moving by whole words.
   if (bitShift == 0) {
      std::memmove(dest, dest + wordshift, wordsToMove * APINT_WORD_SIZE);
   } else {
      for (unsigned i = 0; i != wordsToMove; ++i) {
         dest[i] = dest[i + wordshift] >> bitShift;
         if (i + 1 != wordsToMove) {
            dest[i] |= dest[i + wordshift + 1] << (APINT_BITS_PER_WORD - bitShift);
         }
      }
   }
   // Fill in the remainder with 0s.
   std::memset(dest + wordsToMove, 0, wordshift * APINT_WORD_SIZE);
}
/* Bitwise and of two bignums.  */
void ApInt::tcAnd(WordType *dst, const WordType *rhs, unsigned parts)
{
   for (unsigned i = 0; i < parts; i++) {
      dst[i] &= rhs[i];
   }
}
/* Bitwise inclusive or of two bignums.  */
void ApInt::tcOr(WordType *dst, const WordType *rhs, unsigned parts)
{
   for (unsigned i = 0; i < parts; i++) {
      dst[i] |= rhs[i];
   }
}
/* Bitwise exclusive or of two bignums.  */
void ApInt::tcXor(WordType *dst, const WordType *rhs, unsigned parts)
{
   for (unsigned i = 0; i < parts; i++) {
      dst[i] ^= rhs[i];
   }
}
/* Complement a bignum in-place.  */
void ApInt::tcComplement(WordType *dst, unsigned parts)
{
   for (unsigned i = 0; i < parts; i++) {
      dst[i] = ~dst[i];
   }
}
/* Comparison (unsigned) of two bignums.  */
int ApInt::tcCompare(const WordType *lhs, const WordType *rhs,
                     unsigned parts)
{
   while (parts) {
      parts--;
      if (lhs[parts] != rhs[parts]) {
         return (lhs[parts] > rhs[parts]) ? 1 : -1;
      }
   }
   return 0;
}
/* Set the least significant BITS bits of a bignum, clear the
   rest.  */
void ApInt::tcSetLeastSignificantBits(WordType *dst, unsigned parts,
                                      unsigned bits)
{
   unsigned i = 0;
   while (bits > APINT_BITS_PER_WORD) {
      dst[i++] = ~(WordType) 0;
      bits -= APINT_BITS_PER_WORD;
   }
   if (bits) {
      dst[i++] = ~(WordType) 0 >> (APINT_BITS_PER_WORD - bits);
   }
   while (i < parts) {
      dst[i++] = 0;
   }
}

ApInt apintops::rounding_udiv(const ApInt &lhs, const ApInt &rhs,
                              ApInt::Rounding rm)
{
   // Currently udivrem always rounds down.
   switch (rm) {
   case ApInt::Rounding::DOWN:
   case ApInt::Rounding::TOWARD_ZERO:
      return lhs.udiv(rhs);
   case ApInt::Rounding::UP: {
      ApInt quo, rem;
      ApInt::udivrem(lhs, rhs, quo, rem);
      if (rem == 0)
         return quo;
      return quo + 1;
   }
   }
   polar_unreachable("Unknown ApInt::Rounding enum");
}

ApInt apintops::rounding_sdiv(const ApInt &lhs, const ApInt &rhs,
                              ApInt::Rounding rm) {
   switch (rm) {
   case ApInt::Rounding::DOWN:
   case ApInt::Rounding::UP: {
      ApInt quo, rem;
      ApInt::sdivrem(lhs, rhs, quo, rem);
      if (rem == 0)
         return quo;
      // This algorithm deals with arbitrary rounding mode used by sdivrem.
      // We want to check whether the non-integer part of the mathematical value
      // is negative or not. If the non-integer part is negative, we need to round
      // down from quo; otherwise, if it's positive or 0, we return quo, as it's
      // already rounded down.
      if (rm == ApInt::Rounding::DOWN) {
         if (rem.isNegative() != rhs.isNegative()) {
            return quo - 1;
         }
         return quo;
      }
      if (rem.isNegative() != rhs.isNegative()) {
         return quo;
      }
      return quo + 1;
   }
      // Currently sdiv rounds twards zero.
   case ApInt::Rounding::TOWARD_ZERO:
      return lhs.sdiv(rhs);
   }
   polar_unreachable("Unknown ApInt::Rounding enum");
}


std::optional<ApInt>
apintops::solve_quadratic_equation_wrap(ApInt A, ApInt B, ApInt C,
                                           unsigned rangeWidth)
{
   unsigned CoeffWidth = A.getBitWidth();
   assert(CoeffWidth == B.getBitWidth() && CoeffWidth == C.getBitWidth());
   assert(rangeWidth <= CoeffWidth &&
          "Value range width should be less than coefficient width");
   assert(rangeWidth > 1 && "Value range bit width should be > 1");

   POLAR_DEBUG(debug_stream() << __func__ << ": solving " << A << "x^2 + " << B
              << "x + " << C << ", rw:" << rangeWidth << '\n');

   // Identify 0 as a (non)solution immediately.
   if (C.sextOrTrunc(rangeWidth).isNullValue() ) {
      POLAR_DEBUG(debug_stream() << __func__ << ": zero solution\n");
      return ApInt(CoeffWidth, 0);
   }

   // The result of ApInt arithmetic has the same bit width as the operands,
   // so it can actually lose high bits. A product of two n-bit integers needs
   // 2n-1 bits to represent the full value.
   // The operation done below (on quadratic coefficients) that can produce
   // the largest value is the evaluation of the equation during bisection,
   // which needs 3 times the bitwidth of the coefficient, so the total number
   // of required bits is 3n.
   //
   // The purpose of this extension is to simulate the set Z of all integers,
   // where n+1 > n for all n in Z. In Z it makes sense to talk about positive
   // and negative numbers (not so much in a modulo arithmetic). The method
   // used to solve the equation is based on the standard formula for real
   // numbers, and uses the concepts of "positive" and "negative" with their
   // usual meanings.
   CoeffWidth *= 3;
   A = A.sext(CoeffWidth);
   B = B.sext(CoeffWidth);
   C = C.sext(CoeffWidth);

   // Make A > 0 for simplicity. Negate cannot overflow at this point because
   // the bit width has increased.
   if (A.isNegative()) {
      A.negate();
      B.negate();
      C.negate();
   }

   // Solving an equation q(x) = 0 with coefficients in modular arithmetic
   // is really solving a set of equations q(x) = kR for k = 0, 1, 2, ...,
   // and R = 2^m_bitWidth.
   // Since we're trying not only to find exact solutions, but also values
   // that "wrap around", such a set will always have a solution, i.e. an x
   // that satisfies at least one of the equations, or such that |q(x)|
   // exceeds kR, while |q(x-1)| for the same k does not.
   //
   // We need to find a value k, such that Ax^2 + Bx + C = kR will have a
   // positive solution n (in the above sense), and also such that the n
   // will be the least among all solutions corresponding to k = 0, 1, ...
   // (more precisely, the least element in the set
   //   { n(k) | k is such that a solution n(k) exists }).
   //
   // Consider the parabola (over real numbers) that corresponds to the
   // quadratic equation. Since A > 0, the arms of the parabola will point
   // up. Picking different values of k will shift it up and down by R.
   //
   // We want to shift the parabola in such a way as to reduce the problem
   // of solving q(x) = kR to solving shifted_q(x) = 0.
   // (The interesting solutions are the ceilings of the real number
   // solutions.)
   ApInt R = ApInt::getOneBitSet(CoeffWidth, rangeWidth);
   ApInt TwoA = 2 * A;
   ApInt SqrB = B * B;
   bool PickLow;

   auto RoundUp = [] (const ApInt &V, const ApInt &A) -> ApInt {
      assert(A.isStrictlyPositive());
      ApInt T = V.abs().urem(A);
      if (T.isNullValue()) {
         return V;
      }
      return V.isNegative() ? V+T : V+(A-T);
   };

   // The vertex of the parabola is at -B/2A, but since A > 0, it's negative
   // iff B is positive.
   if (B.isNonNegative()) {
      // If B >= 0, the vertex it at a negative location (or at 0), so in
      // order to have a non-negative solution we need to pick k that makes
      // C-kR negative. To satisfy all the requirements for the solution
      // that we are looking for, it needs to be closest to 0 of all k.
      C = C.srem(R);
      if (C.isStrictlyPositive()) {
          C -= R;
      }
      // Pick the greater solution.
      PickLow = false;
   } else {
      // If B < 0, the vertex is at a positive location. For any solution
      // to exist, the discriminant must be non-negative. This means that
      // C-kR <= B^2/4A is a necessary condition for k, i.e. there is a
      // lower bound on values of k: kR >= C - B^2/4A.
      ApInt LowkR = C - SqrB.udiv(2*TwoA); // udiv because all values > 0.
      // Round LowkR up (towards +inf) to the nearest kR.
      LowkR = RoundUp(LowkR, R);

      // If there exists k meeting the condition above, and such that
      // C-kR > 0, there will be two positive real number solutions of
      // q(x) = kR. Out of all such values of k, pick the one that makes
      // C-kR closest to 0, (i.e. pick maximum k such that C-kR > 0).
      // In other words, find maximum k such that LowkR <= kR < C.
      if (C.sgt(LowkR)) {
         // If LowkR < C, then such a k is guaranteed to exist because
         // LowkR itself is a multiple of R.
         C -= -RoundUp(-C, R);      // C = C - RoundDown(C, R)
         // Pick the smaller solution.
         PickLow = true;
      } else {
         // If C-kR < 0 for all potential k's, it means that one solution
         // will be negative, while the other will be positive. The positive
         // solution will shift towards 0 if the parabola is moved up.
         // Pick the kR closest to the lower bound (i.e. make C-kR closest
         // to 0, or in other words, out of all parabolas that have solutions,
         // pick the one that is the farthest "up").
         // Since LowkR is itself a multiple of R, simply take C-LowkR.
         C -= LowkR;
         // Pick the greater solution.
         PickLow = false;
      }
   }

   POLAR_DEBUG(debug_stream() << __func__ << ": updated coefficients " << A << "x^2 + "
              << B << "x + " << C << ", rw:" << rangeWidth << '\n');

   ApInt D = SqrB - 4*A*C;
   assert(D.isNonNegative() && "Negative discriminant");
   ApInt SQ = D.sqrt();

   ApInt Q = SQ * SQ;
   bool InexactSQ = Q != D;
   // The calculated SQ may actually be greater than the exact (non-integer)
   // value. If that's the case, decremement SQ to get a value that is lower.
   if (Q.sgt(D)) {
      SQ -= 1;
   }

   ApInt X;
   ApInt Rem;

   // SQ is rounded down (i.e SQ * SQ <= D), so the roots may be inexact.
   // When using the quadratic formula directly, the calculated low root
   // may be greater than the exact one, since we would be subtracting SQ.
   // To make sure that the calculated root is not greater than the exact
   // one, subtract SQ+1 when calculating the low root (for inexact value
   // of SQ).
   if (PickLow) {
       ApInt::sdivrem(-B - (SQ+InexactSQ), TwoA, X, Rem);
   } else {
      ApInt::sdivrem(-B + SQ, TwoA, X, Rem);
   }

   // The updated coefficients should be such that the (exact) solution is
   // positive. Since ApInt division rounds towards 0, the calculated one
   // can be 0, but cannot be negative.
   assert(X.isNonNegative() && "Solution should be non-negative");

   if (!InexactSQ && Rem.isNullValue()) {
      POLAR_DEBUG(debug_stream() << __func__ << ": solution (root): " << X << '\n');
      return X;
   }

   assert((SQ*SQ).sle(D) && "SQ = |_sqrt(D)_|, so SQ*SQ <= D");
   // The exact value of the square root of D should be between SQ and SQ+1.
   // This implies that the solution should be between that corresponding to
   // SQ (i.e. X) and that corresponding to SQ+1.
   //
   // The calculated X cannot be greater than the exact (real) solution.
   // Actually it must be strictly less than the exact solution, while
   // X+1 will be greater than or equal to it.

   ApInt VX = (A*X + B)*X + C;
   ApInt VY = VX + TwoA*X + A + B;
   bool SignChange = VX.isNegative() != VY.isNegative() ||
         VX.isNullValue() != VY.isNullValue();
   // If the sign did not change between X and X+1, X is not a valid solution.
   // This could happen when the actual (exact) roots don't have an integer
   // between them, so they would both be contained between X and X+1.
   if (!SignChange) {
      POLAR_DEBUG(debug_stream() << __func__ << ": no valid solution\n");
      return std::nullopt;
   }

   X += 1;
   POLAR_DEBUG(debug_stream() << __func__ << ": solution (wrap): " << X << '\n');
   return X;
}


} // basic
} // polar
