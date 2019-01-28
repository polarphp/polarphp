// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/07.

#include "polarphp/basic/adt/ApFloat.h"
#include "polarphp/basic/adt/ApSInt.h"
#include "polarphp/basic/adt/Hashing.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/utils/FormatVariadic.h"
#include "polarphp/utils/RawOutStream.h"
#include "gtest/gtest.h"
#include <cmath>
#include <ostream>
#include <string>
#include <tuple>

using namespace polar::basic;

namespace {

using polar::utils::formatv;

static double convertToDoubleFromString(const char *str)
{
   ApFloat F(0.0);
   F.convertFromString(str, ApFloat::RoundingMode::rmNearestTiesToEven);
   return F.convertToDouble();
}

static std::string convertToString(double d, unsigned Prec, unsigned Pad,
                                   bool Tr = true)
{
   SmallVector<char, 100> Buffer;
   ApFloat F(d);
   F.toString(Buffer, Prec, Pad, Tr);
   return std::string(Buffer.getData(), Buffer.getSize());
}

namespace {

TEST(ApFloatTest, testIsSignaling)
{
   // We test qNaN, -qNaN, +sNaN, -sNaN with and without payloads. *NOTE* The
   // positive/negative distinction is included only since the getQNaN/getSNaN
   // API provides the option.
   ApInt payload = ApInt::getOneBitSet(4, 2);
   EXPECT_FALSE(ApFloat::getQNaN(ApFloat::getIEEEsingle(), false).isSignaling());
   EXPECT_FALSE(ApFloat::getQNaN(ApFloat::getIEEEsingle(), true).isSignaling());
   EXPECT_FALSE(ApFloat::getQNaN(ApFloat::getIEEEsingle(), false, &payload).isSignaling());
   EXPECT_FALSE(ApFloat::getQNaN(ApFloat::getIEEEsingle(), true, &payload).isSignaling());
   EXPECT_TRUE(ApFloat::getSNaN(ApFloat::getIEEEsingle(), false).isSignaling());
   EXPECT_TRUE(ApFloat::getSNaN(ApFloat::getIEEEsingle(), true).isSignaling());
   EXPECT_TRUE(ApFloat::getSNaN(ApFloat::getIEEEsingle(), false, &payload).isSignaling());
   EXPECT_TRUE(ApFloat::getSNaN(ApFloat::getIEEEsingle(), true, &payload).isSignaling());
}

TEST(ApFloatTest, testNext)
{

   ApFloat test(ApFloat::getIEEEquad(), ApFloat::UninitializedTag::uninitialized);
   ApFloat expected(ApFloat::getIEEEquad(), ApFloat::UninitializedTag::uninitialized);

   // 1. Test Special Cases Values.
   //
   // Test all special values for nextUp and nextDown perscribed by IEEE-754R
   // 2008. These are:
   //   1.  +inf
   //   2.  -inf
   //   3.  getLargest()
   //   4.  -getLargest()
   //   5.  getSmallest()
   //   6.  -getSmallest()
   //   7.  qNaN
   //   8.  sNaN
   //   9.  +0
   //   10. -0

   // nextUp(+inf) = +inf.
   test = ApFloat::getInf(ApFloat::getIEEEquad(), false);
   expected = ApFloat::getInf(ApFloat::getIEEEquad(), false);
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.isInfinity());
   EXPECT_TRUE(!test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(+inf) = -nextUp(-inf) = -(-getLargest()) = getLargest()
   test = ApFloat::getInf(ApFloat::getIEEEquad(), false);
   expected = ApFloat::getLargest(ApFloat::getIEEEquad(), false);
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(!test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextUp(-inf) = -getLargest()
   test = ApFloat::getInf(ApFloat::getIEEEquad(), true);
   expected = ApFloat::getLargest(ApFloat::getIEEEquad(), true);
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(-inf) = -nextUp(+inf) = -(+inf) = -inf.
   test = ApFloat::getInf(ApFloat::getIEEEquad(), true);
   expected = ApFloat::getInf(ApFloat::getIEEEquad(), true);
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(test.isInfinity() && test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextUp(getLargest()) = +inf
   test = ApFloat::getLargest(ApFloat::getIEEEquad(), false);
   expected = ApFloat::getInf(ApFloat::getIEEEquad(), false);
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.isInfinity() && !test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(getLargest()) = -nextUp(-getLargest())
   //                        = -(-getLargest() + inc)
   //                        = getLargest() - inc.
   test = ApFloat::getLargest(ApFloat::getIEEEquad(), false);
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "0x1.fffffffffffffffffffffffffffep+16383");
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(!test.isInfinity() && !test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextUp(-getLargest()) = -getLargest() + inc.
   test = ApFloat::getLargest(ApFloat::getIEEEquad(), true);
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "-0x1.fffffffffffffffffffffffffffep+16383");
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(-getLargest()) = -nextUp(getLargest()) = -(inf) = -inf.
   test = ApFloat::getLargest(ApFloat::getIEEEquad(), true);
   expected = ApFloat::getInf(ApFloat::getIEEEquad(), true);
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(test.isInfinity() && test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextUp(getSmallest()) = getSmallest() + inc.
   test = ApFloat(ApFloat::getIEEEquad(), "0x0.0000000000000000000000000001p-16382");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "0x0.0000000000000000000000000002p-16382");
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(getSmallest()) = -nextUp(-getSmallest()) = -(-0) = +0.
   test = ApFloat(ApFloat::getIEEEquad(), "0x0.0000000000000000000000000001p-16382");
   expected = ApFloat::getZero(ApFloat::getIEEEquad(), false);
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(test.isPosZero());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextUp(-getSmallest()) = -0.
   test = ApFloat(ApFloat::getIEEEquad(), "-0x0.0000000000000000000000000001p-16382");
   expected = ApFloat::getZero(ApFloat::getIEEEquad(), true);
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.isNegZero());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(-getSmallest()) = -nextUp(getSmallest()) = -getSmallest() - inc.
   test = ApFloat(ApFloat::getIEEEquad(), "-0x0.0000000000000000000000000001p-16382");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "-0x0.0000000000000000000000000002p-16382");
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextUp(qNaN) = qNaN
   test = ApFloat::getQNaN(ApFloat::getIEEEquad(), false);
   expected = ApFloat::getQNaN(ApFloat::getIEEEquad(), false);
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(qNaN) = qNaN
   test = ApFloat::getQNaN(ApFloat::getIEEEquad(), false);
   expected = ApFloat::getQNaN(ApFloat::getIEEEquad(), false);
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextUp(sNaN) = qNaN
   test = ApFloat::getSNaN(ApFloat::getIEEEquad(), false);
   expected = ApFloat::getQNaN(ApFloat::getIEEEquad(), false);
   EXPECT_EQ(test.next(false), ApFloat::opInvalidOp);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(sNaN) = qNaN
   test = ApFloat::getSNaN(ApFloat::getIEEEquad(), false);
   expected = ApFloat::getQNaN(ApFloat::getIEEEquad(), false);
   EXPECT_EQ(test.next(true), ApFloat::opInvalidOp);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextUp(+0) = +getSmallest()
   test = ApFloat::getZero(ApFloat::getIEEEquad(), false);
   expected = ApFloat::getSmallest(ApFloat::getIEEEquad(), false);
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(+0) = -nextUp(-0) = -getSmallest()
   test = ApFloat::getZero(ApFloat::getIEEEquad(), false);
   expected = ApFloat::getSmallest(ApFloat::getIEEEquad(), true);
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextUp(-0) = +getSmallest()
   test = ApFloat::getZero(ApFloat::getIEEEquad(), true);
   expected = ApFloat::getSmallest(ApFloat::getIEEEquad(), false);
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(-0) = -nextUp(0) = -getSmallest()
   test = ApFloat::getZero(ApFloat::getIEEEquad(), true);
   expected = ApFloat::getSmallest(ApFloat::getIEEEquad(), true);
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // 2. Binade Boundary Tests.

   // 2a. Test denormal <-> normal binade boundaries.
   //     * nextUp(+Largest Denormal) -> +Smallest Normal.
   //     * nextDown(-Largest Denormal) -> -Smallest Normal.
   //     * nextUp(-Smallest Normal) -> -Largest Denormal.
   //     * nextDown(+Smallest Normal) -> +Largest Denormal.

   // nextUp(+Largest Denormal) -> +Smallest Normal.
   test = ApFloat(ApFloat::getIEEEquad(), "0x0.ffffffffffffffffffffffffffffp-16382");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "0x1.0000000000000000000000000000p-16382");
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_FALSE(test.isDenormal());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(-Largest Denormal) -> -Smallest Normal.
   test = ApFloat(ApFloat::getIEEEquad(),
                  "-0x0.ffffffffffffffffffffffffffffp-16382");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "-0x1.0000000000000000000000000000p-16382");
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_FALSE(test.isDenormal());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextUp(-Smallest Normal) -> -LargestDenormal.
   test = ApFloat(ApFloat::getIEEEquad(),
                  "-0x1.0000000000000000000000000000p-16382");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "-0x0.ffffffffffffffffffffffffffffp-16382");
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.isDenormal());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(+Smallest Normal) -> +Largest Denormal.
   test = ApFloat(ApFloat::getIEEEquad(),
                  "+0x1.0000000000000000000000000000p-16382");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "+0x0.ffffffffffffffffffffffffffffp-16382");
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(test.isDenormal());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // 2b. Test normal <-> normal binade boundaries.
   //     * nextUp(-Normal Binade Boundary) -> -Normal Binade Boundary + 1.
   //     * nextDown(+Normal Binade Boundary) -> +Normal Binade Boundary - 1.
   //     * nextUp(+Normal Binade Boundary - 1) -> +Normal Binade Boundary.
   //     * nextDown(-Normal Binade Boundary + 1) -> -Normal Binade Boundary.

   // nextUp(-Normal Binade Boundary) -> -Normal Binade Boundary + 1.
   test = ApFloat(ApFloat::getIEEEquad(), "-0x1p+1");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "-0x1.ffffffffffffffffffffffffffffp+0");
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(+Normal Binade Boundary) -> +Normal Binade Boundary - 1.
   test = ApFloat(ApFloat::getIEEEquad(), "0x1p+1");
   expected = ApFloat(ApFloat::getIEEEquad(), "0x1.ffffffffffffffffffffffffffffp+0");
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextUp(+Normal Binade Boundary - 1) -> +Normal Binade Boundary.
   test = ApFloat(ApFloat::getIEEEquad(), "0x1.ffffffffffffffffffffffffffffp+0");
   expected = ApFloat(ApFloat::getIEEEquad(), "0x1p+1");
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(-Normal Binade Boundary + 1) -> -Normal Binade Boundary.
   test = ApFloat(ApFloat::getIEEEquad(), "-0x1.ffffffffffffffffffffffffffffp+0");
   expected = ApFloat(ApFloat::getIEEEquad(), "-0x1p+1");
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // 2c. Test using next at binade boundaries with a direction away from the
   // binade boundary. Away from denormal <-> normal boundaries.
   //
   // This is to make sure that even though we are at a binade boundary, since
   // we are rounding away, we do not trigger the binade boundary code. Thus we
   // test:
   //   * nextUp(-Largest Denormal) -> -Largest Denormal + inc.
   //   * nextDown(+Largest Denormal) -> +Largest Denormal - inc.
   //   * nextUp(+Smallest Normal) -> +Smallest Normal + inc.
   //   * nextDown(-Smallest Normal) -> -Smallest Normal - inc.

   // nextUp(-Largest Denormal) -> -Largest Denormal + inc.
   test = ApFloat(ApFloat::getIEEEquad(), "-0x0.ffffffffffffffffffffffffffffp-16382");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "-0x0.fffffffffffffffffffffffffffep-16382");
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.isDenormal());
   EXPECT_TRUE(test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(+Largest Denormal) -> +Largest Denormal - inc.
   test = ApFloat(ApFloat::getIEEEquad(), "0x0.ffffffffffffffffffffffffffffp-16382");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "0x0.fffffffffffffffffffffffffffep-16382");
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(test.isDenormal());
   EXPECT_TRUE(!test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextUp(+Smallest Normal) -> +Smallest Normal + inc.
   test = ApFloat(ApFloat::getIEEEquad(), "0x1.0000000000000000000000000000p-16382");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "0x1.0000000000000000000000000001p-16382");
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(!test.isDenormal());
   EXPECT_TRUE(!test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(-Smallest Normal) -> -Smallest Normal - inc.
   test = ApFloat(ApFloat::getIEEEquad(), "-0x1.0000000000000000000000000000p-16382");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "-0x1.0000000000000000000000000001p-16382");
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(!test.isDenormal());
   EXPECT_TRUE(test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // 2d. Test values which cause our exponent to go to min exponent. This
   // is to ensure that guards in the code to check for min exponent
   // trigger properly.
   //     * nextUp(-0x1p-16381) -> -0x1.ffffffffffffffffffffffffffffp-16382
   //     * nextDown(-0x1.ffffffffffffffffffffffffffffp-16382) ->
   //         -0x1p-16381
   //     * nextUp(0x1.ffffffffffffffffffffffffffffp-16382) -> 0x1p-16382
   //     * nextDown(0x1p-16382) -> 0x1.ffffffffffffffffffffffffffffp-16382

   // nextUp(-0x1p-16381) -> -0x1.ffffffffffffffffffffffffffffp-16382
   test = ApFloat(ApFloat::getIEEEquad(), "-0x1p-16381");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "-0x1.ffffffffffffffffffffffffffffp-16382");
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(-0x1.ffffffffffffffffffffffffffffp-16382) ->
   //         -0x1p-16381
   test = ApFloat(ApFloat::getIEEEquad(), "-0x1.ffffffffffffffffffffffffffffp-16382");
   expected = ApFloat(ApFloat::getIEEEquad(), "-0x1p-16381");
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextUp(0x1.ffffffffffffffffffffffffffffp-16382) -> 0x1p-16381
   test = ApFloat(ApFloat::getIEEEquad(), "0x1.ffffffffffffffffffffffffffffp-16382");
   expected = ApFloat(ApFloat::getIEEEquad(), "0x1p-16381");
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(0x1p-16381) -> 0x1.ffffffffffffffffffffffffffffp-16382
   test = ApFloat(ApFloat::getIEEEquad(), "0x1p-16381");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "0x1.ffffffffffffffffffffffffffffp-16382");
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // 3. Now we test both denormal/normal computation which will not cause us
   // to go across binade boundaries. Specifically we test:
   //   * nextUp(+Denormal) -> +Denormal.
   //   * nextDown(+Denormal) -> +Denormal.
   //   * nextUp(-Denormal) -> -Denormal.
   //   * nextDown(-Denormal) -> -Denormal.
   //   * nextUp(+Normal) -> +Normal.
   //   * nextDown(+Normal) -> +Normal.
   //   * nextUp(-Normal) -> -Normal.
   //   * nextDown(-Normal) -> -Normal.

   // nextUp(+Denormal) -> +Denormal.
   test = ApFloat(ApFloat::getIEEEquad(),
                  "0x0.ffffffffffffffffffffffff000cp-16382");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "0x0.ffffffffffffffffffffffff000dp-16382");
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.isDenormal());
   EXPECT_TRUE(!test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(+Denormal) -> +Denormal.
   test = ApFloat(ApFloat::getIEEEquad(),
                  "0x0.ffffffffffffffffffffffff000cp-16382");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "0x0.ffffffffffffffffffffffff000bp-16382");
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(test.isDenormal());
   EXPECT_TRUE(!test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextUp(-Denormal) -> -Denormal.
   test = ApFloat(ApFloat::getIEEEquad(),
                  "-0x0.ffffffffffffffffffffffff000cp-16382");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "-0x0.ffffffffffffffffffffffff000bp-16382");
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(test.isDenormal());
   EXPECT_TRUE(test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(-Denormal) -> -Denormal
   test = ApFloat(ApFloat::getIEEEquad(),
                  "-0x0.ffffffffffffffffffffffff000cp-16382");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "-0x0.ffffffffffffffffffffffff000dp-16382");
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(test.isDenormal());
   EXPECT_TRUE(test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextUp(+Normal) -> +Normal.
   test = ApFloat(ApFloat::getIEEEquad(),
                  "0x1.ffffffffffffffffffffffff000cp-16000");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "0x1.ffffffffffffffffffffffff000dp-16000");
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(!test.isDenormal());
   EXPECT_TRUE(!test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(+Normal) -> +Normal.
   test = ApFloat(ApFloat::getIEEEquad(),
                  "0x1.ffffffffffffffffffffffff000cp-16000");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "0x1.ffffffffffffffffffffffff000bp-16000");
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(!test.isDenormal());
   EXPECT_TRUE(!test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextUp(-Normal) -> -Normal.
   test = ApFloat(ApFloat::getIEEEquad(),
                  "-0x1.ffffffffffffffffffffffff000cp-16000");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "-0x1.ffffffffffffffffffffffff000bp-16000");
   EXPECT_EQ(test.next(false), ApFloat::opOK);
   EXPECT_TRUE(!test.isDenormal());
   EXPECT_TRUE(test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   // nextDown(-Normal) -> -Normal.
   test = ApFloat(ApFloat::getIEEEquad(),
                  "-0x1.ffffffffffffffffffffffff000cp-16000");
   expected = ApFloat(ApFloat::getIEEEquad(),
                      "-0x1.ffffffffffffffffffffffff000dp-16000");
   EXPECT_EQ(test.next(true), ApFloat::opOK);
   EXPECT_TRUE(!test.isDenormal());
   EXPECT_TRUE(test.isNegative());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));
}

TEST(ApFloatTest, testFMA)
{
   ApFloat::RoundingMode rdmd = ApFloat::RoundingMode::rmNearestTiesToEven;

   {
      ApFloat f1(14.5f);
      ApFloat f2(-14.5f);
      ApFloat f3(225.0f);
      f1.fusedMultiplyAdd(f2, f3, ApFloat::RoundingMode::rmNearestTiesToEven);
      EXPECT_EQ(14.75f, f1.convertToFloat());
   }

   {
      ApFloat Val2(2.0f);
      ApFloat f1((float)1.17549435e-38F);
      ApFloat f2((float)1.17549435e-38F);
      f1.divide(Val2, rdmd);
      f2.divide(Val2, rdmd);
      ApFloat f3(12.0f);
      f1.fusedMultiplyAdd(f2, f3, ApFloat::RoundingMode::rmNearestTiesToEven);
      EXPECT_EQ(12.0f, f1.convertToFloat());
   }

   // Test for correct zero sign when answer is exactly zero.
   // fma(1.0, -1.0, 1.0) -> +ve 0.
   {
      ApFloat f1(1.0);
      ApFloat f2(-1.0);
      ApFloat f3(1.0);
      f1.fusedMultiplyAdd(f2, f3, ApFloat::RoundingMode::rmNearestTiesToEven);
      EXPECT_TRUE(!f1.isNegative() && f1.isZero());
   }

   // Test for correct zero sign when answer is exactly zero and rounding towards
   // negative.
   // fma(1.0, -1.0, 1.0) -> +ve 0.
   {
      ApFloat f1(1.0);
      ApFloat f2(-1.0);
      ApFloat f3(1.0);
      f1.fusedMultiplyAdd(f2, f3, ApFloat::RoundingMode::rmTowardNegative);
      EXPECT_TRUE(f1.isNegative() && f1.isZero());
   }

   // Test for correct (in this case -ve) sign when adding like signed zeros.
   // Test fma(0.0, -0.0, -0.0) -> -ve 0.
   {
      ApFloat f1(0.0);
      ApFloat f2(-0.0);
      ApFloat f3(-0.0);
      f1.fusedMultiplyAdd(f2, f3, ApFloat::RoundingMode::rmNearestTiesToEven);
      EXPECT_TRUE(f1.isNegative() && f1.isZero());
   }

   // Test -ve sign preservation when small negative results underflow.
   {
      ApFloat f1(ApFloat::getIEEEdouble(),  "-0x1p-1074");
      ApFloat f2(ApFloat::getIEEEdouble(), "+0x1p-1074");
      ApFloat f3(0.0);
      f1.fusedMultiplyAdd(f2, f3, ApFloat::RoundingMode::rmNearestTiesToEven);
      EXPECT_TRUE(f1.isNegative() && f1.isZero());
   }

   // Test x87 extended precision case from http://llvm.org/PR20728.
   {
      ApFloat M1(ApFloat::getX87DoubleExtended(), 1.0);
      ApFloat M2(ApFloat::getX87DoubleExtended(), 1.0);
      ApFloat A(ApFloat::getX87DoubleExtended(), 3.0);

      bool losesInfo = false;
      M1.fusedMultiplyAdd(M1, A, ApFloat::RoundingMode::rmNearestTiesToEven);
      M1.convert(ApFloat::getIEEEsingle(), ApFloat::RoundingMode::rmNearestTiesToEven, &losesInfo);
      EXPECT_FALSE(losesInfo);
      EXPECT_EQ(4.0f, M1.convertToFloat());
   }
}

TEST(ApFloatTest, testMinNum)
{
   ApFloat f1(1.0);
   ApFloat f2(2.0);
   ApFloat nan = ApFloat::getNaN(ApFloat::getIEEEdouble());

   EXPECT_EQ(1.0, minnum(f1, f2).convertToDouble());
   EXPECT_EQ(1.0, minnum(f2, f1).convertToDouble());
   EXPECT_EQ(1.0, minnum(f1, nan).convertToDouble());
   EXPECT_EQ(1.0, minnum(nan, f1).convertToDouble());
}

TEST(ApFloatTest, testMaxNum)
{
   ApFloat f1(1.0);
   ApFloat f2(2.0);
   ApFloat nan = ApFloat::getNaN(ApFloat::getIEEEdouble());

   EXPECT_EQ(2.0, maxnum(f1, f2).convertToDouble());
   EXPECT_EQ(2.0, maxnum(f2, f1).convertToDouble());
   EXPECT_EQ(1.0, maxnum(f1, nan).convertToDouble());
   EXPECT_EQ(1.0, maxnum(nan, f1).convertToDouble());
}


TEST(APFloatTest, testMinimum)
{
  ApFloat f1(1.0);
  ApFloat f2(2.0);
  ApFloat zp(0.0);
  ApFloat zn(-0.0);
  ApFloat nan = ApFloat::getNaN(ApFloat::getIEEEdouble());

  EXPECT_EQ(1.0, minimum(f1, f2).convertToDouble());
  EXPECT_EQ(1.0, minimum(f2, f1).convertToDouble());
  EXPECT_EQ(-0.0, minimum(zp, zn).convertToDouble());
  EXPECT_EQ(-0.0, minimum(zn, zp).convertToDouble());
  EXPECT_TRUE(std::isnan(minimum(f1, nan).convertToDouble()));
  EXPECT_TRUE(std::isnan(minimum(nan, f1).convertToDouble()));
}

