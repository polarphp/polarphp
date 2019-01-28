// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/29.

#include <gtest/gtest.h>
#include "Utils.h"
#include "Config.h"
#include <string>
#include <filesystem>

namespace fs = std::filesystem;
static fs::path sg_dataDir;

class UtilsTest : public ::testing::Test
{
public:

   static void SetUpTestCase()
   {
      if (!fs::exists(sm_tempDir)) {
         std::error_code errcode;
         if (!fs::create_directory(sm_tempDir, errcode)) {
            FAIL() << errcode.message();
         }
      }
      sg_dataDir = UNITTEST_LIT_DATA_DIR;
   }

   static void TearDownTestCase()
   {
      if (fs::exists(sm_tempDir)) {
         std::error_code errcode;
         if (!fs::remove_all(sm_tempDir, errcode)) {
            FAIL() << errcode.message();
         }
      }
   }

   virtual void SetUp() override
   {
      if (fs::exists(sm_tempDir)) {
         std::error_code errcode;
         if (!fs::remove_all(sm_tempDir, errcode)) {
            FAIL() << errcode.message();
         }
      }
   }

   static fs::path sm_tempDir;
};

fs::path UtilsTest::sm_tempDir{UNITTEST_TEMP_DIR};

TEST_F(UtilsTest, testSplitString)
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
   {
      std::string str{":aaa:bbb :ccc:"};
      std::list<std::string> result = polar::lit::split_string(str, ':', 1);
      std::list<std::string> expected{"aaa","bbb :ccc:"};
      ASSERT_EQ(result.size(), 2);
      ASSERT_EQ(result, expected);
   }
   {
      std::string str{":aaa:bbb :ccc:"};
      std::list<std::string> result = polar::lit::split_string(str, ':', 2);
      std::list<std::string> expected{"aaa","bbb ","ccc:"};
      ASSERT_EQ(result.size(), 3);
      ASSERT_EQ(result, expected);
   }
}

TEST_F(UtilsTest, testCenterString)
{
   std::string test = "polarphp";
   ASSERT_EQ(polar::lit::center_string(test, 0), "polarphp");
   ASSERT_EQ(polar::lit::center_string(test, 10), " polarphp ");
   ASSERT_EQ(polar::lit::center_string(test, 10, '-'), "-polarphp-");
   ASSERT_EQ(polar::lit::center_string(test, 13, '-'), "--polarphp--");
}

TEST_F(UtilsTest, testStartswithAndEndswidth)
{
   std::string str = "I am a programmer, I love php";
   {
      ASSERT_TRUE(polar::lit::string_startswith("abc", ""));
      ASSERT_TRUE(polar::lit::string_endswith("abc", ""));
      ASSERT_FALSE(polar::lit::string_startswith("abc", "abcd"));
      ASSERT_FALSE(polar::lit::string_endswith("abc", "abcd"));
   }
   {
      ASSERT_TRUE(polar::lit::string_startswith(str, "I am"));
      ASSERT_FALSE(polar::lit::string_startswith(str, "I amx"));
      ASSERT_TRUE(polar::lit::string_endswith(str, "php"));
      ASSERT_FALSE(polar::lit::string_startswith(str, "Php"));
      ASSERT_FALSE(polar::lit::string_startswith(str, "xphp"));
   }
}

TEST_F(UtilsTest, testListdirFiles)
{
   try {
      fs::create_directories(sm_tempDir / "aaa" / "bbb" / "ccc");
      fs::create_directories(sm_tempDir / "aaa" / "ddd");
      fs::create_directories(sm_tempDir / "eee");
      fs::copy_file(sg_dataDir/"Empty", sm_tempDir / "aaa"/ "a.txt");
      fs::copy_file(sg_dataDir/"Empty", sm_tempDir / "aaa"/ "b.txt");
      fs::copy_file(sg_dataDir/"Empty", sm_tempDir / "eee"/ "empty");
      fs::copy_file(sg_dataDir/"Empty", sm_tempDir / "eee"/ "polarphp");
      fs::copy_file(sg_dataDir/"Empty", sm_tempDir / "aaa" / "bbb" / "ccc" / "polarphp.dynamic");
      fs::copy_file(sg_dataDir/"Empty", sm_tempDir / "aaa" / "bbb" / "polarphp.exe");
   } catch(...) {
      FAIL() << "testListdirFiles prepare directory error";
   }
   {
      std::list<std::string> expected{
         sm_tempDir / "aaa"/ "a.txt",
               sm_tempDir / "aaa"/ "b.txt",
               sm_tempDir / "aaa" / "bbb" / "ccc" / "polarphp.dynamic",
               sm_tempDir / "aaa" / "bbb" / "polarphp.exe"
      };
      std::list<std::string> files = polar::lit::listdir_files((sm_tempDir / "aaa").string());
      ASSERT_EQ(files, expected);
   }
   {
      std::list<std::string> expected{
         sm_tempDir / "aaa" / "bbb" / "ccc" / "polarphp.dynamic"
      };
      std::list<std::string> files = polar::lit::listdir_files(( sm_tempDir / "aaa" / "bbb" / "ccc").string());
      ASSERT_EQ(files, expected);
   }
   {
      std::list<std::string> expected{
         sm_tempDir / "aaa"/ "a.txt",
               sm_tempDir / "aaa"/ "b.txt"
      };
      std::list<std::string> files = polar::lit::listdir_files((sm_tempDir / "aaa").string(), {"txt"});
      ASSERT_EQ(files, expected);
   }
   {
      std::list<std::string> expected{
         sm_tempDir / "aaa"/ "a.txt",
               sm_tempDir / "aaa"/ "b.txt",
               sm_tempDir / "aaa" / "bbb" / "polarphp.exe"
      };
      std::list<std::string> files = polar::lit::listdir_files((sm_tempDir / "aaa").string(), {""}, {
                                                                  sm_tempDir / "aaa" / "bbb" / "ccc" / "polarphp.dynamic"
                                                               });
      ASSERT_EQ(files, expected);
   }
}

TEST_F(UtilsTest, testJoinStringList)
{
   {
      std::list<std::string> paths{
         "aaa",
         "bbb",
         "ccc"
      };
      ASSERT_EQ(polar::lit::join_string_list(paths, ""), "aaabbbccc");
      ASSERT_EQ(polar::lit::join_string_list(paths, "-"), "aaa-bbb-ccc");
      ASSERT_EQ(polar::lit::join_string_list(paths, "xxx"), "aaaxxxbbbxxxccc");
   }
   {
      std::list<std::string> paths{
         "aaa"
      };
      ASSERT_EQ(polar::lit::join_string_list(paths, ""), "aaa");
      ASSERT_EQ(polar::lit::join_string_list(paths, "-"), "aaa");
      ASSERT_EQ(polar::lit::join_string_list(paths, "xxx"), "aaa");
   }
}

TEST_F(UtilsTest, testReplaceString)
{
   {
      std::string str("I am from China, I love php programming language!");
      polar::lit::replace_string("php", "polarphp", str);
      ASSERT_EQ(str, "I am from China, I love polarphp programming language!");
   }
   {
      std::string str("aaabbbccc");
      polar::lit::replace_string("php", "polarphp", str);
      ASSERT_EQ(str, "aaabbbccc");
   }
}
