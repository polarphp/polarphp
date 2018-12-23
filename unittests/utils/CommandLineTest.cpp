// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/07/10.

#include "polarphp/utils/CommandLine.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/Path.h"
#include "polarphp/utils/Host.h"
#include "polarphp/utils/Program.h"
#include "polarphp/utils/StringSaver.h"
#include "polarphp/basic/adt/Triple.h"
#include "gtest/gtest.h"
#include <fstream>
#include <stdlib.h>
#include <string>
#include <ostream>

using namespace polar::basic;
using namespace polar::utils;
using namespace polar;

namespace {

class TempEnvVar {
public:
   TempEnvVar(const char *name, const char *value)
      : name(name) {
      const char *old_value = getenv(name);
      EXPECT_EQ(nullptr, old_value) << old_value;
#ifdef HAVE_SETENV
      setenv(name, value, true);
#else
#   define SKIP_ENVIRONMENT_TESTS
#endif
   }

   ~TempEnvVar() {
#ifdef HAVE_SETENV
      // Assume setenv and unsetenv come together.
      unsetenv(name);
#else
      (void)name; // Suppress -Wunused-private-field.
#endif
   }

private:
   const char *const name;
};

template <typename T, typename Base = cmd::Opt<T>>
class StackOption : public Base
{
public:
   template <class... Ts>
   explicit StackOption(Ts &&... Ms) : Base(std::forward<Ts>(Ms)...) {}

   ~StackOption() override { this->removeArgument(); }

   template <class DT> StackOption<T> &operator=(const DT &V) {
      this->setValue(V);
      return *this;
   }
};

class StackSubCommand : public cmd::SubCommand {
public:
   StackSubCommand(StringRef Name,
                   StringRef Description = StringRef())
      : SubCommand(Name, Description) {}

   StackSubCommand() : SubCommand() {}