TEST(APFloatTest, testMaximum)
{
  ApFloat f1(1.0);
  ApFloat f2(2.0);
  ApFloat zp(0.0);
  ApFloat zn(-0.0);
  ApFloat nan = ApFloat::getNaN(ApFloat::getIEEEdouble());

  EXPECT_EQ(2.0, maximum(f1, f2).convertToDouble());
  EXPECT_EQ(2.0, maximum(f2, f1).convertToDouble());
  EXPECT_EQ(0.0, maximum(zp, zn).convertToDouble());
  EXPECT_EQ(0.0, maximum(zn, zp).convertToDouble());
  EXPECT_TRUE(std::isnan(maximum(f1, nan).convertToDouble()));
  EXPECT_TRUE(std::isnan(maximum(nan, f1).convertToDouble()));
}

TEST(ApFloatTest, testDenormal)
{
   ApFloat::RoundingMode rdmd = ApFloat::RoundingMode::rmNearestTiesToEven;

   // Test single precision
   {
      const char *MinNormalStr = "1.17549435082228750797e-38";
      EXPECT_FALSE(ApFloat(ApFloat::getIEEEsingle(), MinNormalStr).isDenormal());
      EXPECT_FALSE(ApFloat(ApFloat::getIEEEsingle(), 0.0).isDenormal());

      ApFloat Val2(ApFloat::getIEEEsingle(), 2.0e0);
      ApFloat T(ApFloat::getIEEEsingle(), MinNormalStr);
      T.divide(Val2, rdmd);
      EXPECT_TRUE(T.isDenormal());
   }

   // Test double precision
   {
      const char *MinNormalStr = "2.22507385850720138309e-308";
      EXPECT_FALSE(ApFloat(ApFloat::getIEEEdouble(), MinNormalStr).isDenormal());
      EXPECT_FALSE(ApFloat(ApFloat::getIEEEdouble(), 0.0).isDenormal());

      ApFloat Val2(ApFloat::getIEEEdouble(), 2.0e0);
      ApFloat T(ApFloat::getIEEEdouble(), MinNormalStr);
      T.divide(Val2, rdmd);
      EXPECT_TRUE(T.isDenormal());
   }

   // Test Intel double-ext
   {
      const char *MinNormalStr = "3.36210314311209350626e-4932";
      EXPECT_FALSE(ApFloat(ApFloat::getX87DoubleExtended(), MinNormalStr).isDenormal());
      EXPECT_FALSE(ApFloat(ApFloat::getX87DoubleExtended(), 0.0).isDenormal());

      ApFloat Val2(ApFloat::getX87DoubleExtended(), 2.0e0);
      ApFloat T(ApFloat::getX87DoubleExtended(), MinNormalStr);
      T.divide(Val2, rdmd);
      EXPECT_TRUE(T.isDenormal());
   }

   // Test quadruple precision
   {
      const char *MinNormalStr = "3.36210314311209350626267781732175260e-4932";
      EXPECT_FALSE(ApFloat(ApFloat::getIEEEquad(), MinNormalStr).isDenormal());
      EXPECT_FALSE(ApFloat(ApFloat::getIEEEquad(), 0.0).isDenormal());

      ApFloat Val2(ApFloat::getIEEEquad(), 2.0e0);
      ApFloat T(ApFloat::getIEEEquad(), MinNormalStr);
      T.divide(Val2, rdmd);
      EXPECT_TRUE(T.isDenormal());
   }
}

TEST(ApFloatTest, testZero)
{
   EXPECT_EQ(0.0f,  ApFloat(0.0f).convertToFloat());
   EXPECT_EQ(-0.0f, ApFloat(-0.0f).convertToFloat());
   EXPECT_TRUE(ApFloat(-0.0f).isNegative());

   EXPECT_EQ(0.0,  ApFloat(0.0).convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(-0.0).convertToDouble());
   EXPECT_TRUE(ApFloat(-0.0).isNegative());
}

TEST(ApFloatTest, testDecimalStringsWithoutNullTerminators)
{
   // Make sure that we can parse strings without null terminators.
   // rdar://14323230.
   ApFloat value(ApFloat::getIEEEdouble());
   value.convertFromString(StringRef("0.00", 3),
                           ApFloat::RoundingMode::rmNearestTiesToEven);
   EXPECT_EQ(value.convertToDouble(), 0.0);
   value.convertFromString(StringRef("0.01", 3),
                           ApFloat::RoundingMode::rmNearestTiesToEven);
   EXPECT_EQ(value.convertToDouble(), 0.0);
   value.convertFromString(StringRef("0.09", 3),
                           ApFloat::RoundingMode::rmNearestTiesToEven);
   EXPECT_EQ(value.convertToDouble(), 0.0);
   value.convertFromString(StringRef("0.095", 4),
                           ApFloat::RoundingMode::rmNearestTiesToEven);
   EXPECT_EQ(value.convertToDouble(), 0.09);
   value.convertFromString(StringRef("0.00e+3", 7),
                           ApFloat::RoundingMode::rmNearestTiesToEven);
   EXPECT_EQ(value.convertToDouble(), 0.00);
   value.convertFromString(StringRef("0e+3", 4),
                           ApFloat::RoundingMode::rmNearestTiesToEven);
   EXPECT_EQ(value.convertToDouble(), 0.00);

}

TEST(ApFloatTest, testFromZeroDecimalString)
{
   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0.").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0.").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0.").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  ".0").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+.0").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-.0").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0.0").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0.0").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0.0").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "00000.").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+00000.").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-00000.").convertToDouble());

   EXPECT_EQ(0.0,  ApFloat(ApFloat::getIEEEdouble(), ".00000").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+.00000").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-.00000").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0000.00000").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0000.00000").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0000.00000").convertToDouble());
}

TEST(ApFloatTest, testFromZeroDecimalSingleExponentString)
{
   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),   "0e1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(),  "+0e1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(),  "-0e1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0e+1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0e+1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0e+1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0e-1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0e-1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0e-1").convertToDouble());


   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),   "0.e1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(),  "+0.e1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(),  "-0.e1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0.e+1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0.e+1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0.e+1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0.e-1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0.e-1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0.e-1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),   ".0e1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(),  "+.0e1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(),  "-.0e1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  ".0e+1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+.0e+1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-.0e+1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  ".0e-1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+.0e-1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-.0e-1").convertToDouble());


   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),   "0.0e1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(),  "+0.0e1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(),  "-0.0e1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0.0e+1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0.0e+1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0.0e+1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0.0e-1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0.0e-1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0.0e-1").convertToDouble());


   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "000.0000e1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+000.0000e+1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-000.0000e+1").convertToDouble());
}

TEST(ApFloatTest, testFromZeroDecimalLargeExponentString)
{
   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0e1234").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0e1234").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0e1234").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0e+1234").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0e+1234").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0e+1234").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0e-1234").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0e-1234").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0e-1234").convertToDouble());

   EXPECT_EQ(0.0,  ApFloat(ApFloat::getIEEEdouble(), "000.0000e1234").convertToDouble());
   EXPECT_EQ(0.0,  ApFloat(ApFloat::getIEEEdouble(), "000.0000e-1234").convertToDouble());

   EXPECT_EQ(0.0,  ApFloat(ApFloat::getIEEEdouble(), StringRef("0e1234" "\0" "2", 6)).convertToDouble());
}

TEST(ApFloatTest, testFromZeroHexadecimalString)
{
   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0x0p1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0x0p1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0x0p1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0x0p+1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0x0p+1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0x0p+1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0x0p-1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0x0p-1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0x0p-1").convertToDouble());


   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0x0.p1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0x0.p1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0x0.p1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0x0.p+1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0x0.p+1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0x0.p+1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0x0.p-1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0x0.p-1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0x0.p-1").convertToDouble());


   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0x.0p1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0x.0p1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0x.0p1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0x.0p+1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0x.0p+1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0x.0p+1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0x.0p-1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0x.0p-1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0x.0p-1").convertToDouble());


   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0x0.0p1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0x0.0p1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0x0.0p1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0x0.0p+1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0x0.0p+1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0x0.0p+1").convertToDouble());

   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(),  "0x0.0p-1").convertToDouble());
   EXPECT_EQ(+0.0, ApFloat(ApFloat::getIEEEdouble(), "+0x0.0p-1").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0x0.0p-1").convertToDouble());


   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(), "0x00000.p1").convertToDouble());
   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(), "0x0000.00000p1").convertToDouble());
   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(), "0x.00000p1").convertToDouble());
   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(), "0x0.p1").convertToDouble());
   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(), "0x0p1234").convertToDouble());
   EXPECT_EQ(-0.0, ApFloat(ApFloat::getIEEEdouble(), "-0x0p1234").convertToDouble());
   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(), "0x00000.p1234").convertToDouble());
   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(), "0x0000.00000p1234").convertToDouble());
   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(), "0x.00000p1234").convertToDouble());
   EXPECT_EQ( 0.0, ApFloat(ApFloat::getIEEEdouble(), "0x0.p1234").convertToDouble());
}

TEST(ApFloatTest, testFromDecimalString)
{
   EXPECT_EQ(1.0,      ApFloat(ApFloat::getIEEEdouble(), "1").convertToDouble());
   EXPECT_EQ(2.0,      ApFloat(ApFloat::getIEEEdouble(), "2.").convertToDouble());
   EXPECT_EQ(0.5,      ApFloat(ApFloat::getIEEEdouble(), ".5").convertToDouble());
   EXPECT_EQ(1.0,      ApFloat(ApFloat::getIEEEdouble(), "1.0").convertToDouble());
   EXPECT_EQ(-2.0,     ApFloat(ApFloat::getIEEEdouble(), "-2").convertToDouble());
   EXPECT_EQ(-4.0,     ApFloat(ApFloat::getIEEEdouble(), "-4.").convertToDouble());
   EXPECT_EQ(-0.5,     ApFloat(ApFloat::getIEEEdouble(), "-.5").convertToDouble());
   EXPECT_EQ(-1.5,     ApFloat(ApFloat::getIEEEdouble(), "-1.5").convertToDouble());
   EXPECT_EQ(1.25e12,  ApFloat(ApFloat::getIEEEdouble(), "1.25e12").convertToDouble());
   EXPECT_EQ(1.25e+12, ApFloat(ApFloat::getIEEEdouble(), "1.25e+12").convertToDouble());
   EXPECT_EQ(1.25e-12, ApFloat(ApFloat::getIEEEdouble(), "1.25e-12").convertToDouble());
   EXPECT_EQ(1024.0,   ApFloat(ApFloat::getIEEEdouble(), "1024.").convertToDouble());
   EXPECT_EQ(1024.05,  ApFloat(ApFloat::getIEEEdouble(), "1024.05000").convertToDouble());
   EXPECT_EQ(0.05,     ApFloat(ApFloat::getIEEEdouble(), ".05000").convertToDouble());
   EXPECT_EQ(2.0,      ApFloat(ApFloat::getIEEEdouble(), "2.").convertToDouble());
   EXPECT_EQ(2.0e2,    ApFloat(ApFloat::getIEEEdouble(), "2.e2").convertToDouble());
   EXPECT_EQ(2.0e+2,   ApFloat(ApFloat::getIEEEdouble(), "2.e+2").convertToDouble());
   EXPECT_EQ(2.0e-2,   ApFloat(ApFloat::getIEEEdouble(), "2.e-2").convertToDouble());
   EXPECT_EQ(2.05e2,    ApFloat(ApFloat::getIEEEdouble(), "002.05000e2").convertToDouble());
   EXPECT_EQ(2.05e+2,   ApFloat(ApFloat::getIEEEdouble(), "002.05000e+2").convertToDouble());
   EXPECT_EQ(2.05e-2,   ApFloat(ApFloat::getIEEEdouble(), "002.05000e-2").convertToDouble());
   EXPECT_EQ(2.05e12,   ApFloat(ApFloat::getIEEEdouble(), "002.05000e12").convertToDouble());
   EXPECT_EQ(2.05e+12,  ApFloat(ApFloat::getIEEEdouble(), "002.05000e+12").convertToDouble());
   EXPECT_EQ(2.05e-12,  ApFloat(ApFloat::getIEEEdouble(), "002.05000e-12").convertToDouble());

   // These are "carefully selected" to overflow the fast log-base
   // calculations in ApFloat.cpp
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "99e99999").isInfinity());
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "-99e99999").isInfinity());
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "1e-99999").isPosZero());
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "-1e-99999").isNegZero());

   EXPECT_EQ(2.71828, convertToDoubleFromString("2.71828"));
}

TEST(ApFloatTest, testFromToStringSpecials)
{
   auto expects = [] (const char *first, const char *second) {
      std::string roundtrip = convertToString(convertToDoubleFromString(second), 0, 3);
      EXPECT_STREQ(first, roundtrip.c_str());
   };
   expects("+Inf", "+Inf");
   expects("+Inf", "INFINITY");
   expects("+Inf", "inf");
   expects("-Inf", "-Inf");
   expects("-Inf", "-INFINITY");
   expects("-Inf", "-inf");
   expects("NaN", "NaN");
   expects("NaN", "nan");
   expects("NaN", "-NaN");
   expects("NaN", "-nan");
}

TEST(ApFloatTest, testFromHexadecimalString)
{
   EXPECT_EQ( 1.0, ApFloat(ApFloat::getIEEEdouble(),  "0x1p0").convertToDouble());
   EXPECT_EQ(+1.0, ApFloat(ApFloat::getIEEEdouble(), "+0x1p0").convertToDouble());
   EXPECT_EQ(-1.0, ApFloat(ApFloat::getIEEEdouble(), "-0x1p0").convertToDouble());

   EXPECT_EQ( 1.0, ApFloat(ApFloat::getIEEEdouble(),  "0x1p+0").convertToDouble());
   EXPECT_EQ(+1.0, ApFloat(ApFloat::getIEEEdouble(), "+0x1p+0").convertToDouble());
   EXPECT_EQ(-1.0, ApFloat(ApFloat::getIEEEdouble(), "-0x1p+0").convertToDouble());

   EXPECT_EQ( 1.0, ApFloat(ApFloat::getIEEEdouble(),  "0x1p-0").convertToDouble());
   EXPECT_EQ(+1.0, ApFloat(ApFloat::getIEEEdouble(), "+0x1p-0").convertToDouble());
   EXPECT_EQ(-1.0, ApFloat(ApFloat::getIEEEdouble(), "-0x1p-0").convertToDouble());


   EXPECT_EQ( 2.0, ApFloat(ApFloat::getIEEEdouble(),  "0x1p1").convertToDouble());
   EXPECT_EQ(+2.0, ApFloat(ApFloat::getIEEEdouble(), "+0x1p1").convertToDouble());
   EXPECT_EQ(-2.0, ApFloat(ApFloat::getIEEEdouble(), "-0x1p1").convertToDouble());

   EXPECT_EQ( 2.0, ApFloat(ApFloat::getIEEEdouble(),  "0x1p+1").convertToDouble());
   EXPECT_EQ(+2.0, ApFloat(ApFloat::getIEEEdouble(), "+0x1p+1").convertToDouble());
   EXPECT_EQ(-2.0, ApFloat(ApFloat::getIEEEdouble(), "-0x1p+1").convertToDouble());

   EXPECT_EQ( 0.5, ApFloat(ApFloat::getIEEEdouble(),  "0x1p-1").convertToDouble());
   EXPECT_EQ(+0.5, ApFloat(ApFloat::getIEEEdouble(), "+0x1p-1").convertToDouble());
   EXPECT_EQ(-0.5, ApFloat(ApFloat::getIEEEdouble(), "-0x1p-1").convertToDouble());


   EXPECT_EQ( 3.0, ApFloat(ApFloat::getIEEEdouble(),  "0x1.8p1").convertToDouble());
   EXPECT_EQ(+3.0, ApFloat(ApFloat::getIEEEdouble(), "+0x1.8p1").convertToDouble());
   EXPECT_EQ(-3.0, ApFloat(ApFloat::getIEEEdouble(), "-0x1.8p1").convertToDouble());

   EXPECT_EQ( 3.0, ApFloat(ApFloat::getIEEEdouble(),  "0x1.8p+1").convertToDouble());
   EXPECT_EQ(+3.0, ApFloat(ApFloat::getIEEEdouble(), "+0x1.8p+1").convertToDouble());
   EXPECT_EQ(-3.0, ApFloat(ApFloat::getIEEEdouble(), "-0x1.8p+1").convertToDouble());

   EXPECT_EQ( 0.75, ApFloat(ApFloat::getIEEEdouble(),  "0x1.8p-1").convertToDouble());
   EXPECT_EQ(+0.75, ApFloat(ApFloat::getIEEEdouble(), "+0x1.8p-1").convertToDouble());
   EXPECT_EQ(-0.75, ApFloat(ApFloat::getIEEEdouble(), "-0x1.8p-1").convertToDouble());


   EXPECT_EQ( 8192.0, ApFloat(ApFloat::getIEEEdouble(),  "0x1000.000p1").convertToDouble());
   EXPECT_EQ(+8192.0, ApFloat(ApFloat::getIEEEdouble(), "+0x1000.000p1").convertToDouble());
   EXPECT_EQ(-8192.0, ApFloat(ApFloat::getIEEEdouble(), "-0x1000.000p1").convertToDouble());

   EXPECT_EQ( 8192.0, ApFloat(ApFloat::getIEEEdouble(),  "0x1000.000p+1").convertToDouble());
   EXPECT_EQ(+8192.0, ApFloat(ApFloat::getIEEEdouble(), "+0x1000.000p+1").convertToDouble());
   EXPECT_EQ(-8192.0, ApFloat(ApFloat::getIEEEdouble(), "-0x1000.000p+1").convertToDouble());

   EXPECT_EQ( 2048.0, ApFloat(ApFloat::getIEEEdouble(),  "0x1000.000p-1").convertToDouble());
   EXPECT_EQ(+2048.0, ApFloat(ApFloat::getIEEEdouble(), "+0x1000.000p-1").convertToDouble());
   EXPECT_EQ(-2048.0, ApFloat(ApFloat::getIEEEdouble(), "-0x1000.000p-1").convertToDouble());


   EXPECT_EQ( 8192.0, ApFloat(ApFloat::getIEEEdouble(),  "0x1000p1").convertToDouble());
   EXPECT_EQ(+8192.0, ApFloat(ApFloat::getIEEEdouble(), "+0x1000p1").convertToDouble());
   EXPECT_EQ(-8192.0, ApFloat(ApFloat::getIEEEdouble(), "-0x1000p1").convertToDouble());

   EXPECT_EQ( 8192.0, ApFloat(ApFloat::getIEEEdouble(),  "0x1000p+1").convertToDouble());
   EXPECT_EQ(+8192.0, ApFloat(ApFloat::getIEEEdouble(), "+0x1000p+1").convertToDouble());
   EXPECT_EQ(-8192.0, ApFloat(ApFloat::getIEEEdouble(), "-0x1000p+1").convertToDouble());

   EXPECT_EQ( 2048.0, ApFloat(ApFloat::getIEEEdouble(),  "0x1000p-1").convertToDouble());
   EXPECT_EQ(+2048.0, ApFloat(ApFloat::getIEEEdouble(), "+0x1000p-1").convertToDouble());
   EXPECT_EQ(-2048.0, ApFloat(ApFloat::getIEEEdouble(), "-0x1000p-1").convertToDouble());


   EXPECT_EQ( 16384.0, ApFloat(ApFloat::getIEEEdouble(),  "0x10p10").convertToDouble());
   EXPECT_EQ(+16384.0, ApFloat(ApFloat::getIEEEdouble(), "+0x10p10").convertToDouble());
   EXPECT_EQ(-16384.0, ApFloat(ApFloat::getIEEEdouble(), "-0x10p10").convertToDouble());

   EXPECT_EQ( 16384.0, ApFloat(ApFloat::getIEEEdouble(),  "0x10p+10").convertToDouble());
   EXPECT_EQ(+16384.0, ApFloat(ApFloat::getIEEEdouble(), "+0x10p+10").convertToDouble());
   EXPECT_EQ(-16384.0, ApFloat(ApFloat::getIEEEdouble(), "-0x10p+10").convertToDouble());

   EXPECT_EQ( 0.015625, ApFloat(ApFloat::getIEEEdouble(),  "0x10p-10").convertToDouble());
   EXPECT_EQ(+0.015625, ApFloat(ApFloat::getIEEEdouble(), "+0x10p-10").convertToDouble());
   EXPECT_EQ(-0.015625, ApFloat(ApFloat::getIEEEdouble(), "-0x10p-10").convertToDouble());

   EXPECT_EQ(1.0625, ApFloat(ApFloat::getIEEEdouble(), "0x1.1p0").convertToDouble());
   EXPECT_EQ(1.0, ApFloat(ApFloat::getIEEEdouble(), "0x1p0").convertToDouble());

   EXPECT_EQ(convertToDoubleFromString("0x1p-150"),
             convertToDoubleFromString("+0x800000000000000001.p-221"));
   EXPECT_EQ(2251799813685248.5,
             convertToDoubleFromString("0x80000000000004000000.010p-28"));
}

TEST(ApFloatTest, testToString)
{
   ASSERT_EQ("10", convertToString(10.0, 6, 3));
   ASSERT_EQ("1.0E+1", convertToString(10.0, 6, 0));
   ASSERT_EQ("10100", convertToString(1.01E+4, 5, 2));
   ASSERT_EQ("1.01E+4", convertToString(1.01E+4, 4, 2));
   ASSERT_EQ("1.01E+4", convertToString(1.01E+4, 5, 1));
   ASSERT_EQ("0.0101", convertToString(1.01E-2, 5, 2));
   ASSERT_EQ("0.0101", convertToString(1.01E-2, 4, 2));
   ASSERT_EQ("1.01E-2", convertToString(1.01E-2, 5, 1));
   ASSERT_EQ("0.78539816339744828", convertToString(0.78539816339744830961, 0, 3));
   ASSERT_EQ("4.9406564584124654E-324", convertToString(4.9406564584124654e-324, 0, 3));
   ASSERT_EQ("873.18340000000001", convertToString(873.1834, 0, 1));
   ASSERT_EQ("8.7318340000000001E+2", convertToString(873.1834, 0, 0));
   ASSERT_EQ("1.7976931348623157E+308", convertToString(1.7976931348623157E+308, 0, 0));
   ASSERT_EQ("10", convertToString(10.0, 6, 3, false));
   ASSERT_EQ("1.000000e+01", convertToString(10.0, 6, 0, false));
   ASSERT_EQ("10100", convertToString(1.01E+4, 5, 2, false));
   ASSERT_EQ("1.0100e+04", convertToString(1.01E+4, 4, 2, false));
   ASSERT_EQ("1.01000e+04", convertToString(1.01E+4, 5, 1, false));
   ASSERT_EQ("0.0101", convertToString(1.01E-2, 5, 2, false));
   ASSERT_EQ("0.0101", convertToString(1.01E-2, 4, 2, false));
   ASSERT_EQ("1.01000e-02", convertToString(1.01E-2, 5, 1, false));
   ASSERT_EQ("0.78539816339744828",
             convertToString(0.78539816339744830961, 0, 3, false));
   ASSERT_EQ("4.94065645841246540e-324",
             convertToString(4.9406564584124654e-324, 0, 3, false));
   ASSERT_EQ("873.18340000000001", convertToString(873.1834, 0, 1, false));
   ASSERT_EQ("8.73183400000000010e+02", convertToString(873.1834, 0, 0, false));
   ASSERT_EQ("1.79769313486231570e+308",
             convertToString(1.7976931348623157E+308, 0, 0, false));

   {
      SmallString<64> str;
      ApFloat unnormalZero(ApFloat::getX87DoubleExtended(), ApInt(80, {0, 1}));
      unnormalZero.toString(str);
      ASSERT_EQ("NaN", str);
   }
}

