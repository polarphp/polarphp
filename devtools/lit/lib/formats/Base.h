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

#ifndef POLAR_DEVLTOOLS_LIT_FORMATS_BASE_H
#define POLAR_DEVLTOOLS_LIT_FORMATS_BASE_H

#include "../ForwardDefs.h"

#include <string>
#include <list>
#include <memory>
#include <boost/regex.hpp>

namespace polar {
namespace lit {

class TestFormat
{
public:
   virtual ~TestFormat() {}
   virtual std::list<std::shared_ptr<Test>> getTestsInDirectory(std::shared_ptr<TestSuite> testSuite,
                                                                const std::list<std::string> &pathInSuite,
                                                                LitConfigPointer litConfig,
                                                                TestingConfigPointer localConfig) = 0;
   virtual bool needSearchAgain();
   virtual ResultPointer execute(TestPointer test, LitConfigPointer litConfig) = 0;
};

class FileBasedTest : public TestFormat
{
public:
   TestList getTestsInDirectory(std::shared_ptr<TestSuite> testSuite,
                                const std::list<std::string> &pathInSuite,
                                LitConfigPointer litConfig,
                                TestingConfigPointer localConfig);
};

class OneCommandPerFileTest : public TestFormat
{
public:
   OneCommandPerFileTest(const std::string &command, const std::string &dir,
                         bool recursive = false,
                         const std::string &pattern = ".*",
                         bool useTempInput = false);
   TestList getTestsInDirectory(std::shared_ptr<TestSuite> testSuite,
                                const std::list<std::string> &pathInSuite,
                                LitConfigPointer litConfig,
                                TestingConfigPointer localConfig);
   void createTempInput(std::FILE *temp, std::shared_ptr<Test> test);
   ResultPointer execute(TestPointer test, LitConfigPointer litConfig);
protected:
   std::string m_command;
   std::string m_dir;
   bool m_recursive;
   boost::regex m_pattern;
   bool m_useTempInput;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_FORMATS_BASE_H
