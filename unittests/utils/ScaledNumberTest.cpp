// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/07/15.

#include "polarphp/utils/ScaledNumber.h"
#include "polarphp/global/DataTypes.h"
#include "gtest/gtest.h"

using namespace polar;
using namespace polar::utils;
using namespace polar::utils::scalednumbers;

namespace {

template <class UIntT> struct ScaledPair
{
   UIntT D;
   int S;
   ScaledPair(const std::pair<UIntT, int16_t> &F) : D(F.first), S(F.second) {}
   ScaledPair(UIntT D, int S) : D(D), S(S) {}

   bool operator==(const ScaledPair<UIntT> &X) const {
      return D == X.D && S == X.S;
   }
};
template <class UIntT>
bool operator==(const std::pair<UIntT, int16_t> &L,
                const ScaledPair<UIntT> &R)
{
   return ScaledPair<UIntT>(L) == R;
}
template <class UIntT>
void PrintTo(const ScaledPair<UIntT> &F, ::std::ostream *os)
{
   *os << F.D << "*2^" << F.S;
}

typedef ScaledPair<uint32_t> SP32;
typedef ScaledPair<uint64_t> SP64;

TEST(ScaledNumberHelpersTest, testGetRounded)
{
   EXPECT_EQ(get_rounded32(0, 0, false), SP32(0, 0));
   EXPECT_EQ(get_rounded32(0, 0, true), SP32(1, 0));
   EXPECT_EQ(get_rounded32(20, 21, true), SP32(21, 21));
   EXPECT_EQ(get_rounded32(UINT32_MAX, 0, false), SP32(UINT32_MAX, 0));
   EXPECT_EQ(get_rounded32(UINT32_MAX, 0, true), SP32(1 << 31, 1));

   EXPECT_EQ(get_rounded64(0, 0, false), SP64(0, 0));
   EXPECT_EQ(get_rounded64(0, 0, true), SP64(1, 0));
   EXPECT_EQ(get_rounded64(20, 21, true), SP64(21, 21));
   EXPECT_EQ(get_rounded64(UINT32_MAX, 0, false), SP64(UINT32_MAX, 0));
   EXPECT_EQ(get_rounded64(UINT32_MAX, 0, true), SP64(UINT64_C(1) << 32, 0));
   EXPECT_EQ(get_rounded64(UINT64_MAX, 0, false), SP64(UINT64_MAX, 0));
   EXPECT_EQ(get_rounded64(UINT64_MAX, 0, true), SP64(UINT64_C(1) << 63, 1));
}

TEST(ScaledNumberHelpersTest, testGetAdjusted)
{
   const uint64_t Max32In64 = UINT32_MAX;
   EXPECT_EQ(get_adjusted32(0), SP32(0, 0));
   EXPECT_EQ(get_adjusted32(0, 5), SP32(0, 5));
   EXPECT_EQ(get_adjusted32(UINT32_MAX), SP32(UINT32_MAX, 0));
   EXPECT_EQ(get_adjusted32(Max32In64 << 1), SP32(UINT32_MAX, 1));
   EXPECT_EQ(get_adjusted32(Max32In64 << 1, 1), SP32(UINT32_MAX, 2));
   EXPECT_EQ(get_adjusted32(Max32In64 << 31), SP32(UINT32_MAX, 31));
   EXPECT_EQ(get_adjusted32(Max32In64 << 32), SP32(UINT32_MAX, 32));
   EXPECT_EQ(get_adjusted32(Max32In64 + 1), SP32(1u << 31, 1));
   EXPECT_EQ(get_adjusted32(UINT64_MAX), SP32(1u << 31, 33));

   EXPECT_EQ(get_adjusted64(0), SP64(0, 0));
   EXPECT_EQ(get_adjusted64(0, 5), SP64(0, 5));
   EXPECT_EQ(get_adjusted64(UINT32_MAX), SP64(UINT32_MAX, 0));
   EXPECT_EQ(get_adjusted64(Max32In64 << 1), SP64(Max32In64 << 1, 0));
   EXPECT_EQ(get_adjusted64(Max32In64 << 1, 1), SP64(Max32In64 << 1, 1));
   EXPECT_EQ(get_adjusted64(Max32In64 << 31), SP64(Max32In64 << 31, 0));
   EXPECT_EQ(get_adjusted64(Max32In64 << 32), SP64(Max32In64 << 32, 0));
   EXPECT_EQ(get_adjusted64(Max32In64 + 1), SP64(Max32In64 + 1, 0));
   EXPECT_EQ(get_adjusted64(UINT64_MAX), SP64(UINT64_MAX, 0));
}

TEST(ScaledNumberHelpersTest, testGetProduct)
{
   // Zero.
   EXPECT_EQ(SP32(0, 0), get_product32(0, 0));
   EXPECT_EQ(SP32(0, 0), get_product32(0, 1));
   EXPECT_EQ(SP32(0, 0), get_product32(0, 33));

   // Basic.
   EXPECT_EQ(SP32(6, 0), get_product32(2, 3));
   EXPECT_EQ(SP32(UINT16_MAX / 3 * UINT16_MAX / 5 * 2, 0),
             get_product32(UINT16_MAX / 3, UINT16_MAX / 5 * 2));

   // Overflow, no loss of precision.
   // ==> 0xf00010 * 0x1001
   // ==> 0xf00f00000 + 0x10010
   // ==> 0xf00f10010
   // ==> 0xf00f1001 * 2^4
   EXPECT_EQ(SP32(0xf00f1001, 4), get_product32(0xf00010, 0x1001));

   // Overflow, loss of precision, rounds down.
   // ==> 0xf000070 * 0x1001
   // ==> 0xf00f000000 + 0x70070
   // ==> 0xf00f070070
   // ==> 0xf00f0700 * 2^8
   EXPECT_EQ(SP32(0xf00f0700, 8), get_product32(0xf000070, 0x1001));

   // Overflow, loss of precision, rounds up.
   // ==> 0xf000080 * 0x1001
   // ==> 0xf00f000000 + 0x80080
   // ==> 0xf00f080080
   // ==> 0xf00f0801 * 2^8
   EXPECT_EQ(SP32(0xf00f0801, 8), get_product32(0xf000080, 0x1001));

   // Reverse operand order.
   EXPECT_EQ(SP32(0, 0), get_product32(1, 0));
   EXPECT_EQ(SP32(0, 0), get_product32(33, 0));
   EXPECT_EQ(SP32(6, 0), get_product32(3, 2));
   EXPECT_EQ(SP32(UINT16_MAX / 3 * UINT16_MAX / 5 * 2, 0),
             get_product32(UINT16_MAX / 5 * 2, UINT16_MAX / 3));
   EXPECT_EQ(SP32(0xf00f1001, 4), get_product32(0x1001, 0xf00010));
   EXPECT_EQ(SP32(0xf00f0700, 8), get_product32(0x1001, 0xf000070));
   EXPECT_EQ(SP32(0xf00f0801, 8), get_product32(0x1001, 0xf000080));

   // Round to overflow.
   EXPECT_EQ(SP64(UINT64_C(1) << 63, 64),
             get_product64(UINT64_C(10376293541461622786),
                           UINT64_C(16397105843297379211)));

   // Big number with rounding.
   EXPECT_EQ(SP64(UINT64_C(9223372036854775810), 64),
             get_product64(UINT64_C(18446744073709551556),
                           UINT64_C(9223372036854775840)));
}

TEST(ScaledNumberHelpersTest, testSetQuotient)
{
   // Zero.
   EXPECT_EQ(SP32(0, 0), get_quotient32(0, 0));
   EXPECT_EQ(SP32(0, 0), get_quotient32(0, 1));
   EXPECT_EQ(SP32(0, 0), get_quotient32(0, 73));
   EXPECT_EQ(SP32(UINT32_MAX, MAX_SCALE), get_quotient32(1, 0));
   EXPECT_EQ(SP32(UINT32_MAX, MAX_SCALE), get_quotient32(6, 0));

   // Powers of two.
   EXPECT_EQ(SP32(1u << 31, -31), get_quotient32(1, 1));
   EXPECT_EQ(SP32(1u << 31, -30), get_quotient32(2, 1));
   EXPECT_EQ(SP32(1u << 31, -33), get_quotient32(4, 16));
   EXPECT_EQ(SP32(7u << 29, -29), get_quotient32(7, 1));
   EXPECT_EQ(SP32(7u << 29, -30), get_quotient32(7, 2));
   EXPECT_EQ(SP32(7u << 29, -33), get_quotient32(7, 16));

   // Divide evenly.
   EXPECT_EQ(SP32(3u << 30, -30), get_quotient32(9, 3));
   EXPECT_EQ(SP32(9u << 28, -28), get_quotient32(63, 7));

   // Divide unevenly.
   EXPECT_EQ(SP32(0xaaaaaaab, -33), get_quotient32(1, 3));
   EXPECT_EQ(SP32(0xd5555555, -31), get_quotient32(5, 3));

   // 64-bit division is hard to test, since divide64 doesn't canonicalize its
   // output.  However, this is the algorithm the implementation uses:
   //
   // - Shift divisor right.
   // - If we have 1 (power of 2), return early -- not canonicalized.
   // - Shift dividend left.
   // - 64-bit integer divide.
   // - If there's a remainder, continue with long division.
   //
   // TODO: require less knowledge about the implementation in the test.

   // Zero.
   EXPECT_EQ(SP64(0, 0), get_quotient64(0, 0));
   EXPECT_EQ(SP64(0, 0), get_quotient64(0, 1));
   EXPECT_EQ(SP64(0, 0), get_quotient64(0, 73));
   EXPECT_EQ(SP64(UINT64_MAX, MAX_SCALE), get_quotient64(1, 0));
   EXPECT_EQ(SP64(UINT64_MAX, MAX_SCALE), get_quotient64(6, 0));

   // Powers of two.
   EXPECT_EQ(SP64(1, 0), get_quotient64(1, 1));
   EXPECT_EQ(SP64(2, 0), get_quotient64(2, 1));
   EXPECT_EQ(SP64(4, -4), get_quotient64(4, 16));
   EXPECT_EQ(SP64(7, 0), get_quotient64(7, 1));
   EXPECT_EQ(SP64(7, -1), get_quotient64(7, 2));
   EXPECT_EQ(SP64(7, -4), get_quotient64(7, 16));

   // Divide evenly.
   EXPECT_EQ(SP64(UINT64_C(3) << 60, -60), get_quotient64(9, 3));
   EXPECT_EQ(SP64(UINT64_C(9) << 58, -58), get_quotient64(63, 7));

   // Divide unevenly.
   EXPECT_EQ(SP64(0xaaaaaaaaaaaaaaab, -65), get_quotient64(1, 3));
   EXPECT_EQ(SP64(0xd555555555555555, -63), get_quotient64(5, 3));
}

TEST(ScaledNumberHelpersTest, testGetLg)
{
   EXPECT_EQ(0, get_lg(UINT32_C(1), 0));
   EXPECT_EQ(1, get_lg(UINT32_C(1), 1));
   EXPECT_EQ(1, get_lg(UINT32_C(2), 0));
   EXPECT_EQ(3, get_lg(UINT32_C(1), 3));
   EXPECT_EQ(3, get_lg(UINT32_C(7), 0));
   EXPECT_EQ(3, get_lg(UINT32_C(8), 0));
   EXPECT_EQ(3, get_lg(UINT32_C(9), 0));
   EXPECT_EQ(3, get_lg(UINT32_C(64), -3));
   EXPECT_EQ(31, get_lg((UINT32_MAX >> 1) + 2, 0));
   EXPECT_EQ(32, get_lg(UINT32_MAX, 0));
   EXPECT_EQ(-1, get_lg(UINT32_C(1), -1));
   EXPECT_EQ(-1, get_lg(UINT32_C(2), -2));
   EXPECT_EQ(INT32_MIN, get_lg(UINT32_C(0), -1));
   EXPECT_EQ(INT32_MIN, get_lg(UINT32_C(0), 0));
   EXPECT_EQ(INT32_MIN, get_lg(UINT32_C(0), 1));

   EXPECT_EQ(0, get_lg(UINT64_C(1), 0));
   EXPECT_EQ(1, get_lg(UINT64_C(1), 1));
   EXPECT_EQ(1, get_lg(UINT64_C(2), 0));
   EXPECT_EQ(3, get_lg(UINT64_C(1), 3));
   EXPECT_EQ(3, get_lg(UINT64_C(7), 0));
   EXPECT_EQ(3, get_lg(UINT64_C(8), 0));
   EXPECT_EQ(3, get_lg(UINT64_C(9), 0));
   EXPECT_EQ(3, get_lg(UINT64_C(64), -3));
   EXPECT_EQ(63, get_lg((UINT64_MAX >> 1) + 2, 0));
   EXPECT_EQ(64, get_lg(UINT64_MAX, 0));
   EXPECT_EQ(-1, get_lg(UINT64_C(1), -1));
   EXPECT_EQ(-1, get_lg(UINT64_C(2), -2));
   EXPECT_EQ(INT32_MIN, get_lg(UINT64_C(0), -1));
   EXPECT_EQ(INT32_MIN, get_lg(UINT64_C(0), 0));
   EXPECT_EQ(INT32_MIN, get_lg(UINT64_C(0), 1));
}

TEST(ScaledNumberHelpersTest, testGetLgFloor)
{
   EXPECT_EQ(0, get_lg_floor(UINT32_C(1), 0));
   EXPECT_EQ(1, get_lg_floor(UINT32_C(1), 1));
   EXPECT_EQ(1, get_lg_floor(UINT32_C(2), 0));
   EXPECT_EQ(2, get_lg_floor(UINT32_C(7), 0));
   EXPECT_EQ(3, get_lg_floor(UINT32_C(1), 3));
   EXPECT_EQ(3, get_lg_floor(UINT32_C(8), 0));
   EXPECT_EQ(3, get_lg_floor(UINT32_C(9), 0));
   EXPECT_EQ(3, get_lg_floor(UINT32_C(64), -3));
   EXPECT_EQ(31, get_lg_floor((UINT32_MAX >> 1) + 2, 0));
   EXPECT_EQ(31, get_lg_floor(UINT32_MAX, 0));
   EXPECT_EQ(INT32_MIN, get_lg_floor(UINT32_C(0), -1));
   EXPECT_EQ(INT32_MIN, get_lg_floor(UINT32_C(0), 0));
   EXPECT_EQ(INT32_MIN, get_lg_floor(UINT32_C(0), 1));

   EXPECT_EQ(0, get_lg_floor(UINT64_C(1), 0));
   EXPECT_EQ(1, get_lg_floor(UINT64_C(1), 1));
   EXPECT_EQ(1, get_lg_floor(UINT64_C(2), 0));
   EXPECT_EQ(2, get_lg_floor(UINT64_C(7), 0));
   EXPECT_EQ(3, get_lg_floor(UINT64_C(1), 3));
   EXPECT_EQ(3, get_lg_floor(UINT64_C(8), 0));
   EXPECT_EQ(3, get_lg_floor(UINT64_C(9), 0));
   EXPECT_EQ(3, get_lg_floor(UINT64_C(64), -3));
   EXPECT_EQ(63, get_lg_floor((UINT64_MAX >> 1) + 2, 0));
   EXPECT_EQ(63, get_lg_floor(UINT64_MAX, 0));
   EXPECT_EQ(INT32_MIN, get_lg_floor(UINT64_C(0), -1));
   EXPECT_EQ(INT32_MIN, get_lg_floor(UINT64_C(0), 0));
   EXPECT_EQ(INT32_MIN, get_lg_floor(UINT64_C(0), 1));
}

TEST(ScaledNumberHelpersTest, testGetLgCeiling)
{
   EXPECT_EQ(0, get_lg_ceiling(UINT32_C(1), 0));
   EXPECT_EQ(1, get_lg_ceiling(UINT32_C(1), 1));
   EXPECT_EQ(1, get_lg_ceiling(UINT32_C(2), 0));
   EXPECT_EQ(3, get_lg_ceiling(UINT32_C(1), 3));
   EXPECT_EQ(3, get_lg_ceiling(UINT32_C(7), 0));
   EXPECT_EQ(3, get_lg_ceiling(UINT32_C(8), 0));
   EXPECT_EQ(3, get_lg_ceiling(UINT32_C(64), -3));
   EXPECT_EQ(4, get_lg_ceiling(UINT32_C(9), 0));
   EXPECT_EQ(32, get_lg_ceiling(UINT32_MAX, 0));
   EXPECT_EQ(32, get_lg_ceiling((UINT32_MAX >> 1) + 2, 0));
   EXPECT_EQ(INT32_MIN, get_lg_ceiling(UINT32_C(0), -1));
   EXPECT_EQ(INT32_MIN, get_lg_ceiling(UINT32_C(0), 0));
   EXPECT_EQ(INT32_MIN, get_lg_ceiling(UINT32_C(0), 1));

   EXPECT_EQ(0, get_lg_ceiling(UINT64_C(1), 0));
   EXPECT_EQ(1, get_lg_ceiling(UINT64_C(1), 1));
   EXPECT_EQ(1, get_lg_ceiling(UINT64_C(2), 0));
   EXPECT_EQ(3, get_lg_ceiling(UINT64_C(1), 3));
   EXPECT_EQ(3, get_lg_ceiling(UINT64_C(7), 0));
   EXPECT_EQ(3, get_lg_ceiling(UINT64_C(8), 0));
   EXPECT_EQ(3, get_lg_ceiling(UINT64_C(64), -3));
   EXPECT_EQ(4, get_lg_ceiling(UINT64_C(9), 0));
   EXPECT_EQ(64, get_lg_ceiling((UINT64_MAX >> 1) + 2, 0));
   EXPECT_EQ(64, get_lg_ceiling(UINT64_MAX, 0));
   EXPECT_EQ(INT32_MIN, get_lg_ceiling(UINT64_C(0), -1));
   EXPECT_EQ(INT32_MIN, get_lg_ceiling(UINT64_C(0), 0));
   EXPECT_EQ(INT32_MIN, get_lg_ceiling(UINT64_C(0), 1));
}

TEST(ScaledNumberHelpersTest, testCompare)
{
   EXPECT_EQ(0, compare(UINT32_C(0), 0, UINT32_C(0), 1));
   EXPECT_EQ(0, compare(UINT32_C(0), 0, UINT32_C(0), -10));
   EXPECT_EQ(0, compare(UINT32_C(0), 0, UINT32_C(0), 20));
   EXPECT_EQ(0, compare(UINT32_C(8), 0, UINT32_C(64), -3));
   EXPECT_EQ(0, compare(UINT32_C(8), 0, UINT32_C(32), -2));
   EXPECT_EQ(0, compare(UINT32_C(8), 0, UINT32_C(16), -1));
   EXPECT_EQ(0, compare(UINT32_C(8), 0, UINT32_C(8), 0));
   EXPECT_EQ(0, compare(UINT32_C(8), 0, UINT32_C(4), 1));
   EXPECT_EQ(0, compare(UINT32_C(8), 0, UINT32_C(2), 2));
   EXPECT_EQ(0, compare(UINT32_C(8), 0, UINT32_C(1), 3));
   EXPECT_EQ(-1, compare(UINT32_C(0), 0, UINT32_C(1), 3));
   EXPECT_EQ(-1, compare(UINT32_C(7), 0, UINT32_C(1), 3));
   EXPECT_EQ(-1, compare(UINT32_C(7), 0, UINT32_C(64), -3));
   EXPECT_EQ(1, compare(UINT32_C(9), 0, UINT32_C(1), 3));
   EXPECT_EQ(1, compare(UINT32_C(9), 0, UINT32_C(64), -3));
   EXPECT_EQ(1, compare(UINT32_C(9), 0, UINT32_C(0), 0));

   EXPECT_EQ(0, compare(UINT64_C(0), 0, UINT64_C(0), 1));
   EXPECT_EQ(0, compare(UINT64_C(0), 0, UINT64_C(0), -10));
   EXPECT_EQ(0, compare(UINT64_C(0), 0, UINT64_C(0), 20));
   EXPECT_EQ(0, compare(UINT64_C(8), 0, UINT64_C(64), -3));
   EXPECT_EQ(0, compare(UINT64_C(8), 0, UINT64_C(32), -2));
   EXPECT_EQ(0, compare(UINT64_C(8), 0, UINT64_C(16), -1));
   EXPECT_EQ(0, compare(UINT64_C(8), 0, UINT64_C(8), 0));
   EXPECT_EQ(0, compare(UINT64_C(8), 0, UINT64_C(4), 1));
   EXPECT_EQ(0, compare(UINT64_C(8), 0, UINT64_C(2), 2));
   EXPECT_EQ(0, compare(UINT64_C(8), 0, UINT64_C(1), 3));
   EXPECT_EQ(-1, compare(UINT64_C(0), 0, UINT64_C(1), 3));
   EXPECT_EQ(-1, compare(UINT64_C(7), 0, UINT64_C(1), 3));
   EXPECT_EQ(-1, compare(UINT64_C(7), 0, UINT64_C(64), -3));
   EXPECT_EQ(1, compare(UINT64_C(9), 0, UINT64_C(1), 3));
   EXPECT_EQ(1, compare(UINT64_C(9), 0, UINT64_C(64), -3));
   EXPECT_EQ(1, compare(UINT64_C(9), 0, UINT64_C(0), 0));
   EXPECT_EQ(-1, compare(UINT64_MAX, 0, UINT64_C(1), 64));
}

TEST(ScaledNumberHelpersTest, testMatchScales)
{
#define MATCH_SCALES(T, LDIn, LSIn, RDIn, RSIn, LDOut, RDOut, SOut)            \
   do {                                                                         \
   T LDx = LDIn;                                                              \
   T RDx = RDIn;                                                              \
   T LDy = LDOut;                                                             \
   T RDy = RDOut;                                                             \
   int16_t LSx = LSIn;                                                        \
   int16_t RSx = RSIn;                                                        \
   int16_t Sy = SOut;                                                         \
   \
   EXPECT_EQ(SOut, match_scales(LDx, LSx, RDx, RSx));                          \
   EXPECT_EQ(LDy, LDx);                                                       \
   EXPECT_EQ(RDy, RDx);                                                       \
   if (LDy) {                                                                 \
   EXPECT_EQ(Sy, LSx);                                                      \
}                                                                          \
   if (RDy) {                                                                 \
   EXPECT_EQ(Sy, RSx);                                                      \
}                                                                          \
} while (false)

   MATCH_SCALES(uint32_t, 0, 0, 0, 0, 0, 0, 0);
   MATCH_SCALES(uint32_t, 0, 50, 7, 1, 0, 7, 1);
   MATCH_SCALES(uint32_t, UINT32_C(1) << 31, 1, 9, 0, UINT32_C(1) << 31, 4, 1);
   MATCH_SCALES(uint32_t, UINT32_C(1) << 31, 2, 9, 0, UINT32_C(1) << 31, 2, 2);
   MATCH_SCALES(uint32_t, UINT32_C(1) << 31, 3, 9, 0, UINT32_C(1) << 31, 1, 3);
   MATCH_SCALES(uint32_t, UINT32_C(1) << 31, 4, 9, 0, UINT32_C(1) << 31, 0, 4);
   MATCH_SCALES(uint32_t, UINT32_C(1) << 30, 4, 9, 0, UINT32_C(1) << 31, 1, 3);
   MATCH_SCALES(uint32_t, UINT32_C(1) << 29, 4, 9, 0, UINT32_C(1) << 31, 2, 2);
   MATCH_SCALES(uint32_t, UINT32_C(1) << 28, 4, 9, 0, UINT32_C(1) << 31, 4, 1);
   MATCH_SCALES(uint32_t, UINT32_C(1) << 27, 4, 9, 0, UINT32_C(1) << 31, 9, 0);
   MATCH_SCALES(uint32_t, 7, 1, 0, 50, 7, 0, 1);
   MATCH_SCALES(uint32_t, 9, 0, UINT32_C(1) << 31, 1, 4, UINT32_C(1) << 31, 1);
   MATCH_SCALES(uint32_t, 9, 0, UINT32_C(1) << 31, 2, 2, UINT32_C(1) << 31, 2);
   MATCH_SCALES(uint32_t, 9, 0, UINT32_C(1) << 31, 3, 1, UINT32_C(1) << 31, 3);
   MATCH_SCALES(uint32_t, 9, 0, UINT32_C(1) << 31, 4, 0, UINT32_C(1) << 31, 4);
   MATCH_SCALES(uint32_t, 9, 0, UINT32_C(1) << 30, 4, 1, UINT32_C(1) << 31, 3);
   MATCH_SCALES(uint32_t, 9, 0, UINT32_C(1) << 29, 4, 2, UINT32_C(1) << 31, 2);
   MATCH_SCALES(uint32_t, 9, 0, UINT32_C(1) << 28, 4, 4, UINT32_C(1) << 31, 1);
   MATCH_SCALES(uint32_t, 9, 0, UINT32_C(1) << 27, 4, 9, UINT32_C(1) << 31, 0);

   MATCH_SCALES(uint64_t, 0, 0, 0, 0, 0, 0, 0);
   MATCH_SCALES(uint64_t, 0, 100, 7, 1, 0, 7, 1);
   MATCH_SCALES(uint64_t, UINT64_C(1) << 63, 1, 9, 0, UINT64_C(1) << 63, 4, 1);
   MATCH_SCALES(uint64_t, UINT64_C(1) << 63, 2, 9, 0, UINT64_C(1) << 63, 2, 2);
   MATCH_SCALES(uint64_t, UINT64_C(1) << 63, 3, 9, 0, UINT64_C(1) << 63, 1, 3);
   MATCH_SCALES(uint64_t, UINT64_C(1) << 63, 4, 9, 0, UINT64_C(1) << 63, 0, 4);
   MATCH_SCALES(uint64_t, UINT64_C(1) << 62, 4, 9, 0, UINT64_C(1) << 63, 1, 3);
   MATCH_SCALES(uint64_t, UINT64_C(1) << 61, 4, 9, 0, UINT64_C(1) << 63, 2, 2);
   MATCH_SCALES(uint64_t, UINT64_C(1) << 60, 4, 9, 0, UINT64_C(1) << 63, 4, 1);
   MATCH_SCALES(uint64_t, UINT64_C(1) << 59, 4, 9, 0, UINT64_C(1) << 63, 9, 0);
   MATCH_SCALES(uint64_t, 7, 1, 0, 100, 7, 0, 1);
   MATCH_SCALES(uint64_t, 9, 0, UINT64_C(1) << 63, 1, 4, UINT64_C(1) << 63, 1);
   MATCH_SCALES(uint64_t, 9, 0, UINT64_C(1) << 63, 2, 2, UINT64_C(1) << 63, 2);
   MATCH_SCALES(uint64_t, 9, 0, UINT64_C(1) << 63, 3, 1, UINT64_C(1) << 63, 3);
   MATCH_SCALES(uint64_t, 9, 0, UINT64_C(1) << 63, 4, 0, UINT64_C(1) << 63, 4);
   MATCH_SCALES(uint64_t, 9, 0, UINT64_C(1) << 62, 4, 1, UINT64_C(1) << 63, 3);
   MATCH_SCALES(uint64_t, 9, 0, UINT64_C(1) << 61, 4, 2, UINT64_C(1) << 63, 2);
   MATCH_SCALES(uint64_t, 9, 0, UINT64_C(1) << 60, 4, 4, UINT64_C(1) << 63, 1);
   MATCH_SCALES(uint64_t, 9, 0, UINT64_C(1) << 59, 4, 9, UINT64_C(1) << 63, 0);
}

TEST(ScaledNumberHelpersTest, testGetSum)
{
   // Zero.
   EXPECT_EQ(SP32(1, 0), get_sum32(0, 0, 1, 0));
   EXPECT_EQ(SP32(8, -3), get_sum32(0, 0, 8, -3));
   EXPECT_EQ(SP32(UINT32_MAX, 0), get_sum32(0, 0, UINT32_MAX, 0));

   // Basic.
   EXPECT_EQ(SP32(2, 0), get_sum32(1, 0, 1, 0));
   EXPECT_EQ(SP32(3, 0), get_sum32(1, 0, 2, 0));
   EXPECT_EQ(SP32(67, 0), get_sum32(7, 0, 60, 0));

   // Different scales.
   EXPECT_EQ(SP32(3, 0), get_sum32(1, 0, 1, 1));
   EXPECT_EQ(SP32(4, 0), get_sum32(2, 0, 1, 1));

   // Loss of precision.
   EXPECT_EQ(SP32(UINT32_C(1) << 31, 1), get_sum32(1, 32, 1, 0));
   EXPECT_EQ(SP32(UINT32_C(1) << 31, -31), get_sum32(1, -32, 1, 0));

   // Not quite loss of precision.
   EXPECT_EQ(SP32((UINT32_C(1) << 31) + 1, 1), get_sum32(1, 32, 1, 1));
   EXPECT_EQ(SP32((UINT32_C(1) << 31) + 1, -32), get_sum32(1, -32, 1, -1));

   // Overflow.
   EXPECT_EQ(SP32(UINT32_C(1) << 31, 1), get_sum32(1, 0, UINT32_MAX, 0));

   // Reverse operand order.
   EXPECT_EQ(SP32(1, 0), get_sum32(1, 0, 0, 0));
   EXPECT_EQ(SP32(8, -3), get_sum32(8, -3, 0, 0));
   EXPECT_EQ(SP32(UINT32_MAX, 0), get_sum32(UINT32_MAX, 0, 0, 0));
   EXPECT_EQ(SP32(3, 0), get_sum32(2, 0, 1, 0));
   EXPECT_EQ(SP32(67, 0), get_sum32(60, 0, 7, 0));
   EXPECT_EQ(SP32(3, 0), get_sum32(1, 1, 1, 0));
   EXPECT_EQ(SP32(4, 0), get_sum32(1, 1, 2, 0));
   EXPECT_EQ(SP32(UINT32_C(1) << 31, 1), get_sum32(1, 0, 1, 32));
   EXPECT_EQ(SP32(UINT32_C(1) << 31, -31), get_sum32(1, 0, 1, -32));
   EXPECT_EQ(SP32((UINT32_C(1) << 31) + 1, 1), get_sum32(1, 1, 1, 32));
   EXPECT_EQ(SP32((UINT32_C(1) << 31) + 1, -32), get_sum32(1, -1, 1, -32));
   EXPECT_EQ(SP32(UINT32_C(1) << 31, 1), get_sum32(UINT32_MAX, 0, 1, 0));

   // Zero.
   EXPECT_EQ(SP64(1, 0), get_sum64(0, 0, 1, 0));
   EXPECT_EQ(SP64(8, -3), get_sum64(0, 0, 8, -3));
   EXPECT_EQ(SP64(UINT64_MAX, 0), get_sum64(0, 0, UINT64_MAX, 0));

   // Basic.
   EXPECT_EQ(SP64(2, 0), get_sum64(1, 0, 1, 0));
   EXPECT_EQ(SP64(3, 0), get_sum64(1, 0, 2, 0));
   EXPECT_EQ(SP64(67, 0), get_sum64(7, 0, 60, 0));

   // Different scales.
   EXPECT_EQ(SP64(3, 0), get_sum64(1, 0, 1, 1));
   EXPECT_EQ(SP64(4, 0), get_sum64(2, 0, 1, 1));

   // Loss of precision.
   EXPECT_EQ(SP64(UINT64_C(1) << 63, 1), get_sum64(1, 64, 1, 0));
   EXPECT_EQ(SP64(UINT64_C(1) << 63, -63), get_sum64(1, -64, 1, 0));

   // Not quite loss of precision.
   EXPECT_EQ(SP64((UINT64_C(1) << 63) + 1, 1), get_sum64(1, 64, 1, 1));
   EXPECT_EQ(SP64((UINT64_C(1) << 63) + 1, -64), get_sum64(1, -64, 1, -1));

   // Overflow.
   EXPECT_EQ(SP64(UINT64_C(1) << 63, 1), get_sum64(1, 0, UINT64_MAX, 0));

   // Reverse operand order.
   EXPECT_EQ(SP64(1, 0), get_sum64(1, 0, 0, 0));
   EXPECT_EQ(SP64(8, -3), get_sum64(8, -3, 0, 0));
   EXPECT_EQ(SP64(UINT64_MAX, 0), get_sum64(UINT64_MAX, 0, 0, 0));
   EXPECT_EQ(SP64(3, 0), get_sum64(2, 0, 1, 0));
   EXPECT_EQ(SP64(67, 0), get_sum64(60, 0, 7, 0));
   EXPECT_EQ(SP64(3, 0), get_sum64(1, 1, 1, 0));
   EXPECT_EQ(SP64(4, 0), get_sum64(1, 1, 2, 0));
   EXPECT_EQ(SP64(UINT64_C(1) << 63, 1), get_sum64(1, 0, 1, 64));
   EXPECT_EQ(SP64(UINT64_C(1) << 63, -63), get_sum64(1, 0, 1, -64));
   EXPECT_EQ(SP64((UINT64_C(1) << 63) + 1, 1), get_sum64(1, 1, 1, 64));
   EXPECT_EQ(SP64((UINT64_C(1) << 63) + 1, -64), get_sum64(1, -1, 1, -64));
   EXPECT_EQ(SP64(UINT64_C(1) << 63, 1), get_sum64(UINT64_MAX, 0, 1, 0));
}

TEST(ScaledNumberHelpersTest, testGetDifference)
{
   // Basic.
   EXPECT_EQ(SP32(0, 0), get_difference32(1, 0, 1, 0));
   EXPECT_EQ(SP32(1, 0), get_difference32(2, 0, 1, 0));
   EXPECT_EQ(SP32(53, 0), get_difference32(60, 0, 7, 0));

   // Equals "0", different scales.
   EXPECT_EQ(SP32(0, 0), get_difference32(2, 0, 1, 1));

   // Subtract "0".
   EXPECT_EQ(SP32(1, 0), get_difference32(1, 0, 0, 0));
   EXPECT_EQ(SP32(8, -3), get_difference32(8, -3, 0, 0));
   EXPECT_EQ(SP32(UINT32_MAX, 0), get_difference32(UINT32_MAX, 0, 0, 0));

   // Loss of precision.
   EXPECT_EQ(SP32((UINT32_C(1) << 31) + 1, 1),
             get_difference32((UINT32_C(1) << 31) + 1, 1, 1, 0));
   EXPECT_EQ(SP32((UINT32_C(1) << 31) + 1, -31),
             get_difference32((UINT32_C(1) << 31) + 1, -31, 1, -32));

   // Not quite loss of precision.
   EXPECT_EQ(SP32(UINT32_MAX, 0), get_difference32(1, 32, 1, 0));
   EXPECT_EQ(SP32(UINT32_MAX, -32), get_difference32(1, 0, 1, -32));

   // Saturate to "0".
   EXPECT_EQ(SP32(0, 0), get_difference32(0, 0, 1, 0));
   EXPECT_EQ(SP32(0, 0), get_difference32(0, 0, 8, -3));
   EXPECT_EQ(SP32(0, 0), get_difference32(0, 0, UINT32_MAX, 0));
   EXPECT_EQ(SP32(0, 0), get_difference32(7, 0, 60, 0));
   EXPECT_EQ(SP32(0, 0), get_difference32(1, 0, 1, 1));
   EXPECT_EQ(SP32(0, 0), get_difference32(1, -32, 1, 0));
   EXPECT_EQ(SP32(0, 0), get_difference32(1, -32, 1, -1));

   // Regression tests for cases that failed during bringup.
   EXPECT_EQ(SP32(UINT32_C(1) << 26, -31),
             get_difference32(1, 0, UINT32_C(31) << 27, -32));

   // Basic.
   EXPECT_EQ(SP64(0, 0), get_difference64(1, 0, 1, 0));
   EXPECT_EQ(SP64(1, 0), get_difference64(2, 0, 1, 0));
   EXPECT_EQ(SP64(53, 0), get_difference64(60, 0, 7, 0));

   // Equals "0", different scales.
   EXPECT_EQ(SP64(0, 0), get_difference64(2, 0, 1, 1));

   // Subtract "0".
   EXPECT_EQ(SP64(1, 0), get_difference64(1, 0, 0, 0));
   EXPECT_EQ(SP64(8, -3), get_difference64(8, -3, 0, 0));
   EXPECT_EQ(SP64(UINT64_MAX, 0), get_difference64(UINT64_MAX, 0, 0, 0));

   // Loss of precision.
   EXPECT_EQ(SP64((UINT64_C(1) << 63) + 1, 1),
             get_difference64((UINT64_C(1) << 63) + 1, 1, 1, 0));
   EXPECT_EQ(SP64((UINT64_C(1) << 63) + 1, -63),
             get_difference64((UINT64_C(1) << 63) + 1, -63, 1, -64));

   // Not quite loss of precision.
   EXPECT_EQ(SP64(UINT64_MAX, 0), get_difference64(1, 64, 1, 0));
   EXPECT_EQ(SP64(UINT64_MAX, -64), get_difference64(1, 0, 1, -64));

   // Saturate to "0".
   EXPECT_EQ(SP64(0, 0), get_difference64(0, 0, 1, 0));
   EXPECT_EQ(SP64(0, 0), get_difference64(0, 0, 8, -3));
   EXPECT_EQ(SP64(0, 0), get_difference64(0, 0, UINT64_MAX, 0));
   EXPECT_EQ(SP64(0, 0), get_difference64(7, 0, 60, 0));
   EXPECT_EQ(SP64(0, 0), get_difference64(1, 0, 1, 1));
   EXPECT_EQ(SP64(0, 0), get_difference64(1, -64, 1, 0));
   EXPECT_EQ(SP64(0, 0), get_difference64(1, -64, 1, -1));
}

TEST(ScaledNumberHelpersTest, testArithmeticOperators)
{
   EXPECT_EQ(ScaledNumber<uint32_t>(10, 0),
             ScaledNumber<uint32_t>(1, 3) + ScaledNumber<uint32_t>(1, 1));
   EXPECT_EQ(ScaledNumber<uint32_t>(6, 0),
             ScaledNumber<uint32_t>(1, 3) - ScaledNumber<uint32_t>(1, 1));
   EXPECT_EQ(ScaledNumber<uint32_t>(2, 3),
             ScaledNumber<uint32_t>(1, 3) * ScaledNumber<uint32_t>(1, 1));
   EXPECT_EQ(ScaledNumber<uint32_t>(1, 2),
             ScaledNumber<uint32_t>(1, 3) / ScaledNumber<uint32_t>(1, 1));
   EXPECT_EQ(ScaledNumber<uint32_t>(1, 2), ScaledNumber<uint32_t>(1, 3) >> 1);
   EXPECT_EQ(ScaledNumber<uint32_t>(1, 4), ScaledNumber<uint32_t>(1, 3) << 1);

   EXPECT_EQ(ScaledNumber<uint64_t>(10, 0),
             ScaledNumber<uint64_t>(1, 3) + ScaledNumber<uint64_t>(1, 1));
   EXPECT_EQ(ScaledNumber<uint64_t>(6, 0),
             ScaledNumber<uint64_t>(1, 3) - ScaledNumber<uint64_t>(1, 1));
   EXPECT_EQ(ScaledNumber<uint64_t>(2, 3),
             ScaledNumber<uint64_t>(1, 3) * ScaledNumber<uint64_t>(1, 1));
   EXPECT_EQ(ScaledNumber<uint64_t>(1, 2),
             ScaledNumber<uint64_t>(1, 3) / ScaledNumber<uint64_t>(1, 1));
   EXPECT_EQ(ScaledNumber<uint64_t>(1, 2), ScaledNumber<uint64_t>(1, 3) >> 1);
   EXPECT_EQ(ScaledNumber<uint64_t>(1, 4), ScaledNumber<uint64_t>(1, 3) << 1);
}

TEST(ScaledNumberHelpersTest, testToIntBug)
{
   ScaledNumber<uint32_t> n(1, 0);
   EXPECT_EQ(1u, (n * n).toInt<uint32_t>());
}

} // anonymous namespace
