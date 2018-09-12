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

#include "TestRunner.h"
#include "Utils.h"
#include "ShellCommands.h"

#include <any>

namespace polar {
namespace lit {

const std::string &ShellEnvironment::getCwd()
{
   return m_cwd;
}

const std::map<std::string, std::string> &ShellEnvironment::getEnv()
{
   return m_env;
}

ShellEnvironment &ShellEnvironment::setCwd(const std::string &cwd)
{
   m_cwd = cwd;
   return *this;
}

ShellEnvironment &ShellEnvironment::setEnvItem(const std::string &key, const std::string &value)
{
   m_env[key] = value;
   return *this;
}

TimeoutHelper::TimeoutHelper(int timeout)
   : m_timeout(timeout),
     m_timeoutReached(false),
     m_doneKillPass(false),
     m_timer(std::nullopt)
{
}

void TimeoutHelper::cancel()
{
   if (m_timer) {
      if (active()) {
         m_timer.value().stop();
      }
   }
}

bool TimeoutHelper::active()
{
   return m_timeout > 0;
}

void TimeoutHelper::addProcess(pid_t process)
{
   if (!active()) {
      return;
   }
   bool needToRunKill = false;
   {
      std::lock_guard lock(m_lock);
      m_procs.push_back(process);
      // Avoid re-entering the lock by finding out if kill needs to be run
      // again here but call it if necessary once we have left the lock.
      // We could use a reentrant lock here instead but this code seems
      // clearer to me.
      needToRunKill = m_doneKillPass;
   }
   // The initial call to _kill() from the timer thread already happened so
   // we need to call it again from this thread, otherwise this process
   // will be left to run even though the timeout was already hit
   if (needToRunKill) {
      assert(timeoutReached());
      kill();
   }
}

void TimeoutHelper::startTimer()
{
   if (!active()) {
      return;
   }
   // Do some late initialisation that's only needed
   // if there is a timeout set
   m_timer.emplace([](){
      //handleTimeoutReached();
   }, std::chrono::milliseconds(m_timeout), true);
   BasicTimer &timer = m_timer.value();
   timer.start();
}

void TimeoutHelper::handleTimeoutReached()
{
   m_timeoutReached = true;
   kill();
}

bool TimeoutHelper::timeoutReached()
{
   return m_timeoutReached;
}

void TimeoutHelper::kill()
{
   std::lock_guard locker(m_lock);
   for (pid_t pid : m_procs) {
      kill_process_and_children(pid);
   }
   m_procs.clear();
   m_doneKillPass = true;
}

ShellCommandResult::ShellCommandResult(const Command &command, std::ostream &outStream,
                                       std::ostream &errStream, int exitCode,
                                       bool timeoutReached, const std::list<std::string> &outputFiles)
   : m_command(command),
     m_outStream(outStream),
     m_errStream(errStream),
     m_exitCode(exitCode),
     m_timeoutReached(timeoutReached),
     m_outputFiles(outputFiles)
{

}

const Command &ShellCommandResult::getCommand()
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

namespace {

int do_execute_shcmd();

} // anonymous namespace

std::tuple<int, std::string> execute_shcmd()
{

}

std::list<std::string> expand_glob(GlobItem &glob, const std::string &cwd)
{
   return glob.resolve(cwd);
}

std::list<std::string> expand_glob(const std::string &path, const std::string &cwd)
{
   return {path};
}

std::list<std::string> expand_glob_expression(const std::list<std::string> &exprs,
                                              const std::string &cwd)
{
   auto iter = exprs.begin();
   auto endMark = exprs.end();
   std::list<std::string> results{
      *iter++
   };
   while (iter != endMark) {
      std::list<std::string> files = expand_glob(*iter, cwd);
      for (const std::string &file : files) {
         results.push_back(file);
      }
      ++iter;
   }
   return results;
}

namespace {

std::optional<int> do_execute_shcmd(std::shared_ptr<AbstractCommand> cmd, ShellEnvironment &shenv,
                                    std::list<ShellCommandResult> &results,
                                    TimeoutHelper &timeoutHelper)
{
   if (timeoutHelper.timeoutReached()) {
      // Prevent further recursion if the timeout has been hit
      // as we should try avoid launching more processes.
      return std::nullopt;
   }
   std::optional<int> result;
   AbstractCommand::Type commandType = cmd->getCommandType();
   if (commandType == AbstractCommand::Type::Seq) {
      Seq *seqCommand = dynamic_cast<Seq *>(cmd.get());
      const std::string &op = seqCommand->getOp();
      if (op == ";") {
         result = do_execute_shcmd(seqCommand->getLhs(), shenv, results, timeoutHelper);
         return do_execute_shcmd(seqCommand->getRhs(), shenv, results, timeoutHelper);
      }
      if (op == "&") {
         throw InternalShellError(seqCommand->operator std::string(), "unsupported shell operator: '&'");
      }
      if (op == "||") {
         result = do_execute_shcmd(seqCommand->getLhs(), shenv, results, timeoutHelper);
         if (result.has_value() && 0 != result.value()) {
            result = do_execute_shcmd(seqCommand->getRhs(), shenv, results, timeoutHelper);
         }
         return result;
      }
      if (op == "&&") {
         result = do_execute_shcmd(seqCommand->getLhs(), shenv, results, timeoutHelper);
         if (!result.has_value()) {
            return result;
         }
         if (result.has_value() && 0 == result.value()) {
            result = do_execute_shcmd(seqCommand->getRhs(), shenv, results, timeoutHelper);
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
   if (firstArgAny.type() == typeid(std::string)) {
      const std::string &firstArg = std::any_cast<const std::string &>(firstArgAny);
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
         if (fs::path(newDir).is_absolute()) {
            shenv.setCwd(newDir);
         } else {
            fs::path basePath(shenv.getCwd());
            basePath /= newDir;
            basePath = fs::canonical(basePath);
            shenv.setCwd(basePath);
         }
         // The cd builtin always succeeds. If the directory does not exist, the
         // following Popen calls will fail instead.
         return 0;
      }
   }
   // Handle "echo" as a builtin if it is not part of a pipeline. This greatly
   // speeds up tests that construct input files by repeatedly echo-appending to
   // a file.
   // FIXME: Standardize on the builtin echo implementation. We can use a
   // temporary file to sidestep blocking pipe write issues.
   if (firstArgAny.type() == typeid(std::string)) {
      const std::string &firstArg = std::any_cast<const std::string &>(firstArgAny);
      if (firstArg == "echo" && pipeCommand->getCommands().size() == 1) {
         // std::string output = execute_builtin_echo(firstAbstractCommand, shenv);
         //         results.emplace_back(firstAbstractCommand, output, "", 0,
         //                              false);
         return 0;
      }
   }
}

} // anonymous namespace

/// Return the standard fds for cmd after applying redirects
/// Returns the three standard file descriptors for the new child process.  Each
/// fd may be an open, writable file object or a sentinel value from the
/// subprocess module.
StdFdsTuple process_redirects(std::shared_ptr<AbstractCommand> cmd, int stdinSource,
                              const ShellEnvironment &shenv,
                              std::list<OpenFileEntryType> &openedFiles)
{
   assert(cmd->getCommandType() == AbstractCommand::Type::Command);
   // Apply the redirections, we use (N,) as a sentinel to indicate stdin,
   // stdout, stderr for N equal to 0, 1, or 2 respectively. Redirects to or
   // from a file are represented with a list [file, mode, file-object]
   // where file-object is initially None.
   std::list<std::any> redirects = {std::tuple<int, int>{0, -1},
                                    std::tuple<int, int>{1, -1},
                                    std::tuple<int, int>{2, -1}};
   Command *command = dynamic_cast<Command *>(cmd.get());
   using OpenFileTuple = std::tuple<std::string, std::string, std::optional<int>>;
   for (const TokenType &redirect : command->getRedirects()) {
      const std::any &opAny = std::get<0>(redirect);
      const std::any &filenameAny = std::get<1>(redirect);
      const ShellTokenType &op = std::any_cast<const ShellTokenType &>(opAny);
      const std::string &filename = std::any_cast<const std::string &>(filenameAny);
      if (op == std::tuple<std::string, int>{">", 2}) {
         auto iter = redirects.begin();
         std::advance(iter, 2);
         *iter = std::any(OpenFileTuple{filename, "w", std::nullopt});
      } else if (op == std::tuple<std::string, int>{">>", 2}) {
         auto iter = redirects.begin();
         std::advance(iter, 2);
         *iter = std::any(OpenFileTuple{filename, "a", std::nullopt});
      } else if (op == std::tuple<std::string, int>{">&", 2} &&
                 (filename == "0" || filename == "1" || filename == "2")) {
         auto iter = redirects.begin();
         std::advance(iter, 2);
         auto targetIter = redirects.begin();
         std::advance(targetIter, std::stoi(filename));
         *iter = *targetIter;
      } else if (op == std::tuple<std::string, int>{">&", -1} ||
                 op == std::tuple<std::string, int>{"&>", -1}) {
         auto iter = redirects.begin();
         *(++iter) = std::any(OpenFileTuple{filename, "w", std::nullopt});
         *(++iter) = std::any(OpenFileTuple{filename, "w", std::nullopt});
      } else if (op == std::tuple<std::string, int>{">", -1}) {
         auto iter = redirects.begin();
         ++iter;
         *iter = std::any(OpenFileTuple{filename, "w", std::nullopt});
      } else if (op == std::tuple<std::string, int>{">>", -1}) {
         auto iter = redirects.begin();
         ++iter;
         *iter = std::any(OpenFileTuple{filename, "a", std::nullopt});
      } else if (op == std::tuple<std::string, int>{"<", -1}) {
         auto iter = redirects.begin();
         *iter = std::any(OpenFileTuple{filename, "a", std::nullopt});
      } else {
         throw InternalShellError(command->operator std::string(),
                                  "Unsupported redirect: (" + std::get<0>(op) + ", " + std::to_string(std::get<1>(op)) + ")" + filename);
      }
   }
   int index = 0;
   auto iter = redirects.begin();
   auto endMark = redirects.end();
   std::list<std::optional<int>> stdFds{std::nullopt, std::nullopt, std::nullopt};
   while (iter != endMark) {
      std::any &itemAny = *iter;
      if (itemAny.type() == typeid(std::tuple<int, int>)) {
         std::tuple<int, int> &item = std::any_cast<std::tuple<int, int> &>(itemAny);
         int fd = -1;
         if (item == std::tuple<int, int>{0, -1}) {
            fd = stdinSource;
         } else if (item == std::tuple<int, int>{1, -1}) {
            if (index == 0) {
               throw InternalShellError(command->operator std::string(),
                                        "Unsupported redirect for stdin");
            } else if (index == 1) {
               fd = SUBPROCESS_FD_PIPE;
            } else {
               fd = 1;
            }
         } else if (item == std::tuple<int, int>{2, -1}) {
            if (index != 2) {
               throw InternalShellError(command->operator std::string(),
                                        "Unsupported redirect for stdout");
            }
            fd = SUBPROCESS_FD_PIPE;
         } else {
            throw InternalShellError(command->operator std::string(),
                                     "Bad redirect");
         }
         auto fdIter = stdFds.begin();
         std::advance(fdIter, index);
         *fdIter = fd;
         ++index;
         ++iter;
         continue;
      }
      OpenFileTuple &item = std::any_cast<OpenFileTuple &>(itemAny);
      std::string &filename = std::get<0>(item);
      std::string &mode = std::get<1>(item);
      std::optional<int> fdOpt = std::get<2>(item);
      // Check if we already have an open fd. This can happen if stdout and
      // stderr go to the same place.
      if (fdOpt.has_value()) {
         auto fdIter = stdFds.begin();
         std::advance(fdIter, index);
         *fdIter = fdOpt.value();
         ++index;
         ++iter;
         continue;
      }
   }
}

std::string execute_builtin_echo()
{

}

} // lit
} // polar
