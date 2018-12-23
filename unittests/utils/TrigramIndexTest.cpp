// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/15.

#include "polarphp/utils/TrigramIndex.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "gtest/gtest.h"

#include <string>
#include <vector>

using namespace polar;
using namespace polar::utils;

namespace {

class TrigramIndexTest : public ::testing::Test
{
protected:
   std::unique_ptr<TrigramIndex> makeTrigramIndex(
         std::vector<std::string> rules) {
      std::unique_ptr<TrigramIndex> titer =
            std::make_unique<TrigramIndex>();
      for (auto &rule : rules)
         titer->insert(rule);
      return titer;
   }
};

TEST_F(TrigramIndexTest, testEmpty)
{
   std::unique_ptr<TrigramIndex> titer =
         makeTrigramIndex({});
   EXPECT_FALSE(titer->isDefeated());
   EXPECT_TRUE(titer->isDefinitelyOut("foo"));
}

TEST_F(TrigramIndexTest, testBasic)
{
   std::unique_ptr<TrigramIndex> titer =
         makeTrigramIndex({"*hello*", "*wor.d*"});
   EXPECT_FALSE(titer->isDefeated());
   EXPECT_TRUE(titer->isDefinitelyOut("foo"));
}

TEST_F(TrigramIndexTest, testNoTrigramsInRules)
{
   std::unique_ptr<TrigramIndex> titer =
         makeTrigramIndex({"b.r", "za*az"});
   EXPECT_TRUE(titer->isDefeated());
   EXPECT_FALSE(titer->isDefinitelyOut("foo"));
   EXPECT_FALSE(titer->isDefinitelyOut("bar"));
   EXPECT_FALSE(titer->isDefinitelyOut("zakaz"));
}

TEST_F(TrigramIndexTest, testNoTrigramsInARule)
{
   std::unique_ptr<TrigramIndex> titer =
         makeTrigramIndex({"*hello*", "*wo.ld*"});
   EXPECT_TRUE(titer->isDefeated());
   EXPECT_FALSE(titer->isDefinitelyOut("foo"));
}

TEST_F(TrigramIndexTest, testRepetitiveRule)
{
   std::unique_ptr<TrigramIndex> titer =
         makeTrigramIndex({"*bar*bar*bar*bar*bar", "bar*bar"});
   EXPECT_FALSE(titer->isDefeated());
   EXPECT_TRUE(titer->isDefinitelyOut("foo"));
   EXPECT_TRUE(titer->isDefinitelyOut("bar"));
   EXPECT_FALSE(titer->isDefinitelyOut("barbara"));
   EXPECT_FALSE(titer->isDefinitelyOut("bar+bar"));
}

TEST_F(TrigramIndexTest, PopularTrigram) {
   std::unique_ptr<TrigramIndex> titer =
         makeTrigramIndex({"*aaa*", "*aaaa*", "*aaaaa*", "*aaaaa*", "*aaaaaa*"});
   EXPECT_TRUE(titer->isDefeated());
}

TEST_F(TrigramIndexTest, testPopularTrigram2)
{
   std::unique_ptr<TrigramIndex> titer =
         makeTrigramIndex({"class1.h", "class2.h", "class3.h", "class4.h", "class.h"});
   EXPECT_TRUE(titer->isDefeated());
}

TEST_F(TrigramIndexTest, testTooComplicatedRegex)
{
   std::unique_ptr<TrigramIndex> titer =
         makeTrigramIndex({"[0-9]+"});
   EXPECT_TRUE(titer->isDefeated());
}

TEST_F(TrigramIndexTest, testTooComplicatedRegex2)
{
   std::unique_ptr<TrigramIndex> titer =
         makeTrigramIndex({"foo|bar"});
   EXPECT_TRUE(titer->isDefeated());
}

TEST_F(TrigramIndexTest, testEscapedSymbols)
{
   std::unique_ptr<TrigramIndex> titer =
         makeTrigramIndex({"*c\\+\\+*", "*hello\\\\world*", "a\\tb", "a\\0b"});
   EXPECT_FALSE(titer->isDefeated());
   EXPECT_FALSE(titer->isDefinitelyOut("c++"));
   EXPECT_TRUE(titer->isDefinitelyOut("c\\+\\+"));
   EXPECT_FALSE(titer->isDefinitelyOut("hello\\world"));
   EXPECT_TRUE(titer->isDefinitelyOut("hello\\\\world"));
   EXPECT_FALSE(titer->isDefinitelyOut("atb"));
   EXPECT_TRUE(titer->isDefinitelyOut("a\\tb"));
   EXPECT_TRUE(titer->isDefinitelyOut("a\tb"));
   EXPECT_FALSE(titer->isDefinitelyOut("a0b"));
}

TEST_F(TrigramIndexTest, testBackreference1)
{
   std::unique_ptr<TrigramIndex> titer =
         makeTrigramIndex({"*foo\\1*"});
   EXPECT_TRUE(titer->isDefeated());
}

TEST_F(TrigramIndexTest, testBackreference2)
{
   std::unique_ptr<TrigramIndex> titer =
         makeTrigramIndex({"*foo\\2*"});
   EXPECT_TRUE(titer->isDefeated());
}

TEST_F(TrigramIndexTest, testSequence)
{
   std::unique_ptr<TrigramIndex> titer =
         makeTrigramIndex({"class1.h", "class2.h", "class3.h", "class4.h"});
   EXPECT_FALSE(titer->isDefeated());
   EXPECT_FALSE(titer->isDefinitelyOut("class1"));
   EXPECT_TRUE(titer->isDefinitelyOut("class.h"));
   EXPECT_TRUE(titer->isDefinitelyOut("class"));
}

}  // namespace
