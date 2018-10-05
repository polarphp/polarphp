// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/27.

#include "CLI/CLI.hpp"
#include "Config.h"
#include "lib/Utils.h"
#include "lib/LitConfig.h"
#include "lib/Discovery.h"
#include "lib/Test.h"
#include "lib/Run.h"
#include "lib/ProgressBar.h"
#include <iostream>
#include <thread>
#include <assert.h>
#include <filesystem>
#include <list>
#include <regex>
#include <vector>
#include <random>
#include <algorithm>
#include <iterator>
#include <chrono>

using polar::lit::LitConfigPointer;
using polar::lit::LitConfig;
using polar::lit::Test;
using polar::lit::TestSuite;
using polar::lit::TestSuitePointer;
using polar::lit::Result;
using polar::lit::ResultPointer;
using polar::lit::TestPointer;
using polar::lit::ResultCode;
using polar::lit::TestList;
using polar::lit::ProgressBar;
using polar::lit::TerminalController;
using polar::lit::SimpleProgressBar;
using polar::lit::AbstractProgressBar;
using polar::lit::format_string;
namespace fs = std::filesystem;

namespace {

std::list<std::string> vector_to_list(const std::vector<std::string> &items)
{
   std::list<std::string> ret;
   for (const std::string &item : items) {
      ret.push_back(item);
   }
   return ret;
}

} // anonymous namespace

void general_exception_handler(std::exception_ptr eptr)
{
   try {
      if (eptr) {
         std::rethrow_exception(eptr);
      }
   } catch (const std::exception &exp) {
      std::cerr << exp.what() << std::endl;
      exit(1);
   }
}

void update_incremental_cache(TestPointer test)
{
   if (!test->getResult()->getCode().isFailure()) {
      return;
   }
   polar::lit::modify_file_utime_and_atime(test->getFilePath());
}

void sort_by_incremental_cache(polar::lit::Run &run)
{
   run.getTests().sort([](TestPointer &lhs, TestPointer &rhs) {
      fs::path lhsPath(lhs->getFilePath());
      fs::path rhsPath(rhs->getFilePath());
      return fs::last_write_time(lhsPath) > fs::last_write_time(rhsPath);
   });
}

class TestingProgressDisplay
{
public:
   TestingProgressDisplay(const CLI::App &opts, int numTests, std::shared_ptr<AbstractProgressBar> progressBar = nullptr)
      : m_opts(opts),
        m_numTests(numTests),
        m_progressBar(progressBar),
        m_completed(0)
   {
      m_showAllOutput = m_opts.get_option("-a,--show-all")->count() > 0 ? true : false;
      m_incremental = m_opts.get_option("-i, --incremental")->count() > 0 ? true : false;
      m_quiet = m_opts.get_option("-q,--quiet")->count() > 0 ? true : false;
      m_succinct = m_opts.get_option("-s,--succinct")->count() > 0 ? true : false;
      m_showOutput = m_opts.get_option("-v,--verbose")->count() > 0 ? true : false;
   }

   void finish()
   {
      if (m_progressBar) {
         m_progressBar->clear();
      } else if (m_quiet) {
         // TODO
      } else if (m_succinct) {
         std::printf("\n");
      }
   }

