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
      ASSERT_EQ((array3[0]).toBooleanVariant().toBoolean(), true);

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

TEST(ArrayVariantTest, testMoveConstructor)
{
   ArrayVariant array;
   array.insert("name", "polarphp");
   array.insert("address", "beijing");
   ASSERT_EQ(array.getSize(), 2);
   ASSERT_EQ(array.getRefCount(), 1);
   ArrayVariant array1 = std::move(array);
   // from here you can't do anything about array
   ASSERT_EQ(array1.getSize(), 2);
   ASSERT_EQ(array1.getRefCount(), 1);
}

TEST(ArrayVariantTest, testCopyFromInitList)
{
   {
      ArrayVariant arrVal;
      arrVal.insert(0, 312);
      arrVal.insert("name", "polarphp");
      arrVal.insert("age", 11);
      ASSERT_EQ(arrVal.getRefCount(), 1);
      ArrayVariant array({
                            {0, 1.2},
                            {1, "polarphp"},
                            {2, true},
                            {3, 123},
                            {"data", arrVal}
                         });
      ASSERT_EQ(arrVal.getRefCount(), 2);
      ASSERT_EQ(array.getRefCount(), 1);
      ASSERT_EQ(array.getSize(), 5);
      ASSERT_EQ((array[0]).toDoubleVariant().toDouble(), 1.2);
      ASSERT_STREQ((array[1]).toStringVariant().getCStr(), "polarphp");
      ASSERT_EQ((array[2]).toBooleanVariant().toBoolean(), true);
      ASSERT_EQ((array[3]).toNumericVariant().toLong(), 123);
      ArrayVariant subArr = array["data"];
      ASSERT_EQ(arrVal.getRefCount(), 3);
      ASSERT_EQ(subArr.getRefCount(), 3);
      ASSERT_EQ((subArr[0]).toNumericVariant().toLong(), 312);
      ASSERT_STREQ((subArr["name"]).toStringVariant().getCStr(), "polarphp");
      ASSERT_EQ((subArr["age"]).toNumericVariant().toLong(), 11);
   }
   {
      ArrayVariant arrVal;
      arrVal.insert(0, 312);
      arrVal.insert("name", "polarphp");
      arrVal.insert("age", 11);
      ASSERT_EQ(arrVal.getRefCount(), 1);
      ArrayVariant array({1.2, "polarphp", true, 123, arrVal});
      ASSERT_EQ(arrVal.getRefCount(), 2);
      ASSERT_EQ(array.getRefCount(), 1);
      ASSERT_EQ(array.getSize(), 5);
      ASSERT_EQ((array[0]).toDoubleVariant().toDouble(), 1.2);
      ASSERT_STREQ((array[1]).toStringVariant().getCStr(), "polarphp");
      ASSERT_EQ((array[2]).toBooleanVariant().toBoolean(), true);
      ASSERT_EQ((array[3]).toNumericVariant().toLong(), 123);
      ArrayVariant subArr = array[4];
      ASSERT_EQ(arrVal.getRefCount(), 3);
      ASSERT_EQ(subArr.getRefCount(), 3);
      ASSERT_EQ((subArr[0]).toNumericVariant().toLong(), 312);
      ASSERT_STREQ((subArr["name"]).toStringVariant().getCStr(), "polarphp");
      ASSERT_EQ((subArr["age"]).toNumericVariant().toLong(), 11);
   }
}

TEST(ArrayVariantTest, testCopyFromStdMap)
{
   {
      ArrayVariant arrVal;
      arrVal.insert(0, 312);
      arrVal.insert("name", "polarphp");
      arrVal.insert("age", 11);
      ASSERT_EQ(arrVal.getRefCount(), 1);
      ArrayVariant array(ArrayVariant::InitMapType(
      {
                               {0, 1.2},
                               {"name", "polarphp"},
                               {3, 123},
                               {"data", arrVal}
                            }));
      ASSERT_EQ(array.getSize(), 4);
      ASSERT_EQ((array[0]).toDoubleVariant().toDouble(), 1.2);
      ASSERT_STREQ((array["name"]).toStringVariant().getCStr(), "polarphp");
      ASSERT_EQ((array[3]).toNumericVariant().toLong(), 123);
      ArrayVariant subArr = array["data"];
      ASSERT_EQ(arrVal.getRefCount(), 3);
      ASSERT_EQ(subArr.getRefCount(), 3);
      ASSERT_EQ((subArr[0]).toNumericVariant().toLong(), 312);
      ASSERT_STREQ((subArr["name"]).toStringVariant().getCStr(), "polarphp");
      ASSERT_EQ((subArr["age"]).toNumericVariant().toLong(), 11);
   }
}

