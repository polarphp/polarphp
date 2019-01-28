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

#ifndef __ppc__

#include "polarphp/basic/adt/PackedVector.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {

TEST(PackedVectorTest, testOperation)
{
   PackedVector<unsigned, 2> vec;
   EXPECT_EQ(0U, vec.size());
   EXPECT_TRUE(vec.empty());

   vec.resize(5);
   EXPECT_EQ(5U, vec.size());
   EXPECT_FALSE(vec.empty());

   vec.resize(11);
   EXPECT_EQ(11U, vec.size());
   EXPECT_FALSE(vec.empty());

   PackedVector<unsigned, 2> Vec2(3);
   EXPECT_EQ(3U, Vec2.size());
   EXPECT_FALSE(Vec2.empty());

   vec.clear();
   EXPECT_EQ(0U, vec.size());
   EXPECT_TRUE(vec.empty());

   vec.push_back(2);
   vec.push_back(0);
   vec.push_back(1);
   vec.push_back(3);

   EXPECT_EQ(2U, vec[0]);
   EXPECT_EQ(0U, vec[1]);
   EXPECT_EQ(1U, vec[2]);
   EXPECT_EQ(3U, vec[3]);

   EXPECT_FALSE(vec == Vec2);
   EXPECT_TRUE(vec != Vec2);

   vec = Vec2;
   EXPECT_TRUE(vec == Vec2);
   EXPECT_FALSE(vec != Vec2);

   vec[1] = 1;
   Vec2[1] = 2;
   vec |= Vec2;
   EXPECT_EQ(3U, vec[1]);
}

#ifdef EXPECT_DEBUG_DEATH

TEST(PackedVectorTest, testUnsignedValues)
{
   PackedVector<unsigned, 2> vec(1);
   vec[0] = 0;
   vec[0] = 1;
   vec[0] = 2;
   vec[0] = 3;
   EXPECT_DEBUG_DEATH(vec[0] = 4, "value is too big");
   EXPECT_DEBUG_DEATH(vec[0] = -1, "value is too big");
   EXPECT_DEBUG_DEATH(vec[0] = 0x100, "value is too big");

   PackedVector<unsigned, 3> Vec2(1);
   Vec2[0] = 0;
   Vec2[0] = 7;
   EXPECT_DEBUG_DEATH(vec[0] = 8, "value is too big");
}

TEST(PackedVectorTest, testSignedValues)
{
   PackedVector<signed, 2> vec(1);
   vec[0] = -2;
   vec[0] = -1;
   vec[0] = 0;
   vec[0] = 1;
   EXPECT_DEBUG_DEATH(vec[0] = -3, "value is too big");
   EXPECT_DEBUG_DEATH(vec[0] = 2, "value is too big");

   PackedVector<signed, 3> Vec2(1);
   Vec2[0] = -4;
   Vec2[0] = 3;
   EXPECT_DEBUG_DEATH(vec[0] = -5, "value is too big");
   EXPECT_DEBUG_DEATH(vec[0] = 4, "value is too big");
}

#endif

} // anonymous namespace

#endif