   void update(TestPointer test)
   {
      m_completed += 1;
      if (m_incremental) {
         update_incremental_cache(test);
      }
      if (m_progressBar) {
         m_progressBar->update(m_completed / m_numTests, test->getFullName());
      }
      bool shouldShow = test->getResult()->getCode().isFailure() ||
            m_showAllOutput ||
            (!m_quiet && !m_succinct);

      if (!shouldShow) {
         return;
      }
      if (m_progressBar) {
         m_progressBar->clear();
      }
      // Show the test result line.
      std::string testName = test->getFullName();
      ResultPointer testResult = test->getResult();
      const ResultCode &resultCode = testResult->getCode();
      std::printf("%s: %s (%d of %d)\n", resultCode.getName().c_str(),
                  testName.c_str(), m_completed, m_numTests);
      // Show the test failure output, if requested.
      if ((resultCode.isFailure() && m_showOutput) ||
          m_showAllOutput) {
         if (resultCode.isFailure()) {
            std::printf("%s TEST '%s' FAILED %s\n", std::string('*', 20).c_str(),
                        test->getFullName().c_str(), std::string('*', 20).c_str());
         }
         std::printf("%s\n", testResult->getOutput().c_str());
         std::printf("%s\n", std::string('*', 20).c_str());
      }
      // Report test metrics, if present.
      if (!testResult->getMetrics().empty()) {
         // @TODO sort the metrics
         std::printf("%s TEST '%s' RESULTS %s\n", std::string('*', 10).c_str(),
                     test->getFullName().c_str(),
                     std::string('*', 10).c_str());
         for (auto &item : testResult->getMetrics()) {
            std::printf("%s: %s \n", item.first.c_str(), item.second->format());
         }
         std::printf("%s\n", std::string('*', 10).c_str());
      }
      // Report micro-tests, if present
      if (!testResult->getMicroResults().empty()) {
         // @TODO sort the MicroResults
         for (auto &item : testResult->getMicroResults()) {
            std::printf("%s MICRO-TEST: %s\n", std::string('*', 3).c_str(), item.first.c_str());
            ResultPointer microTest = item.second;
            if (!microTest->getMetrics().empty()) {
               // @TODO sort the metrics
               for (auto &microItem : microTest->getMetrics()) {
                  std::printf("    %s:  %s \n", microItem.first.c_str(), microItem.second->format());
               }
            }
         }
      }
      // Ensure the output is flushed.
      std::fflush(stdout);
   }
private:
   const CLI::App &m_opts;
   int m_numTests;
   std::shared_ptr<AbstractProgressBar> m_progressBar;
   int m_completed;
   bool m_quiet;
   bool m_succinct;
   bool m_showAllOutput;
   bool m_incremental;
   bool m_showOutput;
};

