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
#include <stdexcept>
#include <string>
#include <regex>
#include <list>

namespace polar {
namespace lit {

class InternalShellError : public std::runtime_error
{
public:
   InternalShellError(const std::string &command, const std::string &message)
      : std::runtime_error(command + ": " + message)
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
const static std::regex sgc_kpdbgRegex("%dbg\(([^)'\"]*)\)");

class ShellEnvironment
{
public:
   ShellEnvironment(const std::string &cwd, std::map<std::string, std::string> &env)
      : m_cwd(cwd),
        m_env(env)
   {}
protected:
   std::string m_cwd;
   std::map<std::string, std::string> m_env;
};

class BasicTimer;

class TimeoutHelper
{
public:
   TimeoutHelper(int timeout);
   void cancel();
   void active();
   void addProcess();
   void startTimer();
   void timeoutReached();

private:
   void handleTimeoutReached();
   void kill();
protected:
   int m_timeout;
   std::list<pid_t> m_procs;
   bool m_timeoutReached;
   bool m_doneKillPass;
   void m_lock;
   std::optional<BasicTimer> m_timer;
};

class ShellCommandResult
{
public:
   ShellCommandResult(const Command &command, std::ostream &outStream, std::ostream &errStream,
                      int exitCode, bool timeoutReached, const std::list<std::string> &outputFiles = []);
protected:
   Command m_command;
   std::ostream &m_outStream;
   std::ostream &m_errStream;
   int m_exitCode;
   bool m_timeoutReached;
   std::list<std::string> m_outputFiles;
};

void execute_shcmd();
void expand_glob();
void expand_glob_expression();
void quote_windows_command();
void update_env();
void execute_builtin_echo();
void execute_builtin_mkdir();
void execute_builtin_diff();
void execute_builtin_rm();
void process_redirects();
void execute_shcmd();
void execute_script_internal();
void execute_script();
void parse_integrated_test_script_commands();
void get_temp_paths();
void colon_normalize_path();
void get_default_substitutions();
void apply_substitutions();

class ParserKind
{
public:
   enum Kind {
      TAG = 0,
      COMMAND,
      LIST,
      BOOLEAN_EXPR,
      CUSTOM
   };
public:
   static std::string allowedKeywordSuffixes(Kind kind);
   static std::string getKindStr(Kind kind);
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
void run_shtest();
void execute_shtest();


} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_TEST_RUNNER_H


