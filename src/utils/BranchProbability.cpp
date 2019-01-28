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

#include "polarphp/utils/BranchProbability.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/Format.h"
#include "polarphp/utils/RawOutStream.h"
#include <cassert>

namespace polar {
namespace utils {

const uint32_t BranchProbability::m_denominator;

RawOutStream &BranchProbability::print(RawOutStream &outstream) const
{
   if (isUnknown()) {
      return outstream << "?%";
   }
   // Get a percentage rounded to two decimal digits. This avoids
   // implementation-defined rounding inside printf.
   double percent = rint(((double)m_numerator / m_denominator) * 100.0 * 100.0) / 100.0;
   return outstream << format("0x%08" PRIx32 " / 0x%08" PRIx32 " = %.2f%%", m_numerator, m_denominator,
                              percent);
}

#if !defined(NDEBUG) || defined(POLAR_ENABLE_DUMP)
POLAR_DUMP_METHOD void BranchProbability::dump() const
{
   print(debug_stream()) << '\n';
}
#endif

BranchProbability::BranchProbability(uint32_t numerator, uint32_t denominator)
{
   assert(denominator > 0 && "denominator cannot be 0!");
   assert(numerator <= denominator && "Probability cannot be bigger than 1!");
   if (denominator == m_denominator) {
      m_numerator = numerator;
   } else {
      uint64_t prob64 =
            (numerator * static_cast<uint64_t>(m_denominator) + denominator / 2) / denominator;
      m_numerator = static_cast<uint32_t>(prob64);
   }
}

BranchProbability
BranchProbability::getBranchProbability(uint64_t numerator,
                                        uint64_t denominator)
{
   assert(numerator <= denominator && "Probability cannot be bigger than 1!");
   // scale down denominator to fit in a 32-bit integer.
   int scale = 0;
   while (denominator > UINT32_MAX) {
      denominator >>= 1;
      scale++;
   }
   return BranchProbability(numerator >> scale, denominator);
}

namespace {

// If ConstD is not zero, then replace m_denominator by ConstD so that division and modulo
// operations by m_denominator can be optimized, in case this function is not inlined by the
// compiler.
template <uint32_t ConstD>
static uint64_t scale(uint64_t num, uint32_t numerator, uint32_t denominator)
{
   if (ConstD > 0) {
      denominator = ConstD;
   }

   assert(denominator && "divide by 0");

   // Fast path for multiplying by 1.0.
   if (!num || denominator == numerator) {
      return num;
   }

   // Split num into upper and lower parts to multiply, then recombine.
   uint64_t productHigh = (num >> 32) * numerator;
   uint64_t productLow = (num & UINT32_MAX) * numerator;

   // Split into 32-bit digits.
   uint32_t upper32 = productHigh >> 32;
   uint32_t lower32 = productLow & UINT32_MAX;
   uint32_t mid32Partial = productHigh & UINT32_MAX;
   uint32_t mid32 = mid32Partial + (productLow >> 32);

   // Carry.
   upper32 += mid32 < mid32Partial;

   // Check for overflow.
   if (upper32 >= denominator) {
      return UINT64_MAX;
   }
   uint64_t rem = (uint64_t(upper32) << 32) | mid32;
   uint64_t upperQ = rem / denominator;

   // Check for overflow.
   if (upperQ > UINT32_MAX) {
      return UINT64_MAX;
   }
   rem = ((rem % denominator) << 32) | lower32;
   uint64_t lowerQ = rem / denominator;
   uint64_t q = (upperQ << 32) + lowerQ;

   // Check for overflow.
   return q < lowerQ ? UINT64_MAX : q;
}

} // anonymous namespace

uint64_t BranchProbability::scale(uint64_t num) const
{
   return polar::utils::scale<m_denominator>(num, m_numerator, m_denominator);
}

uint64_t BranchProbability::scaleByInverse(uint64_t num) const
{
   return polar::utils::scale<0>(num, m_denominator, m_numerator);
}

} // utils
} // polar