   ~StackSubCommand() { unregisterSubCommand(); }
};


cmd::OptionCategory TestCategory("Test Options", "Description");

TEST(CommandLineTest, testModifyExisitingOption)
{
   StackOption<int> TestOption("test-option", cmd::Desc("old description"));

   static const char Description[] = "New description";
   static const char ArgString[] = "new-test-option";
   static const char ValueString[] = "Integer";

   StringMap<cmd::Option *> &Map =
         cmd::get_registered_options(*cmd::sg_topLevelSubCommand);

   ASSERT_TRUE(Map.count("test-option") == 1) << "Could not find option in map.";

   cmd::Option *Retrieved = Map["test-option"];
   ASSERT_EQ(&TestOption, Retrieved) << "Retrieved wrong option.";

   ASSERT_EQ(&cmd::sg_generalCategory,Retrieved->m_category) <<
                                                                "Incorrect default option category.";

   Retrieved->setCategory(TestCategory);
   ASSERT_EQ(&TestCategory,Retrieved->m_category) <<
                                                     "Failed to modify option's option category.";

   Retrieved->setDescription(Description);
   ASSERT_STREQ(Retrieved->m_helpStr.getData(), Description)
         << "Changing option description failed.";

   Retrieved->setArgStr(ArgString);
   ASSERT_STREQ(ArgString, Retrieved->m_argStr.getData())
         << "Failed to modify option's Argument string.";

   Retrieved->setValueStr(ValueString);
   ASSERT_STREQ(Retrieved->m_valueStr.getData(), ValueString)
         << "Failed to modify option's Value string.";

   Retrieved->setHiddenFlag(cmd::Hidden);
   ASSERT_EQ(cmd::Hidden, TestOption.getOptionHiddenFlag()) <<
                                                               "Failed to modify option's hidden flag.";
}
#ifndef SKIP_ENVIRONMENT_TESTS

const char test_env_var[] = "LLVM_TEST_COMMAND_LINE_FLAGS";

cmd::Opt<std::string> EnvironmentTestOption("env-test-opt");
TEST(CommandLineTest, testParseEnvironment) {
   TempEnvVar TEV(test_env_var, "-env-test-opt=hello");
   EXPECT_EQ("", EnvironmentTestOption);
   cmd::parse_environment_options("CommandLineTest", test_env_var);
   EXPECT_EQ("hello", EnvironmentTestOption);
}

// This test used to make valgrind complain
// ("Conditional jump or move depends on uninitialised value(s)")
//
// Warning: Do not run any tests after this one that try to gain access to
// registered command line options because this will likely result in a
// SEGFAULT. This can occur because the cmd::opt in the test below is declared
// on the stack which will be destroyed after the test completes but the
// command line system will still hold a pointer to a deallocated cmd::Option.
TEST(CommandLineTest, testParseEnvironmentToLocalVar) {
   // Put cmd::opt on stack to check for proper initialization of fields.
   StackOption<std::string> EnvironmentTestOptionLocal("env-test-opt-local");
   TempEnvVar TEV(test_env_var, "-env-test-opt-local=hello-local");
   EXPECT_EQ("", EnvironmentTestOptionLocal);
   cmd::parse_environment_options("CommandLineTest", test_env_var);
   EXPECT_EQ("hello-local", EnvironmentTestOptionLocal);
}

#endif  // SKIP_ENVIRONMENT_TESTS

TEST(CommandLineTest, testUseOptionCategory) {
   StackOption<int> TestOption2("test-option", cmd::Category(TestCategory));

   ASSERT_EQ(&TestCategory,TestOption2.m_category) << "Failed to assign Option "
                                                      "Category.";
}

typedef void ParserFunction(StringRef Source, StringSaver &Saver,
                            SmallVectorImpl<const char *> &NewArgv,
                            bool MarkEOLs);

void testCommandLineTokenizer(ParserFunction *parse, StringRef Input,
                              const char *const Output[], size_t OutputSize)
{
   SmallVector<const char *, 0> Actual;
   BumpPtrAllocator A;
   StringSaver Saver(A);
   parse(Input, Saver, Actual, /*MarkEOLs=*/false);
   EXPECT_EQ(OutputSize, Actual.size());
   for (unsigned I = 0, E = Actual.size(); I != E; ++I) {
      if (I < OutputSize) {
         EXPECT_STREQ(Output[I], Actual[I]);
      }
   }
}

TEST(CommandLineTest, testTokenizeGNUCommandLine)
{
   const char Input[] =
         "foo\\ bar \"foo bar\" \'foo bar\' 'foo\\\\bar' -DFOO=bar\\(\\) "
         "foo\"bar\"baz C:\\\\src\\\\foo.cpp \"C:\\src\\foo.cpp\"";
   const char *const Output[] = {
      "foo bar",     "foo bar",   "foo bar",          "foo\\bar",
      "-DFOO=bar()", "foobarbaz", "C:\\src\\foo.cpp", "C:srcfoo.cpp"};
   testCommandLineTokenizer(cmd::tokenize_gnu_command_line, Input, Output,
                            array_lengthof(Output));
}

TEST(CommandLineTest, testTokenizeWindowsCommandLine)
{
   const char Input[] = "a\\b c\\\\d e\\\\\"f g\" h\\\"i j\\\\\\\"k \"lmn\" o pqr "
                        "\"st \\\"u\" \\v";
   const char *const Output[] = { "a\\b", "c\\\\d", "e\\f g", "h\"i", "j\\\"k",
                                  "lmn", "o", "pqr", "st \"u", "\\v" };
   testCommandLineTokenizer(cmd::tokenize_windows_command_line, Input, Output,
                            array_lengthof(Output));
}

TEST(CommandLineTest, testTokenizeConfigFile1)
{
   const char *Input = "\\";
   const char *const Output[] = { "\\" };
   testCommandLineTokenizer(cmd::tokenize_config_file, Input, Output,
                            array_lengthof(Output));
}

TEST(CommandLineTest, testTokenizeConfigFile2)
{
   const char *Input = "\\abc";
   const char *const Output[] = { "abc" };
   testCommandLineTokenizer(cmd::tokenize_config_file, Input, Output,
                            array_lengthof(Output));
}

TEST(CommandLineTest, testTokenizeConfigFile3)
{
   const char *Input = "abc\\";
   const char *const Output[] = { "abc\\" };
   testCommandLineTokenizer(cmd::tokenize_config_file, Input, Output,
                            array_lengthof(Output));
}

TEST(CommandLineTest, testTokenizeConfigFile4)
{
   const char *Input = "abc\\\n123";
   const char *const Output[] = { "abc123" };
   testCommandLineTokenizer(cmd::tokenize_config_file, Input, Output,
                            array_lengthof(Output));
}

TEST(CommandLineTest, testTokenizeConfigFile5)
{
   const char *Input = "abc\\\r\n123";
   const char *const Output[] = { "abc123" };
   testCommandLineTokenizer(cmd::tokenize_config_file, Input, Output,
                            array_lengthof(Output));
}

TEST(CommandLineTest, testTokenizeConfigFile6)
{
   const char *Input = "abc\\\n";
   const char *const Output[] = { "abc" };
   testCommandLineTokenizer(cmd::tokenize_config_file, Input, Output,
                            array_lengthof(Output));
}

TEST(CommandLineTest, testTokenizeConfigFile7)
{
   const char *Input = "abc\\\r\n";
   const char *const Output[] = { "abc" };
   testCommandLineTokenizer(cmd::tokenize_config_file, Input, Output,
                            array_lengthof(Output));
}

TEST(CommandLineTest, testTokenizeConfigFile8)
{
   SmallVector<const char *, 0> Actual;
   BumpPtrAllocator A;
   StringSaver Saver(A);
   cmd::tokenize_config_file("\\\n", Saver, Actual, /*MarkEOLs=*/false);
   EXPECT_TRUE(Actual.empty());
}

TEST(CommandLineTest, testTokenizeConfigFile9)
{
   SmallVector<const char *, 0> Actual;
   BumpPtrAllocator A;
   StringSaver Saver(A);
   cmd::tokenize_config_file("\\\r\n", Saver, Actual, /*MarkEOLs=*/false);
   EXPECT_TRUE(Actual.empty());
}

TEST(CommandLineTest, testTokenizeConfigFile10)
{
   const char *Input = "\\\nabc";
   const char *const Output[] = { "abc" };
   testCommandLineTokenizer(cmd::tokenize_config_file, Input, Output,
                            array_lengthof(Output));
}

TEST(CommandLineTest, testTokenizeConfigFile11)
{
   const char *Input = "\\\r\nabc";
   const char *const Output[] = { "abc" };
   testCommandLineTokenizer(cmd::tokenize_config_file, Input, Output,
                            array_lengthof(Output));
}

TEST(CommandLineTest, testAliasesWithArguments)
{
   static const size_t ARGC = 3;
   const char *const Inputs[][ARGC] = {
      { "-tool", "-actual=x", "-extra" },
      { "-tool", "-actual", "x" },
      { "-tool", "-alias=x", "-extra" },
      { "-tool", "-alias", "x" }
   };

   for (size_t i = 0, e = array_lengthof(Inputs); i < e; ++i) {
      StackOption<std::string> Actual("actual");
      StackOption<bool> Extra("extra");
      StackOption<std::string> Input(cmd::Positional);

      cmd::Alias Alias("alias", cmd::AliasOpt(Actual));

      cmd::parse_commandline_options(ARGC, Inputs[i]);
      EXPECT_EQ("x", Actual);
      EXPECT_EQ(0, Input.getNumOccurrences());

      Alias.removeArgument();
   }
}

void testAliasRequired(int argc, const char *const *argv)
{
   StackOption<std::string> Option("option", cmd::Required);
   cmd::Alias Alias("o", cmd::AliasOpt(Option));

   cmd::parse_commandline_options(argc, argv);
   EXPECT_EQ("x", Option);
   EXPECT_EQ(1, Option.getNumOccurrences());

   Alias.removeArgument();
}

TEST(CommandLineTest, AliasRequired)
{
   const char *opts1[] = { "-tool", "-option=x" };
   const char *opts2[] = { "-tool", "-o", "x" };
   testAliasRequired(array_lengthof(opts1), opts1);
   testAliasRequired(array_lengthof(opts2), opts2);
}

TEST(CommandLineTest, HideUnrelatedOptions)
{
   StackOption<int> TestOption1("hide-option-1");
   StackOption<int> TestOption2("hide-option-2", cmd::Category(TestCategory));

   cmd::hide_unrelated_options(TestCategory);

   ASSERT_EQ(cmd::ReallyHidden, TestOption1.getOptionHiddenFlag())
         << "Failed to hide extra option.";
   ASSERT_EQ(cmd::NotHidden, TestOption2.getOptionHiddenFlag())
         << "Hid extra option that should be visable.";

   StringMap<cmd::Option *> &Map =
         cmd::get_registered_options(*cmd::sg_topLevelSubCommand);
   ASSERT_EQ(cmd::NotHidden, Map["help"]->getOptionHiddenFlag())
         << "Hid default option that should be visable.";
}

cmd::OptionCategory TestCategory2("Test Options set 2", "Description");

TEST(CommandLineTest, HideUnrelatedOptionsMulti)
{
   StackOption<int> TestOption1("multi-hide-option-1");
   StackOption<int> TestOption2("multi-hide-option-2", cmd::Category(TestCategory));
   StackOption<int> TestOption3("multi-hide-option-3", cmd::Category(TestCategory2));

   const cmd::OptionCategory *VisibleCategories[] = {&TestCategory,
                                                     &TestCategory2};

   cmd::hide_unrelated_options(make_array_ref(VisibleCategories));

   ASSERT_EQ(cmd::ReallyHidden, TestOption1.getOptionHiddenFlag())
         << "Failed to hide extra option.";
   ASSERT_EQ(cmd::NotHidden, TestOption2.getOptionHiddenFlag())
         << "Hid extra option that should be visable.";
   ASSERT_EQ(cmd::NotHidden, TestOption3.getOptionHiddenFlag())
         << "Hid extra option that should be visable.";

   StringMap<cmd::Option *> &Map =
         cmd::get_registered_options(*cmd::sg_topLevelSubCommand);
   ASSERT_EQ(cmd::NotHidden, Map["help"]->getOptionHiddenFlag())
         << "Hid default option that should be visable.";
}

TEST(CommandLineTest, testSetValueInSubcategories)
{
   cmd::reset_command_line_parser();

   StackSubCommand SC1("sc1", "First subcommand");
   StackSubCommand SC2("sc2", "Second subcommand");

   StackOption<bool> TopLevelOpt("top-level", cmd::init(false));
   StackOption<bool> SC1Opt("sc1", cmd::Sub(SC1), cmd::init(false));
   StackOption<bool> SC2Opt("sc2", cmd::Sub(SC2), cmd::init(false));

   EXPECT_FALSE(TopLevelOpt);
   EXPECT_FALSE(SC1Opt);
   EXPECT_FALSE(SC2Opt);
   const char *args[] = {"prog", "-top-level"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args, StringRef(), &null_stream()));
   EXPECT_TRUE(TopLevelOpt);
   EXPECT_FALSE(SC1Opt);
   EXPECT_FALSE(SC2Opt);

