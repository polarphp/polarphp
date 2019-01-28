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
using polar::vmapi::VariantKeyLess;

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
      ArrayVariant array(std::map<Variant, Variant, VariantKeyLess>{
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
   // polar::vmapi::array_unset(array[3]["name"][1]); // Fatal error: Can't use string offset as an array
   // polar::vmapi::array_unset(array[3]["name"]["key"]); // Fatal error - Can't use string offset as an array
   ASSERT_FALSE(polar::vmapi::array_unset(array[3][1]["age"]));
   ASSERT_FALSE(polar::vmapi::array_unset(array[3]["data"][22]));
   ASSERT_FALSE(polar::vmapi::array_unset(array[3]["data"]["xiuxiu"]));
   // polar::vmapi::array_unset(array[3]["name"][1]); // Can't use string offset as an array
   // polar::vmapi::array_unset(array[3]["name"]["age"]); // Can't use string offset as an array
   ASSERT_TRUE(polar::vmapi::array_isset(array[3]["data"]));
   ASSERT_TRUE(polar::vmapi::array_unset(array[3]["data"]));
   ASSERT_FALSE(polar::vmapi::array_isset(array[3]["data"]));
   ASSERT_FALSE(polar::vmapi::array_unset(array[3]["data"]));
}

TEST(ArrayVariantTest, testIsset)
{
   ArrayVariant array;
   array.insert("name", "polarphp");
   array.insert("age", 123);
   array[1][2][3][4][5][6] = "polarphp";
   array[1][2][3][4][5]["info"] = "cloud";
   //std::cout << str << std::endl;
   ASSERT_FALSE(polar::vmapi::array_isset(array[9][2][3][4][5][7]));
   ASSERT_TRUE(polar::vmapi::array_isset(array[1][2][3][4][5]["info"]));
}

TEST(ArrayVariantTest, testCastOperators)
{
   ArrayVariant array;
   array.insert("name", "polarphp");
   array.insert("age", 123);
   array[1][2][3][4][5][6] = "polarphp";
   array[1][2][3][4][5]["info"] = "cloud";
   array[1][2][3]["name"][5]["info"] = "zzu_softboy";
   array[1][2][3][4][5][5] = true;
   array[1][2][3][4][5][4] = 3.14;
   array[1][2][3][4][5]["num"] = 123;

   ASSERT_THROW(StringVariant str = array[2], std::bad_cast);
   ASSERT_THROW(NumericVariant num = array[2], std::bad_cast);
   ASSERT_THROW(DoubleVariant dnum = array[2], std::bad_cast);
   ASSERT_THROW(BooleanVariant bval = array[2], std::bad_cast);
   ASSERT_THROW(Variant var = array[2], std::bad_cast);
   ASSERT_THROW(StringVariant str = array[2][3], std::bad_cast);
   ASSERT_THROW(NumericVariant num = array[2][3], std::bad_cast);
   ASSERT_THROW(DoubleVariant dnum = array[2][3], std::bad_cast);
   ASSERT_THROW(BooleanVariant bval = array[2][3], std::bad_cast);
   ASSERT_THROW(Variant var = array[2][3], std::bad_cast);

   ASSERT_THROW(StringVariant str = array["key"], std::bad_cast);
   ASSERT_THROW(NumericVariant num = array["key"], std::bad_cast);
   ASSERT_THROW(DoubleVariant dnum = array["key"], std::bad_cast);
   ASSERT_THROW(BooleanVariant bval = array["key"], std::bad_cast);
   ASSERT_THROW(Variant var = array["key"], std::bad_cast);
   ASSERT_THROW(StringVariant str = array["key"][3]["key2"], std::bad_cast);
   ASSERT_THROW(NumericVariant num = array["key"][3]["key2"], std::bad_cast);
   ASSERT_THROW(DoubleVariant dnum = array["key"][3]["key2"], std::bad_cast);
   ASSERT_THROW(BooleanVariant bval = array["key"][3]["key2"], std::bad_cast);
   ASSERT_THROW(Variant var = array["key"][3]["key2"], std::bad_cast);

   StringVariant str = array[1][2][3][4][5][6];
   ASSERT_STREQ(str.getCStr(), "polarphp");
   str = array[1][2][3][4][5]["info"];
   ASSERT_STREQ(str.getCStr(), "cloud");
   NumericVariant num = array[1][2][3][4][5]["num"];
   ASSERT_EQ(num.toLong(), 123);
   num = array[1][2][3][4][5]["num"];
   ASSERT_EQ(num.toLong(), 123);

   BooleanVariant bval = array[1][2][3][4][5][5];
   ASSERT_TRUE(bval.toBoolean());
   bval = array[1][2][3][4][5][5];
   ASSERT_TRUE(bval.toBoolean());

   DoubleVariant dval = array[1][2][3][4][5][4];
   ASSERT_EQ(dval.toDouble(), 3.14);
   dval = array[1][2][3][4][5][4];

   Variant var = array[1][2][3]["name"][5]["info"];
   var = array[1][2][3]["name"][5]["info"];
}

TEST(ArrayVariantTest, testGetNextInertIndex)
{
   ArrayVariant array;
   ASSERT_EQ(array.getNextInsertIndex(), 0);
   array.append(123);
   ASSERT_EQ(array.getNextInsertIndex(), 1);
   array.append("beijing");
   ASSERT_EQ(array.getNextInsertIndex(), 2);
   array.insert(11, "360");
   ASSERT_EQ(array.getNextInsertIndex(), 12);
   array.append("beijing");
   ASSERT_EQ(array.getNextInsertIndex(), 13);
}

TEST(ArrayVariantTest, testGetKeys)
{
   ArrayVariant array;

   std::list<KeyType> expectKeys = {
      KeyType(-1, std::shared_ptr<std::string>(new std::string("name"))),
      KeyType(-1, std::shared_ptr<std::string>(new std::string("age"))),
      KeyType(0, nullptr),
      KeyType(1, nullptr),
      KeyType(2, nullptr),
      KeyType(-1, std::shared_ptr<std::string>(new std::string("info"))),
      KeyType(-1, std::shared_ptr<std::string>(new std::string("data"))),
      KeyType(-1, std::shared_ptr<std::string>(new std::string("xxx"))),
      KeyType(-1, std::shared_ptr<std::string>(new std::string("key3"))),
   };
   std::list<ArrayVariant::KeyType> keys = array.getKeys();
   ASSERT_EQ(keys.size(), 0);
   array.insert("name", "polarphp");
   array.insert("age", 123);
   array.append("beijing");
   array.append("aaa");
   array.append("bbb");
   array.insert("info", "ccc");
   array.insert("data", 3.14);
   array.insert("xxx", 3.14);
   array.insert("key3", "ccc");
   keys = array.getKeys();
   ASSERT_EQ(keys.size(), expectKeys.size());
   auto iter = keys.begin();
   auto expectIter = expectKeys.begin();
   while (iter != keys.end()) {
      ASSERT_EQ(iter->first, expectIter->first);
      if (iter->second) {
         ASSERT_EQ(*iter->second, *expectIter->second);
      }
      ++iter;
      ++expectIter;
   }
   keys = array.getKeys("notExistValue");
   ASSERT_TRUE(keys.empty());
   keys = array.getKeys(3.14);
   expectKeys = {
      KeyType(-1, std::shared_ptr<std::string>(new std::string("data"))),
      KeyType(-1, std::shared_ptr<std::string>(new std::string("xxx"))),
   };
   ASSERT_EQ(keys.size(), expectKeys.size());
   iter = keys.begin();
   expectIter = expectKeys.begin();
   while (iter != keys.end()) {
      ASSERT_EQ(iter->first, expectIter->first);
      if (iter->second) {
         ASSERT_EQ(*iter->second, *expectIter->second);
      }
      ++iter;
      ++expectIter;
   }
}

TEST(ArrayVariantTest, testGetValues)
{
   ArrayVariant array;
   std::list<Variant> values = array.getValues();
   std::list<Variant> expectValues = {
      Variant("polarphp"),
      Variant(123)
   };
   ASSERT_EQ(values.size(), 0);
   array.insert("name", "polarphp");
   array.insert("age", 123);
   values = array.getValues();
   ASSERT_EQ(values.size(), 2);
   ASSERT_EQ(values, expectValues);
}

TEST(ArrayVariantTest, testFind)
{
   ArrayVariant array;
   array.insert("name", "polarphp");
   array.insert("age", 123);
   array.append("beijing");
   array.append("aaa");
   array.append("bbb");
   array.insert("info", "ccc");
   array.insert("data", 3.14);
   array.insert("xxx", 3.14);
   array.insert("key3", "ccc");
   ArrayVariant::Iterator iter = array.find("notExist");
   ASSERT_TRUE(iter == array.end());
   iter = array.find(122);
   ASSERT_TRUE(iter == array.end());
   iter = array.find("name");
   ASSERT_STREQ(StringVariant(iter.getValue()).getCStr(), "polarphp");
   ++iter;
   ASSERT_EQ(NumericVariant(iter.getValue()).toLong(), 123);

   const ArrayVariant &carray = array;
   ArrayVariant::ConstIterator citer = carray.find("notExist");
   ASSERT_TRUE(citer == carray.cend());
   citer = carray.find(122);
   ASSERT_TRUE(citer == carray.cend());
   citer = carray.find("name");
   ASSERT_STREQ(StringVariant(citer.getValue()).getCStr(), "polarphp");
   ++citer;
   ASSERT_EQ(NumericVariant(citer.getValue()).toLong(), 123);
}

TEST(ArrayVariantTest, testMap)
{
   ArrayVariant array;
   array.insert("name", "polarphp");
   array.insert("age", 123);
   array.append("beijing");
   array.append("aaa");
   array.append("bbb");
   array.insert("info", "ccc");
   array.insert("data", 3.14);
   array.insert("xxx", 3.14);
   array.insert("key3", "ccc");
   std::list<std::string> strKeys;
   std::list<std::string> expectedStrKeys {
      "name", "age", "info", "data", "xxx", "key3"
   };
   std::list<vmapi_ulong> indexes;
   std::list<vmapi_ulong> expectedIndexes {
      0, 1, 2
   };
   array.map([&strKeys, &indexes](const ArrayVariant::KeyType &key, const Variant &value) -> bool {
      if (key.second) {
         strKeys.push_back(*key.second.get());
      } else {
         indexes.push_back(key.first);
      }
      return true;
   });
   ASSERT_EQ(strKeys, expectedStrKeys);
   ASSERT_EQ(indexes, expectedIndexes);

   strKeys.clear();
   expectedStrKeys = {"name", "age"};
   array.map([&strKeys](const ArrayVariant::KeyType &key, const Variant &value) -> bool {
      if (key.second) {
         std::string &str = *key.second.get();
         if (str != "info") {
            strKeys.push_back(*key.second.get());
            return true;
         } else {
            return false;
         }
      }
      return true;
   });
   ASSERT_EQ(strKeys, expectedStrKeys);
}

TEST(ArrayVariantTest, testInsert)
{
   ArrayVariant array;
   ASSERT_TRUE(array.isEmpty());
   array.insert(1, "polarphp");
   array.insert(5, true);
   BooleanVariant boolVar = array.getValue(5);
   StringVariant strVar = array.getValue(1);
   ASSERT_EQ(array.getSize(), 2);
   ASSERT_EQ(boolVar.toBoolean(), true);
   ASSERT_STREQ(strVar.getCStr(), "polarphp");
   ASSERT_EQ(strVar.getRefCount(), 2);
   array.insert(1, "zzu_softboy");
   ASSERT_EQ(strVar.getRefCount(), 1);
   //   array.getValue(111); echo notice msg
   //   array.getValue("name"); echo notice msg
   ArrayVariant::Iterator iter =  array.insert("name", "zzu_softboy");
   array.insert("age", 123);
   StringVariant name = array.getValue("name");
   ASSERT_EQ(array.getSize(), 4);
   ASSERT_STREQ(name.getCStr(), "zzu_softboy");
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "zzu_softboy");
   ASSERT_STREQ(StringVariant(iter.getValue()).getCStr(), "zzu_softboy");
   array.insert(0, "xiuxiu");
   iter = array.begin();
   // array keep insert order
   ASSERT_STREQ(StringVariant(iter.getZvalPtr()).getCStr(), "zzu_softboy");

   ArrayVariant arr2;
   arr2.insert("info", "beijing");
   array.insert("data", arr2);

   Variant ditem = array["data"];
   ASSERT_EQ(ditem.getType(), polar::vmapi::Type::Array);
   ArrayVariant arr3(std::move(ditem)); // can not use ditem anymore
   ASSERT_EQ(arr2.getRefCount(), 3);
   ASSERT_EQ(arr3.getRefCount(), 3);
   StringVariant info = arr3["info"];
   ASSERT_STREQ(info.getCStr(), "beijing");
}

