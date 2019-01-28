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

#ifndef POLAR_DEVLTOOLS_LIT_RUN_H
#define POLAR_DEVLTOOLS_LIT_RUN_H

#include "Test.h"
#include "Semaphore.h"
#include "threadpool/ThreadPool.h"
#include "ForwardDefs.h"
#include <memory>
#include <atomic>

namespace polar {
namespace lit {

using ThreadPoolPointer = std::shared_ptr<threadpool::ThreadPool>;

class Run
{
public:
   Run(LitConfigPointer litConfig, const TestList &tests);
   const TestList &getTests() const;
   TestList &getTests();
   void executeTest(TestPointer test);
   void executeTestsInPool(size_t jobs, size_t maxTime = 0);
   void executeTests(TestingProgressDisplayPointer display, size_t jobs, size_t maxTime = 0);
   void consumeTestResult(std::tuple<int, TestPointer> &poolResult);
   std::atomic_bool m_hitMaxFailures;
protected:
   int m_failureCount;
   LitConfigPointer m_litConfig;
   TestList m_tests;
   TestingProgressDisplayPointer m_display;
   std::map<std::string, Semaphore> m_parallelismSemaphores;
   ThreadPoolPointer m_threadPool;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_RUN_H