   TopLevelOpt = false;

   cmd::reset_all_option_occurrences();
   EXPECT_FALSE(TopLevelOpt);
   EXPECT_FALSE(SC1Opt);
   EXPECT_FALSE(SC2Opt);
   const char *args2[] = {"prog", "sc1", "-sc1"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(3, args2, StringRef(), &null_stream()));
   EXPECT_FALSE(TopLevelOpt);
   EXPECT_TRUE(SC1Opt);
   EXPECT_FALSE(SC2Opt);

   SC1Opt = false;

   cmd::reset_all_option_occurrences();
   EXPECT_FALSE(TopLevelOpt);
   EXPECT_FALSE(SC1Opt);
   EXPECT_FALSE(SC2Opt);
   const char *args3[] = {"prog", "sc2", "-sc2"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(3, args3, StringRef(), &null_stream()));
   EXPECT_FALSE(TopLevelOpt);
   EXPECT_FALSE(SC1Opt);
   EXPECT_TRUE(SC2Opt);
}

TEST(CommandLineTest, testLookupFailsInWrongSubCommand)
{
   cmd::reset_command_line_parser();

   StackSubCommand SC1("sc1", "First subcommand");
   StackSubCommand SC2("sc2", "Second subcommand");

   StackOption<bool> SC1Opt("sc1", cmd::Sub(SC1), cmd::init(false));
   StackOption<bool> SC2Opt("sc2", cmd::Sub(SC2), cmd::init(false));

   std::string Errs;
   RawStringOutStream outstream(Errs);

   const char *args[] = {"prog", "sc1", "-sc2"};
   EXPECT_FALSE(cmd::parse_commandline_options(3, args, StringRef(), &outstream));
   outstream.flush();
   EXPECT_FALSE(Errs.empty());
}

