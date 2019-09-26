//===- llvm/unittest/ADT/ApSIntTest.cpp - ApSInt unit tests ---------------===//
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
// Created by polarboy on 2018/07/07.

#include "polarphp/basic/adt/ApSInt.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {

TEST(ApSIntTest, testMoveTest)
{
   ApSInt A(32, true);
   EXPECT_TRUE(A.isUnsigned());

   ApSInt B(128, false);
   A = B;
   EXPECT_FALSE(A.isUnsigned());

   ApSInt C(B);
   EXPECT_FALSE(C.isUnsigned());

   ApInt wide(256, 0);
   const uint64_t *Bits = wide.getRawData();
   ApSInt D(std::move(wide));
   EXPECT_TRUE(D.isUnsigned());
   EXPECT_EQ(Bits, D.getRawData()); // Verify that "wide" was really moved.

   A = ApSInt(64, true);
   EXPECT_TRUE(A.isUnsigned());

   wide = ApInt(128, 1);
   Bits = wide.getRawData();
   A = std::move(wide);
   EXPECT_TRUE(A.isUnsigned());
   EXPECT_EQ(Bits, A.getRawData()); // Verify that "wide" was really moved.
}

TEST(ApSIntTest, testGet)
{
   EXPECT_TRUE(ApSInt::get(7).isSigned());
   EXPECT_EQ(64u, ApSInt::get(7).getBitWidth());
   EXPECT_EQ(7u, ApSInt::get(7).getZeroExtValue());
   EXPECT_EQ(7, ApSInt::get(7).getSignExtValue());
   EXPECT_TRUE(ApSInt::get(-7).isSigned());
   EXPECT_EQ(64u, ApSInt::get(-7).getBitWidth());
   EXPECT_EQ(-7, ApSInt::get(-7).getSignExtValue());
   EXPECT_EQ(UINT64_C(0) - 7, ApSInt::get(-7).getZeroExtValue());
}

TEST(ApSIntTest, testGetUnsigned)
{
   EXPECT_TRUE(ApSInt::getUnsigned(7).isUnsigned());
   EXPECT_EQ(64u, ApSInt::getUnsigned(7).getBitWidth());
   EXPECT_EQ(7u, ApSInt::getUnsigned(7).getZeroExtValue());
   EXPECT_EQ(7, ApSInt::getUnsigned(7).getSignExtValue());
   EXPECT_TRUE(ApSInt::getUnsigned(-7).isUnsigned());
   EXPECT_EQ(64u, ApSInt::getUnsigned(-7).getBitWidth());
   EXPECT_EQ(-7, ApSInt::getUnsigned(-7).getSignExtValue());
   EXPECT_EQ(UINT64_C(0) - 7, ApSInt::getUnsigned(-7).getZeroExtValue());
}

TEST(ApSIntTest, testGetExtValue)
{
   EXPECT_TRUE(ApSInt(ApInt(3, 7), true).isUnsigned());
   EXPECT_TRUE(ApSInt(ApInt(3, 7), false).isSigned());
   EXPECT_TRUE(ApSInt(ApInt(4, 7), true).isUnsigned());
   EXPECT_TRUE(ApSInt(ApInt(4, 7), false).isSigned());
   EXPECT_TRUE(ApSInt(ApInt(4, -7), true).isUnsigned());
   EXPECT_TRUE(ApSInt(ApInt(4, -7), false).isSigned());
   EXPECT_EQ(7, ApSInt(ApInt(3, 7), true).getExtValue());
   EXPECT_EQ(-1, ApSInt(ApInt(3, 7), false).getExtValue());
   EXPECT_EQ(7, ApSInt(ApInt(4, 7), true).getExtValue());
   EXPECT_EQ(7, ApSInt(ApInt(4, 7), false).getExtValue());
   EXPECT_EQ(9, ApSInt(ApInt(4, -7), true).getExtValue());
   EXPECT_EQ(-7, ApSInt(ApInt(4, -7), false).getExtValue());
}

