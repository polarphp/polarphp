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

namespace polar {
namespace lit {

class TestSuite;
class TestingConfig;
class LitConfig;
class Command;
class Test;

class TestFormat
{
public:
   virtual ~TestFormat() = 0;
   virtual std::list<Test> getTestsInDirectory(const TestSuite &testSuite, const std::list<std::string> &pathInSuite,
                                               const LitConfig &litConfig, const TestingConfig &config) = 0;
};

class FileBasedTest : public TestFormat
{
public:
   std::list<Test> getTestsInDirectory(const TestSuite &testSuite, const std::list<std::string> &pathInSuite,
                                       const LitConfig &litConfig, const TestingConfig &config);
};

class OneCommandPerFileTest : public TestFormat
{
public:
   OneCommandPerFileTest(const Command &command, const std::string &dir,
                         bool recursive = false,
                         const std::string &pattern = ".*",
                         bool useTempInput = false);
   void getTestsInDirectory();
   void createTempInput();
   void execute();
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_FORMATS_BASE_H