TEST(CommandLineTest, testAddToAllSubCommands)
{
   cmd::reset_command_line_parser();

   StackSubCommand SC1("sc1", "First subcommand");
   StackOption<bool> AllOpt("everywhere", cmd::Sub(*cmd::sg_allSubCommands),
                            cmd::init(false));
   StackSubCommand SC2("sc2", "Second subcommand");

   const char *args[] = {"prog", "-everywhere"};
   const char *args2[] = {"prog", "sc1", "-everywhere"};
   const char *args3[] = {"prog", "sc2", "-everywhere"};

   std::string Errs;
   RawStringOutStream outstream(Errs);

   EXPECT_FALSE(AllOpt);
   EXPECT_TRUE(cmd::parse_commandline_options(2, args, StringRef(), &outstream));
   EXPECT_TRUE(AllOpt);

   AllOpt = false;

   cmd::reset_all_option_occurrences();
   EXPECT_FALSE(AllOpt);
   EXPECT_TRUE(cmd::parse_commandline_options(3, args2, StringRef(), &outstream));
   EXPECT_TRUE(AllOpt);

   AllOpt = false;

   cmd::reset_all_option_occurrences();
   EXPECT_FALSE(AllOpt);
   EXPECT_TRUE(cmd::parse_commandline_options(3, args3, StringRef(), &outstream));
   EXPECT_TRUE(AllOpt);

   // Since all parsing succeeded, the error message should be empty.
   outstream.flush();
   EXPECT_TRUE(Errs.empty());
}

