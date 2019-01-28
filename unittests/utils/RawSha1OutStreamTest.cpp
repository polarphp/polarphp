// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/15.

#include "polarphp/utils/Format.h"
#include "polarphp/utils/RawSha1OutStream.h"
#include "gtest/gtest.h"

#include <string>

using namespace polar;
using namespace polar::utils;

static std::string toHex(StringRef input)
{
   static const char *const LUT = "0123456789ABCDEF";
   size_t Length = input.size();

   std::string Output;
   Output.reserve(2 * Length);
   for (size_t i = 0; i < Length; ++i) {
      const unsigned char c = input[i];
      Output.push_back(LUT[c >> 4]);
      Output.push_back(LUT[c & 15]);
   }
   return Output;
}

TEST(RawSha1OutStreamTest, testBasic)
{
   RawSha1OutStream Sha1Stream;
   Sha1Stream << "Hello World!";
   auto Hash = toHex(Sha1Stream.getSha1());

   ASSERT_EQ("2EF7BDE608CE5404E97D5F042F95F89F1C232871", Hash);
}

TEST(RawSha1OutStreamTest, testSha1Hash)
{
   ArrayRef<uint8_t> input((const uint8_t *)"Hello World!", 12);
   std::array<uint8_t, 20> Vec = Sha1::hash(input);
   std::string Hash = toHex({(const char *)Vec.data(), 20});
   ASSERT_EQ("2EF7BDE608CE5404E97D5F042F95F89F1C232871", Hash);
}

// Check that getting the intermediate hash in the middle of the stream does
// not invalidate the final result.
TEST(RawSha1OutStreamTest, testIntermediate) {
   RawSha1OutStream Sha1Stream;
   Sha1Stream << "Hello";
   auto Hash = toHex(Sha1Stream.getSha1());

   ASSERT_EQ("F7FF9E8B7BB2E09B70935A5D785E0CC5D9D0ABF0", Hash);
   Sha1Stream << " World!";
   Hash = toHex(Sha1Stream.getSha1());

   // Compute the non-split hash separately as a reference.
   RawSha1OutStream NonSplitSha1Stream;
   NonSplitSha1Stream << "Hello World!";
   auto NonSplitHash = toHex(NonSplitSha1Stream.getSha1());

   ASSERT_EQ(NonSplitHash, Hash);
}

TEST(RawSha1OutStreamTest, testReset)
{
   RawSha1OutStream Sha1Stream;
   Sha1Stream << "Hello";
   auto Hash = toHex(Sha1Stream.getSha1());

   ASSERT_EQ("F7FF9E8B7BB2E09B70935A5D785E0CC5D9D0ABF0", Hash);

   Sha1Stream.resetHash();
   Sha1Stream << " World!";
   Hash = toHex(Sha1Stream.getSha1());

   ASSERT_EQ("7447F2A5A42185C8CF91E632789C431830B59067", Hash);
}

