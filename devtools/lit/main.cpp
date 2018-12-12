// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/27.

#include "CLI/CLI.hpp"
#include "lib/Utils.h"
#include "lib/LitConfig.h"
#include "lib/Discovery.h"
#include "lib/LitGlobal.h"
#include "lib/Test.h"
#include "lib/Run.h"
#include "lib/ProgressBar.h"
#include "polarphp/basic/adt/StringRef.h"
#include "nlohmann/json.hpp"
#include <signal.h>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <assert.h>
#include <filesystem>
#include <list>
#include <vector>
#include <random>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <fstream>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <boost/regex.hpp>

using polar::lit::LitConfigPointer;
using polar::lit::LitConfig;
using polar::lit::Test;
using polar::lit::TestSuite;
using polar::lit::TestSuitePointer;
using polar::lit::Result;
using polar::lit::ResultPointer;
using polar::lit::TestPointer;
using polar::lit::ResultCode;
using polar::lit::MetricValue;
using polar::lit::IntMetricValue;
using polar::lit::RealMetricValue;
using polar::lit::JSONMetricValue;
using polar::lit::MetricValuePointer;
using polar::lit::TestList;
using polar::lit::ProgressBar;
using polar::lit::TerminalController;
using polar::lit::SimpleProgressBar;
using polar::lit::AbstractProgressBar;
using polar::lit::TestingProgressDisplay;
using polar::lit::format_string;
namespace fs = std::filesystem;

static std::mutex sg_signalMutex{};
static std::condition_variable sg_signalConditionVar{};
static std::atomic_bool sg_testFinished = false;

namespace {

std::list<std::string> vector_to_list(const std::vector<std::string> &items)
{
   std::list<std::string> ret;
   for (const std::string &item : items) {
      ret.push_back(item);
   }
   return ret;
}

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

void sort_by_incremental_cache(polar::lit::Run &run)
{
   run.getTests().sort([](TestPointer &lhs, TestPointer &rhs) {
      fs::path lhsPath(lhs->getFilePath());
      fs::path rhsPath(rhs->getFilePath());
      return fs::last_write_time(lhsPath) > fs::last_write_time(rhsPath);
   });
}

void write_test_results(polar::lit::Run &run, LitConfigPointer, size_t testingTime,
                        const std::string &outputPath)
{
   // Construct the data we will write.
   nlohmann::json testDoc = nlohmann::json::object();
   testDoc["engineVersion"] = POLAR_LIT_VERSION;
   testDoc["elapsed"] = testingTime;
   // FIXME: Record some information on the lit configuration used?
   // FIXME: Record information from the individual test suites?
   // Encode the tests.
   nlohmann::json testsData = nlohmann::json::array();
   for (TestPointer test : run.getTests()) {
      ResultPointer testResult = test->getResult();
      int elapse = test->getResult()->getElapsed().has_value() ? static_cast<int>(test->getResult()->getElapsed().value()) : -1;
      nlohmann::json testData = nlohmann::json::object(
      {
                  {"name", test->getFullName()},
                  {"code", test->getResult()->getCode()->getName()},
                  {"output", test->getResult()->getOutput()},
                  {"elapse", elapse}
               });
      // Add test metrics, if present.
      if (!testResult->getMetrics().empty()) {
         nlohmann::json metrics = nlohmann::json::object();
         for (auto &metricItem : testResult->getMetrics()) {
            MetricValuePointer metricValue = metricItem.second;
            if (metricValue->getValueType() == MetricValue::ValueType::Integer) {
               IntMetricValue *metric = dynamic_cast<IntMetricValue *>(metricValue.get());
               metrics[metricItem.first] = metric->toData();
            } else if (metricValue->getValueType() == MetricValue::ValueType::Real) {
               RealMetricValue *metric = dynamic_cast<RealMetricValue *>(metricValue.get());
               metrics[metricItem.first] = metric->toData();
            } else if(metricValue->getValueType() == MetricValue::ValueType::Json) {
               JSONMetricValue *metric = dynamic_cast<JSONMetricValue *>(metricValue.get());
               metrics[metricItem.first] = metric->toData();
            }
            testData["metrics"] = metrics;
         }
      }
      // Report micro-tests separately, if present
      if (!testResult->getMicroResults().empty()) {
         // Expand parent test name with micro test name
         for (auto &microItem : testResult->getMicroResults()){
            std::string parentName = test->getFullName();
            ResultPointer microTest = microItem.second;
            std::string microFullName = parentName + ":" + microItem.first;
            int elapse = microTest->getElapsed().has_value() ? static_cast<int>(microTest->getElapsed().value()) : -1;
            nlohmann::json microTestData = nlohmann::json::object(
            {
                        {"name", microFullName},
                        {"code", microTest->getCode()->getName()},
                        {"output", microTest->getOutput()},
                        {"elapsed", elapse}
                     });
            if (!microTest->getMetrics().empty()) {
               nlohmann::json microMetrics = nlohmann::json::object();
               for (auto &microMetricItem : testResult->getMetrics()) {
                  MetricValuePointer microMetricValue = microMetricItem.second;
                  if (microMetricValue->getValueType() == MetricValue::ValueType::Integer) {
                     IntMetricValue *metric = dynamic_cast<IntMetricValue *>(microMetricValue.get());
                     microMetrics[microMetricItem.first] = metric->toData();
                  } else if (microMetricValue->getValueType() == MetricValue::ValueType::Real) {
                     RealMetricValue *metric = dynamic_cast<RealMetricValue *>(microMetricValue.get());
                     microMetrics[microMetricItem.first] = metric->toData();
                  } else if(microMetricValue->getValueType() == MetricValue::ValueType::Json) {
                     JSONMetricValue *metric = dynamic_cast<JSONMetricValue *>(microMetricValue.get());
                     microMetrics[microMetricItem.first] = metric->toData();
                  }
                  microTestData["metrics"] = microMetrics;
               }
            }
            testsData.push_back(microTestData);
         }
      }
      testsData.push_back(testData);
   }
   fs::path outputDir(outputPath);
   outputDir = outputDir.parent_path();
   if (!fs::exists(outputDir)) {
      fs::create_directory(outputDir);
   }
   std::fstream jsonStream(outputPath, std::ios_base::out | std::ios_base::trunc);
   if (!jsonStream.is_open()) {
      throw std::runtime_error(format_string("open json file %s error", outputPath.c_str()));
   }
   jsonStream << std::setw(4) << testsData;
}

} // anonymous namespace