TEST(CommandLineTest, ReparseCommandLineOptions)
{
   cmd::reset_command_line_parser();

   StackOption<bool> TopLevelOpt("top-level", cmd::Sub(*cmd::sg_topLevelSubCommand),
                                 cmd::init(false));

   const char *args[] = {"prog", "-top-level"};

   EXPECT_FALSE(TopLevelOpt);
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args, StringRef(), &null_stream()));
   EXPECT_TRUE(TopLevelOpt);

   TopLevelOpt = false;

   cmd::reset_all_option_occurrences();
   EXPECT_FALSE(TopLevelOpt);
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args, StringRef(), &null_stream()));
   EXPECT_TRUE(TopLevelOpt);
}

TEST(CommandLineTest, RemoveFromRegularSubCommand)
{
   cmd::reset_command_line_parser();

   StackSubCommand SC("sc", "Subcommand");
   StackOption<bool> RemoveOption("remove-option", cmd::Sub(SC), cmd::init(false));
   StackOption<bool> KeepOption("keep-option", cmd::Sub(SC), cmd::init(false));

   const char *args[] = {"prog", "sc", "-remove-option"};

   std::string Errs;
   RawStringOutStream outstream(Errs);

   EXPECT_FALSE(RemoveOption);
   EXPECT_TRUE(cmd::parse_commandline_options(3, args, StringRef(), &outstream));
   EXPECT_TRUE(RemoveOption);
   outstream.flush();
   EXPECT_TRUE(Errs.empty());

   RemoveOption.removeArgument();

   cmd::reset_all_option_occurrences();
   EXPECT_FALSE(cmd::parse_commandline_options(3, args, StringRef(), &outstream));
   outstream.flush();
   EXPECT_FALSE(Errs.empty());
}

