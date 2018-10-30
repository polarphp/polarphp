// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/28.

#include "../ProcessUtils.h"
#include "../Utils.h"
#include <unistd.h>
#include <cstdio>
#include <list>
#include <cstdlib>
#include <sys/wait.h>
#include <iostream>
#include <memory>

namespace polar {
namespace lit {

bool find_executable(const stdfs::path &filepath) noexcept
{
   std::error_code errCode;
   if (stdfs::exists(filepath, errCode) && stdfs::is_regular_file(filepath)) {
      return true;
   }
   return false;
}

std::optional<std::string> look_path(const std::string &file) noexcept
{
   if (std::string::npos != file.find_first_of('/')) {
      if (find_executable(file)) {
         return file;
      }
      return std::nullopt;
   }
   std::string pathEnv{std::getenv("PATH")};
   std::list<std::string> dirs(split_string(pathEnv, ':'));
   for (std::string dir : dirs) {
      if (dir == "") {
         dir = ".";
      }
      stdfs::path path(dir);
      path /= file;
      if (find_executable(path)) {
         return path.string();
      }
   }
   return std::nullopt;
}

namespace internal {
void do_run_program(const std::string &cmd, int &exitCode,
                    const std::optional<std::string> &cwd,
                    const std::optional<std::map<std::string, std::string>> &env,
                    const std::optional<std::string> &input,
                    std::string &output, std::string &errMsg,
                    const size_t count, ...)
{
   int stdinChannel[2];
   int stdoutChannel[2];
   int stderrChannel[2];
   if (-1 == pipe(stdinChannel) || -1 == pipe(stdoutChannel) || -1 == pipe(stderrChannel)) {
      exitCode = -1;
      errMsg = strerror(errno);
   }
   // prepare args
   std::shared_ptr<char *[]> targetArgs(new char *[count]);
   targetArgs[static_cast<std::ptrdiff_t>(count - 1)] = nullptr;
   targetArgs[0] = const_cast<char *>(cmd.c_str());
   va_list args;
   va_start(args, count);
   for (size_t i = 1; i < count - 1; ++i) {
      targetArgs[static_cast<std::ptrdiff_t>(count - 1)] = va_arg(args, char *);
   }
   va_end(args);
   pid_t cpid = fork();
   if (0 == cpid) {
      // close read fd
      if (-1 == close(stdoutChannel[0]) || -1 == close(stderrChannel[0])) {
         perror("close stdoutChannel or stderrChannel read fd error");
         exit(1);
      }
      if (-1 == close(stdinChannel[1])) {
         perror("clse stdinChannel write fd error");
      }
      // io redirection
      if (-1 == close(STDIN_FILENO) || -1 == close(STDOUT_FILENO) || -1 == close(STDERR_FILENO)) {
         perror("close stdin or stdout or stderr fd of child process error");
         exit(1);
      }
      if (-1 == dup2(stdinChannel[0], STDIN_FILENO) ||
          -1 == dup2(stdoutChannel[1], STDOUT_FILENO) ||
          -1 == dup2(stderrChannel[1], STDERR_FILENO)) {
         std::cerr << strerror(errno) << std::endl;
         exit(1);
      }

      std::optional<std::string> cmdpath = look_path(cmd.c_str());
      if (!cmdpath.has_value()) {
         std::cerr << "command is not found" << std::endl;
         exit(1);
      }
      // if we chdir, wether need detect target directory exists?
      if (cwd.has_value()) {
         const std::string &cwdStr = cwd.value();
         if (!stdfs::exists(cwdStr)) {
            std::cerr << "chdir error: target directory is not exist" << std::endl;
            exit(1);
         }
         if (-1 == chdir(cwdStr.c_str())) {
            std::cerr << strerror(errno);
            exit(1);
         }
      }
      std::unique_ptr<char *[]> envArray = nullptr;
      if (env.has_value()) {
         const std::map<std::string, std::string> envMap = env.value();
         envArray.reset(new char *[envMap.size() + 1]);
         envArray[envMap.size()] = nullptr;
         char lineBuffer[256];
         size_t i = 0;
         for (auto &envItem : envMap) {
            if (-1 == sprintf(lineBuffer, "%s=%s", envItem.first.c_str(), envItem.second.c_str())) {
               std::cerr << strerror(errno);
               exit(1);
            }
            // @TODO wether need release?
            envArray[i] = new char[std::strlen(lineBuffer)];
            std::strcpy(envArray[i], lineBuffer);
            ++i;
         }
      }
      if (-1 == execve(cmdpath.value().c_str(), targetArgs.get(), envArray.get())) {
         std::cerr << "execlp error: " << strerror(errno) << std::endl;
         exit(1);
      }
   } else if (-1 == cpid) {
      exitCode = -1;
      errMsg = strerror(errno);
      return;
   } else {
      if (-1 == close(stdoutChannel[1]) || -1 == close(stderrChannel[1])) {
         exitCode = -1;
         errMsg = strerror(errno);
         return;
      }
      if (-1 == close(stdinChannel[0])) {
         exitCode = -1;
         errMsg = strerror(errno);
         return;
      }
      // write input to child process
      if (input.has_value()) {
         const std::string &inputStr = input.value();
         size_t tobeSize = inputStr.size();
         if (tobeSize > 0) {
            size_t written = 0;
            const char *data = inputStr.data();
            while (ssize_t n = write(stdinChannel[1], data, 256) != 0 && written < tobeSize) {
               if (n == -1) {
                  exitCode = -1;
                  errMsg = strerror(errno);
               } else if (n > 0) {
                  written += static_cast<size_t>(n);
               }
            }
         }
      }
   }
   while(-1 != waitpid(cpid, &exitCode, 0) || errno == EAGAIN) {}
   char readBuffer[256];
   ssize_t readed = 0;
   if (0 == exitCode) {
      do {
         readed = read(stdoutChannel[0], readBuffer, 256);
         if (-1 == readed) {
            exitCode = -1;
            errMsg = strerror(errno);
            return;
         }
         if (readed > 0) {
            output += std::string(readBuffer, static_cast<size_t>(readed));
         }
      } while (readed > 0);
   } else {
      readed = 0;
      do {
         readed = read(stderrChannel[0], readBuffer, 256);
         if (-1 == readed) {
            exitCode = -1;
            errMsg = strerror(errno);
            return;
         }
         if (readed > 0) {
            errMsg += std::string(readBuffer, static_cast<size_t>(readed));
         }
      } while (readed > 0);
   }
}
} // internal

} // lit
} // polar
