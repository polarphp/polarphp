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
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include "ProcessUtils.h"

namespace {

namespace fs = std::filesystem;

class ProcessUtilsTest : public ::testing::Test
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
      // setup env variable PATH
      std::string path = std::getenv("PATH");
      sm_oldPath = path;
      path += ":";
      path += POLAR_BUILD_BINARY_DIR;
      if(setenv("PATH", path.c_str(), true)) {
         FAIL() << "setup PATH env var fail";
      }
   }

   static void TearDownTestCase()
   {
      if (fs::exists(sm_tempDir)) {
         std::error_code errcode;
         if (!fs::remove_all(sm_tempDir, errcode)) {
            FAIL() << errcode.message();
         }
      }
      if(setenv("PATH", sm_oldPath.c_str(), true)) {
         FAIL() << "restore PATH env var fail";
      }
   }

   virtual void SetUp() override
   {
      std::error_code errcode;
      if (!fs::remove_all(sm_tempDir, errcode)) {
         FAIL() << errcode.message();
      }
   }

   static fs::path sm_tempDir;
   static std::string sm_oldPath;
};

fs::path ProcessUtilsTest::sm_tempDir{UNITTEST_TEMP_DIR};
std::string ProcessUtilsTest::sm_oldPath;

TEST_F(ProcessUtilsTest, testFindExecutable)
{
   ASSERT_FALSE(polar::lit::find_executable("something_not_exists"));
   ASSERT_TRUE(polar::lit::find_executable(std::string(UNITTEST_CURRENT_BUILD_DIR)+"/LitlibTest"));
}

}