TEST(CommandLineTest, RemoveFromTopLevelSubCommand)
{
   cmd::reset_command_line_parser();

   StackOption<bool> TopLevelRemove(
            "top-level-remove", cmd::Sub(*cmd::sg_topLevelSubCommand), cmd::init(false));
   StackOption<bool> TopLevelKeep(
            "top-level-keep", cmd::Sub(*cmd::sg_topLevelSubCommand), cmd::init(false));

   const char *args[] = {"prog", "-top-level-remove"};

   EXPECT_FALSE(TopLevelRemove);
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args, StringRef(), &null_stream()));
   EXPECT_TRUE(TopLevelRemove);

   TopLevelRemove.removeArgument();

   cmd::reset_all_option_occurrences();
   EXPECT_FALSE(
            cmd::parse_commandline_options(2, args, StringRef(), &null_stream()));
}

TEST(CommandLineTest, RemoveFromAllSubCommands)
{
   cmd::reset_command_line_parser();

   StackSubCommand SC1("sc1", "First Subcommand");
   StackSubCommand SC2("sc2", "Second Subcommand");
   StackOption<bool> RemoveOption("remove-option", cmd::Sub(*cmd::sg_allSubCommands),
                                  cmd::init(false));
   StackOption<bool> KeepOption("keep-option", cmd::Sub(*cmd::sg_allSubCommands),
                                cmd::init(false));

   const char *args0[] = {"prog", "-remove-option"};
   const char *args1[] = {"prog", "sc1", "-remove-option"};
   const char *args2[] = {"prog", "sc2", "-remove-option"};

   // It should work for all subcommands including the top-level.
   EXPECT_FALSE(RemoveOption);
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args0, StringRef(), &null_stream()));
   EXPECT_TRUE(RemoveOption);

   RemoveOption = false;

   cmd::reset_all_option_occurrences();
   EXPECT_FALSE(RemoveOption);
   EXPECT_TRUE(
            cmd::parse_commandline_options(3, args1, StringRef(), &null_stream()));
   EXPECT_TRUE(RemoveOption);

   RemoveOption = false;

   cmd::reset_all_option_occurrences();
   EXPECT_FALSE(RemoveOption);
   EXPECT_TRUE(
            cmd::parse_commandline_options(3, args2, StringRef(), &null_stream()));
   EXPECT_TRUE(RemoveOption);

   RemoveOption.removeArgument();

   // It should not work for any subcommands including the top-level.
   cmd::reset_all_option_occurrences();
   EXPECT_FALSE(
            cmd::parse_commandline_options(2, args0, StringRef(), &null_stream()));
   cmd::reset_all_option_occurrences();
   EXPECT_FALSE(
            cmd::parse_commandline_options(3, args1, StringRef(), &null_stream()));
   cmd::reset_all_option_occurrences();
   EXPECT_FALSE(
            cmd::parse_commandline_options(3, args2, StringRef(), &null_stream()));
}

TEST(CommandLineTest, GetRegisteredSubcommands)
{
   cmd::reset_command_line_parser();

   StackSubCommand SC1("sc1", "First Subcommand");
   StackOption<bool> Opt1("opt1", cmd::Sub(SC1), cmd::init(false));
   StackSubCommand SC2("sc2", "Second subcommand");
   StackOption<bool> Opt2("opt2", cmd::Sub(SC2), cmd::init(false));

   const char *args0[] = {"prog", "sc1"};
   const char *args1[] = {"prog", "sc2"};

   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args0, StringRef(), &null_stream()));
   EXPECT_FALSE(Opt1);
   EXPECT_FALSE(Opt2);
   for (auto *S : cmd::get_registered_subcommands()) {
      if (*S) {
         EXPECT_EQ("sc1", S->getName());
      }
   }

   cmd::reset_all_option_occurrences();
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args1, StringRef(), &null_stream()));
   EXPECT_FALSE(Opt1);
   EXPECT_FALSE(Opt2);
   for (auto *S : cmd::get_registered_subcommands()) {
      if (*S) {
         EXPECT_EQ("sc2", S->getName());
      }
   }
}

