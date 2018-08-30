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
#include <thread>
#include "ProcessUtils.h"
#include <sys/types.h>
#include <signal.h>
#include <set>

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
      if(-1 == setenv("PATH", path.c_str(), true)) {
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
      if(-1 == setenv("PATH", sm_oldPath.c_str(), true)) {
         FAIL() << "restore PATH env var fail";
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
   static std::string sm_oldPath;
};

fs::path ProcessUtilsTest::sm_tempDir{UNITTEST_TEMP_DIR};
std::string ProcessUtilsTest::sm_oldPath;

TEST_F(ProcessUtilsTest, testFindExecutable)
{
   ASSERT_FALSE(polar::lit::find_executable("something_not_exists"));
   ASSERT_TRUE(polar::lit::find_executable(std::string(UNITTEST_CURRENT_BUILD_DIR)+"/LitlibTest"));
}

TEST_F(ProcessUtilsTest, testLookPath)
{
   {
      std::optional<std::string> file = polar::lit::look_path("ls");
      ASSERT_TRUE(file.has_value());
   }
   {
      std::optional<std::string> file = polar::lit::look_path("not_exist_cmd");
      ASSERT_FALSE(file.has_value());
   }
   {
      std::optional<std::string> file = polar::lit::look_path("lit");
      ASSERT_TRUE(file.has_value());
      ASSERT_EQ(file.value(), std::string(POLAR_BUILD_BINARY_DIR)+"/lit");
   }
}

TEST_F(ProcessUtilsTest, testCallPgrepCommand)
{
   std::set<pid_t> processes;
   for (int i = 0; i < 3; i++) {
      pid_t pid = fork();
      if (pid == 0) {
         // child process
         std::this_thread::sleep_for(std::chrono::milliseconds(800));
         exit(0);
      } else if (pid == -1) {
         FAIL() << "fork error";
      } else {
         processes.insert(pid);
      }
   }
   std::set<pid_t> expectedPids;
   int status = 0;
   std::tuple<std::list<pid_t>, bool> result = polar::lit::call_pgrep_command(getpid());
   if (std::get<1>(result)) {
      for (int cpid : std::get<0>(result)) {
         expectedPids.insert(cpid);
      }
   }
   while (-1 != wait(&status) || errno == EINTR || errno == EAGAIN)
   {}
   ASSERT_EQ(processes, expectedPids);
}

} // anonymous namespace
