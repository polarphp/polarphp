//===- llvm/unittest/Support/CommandLineTest.cpp - CommandLine tests ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/10.

#include "polarphp/utils/CommandLine.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/Path.h"
#include "polarphp/utils/Host.h"
#include "polarphp/utils/Program.h"
#include "polarphp/utils/MemoryBuffer.h"
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

   ASSERT_TRUE(Map.count("test-option") == 1) <<
                                                 "Could not find option in map.";

   cmd::Option *retrieved = Map["test-option"];
   ASSERT_EQ(&TestOption, retrieved) << "retrieved wrong option.";

   ASSERT_NE(retrieved->categories.end(),
             find_if(retrieved->categories,
                     [&](const cmd::OptionCategory *category) {
      return category == &cmd::sg_generalCategory;
   }))
         << "Incorrect default option category.";

   retrieved->addCategory(TestCategory);
   ASSERT_NE(retrieved->categories.end(),
             find_if(retrieved->categories,
                     [&](const cmd::OptionCategory *category) {
      return category == &TestCategory;
   }))
         << "Failed to modify option's option category.";

   retrieved->setDescription(Description);
   ASSERT_STREQ(retrieved->helpStr.data(), Description)
         << "Changing option description failed.";

   retrieved->setArgStr(ArgString);
   ASSERT_STREQ(ArgString, retrieved->argStr.data())
         << "Failed to modify option's Argument string.";

   retrieved->setValueStr(ValueString);
   ASSERT_STREQ(retrieved->valueStr.data(), ValueString)
         << "Failed to modify option's Value string.";

   retrieved->setHiddenFlag(cmd::Hidden);
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

   ASSERT_NE(TestOption2.categories.end(),
             find_if(TestOption2.categories,
                     [&](const cmd::OptionCategory *Category) {
      return Category == &TestCategory;
   }))
         << "Failed to assign Option Category.";
}

TEST(CommandLineTest, testUseMultipleCategories) {
   StackOption<int> TestOption2("test-option2", cmd::Category(TestCategory),
                                cmd::Category(cmd::sg_generalCategory),
                                cmd::Category(cmd::sg_generalCategory));

   // Make sure cmd::sg_generalCategory wasn't added twice.
   ASSERT_EQ(TestOption2.categories.size(), 2U);

   ASSERT_NE(TestOption2.categories.end(),
             find_if(TestOption2.categories,
                     [&](const cmd::OptionCategory *category) {
      return category == &TestCategory;
   }))
         << "Failed to assign Option Category.";
   ASSERT_NE(TestOption2.categories.end(),
             find_if(TestOption2.categories,
                     [&](const cmd::OptionCategory *category) {
      return category == &cmd::sg_generalCategory;
   }))
         << "Failed to assign General Category.";

   cmd::OptionCategory AnotherCategory("Additional test Options", "Description");
   StackOption<int> TestOption("test-option", cmd::Category(TestCategory),
                               cmd::Category(AnotherCategory));
   ASSERT_EQ(TestOption.categories.end(),
             find_if(TestOption.categories,
                     [&](const cmd::OptionCategory *category) {
      return category == &cmd::sg_generalCategory;
   }))
         << "Failed to remove General Category.";
   ASSERT_NE(TestOption.categories.end(),
             find_if(TestOption.categories,
                     [&](const cmd::OptionCategory *category) {
      return category == &TestCategory;
   }))
         << "Failed to assign Option Category.";
   ASSERT_NE(TestOption.categories.end(),
             find_if(TestOption.categories,
                     [&](const cmd::OptionCategory *category) {
      return category == &AnotherCategory;
   }))
         << "Failed to assign Another Category.";
}

typedef void ParserFunction(StringRef Source, StringSaver &saver,
                            SmallVectorImpl<const char *> &NewArgv,
                            bool MarkEOLs);

