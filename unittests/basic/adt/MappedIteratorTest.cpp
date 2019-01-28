// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/07/08.

#include "polarphp/basic/adt/StlExtras.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {

TEST(MappedIteratorTest, testApplyFunctionOnDereference)
{
   std::vector<int> V({0});

   auto I = map_iterator(V.begin(), [](int X) { return X + 1; });

   EXPECT_EQ(*I, 1) << "should have applied function in dereference";
}

TEST(MappedIteratorTest, testApplyFunctionOnArrow)
{
   struct S {
      int Z = 0;
   };

   std::vector<int> V({0});
   S Y;
   S* P = &Y;

   auto I = map_iterator(V.begin(), [&](int X) -> S& { return *(P + X); });

   I->Z = 42;

   EXPECT_EQ(Y.Z, 42) << "should have applied function during arrow";
}

TEST(MappedIteratorTest, testFunctionPreservesReferences)
{
   std::vector<int> V({1});
   std::map<int, int> M({ {1, 1} });

   auto I = map_iterator(V.begin(), [&](int X) -> int& { return M[X]; });
   *I = 42;

   EXPECT_EQ(M[1], 42) << "assignment should have modified M";
}

} // anonymous namespace
