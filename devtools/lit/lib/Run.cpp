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

#include "Run.h"
#include "Utils.h"
#include "Test.h"
#include "LitConfig.h"
#include "ProgressBar.h"
#include "formats/Base.h"
#include "ForwardDefs.h"
#include "polarphp/basic/adt/StringRef.h"
#include <tuple>
#include <string>
#include <chrono>
#include <future>
#include <functional>
#include <list>
#include <mutex>

namespace polar {
namespace lit {

using threadpool::ThreadPool;
using threadpool::ThreadPoolOptions;
using TaskType = std::packaged_task<void()>;

LitConfig *sg_litCfg = nullptr;

void worker_run_one_test(int testIndex, TestPointer test,
                         LitConfigPointer litConfig,
                         Run &run, std::map<std::string,Semaphore> &parallelismSemaphores);

void do_execute_test(TestPointer test, LitConfigPointer litConfig, std::map<std::string,
                     Semaphore> &parallelismSemaphores);

Run::Run(LitConfigPointer litConfig, const TestList &tests)
   : m_litConfig(litConfig),
     m_tests(tests),
     m_threadPool(nullptr)
{
   sg_litCfg = litConfig.get();
   for (auto &item: m_litConfig->getParallelismGroups()) {
      m_parallelismSemaphores.emplace(item.first, item.second);
   }
}

const TestList &Run::getTests() const
{
   return m_tests;
}

TestList &Run::getTests()
{
   return m_tests;
}

void Run::executeTest(TestPointer test)
{
   do_execute_test(test, m_litConfig, m_parallelismSemaphores);
}

void Run::executeTestsInPool(size_t jobs, size_t maxTime)
{
   // We need to issue many wait calls, so compute the final deadline and
   // subtract time.time() from that as we go along.
   bool hasDeadline = false;
   std::chrono::time_point deadline = std::chrono::system_clock::now();
   if (maxTime > 0) {
      deadline += std::chrono::seconds(maxTime);
      hasDeadline = true;
   }
   // start a thread pool
   ThreadPoolOptions threadOpts;
   threadOpts.setThreadCount(jobs);
   m_threadPool.reset(new ThreadPool(threadOpts));
   std::list<std::shared_ptr<TaskType>> tasks;
   // @TODO port to WINDOWS
   std::list<std::future<void>> asyncResults;
   int testIndex = 0;
   for (TestPointer test: m_tests) {
      std::shared_ptr<TaskType> task = std::make_shared<TaskType>(std::bind(worker_run_one_test, testIndex, test, m_litConfig,
                                                                            std::ref(*this), std::ref(m_parallelismSemaphores)));
      tasks.push_back(task);
      asyncResults.push_back(task->get_future());
      m_threadPool->post(*task.get());
      ++testIndex;
   }
   try {
      for (std::future<void> &future: asyncResults) {
         if (hasDeadline) {
            future.wait_until(deadline);
         } else {
            future.wait();
         }
         future.get();
         if (m_hitMaxFailures) {
            break;
         }
      }
   } catch (std::exception &exp) {
      std::cerr << "catch exception from test packaged_task, error: " << exp.what() << std::endl;
      m_threadPool->terminate();
      throw;
   }
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
         worker_run_one_test(index, test, m_litConfig, *this, m_parallelismSemaphores);
         ++index;
      }
   } else {
      executeTestsInPool(jobs, maxTime);
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
   int testIndex = std::get<0>(poolResult);
   TestPointer testWithResult = std::get<1>(poolResult);
   // Don't add any more test results after we've hit the maximum failure
   // count.  Otherwise we're racing with the main thread, which is going
   // to terminate the process pool soon.
   if (m_hitMaxFailures.load()) {
      ResultPointer result = testWithResult->getResult();
      if (result) {
         result->setCode(UNRESOLVED);
      }
      return;
   }
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
      m_hitMaxFailures.store(true);
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
std::mutex sg_workerMutex;
void worker_run_one_test(int testIndex, TestPointer test, LitConfigPointer litConfig,
                         Run& run, std::map<std::string, Semaphore> &parallelismSemaphores)
{
   std::lock_guard locker(sg_workerMutex);
   if (run.m_hitMaxFailures.load()) {
      return;
   }
   do_execute_test(test, litConfig, parallelismSemaphores);
   std::tuple<int, TestPointer> result = std::make_tuple(testIndex, test);
   run.consumeTestResult(result);
}

namespace {

class SemaphoreReleaser
{
public:
   SemaphoreReleaser(Semaphore *semaphore = nullptr)
      : m_semaphore(semaphore)
   {
   }

   SemaphoreReleaser &assign(Semaphore *semaphore)
   {
      m_semaphore = semaphore;
      return *this;
   }

   ~SemaphoreReleaser()
   {
      if (m_semaphore != nullptr) {
         m_semaphore->notify();
      }
   }
protected:
   Semaphore *m_semaphore;
};

} // anonymous namespace

void do_execute_test(TestPointer test, LitConfigPointer litConfig,
                     std::map<std::string, Semaphore> &parallelismSemaphores)
{
   std::any &pg = test->getConfig()->getParallelismGroup();
   std::string pgName;
   if (pg.has_value()) {
      if (pg.type() == typeid(ParallelismGroupSetter)) {
         pgName = std::any_cast<ParallelismGroupSetter>(pg)(test);
      } else if (pg.type() == typeid(std::string)) {
         pgName = std::any_cast<std::string>(pg);
      }
   }
   ResultPointer result;
   try {
      SemaphoreReleaser pgReleaser;
      Semaphore *semaphore = nullptr;
      if (!pgName.empty() && parallelismSemaphores.find(pgName) != parallelismSemaphores.end()) {
         semaphore = &parallelismSemaphores.at(pgName);
      }
      if (semaphore != nullptr) {
         pgReleaser.assign(semaphore);
         semaphore->wait();
      }
      std::chrono::time_point startTime = std::chrono::system_clock::now();
      TestFormatPointer testFormatter = test->getConfig()->getTestFormat();
      // here we need setting the shtest format as the default formatter ?
      if (!testFormatter) {
         throw ValueError("Test Format is not settings");
      }
      result = testFormatter->execute(test, litConfig);
      result->setElapsed(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - startTime).count());
   } catch (std::exception &exp) {
      if (litConfig->isDebug()) {
         throw;
      }
      std::string output = "Exception during script execution:\n";
      output += exp.what();
      output += "\n";
      result = std::make_shared<Result>(UNRESOLVED, output);
   }
   test->setResult(result);
}
} // lit
} // polar


