// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/04.
//===----------------------------------------------------------------------===//
//
// This file contains a class for representing known zeros and ones used by
// computeKnownBits.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/KnownBits.h"

namespace polar {
namespace utils {

KnownBits KnownBits::computeForAddSub(bool add, bool nsw,
                                      const KnownBits &lhs, KnownBits rhs)
{
   // Carry in a 1 for a subtract, rather than 0.
   bool carryIn = false;
   if (!add) {
      // Sum = lhs + ~rhs + 1
      std::swap(rhs.m_zero, rhs.m_one);
      carryIn = true;
   }

   ApInt possibleSumZero = ~lhs.m_zero + ~rhs.m_zero + carryIn;
   ApInt possibleSumOne = lhs.m_one + rhs.m_one + carryIn;

   // Compute known bits of the carry.
   ApInt carryKnownZero = ~(possibleSumZero ^ lhs.m_zero ^ rhs.m_zero);
   ApInt carryKnownOne = possibleSumOne ^ lhs.m_one ^ rhs.m_one;

   // Compute set of known bits (where all three relevant bits are known).
   ApInt lhsKnownUnion = lhs.m_zero | lhs.m_one;
   ApInt rhsKnownUnion = rhs.m_zero | rhs.m_one;
   ApInt carryKnownUnion = std::move(carryKnownZero) | carryKnownOne;
   ApInt known = std::move(lhsKnownUnion) & rhsKnownUnion & carryKnownUnion;

   assert((possibleSumZero & known) == (possibleSumOne & known) &&
          "known bits of sum differ");

   // Compute known bits of the result.
   KnownBits knownOut;
   knownOut.m_zero = ~std::move(possibleSumZero) & known;
   knownOut.m_one = std::move(possibleSumOne) & known;

   // Are we still trying to solve for the sign bit?
   if (!known.isSignBitSet()) {
      if (nsw) {
         // Adding two non-negative numbers, or subtracting a negative number from
         // a non-negative one, can't wrap into negative.
         if (lhs.isNonNegative() && rhs.isNonNegative()) {
            knownOut.makeNonNegative();
         }
         // Adding two negative numbers, or subtracting a non-negative number from
         // a negative one, can't wrap into non-negative.
         else if (lhs.isNegative() && rhs.isNegative()) {
            knownOut.makeNegative();
         }
      }
   }
   return knownOut;
}

} // utils
} // polar