TEST(ArrayVariantTest, testAssignOperators)
{
   ArrayVariant array1;
   ArrayVariant array2;
   array1.insert("name", "polarphp1");
   array2.insert(1, "xiuxiu");
   array2.insert(2, "beijing");
   ASSERT_EQ(array1.getRefCount(), 1);
   ASSERT_EQ(array2.getRefCount(), 1);
   ASSERT_EQ(array1.getSize(), 1);
   ASSERT_EQ(array2.getSize(), 2);
   ASSERT_TRUE(array1.contains("name"));
   ASSERT_TRUE(array2.contains(1));
   ASSERT_TRUE(array2.contains(2));
   array1 = array2;
   ASSERT_EQ(array1.getRefCount(), 2);
   ASSERT_EQ(array2.getRefCount(), 2);
   ASSERT_FALSE(array1.contains("name"));
   ASSERT_TRUE(array1.contains(1));
   ASSERT_TRUE(array1.contains(2));

   // move assgin
   array1 = std::move(array2);
   ASSERT_TRUE(array1.contains(1));
   ASSERT_TRUE(array1.contains(2));
   ASSERT_EQ(array1.getRefCount(), 2);

}

TEST(ArrayVariantTest, testMoveAssignOperators)
{
   // test assign from ArrayVariant
   ArrayVariant array1;
   Variant val1(123);
   ASSERT_EQ(array1.getRefCount(), 1);
   ASSERT_EQ(val1.getRefCount(), 0);
   ASSERT_EQ(array1.getSize(), 0);
   array1 = val1;
   ASSERT_EQ(array1.getSize(), 1);
   ASSERT_EQ((array1[0]).toNumericVariant().toLong(), 123);
   ASSERT_EQ(array1.getRefCount(), 1);
   ASSERT_EQ(val1.getRefCount(), 0);

   Variant val2(true);
   ASSERT_EQ(val2.getRefCount(), 0);
   array1 = val2;
   ASSERT_EQ(array1.getSize(), 1);
   ASSERT_EQ((array1[0]).toBooleanVariant().toBoolean(), true);
   ASSERT_EQ(array1.getRefCount(), 1);
   ASSERT_EQ(val2.getRefCount(), 0);

   Variant val3("polarphp");
   ASSERT_EQ(val3.getRefCount(), 1);
   array1 = val3;
   ASSERT_EQ(val3.getRefCount(), 2);
   ASSERT_EQ(array1.getSize(), 1);
   ASSERT_STREQ((array1[0]).toStringVariant().getCStr(), "polarphp");
   ASSERT_EQ(array1.getRefCount(), 1);
   ASSERT_EQ(val3.getRefCount(), 2);

   ArrayVariant array2;
   array2[1] = "polarphp";
   array2[2] = true;
   array2[3] = 3.14;
   Variant val4(array2);
   ASSERT_EQ(val4.getRefCount(), 2);
   array1 = val4;
   ASSERT_EQ(array1.getRefCount(), 3);
   ASSERT_EQ(val4.getRefCount(), 3);
   ASSERT_STREQ((array1[1]).toStringVariant().getCStr(), "polarphp");
   ASSERT_EQ((array1[2]).toBooleanVariant().toBoolean(), true);
   ASSERT_EQ((array1[3]).toDoubleVariant().toDouble(), 3.14);
   // test move assign from ArrayVariant

   array1 = std::move(val1);
   // can't do anything about val1
   ASSERT_EQ(array1.getSize(), 1);
   ASSERT_EQ((array1[0]).toNumericVariant().toLong(), 123);
   ASSERT_EQ(array1.getRefCount(), 1);

   array1 = std::move(val2);
   // can't do anything about val2
   ASSERT_EQ(array1.getSize(), 1);
   ASSERT_EQ((array1[0]).toBooleanVariant().toBoolean(), true);
   ASSERT_EQ(array1.getRefCount(), 1);

   array1 = std::move(val3);
   // can't do anything about val3
   ASSERT_EQ(array1.getSize(), 1);
   ASSERT_STREQ((array1[0]).toStringVariant().getCStr(), "polarphp");
   ASSERT_EQ(array1.getRefCount(), 1);

   array1 = std::move(val4);
   // can't do anything about val4
   ASSERT_EQ(array1.getRefCount(), 3);
   ASSERT_STREQ((array1[1]).toStringVariant().getCStr(), "polarphp");
   ASSERT_EQ((array1[2]).toBooleanVariant().toBoolean(), true);
   ASSERT_EQ((array1[3]).toDoubleVariant().toDouble(), 3.14);
}