TEST(ArrayVariantTest, testIterators)
{
   ArrayVariant array;
   array.append(1);
   array.append("polarphp");
   array.append("zzu_softboy");
   array.append("aaa");
   array.append("bbb");
   array.append("ccc");
   ArrayVariant::Iterator iter = array.begin();
   ArrayVariant::ConstIterator citer = array.cbegin();
   zval &item1 = iter.getZval();
   ASSERT_EQ(Z_LVAL(item1), 1);
   const zval &citem1 = citer.getZval();
   ASSERT_EQ(Z_LVAL(item1), 1);
   // Z_LVAL(citem1) = 123; compile error
   Z_LVAL(item1) = 123;
   ASSERT_EQ(Z_LVAL(citem1), 123);
   // iterator ++ operators
   iter++;
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "polarphp");
   iter += 2;
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "aaa");
   iter += -2;
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "polarphp");
   iter += 2;
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "aaa");
   iter -= 2;
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "polarphp");
   iter++;
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "zzu_softboy");
   iter--;
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "polarphp");
   iter--;

   ++iter;
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "polarphp");
   iter += 2;
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "aaa");
   iter += -2;
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "polarphp");
   iter += 2;
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "aaa");
   iter -= 2;
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "polarphp");
   ++iter;
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "zzu_softboy");
   --iter;
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "polarphp");

   ArrayVariant::Iterator preIter = iter++;
   ASSERT_STREQ(Z_STRVAL_P(preIter.getZvalPtr()), "polarphp");
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "zzu_softboy");

   preIter = ++iter;
   ASSERT_STREQ(Z_STRVAL_P(preIter.getZvalPtr()), "aaa");
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "aaa");

   preIter = iter--;
   ASSERT_STREQ(Z_STRVAL_P(preIter.getZvalPtr()), "aaa");
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "zzu_softboy");

   preIter = --iter;
   ASSERT_STREQ(Z_STRVAL_P(preIter.getZvalPtr()), "polarphp");
   ASSERT_STREQ(Z_STRVAL_P(iter.getZvalPtr()), "polarphp");

   // const
   citer++;
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "polarphp");
   citer += 2;
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "aaa");
   citer += -2;
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "polarphp");
   citer += 2;
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "aaa");
   citer -= 2;
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "polarphp");
   citer++;
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "zzu_softboy");
   citer--;
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "polarphp");
   citer--;

   ++citer;
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "polarphp");
   citer += 2;
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "aaa");
   citer += -2;
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "polarphp");
   citer += 2;
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "aaa");
   citer -= 2;
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "polarphp");
   ++citer;
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "zzu_softboy");
   --citer;
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "polarphp");

   ArrayVariant::ConstIterator preciter = citer++;
   ASSERT_STREQ(Z_STRVAL_P(preciter.getZvalPtr()), "polarphp");
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "zzu_softboy");

   preciter = ++citer;
   ASSERT_STREQ(Z_STRVAL_P(preciter.getZvalPtr()), "aaa");
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "aaa");

   preciter = citer--;
   ASSERT_STREQ(Z_STRVAL_P(preciter.getZvalPtr()), "aaa");
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "zzu_softboy");

   preciter = --citer;
   ASSERT_STREQ(Z_STRVAL_P(preciter.getZvalPtr()), "polarphp");
   ASSERT_STREQ(Z_STRVAL_P(citer.getZvalPtr()), "polarphp");
}

