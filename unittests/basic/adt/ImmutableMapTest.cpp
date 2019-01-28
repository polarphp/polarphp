// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/08.

#include "polarphp/basic/adt/ImmutableMap.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {

TEST(ImmutableMapTest, testEmptyIntMap)
{
   ImmutableMap<int, int>::Factory f;
   EXPECT_TRUE(f.getEmptyMap() == f.getEmptyMap());
   EXPECT_FALSE(f.getEmptyMap() != f.getEmptyMap());
   EXPECT_TRUE(f.getEmptyMap().isEmpty());
   ImmutableMap<int, int> S = f.getEmptyMap();
   EXPECT_EQ(0u, S.getHeight());
   EXPECT_TRUE(S.begin() == S.end());
   EXPECT_FALSE(S.begin() != S.end());
}

TEST(ImmutableMapTest, testMultiElemIntMap)
{
   ImmutableMap<int, int>::Factory f;
   ImmutableMap<int, int> S = f.getEmptyMap();
   ImmutableMap<int, int> S2 = f.add(f.add(f.add(S, 3, 10), 4, 11), 5, 12);
   EXPECT_TRUE(S.isEmpty());
   EXPECT_FALSE(S2.isEmpty());
   EXPECT_EQ(nullptr, S.lookup(3));
   EXPECT_EQ(nullptr, S.lookup(9));
   EXPECT_EQ(10, *S2.lookup(3));
   EXPECT_EQ(11, *S2.lookup(4));
   EXPECT_EQ(12, *S2.lookup(5));
   EXPECT_EQ(5, S2.getMaxElement()->first);
   EXPECT_EQ(3U, S2.getHeight());
}

} // anonymous namespace
