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

#include "polarphp/utils/Process.h"
#include "gtest/gtest.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

using namespace polar::basic;
using namespace polar::utils;
using namespace polar::sys;

TEST(ProcessTest, testGetRandomNumberTest)
{
   const unsigned r1 = Process::getRandomNumber();
   const unsigned r2 = Process::getRandomNumber();
   // It should be extremely unlikely that both r1 and r2 are 0.
   EXPECT_NE((r1 | r2), 0u);
}

#ifdef _MSC_VER
#define setenv(name, var, ignore) _putenv_s(name, var)
#endif

#if defined(HAVE_SETENV) || defined(_MSC_VER)
TEST(ProcessTest, testBasic) {
   setenv("__POLAR_TEST_ENVIRON_VAR__", "abc", true);
   std::optional<std::string> val(Process::getEnv("__POLAR_TEST_ENVIRON_VAR__"));
   EXPECT_TRUE(val.has_value());
   EXPECT_STREQ("abc", val->c_str());
}

TEST(ProcessTest, testNone)
{
   std::optional<std::string> val(
            Process::getEnv("__POLAR_TEST_ENVIRON_NO_SUCH_VAR__"));
   EXPECT_FALSE(val.has_value());
}
#endif

#ifdef _WIN32

TEST(ProcessTest, testEmptyVal)
{
   SetEnvironmentVariableA("__POLAR_TEST_ENVIRON_VAR__", "");
   std::optional<std::string> val(Process::getEnv("__POLAR_TEST_ENVIRON_VAR__"));
   EXPECT_TRUE(val.has_value());
   EXPECT_STREQ("", val->c_str());
}

TEST(ProcessTest, testWchar)
{
   SetEnvironmentVariableW(L"__POLAR_TEST_ENVIRON_VAR__", L"abcdefghijklmnopqrs");
   std::optional<std::string> val(Process::getEnv("__POLAR_TEST_ENVIRON_VAR__"));
   EXPECT_TRUE(val.hasValue());
   EXPECT_STREQ("abcdefghijklmnopqrs", val->c_str());
}
#endif

} // end anonymous namespace