TEST(ArrayVariantTest, testEqualAndNotEqual)
{
   {
      ArrayVariant arr1;
      arr1.append(1);
      arr1.append(2);
      ArrayVariant arr2;
      arr2.append(2);
      arr2.append(1);
      ArrayVariant arr3;
      arr3.append(1);
      arr3.append(2);
      ArrayVariant arr4;
      arr4.append(1);
      arr4.append(2);
      arr4.append(3);

      ASSERT_TRUE(arr1.strictEqual(arr1));
      ASSERT_TRUE(arr2.strictEqual(arr2));
      ASSERT_TRUE(arr2.strictEqual(arr2));

      ASSERT_FALSE(arr1 == arr2);
      ASSERT_TRUE(arr1 == arr3);
      ASSERT_TRUE(arr1 == arr1);
      ASSERT_TRUE(arr2 == arr2);
      ASSERT_TRUE(arr3 == arr3);
      ASSERT_FALSE(arr1 == arr4);

      ASSERT_TRUE(arr1 != arr2);
      ASSERT_FALSE(arr1 != arr3);
      ASSERT_FALSE(arr1 != arr1);
      ASSERT_FALSE(arr2 != arr2);
      ASSERT_FALSE(arr3 != arr3);
      ASSERT_TRUE(arr1 != arr4);
   }
   {
      ArrayVariant arr1;
      arr1["name"] = "polarphp";
      arr1[0] = 123;
      arr1["address"] = "beijing";
      ArrayVariant arr2;
      arr2["address"] = "beijing";
      arr2[0] = 123;
      arr2["name"] = "polarphp";
      ArrayVariant arr3;
      arr3["name"] = "polarphp";
      arr3[0] = 123;
      arr3["address"] = "beijing";
      ArrayVariant arr4;
      arr4["name"] = "polarphp";
      arr4["address"] = "beijing";
      arr4["info"] = 3.14;
      ArrayVariant arr5;
      arr5["name"] = "polarphp";
      arr5["address"] = "beijing";
      arr5[0] = 123;

      ASSERT_TRUE(arr1.strictEqual(arr1));
      ASSERT_TRUE(arr2.strictEqual(arr2));
      ASSERT_TRUE(arr3.strictEqual(arr3));
      ASSERT_TRUE(arr4.strictEqual(arr4));

      ASSERT_TRUE(arr1 == arr1);
      ASSERT_TRUE(arr2 == arr2);
      ASSERT_TRUE(arr3 == arr3);
      ASSERT_TRUE(arr4 == arr4);
      ASSERT_TRUE(arr1 == arr2);
      ASSERT_FALSE(arr1.strictEqual(arr2));
      ASSERT_TRUE(arr1.strictEqual(arr3));
      ASSERT_TRUE(arr1 == arr3);
      ASSERT_FALSE(arr2 == arr4);

      ASSERT_FALSE(arr1 != arr1);
      ASSERT_FALSE(arr2 != arr2);
      ASSERT_FALSE(arr3 != arr3);
      ASSERT_FALSE(arr4 != arr4);
      ASSERT_FALSE(arr1 != arr2);
      ASSERT_FALSE(arr1 != arr3);
      ASSERT_TRUE(arr2 != arr4);

      ASSERT_TRUE(arr1 == arr5);
      ASSERT_FALSE(arr1 != arr5);
      ASSERT_FALSE(arr1.strictEqual(arr5));
      ASSERT_TRUE(arr1.strictNotEqual(arr5));
   }
}

TEST(ArrayVariantTest, testContains)
{
   ArrayVariant array;
   array.insert("name", "polarphp");
   array.insert("address", "beijing");
   ASSERT_FALSE(array.contains("age"));
   ASSERT_TRUE(array.contains("name"));
   ASSERT_TRUE(array.contains("address"));
   array["age"] = 123;
   ASSERT_TRUE(array.contains("age"));
}

TEST(ArrayVariantTest, testAppend)
{
   ArrayVariant array;
   ASSERT_TRUE(array.isEmpty());
   array.append(1);
   ASSERT_FALSE(array.isEmpty());
   ASSERT_EQ(array.getSize(), 1);
   array.append("polarphp");
   ASSERT_EQ(array.getSize(), 2);
   NumericVariant num = array.getValue(0);
   StringVariant str = array.getValue(1);
   ASSERT_EQ(num.toLong(), 1);
   ASSERT_STREQ(str.getCStr(), "polarphp");
   ASSERT_EQ(str.getRefCount(), 2);
   // std::cout << str << std::endl;
   {
      // test for reference
      ArrayVariant arr1{1, 2};
      ArrayVariant arr2(arr1);
      ArrayVariant arr3 = arr2;
      ASSERT_EQ(arr1.getSize(), 2);
      ASSERT_EQ(arr2.getSize(), 2);
      ASSERT_EQ(arr3.getSize(), 2);
      ASSERT_EQ(arr1.getRefCount(), 3);
      ASSERT_EQ(arr2.getRefCount(), 3);
      ASSERT_EQ(arr3.getRefCount(), 3);
      arr1.append(3);
      ASSERT_EQ(arr1.getSize(), 3);
      ASSERT_EQ(arr2.getSize(), 2);
      ASSERT_EQ(arr3.getSize(), 2);
      ASSERT_EQ(arr1.getRefCount(), 1);
      ASSERT_EQ(arr2.getRefCount(), 2);
      ASSERT_EQ(arr3.getRefCount(), 2);
   }
}

