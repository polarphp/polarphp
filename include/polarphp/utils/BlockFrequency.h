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
// This file implements Block m_frequency class.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_BLOCKFREQUENCY_H
#define POLARPHP_UTILS_BLOCKFREQUENCY_H

#include "polarphp/utils/BranchProbability.h"
#include "polarphp/global/DataTypes.h"

namespace polar {
namespace utils {

class RawOutStream;

// This class represents Block frequency as a 64-bit value.
class BlockFrequency
{
   uint64_t m_frequency;

public:
   BlockFrequency(uint64_t freq = 0) : m_frequency(freq)
   {}

   /// \brief Returns the maximum possible frequency, the saturation value.
   static uint64_t getMaxFrequency()
   {
      return -1ULL;
   }

   /// \brief Returns the frequency as a fixpoint number scaled by the entry
   /// frequency.
   uint64_t getFrequency() const
   {
      return m_frequency;
   }

   /// \brief Multiplies with a branch probability. The computation will never
   /// overflow.
   BlockFrequency &operator*=(BranchProbability prob);
   BlockFrequency operator*(BranchProbability prob) const;

   /// \brief Divide by a non-zero branch probability using saturating
   /// arithmetic.
   BlockFrequency &operator/=(BranchProbability prob);
   BlockFrequency operator/(BranchProbability prob) const;

   /// \brief Adds another block frequency using saturating arithmetic.
   BlockFrequency &operator+=(BlockFrequency freq);
   BlockFrequency operator+(BlockFrequency freq) const;

   /// \brief Subtracts another block frequency using saturating arithmetic.
   BlockFrequency &operator-=(BlockFrequency freq);
   BlockFrequency operator-(BlockFrequency freq) const;

   /// \brief Shift block frequency to the right by count digits saturating to 1.
   BlockFrequency &operator>>=(const unsigned count);

   bool operator<(BlockFrequency other) const
   {
      return m_frequency < other.m_frequency;
   }

   bool operator<=(BlockFrequency other) const
   {
      return m_frequency <= other.m_frequency;
   }

   bool operator>(BlockFrequency other) const
   {
      return m_frequency > other.m_frequency;
   }

   bool operator>=(BlockFrequency other) const
   {
      return m_frequency >= other.m_frequency;
   }

   bool operator==(BlockFrequency other) const
   {
      return m_frequency == other.m_frequency;
   }
};

} // utils
} // polar

#endif // POLARPHP_UTILS_BLOCKFREQUENCY_H
