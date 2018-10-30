// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/05.

#include "GoogleTest.h"
#include "../Utils.h"
#include "../ProcessUtils.h"
#include "../LitConfig.h"
#include "../TestingConfig.h"
#include "polarphp/basic/adt/StringRef.h"
#include "../Test.h"
#include <iostream>
#include <sstream>

namespace polar {
namespace lit {

GoogleTest::GoogleTest(const std::string &testSubDirs,
                       const std::string &testSuffix)
{
   m_testSubDirs = split_string(testSubDirs, ';');
   // On Windows, assume tests will also end in '.exe'.
#if defined(POLAR_OS_WIN32) || defined(POLAR_OS_CYGWIN)
   m_testSuffixes.insert(testSuffix+".exe");
#endif
   m_testSuffixes.insert(testSuffix + ".littest");
}

namespace {
inline bool search_in_string_list(std::function<bool(const std::string &)> predictor,
                                  const std::list<std::string> &strList)
{
   for (const std::string &str : strList) {
      if (predictor(str)) {
         return true;
      }
   }
   return false;
}

inline bool is_start_with(const std::string &str)
{
   return string_startswith(str, "DISABLED_");
}

inline std::list<std::string> &append_string_list(std::list<std::string> &list, const std::string &str)
{
   list.push_back(str);
   return list;
}
} // anonymous namespace

///
/// getGTestTests(path) - [name]
/// Return the tests available in gtest executable.
///
/// Args:
/// path: String path to a gtest executable
/// litConfig: LitConfig instance
/// localConfig: TestingConfig instance
///
std::list<std::string> GoogleTest::getGTestTests(const std::string &path, LitConfigPointer litConfig,
                                                 TestingConfigPointer localConfig)
{
   RunCmdResponse response = run_program(path, std::nullopt, localConfig->getEnvironment(),
                                         std::nullopt, "--gtest_list_tests");
   int exitCode = std::get<0>(response);
   std::string output = std::get<1>(response);
   std::string errorMsg = std::get<2>(response);
   if (0 != exitCode) {
      litConfig->warning(
               format_string("unable to discover google-tests in %s. Process output: %s",
                             path.c_str(), output.c_str()));
      throw std::runtime_error(format_string("unable to discover google-tests in %s. Process output: %s",
                                             path.c_str(), errorMsg.c_str()));
   }
   std::istringstream stream(output);
   char lineBuf[256];
   std::list<std::string> nestedTests;
   std::list<std::string> tests;
   while (stream.getline(lineBuf, sizeof(lineBuf))) {
      std::string line(lineBuf);
      if (line.empty()) {
         continue;
      }
      if (line.find("Running main() from") != std::string::npos) {
         // Upstream googletest prints this to stdout prior to running
         // tests. polarphp removed that print statement in r61540, but we
         // handle it here in case upstream googletest is being used.
         continue;
      }
      // The test name list includes trailing comments beginning with
      // a '#' on some lines, so skip those. We don't support test names
      // that use escaping to embed '#' into their name as the names come
      // from C++ class and method names where such things are hard and
      // uninteresting to support.
      std::list<std::string> parts = split_string(line, '#', 1);
      line = parts.front();
      rtrim_string(line);
      ltrim_string(line);
      if (line.empty()) {
         continue;
      }
      size_t index = 0;
      while (line.substr(index * 2, 2) == "  ") {
         index += 1;
      }
      while (nestedTests.size() > index) {
         nestedTests.pop_back();
      }
      line = line.substr(index * 2);
      if (string_endswith(line, ".")) {
         nestedTests.push_back(line);
      } else if (search_in_string_list(is_start_with, append_string_list(nestedTests, line))) {
         // Gtest will internally skip these tests. No need to launch a
         // child process for it.
         continue;
      } else {
         tests.push_back(join_string_list(nestedTests, "") + line);
      }
   }
   return tests;
}

TestList GoogleTest::getTestsInDirectory(std::shared_ptr<TestSuite> testSuite,
                                         const std::list<std::string> &pathInSuite,
                                         LitConfigPointer litConfig,
                                         TestingConfigPointer localConfig)
{
   TestList tests;
   std::string sourcePath = testSuite->getSourcePath(pathInSuite);
   for (const std::string &subDir : m_testSubDirs) {
      std::string dirPath = stdfs::path(sourcePath) / subDir;
      if (stdfs::is_directory(dirPath)) {
         continue;
      }
      for (const std::string &fn : listdir_files(dirPath, m_testSuffixes)) {
         // Discover the tests in this executable.
         std::string execPath = stdfs::path(sourcePath) / subDir / fn;
         std::list<std::string> testNames = getGTestTests(execPath, litConfig, localConfig);
         for (const std::string &testName : testNames) {
            std::list<std::string> testPath = pathInSuite;
            append_string_list(testPath, subDir);
            append_string_list(testPath, fn);
            append_string_list(testPath, testName);
            tests.push_back(std::make_shared<Test>(testSuite, testPath, localConfig, execPath));
         }
      }
   }
   return tests;
}

ResultPointer GoogleTest::execute(TestPointer test, LitConfigPointer litConfig)
{
   stdfs::path sourcePath = test->getSourcePath();
   std::string testPath = sourcePath.parent_path();
   std::string testName = sourcePath.filename();
   while (!stdfs::exists(testPath)) {
      // Handle GTest parametrized and typed tests, whose name includes
      // some '/'s.
      stdfs::path curPath = testPath;
      testPath = curPath.parent_path();
      std::string namePrefix = curPath.filename();
      testName = namePrefix + "/" + testName;
   }
   std::string cmd = testPath + "--gtest_filter=" + testName;
   if (litConfig->isUseValgrind()) {
      cmd = join_string_list(litConfig->getValgrindArgs(), " ") + cmd;
   }
   if (litConfig->isNoExecute()) {
      return std::make_shared<Result>(PASS, "");
   }
   try {
      RunCmdResponse runResponse = execute_command(cmd, std::nullopt, test->getConfig()->getEnvironment(),
                                                   std::nullopt, litConfig->getMaxIndividualTestTime());
      int exitCode = std::get<0>(runResponse);
      std::string output = std::get<1>(runResponse);
      std::string errorMsg = std::get<2>(runResponse);
      if (exitCode != 0) {
         return std::make_shared<Result>(FAIL, output + errorMsg);
      }
      std::string passingTestLine = "[  PASSED  ] 1 test.";
      if (output.find(passingTestLine) == std::string::npos) {
         std::string msg = format_string("Unable to find %s in gtest output:\n\n%s%s",
                                         passingTestLine.c_str(), output.c_str(), errorMsg.c_str());
         return std::make_shared<Result>(UNRESOLVED, msg);
      }
      return std::make_shared<Result>(PASS, "");
   } catch (ExecuteCommandTimeoutException &) {
      return std::make_shared<Result>(
         TIMEOUT, format_string("Reached timeout of %d seconds", litConfig->getMaxIndividualTestTime()));
   }
}

} // lit
} // polar
