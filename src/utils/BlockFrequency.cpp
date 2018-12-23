// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/03.

#include "polarphp/utils/BlockFrequency.h"
#include <cassert>

namespace polar {
namespace utils {

BlockFrequency &BlockFrequency::operator*=(BranchProbability probability)
{
   m_frequency = probability.scale(m_frequency);
   return *this;
}

BlockFrequency BlockFrequency::operator*(BranchProbability probability) const
{
   BlockFrequency frequency(m_frequency);
   frequency *= probability;
   return frequency;
}

BlockFrequency &BlockFrequency::operator/=(BranchProbability probability)
{
   m_frequency = probability.scaleByInverse(m_frequency);
   return *this;
}

BlockFrequency BlockFrequency::operator/(BranchProbability probability) const
{
   BlockFrequency frequency(m_frequency);
   frequency /= probability;
   return frequency;
}

BlockFrequency &BlockFrequency::operator+=(BlockFrequency frequency)
{
   uint64_t before = frequency.m_frequency;
   m_frequency += frequency.m_frequency;
   // If overflow, set frequency to the maximum value.
   if (m_frequency < before) {
      m_frequency = UINT64_MAX;
   }
   return *this;
}

BlockFrequency BlockFrequency::operator+(BlockFrequency frequency) const
{
   BlockFrequency newFreq(m_frequency);
   newFreq += frequency;
   return newFreq;
}

BlockFrequency &BlockFrequency::operator-=(BlockFrequency frequency)
{
   // If underflow, set frequency to 0.
   if (m_frequency <= frequency.m_frequency){
      m_frequency = 0;
   } else {
      m_frequency -= frequency.m_frequency;
   }
   return *this;
}

BlockFrequency BlockFrequency::operator-(BlockFrequency frequency) const
{
   BlockFrequency newFreq(m_frequency);
   newFreq -= frequency;
   return newFreq;
}

BlockFrequency &BlockFrequency::operator>>=(const unsigned count)
{
   // m_frequency can never be 0 by design.
   assert(m_frequency != 0);
   // Shift right by count.
   m_frequency >>= count;
   // Saturate to 1 if we are 0.
   m_frequency |= m_frequency == 0;
   return *this;
}

} // utils
} // polar
