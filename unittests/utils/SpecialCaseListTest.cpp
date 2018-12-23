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

#include "polarphp/utils/SpecialCaseList.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "gtest/gtest.h"

using namespace polar;
using namespace polar::basic;
using namespace polar::utils;

namespace {

class SpecialCaseListTest : public ::testing::Test
{
protected:
   std::unique_ptr<SpecialCaseList> makeSpecialCaseList(StringRef list,
                                                        std::string &error)
   {
      std::unique_ptr<MemoryBuffer> memoryBuffer = MemoryBuffer::getMemBuffer(list);
      return SpecialCaseList::create(memoryBuffer.get(), error);
   }

   std::unique_ptr<SpecialCaseList> makeSpecialCaseList(StringRef list)
   {
      std::string error;
      auto SCL = makeSpecialCaseList(list, error);
      assert(SCL);
      assert(error == "");
      return SCL;
   }

   std::string makeSpecialCaseListFile(StringRef contents)
   {
      int fd;
      SmallString<64> path;
      fs::create_temporary_file("SpecialCaseListTest", "temp", fd, path);
      RawFdOutStream OF(fd, true, true);
      OF << contents;
      OF.close();
      return path.getStr();
   }
};

TEST_F(SpecialCaseListTest, testBasic)
{
   std::unique_ptr<SpecialCaseList> SCL =
         makeSpecialCaseList(
            "# This is a comment.\n"
            "\n"
            "src:hello\n"
            "src:bye\n"
            "src:hi=category\n"
            "src:z*=category\n");
   EXPECT_TRUE(SCL->inSection("", "src", "hello"));
   EXPECT_TRUE(SCL->inSection("", "src", "bye"));
   EXPECT_TRUE(SCL->inSection("", "src", "hi", "category"));
   EXPECT_TRUE(SCL->inSection("", "src", "zzzz", "category"));
   EXPECT_FALSE(SCL->inSection("", "src", "hi"));
   EXPECT_FALSE(SCL->inSection("", "fun", "hello"));
   EXPECT_FALSE(SCL->inSection("", "src", "hello", "category"));

   EXPECT_EQ(3u, SCL->inSectionBlame("", "src", "hello"));
   EXPECT_EQ(4u, SCL->inSectionBlame("", "src", "bye"));
   EXPECT_EQ(5u, SCL->inSectionBlame("", "src", "hi", "category"));
   EXPECT_EQ(6u, SCL->inSectionBlame("", "src", "zzzz", "category"));
   EXPECT_EQ(0u, SCL->inSectionBlame("", "src", "hi"));
   EXPECT_EQ(0u, SCL->inSectionBlame("", "fun", "hello"));
   EXPECT_EQ(0u, SCL->inSectionBlame("", "src", "hello", "category"));
}

TEST_F(SpecialCaseListTest, testCorrectErrorLineNumberWithBlankLine)
{
   std::string error;
   EXPECT_EQ(nullptr, makeSpecialCaseList("# This is a comment.\n"
                                          "\n"
                                          "[not valid\n",
                                          error));
   EXPECT_TRUE(
            ((StringRef)error).startsWith("malformed section header on line 3:"));

   EXPECT_EQ(nullptr, makeSpecialCaseList("\n\n\n"
                                          "[not valid\n",
                                          error));
   EXPECT_TRUE(
            ((StringRef)error).startsWith("malformed section header on line 4:"));
}

TEST_F(SpecialCaseListTest, testSectionRegexErrorHandling)
{
   std::string error;
   EXPECT_EQ(makeSpecialCaseList("[address", error), nullptr);
   EXPECT_TRUE(((StringRef)error).startsWith("malformed section header "));

   EXPECT_EQ(makeSpecialCaseList("[[]", error), nullptr);
   EXPECT_TRUE(((StringRef)error).startsWith("malformed regex for section [: "));

   EXPECT_EQ(makeSpecialCaseList("src:=", error), nullptr);
   EXPECT_TRUE(((StringRef)error).endsWith("Supplied regexp was blank"));
}

TEST_F(SpecialCaseListTest, testSection)
{
   std::unique_ptr<SpecialCaseList> SCL = makeSpecialCaseList("src:global\n"
                                                              "[sect1|sect2]\n"
                                                              "src:test1\n"
                                                              "[sect3*]\n"
                                                              "src:test2\n");
   EXPECT_TRUE(SCL->inSection("arbitrary", "src", "global"));
   EXPECT_TRUE(SCL->inSection("", "src", "global"));
   EXPECT_TRUE(SCL->inSection("sect1", "src", "test1"));
   EXPECT_FALSE(SCL->inSection("sect1-arbitrary", "src", "test1"));
   EXPECT_FALSE(SCL->inSection("sect", "src", "test1"));
   EXPECT_FALSE(SCL->inSection("sect1", "src", "test2"));
   EXPECT_TRUE(SCL->inSection("sect2", "src", "test1"));
   EXPECT_TRUE(SCL->inSection("sect3", "src", "test2"));
   EXPECT_TRUE(SCL->inSection("sect3-arbitrary", "src", "test2"));
   EXPECT_FALSE(SCL->inSection("", "src", "test1"));
   EXPECT_FALSE(SCL->inSection("", "src", "test2"));
}

TEST_F(SpecialCaseListTest, GlobalInit)
{
   std::unique_ptr<SpecialCaseList> SCL =
         makeSpecialCaseList("global:foo=init\n");
   EXPECT_FALSE(SCL->inSection("", "global", "foo"));
   EXPECT_FALSE(SCL->inSection("", "global", "bar"));
   EXPECT_TRUE(SCL->inSection("", "global", "foo", "init"));
   EXPECT_FALSE(SCL->inSection("", "global", "bar", "init"));

   SCL = makeSpecialCaseList("type:t2=init\n");
   EXPECT_FALSE(SCL->inSection("", "type", "t1"));
   EXPECT_FALSE(SCL->inSection("", "type", "t2"));
   EXPECT_FALSE(SCL->inSection("", "type", "t1", "init"));
   EXPECT_TRUE(SCL->inSection("", "type", "t2", "init"));

   SCL = makeSpecialCaseList("src:hello=init\n");
   EXPECT_FALSE(SCL->inSection("", "src", "hello"));
   EXPECT_FALSE(SCL->inSection("", "src", "bye"));
   EXPECT_TRUE(SCL->inSection("", "src", "hello", "init"));
   EXPECT_FALSE(SCL->inSection("", "src", "bye", "init"));
}

TEST_F(SpecialCaseListTest, Substring)
{
   std::unique_ptr<SpecialCaseList> SCL = makeSpecialCaseList("src:hello\n"
                                                              "fun:foo\n"
                                                              "global:bar\n");
   EXPECT_FALSE(SCL->inSection("", "src", "othello"));
   EXPECT_FALSE(SCL->inSection("", "fun", "tomfoolery"));
   EXPECT_FALSE(SCL->inSection("", "global", "bartender"));

   SCL = makeSpecialCaseList("fun:*foo*\n");
   EXPECT_TRUE(SCL->inSection("", "fun", "tomfoolery"));
   EXPECT_TRUE(SCL->inSection("", "fun", "foobar"));
}

TEST_F(SpecialCaseListTest, InvalidSpecialCaseList)
{
   std::string error;
   EXPECT_EQ(nullptr, makeSpecialCaseList("badline", error));
   EXPECT_EQ("malformed line 1: 'badline'", error);
   EXPECT_EQ(nullptr, makeSpecialCaseList("src:bad[a-", error));
   EXPECT_EQ("malformed regex in line 1: 'bad[a-': Invalid range in bracket expression.",
             error);
   EXPECT_EQ(nullptr, makeSpecialCaseList("src:a.c\n"
                                          "fun:fun(a\n",
                                          error));
   EXPECT_EQ("malformed regex in line 2: 'fun(a': Parenthesis is not closed.",
             error);
   std::vector<std::string> files(1, "unexisting");
   EXPECT_EQ(nullptr, SpecialCaseList::create(files, error));
   EXPECT_EQ(0U, error.find("can't open file 'unexisting':"));
}

TEST_F(SpecialCaseListTest, EmptySpecialCaseList)
{
   std::unique_ptr<SpecialCaseList> SCL = makeSpecialCaseList("");
   EXPECT_FALSE(SCL->inSection("", "foo", "bar"));
}

TEST_F(SpecialCaseListTest, MultipleBlacklists)
{
   std::vector<std::string> files;
   files.push_back(makeSpecialCaseListFile("src:bar\n"
                                           "src:*foo*\n"
                                           "src:ban=init\n"));
   files.push_back(makeSpecialCaseListFile("src:baz\n"
                                           "src:*fog*\n"));
   auto SCL = SpecialCaseList::createOrDie(files);
   EXPECT_TRUE(SCL->inSection("", "src", "bar"));
   EXPECT_TRUE(SCL->inSection("", "src", "baz"));
   EXPECT_FALSE(SCL->inSection("", "src", "ban"));
   EXPECT_TRUE(SCL->inSection("", "src", "ban", "init"));
   EXPECT_TRUE(SCL->inSection("", "src", "tomfoolery"));
   EXPECT_TRUE(SCL->inSection("", "src", "tomfoglery"));
   for (auto &path : files) {
      fs::remove(path);
   }
}

TEST_F(SpecialCaseListTest, testNoTrigramsInRules)
{
   std::unique_ptr<SpecialCaseList> SCL = makeSpecialCaseList("fun:b.r\n"
                                                              "fun:za*az\n");
   EXPECT_TRUE(SCL->inSection("", "fun", "bar"));
   EXPECT_FALSE(SCL->inSection("", "fun", "baz"));
   EXPECT_TRUE(SCL->inSection("", "fun", "zakaz"));
   EXPECT_FALSE(SCL->inSection("", "fun", "zaraza"));
}

TEST_F(SpecialCaseListTest, testNoTrigramsInARule)
{
   std::unique_ptr<SpecialCaseList> SCL = makeSpecialCaseList("fun:*bar*\n"
                                                              "fun:za*az\n");
   EXPECT_TRUE(SCL->inSection("", "fun", "abara"));
   EXPECT_FALSE(SCL->inSection("", "fun", "bor"));
   EXPECT_TRUE(SCL->inSection("", "fun", "zakaz"));
   EXPECT_FALSE(SCL->inSection("", "fun", "zaraza"));
}

TEST_F(SpecialCaseListTest, testRepetitiveRule)
{
   std::unique_ptr<SpecialCaseList> SCL = makeSpecialCaseList("fun:*bar*bar*bar*bar*\n"
                                                              "fun:bar*\n");
   EXPECT_TRUE(SCL->inSection("", "fun", "bara"));
   EXPECT_FALSE(SCL->inSection("", "fun", "abara"));
   EXPECT_TRUE(SCL->inSection("", "fun", "barbarbarbar"));
   EXPECT_TRUE(SCL->inSection("", "fun", "abarbarbarbar"));
   EXPECT_FALSE(SCL->inSection("", "fun", "abarbarbar"));
}

TEST_F(SpecialCaseListTest, testSpecialSymbolRule)
{
   std::unique_ptr<SpecialCaseList> SCL = makeSpecialCaseList("src:*c\\+\\+abi*\n");
   EXPECT_TRUE(SCL->inSection("", "src", "c++abi"));
   EXPECT_FALSE(SCL->inSection("", "src", "c\\+\\+abi"));
}

TEST_F(SpecialCaseListTest, testPopularTrigram)
{
   std::unique_ptr<SpecialCaseList> SCL = makeSpecialCaseList("fun:*aaaaaa*\n"
                                                              "fun:*aaaaa*\n"
                                                              "fun:*aaaa*\n"
                                                              "fun:*aaa*\n");
   EXPECT_TRUE(SCL->inSection("", "fun", "aaa"));
   EXPECT_TRUE(SCL->inSection("", "fun", "aaaa"));
   EXPECT_TRUE(SCL->inSection("", "fun", "aaaabbbaaa"));
}

TEST_F(SpecialCaseListTest, testEscapedSymbols)
{
   std::unique_ptr<SpecialCaseList> SCL = makeSpecialCaseList("src:*c\\+\\+abi*\n"
                                                              "src:*hello\\\\world*\n");
   EXPECT_TRUE(SCL->inSection("", "src", "dir/c++abi"));
   EXPECT_FALSE(SCL->inSection("", "src", "dir/c\\+\\+abi"));
   EXPECT_FALSE(SCL->inSection("", "src", "c\\+\\+abi"));
   EXPECT_TRUE(SCL->inSection("", "src", "C:\\hello\\world"));
   EXPECT_TRUE(SCL->inSection("", "src", "hello\\world"));
   EXPECT_FALSE(SCL->inSection("", "src", "hello\\\\world"));
}

} // anonymous namespace