TEST(CommandLineTest, ArgumentLimit)
{
   std::string args(32 * 4096, 'a');
   EXPECT_FALSE(sys::commandline_fits_within_system_limits("cmd", args.data()));
}

TEST(CommandLineTest, ResponseFileWindows)
{
   if (!polar::basic::Triple(sys::get_process_triple()).isOSWindows())
      return;

   StackOption<std::string, cmd::List<std::string>> InputFilenames(
            cmd::Positional, cmd::Desc("<input files>"), cmd::ZeroOrMore);
   StackOption<bool> TopLevelOpt("top-level", cmd::init(false));

   // Create response file.
   int FileDescriptor;
   SmallString<64> TempPath;
   std::error_code EC =
         polar::fs::create_temporary_file("resp-", ".txt", FileDescriptor, TempPath);
   EXPECT_TRUE(!EC);

   std::ofstream RspFile(TempPath.getCStr());
   EXPECT_TRUE(RspFile.is_open());
   RspFile << "-top-level\npath\\dir\\file1\npath/dir/file2";
   RspFile.close();

   SmallString<128> RspOpt;
   RspOpt.append(1, '@');
   RspOpt.append(TempPath.getCStr());
   const char *args[] = {"prog", RspOpt.getCStr()};
   EXPECT_FALSE(TopLevelOpt);
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args, StringRef(), &polar::utils::null_stream()));
   EXPECT_TRUE(TopLevelOpt);
   EXPECT_TRUE(InputFilenames[0] == "path\\dir\\file1");
   EXPECT_TRUE(InputFilenames[1] == "path/dir/file2");

   polar::fs::remove(TempPath.getCStr());
}

TEST(CommandLineTest, testResponseFiles)
{
   SmallString<128> TestDir;
   std::error_code EC =
         fs::create_unique_directory("unittest", TestDir);
   EXPECT_TRUE(!EC);

   // Create included response file of first level.
   SmallString<128> IncludedFileName;
   fs::path::append(IncludedFileName, TestDir, "resp1");
   std::ofstream IncludedFile(IncludedFileName.getCStr());
   EXPECT_TRUE(IncludedFile.is_open());
   IncludedFile << "-option_1 -option_2\n"
                   "@incdir/resp2\n"
                   "-option_3=abcd\n";
   IncludedFile.close();

   // Directory for included file.
   SmallString<128> IncDir;
   fs::path::append(IncDir, TestDir, "incdir");
   EC = fs::create_directory(IncDir);
   EXPECT_TRUE(!EC);

   // Create included response file of second level.
   SmallString<128> IncludedFileName2;
   fs::path::append(IncludedFileName2, IncDir, "resp2");
   std::ofstream IncludedFile2(IncludedFileName2.getCStr());
   EXPECT_TRUE(IncludedFile2.is_open());
   IncludedFile2 << "-option_21 -option_22\n";
   IncludedFile2 << "-option_23=abcd\n";
   IncludedFile2.close();

   // Prepare 'file' with reference to response file.
   SmallString<128> IncRef;
   IncRef.append(1, '@');
   IncRef.append(IncludedFileName.getCStr());
   SmallVector<const char *, 4> Argv =
   { "test/test", "-flag_1", IncRef.getCStr(), "-flag_2" };

   // Expand response files.
   BumpPtrAllocator A;
   StringSaver Saver(A);
   bool Res = cmd::expand_response_files(
            Saver, cmd::tokenize_gnu_command_line, Argv, false, true);
   EXPECT_TRUE(Res);
   EXPECT_EQ(Argv.size(), 9U);
   EXPECT_STREQ(Argv[0], "test/test");
   EXPECT_STREQ(Argv[1], "-flag_1");
   EXPECT_STREQ(Argv[2], "-option_1");
   EXPECT_STREQ(Argv[3], "-option_2");
   EXPECT_STREQ(Argv[4], "-option_21");
   EXPECT_STREQ(Argv[5], "-option_22");
   EXPECT_STREQ(Argv[6], "-option_23=abcd");
   EXPECT_STREQ(Argv[7], "-option_3=abcd");
   EXPECT_STREQ(Argv[8], "-flag_2");

   fs::remove(IncludedFileName2);
   fs::remove(IncDir);
   fs::remove(IncludedFileName);
   fs::remove(TestDir);
}

