// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/07.

#include "polarphp/basic/adt/EquivalenceClasses.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {

TEST(EquivalenceClassesTest, testNoMerges)
{
   EquivalenceClasses<int> eqClasses;
   // Until we merged any sets, check that every element is only equivalent to
   // itself.
   for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
         if (i == j) {
            EXPECT_TRUE(eqClasses.isEquivalent(i, j));
         }  else {
            EXPECT_FALSE(eqClasses.isEquivalent(i, j));
         }
      }
   }
}

TEST(EquivalenceClassesTest, testSimpleMerge1)
{
   EquivalenceClasses<int> eqClasses;
   // Check that once we merge (A, B), (B, C), (C, D), then all elements belong
   // to one set.
   eqClasses.unionSets(0, 1);
   eqClasses.unionSets(1, 2);
   eqClasses.unionSets(2, 3);
   for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
         EXPECT_TRUE(eqClasses.isEquivalent(i, j));
      }
   }
}

TEST(EquivalenceClassesTest, testSimpleMerge2)
{
   EquivalenceClasses<int> eqClasses;
   // Check that once we merge (A, B), (C, D), (A, C), then all elements belong
   // to one set.
   eqClasses.unionSets(0, 1);
   eqClasses.unionSets(2, 3);
   eqClasses.unionSets(0, 2);
   for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
         EXPECT_TRUE(eqClasses.isEquivalent(i, j));
      }
   }
}

TEST(EquivalenceClassesTest, testTwoSets)
{
   EquivalenceClasses<int> eqClasses;
   // Form sets of odd and even numbers, check that we split them into these
   // two sets correcrly.
   for (int i = 0; i < 30; i += 2) {
      eqClasses.unionSets(0, i);
   }
   for (int i = 1; i < 30; i += 2) {
      eqClasses.unionSets(1, i);
   }
   for (int i = 0; i < 30; i++) {
      for (int j = 0; j < 30; j++) {
         if (i % 2 == j % 2) {
            EXPECT_TRUE(eqClasses.isEquivalent(i, j));
         } else {
            EXPECT_FALSE(eqClasses.isEquivalent(i, j));
         }
      }
   }
}

TEST(EquivalenceClassesTest, testMultipleSets)
{
   EquivalenceClasses<int> eqClasses;
   // Split numbers from [0, 100) into sets so that values in the same set have
   // equal remainders (mod 17).
   for (int i = 0; i < 100; i++) {
      eqClasses.unionSets(i % 17, i);
   }
   for (int i = 0; i < 100; i++) {
      for (int j = 0; j < 100; j++) {
         if (i % 17 == j % 17) {
            EXPECT_TRUE(eqClasses.isEquivalent(i, j));
         } else {
            EXPECT_FALSE(eqClasses.isEquivalent(i, j));
         }
      }
   }
}

} // anonymous namespace
