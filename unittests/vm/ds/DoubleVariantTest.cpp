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

using polar::vmapi::Type;
using polar::vmapi::NumericVariant;
using polar::vmapi::DoubleVariant;
using polar::vmapi::Variant;

TEST(DoubleVariantTest, testMoveConstruct)
{
   // test move construct
   DoubleVariant num1(123);
   DoubleVariant num2(num1, true);
   ASSERT_EQ(num1.getUnDerefType(), Type::Reference);
   ASSERT_EQ(num2.getUnDerefType(), Type::Reference);
   DoubleVariant num3(std::move(num1));
   ASSERT_EQ(num3.getUnDerefType(), Type::Reference);
   DoubleVariant num4(num3);
   ASSERT_EQ(num4.getUnDerefType(), Type::Double);
}

TEST(DoubleVariantTest, testRefConstruct)
{
   {
      DoubleVariant num1(123);
      DoubleVariant num2(num1, false);
      ASSERT_EQ(num1.getUnDerefType(), Type::Double);
      ASSERT_EQ(num2.getUnDerefType(), Type::Double);
      ASSERT_EQ(num1.toDouble(), 123);
      ASSERT_EQ(num2.toDouble(), 123);
   }
   {
      DoubleVariant num1(123);
      DoubleVariant num2(num1, true);
      ASSERT_EQ(num1.getUnDerefType(), Type::Reference);
      ASSERT_EQ(num2.getUnDerefType(), Type::Reference);
      ASSERT_EQ(num1.toDouble(), 123);
      ASSERT_EQ(num2.toDouble(), 123);
      DoubleVariant num3(num2, false);
      DoubleVariant num4(num1);
      ASSERT_EQ(num3.toDouble(), 123);
      ASSERT_EQ(num4.toDouble(), 123);
      num1 = 321;
      ASSERT_EQ(num1.toDouble(), 321);
      ASSERT_EQ(num2.toDouble(), 321);
      ASSERT_EQ(num3.toDouble(), 123);
      ASSERT_EQ(num4.toDouble(), 123);
      num3 = num1;
      ASSERT_EQ(num3.toDouble(), 321);
      ASSERT_EQ(num3.getUnDerefType(), Type::Double);
   }
   {
      zval numVar;
      ZVAL_DOUBLE(&numVar, 123);
      DoubleVariant num2(numVar, false);
      ASSERT_EQ(num2.getUnDerefType(), Type::Double);
      ASSERT_EQ(num2.toDouble(), 123);
      ASSERT_EQ(Z_TYPE(numVar), IS_DOUBLE);
   }
   {
      zval numVar;
      ZVAL_DOUBLE(&numVar, 123);
      DoubleVariant num1(numVar, true);
      ASSERT_EQ(num1.getUnDerefType(), Type::Reference);
      ASSERT_EQ(num1.getType(), Type::Double);
      ASSERT_EQ(num1.toDouble(), 123);
      zval *rval = &numVar;
      ZVAL_DEREF(rval);
      ASSERT_TRUE(Z_TYPE_P(rval) == IS_DOUBLE);
      num1 = 321;
      ASSERT_EQ(num1.toDouble(), 321);
      ASSERT_EQ(Z_DVAL_P(rval), 321);
      zval_dtor(&numVar);
   }
}
