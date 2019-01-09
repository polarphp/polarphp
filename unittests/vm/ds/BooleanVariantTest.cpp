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

#include "polarphp/vm/ds/BooleanVariant.h"

using polar::vmapi::Variant;
using polar::vmapi::BooleanVariant;
using polar::vmapi::Type;

TEST(BooleanVariantTest, testBoolVaraint)
{
   BooleanVariant defaultVar;
   BooleanVariant trueVar(true);
   BooleanVariant falseVar(false);
   ASSERT_FALSE(defaultVar);
   ASSERT_TRUE(trueVar);
   ASSERT_FALSE(falseVar);
   ASSERT_EQ(falseVar, defaultVar);
   BooleanVariant copyed(trueVar);
   ASSERT_TRUE(copyed);
   copyed = falseVar;
   ASSERT_FALSE(copyed);
   copyed = true;
   ASSERT_TRUE(copyed);
   copyed = false;
   ASSERT_FALSE(copyed);
   copyed = 1;
   ASSERT_TRUE(copyed);
   copyed = 0;
   ASSERT_FALSE(copyed);
   copyed = 0.0;
   ASSERT_FALSE(copyed);
   copyed = 1;
   ASSERT_TRUE(copyed);
   Variant baseVar(true);
   BooleanVariant copyFromBaseVar(baseVar);
   ASSERT_TRUE(copyFromBaseVar);
   baseVar = false;
   ASSERT_FALSE(baseVar);
   ASSERT_TRUE(copyFromBaseVar);
}

TEST(BooleanVariantTest, testMoveConstuctorAndAssign)
{
   BooleanVariant bool1(true);
   BooleanVariant bool2(false);
   Variant bool3(true);
   Variant str("polarphp"); // true
   BooleanVariant bool4(std::move(bool1));
   ASSERT_TRUE(bool4);
   bool4 = bool2;
   ASSERT_FALSE(bool4);
   ASSERT_FALSE(bool2);
   bool4 = std::move(bool2);
   ASSERT_FALSE(bool4);
   bool4 = bool3;
   ASSERT_TRUE(bool4);
   ASSERT_TRUE(bool3);
   bool4 = std::move(bool3);
   ASSERT_TRUE(bool4);
   bool4 = str;
   ASSERT_TRUE(bool4);
   ASSERT_TRUE(nullptr != str.getZvalPtr());
   bool4 = std::move(str);
   ASSERT_TRUE(bool4);
   {
      // test ref
      Variant variant1(true);
      Variant variant2(variant1, true);
      BooleanVariant boolVariant(variant2);
      ASSERT_EQ(boolVariant.getUnDerefType(), Type::True);
      BooleanVariant boolVariant1(std::move(variant2));
      ASSERT_EQ(boolVariant1.getUnDerefType(), Type::True);
   }
   {
      // test ref
      Variant variant1(123);
      Variant variant2(variant1, true);
      BooleanVariant boolVariant(variant2);
      ASSERT_EQ(boolVariant.getUnDerefType(), Type::True);
      BooleanVariant boolVariant1(std::move(variant2));
      ASSERT_EQ(boolVariant1.getUnDerefType(), Type::True);
   }
}

TEST(BooleanVariantTest, testMoveConstruct)
{
   // test move construct
   BooleanVariant bool1(true);
   BooleanVariant bool2(bool1, true);
   ASSERT_EQ(bool1.getUnDerefType(), Type::Reference);
   ASSERT_EQ(bool2.getUnDerefType(), Type::Reference);
   BooleanVariant bool3(std::move(bool1));
   ASSERT_EQ(bool3.getUnDerefType(), Type::Reference);
   BooleanVariant bool4(bool3);
   ASSERT_EQ(bool4.getUnDerefType(), Type::True);
}

TEST(BooleanVariantTest, testRefConstruct)
{
   {
      BooleanVariant bool1(true);
      ASSERT_EQ(bool1.getUnDerefType(), Type::True);
      BooleanVariant bool2(bool1, false);
      ASSERT_EQ(bool2.getUnDerefType(), Type::True);
   }
   {
      BooleanVariant bool1(true);
      ASSERT_EQ(bool1.getUnDerefType(), Type::True);
      BooleanVariant bool2(bool1, true);
      ASSERT_EQ(bool2.getUnDerefType(), Type::Reference);
      ASSERT_EQ(bool2.getType(), Type::True);
      BooleanVariant bool3 = bool2;
      ASSERT_EQ(bool3.getUnDerefType(), Type::True);
      ASSERT_EQ(bool3.getType(), Type::True);
      ASSERT_TRUE(bool1.toBoolean());
      ASSERT_TRUE(bool2.toBoolean());
      bool2 = false;
      ASSERT_FALSE(bool1.toBoolean());
      ASSERT_FALSE(bool2.toBoolean());
      ASSERT_TRUE(bool3.toBoolean());
      bool3 = bool2;
      ASSERT_FALSE(bool1.toBoolean());
      ASSERT_FALSE(bool2.toBoolean());
      ASSERT_FALSE(bool3.toBoolean());
      ASSERT_EQ(bool3.getType(), Type::False);
   }
   {
      zval var1;
      ZVAL_BOOL(&var1, true);
      BooleanVariant bool1(var1, false);
      ASSERT_EQ(bool1.getUnDerefType(), Type::True);
      ASSERT_TRUE(Z_TYPE_P(&var1) == IS_TRUE);
      BooleanVariant bool2(var1, true);
      ASSERT_EQ(bool2.getUnDerefType(), Type::Reference);
      ASSERT_TRUE(Z_TYPE_P(&var1) == IS_REFERENCE);
      zval_dtor(&var1);
      bool2 = false;
      ASSERT_FALSE(bool2.toBoolean());
      zval *rval = &var1;
      ZVAL_DEREF(rval);
      ASSERT_TRUE(Z_TYPE_P(rval) == IS_FALSE);
   }
}
