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

int main(int argc, char *argv[])
{
   CLI::App litApp;
   std::vector<std::string> testPaths;
   bool showVersion;
   int threadNumbers;
   std::string cfgPrefix;
   std::string params;
   litApp.add_option("test_paths", testPaths, "Files or paths to include in the test suite");
   litApp.add_option("--version", showVersion, "Show version and exit", false);
   litApp.add_option("-j,--threads", threadNumbers, "Number of testing threads");
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
   litApp.add_option("-q,--quiet", quiet, "Suppress no error output", false)->group("Output Format");
   litApp.add_option("-s,--succinct", succinct, "Reduce amount of output", false)->group("Output Format");
   litApp.add_option("-v,--verbose", verbose, "Show test output for failures", false)->group("Output Format");
   litApp.add_option("--echo-all-commands", verbose, "Echo all commands as they are executed to stdout."
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
   litApp.add_option("--max-failures", maxFailures, "Stop execution after the given number of failures.", 0)->group("Test Execution");

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
   litApp.add_option("--shuffle", shuffle, "Run tests in random order", false)->group("Test Selection");
   litApp.add_option("-i, --incremental", incremental, "Run modified and failing tests first (updates "
                                                       "mtimes)", false)->group("Test Selection");
   litApp.add_option("--filter", filter, "Only run tests with paths matching the given "
                                         "regular expression")->envname("LIT_FILTER")->group("Test Selection");
   litApp.add_option("--num-shards", numShards, "Split testsuite into M pieces and only run one")->envname("LIT_NUM_SHARDS")->group("Test Selection");
   litApp.add_option("--run-shard", runShard, "Run shard #N of the testsuite")->envname("LIT_RUN_SHARD")->group("Test Selection");

   /// debug
   bool debug;
   bool showSuites;
   bool showTests;
   bool singleProcess;
   litApp.add_option("--debug", debug, "Enable debugging (for 'lit' development)", false)->group("Debug and Experimental Options");
   litApp.add_option("--show-suites", showSuites, "Show discovered test suites", false)->group("Debug and Experimental Options");
   litApp.add_option("--show-tests", showTests, "Show all discovered tests", false)->group("Debug and Experimental Options");
   litApp.add_option("--single-process", singleProcess, "Don't run tests in parallel.  Intended for debugging "
                                                "single test failures", false)->group("Debug and Experimental Options");

   CLI11_PARSE(litApp, argc, argv);
   return 0;
}
