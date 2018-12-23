// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/13.

#include "polarphp/utils/MathExtras.h"
#include "gtest/gtest.h"

using namespace polar;
using namespace polar::utils;

namespace {

TEST(MathExtrasTest, testCountTrailingZeros)
{
   uint8_t Z8 = 0;
   uint16_t Z16 = 0;
   uint32_t Z32 = 0;
   uint64_t Z64 = 0;
   EXPECT_EQ(8u, count_trailing_zeros(Z8));
   EXPECT_EQ(16u, count_trailing_zeros(Z16));
   EXPECT_EQ(32u, count_trailing_zeros(Z32));
   EXPECT_EQ(64u, count_trailing_zeros(Z64));

   uint8_t NZ8 = 42;
   uint16_t NZ16 = 42;
   uint32_t NZ32 = 42;
   uint64_t NZ64 = 42;
   EXPECT_EQ(1u, count_trailing_zeros(NZ8));
   EXPECT_EQ(1u, count_trailing_zeros(NZ16));
   EXPECT_EQ(1u, count_trailing_zeros(NZ32));
   EXPECT_EQ(1u, count_trailing_zeros(NZ64));
}

TEST(MathExtrasTest, testCountLeadingZeros)
{
   uint8_t Z8 = 0;
   uint16_t Z16 = 0;
   uint32_t Z32 = 0;
   uint64_t Z64 = 0;
   EXPECT_EQ(8u, count_leading_zeros(Z8));
   EXPECT_EQ(16u, count_leading_zeros(Z16));
   EXPECT_EQ(32u, count_leading_zeros(Z32));
   EXPECT_EQ(64u, count_leading_zeros(Z64));

   uint8_t NZ8 = 42;
   uint16_t NZ16 = 42;
   uint32_t NZ32 = 42;
   uint64_t NZ64 = 42;
   EXPECT_EQ(2u, count_leading_zeros(NZ8));
   EXPECT_EQ(10u, count_leading_zeros(NZ16));
   EXPECT_EQ(26u, count_leading_zeros(NZ32));
   EXPECT_EQ(58u, count_leading_zeros(NZ64));

   EXPECT_EQ(8u, count_leading_zeros(0x00F000FFu));
   EXPECT_EQ(8u, count_leading_zeros(0x00F12345u));
   for (unsigned i = 0; i <= 30; ++i) {
      EXPECT_EQ(31 - i, count_leading_zeros(1u << i));
   }

   EXPECT_EQ(8u, count_leading_zeros(0x00F1234500F12345ULL));
   EXPECT_EQ(1u, count_leading_zeros(1ULL << 62));
   for (unsigned i = 0; i <= 62; ++i) {
      EXPECT_EQ(63 - i, count_leading_zeros(1ULL << i));
   }
}

TEST(MathExtrasTest, testOnesMask)
{
   EXPECT_EQ(0U, mask_leading_ones<uint8_t>(0));
   EXPECT_EQ(0U, mask_trailing_ones<uint8_t>(0));
   EXPECT_EQ(0U, mask_leading_ones<uint16_t>(0));
   EXPECT_EQ(0U, mask_trailing_ones<uint16_t>(0));
   EXPECT_EQ(0U, mask_leading_ones<uint32_t>(0));
   EXPECT_EQ(0U, mask_trailing_ones<uint32_t>(0));
   EXPECT_EQ(0U, mask_leading_ones<uint64_t>(0));
   EXPECT_EQ(0U, mask_trailing_ones<uint64_t>(0));

   EXPECT_EQ(0x00000003U, mask_trailing_ones<uint32_t>(2U));
   EXPECT_EQ(0xC0000000U, mask_leading_ones<uint32_t>(2U));

   EXPECT_EQ(0x000007FFU, mask_trailing_ones<uint32_t>(11U));
   EXPECT_EQ(0xFFE00000U, mask_leading_ones<uint32_t>(11U));

   EXPECT_EQ(0xFFFFFFFFU, mask_trailing_ones<uint32_t>(32U));
   EXPECT_EQ(0xFFFFFFFFU, mask_leading_ones<uint32_t>(32U));
   EXPECT_EQ(0xFFFFFFFFFFFFFFFFULL, mask_trailing_ones<uint64_t>(64U));
   EXPECT_EQ(0xFFFFFFFFFFFFFFFFULL, mask_leading_ones<uint64_t>(64U));

   EXPECT_EQ(0x0000FFFFFFFFFFFFULL, mask_trailing_ones<uint64_t>(48U));
   EXPECT_EQ(0xFFFFFFFFFFFF0000ULL, mask_leading_ones<uint64_t>(48U));
}

TEST(MathExtrasTest, testFindFirstSet)
{
   uint8_t Z8 = 0;
   uint16_t Z16 = 0;
   uint32_t Z32 = 0;
   uint64_t Z64 = 0;
   EXPECT_EQ(0xFFULL, find_first_set(Z8));
   EXPECT_EQ(0xFFFFULL, find_first_set(Z16));
   EXPECT_EQ(0xFFFFFFFFULL, find_first_set(Z32));
   EXPECT_EQ(0xFFFFFFFFFFFFFFFFULL, find_first_set(Z64));

   uint8_t NZ8 = 42;
   uint16_t NZ16 = 42;
   uint32_t NZ32 = 42;
   uint64_t NZ64 = 42;
   EXPECT_EQ(1u, find_first_set(NZ8));
   EXPECT_EQ(1u, find_first_set(NZ16));
   EXPECT_EQ(1u, find_first_set(NZ32));
   EXPECT_EQ(1u, find_first_set(NZ64));
}

TEST(MathExtrasTest, testFindLastSet)
{
   uint8_t Z8 = 0;
   uint16_t Z16 = 0;
   uint32_t Z32 = 0;
   uint64_t Z64 = 0;
   EXPECT_EQ(0xFFULL, find_last_set(Z8));
   EXPECT_EQ(0xFFFFULL, find_last_set(Z16));
   EXPECT_EQ(0xFFFFFFFFULL, find_last_set(Z32));
   EXPECT_EQ(0xFFFFFFFFFFFFFFFFULL, find_last_set(Z64));

   uint8_t NZ8 = 42;
   uint16_t NZ16 = 42;
   uint32_t NZ32 = 42;
   uint64_t NZ64 = 42;
   EXPECT_EQ(5u, find_last_set(NZ8));
   EXPECT_EQ(5u, find_last_set(NZ16));
   EXPECT_EQ(5u, find_last_set(NZ32));
   EXPECT_EQ(5u, find_last_set(NZ64));
}

TEST(MathExtrasTest, testIsIntN)
{
   EXPECT_TRUE(is_int_n(16, 32767));
   EXPECT_FALSE(is_int_n(16, 32768));
}

TEST(MathExtrasTest, testIsUIntN)
{
   EXPECT_TRUE(is_uint_n(16, 65535));
   EXPECT_FALSE(is_uint_n(16, 65536));
   EXPECT_TRUE(is_uint_n(1, 0));
   EXPECT_TRUE(is_uint_n(6, 63));
}

TEST(MathExtrasTest, testMaxIntN)
{
   EXPECT_EQ(32767, max_int_n(16));
   EXPECT_EQ(2147483647, max_int_n(32));
   EXPECT_EQ(std::numeric_limits<int32_t>::max(), max_int_n(32));
   EXPECT_EQ(std::numeric_limits<int64_t>::max(), max_int_n(64));
}

TEST(MathExtrasTest, testMinIntN)
{
   EXPECT_EQ(-32768LL, min_int_n(16));
   EXPECT_EQ(-64LL, min_int_n(7));
   EXPECT_EQ(std::numeric_limits<int32_t>::min(), min_int_n(32));
   EXPECT_EQ(std::numeric_limits<int64_t>::min(), min_int_n(64));
}

TEST(MathExtrasTest, testMaxUIntN)
{
   EXPECT_EQ(0xffffULL, max_uint_n(16));
   EXPECT_EQ(0xffffffffULL, max_uint_n(32));
   EXPECT_EQ(0xffffffffffffffffULL, max_uint_n(64));
   EXPECT_EQ(1ULL, max_uint_n(1));
   EXPECT_EQ(0x0fULL, max_uint_n(4));
}

TEST(MathExtrasTest, testReverseBits)
{
   uint8_t NZ8 = 42;
   uint16_t NZ16 = 42;
   uint32_t NZ32 = 42;
   uint64_t NZ64 = 42;
   EXPECT_EQ(0x54ULL, reverse_bits(NZ8));
   EXPECT_EQ(0x5400ULL, reverse_bits(NZ16));
   EXPECT_EQ(0x54000000ULL, reverse_bits(NZ32));
   EXPECT_EQ(0x5400000000000000ULL, reverse_bits(NZ64));
}

TEST(MathExtrasTest, testIsPowerOf2_32)
{
   EXPECT_FALSE(is_power_of_two32(0));
   EXPECT_TRUE(is_power_of_two32(1 << 6));
   EXPECT_TRUE(is_power_of_two32(1 << 12));
   EXPECT_FALSE(is_power_of_two32((1 << 19) + 3));
   EXPECT_FALSE(is_power_of_two32(0xABCDEF0));
}

TEST(MathExtrasTest, testIsPowerOf2_64)
{
   EXPECT_FALSE(is_power_of_two64(0));
   EXPECT_TRUE(is_power_of_two64(1LL << 46));
   EXPECT_TRUE(is_power_of_two64(1LL << 12));
   EXPECT_FALSE(is_power_of_two64((1LL << 53) + 3));
   EXPECT_FALSE(is_power_of_two64(0xABCDEF0ABCDEF0LL));
}

TEST(MathExtrasTest, testPowerOf2Ceil)
{
   EXPECT_EQ(0U, power_of_two_ceil(0U));
   EXPECT_EQ(8U, power_of_two_ceil(8U));
   EXPECT_EQ(8U, power_of_two_ceil(7U));
}

TEST(MathExtrasTest, testPowerOf2Floor)
{
   EXPECT_EQ(0U, power_of_two_floor(0U));
   EXPECT_EQ(8U, power_of_two_floor(8U));
   EXPECT_EQ(4U, power_of_two_floor(7U));
}

TEST(MathExtrasTest, testByteSwap_32)
{
   EXPECT_EQ(0x44332211u, byte_swap32(0x11223344));
   EXPECT_EQ(0xDDCCBBAAu, byte_swap32(0xAABBCCDD));
}

TEST(MathExtrasTest, testByteSwap_64)
{
   EXPECT_EQ(0x8877665544332211ULL, byte_swap64(0x1122334455667788LL));
   EXPECT_EQ(0x1100FFEEDDCCBBAAULL, byte_swap64(0xAABBCCDDEEFF0011LL));
}

TEST(MathExtrasTest, testCountLeadingOnes)
{
   for (int i = 30; i >= 0; --i) {
      // Start with all ones and unset some bit.
      EXPECT_EQ(31u - i, count_leading_ones(0xFFFFFFFF ^ (1 << i)));
   }
   for (int i = 62; i >= 0; --i) {
      // Start with all ones and unset some bit.
      EXPECT_EQ(63u - i, count_leading_ones(0xFFFFFFFFFFFFFFFFULL ^ (1LL << i)));
   }
   for (int i = 30; i >= 0; --i) {
      // Start with all ones and unset some bit.
      EXPECT_EQ(31u - i, count_leading_ones(0xFFFFFFFF ^ (1 << i)));
   }
}

TEST(MathExtrasTest, testFloatBits)
{
   static const float kValue = 5632.34f;
   EXPECT_FLOAT_EQ(kValue, bits_to_float(float_to_bits(kValue)));
}

TEST(MathExtrasTest, testDoubleBits)
{
   static const double kValue = 87987234.983498;
   EXPECT_DOUBLE_EQ(kValue, bits_to_double(double_to_bits(kValue)));
}

TEST(MathExtrasTest, testMinAlign)
{
   EXPECT_EQ(1u, min_align(2, 3));
   EXPECT_EQ(2u, min_align(2, 4));
   EXPECT_EQ(1u, min_align(17, 64));
   EXPECT_EQ(256u, min_align(256, 512));
}

TEST(MathExtrasTest, testNextPowerOf2)
{
   EXPECT_EQ(4u, next_power_of_two(3));
   EXPECT_EQ(16u, next_power_of_two(15));
   EXPECT_EQ(256u, next_power_of_two(128));
}

TEST(MathExtrasTest, testAlignTo)
{
   EXPECT_EQ(8u, align_to(5, 8));
   EXPECT_EQ(24u, align_to(17, 8));
   EXPECT_EQ(0u, align_to(~0LL, 8));

   EXPECT_EQ(7u, align_to(5, 8, 7));
   EXPECT_EQ(17u, align_to(17, 8, 1));
   EXPECT_EQ(3u, align_to(~0LL, 8, 3));
   EXPECT_EQ(552u, align_to(321, 255, 42));
}

template<typename T>
void SaturatingAddTestHelper()
{
   const T Max = std::numeric_limits<T>::max();
   bool ResultOverflowed;

   EXPECT_EQ(T(3), saturating_add(T(1), T(2)));
   EXPECT_EQ(T(3), saturating_add(T(1), T(2), &ResultOverflowed));
   EXPECT_FALSE(ResultOverflowed);

   EXPECT_EQ(Max, saturating_add(Max, T(1)));
   EXPECT_EQ(Max, saturating_add(Max, T(1), &ResultOverflowed));
   EXPECT_TRUE(ResultOverflowed);

   EXPECT_EQ(Max, saturating_add(T(1), T(Max - 1)));
   EXPECT_EQ(Max, saturating_add(T(1), T(Max - 1), &ResultOverflowed));
   EXPECT_FALSE(ResultOverflowed);

   EXPECT_EQ(Max, saturating_add(T(1), Max));
   EXPECT_EQ(Max, saturating_add(T(1), Max, &ResultOverflowed));
   EXPECT_TRUE(ResultOverflowed);

   EXPECT_EQ(Max, saturating_add(Max, Max));
   EXPECT_EQ(Max, saturating_add(Max, Max, &ResultOverflowed));
   EXPECT_TRUE(ResultOverflowed);
}

TEST(MathExtrasTest, saturating_add)
{
   SaturatingAddTestHelper<uint8_t>();
   SaturatingAddTestHelper<uint16_t>();
   SaturatingAddTestHelper<uint32_t>();
   SaturatingAddTestHelper<uint64_t>();
}

template<typename T>
void saturating_multiply_test_helper()
{
   const T Max = std::numeric_limits<T>::max();
   bool ResultOverflowed;

   // Test basic multiplication.
   EXPECT_EQ(T(6), saturating_multiply(T(2), T(3)));
   EXPECT_EQ(T(6), saturating_multiply(T(2), T(3), &ResultOverflowed));
   EXPECT_FALSE(ResultOverflowed);

   EXPECT_EQ(T(6), saturating_multiply(T(3), T(2)));
   EXPECT_EQ(T(6), saturating_multiply(T(3), T(2), &ResultOverflowed));
   EXPECT_FALSE(ResultOverflowed);

   // Test multiplication by zero.
   EXPECT_EQ(T(0), saturating_multiply(T(0), T(0)));
   EXPECT_EQ(T(0), saturating_multiply(T(0), T(0), &ResultOverflowed));
   EXPECT_FALSE(ResultOverflowed);

   EXPECT_EQ(T(0), saturating_multiply(T(1), T(0)));
   EXPECT_EQ(T(0), saturating_multiply(T(1), T(0), &ResultOverflowed));
   EXPECT_FALSE(ResultOverflowed);

   EXPECT_EQ(T(0), saturating_multiply(T(0), T(1)));
   EXPECT_EQ(T(0), saturating_multiply(T(0), T(1), &ResultOverflowed));
   EXPECT_FALSE(ResultOverflowed);

   EXPECT_EQ(T(0), saturating_multiply(Max, T(0)));
   EXPECT_EQ(T(0), saturating_multiply(Max, T(0), &ResultOverflowed));
   EXPECT_FALSE(ResultOverflowed);

   EXPECT_EQ(T(0), saturating_multiply(T(0), Max));
   EXPECT_EQ(T(0), saturating_multiply(T(0), Max, &ResultOverflowed));
   EXPECT_FALSE(ResultOverflowed);

   // Test multiplication by maximum value.
   EXPECT_EQ(Max, saturating_multiply(Max, T(2)));
   EXPECT_EQ(Max, saturating_multiply(Max, T(2), &ResultOverflowed));
   EXPECT_TRUE(ResultOverflowed);

   EXPECT_EQ(Max, saturating_multiply(T(2), Max));
   EXPECT_EQ(Max, saturating_multiply(T(2), Max, &ResultOverflowed));
   EXPECT_TRUE(ResultOverflowed);

   EXPECT_EQ(Max, saturating_multiply(Max, Max));
   EXPECT_EQ(Max, saturating_multiply(Max, Max, &ResultOverflowed));
   EXPECT_TRUE(ResultOverflowed);

   // Test interesting boundary conditions for algorithm -
   // ((1 << A) - 1) * ((1 << B) + K) for K in [-1, 0, 1]
   // and A + B == std::numeric_limits<T>::digits.
   // We expect overflow iff A > B and K = 1.
   const int Digits = std::numeric_limits<T>::digits;
   for (int A = 1, B = Digits - 1; B >= 1; ++A, --B) {
      for (int K = -1; K <= 1; ++K) {
         T X = (T(1) << A) - T(1);
         T Y = (T(1) << B) + K;
         bool OverflowExpected = A > B && K == 1;

         if(OverflowExpected) {
            EXPECT_EQ(Max, saturating_multiply(X, Y));
            EXPECT_EQ(Max, saturating_multiply(X, Y, &ResultOverflowed));
            EXPECT_TRUE(ResultOverflowed);
         } else {
            EXPECT_EQ(X * Y, saturating_multiply(X, Y));
            EXPECT_EQ(X * Y, saturating_multiply(X, Y, &ResultOverflowed));
            EXPECT_FALSE(ResultOverflowed);
         }
      }
   }
}

TEST(MathExtrasTest, testSaturatingMultiply)
{
   saturating_multiply_test_helper<uint8_t>();
   saturating_multiply_test_helper<uint16_t>();
   saturating_multiply_test_helper<uint32_t>();
   saturating_multiply_test_helper<uint64_t>();
}

template<typename T>
void saturating_multiply_add_test_helper()
{
   const T Max = std::numeric_limits<T>::max();
   bool ResultOverflowed;

   // Test basic multiply-add.
   EXPECT_EQ(T(16), saturating_multiply_add(T(2), T(3), T(10)));
   EXPECT_EQ(T(16), saturating_multiply_add(T(2), T(3), T(10), &ResultOverflowed));
   EXPECT_FALSE(ResultOverflowed);

   // Test multiply overflows, add doesn't overflow
   EXPECT_EQ(Max, saturating_multiply_add(Max, Max, T(0), &ResultOverflowed));
   EXPECT_TRUE(ResultOverflowed);

   // Test multiply doesn't overflow, add overflows
   EXPECT_EQ(Max, saturating_multiply_add(T(1), T(1), Max, &ResultOverflowed));
   EXPECT_TRUE(ResultOverflowed);

   // Test multiply-add with Max as operand
   EXPECT_EQ(Max, saturating_multiply_add(T(1), T(1), Max, &ResultOverflowed));
   EXPECT_TRUE(ResultOverflowed);

   EXPECT_EQ(Max, saturating_multiply_add(T(1), Max, T(1), &ResultOverflowed));
   EXPECT_TRUE(ResultOverflowed);

   EXPECT_EQ(Max, saturating_multiply_add(Max, Max, T(1), &ResultOverflowed));
   EXPECT_TRUE(ResultOverflowed);

   EXPECT_EQ(Max, saturating_multiply_add(Max, Max, Max, &ResultOverflowed));
   EXPECT_TRUE(ResultOverflowed);

   // Test multiply-add with 0 as operand
   EXPECT_EQ(T(1), saturating_multiply_add(T(1), T(1), T(0), &ResultOverflowed));
   EXPECT_FALSE(ResultOverflowed);

   EXPECT_EQ(T(1), saturating_multiply_add(T(1), T(0), T(1), &ResultOverflowed));
   EXPECT_FALSE(ResultOverflowed);

   EXPECT_EQ(T(1), saturating_multiply_add(T(0), T(0), T(1), &ResultOverflowed));
   EXPECT_FALSE(ResultOverflowed);

   EXPECT_EQ(T(0), saturating_multiply_add(T(0), T(0), T(0), &ResultOverflowed));
   EXPECT_FALSE(ResultOverflowed);

}

TEST(MathExtrasTest, testSaturatingMultiplyAdd)
{
   saturating_multiply_add_test_helper<uint8_t>();
   saturating_multiply_add_test_helper<uint16_t>();
   saturating_multiply_add_test_helper<uint32_t>();
   saturating_multiply_add_test_helper<uint64_t>();
}

TEST(MathExtrasTest, testIsShiftedUInt)
{
   EXPECT_TRUE((is_shifted_uint<1, 0>(0)));
   EXPECT_TRUE((is_shifted_uint<1, 0>(1)));
   EXPECT_FALSE((is_shifted_uint<1, 0>(2)));
   EXPECT_FALSE((is_shifted_uint<1, 0>(3)));
   EXPECT_FALSE((is_shifted_uint<1, 0>(0x8000000000000000)));
   EXPECT_TRUE((is_shifted_uint<1, 63>(0x8000000000000000)));
   EXPECT_TRUE((is_shifted_uint<2, 62>(0xC000000000000000)));
   EXPECT_FALSE((is_shifted_uint<2, 62>(0xE000000000000000)));

   // 0x201 is ten bits long and has a 1 in the MSB and LSB.
   EXPECT_TRUE((is_shifted_uint<10, 5>(uint64_t(0x201) << 5)));
   EXPECT_FALSE((is_shifted_uint<10, 5>(uint64_t(0x201) << 4)));
   EXPECT_FALSE((is_shifted_uint<10, 5>(uint64_t(0x201) << 6)));
}

TEST(MathExtrasTest, testIsShiftedInt)
{
   EXPECT_TRUE((is_shifted_int<1, 0>(0)));
   EXPECT_TRUE((is_shifted_int<1, 0>(-1)));
   EXPECT_FALSE((is_shifted_int<1, 0>(2)));
   EXPECT_FALSE((is_shifted_int<1, 0>(3)));
   EXPECT_FALSE((is_shifted_int<1, 0>(0x8000000000000000)));
   EXPECT_TRUE((is_shifted_int<1, 63>(0x8000000000000000)));
   EXPECT_TRUE((is_shifted_int<2, 62>(0xC000000000000000)));
   EXPECT_FALSE((is_shifted_int<2, 62>(0xE000000000000000)));

   // 0x201 is ten bits long and has a 1 in the MSB and LSB.
   EXPECT_TRUE((is_shifted_int<11, 5>(int64_t(0x201) << 5)));
   EXPECT_FALSE((is_shifted_int<11, 5>(int64_t(0x201) << 3)));
   EXPECT_FALSE((is_shifted_int<11, 5>(int64_t(0x201) << 6)));
   EXPECT_TRUE((is_shifted_int<11, 5>(-(int64_t(0x201) << 5))));
   EXPECT_FALSE((is_shifted_int<11, 5>(-(int64_t(0x201) << 3))));
   EXPECT_FALSE((is_shifted_int<11, 5>(-(int64_t(0x201) << 6))));

   EXPECT_TRUE((is_shifted_int<6, 10>(-(int64_t(1) << 15))));
   EXPECT_FALSE((is_shifted_int<6, 10>(int64_t(1) << 15)));
}

} // namespace
