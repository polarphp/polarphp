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

#ifndef POLAR_DEVLTOOLS_LIT_RUN_H
#define POLAR_DEVLTOOLS_LIT_RUN_H

#include "Test.h"
#include "Semaphore.h"

namespace polar {
namespace lit {

class LitConfig;
class Run;
class TestingProgressDisplay;
using RunPointer = std::shared_ptr<Run>;
using LitConfigPointer = std::shared_ptr<LitConfig>;
using TestingProgressDisplayPointer = std::shared_ptr<TestingProgressDisplay>;

class Run
{
public:
   Run(LitConfigPointer litConfig, const TestList &tests);
   const TestList &getTests() const;
   TestList &getTests();
   void executeTest(TestPointer test);
   void executeTestsInPool(int jobs, int maxTime = -1);
   void executeTests(TestingProgressDisplayPointer display, size_t jobs, size_t maxTime = 0);
protected:
   void consumeTestResult(std::tuple<int, TestPointer> &poolResult);
protected:
   LitConfigPointer m_litConfig;
   TestList m_tests;
   TestingProgressDisplayPointer m_display;
   int m_failureCount;
   bool m_hitMaxFailures;
   std::map<std::string, Semaphore> m_parallelismSemaphores;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_RUN_H
