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

#include "Run.h"
#include "Utils.h"
#include "Test.h"
#include "LitConfig.h"
#include "ProgressBar.h"
#include <iostream>
#include <tuple>

namespace polar {
namespace lit {

std::tuple<int, TestPointer> worker_run_one_test(int testIndex, TestPointer test);

Run::Run(LitConfigPointer litConfig, const TestList &tests)
   : m_litConfig(litConfig),
     m_tests(tests)
{
}

const TestList &Run::getTests() const
{
   return m_tests;
}

TestList &Run::getTests()
{
   return m_tests;
}

// execute_tests(display, jobs, [max_time])
//
// Execute each of the tests in the run, using up to jobs number of
// parallel tasks, and inform the display of each individual result. The
// provided tests should be a subset of the tests available in this run
// object.
//
// If max_time is non-None, it should be a time in seconds after which to
// stop executing tests.
//
// The display object will have its update method called with each test as
// it is completed. The calls are guaranteed to be locked with respect to
// one another, but are *not* guaranteed to be called on the same thread as
// this method was invoked on.
//
// Upon completion, each test in the run will have its result
// computed. Tests which were not actually executed (for any reason) will
// be given an UNRESOLVED result.
void Run::executeTests(TestingProgressDisplayPointer display, size_t jobs, size_t maxTime)
{
   // Don't do anything if we aren't going to run any tests.
   if (m_tests.empty() || jobs == 0) {
      return;
   }
   // Save the display object on the runner so that we can update it from
   // our task completion callback.
   m_display = display;
   m_failureCount = 0;
   m_hitMaxFailures = false;
   if (m_litConfig->isSingleProcess()) {
      int index = 0;
      for (auto test : m_tests) {
         std::tuple<int, TestPointer> result = worker_run_one_test(index, test);
         consumeTestResult(result);
         ++index;
      }
   }
   // Mark any tests that weren't run as UNRESOLVED.
   for (auto test : m_tests) {
      if (!test->getResult()) {
         test->setResult(std::make_shared<Result>(UNRESOLVED, "", 0.0));
      }
   }
}

// Test completion callback for worker_run_one_test
//
// Updates the test result status in the parent process. Each task in the
// pool returns the test index and the result, and we use the index to look
// up the original test object. Also updates the progress bar as tasks
// complete.
void Run::consumeTestResult(std::tuple<int, TestPointer> &poolResult)
{
   // Don't add any more test results after we've hit the maximum failure
   // count.  Otherwise we're racing with the main thread, which is going
   // to terminate the process pool soon.
   if (m_hitMaxFailures) {
      return;
   }
   int testIndex = std::get<0>(poolResult);
   TestPointer testWithResult = std::get<1>(poolResult);
   auto testIter = m_tests.begin();
   std::advance(testIter, testIndex);
   // Update the parent process copy of the test. This includes the result,
   // XFAILS, REQUIRES, and UNSUPPORTED statuses.
   // parent and child disagree on test path
   assert((*testIter)->getFilePath() == testWithResult->getFilePath());
   *testIter = testWithResult;
   m_display->update(testWithResult);
   // If we've finished all the tests or too many tests have failed, notify
   // the main thread that we've stopped testing.
   m_failureCount += testWithResult->getResult()->getCode() == FAIL;
   if (m_litConfig->getMaxFailures() && m_failureCount == m_litConfig->getMaxFailures().value()) {
      m_hitMaxFailures = true;
   }
}

/// Run one test in a multiprocessing.Pool
///
/// Side effects in this function and functions it calls are not visible in the
/// main lit process.
///
/// Arguments and results of this function are pickled, so they should be cheap
/// to copy. For efficiency, we copy all data needed to execute all tests into
/// each worker and store it in the child_* global variables. This reduces the
/// cost of each task.
///
/// Returns an index and a Result, which the parent process uses to update
/// the display.
std::tuple<int, TestPointer> worker_run_one_test(int testIndex, TestPointer test)
{
   test->setResult(std::make_shared<Result>(PASS, "pass the test", rand() % 15));
   return std::make_tuple(testIndex, test);
}

} // lit
} // polar


