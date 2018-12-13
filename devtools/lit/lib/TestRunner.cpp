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

#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/OptionalError.h"
#include "polarphp/utils/StringUtils.h"
#include "polarphp/utils/Program.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/FileUtils.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "dtl/dtl.hpp"

#include "TestRunner.h"
#include "Utils.h"
#include "ShellCommands.h"
#include "Test.h"
#include "LitConfig.h"
#include "ShellUtil.h"
#include "CLI/CLI.hpp"
#include "BooleanExpression.h"
#include "ProcessUtils.h"

#include <set>
#include <map>
#include <cstdio>
#include <any>
#include <sstream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <boost/regex.hpp>
#ifdef POLAR_HAVE_FCNTL_H
#include <fcntl.h>
#endif

namespace polar {
namespace lit {

using polar::utils::MemoryBuffer;
using polar::utils::OptionalError;
using polar::fs::FileRemover;
using polar::basic::Twine;
using polar::basic::ArrayRef;
using polar::basic::SmallString;
using polar::fs::DirectoryEntry;
using polar::basic::SmallVectorImpl;
using polar::sys::ProcessInfo;

#define TIMEOUT_ERROR_CODE -999

std::map<StringRef, StringRef> sg_builtinCommands
{
   {"cat", CAT_BIN}
};

const std::string &ShellEnvironment::getCwd() const
{
   return m_cwd;
}

std::list<std::string> &ShellEnvironment::getEnv()
{
   return m_env;
}

ShellEnvironment &ShellEnvironment::setCwd(const std::string &cwd)
{
   m_cwd = cwd;
   return *this;
}

ShellEnvironment &ShellEnvironment::setEnvItem(StringRef key, StringRef value)
{
   //first search
   auto iter = std::find_if(m_env.begin(), m_env.end(), [key](const std::string &item) -> bool {
      return StringRef(item).startsWith(key.trim());
   });
   Twine envItem = Twine(key.trim()).concat("=").concat(value.trim());
   if (iter != m_env.end()) {
      *iter = envItem.getStr();
   } else {
      m_env.push_back(envItem.getStr());
   }
   return *this;
}

StringRef ShellEnvironment::getEnvItem(StringRef key)
{
   auto iter = std::find_if(m_env.begin(), m_env.end(), [key](const std::string &item) -> bool {
      return StringRef(item).startsWith(key.trim());
   });
   if (iter != m_env.end()) {
      return *iter;
   }
   return StringRef(sg_emptyStr);
}

ShellEnvironment &ShellEnvironment::deleteEnvItem(StringRef key)
{
   m_env.remove_if([key](StringRef item) {
      return item.startsWith(key.trim());
   });
   return *this;
}

ShellCommandResult::ShellCommandResult(AbstractCommand *command, const std::string &outputMsg,
                                       const std::string &errorMsg, int exitCode,
                                       bool timeoutReached)
   : m_command(command),
     m_outputMsg(outputMsg),
     m_errorMsg(errorMsg),
     m_exitCode(exitCode),
     m_timeoutReached(timeoutReached)
{
}

AbstractCommand *ShellCommandResult::getCommand()
{
   return m_command;
}

int ShellCommandResult::getExitCode()
{
   return m_exitCode;
}

bool ShellCommandResult::isTimeoutReached()
{
   return m_timeoutReached;
}

const std::string &ShellCommandResult::getOutputMsg()
{
   return m_outputMsg;
}

const std::string &ShellCommandResult::getErrorMsg()
{
   return m_errorMsg;
}

namespace {
/// forward declare function
int do_execute_shcmd(AbstractCommandPointer cmd, ShellEnvironmentPointer shenv,
                     ShExecResultList &results, size_t execTimeout, bool &timeoutReached);
} // anonymous namespace

std::pair<int, std::string> execute_shcmd(AbstractCommandPointer cmd, ShellEnvironmentPointer shenv, ShExecResultList &results,
                                          size_t execTimeout)
{
   bool timeoutReached = false;
   int exitCode = do_execute_shcmd(cmd, shenv, results, execTimeout, timeoutReached);
   std::string timeoutInfo;
   if (timeoutReached) {
      timeoutInfo = format_string("Reached timeout of %d seconds", execTimeout);
   }
   return std::make_pair(exitCode, timeoutInfo);
}

std::list<std::string> expand_glob(const GlobItem &glob, const std::string &cwd)
{
   return glob.resolve(cwd);
}

std::list<std::string> expand_glob(const std::string &path, const std::string &cwd)
{
   return {path};
}

std::list<std::string> expand_glob_expression(const std::list<std::any> &exprs,
                                              const std::string &cwd)
{
   auto iter = exprs.begin();
   auto endMark = exprs.end();
   std::list<std::string> results{
      std::any_cast<std::string>(*iter++)
   };
   while (iter != endMark) {
      const std::any &exprAny = *iter;
      std::list<std::string> files;
      if (exprAny.type() == typeid(std::string)) {
         files = expand_glob(std::any_cast<std::string>(exprAny), cwd);
      } else if(exprAny.type() == typeid(GlobItem)){
         files = expand_glob(std::any_cast<GlobItem>(exprAny), cwd);
      }
      for (const std::string &file : files) {
         results.push_back(file);
      }
      ++iter;
   }
   return results;
}

std::string quote_windows_command(const std::list<std::string> &seq)
{
   std::vector<std::string> result;
   bool needQuote = false;
   for (const std::string &arg : seq) {
      std::string bsBuf;
      // Add a space to separate this argument from the others
      if (!result.empty()) {
         result.push_back(" ");
      }
      // This logic differs from upstream list2cmdline.
      needQuote = arg.find(' ') != std::string::npos ||
            arg.find('\t') != std::string::npos ||
            arg.find('\"') != std::string::npos ||
            arg.empty();
      if (needQuote) {
         result.push_back("\"");
      }
      for (char c: arg) {
         if (c == '\\') {
            // Don't know if we need to double yet.
            bsBuf += c;
         } else if (c == '"') {
            // Double backslashes.
            result.push_back(std::string(bsBuf.size() * 2, '\\'));
            bsBuf.clear();
            result.push_back("\\\"");
         } else {
            // Normal char
            if (!bsBuf.empty()) {
               for (char item: bsBuf) {
                  result.push_back(std::string(1, item));
               }
               bsBuf.clear();
               result.push_back(std::string(1, c));
            }
         }
      }
      // Add remaining backslashes, if any.
      if (!bsBuf.empty()) {
         for (char item: bsBuf) {
            result.push_back(std::string(1, item));
         }
      }
      if (needQuote) {
         for (char item: bsBuf) {
            result.push_back(std::string(1, item));
         }
         result.push_back(std::string(1, '"'));
      }
   }
   return join_string_list(result, "");
}

void update_env(ShellEnvironmentPointer shenv, Command *command)
{
   bool unsetNextEnvVar = false;
   std::list<std::any> &args = command->getArgs();
   auto iter = args.begin();
   // skip command name
   ++iter;
   auto endMark = args.end();
   int argIdx = 0;
   while (iter != endMark) {
      std::string arg = std::any_cast<std::string>(*iter);
      // Support for the -u flag (unsetting) for env command
      // e.g., env -u FOO -u BAR will remove both FOO and BAR
      // from the environment.
      if (arg == "-u") {
         unsetNextEnvVar = true;
         ++argIdx;
         ++iter;
         continue;
      }
      if (unsetNextEnvVar) {
         unsetNextEnvVar = false;
         shenv->deleteEnvItem(arg);
         ++argIdx;
         ++iter;
         continue;
      }
      // Partition the string into KEY=VALUE.
      std::string::size_type equalPos = arg.find('=');
      if (equalPos == std::string::npos) {
         break;
      }
      std::string key = arg.substr(0, equalPos);
      std::string value = arg.substr(equalPos + 1);
      shenv->setEnvItem(key, value);
      ++argIdx;
      ++iter;
   }
   for (int i = 0; i <= argIdx; ++i) {
      args.pop_front();
   }
}

namespace {

std::tuple<bool, std::string, std::string> get_process_output(std::optional<StringRef> stdoutFilename,
                                                              std::optional<StringRef> stderrFilename)
{
   std::string output;
   std::string errorMsg;
   if (stdoutFilename) {
      auto outputBuf = MemoryBuffer::getFile(stdoutFilename.value().getStr());
      if (!outputBuf) {
         return std::make_tuple(false, output, outputBuf.getError().message());
      }
      output = outputBuf.get()->getBuffer().getStr();
   }
   if (stderrFilename) {
      auto outputBuf = MemoryBuffer::getFile(stderrFilename.value().getStr());
      if (!outputBuf) {
         return std::make_tuple(false, output, outputBuf.getError().message());
      }
      errorMsg = outputBuf.get()->getBuffer().getStr();
   }
   return std::make_tuple(true, output, errorMsg);
}

class OpenTempFilesMgr
{
public:
   OpenTempFilesMgr &registerTempFile(std::string temp)
   {
      m_files.push_back(std::make_shared<FileRemover>(temp.c_str()));
      return *this;
   }
protected:
   std::list<std::shared_ptr<FileRemover>> m_files;
};

#define TESTRUNNER_ROOT_PROCESS_STDIN_PREFIX "polarphp-lit-testrunner-shell-command-root-stdin"
#define TESTRUNNER_ROOT_ROCESS_STDOUT_PREFIX "polarphp-lit-testrunner-shell-command-root-stdout"
#define TESTRUNNER_ROOT_ROCESS_STDERR_PREFIX "polarphp-lit-testrunner-shell-command-root-stderr"

#define TESTRUNNER_SUB_PROCESS_STDIN_PREFIX "polarphp-lit-testrunner-shell-command-sub-stdin"
#define TESTRUNNER_SUB_ROCESS_STDOUT_PREFIX "polarphp-lit-testrunner-shell-command-sub-stdout"
#define TESTRUNNER_SUB_ROCESS_STDERR_PREFIX "polarphp-lit-testrunner-shell-command-sub-stderr"

#define TESTRUNNER_SUB_ROCESS_TEMPFILE_PREFIX "polarphp-lit-testrunner-shell-command-sub-temp"

int do_execute_shcmd(AbstractCommandPointer cmd, ShellEnvironmentPointer shenv,
                     ShExecResultList &results, size_t execTimeout, bool &timeoutReached)
{
   //std::cout << cmd->operator std::string() << std::endl;
   int result;
   AbstractCommand::Type commandType = cmd->getCommandType();
   if (commandType == AbstractCommand::Type::Seq) {
      Seq *seqCommand = dynamic_cast<Seq *>(cmd.get());
      const std::string &op = seqCommand->getOp();
      if (op == ";") {
         result = do_execute_shcmd(seqCommand->getLhs(), shenv, results, execTimeout, timeoutReached);
         return do_execute_shcmd(seqCommand->getRhs(), shenv, results, execTimeout, timeoutReached);
      }
      if (op == "&") {
         throw InternalShellError(seqCommand, "unsupported shell operator: '&'");
      }
      if (op == "||") {
         result = do_execute_shcmd(seqCommand->getLhs(), shenv, results, execTimeout, timeoutReached);
         if (0 != result) {
            result = do_execute_shcmd(seqCommand->getRhs(), shenv, results, execTimeout, timeoutReached);
         }
         return result;
      }
      if (op == "&&") {
         result = do_execute_shcmd(seqCommand->getLhs(), shenv, results, execTimeout, timeoutReached);
         if (result == TIMEOUT_ERROR_CODE) {
            return result;
         }
         if (0 == result) {
            result = do_execute_shcmd(seqCommand->getRhs(), shenv, results, execTimeout, timeoutReached);
         }
         return result;
      }
      throw ValueError("Unknown shell command: " + op);
   }
   assert(commandType == AbstractCommand::Type::Pipeline);
   // Handle shell builtins first.
   Pipeline *pipeCommand = dynamic_cast<Pipeline *>(cmd.get());
   // check first command
   const std::shared_ptr<AbstractCommand> firstAbstractCommand = pipeCommand->getCommands().front();
   assert(firstAbstractCommand->getCommandType() == AbstractCommand::Type::Command);
   Command *firstCommand = dynamic_cast<Command *>(firstAbstractCommand.get());
   const std::any &firstArgAny = firstCommand->getArgs().front();
   assert(firstArgAny.type() == typeid(std::string));
   const std::string &firstArg = std::any_cast<std::string>(firstArgAny);
   if (firstArg == "cd") {
      if (pipeCommand->getCommands().size() != 1) {
         throw ValueError("'cd' cannot be part of a pipeline");
      }
      if (firstCommand->getArgs().size() != 2) {
         throw ValueError("'cd' supports only one argument");
      }
      auto iter = firstCommand->getArgs().begin();
      ++iter;
      std::string newDir = std::any_cast<std::string>(*iter);
      // Update the cwd in the parent environment.
      if (stdfs::path(newDir).is_absolute()) {
         shenv->setCwd(newDir);
      } else {
         stdfs::path basePath(shenv->getCwd());
         basePath /= newDir;
         basePath = stdfs::canonical(basePath);
         shenv->setCwd(basePath);
      }
      // The cd builtin always succeeds. If the directory does not exist, the
      // following Popen calls will fail instead.
      return 0;
   }

   // Handle "echo" as a builtin if it is not part of a pipeline. This greatly
   // speeds up tests that construct input files by repeatedly echo-appending to
   // a file.
   // FIXME: Standardize on the builtin echo implementation. We can use a
   // temporary file to sidestep blocking pipe write issues.
   if (firstArg == "echo" && pipeCommand->getCommands().size() == 1) {
      std::string output = execute_builtin_echo(firstCommand, shenv);
      results.push_back(std::make_shared<ShellCommandResult>(firstCommand, output, "", 0,
                                                             false));
      return 0;
   }
   if (firstArg == "export") {
      size_t commandSize = pipeCommand->getCommands().size();
      if (commandSize != 1) {
         throw ValueError("'export' cannot be part of a pipeline");
      }
      if (commandSize != 2) {
         throw ValueError("'export' supports only one argument");
      }
      update_env(shenv, firstCommand);
      return 0;
   }
   if (firstArg == "mkdir") {
      if (pipeCommand->getCommands().size() != 1) {
         throw InternalShellError(firstCommand, "Unsupported: 'mkdir' "
                                                "cannot be part of a pipeline");
      }
      ShellCommandResultPointer cmdResult = execute_builtin_mkdir(firstCommand, shenv);
      results.push_back(cmdResult);
      return cmdResult->getExitCode();
   }
   if (firstArg == "diff") {
      if (pipeCommand->getCommands().size() != 1) {
         throw InternalShellError(firstCommand, "Unsupported: 'diff' "
                                                "cannot be part of a pipeline");
      }
      ShellCommandResultPointer cmdResult = execute_builtin_diff(firstCommand, shenv);
      results.push_back(cmdResult);
      return cmdResult->getExitCode();
   }
   if (firstArg == "rm") {
      if (pipeCommand->getCommands().size() != 1) {
         throw InternalShellError(firstCommand, "Unsupported: 'rm' "
                                                "cannot be part of a pipeline");
      }
      ShellCommandResultPointer cmdResult = execute_builtin_rm(firstCommand, shenv);
      results.push_back(cmdResult);
      return cmdResult->getExitCode();
   }
   if (firstArg == ":") {
      if (pipeCommand->getCommands().size() != 1) {
         throw InternalShellError(firstCommand, "Unsupported: ':' "
                                                "cannot be part of a pipeline");
      }
      results.push_back(std::make_shared<ShellCommandResult>(firstCommand, "", "", 0, false));
      return 0;
   }
   SmallVector<std::optional<std::tuple<int, std::string, std::string, bool>>, 6> processesData;
   std::list<std::pair<int, StringRef>> stderrTempFiles;
   std::list<std::shared_ptr<SmallString<32>>> tempFilenamesPool;
   OpenTempFilesMgr tempFilesMgr;
   int inputFd;
   SmallString<32> rootInputFile;
   SmallString<32> rootOutputFile;
   fs::create_temporary_file(TESTRUNNER_ROOT_PROCESS_STDIN_PREFIX, "", inputFd, rootInputFile);
   tempFilesMgr.registerTempFile(rootInputFile.getCStr());
   fs::create_temporary_file(TESTRUNNER_ROOT_ROCESS_STDOUT_PREFIX, "", rootOutputFile);
   tempFilesMgr.registerTempFile(rootOutputFile.getCStr());
   // @TODO to be test
   std::string defaultStdin = rootInputFile.getStr().getStr();
   // To avoid deadlock, we use a single stderr stream for piped
   // output. This is null until we have seen some output using
   // stderr.
   CommandList abstractCommands = pipeCommand->getCommands();
   for (size_t i = 0; i < abstractCommands.size(); ++i) {
      processesData.push_back(std::nullopt);
   }
   size_t i = 0;
   size_t commandSize = abstractCommands.size();
   for (AbstractCommandPointer abstractCommand : abstractCommands) {
      Command *command = dynamic_cast<Command *>(abstractCommand.get());
      ShellEnvironmentPointer cmdShEnv = shenv;
      std::any &firstArgAny = command->getArgs().front();
      assert(firstArgAny.type() == typeid(std::string));
      std::string firstArg = std::any_cast<std::string>(firstArgAny);
      if (firstArg == "env") {
         // Create a copy of the global environment and modify it for this one
         // command. There might be multiple envs in a pipeline:
         //  env FOO=1 llc < %s | env BAR=2 llvm-mc | FileCheck %s
         cmdShEnv = std::make_shared<ShellEnvironment>(shenv->getCwd(), shenv->getEnv());
         update_env(cmdShEnv, command);
         std::list<std::any> &args = command->getArgs();
         if (args.size() == 0) {
            throw InternalShellError(command, "env command syntax error, maybe you need add command after setup envronment var");
         }
         // here we must reset firstArg
         std::any &firstArgAny = args.front();
         assert(firstArgAny.type() == typeid(std::string));
         firstArg = std::any_cast<std::string>(firstArgAny);
      }

      StdFdsTuple fds = process_redirects(command, defaultStdin, cmdShEnv);
      std::optional<StringRef> stdinFilename;

      StdFdPair stdinFd = std::get<0>(fds);
      StdFdPair stdoutFd = std::get<1>(fds);
      StdFdPair stderrFd = std::get<2>(fds);

      std::string rawStdinFilename = std::get<0>(stdinFd);
      std::string rawStdoutFilename = std::get<0>(stdoutFd);
      std::string rawStderrFilename = std::get<0>(stderrFd);

      std::ios_base::openmode rawStdinOpenMode = std::get<1>(stdinFd);
      std::ios_base::openmode rawStdoutOpenMode = std::get<1>(stdoutFd);
      std::ios_base::openmode rawStderrOpenMode = std::get<1>(stderrFd);

      std::optional<int> stdinOpenMode = std::nullopt;
      std::optional<int> stdoutOpenMode = std::nullopt;
      std::optional<int> stderrOpenMode = std::nullopt;

      if (!rawStdinFilename.empty()) {
         stdinFilename = rawStdinFilename;
      }
      std::optional<StringRef> stdoutFilename;
      if (!rawStdoutFilename.empty()) {
         stdoutFilename = rawStdoutFilename;
      }
      std::optional<StringRef> stderrFilename;
      if (!rawStderrFilename.empty()) {
         stderrFilename = rawStderrFilename;
      }

      bool stderrIsStdout = false;
      // If stderr wants to come from stdout, but stdout isn't a pipe, then put
      // stderr on a pipe and treat it as stdout.
      if(stderrFilename == SUBPROCESS_FD_PIPE && stdoutFilename != SUBPROCESS_FD_PIPE){
         stderrFilename = SUBPROCESS_FD_PIPE;
         stderrIsStdout = true;
      } else {
         stderrIsStdout = false;
         if (stderrFilename == SUBPROCESS_FD_PIPE) {
            if (i < commandSize - 1) {
               std::shared_ptr<SmallString<32>> tempFilename(new SmallString<32>{});
               fs::create_temporary_file(TESTRUNNER_SUB_ROCESS_STDERR_PREFIX, "", *tempFilename.get());
               tempFilesMgr.registerTempFile(tempFilename->getCStr());
               stderrFilename = tempFilename->getStr();
               stderrTempFiles.push_back(std::make_pair(i, tempFilename->getStr()));
               tempFilenamesPool.push_back(tempFilename);
            }
         }
      }
      std::list<std::any> &args = command->getArgs();
      // Resolve the executable path ourselves.
      std::optional<std::string> executable;
      bool isBuiltinCmd = sg_builtinCommands.find(firstArg) != sg_builtinCommands.end();
      if (!isBuiltinCmd) {
         // For paths relative to cwd, use the cwd of the shell environment.
         StringRef firstArgRef = firstArg;
         if (firstArgRef.startsWith(".")) {
            stdfs::path execInCwd = stdfs::path(cmdShEnv->getCwd()) / firstArg;
            if (stdfs::is_regular_file(execInCwd)) {
               executable = execInCwd.string();
            }
         } else if (firstArgRef.startsWith("/") && stdfs::is_regular_file(firstArgRef.getStr())) {
            executable = firstArgRef.getStr();
         }
         if (!executable) {
            StringRef pathStr = shenv->getEnvItem("PATH");
            pathStr = pathStr.substr(pathStr.find("=") + 1);
            SmallVector<StringRef, 10> parts;
            pathStr.split(parts, ':', -1, false);
            OptionalError<std::string> findResult = polar::sys::find_program_by_name(firstArgRef, parts);
            if (findResult) {
               executable = *findResult;
            }
         }
         if (!executable) {
            throw InternalShellError(command, format_string("%s: command not found", firstArg.c_str()));
         }
      }
      // Replace uses of /dev/null with temporary files.
      // sgc_kdevNull
#ifdef POLAR_AVOID_DEV_NULL
      int j = 0;
      for (std::any &argAny: args) {
         if (j != 0) {
            if (argAny.type() == typeid(std::string)) {
               StringRef argRef = std::any_cast<std::string>(argAny);
               if (argRef.startsWith(sgc_kdevNull)) {
                  std::shared_ptr<SmallString<32>> tempFilename(new SmallString<32>{});
                  fs::create_temporary_file(TESTRUNNER_SUB_ROCESS_STDOUT_PREFIX, "", *tempFilename.get());
                  tempFilesMgr.registerTempFile(tempFilename->getCStr());
                  argAny = (tempFilename->getStr() + argRef.substr(sgc_kdevNull.size())).getStr();
               }
            }
         }
         ++j;
      }
#endif
      // Expand all glob expressions

      std::list<std::string> expandedArgs = expand_glob_expression(args, shenv->getCwd());
      if (isBuiltinCmd) {
         executable = sg_builtinCommands.at(firstArg);
      }
#ifdef POLAR_OS_WIN32
      // @TODO quote windows
      //      expandedArgs = quote_windows_command(expandedArgs);
#endif
      SmallVector<StringRef, 10> argsRef;
      for (const std::string &argStr : expandedArgs) {
         argsRef.push_back(argStr);
      }
      SmallVector<StringRef, 10> envsRef;
      for (auto &item : cmdShEnv->getEnv()) {
         envsRef.pushBack(item);
      }
      if (stdoutFilename == SUBPROCESS_FD_PIPE) {
         std::shared_ptr<SmallString<32>> tempFilename(new SmallString<32>{});
         fs::create_temporary_file(TESTRUNNER_SUB_ROCESS_STDOUT_PREFIX, "", *tempFilename.get());
         tempFilesMgr.registerTempFile(tempFilename->getCStr());
         stdoutFilename = tempFilename->getCStr();
         tempFilenamesPool.push_back(tempFilename);
      }
      if (stderrFilename == SUBPROCESS_FD_PIPE) {
         std::shared_ptr<SmallString<32>> tempFilename(new SmallString<32>{});
         fs::create_temporary_file(TESTRUNNER_SUB_ROCESS_STDERR_PREFIX, "", *tempFilename.get());
         tempFilesMgr.registerTempFile(tempFilename->getCStr());
         stderrFilename = tempFilename->getCStr();
         tempFilenamesPool.push_back(tempFilename);
      }
      std::optional<int> curOpenMode = std::nullopt;
      if (rawStdinOpenMode & std::ios_base::in) {
         if (curOpenMode) {
            curOpenMode = curOpenMode.value() | O_RDONLY;
         } else {
            curOpenMode = O_RDONLY;
         }
      }
      if (curOpenMode) {
         stdinOpenMode = curOpenMode.value();
      }
      curOpenMode = std::nullopt;
      if (rawStdoutOpenMode & std::ios_base::out) {
         curOpenMode = O_CREAT | O_WRONLY;
      }
      if (rawStdoutOpenMode & std::ios_base::app) {
         if (curOpenMode) {
            curOpenMode = curOpenMode.value() | O_APPEND;
         } else {
            curOpenMode = O_APPEND;
         }
      } else if (rawStdoutOpenMode & std::ios_base::trunc) {
         if (curOpenMode) {
            curOpenMode = curOpenMode.value() | O_TRUNC;
         } else {
            curOpenMode = O_TRUNC;
         }
      }
      if (curOpenMode) {
         stdoutOpenMode = curOpenMode.value();
      }
      curOpenMode = std::nullopt;
      if (rawStderrOpenMode & std::ios_base::out) {
         curOpenMode = O_CREAT | O_WRONLY;
      }
      if (rawStderrOpenMode & std::ios_base::app) {
         if (curOpenMode) {
            curOpenMode = curOpenMode.value() | O_APPEND;
         } else {
            curOpenMode = O_APPEND;
         }
      } else if (rawStderrOpenMode & std::ios_base::trunc) {
         if (curOpenMode) {
            curOpenMode = curOpenMode.value() | O_TRUNC;
         } else {
            curOpenMode = O_TRUNC;
         }
      }
      if (curOpenMode) {
         stderrOpenMode = curOpenMode.value();
      }
      ArrayRef<std::optional<StringRef>> redirects{
         stdinFilename,
               stdoutFilename,
               stderrFilename
      };
      ArrayRef<std::optional<int>> redirectsOpenModes{
         stdinOpenMode,
               stdoutOpenMode,
               stderrOpenMode
      };
      std::string errorMsg;
      bool execFailed;
      // std::cout << executable.value() << std::endl;
      //       std::cout << stdinFilename.value().getStr() << std::endl;
      //       std::cout << stdoutFilename.value().getStr() << std::endl;
      //       std::cout << stderrFilename.value().getStr() << std::endl;
      // TODO need check stdoutFilename and stderrFilename
      if (stdinFilename && !fs::exists(stdinFilename.value())) {
         throw InternalShellError(command, format_string("%s: file or directory %s is not exist", executable.value().c_str(),
                                                         stdinFilename.value().getStr().c_str()));
      }
      int returnCode = polar::lit::execute_and_wait(executable.value(), argsRef, cmdShEnv->getCwd(), envsRef,
                                                    redirects, redirectsOpenModes, execTimeout, 0, &errorMsg,
                                                    &execFailed);
      if(execFailed) {
         throw InternalShellError(command, format_string("Could not create process (%s) due to %s",
                                                         executable.value().c_str(), errorMsg.c_str()));
      }
      if (returnCode == -1 || returnCode == -2) {
         if (returnCode == -2) {
            timeoutReached = true;
         }
         processesData[i] = std::make_tuple(returnCode, "", errorMsg, timeoutReached);
      } else {
         auto processResult = get_process_output(stdoutFilename, stderrFilename);
         //         std::cout << std::get<1>(processResult) << std::endl;
         //         std::cout << "---" << std::endl;
         //         std::cout << std::get<2>(processResult) << std::endl;
         if (std::get<0>(processResult)) {
            processesData[i] = std::make_tuple(returnCode, std::get<1>(processResult), std::get<2>(processResult), false);
         } else {
            throw InternalShellError(command, format_string("get command stdout or stderr content error: %s",
                                                            std::get<2>(processResult).c_str()));
         }
      }
      // Let the helper know about this process
      // Update the current stdin source.
      if (rawStdoutFilename == SUBPROCESS_FD_PIPE) {
         defaultStdin = stdoutFilename.value().getStr();
      } else if (stderrIsStdout) {
         defaultStdin = stderrFilename.value().getStr();
      } else {
         defaultStdin = rootInputFile.getCStr();
      }
      ++i;
   }

   for (auto &item : stderrTempFiles) {
      int i = std::get<0>(item);
      StringRef filename = std::get<1>(item);
      std::string errorMsg;
      auto outputBuf = MemoryBuffer::getFile(filename);
      if (outputBuf) {
         errorMsg = outputBuf.get()->getBuffer().getStr();
      }
      std::optional<std::tuple<bool, std::string, std::string, bool>> errorTuple = processesData[i];
      if (errorTuple) {
         auto errorValue = errorTuple.value();
         std::get<2>(errorValue) = errorMsg;
         processesData[i] = errorValue;
      } else {
         processesData[i] = std::make_tuple(-4, "", errorMsg, false);
      }
   }
   int lastExitCode = 0;
   CommandList cmds = pipeCommand->getCommands();
   auto cmdIter = cmds.begin();
   for (size_t i = 0; i < processesData.size(); ++i) {
      auto processDataOpt = processesData[i];
      AbstractCommandPointer acmd = *cmdIter++;
      if (processDataOpt) {
         auto processData = processDataOpt.value();
         lastExitCode = std::get<0>(processData);
         results.push_back(std::make_shared<ShellCommandResult>(
                              acmd.get(),
                              std::get<1>(processData), std::get<2>(processData),
                              lastExitCode,
                              timeoutReached));
      }
   }

   if (pipeCommand->isNegate()) {
      lastExitCode = lastExitCode == 0 ? -1 : 0;
   }
   return lastExitCode;
}

} // anonymous namespace

/// Return the standard fds for cmd after applying redirects
/// Returns the three standard file descriptors for the new child process.  Each
/// fd may be an open, writable file object or a sentinel value from the
/// subprocess module.
StdFdsTuple process_redirects(Command *command, const std::string &stdinSource,
                              ShellEnvironmentPointer shenv)
{
   // Apply the redirections, we use (N,) as a sentinel to indicate stdin,
   // stdout, stderr for N equal to 0, 1, or 2 respectively. Redirects to or
   // from a file are represented with a list [file, mode, file-object]
   // where file-object is initially None.
   SmallVector<std::any, 3> redirects = {
      std::tuple<int, int>{0, -1},
      std::tuple<int, int>{1, -1},
      std::tuple<int, int>{2, -1}
   };

   for (const RedirectTokenType &redirect : command->getRedirects()) {
      const ShellTokenType &op = std::get<0>(redirect);
      const std::string &filename = std::get<1>(redirect);
      if (op == std::tuple<std::string, int>{">", 2}) {
         redirects[2] = std::any(OpenFileTuple{filename, std::ios_base::out, std::nullopt});
      } else if (op == std::tuple<std::string, int>{">>", 2}) {
         redirects[2] = std::any(OpenFileTuple{filename, std::ios_base::out| std::ios_base::app, std::nullopt});
      } else if (op == std::tuple<std::string, int>{">&", 2} &&
                 (filename == "0" || filename == "1" || filename == "2")) {
         redirects[2] = redirects[std::stoi(filename)];
      } else if (op == std::tuple<std::string, int>{">&", SHELL_CMD_REDIRECT_TOKEN} ||
                 op == std::tuple<std::string, int>{"&>", SHELL_CMD_REDIRECT_TOKEN}) {
         redirects[1] = redirects[2] = std::any(OpenFileTuple{filename, std::ios_base::out | std::ios_base::trunc, std::nullopt});
      } else if (op == std::tuple<std::string, int>{">", SHELL_CMD_REDIRECT_TOKEN}) {
         redirects[1] = std::any(OpenFileTuple{filename, std::ios_base::out | std::ios_base::trunc, std::nullopt});
      } else if (op == std::tuple<std::string, int>{">>", SHELL_CMD_REDIRECT_TOKEN}) {
         redirects[1] = std::any(OpenFileTuple{filename, std::ios_base::out | std::ios_base::app, std::nullopt});
      } else if (op == std::tuple<std::string, int>{"<", SHELL_CMD_REDIRECT_TOKEN}) {
         redirects[0] = std::any(OpenFileTuple{filename, std::ios_base::in, std::nullopt});
      } else {
         throw InternalShellError(command,
                                  "Unsupported redirect: (" + std::get<0>(op) + ", " + std::to_string(std::get<1>(op)) + ")");
      }
   }
   // Open file descriptors in a second pass.
   SmallVector<StdFdPair, 3> stdFds
   {
      StdFdPair{"", std::ios_base::in},
      StdFdPair{"", std::ios_base::out},
      StdFdPair{"", std::ios_base::out}
   };
   for (int index = 0; index <= 2; ++index) {
      std::string fd;
      std::any &itemAny = redirects[index];
      // Handle the sentinel values for defaults up front.
      if (itemAny.type() == typeid(std::tuple<int, int>)) {
         std::tuple<int, int> &item = std::any_cast<std::tuple<int, int> &>(itemAny);
         if (item == std::tuple<int, int>{0, -1}) {
            fd = stdinSource;
         } else if (item == std::tuple<int, int>{1, -1}) {
            if (index == 0) {
               throw InternalShellError(command,
                                        "Unsupported redirect for stdin");
            } else if (index == 1) {
               fd = SUBPROCESS_FD_PIPE;
            } else {
               fd = SUBPROCESS_FD_STDOUT;
            }
         } else if (item == std::tuple<int, int>{2, -1}) {
            if (index != 2) {
               throw InternalShellError(command,
                                        "Unsupported redirect for stdout");
            }
            fd = SUBPROCESS_FD_PIPE;
         } else {
            throw InternalShellError(command,
                                     "Bad redirect");
         }
         stdFds[index].first = fd;
         continue;
      }
      OpenFileTuple &item = std::any_cast<OpenFileTuple &>(itemAny);
      std::string &filename = std::get<0>(item);
      std::string redirFilename;
      std::list<std::string> names = expand_glob(filename, shenv->getCwd());
      if (names.size() != 1) {
         throw InternalShellError(command,
                                  "Unsupported: glob in "
                                  "redirect expanded to multiple files");
      }
      std::string &name = names.front();
      // Make sure relative paths are relative to the cwd.
      redirFilename = stdfs::path(shenv->getCwd()) / name;
      stdFds[index] = std::make_pair(redirFilename, std::ios_base::openmode(std::get<1>(item)));
   }
   return StdFdsTuple{stdFds[0], stdFds[1], stdFds[2]};
}

/// TODO the fd may leak
std::string execute_builtin_echo(Command *command,
                                 ShellEnvironmentPointer shenv)
{
   StdFdsTuple fds = process_redirects(command, SUBPROCESS_FD_PIPE, shenv);

   StdFdPair stdinFd = std::get<0>(fds);
   StdFdPair stdoutFd = std::get<1>(fds);
   StdFdPair stderrFd = std::get<2>(fds);

   std::string rawStdinFilename = std::get<0>(stdinFd);
   std::string rawStdoutFilename = std::get<0>(stdoutFd);
   std::string rawStderrFilename = std::get<0>(stderrFd);
   if (rawStdinFilename != SUBPROCESS_FD_PIPE || rawStderrFilename != SUBPROCESS_FD_PIPE) {
      throw InternalShellError(command,
                               "stdin and stderr redirects not supported for echo");
   }
   // Some tests have un-redirected echo commands to help debug test failures.
   // Buffer our output and return it to the caller.
   std::ostream *outstream = nullptr;
   std::unique_ptr<std::ostringstream> strstream;
   std::unique_ptr<std::ofstream> fostream;
   bool isRedirected = true;
   if (rawStdoutFilename == SUBPROCESS_FD_PIPE) {
      isRedirected = false;
      strstream.reset(new std::ostringstream);
      outstream = strstream.get();
   } else {
      std::ios_base::openmode openModes = std::get<1>(stdoutFd);
#ifdef POLAR_OS_WIN32
      openModes |= std::ios_base::binary;
#else
      fostream.reset(new std::ofstream(rawStdoutFilename, openModes));
      if (!fostream->is_open()) {
         throw InternalShellError(command,
                                  strerror(errno));
      }
      outstream = fostream.get();
#endif
   }
   // Implement echo flags. We only support -e and -n, and not yet in
   // combination. We have to ignore unknown flags, because `echo "-D FOO"`
   // prints the dash.
   std::list<std::any> args = command->getArgs();
   args.pop_front();
   //bool interpretEscapes = false;
   bool writeNewline = true;
   while (args.size() >= 1) {
      std::any flagAny = args.front();
      std::string flag = std::any_cast<std::string>(flagAny);
      if (flag == "-e" || flag == "-n") {
         args.pop_front();
         if (flag == "-e") {
            //interpretEscapes = true;
         } else if (flag == "-n") {
            writeNewline = false;
         }
      } else {
         break;
      }
   }
   if (!args.empty()) {
      size_t i = 0;
      size_t argSize = args.size();
      auto iter = args.begin();
      auto endMark = args.end();
      while (iter != endMark) {
         if (i != argSize -1) {
            *outstream << std::any_cast<std::string>(*iter) << " ";
         } else {
            *outstream << std::any_cast<std::string>(*iter);
         }
         ++iter;
      }
   }
   if (writeNewline) {
      *outstream << std::endl;
   }
   outstream->flush();
   if (!isRedirected) {
      return dynamic_cast<std::ostringstream *>(outstream)->str();
   }
   return "";
}

namespace {
bool delete_dir_error_handler(const DirectoryEntry &entry)
{
   // path contains the path of the file that couldn't be removed
   // let's just assume that it's read-only and remove it.
   // need check if permission error ?
   OptionalError<polar::fs::Permission> permsOr = polar::fs::get_permissions(entry.getPath());
   if (!permsOr) {
      // stop process
      return false;
   }
   polar::fs::Permission perms = permsOr.get();
   perms |= polar::fs::Permission::all_write;
   // @TODO ignore error ok ?
   polar::fs::set_permissions(entry.getPath(), perms);
   polar::fs::remove(entry.getPath());
   return true;
}

using DiffDirItems = std::list<std::pair<std::string, std::list<std::string>>>;

///
/// Tree is a tuple of form (dirname, child_trees).
/// An empty dir has child_trees = [], a file has child_trees = None.
///
void get_dir_tree(const std::string &path, std::list<std::pair<std::string, std::list<std::string>>> &list)
{
   std::list<std::string> files;
   for(const stdfs::directory_entry &entry: stdfs::directory_iterator(path)) {
      if (entry.is_directory()) {
         get_dir_tree(entry.path(), list);
      } else {
         files.push_back(entry.path().filename());
      }
   }
   files.sort();
   list.push_back(std::make_pair(path, files));
}

void show_diff_stats(StringRef lhsFilename, StringRef rhsFilename,
                     std::ostringstream &outStream, std::ostringstream &errStream)
{
   const int    MAX_LENGTH    = 255;
   char         time_format[] = "%Y-%m-%d %H:%M:%S %z";
   time_t       rawtime[2];
   struct tm   *timeinfo[2];
   struct stat  st[2];

   if (stat(lhsFilename.getStr().c_str(), &st[0]) == -1) {
      errStream << "argv1 is invalid." << std::endl;
      return;
   }
   if (stat(lhsFilename.getStr().c_str(), &st[1]) == -1) {
      errStream << "argv2 is invalid" << std::endl;
      return;
   }
   char buf[2][MAX_LENGTH + 1];
   rawtime[0] = st[0].st_mtime;
   timeinfo[0] = localtime(&rawtime[0]);
   strftime(buf[0], MAX_LENGTH, time_format, timeinfo[0]);
   outStream << "--- " << lhsFilename.getStr() << '\t' << buf[0] << std::endl;
   rawtime[1] = st[1].st_mtime;
   timeinfo[1] = localtime(&rawtime[1]);
   strftime(buf[1], MAX_LENGTH, time_format, timeinfo[1]);
   outStream << "+++ " << rhsFilename.getStr() << '\t' << buf[1] << std::endl;
}

bool unified_diff(std::vector<std::string> &lhs, std::vector<std::string> &rhs,
                  std::ostringstream &outStream, std::ostringstream &errStream)
{
   using dtl::Diff;
   using dtl::elemInfo;
   using dtl::uniHunk;
   std::string lhsFilename = lhs.back();
   std::string rhsFilename = rhs.back();
   lhs.pop_back();
   rhs.pop_back();
   Diff<std::string> diff(lhs, rhs);
   diff.onHuge();
   //diff.onUnserious();
   diff.compose();
   // type unihunk definition test
   // uniHunk<std::pair<StringRef, elemInfo>> hunk;
   if (diff.getEditDistance() == 0) {
      return true;
   }
   show_diff_stats(lhsFilename, rhsFilename, outStream, errStream);
   diff.composeUnifiedHunks();
   diff.printUnifiedFormat(outStream);
   return false;
}

int compare_two_binary_files(std::pair<StringRef, StringRef> lhs, std::pair<StringRef, StringRef> rhs,
                             std::ostringstream &outStream, std::ostringstream &errStream)
{
   SmallVector<std::vector<std::string>, 2> fileContents{
      {}, {}
   };
   {
      SmallVector<StringRef, 128> lines;
      lhs.second.split(lines, '\n');
      for (size_t j = 0; j < lines.size(); j++) {
         fileContents[0].push_back(lines[j]);
      }
      fileContents[0].push_back(lhs.first);
   }
   {
      SmallVector<StringRef, 128> lines;
      rhs.second.split(lines, '\n');
      for (size_t j = 0; j < lines.size(); j++) {
         fileContents[1].push_back(lines[j]);
      }
      fileContents[1].push_back(rhs.first);
   }
   int exitCode = 0;
   if (!unified_diff(fileContents[0], fileContents[1], outStream, errStream)) {
      exitCode = -1;
   }
   return exitCode;
}

std::string filter_text_diff_line(StringRef line, bool stripTrailingCR, bool ignoreAllSpace,
                                  bool ignoreSpaceChange)
{
   std::string filtered;
   if (stripTrailingCR) {
      line = line.rtrim("\r");
   }
   if (ignoreSpaceChange || ignoreAllSpace) {
      SmallVector<StringRef, 32> parts;
      line.split(parts, " ", -1, false);
      if (ignoreAllSpace) {
         filtered = polar::basic::join(parts.begin(), parts.end(), StringRef(""));
      } else {
         filtered = polar::basic::join(parts.begin(), parts.end(), " ");
      }
   } else {
      filtered = line.getStr();
   }
   return filtered;
}

int compare_two_text_files(std::pair<StringRef, StringRef> lhs, std::pair<StringRef, StringRef> rhs,
                           bool stripTrailingCR, bool ignoreAllSpace,
                           bool ignoreSpaceChange, std::ostringstream &outStream,
                           std::ostringstream &errStream)
{
   SmallVector<std::vector<std::string>, 2> fileContents{
      {}, {}
   };
   {
      SmallVector<StringRef, 128> lines;
      lhs.second.split(lines, '\n');
      for (size_t j = 0; j < lines.size(); j++) {
         fileContents[0].push_back(filter_text_diff_line(lines[j], stripTrailingCR,
                                                         ignoreAllSpace, ignoreSpaceChange));
      }
      fileContents[0].push_back(lhs.first);
   }
   {
      SmallVector<StringRef, 128> lines;
      rhs.second.split(lines, '\n');
      for (size_t j = 0; j < lines.size(); j++) {
         fileContents[1].push_back(filter_text_diff_line(lines[j], stripTrailingCR,
                                                         ignoreAllSpace, ignoreSpaceChange));
      }
      fileContents[1].push_back(rhs.first);
   }
   int exitCode = 0;
   if (!unified_diff(fileContents[0], fileContents[1], outStream, errStream)) {
      exitCode = 1;
   }
   return exitCode;
}

int compare_two_files(const SmallVectorImpl<std::string> &filePaths,
                      bool stripTrailingCR, bool ignoreAllSpace,
                      bool ignoreSpaceChange, bool isBinaryFile,
                      std::ostringstream &outStream, std::ostringstream &errStream)
{
   SmallVector<std::string, 2> fileContents;
   for (const std::string &file : filePaths) {
      if (!stdfs::exists(file)) {
         errStream << "file: " << file << " is not exist" << std::endl;
         return -1;
      }
      std::ifstream istrm(file, std::ios_base::in | std::ios_base::binary);
      std::string curFileContent;
      if (!istrm.is_open()) {
         errStream << "open file: " << file << " error: " << strerror(errno) << std::endl;;
         return -1;
      }
      istrm.seekg(0, std::ios::end);
      curFileContent.reserve(istrm.tellg());
      istrm.seekg(0, std::ios::beg);
      curFileContent.assign((std::istreambuf_iterator<char>(istrm)),
                            std::istreambuf_iterator<char>());
      fileContents.push_back(std::move(curFileContent));
   }
   std::pair<StringRef, StringRef> lhs = std::make_pair(StringRef(filePaths[0]), fileContents[0]);
   std::pair<StringRef, StringRef> rhs = std::make_pair(StringRef(filePaths[1]), fileContents[1]);
   if (isBinaryFile) {
      return compare_two_binary_files(lhs, rhs, outStream, errStream);
   }
   return compare_two_text_files(lhs, rhs, stripTrailingCR, ignoreAllSpace, ignoreSpaceChange,
                                 outStream, errStream);
}

void print_only_in(const std::string &dir, const std::string &filename,
                   std::ostringstream &outStream)
{
   outStream << "Only in " << dir << ": " << filename
             << std::endl;
}

int compare_dir_trees(SmallVectorImpl<DiffDirItems> &dirTrees,
                      bool stripTrailingCR, bool ignoreAllSpace,
                      bool ignoreSpaceChange, bool isBinaryFile,
                      std::ostringstream &outStream,
                      std::ostringstream &errStream)
{
   DiffDirItems &lhs = dirTrees[0];
   DiffDirItems &rhs = dirTrees[1];
   size_t lhsSize = lhs.size();
   size_t rhsSize = rhs.size();
   if (lhsSize == 0 && rhsSize == 0) {
      return 0;
   }
   if (lhsSize == 0 && rhsSize != 0) {
      for (auto &dirItem : rhs) {
         for (std::string &filename : dirItem.second) {
            print_only_in(dirItem.first, filename, outStream);
         }
      }
      return 1;
   }
   if (lhsSize != 0 && rhsSize == 0) {
      for (auto &dirItem : lhs) {
         for (std::string &filename : dirItem.second) {
            print_only_in(dirItem.first, filename, outStream);
         }
      }
      return 1;
   }
   int exitCode = 0;
   // compare files
   auto liter = lhs.begin();
   auto riter = rhs.begin();
   while (liter != lhs.end() && riter != rhs.end()) {
      auto &lhsDirItems = *liter;
      auto &rhsDirItems = *riter;
      std::string &lhsDirName = lhsDirItems.first;
      std::string &rhsDirName = rhsDirItems.first;
      std::list<std::string> &lhsFiles = lhsDirItems.second;
      std::list<std::string> &rhsFiles = rhsDirItems.second;
      auto fliter = lhsFiles.begin();
      auto friter = rhsFiles.begin();
      while (fliter != lhsFiles.end() && friter != rhsFiles.end()) {
         std::string &lfilename = *fliter;
         std::string &rfilename = *friter;
         if (lfilename < rfilename) {
            print_only_in(lhsDirName, lfilename, outStream);
            exitCode = 1;
            ++fliter;
         } else if (lfilename > rfilename) {
            print_only_in(rhsDirName, rfilename, outStream);
            exitCode = 1;
            ++friter;
         } else {
            // compare file content
            SmallVector<std::string, 2> filePaths;
            filePaths.push_back(stdfs::path(lhsDirName) / lfilename);
            filePaths.push_back(stdfs::path(rhsDirName) / rfilename);
            exitCode = compare_two_files(filePaths, stripTrailingCR, ignoreAllSpace,
                                         ignoreSpaceChange, isBinaryFile, outStream,
                                         errStream);
            ++fliter;
            ++friter;
         }
      }
      while (fliter != lhsFiles.end()) {
         print_only_in(lhsDirName, *fliter, outStream);
         exitCode = 1;
         ++fliter;
      }
      while (friter != rhsFiles.end()) {
         print_only_in(rhsDirName, *friter, outStream);
         exitCode = 1;
         ++friter;
      }
      ++liter;
      ++riter;
   }

   while (liter != lhs.end()) {
      exitCode = 1;
      auto &dirItems = *liter;
      if (!dirItems.second.empty()) {
         for (std::string &filename : dirItems.second) {
            print_only_in(dirItems.first, filename, outStream);
         }
      } else {
         stdfs::path filePath(dirItems.first);
         print_only_in(filePath.parent_path().string(), filePath.filename().string(), outStream);
      }
      ++liter;
   }
   while (riter != rhs.end()) {
      exitCode = 1;
      auto &dirItems = *riter;

      if (!dirItems.second.empty()) {
         for (std::string &filename : dirItems.second) {
            print_only_in(dirItems.first, filename, outStream);
         }
      } else {
         stdfs::path filePath(dirItems.first);
         print_only_in(filePath.parent_path().string(), filePath.filename().string(), outStream);
      }
      ++riter;
   }
   return exitCode;
}

} // anonymous namespace

ShellCommandResultPointer execute_builtin_diff(Command *command, ShellEnvironmentPointer shenv)
{
   std::list<std::string> args = expand_glob_expression(command->getArgs(), shenv->getCwd());
   std::shared_ptr<const char *[]> argv(new const char *[args.size()]);
   size_t i = 0;
   for (std::string &arg : args) {
      argv[i++] = arg.c_str();
   }
   CLI::App cmdParser;
   int exitCode = 0;
   bool ignoreAllSpace = false;
   bool ignoreSpaceChange = false;
   bool unifiedDiff = false;
   bool recursiveDiff = false;
   bool stripTrailingCr = false;
   bool binaryMode = false;
   std::vector<std::string> paths;
   cmdParser.add_flag("-w", ignoreAllSpace, "Ignore all white space.");
   cmdParser.add_flag("-b", ignoreSpaceChange, "Ignore changes in the amount of white space.");
   cmdParser.add_flag("-u", unifiedDiff, "use unified diff");
   cmdParser.add_flag("-r", recursiveDiff, "use recursive diff.");
   cmdParser.add_flag("--strip-trailing-cr", stripTrailingCr, "strip trailing cr.");
   cmdParser.add_flag("--binary-mode", binaryMode, "compare with binary mode");
   cmdParser.add_option("paths", paths, "paths to be diff")->required();
   SmallVector<std::string, 2> filePaths;
   SmallVector<DiffDirItems, 2> dirTrees;
   std::ostringstream outStream(std::stringstream::out|std::stringstream::binary);
   std::ostringstream errStream(std::stringstream::out|std::stringstream::binary);
   try {
      cmdParser.parse(args.size(), argv.get());
      if (paths.size() != 2) {
         throw InternalShellError(command, "Error:  missing or extra operand");
      }
      for (std::string &file : paths) {
         std::list<std::pair<std::string, std::list<std::string>>> list{};
         stdfs::path filepath(file);
         if (!filepath.is_absolute()) {
            filepath = stdfs::canonical(stdfs::path(shenv->getCwd()) / filepath);
         }
         if (recursiveDiff) {
            get_dir_tree(filepath.string(), list);
            // sort
            list.sort([](auto &lhs, auto &rhs) -> bool {
               return lhs.first < rhs.first;
            });
            dirTrees.push_back(list);
         } else {
            filePaths.push_back(filepath.string());
         }
      }

      if (!recursiveDiff) {
         exitCode = compare_two_files(filePaths, stripTrailingCr, ignoreAllSpace,
                                      ignoreSpaceChange, binaryMode,
                                      outStream, errStream);
      } else {
         exitCode = compare_dir_trees(dirTrees, stripTrailingCr, ignoreAllSpace,
                                      ignoreSpaceChange, binaryMode, outStream, errStream);
      }
   } catch (const CLI::ParseError &e) {
      throw InternalShellError(command, format_string("Unsupported: 'diff': %s\n", e.what()));
   } catch (const std::filesystem::filesystem_error &e) {
      errStream << "Error: 'diff' command failed, " << e.what() << std::endl;
      exitCode = 1;
   }
   return std::make_shared<ShellCommandResult>(command, outStream.str(), errStream.str(), exitCode, false);
}

ShellCommandResultPointer execute_builtin_mkdir(Command *command, ShellEnvironmentPointer shenv)
{
   std::list<std::string> args = expand_glob_expression(command->getArgs(), shenv->getCwd());
   std::shared_ptr<const char *[]> argv(new const char *[args.size()]);
   size_t i = 0;
   for (std::string &arg : args) {
      argv[i++] = arg.c_str();
   }
   CLI::App cmdParser;
   bool parent = false;
   std::vector<std::string> paths;
   cmdParser.add_option("paths", paths, "paths to be create")->required();
   cmdParser.add_flag("-p", parent, "force remove items");
   int exitCode = 0;
   std::ostringstream errorStream;
   try {
      cmdParser.parse(args.size(), argv.get());
      for (std::string &pathStr: paths) {
         stdfs::path path(pathStr);
         if (!path.is_absolute()) {
            path = stdfs::path(shenv->getCwd()) / path;
         }
         if (parent) {
            stdfs::create_directories(path);
         } else {
            stdfs::create_directory(path);
         }
      }
   } catch (const CLI::ParseError &e) {
      throw InternalShellError(command, format_string("Unsupported: 'rm': %s\n", e.what()));
   } catch(std::exception &e) {
      exitCode = 1;
      errorStream << "Error: 'mkdir' command failed, " << e.what() << std::endl;
   }
   return std::make_shared<ShellCommandResult>(command, "", errorStream.str(), exitCode, false);
}

/// executeBuiltinRm - Removes (deletes) files or directories.
/// @TODO do remove std::any args
///
ShellCommandResultPointer execute_builtin_rm(Command *command, ShellEnvironmentPointer shenv)
{
   std::list<std::string> args = expand_glob_expression(command->getArgs(), shenv->getCwd());
   std::shared_ptr<const char *[]> argv(new const char *[args.size()]);
   size_t i = 0;
   for (std::string &arg : args) {
      argv[i++] = arg.c_str();
   }
   bool force = false;
   bool recursive = false;
   std::vector<std::string> paths;
   CLI::App cmdParser;
   cmdParser.add_option("paths", paths, "paths to be removed")->required();
   cmdParser.add_flag("-f", force, "force remove items");
   cmdParser.add_flag("-r,-R,--recursive", recursive, "remove items recursive");
   try {
      cmdParser.parse(args.size(), argv.get());
      std::ostringstream errorStream;
      int exitCode = 0;
      std::error_code errorCode;
      for (std::string &pathStr: paths) {
         stdfs::path path(pathStr);
         if (!path.is_absolute()) {
            path = stdfs::path(shenv->getCwd()) / path;
         }
         if (force && !stdfs::exists(path, errorCode)) {
            continue;
         }
         try {
            if (!stdfs::exists(path)) {
               throw ValueError(format_string("No such file or directory: %s", path.string().c_str()));
            }
            if (stdfs::is_directory(path)) {
               if (!recursive) {
                  errorStream << "Error: "<< path.string() << " is a directory" << std::endl;
                  exitCode = 1;
               }
               polar::fs::remove_directories_with_callback(path.string(), delete_dir_error_handler);
            } else {
               stdfs::file_status fileStatus = stdfs::status(path);
               if (force && ((fileStatus.permissions() & (stdfs::perms::owner_write | stdfs::perms::group_write | stdfs::perms::others_write)) == stdfs::perms::none)) {
                  stdfs::permissions(path, fileStatus.permissions() | stdfs::perms::owner_write, errorCode);
               }
               std::error_code error;
               if (!stdfs::remove(path, error)) {
                  throw ValueError(error.message());
               }
            }
         } catch (std::exception &e) {
            errorStream << "Error: 'rm' command failed, " << e.what();
            exitCode = 1;
            break;
         }
      }
      return std::make_shared<ShellCommandResult>(command, "", errorStream.str(), exitCode, false);
   } catch(const CLI::ParseError &e) {
      throw InternalShellError(command, format_string("Unsupported: 'rm':  %s", e.what()));
   }
}

ExecScriptResult execute_script_internal(TestPointer test, LitConfigPointer litConfig,
                                         const std::string &, std::vector<std::string> &commands,
                                         const std::string &cwd, ResultPointer &result)
{
   std::vector<AbstractCommandPointer> cmds;
   for (std::string &cmdStr: commands) {
      cmdStr = boost::regex_replace(cmdStr, boost::regex(sgc_kpdbgRegex, boost::match_default | boost::match_all), ": '$1'; ");
      try {
         cmds.push_back(ShParser(cmdStr, litConfig->isWindows(), test->getConfig()->isPipefail()).parse());
      } catch (...) {
         result = std::make_shared<Result>(FAIL, format_string("shell parser error on: %s", cmdStr.c_str()));
         return ExecScriptResult{};
      }
   }
   ShExecResultList results;
   int exitCode = 0;
   std::string timeoutInfo;
   size_t total = commands.size();
   size_t pageSize = 30;
   size_t curPos = 0;
   size_t pages = (int)std::ceil(total / float(pageSize));
   ShellEnvironmentPointer shenv = std::make_shared<ShellEnvironment>(cwd, test->getConfig()->getEnvironment());
   for (size_t page = 0; page < pages; ++page){
      curPos = page * pageSize;
      size_t cycleStop = std::min(curPos + pageSize, total);
      AbstractCommandPointer cmd = cmds[curPos];
      for (size_t i = curPos + 1; i < cycleStop; ++i) {
         cmd = std::make_shared<Seq>(cmd, "&&", cmds[i]);
      }
      try {
         std::pair<int, std::string> r = execute_shcmd(cmd, shenv, results, litConfig->getMaxIndividualTestTime());
         exitCode = std::get<0>(r);
         timeoutInfo = std::get<1>(r);
      } catch (InternalShellError &e) {
         exitCode = 127;
         results.push_back(
                  std::make_shared<ShellCommandResult>(e.getCommand(), "", e.what(), exitCode, false));
         break;
      }
   }

   std::string out;
   std::string err;
   int i = 0;
   for (ShellCommandResultPointer shExecResult : results) {
      // Write the command line run.
      Command *cmd = dynamic_cast<Command *>(shExecResult->getCommand());
      if (cmd != nullptr) {
         std::string argMsg;
         int j = 0;
         int argSize = cmd->getArgs().size();
         for (const std::any &argAny : cmd->getArgs()) {
            std::string argStr;
            if (argAny.type() == typeid(std::string)) {
               argStr = std::any_cast<std::string>(argAny);
            } else if (argAny.type() == typeid(GlobItem)) {
               argStr = std::any_cast<GlobItem>(argAny);
            }
            if (i < argSize - 1) {
               argMsg += " " + argStr;
            } else {
               argMsg += argStr + " ";
            }
            ++j;
         }
         out += Twine("$ ", StringRef(argMsg).ltrim()).concat("\n").getStr();
      } else {
         out += "$ \n";
      }

      //  If nothing interesting happened, move on.
      if (litConfig->getMaxIndividualTestTime() == 0 && shExecResult->getExitCode() == 0 &&
          StringRef(shExecResult->getOutputMsg()).trim().empty() &&
          StringRef(shExecResult->getErrorMsg()).trim().empty()) {
         continue;
      }
      // Otherwise, something failed or was printed, show it.
      if (!StringRef(shExecResult->getOutputMsg()).trim().empty()) {
         out += Twine("# command output:\n", shExecResult->getOutputMsg()).concat("\n").getStr();
      }
      if (!StringRef(shExecResult->getErrorMsg()).trim().empty()) {
         err += Twine("# command stderr:\n", shExecResult->getErrorMsg()).concat("\n").getStr();
      }
      if (StringRef(shExecResult->getOutputMsg()).trim().empty() &&
          StringRef(shExecResult->getErrorMsg()).trim().empty()) {
         out += "note: command had no output on stdout or stderr\n";
      }
      // Show the error conditions:
      if (shExecResult->getExitCode() != 0) {
         std::string codeStr;
         // On Windows, a negative exit code indicates a signal, and those are
         // easier to recognize or look up if we print them in hex.
         if (litConfig->isWindows() && shExecResult->getExitCode() < 0) {
            int exitCode = shExecResult->getExitCode() & 0xFFFFFFFF;
            codeStr = polar::basic::utohexstr(exitCode);
         } else {
            codeStr = std::to_string(shExecResult->getExitCode());
         }
         err += Twine("error: command failed with exit status: ", codeStr).concat("\n").getStr();
      }
      if (litConfig->getMaxIndividualTestTime() > 0) {
         err += Twine(StringRef("error: command reached timeout: "),
                      shExecResult->isTimeoutReached() ? "true" : "false").concat("\n").getStr();
      }
      ++i;
   }
   return std::make_tuple(out, err, exitCode, timeoutInfo);
}

ExecScriptResult execute_script(TestPointer test, LitConfigPointer litConfig,
                                const std::string &tempBase, std::vector<std::string> &commands,
                                const std::string &cwd, ResultPointer &)
{
   const std::optional<std::string> &bashPath = litConfig->getBashPath();
   bool isWin32CMDEXE = litConfig->isWindows() && !bashPath;
   std::string script = tempBase + ".script";
   if (isWin32CMDEXE) {
      script +=".bat";
   }
   // Write script file
   std::ios_base::openmode openModes = std::ios_base::out;
   if (litConfig->isWindows() && !isWin32CMDEXE) {
      openModes |= std::ios_base::binary;
   }
   std::ofstream ostream(script, openModes);
   if (!ostream.is_open()) {
      std::cerr << "open script file error" << std::endl;
   }
   boost::regex kpdbgRegex(sgc_kpdbgRegex, boost::match_default | boost::match_all);
   if (isWin32CMDEXE) {
      for (std::string &command : commands) {
         command = boost::regex_replace(command, kpdbgRegex, "echo '$1' > nul && ");
      }
      if (litConfig->isEchoAllCommands()) {
         ostream << "@echo on" << std::endl;
      } else {
         ostream << "@echo off" << std::endl;
      }
      ostream << join_string_list(commands, "\n@if %ERRORLEVEL% NEQ 0 EXIT\n");
   } else {
      for (std::string &command : commands) {
         command = boost::regex_replace(command, kpdbgRegex, ": '$1'; ");
      }
      if (test->getConfig()->isPipefail()) {
         ostream << "set -o pipefail;";
      }
      if (litConfig->isEchoAllCommands()) {
         ostream << "set -x;";
      }
      ostream << "{ " << join_string_list(commands, "; } &&\n{ ") << "; }" << std::endl;
   }

   ostream.flush();
   ostream.close();
   std::error_code ec;
   stdfs::permissions(script.c_str(), stdfs::perms::owner_all, stdfs::perm_options::add, ec);
   if (ec) {
      throw ValueError(format_string("set file: %s permission error: %s",
                                     script.c_str(), ec.message().c_str()));
   }
   std::string cmdStr;
   SmallVector<StringRef, 10> argsRef;
   if (isWin32CMDEXE) {
      cmdStr = "cmd";
      argsRef.push_back(cmdStr);
      argsRef.push_back(StringRef("/c"));
      argsRef.push_back(script);
   } else {
      if (litConfig->isUseValgrind()) {
         /// @TODO
         /// FIXME: Running valgrind on sh is overkill. We probably could just
         /// run on clang with no real loss.
         for (const std::string &valgrindArg: litConfig->getValgrindArgs()) {
            argsRef.push_back(valgrindArg);
         }
      }
      if (bashPath) {
         cmdStr = bashPath.value();
         argsRef.pushBack("");
         argsRef.push_back("-c");
         argsRef.push_back(script);
      } else {
         cmdStr = "/bin/sh";
         argsRef.pushBack("");
         argsRef.push_back("-c");
         argsRef.push_back(script);
      }
   }
   std::vector<StringRef> envRef;
   for (auto &envitem : test->getConfig()->getEnvironment()) {
      envRef.push_back(StringRef(envitem));
   }
   SmallString<32> outputFile;
   SmallString<32> errorFile;

   polar::fs::create_temporary_file(TESTRUNNER_ROOT_ROCESS_STDOUT_PREFIX, "", outputFile);
   polar::fs::create_temporary_file(TESTRUNNER_ROOT_ROCESS_STDERR_PREFIX, "", errorFile);
   FileRemover outputRemover(outputFile.getCStr());
   FileRemover errorRemover(errorFile.getCStr());
   std::optional<StringRef> redirects[] = {
      std::nullopt,
      outputFile,
      errorFile};
   std::string errorMsg;
   bool execFailed;
   int returnCode = polar::lit::execute_and_wait(cmdStr, argsRef, cwd, envRef, redirects,
                                                 litConfig->getMaxIndividualTestTime(), 0, &errorMsg, &execFailed);
   if(execFailed) {
      throw ValueError(format_string("Could not create process (%s) due to %s",
                                     cmdStr.c_str(), errorMsg.c_str()));
   }
   if (returnCode == -1 || returnCode == -2) {
      if (returnCode == -2) {
         errorMsg = format_string("Reached timeout of %d seconds", litConfig->getMaxIndividualTestTime());
         return std::make_tuple("", "", returnCode, errorMsg);
      }
      return std::make_tuple("", errorMsg, returnCode, "");
   } else {
      if (returnCode != 0) {
         auto errorBuf = MemoryBuffer::getFile(errorFile.getCStr());
         if (!errorBuf) {
            return std::make_tuple("", "", -errno, strerror(errno));
         }
         StringRef errorOutput = errorBuf.get()->getBuffer();
         return std::make_tuple("", errorOutput.getStr(), returnCode, errorMsg);
      }
      auto outputBuf = MemoryBuffer::getFile(outputFile.getCStr());
      if (!outputBuf) {
         return std::make_tuple("", "", -errno, strerror(errno));
      }
      StringRef output = outputBuf.get()->getBuffer();
      return std::make_tuple(output.getStr(), "", returnCode, "");
   }
}

ParsedScriptLines parse_integrated_test_script_commands(StringRef sourcePath,
                                                        const std::vector<StringRef> &keywords)
{
   ParsedScriptLines lines;
   std::string fileContent;
   {
      // Open the file to check and add it to SourceMgr.
      OptionalError<std::unique_ptr<MemoryBuffer>> inputFileOrErr =
            MemoryBuffer::getFileOrStdIn(sourcePath);
      if (std::error_code errorCode = inputFileOrErr.getError()) {
         std::cerr << "Could not open input file '" << sourcePath
                   << "': " << errorCode.message() << '\n';
         return lines;
      }
      MemoryBuffer &inputFile = *inputFileOrErr.get();
      if (inputFile.getBufferSize() == 0) {
         return lines;
      }
      fileContent = inputFile.getBuffer();
      if (fileContent.back() != '\n') {
         fileContent.push_back('\n');
      }
   }
   std::vector<std::string> filteredKeywords;
   std::for_each(keywords.begin(), keywords.end(), [&filteredKeywords](const StringRef &keywork) {
      filteredKeywords.push_back(polar::utils::regex_escape(keywork));
   });
   try {
      size_t lineNumber = 1;
      size_t lastMatchPosition = 0;
      std::string regexStr = format_string("(%s)(.*)", polar::basic::join(filteredKeywords, "|").c_str());
      boost::sregex_iterator riter(fileContent.begin(), fileContent.end(), boost::regex(regexStr), boost::match_not_dot_newline);
      boost::sregex_iterator eiter;
      std::for_each(riter, eiter, [&lines, &lineNumber, &lastMatchPosition, &fileContent](const boost::smatch &match){
         int matchPosition = match.position();
         lineNumber += StringRef(fileContent.data() + lastMatchPosition, matchPosition - lastMatchPosition).count('\n');
         lastMatchPosition = matchPosition;
         lines.emplace_back(lineNumber, match[1].str(), StringRef(match[2].str()).rtrim("\r").getStr());
      });
      return lines;
   } catch (boost::bad_expression &e) {
      std::cerr << "regex syntax error: " << e.what() << std::endl;
      return lines;
   }
}

/// Get the temporary location, this is always relative to the test suite
/// root, not test source root.
std::pair<std::string, std::string> get_temp_paths(TestPointer test)
{
   stdfs::path execPath(test->getExecPath());
   stdfs::path execDir = execPath.parent_path();
   stdfs::path execBase = execPath.filename();
   stdfs::path tempDir = execDir / "Output";
   stdfs::path tempBase = tempDir / execBase;
   return std::make_pair(tempDir, tempBase);
}

std::string colon_normalize_path(std::string path)
{
#ifdef POLAR_OS_WIN32
   replace_string("\\", "/", path);
   return boost::regex_replace(path, boost::regex("^(.):"), [](boost::smatch match) -> std::string{
      return match[1].str();
   }, boost::match_default | boost::match_all);
#else
   assert(path[0] == '/');
   return path.substr(1);
#endif
}

namespace {
void merge_substitution_list(SubstitutionList &target, const SubstitutionList &source)
{
   for (const SubstitutionPair &item : source) {
      target.push_back(item);
   }
}
}

SubstitutionList get_default_substitutions(TestPointer test, std::string tempDir, std::string tempBase,
                                           bool normalizeSlashes)
{
   stdfs::path sourcePath(test->getSourcePath());
   stdfs::path sourceDir = sourcePath.parent_path();
   //  Normalize slashes, if requested.
   if (normalizeSlashes) {
      std::string sourcePathStr = sourcePath.string();
      std::string sourceDirStr = sourceDir.string();
      replace_string("\\", "/", sourcePathStr);
      replace_string("\\", "/", sourceDirStr);
      replace_string("\\", "/", tempDir);
      replace_string("\\", "/", tempBase);
      sourcePath = sourcePathStr;
      sourceDir = sourceDirStr;
   }
   // We use #_MARKER_# to hide %% while we do the other substitutions.
   SubstitutionList list{
      {"%%", "#_MARKER_#"}
   };
   // apply testcase config registered substitutions
   merge_substitution_list(list, test->getConfig()->getSubstitutions());
   std::string tempName = tempBase + ".temp";
   std::string baseName = stdfs::path(tempName).filename();
   merge_substitution_list(list,
   {
                              {"%s", sourcePath},
                              {"%S", sourceDir},
                              {"%P", sourceDir},
                              {"%{pathseq}", std::string(1, stdfs::path::preferred_separator)},
                              {"%t", tempName},
                              {"basename_t", baseName},
                              {"%T", tempDir},
                              {"#_MARKER_#", "%"}
                           });
   // "%:[STpst]" are normalized paths without colons and without a leading
   // slash.
   merge_substitution_list(list,
   {
                              {"%:s", colon_normalize_path(sourcePath)},
                              {"%:S", colon_normalize_path(sourceDir)},
                              {"%s:p", colon_normalize_path(sourceDir)},
                              {"%s:t", colon_normalize_path(tempBase + ".temp")},
                              {"%s:T", colon_normalize_path(tempDir)}
                           });
   return list;
}

std::vector<std::string> &apply_substitutions(std::vector<std::string> &script, const SubstitutionList &substitutions)
{
   for (size_t i = 0; i < script.size(); ++i) {
      std::string &line = script[i];
      for (const SubstitutionPair &substitution: substitutions) {
         StringRef a = std::get<0>(substitution);
         std::string b = std::get<1>(substitution);
#ifdef POLAR_OS_WIN32
         replace_string("\\", "\\\\", b);

#endif
         line = boost::regex_replace(line, boost::regex(a.getStr()), b);
      }
   }
   return script;
}

const SmallVector<StringRef, 4> &ParserKind::allowedKeywordSuffixes(Kind kind)
{
   return sm_allowedSuffixes[kind];
}

StringRef ParserKind::getKindStr(Kind kind)
{
   return sm_keywordStrMap[kind];
}

std::map<ParserKind::Kind, SmallVector<StringRef, 4>> ParserKind::sm_allowedSuffixes{
   {ParserKind::TAG, {"."}},
   {ParserKind::COMMAND, {":"}},
   {ParserKind::LIST, {":"}},
   {ParserKind::BOOLEAN_EXPR, {":"}},
   {ParserKind::CUSTOM, {":", "."}}
};

std::map<ParserKind::Kind, StringRef> ParserKind::sm_keywordStrMap{
   {ParserKind::TAG, "TAG"},
   {ParserKind::COMMAND, "COMMAND"},
   {ParserKind::LIST, "LIST"},
   {ParserKind::BOOLEAN_EXPR, "BOOLEAN_EXPR"},
   {ParserKind::CUSTOM, "CUSTOM"}
};

IntegratedTestKeywordParser::IntegratedTestKeywordParser(const std::string &keyword, ParserKind::Kind kind,
                                                         ParserHandler parser, const std::vector<std::string> &initialValue)
   : m_kind(kind),
     m_keyword(keyword),
     m_value(initialValue)
{
   SmallVector<StringRef, 4> allowedSuffixes = ParserKind::allowedKeywordSuffixes(kind);

   if (keyword.empty() || (std::find(allowedSuffixes.begin(), allowedSuffixes.end(), StringRef(keyword.data() + keyword.size() - 1, 1)) == allowedSuffixes.end())) {
      if (allowedSuffixes.getSize() == 1) {
         throw ValueError(format_string("Keyword '%s' of kind '%s' must end in '%s'", keyword.c_str(),
                                        ParserKind::getKindStr(kind).getStr().c_str(), allowedSuffixes.front().getStr().c_str()));
      } else {
         throw ValueError(format_string("Keyword '%s' of kind '%s' must end in "
                                        " one of '%s'", keyword.c_str(),
                                        ParserKind::getKindStr(kind).getStr().c_str(),
                                        polar::basic::join(allowedSuffixes.begin(), allowedSuffixes.end(), " ").c_str()));
      }
   }
   if (parser != nullptr && kind != ParserKind::CUSTOM) {
      throw ValueError("custom parsers can only be specified with "
                       "ParserKind.CUSTOM");
   }
   if (kind == ParserKind::COMMAND) {
      m_parser = [&](int lineNumber, std::string &line, std::vector<std::string> &output) -> std::vector<std::string> & {
         return IntegratedTestKeywordParser::handleCommand(lineNumber, line, output, this->m_keyword);
      };
   } else if (kind == ParserKind::LIST) {
      m_parser = IntegratedTestKeywordParser::handleList;
   } else if (kind == ParserKind::BOOLEAN_EXPR) {
      m_parser = IntegratedTestKeywordParser::handleBooleanExpr;
   } else if (kind == ParserKind::TAG) {
      m_parser = IntegratedTestKeywordParser::handleTag;
   } else if (kind == ParserKind::CUSTOM) {
      if (parser == nullptr) {
         throw ValueError("ParserKind.CUSTOM requires a custom parser");
      }
      m_parser = parser;
   } else {
      throw ValueError(format_string("Unknown kind '%s'", ParserKind::getKindStr(kind).getStr().c_str()));
   }
}

void IntegratedTestKeywordParser::parseLine(int lineNumber, std::string &line)
{
   try {
      m_parsedLines.push_back(std::make_pair(lineNumber, line));
      m_value = m_parser(lineNumber, line, m_value);
   } catch (ValueError &e) {
      throw ValueError(format_string("%s \nin %s directive on test line %d", e.what(),
                                     m_keyword.c_str(), lineNumber));
   }
}

ParserKind::Kind IntegratedTestKeywordParser::getKind()
{
   return m_kind;
}

const std::string &IntegratedTestKeywordParser::getKeyword()
{
   return m_keyword;
}

const std::list<std::pair<int, StringRef>> IntegratedTestKeywordParser::getParsedLines()
{
   return m_parsedLines;
}

const std::vector<std::string> &IntegratedTestKeywordParser::getValue()
{
   return m_value;
}

std::vector<std::string> &IntegratedTestKeywordParser::handleTag(int, std::string &line, std::vector<std::string> &output)
{
   StringRef lineTrip(line);
   lineTrip = lineTrip.trim();
   if (lineTrip.empty()) {
      output.push_back(lineTrip.getStr());
   }
   return output;
}

std::vector<std::string> &IntegratedTestKeywordParser::handleCommand(int lineNumber, std::string &line, std::vector<std::string> &output,
                                                                     const std::string &keyword)
{
   line = StringRef(line).rtrim().getStr();
   line = boost::regex_replace(line, boost::regex("%\\(line\\)"), std::to_string(lineNumber));
   line = boost::regex_replace(line, boost::regex("%\\(line *([\\+-]) *(\\d+)\\)"), [&lineNumber](const boost::smatch &match) -> std::string{
      std::string mstr = match[1].str();
      std::string number = match[2].str();
      if (mstr == "+") {
         return std::to_string(lineNumber + std::stoi(number));
      }
      if (mstr == "-") {
         return std::to_string(lineNumber - std::stoi(number));
      }
      return match[0].str();
   });
   // Collapse lines with trailing '\\'.
   if (!output.empty() && output.back().back() == '\\') {
      std::string &currentLine = output.back();
      currentLine = currentLine.substr(0, currentLine.size() - 1) + line;
   } else {
      std::string pdbg = format_string("%%dbg(%s at line %d)", keyword.c_str(), lineNumber);
      assert(boost::regex_match(pdbg, boost::regex(sgc_kpdbgRegex + "$", boost::match_default | boost::match_all)) && "kPdbgRegex expected to match actual %dbg usage");
      line = format_string("%s %s", pdbg.c_str(), line.c_str());
      output.push_back(line);
   }
   return output;
}

std::vector<std::string> &IntegratedTestKeywordParser::handleList(int lineNumber, std::string &line,
                                                                  std::vector<std::string> &output)
{
   StringRef lineRef(line);
   SmallVector<StringRef, 10> parts;
   lineRef.split(parts, ",", -1, false);
   auto iter = parts.begin();
   auto endMark = parts.end();
   while (iter != endMark) {
      output.push_back((*iter).trim().getStr());
      ++iter;
   }
   return output;
}

std::vector<std::string> &IntegratedTestKeywordParser::handleBooleanExpr(int lineNumber, std::string &line,
                                                                         std::vector<std::string> &output)
{
   StringRef lineRef(line);
   SmallVector<StringRef, 10> parts;
   lineRef.split(parts, ",", -1, false);
   auto iter = parts.begin();
   auto endMark = parts.end();
   while (iter != endMark) {
      output.push_back((*iter).trim().getStr());
      ++iter;
   }
   // Evaluate each expression to verify syntax.
   // We don't want any results, just the raised ValueError.
   for (std::string &str : output) {
      if (str != "*") {
         BooleanExpression::evaluate(str, {});
      }
   }
   return output;
}

std::vector<std::string> &IntegratedTestKeywordParser::handleRequiresAny(int lineNumber, std::string &line,
                                                                         std::vector<std::string> &output)
{
   // Extract the conditions specified in REQUIRES-ANY: as written.
   std::vector<std::string> conditions;
   handleList(lineNumber, line, conditions);
   // Output a `REQUIRES: a || b || c` expression in its place.
   std::string expression = join_string_list(conditions, " || ");
   handleBooleanExpr(lineNumber, expression, output);
   return output;
}

std::vector<std::string> parse_integrated_test_script(TestPointer test, ResultPointer &result, IntegratedTestKeywordParserList additionalParsers,
                                                      bool requireScript)
{
   std::vector<std::string> script;
   // Install the built-in keyword parsers.
   IntegratedTestKeywordParserList builtinParsers{
      std::make_shared<IntegratedTestKeywordParser>("RUN:", ParserKind::COMMAND, nullptr, script),
            std::make_shared<IntegratedTestKeywordParser>("XFAIL:", ParserKind::BOOLEAN_EXPR, nullptr, test->getXFails()),
            std::make_shared<IntegratedTestKeywordParser>("REQUIRES:", ParserKind::BOOLEAN_EXPR, nullptr, test->getRequires()),
            std::make_shared<IntegratedTestKeywordParser>("REQUIRES-ANY:", ParserKind::CUSTOM,
                                                          IntegratedTestKeywordParser::handleRequiresAny, test->getRequires()),
            std::make_shared<IntegratedTestKeywordParser>("UNSUPPORTED:", ParserKind::BOOLEAN_EXPR, nullptr, test->getRawUnSupported()),
            std::make_shared<IntegratedTestKeywordParser>("END.", ParserKind::TAG)
   };
   std::map<StringRef, IntegratedTestKeywordParserPointer> keywordParsers;
   std::vector<StringRef> keywords;
   for (IntegratedTestKeywordParserPointer parser : builtinParsers) {
      keywordParsers[parser->getKeyword()] = parser;
      keywords.push_back(parser->getKeyword());
   }

   // Install user-defined additional parsers.
   for (IntegratedTestKeywordParserPointer parser : additionalParsers) {
      if (keywordParsers.find(parser->getKeyword()) != keywordParsers.end()) {
         throw ValueError(format_string("Parser for keyword '%s' already exists", parser->getKeyword().c_str()));
      }
   }
   // Collect the test lines from the script.
   std::string sourcePath = test->getSourcePath();
   for (ParsedScriptLine &entry : parse_integrated_test_script_commands(sourcePath, keywords)) {
      size_t lineNumber = std::get<0>(entry);
      std::string &commandType = std::get<1>(entry);
      std::string &line = std::get<2>(entry);
      IntegratedTestKeywordParserPointer parser = keywordParsers[commandType];

      parser->parseLine(lineNumber, line);
      if (commandType == "END." && !parser->getValue().empty()) {
         break;
      }
   }
   // get parser result
   {
      IntegratedTestKeywordParserPointer parser = keywordParsers["RUN:"];
      script = parser->getValue();
   }
   {
      IntegratedTestKeywordParserPointer parser = keywordParsers["XFAIL:"];
      std::vector<std::string> &fails = test->getXFails();
      for (const std::string &fail : parser->getValue()) {
         fails.push_back(fail);
      }
   }
   {
      IntegratedTestKeywordParserPointer parser = keywordParsers["REQUIRES:"];
      std::vector<std::string> &requires = test->getRequires();
      for (const std::string &require : parser->getValue()) {
         requires.push_back(require);
      }
   }
   {
      IntegratedTestKeywordParserPointer parser = keywordParsers["REQUIRES-ANY:"];
      std::vector<std::string> &requires = test->getRequires();
      for (const std::string &require : parser->getValue()) {
         requires.push_back(require);
      }
   }
   {
      IntegratedTestKeywordParserPointer parser = keywordParsers["UNSUPPORTED:"];
      std::vector<std::string> &unsupporteds = test->getRawUnSupported();
      for (const std::string &unsupported : parser->getValue()) {
         unsupporteds.push_back(unsupported);
      }
   }
   // Verify the script contains a run line.
   if(requireScript && script.empty()) {
      result = std::make_shared<Result>(UNRESOLVED, "Test has no run line!");
      return script;
   }
   // Check for unterminated run lines.
   if (!script.empty() && script.back().back() == '\\') {
      result = std::make_shared<Result>(UNSUPPORTED, "Test has unterminated run lines (with '\\')");
      return script;
   }

   // Enforce REQUIRES:
   std::list<std::string> missingRequiredFeatures = test->getMissingRequiredFeatures();
   if (!missingRequiredFeatures.empty()) {
      std::string msg = join_string_list(missingRequiredFeatures, ", ");
      result = std::make_shared<Result>(UNSUPPORTED, format_string("Test requires the following unavailable  "
                                                                   "features: %s", msg.c_str()));
      return script;
   }

   // Enforce UNSUPPORTED:
   std::vector<std::string> unsupportedFeatures = test->getUnSupportedFeatures();
   if (!unsupportedFeatures.empty()) {
      std::string msg = join_string_list(unsupportedFeatures, ", ");
      result = std::make_shared<Result>(UNSUPPORTED, format_string("Test does not support the following features "
                                                                   "and/or targets: %s", msg.c_str()));
      return script;
   }
   // Enforce limit_to_features.
   if (!test->isWithinFeatureLimits()) {
      std::string msg = join_string_list(test->getConfig()->getLimitToFeatures(), ", ");
      result = std::make_shared<Result>(UNSUPPORTED, format_string("Test does not require any of the features "
                                                                   "specified in limit_to_features: %s", msg.c_str()));
      return script;
   }
   return script;
}

namespace {
ResultPointer do_run_shtest(TestPointer test, LitConfigPointer litConfig, bool useExternalSh,
                            std::vector<std::string> &script, const std::string &tempBase)
{
   // Create the output directory if it does not already exist.
   ResultPointer result;
   std::error_code errorCode;
   std::string execDir = stdfs::path(test->getExecPath()).parent_path();
   stdfs::create_directories(stdfs::path(tempBase).parent_path(), errorCode);
   if (errorCode) {
      return std::make_shared<Result>(FAIL, errorCode.message());
   }
   ExecScriptResult execScriptResult;
   if (useExternalSh) {
      execScriptResult = execute_script(test, litConfig, tempBase, script, execDir, result);
   } else {
      execScriptResult = execute_script_internal(test, litConfig, tempBase, script, execDir, result);
   }
   if (result) {
      return result;
   }
   std::string &out = std::get<0>(execScriptResult);
   std::string &errorMsg = std::get<1>(execScriptResult);
   int exitCode = std::get<2>(execScriptResult);
   std::string &timeoutInfo = std::get<3>(execScriptResult);
   const ResultCode *status = nullptr;
   if (exitCode == 0) {
      status = PASS;
   } else {
      if (exitCode == -2 && !timeoutInfo.empty()) {
         status = TIMEOUT;
      } else if (exitCode != 0) {
         status = FAIL;
      }
   }
   // Form the output log.
   std::string output = Twine("Script:\n--\n", join_string_list(script, "\n"))
         .concat("\n--\nExit Code: ")
         .concat(std::to_string(exitCode))
         .concat("\n").getStr();
   if (!timeoutInfo.empty()) {
      output += Twine("Timeout: ", timeoutInfo).concat("\n").getStr();
   }
   output += "\n";
   // Append the outputs, if present.
   if (!out.empty()) {
      output += Twine("Command Output (stdout):\n--\n", out).concat("\n--\n").getStr();
   }
   if (!errorMsg.empty()) {
      output += Twine("Command Output (stderr):\n--\n", errorMsg).concat("\n--\n").getStr();
   }
   return std::make_shared<Result>(status, output);
}
} // anonymous namespace

ResultPointer execute_shtest(TestPointer test, LitConfigPointer litConfig, bool useExternalSh,
                             SubstitutionList extraSubstitutions)
{
   if (test->getConfig()->isUnsupported()) {
      return std::make_shared<Result>(UNSUPPORTED, "Test is unsupported");
   }
   ResultPointer result;
   std::vector<std::string> script = parse_integrated_test_script(test, result, {}, true);
   if (result) {
      return result;
   }
   if (litConfig->isNoExecute()) {
      return std::make_shared<Result>(PASS);
   }
   std::pair<std::string, std::string> tempPaths = get_temp_paths(test);
   for (const SubstitutionPair &item : get_default_substitutions(test, std::get<0>(tempPaths), std::get<1>(tempPaths), useExternalSh)) {
      extraSubstitutions.push_back(item);
   }
   apply_substitutions(script, extraSubstitutions);
   // Re-run failed tests up to test_retry_attempts times.
   int attempts = 1;
   TestingConfigPointer testConfig = test->getConfig();
   if (testConfig->hasExtraConfig("TestRetryAttempts")) {
      try {
         attempts += testConfig->getExtraConfig<int>("TestRetryAttempts");
      } catch(...) {}
   }
   int i = 0;
   for (; i < attempts; ++i) {
      result = do_run_shtest(test, litConfig, useExternalSh, script, std::get<1>(tempPaths));
      if (result->getCode() != FAIL) {
         break;
      }
   }
   // If we had to run the test more than once, count it as a flaky pass. These
   // will be printed separately in the test summary.
   if (i > 0 && result->getCode() == PASS) {
      result->setCode(FLAKYPASS);
   }
   return result;
}

} // lit
} // polar
