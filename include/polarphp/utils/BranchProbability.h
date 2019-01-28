// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/29.

//===----------------------------------------------------------------------===//
//
// Definition of BranchProbability shared by IR and Machine Instructions.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_BRANCHPROBABILITY_H
#define POLARPHP_UTILS_BRANCHPROBABILITY_H

#include "polarphp/global/DataTypes.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include <numeric>

namespace polar {
namespace utils {

class RawOutStream;

// This class represents Branch Probability as a non-negative fraction that is
// no greater than 1. It uses a fixed-point-like implementation, in which the
// denominator is always a constant value (here we use 1<<31 for maximum
// precision).
class BranchProbability
{
   // numerator
   uint32_t m_numerator;

   // denominator, which is a constant value.
   static const uint32_t m_denominator = 1u << 31;
   static const uint32_t m_unknown = UINT32_MAX;

   // Construct a BranchProbability with only numerator assuming the denominator
   // is 1<<31. For internal use only.
   explicit BranchProbability(uint32_t n) : m_numerator(n)
   {}

public:
   BranchProbability() : m_numerator(m_unknown)
   {}

   BranchProbability(uint32_t numerator, uint32_t denominator);

   bool isZero() const
   {
      return m_numerator == 0;
   }

   bool isUnknown() const
   {
      return m_numerator == m_unknown;
   }

   static BranchProbability getZero()
   {
      return BranchProbability(0);
   }

   static BranchProbability getOne()
   {
      return BranchProbability(m_denominator);
   }

   static BranchProbability getUnknown()
   {
      return BranchProbability(m_unknown);
   }

   // Create a BranchProbability object with the given numerator and 1<<31
   // as denominator.
   static BranchProbability getRaw(uint32_t m_numerator)
   {
      return BranchProbability(m_numerator);
   }

   // Create a BranchProbability object from 64-bit integers.
   static BranchProbability getBranchProbability(uint64_t numerator,
                                                 uint64_t denominator);

   // Normalize given probabilties so that the sum of them becomes approximate
   // one.
   template <class ProbabilityIter>
   static void normalizeProbabilities(ProbabilityIter begin,
                                      ProbabilityIter end);

   uint32_t getNumerator() const
   {
      return m_numerator;
   }

   static uint32_t getDenominator()
   {
      return m_denominator;
   }

   // Return (1 - Probability).
   BranchProbability getCompl() const
   {
      return BranchProbability(m_denominator - m_numerator);
   }

   RawOutStream &print(RawOutStream &outstream) const;

   void dump() const;

   /// \brief Scale a large integer.
   ///
   /// Scales \c num.  Guarantees full precision.  Returns the floor of the
   /// result.
   ///
   /// \return \c num times \c this.
   uint64_t scale(uint64_t num) const;

   /// \brief Scale a large integer by the inverse.
   ///
   /// Scales \c num by the inverse of \c this.  Guarantees full precision.
   /// Returns the floor of the result.
   ///
   /// \return \c num divided by \c this.
   uint64_t scaleByInverse(uint64_t num) const;

   BranchProbability &operator+=(BranchProbability other)
   {
      assert(m_numerator != m_unknown && other.m_numerator != m_unknown &&
            "Unknown probability cannot participate in arithmetics.");
      // Saturate the result in case of overflow.
      m_numerator = (uint64_t(m_numerator) + other.m_numerator > m_denominator) ? m_denominator : m_numerator + other.m_numerator;
      return *this;
   }

   BranchProbability &operator-=(BranchProbability other)
   {
      assert(m_numerator != m_unknown && other.m_numerator != m_unknown &&
            "Unknown probability cannot participate in arithmetics.");
      // Saturate the result in case of underflow.
      m_numerator = m_numerator < other.m_numerator ? 0 : m_numerator - other.m_numerator;
      return *this;
   }

   BranchProbability &operator*=(BranchProbability other)
   {
      assert(m_numerator != m_unknown && other.m_numerator != m_unknown &&
            "Unknown probability cannot participate in arithmetics.");
      m_numerator = (static_cast<uint64_t>(m_numerator) * other.m_numerator + m_denominator / 2) / m_denominator;
      return *this;
   }