TEST(ApSIntTest, testCompareValues) {
   auto U = [](uint64_t V) { return ApSInt::getUnsigned(V); };
   auto S = [](int64_t V) { return ApSInt::get(V); };

   // Bit-width matches and is-signed.
   EXPECT_TRUE(ApSInt::compareValues(S(7), S(8)) < 0);
   EXPECT_TRUE(ApSInt::compareValues(S(8), S(7)) > 0);
   EXPECT_TRUE(ApSInt::compareValues(S(7), S(7)) == 0);
   EXPECT_TRUE(ApSInt::compareValues(S(-7), S(8)) < 0);
   EXPECT_TRUE(ApSInt::compareValues(S(8), S(-7)) > 0);
   EXPECT_TRUE(ApSInt::compareValues(S(-7), S(-7)) == 0);
   EXPECT_TRUE(ApSInt::compareValues(S(-7), S(-8)) > 0);
   EXPECT_TRUE(ApSInt::compareValues(S(-8), S(-7)) < 0);
   EXPECT_TRUE(ApSInt::compareValues(S(-7), S(-7)) == 0);

   // Bit-width matches and not is-signed.
   EXPECT_TRUE(ApSInt::compareValues(U(7), U(8)) < 0);
   EXPECT_TRUE(ApSInt::compareValues(U(8), U(7)) > 0);
   EXPECT_TRUE(ApSInt::compareValues(U(7), U(7)) == 0);

   // Bit-width matches and mixed signs.
   EXPECT_TRUE(ApSInt::compareValues(U(7), S(8)) < 0);
   EXPECT_TRUE(ApSInt::compareValues(U(8), S(7)) > 0);
   EXPECT_TRUE(ApSInt::compareValues(U(7), S(7)) == 0);
   EXPECT_TRUE(ApSInt::compareValues(U(8), S(-7)) > 0);

   // Bit-width mismatch and is-signed.
   EXPECT_TRUE(ApSInt::compareValues(S(7).trunc(32), S(8)) < 0);
   EXPECT_TRUE(ApSInt::compareValues(S(8).trunc(32), S(7)) > 0);
   EXPECT_TRUE(ApSInt::compareValues(S(7).trunc(32), S(7)) == 0);
   EXPECT_TRUE(ApSInt::compareValues(S(-7).trunc(32), S(8)) < 0);
   EXPECT_TRUE(ApSInt::compareValues(S(8).trunc(32), S(-7)) > 0);
   EXPECT_TRUE(ApSInt::compareValues(S(-7).trunc(32), S(-7)) == 0);
   EXPECT_TRUE(ApSInt::compareValues(S(-7).trunc(32), S(-8)) > 0);
   EXPECT_TRUE(ApSInt::compareValues(S(-8).trunc(32), S(-7)) < 0);
   EXPECT_TRUE(ApSInt::compareValues(S(-7).trunc(32), S(-7)) == 0);
   EXPECT_TRUE(ApSInt::compareValues(S(7), S(8).trunc(32)) < 0);
   EXPECT_TRUE(ApSInt::compareValues(S(8), S(7).trunc(32)) > 0);
   EXPECT_TRUE(ApSInt::compareValues(S(7), S(7).trunc(32)) == 0);
   EXPECT_TRUE(ApSInt::compareValues(S(-7), S(8).trunc(32)) < 0);
   EXPECT_TRUE(ApSInt::compareValues(S(8), S(-7).trunc(32)) > 0);
   EXPECT_TRUE(ApSInt::compareValues(S(-7), S(-7).trunc(32)) == 0);
   EXPECT_TRUE(ApSInt::compareValues(S(-7), S(-8).trunc(32)) > 0);
   EXPECT_TRUE(ApSInt::compareValues(S(-8), S(-7).trunc(32)) < 0);
   EXPECT_TRUE(ApSInt::compareValues(S(-7), S(-7).trunc(32)) == 0);

   // Bit-width mismatch and not is-signed.
   EXPECT_TRUE(ApSInt::compareValues(U(7), U(8).trunc(32)) < 0);
   EXPECT_TRUE(ApSInt::compareValues(U(8), U(7).trunc(32)) > 0);
   EXPECT_TRUE(ApSInt::compareValues(U(7), U(7).trunc(32)) == 0);
   EXPECT_TRUE(ApSInt::compareValues(U(7).trunc(32), U(8)) < 0);
   EXPECT_TRUE(ApSInt::compareValues(U(8).trunc(32), U(7)) > 0);
   EXPECT_TRUE(ApSInt::compareValues(U(7).trunc(32), U(7)) == 0);

   // Bit-width mismatch and mixed signs.
   EXPECT_TRUE(ApSInt::compareValues(U(7).trunc(32), S(8)) < 0);
   EXPECT_TRUE(ApSInt::compareValues(U(8).trunc(32), S(7)) > 0);
   EXPECT_TRUE(ApSInt::compareValues(U(7).trunc(32), S(7)) == 0);
   EXPECT_TRUE(ApSInt::compareValues(U(8).trunc(32), S(-7)) > 0);
   EXPECT_TRUE(ApSInt::compareValues(U(7), S(8).trunc(32)) < 0);
   EXPECT_TRUE(ApSInt::compareValues(U(8), S(7).trunc(32)) > 0);
   EXPECT_TRUE(ApSInt::compareValues(U(7), S(7).trunc(32)) == 0);
   EXPECT_TRUE(ApSInt::compareValues(U(8), S(-7).trunc(32)) > 0);
}

TEST(ApSIntTest, testFromString)
{
   EXPECT_EQ(ApSInt("1").getExtValue(), 1);
   EXPECT_EQ(ApSInt("-1").getExtValue(), -1);
   EXPECT_EQ(ApSInt("0").getExtValue(), 0);
   EXPECT_EQ(ApSInt("56789").getExtValue(), 56789);
   EXPECT_EQ(ApSInt("-1234").getExtValue(), -1234);
}