TEST(ApFloatTest, testToInteger)
{
   bool isExact = false;
   ApSInt result(5, /*isUnsigned=*/true);

   EXPECT_EQ(ApFloat::opOK,
             ApFloat(ApFloat::getIEEEdouble(), "10")
             .convertToInteger(result, ApFloat::RoundingMode::rmTowardZero, &isExact));
   EXPECT_TRUE(isExact);
   EXPECT_EQ(ApSInt(ApInt(5, 10), true), result);

   EXPECT_EQ(ApFloat::opInvalidOp,
             ApFloat(ApFloat::getIEEEdouble(), "-10")
             .convertToInteger(result, ApFloat::RoundingMode::rmTowardZero, &isExact));
   EXPECT_FALSE(isExact);
   EXPECT_EQ(ApSInt::getMinValue(5, true), result);

   EXPECT_EQ(ApFloat::opInvalidOp,
             ApFloat(ApFloat::getIEEEdouble(), "32")
             .convertToInteger(result, ApFloat::RoundingMode::rmTowardZero, &isExact));
   EXPECT_FALSE(isExact);
   EXPECT_EQ(ApSInt::getMaxValue(5, true), result);

   EXPECT_EQ(ApFloat::opInexact,
             ApFloat(ApFloat::getIEEEdouble(), "7.9")
             .convertToInteger(result, ApFloat::RoundingMode::rmTowardZero, &isExact));
   EXPECT_FALSE(isExact);
   EXPECT_EQ(ApSInt(ApInt(5, 7), true), result);

   result.setIsUnsigned(false);
   EXPECT_EQ(ApFloat::opOK,
             ApFloat(ApFloat::getIEEEdouble(), "-10")
             .convertToInteger(result, ApFloat::RoundingMode::rmTowardZero, &isExact));
   EXPECT_TRUE(isExact);
   EXPECT_EQ(ApSInt(ApInt(5, -10, true), false), result);

   EXPECT_EQ(ApFloat::opInvalidOp,
             ApFloat(ApFloat::getIEEEdouble(), "-17")
             .convertToInteger(result, ApFloat::RoundingMode::rmTowardZero, &isExact));
   EXPECT_FALSE(isExact);
   EXPECT_EQ(ApSInt::getMinValue(5, false), result);

   EXPECT_EQ(ApFloat::opInvalidOp,
             ApFloat(ApFloat::getIEEEdouble(), "16")
             .convertToInteger(result, ApFloat::RoundingMode::rmTowardZero, &isExact));
   EXPECT_FALSE(isExact);
   EXPECT_EQ(ApSInt::getMaxValue(5, false), result);
}

static ApInt nanbits(const FltSemantics &Sem,
                     bool SNaN, bool Negative, uint64_t fill) {
   ApInt apfill(64, fill);
   if (SNaN)
      return ApFloat::getSNaN(Sem, Negative, &apfill).bitcastToApInt();
   else
      return ApFloat::getQNaN(Sem, Negative, &apfill).bitcastToApInt();
}

TEST(ApFloatTest, testMakeNaN)
{
   ASSERT_EQ(0x7fc00000, nanbits(ApFloat::getIEEEsingle(), false, false, 0));
   ASSERT_EQ(0xffc00000, nanbits(ApFloat::getIEEEsingle(), false, true, 0));
   ASSERT_EQ(0x7fc0ae72, nanbits(ApFloat::getIEEEsingle(), false, false, 0xae72));
   ASSERT_EQ(0x7fffae72, nanbits(ApFloat::getIEEEsingle(), false, false, 0xffffae72));
   ASSERT_EQ(0x7fa00000, nanbits(ApFloat::getIEEEsingle(), true, false, 0));
   ASSERT_EQ(0xffa00000, nanbits(ApFloat::getIEEEsingle(), true, true, 0));
   ASSERT_EQ(0x7f80ae72, nanbits(ApFloat::getIEEEsingle(), true, false, 0xae72));
   ASSERT_EQ(0x7fbfae72, nanbits(ApFloat::getIEEEsingle(), true, false, 0xffffae72));

   ASSERT_EQ(0x7ff8000000000000ULL, nanbits(ApFloat::getIEEEdouble(), false, false, 0));
   ASSERT_EQ(0xfff8000000000000ULL, nanbits(ApFloat::getIEEEdouble(), false, true, 0));
   ASSERT_EQ(0x7ff800000000ae72ULL, nanbits(ApFloat::getIEEEdouble(), false, false, 0xae72));
   ASSERT_EQ(0x7fffffffffffae72ULL, nanbits(ApFloat::getIEEEdouble(), false, false, 0xffffffffffffae72ULL));
   ASSERT_EQ(0x7ff4000000000000ULL, nanbits(ApFloat::getIEEEdouble(), true, false, 0));
   ASSERT_EQ(0xfff4000000000000ULL, nanbits(ApFloat::getIEEEdouble(), true, true, 0));
   ASSERT_EQ(0x7ff000000000ae72ULL, nanbits(ApFloat::getIEEEdouble(), true, false, 0xae72));
   ASSERT_EQ(0x7ff7ffffffffae72ULL, nanbits(ApFloat::getIEEEdouble(), true, false, 0xffffffffffffae72ULL));
}

#ifdef GTEST_HAS_DEATH_TEST
#ifdef POLAR_DEBUG_MODE
TEST(ApFloatTest, testSemanticsDeath)
{
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEsingle(), 0.0f).convertToDouble(), "Float semantics are not IEEEdouble");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), 0.0 ).convertToFloat(),  "Float semantics are not IEEEsingle");
}

TEST(ApFloatTest, testStringDecimalDeath)
{
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  ""), "Invalid string length");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+"), "String has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-"), "String has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), StringRef("\0", 1)), "Invalid character in significand");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), StringRef("1\0", 2)), "Invalid character in significand");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), StringRef("1" "\0" "2", 3)), "Invalid character in significand");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), StringRef("1" "\0" "2e1", 5)), "Invalid character in significand");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), StringRef("1e\0", 3)), "Invalid character in exponent");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), StringRef("1e1\0", 4)), "Invalid character in exponent");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), StringRef("1e1" "\0" "2", 5)), "Invalid character in exponent");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "1.0f"), "Invalid character in significand");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), ".."), "String contains multiple dots");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "..0"), "String contains multiple dots");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "1.0.0"), "String contains multiple dots");
}

TEST(ApFloatTest, testStringDecimalSignificandDeath)
{
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "."), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+."), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-."), "Significand has no digits");


   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "e"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+e"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-e"), "Significand has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "e1"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+e1"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-e1"), "Significand has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  ".e1"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+.e1"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-.e1"), "Significand has no digits");


   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  ".e"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+.e"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-.e"), "Significand has no digits");
}

TEST(ApFloatTest, StringDecimalExponentDeath)
{
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),   "1e"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "+1e"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "-1e"), "Exponent has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),   "1.e"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "+1.e"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "-1.e"), "Exponent has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),   ".1e"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "+.1e"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "-.1e"), "Exponent has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),   "1.1e"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "+1.1e"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "-1.1e"), "Exponent has no digits");


   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "1e+"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "1e-"), "Exponent has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  ".1e"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), ".1e+"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), ".1e-"), "Exponent has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "1.0e"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "1.0e+"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "1.0e-"), "Exponent has no digits");
}

TEST(ApFloatTest, testStringHexadecimalDeath)
{
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x"), "Invalid string");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x"), "Invalid string");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x"), "Invalid string");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x0"), "Hex strings require an exponent");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x0"), "Hex strings require an exponent");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x0"), "Hex strings require an exponent");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x0."), "Hex strings require an exponent");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x0."), "Hex strings require an exponent");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x0."), "Hex strings require an exponent");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x.0"), "Hex strings require an exponent");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x.0"), "Hex strings require an exponent");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x.0"), "Hex strings require an exponent");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x0.0"), "Hex strings require an exponent");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x0.0"), "Hex strings require an exponent");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x0.0"), "Hex strings require an exponent");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), StringRef("0x\0", 3)), "Invalid character in significand");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), StringRef("0x1\0", 4)), "Invalid character in significand");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), StringRef("0x1" "\0" "2", 5)), "Invalid character in significand");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), StringRef("0x1" "\0" "2p1", 7)), "Invalid character in significand");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), StringRef("0x1p\0", 5)), "Invalid character in exponent");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), StringRef("0x1p1\0", 6)), "Invalid character in exponent");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), StringRef("0x1p1" "\0" "2", 7)), "Invalid character in exponent");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "0x1p0f"), "Invalid character in exponent");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "0x..p1"), "String contains multiple dots");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "0x..0p1"), "String contains multiple dots");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "0x1.0.0p1"), "String contains multiple dots");
}

TEST(ApFloatTest, testStringHexadecimalSignificandDeath)
{
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x."), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x."), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x."), "Significand has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0xp"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0xp"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0xp"), "Significand has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0xp+"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0xp+"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0xp+"), "Significand has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0xp-"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0xp-"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0xp-"), "Significand has no digits");


   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x.p"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x.p"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x.p"), "Significand has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x.p+"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x.p+"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x.p+"), "Significand has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x.p-"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x.p-"), "Significand has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x.p-"), "Significand has no digits");
}

TEST(ApFloatTest, testStringHexadecimalExponentDeath)
{
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x1p"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x1p"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x1p"), "Exponent has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x1p+"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x1p+"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x1p+"), "Exponent has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x1p-"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x1p-"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x1p-"), "Exponent has no digits");


   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x1.p"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x1.p"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x1.p"), "Exponent has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x1.p+"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x1.p+"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x1.p+"), "Exponent has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x1.p-"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x1.p-"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x1.p-"), "Exponent has no digits");


   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x.1p"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x.1p"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x.1p"), "Exponent has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x.1p+"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x.1p+"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x.1p+"), "Exponent has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x.1p-"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x.1p-"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x.1p-"), "Exponent has no digits");


   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x1.1p"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x1.1p"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x1.1p"), "Exponent has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x1.1p+"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x1.1p+"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x1.1p+"), "Exponent has no digits");

   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(),  "0x1.1p-"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "+0x1.1p-"), "Exponent has no digits");
   EXPECT_DEATH(ApFloat(ApFloat::getIEEEdouble(), "-0x1.1p-"), "Exponent has no digits");
}
#endif
#endif

TEST(ApFloatTest, testExactInverse)
{
   ApFloat inv(0.0f);

   // Trivial operation.
   EXPECT_TRUE(ApFloat(2.0).getExactInverse(&inv));
   EXPECT_TRUE(inv.bitwiseIsEqual(ApFloat(0.5)));
   EXPECT_TRUE(ApFloat(2.0f).getExactInverse(&inv));
   EXPECT_TRUE(inv.bitwiseIsEqual(ApFloat(0.5f)));
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEquad(), "2.0").getExactInverse(&inv));
   EXPECT_TRUE(inv.bitwiseIsEqual(ApFloat(ApFloat::getIEEEquad(), "0.5")));
   EXPECT_TRUE(ApFloat(ApFloat::getPPCDoubleDouble(), "2.0").getExactInverse(&inv));
   EXPECT_TRUE(inv.bitwiseIsEqual(ApFloat(ApFloat::getPPCDoubleDouble(), "0.5")));
   EXPECT_TRUE(ApFloat(ApFloat::getX87DoubleExtended(), "2.0").getExactInverse(&inv));
   EXPECT_TRUE(inv.bitwiseIsEqual(ApFloat(ApFloat::getX87DoubleExtended(), "0.5")));

   // FLT_MIN
   EXPECT_TRUE(ApFloat(1.17549435e-38f).getExactInverse(&inv));
   EXPECT_TRUE(inv.bitwiseIsEqual(ApFloat(8.5070592e+37f)));

   // Large float, inverse is a denormal.
   EXPECT_FALSE(ApFloat(1.7014118e38f).getExactInverse(nullptr));
   // Zero
   EXPECT_FALSE(ApFloat(0.0).getExactInverse(nullptr));
   // Denormalized float
   EXPECT_FALSE(ApFloat(1.40129846e-45f).getExactInverse(nullptr));
}

TEST(ApFloatTest, testRoundToIntegral)
{
   ApFloat T(-0.5), S(3.14), R(ApFloat::getLargest(ApFloat::getIEEEdouble())), P(0.0);

   P = T;
   P.roundToIntegral(ApFloat::RoundingMode::rmTowardZero);
   EXPECT_EQ(-0.0, P.convertToDouble());
   P = T;
   P.roundToIntegral(ApFloat::RoundingMode::rmTowardNegative);
   EXPECT_EQ(-1.0, P.convertToDouble());
   P = T;
   P.roundToIntegral(ApFloat::RoundingMode::rmTowardPositive);
   EXPECT_EQ(-0.0, P.convertToDouble());
   P = T;
   P.roundToIntegral(ApFloat::RoundingMode::rmNearestTiesToEven);
   EXPECT_EQ(-0.0, P.convertToDouble());

   P = S;
   P.roundToIntegral(ApFloat::RoundingMode::rmTowardZero);
   EXPECT_EQ(3.0, P.convertToDouble());
   P = S;
   P.roundToIntegral(ApFloat::RoundingMode::rmTowardNegative);
   EXPECT_EQ(3.0, P.convertToDouble());
   P = S;
   P.roundToIntegral(ApFloat::RoundingMode::rmTowardPositive);
   EXPECT_EQ(4.0, P.convertToDouble());
   P = S;
   P.roundToIntegral(ApFloat::RoundingMode::rmNearestTiesToEven);
   EXPECT_EQ(3.0, P.convertToDouble());

   P = R;
   P.roundToIntegral(ApFloat::RoundingMode::rmTowardZero);
   EXPECT_EQ(R.convertToDouble(), P.convertToDouble());
   P = R;
   P.roundToIntegral(ApFloat::RoundingMode::rmTowardNegative);
   EXPECT_EQ(R.convertToDouble(), P.convertToDouble());
   P = R;
   P.roundToIntegral(ApFloat::RoundingMode::rmTowardPositive);
   EXPECT_EQ(R.convertToDouble(), P.convertToDouble());
   P = R;
   P.roundToIntegral(ApFloat::RoundingMode::rmNearestTiesToEven);
   EXPECT_EQ(R.convertToDouble(), P.convertToDouble());

   P = ApFloat::getZero(ApFloat::getIEEEdouble());
   P.roundToIntegral(ApFloat::RoundingMode::rmTowardZero);
   EXPECT_EQ(0.0, P.convertToDouble());
   P = ApFloat::getZero(ApFloat::getIEEEdouble(), true);
   P.roundToIntegral(ApFloat::RoundingMode::rmTowardZero);
   EXPECT_EQ(-0.0, P.convertToDouble());
   P = ApFloat::getNaN(ApFloat::getIEEEdouble());
   P.roundToIntegral(ApFloat::RoundingMode::rmTowardZero);
   EXPECT_TRUE(std::isnan(P.convertToDouble()));
   P = ApFloat::getInf(ApFloat::getIEEEdouble());
   P.roundToIntegral(ApFloat::RoundingMode::rmTowardZero);
   EXPECT_TRUE(std::isinf(P.convertToDouble()) && P.convertToDouble() > 0.0);
   P = ApFloat::getInf(ApFloat::getIEEEdouble(), true);
   P.roundToIntegral(ApFloat::RoundingMode::rmTowardZero);
   EXPECT_TRUE(std::isinf(P.convertToDouble()) && P.convertToDouble() < 0.0);
}

TEST(ApFloatTest, getIsInteger)
{
   ApFloat T(-0.0);
   EXPECT_TRUE(T.isInteger());
   T = ApFloat(3.14159);
   EXPECT_FALSE(T.isInteger());
   T = ApFloat::getNaN(ApFloat::getIEEEdouble());
   EXPECT_FALSE(T.isInteger());
   T = ApFloat::getInf(ApFloat::getIEEEdouble());
   EXPECT_FALSE(T.isInteger());
   T = ApFloat::getInf(ApFloat::getIEEEdouble(), true);
   EXPECT_FALSE(T.isInteger());
   T = ApFloat::getLargest(ApFloat::getIEEEdouble());
   EXPECT_TRUE(T.isInteger());
}

TEST(DoubleAPFloatTest, testIsInteger)
{
   ApFloat F1(-0.0);
   ApFloat F2(-0.0);
   internal::DoubleApFloat T(ApFloat::getPPCDoubleDouble(), std::move(F1),
                             std::move(F2));
   EXPECT_TRUE(T.isInteger());
   ApFloat F3(3.14159);
   ApFloat F4(-0.0);
   internal::DoubleApFloat T2(ApFloat::getPPCDoubleDouble(), std::move(F3),
                              std::move(F4));
   EXPECT_FALSE(T2.isInteger());
   ApFloat F5(-0.0);
   ApFloat F6(3.14159);
   internal::DoubleApFloat T3(ApFloat::getPPCDoubleDouble(), std::move(F5),
                              std::move(F6));
   EXPECT_FALSE(T3.isInteger());
}

TEST(ApFloatTest, testGetLargest)
{
   EXPECT_EQ(3.402823466e+38f, ApFloat::getLargest(ApFloat::getIEEEsingle()).convertToFloat());
   EXPECT_EQ(1.7976931348623158e+308, ApFloat::getLargest(ApFloat::getIEEEdouble()).convertToDouble());
}

TEST(ApFloatTest, testGetSmallest)
{
   ApFloat test = ApFloat::getSmallest(ApFloat::getIEEEsingle(), false);
   ApFloat expected = ApFloat(ApFloat::getIEEEsingle(), "0x0.000002p-126");
   EXPECT_FALSE(test.isNegative());
   EXPECT_TRUE(test.isFiniteNonZero());
   EXPECT_TRUE(test.isDenormal());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   test = ApFloat::getSmallest(ApFloat::getIEEEsingle(), true);
   expected = ApFloat(ApFloat::getIEEEsingle(), "-0x0.000002p-126");
   EXPECT_TRUE(test.isNegative());
   EXPECT_TRUE(test.isFiniteNonZero());
   EXPECT_TRUE(test.isDenormal());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   test = ApFloat::getSmallest(ApFloat::getIEEEquad(), false);
   expected = ApFloat(ApFloat::getIEEEquad(), "0x0.0000000000000000000000000001p-16382");
   EXPECT_FALSE(test.isNegative());
   EXPECT_TRUE(test.isFiniteNonZero());
   EXPECT_TRUE(test.isDenormal());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   test = ApFloat::getSmallest(ApFloat::getIEEEquad(), true);
   expected = ApFloat(ApFloat::getIEEEquad(), "-0x0.0000000000000000000000000001p-16382");
   EXPECT_TRUE(test.isNegative());
   EXPECT_TRUE(test.isFiniteNonZero());
   EXPECT_TRUE(test.isDenormal());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));
}

TEST(ApFloatTest, testGetSmallestNormalized)
{
   ApFloat test = ApFloat::getSmallestNormalized(ApFloat::getIEEEsingle(), false);
   ApFloat expected = ApFloat(ApFloat::getIEEEsingle(), "0x1p-126");
   EXPECT_FALSE(test.isNegative());
   EXPECT_TRUE(test.isFiniteNonZero());
   EXPECT_FALSE(test.isDenormal());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   test = ApFloat::getSmallestNormalized(ApFloat::getIEEEsingle(), true);
   expected = ApFloat(ApFloat::getIEEEsingle(), "-0x1p-126");
   EXPECT_TRUE(test.isNegative());
   EXPECT_TRUE(test.isFiniteNonZero());
   EXPECT_FALSE(test.isDenormal());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   test = ApFloat::getSmallestNormalized(ApFloat::getIEEEquad(), false);
   expected = ApFloat(ApFloat::getIEEEquad(), "0x1p-16382");
   EXPECT_FALSE(test.isNegative());
   EXPECT_TRUE(test.isFiniteNonZero());
   EXPECT_FALSE(test.isDenormal());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));

   test = ApFloat::getSmallestNormalized(ApFloat::getIEEEquad(), true);
   expected = ApFloat(ApFloat::getIEEEquad(), "-0x1p-16382");
   EXPECT_TRUE(test.isNegative());
   EXPECT_TRUE(test.isFiniteNonZero());
   EXPECT_FALSE(test.isDenormal());
   EXPECT_TRUE(test.bitwiseIsEqual(expected));
}

TEST(ApFloatTest, testGetZero)
{
   struct {
      const FltSemantics *semantics;
      const bool sign;
      const unsigned long long bitPattern[2];
      const unsigned bitPatternLength;
   } const GetZeroTest[] = {
   { &ApFloat::getIEEEhalf(), false, {0, 0}, 1},
   { &ApFloat::getIEEEhalf(), true, {0x8000ULL, 0}, 1},
   { &ApFloat::getIEEEsingle(), false, {0, 0}, 1},
   { &ApFloat::getIEEEsingle(), true, {0x80000000ULL, 0}, 1},
   { &ApFloat::getIEEEdouble(), false, {0, 0}, 1},
   { &ApFloat::getIEEEdouble(), true, {0x8000000000000000ULL, 0}, 1},
   { &ApFloat::getIEEEquad(), false, {0, 0}, 2},
   { &ApFloat::getIEEEquad(), true, {0, 0x8000000000000000ULL}, 2},
   { &ApFloat::getPPCDoubleDouble(), false, {0, 0}, 2},
   { &ApFloat::getPPCDoubleDouble(), true, {0x8000000000000000ULL, 0}, 2},
   { &ApFloat::getX87DoubleExtended(), false, {0, 0}, 2},
   { &ApFloat::getX87DoubleExtended(), true, {0, 0x8000ULL}, 2},
};
   const unsigned NumGetZeroTests = 12;
   for (unsigned i = 0; i < NumGetZeroTests; ++i) {
      ApFloat test = ApFloat::getZero(*GetZeroTest[i].semantics,
                                      GetZeroTest[i].sign);
      const char *pattern = GetZeroTest[i].sign? "-0x0p+0" : "0x0p+0";
      ApFloat expected = ApFloat(*GetZeroTest[i].semantics,
                                 pattern);
      EXPECT_TRUE(test.isZero());
      EXPECT_TRUE(GetZeroTest[i].sign? test.isNegative() : !test.isNegative());
      EXPECT_TRUE(test.bitwiseIsEqual(expected));
      for (unsigned j = 0, je = GetZeroTest[i].bitPatternLength; j < je; ++j) {
         EXPECT_EQ(GetZeroTest[i].bitPattern[j],
                   test.bitcastToApInt().getRawData()[j]);
      }
   }
}