void testCommandLineTokenizer(ParserFunction *parse, StringRef Input,
                              const char *const Output[], size_t OutputSize)
{
   SmallVector<const char *, 0> Actual;
   BumpPtrAllocator A;
   StringSaver saver(A);
   parse(Input, saver, Actual, /*MarkEOLs=*/false);
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

TEST(CommandLineTest, testTokenizeWindowsCommandLine1)
{
   const char Input[] = "a\\b c\\\\d e\\\\\"f g\" h\\\"i j\\\\\\\"k \"lmn\" o pqr "
                        "\"st \\\"u\" \\v";
   const char *const Output[] = { "a\\b", "c\\\\d", "e\\f g", "h\"i", "j\\\"k",
                                  "lmn", "o", "pqr", "st \"u", "\\v" };
   testCommandLineTokenizer(cmd::tokenize_windows_command_line, Input, Output,
                            array_lengthof(Output));
}

TEST(CommandLineTest, TokenizeWindowsCommandLine2) {
   const char Input[] = "clang -c -DFOO=\"\"\"ABC\"\"\" x.cpp";
   const char *const Output[] = { "clang", "-c", "-DFOO=\"ABC\"", "x.cpp"};
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
   StringSaver saver(A);
   cmd::tokenize_config_file("\\\n", saver, Actual, /*MarkEOLs=*/false);
   EXPECT_TRUE(Actual.empty());
}

TEST(CommandLineTest, testTokenizeConfigFile9)
{
   SmallVector<const char *, 0> Actual;
   BumpPtrAllocator A;
   StringSaver saver(A);
   cmd::tokenize_config_file("\\\r\n", saver, Actual, /*MarkEOLs=*/false);
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

   std::string errors;
   RawStringOutStream out_streamtream(errors);

   const char *args[] = {"prog", "sc1", "-sc2"};
   EXPECT_FALSE(cmd::parse_commandline_options(3, args, StringRef(), &out_streamtream));
   out_streamtream.flush();
   EXPECT_FALSE(errors.empty());
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

   std::string errors;
   RawStringOutStream out_streamtream(errors);

   EXPECT_FALSE(AllOpt);
   EXPECT_TRUE(cmd::parse_commandline_options(2, args, StringRef(), &out_streamtream));
   EXPECT_TRUE(AllOpt);

   AllOpt = false;

   cmd::reset_all_option_occurrences();
   EXPECT_FALSE(AllOpt);
   EXPECT_TRUE(cmd::parse_commandline_options(3, args2, StringRef(), &out_streamtream));
   EXPECT_TRUE(AllOpt);

   AllOpt = false;

   cmd::reset_all_option_occurrences();
   EXPECT_FALSE(AllOpt);
   EXPECT_TRUE(cmd::parse_commandline_options(3, args3, StringRef(), &out_streamtream));
   EXPECT_TRUE(AllOpt);

   // Since all parsing succeeded, the error message should be empty.
   out_streamtream.flush();
   EXPECT_TRUE(errors.empty());
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

   std::string errors;
   RawStringOutStream out_streamtream(errors);

   EXPECT_FALSE(RemoveOption);
   EXPECT_TRUE(cmd::parse_commandline_options(3, args, StringRef(), &out_streamtream));
   EXPECT_TRUE(RemoveOption);
   out_streamtream.flush();
   EXPECT_TRUE(errors.empty());

   RemoveOption.removeArgument();

   cmd::reset_all_option_occurrences();
   EXPECT_FALSE(cmd::parse_commandline_options(3, args, StringRef(), &out_streamtream));
   out_streamtream.flush();
   EXPECT_FALSE(errors.empty());
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

TEST(CommandLineTest, testGetRegisteredSubcommands)
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

TEST(CommandLineTest, testDefaultOptions) {
   cmd::reset_command_line_parser();

   StackOption<std::string> Bar("bar", cmd::Sub(*cmd::sg_allSubCommands),
                                cmd::DefaultOption);
   StackOption<std::string, cmd::Alias> Bar_Alias(
            "b", cmd::Desc("Alias for -bar"), cmd::AliasOpt(Bar), cmd::DefaultOption);

   StackOption<bool> Foo("foo", cmd::init(false), cmd::Sub(*cmd::sg_allSubCommands),
                         cmd::DefaultOption);
   StackOption<bool, cmd::Alias> Foo_Alias("f", cmd::Desc("Alias for -foo"),
                                           cmd::AliasOpt(Foo), cmd::DefaultOption);

   StackSubCommand SC1("sc1", "First Subcommand");
   // Override "-b" and change type in sc1 SubCommand.
   StackOption<bool> SC1_B("b", cmd::Sub(SC1), cmd::init(false));
   StackSubCommand SC2("sc2", "Second subcommand");
   // Override "-foo" and change type in sc2 SubCommand.  Note that this does not
   // affect "-f" alias, which continues to work correctly.
   StackOption<std::string> SC2_Foo("foo", cmd::Sub(SC2));

   const char *args0[] = {"prog", "-b", "args0 bar string", "-f"};
   EXPECT_TRUE(cmd::parse_commandline_options(sizeof(args0) / sizeof(char *), args0,
                                              StringRef(), &null_stream()));
   EXPECT_TRUE(Bar == "args0 bar string");
   EXPECT_TRUE(Foo);
   EXPECT_FALSE(SC1_B);
   EXPECT_TRUE(SC2_Foo.empty());

   cmd::reset_all_option_occurrences();

   const char *args1[] = {"prog", "sc1", "-b", "-bar", "args1 bar string", "-f"};
   EXPECT_TRUE(cmd::parse_commandline_options(sizeof(args1) / sizeof(char *), args1,
                                              StringRef(), &null_stream()));
   EXPECT_TRUE(Bar == "args1 bar string");
   EXPECT_TRUE(Foo);
   EXPECT_TRUE(SC1_B);
   EXPECT_TRUE(SC2_Foo.empty());
   for (auto *S : cmd::get_registered_subcommands()) {
      if (*S) {
         EXPECT_EQ("sc1", S->getName());
      }
   }

   cmd::reset_all_option_occurrences();

   const char *args2[] = {"prog", "sc2", "-b", "args2 bar string",
                          "-f", "-foo", "foo string"};
   EXPECT_TRUE(cmd::parse_commandline_options(sizeof(args2) / sizeof(char *), args2,
                                              StringRef(), &null_stream()));
   EXPECT_TRUE(Bar == "args2 bar string");
   EXPECT_TRUE(Foo);
   EXPECT_FALSE(SC1_B);
   EXPECT_TRUE(SC2_Foo == "foo string");
   for (auto *S : cmd::get_registered_subcommands()) {
      if (*S) {
         EXPECT_EQ("sc2", S->getName());
      }
   }
   cmd::reset_command_line_parser();
}

////TEST(CommandLineTest, testArgumentLimit)
////{
////   std::string args(32 * 4096, 'a');
////   EXPECT_FALSE(sys::commandline_fits_within_system_limits("cmd", args.data()));
////}

////TEST(CommandLineTest, testResponseFileWindows)
////{
////   if (!polar::basic::Triple(sys::get_process_triple()).isOSWindows())
////      return;

////   StackOption<std::string, cmd::List<std::string>> InputFilenames(
////            cmd::Positional, cmd::Desc("<input files>"), cmd::ZeroOrMore);
////   StackOption<bool> TopLevelOpt("top-level", cmd::init(false));

////   // Create response file.
////   int FileDescriptor;
////   SmallString<64> TempPath;
////   std::error_code errorCode =
////         polar::fs::create_temporary_file("resp-", ".txt", FileDescriptor, TempPath);
////   EXPECT_TRUE(!errorCode);

////   std::ofstream RspFile(TempPath.getCStr());
////   EXPECT_TRUE(RspFile.is_open());
////   RspFile << "-top-level\npath\\dir\\file1\npath/dir/file2";
////   RspFile.close();

////   SmallString<128> RspOpt;
////   RspOpt.append(1, '@');
////   RspOpt.append(TempPath.getCStr());
////   const char *args[] = {"prog", RspOpt.getCStr()};
////   EXPECT_FALSE(TopLevelOpt);
////   EXPECT_TRUE(
////            cmd::parse_commandline_options(2, args, StringRef(), &polar::utils::null_stream()));
////   EXPECT_TRUE(TopLevelOpt);
////   EXPECT_TRUE(InputFilenames[0] == "path\\dir\\file1");
////   EXPECT_TRUE(InputFilenames[1] == "path/dir/file2");

////   polar::fs::remove(TempPath.getCStr());
////}

////TEST(CommandLineTest, testResponseFiles)
////{
////   SmallString<128> testDir;
////   std::error_code errorCode =
////         fs::create_unique_directory("unittest", testDir);
////   EXPECT_TRUE(!errorCode);

////   // Create included response file of first level.
////   SmallString<128> includedFileName;
////   fs::path::append(includedFileName, testDir, "resp1");
////   std::ofstream IncludedFile(includedFileName.getCStr());
////   EXPECT_TRUE(IncludedFile.is_open());
////   IncludedFile << "-option_1 -option_2\n"
////                   "@incdir/resp2\n"
////                   "-option_3=abcd\n"
////                   "@incdir/resp3\n"
////                   "-option_4=efjk\n";
////   IncludedFile.close();

////   // Directory for included file.
////   SmallString<128> incDir;
////   fs::path::append(incDir, testDir, "incdir");
////   errorCode = fs::create_directory(incDir);
////   EXPECT_TRUE(!errorCode);

////   // Create included response file of second level.
////   SmallString<128> includedFileName2;
////   fs::path::append(includedFileName2, incDir, "resp2");
////   std::ofstream includedFile2(includedFileName2.getCStr());
////   EXPECT_TRUE(includedFile2.is_open());
////   includedFile2 << "-option_21 -option_22\n";
////   includedFile2 << "-option_23=abcd\n";
////   includedFile2.close();

////   // Create second included response file of second level.
////   SmallString<128> includedFileName3;
////   fs::path::append(includedFileName3, incDir, "resp3");
////   std::ofstream IncludedFile3(includedFileName3.getCStr());
////   EXPECT_TRUE(IncludedFile3.is_open());
////   IncludedFile3 << "-option_31 -option_32\n";
////   IncludedFile3 << "-option_33=abcd\n";
////   IncludedFile3.close();

////   // Prepare 'file' with reference to response file.
////   SmallString<128> IncRef;
////   IncRef.append(1, '@');
////   IncRef.append(includedFileName.getCStr());
////   SmallVector<const char *, 4> argv =
////   { "test/test", "-flag_1", IncRef.getCStr(), "-flag_2" };

////   // Expand response files.
////   BumpPtrAllocator A;
////   StringSaver saver(A);
////   bool res = cmd::expand_response_files(
////            saver, cmd::tokenize_gnu_command_line, argv, false, true);
////   EXPECT_TRUE(res);
////   EXPECT_EQ(argv.size(), 13U);
////   EXPECT_STREQ(argv[0], "test/test");
////   EXPECT_STREQ(argv[1], "-flag_1");
////   EXPECT_STREQ(argv[2], "-option_1");
////   EXPECT_STREQ(argv[3], "-option_2");
////   EXPECT_STREQ(argv[4], "-option_21");
////   EXPECT_STREQ(argv[5], "-option_22");
////   EXPECT_STREQ(argv[6], "-option_23=abcd");
////   EXPECT_STREQ(argv[7], "-option_3=abcd");
////   EXPECT_STREQ(argv[8], "-option_31");
////   EXPECT_STREQ(argv[9], "-option_32");
////   EXPECT_STREQ(argv[10], "-option_33=abcd");
////   EXPECT_STREQ(argv[11], "-option_4=efjk");
////   EXPECT_STREQ(argv[12], "-flag_2");

////   fs::remove(includedFileName3);
////   fs::remove(includedFileName2);
////   fs::remove(incDir);
////   fs::remove(includedFileName);
////   fs::remove(testDir);
////}

////TEST(CommandLineTest, testRecursiveResponseFiles) {
////   SmallString<128> testDir;
////   std::error_code errorCode = fs::create_unique_directory("unittest", testDir);
////   EXPECT_TRUE(!errorCode);

////   SmallString<128> selfFilePath;
////   fs::path::append(selfFilePath, testDir, "self.rsp");
////   std::string selfFileRef = std::string("@") + selfFilePath.getCStr();

////   SmallString<128> nestedFilePath;
////   fs::path::append(nestedFilePath, testDir, "nested.rsp");
////   std::string nestedFileRef = std::string("@") + nestedFilePath.getCStr();

////   SmallString<128> flagFilePath;
////   fs::path::append(flagFilePath, testDir, "flag.rsp");
////   std::string flagFileRef = std::string("@") + flagFilePath.getCStr();

////   std::ofstream selfFile(selfFilePath.getStr());
////   EXPECT_TRUE(selfFile.is_open());
////   selfFile << "-option_1\n";
////   selfFile << flagFileRef << "\n";
////   selfFile << nestedFileRef << "\n";
////   selfFile << selfFileRef << "\n";
////   selfFile.close();

////   std::ofstream nestedFile(nestedFilePath.getStr());
////   EXPECT_TRUE(nestedFile.is_open());
////   nestedFile << "-option_2\n";
////   nestedFile << flagFileRef << "\n";
////   nestedFile << selfFileRef << "\n";
////   nestedFile << nestedFileRef << "\n";
////   nestedFile.close();

////   std::ofstream flagFile(flagFilePath.getStr());
////   EXPECT_TRUE(flagFile.is_open());
////   flagFile << "-option_x\n";
////   flagFile.close();

////   // Ensure:
////   // Recursive expansion terminates
////   // Recursive files never expand
////   // Non-recursive repeats are allowed
////   SmallVector<const char *, 4> argv = {"test/test", selfFileRef.c_str(),
////                                        "-option_3"};
////   BumpPtrAllocator A;
////   StringSaver saver(A);
////#ifdef _WIN32
////   cmd::TokenizerCallback Tokenizer = cms::tokenize_windows_command_line;
////#else
////   cmd::TokenizerCallback Tokenizer = cmd::tokenize_gnu_command_line;
////#endif
////   bool res = cmd::expand_response_files(saver, Tokenizer, argv, false, false);
////   EXPECT_FALSE(res);

////   EXPECT_EQ(argv.size(), 9U);
////   EXPECT_STREQ(argv[0], "test/test");
////   EXPECT_STREQ(argv[1], "-option_1");
////   EXPECT_STREQ(argv[2], "-option_x");
////   EXPECT_STREQ(argv[3], "-option_2");
////   EXPECT_STREQ(argv[4], "-option_x");
////   EXPECT_STREQ(argv[5], selfFileRef.c_str());
////   EXPECT_STREQ(argv[6], nestedFileRef.c_str());
////   EXPECT_STREQ(argv[7], selfFileRef.c_str());
////   EXPECT_STREQ(argv[8], "-option_3");
////}

////TEST(CommandLineTest, testResponseFilesAtArguments) {
////   SmallString<128> testDir;
////   std::error_code errorCode = fs::create_unique_directory("unittest", testDir);
////   EXPECT_TRUE(!errorCode);

////   SmallString<128> responseFilePath;
////   fs::path::append(responseFilePath, testDir, "test.rsp");

////   std::ofstream ResponseFile(responseFilePath.getCStr());
////   EXPECT_TRUE(ResponseFile.is_open());
////   ResponseFile << "-foo" << "\n";
////   ResponseFile << "-bar" << "\n";
////   ResponseFile.close();

////   // Ensure we expand rsp files after lots of non-rsp arguments starting with @.
////   constexpr size_t NON_RSP_AT_ARGS = 64;
////   SmallVector<const char *, 4> argv = {"test/test"};
////   argv.append(NON_RSP_AT_ARGS, "@non_rsp_at_arg");
////   std::string ResponseFileRef = std::string("@") + responseFilePath.getCStr();
////   argv.push_back(ResponseFileRef.c_str());

////   BumpPtrAllocator A;
////   StringSaver saver(A);
////   bool res = cmd::expand_response_files(saver, cmd::tokenize_gnu_command_line, argv,
////                                         false, false);
////   EXPECT_FALSE(res);

////   // ASSERT instead of EXPECT to prevent potential out-of-bounds access.
////   ASSERT_EQ(argv.size(), 1 + NON_RSP_AT_ARGS + 2);
////   size_t i = 0;
////   EXPECT_STREQ(argv[i++], "test/test");
////   for (; i < 1 + NON_RSP_AT_ARGS; ++i)
////      EXPECT_STREQ(argv[i], "@non_rsp_at_arg");
////   EXPECT_STREQ(argv[i++], "-foo");
////   EXPECT_STREQ(argv[i++], "-bar");
////}

//TEST(CommandLineTest, testSetDefautValue) {
//   cmd::reset_command_line_parser();

//   StackOption<std::string> Opt1("opt1", cmd::init("true"));
//   StackOption<bool> Opt2("opt2", cmd::init(true));
//   cmd::Alias Alias("alias", cmd::AliasOpt(Opt2));
//   StackOption<int> Opt3("opt3", cmd::init(3));

//   const char *args[] = {"prog", "-opt1=false", "-opt2", "-opt3"};

//   EXPECT_TRUE(
//            cmd::parse_commandline_options(2, args, StringRef(), &null_stream()));

//   EXPECT_TRUE(Opt1 == "false");
//   EXPECT_TRUE(Opt2);
//   EXPECT_TRUE(Opt3 == 3);

//   Opt2 = false;
//   Opt3 = 1;

//   cmd::reset_all_option_occurrences();

//   for (auto &OM : cmd::get_registered_options(*cmd::sg_topLevelSubCommand)) {
//      cmd::Option *opt = OM.second;
//      if (opt->argStr == "opt2") {
//         continue;
//      }
//      opt->setDefault();
//   }

//   EXPECT_TRUE(Opt1 == "true");
//   EXPECT_TRUE(Opt2);
//   EXPECT_TRUE(Opt3 == 3);
//   Alias.removeArgument();
//}

////TEST(CommandLineTest, testReadConfigFile)
////{
////   SmallVector<const char *, 1> argv;

////   SmallString<128> testDir;
////   std::error_code errorCode =
////         fs::create_unique_directory("unittest", testDir);
////   EXPECT_TRUE(!errorCode);

////   SmallString<128> TestCfg;
////   fs::path::append(TestCfg, testDir, "foo");
////   std::ofstream ConfigFile(TestCfg.getCStr());
////   EXPECT_TRUE(ConfigFile.is_open());
////   ConfigFile << "# Comment\n"
////                 "-option_1\n"
////                 "@subconfig\n"
////                 "-option_3=abcd\n"
////                 "-option_4=\\\n"
////                 "cdef\n";
////   ConfigFile.close();

////   SmallString<128> TestCfg2;
////   fs::path::append(TestCfg2, testDir, "subconfig");
////   std::ofstream ConfigFile2(TestCfg2.getCStr());
////   EXPECT_TRUE(ConfigFile2.is_open());
////   ConfigFile2 << "-option_2\n"
////                  "\n"
////                  "   # comment\n";
////   ConfigFile2.close();

////   // Make sure the current directory is not the directory where config files
////   // resides. In this case the code that expands response files will not find
////   // 'subconfig' unless it resolves nested inclusions relative to the including
////   // file.
////   SmallString<128> CurrDir;
////   errorCode = fs::current_path(CurrDir);
////   EXPECT_TRUE(!errorCode);
////   EXPECT_TRUE(StringRef(CurrDir) != StringRef(testDir));

////   BumpPtrAllocator A;
////   StringSaver saver(A);
////   bool Result = cmd::read_config_file(TestCfg, saver, argv);

////   EXPECT_TRUE(Result);
////   EXPECT_EQ(argv.size(), 4U);
////   EXPECT_STREQ(argv[0], "-option_1");
////   EXPECT_STREQ(argv[1], "-option_2");
////   EXPECT_STREQ(argv[2], "-option_3=abcd");
////   EXPECT_STREQ(argv[3], "-option_4=cdef");

////   fs::remove(TestCfg2);
////   fs::remove(TestCfg);
////   fs::remove(testDir);
////}

TEST(CommandLineTest, testPositionalEatArgsError) {
   cmd::reset_command_line_parser();

   StackOption<std::string, cmd::List<std::string>> posEatArgs(
            "positional-eat-args", cmd::Positional, cmd::Desc("<arguments>..."),
            cmd::ZeroOrMore, cmd::PositionalEatsArgs);
   StackOption<std::string, cmd::List<std::string>> posEatArgs2(
            "positional-eat-args2", cmd::Positional, cmd::Desc("Some strings"),
            cmd::ZeroOrMore, cmd::PositionalEatsArgs);

   const char *args[] = {"prog", "-positional-eat-args=XXXX"};
   const char *args2[] = {"prog", "-positional-eat-args=XXXX", "-foo"};
   const char *args3[] = {"prog", "-positional-eat-args", "-foo"};
   const char *args4[] = {"prog", "-positional-eat-args",
                          "-foo", "-positional-eat-args2",
                          "-bar", "foo"};

   std::string errors;
   RawStringOutStream OS(errors);
   EXPECT_FALSE(cmd::parse_commandline_options(2, args, StringRef(), &OS)); OS.flush();
   EXPECT_FALSE(errors.empty()); errors.clear();
   EXPECT_FALSE(cmd::parse_commandline_options(3, args2, StringRef(), &OS)); OS.flush();
   EXPECT_FALSE(errors.empty()); errors.clear();
   EXPECT_TRUE(cmd::parse_commandline_options(3, args3, StringRef(), &OS)); OS.flush();
   EXPECT_TRUE(errors.empty()); errors.clear();

   cmd::reset_all_option_occurrences();
   EXPECT_TRUE(cmd::parse_commandline_options(6, args4, StringRef(), &OS)); OS.flush();
   EXPECT_TRUE(posEatArgs.getSize() == 1);
   EXPECT_TRUE(posEatArgs2.getSize() == 2);
   EXPECT_TRUE(errors.empty());
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

class OutputRedirector {
public:
   OutputRedirector(int RedirectFD)
      : RedirectFD(RedirectFD), OldFD(dup(RedirectFD)) {
      if (OldFD == -1 ||
          fs::create_temporary_file("unittest-redirect", "", NewFD,
                                    FilePath) ||
          dup2(NewFD, RedirectFD) == -1)
         Valid = false;
   }

   ~OutputRedirector() {
      dup2(OldFD, RedirectFD);
      close(OldFD);
      close(NewFD);
   }

   SmallVector<char, 128> FilePath;
   bool Valid = true;

private:
   int RedirectFD;
   int OldFD;
   int NewFD;
};

struct AutoDeleteFile {
   SmallVector<char, 128> FilePath;
   ~AutoDeleteFile() {
      if (!FilePath.empty())
         fs::remove(std::string(FilePath.data(), FilePath.size()));
   }
};

class PrintOptionInfoTest : public ::testing::Test {
public:
   // Return std::string because the output of a failing EXPECT check is
   // unreadable for StringRef. It also avoids any lifetime issues.
   template <typename... Ts> std::string runTest(Ts... OptionAttributes) {
      out_stream().flush();  // flush any output from previous tests
      AutoDeleteFile File;
      {
         OutputRedirector stdoutFd(fileno(stdout));
         if (!stdoutFd.Valid)
            return "";
         File.FilePath = stdoutFd.FilePath;

         StackOption<OptionValue> TestOption(Opt, cmd::Desc(HelpText),
                                             OptionAttributes...);
         printOptionInfo(TestOption, 26);
         out_stream().flush();
      }
      auto buffer = MemoryBuffer::getFile(File.FilePath);
      if (!buffer)
         return "";
      return buffer->get()->getBuffer().getStr();
   }

   enum class OptionValue { Val };
   const StringRef Opt = "some-option";
   const StringRef HelpText = "some help";

private:
   // This is a workaround for cmd::Option sub-classes having their
   // printOptionInfo functions private.
   void printOptionInfo(const cmd::Option &O, size_t Width) {
      O.printOptionInfo(Width);
   }
};

TEST_F(PrintOptionInfoTest, testPrintOptionInfoValueOptionalWithoutSentinel) {
   std::string Output =
         runTest(cmd::ValueOptional,
                 cmd::values(clEnumValN(OptionValue::Val, "v1", "desc1")));

   // clang-format off
   EXPECT_EQ(Output, ("  --" + Opt + "=<value> - " + HelpText + "\n"
                                                                "    =v1                 -   desc1\n")
             .getStr());
   // clang-format on
}

TEST_F(PrintOptionInfoTest, testPrintOptionInfoValueOptionalWithSentinel) {
   std::string Output = runTest(
            cmd::ValueOptional, cmd::values(clEnumValN(OptionValue::Val, "v1", "desc1"),
                                            clEnumValN(OptionValue::Val, "", "")));

   // clang-format off
   EXPECT_EQ(Output,
             ("  --" + Opt + "         - " + HelpText + "\n"
                                                        "  --" + Opt + "=<value> - " + HelpText + "\n"
                                                                                                  "    =v1                 -   desc1\n")
             .getStr());
   // clang-format on
}

TEST_F(PrintOptionInfoTest, PrintOptionInfoValueOptionalWithSentinelWithHelp) {
   std::string Output = runTest(
            cmd::ValueOptional, cmd::values(clEnumValN(OptionValue::Val, "v1", "desc1"),
                                            clEnumValN(OptionValue::Val, "", "desc2")));

   // clang-format off
   EXPECT_EQ(Output, ("  --" + Opt + "         - " + HelpText + "\n"
                                                                "  --" + Opt + "=<value> - " + HelpText + "\n"
                                                                                                          "    =v1                 -   desc1\n"
                                                                                                          "    =<empty>            -   desc2\n")
             .getStr());
   // clang-format on
}

TEST_F(PrintOptionInfoTest, PrintOptionInfoValueRequiredWithEmptyValueName) {
   std::string Output = runTest(
            cmd::ValueRequired, cmd::values(clEnumValN(OptionValue::Val, "v1", "desc1"),
                                            clEnumValN(OptionValue::Val, "", "")));

   // clang-format off
   EXPECT_EQ(Output, ("  --" + Opt + "=<value> - " + HelpText + "\n"
                                                                "    =v1                 -   desc1\n"
                                                                "    =<empty>\n")
             .getStr());
   // clang-format on
}

TEST_F(PrintOptionInfoTest, PrintOptionInfoEmptyValueDescription) {
   std::string Output = runTest(
            cmd::ValueRequired, cmd::values(clEnumValN(OptionValue::Val, "v1", "")));

   // clang-format off
   EXPECT_EQ(Output,
             ("  --" + Opt + "=<value> - " + HelpText + "\n"
                                                        "    =v1\n").getStr());
   // clang-format on
}

class GetOptionWidthTest : public ::testing::Test {
public:
   enum class OptionValue { Val };

   template <typename... Ts>
   size_t runTest(StringRef ArgName, Ts... OptionAttributes) {
      StackOption<OptionValue> TestOption(ArgName, cmd::Desc("some help"),
                                          OptionAttributes...);
      return getOptionWidth(TestOption);
   }

private:
   // This is a workaround for cmd::Option sub-classes having their
   // printOptionInfo
   // functions private.
   size_t getOptionWidth(const cmd::Option &O) { return O.getOptionWidth(); }
};

TEST_F(GetOptionWidthTest, GetOptionWidthArgNameLonger) {
   StringRef ArgName("a-long-argument-name");
   size_t ExpectedStrSize = ("  --" + ArgName + "=<value> - ").getStr().size();
   EXPECT_EQ(
            runTest(ArgName, cmd::values(clEnumValN(OptionValue::Val, "v", "help"))),
            ExpectedStrSize);
}

TEST_F(GetOptionWidthTest, GetOptionWidthFirstOptionNameLonger) {
   StringRef OptName("a-long-option-name");
   size_t ExpectedStrSize = ("    =" + OptName + " - ").getStr().size();
   EXPECT_EQ(
            runTest("a", cmd::values(clEnumValN(OptionValue::Val, OptName, "help"),
                                     clEnumValN(OptionValue::Val, "b", "help"))),
            ExpectedStrSize);
}

TEST_F(GetOptionWidthTest, GetOptionWidthSecondOptionNameLonger) {
   StringRef OptName("a-long-option-name");
   size_t ExpectedStrSize = ("    =" + OptName + " - ").getStr().size();
   EXPECT_EQ(
            runTest("a", cmd::values(clEnumValN(OptionValue::Val, "b", "help"),
                                     clEnumValN(OptionValue::Val, OptName, "help"))),
            ExpectedStrSize);
}

TEST_F(GetOptionWidthTest, GetOptionWidthEmptyOptionNameLonger) {
   size_t ExpectedStrSize = StringRef("    =<empty> - ").size();
   // The length of a=<value> (including indentation) is actually the same as the
   // =<empty> string, so it is impossible to distinguish via testing the case
   // where the empty string is picked from where the option name is picked.
   EXPECT_EQ(runTest("a", cmd::values(clEnumValN(OptionValue::Val, "b", "help"),
                                      clEnumValN(OptionValue::Val, "", "help"))),
             ExpectedStrSize);
}

TEST_F(GetOptionWidthTest,
       GetOptionWidthValueOptionalEmptyOptionWithNoDescription) {
   StringRef ArgName("a");
   // The length of a=<value> (including indentation) is actually the same as the
   // =<empty> string, so it is impossible to distinguish via testing the case
   // where the empty string is ignored from where it is not ignored.
   // The dash will not actually be printed, but the space it would take up is
   // included to ensure a consistent column width.
   size_t ExpectedStrSize = ("  -" + ArgName + "=<value> - ").getStr().size();
   EXPECT_EQ(runTest(ArgName, cmd::ValueOptional,
                     cmd::values(clEnumValN(OptionValue::Val, "value", "help"),
                                 clEnumValN(OptionValue::Val, "", ""))),
             ExpectedStrSize);
}

TEST_F(GetOptionWidthTest,
       GetOptionWidthValueRequiredEmptyOptionWithNoDescription) {
   // The length of a=<value> (including indentation) is actually the same as the
   // =<empty> string, so it is impossible to distinguish via testing the case
   // where the empty string is picked from where the option name is picked
   size_t ExpectedStrSize = StringRef("    =<empty> - ").size();
   EXPECT_EQ(runTest("a", cmd::ValueRequired,
                     cmd::values(clEnumValN(OptionValue::Val, "value", "help"),
                                 clEnumValN(OptionValue::Val, "", ""))),
             ExpectedStrSize);
}

TEST(CommandLineTest, testPrefixOptions) {
   cmd::reset_command_line_parser();

   StackOption<std::string, cmd::List<std::string>> includeDirs(
            "I", cmd::Prefix, cmd::Desc("Declare an include directory"));

   // Test non-prefixed variant works with cmd::Prefix options.
   EXPECT_TRUE(includeDirs.empty());
   const char *args[] = {"prog", "-I=/usr/include"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args, StringRef(), &polar::utils::null_stream()));
   EXPECT_TRUE(includeDirs.getSize() == 1);
   EXPECT_TRUE(includeDirs.front().compare("/usr/include") == 0);

   includeDirs.erase(includeDirs.begin());
   cmd::reset_all_option_occurrences();

   // Test non-prefixed variant works with cmd::Prefix options when value is
   // passed in following argument.
   EXPECT_TRUE(includeDirs.empty());
   const char *args2[] = {"prog", "-I", "/usr/include"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(3, args2, StringRef(), &polar::utils::null_stream()));
   EXPECT_TRUE(includeDirs.getSize() == 1);
   EXPECT_TRUE(includeDirs.front().compare("/usr/include") == 0);

   includeDirs.erase(includeDirs.begin());
   cmd::reset_all_option_occurrences();

   // Test prefixed variant works with cmd::Prefix options.
   EXPECT_TRUE(includeDirs.empty());
   const char *args3[] = {"prog", "-I/usr/include"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args3, StringRef(), &polar::utils::null_stream()));
   EXPECT_TRUE(includeDirs.getSize() == 1);
   EXPECT_TRUE(includeDirs.front().compare("/usr/include") == 0);

   StackOption<std::string, cmd::List<std::string>> macroDefs(
            "D", cmd::AlwaysPrefix, cmd::Desc("Define a macro"),
            cmd::ValueDesc("MACRO[=VALUE]"));

   cmd::reset_all_option_occurrences();

   // Test non-prefixed variant does not work with cmd::AlwaysPrefix options:
   // equal sign is part of the value.
   EXPECT_TRUE(macroDefs.empty());
   const char *args4[] = {"prog", "-D=HAVE_FOO"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args4, StringRef(), &polar::utils::null_stream()));
   EXPECT_TRUE(macroDefs.getSize() == 1);
   EXPECT_TRUE(macroDefs.front().compare("=HAVE_FOO") == 0);

   macroDefs.erase(macroDefs.begin());
   cmd::reset_all_option_occurrences();

   // Test non-prefixed variant does not allow value to be passed in following
   // argument with cmd::AlwaysPrefix options.
   EXPECT_TRUE(macroDefs.empty());
   const char *args5[] = {"prog", "-D", "HAVE_FOO"};
   EXPECT_FALSE(
            cmd::parse_commandline_options(3, args5, StringRef(), &polar::utils::null_stream()));
   EXPECT_TRUE(macroDefs.empty());

   cmd::reset_all_option_occurrences();

   // Test prefixed variant works with cmd::AlwaysPrefix options.
   EXPECT_TRUE(macroDefs.empty());
   const char *args6[] = {"prog", "-DHAVE_FOO"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args6, StringRef(), &polar::utils::null_stream()));
   EXPECT_TRUE(macroDefs.getSize() == 1);
   EXPECT_TRUE(macroDefs.front().compare("HAVE_FOO") == 0);
}