TEST(ArrayVariantTest, testClear)
{
   ArrayVariant array;
   array.insert("name", "polarphp");
   array.insert("address", "beijing");
   ASSERT_EQ(array.getSize(), 2);
   ASSERT_EQ(array.getCapacity(), 8);
   array.clear();
   ASSERT_EQ(array.getSize(), 0);
   ASSERT_EQ(array.getCapacity(), 8);
   array.insert("age", 123);
   ASSERT_EQ(array.getSize(), 1);
   ASSERT_EQ(array.getCapacity(), 8);
}

TEST(ArrayVariantTest,testRemove)
{
   ArrayVariant array;
   ASSERT_FALSE(array.remove(1));
   ASSERT_FALSE(array.remove("notExistItem"));
   array.append("polarphp");
   array.insert("name", "zzu_softboy");
   ASSERT_TRUE(array.contains(0));
   ASSERT_TRUE(array.contains("name"));
   ASSERT_EQ(array.getSize(), 2);
   ASSERT_TRUE(array.remove(0));
   ASSERT_TRUE(array.remove("name"));
   ASSERT_EQ(array.getSize(), 0);
}

TEST(ArrayVariantTest, testErase)
{
   ArrayVariant array;
   array.insert("name", "polarphp");
   ArrayVariant::Iterator iter = array.insert("address", "beijing");
   array.append(1);
   array.append(2);
   array.append(3);
   ASSERT_EQ(array.getSize(), 5);
   ASSERT_STREQ(StringVariant(iter.getValue()).getCStr(), "beijing");
   iter = array.erase(iter);
   ASSERT_EQ(array.getSize(), 4);
   ASSERT_EQ(NumericVariant(iter.getValue()).toLong(), 1);
   iter = array.end();
   iter = array.erase(iter);
   ASSERT_TRUE(iter == iter);
   ArrayVariant::ConstIterator citer = array.cbegin();
   ASSERT_STREQ(StringVariant(citer.getValue()).getCStr(), "polarphp");
   citer += 2;
   ASSERT_EQ(NumericVariant(citer.getValue()).toLong(), 2);
   iter = array.erase(citer);
   ASSERT_EQ(NumericVariant(citer.getValue()).toLong(), 3);
}

TEST(ArrayVariantTest, testTake)
{
   ArrayVariant array;
   array.insert("name", "polarphp");
   array.insert("address", "beijing");
   array.append(1);
   array.append(2);
   array.append(3);
   ASSERT_EQ(array.getSize(), 5);
   StringVariant name = array.take("name");
   ASSERT_EQ(array.getSize(), 4);
   ASSERT_STREQ(name.getCStr(), "polarphp");
   ASSERT_EQ(name.getRefCount(), 1);
}

TEST(ArrayVariantTest, testUnset)
{
   ArrayVariant array;
   polar::vmapi::array_unset(array[1]); // quiet unset
   array[1] = "polarphp";
   array[2] = 123;
   array[3]["name"] = "polarphp";
   array[3][1] = 123;
   array[3]["data"] = 123;
   // polar::vmapi::array_unset(array[1]["name"]); // Fatal error - Can't use string offset as an array
   // polar::vmapi::array_unset(array[1][1]); // Can't use string offset as an array
   ASSERT_FALSE(polar::vmapi::array_unset(array[2][1]));
//   // polar::vmapi::array_unset(array[3]["name"][1]); // Fatal error: Can't use string offset as an array
//   // polar::vmapi::array_unset(array[3]["name"]["key"]); // Fatal error - Can't use string offset as an array
//   ASSERT_FALSE(polar::vmapi::array_unset(array[3][1]["age"]));
//   ASSERT_FALSE(polar::vmapi::array_unset(array[3]["data"][22]));
//   ASSERT_FALSE(polar::vmapi::array_unset(array[3]["data"]["xiuxiu"]));
//   // polar::vmapi::array_unset(array[3]["name"][1]); // Can't use string offset as an array
//   // polar::vmapi::array_unset(array[3]["name"]["age"]); // Can't use string offset as an array
//   ASSERT_TRUE(polar::vmapi::array_isset(array[3]["data"]));
//   ASSERT_TRUE(polar::vmapi::array_unset(array[3]["data"]));
//   ASSERT_FALSE(polar::vmapi::array_isset(array[3]["data"]));
//   ASSERT_FALSE(polar::vmapi::array_unset(array[3]["data"]));
}