TEST(ApFloatTest, testCopySign)
{
   EXPECT_TRUE(ApFloat(-42.0).bitwiseIsEqual(
                  ApFloat::copySign(ApFloat(42.0), ApFloat(-1.0))));
   EXPECT_TRUE(ApFloat(42.0).bitwiseIsEqual(
                  ApFloat::copySign(ApFloat(-42.0), ApFloat(1.0))));
   EXPECT_TRUE(ApFloat(-42.0).bitwiseIsEqual(
                  ApFloat::copySign(ApFloat(-42.0), ApFloat(-1.0))));
   EXPECT_TRUE(ApFloat(42.0).bitwiseIsEqual(
                  ApFloat::copySign(ApFloat(42.0), ApFloat(1.0))));
}

TEST(ApFloatTest, testConvert)
{
   bool losesInfo;
   ApFloat test(ApFloat::getIEEEdouble(), "1.0");
   test.convert(ApFloat::getIEEEsingle(), ApFloat::RoundingMode::rmNearestTiesToEven, &losesInfo);
   EXPECT_EQ(1.0f, test.convertToFloat());
   EXPECT_FALSE(losesInfo);

   test = ApFloat(ApFloat::getX87DoubleExtended(), "0x1p-53");
   test.add(ApFloat(ApFloat::getX87DoubleExtended(), "1.0"), ApFloat::RoundingMode::rmNearestTiesToEven);
   test.convert(ApFloat::getIEEEdouble(), ApFloat::RoundingMode::rmNearestTiesToEven, &losesInfo);
   EXPECT_EQ(1.0, test.convertToDouble());
   EXPECT_TRUE(losesInfo);

   test = ApFloat(ApFloat::getIEEEquad(), "0x1p-53");
   test.add(ApFloat(ApFloat::getIEEEquad(), "1.0"), ApFloat::RoundingMode::rmNearestTiesToEven);
   test.convert(ApFloat::getIEEEdouble(), ApFloat::RoundingMode::rmNearestTiesToEven, &losesInfo);
   EXPECT_EQ(1.0, test.convertToDouble());
   EXPECT_TRUE(losesInfo);

   test = ApFloat(ApFloat::getX87DoubleExtended(), "0xf.fffffffp+28");
   test.convert(ApFloat::getIEEEdouble(), ApFloat::RoundingMode::rmNearestTiesToEven, &losesInfo);
   EXPECT_EQ(4294967295.0, test.convertToDouble());
   EXPECT_FALSE(losesInfo);

   test = ApFloat::getSNaN(ApFloat::getIEEEsingle());
   ApFloat X87SNaN = ApFloat::getSNaN(ApFloat::getX87DoubleExtended());
   test.convert(ApFloat::getX87DoubleExtended(), ApFloat::RoundingMode::rmNearestTiesToEven,
                &losesInfo);
   EXPECT_TRUE(test.bitwiseIsEqual(X87SNaN));
   EXPECT_FALSE(losesInfo);

   test = ApFloat::getQNaN(ApFloat::getIEEEsingle());
   ApFloat X87QNaN = ApFloat::getQNaN(ApFloat::getX87DoubleExtended());
   test.convert(ApFloat::getX87DoubleExtended(), ApFloat::RoundingMode::rmNearestTiesToEven,
                &losesInfo);
   EXPECT_TRUE(test.bitwiseIsEqual(X87QNaN));
   EXPECT_FALSE(losesInfo);

   test = ApFloat::getSNaN(ApFloat::getX87DoubleExtended());
   test.convert(ApFloat::getX87DoubleExtended(), ApFloat::RoundingMode::rmNearestTiesToEven,
                &losesInfo);
   EXPECT_TRUE(test.bitwiseIsEqual(X87SNaN));
   EXPECT_FALSE(losesInfo);

   test = ApFloat::getQNaN(ApFloat::getX87DoubleExtended());
   test.convert(ApFloat::getX87DoubleExtended(), ApFloat::RoundingMode::rmNearestTiesToEven,
                &losesInfo);
   EXPECT_TRUE(test.bitwiseIsEqual(X87QNaN));
   EXPECT_FALSE(losesInfo);
}

TEST(ApFloatTest, testGetPPCDoubleDouble)
{
   ApFloat test(ApFloat::getPPCDoubleDouble(), "1.0");
   EXPECT_EQ(0x3ff0000000000000ull, test.bitcastToApInt().getRawData()[0]);
   EXPECT_EQ(0x0000000000000000ull, test.bitcastToApInt().getRawData()[1]);

   // LDBL_MAX
   test = ApFloat(ApFloat::getPPCDoubleDouble(), "1.79769313486231580793728971405301e+308");
   EXPECT_EQ(0x7fefffffffffffffull, test.bitcastToApInt().getRawData()[0]);
   EXPECT_EQ(0x7c8ffffffffffffeull, test.bitcastToApInt().getRawData()[1]);

   // LDBL_MIN
   test = ApFloat(ApFloat::getPPCDoubleDouble(), "2.00416836000897277799610805135016e-292");
   EXPECT_EQ(0x0360000000000000ull, test.bitcastToApInt().getRawData()[0]);
   EXPECT_EQ(0x0000000000000000ull, test.bitcastToApInt().getRawData()[1]);

   // PR30869
   {
      auto Result = ApFloat(ApFloat::getPPCDoubleDouble(), "1.0") +
            ApFloat(ApFloat::getPPCDoubleDouble(), "1.0");
      EXPECT_EQ(&ApFloat::getPPCDoubleDouble(), &Result.getSemantics());

      Result = ApFloat(ApFloat::getPPCDoubleDouble(), "1.0") -
            ApFloat(ApFloat::getPPCDoubleDouble(), "1.0");
      EXPECT_EQ(&ApFloat::getPPCDoubleDouble(), &Result.getSemantics());

      Result = ApFloat(ApFloat::getPPCDoubleDouble(), "1.0") *
            ApFloat(ApFloat::getPPCDoubleDouble(), "1.0");
      EXPECT_EQ(&ApFloat::getPPCDoubleDouble(), &Result.getSemantics());

      Result = ApFloat(ApFloat::getPPCDoubleDouble(), "1.0") /
            ApFloat(ApFloat::getPPCDoubleDouble(), "1.0");
      EXPECT_EQ(&ApFloat::getPPCDoubleDouble(), &Result.getSemantics());

      int Exp;
      Result = frexp(ApFloat(ApFloat::getPPCDoubleDouble(), "1.0"), Exp,
                     ApFloat::RoundingMode::rmNearestTiesToEven);
      EXPECT_EQ(&ApFloat::getPPCDoubleDouble(), &Result.getSemantics());

      Result = scalbn(ApFloat(ApFloat::getPPCDoubleDouble(), "1.0"), 1,
                      ApFloat::RoundingMode::rmNearestTiesToEven);
      EXPECT_EQ(&ApFloat::getPPCDoubleDouble(), &Result.getSemantics());
   }
}

TEST(ApFloatTest, testIsNegative)
{
   ApFloat t(ApFloat::getIEEEsingle(), "0x1p+0");
   EXPECT_FALSE(t.isNegative());
   t = ApFloat(ApFloat::getIEEEsingle(), "-0x1p+0");
   EXPECT_TRUE(t.isNegative());

   EXPECT_FALSE(ApFloat::getInf(ApFloat::getIEEEsingle(), false).isNegative());
   EXPECT_TRUE(ApFloat::getInf(ApFloat::getIEEEsingle(), true).isNegative());

   EXPECT_FALSE(ApFloat::getZero(ApFloat::getIEEEsingle(), false).isNegative());
   EXPECT_TRUE(ApFloat::getZero(ApFloat::getIEEEsingle(), true).isNegative());

   EXPECT_FALSE(ApFloat::getNaN(ApFloat::getIEEEsingle(), false).isNegative());
   EXPECT_TRUE(ApFloat::getNaN(ApFloat::getIEEEsingle(), true).isNegative());

   EXPECT_FALSE(ApFloat::getSNaN(ApFloat::getIEEEsingle(), false).isNegative());
   EXPECT_TRUE(ApFloat::getSNaN(ApFloat::getIEEEsingle(), true).isNegative());
}

TEST(ApFloatTest, testIsNormal)
{
   ApFloat t(ApFloat::getIEEEsingle(), "0x1p+0");
   EXPECT_TRUE(t.isNormal());

   EXPECT_FALSE(ApFloat::getInf(ApFloat::getIEEEsingle(), false).isNormal());
   EXPECT_FALSE(ApFloat::getZero(ApFloat::getIEEEsingle(), false).isNormal());
   EXPECT_FALSE(ApFloat::getNaN(ApFloat::getIEEEsingle(), false).isNormal());
   EXPECT_FALSE(ApFloat::getSNaN(ApFloat::getIEEEsingle(), false).isNormal());
   EXPECT_FALSE(ApFloat(ApFloat::getIEEEsingle(), "0x1p-149").isNormal());
}

TEST(ApFloatTest, testIsFinite)
{
   ApFloat t(ApFloat::getIEEEsingle(), "0x1p+0");
   EXPECT_TRUE(t.isFinite());
   EXPECT_FALSE(ApFloat::getInf(ApFloat::getIEEEsingle(), false).isFinite());
   EXPECT_TRUE(ApFloat::getZero(ApFloat::getIEEEsingle(), false).isFinite());
   EXPECT_FALSE(ApFloat::getNaN(ApFloat::getIEEEsingle(), false).isFinite());
   EXPECT_FALSE(ApFloat::getSNaN(ApFloat::getIEEEsingle(), false).isFinite());
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEsingle(), "0x1p-149").isFinite());
}

TEST(ApFloatTest, testIsInfinity)
{
   ApFloat t(ApFloat::getIEEEsingle(), "0x1p+0");
   EXPECT_FALSE(t.isInfinity());
   EXPECT_TRUE(ApFloat::getInf(ApFloat::getIEEEsingle(), false).isInfinity());
   EXPECT_FALSE(ApFloat::getZero(ApFloat::getIEEEsingle(), false).isInfinity());
   EXPECT_FALSE(ApFloat::getNaN(ApFloat::getIEEEsingle(), false).isInfinity());
   EXPECT_FALSE(ApFloat::getSNaN(ApFloat::getIEEEsingle(), false).isInfinity());
   EXPECT_FALSE(ApFloat(ApFloat::getIEEEsingle(), "0x1p-149").isInfinity());
}

TEST(ApFloatTest, testIsNaN)
{
   ApFloat t(ApFloat::getIEEEsingle(), "0x1p+0");
   EXPECT_FALSE(t.isNaN());
   EXPECT_FALSE(ApFloat::getInf(ApFloat::getIEEEsingle(), false).isNaN());
   EXPECT_FALSE(ApFloat::getZero(ApFloat::getIEEEsingle(), false).isNaN());
   EXPECT_TRUE(ApFloat::getNaN(ApFloat::getIEEEsingle(), false).isNaN());
   EXPECT_TRUE(ApFloat::getSNaN(ApFloat::getIEEEsingle(), false).isNaN());
   EXPECT_FALSE(ApFloat(ApFloat::getIEEEsingle(), "0x1p-149").isNaN());
}

TEST(ApFloatTest, testIsFiniteNonZero)
{
   // Test positive/negative normal value.
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEsingle(), "0x1p+0").isFiniteNonZero());
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEsingle(), "-0x1p+0").isFiniteNonZero());

   // Test positive/negative denormal value.
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEsingle(), "0x1p-149").isFiniteNonZero());
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEsingle(), "-0x1p-149").isFiniteNonZero());

   // Test +/- Infinity.
   EXPECT_FALSE(ApFloat::getInf(ApFloat::getIEEEsingle(), false).isFiniteNonZero());
   EXPECT_FALSE(ApFloat::getInf(ApFloat::getIEEEsingle(), true).isFiniteNonZero());

   // Test +/- Zero.
   EXPECT_FALSE(ApFloat::getZero(ApFloat::getIEEEsingle(), false).isFiniteNonZero());
   EXPECT_FALSE(ApFloat::getZero(ApFloat::getIEEEsingle(), true).isFiniteNonZero());

   // Test +/- qNaN. +/- dont mean anything with qNaN but paranoia can't hurt in
   // this instance.
   EXPECT_FALSE(ApFloat::getNaN(ApFloat::getIEEEsingle(), false).isFiniteNonZero());
   EXPECT_FALSE(ApFloat::getNaN(ApFloat::getIEEEsingle(), true).isFiniteNonZero());

   // Test +/- sNaN. +/- dont mean anything with sNaN but paranoia can't hurt in
   // this instance.
   EXPECT_FALSE(ApFloat::getSNaN(ApFloat::getIEEEsingle(), false).isFiniteNonZero());
   EXPECT_FALSE(ApFloat::getSNaN(ApFloat::getIEEEsingle(), true).isFiniteNonZero());
}

