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

#include "polarphp/basic/adt/ApInt.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/Twine.h"
#include "gtest/gtest.h"
#include <array>

using namespace polar::basic;

namespace {

TEST(ApIntTest, testValueInit)
{
   ApInt zero = ApInt();
   EXPECT_TRUE(!zero);
   EXPECT_TRUE(!zero.zext(64));
   EXPECT_TRUE(!zero.sext(64));
}

// Test that ApInt shift left works when bitwidth > 64 and shiftamt == 0
TEST(ApIntTest, testShiftLeftByZero)
{
   ApInt One = ApInt::getNullValue(65) + 1;
   ApInt Shl = One.shl(0);
   EXPECT_TRUE(Shl[0]);
   EXPECT_FALSE(Shl[1]);
}

TEST(ApIntTest, testI64ArithmeticRightShiftNegative)
{
   const ApInt neg_one(64, static_cast<uint64_t>(-1), true);
   EXPECT_EQ(neg_one, neg_one.ashr(7));
}

TEST(ApIntTest, testI128NegativeCount)
{
   ApInt minus3(128, static_cast<uint64_t>(-3), true);
   EXPECT_EQ(126u, minus3.countLeadingOnes());
   EXPECT_EQ(-3, minus3.getSignExtValue());

   ApInt minus1(128, static_cast<uint64_t>(-1), true);
   EXPECT_EQ(0u, minus1.countLeadingZeros());
   EXPECT_EQ(128u, minus1.countLeadingOnes());
   EXPECT_EQ(128u, minus1.getActiveBits());
   EXPECT_EQ(0u, minus1.countTrailingZeros());
   EXPECT_EQ(128u, minus1.countTrailingOnes());
   EXPECT_EQ(128u, minus1.countPopulation());
   EXPECT_EQ(-1, minus1.getSignExtValue());
}

TEST(ApIntTest, testI33Count)
{
   ApInt i33minus2(33, static_cast<uint64_t>(-2), true);
   EXPECT_EQ(0u, i33minus2.countLeadingZeros());
   EXPECT_EQ(32u, i33minus2.countLeadingOnes());
   EXPECT_EQ(33u, i33minus2.getActiveBits());
   EXPECT_EQ(1u, i33minus2.countTrailingZeros());
   EXPECT_EQ(32u, i33minus2.countPopulation());
   EXPECT_EQ(-2, i33minus2.getSignExtValue());
   EXPECT_EQ(((uint64_t)-2)&((1ull<<33) -1), i33minus2.getZeroExtValue());
}

TEST(ApIntTest, testI61Count)
{
   ApInt i61(61, 1 << 15);
   EXPECT_EQ(45u, i61.countLeadingZeros());
   EXPECT_EQ(0u, i61.countLeadingOnes());
   EXPECT_EQ(16u, i61.getActiveBits());
   EXPECT_EQ(15u, i61.countTrailingZeros());
   EXPECT_EQ(1u, i61.countPopulation());
   EXPECT_EQ(static_cast<int64_t>(1 << 15), i61.getSignExtValue());
   EXPECT_EQ(static_cast<uint64_t>(1 << 15), i61.getZeroExtValue());

   i61.setBits(8, 19);
   EXPECT_EQ(42u, i61.countLeadingZeros());
   EXPECT_EQ(0u, i61.countLeadingOnes());
   EXPECT_EQ(19u, i61.getActiveBits());
   EXPECT_EQ(8u, i61.countTrailingZeros());
   EXPECT_EQ(11u, i61.countPopulation());
   EXPECT_EQ(static_cast<int64_t>((1 << 19) - (1 << 8)), i61.getSignExtValue());
   EXPECT_EQ(static_cast<uint64_t>((1 << 19) - (1 << 8)), i61.getZeroExtValue());
}

TEST(ApIntTest, testI65Count)
{
   ApInt i65(65, 0, true);
   EXPECT_EQ(65u, i65.countLeadingZeros());
   EXPECT_EQ(0u, i65.countLeadingOnes());
   EXPECT_EQ(0u, i65.getActiveBits());
   EXPECT_EQ(1u, i65.getActiveWords());
   EXPECT_EQ(65u, i65.countTrailingZeros());
   EXPECT_EQ(0u, i65.countPopulation());

   ApInt i65minus(65, 0, true);
   i65minus.setBit(64);
   EXPECT_EQ(0u, i65minus.countLeadingZeros());
   EXPECT_EQ(1u, i65minus.countLeadingOnes());
   EXPECT_EQ(65u, i65minus.getActiveBits());
   EXPECT_EQ(64u, i65minus.countTrailingZeros());
   EXPECT_EQ(1u, i65minus.countPopulation());
}

TEST(ApIntTest, testI128PositiveCount)
{
   ApInt u128max = ApInt::getAllOnesValue(128);
   EXPECT_EQ(128u, u128max.countLeadingOnes());
   EXPECT_EQ(0u, u128max.countLeadingZeros());
   EXPECT_EQ(128u, u128max.getActiveBits());
   EXPECT_EQ(0u, u128max.countTrailingZeros());
   EXPECT_EQ(128u, u128max.countTrailingOnes());
   EXPECT_EQ(128u, u128max.countPopulation());

   ApInt u64max(128, static_cast<uint64_t>(-1), false);
   EXPECT_EQ(64u, u64max.countLeadingZeros());
   EXPECT_EQ(0u, u64max.countLeadingOnes());
   EXPECT_EQ(64u, u64max.getActiveBits());
   EXPECT_EQ(0u, u64max.countTrailingZeros());
   EXPECT_EQ(64u, u64max.countTrailingOnes());
   EXPECT_EQ(64u, u64max.countPopulation());
   EXPECT_EQ((uint64_t)~0ull, u64max.getZeroExtValue());

   ApInt zero(128, 0, true);
   EXPECT_EQ(128u, zero.countLeadingZeros());
   EXPECT_EQ(0u, zero.countLeadingOnes());
   EXPECT_EQ(0u, zero.getActiveBits());
   EXPECT_EQ(128u, zero.countTrailingZeros());
   EXPECT_EQ(0u, zero.countTrailingOnes());
   EXPECT_EQ(0u, zero.countPopulation());
   EXPECT_EQ(0u, zero.getSignExtValue());
   EXPECT_EQ(0u, zero.getZeroExtValue());

   ApInt one(128, 1, true);
   EXPECT_EQ(127u, one.countLeadingZeros());
   EXPECT_EQ(0u, one.countLeadingOnes());
   EXPECT_EQ(1u, one.getActiveBits());
   EXPECT_EQ(0u, one.countTrailingZeros());
   EXPECT_EQ(1u, one.countTrailingOnes());
   EXPECT_EQ(1u, one.countPopulation());
   EXPECT_EQ(1, one.getSignExtValue());
   EXPECT_EQ(1u, one.getZeroExtValue());

   ApInt s128(128, 2, true);
   EXPECT_EQ(126u, s128.countLeadingZeros());
   EXPECT_EQ(0u, s128.countLeadingOnes());
   EXPECT_EQ(2u, s128.getActiveBits());
   EXPECT_EQ(1u, s128.countTrailingZeros());
   EXPECT_EQ(0u, s128.countTrailingOnes());
   EXPECT_EQ(1u, s128.countPopulation());
   EXPECT_EQ(2, s128.getSignExtValue());
   EXPECT_EQ(2u, s128.getZeroExtValue());

   // NOP Test
   s128.setBits(42, 42);
   EXPECT_EQ(126u, s128.countLeadingZeros());
   EXPECT_EQ(0u, s128.countLeadingOnes());
   EXPECT_EQ(2u, s128.getActiveBits());
   EXPECT_EQ(1u, s128.countTrailingZeros());
   EXPECT_EQ(0u, s128.countTrailingOnes());
   EXPECT_EQ(1u, s128.countPopulation());
   EXPECT_EQ(2, s128.getSignExtValue());
   EXPECT_EQ(2u, s128.getZeroExtValue());

   s128.setBits(3, 32);
   EXPECT_EQ(96u, s128.countLeadingZeros());
   EXPECT_EQ(0u, s128.countLeadingOnes());
   EXPECT_EQ(32u, s128.getActiveBits());
   EXPECT_EQ(33u, s128.getMinSignedBits());
   EXPECT_EQ(1u, s128.countTrailingZeros());
   EXPECT_EQ(0u, s128.countTrailingOnes());
   EXPECT_EQ(30u, s128.countPopulation());
   EXPECT_EQ(static_cast<uint32_t>((~0u << 3) | 2), s128.getZeroExtValue());

   s128.setBits(62, 128);
   EXPECT_EQ(0u, s128.countLeadingZeros());
   EXPECT_EQ(66u, s128.countLeadingOnes());
   EXPECT_EQ(128u, s128.getActiveBits());
   EXPECT_EQ(63u, s128.getMinSignedBits());
   EXPECT_EQ(1u, s128.countTrailingZeros());
   EXPECT_EQ(0u, s128.countTrailingOnes());
   EXPECT_EQ(96u, s128.countPopulation());
   EXPECT_EQ(static_cast<int64_t>((3ull << 62) |
                                  static_cast<uint32_t>((~0u << 3) | 2)),
             s128.getSignExtValue());
}

TEST(ApIntTest, testI256)
{
   ApInt s256(256, 15, true);
   EXPECT_EQ(252u, s256.countLeadingZeros());
   EXPECT_EQ(0u, s256.countLeadingOnes());
   EXPECT_EQ(4u, s256.getActiveBits());
   EXPECT_EQ(0u, s256.countTrailingZeros());
   EXPECT_EQ(4u, s256.countTrailingOnes());
   EXPECT_EQ(4u, s256.countPopulation());
   EXPECT_EQ(15, s256.getSignExtValue());
   EXPECT_EQ(15u, s256.getZeroExtValue());

   s256.setBits(62, 66);
   EXPECT_EQ(190u, s256.countLeadingZeros());
   EXPECT_EQ(0u, s256.countLeadingOnes());
   EXPECT_EQ(66u, s256.getActiveBits());
   EXPECT_EQ(67u, s256.getMinSignedBits());
   EXPECT_EQ(0u, s256.countTrailingZeros());
   EXPECT_EQ(4u, s256.countTrailingOnes());
   EXPECT_EQ(8u, s256.countPopulation());

   s256.setBits(60, 256);
   EXPECT_EQ(0u, s256.countLeadingZeros());
   EXPECT_EQ(196u, s256.countLeadingOnes());
   EXPECT_EQ(256u, s256.getActiveBits());
   EXPECT_EQ(61u, s256.getMinSignedBits());
   EXPECT_EQ(0u, s256.countTrailingZeros());
   EXPECT_EQ(4u, s256.countTrailingOnes());
   EXPECT_EQ(200u, s256.countPopulation());
   EXPECT_EQ(static_cast<int64_t>((~0ull << 60) | 15), s256.getSignExtValue());
}

TEST(ApIntTest, testI1)
{
   const ApInt neg_two(1, static_cast<uint64_t>(-2), true);
   const ApInt neg_one(1, static_cast<uint64_t>(-1), true);
   const ApInt zero(1, 0);
   const ApInt one(1, 1);
   const ApInt two(1, 2);

   EXPECT_EQ(0, neg_two.getSignExtValue());
   EXPECT_EQ(-1, neg_one.getSignExtValue());
   EXPECT_EQ(1u, neg_one.getZeroExtValue());
   EXPECT_EQ(0u, zero.getZeroExtValue());
   EXPECT_EQ(-1, one.getSignExtValue());
   EXPECT_EQ(1u, one.getZeroExtValue());
   EXPECT_EQ(0u, two.getZeroExtValue());
   EXPECT_EQ(0, two.getSignExtValue());

   // Basic equalities for 1-bit values.
   EXPECT_EQ(zero, two);
   EXPECT_EQ(zero, neg_two);
   EXPECT_EQ(one, neg_one);
   EXPECT_EQ(two, neg_two);

   // Min/max signed values.
   EXPECT_TRUE(zero.isMaxSignedValue());
   EXPECT_FALSE(one.isMaxSignedValue());
   EXPECT_FALSE(zero.isMinSignedValue());
   EXPECT_TRUE(one.isMinSignedValue());

   // Additions.
   EXPECT_EQ(two, one + one);
   EXPECT_EQ(zero, neg_one + one);
   EXPECT_EQ(neg_two, neg_one + neg_one);

   // Subtractions.
   EXPECT_EQ(neg_two, neg_one - one);
   EXPECT_EQ(two, one - neg_one);
   EXPECT_EQ(zero, one - one);

   // And
   EXPECT_EQ(zero, zero & zero);
   EXPECT_EQ(zero, one & zero);
   EXPECT_EQ(zero, zero & one);
   EXPECT_EQ(one, one & one);
   EXPECT_EQ(zero, zero & zero);
   EXPECT_EQ(zero, neg_one & zero);
   EXPECT_EQ(zero, zero & neg_one);
   EXPECT_EQ(neg_one, neg_one & neg_one);

   // Or
   EXPECT_EQ(zero, zero | zero);
   EXPECT_EQ(one, one | zero);
   EXPECT_EQ(one, zero | one);
   EXPECT_EQ(one, one | one);
   EXPECT_EQ(zero, zero | zero);
   EXPECT_EQ(neg_one, neg_one | zero);
   EXPECT_EQ(neg_one, zero | neg_one);
   EXPECT_EQ(neg_one, neg_one | neg_one);

   // Xor
   EXPECT_EQ(zero, zero ^ zero);
   EXPECT_EQ(one, one ^ zero);
   EXPECT_EQ(one, zero ^ one);
   EXPECT_EQ(zero, one ^ one);
   EXPECT_EQ(zero, zero ^ zero);
   EXPECT_EQ(neg_one, neg_one ^ zero);
   EXPECT_EQ(neg_one, zero ^ neg_one);
   EXPECT_EQ(zero, neg_one ^ neg_one);

   // Shifts.
   EXPECT_EQ(zero, one << one);
   EXPECT_EQ(one, one << zero);
   EXPECT_EQ(zero, one.shl(1));
   EXPECT_EQ(one, one.shl(0));
   EXPECT_EQ(zero, one.lshr(1));
   EXPECT_EQ(one, one.ashr(1));

   // Rotates.
   EXPECT_EQ(one, one.rotl(0));
   EXPECT_EQ(one, one.rotl(1));
   EXPECT_EQ(one, one.rotr(0));
   EXPECT_EQ(one, one.rotr(1));

   // Multiplies.
   EXPECT_EQ(neg_one, neg_one * one);
   EXPECT_EQ(neg_one, one * neg_one);
   EXPECT_EQ(one, neg_one * neg_one);
   EXPECT_EQ(one, one * one);

   // Divides.
   EXPECT_EQ(neg_one, one.sdiv(neg_one));
   EXPECT_EQ(neg_one, neg_one.sdiv(one));
   EXPECT_EQ(one, neg_one.sdiv(neg_one));
   EXPECT_EQ(one, one.sdiv(one));

   EXPECT_EQ(neg_one, one.udiv(neg_one));
   EXPECT_EQ(neg_one, neg_one.udiv(one));
   EXPECT_EQ(one, neg_one.udiv(neg_one));
   EXPECT_EQ(one, one.udiv(one));

   // Remainders.
   EXPECT_EQ(zero, neg_one.srem(one));
   EXPECT_EQ(zero, neg_one.urem(one));
   EXPECT_EQ(zero, one.srem(neg_one));

   // sdivrem
   {
      ApInt q(8, 0);
      ApInt r(8, 0);
      ApInt one(8, 1);
      ApInt two(8, 2);
      ApInt nine(8, 9);
      ApInt four(8, 4);

      EXPECT_EQ(nine.srem(two), one);
      EXPECT_EQ(nine.srem(-two), one);
      EXPECT_EQ((-nine).srem(two), -one);
      EXPECT_EQ((-nine).srem(-two), -one);

      ApInt::sdivrem(nine, two, q, r);
      EXPECT_EQ(four, q);
      EXPECT_EQ(one, r);
      ApInt::sdivrem(-nine, two, q, r);
      EXPECT_EQ(-four, q);
      EXPECT_EQ(-one, r);
      ApInt::sdivrem(nine, -two, q, r);
      EXPECT_EQ(-four, q);
      EXPECT_EQ(one, r);
      ApInt::sdivrem(-nine, -two, q, r);
      EXPECT_EQ(four, q);
      EXPECT_EQ(-one, r);
   }
}

TEST(ApIntTest, testCompare) {
   std::array<ApInt, 5> testVals{{
         ApInt{16, 2},
         ApInt{16, 1},
         ApInt{16, 0},
         ApInt{16, (uint64_t)-1, true},
         ApInt{16, (uint64_t)-2, true},
                                 }};

   for (auto &arg1 : testVals)
      for (auto &arg2 : testVals) {
         auto uv1 = arg1.getZeroExtValue();
         auto uv2 = arg2.getZeroExtValue();
         auto sv1 = arg1.getSignExtValue();
         auto sv2 = arg2.getSignExtValue();

         EXPECT_EQ(uv1 <  uv2, arg1.ult(arg2));
         EXPECT_EQ(uv1 <= uv2, arg1.ule(arg2));
         EXPECT_EQ(uv1 >  uv2, arg1.ugt(arg2));
         EXPECT_EQ(uv1 >= uv2, arg1.uge(arg2));

         EXPECT_EQ(sv1 <  sv2, arg1.slt(arg2));
         EXPECT_EQ(sv1 <= sv2, arg1.sle(arg2));
         EXPECT_EQ(sv1 >  sv2, arg1.sgt(arg2));
         EXPECT_EQ(sv1 >= sv2, arg1.sge(arg2));

         EXPECT_EQ(uv1 <  uv2, arg1.ult(uv2));
         EXPECT_EQ(uv1 <= uv2, arg1.ule(uv2));
         EXPECT_EQ(uv1 >  uv2, arg1.ugt(uv2));
         EXPECT_EQ(uv1 >= uv2, arg1.uge(uv2));

         EXPECT_EQ(sv1 <  sv2, arg1.slt(sv2));
         EXPECT_EQ(sv1 <= sv2, arg1.sle(sv2));
         EXPECT_EQ(sv1 >  sv2, arg1.sgt(sv2));
         EXPECT_EQ(sv1 >= sv2, arg1.sge(sv2));
      }
}

TEST(ApIntTest, testCompareWithRawIntegers)
{
   EXPECT_TRUE(!ApInt(8, 1).uge(256));
   EXPECT_TRUE(!ApInt(8, 1).ugt(256));
   EXPECT_TRUE( ApInt(8, 1).ule(256));
   EXPECT_TRUE( ApInt(8, 1).ult(256));
   EXPECT_TRUE(!ApInt(8, 1).sge(256));
   EXPECT_TRUE(!ApInt(8, 1).sgt(256));
   EXPECT_TRUE( ApInt(8, 1).sle(256));
   EXPECT_TRUE( ApInt(8, 1).slt(256));
   EXPECT_TRUE(!(ApInt(8, 0) == 256));
   EXPECT_TRUE(  ApInt(8, 0) != 256);
   EXPECT_TRUE(!(ApInt(8, 1) == 256));
   EXPECT_TRUE(  ApInt(8, 1) != 256);

   auto uint64max = UINT64_MAX;
   auto int64max  = INT64_MAX;
   auto int64min  = INT64_MIN;

   auto u64 = ApInt{128, uint64max};
   auto s64 = ApInt{128, static_cast<uint64_t>(int64max), true};
   auto big = u64 + 1;

   EXPECT_TRUE( u64.uge(uint64max));
   EXPECT_TRUE(!u64.ugt(uint64max));
   EXPECT_TRUE( u64.ule(uint64max));
   EXPECT_TRUE(!u64.ult(uint64max));
   EXPECT_TRUE( u64.sge(int64max));
   EXPECT_TRUE( u64.sgt(int64max));
   EXPECT_TRUE(!u64.sle(int64max));
   EXPECT_TRUE(!u64.slt(int64max));
   EXPECT_TRUE( u64.sge(int64min));
   EXPECT_TRUE( u64.sgt(int64min));
   EXPECT_TRUE(!u64.sle(int64min));
   EXPECT_TRUE(!u64.slt(int64min));

   EXPECT_TRUE(u64 == uint64max);
   EXPECT_TRUE(u64 != int64max);
   EXPECT_TRUE(u64 != int64min);

   EXPECT_TRUE(!s64.uge(uint64max));
   EXPECT_TRUE(!s64.ugt(uint64max));
   EXPECT_TRUE( s64.ule(uint64max));
   EXPECT_TRUE( s64.ult(uint64max));
   EXPECT_TRUE( s64.sge(int64max));
   EXPECT_TRUE(!s64.sgt(int64max));
   EXPECT_TRUE( s64.sle(int64max));
   EXPECT_TRUE(!s64.slt(int64max));
   EXPECT_TRUE( s64.sge(int64min));
   EXPECT_TRUE( s64.sgt(int64min));
   EXPECT_TRUE(!s64.sle(int64min));
   EXPECT_TRUE(!s64.slt(int64min));

   EXPECT_TRUE(s64 != uint64max);
   EXPECT_TRUE(s64 == int64max);
   EXPECT_TRUE(s64 != int64min);

   EXPECT_TRUE( big.uge(uint64max));
   EXPECT_TRUE( big.ugt(uint64max));
   EXPECT_TRUE(!big.ule(uint64max));
   EXPECT_TRUE(!big.ult(uint64max));
   EXPECT_TRUE( big.sge(int64max));
   EXPECT_TRUE( big.sgt(int64max));
   EXPECT_TRUE(!big.sle(int64max));
   EXPECT_TRUE(!big.slt(int64max));
   EXPECT_TRUE( big.sge(int64min));
   EXPECT_TRUE( big.sgt(int64min));
   EXPECT_TRUE(!big.sle(int64min));
   EXPECT_TRUE(!big.slt(int64min));

   EXPECT_TRUE(big != uint64max);
   EXPECT_TRUE(big != int64max);
   EXPECT_TRUE(big != int64min);
}

TEST(ApIntTest, testCompareWithInt64Min)
{
   int64_t edge = INT64_MIN;
   int64_t edgeP1 = edge + 1;
   int64_t edgeM1 = INT64_MAX;
   auto a = ApInt{64, static_cast<uint64_t>(edge), true};

   EXPECT_TRUE(!a.slt(edge));
   EXPECT_TRUE( a.sle(edge));
   EXPECT_TRUE(!a.sgt(edge));
   EXPECT_TRUE( a.sge(edge));
   EXPECT_TRUE( a.slt(edgeP1));
   EXPECT_TRUE( a.sle(edgeP1));
   EXPECT_TRUE(!a.sgt(edgeP1));
   EXPECT_TRUE(!a.sge(edgeP1));
   EXPECT_TRUE( a.slt(edgeM1));
   EXPECT_TRUE( a.sle(edgeM1));
   EXPECT_TRUE(!a.sgt(edgeM1));
   EXPECT_TRUE(!a.sge(edgeM1));
}

TEST(ApIntTest, testCompareWithHalfInt64Max)
{
   uint64_t edge = 0x4000000000000000;
   uint64_t edgeP1 = edge + 1;
   uint64_t edgeM1 = edge - 1;
   auto a = ApInt{64, edge};

   EXPECT_TRUE(!a.ult(edge));
   EXPECT_TRUE( a.ule(edge));
   EXPECT_TRUE(!a.ugt(edge));
   EXPECT_TRUE( a.uge(edge));
   EXPECT_TRUE( a.ult(edgeP1));
   EXPECT_TRUE( a.ule(edgeP1));
   EXPECT_TRUE(!a.ugt(edgeP1));
   EXPECT_TRUE(!a.uge(edgeP1));
   EXPECT_TRUE(!a.ult(edgeM1));
   EXPECT_TRUE(!a.ule(edgeM1));
   EXPECT_TRUE( a.ugt(edgeM1));
   EXPECT_TRUE( a.uge(edgeM1));

   EXPECT_TRUE(!a.slt(edge));
   EXPECT_TRUE( a.sle(edge));
   EXPECT_TRUE(!a.sgt(edge));
   EXPECT_TRUE( a.sge(edge));
   EXPECT_TRUE( a.slt(edgeP1));
   EXPECT_TRUE( a.sle(edgeP1));
   EXPECT_TRUE(!a.sgt(edgeP1));
   EXPECT_TRUE(!a.sge(edgeP1));
   EXPECT_TRUE(!a.slt(edgeM1));
   EXPECT_TRUE(!a.sle(edgeM1));
   EXPECT_TRUE( a.sgt(edgeM1));
   EXPECT_TRUE( a.sge(edgeM1));
}

TEST(ApIntTest, testCompareLargeIntegers)
{
   // Make sure all the combinations of signed comparisons work with big ints.
   auto One = ApInt{128, static_cast<uint64_t>(1), true};
   auto Two = ApInt{128, static_cast<uint64_t>(2), true};
   auto MinusOne = ApInt{128, static_cast<uint64_t>(-1), true};
   auto MinusTwo = ApInt{128, static_cast<uint64_t>(-2), true};

   EXPECT_TRUE(!One.slt(One));
   EXPECT_TRUE(!Two.slt(One));
   EXPECT_TRUE(MinusOne.slt(One));
   EXPECT_TRUE(MinusTwo.slt(One));

   EXPECT_TRUE(One.slt(Two));
   EXPECT_TRUE(!Two.slt(Two));
   EXPECT_TRUE(MinusOne.slt(Two));
   EXPECT_TRUE(MinusTwo.slt(Two));

   EXPECT_TRUE(!One.slt(MinusOne));
   EXPECT_TRUE(!Two.slt(MinusOne));
   EXPECT_TRUE(!MinusOne.slt(MinusOne));
   EXPECT_TRUE(MinusTwo.slt(MinusOne));

   EXPECT_TRUE(!One.slt(MinusTwo));
   EXPECT_TRUE(!Two.slt(MinusTwo));
   EXPECT_TRUE(!MinusOne.slt(MinusTwo));
   EXPECT_TRUE(!MinusTwo.slt(MinusTwo));
}

TEST(ApIntTest, testBinaryOpsWithRawIntegers)
{
   // Single word check.
   uint64_t E1 = 0x2CA7F46BF6569915ULL;
   ApInt A1(64, E1);

   EXPECT_EQ(A1 & E1, E1);
   EXPECT_EQ(A1 & 0, 0);
   EXPECT_EQ(A1 & 1, 1);
   EXPECT_EQ(A1 & 5, 5);
   EXPECT_EQ(A1 & UINT64_MAX, E1);

   EXPECT_EQ(A1 | E1, E1);
   EXPECT_EQ(A1 | 0, E1);
   EXPECT_EQ(A1 | 1, E1);
   EXPECT_EQ(A1 | 2, E1 | 2);
   EXPECT_EQ(A1 | UINT64_MAX, UINT64_MAX);

   EXPECT_EQ(A1 ^ E1, 0);
   EXPECT_EQ(A1 ^ 0, E1);
   EXPECT_EQ(A1 ^ 1, E1 ^ 1);
   EXPECT_EQ(A1 ^ 7, E1 ^ 7);
   EXPECT_EQ(A1 ^ UINT64_MAX, ~E1);

   // Multiword check.
   uint64_t N = 0xEB6EB136591CBA21ULL;
   ApInt::WordType E2[4] = {
      N,
      0x7B9358BD6A33F10AULL,
      0x7E7FFA5EADD8846ULL,
      0x305F341CA00B613DULL
   };
   ApInt A2(ApInt::APINT_BITS_PER_WORD*4, E2);

   EXPECT_EQ(A2 & N, N);
   EXPECT_EQ(A2 & 0, 0);
   EXPECT_EQ(A2 & 1, 1);
   EXPECT_EQ(A2 & 5, 1);
   EXPECT_EQ(A2 & UINT64_MAX, N);

   EXPECT_EQ(A2 | N, A2);
   EXPECT_EQ(A2 | 0, A2);
   EXPECT_EQ(A2 | 1, A2);
   EXPECT_EQ(A2 | 2, A2 + 2);
   EXPECT_EQ(A2 | UINT64_MAX, A2 - N + UINT64_MAX);

   EXPECT_EQ(A2 ^ N, A2 - N);
   EXPECT_EQ(A2 ^ 0, A2);
   EXPECT_EQ(A2 ^ 1, A2 - 1);
   EXPECT_EQ(A2 ^ 7, A2 + 5);
   EXPECT_EQ(A2 ^ UINT64_MAX, A2 - N + ~N);
}

TEST(ApIntTest, testRvalueArithmetic)
{
   // Test all combinations of lvalue/rvalue lhs/rhs of add/sub

   // Lamdba to return an ApInt by value, but also provide the raw value of the
   // allocated data.
   auto getRValue = [](const char *HexString, uint64_t const *&RawData) {
      ApInt V(129, HexString, 16);
      RawData = V.getRawData();
      return V;
   };

   ApInt One(129, "1", 16);
   ApInt Two(129, "2", 16);
   ApInt Three(129, "3", 16);
   ApInt MinusOne = -One;

   const uint64_t *RawDataL = nullptr;
   const uint64_t *RawDataR = nullptr;

   {
      // 1 + 1 = 2
      ApInt AddLL = One + One;
      EXPECT_EQ(AddLL, Two);

      ApInt AddLR = One + getRValue("1", RawDataR);
      EXPECT_EQ(AddLR, Two);
      EXPECT_EQ(AddLR.getRawData(), RawDataR);

      ApInt AddRL = getRValue("1", RawDataL) + One;
      EXPECT_EQ(AddRL, Two);
      EXPECT_EQ(AddRL.getRawData(), RawDataL);

      ApInt AddRR = getRValue("1", RawDataL) + getRValue("1", RawDataR);
      EXPECT_EQ(AddRR, Two);
      EXPECT_EQ(AddRR.getRawData(), RawDataR);

      // LValue's and constants
      ApInt AddLK = One + 1;
      EXPECT_EQ(AddLK, Two);

      ApInt AddKL = 1 + One;
      EXPECT_EQ(AddKL, Two);

      // RValue's and constants
      ApInt AddRK = getRValue("1", RawDataL) + 1;
      EXPECT_EQ(AddRK, Two);
      EXPECT_EQ(AddRK.getRawData(), RawDataL);

      ApInt AddKR = 1 + getRValue("1", RawDataR);
      EXPECT_EQ(AddKR, Two);
      EXPECT_EQ(AddKR.getRawData(), RawDataR);
   }

   {
      // 0x0,FFFF...FFFF + 0x2 = 0x100...0001
      ApInt AllOnes(129, "0FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", 16);
      ApInt HighOneLowOne(129, "100000000000000000000000000000001", 16);

      ApInt AddLL = AllOnes + Two;
      EXPECT_EQ(AddLL, HighOneLowOne);

      ApInt AddLR = AllOnes + getRValue("2", RawDataR);
      EXPECT_EQ(AddLR, HighOneLowOne);
      EXPECT_EQ(AddLR.getRawData(), RawDataR);

      ApInt AddRL = getRValue("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", RawDataL) + Two;
      EXPECT_EQ(AddRL, HighOneLowOne);
      EXPECT_EQ(AddRL.getRawData(), RawDataL);

      ApInt AddRR = getRValue("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", RawDataL) +
            getRValue("2", RawDataR);
      EXPECT_EQ(AddRR, HighOneLowOne);
      EXPECT_EQ(AddRR.getRawData(), RawDataR);

      // LValue's and constants
      ApInt AddLK = AllOnes + 2;
      EXPECT_EQ(AddLK, HighOneLowOne);

      ApInt AddKL = 2 + AllOnes;
      EXPECT_EQ(AddKL, HighOneLowOne);

      // RValue's and constants
      ApInt AddRK = getRValue("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", RawDataL) + 2;
      EXPECT_EQ(AddRK, HighOneLowOne);
      EXPECT_EQ(AddRK.getRawData(), RawDataL);

      ApInt AddKR = 2 + getRValue("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", RawDataR);
      EXPECT_EQ(AddKR, HighOneLowOne);
      EXPECT_EQ(AddKR.getRawData(), RawDataR);
   }

   {
      // 2 - 1 = 1
      ApInt SubLL = Two - One;
      EXPECT_EQ(SubLL, One);

      ApInt SubLR = Two - getRValue("1", RawDataR);
      EXPECT_EQ(SubLR, One);
      EXPECT_EQ(SubLR.getRawData(), RawDataR);

      ApInt SubRL = getRValue("2", RawDataL) - One;
      EXPECT_EQ(SubRL, One);
      EXPECT_EQ(SubRL.getRawData(), RawDataL);

      ApInt SubRR = getRValue("2", RawDataL) - getRValue("1", RawDataR);
      EXPECT_EQ(SubRR, One);
      EXPECT_EQ(SubRR.getRawData(), RawDataR);

      // LValue's and constants
      ApInt SubLK = Two - 1;
      EXPECT_EQ(SubLK, One);

      ApInt SubKL = 2 - One;
      EXPECT_EQ(SubKL, One);

      // RValue's and constants
      ApInt SubRK = getRValue("2", RawDataL) - 1;
      EXPECT_EQ(SubRK, One);
      EXPECT_EQ(SubRK.getRawData(), RawDataL);

      ApInt SubKR = 2 - getRValue("1", RawDataR);
      EXPECT_EQ(SubKR, One);
      EXPECT_EQ(SubKR.getRawData(), RawDataR);
   }

   {
      // 0x100...0001 - 0x0,FFFF...FFFF = 0x2
      ApInt AllOnes(129, "0FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", 16);
      ApInt HighOneLowOne(129, "100000000000000000000000000000001", 16);

      ApInt SubLL = HighOneLowOne - AllOnes;
      EXPECT_EQ(SubLL, Two);

      ApInt SubLR = HighOneLowOne -
            getRValue("0FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", RawDataR);
      EXPECT_EQ(SubLR, Two);
      EXPECT_EQ(SubLR.getRawData(), RawDataR);

      ApInt SubRL = getRValue("100000000000000000000000000000001", RawDataL) -
            AllOnes;
      EXPECT_EQ(SubRL, Two);
      EXPECT_EQ(SubRL.getRawData(), RawDataL);

      ApInt SubRR = getRValue("100000000000000000000000000000001", RawDataL) -
            getRValue("0FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", RawDataR);
      EXPECT_EQ(SubRR, Two);
      EXPECT_EQ(SubRR.getRawData(), RawDataR);

      // LValue's and constants
      // 0x100...0001 - 0x2 = 0x0,FFFF...FFFF
      ApInt SubLK = HighOneLowOne - 2;
      EXPECT_EQ(SubLK, AllOnes);

      // 2 - (-1) = 3
      ApInt SubKL = 2 - MinusOne;
      EXPECT_EQ(SubKL, Three);

      // RValue's and constants
      // 0x100...0001 - 0x2 = 0x0,FFFF...FFFF
      ApInt SubRK = getRValue("100000000000000000000000000000001", RawDataL) - 2;
      EXPECT_EQ(SubRK, AllOnes);
      EXPECT_EQ(SubRK.getRawData(), RawDataL);

      ApInt SubKR = 2 - getRValue("1FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", RawDataR);
      EXPECT_EQ(SubKR, Three);
      EXPECT_EQ(SubKR.getRawData(), RawDataR);
   }
}

TEST(ApIntTest, testRvalueBitwise)
{
   // Test all combinations of lvalue/rvalue lhs/rhs of and/or/xor

   // Lamdba to return an ApInt by value, but also provide the raw value of the
   // allocated data.
   auto getRValue = [](const char *HexString, uint64_t const *&RawData) {
      ApInt V(129, HexString, 16);
      RawData = V.getRawData();
      return V;
   };

   ApInt Ten(129, "A", 16);
   ApInt Twelve(129, "C", 16);

   const uint64_t *RawDataL = nullptr;
   const uint64_t *RawDataR = nullptr;

   {
      // 12 & 10 = 8
      ApInt AndLL = Ten & Twelve;
      EXPECT_EQ(AndLL, 0x8);

      ApInt AndLR = Ten & getRValue("C", RawDataR);
      EXPECT_EQ(AndLR, 0x8);
      EXPECT_EQ(AndLR.getRawData(), RawDataR);

      ApInt AndRL = getRValue("A", RawDataL) & Twelve;
      EXPECT_EQ(AndRL, 0x8);
      EXPECT_EQ(AndRL.getRawData(), RawDataL);

      ApInt AndRR = getRValue("A", RawDataL) & getRValue("C", RawDataR);
      EXPECT_EQ(AndRR, 0x8);
      EXPECT_EQ(AndRR.getRawData(), RawDataR);

      // LValue's and constants
      ApInt AndLK = Ten & 0xc;
      EXPECT_EQ(AndLK, 0x8);

      ApInt AndKL = 0xa & Twelve;
      EXPECT_EQ(AndKL, 0x8);

      // RValue's and constants
      ApInt AndRK = getRValue("A", RawDataL) & 0xc;
      EXPECT_EQ(AndRK, 0x8);
      EXPECT_EQ(AndRK.getRawData(), RawDataL);

      ApInt AndKR = 0xa & getRValue("C", RawDataR);
      EXPECT_EQ(AndKR, 0x8);
      EXPECT_EQ(AndKR.getRawData(), RawDataR);
   }

   {
      // 12 | 10 = 14
      ApInt OrLL = Ten | Twelve;
      EXPECT_EQ(OrLL, 0xe);

      ApInt OrLR = Ten | getRValue("C", RawDataR);
      EXPECT_EQ(OrLR, 0xe);
      EXPECT_EQ(OrLR.getRawData(), RawDataR);

      ApInt OrRL = getRValue("A", RawDataL) | Twelve;
      EXPECT_EQ(OrRL, 0xe);
      EXPECT_EQ(OrRL.getRawData(), RawDataL);

      ApInt OrRR = getRValue("A", RawDataL) | getRValue("C", RawDataR);
      EXPECT_EQ(OrRR, 0xe);
      EXPECT_EQ(OrRR.getRawData(), RawDataR);

      // LValue's and constants
      ApInt OrLK = Ten | 0xc;
      EXPECT_EQ(OrLK, 0xe);

      ApInt OrKL = 0xa | Twelve;
      EXPECT_EQ(OrKL, 0xe);

      // RValue's and constants
      ApInt OrRK = getRValue("A", RawDataL) | 0xc;
      EXPECT_EQ(OrRK, 0xe);
      EXPECT_EQ(OrRK.getRawData(), RawDataL);

      ApInt OrKR = 0xa | getRValue("C", RawDataR);
      EXPECT_EQ(OrKR, 0xe);
      EXPECT_EQ(OrKR.getRawData(), RawDataR);
   }

   {
      // 12 ^ 10 = 6
      ApInt XorLL = Ten ^ Twelve;
      EXPECT_EQ(XorLL, 0x6);

      ApInt XorLR = Ten ^ getRValue("C", RawDataR);
      EXPECT_EQ(XorLR, 0x6);
      EXPECT_EQ(XorLR.getRawData(), RawDataR);

      ApInt XorRL = getRValue("A", RawDataL) ^ Twelve;
      EXPECT_EQ(XorRL, 0x6);
      EXPECT_EQ(XorRL.getRawData(), RawDataL);

      ApInt XorRR = getRValue("A", RawDataL) ^ getRValue("C", RawDataR);
      EXPECT_EQ(XorRR, 0x6);
      EXPECT_EQ(XorRR.getRawData(), RawDataR);

      // LValue's and constants
      ApInt XorLK = Ten ^ 0xc;
      EXPECT_EQ(XorLK, 0x6);

      ApInt XorKL = 0xa ^ Twelve;
      EXPECT_EQ(XorKL, 0x6);

      // RValue's and constants
      ApInt XorRK = getRValue("A", RawDataL) ^ 0xc;
      EXPECT_EQ(XorRK, 0x6);
      EXPECT_EQ(XorRK.getRawData(), RawDataL);

      ApInt XorKR = 0xa ^ getRValue("C", RawDataR);
      EXPECT_EQ(XorKR, 0x6);
      EXPECT_EQ(XorKR.getRawData(), RawDataR);
   }
}

TEST(ApIntTest, testRvalueInvert)
{
   // Lamdba to return an ApInt by value, but also provide the raw value of the
   // allocated data.
   auto getRValue = [](const char *HexString, uint64_t const *&RawData) {
      ApInt V(129, HexString, 16);
      RawData = V.getRawData();
      return V;
   };

   ApInt One(129, 1);
   ApInt NegativeTwo(129, -2ULL, true);

   const uint64_t *RawData = nullptr;

   {
      // ~1 = -2
      ApInt NegL = ~One;
      EXPECT_EQ(NegL, NegativeTwo);

      ApInt NegR = ~getRValue("1", RawData);
      EXPECT_EQ(NegR, NegativeTwo);
      EXPECT_EQ(NegR.getRawData(), RawData);
   }
}

// Tests different div/rem varaints using scheme (a * b + c) / a
void testDiv(ApInt a, ApInt b, ApInt c)
{
   ASSERT_TRUE(a.uge(b)); // Must: a >= b
   ASSERT_TRUE(a.ugt(c)); // Must: a > c

   auto p = a * b + c;

   auto q = p.udiv(a);
   auto r = p.urem(a);
   EXPECT_EQ(b, q);
   EXPECT_EQ(c, r);
   ApInt::udivrem(p, a, q, r);
   EXPECT_EQ(b, q);
   EXPECT_EQ(c, r);
   q = p.sdiv(a);
   r = p.srem(a);
   EXPECT_EQ(b, q);
   EXPECT_EQ(c, r);
   ApInt::sdivrem(p, a, q, r);
   EXPECT_EQ(b, q);
   EXPECT_EQ(c, r);

   if (b.ugt(c)) { // Test also symmetric case
      q = p.udiv(b);
      r = p.urem(b);
      EXPECT_EQ(a, q);
      EXPECT_EQ(c, r);
      ApInt::udivrem(p, b, q, r);
      EXPECT_EQ(a, q);
      EXPECT_EQ(c, r);
      q = p.sdiv(b);
      r = p.srem(b);
      EXPECT_EQ(a, q);
      EXPECT_EQ(c, r);
      ApInt::sdivrem(p, b, q, r);
      EXPECT_EQ(a, q);
      EXPECT_EQ(c, r);
   }
}

TEST(ApIntTest, testDivremBig1)
{
   // Tests KnuthDiv rare step D6
   testDiv({256, "1ffffffffffffffff", 16},
   {256, "1ffffffffffffffff", 16},
   {256, 0});
}

TEST(ApIntTest, testDivremBig2)
{
   // Tests KnuthDiv rare step D6
   testDiv({1024,                       "112233ceff"
            "cecece000000ffffffffffffffffffff"
            "ffffffffffffffffffffffffffffffff"
            "ffffffffffffffffffffffffffffffff"
            "ffffffffffffffffffffffffffffff33", 16},
   {1024,           "111111ffffffffffffffff"
    "ffffffffffffffffffffffffffffffff"
    "fffffffffffffffffffffffffffffccf"
    "ffffffffffffffffffffffffffffff00", 16},
   {1024, 7919});
}

TEST(ApIntTest, testDivremBig3)
{
   // Tests KnuthDiv case without shift
   testDiv({256, "80000001ffffffffffffffff", 16},
   {256, "ffffffffffffff0000000", 16},
   {256, 4219});
}

TEST(ApIntTest, testDivremBig4)
{
   // Tests heap allocation in divide() enfoced by huge numbers
   testDiv(ApInt{4096, 5}.shl(2001),
           ApInt{4096, 1}.shl(2000),
           ApInt{4096, 4219*13});
}

TEST(ApIntTest, testDivremBig5)
{
   // Tests one word divisor case of divide()
   testDiv(ApInt{1024, 19}.shl(811),
           ApInt{1024, 4356013}, // one word
           ApInt{1024, 1});
}

TEST(ApIntTest, testDivremBig6)
{
   // Tests some rare "borrow" cases in D4 step
   testDiv(ApInt{512, "ffffffffffffffff00000000000000000000000001", 16},
           ApInt{512, "10000000000000001000000000000001", 16},
           ApInt{512, "10000000000000000000000000000000", 16});
}

TEST(ApIntTest, testDivremBig7)
{
   // Yet another test for KnuthDiv rare step D6.
   testDiv({224, "800000008000000200000005", 16},
   {224, "fffffffd", 16},
   {224, "80000000800000010000000f", 16});
}

void testDiv(ApInt a, uint64_t b, ApInt c)
{
   auto p = a * b + c;

   ApInt q;
   uint64_t r;
   // Unsigned division will only work if our original number wasn't negative.
   if (!a.isNegative()) {
      q = p.udiv(b);
      r = p.urem(b);
      EXPECT_EQ(a, q);
      EXPECT_EQ(c, r);
      ApInt::udivrem(p, b, q, r);
      EXPECT_EQ(a, q);
      EXPECT_EQ(c, r);
   }
   q = p.sdiv(b);
   r = p.srem(b);
   EXPECT_EQ(a, q);
   if (c.isNegative())
      EXPECT_EQ(-c, -r); // Need to negate so the uint64_t compare will work.
   else
      EXPECT_EQ(c, r);
   int64_t sr;
   ApInt::sdivrem(p, b, q, sr);
   EXPECT_EQ(a, q);
   if (c.isNegative())
      EXPECT_EQ(-c, -sr); // Need to negate so the uint64_t compare will work.
   else
      EXPECT_EQ(c, sr);
}

TEST(ApIntTest, testDivremuint)
{
   // Single word ApInt
   testDiv(ApInt{64, 9},
           2,
           ApInt{64, 1});

   // Single word negative ApInt
   testDiv(-ApInt{64, 9},
           2,
           -ApInt{64, 1});

   // Multiword dividend with only one significant word.
   testDiv(ApInt{256, 9},
           2,
           ApInt{256, 1});

   // Negative dividend.
   testDiv(-ApInt{256, 9},
           2,
           -ApInt{256, 1});

   // Multiword dividend
   testDiv(ApInt{1024, 19}.shl(811),
           4356013, // one word
           ApInt{1024, 1});
}

TEST(ApIntTest, testDivremSimple)
{
   // Test simple cases.
   ApInt A(65, 2), B(65, 2);
   ApInt Q, R;

   // X / X
   ApInt::sdivrem(A, B, Q, R);
   EXPECT_EQ(Q, ApInt(65, 1));
   EXPECT_EQ(R, ApInt(65, 0));
   ApInt::udivrem(A, B, Q, R);
   EXPECT_EQ(Q, ApInt(65, 1));
   EXPECT_EQ(R, ApInt(65, 0));

   // 0 / X
   ApInt O(65, 0);
   ApInt::sdivrem(O, B, Q, R);
   EXPECT_EQ(Q, ApInt(65, 0));
   EXPECT_EQ(R, ApInt(65, 0));
   ApInt::udivrem(O, B, Q, R);
   EXPECT_EQ(Q, ApInt(65, 0));
   EXPECT_EQ(R, ApInt(65, 0));

   // X / 1
   ApInt I(65, 1);
   ApInt::sdivrem(A, I, Q, R);
   EXPECT_EQ(Q, A);
   EXPECT_EQ(R, ApInt(65, 0));
   ApInt::udivrem(A, I, Q, R);
   EXPECT_EQ(Q, A);
   EXPECT_EQ(R, ApInt(65, 0));
}

TEST(ApIntTest, testFromString)
{
   EXPECT_EQ(ApInt(32, 0), ApInt(32,   "0", 2));
   EXPECT_EQ(ApInt(32, 1), ApInt(32,   "1", 2));
   EXPECT_EQ(ApInt(32, 2), ApInt(32,  "10", 2));
   EXPECT_EQ(ApInt(32, 3), ApInt(32,  "11", 2));
   EXPECT_EQ(ApInt(32, 4), ApInt(32, "100", 2));

   EXPECT_EQ(ApInt(32, 0), ApInt(32,   "+0", 2));
   EXPECT_EQ(ApInt(32, 1), ApInt(32,   "+1", 2));
   EXPECT_EQ(ApInt(32, 2), ApInt(32,  "+10", 2));
   EXPECT_EQ(ApInt(32, 3), ApInt(32,  "+11", 2));
   EXPECT_EQ(ApInt(32, 4), ApInt(32, "+100", 2));

   EXPECT_EQ(ApInt(32, uint64_t(-0LL)), ApInt(32,   "-0", 2));
   EXPECT_EQ(ApInt(32, uint64_t(-1LL)), ApInt(32,   "-1", 2));
   EXPECT_EQ(ApInt(32, uint64_t(-2LL)), ApInt(32,  "-10", 2));
   EXPECT_EQ(ApInt(32, uint64_t(-3LL)), ApInt(32,  "-11", 2));
   EXPECT_EQ(ApInt(32, uint64_t(-4LL)), ApInt(32, "-100", 2));

   EXPECT_EQ(ApInt(32,  0), ApInt(32,  "0",  8));
   EXPECT_EQ(ApInt(32,  1), ApInt(32,  "1",  8));
   EXPECT_EQ(ApInt(32,  7), ApInt(32,  "7",  8));
   EXPECT_EQ(ApInt(32,  8), ApInt(32,  "10", 8));
   EXPECT_EQ(ApInt(32, 15), ApInt(32,  "17", 8));
   EXPECT_EQ(ApInt(32, 16), ApInt(32,  "20", 8));

   EXPECT_EQ(ApInt(32,  +0), ApInt(32,  "+0",  8));
   EXPECT_EQ(ApInt(32,  +1), ApInt(32,  "+1",  8));
   EXPECT_EQ(ApInt(32,  +7), ApInt(32,  "+7",  8));
   EXPECT_EQ(ApInt(32,  +8), ApInt(32,  "+10", 8));
   EXPECT_EQ(ApInt(32, +15), ApInt(32,  "+17", 8));
   EXPECT_EQ(ApInt(32, +16), ApInt(32,  "+20", 8));

   EXPECT_EQ(ApInt(32,  uint64_t(-0LL)), ApInt(32,  "-0",  8));
   EXPECT_EQ(ApInt(32,  uint64_t(-1LL)), ApInt(32,  "-1",  8));
   EXPECT_EQ(ApInt(32,  uint64_t(-7LL)), ApInt(32,  "-7",  8));
   EXPECT_EQ(ApInt(32,  uint64_t(-8LL)), ApInt(32,  "-10", 8));
   EXPECT_EQ(ApInt(32, uint64_t(-15LL)), ApInt(32,  "-17", 8));
   EXPECT_EQ(ApInt(32, uint64_t(-16LL)), ApInt(32,  "-20", 8));

   EXPECT_EQ(ApInt(32,  0), ApInt(32,  "0", 10));
   EXPECT_EQ(ApInt(32,  1), ApInt(32,  "1", 10));
   EXPECT_EQ(ApInt(32,  9), ApInt(32,  "9", 10));
   EXPECT_EQ(ApInt(32, 10), ApInt(32, "10", 10));
   EXPECT_EQ(ApInt(32, 19), ApInt(32, "19", 10));
   EXPECT_EQ(ApInt(32, 20), ApInt(32, "20", 10));

   EXPECT_EQ(ApInt(32,  uint64_t(-0LL)), ApInt(32,  "-0", 10));
   EXPECT_EQ(ApInt(32,  uint64_t(-1LL)), ApInt(32,  "-1", 10));
   EXPECT_EQ(ApInt(32,  uint64_t(-9LL)), ApInt(32,  "-9", 10));
   EXPECT_EQ(ApInt(32, uint64_t(-10LL)), ApInt(32, "-10", 10));
   EXPECT_EQ(ApInt(32, uint64_t(-19LL)), ApInt(32, "-19", 10));
   EXPECT_EQ(ApInt(32, uint64_t(-20LL)), ApInt(32, "-20", 10));

   EXPECT_EQ(ApInt(32,  0), ApInt(32,  "0", 16));
   EXPECT_EQ(ApInt(32,  1), ApInt(32,  "1", 16));
   EXPECT_EQ(ApInt(32, 15), ApInt(32,  "F", 16));
   EXPECT_EQ(ApInt(32, 16), ApInt(32, "10", 16));
   EXPECT_EQ(ApInt(32, 31), ApInt(32, "1F", 16));
   EXPECT_EQ(ApInt(32, 32), ApInt(32, "20", 16));

   EXPECT_EQ(ApInt(32,  uint64_t(-0LL)), ApInt(32,  "-0", 16));
   EXPECT_EQ(ApInt(32,  uint64_t(-1LL)), ApInt(32,  "-1", 16));
   EXPECT_EQ(ApInt(32, uint64_t(-15LL)), ApInt(32,  "-F", 16));
   EXPECT_EQ(ApInt(32, uint64_t(-16LL)), ApInt(32, "-10", 16));
   EXPECT_EQ(ApInt(32, uint64_t(-31LL)), ApInt(32, "-1F", 16));
   EXPECT_EQ(ApInt(32, uint64_t(-32LL)), ApInt(32, "-20", 16));

   EXPECT_EQ(ApInt(32,  0), ApInt(32,  "0", 36));
   EXPECT_EQ(ApInt(32,  1), ApInt(32,  "1", 36));
   EXPECT_EQ(ApInt(32, 35), ApInt(32,  "Z", 36));
   EXPECT_EQ(ApInt(32, 36), ApInt(32, "10", 36));
   EXPECT_EQ(ApInt(32, 71), ApInt(32, "1Z", 36));
   EXPECT_EQ(ApInt(32, 72), ApInt(32, "20", 36));

   EXPECT_EQ(ApInt(32,  uint64_t(-0LL)), ApInt(32,  "-0", 36));
   EXPECT_EQ(ApInt(32,  uint64_t(-1LL)), ApInt(32,  "-1", 36));
   EXPECT_EQ(ApInt(32, uint64_t(-35LL)), ApInt(32,  "-Z", 36));
   EXPECT_EQ(ApInt(32, uint64_t(-36LL)), ApInt(32, "-10", 36));
   EXPECT_EQ(ApInt(32, uint64_t(-71LL)), ApInt(32, "-1Z", 36));
   EXPECT_EQ(ApInt(32, uint64_t(-72LL)), ApInt(32, "-20", 36));
}


TEST(ApIntTest, testSaturatingMath) {
   ApInt AP_10 = ApInt(8, 10);
   ApInt AP_100 = ApInt(8, 100);
   ApInt AP_200 = ApInt(8, 200);

   EXPECT_EQ(ApInt(8, 200), AP_100.uaddSaturate(AP_100));
   EXPECT_EQ(ApInt(8, 255), AP_100.uaddSaturate(AP_200));
   EXPECT_EQ(ApInt(8, 255), ApInt(8, 255).uaddSaturate(ApInt(8, 255)));

   EXPECT_EQ(ApInt(8, 110), AP_10.saddSaturate(AP_100));
   EXPECT_EQ(ApInt(8, 127), AP_100.saddSaturate(AP_100));
   EXPECT_EQ(ApInt(8, -128), (-AP_100).saddSaturate(-AP_100));
   EXPECT_EQ(ApInt(8, -128), ApInt(8, -128).saddSaturate(ApInt(8, -128)));

   EXPECT_EQ(ApInt(8, 90), AP_100.usubSaturate(AP_10));
   EXPECT_EQ(ApInt(8, 0), AP_100.usubSaturate(AP_200));
   EXPECT_EQ(ApInt(8, 0), ApInt(8, 0).usubSaturate(ApInt(8, 255)));

   EXPECT_EQ(ApInt(8, -90), AP_10.ssubSaturate(AP_100));
   EXPECT_EQ(ApInt(8, 127), AP_100.ssubSaturate(-AP_100));
   EXPECT_EQ(ApInt(8, -128), (-AP_100).ssubSaturate(AP_100));
   EXPECT_EQ(ApInt(8, -128), ApInt(8, -128).ssubSaturate(ApInt(8, 127)));
}

TEST(ApIntTest, testFromArray)
{
   EXPECT_EQ(ApInt(32, uint64_t(1)), ApInt(32, ArrayRef<uint64_t>(1)));
}

TEST(ApIntTest, testStringBitsNeeded2)
{
   EXPECT_EQ(1U, ApInt::getBitsNeeded(  "0", 2));
   EXPECT_EQ(1U, ApInt::getBitsNeeded(  "1", 2));
   EXPECT_EQ(2U, ApInt::getBitsNeeded( "10", 2));
   EXPECT_EQ(2U, ApInt::getBitsNeeded( "11", 2));
   EXPECT_EQ(3U, ApInt::getBitsNeeded("100", 2));

   EXPECT_EQ(1U, ApInt::getBitsNeeded(  "+0", 2));
   EXPECT_EQ(1U, ApInt::getBitsNeeded(  "+1", 2));
   EXPECT_EQ(2U, ApInt::getBitsNeeded( "+10", 2));
   EXPECT_EQ(2U, ApInt::getBitsNeeded( "+11", 2));
   EXPECT_EQ(3U, ApInt::getBitsNeeded("+100", 2));

   EXPECT_EQ(2U, ApInt::getBitsNeeded(  "-0", 2));
   EXPECT_EQ(2U, ApInt::getBitsNeeded(  "-1", 2));
   EXPECT_EQ(3U, ApInt::getBitsNeeded( "-10", 2));
   EXPECT_EQ(3U, ApInt::getBitsNeeded( "-11", 2));
   EXPECT_EQ(4U, ApInt::getBitsNeeded("-100", 2));
}

TEST(ApIntTest, testStringBitsNeeded8)
{
   EXPECT_EQ(3U, ApInt::getBitsNeeded( "0", 8));
   EXPECT_EQ(3U, ApInt::getBitsNeeded( "7", 8));
   EXPECT_EQ(6U, ApInt::getBitsNeeded("10", 8));
   EXPECT_EQ(6U, ApInt::getBitsNeeded("17", 8));
   EXPECT_EQ(6U, ApInt::getBitsNeeded("20", 8));

   EXPECT_EQ(3U, ApInt::getBitsNeeded( "+0", 8));
   EXPECT_EQ(3U, ApInt::getBitsNeeded( "+7", 8));
   EXPECT_EQ(6U, ApInt::getBitsNeeded("+10", 8));
   EXPECT_EQ(6U, ApInt::getBitsNeeded("+17", 8));
   EXPECT_EQ(6U, ApInt::getBitsNeeded("+20", 8));

   EXPECT_EQ(4U, ApInt::getBitsNeeded( "-0", 8));
   EXPECT_EQ(4U, ApInt::getBitsNeeded( "-7", 8));
   EXPECT_EQ(7U, ApInt::getBitsNeeded("-10", 8));
   EXPECT_EQ(7U, ApInt::getBitsNeeded("-17", 8));
   EXPECT_EQ(7U, ApInt::getBitsNeeded("-20", 8));
}

TEST(ApIntTest, testStringBitsNeeded10)
{
   EXPECT_EQ(1U, ApInt::getBitsNeeded( "0", 10));
   EXPECT_EQ(2U, ApInt::getBitsNeeded( "3", 10));
   EXPECT_EQ(4U, ApInt::getBitsNeeded( "9", 10));
   EXPECT_EQ(4U, ApInt::getBitsNeeded("10", 10));
   EXPECT_EQ(5U, ApInt::getBitsNeeded("19", 10));
   EXPECT_EQ(5U, ApInt::getBitsNeeded("20", 10));

   EXPECT_EQ(1U, ApInt::getBitsNeeded( "+0", 10));
   EXPECT_EQ(4U, ApInt::getBitsNeeded( "+9", 10));
   EXPECT_EQ(4U, ApInt::getBitsNeeded("+10", 10));
   EXPECT_EQ(5U, ApInt::getBitsNeeded("+19", 10));
   EXPECT_EQ(5U, ApInt::getBitsNeeded("+20", 10));

   EXPECT_EQ(2U, ApInt::getBitsNeeded( "-0", 10));
   EXPECT_EQ(5U, ApInt::getBitsNeeded( "-9", 10));
   EXPECT_EQ(5U, ApInt::getBitsNeeded("-10", 10));
   EXPECT_EQ(6U, ApInt::getBitsNeeded("-19", 10));
   EXPECT_EQ(6U, ApInt::getBitsNeeded("-20", 10));
}

TEST(ApIntTest, testStringBitsNeeded16)
{
   EXPECT_EQ(4U, ApInt::getBitsNeeded( "0", 16));
   EXPECT_EQ(4U, ApInt::getBitsNeeded( "F", 16));
   EXPECT_EQ(8U, ApInt::getBitsNeeded("10", 16));
   EXPECT_EQ(8U, ApInt::getBitsNeeded("1F", 16));
   EXPECT_EQ(8U, ApInt::getBitsNeeded("20", 16));

   EXPECT_EQ(4U, ApInt::getBitsNeeded( "+0", 16));
   EXPECT_EQ(4U, ApInt::getBitsNeeded( "+F", 16));
   EXPECT_EQ(8U, ApInt::getBitsNeeded("+10", 16));
   EXPECT_EQ(8U, ApInt::getBitsNeeded("+1F", 16));
   EXPECT_EQ(8U, ApInt::getBitsNeeded("+20", 16));

   EXPECT_EQ(5U, ApInt::getBitsNeeded( "-0", 16));
   EXPECT_EQ(5U, ApInt::getBitsNeeded( "-F", 16));
   EXPECT_EQ(9U, ApInt::getBitsNeeded("-10", 16));
   EXPECT_EQ(9U, ApInt::getBitsNeeded("-1F", 16));
   EXPECT_EQ(9U, ApInt::getBitsNeeded("-20", 16));
}

TEST(ApIntTest, testToString)
{
   SmallString<16> str;
   bool isSigned;

   ApInt(8, 0).toString(str, 2, true, true);
   EXPECT_EQ(str.getStr().getStr(), "0b0");
   str.clear();
   ApInt(8, 0).toString(str, 8, true, true);
   EXPECT_EQ(str.getStr().getStr(), "00");
   str.clear();
   ApInt(8, 0).toString(str, 10, true, true);
   EXPECT_EQ(str.getStr().getStr(), "0");
   str.clear();
   ApInt(8, 0).toString(str, 16, true, true);
   EXPECT_EQ(str.getStr().getStr(), "0x0");
   str.clear();
   ApInt(8, 0).toString(str, 36, true, false);
   EXPECT_EQ(str.getStr().getStr(), "0");
   str.clear();

   isSigned = false;
   ApInt(8, 255, isSigned).toString(str, 2, isSigned, true);
   EXPECT_EQ(str.getStr().getStr(), "0b11111111");
   str.clear();
   ApInt(8, 255, isSigned).toString(str, 8, isSigned, true);
   EXPECT_EQ(str.getStr().getStr(), "0377");
   str.clear();
   ApInt(8, 255, isSigned).toString(str, 10, isSigned, true);
   EXPECT_EQ(str.getStr().getStr(), "255");
   str.clear();
   ApInt(8, 255, isSigned).toString(str, 16, isSigned, true);
   EXPECT_EQ(str.getStr().getStr(), "0xFF");
   str.clear();
   ApInt(8, 255, isSigned).toString(str, 36, isSigned, false);
   EXPECT_EQ(str.getStr().getStr(), "73");
   str.clear();

   isSigned = true;
   ApInt(8, 255, isSigned).toString(str, 2, isSigned, true);
   EXPECT_EQ(str.getStr().getStr(), "-0b1");
   str.clear();
   ApInt(8, 255, isSigned).toString(str, 8, isSigned, true);
   EXPECT_EQ(str.getStr().getStr(), "-01");
   str.clear();
   ApInt(8, 255, isSigned).toString(str, 10, isSigned, true);
   EXPECT_EQ(str.getStr().getStr(), "-1");
   str.clear();
   ApInt(8, 255, isSigned).toString(str, 16, isSigned, true);
   EXPECT_EQ(str.getStr().getStr(), "-0x1");
   str.clear();
   ApInt(8, 255, isSigned).toString(str, 36, isSigned, false);
   EXPECT_EQ(str.getStr().getStr(), "-1");
   str.clear();
}

TEST(ApIntTest, testLog2)
{
   EXPECT_EQ(ApInt(15, 7).logBase2(), 2U);
   EXPECT_EQ(ApInt(15, 7).ceilLogBase2(), 3U);
   EXPECT_EQ(ApInt(15, 7).exactLogBase2(), -1);
   EXPECT_EQ(ApInt(15, 8).logBase2(), 3U);
   EXPECT_EQ(ApInt(15, 8).ceilLogBase2(), 3U);
   EXPECT_EQ(ApInt(15, 8).exactLogBase2(), 3);
   EXPECT_EQ(ApInt(15, 9).logBase2(), 3U);
   EXPECT_EQ(ApInt(15, 9).ceilLogBase2(), 4U);
   EXPECT_EQ(ApInt(15, 9).exactLogBase2(), -1);
}

TEST(ApIntTest, testMagic)
{
   EXPECT_EQ(ApInt(32, 3).getMagic().m_magic, ApInt(32, "55555556", 16));
   EXPECT_EQ(ApInt(32, 3).getMagic().m_shift, 0U);
   EXPECT_EQ(ApInt(32, 5).getMagic().m_magic, ApInt(32, "66666667", 16));
   EXPECT_EQ(ApInt(32, 5).getMagic().m_shift, 1U);
   EXPECT_EQ(ApInt(32, 7).getMagic().m_magic, ApInt(32, "92492493", 16));
   EXPECT_EQ(ApInt(32, 7).getMagic().m_shift, 2U);
}

TEST(ApIntTest, testMagicu)
{
   EXPECT_EQ(ApInt(32, 3).getMagicUnsign().m_magic, ApInt(32, "AAAAAAAB", 16));
   EXPECT_EQ(ApInt(32, 3).getMagicUnsign().m_shift, 1U);
   EXPECT_EQ(ApInt(32, 5).getMagicUnsign().m_magic, ApInt(32, "CCCCCCCD", 16));
   EXPECT_EQ(ApInt(32, 5).getMagicUnsign().m_shift, 2U);
   EXPECT_EQ(ApInt(32, 7).getMagicUnsign().m_magic, ApInt(32, "24924925", 16));
   EXPECT_EQ(ApInt(32, 7).getMagicUnsign().m_shift, 3U);
   EXPECT_EQ(ApInt(64, 25).getMagicUnsign(1).m_magic, ApInt(64, "A3D70A3D70A3D70B", 16));
   EXPECT_EQ(ApInt(64, 25).getMagicUnsign(1).m_shift, 4U);
}

#ifdef GTEST_HAS_DEATH_TEST
#ifndef NDEBUG
TEST(ApIntTest, testStringDeath)
{
   EXPECT_DEATH(ApInt(0, "", 0), "Bitwidth too small");
   EXPECT_DEATH(ApInt(32, "", 0), "Invalid string length");
   EXPECT_DEATH(ApInt(32, "0", 0), "Radix should be 2, 8, 10, 16, or 36!");
   EXPECT_DEATH(ApInt(32, "", 10), "Invalid string length");
   EXPECT_DEATH(ApInt(32, "-", 10), "String is only a sign, needs a value.");
   EXPECT_DEATH(ApInt(1, "1234", 10), "Insufficient bit width");
   EXPECT_DEATH(ApInt(32, "\0", 10), "Invalid string length");
   EXPECT_DEATH(ApInt(32, StringRef("1\02", 3), 10), "Invalid character in digit string");
   EXPECT_DEATH(ApInt(32, "1L", 10), "Invalid character in digit string");
}
#endif
#endif

TEST(ApIntTest, testMulClear)
{
   ApInt ValA(65, -1ULL);
   ApInt ValB(65, 4);
   ApInt ValC(65, 0);
   ValC = ValA * ValB;
   ValA *= ValB;
   EXPECT_EQ(ValA.toString(10, false), ValC.toString(10, false));
}

TEST(ApIntTest, testRotate)
{
   EXPECT_EQ(ApInt(8, 1),  ApInt(8, 1).rotl(0));
   EXPECT_EQ(ApInt(8, 2),  ApInt(8, 1).rotl(1));
   EXPECT_EQ(ApInt(8, 4),  ApInt(8, 1).rotl(2));
   EXPECT_EQ(ApInt(8, 16), ApInt(8, 1).rotl(4));
   EXPECT_EQ(ApInt(8, 1),  ApInt(8, 1).rotl(8));

   EXPECT_EQ(ApInt(8, 16), ApInt(8, 16).rotl(0));
   EXPECT_EQ(ApInt(8, 32), ApInt(8, 16).rotl(1));
   EXPECT_EQ(ApInt(8, 64), ApInt(8, 16).rotl(2));
   EXPECT_EQ(ApInt(8, 1),  ApInt(8, 16).rotl(4));
   EXPECT_EQ(ApInt(8, 16), ApInt(8, 16).rotl(8));

   EXPECT_EQ(ApInt(32, 2), ApInt(32, 1).rotl(33));
   EXPECT_EQ(ApInt(32, 2), ApInt(32, 1).rotl(ApInt(32, 33)));

   EXPECT_EQ(ApInt(32, 2), ApInt(32, 1).rotl(33));
   EXPECT_EQ(ApInt(32, 2), ApInt(32, 1).rotl(ApInt(32, 33)));
   EXPECT_EQ(ApInt(32, 2), ApInt(32, 1).rotl(ApInt(33, 33)));
   EXPECT_EQ(ApInt(32, (1 << 8)), ApInt(32, 1).rotl(ApInt(32, 40)));
   EXPECT_EQ(ApInt(32, (1 << 30)), ApInt(32, 1).rotl(ApInt(31, 30)));
   EXPECT_EQ(ApInt(32, (1 << 31)), ApInt(32, 1).rotl(ApInt(31, 31)));

   EXPECT_EQ(ApInt(32, 1), ApInt(32, 1).rotl(ApInt(1, 0)));
   EXPECT_EQ(ApInt(32, 2), ApInt(32, 1).rotl(ApInt(1, 1)));

   EXPECT_EQ(ApInt(32, 16), ApInt(32, 1).rotl(ApInt(3, 4)));

   EXPECT_EQ(ApInt(32, 1), ApInt(32, 1).rotl(ApInt(64, 64)));
   EXPECT_EQ(ApInt(32, 2), ApInt(32, 1).rotl(ApInt(64, 65)));

   EXPECT_EQ(ApInt(7, 24), ApInt(7, 3).rotl(ApInt(7, 3)));
   EXPECT_EQ(ApInt(7, 24), ApInt(7, 3).rotl(ApInt(7, 10)));
   EXPECT_EQ(ApInt(7, 24), ApInt(7, 3).rotl(ApInt(5, 10)));
   EXPECT_EQ(ApInt(7, 6), ApInt(7, 3).rotl(ApInt(12, 120)));

   EXPECT_EQ(ApInt(8, 16), ApInt(8, 16).rotr(0));
   EXPECT_EQ(ApInt(8, 8),  ApInt(8, 16).rotr(1));
   EXPECT_EQ(ApInt(8, 4),  ApInt(8, 16).rotr(2));
   EXPECT_EQ(ApInt(8, 1),  ApInt(8, 16).rotr(4));
   EXPECT_EQ(ApInt(8, 16), ApInt(8, 16).rotr(8));

   EXPECT_EQ(ApInt(8, 1),   ApInt(8, 1).rotr(0));
   EXPECT_EQ(ApInt(8, 128), ApInt(8, 1).rotr(1));
   EXPECT_EQ(ApInt(8, 64),  ApInt(8, 1).rotr(2));
   EXPECT_EQ(ApInt(8, 16),  ApInt(8, 1).rotr(4));
   EXPECT_EQ(ApInt(8, 1),   ApInt(8, 1).rotr(8));

   EXPECT_EQ(ApInt(32, (1 << 31)), ApInt(32, 1).rotr(33));
   EXPECT_EQ(ApInt(32, (1 << 31)), ApInt(32, 1).rotr(ApInt(32, 33)));

   EXPECT_EQ(ApInt(32, (1 << 31)), ApInt(32, 1).rotr(33));
   EXPECT_EQ(ApInt(32, (1 << 31)), ApInt(32, 1).rotr(ApInt(32, 33)));
   EXPECT_EQ(ApInt(32, (1 << 31)), ApInt(32, 1).rotr(ApInt(33, 33)));
   EXPECT_EQ(ApInt(32, (1 << 24)), ApInt(32, 1).rotr(ApInt(32, 40)));

   EXPECT_EQ(ApInt(32, (1 << 2)), ApInt(32, 1).rotr(ApInt(31, 30)));
   EXPECT_EQ(ApInt(32, (1 << 1)), ApInt(32, 1).rotr(ApInt(31, 31)));

   EXPECT_EQ(ApInt(32, 1), ApInt(32, 1).rotr(ApInt(1, 0)));
   EXPECT_EQ(ApInt(32, (1 << 31)), ApInt(32, 1).rotr(ApInt(1, 1)));

   EXPECT_EQ(ApInt(32, (1 << 28)), ApInt(32, 1).rotr(ApInt(3, 4)));

   EXPECT_EQ(ApInt(32, 1), ApInt(32, 1).rotr(ApInt(64, 64)));
   EXPECT_EQ(ApInt(32, (1 << 31)), ApInt(32, 1).rotr(ApInt(64, 65)));

   EXPECT_EQ(ApInt(7, 48), ApInt(7, 3).rotr(ApInt(7, 3)));
   EXPECT_EQ(ApInt(7, 48), ApInt(7, 3).rotr(ApInt(7, 10)));
   EXPECT_EQ(ApInt(7, 48), ApInt(7, 3).rotr(ApInt(5, 10)));
   EXPECT_EQ(ApInt(7, 65), ApInt(7, 3).rotr(ApInt(12, 120)));

   ApInt Big(256, "00004000800000000000000000003fff8000000000000003", 16);
   ApInt Rot(256, "3fff80000000000000030000000000000000000040008000", 16);
   EXPECT_EQ(Rot, Big.rotr(144));

   EXPECT_EQ(ApInt(32, 8), ApInt(32, 1).rotl(Big));
   EXPECT_EQ(ApInt(32, (1 << 29)), ApInt(32, 1).rotr(Big));
}

TEST(ApIntTest, testSplat)
{
   ApInt ValA(8, 0x01);
   EXPECT_EQ(ValA, ApInt::getSplat(8, ValA));
   EXPECT_EQ(ApInt(64, 0x0101010101010101ULL), ApInt::getSplat(64, ValA));

   ApInt ValB(3, 5);
   EXPECT_EQ(ApInt(4, 0xD), ApInt::getSplat(4, ValB));
   EXPECT_EQ(ApInt(15, 0xDB6D), ApInt::getSplat(15, ValB));
}

TEST(ApIntTest, testTcDecrement)
{
   // Test single word decrement.

   // No out borrow.
   {
      ApInt::WordType singleWord = ~ApInt::WordType(0) << (ApInt::APINT_BITS_PER_WORD - 1);
      ApInt::WordType carry = ApInt::tcDecrement(&singleWord, 1);
      EXPECT_EQ(carry, ApInt::WordType(0));
      EXPECT_EQ(singleWord, ~ApInt::WordType(0) >> 1);
   }

   // With out borrow.
   {
      ApInt::WordType singleWord = 0;
      ApInt::WordType carry = ApInt::tcDecrement(&singleWord, 1);
      EXPECT_EQ(carry, ApInt::WordType(1));
      EXPECT_EQ(singleWord, ~ApInt::WordType(0));
   }

   // Test multiword decrement.

   // No across word borrow, no out borrow.
   {
      ApInt::WordType test[4] = {0x1, 0x1, 0x1, 0x1};
      ApInt::WordType expected[4] = {0x0, 0x1, 0x1, 0x1};
      ApInt::tcDecrement(test, 4);
      EXPECT_EQ(ApInt::tcCompare(test, expected, 4), 0);
   }

   // 1 across word borrow, no out borrow.
   {
      ApInt::WordType test[4] = {0x0, 0xF, 0x1, 0x1};
      ApInt::WordType expected[4] = {~ApInt::WordType(0), 0xE, 0x1, 0x1};
      ApInt::WordType carry = ApInt::tcDecrement(test, 4);
      EXPECT_EQ(carry, ApInt::WordType(0));
      EXPECT_EQ(ApInt::tcCompare(test, expected, 4), 0);
   }

   // 2 across word borrow, no out borrow.
   {
      ApInt::WordType test[4] = {0x0, 0x0, 0xC, 0x1};
      ApInt::WordType expected[4] = {~ApInt::WordType(0), ~ApInt::WordType(0), 0xB, 0x1};
      ApInt::WordType carry = ApInt::tcDecrement(test, 4);
      EXPECT_EQ(carry, ApInt::WordType(0));
      EXPECT_EQ(ApInt::tcCompare(test, expected, 4), 0);
   }

   // 3 across word borrow, no out borrow.
   {
      ApInt::WordType test[4] = {0x0, 0x0, 0x0, 0x1};
      ApInt::WordType expected[4] = {~ApInt::WordType(0), ~ApInt::WordType(0), ~ApInt::WordType(0), 0x0};
      ApInt::WordType carry = ApInt::tcDecrement(test, 4);
      EXPECT_EQ(carry, ApInt::WordType(0));
      EXPECT_EQ(ApInt::tcCompare(test, expected, 4), 0);
   }

   // 3 across word borrow, with out borrow.
   {
      ApInt::WordType test[4] = {0x0, 0x0, 0x0, 0x0};
      ApInt::WordType expected[4] = {~ApInt::WordType(0), ~ApInt::WordType(0), ~ApInt::WordType(0), ~ApInt::WordType(0)};
      ApInt::WordType carry = ApInt::tcDecrement(test, 4);
      EXPECT_EQ(carry, ApInt::WordType(1));
      EXPECT_EQ(ApInt::tcCompare(test, expected, 4), 0);
   }
}

TEST(ApIntTest, testArrayAccess)
{
   // Single word check.
   uint64_t E1 = 0x2CA7F46BF6569915ULL;
   ApInt A1(64, E1);
   for (unsigned i = 0, e = 64; i < e; ++i) {
      EXPECT_EQ(bool(E1 & (1ULL << i)),
                A1[i]);
   }

   // Multiword check.
   ApInt::WordType E2[4] = {
      0xEB6EB136591CBA21ULL,
      0x7B9358BD6A33F10AULL,
      0x7E7FFA5EADD8846ULL,
      0x305F341CA00B613DULL
   };
   ApInt A2(ApInt::APINT_BITS_PER_WORD*4, E2);
   for (unsigned i = 0; i < 4; ++i) {
      for (unsigned j = 0; j < ApInt::APINT_BITS_PER_WORD; ++j) {
         EXPECT_EQ(bool(E2[i] & (1ULL << j)),
                   A2[i*ApInt::APINT_BITS_PER_WORD + j]);
      }
   }
}

TEST(ApIntTest, testLargeAPIntConstruction)
{
   // Check that we can properly construct very large ApInt. It is very
   // unlikely that people will ever do this, but it is a legal input,
   // so we should not crash on it.
   ApInt A9(UINT32_MAX, 0);
   EXPECT_FALSE(A9.getBoolValue());
}

TEST(ApIntTest, testNearestLogBase2)
{
   // Single word check.

   // Test round up.
   uint64_t I1 = 0x1800001;
   ApInt A1(64, I1);
   EXPECT_EQ(A1.nearestLogBase2(), A1.ceilLogBase2());

   // Test round down.
   uint64_t I2 = 0x1000011;
   ApInt A2(64, I2);
   EXPECT_EQ(A2.nearestLogBase2(), A2.logBase2());

   // Test ties round up.
   uint64_t I3 = 0x1800000;
   ApInt A3(64, I3);
   EXPECT_EQ(A3.nearestLogBase2(), A3.ceilLogBase2());

   // Multiple word check.

   // Test round up.
   ApInt::WordType I4[4] = {0x0, 0xF, 0x18, 0x0};
   ApInt A4(ApInt::APINT_BITS_PER_WORD*4, I4);
   EXPECT_EQ(A4.nearestLogBase2(), A4.ceilLogBase2());

   // Test round down.
   ApInt::WordType I5[4] = {0x0, 0xF, 0x10, 0x0};
   ApInt A5(ApInt::APINT_BITS_PER_WORD*4, I5);
   EXPECT_EQ(A5.nearestLogBase2(), A5.logBase2());

   // Test ties round up.
   uint64_t I6[4] = {0x0, 0x0, 0x0, 0x18};
   ApInt A6(ApInt::APINT_BITS_PER_WORD*4, I6);
   EXPECT_EQ(A6.nearestLogBase2(), A6.ceilLogBase2());

   // Test BitWidth == 1 special cases.
   ApInt A7(1, 1);
   EXPECT_EQ(A7.nearestLogBase2(), 0ULL);
   ApInt A8(1, 0);
   EXPECT_EQ(A8.nearestLogBase2(), UINT32_MAX);

   // Test the zero case when we have a bit width large enough such
   // that the bit width is larger than UINT32_MAX-1.
   ApInt A9(UINT32_MAX, 0);
   EXPECT_EQ(A9.nearestLogBase2(), UINT32_MAX);
}

TEST(ApIntTest, testIsSplat)
{
   ApInt A(32, 0x01010101);
   EXPECT_FALSE(A.isSplat(1));
   EXPECT_FALSE(A.isSplat(2));
   EXPECT_FALSE(A.isSplat(4));
   EXPECT_TRUE(A.isSplat(8));
   EXPECT_TRUE(A.isSplat(16));
   EXPECT_TRUE(A.isSplat(32));

   ApInt B(24, 0xAAAAAA);
   EXPECT_FALSE(B.isSplat(1));
   EXPECT_TRUE(B.isSplat(2));
   EXPECT_TRUE(B.isSplat(4));
   EXPECT_TRUE(B.isSplat(8));
   EXPECT_TRUE(B.isSplat(24));

   ApInt C(24, 0xABAAAB);
   EXPECT_FALSE(C.isSplat(1));
   EXPECT_FALSE(C.isSplat(2));
   EXPECT_FALSE(C.isSplat(4));
   EXPECT_FALSE(C.isSplat(8));
   EXPECT_TRUE(C.isSplat(24));

   ApInt D(32, 0xABBAABBA);
   EXPECT_FALSE(D.isSplat(1));
   EXPECT_FALSE(D.isSplat(2));
   EXPECT_FALSE(D.isSplat(4));
   EXPECT_FALSE(D.isSplat(8));
   EXPECT_TRUE(D.isSplat(16));
   EXPECT_TRUE(D.isSplat(32));

   ApInt E(32, 0);
   EXPECT_TRUE(E.isSplat(1));
   EXPECT_TRUE(E.isSplat(2));
   EXPECT_TRUE(E.isSplat(4));
   EXPECT_TRUE(E.isSplat(8));
   EXPECT_TRUE(E.isSplat(16));
   EXPECT_TRUE(E.isSplat(32));
}

TEST(ApIntTest, testIsMask)
{
   EXPECT_FALSE(ApInt(32, 0x01010101).isMask());
   EXPECT_FALSE(ApInt(32, 0xf0000000).isMask());
   EXPECT_FALSE(ApInt(32, 0xffff0000).isMask());
   EXPECT_FALSE(ApInt(32, 0xff << 1).isMask());

   for (int N : { 1, 2, 3, 4, 7, 8, 16, 32, 64, 127, 128, 129, 256 }) {
      EXPECT_FALSE(ApInt(N, 0).isMask());

      ApInt One(N, 1);
      for (int I = 1; I <= N; ++I) {
         ApInt MaskVal = One.shl(I) - 1;
         EXPECT_TRUE(MaskVal.isMask());
         EXPECT_TRUE(MaskVal.isMask(I));
      }
   }
}

TEST(ApIntTest, testIsShiftedMask)
{
   EXPECT_FALSE(ApInt(32, 0x01010101).isShiftedMask());
   EXPECT_TRUE(ApInt(32, 0xf0000000).isShiftedMask());
   EXPECT_TRUE(ApInt(32, 0xffff0000).isShiftedMask());
   EXPECT_TRUE(ApInt(32, 0xff << 1).isShiftedMask());

   for (int N : { 1, 2, 3, 4, 7, 8, 16, 32, 64, 127, 128, 129, 256 }) {
      EXPECT_FALSE(ApInt(N, 0).isShiftedMask());

      ApInt One(N, 1);
      for (int I = 1; I < N; ++I) {
         ApInt MaskVal = One.shl(I) - 1;
         EXPECT_TRUE(MaskVal.isShiftedMask());
      }
      for (int I = 1; I < N - 1; ++I) {
         ApInt MaskVal = One.shl(I);
         EXPECT_TRUE(MaskVal.isShiftedMask());
      }
      for (int I = 1; I < N; ++I) {
         ApInt MaskVal = ApInt::getHighBitsSet(N, I);
         EXPECT_TRUE(MaskVal.isShiftedMask());
      }
   }
}

// Test that self-move works, but only when we're using MSVC.
#if defined(_MSC_VER)
#if defined(__clang__)
// Disable the pragma warning from versions of Clang without -Wself-move
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
// Disable the warning that triggers on exactly what is being tested.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
#endif
TEST(ApIntTest, SelfMoveAssignment) {
   ApInt X(32, 0xdeadbeef);
   X = std::move(X);
   EXPECT_EQ(32u, X.getBitWidth());
   EXPECT_EQ(0xdeadbeefULL, X.getLimitedValue());

   uint64_t Bits[] = {0xdeadbeefdeadbeefULL, 0xdeadbeefdeadbeefULL};
   ApInt Y(128, Bits);
   Y = std::move(Y);
   EXPECT_EQ(128u, Y.getBitWidth());
   EXPECT_EQ(~0ULL, Y.getLimitedValue());
   const uint64_t *Raw = Y.getRawData();
   EXPECT_EQ(2u, Y.getNumWords());
   EXPECT_EQ(0xdeadbeefdeadbeefULL, Raw[0]);
   EXPECT_EQ(0xdeadbeefdeadbeefULL, Raw[1]);
}
#if defined(__clang__)
#pragma clang diagnostic pop
#pragma clang diagnostic pop
#endif
#endif // _MSC_VER

TEST(ApIntTest, testReverseBits) {
   EXPECT_EQ(1, ApInt(1, 1).reverseBits());
   EXPECT_EQ(0, ApInt(1, 0).reverseBits());

   EXPECT_EQ(3, ApInt(2, 3).reverseBits());
   EXPECT_EQ(3, ApInt(2, 3).reverseBits());

   EXPECT_EQ(0xb, ApInt(4, 0xd).reverseBits());
   EXPECT_EQ(0xd, ApInt(4, 0xb).reverseBits());
   EXPECT_EQ(0xf, ApInt(4, 0xf).reverseBits());

   EXPECT_EQ(0x30, ApInt(7, 0x6).reverseBits());
   EXPECT_EQ(0x5a, ApInt(7, 0x2d).reverseBits());

   EXPECT_EQ(0x0f, ApInt(8, 0xf0).reverseBits());
   EXPECT_EQ(0xf0, ApInt(8, 0x0f).reverseBits());

   EXPECT_EQ(0x0f0f, ApInt(16, 0xf0f0).reverseBits());
   EXPECT_EQ(0xf0f0, ApInt(16, 0x0f0f).reverseBits());

   EXPECT_EQ(0x0f0f0f0f, ApInt(32, 0xf0f0f0f0).reverseBits());
   EXPECT_EQ(0xf0f0f0f0, ApInt(32, 0x0f0f0f0f).reverseBits());

   EXPECT_EQ(0x402880a0 >> 1, ApInt(31, 0x05011402).reverseBits());

   EXPECT_EQ(0x0f0f0f0f, ApInt(32, 0xf0f0f0f0).reverseBits());
   EXPECT_EQ(0xf0f0f0f0, ApInt(32, 0x0f0f0f0f).reverseBits());

   EXPECT_EQ(0x0f0f0f0f0f0f0f0f, ApInt(64, 0xf0f0f0f0f0f0f0f0).reverseBits());
   EXPECT_EQ(0xf0f0f0f0f0f0f0f0, ApInt(64, 0x0f0f0f0f0f0f0f0f).reverseBits());

   for (unsigned N : { 1, 8, 16, 24, 31, 32, 33,
        63, 64, 65, 127, 128, 257, 1024 }) {
      for (unsigned I = 0; I < N; ++I) {
         ApInt X = ApInt::getOneBitSet(N, I);
         ApInt Y = ApInt::getOneBitSet(N, N - (I + 1));
         EXPECT_EQ(Y, X.reverseBits());
         EXPECT_EQ(X, Y.reverseBits());
      }
   }
}

TEST(ApIntTest, testInsertBits)
{
   ApInt iSrc(31, 0x00123456);

   // Direct copy.
   ApInt i31(31, 0x76543210ull);
   i31.insertBits(iSrc, 0);
   EXPECT_EQ(static_cast<int64_t>(0x00123456ull), i31.getSignExtValue());

   // Single word src/dst insertion.
   ApInt i63(63, 0x01234567FFFFFFFFull);
   i63.insertBits(iSrc, 4);
   EXPECT_EQ(static_cast<int64_t>(0x012345600123456Full), i63.getSignExtValue());

   // Insert single word src into one word of dst.
   ApInt i120(120, UINT64_MAX, true);
   i120.insertBits(iSrc, 8);
   EXPECT_EQ(static_cast<int64_t>(0xFFFFFF80123456FFull), i120.getSignExtValue());

   // Insert single word src into two words of dst.
   ApInt i127(127, UINT64_MAX, true);
   i127.insertBits(iSrc, 48);
   EXPECT_EQ(i127.extractBits(64, 0).getZeroExtValue(), 0x3456FFFFFFFFFFFFull);
   EXPECT_EQ(i127.extractBits(63, 64).getZeroExtValue(), 0x7FFFFFFFFFFF8012ull);

   // Insert on word boundaries.
   ApInt i128(128, 0);
   i128.insertBits(ApInt(64, UINT64_MAX, true), 0);
   i128.insertBits(ApInt(64, UINT64_MAX, true), 64);
   EXPECT_EQ(-1, i128.getSignExtValue());

   ApInt i256(256, UINT64_MAX, true);
   i256.insertBits(ApInt(65, 0), 0);
   i256.insertBits(ApInt(69, 0), 64);
   i256.insertBits(ApInt(128, 0), 128);
   EXPECT_EQ(0u, i256.getSignExtValue());

   ApInt i257(257, 0);
   i257.insertBits(ApInt(96, UINT64_MAX, true), 64);
   EXPECT_EQ(i257.extractBits(64, 0).getZeroExtValue(), 0x0000000000000000ull);
   EXPECT_EQ(i257.extractBits(64, 64).getZeroExtValue(), 0xFFFFFFFFFFFFFFFFull);
   EXPECT_EQ(i257.extractBits(64, 128).getZeroExtValue(), 0x00000000FFFFFFFFull);
   EXPECT_EQ(i257.extractBits(65, 192).getZeroExtValue(), 0x0000000000000000ull);

   // General insertion.
   ApInt i260(260, UINT64_MAX, true);
   i260.insertBits(ApInt(129, 1ull << 48), 15);
   EXPECT_EQ(i260.extractBits(64, 0).getZeroExtValue(), 0x8000000000007FFFull);
   EXPECT_EQ(i260.extractBits(64, 64).getZeroExtValue(), 0x0000000000000000ull);
   EXPECT_EQ(i260.extractBits(64, 128).getZeroExtValue(), 0xFFFFFFFFFFFF0000ull);
   EXPECT_EQ(i260.extractBits(64, 192).getZeroExtValue(), 0xFFFFFFFFFFFFFFFFull);
   EXPECT_EQ(i260.extractBits(4, 256).getZeroExtValue(), 0x000000000000000Full);
}

TEST(ApIntTest, testExtractBits)
{
   ApInt i32(32, 0x1234567);
   EXPECT_EQ(0x3456, i32.extractBits(16, 4));

   ApInt i257(257, 0xFFFFFFFFFF0000FFull, true);
   EXPECT_EQ(0xFFu, i257.extractBits(16, 0));
   EXPECT_EQ((0xFFu >> 1), i257.extractBits(16, 1));
   EXPECT_EQ(-1, i257.extractBits(32, 64).getSignExtValue());
   EXPECT_EQ(-1, i257.extractBits(128, 128).getSignExtValue());
   EXPECT_EQ(-1, i257.extractBits(66, 191).getSignExtValue());
   EXPECT_EQ(static_cast<int64_t>(0xFFFFFFFFFF80007Full),
             i257.extractBits(128, 1).getSignExtValue());
   EXPECT_EQ(static_cast<int64_t>(0xFFFFFFFFFF80007Full),
             i257.extractBits(129, 1).getSignExtValue());

   EXPECT_EQ(ApInt(48, 0),
             ApInt(144, "281474976710655", 10).extractBits(48, 48));
   EXPECT_EQ(ApInt(48, 0x0000ffffffffffffull),
             ApInt(144, "281474976710655", 10).extractBits(48, 0));
   EXPECT_EQ(ApInt(48, 0x00007fffffffffffull),
             ApInt(144, "281474976710655", 10).extractBits(48, 1));
}

TEST(ApIntTest, testGetLowBitsSet)
{
   ApInt i128lo64 = ApInt::getLowBitsSet(128, 64);
   EXPECT_EQ(0u, i128lo64.countLeadingOnes());
   EXPECT_EQ(64u, i128lo64.countLeadingZeros());
   EXPECT_EQ(64u, i128lo64.getActiveBits());
   EXPECT_EQ(0u, i128lo64.countTrailingZeros());
   EXPECT_EQ(64u, i128lo64.countTrailingOnes());
   EXPECT_EQ(64u, i128lo64.countPopulation());
}

TEST(ApIntTest, testGetBitsSet)
{
   ApInt i64hi1lo1 = ApInt::getBitsSet(64, 1, 63);
   EXPECT_EQ(0u, i64hi1lo1.countLeadingOnes());
   EXPECT_EQ(1u, i64hi1lo1.countLeadingZeros());
   EXPECT_EQ(63u, i64hi1lo1.getActiveBits());
   EXPECT_EQ(1u, i64hi1lo1.countTrailingZeros());
   EXPECT_EQ(0u, i64hi1lo1.countTrailingOnes());
   EXPECT_EQ(62u, i64hi1lo1.countPopulation());

   ApInt i127hi1lo1 = ApInt::getBitsSet(127, 1, 126);
   EXPECT_EQ(0u, i127hi1lo1.countLeadingOnes());
   EXPECT_EQ(1u, i127hi1lo1.countLeadingZeros());
   EXPECT_EQ(126u, i127hi1lo1.getActiveBits());
   EXPECT_EQ(1u, i127hi1lo1.countTrailingZeros());
   EXPECT_EQ(0u, i127hi1lo1.countTrailingOnes());
   EXPECT_EQ(125u, i127hi1lo1.countPopulation());
}

TEST(ApIntTest, testGetHighBitsSet)
{
   ApInt i64hi32 = ApInt::getHighBitsSet(64, 32);
   EXPECT_EQ(32u, i64hi32.countLeadingOnes());
   EXPECT_EQ(0u, i64hi32.countLeadingZeros());
   EXPECT_EQ(64u, i64hi32.getActiveBits());
   EXPECT_EQ(32u, i64hi32.countTrailingZeros());
   EXPECT_EQ(0u, i64hi32.countTrailingOnes());
   EXPECT_EQ(32u, i64hi32.countPopulation());
}

TEST(ApIntTest, testGetBitsSetFrom)
{
   ApInt i64hi31 = ApInt::getBitsSetFrom(64, 33);
   EXPECT_EQ(31u, i64hi31.countLeadingOnes());
   EXPECT_EQ(0u, i64hi31.countLeadingZeros());
   EXPECT_EQ(64u, i64hi31.getActiveBits());
   EXPECT_EQ(33u, i64hi31.countTrailingZeros());
   EXPECT_EQ(0u, i64hi31.countTrailingOnes());
   EXPECT_EQ(31u, i64hi31.countPopulation());
}

TEST(ApIntTest, testSetLowBits)
{
   ApInt i64lo32(64, 0);
   i64lo32.setLowBits(32);
   EXPECT_EQ(0u, i64lo32.countLeadingOnes());
   EXPECT_EQ(32u, i64lo32.countLeadingZeros());
   EXPECT_EQ(32u, i64lo32.getActiveBits());
   EXPECT_EQ(0u, i64lo32.countTrailingZeros());
   EXPECT_EQ(32u, i64lo32.countTrailingOnes());
   EXPECT_EQ(32u, i64lo32.countPopulation());

   ApInt i128lo64(128, 0);
   i128lo64.setLowBits(64);
   EXPECT_EQ(0u, i128lo64.countLeadingOnes());
   EXPECT_EQ(64u, i128lo64.countLeadingZeros());
   EXPECT_EQ(64u, i128lo64.getActiveBits());
   EXPECT_EQ(0u, i128lo64.countTrailingZeros());
   EXPECT_EQ(64u, i128lo64.countTrailingOnes());
   EXPECT_EQ(64u, i128lo64.countPopulation());

   ApInt i128lo24(128, 0);
   i128lo24.setLowBits(24);
   EXPECT_EQ(0u, i128lo24.countLeadingOnes());
   EXPECT_EQ(104u, i128lo24.countLeadingZeros());
   EXPECT_EQ(24u, i128lo24.getActiveBits());
   EXPECT_EQ(0u, i128lo24.countTrailingZeros());
   EXPECT_EQ(24u, i128lo24.countTrailingOnes());
   EXPECT_EQ(24u, i128lo24.countPopulation());

   ApInt i128lo104(128, 0);
   i128lo104.setLowBits(104);
   EXPECT_EQ(0u, i128lo104.countLeadingOnes());
   EXPECT_EQ(24u, i128lo104.countLeadingZeros());
   EXPECT_EQ(104u, i128lo104.getActiveBits());
   EXPECT_EQ(0u, i128lo104.countTrailingZeros());
   EXPECT_EQ(104u, i128lo104.countTrailingOnes());
   EXPECT_EQ(104u, i128lo104.countPopulation());

   ApInt i128lo0(128, 0);
   i128lo0.setLowBits(0);
   EXPECT_EQ(0u, i128lo0.countLeadingOnes());
   EXPECT_EQ(128u, i128lo0.countLeadingZeros());
   EXPECT_EQ(0u, i128lo0.getActiveBits());
   EXPECT_EQ(128u, i128lo0.countTrailingZeros());
   EXPECT_EQ(0u, i128lo0.countTrailingOnes());
   EXPECT_EQ(0u, i128lo0.countPopulation());

   ApInt i80lo79(80, 0);
   i80lo79.setLowBits(79);
   EXPECT_EQ(0u, i80lo79.countLeadingOnes());
   EXPECT_EQ(1u, i80lo79.countLeadingZeros());
   EXPECT_EQ(79u, i80lo79.getActiveBits());
   EXPECT_EQ(0u, i80lo79.countTrailingZeros());
   EXPECT_EQ(79u, i80lo79.countTrailingOnes());
   EXPECT_EQ(79u, i80lo79.countPopulation());
}

TEST(ApIntTest, testSetHighBits)
{
   ApInt i64hi32(64, 0);
   i64hi32.setHighBits(32);
   EXPECT_EQ(32u, i64hi32.countLeadingOnes());
   EXPECT_EQ(0u, i64hi32.countLeadingZeros());
   EXPECT_EQ(64u, i64hi32.getActiveBits());
   EXPECT_EQ(32u, i64hi32.countTrailingZeros());
   EXPECT_EQ(0u, i64hi32.countTrailingOnes());
   EXPECT_EQ(32u, i64hi32.countPopulation());

   ApInt i128hi64(128, 0);
   i128hi64.setHighBits(64);
   EXPECT_EQ(64u, i128hi64.countLeadingOnes());
   EXPECT_EQ(0u, i128hi64.countLeadingZeros());
   EXPECT_EQ(128u, i128hi64.getActiveBits());
   EXPECT_EQ(64u, i128hi64.countTrailingZeros());
   EXPECT_EQ(0u, i128hi64.countTrailingOnes());
   EXPECT_EQ(64u, i128hi64.countPopulation());

   ApInt i128hi24(128, 0);
   i128hi24.setHighBits(24);
   EXPECT_EQ(24u, i128hi24.countLeadingOnes());
   EXPECT_EQ(0u, i128hi24.countLeadingZeros());
   EXPECT_EQ(128u, i128hi24.getActiveBits());
   EXPECT_EQ(104u, i128hi24.countTrailingZeros());
   EXPECT_EQ(0u, i128hi24.countTrailingOnes());
   EXPECT_EQ(24u, i128hi24.countPopulation());

   ApInt i128hi104(128, 0);
   i128hi104.setHighBits(104);
   EXPECT_EQ(104u, i128hi104.countLeadingOnes());
   EXPECT_EQ(0u, i128hi104.countLeadingZeros());
   EXPECT_EQ(128u, i128hi104.getActiveBits());
   EXPECT_EQ(24u, i128hi104.countTrailingZeros());
   EXPECT_EQ(0u, i128hi104.countTrailingOnes());
   EXPECT_EQ(104u, i128hi104.countPopulation());

   ApInt i128hi0(128, 0);
   i128hi0.setHighBits(0);
   EXPECT_EQ(0u, i128hi0.countLeadingOnes());
   EXPECT_EQ(128u, i128hi0.countLeadingZeros());
   EXPECT_EQ(0u, i128hi0.getActiveBits());
   EXPECT_EQ(128u, i128hi0.countTrailingZeros());
   EXPECT_EQ(0u, i128hi0.countTrailingOnes());
   EXPECT_EQ(0u, i128hi0.countPopulation());

   ApInt i80hi1(80, 0);
   i80hi1.setHighBits(1);
   EXPECT_EQ(1u, i80hi1.countLeadingOnes());
   EXPECT_EQ(0u, i80hi1.countLeadingZeros());
   EXPECT_EQ(80u, i80hi1.getActiveBits());
   EXPECT_EQ(79u, i80hi1.countTrailingZeros());
   EXPECT_EQ(0u, i80hi1.countTrailingOnes());
   EXPECT_EQ(1u, i80hi1.countPopulation());

   ApInt i32hi16(32, 0);
   i32hi16.setHighBits(16);
   EXPECT_EQ(16u, i32hi16.countLeadingOnes());
   EXPECT_EQ(0u, i32hi16.countLeadingZeros());
   EXPECT_EQ(32u, i32hi16.getActiveBits());
   EXPECT_EQ(16u, i32hi16.countTrailingZeros());
   EXPECT_EQ(0u, i32hi16.countTrailingOnes());
   EXPECT_EQ(16u, i32hi16.countPopulation());
}

TEST(ApIntTest, testSetBitsFrom)
{
   ApInt i64from63(64, 0);
   i64from63.setBitsFrom(63);
   EXPECT_EQ(1u, i64from63.countLeadingOnes());
   EXPECT_EQ(0u, i64from63.countLeadingZeros());
   EXPECT_EQ(64u, i64from63.getActiveBits());
   EXPECT_EQ(63u, i64from63.countTrailingZeros());
   EXPECT_EQ(0u, i64from63.countTrailingOnes());
   EXPECT_EQ(1u, i64from63.countPopulation());
}

TEST(ApIntTest, testSetAllBits)
{
   ApInt i32(32, 0);
   i32.setAllBits();
   EXPECT_EQ(32u, i32.countLeadingOnes());
   EXPECT_EQ(0u, i32.countLeadingZeros());
   EXPECT_EQ(32u, i32.getActiveBits());
   EXPECT_EQ(0u, i32.countTrailingZeros());
   EXPECT_EQ(32u, i32.countTrailingOnes());
   EXPECT_EQ(32u, i32.countPopulation());

   ApInt i64(64, 0);
   i64.setAllBits();
   EXPECT_EQ(64u, i64.countLeadingOnes());
   EXPECT_EQ(0u, i64.countLeadingZeros());
   EXPECT_EQ(64u, i64.getActiveBits());
   EXPECT_EQ(0u, i64.countTrailingZeros());
   EXPECT_EQ(64u, i64.countTrailingOnes());
   EXPECT_EQ(64u, i64.countPopulation());

   ApInt i96(96, 0);
   i96.setAllBits();
   EXPECT_EQ(96u, i96.countLeadingOnes());
   EXPECT_EQ(0u, i96.countLeadingZeros());
   EXPECT_EQ(96u, i96.getActiveBits());
   EXPECT_EQ(0u, i96.countTrailingZeros());
   EXPECT_EQ(96u, i96.countTrailingOnes());
   EXPECT_EQ(96u, i96.countPopulation());

   ApInt i128(128, 0);
   i128.setAllBits();
   EXPECT_EQ(128u, i128.countLeadingOnes());
   EXPECT_EQ(0u, i128.countLeadingZeros());
   EXPECT_EQ(128u, i128.getActiveBits());
   EXPECT_EQ(0u, i128.countTrailingZeros());
   EXPECT_EQ(128u, i128.countTrailingOnes());
   EXPECT_EQ(128u, i128.countPopulation());
}

TEST(ApIntTest, testGetLoBits)
{
   ApInt i32(32, 0xfa);
   i32.setHighBits(1);
   EXPECT_EQ(0xa, i32.getLoBits(4));
   ApInt i128(128, 0xfa);
   i128.setHighBits(1);
   EXPECT_EQ(0xa, i128.getLoBits(4));
}

TEST(ApIntTest, testGetHiBits)
{
   ApInt i32(32, 0xfa);
   i32.setHighBits(2);
   EXPECT_EQ(0xc, i32.getHiBits(4));
   ApInt i128(128, 0xfa);
   i128.setHighBits(2);
   EXPECT_EQ(0xc, i128.getHiBits(4));
}

TEST(ApIntTest, testGCD)
{
   using apintops::greatest_common_divisor;

   for (unsigned Bits : {1, 2, 32, 63, 64, 65}) {
      // Test some corner cases near zero.
      ApInt Zero(Bits, 0), One(Bits, 1);
      EXPECT_EQ(greatest_common_divisor(Zero, Zero), Zero);
      EXPECT_EQ(greatest_common_divisor(Zero, One), One);
      EXPECT_EQ(greatest_common_divisor(One, Zero), One);
      EXPECT_EQ(greatest_common_divisor(One, One), One);

      if (Bits > 1) {
         ApInt Two(Bits, 2);
         EXPECT_EQ(greatest_common_divisor(Zero, Two), Two);
         EXPECT_EQ(greatest_common_divisor(One, Two), One);
         EXPECT_EQ(greatest_common_divisor(Two, Two), Two);

         // Test some corner cases near the highest representable value.
         ApInt Max(Bits, 0);
         Max.setAllBits();
         EXPECT_EQ(greatest_common_divisor(Zero, Max), Max);
         EXPECT_EQ(greatest_common_divisor(One, Max), One);
         EXPECT_EQ(greatest_common_divisor(Two, Max), One);
         EXPECT_EQ(greatest_common_divisor(Max, Max), Max);

         ApInt MaxOver2 = Max.udiv(Two);
         EXPECT_EQ(greatest_common_divisor(MaxOver2, Max), One);
         // Max - 1 == Max / 2 * 2, because Max is odd.
         EXPECT_EQ(greatest_common_divisor(MaxOver2, Max - 1), MaxOver2);
      }
   }

   // Compute the 20th Mersenne prime.
   const unsigned BitWidth = 4450;
   ApInt HugePrime = ApInt::getLowBitsSet(BitWidth, 4423);

   // 9931 and 123456 are coprime.
   ApInt A = HugePrime * ApInt(BitWidth, 9931);
   ApInt B = HugePrime * ApInt(BitWidth, 123456);
   ApInt C = greatest_common_divisor(A, B);
   EXPECT_EQ(C, HugePrime);
}

TEST(ApIntTest, testLogicalRightShift)
{
   ApInt i256(ApInt::getHighBitsSet(256, 2));

   i256.lshrInPlace(1);
   EXPECT_EQ(1U, i256.countLeadingZeros());
   EXPECT_EQ(253U, i256.countTrailingZeros());
   EXPECT_EQ(2U, i256.countPopulation());

   i256.lshrInPlace(62);
   EXPECT_EQ(63U, i256.countLeadingZeros());
   EXPECT_EQ(191U, i256.countTrailingZeros());
   EXPECT_EQ(2U, i256.countPopulation());

   i256.lshrInPlace(65);
   EXPECT_EQ(128U, i256.countLeadingZeros());
   EXPECT_EQ(126U, i256.countTrailingZeros());
   EXPECT_EQ(2U, i256.countPopulation());

   i256.lshrInPlace(64);
   EXPECT_EQ(192U, i256.countLeadingZeros());
   EXPECT_EQ(62U, i256.countTrailingZeros());
   EXPECT_EQ(2U, i256.countPopulation());

   i256.lshrInPlace(63);
   EXPECT_EQ(255U, i256.countLeadingZeros());
   EXPECT_EQ(0U, i256.countTrailingZeros());
   EXPECT_EQ(1U, i256.countPopulation());

   // Ensure we handle large shifts of multi-word.
   const ApInt neg_one(128, static_cast<uint64_t>(-1), true);
   EXPECT_EQ(0, neg_one.lshr(128));
}

TEST(ApIntTest, testArithmeticRightShift)
{
   ApInt i72(ApInt::getHighBitsSet(72, 1));
   i72.ashrInPlace(46);
   EXPECT_EQ(47U, i72.countLeadingOnes());
   EXPECT_EQ(25U, i72.countTrailingZeros());
   EXPECT_EQ(47U, i72.countPopulation());

   i72 = ApInt::getHighBitsSet(72, 1);
   i72.ashrInPlace(64);
   EXPECT_EQ(65U, i72.countLeadingOnes());
   EXPECT_EQ(7U, i72.countTrailingZeros());
   EXPECT_EQ(65U, i72.countPopulation());

   ApInt i128(ApInt::getHighBitsSet(128, 1));
   i128.ashrInPlace(64);
   EXPECT_EQ(65U, i128.countLeadingOnes());
   EXPECT_EQ(63U, i128.countTrailingZeros());
   EXPECT_EQ(65U, i128.countPopulation());

   // Ensure we handle large shifts of multi-word.
   const ApInt signmin32(ApInt::getSignedMinValue(32));
   EXPECT_TRUE(signmin32.ashr(32).isAllOnesValue());

   // Ensure we handle large shifts of multi-word.
   const ApInt umax32(ApInt::getSignedMaxValue(32));
   EXPECT_EQ(0, umax32.ashr(32));

   // Ensure we handle large shifts of multi-word.
   const ApInt signmin128(ApInt::getSignedMinValue(128));
   EXPECT_TRUE(signmin128.ashr(128).isAllOnesValue());

   // Ensure we handle large shifts of multi-word.
   const ApInt umax128(ApInt::getSignedMaxValue(128));
   EXPECT_EQ(0, umax128.ashr(128));
}

TEST(ApIntTest, testLeftShift)
{
   ApInt i256(ApInt::getLowBitsSet(256, 2));

   i256 <<= 1;
   EXPECT_EQ(253U, i256.countLeadingZeros());
   EXPECT_EQ(1U, i256.countTrailingZeros());
   EXPECT_EQ(2U, i256.countPopulation());

   i256 <<= 62;
   EXPECT_EQ(191U, i256.countLeadingZeros());
   EXPECT_EQ(63U, i256.countTrailingZeros());
   EXPECT_EQ(2U, i256.countPopulation());

   i256 <<= 65;
   EXPECT_EQ(126U, i256.countLeadingZeros());
   EXPECT_EQ(128U, i256.countTrailingZeros());
   EXPECT_EQ(2U, i256.countPopulation());

   i256 <<= 64;
   EXPECT_EQ(62U, i256.countLeadingZeros());
   EXPECT_EQ(192U, i256.countTrailingZeros());
   EXPECT_EQ(2U, i256.countPopulation());

   i256 <<= 63;
   EXPECT_EQ(0U, i256.countLeadingZeros());
   EXPECT_EQ(255U, i256.countTrailingZeros());
   EXPECT_EQ(1U, i256.countPopulation());

   // Ensure we handle large shifts of multi-word.
   const ApInt neg_one(128, static_cast<uint64_t>(-1), true);
   EXPECT_EQ(0, neg_one.shl(128));
}

TEST(ApIntTest, testIsSubsetOf)
{
   ApInt i32_1(32, 1);
   ApInt i32_2(32, 2);
   ApInt i32_3(32, 3);
   EXPECT_FALSE(i32_3.isSubsetOf(i32_1));
   EXPECT_TRUE(i32_1.isSubsetOf(i32_3));
   EXPECT_FALSE(i32_2.isSubsetOf(i32_1));
   EXPECT_FALSE(i32_1.isSubsetOf(i32_2));
   EXPECT_TRUE(i32_3.isSubsetOf(i32_3));

   ApInt i128_1(128, 1);
   ApInt i128_2(128, 2);
   ApInt i128_3(128, 3);
   EXPECT_FALSE(i128_3.isSubsetOf(i128_1));
   EXPECT_TRUE(i128_1.isSubsetOf(i128_3));
   EXPECT_FALSE(i128_2.isSubsetOf(i128_1));
   EXPECT_FALSE(i128_1.isSubsetOf(i128_2));
   EXPECT_TRUE(i128_3.isSubsetOf(i128_3));

   i128_1 <<= 64;
   i128_2 <<= 64;
   i128_3 <<= 64;
   EXPECT_FALSE(i128_3.isSubsetOf(i128_1));
   EXPECT_TRUE(i128_1.isSubsetOf(i128_3));
   EXPECT_FALSE(i128_2.isSubsetOf(i128_1));
   EXPECT_FALSE(i128_1.isSubsetOf(i128_2));
   EXPECT_TRUE(i128_3.isSubsetOf(i128_3));
}

TEST(ApIntTest, testSext)
{
   EXPECT_EQ(0, ApInt(1, 0).sext(64));
   EXPECT_EQ(~uint64_t(0), ApInt(1, 1).sext(64));

   ApInt i32_max(ApInt::getSignedMaxValue(32).sext(63));
   EXPECT_EQ(32U, i32_max.countLeadingZeros());
   EXPECT_EQ(0U, i32_max.countTrailingZeros());
   EXPECT_EQ(31U, i32_max.countPopulation());

   ApInt i32_min(ApInt::getSignedMinValue(32).sext(63));
   EXPECT_EQ(32U, i32_min.countLeadingOnes());
   EXPECT_EQ(31U, i32_min.countTrailingZeros());
   EXPECT_EQ(32U, i32_min.countPopulation());

   ApInt i32_neg1(ApInt(32, ~uint64_t(0)).sext(63));
   EXPECT_EQ(63U, i32_neg1.countLeadingOnes());
   EXPECT_EQ(0U, i32_neg1.countTrailingZeros());
   EXPECT_EQ(63U, i32_neg1.countPopulation());
}

TEST(ApIntTest, testMultiply)
{
   ApInt i64(64, 1234);

   EXPECT_EQ(7006652, i64 * 5678);
   EXPECT_EQ(7006652, 5678 * i64);

   ApInt i128 = ApInt::getOneBitSet(128, 64);
   ApInt i128_1234(128, 1234);
   i128_1234 <<= 64;
   EXPECT_EQ(i128_1234, i128 * 1234);
   EXPECT_EQ(i128_1234, 1234 * i128);

   ApInt i96 = ApInt::getOneBitSet(96, 64);
   i96 *= ~0ULL;
   EXPECT_EQ(32U, i96.countLeadingOnes());
   EXPECT_EQ(32U, i96.countPopulation());
   EXPECT_EQ(64U, i96.countTrailingZeros());
}

TEST(ApIntTest, testRoundingUDiv)
{
   for (uint64_t Ai = 1; Ai <= 255; Ai++) {
      ApInt A(8, Ai);
      ApInt Zero(8, 0);
      EXPECT_EQ(0, apintops::rounding_udiv(Zero, A, ApInt::Rounding::UP));
      EXPECT_EQ(0, apintops::rounding_udiv(Zero, A, ApInt::Rounding::DOWN));
      EXPECT_EQ(0, apintops::rounding_udiv(Zero, A, ApInt::Rounding::TOWARD_ZERO));

      for (uint64_t Bi = 1; Bi <= 255; Bi++) {
         ApInt B(8, Bi);
         {
            ApInt quo = apintops::rounding_udiv(A, B, ApInt::Rounding::UP);
            auto prod = quo.zext(16) * B.zext(16);
            EXPECT_TRUE(prod.uge(Ai));
            if (prod.ugt(Ai)) {
               EXPECT_TRUE(((quo - 1).zext(16) * B.zext(16)).ult(Ai));
            }
         }
         {
            ApInt quo = A.udiv(B);
            EXPECT_EQ(quo, apintops::rounding_udiv(A, B, ApInt::Rounding::TOWARD_ZERO));
            EXPECT_EQ(quo, apintops::rounding_udiv(A, B, ApInt::Rounding::DOWN));
         }
      }
   }
}

TEST(ApIntTest, testRoundingSDiv)
{
   for (int64_t Ai = -128; Ai <= 127; Ai++) {
      ApInt A(8, Ai);

      if (Ai != 0) {
         ApInt Zero(8, 0);
         EXPECT_EQ(0, apintops::rounding_sdiv(Zero, A, ApInt::Rounding::UP));
         EXPECT_EQ(0, apintops::rounding_sdiv(Zero, A, ApInt::Rounding::DOWN));
         EXPECT_EQ(0, apintops::rounding_sdiv(Zero, A, ApInt::Rounding::TOWARD_ZERO));
      }

      for (uint64_t Bi = -128; Bi <= 127; Bi++) {
         if (Bi == 0)
            continue;

         ApInt B(8, Bi);
         {
            ApInt quo = apintops::rounding_sdiv(A, B, ApInt::Rounding::UP);
            auto prod = quo.sext(16) * B.sext(16);
            EXPECT_TRUE(prod.uge(A));
            if (prod.ugt(A)) {
               EXPECT_TRUE(((quo - 1).sext(16) * B.sext(16)).ult(A));
            }
         }
         {
            ApInt quo = apintops::rounding_sdiv(A, B, ApInt::Rounding::DOWN);
            auto prod = quo.sext(16) * B.sext(16);
            EXPECT_TRUE(prod.ule(A));
            if (prod.ult(A)) {
               EXPECT_TRUE(((quo + 1).sext(16) * B.sext(16)).ugt(A));
            }
         }
         {
            ApInt quo = A.sdiv(B);
            EXPECT_EQ(quo, apintops::rounding_sdiv(A, B, ApInt::Rounding::TOWARD_ZERO));
         }
      }
   }
}

TEST(ApIntTest, testSolveQuadraticEquationWrap)
{
   // Verify that "Solution" is the first non-negative integer that solves
   // Ax^2 + Bx + C = "0 or overflow", i.e. that it is a correct solution
   // as calculated by SolveQuadraticEquationWrap.
   auto Validate = [] (int A, int B, int C, unsigned Width, int Solution) {
      int Mask = (1 << Width) - 1;

      // Solution should be non-negative.
      EXPECT_GE(Solution, 0);

      auto OverflowBits = [] (int64_t V, unsigned W) {
         return V & -(1 << W);
      };

      int64_t Over0 = OverflowBits(C, Width);

      auto IsZeroOrOverflow = [&] (int X) {
         int64_t ValueAtX = A*X*X + B*X + C;
         int64_t OverX = OverflowBits(ValueAtX, Width);
         return (ValueAtX & Mask) == 0 || OverX != Over0;
      };

      auto EquationToString = [&] (const char *X_str) {
         return (Twine(A) + Twine(X_str) + Twine("^2 + ") + Twine(B) +
                 Twine(X_str) + Twine(" + ") + Twine(C) + Twine(", bitwidth: ") +
                 Twine(Width)).getStr();
      };

      auto IsSolution = [&] (const char *X_str, int X) {
         if (IsZeroOrOverflow(X))
            return ::testing::AssertionSuccess()
                  << X << " is a solution of " << EquationToString(X_str);
         return ::testing::AssertionFailure()
               << X << " is not an expected solution of "
               << EquationToString(X_str);
      };

      auto IsNotSolution = [&] (const char *X_str, int X) {
         if (!IsZeroOrOverflow(X))
            return ::testing::AssertionSuccess()
                  << X << " is not a solution of " << EquationToString(X_str);
         return ::testing::AssertionFailure()
               << X << " is an unexpected solution of "
               << EquationToString(X_str);
      };

      // This is the important part: make sure that there is no solution that
      // is less than the calculated one.
      if (Solution > 0) {
         for (int X = 1; X < Solution-1; ++X)
            EXPECT_PRED_FORMAT1(IsNotSolution, X);
      }

      // Verify that the calculated solution is indeed a solution.
      EXPECT_PRED_FORMAT1(IsSolution, Solution);
   };

   // Generate all possible quadratic equations with Width-bit wide integer
   // coefficients, get the solution from SolveQuadraticEquationWrap, and
   // verify that the solution is correct.
   auto Iterate = [&] (unsigned Width) {
      assert(1 < Width && Width < 32);
      int Low = -(1 << (Width-1));
      int High = (1 << (Width-1));

      for (int A = Low; A != High; ++A) {
         if (A == 0)
            continue;
         for (int B = Low; B != High; ++B) {
            for (int C = Low; C != High; ++C) {
               std::optional<ApInt> S = apintops::solve_quadratic_equation_wrap(
                        ApInt(Width, A), ApInt(Width, B),
                        ApInt(Width, C), Width);
               if (S.has_value())
                  Validate(A, B, C, Width, S->getSignExtValue());
            }
         }
      }
   };

   // Test all widths in [2..6].
   for (unsigned i = 2; i <= 6; ++i)
      Iterate(i);
}

} // anonymous namespace
