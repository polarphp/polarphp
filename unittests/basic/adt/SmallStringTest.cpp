// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/06.

#include "polarphp/basic/adt/SmallString.h"
#include "gtest/gtest.h"
#include <climits>
#include <cstring>
#include <stdarg.h>

using namespace polar::basic;

namespace {

// Test fixture class
class SmallStringTest : public testing::Test
{
protected:
   typedef SmallString<40> StringType;

   StringType theString;

   void assertEmpty(StringType & v) {
      // Size tests
      EXPECT_EQ(0u, v.getSize());
      EXPECT_TRUE(v.empty());
      // Iterator tests
      EXPECT_TRUE(v.begin() == v.end());
   }
};

// New string test.
TEST_F(SmallStringTest, testEmptyStringTest)
{
   SCOPED_TRACE("EmptyStringTest");
   assertEmpty(theString);
   EXPECT_TRUE(theString.rbegin() == theString.rend());
}

TEST_F(SmallStringTest, testAssignRepeated)
{
   theString.assign(3, 'a');
   EXPECT_EQ(3u, theString.getSize());
   EXPECT_STREQ("aaa", theString.getCStr());
}

TEST_F(SmallStringTest, testAssignIterPair)
{
   StringRef abc = "abc";
   theString.assign(abc.begin(), abc.end());
   EXPECT_EQ(3u, theString.getSize());
   EXPECT_STREQ("abc", theString.getCStr());
}

TEST_F(SmallStringTest, testAssignStringRef)
{
   StringRef abc = "abc";
   theString.assign(abc);
   EXPECT_EQ(3u, theString.getSize());
   EXPECT_STREQ("abc", theString.getCStr());
}

TEST_F(SmallStringTest, testAssignSmallVector)
{
   StringRef abc = "abc";
   SmallVector<char, 10> abcVec(abc.begin(), abc.end());
   theString.assign(abcVec);
   EXPECT_EQ(3u, theString.getSize());
   EXPECT_STREQ("abc", theString.getCStr());
}

TEST_F(SmallStringTest, testAppendIterPair)
{
   StringRef abc = "abc";
   theString.append(abc.begin(), abc.end());
   theString.append(abc.begin(), abc.end());
   EXPECT_EQ(6u, theString.getSize());
   EXPECT_STREQ("abcabc", theString.getCStr());
}

TEST_F(SmallStringTest, testAppendStringRef)
{
   StringRef abc = "abc";
   theString.append(abc);
   theString.append(abc);
   EXPECT_EQ(6u, theString.getSize());
   EXPECT_STREQ("abcabc", theString.getCStr());
}

TEST_F(SmallStringTest, testAppendSmallVector)
{
   StringRef abc = "abc";
   SmallVector<char, 10> abcVec(abc.begin(), abc.end());
   theString.append(abcVec);
   theString.append(abcVec);
   EXPECT_EQ(6u, theString.getSize());
   EXPECT_STREQ("abcabc", theString.getCStr());
}

TEST_F(SmallStringTest, testSubstr)
{
   theString = "hello";
   EXPECT_EQ("lo", theString.substr(3));
   EXPECT_EQ("", theString.substr(100));
   EXPECT_EQ("hello", theString.substr(0, 100));
   EXPECT_EQ("o", theString.substr(4, 10));
}

TEST_F(SmallStringTest, testSlice)
{
   theString = "hello";
   EXPECT_EQ("l", theString.slice(2, 3));
   EXPECT_EQ("ell", theString.slice(1, 4));
   EXPECT_EQ("llo", theString.slice(2, 100));
   EXPECT_EQ("", theString.slice(2, 1));
   EXPECT_EQ("", theString.slice(10, 20));
}

TEST_F(SmallStringTest, testFind)
{
   theString = "hello";
   EXPECT_EQ(2U, theString.find('l'));
   EXPECT_EQ(StringRef::npos, theString.find('z'));
   EXPECT_EQ(StringRef::npos, theString.find("helloworld"));
   EXPECT_EQ(0U, theString.find("hello"));
   EXPECT_EQ(1U, theString.find("ello"));
   EXPECT_EQ(StringRef::npos, theString.find("zz"));
   EXPECT_EQ(2U, theString.find("ll", 2));
   EXPECT_EQ(StringRef::npos, theString.find("ll", 3));
   EXPECT_EQ(0U, theString.find(""));

   EXPECT_EQ(3U, theString.rfind('l'));
   EXPECT_EQ(StringRef::npos, theString.rfind('z'));
   EXPECT_EQ(StringRef::npos, theString.rfind("helloworld"));
   EXPECT_EQ(0U, theString.rfind("hello"));
   EXPECT_EQ(1U, theString.rfind("ello"));
   EXPECT_EQ(StringRef::npos, theString.rfind("zz"));

   EXPECT_EQ(2U, theString.findFirstOf('l'));
   EXPECT_EQ(1U, theString.findFirstOf("el"));
   EXPECT_EQ(StringRef::npos, theString.findFirstOf("xyz"));

   EXPECT_EQ(1U, theString.findFirstNotOf('h'));
   EXPECT_EQ(4U, theString.findFirstNotOf("hel"));
   EXPECT_EQ(StringRef::npos, theString.findFirstNotOf("hello"));

   theString = "hellx xello hell ello world foo bar hello";
   EXPECT_EQ(36U, theString.find("hello"));
   EXPECT_EQ(28U, theString.find("foo"));
   EXPECT_EQ(12U, theString.find("hell", 2));
   EXPECT_EQ(0U, theString.find(""));
}

TEST_F(SmallStringTest, testCount)
{
   theString = "hello";
   EXPECT_EQ(2U, theString.count('l'));
   EXPECT_EQ(1U, theString.count('o'));
   EXPECT_EQ(0U, theString.count('z'));
   EXPECT_EQ(0U, theString.count("helloworld"));
   EXPECT_EQ(1U, theString.count("hello"));
   EXPECT_EQ(1U, theString.count("ello"));
   EXPECT_EQ(0U, theString.count("zz"));
}

TEST_F(SmallStringTest, testRealloc)
{
   theString = "abcd";
   theString.reserve(100);
   EXPECT_EQ("abcd", theString);
   unsigned const N = 100000;
   theString.reserve(N);
   for (unsigned i = 0; i < N - 4; ++i)
      theString.push_back('y');
   EXPECT_EQ("abcdyyy", theString.slice(0, 7));
}

TEST_F(SmallStringTest, testComparisons)
{
   EXPECT_EQ(-1, SmallString<10>("aab").compare("aad"));
   EXPECT_EQ( 0, SmallString<10>("aab").compare("aab"));
   EXPECT_EQ( 1, SmallString<10>("aab").compare("aaa"));
   EXPECT_EQ(-1, SmallString<10>("aab").compare("aabb"));
   EXPECT_EQ( 1, SmallString<10>("aab").compare("aa"));
   EXPECT_EQ( 1, SmallString<10>("\xFF").compare("\1"));

   EXPECT_EQ(-1, SmallString<10>("AaB").compareLower("aAd"));
   EXPECT_EQ( 0, SmallString<10>("AaB").compareLower("aab"));
   EXPECT_EQ( 1, SmallString<10>("AaB").compareLower("AAA"));
   EXPECT_EQ(-1, SmallString<10>("AaB").compareLower("aaBb"));
   EXPECT_EQ( 1, SmallString<10>("AaB").compareLower("aA"));
   EXPECT_EQ( 1, SmallString<10>("\xFF").compareLower("\1"));

   EXPECT_EQ(-1, SmallString<10>("aab").compareNumeric("aad"));
   EXPECT_EQ( 0, SmallString<10>("aab").compareNumeric("aab"));
   EXPECT_EQ( 1, SmallString<10>("aab").compareNumeric("aaa"));
   EXPECT_EQ(-1, SmallString<10>("aab").compareNumeric("aabb"));
   EXPECT_EQ( 1, SmallString<10>("aab").compareNumeric("aa"));
   EXPECT_EQ(-1, SmallString<10>("1").compareNumeric("10"));
   EXPECT_EQ( 0, SmallString<10>("10").compareNumeric("10"));
   EXPECT_EQ( 0, SmallString<10>("10a").compareNumeric("10a"));
   EXPECT_EQ( 1, SmallString<10>("2").compareNumeric("1"));
   EXPECT_EQ( 0, SmallString<10>("llvm_v1i64_ty").compareNumeric("llvm_v1i64_ty"));
   EXPECT_EQ( 1, SmallString<10>("\xFF").compareNumeric("\1"));
   EXPECT_EQ( 1, SmallString<10>("V16").compareNumeric("V1_q0"));
   EXPECT_EQ(-1, SmallString<10>("V1_q0").compareNumeric("V16"));
   EXPECT_EQ(-1, SmallString<10>("V8_q0").compareNumeric("V16"));
   EXPECT_EQ( 1, SmallString<10>("V16").compareNumeric("V8_q0"));
   EXPECT_EQ(-1, SmallString<10>("V1_q0").compareNumeric("V8_q0"));
   EXPECT_EQ( 1, SmallString<10>("V8_q0").compareNumeric("V1_q0"));
}

} // anonymous namespace

