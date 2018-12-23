// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/01.

#include "gtest/gtest.h"
#include "polarphp/utils/SwapByteOrder.h"
#include <cstdlib>
#include <ctime>

using polar::utils::get_swapped_bytes;
using polar::utils::swap_byte_order;

namespace {

TEST(GetSwappedBytesTest, testUnsignedRoundTrip)
{
   // The point of the bit twiddling of magic is to test with and without bits
   // in every byte.
   uint64_t value = 1;
   for (std::size_t i = 0; i <= sizeof(value); ++i) {
      uint8_t original_uint8 = static_cast<uint8_t>(value);
      EXPECT_EQ(original_uint8,
                get_swapped_bytes(get_swapped_bytes(original_uint8)));

      uint16_t original_uint16 = static_cast<uint16_t>(value);
      EXPECT_EQ(original_uint16,
                get_swapped_bytes(get_swapped_bytes(original_uint16)));

      uint32_t original_uint32 = static_cast<uint32_t>(value);
      EXPECT_EQ(original_uint32,
                get_swapped_bytes(get_swapped_bytes(original_uint32)));

      uint64_t original_uint64 = static_cast<uint64_t>(value);
      EXPECT_EQ(original_uint64,
                get_swapped_bytes(get_swapped_bytes(original_uint64)));

      value = (value << 8) | 0x55; // binary 0101 0101.
   }
}

TEST(GetSwappedBytesTest, testSignedRoundTrip)
{
   // The point of the bit twiddling of magic is to test with and without bits
   // in every byte.
   uint64_t value = 1;
   for (std::size_t i = 0; i <= sizeof(value); ++i) {
      int8_t original_int8 = static_cast<int8_t>(value);
      EXPECT_EQ(original_int8,
                get_swapped_bytes(get_swapped_bytes(original_int8)));

      int16_t original_int16 = static_cast<int16_t>(value);
      EXPECT_EQ(original_int16,
                get_swapped_bytes(get_swapped_bytes(original_int16)));

      int32_t original_int32 = static_cast<int32_t>(value);
      EXPECT_EQ(original_int32,
                get_swapped_bytes(get_swapped_bytes(original_int32)));

      int64_t original_int64 = static_cast<int64_t>(value);
      EXPECT_EQ(original_int64,
                get_swapped_bytes(get_swapped_bytes(original_int64)));

      // Test other sign.
      value *= -1;

      original_int8 = static_cast<int8_t>(value);
      EXPECT_EQ(original_int8,
                get_swapped_bytes(get_swapped_bytes(original_int8)));

      original_int16 = static_cast<int16_t>(value);
      EXPECT_EQ(original_int16,
                get_swapped_bytes(get_swapped_bytes(original_int16)));

      original_int32 = static_cast<int32_t>(value);
      EXPECT_EQ(original_int32,
                get_swapped_bytes(get_swapped_bytes(original_int32)));

      original_int64 = static_cast<int64_t>(value);
      EXPECT_EQ(original_int64,
                get_swapped_bytes(get_swapped_bytes(original_int64)));

      // Return to normal sign and twiddle.
      value *= -1;
      value = (value << 8) | 0x55; // binary 0101 0101.
   }
}

TEST(GetSwappedBytesTest, testUInt8_t)
{
   EXPECT_EQ(uint8_t(0x11), get_swapped_bytes(uint8_t(0x11)));
}

TEST(GetSwappedBytesTest, testUint16_t)
{
   EXPECT_EQ(uint16_t(0x1122), get_swapped_bytes(uint16_t(0x2211)));
}

TEST(GetSwappedBytesTest, testUint32_t)
{
   EXPECT_EQ(uint32_t(0x11223344), get_swapped_bytes(uint32_t(0x44332211)));
}

TEST(GetSwappedBytesTest, testUint64_t)
{
   EXPECT_EQ(uint64_t(0x1122334455667788ULL),
             get_swapped_bytes(uint64_t(0x8877665544332211ULL)));
}

TEST(GetSwappedBytesTest, TestInt8_t)
{
   EXPECT_EQ(int8_t(0x11), get_swapped_bytes(int8_t(0x11)));
}

TEST(GetSwappedBytesTest, TestInt16_t)
{
   EXPECT_EQ(int16_t(0x1122), get_swapped_bytes(int16_t(0x2211)));
}

TEST(GetSwappedBytesTest, TestInt32_t)
{
   EXPECT_EQ(int32_t(0x11223344), get_swapped_bytes(int32_t(0x44332211)));
}

TEST(GetSwappedBytesTest, TestInt64_t)
{
   EXPECT_EQ(int64_t(0x1122334455667788LL),
             get_swapped_bytes(int64_t(0x8877665544332211LL)));
}

TEST(GetSwappedBytesTest, testFloat)
{
   EXPECT_EQ(1.79366203433576585078237386661e-43f, get_swapped_bytes(-0.0f));
   // 0x11223344
   EXPECT_EQ(7.1653228759765625e2f, get_swapped_bytes(1.2795344e-28f));
}

TEST(GetSwappedBytesTest, testDouble)
{
   EXPECT_EQ(6.32404026676795576546008054871e-322, get_swapped_bytes(-0.0));
   // 0x1122334455667788
   EXPECT_EQ(-7.08687663657301358331704585496e-268,
             get_swapped_bytes(3.84141202447173065923064450234e-226));
}

TEST(SwapByteOrderTest, testUint8_t)
{
   uint8_t value = 0x11;
   swap_byte_order(value);
   EXPECT_EQ(uint8_t(0x11), value);
}

TEST(SwapByteOrderTest, testUint16_t)
{
   uint16_t value = 0x2211;
   swap_byte_order(value);
   EXPECT_EQ(uint16_t(0x1122), value);
}

TEST(SwapByteOrderTest, testUint32_t)
{
   uint32_t value = 0x44332211;
   swap_byte_order(value);
   EXPECT_EQ(uint32_t(0x11223344), value);
}

TEST(SwapByteOrderTest, testUint64_t)
{
   uint64_t value = 0x8877665544332211ULL;
   swap_byte_order(value);
   EXPECT_EQ(uint64_t(0x1122334455667788ULL), value);
}

TEST(SwapByteOrderTest, testInt8_t)
{
   int8_t value = 0x11;
   swap_byte_order(value);
   EXPECT_EQ(int8_t(0x11), value);
}

TEST(SwapByteOrderTest, testInt16_t)
{
   int16_t value = 0x2211;
   swap_byte_order(value);
   EXPECT_EQ(int16_t(0x1122), value);
}

TEST(SwapByteOrderTest, testInt32_t)
{
   int32_t value = 0x44332211;
   swap_byte_order(value);
   EXPECT_EQ(int32_t(0x11223344), value);
}

TEST(SwapByteOrderTest, testInt64_t)
{
   int64_t value = 0x8877665544332211LL;
   swap_byte_order(value);
   EXPECT_EQ(int64_t(0x1122334455667788LL), value);
}

TEST(SwapByteOrderTest, testFloat)
{
   float value = 7.1653228759765625e2f; // 0x44332211
   swap_byte_order(value);
   EXPECT_EQ(1.2795344e-28f, value);
}

TEST(SwapByteOrderTest, testDouble)
{
   double value = -7.08687663657301358331704585496e-268; // 0x8877665544332211
   swap_byte_order(value);
   EXPECT_EQ(3.84141202447173065923064450234e-226, value);
}

} // anonymous namespace
