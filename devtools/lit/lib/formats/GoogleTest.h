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

#ifndef POLAR_DEVLTOOLS_LIT_FORMATS_GOOGLETEST_H
#define POLAR_DEVLTOOLS_LIT_FORMATS_GOOGLETEST_H

#include "Base.h"
#include "../ForwardDefs.h"

namespace polar {
namespace lit {

class GoogleTest : public TestFormat
{
public:
   GoogleTest(const std::list<std::string> &googletestBins);
   std::list<std::string> getGTestTests(const std::string &path, LitConfigPointer litConfig,
                                        TestingConfigPointer localConfig);
   TestList getTestsInDirectory(std::shared_ptr<TestSuite> testSuite,
                                                        const std::list<std::string> &pathInSuite,
                                                        LitConfigPointer litConfig,
                                                        TestingConfigPointer localConfig) override;
   bool needSearchAgain() override;
   ResultPointer execute(TestPointer test, LitConfigPointer litConfig);
protected:
   std::list<std::string> m_testSubDirs;
   std::set<std::string> m_testSuffixes;
   std::list<std::string> m_googletestBins;
   bool m_searched;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_FORMATS_GOOGLETEST_H
