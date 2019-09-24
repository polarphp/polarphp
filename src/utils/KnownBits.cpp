//===-- KnownBits.cpp - Stores known zeros/ones ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
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

namespace polar::utils {

namespace {
KnownBits compute_for_add_carry(
      const KnownBits &lhs, const KnownBits &rhs,
      bool carryZero, bool carryOne)
{
   assert(!(carryZero && carryOne) &&
          "carry can't be zero and one at the same time");

   ApInt possibleSumZero = ~lhs.zero + ~rhs.zero + !carryZero;
   ApInt possibleSumOne = lhs.one + rhs.one + carryOne;

   // Compute known bits of the carry.
   ApInt carryKnownZero = ~(possibleSumZero ^ lhs.zero ^ rhs.zero);
   ApInt carryKnownOne = possibleSumOne ^ lhs.one ^ rhs.one;

   // Compute set of known bits (where all three relevant bits are known).
   ApInt lhsKnownUnion = lhs.zero | lhs.one;
   ApInt rhsKnownUnion = rhs.zero | rhs.one;
   ApInt carryKnownUnion = std::move(carryKnownZero) | carryKnownOne;
   ApInt known = std::move(lhsKnownUnion) & rhsKnownUnion & carryKnownUnion;

   assert((possibleSumZero & known) == (possibleSumOne & known) &&
          "known bits of sum differ");

   // Compute known bits of the result.
   KnownBits knownOut;
   knownOut.zero = ~std::move(possibleSumZero) & known;
   knownOut.one = std::move(possibleSumOne) & known;
   return knownOut;
}
}

KnownBits KnownBits::computeForAddCarry(
      const KnownBits &lhs, const KnownBits &rhs, const KnownBits &carry)
{
   assert(carry.getBitWidth() == 1 && "carry must be 1-bit");
   return compute_for_add_carry(
            lhs, rhs, carry.zero.getBoolValue(), carry.one.getBoolValue());
}

KnownBits KnownBits::computeForAddSub(bool add, bool nsw,
                                      const KnownBits &lhs, KnownBits rhs)
{
   KnownBits knownOut;
   if (add) {
      // Sum = lhs + rhs + 0
      knownOut = compute_for_add_carry(
               lhs, rhs, /*carryZero*/true, /*carryOne*/false);
   } else {
      // Sum = lhs + ~rhs + 1
      std::swap(rhs.zero, rhs.one);
      knownOut = compute_for_add_carry(
               lhs, rhs, /*carryZero*/false, /*carryOne*/true);
   }
   // Are we still trying to solve for the sign bit?
   if (!knownOut.isNegative() && !knownOut.isNonNegative()) {
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

} // polar::utils
