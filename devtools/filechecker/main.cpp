// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/28.

#include "CLI/CLI.hpp"
#include "CheckFuncs.h"
#include "CheckPattern.h"
#include "CheckString.h"
#include "lib/Global.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/utils/InitPolar.h"
#include "polarphp/utils/SourceMgr.h"
#include "polarphp/utils/OptionalError.h"
#include "polarphp/utils/MemoryBuffer.h"
#include <boost/regex.hpp>

using polar::basic::StringRef;
using namespace polar::filechecker;
using namespace polar::utils;
using namespace polar::basic;

int main(int argc, char *argv[])
{
   polar::InitPolar polarInitializer(argc, argv);
   CLI::App cmdParser;
   sg_commandParser = &cmdParser;
   polarInitializer.initNgOpts(cmdParser);
   std::string checkFilename;
   std::string inputFilename;
   std::vector<std::string> checkPrefix;
   bool noCanonicalizeWhiteSpace;
   bool allowEmptyInput;
   bool matchFullLines;
   bool enableVarScope;
   bool allowDeprecatedDagOverlap;
   bool verbose;
   int dumpInputOnFailure = 0;

   cmdParser.add_option("check-filename", checkFilename, "<check-file>")->required(true);
   cmdParser.add_option("--input-file", inputFilename, "File to check (defaults to stdin)")->default_val("-")
         ->type_name("filename");
   cmdParser.add_option("--check-prefix", checkPrefix, "Prefix to use from check file (defaults to 'CHECK')");
   cmdParser.add_option("--check-prefixes", sg_checkPrefixes, "Alias for -check-prefix permitting multiple comma separated values");
   cmdParser.add_flag("--strict-whitespace", noCanonicalizeWhiteSpace, "Do not treat all horizontal whitespace as equivalent");
   cmdParser.add_option("--implicit-check-not", sg_implicitCheckNot, "Add an implicit negative check with this pattern to every"
                                                                  "positive check. This can be used to ensure that no instances of"
                                                                  "this pattern occur which are not matched by a positive pattern")
         ->type_name("pattern");
   cmdParser.add_option("-D", sg_defines, "Define a variable to be used in capture patterns.")->type_name("VAR=VALUE");
   cmdParser.add_flag("--allow-empty", allowEmptyInput, "Allow the input file to be empty. This is useful when making"
                                                        "checks that some error message does not occur, for example.");
   cmdParser.add_flag("--match-full-lines", matchFullLines, "Require all positive matches to cover an entire input line."
                                                            "Allows leading and trailing whitespace if --strict-whitespace"
                                                            "is not also passed.");
   cmdParser.add_flag("--enable-var-scope", enableVarScope, "Enables scope for regex variables. Variables with names that"
                                                            "do not start with '$' will be reset at the beginning of"
                                                            "each CHECK-LABEL block.");
   cmdParser.add_flag("--allow-deprecated-dag-overlap", allowDeprecatedDagOverlap, "Enable overlapping among matches in a group of consecutive"
                                                                                   "CHECK-DAG directives.  This option is deprecated and is only"
                                                                                   "provided for convenience as old tests are migrated to the new"
                                                                                   "non-overlapping CHECK-DAG implementation.");
   cmdParser.add_flag("-v", verbose, "Print directive pattern matches, you can specify --vv to print extra verbose info.");
   CLI::Option *dumpInputOnFailureOpt = cmdParser.add_option("--dump-input-on-failure", dumpInputOnFailure, "Dump original input to stderr before failing."
                                                                                                          "The value can be also controlled using "
                                                                                                          "FILECHECK_DUMP_INPUT_ON_FAILURE environment variable.");
   CLI11_PARSE(cmdParser, argc, argv);
   if (dumpInputOnFailureOpt->count() == 0) {
      std::string dumpInputOnFailureEnv = StringRef(std::getenv("FILECHECK_DUMP_INPUT_ON_FAILURE")).trim().toLower();
      if (!dumpInputOnFailureEnv.empty() && (dumpInputOnFailureEnv == "true" || dumpInputOnFailureEnv == "on" || dumpInputOnFailureEnv == "1")) {
         dumpInputOnFailure = 1;
      } else {
         dumpInputOnFailure = 0;
      }
   }
   if (!checkPrefix.empty()) {
      for (std::string &prefix : checkPrefix) {
         sg_checkPrefixes.push_back(prefix);
      }
   }

   if (!validate_check_prefixes()) {
      error_stream() << "Supplied check-prefix is invalid! Prefixes must be unique and "
                        "start with a letter and contain only alphanumeric characters, "
                        "hyphens and underscores\n";
      return 2;
   }


   boost::regex prefixRegex;
   std::string regexError;
   if (!build_check_prefix_regex(prefixRegex, regexError)) {
      error_stream() << "Unable to combine check-prefix strings into a prefix regular "
                        "expression! This is likely a bug in filechecker's verification of "
                        "the check-prefix strings. Regular expression parsing failed "
                        "with the following error: "
                     << regexError << "\n";
      return 2;
   }
   SourceMgr sourceMgr;
   OptionalError<std::unique_ptr<MemoryBuffer>> checkFileOrErr =
         MemoryBuffer::getFileOrStdIn(checkFilename);
   if (std::error_code errorCode = checkFileOrErr.getError()) {
      error_stream() << "Could not open check file '" << checkFilename
                     << "': " << errorCode.message() << '\n';
      return 2;
   }
   MemoryBuffer &checkFile = *checkFileOrErr.get();

   SmallString<4096> checkFileBuffer;
   StringRef checkFileText = canonicalize_file(checkFile, checkFileBuffer);
   sourceMgr.addNewSourceBuffer(MemoryBuffer::getMemBuffer(
                                   checkFileText, checkFile.getBufferIdentifier()),
                                SMLocation());

   std::vector<CheckString> checkStrings;
   if (read_check_file(sourceMgr, checkFileText, prefixRegex, checkStrings)) {
      return 2;
   }
   // Open the file to check and add it to SourceMgr.
   OptionalError<std::unique_ptr<MemoryBuffer>> inputFileOrErr =
         MemoryBuffer::getFileOrStdIn(inputFilename);
   if (std::error_code errorCode = inputFileOrErr.getError()) {
      error_stream() << "Could not open input file '" << inputFilename
                     << "': " << errorCode.message() << '\n';
      return 2;
   }
   MemoryBuffer &inputFile = *inputFileOrErr.get();
   if (inputFile.getBufferSize() == 0 && !allowEmptyInput) {
      error_stream() << "filechecker error: '" << inputFilename << "' is empty.\n";
      dump_command_line(argc, argv);
      return 2;
   }

   SmallString<4096> inputFileBuffer;
   StringRef inputFileText = canonicalize_file(inputFile, inputFileBuffer);
   sourceMgr.addNewSourceBuffer(MemoryBuffer::getMemBuffer(
                                   inputFileText, inputFile.getBufferIdentifier()),
                                SMLocation());

   int exitCode = check_input(sourceMgr, inputFileText, checkStrings) ? EXIT_SUCCESS : 1;
   if (exitCode == 1 && dumpInputOnFailure != 0) {
      error_stream() << "Full input was:\n<<<<<<\n" << inputFileText << "\n>>>>>>\n";
   }
   return exitCode;
}
