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

#ifndef POLAR_DEVLTOOLS_LIT_FORMATS_GOOGLETEST_H
#define POLAR_DEVLTOOLS_LIT_FORMATS_GOOGLETEST_H

#include "Base.h"
#include <set>

namespace polar {
namespace lit {

using TestList = std::shared_ptr<Test>;

class GoogleTest : public TestFormat
{
public:
   GoogleTest(const std::string &testSubDirs,
              const std::string &testSuffix);
   std::list<std::string> getGTestTests(const std::string &path, LitConfigPointer litConfig,
                                        TestingConfigPointer localConfig);
   TestList getTestsInDirectory(std::shared_ptr<TestSuite> testSuite,
                                                        const std::list<std::string> &pathInSuite,
                                                        LitConfigPointer litConfig,
                                                        TestingConfigPointer localConfig);
   ExecResultTuple execute(TestPointer test, LitConfigPointer litConfig);
protected:
   std::list<std::string> m_testSubDirs;
   std::set<std::string> m_testSuffixes;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_FORMATS_GOOGLETEST_H
