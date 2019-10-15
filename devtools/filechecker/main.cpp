// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/28.

#include "CLI/CLI.hpp"
#include "lib/ExtraFuncs.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/utils/InitPolar.h"
#include "polarphp/utils/SourceMgr.h"
#include "polarphp/utils/OptionalError.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/Process.h"
#include <boost/regex.hpp>

using polar::basic::StringRef;
using namespace polar::filechecker;
using namespace polar::utils;
using namespace polar::basic;

int main(int argc, char *argv[])
{
   // Enable use of ANSI color codes because FileCheck is using them to
   // highlight text.
   polar::sys::Process::useANSIEscapeCodes(true);
   polar::InitPolar polarInitializer(argc, argv);
   CLI::App cmdParser;
   sg_commandParser = &cmdParser;
   polarInitializer.initNgOpts(cmdParser);
   std::string checkFilename;
   std::string inputFilename;
   std::string dumpInputStr;
   DumpInputValue dumpInput = DumpInputValue::Default;
   std::vector<std::string> checkPrefix;
   bool noCanonicalizeWhiteSpace;
   bool allowEmptyInput;
   bool matchFullLines;
   bool enableVarScope;
   bool allowDeprecatedDagOverlap;
   bool verbose;
   bool verboseVerbose = false;
   int dumpInputOnFailure = 0;

   cmdParser.add_option("check-filename", checkFilename, "<check-file>")->required(true)->check(CLI::ExistingFile);
   cmdParser.add_option("--input-file", inputFilename, "File to check (defaults to stdin)")->default_val("-")
         ->type_name("filename");
   cmdParser.add_option("--check-prefix", checkPrefix, "Prefix to use from check file (defaults to 'CHECK')");
   cmdParser.add_option("--check-prefixes", sg_checkPrefixes, "Alias for --check-prefix permitting multiple comma separated values");
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
   CLI::Option* verboseFlag = cmdParser.add_flag("-v", verbose, "Print directive pattern matches, you can specify --vv to print extra verbose info.");
   cmdParser.add_option("--dump-input", dumpInputStr, "Dump input to stderr, adding annotations representing"
                                                      " currently enabled diagnostics\n\n"
                                                      "available options:\n"
                                                      "\thelp   Explain dump format and quit\n"
                                                      "\tnever  Never dump input\n"
                                                      "\tfail   Dump input on failure\n"
                                                      "\talways Always dump input\n")
         ->default_val("default")
         ->check(dump_input_checker);
   CLI::Option *dumpInputOnFailureOpt = cmdParser.add_option("--dump-input-on-failure", dumpInputOnFailure, "Dump original input to stderr before failing."
                                                                                                            "The value can be also controlled using "
                                                                                                            "FILECHECK_DUMP_INPUT_ON_FAILURE environment variable.");
   CLI11_PARSE(cmdParser, argc, argv);
   dumpInput = get_dump_input_type(dumpInputStr);
   if (dumpInput == DumpInputValue::Help) {
      dump_input_annotation_help(polar::utils::out_stream());
      return 0;
   }

   if (dumpInputOnFailureOpt->count() == 0) {
      std::string dumpInputOnFailureEnv = StringRef(std::getenv("FILECHECK_DUMP_INPUT_ON_FAILURE")).trim().toLower();
      if (!dumpInputOnFailureEnv.empty() && (dumpInputOnFailureEnv == "true" || dumpInputOnFailureEnv == "on" || dumpInputOnFailureEnv == "1")) {
         dumpInputOnFailure = 1;
      } else {
         dumpInputOnFailure = 0;
      }
   }

   FileCheckRequest checkRequest;
   if (!checkPrefix.empty()) {
      for (std::string &prefix : checkPrefix) {
         checkRequest.checkPrefixes.push_back(prefix);
      }
   }

   for (auto item : sg_implicitCheckNot) {
      checkRequest.implicitCheckNot.push_back(item);
   }

   bool globalDefineError = false;
   for (auto def : sg_defines) {
      size_t eqIdx = def.find('=');
      if (eqIdx == std::string::npos) {
         error_stream() << "Missing equal sign in command-line definition '-D" << def
                        << "'\n";
         globalDefineError = true;
         continue;
      }
      if (eqIdx == 0) {
         error_stream() << "Missing pattern variable name in command-line definition '-D"
                        << def << "'\n";
         globalDefineError = true;
         continue;
      }
      checkRequest.globalDefines.push_back(def);
   }

   if (globalDefineError) {
      return 2;
   }

   if (verboseFlag->count() > 1) {
      verboseVerbose = true;
   }

   checkRequest.allowEmptyInput = allowEmptyInput;
   checkRequest.enableVarScope = enableVarScope;
   checkRequest.allowDeprecatedDagOverlap = allowDeprecatedDagOverlap;
   checkRequest.verbose = verbose;
   checkRequest.verboseVerbose = verboseVerbose;
   checkRequest.noCanonicalizeWhiteSpace = noCanonicalizeWhiteSpace;
   checkRequest.matchFullLines = matchFullLines;

   if (verboseVerbose) {
      checkRequest.verbose = true;
   }

   FileCheck fileChecker(checkRequest);
   if (!fileChecker.validateCheckPrefixes()) {
      error_stream() << "Supplied check-prefix is invalid! Prefixes must be unique and "
                        "start with a letter and contain only alphanumeric characters, "
                        "hyphens and underscores\n";
      return 2;
   }

   boost::regex prefixRegex;
   try {
      prefixRegex = fileChecker.buildCheckPrefixRegex();
   } catch (std::exception &e) {
      error_stream() << "Unable to combine check-prefix strings into a prefix regular "
                        "expression! This is likely a bug in FileCheck's verification of "
                        "the check-prefix strings. Regular expression parsing failed "
                        "with the following error: "
                     << e.what() << "\n";
      return 2;
   }

   SourceMgr sourceMgr;

   // Read the expected strings from the check file.
   OptionalError<std::unique_ptr<MemoryBuffer>> checkFileOrError =
         MemoryBuffer::getFileOrStdIn(checkFilename);
   if (std::error_code errorCode = checkFileOrError.getError()) {
      error_stream() << "Could not open check file '" << checkFilename
                     << "': " << errorCode.message() << '\n';
      return 2;
   }
   MemoryBuffer &checkFile = *checkFileOrError.get();

   SmallString<4096> checkFileBuffer;
   StringRef checkFileText = fileChecker.canonicalizeFile(checkFile, checkFileBuffer);

   sourceMgr.addNewSourceBuffer(MemoryBuffer::getMemBuffer(
                                   checkFileText, checkFile.getBufferIdentifier()),
                                SMLocation());

   std::vector<FileCheckString> checkStrings;
   if (fileChecker.readCheckFile(sourceMgr, checkFileText, prefixRegex, checkStrings)) {
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
      error_stream() << "FileCheck error: '" << inputFilename << "' is empty.\n";
      dump_command_line(argc, argv);
      return 2;
   }

   SmallString<4096> InputFileBuffer;
   StringRef inputFileText = fileChecker.canonicalizeFile(inputFile, InputFileBuffer);

   sourceMgr.addNewSourceBuffer(MemoryBuffer::getMemBuffer(
                                   inputFileText, inputFile.getBufferIdentifier()),
                                SMLocation());

   if (dumpInput == DumpInputValue::Default) {
      dumpInput = dumpInputOnFailure ? DumpInputValue::Fail : DumpInputValue::Never;
   }

   std::vector<FileCheckDiag> diags;
   int exitCode = fileChecker.checkInput(sourceMgr, inputFileText, checkStrings,
                                         dumpInput == DumpInputValue::Never ? nullptr : &diags)
         ? EXIT_SUCCESS
         : 1;
   if (dumpInput == DumpInputValue::Always ||
       (exitCode == 1 && dumpInput == DumpInputValue::Fail)) {
      error_stream() << "\n"
                     << "Input file: "
                     << (inputFilename == "-" ? "<stdin>" : inputFilename)
                     << "\n"
                     << "Check file: " << checkFilename << "\n"
                     << "\n"
                     << "-dump-input=help describes the format of the following dump.\n"
                     << "\n";
      std::vector<InputAnnotation> annotations;
      unsigned labelWidth;
      build_input_annotations(diags, annotations, labelWidth);
      dump_annotated_input(error_stream(), checkRequest, inputFileText, annotations, labelWidth);
   }

   return exitCode;
}
