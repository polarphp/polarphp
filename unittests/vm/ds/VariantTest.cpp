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

#include "polarphp/vm/ds/Variant.h"

using polar::vmapi::Variant;
using polar::vmapi::Type;

TEST(VariantTest, testRefConstruct)
{
   zval var1;
   ZVAL_LONG(&var1, 123);
   ASSERT_EQ(Z_LVAL(var1), 123);

   Variant var(var1, true);
   ASSERT_EQ(Z_LVAL(var.getZval()), 123);

   var = 213;
   ASSERT_EQ(Z_LVAL(var.getZval()), 213);
   ASSERT_EQ(Z_LVAL_P(Z_REFVAL(var1)), 213);

   var = "polarphp";
   ASSERT_STREQ(Z_STRVAL(var.getZval()), "polarphp");
   ASSERT_STREQ(Z_STRVAL_P(Z_REFVAL(var1)), "polarphp");

   var = Variant(2019);
   ASSERT_EQ(Z_LVAL(var.getZval()), 2019);
   ASSERT_EQ(Z_LVAL_P(Z_REFVAL(var1)), 2019);

   zval_dtor(&var1);
}

TEST(VariantTest, testCompareOperators)
{
   zval var1;
   ZVAL_LONG(&var1, 123);
   Variant variant1(var1, true);
   zval var2;
   ZVAL_LONG(&var2, 123);
   Variant variant2(var2, true);
   Variant variant3(123);
   ASSERT_TRUE(variant1 == variant2);
   ASSERT_TRUE(variant1 == variant3);
   zval_dtor(&var1);
   zval_dtor(&var2);
}