TEST(CommandLineTest, GroupingWithValue) {
   cmd::reset_command_line_parser();

   StackOption<bool> OptF("f", cmd::Grouping, cmd::Desc("Some flag"));
   StackOption<bool> OptB("b", cmd::Grouping, cmd::Desc("Another flag"));
   StackOption<bool> OptD("d", cmd::Grouping, cmd::ValueDisallowed,
                          cmd::Desc("ValueDisallowed option"));
   StackOption<std::string> OptV("v", cmd::Grouping,
                                 cmd::Desc("ValueRequired option"));
   StackOption<std::string> OptO("o", cmd::Grouping, cmd::ValueOptional,
                                 cmd::Desc("ValueOptional option"));

   // Should be possible to use an option which requires a value
   // at the end of a group.
   const char *args1[] = {"prog", "-fv", "val1"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(3, args1, StringRef(), &null_stream()));
   EXPECT_TRUE(OptF);
   EXPECT_STREQ("val1", OptV.c_str());
   OptV.clear();
   cmd::reset_all_option_occurrences();

   // Should not crash if it is accidentally used elsewhere in the group.
   const char *args2[] = {"prog", "-vf", "val2"};
   EXPECT_FALSE(
            cmd::parse_commandline_options(3, args2, StringRef(), &null_stream()));
   OptV.clear();
   cmd::reset_all_option_occurrences();

   // Should allow the "opt=value" form at the end of the group
   const char *args3[] = {"prog", "-fv=val3"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args3, StringRef(), &null_stream()));
   EXPECT_TRUE(OptF);
   EXPECT_STREQ("val3", OptV.c_str());
   OptV.clear();
   cmd::reset_all_option_occurrences();

   // Should allow assigning a value for a ValueOptional option
   // at the end of the group
   const char *args4[] = {"prog", "-fo=val4"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args4, StringRef(), &null_stream()));
   EXPECT_TRUE(OptF);
   EXPECT_STREQ("val4", OptO.c_str());
   OptO.clear();
   cmd::reset_all_option_occurrences();

   // Should assign an empty value if a ValueOptional option is used elsewhere
   // in the group.
   const char *args5[] = {"prog", "-fob"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args5, StringRef(), &null_stream()));
   EXPECT_TRUE(OptF);
   EXPECT_EQ(1, OptO.getNumOccurrences());
   EXPECT_EQ(1, OptB.getNumOccurrences());
   EXPECT_TRUE(OptO.empty());
   cmd::reset_all_option_occurrences();

   // Should not allow an assignment for a ValueDisallowed option.
   const char *args6[] = {"prog", "-fd=false"};
   EXPECT_FALSE(
            cmd::parse_commandline_options(2, args6, StringRef(), &null_stream()));
}

