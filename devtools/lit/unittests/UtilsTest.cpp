// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/29.

#include <gtest/gtest.h>
#include "Utils.h"
#include <string>

TEST(UtilsTest, testSplitString)
{
   {
      std::string str{"aaa"};
      std::list<std::string> result = polar::lit::split_string(str, ':');
      std::list<std::string> expected{"aaa"};
      ASSERT_EQ(result.size(), 1);
      ASSERT_EQ(result, expected);
   }
   {
      std::string str{"aaa:bbb:ccc"};
      std::list<std::string> result = polar::lit::split_string(str, ':');
      std::list<std::string> expected{"aaa","bbb","ccc"};
      ASSERT_EQ(result.size(), 3);
      ASSERT_EQ(result, expected);
   }
   {
      std::string str{":aaa:bbb :ccc:"};
      std::list<std::string> result = polar::lit::split_string(str, ':');
      std::list<std::string> expected{"aaa","bbb ","ccc"};
      ASSERT_EQ(result.size(), 3);
      ASSERT_EQ(result, expected);
   }
}


TEST(UtilsTest, testCenterString)
{
   std::string test = "polarphp";
   ASSERT_EQ(polar::lit::center_string(test, 0), "polarphp");
   ASSERT_EQ(polar::lit::center_string(test, 10), " polarphp ");
   ASSERT_EQ(polar::lit::center_string(test, 10, '-'), "-polarphp-");
   ASSERT_EQ(polar::lit::center_string(test, 13, '-'), "--polarphp--");
}
