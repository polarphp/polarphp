// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/05.

#include "GoogleTest.h"
#include "../Utils.h"
#include "../ProcessUtils.h"
#include "../LitConfig.h"
#include "../TestingConfig.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/FileUtils.h"
#include "polarphp/utils/Program.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "../Test.h"
#include <iostream>
#include <sstream>

using polar::basic::ArrayRef;
using polar::basic::SmallVector;
using polar::basic::SmallString;
using polar::basic::Twine;
using polar::utils::MemoryBuffer;
using polar::fs::create_temporary_file;
using polar::fs::FileRemover;

#define TESTRUNNER_GTEST_FORMAT_PROCESS_TEMP_PREFIX "polarphp-lit-gtest-format-"

namespace polar {
namespace lit {

GoogleTest::GoogleTest(const std::list<std::string> &googletestBins)
   : m_googletestBins(googletestBins),
     m_searched(false)
{
}

namespace {
inline bool search_in_string_list(std::function<bool(StringRef)> predictor,
                                  const std::list<std::string> &strList)
{
   for (StringRef str : strList) {
      if (predictor(str)) {
         return true;
      }
   }
   return false;
}

inline bool is_start_with(StringRef str)
{
   return str.startsWith("DISABLED_");
}

inline std::list<std::string> &append_string_list(std::list<std::string> &list, StringRef str)
{
   list.push_back(str.getStr());
   return list;
}
inline std::list<std::string> append_string_list_copy(std::list<std::string> list, StringRef str)
{
   list.push_back(str.getStr());
   return list;
}
} // anonymous namespace

std::list<std::string> GoogleTest::getGTestTests(const std::string &path, LitConfigPointer litConfig,
                                                 TestingConfigPointer localConfig)
{
   ArrayRef<StringRef> args{
      path,
            "--gtest_list_tests"
   };
   SmallVector<StringRef, 10> envsRef;
   for (const std::string &envStr : localConfig->getEnvironment()) {
      envsRef.pushBack(envStr);
   }

   std::string errorMsg;
   bool execFailed;
   SmallString<32> outTempFilename;
   fs::create_temporary_file(TESTRUNNER_GTEST_FORMAT_PROCESS_TEMP_PREFIX, "", outTempFilename);
   SmallString<32> errorTempFilename;
   fs::create_temporary_file(TESTRUNNER_GTEST_FORMAT_PROCESS_TEMP_PREFIX, "", errorTempFilename);
   FileRemover outTempRemover(outTempFilename);
   FileRemover errTempRemover(errorTempFilename);
   ArrayRef<std::optional<StringRef>> redirects{
      std::nullopt,
            outTempFilename,
            errorTempFilename
   };
   int exitCode = polar::sys::execute_and_wait(path, args, std::nullopt, envsRef,
                                               redirects, 0, 0, &errorMsg, &execFailed);
   if(execFailed) {
      throw ValueError(format_string("Could not create process (%s) due to %s",
                                     path.c_str(), errorMsg.c_str()));
   }

   if (0 != exitCode) {
      std::string errOutput;
      auto errorMsgBuffer = MemoryBuffer::getFile(errorTempFilename);
      if (errorMsgBuffer) {
         errOutput = errorMsgBuffer.get()->getBuffer().getStr();
      } else {
         // here get the buffer info error
         throw std::runtime_error(format_string("get error output buffer error: %s",
                                                errorMsgBuffer.getError().message()));
      }

      litConfig->warning(
               format_string("unable to discover google-tests in %s. Process output: %s",
                             path.c_str(), errOutput.c_str()));
      throw std::runtime_error(format_string("unable to discover google-tests in %s. Process output: %s",
                                             path.c_str(), errOutput.c_str()));
   }

   std::string output;
   // here we can get the error message or output message
   auto outputBuf = MemoryBuffer::getFile(outTempFilename);
   if (outputBuf) {
      output = outputBuf.get()->getBuffer().getStr();
   } else {
      // here get the buffer info error
      throw std::runtime_error(format_string("get output buffer error: %s",
                                             outputBuf.getError().message()));
   }
   std::istringstream stream(output);
   char lineBuf[1024];
   std::list<std::string> nestedTests;
   std::list<std::string> tests;
   while (stream.getline(lineBuf, sizeof(lineBuf))) {
      StringRef lineRef(lineBuf);
      if (lineRef.empty()) {
         continue;
      }
      if (lineRef.find("Running main() from") != StringRef::npos) {
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
      SmallVector<StringRef, 3> parts;
      lineRef.split(parts, '#', 1);
      lineRef = parts[0].rtrim();
      if (lineRef.ltrim().empty()) {
         continue;
      }
      size_t index = 0;
      while (lineRef.substr(index * 2, 2) == "  ") {
         index += 1;
      }
      while (nestedTests.size() > index) {
         nestedTests.pop_back();
      }
      lineRef = lineRef.substr(index * 2);
      if (lineRef.endsWith(".")) {
         nestedTests.push_back(lineRef);
      } else if (search_in_string_list(is_start_with, append_string_list_copy(nestedTests, lineRef))) {
         // Gtest will internally skip these tests. No need to launch a
         // child process for it.
         continue;
      } else {
         tests.push_back(polar::basic::join(nestedTests.begin(), nestedTests.end(), StringRef("")) + lineRef.getStr());
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
   size_t testNameStartPos = StringRef(UNITEST_BIN_DIR).size() + 1;
   for (const std::string &gtestBin : m_googletestBins) {
      std::string gtestBaseName = gtestBin.substr(testNameStartPos);
      std::list<std::string> testNames = getGTestTests(gtestBin, litConfig, localConfig);
      for (const std::string &testName : testNames) {
         std::list<std::string> testPath = pathInSuite;
         testPath.push_back(gtestBaseName);
         testPath.push_back(testName);
         tests.push_back(std::make_shared<Test>(testSuite, testPath, localConfig, gtestBin));
      }
   }
   m_searched = true;
   return tests;
}

bool GoogleTest::needSearchAgain()
{
   return !m_searched;
}

ResultPointer GoogleTest::execute(TestPointer test, LitConfigPointer litConfig)
{
   std::string sourceRoot = test->getTestSuite()->getSourcePath();
   std::string sourcePath = test->getSourcePath().substr(sourceRoot.size() + 1);
   size_t splitPos = sourcePath.find('/');
   std::string testName = sourcePath;
   if (splitPos != std::string::npos) {
      testName = sourcePath.substr(splitPos + 1);
   }
   std::string executable = test->getFilePath();
   if (!stdfs::exists(executable)) {
      return std::make_shared<Result>(UNRESOLVED, format_string("executable: %s is not exist", executable.c_str()));
   }
   SmallVector<StringRef, 10> args;
   if (litConfig->isUseValgrind()) {
      for (const std::string &arg : litConfig->getValgrindArgs()) {
         args.push_back(arg);
      }
   }
   args.push_back(executable);
   std::string testFilter = "--gtest_filter=" + testName;
   args.push_back(testFilter);
   if (litConfig->isNoExecute()) {
      return std::make_shared<Result>(PASS, "");
   }
   SmallVector<StringRef, 10> envsRef;
   for (const std::string &envStr : test->getConfig()->getEnvironment()) {
      envsRef.pushBack(envStr);
   }

   std::string errorMsg;
   bool execFailed;
   SmallString<32> outTempFilename;
   fs::create_temporary_file(TESTRUNNER_GTEST_FORMAT_PROCESS_TEMP_PREFIX, "", outTempFilename);
   SmallString<32> errorTempFilename;
   fs::create_temporary_file(TESTRUNNER_GTEST_FORMAT_PROCESS_TEMP_PREFIX, "", errorTempFilename);
   FileRemover outTempRemover(outTempFilename);
   FileRemover errTempRemover(errorTempFilename);
   ArrayRef<std::optional<StringRef>> redirects{
      std::nullopt,
            outTempFilename,
            errorTempFilename
   };

   int exitCode = polar::lit::execute_and_wait(executable, args, std::nullopt, envsRef,
                                               redirects, litConfig->getMaxIndividualTestTime(), 0,
                                               &errorMsg, &execFailed);
   if(execFailed) {
      throw ValueError(format_string("Could not create process (%s) due to %s",
                                     executable.c_str(), errorMsg.c_str()));
   }

   if (0 != exitCode) {
      std::string errOutput;
      auto errorMsgBuffer = MemoryBuffer::getFile(errorTempFilename);
      if (errorMsgBuffer) {
         errOutput = errorMsgBuffer.get()->getBuffer().getStr();
      } else {
         // here get the buffer info error
         throw std::runtime_error(format_string("get error output buffer error: %s",
                                                errorMsgBuffer.getError().message()));
      }
      if (-2 == exitCode) {
         return std::make_shared<Result>(TIMEOUT, errorMsg);
      }
      return std::make_shared<Result>(FAIL, errOutput);
   }
   std::string output;
   auto outputBuf = MemoryBuffer::getFile(outTempFilename);
   if (outputBuf) {
      output = outputBuf.get()->getBuffer().getStr();
   } else {
      // here get the buffer info error
      throw std::runtime_error(format_string("get output buffer error: %s",
                                             outputBuf.getError().message()));
   }
   StringRef outputRef(output);
   StringRef passingTestLine("[  PASSED  ] 1 test.");
   if (outputRef.find(passingTestLine) == StringRef::npos) {
      std::string msg = Twine("Unable to find ",  passingTestLine)
            .concat(" in gtest output:\n\n")
            .concat(output)
            .concat(errorMsg).getStr();
      return std::make_shared<Result>(UNRESOLVED, msg);
   }
   return std::make_shared<Result>(PASS, "");
}

} // lit
} // polar
