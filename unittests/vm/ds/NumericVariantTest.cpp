// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/28.

#include "gtest/gtest.h"

#include "polarphp/vm/ds/DoubleVariant.h"
#include "polarphp/vm/ds/NumericVariant.h"

using polar::vmapi::Type;
using polar::vmapi::NumericVariant;
using polar::vmapi::DoubleVariant;
using polar::vmapi::Variant;

TEST(NumericVarintTest, testBaseFuncs)
{
   NumericVariant num1(12);
   NumericVariant num2(21);
   ASSERT_FALSE(num1 == num2);
   ASSERT_TRUE(num1 != num2);
   ASSERT_TRUE(num1 < num2);
   ASSERT_TRUE(num1 <= num2);
   ASSERT_FALSE(num1 > num2);
   ASSERT_FALSE(num1 >= num2);
   ASSERT_FALSE(num1 == 13);
   ASSERT_TRUE(num1 != 13);
   ASSERT_TRUE(num1 < 13);
   ASSERT_TRUE(num1 <= 13);
   ASSERT_FALSE(num1 > 13);
   ASSERT_FALSE(num1 >= 13);
   ASSERT_TRUE(12 != num2);
   ASSERT_TRUE(12 < num2);
   ASSERT_TRUE(12 <= num2);
   ASSERT_FALSE(12 > num2);
   ASSERT_FALSE(12 >= num2);
   Variant gvar(666);
   NumericVariant gcopy(gvar);
   ASSERT_EQ(gcopy.toLong(), 666);
}

TEST(NumericVariantTest, testAddOps)
{
   NumericVariant num1(1);
   num1++;
   ASSERT_EQ(num1, 2);
   num1 += 3;
   ASSERT_EQ(num1, 5);
   NumericVariant num2(3);
   ASSERT_EQ(num1 + num2, 8);
   num1 += 2.2; // trucate
   ASSERT_EQ(num1, 7);
   num1 += 2.7;
   ASSERT_EQ(num1, 10);
   ASSERT_EQ(num1 + 1, 11);
   ASSERT_EQ(1 + num1, 11);
   ASSERT_EQ(1.0 + num1, 11.0);
   ASSERT_EQ(num1 + 1.0, 11.0);
   DoubleVariant dnum1(1.0);
   ASSERT_EQ(num1 + dnum1, 11.0);
   dnum1 += 2.3;
   dnum1 %= 2.1;
   DoubleVariant dnum2(1.2);
   EXPECT_DOUBLE_EQ(dnum1, 1.2);
}

TEST(NumericVariantTest, testConstructors)
{
   NumericVariant empty;
   ASSERT_EQ(empty.toLong(), 0);
   NumericVariant num1(1);
   ASSERT_EQ(num1.toLong(), 1);
   empty = 123;
   ASSERT_EQ(empty.toLong(), 123);
   ASSERT_FALSE(empty.isNull());
   NumericVariant num2(std::move(empty)); // move construct from here you can't do anything about empty
   ASSERT_EQ(num2.toLong(), 123);
   NumericVariant num4(222);
   num2 = num4;
   ASSERT_EQ(num4.toLong(), 222);
   ASSERT_EQ(num2.toLong(), 222); // move assign from here you can't do anything about num4
   num2 = std::move(num4);
   Variant num5(5);
   num2 = num5;
   ASSERT_EQ(num2.toLong(), 5);
   ASSERT_EQ(Z_LVAL_P(num5.getZvalPtr()), 5);
   num2 = std::move(num5);
   ASSERT_EQ(num2.toLong(), 5);
   {
      // test move construct
      NumericVariant num1(123);
      NumericVariant num2(num1, true);
      ASSERT_EQ(num1.getUnDerefType(), Type::Reference);
      ASSERT_EQ(num2.getUnDerefType(), Type::Reference);
      NumericVariant num3(std::move(num1));
      ASSERT_EQ(num3.getUnDerefType(), Type::Reference);
      NumericVariant num4(num3);
      ASSERT_EQ(num4.getUnDerefType(), Type::Long);
   }
}


TEST(NumericVariantTest, testAssignOperators)
{
   NumericVariant num1(1);
   ASSERT_EQ(num1, 1);
   num1 = 12;
   ASSERT_EQ(num1, 12);
   num1 = 222;
   ASSERT_EQ(num1, 222);
   NumericVariant num2 = num1;
   ASSERT_EQ(num2, 222);
   num1 = 666;
   num2 = num1;
   DoubleVariant dval(2.2);
   num2 = dval;
   ASSERT_EQ(num2, 2);
}

TEST(NumericVariantTest, testRefConstruct)
{
   {
      NumericVariant num1(123);
      NumericVariant num2(num1, false);
      ASSERT_EQ(num1.getUnDerefType(), Type::Numeric);
      ASSERT_EQ(num2.getUnDerefType(), Type::Numeric);
      ASSERT_EQ(num1.toLong(), 123);
      ASSERT_EQ(num2.toLong(), 123);
   }
   {
      NumericVariant num1(123);
      NumericVariant num2(num1, true);
      ASSERT_EQ(num1.getUnDerefType(), Type::Reference);
      ASSERT_EQ(num2.getUnDerefType(), Type::Reference);
      ASSERT_EQ(num1.toLong(), 123);
      ASSERT_EQ(num2.toLong(), 123);
      NumericVariant num3(num2, false);
      NumericVariant num4(num1);
      ASSERT_EQ(num3.toLong(), 123);
      ASSERT_EQ(num4.toLong(), 123);
      num1 = 321;
      ASSERT_EQ(num1.toLong(), 321);
      ASSERT_EQ(num2.toLong(), 321);
      ASSERT_EQ(num3.toLong(), 123);
      ASSERT_EQ(num4.toLong(), 123);
      num3 = num1;
      ASSERT_EQ(num3.toLong(), 321);
      ASSERT_EQ(num3.getUnDerefType(), Type::Numeric);
   }
   {
      zval numVar;
      ZVAL_LONG(&numVar, 123);
      NumericVariant num2(numVar, false);
      ASSERT_EQ(num2.getUnDerefType(), Type::Numeric);
      ASSERT_EQ(num2.toLong(), 123);
      ASSERT_EQ(Z_TYPE(numVar), IS_LONG);
   }
   {
      zval numVar;
      ZVAL_LONG(&numVar, 123);
      NumericVariant num2(numVar, true);
      ASSERT_EQ(num2.getUnDerefType(), Type::Reference);
      ASSERT_EQ(num2.getType(), Type::Numeric);
      ASSERT_EQ(num2.toLong(), 123);
      zval *rval = &numVar;
      ZVAL_DEREF(rval);
      ASSERT_TRUE(Z_TYPE_P(rval) == IS_LONG);
      num2 = 321;
      ASSERT_EQ(num2.toLong(), 321);
      ASSERT_EQ(Z_LVAL_P(rval), 321);
      zval_dtor(&numVar);
   }
}
