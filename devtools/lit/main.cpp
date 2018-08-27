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
   litApp.add_option("--path", paths, "Additional paths to add to testing environment")->group("Test Execution");

   CLI11_PARSE(litApp, argc, argv);
   return 0;
}
