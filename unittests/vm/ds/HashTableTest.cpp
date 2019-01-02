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