TEST(ApFloatTest, testAdd)
{
   // Test Special Cases against each other and normal values.

   // TODOS/NOTES:
   // 1. Since we perform only default exception handling all operations with
   // signaling NaNs should have a result that is a quiet NaN. Currently they
   // return sNaN.

   ApFloat PInf = ApFloat::getInf(ApFloat::getIEEEsingle(), false);
   ApFloat MInf = ApFloat::getInf(ApFloat::getIEEEsingle(), true);
   ApFloat PZero = ApFloat::getZero(ApFloat::getIEEEsingle(), false);
   ApFloat MZero = ApFloat::getZero(ApFloat::getIEEEsingle(), true);
   ApFloat QNaN = ApFloat::getNaN(ApFloat::getIEEEsingle(), false);
   ApFloat SNaN = ApFloat::getSNaN(ApFloat::getIEEEsingle(), false);
   ApFloat PNormalValue = ApFloat(ApFloat::getIEEEsingle(), "0x1p+0");
   ApFloat MNormalValue = ApFloat(ApFloat::getIEEEsingle(), "-0x1p+0");
   ApFloat PLargestValue = ApFloat::getLargest(ApFloat::getIEEEsingle(), false);
   ApFloat MLargestValue = ApFloat::getLargest(ApFloat::getIEEEsingle(), true);
   ApFloat PSmallestValue = ApFloat::getSmallest(ApFloat::getIEEEsingle(), false);
   ApFloat MSmallestValue = ApFloat::getSmallest(ApFloat::getIEEEsingle(), true);
   ApFloat PSmallestNormalized =
         ApFloat::getSmallestNormalized(ApFloat::getIEEEsingle(), false);
   ApFloat MSmallestNormalized =
         ApFloat::getSmallestNormalized(ApFloat::getIEEEsingle(), true);

   const int OverflowStatus = ApFloat::opOverflow | ApFloat::opInexact;

   const unsigned NumTests = 169;
   struct {
      ApFloat x;
      ApFloat y;
      const char *result;
      int status;
      ApFloat::FltCategory category;
   } SpecialCaseTests[NumTests] = {
   { PInf, PInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { PInf, PZero, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MZero, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PInf, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PInf, PNormalValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MNormalValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, PLargestValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MLargestValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, PSmallestValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MSmallestValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, PSmallestNormalized, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MSmallestNormalized, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { MInf, MInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PZero, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MZero, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MInf, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MInf, PNormalValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MNormalValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PLargestValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MLargestValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PSmallestValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MSmallestValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PSmallestNormalized, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MSmallestNormalized, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PZero, PInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PZero, MInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PZero, PZero, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, MZero, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PZero, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PZero, PNormalValue, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PZero, MNormalValue, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PZero, PLargestValue, "0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PZero, MLargestValue, "-0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PZero, PSmallestValue, "0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PZero, MSmallestValue, "-0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PZero, PSmallestNormalized, "0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PZero, MSmallestNormalized, "-0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MZero, PInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MZero, MInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MZero, PZero, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, MZero, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MZero, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MZero, PNormalValue, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MZero, MNormalValue, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MZero, PLargestValue, "0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MZero, MLargestValue, "-0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MZero, PSmallestValue, "0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MZero, MSmallestValue, "-0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MZero, PSmallestNormalized, "0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MZero, MSmallestNormalized, "-0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { QNaN, PInf, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MInf, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, PZero, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MZero, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { QNaN, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { QNaN, PNormalValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MNormalValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, PLargestValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MLargestValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, PSmallestValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MSmallestValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, PSmallestNormalized, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MSmallestNormalized, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { SNaN, PInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PZero, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MZero, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, QNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PNormalValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MNormalValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PLargestValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MLargestValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PSmallestValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MSmallestValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PSmallestNormalized, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MSmallestNormalized, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PNormalValue, PInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PNormalValue, MInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PNormalValue, PZero, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PNormalValue, MZero, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PNormalValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PNormalValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PNormalValue, PNormalValue, "0x1p+1", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PNormalValue, MNormalValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PNormalValue, PLargestValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PNormalValue, MLargestValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PNormalValue, PSmallestValue, "0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PNormalValue, MSmallestValue, "0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PNormalValue, PSmallestNormalized, "0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PNormalValue, MSmallestNormalized, "0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MNormalValue, PInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MNormalValue, MInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MNormalValue, PZero, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, MZero, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MNormalValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MNormalValue, PNormalValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MNormalValue, MNormalValue, "-0x1p+1", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, PLargestValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MNormalValue, MLargestValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MNormalValue, PSmallestValue, "-0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MNormalValue, MSmallestValue, "-0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MNormalValue, PSmallestNormalized, "-0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MNormalValue, MSmallestNormalized, "-0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PLargestValue, PInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PLargestValue, MInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PLargestValue, PZero, "0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PLargestValue, MZero, "0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PLargestValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PLargestValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PLargestValue, PNormalValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PLargestValue, MNormalValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PLargestValue, PLargestValue, "inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { PLargestValue, MLargestValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PLargestValue, PSmallestValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PLargestValue, MSmallestValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PLargestValue, PSmallestNormalized, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PLargestValue, MSmallestNormalized, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MLargestValue, PInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MLargestValue, MInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MLargestValue, PZero, "-0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MLargestValue, MZero, "-0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MLargestValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MLargestValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MLargestValue, PNormalValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MLargestValue, MNormalValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MLargestValue, PLargestValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MLargestValue, MLargestValue, "-inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { MLargestValue, PSmallestValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MLargestValue, MSmallestValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MLargestValue, PSmallestNormalized, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MLargestValue, MSmallestNormalized, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, PInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PSmallestValue, MInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PSmallestValue, PZero, "0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, MZero, "0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PSmallestValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PSmallestValue, PNormalValue, "0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, MNormalValue, "-0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, PLargestValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, MLargestValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, PSmallestValue, "0x1p-148", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, MSmallestValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PSmallestValue, PSmallestNormalized, "0x1.000002p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, MSmallestNormalized, "-0x1.fffffcp-127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, PInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MSmallestValue, MInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MSmallestValue, PZero, "-0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, MZero, "-0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MSmallestValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MSmallestValue, PNormalValue, "0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, MNormalValue, "-0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, PLargestValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, MLargestValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, PSmallestValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MSmallestValue, MSmallestValue, "-0x1p-148", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, PSmallestNormalized, "0x1.fffffcp-127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, MSmallestNormalized, "-0x1.000002p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, PInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PSmallestNormalized, MInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PSmallestNormalized, PZero, "0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, MZero, "0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PSmallestNormalized, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PSmallestNormalized, PNormalValue, "0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, MNormalValue, "-0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, PLargestValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, MLargestValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, PSmallestValue, "0x1.000002p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, MSmallestValue, "0x1.fffffcp-127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, PSmallestNormalized, "0x1p-125", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, MSmallestNormalized, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MSmallestNormalized, PInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MSmallestNormalized, MInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MSmallestNormalized, PZero, "-0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, MZero, "-0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MSmallestNormalized, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MSmallestNormalized, PNormalValue, "0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, MNormalValue, "-0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, PLargestValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, MLargestValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, PSmallestValue, "-0x1.fffffcp-127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, MSmallestValue, "-0x1.000002p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, PSmallestNormalized, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MSmallestNormalized, MSmallestNormalized, "-0x1p-125", ApFloat::opOK, ApFloat::FltCategory::fcNormal}
};

   for (size_t i = 0; i < NumTests; ++i) {
      ApFloat x(SpecialCaseTests[i].x);
      ApFloat y(SpecialCaseTests[i].y);
      ApFloat::OpStatus status = x.add(y, ApFloat::RoundingMode::rmNearestTiesToEven);

      ApFloat result(ApFloat::getIEEEsingle(), SpecialCaseTests[i].result);

      EXPECT_TRUE(result.bitwiseIsEqual(x));
      EXPECT_TRUE((int)status == SpecialCaseTests[i].status);
      EXPECT_TRUE(x.getCategory() == SpecialCaseTests[i].category);
   }
}

TEST(ApFloatTest, testSubtract)
{
   // Test Special Cases against each other and normal values.

   // TODOS/NOTES:
   // 1. Since we perform only default exception handling all operations with
   // signaling NaNs should have a result that is a quiet NaN. Currently they
   // return sNaN.

   ApFloat PInf = ApFloat::getInf(ApFloat::getIEEEsingle(), false);
   ApFloat MInf = ApFloat::getInf(ApFloat::getIEEEsingle(), true);
   ApFloat PZero = ApFloat::getZero(ApFloat::getIEEEsingle(), false);
   ApFloat MZero = ApFloat::getZero(ApFloat::getIEEEsingle(), true);
   ApFloat QNaN = ApFloat::getNaN(ApFloat::getIEEEsingle(), false);
   ApFloat SNaN = ApFloat::getSNaN(ApFloat::getIEEEsingle(), false);
   ApFloat PNormalValue = ApFloat(ApFloat::getIEEEsingle(), "0x1p+0");
   ApFloat MNormalValue = ApFloat(ApFloat::getIEEEsingle(), "-0x1p+0");
   ApFloat PLargestValue = ApFloat::getLargest(ApFloat::getIEEEsingle(), false);
   ApFloat MLargestValue = ApFloat::getLargest(ApFloat::getIEEEsingle(), true);
   ApFloat PSmallestValue = ApFloat::getSmallest(ApFloat::getIEEEsingle(), false);
   ApFloat MSmallestValue = ApFloat::getSmallest(ApFloat::getIEEEsingle(), true);
   ApFloat PSmallestNormalized =
         ApFloat::getSmallestNormalized(ApFloat::getIEEEsingle(), false);
   ApFloat MSmallestNormalized =
         ApFloat::getSmallestNormalized(ApFloat::getIEEEsingle(), true);

   const int OverflowStatus = ApFloat::opOverflow | ApFloat::opInexact;

   const unsigned NumTests = 169;
   struct {
      ApFloat x;
      ApFloat y;
      const char *result;
      int status;
      ApFloat::FltCategory category;
   } SpecialCaseTests[NumTests] = {
   { PInf, PInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { PInf, MInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, PZero, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MZero, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, QNaN, "-nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PInf, SNaN, "-nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PInf, PNormalValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MNormalValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, PLargestValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MLargestValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, PSmallestValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MSmallestValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, PSmallestNormalized, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MSmallestNormalized, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { MInf, PZero, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MZero, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, QNaN, "-nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MInf, SNaN, "-nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MInf, PNormalValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MNormalValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PLargestValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MLargestValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PSmallestValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MSmallestValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PSmallestNormalized, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MSmallestNormalized, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PZero, PInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PZero, MInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PZero, PZero, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, MZero, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, QNaN, "-nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PZero, SNaN, "-nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PZero, PNormalValue, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PZero, MNormalValue, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PZero, PLargestValue, "-0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PZero, MLargestValue, "0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PZero, PSmallestValue, "-0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PZero, MSmallestValue, "0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PZero, PSmallestNormalized, "-0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PZero, MSmallestNormalized, "0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MZero, PInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MZero, MInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MZero, PZero, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, MZero, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, QNaN, "-nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MZero, SNaN, "-nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MZero, PNormalValue, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MZero, MNormalValue, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MZero, PLargestValue, "-0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MZero, MLargestValue, "0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MZero, PSmallestValue, "-0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MZero, MSmallestValue, "0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MZero, PSmallestNormalized, "-0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MZero, MSmallestNormalized, "0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { QNaN, PInf, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MInf, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, PZero, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MZero, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { QNaN, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { QNaN, PNormalValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MNormalValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, PLargestValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MLargestValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, PSmallestValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MSmallestValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, PSmallestNormalized, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MSmallestNormalized, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { SNaN, PInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PZero, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MZero, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, QNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PNormalValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MNormalValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PLargestValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MLargestValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PSmallestValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MSmallestValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PSmallestNormalized, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MSmallestNormalized, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PNormalValue, PInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PNormalValue, MInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PNormalValue, PZero, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PNormalValue, MZero, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PNormalValue, QNaN, "-nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PNormalValue, SNaN, "-nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PNormalValue, PNormalValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PNormalValue, MNormalValue, "0x1p+1", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PNormalValue, PLargestValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PNormalValue, MLargestValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PNormalValue, PSmallestValue, "0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PNormalValue, MSmallestValue, "0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PNormalValue, PSmallestNormalized, "0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PNormalValue, MSmallestNormalized, "0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MNormalValue, PInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MNormalValue, MInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MNormalValue, PZero, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, MZero, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, QNaN, "-nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MNormalValue, SNaN, "-nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MNormalValue, PNormalValue, "-0x1p+1", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, MNormalValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MNormalValue, PLargestValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MNormalValue, MLargestValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MNormalValue, PSmallestValue, "-0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MNormalValue, MSmallestValue, "-0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MNormalValue, PSmallestNormalized, "-0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MNormalValue, MSmallestNormalized, "-0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PLargestValue, PInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PLargestValue, MInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PLargestValue, PZero, "0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PLargestValue, MZero, "0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PLargestValue, QNaN, "-nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PLargestValue, SNaN, "-nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PLargestValue, PNormalValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PLargestValue, MNormalValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PLargestValue, PLargestValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PLargestValue, MLargestValue, "inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { PLargestValue, PSmallestValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PLargestValue, MSmallestValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PLargestValue, PSmallestNormalized, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PLargestValue, MSmallestNormalized, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MLargestValue, PInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MLargestValue, MInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MLargestValue, PZero, "-0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MLargestValue, MZero, "-0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MLargestValue, QNaN, "-nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MLargestValue, SNaN, "-nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MLargestValue, PNormalValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MLargestValue, MNormalValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MLargestValue, PLargestValue, "-inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { MLargestValue, MLargestValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MLargestValue, PSmallestValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MLargestValue, MSmallestValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MLargestValue, PSmallestNormalized, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MLargestValue, MSmallestNormalized, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, PInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PSmallestValue, MInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PSmallestValue, PZero, "0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, MZero, "0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, QNaN, "-nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PSmallestValue, SNaN, "-nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PSmallestValue, PNormalValue, "-0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, MNormalValue, "0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, PLargestValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, MLargestValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, PSmallestValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PSmallestValue, MSmallestValue, "0x1p-148", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, PSmallestNormalized, "-0x1.fffffcp-127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, MSmallestNormalized, "0x1.000002p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, PInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MSmallestValue, MInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MSmallestValue, PZero, "-0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, MZero, "-0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, QNaN, "-nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MSmallestValue, SNaN, "-nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MSmallestValue, PNormalValue, "-0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, MNormalValue, "0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, PLargestValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, MLargestValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, PSmallestValue, "-0x1p-148", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, MSmallestValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MSmallestValue, PSmallestNormalized, "-0x1.000002p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, MSmallestNormalized, "0x1.fffffcp-127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, PInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PSmallestNormalized, MInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PSmallestNormalized, PZero, "0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, MZero, "0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, QNaN, "-nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PSmallestNormalized, SNaN, "-nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PSmallestNormalized, PNormalValue, "-0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, MNormalValue, "0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, PLargestValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, MLargestValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, PSmallestValue, "0x1.fffffcp-127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, MSmallestValue, "0x1.000002p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, PSmallestNormalized, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PSmallestNormalized, MSmallestNormalized, "0x1p-125", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, PInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MSmallestNormalized, MInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MSmallestNormalized, PZero, "-0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, MZero, "-0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, QNaN, "-nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MSmallestNormalized, SNaN, "-nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MSmallestNormalized, PNormalValue, "-0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, MNormalValue, "0x1p+0", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, PLargestValue, "-0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, MLargestValue, "0x1.fffffep+127", ApFloat::opInexact, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, PSmallestValue, "-0x1.000002p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, MSmallestValue, "-0x1.fffffcp-127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, PSmallestNormalized, "-0x1p-125", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, MSmallestNormalized, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero}
};

   for (size_t i = 0; i < NumTests; ++i) {
      ApFloat x(SpecialCaseTests[i].x);
      ApFloat y(SpecialCaseTests[i].y);
      ApFloat::OpStatus status = x.subtract(y, ApFloat::RoundingMode::rmNearestTiesToEven);

      ApFloat result(ApFloat::getIEEEsingle(), SpecialCaseTests[i].result);

      EXPECT_TRUE(result.bitwiseIsEqual(x));
      EXPECT_TRUE((int)status == SpecialCaseTests[i].status);
      EXPECT_TRUE(x.getCategory() == SpecialCaseTests[i].category);
   }
}

TEST(ApFloatTest, testMultiply)
{
   // Test Special Cases against each other and normal values.

   // TODOS/NOTES:
   // 1. Since we perform only default exception handling all operations with
   // signaling NaNs should have a result that is a quiet NaN. Currently they
   // return sNaN.

   ApFloat PInf = ApFloat::getInf(ApFloat::getIEEEsingle(), false);
   ApFloat MInf = ApFloat::getInf(ApFloat::getIEEEsingle(), true);
   ApFloat PZero = ApFloat::getZero(ApFloat::getIEEEsingle(), false);
   ApFloat MZero = ApFloat::getZero(ApFloat::getIEEEsingle(), true);
   ApFloat QNaN = ApFloat::getNaN(ApFloat::getIEEEsingle(), false);
   ApFloat SNaN = ApFloat::getSNaN(ApFloat::getIEEEsingle(), false);
   ApFloat PNormalValue = ApFloat(ApFloat::getIEEEsingle(), "0x1p+0");
   ApFloat MNormalValue = ApFloat(ApFloat::getIEEEsingle(), "-0x1p+0");
   ApFloat PLargestValue = ApFloat::getLargest(ApFloat::getIEEEsingle(), false);
   ApFloat MLargestValue = ApFloat::getLargest(ApFloat::getIEEEsingle(), true);
   ApFloat PSmallestValue = ApFloat::getSmallest(ApFloat::getIEEEsingle(), false);
   ApFloat MSmallestValue = ApFloat::getSmallest(ApFloat::getIEEEsingle(), true);
   ApFloat PSmallestNormalized =
         ApFloat::getSmallestNormalized(ApFloat::getIEEEsingle(), false);
   ApFloat MSmallestNormalized =
         ApFloat::getSmallestNormalized(ApFloat::getIEEEsingle(), true);

   const int OverflowStatus = ApFloat::opOverflow | ApFloat::opInexact;
   const int UnderflowStatus = ApFloat::opUnderflow | ApFloat::opInexact;

   const unsigned NumTests = 169;
   struct {
      ApFloat x;
      ApFloat y;
      const char *result;
      int status;
      ApFloat::FltCategory category;
   } SpecialCaseTests[NumTests] = {
   { PInf, PInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, PZero, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { PInf, MZero, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { PInf, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PInf, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PInf, PNormalValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MNormalValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, PLargestValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MLargestValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, PSmallestValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MSmallestValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, PSmallestNormalized, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MSmallestNormalized, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PZero, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { MInf, MZero, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { MInf, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MInf, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MInf, PNormalValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MNormalValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PLargestValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MLargestValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PSmallestValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MSmallestValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PSmallestNormalized, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MSmallestNormalized, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PZero, PInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { PZero, MInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { PZero, PZero, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, MZero, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PZero, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PZero, PNormalValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, MNormalValue, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, PLargestValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, MLargestValue, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, PSmallestValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, MSmallestValue, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, PSmallestNormalized, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, MSmallestNormalized, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, PInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { MZero, MInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { MZero, PZero, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, MZero, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MZero, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MZero, PNormalValue, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, MNormalValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, PLargestValue, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, MLargestValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, PSmallestValue, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, MSmallestValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, PSmallestNormalized, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, MSmallestNormalized, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { QNaN, PInf, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MInf, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, PZero, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MZero, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { QNaN, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { QNaN, PNormalValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MNormalValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, PLargestValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MLargestValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, PSmallestValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MSmallestValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, PSmallestNormalized, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MSmallestNormalized, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { SNaN, PInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PZero, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MZero, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, QNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PNormalValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MNormalValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PLargestValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MLargestValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PSmallestValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MSmallestValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PSmallestNormalized, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MSmallestNormalized, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PNormalValue, PInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PNormalValue, MInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PNormalValue, PZero, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PNormalValue, MZero, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PNormalValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PNormalValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PNormalValue, PNormalValue, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PNormalValue, MNormalValue, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PNormalValue, PLargestValue, "0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PNormalValue, MLargestValue, "-0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PNormalValue, PSmallestValue, "0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PNormalValue, MSmallestValue, "-0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PNormalValue, PSmallestNormalized, "0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PNormalValue, MSmallestNormalized, "-0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, PInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MNormalValue, MInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MNormalValue, PZero, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MNormalValue, MZero, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MNormalValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MNormalValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MNormalValue, PNormalValue, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, MNormalValue, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, PLargestValue, "-0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, MLargestValue, "0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, PSmallestValue, "-0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, MSmallestValue, "0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, PSmallestNormalized, "-0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, MSmallestNormalized, "0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PLargestValue, PInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PLargestValue, MInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PLargestValue, PZero, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PLargestValue, MZero, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PLargestValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PLargestValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PLargestValue, PNormalValue, "0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PLargestValue, MNormalValue, "-0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PLargestValue, PLargestValue, "inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { PLargestValue, MLargestValue, "-inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { PLargestValue, PSmallestValue, "0x1.fffffep-22", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PLargestValue, MSmallestValue, "-0x1.fffffep-22", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PLargestValue, PSmallestNormalized, "0x1.fffffep+1", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PLargestValue, MSmallestNormalized, "-0x1.fffffep+1", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MLargestValue, PInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MLargestValue, MInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MLargestValue, PZero, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MLargestValue, MZero, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MLargestValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MLargestValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MLargestValue, PNormalValue, "-0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MLargestValue, MNormalValue, "0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MLargestValue, PLargestValue, "-inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { MLargestValue, MLargestValue, "inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { MLargestValue, PSmallestValue, "-0x1.fffffep-22", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MLargestValue, MSmallestValue, "0x1.fffffep-22", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MLargestValue, PSmallestNormalized, "-0x1.fffffep+1", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MLargestValue, MSmallestNormalized, "0x1.fffffep+1", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, PInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PSmallestValue, MInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PSmallestValue, PZero, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PSmallestValue, MZero, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PSmallestValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PSmallestValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PSmallestValue, PNormalValue, "0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, MNormalValue, "-0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, PLargestValue, "0x1.fffffep-22", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, MLargestValue, "-0x1.fffffep-22", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, PSmallestValue, "0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { PSmallestValue, MSmallestValue, "-0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { PSmallestValue, PSmallestNormalized, "0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { PSmallestValue, MSmallestNormalized, "-0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { MSmallestValue, PInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MSmallestValue, MInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MSmallestValue, PZero, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MSmallestValue, MZero, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MSmallestValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MSmallestValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MSmallestValue, PNormalValue, "-0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, MNormalValue, "0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, PLargestValue, "-0x1.fffffep-22", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, MLargestValue, "0x1.fffffep-22", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, PSmallestValue, "-0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { MSmallestValue, MSmallestValue, "0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { MSmallestValue, PSmallestNormalized, "-0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { MSmallestValue, MSmallestNormalized, "0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { PSmallestNormalized, PInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PSmallestNormalized, MInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PSmallestNormalized, PZero, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PSmallestNormalized, MZero, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PSmallestNormalized, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PSmallestNormalized, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PSmallestNormalized, PNormalValue, "0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, MNormalValue, "-0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, PLargestValue, "0x1.fffffep+1", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, MLargestValue, "-0x1.fffffep+1", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, PSmallestValue, "0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { PSmallestNormalized, MSmallestValue, "-0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { PSmallestNormalized, PSmallestNormalized, "0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { PSmallestNormalized, MSmallestNormalized, "-0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { MSmallestNormalized, PInf, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MSmallestNormalized, MInf, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MSmallestNormalized, PZero, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MSmallestNormalized, MZero, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MSmallestNormalized, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MSmallestNormalized, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MSmallestNormalized, PNormalValue, "-0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, MNormalValue, "0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, PLargestValue, "-0x1.fffffep+1", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, MLargestValue, "0x1.fffffep+1", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, PSmallestValue, "-0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { MSmallestNormalized, MSmallestValue, "0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { MSmallestNormalized, PSmallestNormalized, "-0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { MSmallestNormalized, MSmallestNormalized, "0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero}
};

   for (size_t i = 0; i < NumTests; ++i) {
      ApFloat x(SpecialCaseTests[i].x);
      ApFloat y(SpecialCaseTests[i].y);
      ApFloat::OpStatus status = x.multiply(y, ApFloat::RoundingMode::rmNearestTiesToEven);

      ApFloat result(ApFloat::getIEEEsingle(), SpecialCaseTests[i].result);

      EXPECT_TRUE(result.bitwiseIsEqual(x));
      EXPECT_TRUE((int)status == SpecialCaseTests[i].status);
      EXPECT_TRUE(x.getCategory() == SpecialCaseTests[i].category);
   }
}

TEST(ApFloatTest, testDivide)
{
   // Test Special Cases against each other and normal values.

   // TODOS/NOTES:
   // 1. Since we perform only default exception handling all operations with
   // signaling NaNs should have a result that is a quiet NaN. Currently they
   // return sNaN.

   ApFloat PInf = ApFloat::getInf(ApFloat::getIEEEsingle(), false);
   ApFloat MInf = ApFloat::getInf(ApFloat::getIEEEsingle(), true);
   ApFloat PZero = ApFloat::getZero(ApFloat::getIEEEsingle(), false);
   ApFloat MZero = ApFloat::getZero(ApFloat::getIEEEsingle(), true);
   ApFloat QNaN = ApFloat::getNaN(ApFloat::getIEEEsingle(), false);
   ApFloat SNaN = ApFloat::getSNaN(ApFloat::getIEEEsingle(), false);
   ApFloat PNormalValue = ApFloat(ApFloat::getIEEEsingle(), "0x1p+0");
   ApFloat MNormalValue = ApFloat(ApFloat::getIEEEsingle(), "-0x1p+0");
   ApFloat PLargestValue = ApFloat::getLargest(ApFloat::getIEEEsingle(), false);
   ApFloat MLargestValue = ApFloat::getLargest(ApFloat::getIEEEsingle(), true);
   ApFloat PSmallestValue = ApFloat::getSmallest(ApFloat::getIEEEsingle(), false);
   ApFloat MSmallestValue = ApFloat::getSmallest(ApFloat::getIEEEsingle(), true);
   ApFloat PSmallestNormalized =
         ApFloat::getSmallestNormalized(ApFloat::getIEEEsingle(), false);
   ApFloat MSmallestNormalized =
         ApFloat::getSmallestNormalized(ApFloat::getIEEEsingle(), true);

   const int OverflowStatus = ApFloat::opOverflow | ApFloat::opInexact;
   const int UnderflowStatus = ApFloat::opUnderflow | ApFloat::opInexact;

   const unsigned NumTests = 169;
   struct {
      ApFloat x;
      ApFloat y;
      const char *result;
      int status;
      ApFloat::FltCategory category;
   } SpecialCaseTests[NumTests] = {
   { PInf, PInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { PInf, MInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { PInf, PZero, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MZero, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PInf, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PInf, PNormalValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MNormalValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, PLargestValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MLargestValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, PSmallestValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MSmallestValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, PSmallestNormalized, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PInf, MSmallestNormalized, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { MInf, MInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { MInf, PZero, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MZero, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MInf, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MInf, PNormalValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MNormalValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PLargestValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MLargestValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PSmallestValue, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MSmallestValue, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, PSmallestNormalized, "-inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { MInf, MSmallestNormalized, "inf", ApFloat::opOK, ApFloat::FltCategory::fcInfinity },
   { PZero, PInf, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, MInf, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, PZero, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { PZero, MZero, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { PZero, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PZero, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PZero, PNormalValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, MNormalValue, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, PLargestValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, MLargestValue, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, PSmallestValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, MSmallestValue, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, PSmallestNormalized, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PZero, MSmallestNormalized, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, PInf, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, MInf, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, PZero, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { MZero, MZero, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { MZero, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MZero, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MZero, PNormalValue, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, MNormalValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, PLargestValue, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, MLargestValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, PSmallestValue, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, MSmallestValue, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, PSmallestNormalized, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MZero, MSmallestNormalized, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { QNaN, PInf, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MInf, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, PZero, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MZero, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { QNaN, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { QNaN, PNormalValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MNormalValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, PLargestValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MLargestValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, PSmallestValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MSmallestValue, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, PSmallestNormalized, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
   { QNaN, MSmallestNormalized, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { SNaN, PInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MInf, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PZero, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MZero, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, QNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PNormalValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MNormalValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PLargestValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MLargestValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PSmallestValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MSmallestValue, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, PSmallestNormalized, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
   { SNaN, MSmallestNormalized, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PNormalValue, PInf, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PNormalValue, MInf, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PNormalValue, PZero, "inf", ApFloat::opDivByZero, ApFloat::FltCategory::fcInfinity },
   { PNormalValue, MZero, "-inf", ApFloat::opDivByZero, ApFloat::FltCategory::fcInfinity },
   { PNormalValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PNormalValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PNormalValue, PNormalValue, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PNormalValue, MNormalValue, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PNormalValue, PLargestValue, "0x1p-128", UnderflowStatus, ApFloat::FltCategory::fcNormal},
   { PNormalValue, MLargestValue, "-0x1p-128", UnderflowStatus, ApFloat::FltCategory::fcNormal},
   { PNormalValue, PSmallestValue, "inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { PNormalValue, MSmallestValue, "-inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { PNormalValue, PSmallestNormalized, "0x1p+126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PNormalValue, MSmallestNormalized, "-0x1p+126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, PInf, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MNormalValue, MInf, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MNormalValue, PZero, "-inf", ApFloat::opDivByZero, ApFloat::FltCategory::fcInfinity },
   { MNormalValue, MZero, "inf", ApFloat::opDivByZero, ApFloat::FltCategory::fcInfinity },
   { MNormalValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MNormalValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MNormalValue, PNormalValue, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, MNormalValue, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, PLargestValue, "-0x1p-128", UnderflowStatus, ApFloat::FltCategory::fcNormal},
   { MNormalValue, MLargestValue, "0x1p-128", UnderflowStatus, ApFloat::FltCategory::fcNormal},
   { MNormalValue, PSmallestValue, "-inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { MNormalValue, MSmallestValue, "inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { MNormalValue, PSmallestNormalized, "-0x1p+126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MNormalValue, MSmallestNormalized, "0x1p+126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PLargestValue, PInf, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PLargestValue, MInf, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PLargestValue, PZero, "inf", ApFloat::opDivByZero, ApFloat::FltCategory::fcInfinity },
   { PLargestValue, MZero, "-inf", ApFloat::opDivByZero, ApFloat::FltCategory::fcInfinity },
   { PLargestValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PLargestValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PLargestValue, PNormalValue, "0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PLargestValue, MNormalValue, "-0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PLargestValue, PLargestValue, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PLargestValue, MLargestValue, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PLargestValue, PSmallestValue, "inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { PLargestValue, MSmallestValue, "-inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { PLargestValue, PSmallestNormalized, "inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { PLargestValue, MSmallestNormalized, "-inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { MLargestValue, PInf, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MLargestValue, MInf, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MLargestValue, PZero, "-inf", ApFloat::opDivByZero, ApFloat::FltCategory::fcInfinity },
   { MLargestValue, MZero, "inf", ApFloat::opDivByZero, ApFloat::FltCategory::fcInfinity },
   { MLargestValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MLargestValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MLargestValue, PNormalValue, "-0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MLargestValue, MNormalValue, "0x1.fffffep+127", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MLargestValue, PLargestValue, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MLargestValue, MLargestValue, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MLargestValue, PSmallestValue, "-inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { MLargestValue, MSmallestValue, "inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { MLargestValue, PSmallestNormalized, "-inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { MLargestValue, MSmallestNormalized, "inf", OverflowStatus, ApFloat::FltCategory::fcInfinity },
   { PSmallestValue, PInf, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PSmallestValue, MInf, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PSmallestValue, PZero, "inf", ApFloat::opDivByZero, ApFloat::FltCategory::fcInfinity },
   { PSmallestValue, MZero, "-inf", ApFloat::opDivByZero, ApFloat::FltCategory::fcInfinity },
   { PSmallestValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PSmallestValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PSmallestValue, PNormalValue, "0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, MNormalValue, "-0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, PLargestValue, "0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { PSmallestValue, MLargestValue, "-0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { PSmallestValue, PSmallestValue, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, MSmallestValue, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, PSmallestNormalized, "0x1p-23", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestValue, MSmallestNormalized, "-0x1p-23", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, PInf, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MSmallestValue, MInf, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MSmallestValue, PZero, "-inf", ApFloat::opDivByZero, ApFloat::FltCategory::fcInfinity },
   { MSmallestValue, MZero, "inf", ApFloat::opDivByZero, ApFloat::FltCategory::fcInfinity },
   { MSmallestValue, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MSmallestValue, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MSmallestValue, PNormalValue, "-0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, MNormalValue, "0x1p-149", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, PLargestValue, "-0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { MSmallestValue, MLargestValue, "0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { MSmallestValue, PSmallestValue, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, MSmallestValue, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, PSmallestNormalized, "-0x1p-23", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestValue, MSmallestNormalized, "0x1p-23", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, PInf, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PSmallestNormalized, MInf, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { PSmallestNormalized, PZero, "inf", ApFloat::opDivByZero, ApFloat::FltCategory::fcInfinity },
   { PSmallestNormalized, MZero, "-inf", ApFloat::opDivByZero, ApFloat::FltCategory::fcInfinity },
   { PSmallestNormalized, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { PSmallestNormalized, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { PSmallestNormalized, PNormalValue, "0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, MNormalValue, "-0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, PLargestValue, "0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { PSmallestNormalized, MLargestValue, "-0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { PSmallestNormalized, PSmallestValue, "0x1p+23", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, MSmallestValue, "-0x1p+23", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, PSmallestNormalized, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { PSmallestNormalized, MSmallestNormalized, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, PInf, "-0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MSmallestNormalized, MInf, "0x0p+0", ApFloat::opOK, ApFloat::FltCategory::fcZero},
   { MSmallestNormalized, PZero, "-inf", ApFloat::opDivByZero, ApFloat::FltCategory::fcInfinity },
   { MSmallestNormalized, MZero, "inf", ApFloat::opDivByZero, ApFloat::FltCategory::fcInfinity },
   { MSmallestNormalized, QNaN, "nan", ApFloat::opOK, ApFloat::FltCategory::fcNaN },
#if 0
   // See Note 1.
   { MSmallestNormalized, SNaN, "nan", ApFloat::opInvalidOp, ApFloat::FltCategory::fcNaN },
#endif
   { MSmallestNormalized, PNormalValue, "-0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, MNormalValue, "0x1p-126", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, PLargestValue, "-0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { MSmallestNormalized, MLargestValue, "0x0p+0", UnderflowStatus, ApFloat::FltCategory::fcZero},
   { MSmallestNormalized, PSmallestValue, "-0x1p+23", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, MSmallestValue, "0x1p+23", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, PSmallestNormalized, "-0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
   { MSmallestNormalized, MSmallestNormalized, "0x1p+0", ApFloat::opOK, ApFloat::FltCategory::fcNormal},
};

   for (size_t i = 0; i < NumTests; ++i) {
      ApFloat x(SpecialCaseTests[i].x);
      ApFloat y(SpecialCaseTests[i].y);
      ApFloat::OpStatus status = x.divide(y, ApFloat::RoundingMode::rmNearestTiesToEven);

      ApFloat result(ApFloat::getIEEEsingle(), SpecialCaseTests[i].result);

      EXPECT_TRUE(result.bitwiseIsEqual(x));
      EXPECT_TRUE((int)status == SpecialCaseTests[i].status);
      EXPECT_TRUE(x.getCategory() == SpecialCaseTests[i].category);
   }
}

TEST(ApFloatTest, testOperatorOverloads)
{
   // This is mostly testing that these operator overloads compile.
   ApFloat One = ApFloat(ApFloat::getIEEEsingle(), "0x1p+0");
   ApFloat Two = ApFloat(ApFloat::getIEEEsingle(), "0x2p+0");
   EXPECT_TRUE(Two.bitwiseIsEqual(One + One));
   EXPECT_TRUE(One.bitwiseIsEqual(Two - One));
   EXPECT_TRUE(Two.bitwiseIsEqual(One * Two));
   EXPECT_TRUE(One.bitwiseIsEqual(Two / Two));
}

TEST(ApFloatTest, testAbs)
{
   ApFloat PInf = ApFloat::getInf(ApFloat::getIEEEsingle(), false);
   ApFloat MInf = ApFloat::getInf(ApFloat::getIEEEsingle(), true);
   ApFloat PZero = ApFloat::getZero(ApFloat::getIEEEsingle(), false);
   ApFloat MZero = ApFloat::getZero(ApFloat::getIEEEsingle(), true);
   ApFloat PQNaN = ApFloat::getNaN(ApFloat::getIEEEsingle(), false);
   ApFloat MQNaN = ApFloat::getNaN(ApFloat::getIEEEsingle(), true);
   ApFloat PSNaN = ApFloat::getSNaN(ApFloat::getIEEEsingle(), false);
   ApFloat MSNaN = ApFloat::getSNaN(ApFloat::getIEEEsingle(), true);
   ApFloat PNormalValue = ApFloat(ApFloat::getIEEEsingle(), "0x1p+0");
   ApFloat MNormalValue = ApFloat(ApFloat::getIEEEsingle(), "-0x1p+0");
   ApFloat PLargestValue = ApFloat::getLargest(ApFloat::getIEEEsingle(), false);
   ApFloat MLargestValue = ApFloat::getLargest(ApFloat::getIEEEsingle(), true);
   ApFloat PSmallestValue = ApFloat::getSmallest(ApFloat::getIEEEsingle(), false);
   ApFloat MSmallestValue = ApFloat::getSmallest(ApFloat::getIEEEsingle(), true);
   ApFloat PSmallestNormalized =
         ApFloat::getSmallestNormalized(ApFloat::getIEEEsingle(), false);
   ApFloat MSmallestNormalized =
         ApFloat::getSmallestNormalized(ApFloat::getIEEEsingle(), true);

   EXPECT_TRUE(PInf.bitwiseIsEqual(abs(PInf)));
   EXPECT_TRUE(PInf.bitwiseIsEqual(abs(MInf)));
   EXPECT_TRUE(PZero.bitwiseIsEqual(abs(PZero)));
   EXPECT_TRUE(PZero.bitwiseIsEqual(abs(MZero)));
   EXPECT_TRUE(PQNaN.bitwiseIsEqual(abs(PQNaN)));
   EXPECT_TRUE(PQNaN.bitwiseIsEqual(abs(MQNaN)));
   EXPECT_TRUE(PSNaN.bitwiseIsEqual(abs(PSNaN)));
   EXPECT_TRUE(PSNaN.bitwiseIsEqual(abs(MSNaN)));
   EXPECT_TRUE(PNormalValue.bitwiseIsEqual(abs(PNormalValue)));
   EXPECT_TRUE(PNormalValue.bitwiseIsEqual(abs(MNormalValue)));
   EXPECT_TRUE(PLargestValue.bitwiseIsEqual(abs(PLargestValue)));
   EXPECT_TRUE(PLargestValue.bitwiseIsEqual(abs(MLargestValue)));
   EXPECT_TRUE(PSmallestValue.bitwiseIsEqual(abs(PSmallestValue)));
   EXPECT_TRUE(PSmallestValue.bitwiseIsEqual(abs(MSmallestValue)));
   EXPECT_TRUE(PSmallestNormalized.bitwiseIsEqual(abs(PSmallestNormalized)));
   EXPECT_TRUE(PSmallestNormalized.bitwiseIsEqual(abs(MSmallestNormalized)));
}

TEST(ApFloatTest, testNeg)
{
   ApFloat One = ApFloat(ApFloat::getIEEEsingle(), "1.0");
   ApFloat NegOne = ApFloat(ApFloat::getIEEEsingle(), "-1.0");
   ApFloat Zero = ApFloat::getZero(ApFloat::getIEEEsingle(), false);
   ApFloat NegZero = ApFloat::getZero(ApFloat::getIEEEsingle(), true);
   ApFloat Inf = ApFloat::getInf(ApFloat::getIEEEsingle(), false);
   ApFloat NegInf = ApFloat::getInf(ApFloat::getIEEEsingle(), true);
   ApFloat QNaN = ApFloat::getNaN(ApFloat::getIEEEsingle(), false);
   ApFloat NegQNaN = ApFloat::getNaN(ApFloat::getIEEEsingle(), true);

   EXPECT_TRUE(NegOne.bitwiseIsEqual(neg(One)));
   EXPECT_TRUE(One.bitwiseIsEqual(neg(NegOne)));
   EXPECT_TRUE(NegZero.bitwiseIsEqual(neg(Zero)));
   EXPECT_TRUE(Zero.bitwiseIsEqual(neg(NegZero)));
   EXPECT_TRUE(NegInf.bitwiseIsEqual(neg(Inf)));
   EXPECT_TRUE(Inf.bitwiseIsEqual(neg(NegInf)));
   EXPECT_TRUE(NegInf.bitwiseIsEqual(neg(Inf)));
   EXPECT_TRUE(Inf.bitwiseIsEqual(neg(NegInf)));
   EXPECT_TRUE(NegQNaN.bitwiseIsEqual(neg(QNaN)));
   EXPECT_TRUE(QNaN.bitwiseIsEqual(neg(NegQNaN)));
}

TEST(ApFloatTest, testIlogb)
{
   EXPECT_EQ(-1074, ilogb(ApFloat::getSmallest(ApFloat::getIEEEdouble(), false)));
   EXPECT_EQ(-1074, ilogb(ApFloat::getSmallest(ApFloat::getIEEEdouble(), true)));
   EXPECT_EQ(-1023, ilogb(ApFloat(ApFloat::getIEEEdouble(), "0x1.ffffffffffffep-1024")));
   EXPECT_EQ(-1023, ilogb(ApFloat(ApFloat::getIEEEdouble(), "0x1.ffffffffffffep-1023")));
   EXPECT_EQ(-1023, ilogb(ApFloat(ApFloat::getIEEEdouble(), "-0x1.ffffffffffffep-1023")));
   EXPECT_EQ(-51, ilogb(ApFloat(ApFloat::getIEEEdouble(), "0x1p-51")));
   EXPECT_EQ(-1023, ilogb(ApFloat(ApFloat::getIEEEdouble(), "0x1.c60f120d9f87cp-1023")));
   EXPECT_EQ(-2, ilogb(ApFloat(ApFloat::getIEEEdouble(), "0x0.ffffp-1")));
   EXPECT_EQ(-1023, ilogb(ApFloat(ApFloat::getIEEEdouble(), "0x1.fffep-1023")));
   EXPECT_EQ(1023, ilogb(ApFloat::getLargest(ApFloat::getIEEEdouble(), false)));
   EXPECT_EQ(1023, ilogb(ApFloat::getLargest(ApFloat::getIEEEdouble(), true)));


   EXPECT_EQ(0, ilogb(ApFloat(ApFloat::getIEEEsingle(), "0x1p+0")));
   EXPECT_EQ(0, ilogb(ApFloat(ApFloat::getIEEEsingle(), "-0x1p+0")));
   EXPECT_EQ(42, ilogb(ApFloat(ApFloat::getIEEEsingle(), "0x1p+42")));
   EXPECT_EQ(-42, ilogb(ApFloat(ApFloat::getIEEEsingle(), "0x1p-42")));

   EXPECT_EQ(ApFloat::IEK_Inf,
             ilogb(ApFloat::getInf(ApFloat::getIEEEsingle(), false)));
   EXPECT_EQ(ApFloat::IEK_Inf,
             ilogb(ApFloat::getInf(ApFloat::getIEEEsingle(), true)));
   EXPECT_EQ(ApFloat::IEK_Zero,
             ilogb(ApFloat::getZero(ApFloat::getIEEEsingle(), false)));
   EXPECT_EQ(ApFloat::IEK_Zero,
             ilogb(ApFloat::getZero(ApFloat::getIEEEsingle(), true)));
   EXPECT_EQ(ApFloat::IEK_NaN,
             ilogb(ApFloat::getNaN(ApFloat::getIEEEsingle(), false)));
   EXPECT_EQ(ApFloat::IEK_NaN,
             ilogb(ApFloat::getSNaN(ApFloat::getIEEEsingle(), false)));

   EXPECT_EQ(127, ilogb(ApFloat::getLargest(ApFloat::getIEEEsingle(), false)));
   EXPECT_EQ(127, ilogb(ApFloat::getLargest(ApFloat::getIEEEsingle(), true)));

   EXPECT_EQ(-149, ilogb(ApFloat::getSmallest(ApFloat::getIEEEsingle(), false)));
   EXPECT_EQ(-149, ilogb(ApFloat::getSmallest(ApFloat::getIEEEsingle(), true)));
   EXPECT_EQ(-126,
             ilogb(ApFloat::getSmallestNormalized(ApFloat::getIEEEsingle(), false)));
   EXPECT_EQ(-126,
             ilogb(ApFloat::getSmallestNormalized(ApFloat::getIEEEsingle(), true)));
}

TEST(ApFloatTest, testScalbn)
{

   const ApFloat::RoundingMode RM = ApFloat::RoundingMode::rmNearestTiesToEven;
   EXPECT_TRUE(
            ApFloat(ApFloat::getIEEEsingle(), "0x1p+0")
            .bitwiseIsEqual(scalbn(ApFloat(ApFloat::getIEEEsingle(), "0x1p+0"), 0, RM)));
   EXPECT_TRUE(
            ApFloat(ApFloat::getIEEEsingle(), "0x1p+42")
            .bitwiseIsEqual(scalbn(ApFloat(ApFloat::getIEEEsingle(), "0x1p+0"), 42, RM)));
   EXPECT_TRUE(
            ApFloat(ApFloat::getIEEEsingle(), "0x1p-42")
            .bitwiseIsEqual(scalbn(ApFloat(ApFloat::getIEEEsingle(), "0x1p+0"), -42, RM)));

   ApFloat PInf = ApFloat::getInf(ApFloat::getIEEEsingle(), false);
   ApFloat MInf = ApFloat::getInf(ApFloat::getIEEEsingle(), true);
   ApFloat PZero = ApFloat::getZero(ApFloat::getIEEEsingle(), false);
   ApFloat MZero = ApFloat::getZero(ApFloat::getIEEEsingle(), true);
   ApFloat QPNaN = ApFloat::getNaN(ApFloat::getIEEEsingle(), false);
   ApFloat QMNaN = ApFloat::getNaN(ApFloat::getIEEEsingle(), true);
   ApFloat SNaN = ApFloat::getSNaN(ApFloat::getIEEEsingle(), false);

   EXPECT_TRUE(PInf.bitwiseIsEqual(scalbn(PInf, 0, RM)));
   EXPECT_TRUE(MInf.bitwiseIsEqual(scalbn(MInf, 0, RM)));
   EXPECT_TRUE(PZero.bitwiseIsEqual(scalbn(PZero, 0, RM)));
   EXPECT_TRUE(MZero.bitwiseIsEqual(scalbn(MZero, 0, RM)));
   EXPECT_TRUE(QPNaN.bitwiseIsEqual(scalbn(QPNaN, 0, RM)));
   EXPECT_TRUE(QMNaN.bitwiseIsEqual(scalbn(QMNaN, 0, RM)));
   EXPECT_FALSE(scalbn(SNaN, 0, RM).isSignaling());

   ApFloat ScalbnSNaN = scalbn(SNaN, 1, RM);
   EXPECT_TRUE(ScalbnSNaN.isNaN() && !ScalbnSNaN.isSignaling());

   // Make sure highest bit of payload is preserved.
   const ApInt Payload(64, (UINT64_C(1) << 50) |
                       (UINT64_C(1) << 49) |
                       (UINT64_C(1234) << 32) |
                       1);

   ApFloat SNaNWithPayload = ApFloat::getSNaN(ApFloat::getIEEEdouble(), false,
                                              &Payload);
   ApFloat QuietPayload = scalbn(SNaNWithPayload, 1, RM);
   EXPECT_TRUE(QuietPayload.isNaN() && !QuietPayload.isSignaling());
   EXPECT_EQ(Payload, QuietPayload.bitcastToApInt().getLoBits(51));

   EXPECT_TRUE(PInf.bitwiseIsEqual(
                  scalbn(ApFloat(ApFloat::getIEEEsingle(), "0x1p+0"), 128, RM)));
   EXPECT_TRUE(MInf.bitwiseIsEqual(
                  scalbn(ApFloat(ApFloat::getIEEEsingle(), "-0x1p+0"), 128, RM)));
   EXPECT_TRUE(PInf.bitwiseIsEqual(
                  scalbn(ApFloat(ApFloat::getIEEEsingle(), "0x1p+127"), 1, RM)));
   EXPECT_TRUE(PZero.bitwiseIsEqual(
                  scalbn(ApFloat(ApFloat::getIEEEsingle(), "0x1p-127"), -127, RM)));
   EXPECT_TRUE(MZero.bitwiseIsEqual(
                  scalbn(ApFloat(ApFloat::getIEEEsingle(), "-0x1p-127"), -127, RM)));
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEsingle(), "-0x1p-149").bitwiseIsEqual(
                  scalbn(ApFloat(ApFloat::getIEEEsingle(), "-0x1p-127"), -22, RM)));
   EXPECT_TRUE(PZero.bitwiseIsEqual(
                  scalbn(ApFloat(ApFloat::getIEEEsingle(), "0x1p-126"), -24, RM)));


   ApFloat SmallestF64 = ApFloat::getSmallest(ApFloat::getIEEEdouble(), false);
   ApFloat NegSmallestF64 = ApFloat::getSmallest(ApFloat::getIEEEdouble(), true);

   ApFloat LargestF64 = ApFloat::getLargest(ApFloat::getIEEEdouble(), false);
   ApFloat NegLargestF64 = ApFloat::getLargest(ApFloat::getIEEEdouble(), true);

   ApFloat SmallestNormalizedF64
         = ApFloat::getSmallestNormalized(ApFloat::getIEEEdouble(), false);
   ApFloat NegSmallestNormalizedF64
         = ApFloat::getSmallestNormalized(ApFloat::getIEEEdouble(), true);

   ApFloat LargestDenormalF64(ApFloat::getIEEEdouble(), "0x1.ffffffffffffep-1023");
   ApFloat NegLargestDenormalF64(ApFloat::getIEEEdouble(), "-0x1.ffffffffffffep-1023");


   EXPECT_TRUE(SmallestF64.bitwiseIsEqual(
                  scalbn(ApFloat(ApFloat::getIEEEdouble(), "0x1p-1074"), 0, RM)));
   EXPECT_TRUE(NegSmallestF64.bitwiseIsEqual(
                  scalbn(ApFloat(ApFloat::getIEEEdouble(), "-0x1p-1074"), 0, RM)));

   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1p+1023")
               .bitwiseIsEqual(scalbn(SmallestF64, 2097, RM)));

   EXPECT_TRUE(scalbn(SmallestF64, -2097, RM).isPosZero());
   EXPECT_TRUE(scalbn(SmallestF64, -2098, RM).isPosZero());
   EXPECT_TRUE(scalbn(SmallestF64, -2099, RM).isPosZero());
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1p+1022")
               .bitwiseIsEqual(scalbn(SmallestF64, 2096, RM)));
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1p+1023")
               .bitwiseIsEqual(scalbn(SmallestF64, 2097, RM)));
   EXPECT_TRUE(scalbn(SmallestF64, 2098, RM).isInfinity());
   EXPECT_TRUE(scalbn(SmallestF64, 2099, RM).isInfinity());

   // Test for integer overflows when adding to exponent.
   EXPECT_TRUE(scalbn(SmallestF64, -INT_MAX, RM).isPosZero());
   EXPECT_TRUE(scalbn(LargestF64, INT_MAX, RM).isInfinity());

   EXPECT_TRUE(LargestDenormalF64
               .bitwiseIsEqual(scalbn(LargestDenormalF64, 0, RM)));
   EXPECT_TRUE(NegLargestDenormalF64
               .bitwiseIsEqual(scalbn(NegLargestDenormalF64, 0, RM)));

   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1.ffffffffffffep-1022")
               .bitwiseIsEqual(scalbn(LargestDenormalF64, 1, RM)));
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "-0x1.ffffffffffffep-1021")
               .bitwiseIsEqual(scalbn(NegLargestDenormalF64, 2, RM)));

   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1.ffffffffffffep+1")
               .bitwiseIsEqual(scalbn(LargestDenormalF64, 1024, RM)));
   EXPECT_TRUE(scalbn(LargestDenormalF64, -1023, RM).isPosZero());
   EXPECT_TRUE(scalbn(LargestDenormalF64, -1024, RM).isPosZero());
   EXPECT_TRUE(scalbn(LargestDenormalF64, -2048, RM).isPosZero());
   EXPECT_TRUE(scalbn(LargestDenormalF64, 2047, RM).isInfinity());
   EXPECT_TRUE(scalbn(LargestDenormalF64, 2098, RM).isInfinity());
   EXPECT_TRUE(scalbn(LargestDenormalF64, 2099, RM).isInfinity());

   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1.ffffffffffffep-2")
               .bitwiseIsEqual(scalbn(LargestDenormalF64, 1021, RM)));
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1.ffffffffffffep-1")
               .bitwiseIsEqual(scalbn(LargestDenormalF64, 1022, RM)));
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1.ffffffffffffep+0")
               .bitwiseIsEqual(scalbn(LargestDenormalF64, 1023, RM)));
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1.ffffffffffffep+1023")
               .bitwiseIsEqual(scalbn(LargestDenormalF64, 2046, RM)));
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1p+974")
               .bitwiseIsEqual(scalbn(SmallestF64, 2048, RM)));

   ApFloat RandomDenormalF64(ApFloat::getIEEEdouble(), "0x1.c60f120d9f87cp+51");
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1.c60f120d9f87cp-972")
               .bitwiseIsEqual(scalbn(RandomDenormalF64, -1023, RM)));
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1.c60f120d9f87cp-1")
               .bitwiseIsEqual(scalbn(RandomDenormalF64, -52, RM)));
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1.c60f120d9f87cp-2")
               .bitwiseIsEqual(scalbn(RandomDenormalF64, -53, RM)));
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1.c60f120d9f87cp+0")
               .bitwiseIsEqual(scalbn(RandomDenormalF64, -51, RM)));

   EXPECT_TRUE(scalbn(RandomDenormalF64, -2097, RM).isPosZero());
   EXPECT_TRUE(scalbn(RandomDenormalF64, -2090, RM).isPosZero());


   EXPECT_TRUE(
            ApFloat(ApFloat::getIEEEdouble(), "-0x1p-1073")
            .bitwiseIsEqual(scalbn(NegLargestF64, -2097, RM)));

   EXPECT_TRUE(
            ApFloat(ApFloat::getIEEEdouble(), "-0x1p-1024")
            .bitwiseIsEqual(scalbn(NegLargestF64, -2048, RM)));

   EXPECT_TRUE(
            ApFloat(ApFloat::getIEEEdouble(), "0x1p-1073")
            .bitwiseIsEqual(scalbn(LargestF64, -2097, RM)));

   EXPECT_TRUE(
            ApFloat(ApFloat::getIEEEdouble(), "0x1p-1074")
            .bitwiseIsEqual(scalbn(LargestF64, -2098, RM)));
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "-0x1p-1074")
               .bitwiseIsEqual(scalbn(NegLargestF64, -2098, RM)));
   EXPECT_TRUE(scalbn(NegLargestF64, -2099, RM).isNegZero());
   EXPECT_TRUE(scalbn(LargestF64, 1, RM).isInfinity());


   EXPECT_TRUE(
            ApFloat(ApFloat::getIEEEdouble(), "0x1p+0")
            .bitwiseIsEqual(scalbn(ApFloat(ApFloat::getIEEEdouble(), "0x1p+52"), -52, RM)));

   EXPECT_TRUE(
            ApFloat(ApFloat::getIEEEdouble(), "0x1p-103")
            .bitwiseIsEqual(scalbn(ApFloat(ApFloat::getIEEEdouble(), "0x1p-51"), -52, RM)));
}

TEST(ApFloatTest, testFrexp)
{
   const ApFloat::RoundingMode RM = ApFloat::RoundingMode::rmNearestTiesToEven;

   ApFloat PZero = ApFloat::getZero(ApFloat::getIEEEdouble(), false);
   ApFloat MZero = ApFloat::getZero(ApFloat::getIEEEdouble(), true);
   ApFloat One(1.0);
   ApFloat MOne(-1.0);
   ApFloat Two(2.0);
   ApFloat MTwo(-2.0);

   ApFloat LargestDenormal(ApFloat::getIEEEdouble(), "0x1.ffffffffffffep-1023");
   ApFloat NegLargestDenormal(ApFloat::getIEEEdouble(), "-0x1.ffffffffffffep-1023");

   ApFloat Smallest = ApFloat::getSmallest(ApFloat::getIEEEdouble(), false);
   ApFloat NegSmallest = ApFloat::getSmallest(ApFloat::getIEEEdouble(), true);

   ApFloat Largest = ApFloat::getLargest(ApFloat::getIEEEdouble(), false);
   ApFloat NegLargest = ApFloat::getLargest(ApFloat::getIEEEdouble(), true);

   ApFloat PInf = ApFloat::getInf(ApFloat::getIEEEdouble(), false);
   ApFloat MInf = ApFloat::getInf(ApFloat::getIEEEdouble(), true);

   ApFloat QPNaN = ApFloat::getNaN(ApFloat::getIEEEdouble(), false);
   ApFloat QMNaN = ApFloat::getNaN(ApFloat::getIEEEdouble(), true);
   ApFloat SNaN = ApFloat::getSNaN(ApFloat::getIEEEdouble(), false);

   // Make sure highest bit of payload is preserved.
   const ApInt Payload(64, (UINT64_C(1) << 50) |
                       (UINT64_C(1) << 49) |
                       (UINT64_C(1234) << 32) |
                       1);

   ApFloat SNaNWithPayload = ApFloat::getSNaN(ApFloat::getIEEEdouble(), false,
                                              &Payload);

   ApFloat SmallestNormalized
         = ApFloat::getSmallestNormalized(ApFloat::getIEEEdouble(), false);
   ApFloat NegSmallestNormalized
         = ApFloat::getSmallestNormalized(ApFloat::getIEEEdouble(), true);

   int Exp;
   ApFloat Frac(ApFloat::getIEEEdouble());


   Frac = frexp(PZero, Exp, RM);
   EXPECT_EQ(0, Exp);
   EXPECT_TRUE(Frac.isPosZero());

   Frac = frexp(MZero, Exp, RM);
   EXPECT_EQ(0, Exp);
   EXPECT_TRUE(Frac.isNegZero());


   Frac = frexp(One, Exp, RM);
   EXPECT_EQ(1, Exp);
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1p-1").bitwiseIsEqual(Frac));

   Frac = frexp(MOne, Exp, RM);
   EXPECT_EQ(1, Exp);
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "-0x1p-1").bitwiseIsEqual(Frac));

   Frac = frexp(LargestDenormal, Exp, RM);
   EXPECT_EQ(-1022, Exp);
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1.ffffffffffffep-1").bitwiseIsEqual(Frac));

   Frac = frexp(NegLargestDenormal, Exp, RM);
   EXPECT_EQ(-1022, Exp);
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "-0x1.ffffffffffffep-1").bitwiseIsEqual(Frac));


   Frac = frexp(Smallest, Exp, RM);
   EXPECT_EQ(-1073, Exp);
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1p-1").bitwiseIsEqual(Frac));

   Frac = frexp(NegSmallest, Exp, RM);
   EXPECT_EQ(-1073, Exp);
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "-0x1p-1").bitwiseIsEqual(Frac));


   Frac = frexp(Largest, Exp, RM);
   EXPECT_EQ(1024, Exp);
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1.fffffffffffffp-1").bitwiseIsEqual(Frac));

   Frac = frexp(NegLargest, Exp, RM);
   EXPECT_EQ(1024, Exp);
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "-0x1.fffffffffffffp-1").bitwiseIsEqual(Frac));


   Frac = frexp(PInf, Exp, RM);
   EXPECT_EQ(INT_MAX, Exp);
   EXPECT_TRUE(Frac.isInfinity() && !Frac.isNegative());

   Frac = frexp(MInf, Exp, RM);
   EXPECT_EQ(INT_MAX, Exp);
   EXPECT_TRUE(Frac.isInfinity() && Frac.isNegative());

   Frac = frexp(QPNaN, Exp, RM);
   EXPECT_EQ(INT_MIN, Exp);
   EXPECT_TRUE(Frac.isNaN());

   Frac = frexp(QMNaN, Exp, RM);
   EXPECT_EQ(INT_MIN, Exp);
   EXPECT_TRUE(Frac.isNaN());

   Frac = frexp(SNaN, Exp, RM);
   EXPECT_EQ(INT_MIN, Exp);
   EXPECT_TRUE(Frac.isNaN() && !Frac.isSignaling());

   Frac = frexp(SNaNWithPayload, Exp, RM);
   EXPECT_EQ(INT_MIN, Exp);
   EXPECT_TRUE(Frac.isNaN() && !Frac.isSignaling());
   EXPECT_EQ(Payload, Frac.bitcastToApInt().getLoBits(51));

   Frac = frexp(ApFloat(ApFloat::getIEEEdouble(), "0x0.ffffp-1"), Exp, RM);
   EXPECT_EQ(-1, Exp);
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1.fffep-1").bitwiseIsEqual(Frac));

   Frac = frexp(ApFloat(ApFloat::getIEEEdouble(), "0x1p-51"), Exp, RM);
   EXPECT_EQ(-50, Exp);
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1p-1").bitwiseIsEqual(Frac));

   Frac = frexp(ApFloat(ApFloat::getIEEEdouble(), "0x1.c60f120d9f87cp+51"), Exp, RM);
   EXPECT_EQ(52, Exp);
   EXPECT_TRUE(ApFloat(ApFloat::getIEEEdouble(), "0x1.c60f120d9f87cp-1").bitwiseIsEqual(Frac));
}

TEST(ApFloatTest, testMod)
{
   {
      ApFloat f1(ApFloat::getIEEEdouble(), "1.5");
      ApFloat f2(ApFloat::getIEEEdouble(), "1.0");
      ApFloat expected(ApFloat::getIEEEdouble(), "0.5");
      EXPECT_EQ(f1.mod(f2), ApFloat::opOK);
      EXPECT_TRUE(f1.bitwiseIsEqual(expected));
   }
   {
      ApFloat f1(ApFloat::getIEEEdouble(), "0.5");
      ApFloat f2(ApFloat::getIEEEdouble(), "1.0");
      ApFloat expected(ApFloat::getIEEEdouble(), "0.5");
      EXPECT_EQ(f1.mod(f2), ApFloat::opOK);
      EXPECT_TRUE(f1.bitwiseIsEqual(expected));
   }
   {
      ApFloat f1(ApFloat::getIEEEdouble(), "0x1.3333333333333p-2"); // 0.3
      ApFloat f2(ApFloat::getIEEEdouble(), "0x1.47ae147ae147bp-7"); // 0.01
      ApFloat expected(ApFloat::getIEEEdouble(),
                       "0x1.47ae147ae1471p-7"); // 0.009999999999999983
      EXPECT_EQ(f1.mod(f2), ApFloat::opOK);
      EXPECT_TRUE(f1.bitwiseIsEqual(expected));
   }
   {
      ApFloat f1(ApFloat::getIEEEdouble(), "0x1p64"); // 1.8446744073709552e19
      ApFloat f2(ApFloat::getIEEEdouble(), "1.5");
      ApFloat expected(ApFloat::getIEEEdouble(), "1.0");
      EXPECT_EQ(f1.mod(f2), ApFloat::opOK);
      EXPECT_TRUE(f1.bitwiseIsEqual(expected));
   }
   {
      ApFloat f1(ApFloat::getIEEEdouble(), "0x1p1000");
      ApFloat f2(ApFloat::getIEEEdouble(), "0x1p-1000");
      ApFloat expected(ApFloat::getIEEEdouble(), "0.0");
      EXPECT_EQ(f1.mod(f2), ApFloat::opOK);
      EXPECT_TRUE(f1.bitwiseIsEqual(expected));
   }
   {
      ApFloat f1(ApFloat::getIEEEdouble(), "0.0");
      ApFloat f2(ApFloat::getIEEEdouble(), "1.0");
      ApFloat expected(ApFloat::getIEEEdouble(), "0.0");
      EXPECT_EQ(f1.mod(f2), ApFloat::opOK);
      EXPECT_TRUE(f1.bitwiseIsEqual(expected));
   }
   {
      ApFloat f1(ApFloat::getIEEEdouble(), "1.0");
      ApFloat f2(ApFloat::getIEEEdouble(), "0.0");
      EXPECT_EQ(f1.mod(f2), ApFloat::opInvalidOp);
      EXPECT_TRUE(f1.isNaN());
   }
   {
      ApFloat f1(ApFloat::getIEEEdouble(), "0.0");
      ApFloat f2(ApFloat::getIEEEdouble(), "0.0");
      EXPECT_EQ(f1.mod(f2), ApFloat::opInvalidOp);
      EXPECT_TRUE(f1.isNaN());
   }
   {
      ApFloat f1 = ApFloat::getInf(ApFloat::getIEEEdouble(), false);
      ApFloat f2(ApFloat::getIEEEdouble(), "1.0");
      EXPECT_EQ(f1.mod(f2), ApFloat::opInvalidOp);
      EXPECT_TRUE(f1.isNaN());
   }
   {
      ApFloat f1(ApFloat::getIEEEdouble(), "-4.0");
      ApFloat f2(ApFloat::getIEEEdouble(), "-2.0");
      ApFloat expected(ApFloat::getIEEEdouble(), "-0.0");
      EXPECT_EQ(f1.mod(f2), ApFloat::opOK);
      EXPECT_TRUE(f1.bitwiseIsEqual(expected));
   }
   {
      ApFloat f1(ApFloat::getIEEEdouble(), "-4.0");
      ApFloat f2(ApFloat::getIEEEdouble(), "2.0");
      ApFloat expected(ApFloat::getIEEEdouble(), "-0.0");
      EXPECT_EQ(f1.mod(f2), ApFloat::opOK);
      EXPECT_TRUE(f1.bitwiseIsEqual(expected));
   }
}

TEST(ApFloatTest, testPPCDoubleDoubleAddSpecial)
{
   using DataType = std::tuple<uint64_t, uint64_t, uint64_t, uint64_t,
   ApFloat::FltCategory, ApFloat::RoundingMode>;
   DataType Data[] = {
      // (1 + 0) + (-1 + 0) = fcZero
      std::make_tuple(0x3ff0000000000000ull, 0, 0xbff0000000000000ull, 0,
      ApFloat::FltCategory::fcZero, ApFloat::RoundingMode::rmNearestTiesToEven),
      // LDBL_MAX + (1.1 >> (1023 - 106) + 0)) = fcInfinity
      std::make_tuple(0x7fefffffffffffffull, 0x7c8ffffffffffffeull,
      0x7948000000000000ull, 0ull, ApFloat::FltCategory::fcInfinity,
      ApFloat::RoundingMode::rmNearestTiesToEven),
      // TODO: change the 4th 0x75effffffffffffe to 0x75efffffffffffff when
      // semPPCDoubleDoubleLegacy is gone.
      // LDBL_MAX + (1.011111... >> (1023 - 106) + (1.1111111...0 >> (1023 -
      // 160))) = fcNormal
      std::make_tuple(0x7fefffffffffffffull, 0x7c8ffffffffffffeull,
      0x7947ffffffffffffull, 0x75effffffffffffeull,
      ApFloat::FltCategory::fcNormal, ApFloat::RoundingMode::rmNearestTiesToEven),
      // LDBL_MAX + (1.1 >> (1023 - 106) + 0)) = fcInfinity
      std::make_tuple(0x7fefffffffffffffull, 0x7c8ffffffffffffeull,
      0x7fefffffffffffffull, 0x7c8ffffffffffffeull,
      ApFloat::FltCategory::fcInfinity, ApFloat::RoundingMode::rmNearestTiesToEven),
      // NaN + (1 + 0) = fcNaN
      std::make_tuple(0x7ff8000000000000ull, 0, 0x3ff0000000000000ull, 0,
      ApFloat::FltCategory::fcNaN, ApFloat::RoundingMode::rmNearestTiesToEven),
   };

   for (auto Tp : Data) {
      uint64_t Op1[2], Op2[2];
      ApFloat::FltCategory Expected;
      ApFloat::RoundingMode RM;
      std::tie(Op1[0], Op1[1], Op2[0], Op2[1], Expected, RM) = Tp;

      {
         ApFloat A1(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op1));
         ApFloat A2(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op2));
         A1.add(A2, RM);
         EXPECT_EQ(Expected, A1.getCategory())
               << formatv("({0:x} + {1:x}) + ({2:x} + {3:x})", Op1[0], Op1[1],
               Op2[0], Op2[1])
               .getStr();
      }
      {
         ApFloat A1(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op1));
         ApFloat A2(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op2));
         A2.add(A1, RM);
         EXPECT_EQ(Expected, A2.getCategory())
               << formatv("({0:x} + {1:x}) + ({2:x} + {3:x})", Op2[0], Op2[1],
               Op1[0], Op1[1])
               .getStr();
      }
   }
}

TEST(ApFloatTest, testPPCDoubleDoubleAdd)
{
   using DataType = std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
   uint64_t, ApFloat::RoundingMode>;
   DataType Data[] = {
      // (1 + 0) + (1e-105 + 0) = (1 + 1e-105)
      std::make_tuple(0x3ff0000000000000ull, 0, 0x3960000000000000ull, 0,
      0x3ff0000000000000ull, 0x3960000000000000ull,
      ApFloat::RoundingMode::rmNearestTiesToEven),
      // (1 + 0) + (1e-106 + 0) = (1 + 1e-106)
      std::make_tuple(0x3ff0000000000000ull, 0, 0x3950000000000000ull, 0,
      0x3ff0000000000000ull, 0x3950000000000000ull,
      ApFloat::RoundingMode::rmNearestTiesToEven),
      // (1 + 1e-106) + (1e-106 + 0) = (1 + 1e-105)
      std::make_tuple(0x3ff0000000000000ull, 0x3950000000000000ull,
      0x3950000000000000ull, 0, 0x3ff0000000000000ull,
      0x3960000000000000ull, ApFloat::RoundingMode::rmNearestTiesToEven),
      // (1 + 0) + (epsilon + 0) = (1 + epsilon)
      std::make_tuple(0x3ff0000000000000ull, 0, 0x0000000000000001ull, 0,
      0x3ff0000000000000ull, 0x0000000000000001ull,
      ApFloat::RoundingMode::rmNearestTiesToEven),
      // TODO: change 0xf950000000000000 to 0xf940000000000000, when
      // semPPCDoubleDoubleLegacy is gone.
      // (DBL_MAX - 1 << (1023 - 105)) + (1 << (1023 - 53) + 0) = DBL_MAX +
      // 1.11111... << (1023 - 52)
      std::make_tuple(0x7fefffffffffffffull, 0xf950000000000000ull,
      0x7c90000000000000ull, 0, 0x7fefffffffffffffull,
      0x7c8ffffffffffffeull, ApFloat::RoundingMode::rmNearestTiesToEven),
      // TODO: change 0xf950000000000000 to 0xf940000000000000, when
      // semPPCDoubleDoubleLegacy is gone.
      // (1 << (1023 - 53) + 0) + (DBL_MAX - 1 << (1023 - 105)) = DBL_MAX +
      // 1.11111... << (1023 - 52)
      std::make_tuple(0x7c90000000000000ull, 0, 0x7fefffffffffffffull,
      0xf950000000000000ull, 0x7fefffffffffffffull,
      0x7c8ffffffffffffeull, ApFloat::RoundingMode::rmNearestTiesToEven),
   };

   for (auto Tp : Data) {
      uint64_t Op1[2], Op2[2], Expected[2];
      ApFloat::RoundingMode RM;
      std::tie(Op1[0], Op1[1], Op2[0], Op2[1], Expected[0], Expected[1], RM) = Tp;

      {
         ApFloat A1(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op1));
         ApFloat A2(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op2));
         A1.add(A2, RM);
         EXPECT_EQ(Expected[0], A1.bitcastToApInt().getRawData()[0])
               << formatv("({0:x} + {1:x}) + ({2:x} + {3:x})", Op1[0], Op1[1],
               Op2[0], Op2[1])
               .getStr();
         EXPECT_EQ(Expected[1], A1.bitcastToApInt().getRawData()[1])
               << formatv("({0:x} + {1:x}) + ({2:x} + {3:x})", Op1[0], Op1[1],
               Op2[0], Op2[1])
               .getStr();
      }
      {
         ApFloat A1(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op1));
         ApFloat A2(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op2));
         A2.add(A1, RM);

         EXPECT_EQ(Expected[0], A2.bitcastToApInt().getRawData()[0])
               << formatv("({0:x} + {1:x}) + ({2:x} + {3:x})", Op2[0], Op2[1],
               Op1[0], Op1[1])
               .getStr();
         EXPECT_EQ(Expected[1], A2.bitcastToApInt().getRawData()[1])
               << formatv("({0:x} + {1:x}) + ({2:x} + {3:x})", Op2[0], Op2[1],
               Op1[0], Op1[1])
               .getStr();
      }
   }
}

TEST(ApFloatTest, testPPCDoubleDoubleSubtract)
{
   using DataType = std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
   uint64_t, ApFloat::RoundingMode>;
   DataType Data[] = {
      // (1 + 0) - (-1e-105 + 0) = (1 + 1e-105)
      std::make_tuple(0x3ff0000000000000ull, 0, 0xb960000000000000ull, 0,
      0x3ff0000000000000ull, 0x3960000000000000ull,
      ApFloat::RoundingMode::rmNearestTiesToEven),
      // (1 + 0) - (-1e-106 + 0) = (1 + 1e-106)
      std::make_tuple(0x3ff0000000000000ull, 0, 0xb950000000000000ull, 0,
      0x3ff0000000000000ull, 0x3950000000000000ull,
      ApFloat::RoundingMode::rmNearestTiesToEven),
   };

   for (auto Tp : Data) {
      uint64_t Op1[2], Op2[2], Expected[2];
      ApFloat::RoundingMode RM;
      std::tie(Op1[0], Op1[1], Op2[0], Op2[1], Expected[0], Expected[1], RM) = Tp;

      ApFloat A1(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op1));
      ApFloat A2(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op2));
      A1.subtract(A2, RM);
      EXPECT_EQ(Expected[0], A1.bitcastToApInt().getRawData()[0])
            << formatv("({0:x} + {1:x}) - ({2:x} + {3:x})", Op1[0], Op1[1], Op2[0],
            Op2[1])
            .getStr();
      EXPECT_EQ(Expected[1], A1.bitcastToApInt().getRawData()[1])
            << formatv("({0:x} + {1:x}) - ({2:x} + {3:x})", Op1[0], Op1[1], Op2[0],
            Op2[1])
            .getStr();
   }
}

TEST(ApFloatTest, testPPCDoubleDoubleMultiplySpecial)
{
   using DataType = std::tuple<uint64_t, uint64_t, uint64_t, uint64_t,
   ApFloat::FltCategory, ApFloat::RoundingMode>;
   DataType Data[] = {
      // fcNaN * fcNaN = fcNaN
      std::make_tuple(0x7ff8000000000000ull, 0, 0x7ff8000000000000ull, 0,
      ApFloat::FltCategory::fcNaN, ApFloat::RoundingMode::rmNearestTiesToEven),
      // fcNaN * fcZero = fcNaN
      std::make_tuple(0x7ff8000000000000ull, 0, 0, 0, ApFloat::FltCategory::fcNaN,
      ApFloat::RoundingMode::rmNearestTiesToEven),
      // fcNaN * fcInfinity = fcNaN
      std::make_tuple(0x7ff8000000000000ull, 0, 0x7ff0000000000000ull, 0,
      ApFloat::FltCategory::fcNaN, ApFloat::RoundingMode::rmNearestTiesToEven),
      // fcNaN * fcNormal = fcNaN
      std::make_tuple(0x7ff8000000000000ull, 0, 0x3ff0000000000000ull, 0,
      ApFloat::FltCategory::fcNaN, ApFloat::RoundingMode::rmNearestTiesToEven),
      // fcInfinity * fcInfinity = fcInfinity
      std::make_tuple(0x7ff0000000000000ull, 0, 0x7ff0000000000000ull, 0,
      ApFloat::FltCategory::fcInfinity, ApFloat::RoundingMode::rmNearestTiesToEven),
      // fcInfinity * fcZero = fcNaN
      std::make_tuple(0x7ff0000000000000ull, 0, 0, 0, ApFloat::FltCategory::fcNaN,
      ApFloat::RoundingMode::rmNearestTiesToEven),
      // fcInfinity * fcNormal = fcInfinity
      std::make_tuple(0x7ff0000000000000ull, 0, 0x3ff0000000000000ull, 0,
      ApFloat::FltCategory::fcInfinity, ApFloat::RoundingMode::rmNearestTiesToEven),
      // fcZero * fcZero = fcZero
      std::make_tuple(0, 0, 0, 0, ApFloat::FltCategory::fcZero,
      ApFloat::RoundingMode::rmNearestTiesToEven),
      // fcZero * fcNormal = fcZero
      std::make_tuple(0, 0, 0x3ff0000000000000ull, 0, ApFloat::FltCategory::fcZero,
      ApFloat::RoundingMode::rmNearestTiesToEven),
   };

   for (auto Tp : Data) {
      uint64_t Op1[2], Op2[2];
      ApFloat::FltCategory Expected;
      ApFloat::RoundingMode RM;
      std::tie(Op1[0], Op1[1], Op2[0], Op2[1], Expected, RM) = Tp;

      {
         ApFloat A1(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op1));
         ApFloat A2(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op2));
         A1.multiply(A2, RM);
         EXPECT_EQ(Expected, A1.getCategory())
               << formatv("({0:x} + {1:x}) * ({2:x} + {3:x})", Op1[0], Op1[1],
               Op2[0], Op2[1])
               .getStr();
      }
      {
         ApFloat A1(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op1));
         ApFloat A2(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op2));
         A2.multiply(A1, RM);
         EXPECT_EQ(Expected, A2.getCategory())
               << formatv("({0:x} + {1:x}) * ({2:x} + {3:x})", Op2[0], Op2[1],
               Op1[0], Op1[1])
               .getStr();
      }
   }
}

TEST(ApFloatTest, testPPCDoubleDoubleMultiply)
{
   using DataType = std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
   uint64_t, ApFloat::RoundingMode>;
   DataType Data[] = {
      // 1/3 * 3 = 1.0
      std::make_tuple(0x3fd5555555555555ull, 0x3c75555555555556ull,
      0x4008000000000000ull, 0, 0x3ff0000000000000ull, 0,
      ApFloat::RoundingMode::rmNearestTiesToEven),
      // (1 + epsilon) * (1 + 0) = fcZero
      std::make_tuple(0x3ff0000000000000ull, 0x0000000000000001ull,
      0x3ff0000000000000ull, 0, 0x3ff0000000000000ull,
      0x0000000000000001ull, ApFloat::RoundingMode::rmNearestTiesToEven),
      // (1 + epsilon) * (1 + epsilon) = 1 + 2 * epsilon
      std::make_tuple(0x3ff0000000000000ull, 0x0000000000000001ull,
      0x3ff0000000000000ull, 0x0000000000000001ull,
      0x3ff0000000000000ull, 0x0000000000000002ull,
      ApFloat::RoundingMode::rmNearestTiesToEven),
      // -(1 + epsilon) * (1 + epsilon) = -1
      std::make_tuple(0xbff0000000000000ull, 0x0000000000000001ull,
      0x3ff0000000000000ull, 0x0000000000000001ull,
      0xbff0000000000000ull, 0, ApFloat::RoundingMode::rmNearestTiesToEven),
      // (0.5 + 0) * (1 + 2 * epsilon) = 0.5 + epsilon
      std::make_tuple(0x3fe0000000000000ull, 0, 0x3ff0000000000000ull,
      0x0000000000000002ull, 0x3fe0000000000000ull,
      0x0000000000000001ull, ApFloat::RoundingMode::rmNearestTiesToEven),
      // (0.5 + 0) * (1 + epsilon) = 0.5
      std::make_tuple(0x3fe0000000000000ull, 0, 0x3ff0000000000000ull,
      0x0000000000000001ull, 0x3fe0000000000000ull, 0,
      ApFloat::RoundingMode::rmNearestTiesToEven),
      // __LDBL_MAX__ * (1 + 1 << 106) = inf
      std::make_tuple(0x7fefffffffffffffull, 0x7c8ffffffffffffeull,
      0x3ff0000000000000ull, 0x3950000000000000ull,
      0x7ff0000000000000ull, 0, ApFloat::RoundingMode::rmNearestTiesToEven),
      // __LDBL_MAX__ * (1 + 1 << 107) > __LDBL_MAX__, but not inf, yes =_=|||
      std::make_tuple(0x7fefffffffffffffull, 0x7c8ffffffffffffeull,
      0x3ff0000000000000ull, 0x3940000000000000ull,
      0x7fefffffffffffffull, 0x7c8fffffffffffffull,
      ApFloat::RoundingMode::rmNearestTiesToEven),
      // __LDBL_MAX__ * (1 + 1 << 108) = __LDBL_MAX__
      std::make_tuple(0x7fefffffffffffffull, 0x7c8ffffffffffffeull,
      0x3ff0000000000000ull, 0x3930000000000000ull,
      0x7fefffffffffffffull, 0x7c8ffffffffffffeull,
      ApFloat::RoundingMode::rmNearestTiesToEven),
   };

   for (auto Tp : Data) {
      uint64_t Op1[2], Op2[2], Expected[2];
      ApFloat::RoundingMode RM;
      std::tie(Op1[0], Op1[1], Op2[0], Op2[1], Expected[0], Expected[1], RM) = Tp;

      {
         ApFloat A1(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op1));
         ApFloat A2(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op2));
         A1.multiply(A2, RM);
         EXPECT_EQ(Expected[0], A1.bitcastToApInt().getRawData()[0])
               << formatv("({0:x} + {1:x}) * ({2:x} + {3:x})", Op1[0], Op1[1],
               Op2[0], Op2[1])
               .getStr();
         EXPECT_EQ(Expected[1], A1.bitcastToApInt().getRawData()[1])
               << formatv("({0:x} + {1:x}) * ({2:x} + {3:x})", Op1[0], Op1[1],
               Op2[0], Op2[1])
               .getStr();
      }
      {
         ApFloat A1(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op1));
         ApFloat A2(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op2));
         A2.multiply(A1, RM);

         EXPECT_EQ(Expected[0], A2.bitcastToApInt().getRawData()[0])
               << formatv("({0:x} + {1:x}) * ({2:x} + {3:x})", Op2[0], Op2[1],
               Op1[0], Op1[1])
               .getStr();
         EXPECT_EQ(Expected[1], A2.bitcastToApInt().getRawData()[1])
               << formatv("({0:x} + {1:x}) * ({2:x} + {3:x})", Op2[0], Op2[1],
               Op1[0], Op1[1])
               .getStr();
      }
   }
}

TEST(ApFloatTest, testPPCDoubleDoubleDivide)
{
   using DataType = std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
   uint64_t, ApFloat::RoundingMode>;
   // TODO: Only a sanity check for now. Add more edge cases when the
   // double-double algorithm is implemented.
   DataType Data[] = {
      // 1 / 3 = 1/3
      std::make_tuple(0x3ff0000000000000ull, 0, 0x4008000000000000ull, 0,
      0x3fd5555555555555ull, 0x3c75555555555556ull,
      ApFloat::RoundingMode::rmNearestTiesToEven),
   };

   for (auto Tp : Data) {
      uint64_t Op1[2], Op2[2], Expected[2];
      ApFloat::RoundingMode RM;
      std::tie(Op1[0], Op1[1], Op2[0], Op2[1], Expected[0], Expected[1], RM) = Tp;

      ApFloat A1(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op1));
      ApFloat A2(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op2));
      A1.divide(A2, RM);
      EXPECT_EQ(Expected[0], A1.bitcastToApInt().getRawData()[0])
            << formatv("({0:x} + {1:x}) / ({2:x} + {3:x})", Op1[0], Op1[1], Op2[0],
            Op2[1])
            .getStr();
      EXPECT_EQ(Expected[1], A1.bitcastToApInt().getRawData()[1])
            << formatv("({0:x} + {1:x}) / ({2:x} + {3:x})", Op1[0], Op1[1], Op2[0],
            Op2[1])
            .getStr();
   }
}

TEST(ApFloatTest, testPPCDoubleDoubleRemainder)
{
   using DataType =
   std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>;
   DataType Data[] = {
      // remainder(3.0 + 3.0 << 53, 1.25 + 1.25 << 53) = (0.5 + 0.5 << 53)
      std::make_tuple(0x4008000000000000ull, 0x3cb8000000000000ull,
      0x3ff4000000000000ull, 0x3ca4000000000000ull,
      0x3fe0000000000000ull, 0x3c90000000000000ull),
      // remainder(3.0 + 3.0 << 53, 1.75 + 1.75 << 53) = (-0.5 - 0.5 << 53)
      std::make_tuple(0x4008000000000000ull, 0x3cb8000000000000ull,
      0x3ffc000000000000ull, 0x3cac000000000000ull,
      0xbfe0000000000000ull, 0xbc90000000000000ull),
   };

   for (auto Tp : Data) {
      uint64_t Op1[2], Op2[2], Expected[2];
      std::tie(Op1[0], Op1[1], Op2[0], Op2[1], Expected[0], Expected[1]) = Tp;

      ApFloat A1(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op1));
      ApFloat A2(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op2));
      A1.remainder(A2);
      EXPECT_EQ(Expected[0], A1.bitcastToApInt().getRawData()[0])
            << formatv("remainder({0:x} + {1:x}), ({2:x} + {3:x}))", Op1[0], Op1[1],
            Op2[0], Op2[1])
            .getStr();
      EXPECT_EQ(Expected[1], A1.bitcastToApInt().getRawData()[1])
            << formatv("remainder(({0:x} + {1:x}), ({2:x} + {3:x}))", Op1[0],
            Op1[1], Op2[0], Op2[1])
            .getStr();
   }
}

TEST(ApFloatTest, testPPCDoubleDoubleMod)
{
   using DataType =
   std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>;
   DataType Data[] = {
      // mod(3.0 + 3.0 << 53, 1.25 + 1.25 << 53) = (0.5 + 0.5 << 53)
      std::make_tuple(0x4008000000000000ull, 0x3cb8000000000000ull,
      0x3ff4000000000000ull, 0x3ca4000000000000ull,
      0x3fe0000000000000ull, 0x3c90000000000000ull),
      // mod(3.0 + 3.0 << 53, 1.75 + 1.75 << 53) = (1.25 + 1.25 << 53)
      // 0xbc98000000000000 doesn't seem right, but it's what we currently have.
      // TODO: investigate
      std::make_tuple(0x4008000000000000ull, 0x3cb8000000000000ull,
      0x3ffc000000000000ull, 0x3cac000000000000ull,
      0x3ff4000000000001ull, 0xbc98000000000000ull),
   };

   for (auto Tp : Data) {
      uint64_t Op1[2], Op2[2], Expected[2];
      std::tie(Op1[0], Op1[1], Op2[0], Op2[1], Expected[0], Expected[1]) = Tp;

      ApFloat A1(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op1));
      ApFloat A2(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op2));
      A1.mod(A2);
      EXPECT_EQ(Expected[0], A1.bitcastToApInt().getRawData()[0])
            << formatv("fmod(({0:x} + {1:x}),  ({2:x} + {3:x}))", Op1[0], Op1[1],
            Op2[0], Op2[1])
            .getStr();
      EXPECT_EQ(Expected[1], A1.bitcastToApInt().getRawData()[1])
            << formatv("fmod(({0:x} + {1:x}), ({2:x} + {3:x}))", Op1[0], Op1[1],
            Op2[0], Op2[1])
            .getStr();
   }
}

TEST(ApFloatTest, testPPCDoubleDoubleFMA)
{
   // Sanity check for now.
   ApFloat A(ApFloat::getPPCDoubleDouble(), "2");
   A.fusedMultiplyAdd(ApFloat(ApFloat::getPPCDoubleDouble(), "3"),
                      ApFloat(ApFloat::getPPCDoubleDouble(), "4"),
                      ApFloat::RoundingMode::rmNearestTiesToEven);
   EXPECT_EQ(ApFloat::CmpResult::cmpEqual,
             ApFloat(ApFloat::getPPCDoubleDouble(), "10").compare(A));
}

TEST(ApFloatTest, testPPCDoubleDoubleRoundToIntegral)
{
   {
      ApFloat A(ApFloat::getPPCDoubleDouble(), "1.5");
      A.roundToIntegral(ApFloat::RoundingMode::rmNearestTiesToEven);
      EXPECT_EQ(ApFloat::CmpResult::cmpEqual,
                ApFloat(ApFloat::getPPCDoubleDouble(), "2").compare(A));
   }
   {
      ApFloat A(ApFloat::getPPCDoubleDouble(), "2.5");
      A.roundToIntegral(ApFloat::RoundingMode::rmNearestTiesToEven);
      EXPECT_EQ(ApFloat::CmpResult::cmpEqual,
                ApFloat(ApFloat::getPPCDoubleDouble(), "2").compare(A));
   }
}

TEST(ApFloatTest, testPPCDoubleDoubleCompare)
{
   using DataType =
   std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, ApFloat::CmpResult>;

   DataType Data[] = {
      // (1 + 0) = (1 + 0)
      std::make_tuple(0x3ff0000000000000ull, 0, 0x3ff0000000000000ull, 0,
      ApFloat::CmpResult::cmpEqual),
      // (1 + 0) < (1.00...1 + 0)
      std::make_tuple(0x3ff0000000000000ull, 0, 0x3ff0000000000001ull, 0,
      ApFloat::CmpResult::cmpLessThan),
      // (1.00...1 + 0) > (1 + 0)
      std::make_tuple(0x3ff0000000000001ull, 0, 0x3ff0000000000000ull, 0,
      ApFloat::CmpResult::cmpGreaterThan),
      // (1 + 0) < (1 + epsilon)
      std::make_tuple(0x3ff0000000000000ull, 0, 0x3ff0000000000001ull,
      0x0000000000000001ull, ApFloat::CmpResult::cmpLessThan),
      // NaN != NaN
      std::make_tuple(0x7ff8000000000000ull, 0, 0x7ff8000000000000ull, 0,
      ApFloat::CmpResult::cmpUnordered),
      // (1 + 0) != NaN
      std::make_tuple(0x3ff0000000000000ull, 0, 0x7ff8000000000000ull, 0,
      ApFloat::CmpResult::cmpUnordered),
      // Inf = Inf
      std::make_tuple(0x7ff0000000000000ull, 0, 0x7ff0000000000000ull, 0,
      ApFloat::CmpResult::cmpEqual),
   };

   for (auto Tp : Data) {
      uint64_t Op1[2], Op2[2];
      ApFloat::CmpResult Expected;
      std::tie(Op1[0], Op1[1], Op2[0], Op2[1], Expected) = Tp;

      ApFloat A1(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op1));
      ApFloat A2(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op2));
      EXPECT_EQ(Expected, A1.compare(A2))
            << formatv("compare(({0:x} + {1:x}), ({2:x} + {3:x}))", Op1[0], Op1[1],
            Op2[0], Op2[1])
            .getStr();
   }
}

TEST(ApFloatTest, testPPCDoubleDoubleBitwiseIsEqual)
{
   using DataType = std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, bool>;

   DataType Data[] = {
      // (1 + 0) = (1 + 0)
      std::make_tuple(0x3ff0000000000000ull, 0, 0x3ff0000000000000ull, 0, true),
      // (1 + 0) != (1.00...1 + 0)
      std::make_tuple(0x3ff0000000000000ull, 0, 0x3ff0000000000001ull, 0,
      false),
      // NaN = NaN
      std::make_tuple(0x7ff8000000000000ull, 0, 0x7ff8000000000000ull, 0, true),
      // NaN != NaN with a different bit pattern
      std::make_tuple(0x7ff8000000000000ull, 0, 0x7ff8000000000000ull,
      0x3ff0000000000000ull, false),
      // Inf = Inf
      std::make_tuple(0x7ff0000000000000ull, 0, 0x7ff0000000000000ull, 0, true),
   };

   for (auto Tp : Data) {
      uint64_t Op1[2], Op2[2];
      bool Expected;
      std::tie(Op1[0], Op1[1], Op2[0], Op2[1], Expected) = Tp;

      ApFloat A1(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op1));
      ApFloat A2(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Op2));
      EXPECT_EQ(Expected, A1.bitwiseIsEqual(A2))
            << formatv("({0:x} + {1:x}) = ({2:x} + {3:x})", Op1[0], Op1[1], Op2[0],
            Op2[1])
            .getStr();
   }
}

TEST(ApFloatTest, testPPCDoubleDoubleHashValue)
{
   uint64_t Data1[] = {0x3ff0000000000001ull, 0x0000000000000001ull};
   uint64_t Data2[] = {0x3ff0000000000001ull, 0};
   // The hash values are *hopefully* different.
   EXPECT_NE(
            hash_value(ApFloat(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Data1))),
            hash_value(ApFloat(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Data2))));
}

TEST(ApFloatTest, testPPCDoubleDoubleChangeSign)
{
   uint64_t Data[] = {
      0x400f000000000000ull, 0xbcb0000000000000ull,
   };
   ApFloat Float(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Data));
   {
      ApFloat Actual =
            ApFloat::copySign(Float, ApFloat(ApFloat::getIEEEdouble(), "1"));
      EXPECT_EQ(0x400f000000000000ull, Actual.bitcastToApInt().getRawData()[0]);
      EXPECT_EQ(0xbcb0000000000000ull, Actual.bitcastToApInt().getRawData()[1]);
   }
   {
      ApFloat Actual =
            ApFloat::copySign(Float, ApFloat(ApFloat::getIEEEdouble(), "-1"));
      EXPECT_EQ(0xc00f000000000000ull, Actual.bitcastToApInt().getRawData()[0]);
      EXPECT_EQ(0x3cb0000000000000ull, Actual.bitcastToApInt().getRawData()[1]);
   }
}

TEST(ApFloatTest, testPPCDoubleDoubleFactories)
{
   {
      uint64_t Data[] = {
         0, 0,
      };
      EXPECT_EQ(ApInt(128, 2, Data),
                ApFloat::getZero(ApFloat::getPPCDoubleDouble()).bitcastToApInt());
   }
   {
      uint64_t Data[] = {
         0x7fefffffffffffffull, 0x7c8ffffffffffffeull,
      };
      EXPECT_EQ(ApInt(128, 2, Data),
                ApFloat::getLargest(ApFloat::getPPCDoubleDouble()).bitcastToApInt());
   }
   {
      uint64_t Data[] = {
         0x0000000000000001ull, 0,
      };
      EXPECT_EQ(
               ApInt(128, 2, Data),
               ApFloat::getSmallest(ApFloat::getPPCDoubleDouble()).bitcastToApInt());
   }
   {
      uint64_t Data[] = {0x0360000000000000ull, 0};
      EXPECT_EQ(ApInt(128, 2, Data),
                ApFloat::getSmallestNormalized(ApFloat::getPPCDoubleDouble())
                .bitcastToApInt());
   }
   {
      uint64_t Data[] = {
         0x8000000000000000ull, 0x0000000000000000ull,
      };
      EXPECT_EQ(
               ApInt(128, 2, Data),
               ApFloat::getZero(ApFloat::getPPCDoubleDouble(), true).bitcastToApInt());
   }
   {
      uint64_t Data[] = {
         0xffefffffffffffffull, 0xfc8ffffffffffffeull,
      };
      EXPECT_EQ(
               ApInt(128, 2, Data),
               ApFloat::getLargest(ApFloat::getPPCDoubleDouble(), true).bitcastToApInt());
   }
   {
      uint64_t Data[] = {
         0x8000000000000001ull, 0x0000000000000000ull,
      };
      EXPECT_EQ(ApInt(128, 2, Data),
                ApFloat::getSmallest(ApFloat::getPPCDoubleDouble(), true)
                .bitcastToApInt());
   }
   {
      uint64_t Data[] = {
         0x8360000000000000ull, 0x0000000000000000ull,
      };
      EXPECT_EQ(ApInt(128, 2, Data),
                ApFloat::getSmallestNormalized(ApFloat::getPPCDoubleDouble(), true)
                .bitcastToApInt());
   }
   EXPECT_TRUE(ApFloat::getSmallest(ApFloat::getPPCDoubleDouble()).isSmallest());
   EXPECT_TRUE(ApFloat::getLargest(ApFloat::getPPCDoubleDouble()).isLargest());
}

TEST(ApFloatTest, testPPCDoubleDoubleIsDenormal)
{
   EXPECT_TRUE(ApFloat::getSmallest(ApFloat::getPPCDoubleDouble()).isDenormal());
   EXPECT_FALSE(ApFloat::getLargest(ApFloat::getPPCDoubleDouble()).isDenormal());
   EXPECT_FALSE(
            ApFloat::getSmallestNormalized(ApFloat::getPPCDoubleDouble()).isDenormal());
   {
      // (4 + 3) is not normalized
      uint64_t Data[] = {
         0x4010000000000000ull, 0x4008000000000000ull,
      };
      EXPECT_TRUE(
               ApFloat(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Data)).isDenormal());
   }
}

TEST(ApFloatTest, testPPCDoubleDoubleScalbn)
{
   // 3.0 + 3.0 << 53
   uint64_t Input[] = {
      0x4008000000000000ull, 0x3cb8000000000000ull,
   };
   ApFloat Result =
         scalbn(ApFloat(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Input)), 1,
                ApFloat::RoundingMode::rmNearestTiesToEven);
   // 6.0 + 6.0 << 53
   EXPECT_EQ(0x4018000000000000ull, Result.bitcastToApInt().getRawData()[0]);
   EXPECT_EQ(0x3cc8000000000000ull, Result.bitcastToApInt().getRawData()[1]);
}

TEST(ApFloatTest, testPPCDoubleDoubleFrexp)
{
   // 3.0 + 3.0 << 53
   uint64_t Input[] = {
      0x4008000000000000ull, 0x3cb8000000000000ull,
   };
   int Exp;
   // 0.75 + 0.75 << 53
   ApFloat Result =
         frexp(ApFloat(ApFloat::getPPCDoubleDouble(), ApInt(128, 2, Input)), Exp,
               ApFloat::RoundingMode::rmNearestTiesToEven);
   EXPECT_EQ(2, Exp);
   EXPECT_EQ(0x3fe8000000000000ull, Result.bitcastToApInt().getRawData()[0]);
   EXPECT_EQ(0x3c98000000000000ull, Result.bitcastToApInt().getRawData()[1]);
}

}

} // anonymous namespace
