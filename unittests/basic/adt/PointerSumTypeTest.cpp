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

#include "polarphp/basic/adt/PointerSumType.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {

struct PointerSumTypeTest : public testing::Test
{
   enum Kinds { Float, Int1, Int2 };
   float f;
   int i1, i2;

   typedef PointerSumType<Kinds, PointerSumTypeMember<Float, float *>,
   PointerSumTypeMember<Int1, int *>,
   PointerSumTypeMember<Int2, int *>>
   SumType;
   SumType a, b, c, n;

   PointerSumTypeTest()
      : f(3.14f), i1(42), i2(-1), a(SumType::create<Float>(&f)),
        b(SumType::create<Int1>(&i1)), c(SumType::create<Int2>(&i2)), n() {}
};

TEST_F(PointerSumTypeTest, testNullTest)
{
   EXPECT_TRUE(a);
   EXPECT_TRUE(b);
   EXPECT_TRUE(c);
   EXPECT_FALSE(n);
}

TEST_F(PointerSumTypeTest, testGetTag)
{
   EXPECT_EQ(Float, a.getTag());
   EXPECT_EQ(Int1, b.getTag());
   EXPECT_EQ(Int2, c.getTag());
   EXPECT_EQ((Kinds)0, n.getTag());
}

TEST_F(PointerSumTypeTest, testIs)
{
   EXPECT_TRUE(a.is<Float>());
   EXPECT_FALSE(a.is<Int1>());
   EXPECT_FALSE(a.is<Int2>());
   EXPECT_FALSE(b.is<Float>());
   EXPECT_TRUE(b.is<Int1>());
   EXPECT_FALSE(b.is<Int2>());
   EXPECT_FALSE(c.is<Float>());
   EXPECT_FALSE(c.is<Int1>());
   EXPECT_TRUE(c.is<Int2>());
}

TEST_F(PointerSumTypeTest, testGet)
{
   EXPECT_EQ(&f, a.get<Float>());
   EXPECT_EQ(nullptr, a.get<Int1>());
   EXPECT_EQ(nullptr, a.get<Int2>());
   EXPECT_EQ(nullptr, b.get<Float>());
   EXPECT_EQ(&i1, b.get<Int1>());
   EXPECT_EQ(nullptr, b.get<Int2>());
   EXPECT_EQ(nullptr, c.get<Float>());
   EXPECT_EQ(nullptr, c.get<Int1>());
   EXPECT_EQ(&i2, c.get<Int2>());

   // Note that we can use .get even on a null sum type. It just always produces
   // a null pointer, even if one of the discriminants is null.
   EXPECT_EQ(nullptr, n.get<Float>());
   EXPECT_EQ(nullptr, n.get<Int1>());
   EXPECT_EQ(nullptr, n.get<Int2>());
}

TEST_F(PointerSumTypeTest, testCast)
{
   EXPECT_EQ(&f, a.cast<Float>());
   EXPECT_EQ(&i1, b.cast<Int1>());
   EXPECT_EQ(&i2, c.cast<Int2>());
}

TEST_F(PointerSumTypeTest, testAssignment)
{
   b = SumType::create<Int2>(&i2);
   EXPECT_EQ(nullptr, b.get<Float>());
   EXPECT_EQ(nullptr, b.get<Int1>());
   EXPECT_EQ(&i2, b.get<Int2>());

   b = SumType::create<Int2>(&i1);
   EXPECT_EQ(nullptr, b.get<Float>());
   EXPECT_EQ(nullptr, b.get<Int1>());
   EXPECT_EQ(&i1, b.get<Int2>());

   float Local = 1.616f;
   b = SumType::create<Float>(&Local);
   EXPECT_EQ(&Local, b.get<Float>());
   EXPECT_EQ(nullptr, b.get<Int1>());
   EXPECT_EQ(nullptr, b.get<Int2>());

   n = SumType::create<Int1>(&i2);
   EXPECT_TRUE(n);
   EXPECT_EQ(nullptr, n.get<Float>());
   EXPECT_EQ(&i2, n.get<Int1>());
   EXPECT_EQ(nullptr, n.get<Int2>());

   n = SumType::create<Float>(nullptr);
   EXPECT_FALSE(n);
   EXPECT_EQ(nullptr, n.get<Float>());
   EXPECT_EQ(nullptr, n.get<Int1>());
   EXPECT_EQ(nullptr, n.get<Int2>());
}

} // anonymous namespace
