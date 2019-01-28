// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/09.

#ifndef POLAR_DEVLTOOLS_LIT_TEST_RUNNER_H
#define POLAR_DEVLTOOLS_LIT_TEST_RUNNER_H

#include "LitGlobal.h"
#include "ShellCommands.h"
#include "ForwardDefs.h"
#include "polarphp/basic/adt/StringRef.h"
#include <stdexcept>
#include <string>
#include <boost/regex.hpp>
#include <list>
#include <tuple>
#include <utility>
#include <map>
#include <functional>

namespace polar {
namespace basic {
template <typename T, unsigned N>
class SmallVector;
class StringRef;
} // basic
} // polar

namespace polar {
namespace lit {

using polar::basic::SmallVector;
using polar::basic::StringRef;
using ParsedScriptLine = std::tuple<size_t, std::string, std::string>;
using ParsedScriptLines = std::list<ParsedScriptLine>;
using ExecScriptResult = std::tuple<std::string, std::string, int, std::string>;

class InternalShellError : public std::runtime_error
{
public:
   InternalShellError(AbstractCommand *command, const std::string &message)
      : std::runtime_error(command->operator std::string() + ": " + message),
        m_command(command),
        m_message(message)
   {}

   AbstractCommand *getCommand() const
   {
      return m_command;
   }

   const std::string &getMessage() const
   {
      return m_message;
   }
protected:
   AbstractCommand *m_command;
   const std::string &m_message;
};

#ifndef POLAR_OS_WIN32
#define POLAR_KUSE_CLOSE_FDS
#else
#define POLAR_AVOID_DEV_NULL
#endif

const static StringRef sgc_kdevNull("/dev/null");
const static std::string sgc_kpdbgRegex("%dbg\\(([^)'\"]*)\\)");

class ShellEnvironment
{
public:
   ShellEnvironment(const std::string &cwd, std::list<std::string> &env)
      : m_cwd(cwd),
        m_env(env)
   {}

   const std::string &getCwd() const;
   std::list<std::string> &getEnv();
   ShellEnvironment &setCwd(const std::string &cwd);
   ShellEnvironment &setEnvItem(StringRef key, StringRef value);
   StringRef getEnvItem(StringRef key);
   ShellEnvironment &deleteEnvItem(StringRef key);
protected:
   std::string m_cwd;
   std::list<std::string> m_env;
};

class ShellCommandResult
{
public:
   ShellCommandResult(AbstractCommand *command, const std::string &outputMsg, const std::string &errorMsg,
                      int exitCode, bool timeoutReached);
   AbstractCommand *getCommand();
   int getExitCode();
   bool isTimeoutReached();
   const std::string &getOutputMsg();
   const std::string &getErrorMsg();
protected:
   AbstractCommand *m_command;
   std::string m_outputMsg;
   std::string m_errorMsg;
   int m_exitCode;
   bool m_timeoutReached;
};

/// Wrapper around _executeShCmd that handles
/// timeout
///
std::pair<int, std::string> execute_shcmd(AbstractCommandPointer cmd, ShellEnvironmentPointer shenv, ShExecResultList &results,
                                          size_t execTimeout = 0);

std::list<std::string> expand_glob(const GlobItem &glob, const std::string &cwd);
std::list<std::string> expand_glob(const std::string &glob, const std::string &cwd);
std::list<std::string> expand_glob_expression(const std::list<std::any> &exprs,
                                              const std::string &cwd);
std::string quote_windows_command(const std::list<std::string> &seq);
void update_env(ShellEnvironmentPointer shenv, Command *command);
std::string execute_builtin_echo(Command *command,
                                 ShellEnvironmentPointer shenv);
/// executeBuiltinMkdir - Create new directories.
///
ShellCommandResultPointer execute_builtin_mkdir(Command *command, ShellEnvironmentPointer shenv);
/// executeBuiltinDiff - Compare files line by line.
///
ShellCommandResultPointer execute_builtin_diff(Command *command, ShellEnvironmentPointer shenv);
ShellCommandResultPointer execute_builtin_rm(Command *command, ShellEnvironmentPointer shenv);
StdFdsTuple process_redirects(Command *command, const std::string &stdinSource,
                              ShellEnvironmentPointer shenv);
ExecScriptResult execute_script_internal(TestPointer test, LitConfigPointer litConfig,
                                         const std::string &tempBase, std::vector<std::string> &commands,
                                         const std::string &cwd, ResultPointer &result);
ExecScriptResult execute_script(TestPointer test, LitConfigPointer litConfig,
                                const std::string &tempBase, std::vector<std::string> &commands,
                                const std::string &cwd, ResultPointer &result);

std::pair<std::string, std::string> get_temp_paths(TestPointer test);
std::string colon_normalize_path(std::string path);
SubstitutionList get_default_substitutions(TestPointer test, std::string tempDir, std::string tempBase,
                                           bool normalizeSlashes=false);

/// Apply substitutions to the script.  Allow full regular expression syntax.
/// Replace each matching occurrence of regular expression pattern a with
/// substitution b in line ln.
///
std::vector<std::string> &apply_substitutions(std::vector<std::string> &script, const SubstitutionList &substitutions);

/// An enumeration representing the style of an integrated test keyword or
/// command.
///
/// TAG: A keyword taking no value. Ex 'END.'
/// COMMAND: A keyword taking a list of shell commands. Ex 'RUN:'
/// LIST: A keyword taking a comma-separated list of values.
/// BOOLEAN_EXPR: A keyword taking a comma-separated list of
///     boolean expressions. Ex 'XFAIL:'
/// CUSTOM: A keyword with custom parsing semantics.
///
class ParserKind
{
public:
   enum Kind
   {
      TAG = 0,
      COMMAND,
      LIST,
      BOOLEAN_EXPR,
      CUSTOM
   };
public:
   static const SmallVector<StringRef, 4> &allowedKeywordSuffixes(Kind kind);
   static StringRef getKindStr(Kind kind);
protected:
   static std::map<Kind, SmallVector<StringRef, 4>> sm_allowedSuffixes;
   static std::map<Kind, StringRef> sm_keywordStrMap;
};

/// parse_integrated_test_script_commands(source_path) -> commands
///
///  Parse the commands in an integrated test script file into a list of
/// (line_number, command_type, line).
///
ParsedScriptLines parse_integrated_test_script_commands(StringRef sourcePath, const std::vector<StringRef> &keywords);

/// A parser for LLVM/Clang style integrated test scripts.
///
/// keyword: The keyword to parse for. It must end in either '.' or ':'.
/// kind: An value of ParserKind.
/// parser: A custom parser. This value may only be specified with
///        ParserKind.CUSTOM.
///
class IntegratedTestKeywordParser
{
public:
   using ParserHandler = std::vector<std::string> &(*)(int, std::string &, std::vector<std::string> &);
   IntegratedTestKeywordParser(const std::string &keyword, ParserKind::Kind kind,
                               ParserHandler parser = nullptr, const std::vector<std::string> &initialValue = {});
   void parseLine(int lineNumber, std::string &line);
   ParserKind::Kind getKind();
   const std::string &getKeyword();
   const std::list<std::pair<int, StringRef>> getParsedLines();
   const std::vector<std::string> &getValue();

