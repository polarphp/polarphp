// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/09.

#ifndef POLAR_DEVLTOOLS_LIT_TEST_RUNNER_H
#define POLAR_DEVLTOOLS_LIT_TEST_RUNNER_H

#include "Global.h"
#include "ShellCommands.h"
#include "BasicTimer.h"
#include <stdexcept>
#include <string>
#include <boost/regex.hpp>
#include <list>
#include <mutex>
#include <tuple>
#include <utility>
#include <map>

namespace polar {
namespace basic {
template <typename T, unsigned N>
class SmallVector;
class StringRef;
} // basic
} // polar

namespace polar {
namespace lit {

class Result;
class Test;
class LitConfig;
using OpenFileEntryType = std::tuple<std::string, std::string, int, std::string>;
using StdFdsTuple = std::tuple<int, int, int>;
using TestPointer = std::shared_ptr<Test>;
using LitConfigPointer = std::shared_ptr<LitConfig>;
using polar::basic::SmallVector;
using polar::basic::StringRef;

class InternalShellError : public std::runtime_error
{
public:
   InternalShellError(const std::string &command, const std::string &message)
      : std::runtime_error(command + ": " + message),
        m_command(command),
        m_message(message)
   {}

   const std::string &getCommand() const
   {
      return m_command;
   }

   const std::string &getMessage() const
   {
      return m_message;
   }
protected:
   const std::string &m_command;
   const std::string &m_message;
};

#ifndef POLAR_OS_WIN32
#define POLAR_KUSE_CLOSE_FDS
#else
#define POLAR_AVOID_DEV_NULL
#endif

const static std::string sgc_kdevNull("/dev/null");
const static boost::regex sgc_kpdbgRegex("%dbg\\(([^)'\"]*)\\)");

class ShellEnvironment
{
public:
   ShellEnvironment(const std::string &cwd, std::map<std::string, std::string> &env)
      : m_cwd(cwd),
        m_env(env)
   {}

   const std::string &getCwd() const;
   const std::map<std::string, std::string> &getEnv() const;
   ShellEnvironment &setCwd(const std::string &cwd);
   ShellEnvironment &setEnvItem(const std::string &key, const std::string &value);
protected:
   std::string m_cwd;
   std::map<std::string, std::string> m_env;
};

class TimeoutHelper
{
public:
   TimeoutHelper(int timeout);
   void cancel();
   bool active();
   void addProcess(pid_t process);
   void startTimer();
   bool timeoutReached();

private:
   void handleTimeoutReached();
   void kill();
protected:
   int m_timeout;
   std::list<pid_t> m_procs;
   bool m_timeoutReached;
   bool m_doneKillPass;
   std::mutex m_lock;
   std::optional<BasicTimer> m_timer;
};

class ShellCommandResult
{
public:
   ShellCommandResult(const Command &command, std::ostream &outStream, std::ostream &errStream,
                      int exitCode, bool timeoutReached, const std::list<std::string> &outputFiles = {});
   const Command &getCommand();
   int getExitCode();
   bool isTimeoutReached();

protected:
   Command m_command;
   std::ostream &m_outStream;
   std::ostream &m_errStream;
   int m_exitCode;
   bool m_timeoutReached;
   std::list<std::string> m_outputFiles;
};

std::tuple<int, std::string> execute_shcmd();

std::list<std::string> expand_glob(GlobItem &glob, const std::string &cwd);
std::list<std::string> expand_glob(const std::string &glob, const std::string &cwd);
std::list<std::string> expand_glob_expression(const std::list<std::string> &exprs,
                                              const std::string &cwd);
void quote_windows_command();
void update_env();
std::string execute_builtin_echo();
void execute_builtin_mkdir();
void execute_builtin_diff();
void execute_builtin_rm();
StdFdsTuple process_redirects(std::shared_ptr<AbstractCommand> cmd, int stdinSource,
                              const ShellEnvironment &shenv,
                              std::list<OpenFileEntryType> &openedFiles);
void execute_script_internal();
void execute_script();
void parse_integrated_test_script_commands();
std::pair<std::string, std::string> get_temp_paths(TestPointer test);
void colon_normalize_path();
void get_default_substitutions();
void apply_substitutions();

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
   static const SmallVector<char, 4> &allowedKeywordSuffixes(Kind kind);
   static StringRef getKindStr(Kind kind);
protected:
   static std::map<Kind, SmallVector<char, 4>> sm_allowedSuffixes;
   static std::map<Kind, StringRef> sm_keywordStrMap;
};

class IntegratedTestKeywordParser
{
public:
   IntegratedTestKeywordParser();
   void parseLine();
   void getValue();
private:
   static void handleTag();
   static void handleCommand();
   static void handleList();
   static void handleBooleanExpr();
   static void handleRequiresAny();
};

void parse_integrated_test_script();
Result execute_shtest(TestPointer test, LitConfigPointer litConfig, bool executeExternal);

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_TEST_RUNNER_H


