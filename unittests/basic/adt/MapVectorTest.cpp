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

#include "polarphp/basic/adt/MapVector.h"
#include "polarphp/basic/adt/IteratorRange.h"
#include "gtest/gtest.h"
#include <utility>

using namespace polar::basic;

namespace {

TEST(MapVectorTest, testSwap)
{
   MapVector<int, int> MV1, MV2;
   std::pair<MapVector<int, int>::iterator, bool> R;

   R = MV1.insert(std::make_pair(1, 2));
   ASSERT_EQ(R.first, MV1.begin());
   EXPECT_EQ(R.first->first, 1);
   EXPECT_EQ(R.first->second, 2);
   EXPECT_TRUE(R.second);

   EXPECT_FALSE(MV1.empty());
   EXPECT_TRUE(MV2.empty());
   MV2.swap(MV1);
   EXPECT_TRUE(MV1.empty());
   EXPECT_FALSE(MV2.empty());

   auto I = MV1.find(1);
   ASSERT_EQ(MV1.end(), I);

   I = MV2.find(1);
   ASSERT_EQ(I, MV2.begin());
   EXPECT_EQ(I->first, 1);
   EXPECT_EQ(I->second, 2);
}

TEST(MapVectorTest, testInsertPop)
{
   MapVector<int, int> MV;
   std::pair<MapVector<int, int>::iterator, bool> R;

   R = MV.insert(std::make_pair(1, 2));
   ASSERT_EQ(R.first, MV.begin());
   EXPECT_EQ(R.first->first, 1);
   EXPECT_EQ(R.first->second, 2);
   EXPECT_TRUE(R.second);

   R = MV.insert(std::make_pair(1, 3));
   ASSERT_EQ(R.first, MV.begin());
   EXPECT_EQ(R.first->first, 1);
   EXPECT_EQ(R.first->second, 2);
   EXPECT_FALSE(R.second);

   R = MV.insert(std::make_pair(4, 5));
   ASSERT_NE(R.first, MV.end());
   EXPECT_EQ(R.first->first, 4);
   EXPECT_EQ(R.first->second, 5);
   EXPECT_TRUE(R.second);

   EXPECT_EQ(MV.size(), 2u);
   EXPECT_EQ(MV[1], 2);
   EXPECT_EQ(MV[4], 5);

   MV.pop_back();
   EXPECT_EQ(MV.size(), 1u);
   EXPECT_EQ(MV[1], 2);

   R = MV.insert(std::make_pair(4, 7));
   ASSERT_NE(R.first, MV.end());
   EXPECT_EQ(R.first->first, 4);
   EXPECT_EQ(R.first->second, 7);
   EXPECT_TRUE(R.second);

   EXPECT_EQ(MV.size(), 2u);
   EXPECT_EQ(MV[1], 2);
   EXPECT_EQ(MV[4], 7);
}

TEST(MapVectorTest, testErase)
{
   MapVector<int, int> MV;

   MV.insert(std::make_pair(1, 2));
   MV.insert(std::make_pair(3, 4));
   MV.insert(std::make_pair(5, 6));
   ASSERT_EQ(MV.size(), 3u);

   MV.erase(MV.find(1));
   ASSERT_EQ(MV.size(), 2u);
   ASSERT_EQ(MV.find(1), MV.end());
   ASSERT_EQ(MV[3], 4);
   ASSERT_EQ(MV[5], 6);

   ASSERT_EQ(MV.erase(3), 1u);
   ASSERT_EQ(MV.size(), 1u);
   ASSERT_EQ(MV.find(3), MV.end());
   ASSERT_EQ(MV[5], 6);

   ASSERT_EQ(MV.erase(79), 0u);
   ASSERT_EQ(MV.size(), 1u);
}

TEST(MapVectorTest, testRemoveIf)
{
   MapVector<int, int> MV;

   MV.insert(std::make_pair(1, 11));
   MV.insert(std::make_pair(2, 12));
   MV.insert(std::make_pair(3, 13));
   MV.insert(std::make_pair(4, 14));
   MV.insert(std::make_pair(5, 15));
   MV.insert(std::make_pair(6, 16));
   ASSERT_EQ(MV.size(), 6u);

   MV.removeIf([](const std::pair<int, int> &Val) { return Val.second % 2; });
   ASSERT_EQ(MV.size(), 3u);
   ASSERT_EQ(MV.find(1), MV.end());
   ASSERT_EQ(MV.find(3), MV.end());
   ASSERT_EQ(MV.find(5), MV.end());
   ASSERT_EQ(MV[2], 12);
   ASSERT_EQ(MV[4], 14);
   ASSERT_EQ(MV[6], 16);
}

TEST(MapVectorTest, testIteration)
{
   MapVector<int, int> MV;

   MV.insert(std::make_pair(1, 11));
   MV.insert(std::make_pair(2, 12));
   MV.insert(std::make_pair(3, 13));
   MV.insert(std::make_pair(4, 14));
   MV.insert(std::make_pair(5, 15));
   MV.insert(std::make_pair(6, 16));
   ASSERT_EQ(MV.size(), 6u);

   int count = 1;
   for (auto P : make_range(MV.begin(), MV.end())) {
      ASSERT_EQ(P.first, count);
      count++;
   }

   count = 6;
   for (auto P : make_range(MV.rbegin(), MV.rend())) {
      ASSERT_EQ(P.first, count);
      count--;
   }
}

TEST(MapVectorTest, testNonCopyable)
{
   MapVector<int, std::unique_ptr<int>> MV;
   MV.insert(std::make_pair(1, std::make_unique<int>(1)));
   MV.insert(std::make_pair(2, std::make_unique<int>(2)));

   ASSERT_EQ(MV.count(1), 1u);
   ASSERT_EQ(*MV.find(2)->second, 2);
}

TEST(SmallMapVectorSmallTest, testInsertPop)
{
   SmallMapVector<int, int, 32> MV;
   std::pair<SmallMapVector<int, int, 32>::iterator, bool> R;

   R = MV.insert(std::make_pair(1, 2));
   ASSERT_EQ(R.first, MV.begin());
   EXPECT_EQ(R.first->first, 1);
   EXPECT_EQ(R.first->second, 2);
   EXPECT_TRUE(R.second);

   R = MV.insert(std::make_pair(1, 3));
   ASSERT_EQ(R.first, MV.begin());
   EXPECT_EQ(R.first->first, 1);
   EXPECT_EQ(R.first->second, 2);
   EXPECT_FALSE(R.second);

   R = MV.insert(std::make_pair(4, 5));
   ASSERT_NE(R.first, MV.end());
   EXPECT_EQ(R.first->first, 4);
   EXPECT_EQ(R.first->second, 5);
   EXPECT_TRUE(R.second);

   EXPECT_EQ(MV.size(), 2u);
   EXPECT_EQ(MV[1], 2);
   EXPECT_EQ(MV[4], 5);

   MV.pop_back();
   EXPECT_EQ(MV.size(), 1u);
   EXPECT_EQ(MV[1], 2);

   R = MV.insert(std::make_pair(4, 7));
   ASSERT_NE(R.first, MV.end());
   EXPECT_EQ(R.first->first, 4);
   EXPECT_EQ(R.first->second, 7);
   EXPECT_TRUE(R.second);

   EXPECT_EQ(MV.size(), 2u);
   EXPECT_EQ(MV[1], 2);
   EXPECT_EQ(MV[4], 7);
}

TEST(SmallMapVectorSmallTest, testErase)
{
   SmallMapVector<int, int, 32> MV;

   MV.insert(std::make_pair(1, 2));
   MV.insert(std::make_pair(3, 4));
   MV.insert(std::make_pair(5, 6));
   ASSERT_EQ(MV.size(), 3u);

   MV.erase(MV.find(1));
   ASSERT_EQ(MV.size(), 2u);
   ASSERT_EQ(MV.find(1), MV.end());
   ASSERT_EQ(MV[3], 4);
   ASSERT_EQ(MV[5], 6);

   ASSERT_EQ(MV.erase(3), 1u);
   ASSERT_EQ(MV.size(), 1u);
   ASSERT_EQ(MV.find(3), MV.end());
   ASSERT_EQ(MV[5], 6);

   ASSERT_EQ(MV.erase(79), 0u);
   ASSERT_EQ(MV.size(), 1u);
}

TEST(SmallMapVectorSmallTest, testRemoveIf)
{
   SmallMapVector<int, int, 32> MV;

   MV.insert(std::make_pair(1, 11));
   MV.insert(std::make_pair(2, 12));
   MV.insert(std::make_pair(3, 13));
   MV.insert(std::make_pair(4, 14));
   MV.insert(std::make_pair(5, 15));
   MV.insert(std::make_pair(6, 16));
   ASSERT_EQ(MV.size(), 6u);

   MV.removeIf([](const std::pair<int, int> &Val) { return Val.second % 2; });
   ASSERT_EQ(MV.size(), 3u);
   ASSERT_EQ(MV.find(1), MV.end());
   ASSERT_EQ(MV.find(3), MV.end());
   ASSERT_EQ(MV.find(5), MV.end());
   ASSERT_EQ(MV[2], 12);
   ASSERT_EQ(MV[4], 14);
   ASSERT_EQ(MV[6], 16);
}

TEST(SmallMapVectorSmallTest, testIteration)
{
   SmallMapVector<int, int, 32> MV;

   MV.insert(std::make_pair(1, 11));
   MV.insert(std::make_pair(2, 12));
   MV.insert(std::make_pair(3, 13));
   MV.insert(std::make_pair(4, 14));
   MV.insert(std::make_pair(5, 15));
   MV.insert(std::make_pair(6, 16));
   ASSERT_EQ(MV.size(), 6u);

   int count = 1;
   for (auto P : make_range(MV.begin(), MV.end())) {
      ASSERT_EQ(P.first, count);
      count++;
   }

   count = 6;
   for (auto P : make_range(MV.rbegin(), MV.rend())) {
      ASSERT_EQ(P.first, count);
      count--;
   }
}

TEST(SmallMapVectorSmallTest, testNonCopyable)
{
   SmallMapVector<int, std::unique_ptr<int>, 8> MV;
   MV.insert(std::make_pair(1, std::make_unique<int>(1)));
   MV.insert(std::make_pair(2, std::make_unique<int>(2)));

   ASSERT_EQ(MV.count(1), 1u);
   ASSERT_EQ(*MV.find(2)->second, 2);
}

TEST(SmallMapVectorLargeTest, testInsertPop)
{
   SmallMapVector<int, int, 1> MV;
   std::pair<SmallMapVector<int, int, 1>::iterator, bool> R;

   R = MV.insert(std::make_pair(1, 2));
   ASSERT_EQ(R.first, MV.begin());
   EXPECT_EQ(R.first->first, 1);
   EXPECT_EQ(R.first->second, 2);
   EXPECT_TRUE(R.second);

   R = MV.insert(std::make_pair(1, 3));
   ASSERT_EQ(R.first, MV.begin());
   EXPECT_EQ(R.first->first, 1);
   EXPECT_EQ(R.first->second, 2);
   EXPECT_FALSE(R.second);

   R = MV.insert(std::make_pair(4, 5));
   ASSERT_NE(R.first, MV.end());
   EXPECT_EQ(R.first->first, 4);
   EXPECT_EQ(R.first->second, 5);
   EXPECT_TRUE(R.second);

   EXPECT_EQ(MV.size(), 2u);
   EXPECT_EQ(MV[1], 2);
   EXPECT_EQ(MV[4], 5);

   MV.pop_back();
   EXPECT_EQ(MV.size(), 1u);
   EXPECT_EQ(MV[1], 2);

   R = MV.insert(std::make_pair(4, 7));
   ASSERT_NE(R.first, MV.end());
   EXPECT_EQ(R.first->first, 4);
   EXPECT_EQ(R.first->second, 7);
   EXPECT_TRUE(R.second);

   EXPECT_EQ(MV.size(), 2u);
   EXPECT_EQ(MV[1], 2);
   EXPECT_EQ(MV[4], 7);
}

TEST(SmallMapVectorLargeTest, testErase)
{
   SmallMapVector<int, int, 1> MV;

   MV.insert(std::make_pair(1, 2));
   MV.insert(std::make_pair(3, 4));
   MV.insert(std::make_pair(5, 6));
   ASSERT_EQ(MV.size(), 3u);

   MV.erase(MV.find(1));
   ASSERT_EQ(MV.size(), 2u);
   ASSERT_EQ(MV.find(1), MV.end());
   ASSERT_EQ(MV[3], 4);
   ASSERT_EQ(MV[5], 6);

   ASSERT_EQ(MV.erase(3), 1u);
   ASSERT_EQ(MV.size(), 1u);
   ASSERT_EQ(MV.find(3), MV.end());
   ASSERT_EQ(MV[5], 6);

   ASSERT_EQ(MV.erase(79), 0u);
   ASSERT_EQ(MV.size(), 1u);
}

TEST(SmallMapVectorLargeTest, testRemoveIf)
{
   SmallMapVector<int, int, 1> MV;

   MV.insert(std::make_pair(1, 11));
   MV.insert(std::make_pair(2, 12));
   MV.insert(std::make_pair(3, 13));
   MV.insert(std::make_pair(4, 14));
   MV.insert(std::make_pair(5, 15));
   MV.insert(std::make_pair(6, 16));
   ASSERT_EQ(MV.size(), 6u);

   MV.removeIf([](const std::pair<int, int> &Val) { return Val.second % 2; });
   ASSERT_EQ(MV.size(), 3u);
   ASSERT_EQ(MV.find(1), MV.end());
   ASSERT_EQ(MV.find(3), MV.end());
   ASSERT_EQ(MV.find(5), MV.end());
   ASSERT_EQ(MV[2], 12);
   ASSERT_EQ(MV[4], 14);
   ASSERT_EQ(MV[6], 16);
}

TEST(SmallMapVectorLargeTest, testIterationTest)
{
   SmallMapVector<int, int, 1> MV;

   MV.insert(std::make_pair(1, 11));
   MV.insert(std::make_pair(2, 12));
   MV.insert(std::make_pair(3, 13));
   MV.insert(std::make_pair(4, 14));
   MV.insert(std::make_pair(5, 15));
   MV.insert(std::make_pair(6, 16));
   ASSERT_EQ(MV.size(), 6u);

   int count = 1;
   for (auto P : make_range(MV.begin(), MV.end())) {
      ASSERT_EQ(P.first, count);
      count++;
   }

   count = 6;
   for (auto P : make_range(MV.rbegin(), MV.rend())) {
      ASSERT_EQ(P.first, count);
      count--;
   }
}

} // anonymous namespace