   /// A helper for parsing TAG type keywords
   static std::vector<std::string> &handleTag(int lineNumber, std::string &line, std::vector<std::string> &output);
   /// A helper for parsing COMMAND type keywords
   static std::vector<std::string> &handleCommand(int lineNumber, std::string &line, std::vector<std::string> &output,
                                                  const std::string &keyword);
   /// A parser for LIST type keywords
   static std::vector<std::string> &handleList(int lineNumber, std::string &line,
                                               std::vector<std::string> &output);
   /// A parser for BOOLEAN_EXPR type keywords
   static std::vector<std::string> &handleBooleanExpr(int lineNumber, std::string &line,
                                                      std::vector<std::string> &output);
   /// A custom parser to transform REQUIRES-ANY: into REQUIRES:
   static std::vector<std::string> &handleRequiresAny(int lineNumber, std::string &line,
                                                      std::vector<std::string> &output);
private:
   ParserKind::Kind m_kind;
   std::string m_keyword;
   std::vector<std::string> m_value;
   std::list<std::pair<int, StringRef>> m_parsedLines;
   std::function<std::vector<std::string> &(int, std::string &, std::vector<std::string> &)> m_parser;
};

/// parseIntegratedTestScript - Scan an LLVM/Clang style integrated test
/// script and extract the lines to 'RUN' as well as 'XFAIL' and 'REQUIRES'
/// and 'UNSUPPORTED' information.
///
/// If additional parsers are specified then the test is also scanned for the
/// keywords they specify and all matches are passed to the custom parser.
///
/// If 'require_script' is False an empty script
/// may be returned. This can be used for test formats where the actual script
/// is optional or ignored.
///
std::vector<std::string> parse_integrated_test_script(TestPointer test, ResultPointer &result, IntegratedTestKeywordParserList additionalParsers = {},
                                                      bool requireScript = true);
ResultPointer execute_shtest(TestPointer test, LitConfigPointer litConfig, bool useExternalSh,
                             SubstitutionList extraSubstitutions = {});

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_TEST_RUNNER_H
