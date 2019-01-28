// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/07/07.

#include "polarphp/basic/adt/StringSwitch.h"
#include "gtest/gtest.h"

using namespace polar::basic;

TEST(StringSwitchTest, testCase)
{
   auto translate = [](StringRef str) {
      return StringSwitch<int>(str)
            .cond("0", 0)
            .cond("1", 1)
            .cond("2", 2)
            .cond("3", 3)
            .cond("4", 4)
            .cond("5", 5)
            .cond("6", 6)
            .cond("7", 7)
            .cond("8", 8)
            .cond("9", 9)
            .cond("A", 10)
            .cond("B", 11)
            .cond("C", 12)
            .cond("D", 13)
            .cond("E", 14)
            .cond("F", 15)
            .defaultCond(-1);
   };
   EXPECT_EQ(1, translate("1"));
   EXPECT_EQ(2, translate("2"));
   EXPECT_EQ(11, translate("B"));
   EXPECT_EQ(-1, translate("b"));
   EXPECT_EQ(-1, translate(""));
   EXPECT_EQ(-1, translate("Test"));
}

TEST(StringSwitchTest, testCaseLower)
{
   auto translate = [](StringRef str)
   {
      return StringSwitch<int>(str)
            .cond("0", 0)
            .cond("1", 1)
            .cond("2", 2)
            .cond("3", 3)
            .cond("4", 4)
            .cond("5", 5)
            .cond("6", 6)
            .cond("7", 7)
            .cond("8", 8)
            .cond("9", 9)
            .condLower("A", 10)
            .condLower("B", 11)
            .condLower("C", 12)
            .condLower("D", 13)
            .condLower("E", 14)
            .condLower("F", 15)
            .defaultCond(-1);
   };
   EXPECT_EQ(1, translate("1"));
   EXPECT_EQ(2, translate("2"));
   EXPECT_EQ(11, translate("B"));
   EXPECT_EQ(11, translate("b"));

   EXPECT_EQ(-1, translate(""));
   EXPECT_EQ(-1, translate("Test"));
}

TEST(StringSwitchTest, testStartsWith)
{
   auto translate = [](StringRef str) {
      return StringSwitch<std::function<int(int, int)>>(str)
                                                       .startsWith("add", [](int X, int Y) { return X + Y; })
                                                       .startsWith("sub", [](int X, int Y) { return X - Y; })
                                                       .startsWith("mul", [](int X, int Y) { return X * Y; })
                                                       .startsWith("div", [](int X, int Y) { return X / Y; })
                                                       .defaultCond([](int X, int Y) { return 0; });
   };

   EXPECT_EQ(15, translate("adder")(10, 5));
   EXPECT_EQ(5, translate("subtracter")(10, 5));
   EXPECT_EQ(50, translate("multiplier")(10, 5));
   EXPECT_EQ(2, translate("divider")(10, 5));

   EXPECT_EQ(0, translate("nothing")(10, 5));
   EXPECT_EQ(0, translate("ADDER")(10, 5));
}

TEST(StringSwitchTest, testStartsWithLower)
{
   auto translate = [](StringRef str) {
      return StringSwitch<std::function<int(int, int)>>(str)
                                                       .startsWithLower("add", [](int X, int Y) { return X + Y; })
                                                       .startsWithLower("sub", [](int X, int Y) { return X - Y; })
                                                       .startsWithLower("mul", [](int X, int Y) { return X * Y; })
                                                       .startsWithLower("div", [](int X, int Y) { return X / Y; })
                                                       .defaultCond([](int X, int Y) { return 0; });
   };

   EXPECT_EQ(15, translate("adder")(10, 5));
   EXPECT_EQ(5, translate("subtracter")(10, 5));
   EXPECT_EQ(50, translate("multiplier")(10, 5));
   EXPECT_EQ(2, translate("divider")(10, 5));

   EXPECT_EQ(15, translate("AdDeR")(10, 5));
   EXPECT_EQ(5, translate("SuBtRaCtEr")(10, 5));
   EXPECT_EQ(50, translate("MuLtIpLiEr")(10, 5));
   EXPECT_EQ(2, translate("DiViDeR")(10, 5));

   EXPECT_EQ(0, translate("nothing")(10, 5));
}