int main(int argc, char *argv[])
{
   CLI::App litApp;
   std::vector<std::string> testPaths;
   bool showVersion;
   size_t threadNumbers;
   std::string cfgPrefix;
   std::vector<std::string> params;
   std::string cfgSetterPluginDir;
   CLI::Option *testPathsOpt = litApp.add_option("test_paths", testPaths, "Files or paths to include in the test suite");
   litApp.add_flag("--version", showVersion, "Show version and exit");
   CLI::Option *threadsOpt = litApp.add_option("-j,--threads", threadNumbers, "Number of testing threads");
   litApp.add_option("--config-prefix", cfgPrefix, "Prefix for 'lit' config files");
   litApp.add_option("-D,--param", params, "Add 'NAME' = 'VAL' to the user defined parameters");
   litApp.add_option("--cfg-setter-plugin-dir", cfgSetterPluginDir, "the cfg setter plugin base dir");

   /// setup command group
   /// Output Format
   bool quiet;
   bool succinct;
   int showOutput;
   bool showAll;
   std::string outputDir;
   bool displayProgressBar;
   bool showUnsupported;
   bool showXFail;
   bool echoAllCommands;
   litApp.add_flag("-q,--quiet", quiet, "Suppress no error output")->group("Output Format");
   litApp.add_flag("-s,--succinct", succinct, "Reduce amount of output")->group("Output Format");
   litApp.add_flag("-v,--verbose", showOutput, "Show test output for failures")->group("Output Format");
   litApp.add_option("--echo-all-commands", echoAllCommands, "Echo all commands as they are executed to stdout."
                                                             "In case of failure, last command shown will be the failing one.")->group("Output Format");
   litApp.add_flag("-a,--show-all", showAll, "Display all commandlines and output")->group("Output Format");
   litApp.add_option("-o,--output", outputDir, "Write test results to the provided path")->check(CLI::ExistingDirectory)->group("Output Format");
   litApp.add_flag("--display-progress-bar", displayProgressBar, "use curses based progress bar")->group("Output Format");
   litApp.add_flag("--show-unsupported", showUnsupported, "Show unsupported tests")->group("Output Format");
   litApp.add_flag("--show-xfail", showXFail, "Show tests that were expected to fail")->group("Output Format");

   /// Test Execution
   std::vector<std::string> paths;
   bool useValgrind;
   bool valgrindLeakCheck;
   std::vector<std::string> valgrindArgs;
   bool timeTests;
   bool noExecute;
   std::string xunitOutputFile;
   int maxIndividualTestTime;
   int maxFailures;
   litApp.add_option("--path", paths, "Additional paths to add to testing environment")->group("Test Execution");
   litApp.add_flag("--vg", useValgrind, "Run tests under valgrind")->group("Test Execution");
   litApp.add_flag("--vg-leak", valgrindLeakCheck, "Check for memory leaks under valgrind")->group("Test Execution");
   litApp.add_option("--vg-arg", valgrindArgs, "Check for memory leaks under valgrind")->group("Test Execution");
   litApp.add_flag("--time-tests", timeTests, "Track elapsed wall time for each test")->group("Test Execution");
   litApp.add_flag("--no-execute", noExecute, "Don't execute any tests (assume PASS)")->group("Test Execution");
   litApp.add_option("--xunit-xml-output", xunitOutputFile, "Write XUnit-compatible XML test reports to the  specified file")->group("Test Execution");
   CLI::Option *maxIndividualTestTimeOpt = litApp.add_option("--timeout", maxIndividualTestTime, "Maximum time to spend running a single test (in seconds)."
                                                                                                 "0 means no time limit. [Default: 0]", 0)->group("Test Execution");
   CLI::Option *maxFailuresOpt = litApp.add_option("--max-failures", maxFailures, "Stop execution after the given number of failures.", 0)->group("Test Execution");

   /// Test Selection
   size_t maxTests;
   float maxTime;
   bool shuffle;
   bool incremental;
   std::string filter;
   int numShards;
   int runShard;
   CLI::Option *maxTestsOpt = litApp.add_option("--max-tests", maxTests, "Maximum number of tests to run")->group("Test Selection");
   litApp.add_option("--max-time", maxTime, "Maximum time to spend testing (in seconds)")->group("Test Selection");
   litApp.add_flag("--shuffle", shuffle, "Run tests in random order")->group("Test Selection");
   litApp.add_flag("-i, --incremental", incremental, "Run modified and failing tests first (updates "
                                                     "mtimes)")->group("Test Selection");
   litApp.add_option("--filter", filter, "Only run tests with paths matching the given "
                                         "regular expression")->envname("LIT_FILTER")->group("Test Selection");
   CLI::Option *numShardsOpt = litApp.add_option("--num-shards", numShards, "Split testsuite into M pieces and only run one")->envname("LIT_NUM_SHARDS")->group("Test Selection");
   CLI::Option *runShardOpt = litApp.add_option("--run-shard", runShard, "Run shard #N of the testsuite")->envname("LIT_RUN_SHARD")->group("Test Selection");

   /// debug
   bool debug;
   bool showSuites;
   bool showTests;
   bool singleProcess;
   litApp.add_flag("--debug", debug, "Enable debugging (for 'lit' development)")->group("Debug and Experimental Options");
   litApp.add_flag("--show-suites", showSuites, "Show discovered test suites")->group("Debug and Experimental Options");
   litApp.add_flag("--show-tests", showTests, "Show all discovered tests")->group("Debug and Experimental Options");
   litApp.add_flag("--single-process", singleProcess, "Don't run tests in parallel.  Intended for debugging "
                                                      "single test failures")->group("Debug and Experimental Options");
   CLI11_PARSE(litApp, argc, argv);

   std::exception_ptr eptr;
   try {
      if (showVersion) {
         std::cout << "lit " << POLAR_LIT_VERSION << std::endl;
         return 0;
      }
      if (testPaths.empty()) {
         std::cerr << "No inputs specified" << std::endl;
         std::cout << litApp.help() << std::endl;
      }
      if (threadsOpt->empty()) {
         threadNumbers = std::thread::hardware_concurrency();
      }
      if (!maxFailuresOpt->empty() && maxFailures == 0) {
         std::cerr << "Setting --max-failures to 0 does not have any effect." << std::endl;
      }
      if (cfgSetterPluginDir.empty()) {
         cfgSetterPluginDir = POLAR_LIT_RUNTIME_DIR;
      }
      if (echoAllCommands) {
         showOutput = true;
      }
      atexit(polar::lit::temp_files_clear_handler);
      std::list<std::string> inputs(vector_to_list(testPaths));
      // Create the user defined parameters.
      std::map<std::string, std::any> userParams;
      for(std::string &item : params) {
         if (item.find("=") == std::string::npos) {
            userParams[item] = "";
         } else {
            std::list<std::string> parts = polar::lit::split_string(item, '=', 1);
            auto iter = parts.begin();
            assert(parts.size() == 2);
            const std::string &name = *iter++;
            const std::string &value = *iter++;
            userParams[name] = value;
         }
      }
      // Decide what the requested maximum indvidual test time should be
      if (maxIndividualTestTimeOpt->empty()) {
         maxIndividualTestTime = 0;
      }
      // Create the global config object.
      LitConfigPointer litConfig = std::make_shared<LitConfig>(
               fs::path(argv[0]).filename(),
            vector_to_list(paths),
            quiet,
            useValgrind,
            valgrindLeakCheck,
            vector_to_list(valgrindArgs),
            noExecute,
            singleProcess,
            debug,
      #ifdef POLAR_OS_WIN32
            true,
      #else
            false,
      #endif
            userParams,
            cfgSetterPluginDir,
            (!cfgPrefix.empty() ? std::optional(cfgPrefix) : std::nullopt),
            maxIndividualTestTime,
            maxFailures,
            std::map<std::string, std::string>{},
      echoAllCommands
            );
      // Perform test discovery.
      polar::lit::Run run(litConfig,
                          polar::lit::find_tests_for_inputs(litConfig, inputs));
      // After test discovery the configuration might have changed
      // the maxIndividualTestTime. If we explicitly set this on the
      // command line then override what was set in the test configuration
      if (!maxIndividualTestTimeOpt->empty()) {
         if (maxIndividualTestTime != litConfig->getMaxIndividualTestTime()) {
            litConfig->note(format_string("The test suite configuration requested an individual"
                                          " test timeout of %d seconds but a timeout of %d seconds was"
                                          " requested on the command line. Forcing timeout to be %d"
                                          " seconds", litConfig->getMaxIndividualTestTime(),
                                          maxIndividualTestTime));
            litConfig->setMaxIndividualTestTime(maxIndividualTestTime);
         }
      }
      TestList &tests = run.getTests();
      if (showSuites || showTests) {
         // Aggregate the tests by suite.
         std::map<int, TestList> suitesAndTests;
         std::map<int, TestSuitePointer> testsuiteMap;
         std::list<std::tuple<TestSuitePointer, TestList>> sortedSuitesAndTests;
         for (TestPointer resultTest : tests) {
            int suiteId = resultTest->getTestSuite()->getId();
            if (testsuiteMap.find(suiteId) == testsuiteMap.end()) {
               testsuiteMap[suiteId] = resultTest->getTestSuite();
            }
            if (suitesAndTests.find(suiteId) == suitesAndTests.end()) {
               suitesAndTests[suiteId] = TestList{};
            }
            suitesAndTests.at(suiteId).push_back(resultTest);
         }

         for (const auto &item : suitesAndTests) {
            sortedSuitesAndTests.push_back(std::make_tuple(testsuiteMap.at(item.first),
                                                           item.second));

         }
         sortedSuitesAndTests.sort([](const auto &lhs, const auto &rhs) -> bool {
            return std::get<0>(lhs)->getName() < std::get<0>(rhs)->getName();
         });
         // Show the suites, if requested.
         if (showSuites) {
            printf("-- Test Suites --\n");
            for (auto &item : sortedSuitesAndTests) {
               TestSuitePointer testsuite = std::get<0>(item);
               const TestList &tests = std::get<1>(item);
               printf("  %s - %d tests\n", testsuite->getName().c_str(), tests.size());
               printf("    Source Root: %s\n", testsuite->getSourcePath().c_str());
               printf("    Exec Root  : %s\n", testsuite->getExecPath().c_str());
            }
         }
         return 0;
      }
      // Select and order the tests.
      int numTotalTests = tests.size();
      // First, select based on the filter expression if given.
      if (!filter.empty()) {
         std::regex filterRegex(filter);
         auto iter = tests.begin();
         auto endMark = tests.end();
         while (iter != endMark) {
            TestPointer test = *iter;
            if (std::regex_search(test->getFullName(), filterRegex)) {
               tests.erase(iter++);
            } else {
               ++iter;
            }
         }
      }
      if (shuffle) {
         std::random_device randonDevice;
         std::mt19937 randomGenerator(randonDevice());
         std::vector<TestPointer> tempTests{};
         tempTests.reserve(tests.size());
         for (auto &test : tests) {
            tempTests.push_back(test);
         }
         std::shuffle(tempTests.begin(), tempTests.end(), randomGenerator);
         tests.clear();
         for (auto &test : tempTests) {
            tests.push_back(test);
         }
      } else if (incremental) {
         sort_by_incremental_cache(run);
      } else {
         tests.sort([](TestPointer lhs, TestPointer rhs) {
            int learly = !lhs->isEarlyTest();
            int rearly = !rhs->isEarlyTest();
            std::string lname = lhs->getFullName();
            std::string rname = rhs->getFullName();
            return std::tie(learly, lname)
                  < std::tie(rearly, rname);
         });
      }
      // Then optionally restrict our attention to a shard of the tests.
      if (!numShardsOpt->empty() || !runShardOpt->empty()) {
         if (numShardsOpt->empty() || runShardOpt->empty()) {
            throw std::runtime_error("--num-shards and --run-shard must be used together");
         }
         if (numShards <= 0) {
            throw std::runtime_error("--num-shards must be positive");
         }
         if (runShard < 1 || runShard > numShards) {
            throw std::runtime_error("--run-shard must be between 1 and --num-shards (inclusive)");
         }
         int numTests = tests.size();
         // Note: user views tests and shard numbers counting from 1.
         std::vector<int> testIxs;
         for (int i = runShard - 1; i < numTests; i += numShards) {
            testIxs.push_back(i);
         }
         auto iter = tests.begin();
         auto endMark = tests.end();
         int i = 0;
         while (iter != endMark) {
            if (std::find(testIxs.begin(), testIxs.end(), i) == testIxs.end()) {
               tests.erase(iter++);
            } else {
               ++iter;
            }
            ++i;
         }
         // Generate a preview of the first few test indices in the shard
         // to accompany the arithmetic expression, for clarity.
         size_t previewLength = 3;
         size_t previewMaxLength = std::max(previewLength, testIxs.size());
         std::string ixPreview;
         for (int i = 0; i < previewMaxLength; ++i) {
            if (ixPreview.empty()) {
               ixPreview = std::to_string(i + 1);
            } else {
               ixPreview += ", " + std::to_string(i + 1);
            }
         }
         if (testIxs.size() > previewLength) {
            ixPreview += ", ...";
         }
         litConfig->note(format_string("Selecting shard %d/%d = size %d/%d = tests #(%d*k)+%d = [%s]",
                                       runShard, numShards, tests.size(), numTests, numShards, runShard,
                                       ixPreview.c_str()));
      }
      // Finally limit the number of tests, if desired.
      if (!maxTestsOpt->empty()) {
         maxTests = std::max(0ul, std::min(tests.size(), maxTests));
         size_t delta = tests.size() - maxTests;
         auto iter = tests.begin();
         std::advance(iter, maxTests - 1);
         while (delta > 0) {
            tests.erase(iter++);
            --delta;
         }
      }
      // Don't create more threads than tests.
      threadNumbers = std::min(threadNumbers, tests.size());
      std::string extra;
      if (tests.size() != numTotalTests) {
         extra = format_string(" of %d", numTotalTests);
      }
      std::string header = format_string("-- Testing: %d%s tests, %d threads --\n",
                                         tests.size(), extra.c_str(), numTotalTests);
      std::shared_ptr<AbstractProgressBar> progressBarPointer;
      std::shared_ptr<TerminalController> terminalControllerPointer;
      if (!quiet) {
         if (succinct && displayProgressBar) {
            try {
               terminalControllerPointer.reset(new TerminalController{});
               progressBarPointer.reset(new ProgressBar(*terminalControllerPointer.get(), header));
            } catch (...) {
               std::printf(header.c_str());
               progressBarPointer.reset(new SimpleProgressBar("Testing: "));
            }
         } else {
            std::printf(header.c_str());
         }
      }
      std::chrono::time_point startTime = std::chrono::system_clock::now();
      int testingTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime).count();
   } catch (...) {
      eptr = std::current_exception();
   }
   general_exception_handler(eptr);
   return 0;
}

