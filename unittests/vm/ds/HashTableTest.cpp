// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 polarboy <polarboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/28.

#include "polarphp/vm/ds/HashTable.h"
#include "polarphp/vm/ds/Variant.h"
#include "polarphp/vm/ZendApi.h"

#include "gtest/gtest.h"
#include <limits>
#include <iostream>
#include <vector>
#include <string>

using ZVMHashTable = polar::vmapi::HashTable;
using polar::vmapi::Variant;

TEST(HashTableTest, testConstructors)
{
   // default constructor
   ZVMHashTable table;
   ASSERT_EQ(table.getSize(), 0);
}

TEST(HashTableTest, testInsertItem)
{
   // default constructor
   ZVMHashTable table;
   ASSERT_EQ(table.getSize(), 0);
   table.insert("name", Variant("polarphp"));
   ASSERT_EQ(table.getSize(), 1);
   table.insert("age", Variant(20));
   ASSERT_EQ(table.getSize(), 2);
}

TEST(HashTableTest, testIterator)
{
   // default constructor
   ZVMHashTable table;
   ASSERT_EQ(table.getSize(), 0);
   table.insert("name", Variant("polarphp"));
   ASSERT_EQ(table.getSize(), 1);
   table.insert("age", Variant(20));
   ASSERT_EQ(table.getSize(), 2);
   table.insert("height", Variant(123));
   ASSERT_EQ(table.getSize(), 3);
   ZVMHashTable::iterator iter = table.begin();
   std::vector<std::string> expectedKeys{"name", "age", "height"};
   std::vector<std::string> actualKeys;
   std::vector<std::string> expectedValueStrs{"polarphp"};
   std::vector<int64_t> expectedValueInts{20, 123};
   std::vector<std::string> actualValueStrs;
   std::vector<int64_t> actualValueInts;
   while (iter != table.end()) {
      ZVMHashTable::HashKeyType keyType = iter.getKeyType();
      if (keyType == ZVMHashTable::HashKeyType::String) {
         actualKeys.push_back(iter.getStrKey());
      }
      Variant value = *iter;
      if (value.getType() == polar::vmapi::Type::String) {
         actualValueStrs.push_back(value.toString());
      } else if (value.getType() == polar::vmapi::Type::Long) {
         actualValueInts.push_back(Z_LVAL(value.getZval()));
      }
      ++iter;
   }
   ASSERT_EQ(expectedKeys, actualKeys);
   ASSERT_EQ(expectedValueStrs, actualValueStrs);
   ASSERT_EQ(expectedValueInts, actualValueInts);
}

TEST(HashTableTest, testGetValue)
{
   {
      ZVMHashTable table;
      table.insert("name", Variant("polarphp"));
      table.insert("city", Variant("beijing"));
      table.insert("height", Variant(123));
      ASSERT_EQ(table.getSize(), 3);
      ASSERT_EQ(table.getValue("name").toString(), "polarphp");
      ASSERT_EQ(table.getValue("city").toString(), "beijing");
      ASSERT_EQ(Z_LVAL(table.getValue("height").getZval()), 123);
      ASSERT_EQ(table["name"].toString(), "polarphp");
      ASSERT_EQ(table["city"].toString(), "beijing");
      ASSERT_EQ(Z_LVAL(table["height"].getZval()), 123);
      // test default value
      ASSERT_EQ(Z_LVAL(table.getValue("notExistKey", 123).getZval()), 123);
      ASSERT_EQ(table.getValue("notExistKey", "polarphp").toString(), "polarphp");
   }
}

TEST(HashTableTest, testGetKey)
{
   ZVMHashTable table;
   // this trigger a warning
   table.getKey();
   table.insert("name", Variant("polarphp"));
   ASSERT_EQ(table.getKey().toString(), "name");
   table.insert("key1", Variant("item1"));
   table.insert("key2", Variant("item2"));
   table.insert("key3", Variant("item3"));
   table.insert("anotherKey1", Variant("item1"));
   table.insert(12, Variant(122));
   // find the first match key
   ASSERT_EQ(table.getKey(Variant("item1")).toString(), "key1");
   ASSERT_EQ(table.getKey(Variant("item2")).toString(), "key2");
   ASSERT_EQ(Z_LVAL(table.getKey(Variant(122)).getZval()), 12);
   // test default key
   ASSERT_EQ(table.getKey(Variant("notExist"), "defaultKey").toString(), "defaultKey");
   ASSERT_EQ(Z_LVAL(table.getKey(Variant(1234), 11).getZval()), 11);
}

