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

#ifndef POLAR_DEVLTOOLS_LIT_FORMATS_BASE_H
#define POLAR_DEVLTOOLS_LIT_FORMATS_BASE_H

#include <string>
#include <list>
#include <memory>
#include <regex>

namespace polar {
namespace lit {

class TestSuite;
class TestingConfig;
class LitConfig;
class Command;
class Test;
class ResultCode;
class Result;

using LitConfigPointer = std::shared_ptr<LitConfig>;
using TestPointer = std::shared_ptr<Test>;
using ExecResultTuple = std::tuple<const ResultCode &, std::string>;

class TestFormat
{
public:
   virtual ~TestFormat() {}
   virtual std::list<std::shared_ptr<Test>> getTestsInDirectory(std::shared_ptr<TestSuite> testSuite,
                                                                const std::list<std::string> &pathInSuite,
                                                                LitConfigPointer litConfig,
                                                                std::shared_ptr<TestingConfig> localConfig) = 0;
   virtual std::tuple<const ResultCode &, std::string> execute(TestPointer test, LitConfigPointer litConfig)
   {}
};

class FileBasedTest : public TestFormat
{
public:
   std::list<std::shared_ptr<Test>> getTestsInDirectory(std::shared_ptr<TestSuite> testSuite,
                                                        const std::list<std::string> &pathInSuite,
                                                        LitConfigPointer litConfig,
                                                        std::shared_ptr<TestingConfig> localConfig);
};

class OneCommandPerFileTest : public TestFormat
{
public:
   OneCommandPerFileTest(const std::string &command, const std::string &dir,
                         bool recursive = false,
                         const std::string &pattern = ".*",
                         bool useTempInput = false);
   std::list<std::shared_ptr<Test>> getTestsInDirectory(std::shared_ptr<TestSuite> testSuite,
                                                        const std::list<std::string> &pathInSuite,
                                                        LitConfigPointer litConfig,
                                                        std::shared_ptr<TestingConfig> localConfig);
   void createTempInput(std::FILE *temp, std::shared_ptr<Test> test);
   std::tuple<const ResultCode &, std::string> execute(TestPointer test, LitConfigPointer litConfig);
protected:
   std::string m_command;
   std::string m_dir;
   bool m_recursive;
   std::regex m_pattern;
   bool m_useTempInput;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_FORMATS_BASE_H
