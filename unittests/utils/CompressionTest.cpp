// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/12.

#include "polarphp/utils/Compression.h"
#include "polarphp/utils/Error.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/StringRef.h"
#include "gtest/gtest.h"

using namespace polar::basic;
using namespace polar::utils;

namespace {

#if defined(POLAR_ENABLE_ZLIB) && defined(HAVE_LIBZ)

void test_zlib_compression(StringRef input, int level)
{
   SmallString<32> compressed;
   SmallString<32> uncompressed;

   Error error = zlib::compress(input, compressed, level);
   EXPECT_FALSE(error);
   consume_error(std::move(error));

   // Check that uncompressed buffer is the same as original.
   error = zlib::uncompress(compressed, uncompressed, input.size());
   EXPECT_FALSE(error);
   consume_error(std::move(error));

   EXPECT_EQ(input, uncompressed);
   if (input.size() > 0) {
      // Uncompression fails if expected length is too short.
      error = zlib::uncompress(compressed, uncompressed, input.size() - 1);
      EXPECT_EQ("zlib error: Z_BUF_ERROR", to_string(std::move(error)));
   }
}

TEST(CompressionTest, testZlib)
{
   test_zlib_compression("", zlib::DefaultCompression);

   test_zlib_compression("hello, world!", zlib::NoCompression);
   test_zlib_compression("hello, world!", zlib::BestSizeCompression);
   test_zlib_compression("hello, world!", zlib::BestSpeedCompression);
   test_zlib_compression("hello, world!", zlib::DefaultCompression);

   const size_t kSize = 1024;
   char binaryData[kSize];
   for (size_t i = 0; i < kSize; ++i) {
      binaryData[i] = i & 255;
   }
   StringRef binaryDataStr(binaryData, kSize);

   test_zlib_compression(binaryDataStr, zlib::NoCompression);
   test_zlib_compression(binaryDataStr, zlib::BestSizeCompression);
   test_zlib_compression(binaryDataStr, zlib::BestSpeedCompression);
   test_zlib_compression(binaryDataStr, zlib::DefaultCompression);
}

TEST(CompressionTest, testZlibCRC32)
{
   EXPECT_EQ(
            0x414FA339U,
            zlib::crc32(StringRef("The quick brown fox jumps over the lazy dog")));
}

#endif

} // anonymous namespace
