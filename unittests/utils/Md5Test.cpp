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

#include "polarphp/utils/Md5.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/SmallString.h"
#include "gtest/gtest.h"

using namespace polar;
using namespace polar::basic;
using namespace polar::utils;

namespace {

/// \brief Tests an arbitrary set of bytes passed as \p Input.
void TestMD5Sum(ArrayRef<uint8_t> Input, StringRef Final)
{
   Md5 Hash;
   Hash.update(Input);
   Md5::Md5Result MD5Res;
   Hash.final(MD5Res);
   SmallString<32> Res;
   Md5::stringifyResult(MD5Res, Res);
   EXPECT_EQ(Res, Final);
}

void TestMD5Sum(StringRef Input, StringRef Final)
{
   Md5 Hash;
   Hash.update(Input);
   Md5::Md5Result MD5Res;
   Hash.final(MD5Res);
   SmallString<32> Res;
   Md5::stringifyResult(MD5Res, Res);
   EXPECT_EQ(Res, Final);
}

TEST(Md5Test, testMd5)
{
   TestMD5Sum(make_array_ref((const uint8_t *)"", (size_t) 0),
              "d41d8cd98f00b204e9800998ecf8427e");
   TestMD5Sum(make_array_ref((const uint8_t *)"a", (size_t) 1),
              "0cc175b9c0f1b6a831c399e269772661");
   TestMD5Sum(make_array_ref((const uint8_t *)"abcdefghijklmnopqrstuvwxyz",
                           (size_t) 26),
              "c3fcd3d76192e4007dfb496cca67e13b");
   TestMD5Sum(make_array_ref((const uint8_t *)"\0", (size_t) 1),
              "93b885adfe0da089cdf634904fd59f71");
   TestMD5Sum(make_array_ref((const uint8_t *)"a\0", (size_t) 2),
              "4144e195f46de78a3623da7364d04f11");
   TestMD5Sum(make_array_ref((const uint8_t *)"abcdefghijklmnopqrstuvwxyz\0",
                           (size_t) 27),
              "81948d1f1554f58cd1a56ebb01f808cb");
   TestMD5Sum("abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b");
}

TEST(Md5, testMd5Hash)
{
   ArrayRef<uint8_t> Input((const uint8_t *)"abcdefghijklmnopqrstuvwxyz", 26);
   std::array<uint8_t, 16> Vec = Md5::hash(Input);
   Md5::Md5Result MD5Res;
   SmallString<32> Res;
   memcpy(MD5Res.m_bytes.data(), Vec.data(), Vec.size());
   Md5::stringifyResult(MD5Res, Res);
   EXPECT_EQ(Res, "c3fcd3d76192e4007dfb496cca67e13b");
   EXPECT_EQ(0x3be167ca6c49fb7dULL, MD5Res.getHigh());
   EXPECT_EQ(0x00e49261d7d3fcc3ULL, MD5Res.getLow());
}

} // anonymous namespace