TEST(CommandLineTest, GroupingAndPrefix) {
   cmd::reset_command_line_parser();

   StackOption<bool> OptF("f", cmd::Grouping, cmd::Desc("Some flag"));
   StackOption<bool> OptB("b", cmd::Grouping, cmd::Desc("Another flag"));
   StackOption<std::string> OptP("p", cmd::Prefix, cmd::Grouping,
                                 cmd::Desc("Prefix and Grouping"));
   StackOption<std::string> optA("a", cmd::AlwaysPrefix, cmd::Grouping,
                                 cmd::Desc("AlwaysPrefix and Grouping"));

   // Should be possible to use a cmd::Prefix option without grouping.
   const char *args1[] = {"prog", "-pval1"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args1, StringRef(), &null_stream()));
   EXPECT_STREQ("val1", OptP.c_str());
   OptP.clear();
   cmd::reset_all_option_occurrences();

   // Should be possible to pass a value in a separate argument.
   const char *args2[] = {"prog", "-p", "val2"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(3, args2, StringRef(), &null_stream()));
   EXPECT_STREQ("val2", OptP.c_str());
   OptP.clear();
   cmd::reset_all_option_occurrences();

   // The "-opt=value" form should work, too.
   const char *args3[] = {"prog", "-p=val3"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args3, StringRef(), &null_stream()));
   EXPECT_STREQ("val3", OptP.c_str());
   OptP.clear();
   cmd::reset_all_option_occurrences();

   // All three previous cases should work the same way if an option with both
   // cmd::Prefix and cmd::Grouping modifiers is used at the end of a group.
   const char *args4[] = {"prog", "-fpval4"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args4, StringRef(), &null_stream()));
   EXPECT_TRUE(OptF);
   EXPECT_STREQ("val4", OptP.c_str());
   OptP.clear();
   cmd::reset_all_option_occurrences();

   const char *args5[] = {"prog", "-fp", "val5"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(3, args5, StringRef(), &null_stream()));
   EXPECT_TRUE(OptF);
   EXPECT_STREQ("val5", OptP.c_str());
   OptP.clear();
   cmd::reset_all_option_occurrences();

   const char *args6[] = {"prog", "-fp=val6"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args6, StringRef(), &null_stream()));
   EXPECT_TRUE(OptF);
   EXPECT_STREQ("val6", OptP.c_str());
   OptP.clear();
   cmd::reset_all_option_occurrences();

   // Should assign a value even if the part after a cmd::Prefix option is equal
   // to the name of another option.
   const char *args7[] = {"prog", "-fpb"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args7, StringRef(), &null_stream()));
   EXPECT_TRUE(OptF);
   EXPECT_STREQ("b", OptP.c_str());
   EXPECT_FALSE(OptB);
   OptP.clear();
   cmd::reset_all_option_occurrences();

   // Should be possible to use a cmd::AlwaysPrefix option without grouping.
   const char *args8[] = {"prog", "-aval8"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args8, StringRef(), &null_stream()));
   EXPECT_STREQ("val8", optA.c_str());
   optA.clear();
   cmd::reset_all_option_occurrences();

   // Should not be possible to pass a value in a separate argument.
   const char *args9[] = {"prog", "-a", "val9"};
   EXPECT_FALSE(
            cmd::parse_commandline_options(3, args9, StringRef(), &null_stream()));
   cmd::reset_all_option_occurrences();

   // With the "-opt=value" form, the "=" symbol should be preserved.
   const char *args10[] = {"prog", "-a=val10"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args10, StringRef(), &null_stream()));
   EXPECT_STREQ("=val10", optA.c_str());
   optA.clear();
   cmd::reset_all_option_occurrences();

   // All three previous cases should work the same way if an option with both
   // cmd::AlwaysPrefix and cmd::Grouping modifiers is used at the end of a group.
   const char *args11[] = {"prog", "-faval11"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args11, StringRef(), &null_stream()));
   EXPECT_TRUE(OptF);
   EXPECT_STREQ("val11", optA.c_str());
   optA.clear();
   cmd::reset_all_option_occurrences();

   const char *args12[] = {"prog", "-fa", "val12"};
   EXPECT_FALSE(
            cmd::parse_commandline_options(3, args12, StringRef(), &null_stream()));
   cmd::reset_all_option_occurrences();

   const char *args13[] = {"prog", "-fa=val13"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args13, StringRef(), &null_stream()));
   EXPECT_TRUE(OptF);
   EXPECT_STREQ("=val13", optA.c_str());
   optA.clear();
   cmd::reset_all_option_occurrences();

   // Should assign a value even if the part after a cmd::AlwaysPrefix option
   // is equal to the name of another option.
   const char *args14[] = {"prog", "-fab"};
   EXPECT_TRUE(
            cmd::parse_commandline_options(2, args14, StringRef(), &null_stream()));
   EXPECT_TRUE(OptF);
   EXPECT_STREQ("b", optA.c_str());
   EXPECT_FALSE(OptB);
   optA.clear();
   cmd::reset_all_option_occurrences();
}

