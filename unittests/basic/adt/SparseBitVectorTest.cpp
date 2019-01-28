// This source file is part of the polarphp.org open source project

// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception

// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

// Created by polarboy on 2018/07/09.

#include "polarphp/basic/adt/SparseBitVector.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {

TEST(SparseBitVectorTest, testTrivialOperation)
{
   SparseBitVector<> vector;
   EXPECT_EQ(0U, vector.count());
   EXPECT_FALSE(vector.test(17));
   vector.set(5);
   EXPECT_TRUE(vector.test(5));
   EXPECT_FALSE(vector.test(17));
   vector.reset(6);
   EXPECT_TRUE(vector.test(5));
   EXPECT_FALSE(vector.test(6));
   vector.reset(5);
   EXPECT_FALSE(vector.test(5));
   EXPECT_TRUE(vector.testAndSet(17));
   EXPECT_FALSE(vector.testAndSet(17));
   EXPECT_TRUE(vector.test(17));
   vector.clear();
   EXPECT_FALSE(vector.test(17));

   vector.set(5);
   const SparseBitVector<> constVec = vector;
   EXPECT_TRUE(constVec.test(5));
   EXPECT_FALSE(constVec.test(17));

   vector.set(1337);
   EXPECT_TRUE(vector.test(1337));
   vector = constVec;
   EXPECT_FALSE(vector.test(1337));

   vector.set(1337);
   EXPECT_FALSE(vector.empty());
   SparseBitVector<> MovedVec(std::move(vector));
   EXPECT_TRUE(vector.empty());
   EXPECT_TRUE(MovedVec.test(5));
   EXPECT_TRUE(MovedVec.test(1337));

   vector = std::move(MovedVec);
   EXPECT_TRUE(MovedVec.empty());
   EXPECT_FALSE(vector.empty());
}

TEST(SparseBitVectorTest, testIntersectWith)
{
   SparseBitVector<> vector, other;

   vector.set(1);
   other.set(1);
   EXPECT_FALSE(vector &= other);
   EXPECT_TRUE(vector.test(1));

   vector.clear();
   vector.set(5);
   other.clear();
   other.set(6);
   EXPECT_TRUE(vector &= other);
   EXPECT_TRUE(vector.empty());

   vector.clear();
   vector.set(5);
   other.clear();
   other.set(225);
   EXPECT_TRUE(vector &= other);
   EXPECT_TRUE(vector.empty());

   vector.clear();
   vector.set(225);
   other.clear();
   other.set(5);
   EXPECT_TRUE(vector &= other);
   EXPECT_TRUE(vector.empty());
}

TEST(SparseBitVectorTest, testSelfAssignment)
{
   SparseBitVector<> vector, other;

   vector.set(23);
   vector.set(234);
   vector = static_cast<SparseBitVector<> &>(vector);
   EXPECT_TRUE(vector.test(23));
   EXPECT_TRUE(vector.test(234));

   vector.clear();
   vector.set(17);
   vector.set(256);
   EXPECT_FALSE(vector |= vector);
   EXPECT_TRUE(vector.test(17));
   EXPECT_TRUE(vector.test(256));

   vector.clear();
   vector.set(56);
   vector.set(517);
   EXPECT_FALSE(vector &= vector);
   EXPECT_TRUE(vector.test(56));
   EXPECT_TRUE(vector.test(517));

   vector.clear();
   vector.set(99);
   vector.set(333);
   EXPECT_TRUE(vector.intersectWithComplement(vector));
   EXPECT_TRUE(vector.empty());
   EXPECT_FALSE(vector.intersectWithComplement(vector));

   vector.clear();
   vector.set(28);
   vector.set(43);
   vector.intersectWithComplement(vector, vector);
   EXPECT_TRUE(vector.empty());

   vector.clear();
   vector.set(42);
   vector.set(567);
   other.set(55);
   other.set(567);
   vector.intersectWithComplement(vector, other);
   EXPECT_TRUE(vector.test(42));
   EXPECT_FALSE(vector.test(567));

   vector.clear();
   vector.set(19);
   vector.set(21);
   other.clear();
   other.set(19);
   other.set(31);
   vector.intersectWithComplement(other, vector);
   EXPECT_FALSE(vector.test(19));
   EXPECT_TRUE(vector.test(31));

   vector.clear();
   vector.set(1);
   other.clear();
   other.set(59);
   other.set(75);
   vector.intersectWithComplement(other, other);
   EXPECT_TRUE(vector.empty());
}

TEST(SparseBitVectorTest, testFind)
{
   SparseBitVector<> vector;
   vector.set(1);
   EXPECT_EQ(1, vector.findFirst());
   EXPECT_EQ(1, vector.findLast());

   vector.set(2);
   EXPECT_EQ(1, vector.findFirst());
   EXPECT_EQ(2, vector.findLast());

   vector.set(0);
   vector.set(3);
   EXPECT_EQ(0, vector.findFirst());
   EXPECT_EQ(3, vector.findLast());

   vector.reset(1);
   vector.reset(0);
   vector.reset(3);
   EXPECT_EQ(2, vector.findFirst());
   EXPECT_EQ(2, vector.findLast());

   // Set some large bits to ensure we are pulling bits from more than just a
   // single bitword.
   vector.set(500);
   vector.set(2000);
   vector.set(3000);
   vector.set(4000);
   vector.reset(2);
   EXPECT_EQ(500, vector.findFirst());
   EXPECT_EQ(4000, vector.findLast());

   vector.reset(500);
   vector.reset(3000);
   vector.reset(4000);
   EXPECT_EQ(2000, vector.findFirst());
   EXPECT_EQ(2000, vector.findLast());

   vector.clear();
}

}
