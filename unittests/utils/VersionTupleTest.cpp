// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/09.

#include "polarphp/utils/VersionTuple.h"
#include "gtest/gtest.h"

using namespace polar::utils;

TEST(VersionTuple, testGetAsString)
{
   EXPECT_EQ("0", VersionTuple().getAsString());
   EXPECT_EQ("1", VersionTuple(1).getAsString());
   EXPECT_EQ("1.2", VersionTuple(1, 2).getAsString());
   EXPECT_EQ("1.2.3", VersionTuple(1, 2, 3).getAsString());
   EXPECT_EQ("1.2.3.4", VersionTuple(1, 2, 3, 4).getAsString());
}

TEST(VersionTuple, testTryParse)
{
   VersionTuple VT;

   EXPECT_FALSE(VT.tryParse("1"));
   EXPECT_EQ("1", VT.getAsString());

   EXPECT_FALSE(VT.tryParse("1.2"));
   EXPECT_EQ("1.2", VT.getAsString());

   EXPECT_FALSE(VT.tryParse("1.2.3"));
   EXPECT_EQ("1.2.3", VT.getAsString());

   EXPECT_FALSE(VT.tryParse("1.2.3.4"));
   EXPECT_EQ("1.2.3.4", VT.getAsString());

   EXPECT_TRUE(VT.tryParse(""));
   EXPECT_TRUE(VT.tryParse("1."));
   EXPECT_TRUE(VT.tryParse("1.2."));
   EXPECT_TRUE(VT.tryParse("1.2.3."));
   EXPECT_TRUE(VT.tryParse("1.2.3.4."));
   EXPECT_TRUE(VT.tryParse("1.2.3.4.5"));
   EXPECT_TRUE(VT.tryParse("1-2"));
   EXPECT_TRUE(VT.tryParse("1+2"));
   EXPECT_TRUE(VT.tryParse(".1"));
   EXPECT_TRUE(VT.tryParse(" 1"));
   EXPECT_TRUE(VT.tryParse("1 "));
   EXPECT_TRUE(VT.tryParse("."));
}