TEST(CommandLineTest, LongOptions) {
   cmd::reset_command_line_parser();

   StackOption<bool> optA("a", cmd::Desc("Some flag"));
   StackOption<bool> OptBLong("long-flag", cmd::Desc("Some long flag"));
   StackOption<bool, cmd::Alias> OptB("b", cmd::Desc("Alias to --long-flag"),
                                      cmd::AliasOpt(OptBLong));
   StackOption<std::string> optAB("ab", cmd::Desc("Another long option"));

   std::string errors;
   RawStringOutStream OS(errors);

   const char *args1[] = {"prog", "-a", "-ab", "val1"};
   const char *args2[] = {"prog", "-a", "--ab", "val1"};
   const char *args3[] = {"prog", "-ab", "--ab", "val1"};

   //
   // The following tests treat `-` and `--` the same, and always match the
   // longest string.
   //

   EXPECT_TRUE(
            cmd::parse_commandline_options(4, args1, StringRef(), &OS)); OS.flush();
   EXPECT_TRUE(optA);
   EXPECT_FALSE(OptBLong);
   EXPECT_STREQ("val1", optAB.c_str());
   EXPECT_TRUE(errors.empty()); errors.clear();
   cmd::reset_all_option_occurrences();

   EXPECT_TRUE(
            cmd::parse_commandline_options(4, args2, StringRef(), &OS)); OS.flush();
   EXPECT_TRUE(optA);
   EXPECT_FALSE(OptBLong);
   EXPECT_STREQ("val1", optAB.c_str());
   EXPECT_TRUE(errors.empty()); errors.clear();
   cmd::reset_all_option_occurrences();

   // Fails because `-ab` and `--ab` are treated the same and appear more than
   // once.  Also, `val1` is unexpected.
   EXPECT_FALSE(
            cmd::parse_commandline_options(4, args3, StringRef(), &OS)); OS.flush();
   out_stream()<< errors << "\n";
   EXPECT_FALSE(errors.empty()); errors.clear();
   cmd::reset_all_option_occurrences();

   //
   // The following tests treat `-` and `--` differently, with `-` for short, and
   // `--` for long options.
   //

   // Fails because `-ab` is treated as `-a -b`, so `-a` is seen twice, and
   // `val1` is unexpected.
   EXPECT_FALSE(cmd::parse_commandline_options(4, args1, StringRef(),
                                               &OS, nullptr, true)); OS.flush();
   EXPECT_FALSE(errors.empty()); errors.clear();
   cmd::reset_all_option_occurrences();

   // Works because `-a` is treated differently than `--ab`.
   EXPECT_TRUE(cmd::parse_commandline_options(4, args2, StringRef(),
                                              &OS, nullptr, true)); OS.flush();
   EXPECT_TRUE(errors.empty()); errors.clear();
   cmd::reset_all_option_occurrences();

   // Works because `-ab` is treated as `-a -b`, and `--ab` is a long option.
   EXPECT_TRUE(cmd::parse_commandline_options(4, args3, StringRef(),
                                              &OS, nullptr, true));
   EXPECT_TRUE(optA);
   EXPECT_TRUE(OptBLong);
   EXPECT_STREQ("val1", optAB.c_str());
   OS.flush();
   EXPECT_TRUE(errors.empty()); errors.clear();
   cmd::reset_all_option_occurrences();
}

} // anonymous namespace