#if defined(GTEST_HAS_DEATH_TEST) && defined(POLAR_DEBUG_MODE)

TEST(ApSIntTest, testStringDeath) {
   EXPECT_DEATH(ApSInt(""), "Invalid string length");
   EXPECT_DEATH(ApSInt("1a"), "Invalid character in digit string");
}

#endif

TEST(ApSIntTest, SignedHighBit) {
  ApSInt False(ApInt(1, 0), false);
  ApSInt True(ApInt(1, 1), false);
  ApSInt CharMin(ApInt(8, 0), false);
  ApSInt CharSmall(ApInt(8, 0x13), false);
  ApSInt CharBoundaryUnder(ApInt(8, 0x7f), false);
  ApSInt CharBoundaryOver(ApInt(8, 0x80), false);
  ApSInt CharLarge(ApInt(8, 0xd9), false);
  ApSInt CharMax(ApInt(8, 0xff), false);

  EXPECT_FALSE(False.isNegative());
  EXPECT_TRUE(False.isNonNegative());
  EXPECT_FALSE(False.isStrictlyPositive());

  EXPECT_TRUE(True.isNegative());
  EXPECT_FALSE(True.isNonNegative());
  EXPECT_FALSE(True.isStrictlyPositive());

  EXPECT_FALSE(CharMin.isNegative());
  EXPECT_TRUE(CharMin.isNonNegative());
  EXPECT_FALSE(CharMin.isStrictlyPositive());

  EXPECT_FALSE(CharSmall.isNegative());
  EXPECT_TRUE(CharSmall.isNonNegative());
  EXPECT_TRUE(CharSmall.isStrictlyPositive());

  EXPECT_FALSE(CharBoundaryUnder.isNegative());
  EXPECT_TRUE(CharBoundaryUnder.isNonNegative());
  EXPECT_TRUE(CharBoundaryUnder.isStrictlyPositive());

  EXPECT_TRUE(CharBoundaryOver.isNegative());
  EXPECT_FALSE(CharBoundaryOver.isNonNegative());
  EXPECT_FALSE(CharBoundaryOver.isStrictlyPositive());

  EXPECT_TRUE(CharLarge.isNegative());
  EXPECT_FALSE(CharLarge.isNonNegative());
  EXPECT_FALSE(CharLarge.isStrictlyPositive());

  EXPECT_TRUE(CharMax.isNegative());
  EXPECT_FALSE(CharMax.isNonNegative());
  EXPECT_FALSE(CharMax.isStrictlyPositive());
}

TEST(ApSIntTest, UnsignedHighBit) {
  ApSInt False(ApInt(1, 0));
  ApSInt True(ApInt(1, 1));
  ApSInt CharMin(ApInt(8, 0));
  ApSInt CharSmall(ApInt(8, 0x13));
  ApSInt CharBoundaryUnder(ApInt(8, 0x7f));
  ApSInt CharBoundaryOver(ApInt(8, 0x80));
  ApSInt CharLarge(ApInt(8, 0xd9));
  ApSInt CharMax(ApInt(8, 0xff));

  EXPECT_FALSE(False.isNegative());
  EXPECT_TRUE(False.isNonNegative());
  EXPECT_FALSE(False.isStrictlyPositive());

  EXPECT_FALSE(True.isNegative());
  EXPECT_TRUE(True.isNonNegative());
  EXPECT_TRUE(True.isStrictlyPositive());

  EXPECT_FALSE(CharMin.isNegative());
  EXPECT_TRUE(CharMin.isNonNegative());
  EXPECT_FALSE(CharMin.isStrictlyPositive());

  EXPECT_FALSE(CharSmall.isNegative());
  EXPECT_TRUE(CharSmall.isNonNegative());
  EXPECT_TRUE(CharSmall.isStrictlyPositive());

  EXPECT_FALSE(CharBoundaryUnder.isNegative());
  EXPECT_TRUE(CharBoundaryUnder.isNonNegative());
  EXPECT_TRUE(CharBoundaryUnder.isStrictlyPositive());

  EXPECT_FALSE(CharBoundaryOver.isNegative());
  EXPECT_TRUE(CharBoundaryOver.isNonNegative());
  EXPECT_TRUE(CharBoundaryOver.isStrictlyPositive());

  EXPECT_FALSE(CharLarge.isNegative());
  EXPECT_TRUE(CharLarge.isNonNegative());
  EXPECT_TRUE(CharLarge.isStrictlyPositive());

  EXPECT_FALSE(CharMax.isNegative());
  EXPECT_TRUE(CharMax.isNonNegative());
  EXPECT_TRUE(CharMax.isStrictlyPositive());
}

}
