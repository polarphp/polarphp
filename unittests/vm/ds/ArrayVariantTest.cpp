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

#include "polarphp/vm/ds/ArrayVariant.h"
#include "polarphp/vm/ds/ArrayItemProxy.h"
#include "polarphp/vm/ds/NumericVariant.h"
#include "polarphp/vm/ds/DoubleVariant.h"
#include "polarphp/vm/ds/StringVariant.h"
#include "polarphp/vm/ds/BooleanVariant.h"

#include <list>

using polar::vmapi::ArrayVariant;
using polar::vmapi::Variant;
using polar::vmapi::NumericVariant;
using polar::vmapi::StringVariant;
using polar::vmapi::BooleanVariant;
using polar::vmapi::DoubleVariant;
using KeyType = ArrayVariant::KeyType;

TEST(ArrayVariantTest, testConstructor)
{
   ArrayVariant array;
   ASSERT_FALSE(array.isNull());
   ASSERT_TRUE(array.isEmpty());
   ASSERT_TRUE(array.isArray());
   ASSERT_EQ(array.getCapacity(), 8);
}

TEST(ArrayVariantTest, testRefConstruct)
{
   {
      zval arrVar;
      array_init(&arrVar);
      ArrayVariant arr1(arrVar);
      ArrayVariant arr2(arr1);
      ArrayVariant arr3(arr2);
      ASSERT_EQ(arr1.getRefCount(), 4);
      ASSERT_EQ(arr2.getRefCount(), 4);
      ASSERT_EQ(arr3.getRefCount(), 4);
      ASSERT_EQ(Z_REFCOUNT_P(&arrVar), 4);
      zval_dtor(&arrVar);
   }
   {
      zval arrVar;
      array_init(&arrVar);
      ArrayVariant arr1(arrVar, true);
      ArrayVariant arr2(arr1, true);
      ArrayVariant arr3(arr2, false);
      ASSERT_EQ(arr1.getRefCount(), 3);
      ASSERT_EQ(arr2.getRefCount(), 3);
      ASSERT_EQ(arr3.getRefCount(), 1);
      zval *rval = &arrVar;
      ZVAL_DEREF(rval);
      ASSERT_EQ(Z_REFCOUNT_P(rval), 1);

      ASSERT_EQ(arr1.getSize(), 0);
      ASSERT_EQ(arr2.getSize(), 0);
      ASSERT_EQ(arr3.getSize(), 0);
      arr1.append(1);
      ASSERT_EQ(arr1.getSize(), 1);
      ASSERT_EQ(arr2.getSize(), 1);
      ASSERT_EQ(arr3.getSize(), 0);
      zval_dtor(&arrVar);
   }
}

TEST(ArrayVariantTest, testCopyConstructor)
{
   ArrayVariant array;
   array.insert("name", "polarphp");
   array.insert("address", "beijing");
   ASSERT_EQ(array.getSize(), 2);
   ASSERT_EQ(array.getRefCount(), 1);
   ArrayVariant array1 = array;
   ASSERT_EQ(array.getSize(), 2);
   ASSERT_EQ(array.getRefCount(), 2);
   ASSERT_EQ(array1.getSize(), 2);
   ASSERT_EQ(array1.getRefCount(), 2);
   array.insert("age", 12);
   ASSERT_EQ(array.getSize(), 3);
   ASSERT_EQ(array.getRefCount(), 1);
   ASSERT_EQ(array1.getSize(), 2);
   ASSERT_EQ(array1.getRefCount(), 1);

   // copy from Variant
   {
      Variant val1; // copy from empty variant
      ArrayVariant array2(val1);
      ASSERT_EQ(array2.getSize(), 0);
      ASSERT_EQ(array2.getRefCount(), 1);
      Variant val3(123);
      ASSERT_EQ(val3.getRefCount(), 0);
      ArrayVariant array3(val3);
      ASSERT_EQ(array3.getSize(), 1);
      ASSERT_EQ((array3[0]).toNumericVariant().toLong(), 123);
      ASSERT_EQ(val3.getRefCount(), 0);
      ASSERT_EQ(array3.getRefCount(), 1);

      // test string
      Variant val4("polarphp");
      ASSERT_EQ(val4.getRefCount(), 1);
      ArrayVariant array4(val4);
      ASSERT_EQ(val4.getRefCount(), 2);
      ASSERT_EQ(array4.getRefCount(), 1);
      ASSERT_STREQ((array4[0]).toStringVariant().getCStr(), "polarphp");

      // test array
      Variant val5(array4);
      ASSERT_EQ(val5.getRefCount(), 2);
      ASSERT_EQ(array4.getRefCount(), 2);
      ArrayVariant array5(val5);
      ASSERT_EQ(val5.getRefCount(), 3);
      ASSERT_EQ(array5.getRefCount(), 3);
      ASSERT_EQ(array5.getSize(), 1);
      ASSERT_STREQ((array5[0]).toStringVariant().getCStr(), "polarphp");
      array5[1] = 123;

      ASSERT_EQ(array4.getRefCount(), 2);
      ASSERT_EQ(val5.getRefCount(), 2);
      ASSERT_EQ(array5.getRefCount(), 1);
   }
   {
      // test string covert to array copy on write
      Variant str("polarphp");
      ArrayVariant array(str);
      ASSERT_STREQ((array[0]).toStringVariant().getCStr(), "polarphp");
      ASSERT_EQ(str.toString(), "polarphp");
      array[0] = "polarboy";
      ASSERT_EQ(str.toString(), "polarphp");
      ASSERT_STREQ((array[0]).toStringVariant().getCStr(), "polarboy");
   }
   {
      // test move Variant
      Variant val1; // copy from empty variant
      ArrayVariant array1(std::move(val1));
      ASSERT_EQ(array1.getSize(), 0);
      ASSERT_EQ(array1.getRefCount(), 1);
      // move scalar Variant
      Variant val2(3.14);
      ASSERT_EQ(val2.getRefCount(), 0);
      ArrayVariant array2(std::move(val2));
      ASSERT_EQ(array2.getRefCount(), 1);
      ASSERT_EQ((array2[0]).toDoubleVariant().toDouble(), 3.14);

      Variant val3(true);
      ASSERT_EQ(val3.getRefCount(), 0);
      ArrayVariant array3(std::move(val3));
      ASSERT_EQ(array3.getRefCount(), 1);
      ASSERT_EQ((array3[0]).toBooleaneanVariant().toBoolean(), true);

      ArrayVariant infoArray;
      infoArray.append("zzu_softboy");
      infoArray["team"] = "polarphp";
      infoArray["age"] = 123;
      Variant val4(infoArray);
      ASSERT_EQ(infoArray.getRefCount(), 2);
      ASSERT_EQ(val4.getRefCount(), 2);
      ArrayVariant array4(std::move(val4));
      ASSERT_EQ(infoArray.getRefCount(), 2);
      ASSERT_EQ(array4.getRefCount(), 2);
      ASSERT_STREQ((array4[0]).toStringVariant().getCStr(), "zzu_softboy");
      ASSERT_STREQ((array4["team"]).toStringVariant().getCStr(), "polarphp");
      ASSERT_EQ((array4["age"]).toNumericVariant().toLong(), 123);
   }
}
