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
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <thread>
#include "ProcessUtils.h"
#include <sys/types.h>

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

TEST_F(ProcessUtilsTest, testRetrieveChildrenPids)
{
   // here we need create a channel, let children write it self child pids to it parent process
   std::set<pid_t> processes;
   for (int i = 0; i < 3; i++) {
      // child process
      int channel[2];
      if (-1 == pipe(channel)) {
         FAIL() << "create channel failt";
      }
      pid_t pid = fork();
      if (pid == 0) {
         // close read fd
         if (-1 == close(channel[0])) {
            perror("close channel read fd error");
            exit(1);
         }
         // io redirection
         if (-1 == close(STDOUT_FILENO)) {
            perror("close stdout fd of child process error");
            exit(1);
         }
         if (-1 == dup2(channel[1], STDOUT_FILENO)) {
            perror("dup stdout fd of child process error");
            exit(1);
         }
         pid_t cpid = fork();
         if (0 == cpid) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            exit(0);
         } else if (cpid == -1) {
            FAIL() << "fork error";
         }

         std::cout << cpid << std::endl;
         std::this_thread::sleep_for(std::chrono::seconds(2));
      } else if (pid == -1) {
         FAIL() << "fork error";
      } else {
         processes.insert(pid);
         std::this_thread::sleep_for(std::chrono::milliseconds(300));
         char readBuffer[64];
         // just read once
         ssize_t count = read(channel[0], readBuffer, 64);
         if (count == -1) {
            FAIL() << "read child process id error";
         }
         processes.insert(std::atoi(readBuffer));
      }
   }
   std::set<pid_t> expectedPids;
   int status = 0;
   std::tuple<std::list<pid_t>, bool> result = polar::lit::retrieve_children_pids(1);
   if (std::get<1>(result)) {
      for (int cpid : std::get<0>(result)) {
         std::cout << cpid<< std::endl;
      }
   }
   while (-1 != wait(&status) || errno == EINTR || errno == EAGAIN)
   {}
   ASSERT_EQ(processes, expectedPids);
}

} // anonymous namespace