TEST(ArrayVariantTest, testAccessOperator)
{
   ArrayVariant array;
   array.append(1);
   array.append("polarphp");
   array.append("zzu_softboy");
   array.append("aaa");
   array.insert(5, "21212");
   array.append("bbb");
   array.append("ccc");
   StringVariant str = array[1];
   ASSERT_STREQ(str.getCStr(), "polarphp");
   array[0] = 123;
   NumericVariant num = array.getValue(0);
   ASSERT_EQ(num.toLong(), 123);
   array[10] = "polar foundation";
   StringVariant team = array[10];
   ASSERT_STREQ(team.getCStr(), "polar foundation");
   array["city"] = "beijing";
   StringVariant city = array["city"];
   ASSERT_STREQ(city.getCStr(), "beijing");
   array[11][1][3] = "zzu_softboy";
   StringVariant str1 = array[11][1][3];
   // ::cout << str1 << std::endl;
   ASSERT_STREQ(str1.getCStr(), "zzu_softboy");
   array[4][5][6][7][8][9][10][11][12]["name"] = "polar foundation";
   array[4][5][6][7][8][9][10][11][12]["address"] = "polarphp Building";
   array[4][5][6][7][8][9][10][11][12]["offical_site"] = "http://polarphp.org";
   array[4][5][6][7][8][9][10][11][12][111] = 213;
   array[4][5][6][7][8][9][10][11][12][112] = 3.1415926;
   NumericVariant num1 = array[4][5][6][7][8][9][10][11][12][111];
   DoubleVariant doubleNum = array[4][5][6][7][8][9][10][11][12][112];
   StringVariant polarphpTeamName = array[4][5][6][7][8][9][10][11][12]["name"];
   StringVariant polarphpTeamAddress = array[4][5][6][7][8][9][10][11][12]["address"];
   StringVariant polarphpTeamOfficalSite = array[4][5][6][7][8][9][10][11][12]["offical_site"];
   ASSERT_STREQ(polarphpTeamName.getCStr(), "polar foundation");
   ASSERT_STREQ(polarphpTeamAddress.getCStr(), "polarphp Building");
   ASSERT_STREQ(polarphpTeamOfficalSite.getCStr(), "http://polarphp.org");
   ASSERT_EQ(num1.toLong(), 213);
   ASSERT_EQ(doubleNum.toDouble(), 3.1415926);
   //   std::cout << polarphpTeamName << std::endl;
   //   std::cout << polarphpTeamAddress << std::endl;
   //   std::cout << polarphpTeamOfficalSite << std::endl;
   //   std::cout << num1 << std::endl;
   //   std::cout << doubleNum << std::endl;
   //array[5][6] = "fata error"; //Fatal error - Can't use string offset as an array
   //array[0][1] = 123;

   // NumericVariant num11 = array[0][1][0]; // Can't use a scalar value as an array
   // NumericVariant num12 = array[10][12]; // Can't use string offset as an array
   // NumericVariant num12 = array[10]["key"]; // Can't use string offset as an array
   // StringVariant name = array["city"][1][2]; // Can't use string offset as an array
   // StringVariant name = array["city"]["name"]["name"]; //  Can't use string offset as an array
   array["info"] = 3.14;
   // StringVariant name = array["info"]["name"]["name"]; // Can't use a scalar value as an array
}
