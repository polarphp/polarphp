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
#include "lib/Run.h"
#include <iostream>
#include <thread>
#include <assert.h>
#include <filesystem>
#include <list>

using polar::lit::LitConfigPointer;
using polar::lit::LitConfig;
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


int main(int argc, char *argv[])
{
   CLI::App litApp;
   std::vector<std::string> testPaths;
   bool showVersion;
   int threadNumbers;
   std::string cfgPrefix;
   std::vector<std::string> params;
   litApp.add_option("test_paths", testPaths, "Files or paths to include in the test suite");
   litApp.add_flag("--version", showVersion, "Show version and exit");
   CLI::Option *threadsOpt = litApp.add_option("-j,--threads", threadNumbers, "Number of testing threads");
   litApp.add_option("--config-prefix", cfgPrefix, "Prefix for 'lit' config files");
   litApp.add_option("-D,--param", params, "Add 'NAME' = 'VAL' to the user defined parameters");
   /// setup command group
   /// Output Format
   bool quiet;
   bool succinct;
   int verbose;
   bool showAll;
   std::string outputDir;
   bool noProgressBar;
   bool showUnsupported;
   bool showXFail;
   bool echoAllCommands;
   litApp.add_option("-q,--quiet", quiet, "Suppress no error output", false)->group("Output Format");
   litApp.add_option("-s,--succinct", succinct, "Reduce amount of output", false)->group("Output Format");
   litApp.add_option("-v,--verbose", verbose, "Show test output for failures", false)->group("Output Format");
   litApp.add_option("--echo-all-commands", echoAllCommands, "Echo all commands as they are executed to stdout."
                                                             "In case of failure, last command shown will be the failing one.")->group("Output Format");
   litApp.add_option("-a,--show-all", showAll, "Display all commandlines and output", false)->group("Output Format");
   litApp.add_option("-o,--output", outputDir, "Write test results to the provided path")->check(CLI::ExistingDirectory)->group("Output Format");
   litApp.add_option("--no-progress-bar", noProgressBar, "Do not use curses based progress bar", true)->group("Output Format");
   litApp.add_option("--show-unsupported", showUnsupported, "Show unsupported tests", false)->group("Output Format");
   litApp.add_option("--show-xfail", showXFail, "Show tests that were expected to fail", false)->group("Output Format");

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
   litApp.add_option("--path", paths, "Additional paths to add to testing environment", false)->group("Test Execution");
   litApp.add_option("--vg", useValgrind, "Run tests under valgrind", false)->group("Test Execution");
   litApp.add_option("--vg-leak", valgrindLeakCheck, "Check for memory leaks under valgrind", false)->group("Test Execution");
   litApp.add_option("--vg-arg", valgrindArgs, "Check for memory leaks under valgrind")->group("Test Execution");
   litApp.add_option("--time-tests", timeTests, "Track elapsed wall time for each test", false)->group("Test Execution");
   litApp.add_option("--no-execute", noExecute, "Don't execute any tests (assume PASS)", false)->group("Test Execution");
   litApp.add_option("--xunit-xml-output", xunitOutputFile, "Write XUnit-compatible XML test reports to the  specified file")->group("Test Execution");
   litApp.add_option("--timeout", maxIndividualTestTime, "Maximum time to spend running a single test (in seconds)."
                                                         "0 means no time limit. [Default: 0]", 0)->group("Test Execution");
   CLI::Option *maxFailuresOpt = litApp.add_option("--max-failures", maxFailures, "Stop execution after the given number of failures.", 0)->group("Test Execution");

   /// Test Selection
   int maxTests;
   float maxTime;
   bool shuffle;
   bool incremental;
   std::string filter;
   int numShards;
   int runShard;
   litApp.add_option("--max-tests", maxTests, "Maximum number of tests to run")->group("Test Selection");
   litApp.add_option("--max-time", maxTime, "Maximum time to spend testing (in seconds)")->group("Test Selection");
   litApp.add_flag("--shuffle", shuffle, "Run tests in random order")->group("Test Selection");
   litApp.add_flag("-i, --incremental", incremental, "Run modified and failing tests first (updates "
                                                       "mtimes)")->group("Test Selection");
   litApp.add_option("--filter", filter, "Only run tests with paths matching the given "
                                         "regular expression")->envname("LIT_FILTER")->group("Test Selection");
   litApp.add_option("--num-shards", numShards, "Split testsuite into M pieces and only run one")->envname("LIT_NUM_SHARDS")->group("Test Selection");
   litApp.add_option("--run-shard", runShard, "Run shard #N of the testsuite")->envname("LIT_RUN_SHARD")->group("Test Selection");

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
   }
   if (threadsOpt->empty()) {
      threadNumbers = std::thread::hardware_concurrency();
   }
   if (!maxFailuresOpt->empty() && maxFailures == 0) {
      std::cerr << "Setting --max-failures to 0 does not have any effect." << std::endl;
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
         (!cfgPrefix.empty() ? std::optional(cfgPrefix) : std::nullopt),
         maxIndividualTestTime,
         maxFailures,
         std::map<std::string, std::string>{},
   echoAllCommands
         );
   // Perform test discovery.
   std::exception_ptr eptr;
   try {
      polar::lit::Run run(litConfig,
                          polar::lit::find_tests_for_inputs(litConfig, inputs));
   } catch (...) {
      eptr = std::current_exception();
   }
   general_exception_handler(eptr);
   return 0;
}

