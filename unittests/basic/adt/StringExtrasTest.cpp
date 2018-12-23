// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/07/06.

#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/utils/RawOutStream.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {

using polar::utils::RawStringOutStream;

TEST(StringExtrasTest, isPrint)
{
   EXPECT_FALSE(is_print('\0'));
   EXPECT_FALSE(is_print('\t'));
   EXPECT_TRUE(is_print('0'));
   EXPECT_TRUE(is_print('a'));
   EXPECT_TRUE(is_print('A'));
   EXPECT_TRUE(is_print(' '));
   EXPECT_TRUE(is_print('~'));
   EXPECT_TRUE(is_print('?'));
}

TEST(StringExtrasTest, testJoin)
{
   std::vector<std::string> items;
   EXPECT_EQ("", join(items.begin(), items.end(), " <sep> "));

   items = {"foo"};
   EXPECT_EQ("foo", join(items.begin(), items.end(), " <sep> "));

   items = {"foo", "bar"};
   EXPECT_EQ("foo <sep> bar", join(items.begin(), items.end(), " <sep> "));

   items = {"foo", "bar", "baz"};
   EXPECT_EQ("foo <sep> bar <sep> baz",
             join(items.begin(), items.end(), " <sep> "));
}

TEST(StringExtrasTest, testJoinItems)
{
   const char *Foo = "foo";
   std::string Bar = "bar";
   StringRef Baz = "baz";
   char X = 'x';

   EXPECT_EQ("", join_items(" <sep> "));
   EXPECT_EQ("", join_items('/'));

   EXPECT_EQ("foo", join_items(" <sep> ", Foo));
   EXPECT_EQ("foo", join_items('/', Foo));

   EXPECT_EQ("foo <sep> bar", join_items(" <sep> ", Foo, Bar));
   EXPECT_EQ("foo/bar", join_items('/', Foo, Bar));

   EXPECT_EQ("foo <sep> bar <sep> baz", join_items(" <sep> ", Foo, Bar, Baz));
   EXPECT_EQ("foo/bar/baz", join_items('/', Foo, Bar, Baz));

   EXPECT_EQ("foo <sep> bar <sep> baz <sep> x",
             join_items(" <sep> ", Foo, Bar, Baz, X));

   EXPECT_EQ("foo/bar/baz/x", join_items('/', Foo, Bar, Baz, X));
}

TEST(StringExtrasTest, testToAndFromHex)
{
   std::vector<uint8_t> OddBytes = {0x5, 0xBD, 0x0D, 0x3E, 0xCD};
   std::string OddStr = "05BD0D3ECD";
   StringRef OddData(reinterpret_cast<const char *>(OddBytes.data()),
                     OddBytes.size());
   EXPECT_EQ(OddStr, to_hex(OddData));
   EXPECT_EQ(OddData, from_hex(StringRef(OddStr).dropFront()));
   EXPECT_EQ(StringRef(OddStr).toLower(), to_hex(OddData, true));

   std::vector<uint8_t> EvenBytes = {0xA5, 0xBD, 0x0D, 0x3E, 0xCD};
   std::string EvenStr = "A5BD0D3ECD";
   StringRef EvenData(reinterpret_cast<const char *>(EvenBytes.data()),
                      EvenBytes.size());
   EXPECT_EQ(EvenStr, to_hex(EvenData));
   EXPECT_EQ(EvenData, from_hex(EvenStr));
   EXPECT_EQ(StringRef(EvenStr).toLower(), to_hex(EvenData, true));
}

TEST(StringExtrasTest, testToFloat)
{
   float F;
   EXPECT_TRUE(to_float("4.7", F));
   EXPECT_FLOAT_EQ(4.7f, F);

   double D;
   EXPECT_TRUE(to_float("4.7", D));
   EXPECT_DOUBLE_EQ(4.7, D);

   long double LD;
   EXPECT_TRUE(to_float("4.7", LD));
   EXPECT_DOUBLE_EQ(4.7, LD);

   EXPECT_FALSE(to_float("foo", F));
   EXPECT_FALSE(to_float("7.4 foo", F));
   EXPECT_FLOAT_EQ(4.7f, F); // F should be unchanged
}

TEST(StringExtrasTest, testPrintLowerCase)
{
   std::string str;
   RawStringOutStream out(str);
   print_lower_case("ABCdefg01234.,&!~`'}\"", out);
   EXPECT_EQ("abcdefg01234.,&!~`'}\"", out.getStr());
}

TEST(StringExtrasTest, printEscapedString)
{
   std::string str;
   RawStringOutStream out(str);
   print_escaped_string("ABCdef123&<>\\\"'\t", out);
   EXPECT_EQ("ABCdef123&<>\\5C\\22'\\09", out.getStr());
}

TEST(StringExtrasTest, printHTMLEscaped) {
   std::string str;
   RawStringOutStream out(str);
   print_html_escaped("ABCdef123&<>\"'", out);
   EXPECT_EQ("ABCdef123&amp;&lt;&gt;&quot;&apos;", out.getStr());
}

} // anonymous namespace