TEST(HashTableTest, testAssignValue)
{
   ZVMHashTable table;
   table.insert("num", Variant(123));
   table["num"] = Variant(213);
   ASSERT_EQ(Z_LVAL(table["num"].getZval()), 213);
   table["num"] = Variant("polarphp");
   ASSERT_EQ(table["num"].toString(), "polarphp");
   table["name"] = Variant("polarboy");
   ASSERT_EQ(table["name"].toString(), "polarboy");
   table["age"] = Variant(123);
   ASSERT_EQ(Z_LVAL(table["age"].getZval()), 123);
   table.append(Variant(1234));
   ASSERT_EQ(Z_LVAL(table[0].getZval()), 1234);
   table.append(Variant(4321));
   ASSERT_EQ(Z_LVAL(table[1].getZval()), 4321);
   table.append(Variant("polarphp"));
   ASSERT_EQ(table[2].toString(), "polarphp");
}

TEST(HashTableTest, testDeleteItem)
{
   ZVMHashTable table;
   table.insert("item1", Variant(123));
   table.insert("item2", Variant("polarboy"));
   table.insert("item3", Variant(true));
   ASSERT_EQ(table.getSize(), 3);
   ASSERT_FALSE(table.remove("notExist"));
   ASSERT_TRUE(table.remove("item1"));
   ASSERT_EQ(table.getSize(), 2);
   ASSERT_TRUE(table.remove("item2"));
   ASSERT_TRUE(table.remove("item3"));
   ASSERT_EQ(table.getSize(), 0);
   table.insert(0, Variant(true));
   table.insert(1, Variant(false));
   ASSERT_EQ(table.getSize(), 2);
   ASSERT_FALSE(table.remove(3));
   ASSERT_TRUE(table.remove(1));
   ASSERT_TRUE(table.remove(0));
   ASSERT_EQ(table.getSize(), 0);
}

TEST(HashTableTest, testContains)
{
   ZVMHashTable table;
   ASSERT_FALSE(table.contains(1));
   table.insert(0, Variant("polarphp"));
   ASSERT_FALSE(table.contains(1));
   table.insert(1, Variant("polarphp"));
   ASSERT_TRUE(table.contains(1));
   ASSERT_FALSE(table.contains("name"));
   table.insert("name", Variant("polarphp"));
   ASSERT_TRUE(table.contains("name"));
}

TEST(HashTableTest, testEach)
{
   ZVMHashTable table;
   table.insert("item1", Variant(123));
   table.insert("item2", Variant("polarboy"));
   table.insert("item3", Variant(true));
   {
      std::vector<std::string> expectedKeys{"item1", "item2", "item3"};
      std::vector<std::string> keys;
      std::vector<Variant> values;
      table.each([&keys, &values](const Variant &key, const Variant &value) mutable{
         if (key.getType() == polar::vmapi::Type::String) {
            keys.push_back(key.toString());
         }
         values.push_back(value);
      });
      ASSERT_EQ(keys.size(), 3);
      ASSERT_EQ(keys, expectedKeys);
      ASSERT_EQ(values.size(), 3);
      ASSERT_EQ(Z_LVAL(values[0].getZval()), 123);
      ASSERT_EQ(values[1].toString(), "polarboy");
      ASSERT_EQ(values[2].toBoolean(), true);
   }
   {
      std::vector<std::string> expectedKeys{"item3", "item2", "item1"};
      std::vector<std::string> keys;
      std::vector<Variant> values;
      table.reverseEach([&keys, &values](const Variant &key, const Variant &value) mutable{
         if (key.getType() == polar::vmapi::Type::String) {
            keys.push_back(key.toString());
         }
         values.push_back(value);
      });
      ASSERT_EQ(keys.size(), 3);
      ASSERT_EQ(keys, expectedKeys);
      ASSERT_EQ(values.size(), 3);
      ASSERT_EQ(values[0].toBoolean(), true);
      ASSERT_EQ(values[1].toString(), "polarboy");
      ASSERT_EQ(Z_LVAL(values[2].getZval()), 123);
   }
}

TEST(HashTableTest, testGetKeysAndValues)
{
   ZVMHashTable table;
   table.insert("item1", Variant(123));
   table.insert("item2", Variant("polarboy"));
   table.insert("item3", Variant(true));
   std::vector<Variant> expectedValues{123, "polarboy", true};
   std::vector<Variant> expectedKeys{"item1", "item2", "item3"};
   ASSERT_EQ(table.getKeys(), expectedKeys);
   ASSERT_EQ(table.getValues(), expectedValues);
}