void polar_signal_handler(int)
{
   sg_signalConditionVar.notify_one();
}

void setup_siganl()
{
   struct sigaction sigIntHandler;
   sigIntHandler.sa_handler = polar_signal_handler;
   sigemptyset(&sigIntHandler.sa_mask);
   sigIntHandler.sa_flags = 0;
   sigaction(SIGINT, &sigIntHandler, nullptr);
}

void signal_detect_worker()
{
   std::unique_lock<std::mutex> lk(sg_signalMutex);
   sg_signalConditionVar.wait(lk);
   if (!sg_testFinished.load()) {
      std::cout << std::endl << "catch ctrl-c request, exit test cycle ... " << std::endl;
      std::exit(2);
   }
}

int main(int argc, char *argv[])
{
   CLI::App litApp;
   std::vector<std::string> testPaths;
   bool showVersion;
   size_t threadNumbers;
   std::string cfgPrefix;
   std::vector<std::string> params;
   std::string cfgSetterPluginDir;
   litApp.add_option("test_paths", testPaths, "Files or paths to include in the test suite");
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
   litApp.add_flag("--echo-all-commands", echoAllCommands, "Echo all commands as they are executed to stdout."
                                                           "In case of failure, last command shown will be the failing one.")->group("Output Format");
   litApp.add_flag("-a,--show-all", showAll, "Display all commandlines and output")->group("Output Format");
   CLI::Option *outputDirOpt = litApp.add_option("-o,--output", outputDir, "Write test results to the provided path")->group("Output Format");
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
   size_t maxTime;
   bool shuffle;
   bool incremental;
   std::string filter;
   size_t numShards;
   size_t runShard;
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
   if (showVersion) {
      std::cout << "lit " << POLAR_LIT_VERSION << std::endl;
      return 0;
   }
   if (testPaths.empty()) {
      std::cerr << "No inputs specified" << std::endl;
      std::cout << litApp.help() << std::endl;
      exit(1);
   }
   std::thread worker(signal_detect_worker);
   std::exception_ptr eptr;
   try {
      if (threadsOpt->empty()) {
         threadNumbers = std::thread::hardware_concurrency();
      }
      std::optional<int> maxFailuresOr;
      if (!maxFailuresOpt->empty() && maxFailures == 0) {
         std::cerr << "error: Setting --max-failures to 0 does not have any effect." << std::endl;
      } else if (!maxFailuresOpt->empty() && maxFailures > 0) {
         maxFailuresOr = maxFailures;
      }
      if (cfgSetterPluginDir.empty()) {
         cfgSetterPluginDir = POLAR_LIT_RUNTIME_DIR;
      }
      if (echoAllCommands) {
         showOutput = true;
      }
      atexit(polar::lit::temp_files_clear_handler);
      atexit(polar::lit::global_resultcode_destroyer);
      setup_siganl();
      std::list<std::string> inputs(vector_to_list(testPaths));
      // Create the user defined parameters.
      std::map<std::string, std::string> userParams;
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
            maxFailuresOr,
            std::map<std::string, int>{},
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
                                          maxIndividualTestTime, maxIndividualTestTime));
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
               printf("  %s - %lu tests\n", testsuite->getName().c_str(), tests.size());
               printf("    Source Root: %s\n", testsuite->getSourcePath().c_str());
               printf("    Exec Root  : %s\n", testsuite->getExecPath().c_str());
            }
         }
         printf("\n");
         if (showTests) {
            printf("-- Test Suites --\n");
            for (auto &item : sortedSuitesAndTests) {
               TestList &tests = std::get<1>(item);
               tests.sort([](auto &lhs, auto &rhs) -> bool {
                  return lhs->getSourcePath() < rhs->getSourcePath();
               });
               for (TestPointer test: tests) {
                  printf(" %s\n", test->getFullName().c_str());
               }
            }
         }
         sg_testFinished.store(true);
         sg_signalConditionVar.notify_one();
         worker.join();
         return 0;
      }
      // Select and order the tests.
      size_t numTotalTests = tests.size();
      // First, select based on the filter expression if given.
      if (!filter.empty()) {
         boost::regex filterRegex(filter);
         auto iter = tests.begin();
         auto endMark = tests.end();
         while (iter != endMark) {
            TestPointer test = *iter;
            if (boost::regex_search(test->getFullName(), filterRegex)) {
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
         size_t numTests = tests.size();
         // Note: user views tests and shard numbers counting from 1.
         std::vector<size_t> testIxs;
         for (size_t i = runShard - 1; i < numTests; i += numShards) {
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
         for (size_t i = 0; i < previewMaxLength; ++i) {
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
      if (singleProcess) {
         threadNumbers = 1;
      }
      threadNumbers = std::min(threadNumbers, tests.size());
      std::string extra;
      if (tests.size() != numTotalTests) {
         extra = format_string(" of %d", numTotalTests);
      }
      std::string header = format_string("-- Testing: %d%s tests, %d threads --\n",
                                         tests.size(), extra.c_str(), threadNumbers);
      std::shared_ptr<AbstractProgressBar> progressBarPointer;
      std::shared_ptr<TerminalController> terminalControllerPointer;
      if (!quiet) {
         if (succinct && displayProgressBar) {
            try {
               terminalControllerPointer.reset(new TerminalController);
               progressBarPointer.reset(new ProgressBar(*terminalControllerPointer.get(), header));
            } catch (...) {
               std::printf("%s\n", header.c_str());
               progressBarPointer.reset(new SimpleProgressBar("Testing: "));
            }
         } else {
            std::printf("%s\n", header.c_str());
         }
      }
      std::chrono::time_point startTime = std::chrono::system_clock::now();
      std::shared_ptr<TestingProgressDisplay> display(new TestingProgressDisplay(litApp, run.getTests().size(), progressBarPointer));
      try {
         run.executeTests(display, threadNumbers, maxTime);
      } catch (...) {

      }
      display->finish();
      size_t testingTime = static_cast<size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime).count());
      if (!quiet) {
         std::printf("Testing Time: %.2fs\n", static_cast<double>(testingTime) / 1000);
      }
      // Write out the test data, if requested.
      if (!outputDirOpt->empty()) {
         write_test_results(run, litConfig, testingTime, outputDir);
      }
      // List test results organized by kind.
      bool hasFailures = false;
      std::unordered_map<const ResultCode *, TestList> byCode;
      for (TestPointer test: run.getTests()) {
         if (byCode.find(test->getResult()->getCode()) == byCode.end()) {
            byCode[test->getResult()->getCode()] = TestList{};
         }
         byCode[test->getResult()->getCode()].push_back(test);
         if (test->getResult()->getCode()->isFailure()) {
            hasFailures = true;
         }
      }
      // Print each test in any of the failing groups.
      std::list<std::pair<std::string, const ResultCode *>> titleCodeMap = {
         {"Unexpected Passing Tests", polar::lit::XPASS},
         {"Failing Tests", polar::lit::FAIL},
         {"Unresolved Tests", polar::lit::UNRESOLVED},
         {"Unsupported Tests", polar::lit::UNSUPPORTED},
         {"Expected Failing Tests", polar::lit::XFAIL},
         {"Timed Out Tests", polar::lit::TIMEOUT}
      };
      for (auto &item: titleCodeMap) {
         const std::string &title = item.first;
         const ResultCode *code = item.second;
         if ((polar::lit::XFAIL == code && !showXFail) ||
             (polar::lit::UNSUPPORTED == code && !showUnsupported) ||
             (polar::lit::UNRESOLVED == code && !maxFailuresOpt->empty())) {
            continue;
         }
         if (byCode.find(code) == byCode.end()) {
            continue;
         }
         TestList &elts = byCode.at(code);
         if (elts.empty()) {
            continue;
         }
         std::printf("%s\n", std::string(20, '*').c_str());
         std::printf("%s (%lu):\n", title.c_str(), elts.size());
         for (TestPointer test: elts) {
            std::printf("    %s\n", test->getFullName().c_str());
         }
         std::printf("\n");
      }
      if (timeTests && !run.getTests().empty()) {
         // Order by time.
         std::list<std::tuple<std::string, int>> testTimes;
         for (TestPointer test: run.getTests()) {
            int elapsed = test->getResult()->getElapsed() ? static_cast<int>(test->getResult()->getElapsed().value()) : -1;
            testTimes.push_back({test->getFullName(), elapsed});
            polar::lit::print_histogram(testTimes, "Tests");
         }
      }
      std::list<std::pair<std::string, const ResultCode *>> nameCodeMap = {
         {"Expected Passes    ", polar::lit::PASS},
         {"Passes With Retry  ", polar::lit::FLAKYPASS},
         {"Expected Failures  ", polar::lit::XFAIL},
         {"Unsupported Tests  ", polar::lit::UNSUPPORTED},
         {"Unresolved Tests   ", polar::lit::UNRESOLVED},
         {"Unexpected Passes  ", polar::lit::XPASS},
         {"Unexpected Failures", polar::lit::FAIL},
         {"Individual Timeouts", polar::lit::TIMEOUT}
      };
      for (auto &item: nameCodeMap) {
         const std::string &name = item.first;
         const ResultCode *code = item.second;
         if (quiet && !code->isFailure()) {
            continue;
         }
         size_t N = 0;
         if (byCode.find(code) != byCode.end()) {
            N = byCode.at(code).size();
         }
         if (N != 0) {
            std::printf("  %s: %lu\n", name.c_str(), N);
         }
      }
      if (!xunitOutputFile.empty()) {
         // Collect the tests, indexed by test suite
         // passes:failures:skipped:tests
         using BySuiteItemType = std::tuple<int, int, int, TestList>;
         std::map<std::string, BySuiteItemType> bySuite;
         for (TestPointer test: run.getTests()) {
            std::string suite = test->getTestSuite()->getName();
            if (bySuite.find(suite) == bySuite.end()) {
               bySuite[suite] = std::make_tuple(0, 0, 0, TestList{});
            }
            std::get<3>(bySuite[suite]).push_back(test);
            if (test->getResult()->getCode()->isFailure()) {
               std::get<1>(bySuite[suite]) += 1;

            } else if (test->getResult()->getCode() == polar::lit::UNSUPPORTED) {
               std::get<2>(bySuite[suite]) += 1;
            } else {
               std::get<0>(bySuite[suite]) += 1;
            }
         }
         std::fstream xmlDoc(xunitOutputFile, std::ios_base::trunc | std::ios_base::out);
         if (!xmlDoc.is_open()) {
            throw std::runtime_error(format_string("open xunitOutputFile: %s error", xunitOutputFile.c_str()));
         }
         xmlDoc << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << std::endl;
         xmlDoc << "<testsuites>" << std::endl;
         for (auto &item: bySuite) {
            std::string suiteName = item.first;
            BySuiteItemType &suiteData = item.second;
            polar::lit::replace_string(".", "-", suiteName);
            xmlDoc << "<testsuite name=\"" << suiteName << "\" ";
            xmlDoc << " tests=\"" << std::get<0>(suiteData) + std::get<1>(suiteData) + std::get<2>(suiteData);
            xmlDoc << "\"";
            xmlDoc << " failures=\"" << std::get<1>(suiteData) << "\"";
            xmlDoc << " skipped=\"" << std::get<2>(suiteData) << "\">" << std::endl;
            for (TestPointer test: std::get<3>(suiteData)) {
               std::string testXml;
               test->writeJUnitXML(testXml);
               xmlDoc << testXml << std::endl;
            }
            xmlDoc << "</testsuite>" << std::endl;
         }
         xmlDoc << "</testsuites>" << std::endl;
      }
      sg_testFinished.store(true);
      sg_signalConditionVar.notify_one();
      worker.join();
      if (hasFailures) {
         exit(1);
      }
      return 0;
   } catch (...) {
      sg_testFinished.store(true);
      sg_signalConditionVar.notify_one();
      worker.join();
      eptr = std::current_exception();
   }
   general_exception_handler(eptr);

}