TEST(CommandLineTest, SetDefautValue) {
   cmd::reset_command_line_parser();

   StackOption<std::string> Opt1("opt1", cmd::init("true"));
   StackOption<bool> Opt2("opt2", cmd::init(true));
   cmd::Alias Alias("alias", cmd::AliasOpt(Opt2));
   StackOption<int> Opt3("opt3", cmd::init(3));

   const char *args[] = {"prog", "-opt1=false", "-opt2", "-opt3"};

   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args, StringRef(), &null_stream()));

   EXPECT_TRUE(Opt1 == "false");
   EXPECT_TRUE(Opt2);
   EXPECT_TRUE(Opt3 == 3);

   Opt2 = false;
   Opt3 = 1;

   cmd::reset_all_option_occurrences();

   for (auto &OM : cmd::get_registered_options(*cmd::sg_topLevelSubCommand)) {
      cmd::Option *O = OM.m_second;
      if (O->m_argStr == "opt2") {
         continue;
      }
      O->setDefault();
   }

   EXPECT_TRUE(Opt1 == "true");
   EXPECT_TRUE(Opt2);
   EXPECT_TRUE(Opt3 == 3);
}

TEST(CommandLineTest, ReadConfigFile)
{
   SmallVector<const char *, 1> Argv;

   SmallString<128> TestDir;
   std::error_code EC =
         fs::create_unique_directory("unittest", TestDir);
   EXPECT_TRUE(!EC);

   SmallString<128> TestCfg;
   fs::path::append(TestCfg, TestDir, "foo");
   std::ofstream ConfigFile(TestCfg.getCStr());
   EXPECT_TRUE(ConfigFile.is_open());
   ConfigFile << "# Comment\n"
                 "-option_1\n"
                 "@subconfig\n"
                 "-option_3=abcd\n"
                 "-option_4=\\\n"
                 "cdef\n";
   ConfigFile.close();

   SmallString<128> TestCfg2;
   fs::path::append(TestCfg2, TestDir, "subconfig");
   std::ofstream ConfigFile2(TestCfg2.getCStr());
   EXPECT_TRUE(ConfigFile2.is_open());
   ConfigFile2 << "-option_2\n"
                  "\n"
                  "   # comment\n";
   ConfigFile2.close();

   // Make sure the current directory is not the directory where config files
   // resides. In this case the code that expands response files will not find
   // 'subconfig' unless it resolves nested inclusions relative to the including
   // file.
   SmallString<128> CurrDir;
   EC = fs::current_path(CurrDir);
   EXPECT_TRUE(!EC);
   EXPECT_TRUE(StringRef(CurrDir) != StringRef(TestDir));

   BumpPtrAllocator A;
   StringSaver Saver(A);
   bool Result = cmd::read_config_file(TestCfg, Saver, Argv);

   EXPECT_TRUE(Result);
   EXPECT_EQ(Argv.size(), 4U);
   EXPECT_STREQ(Argv[0], "-option_1");
   EXPECT_STREQ(Argv[1], "-option_2");
   EXPECT_STREQ(Argv[2], "-option_3=abcd");
   EXPECT_STREQ(Argv[3], "-option_4=cdef");

   fs::remove(TestCfg2);
   fs::remove(TestCfg);
   fs::remove(TestDir);
}


#ifdef _WIN32
TEST(CommandLineTest, testGetCommandLineArguments) {
   int argc = __argc;
   char **argv = __argv;

   // GetCommandLineArguments is called in InitLLVM.
   polar::InitPolar X(argc, argv);

   EXPECT_EQ(polar::fs::path::is_absolute(argv[0]),
         polar::fs::path::is_absolute(__argv[0]));

   EXPECT_TRUEpolar::fs::path::filename(argv[0])
         .equals_lower("supporttests.exe"))
         << "Filename of test executable is "
         << polar::fs::path::filename(argv[0]);
}
#endif

} // anonymous namespace