TEST(StringSwitchTest, testEndsWith)
{
   enum class Suffix { Possible, PastTense, Process, InProgressAction, Unknown };

   auto translate = [](StringRef str) {
      return StringSwitch<Suffix>(str)
            .endsWith("able", Suffix::Possible)
            .endsWith("ed", Suffix::PastTense)
            .endsWith("ation", Suffix::Process)
            .endsWith("ing", Suffix::InProgressAction)
            .defaultCond(Suffix::Unknown);
   };

   EXPECT_EQ(Suffix::Possible, translate("optimizable"));
   EXPECT_EQ(Suffix::PastTense, translate("optimized"));
   EXPECT_EQ(Suffix::Process, translate("optimization"));
   EXPECT_EQ(Suffix::InProgressAction, translate("optimizing"));
   EXPECT_EQ(Suffix::Unknown, translate("optimizer"));
   EXPECT_EQ(Suffix::Unknown, translate("OPTIMIZABLE"));
}

TEST(StringSwitchTest, endsWithLower)
{
   enum class Suffix { Possible, PastTense, Process, InProgressAction, Unknown };

   auto translate = [](StringRef str) {
      return StringSwitch<Suffix>(str)
            .endsWithLower("able", Suffix::Possible)
            .endsWithLower("ed", Suffix::PastTense)
            .endsWithLower("ation", Suffix::Process)
            .endsWithLower("ing", Suffix::InProgressAction)
            .defaultCond(Suffix::Unknown);
   };

   EXPECT_EQ(Suffix::Possible, translate("optimizable"));
   EXPECT_EQ(Suffix::Possible, translate("OPTIMIZABLE"));
   EXPECT_EQ(Suffix::PastTense, translate("optimized"));
   EXPECT_EQ(Suffix::Process, translate("optimization"));
   EXPECT_EQ(Suffix::InProgressAction, translate("optimizing"));
   EXPECT_EQ(Suffix::Unknown, translate("optimizer"));
}

TEST(StringSwitchTest, testCases)
{
   enum class OSType { Windows, Linux, Unknown };

   auto translate = [](StringRef str) {
      return StringSwitch<OSType>(str)
            .conds(StringLiteral::withInnerNUL("wind\0ws"), "win32", "winnt", OSType::Windows)
            .conds("linux", "unix", "*nix", "posix", OSType::Linux)
            .defaultCond(OSType::Unknown);
   };

   EXPECT_EQ(OSType::Windows, translate(StringRef("wind\0ws", 7)));
   EXPECT_EQ(OSType::Windows, translate("win32"));
   EXPECT_EQ(OSType::Windows, translate("winnt"));

   EXPECT_EQ(OSType::Linux, translate("linux"));
   EXPECT_EQ(OSType::Linux, translate("unix"));
   EXPECT_EQ(OSType::Linux, translate("*nix"));
   EXPECT_EQ(OSType::Linux, translate("posix"));

   // Note that the whole null-terminator embedded string is required for the
   // case to match.
   EXPECT_EQ(OSType::Unknown, translate("wind"));
   EXPECT_EQ(OSType::Unknown, translate("Windows"));
   EXPECT_EQ(OSType::Unknown, translate(""));
}

TEST(StringSwitchTest, testCondsLower)
{
   enum class OSType { Windows, Linux, Unknown };

   auto translate = [](StringRef str) {
      return StringSwitch<OSType>(str)
            .condsLower(StringLiteral::withInnerNUL("wind\0ws"), "win32", "winnt", OSType::Windows)
            .condsLower("linux", "unix", "*nix", "posix", OSType::Linux)
            .defaultCond(OSType::Unknown);
   };

   EXPECT_EQ(OSType::Windows, translate(StringRef("WIND\0WS", 7)));
   EXPECT_EQ(OSType::Windows, translate("WIN32"));
   EXPECT_EQ(OSType::Windows, translate("WINNT"));

   EXPECT_EQ(OSType::Linux, translate("LINUX"));
   EXPECT_EQ(OSType::Linux, translate("UNIX"));
   EXPECT_EQ(OSType::Linux, translate("*NIX"));
   EXPECT_EQ(OSType::Linux, translate("POSIX"));

   EXPECT_EQ(OSType::Windows, translate(StringRef("wind\0ws", 7)));
   EXPECT_EQ(OSType::Linux, translate("linux"));

   EXPECT_EQ(OSType::Unknown, translate("wind"));
   EXPECT_EQ(OSType::Unknown, translate(""));
}