   BranchProbability &operator*=(uint32_t other)
   {
      assert(m_numerator != m_unknown &&
            "Unknown probability cannot participate in arithmetics.");
      m_numerator = (uint64_t(m_numerator) * other > m_denominator) ? m_denominator : m_numerator * other;
      return *this;
   }

   BranchProbability &operator/=(uint32_t other)
   {
      assert(m_numerator != m_unknown &&
            "Unknown probability cannot participate in arithmetics.");
      assert(other > 0 && "The divider cannot be zero.");
      m_numerator /= other;
      return *this;
   }

   BranchProbability operator+(BranchProbability other) const
   {
      BranchProbability prob(*this);
      return prob += other;
   }

   BranchProbability operator-(BranchProbability other) const
   {
      BranchProbability prob(*this);
      return prob -= other;
   }

   BranchProbability operator*(BranchProbability other) const
   {
      BranchProbability prob(*this);
      return prob *= other;
   }

   BranchProbability operator*(uint32_t other) const
   {
      BranchProbability prob(*this);
      return prob *= other;
   }

   BranchProbability operator/(uint32_t other) const
   {
      BranchProbability prob(*this);
      return prob /= other;
   }

   bool operator==(BranchProbability other) const
   {
      return m_numerator == other.m_numerator;
   }

   bool operator!=(BranchProbability other) const
   {
      return !(*this == other);
   }

   bool operator<(BranchProbability other) const
   {
      assert(m_numerator != m_unknown && other.m_numerator != m_unknown &&
            "Unknown probability cannot participate in comparisons.");
      return m_numerator < other.m_numerator;
   }

   bool operator>(BranchProbability other) const
   {
      assert(m_numerator != m_unknown && other.m_numerator != m_unknown &&
            "Unknown probability cannot participate in comparisons.");
      return other < *this;
   }

   bool operator<=(BranchProbability other) const
   {
      assert(m_numerator != m_unknown && other.m_numerator != m_unknown &&
            "Unknown probability cannot participate in comparisons.");
      return !(other < *this);
   }

   bool operator>=(BranchProbability other) const
   {
      assert(m_numerator != m_unknown && other.m_numerator != m_unknown &&
            "Unknown probability cannot participate in comparisons.");
      return !(*this < other);
   }
};

inline RawOutStream &operator<<(RawOutStream &outstream, BranchProbability prob)
{
   return prob.print(outstream);
}

template <class ProbabilityIter>
void BranchProbability::normalizeProbabilities(ProbabilityIter begin,
                                               ProbabilityIter end)
{
   if (begin == end) {
      return;
   }
   unsigned unknownProbCount = 0;
   auto handler = [&](uint64_t str, const BranchProbability &bp) {
      if (!bp.isUnknown()) {
         return str + bp.m_numerator;
      }
      unknownProbCount++;
      return str;
   };
   uint64_t sum = std::accumulate(begin, end, uint64_t(0),
                                  handler);

   if (unknownProbCount > 0) {
      BranchProbability probForUnknown = BranchProbability::getZero();
      // If the sum of all known probabilities is less than one, evenly distribute
      // the complement of sum to unknown probabilities. Otherwise, set unknown
      // probabilities to zeros and continue to normalize known probabilities.
      if (sum < BranchProbability::getDenominator()) {
         probForUnknown = BranchProbability::getRaw(
                  (BranchProbability::getDenominator() - sum) / unknownProbCount);
      }
      std::replace_if(begin, end,
                      [](const BranchProbability &bp) { return bp.isUnknown(); },
      probForUnknown);
      if (sum <= BranchProbability::getDenominator()) {
         return;
      }
   }

   if (sum == 0) {
      BranchProbability bp(1, std::distance(begin, end));
      std::fill(begin, end, bp);
      return;
   }

   for (auto iter = begin; iter != end; ++iter) {
      iter->m_numerator = (iter->m_numerator * uint64_t(m_denominator) + sum / 2) / sum;
   }
}

} // utils
} // polar

#endif // POLARPHP_UTILS_BRANCHPROBABILITY_H
