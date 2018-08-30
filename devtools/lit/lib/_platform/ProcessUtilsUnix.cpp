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
#include "unistd.h"
#include <cstdio>
#include <list>
#include <cstdlib>
#include <sys/wait.h>

#include <iostream>
namespace polar {
namespace lit {

bool find_executable(const fs::path &filepath) noexcept
{
   std::error_code errCode;
   if (fs::exists(filepath, errCode)) {
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
      fs::path path(dir);
      path /= file;
      if (find_executable(path)) {
         return path.string();
      }
   }
   return std::nullopt;
}

std::tuple<std::list<pid_t>, bool> call_pgrep_command(pid_t pid) noexcept
{
   int stdoutChannel[2];
   int stderrChannel[2];
   if (-1 == pipe(stdoutChannel) || -1 == pipe(stderrChannel)) {
      return std::make_tuple(std::list<pid_t>{}, false);
   }
   pid_t cpid = fork();
   if (0 == cpid) {
      // close read fd
      if (-1 == close(stdoutChannel[0]) || -1 == close(stderrChannel[0])) {
         perror("close stdoutChannel or stderrChannel read fd error");
         exit(1);
      }
      // io redirection
      if (-1 == close(STDOUT_FILENO) || -1 == close(STDERR_FILENO)) {
         perror("close stdout or stderr fd of child process error");
         exit(1);
      }
      if (-1 == dup2(stdoutChannel[1], STDOUT_FILENO) ||
          -1 == dup2(stderrChannel[1], STDERR_FILENO)) {
         perror("dup stdout or stderr fd of child process error");
         exit(1);
      }

      std::optional<std::string> cmd = look_path("pgrep");
      if (!cmd.has_value()) {
         std::cerr << "command pgrep is not found" << std::endl;
         exit(1);
      }
      if (-1 == execlp(cmd.value().c_str(), "pgrep", ("-P " + std::to_string(pid)).c_str(), NULL)) {
         std::cerr << "execlp pgrep error: " << strerror(errno) << std::endl;
         exit(1);
      }
   } else if (-1 == cpid) {
      return std::make_tuple(std::list<pid_t>{}, false);
   } else {
      if (-1 == close(stdoutChannel[1]) || -1 == close(stderrChannel[1])) {
         return std::make_tuple(std::list<pid_t>{}, false);
      }
   }

   int exitCode;
   // @TODO maybe something wrong
   while(-1 != waitpid(cpid, &exitCode, 0) || errno == EAGAIN) {}
   char readBuffer[256];
   std::string output;
   std::string errMsg;
   ssize_t count = 0;
   do {
      count = read(stdoutChannel[0], readBuffer, 256);
      if (-1 == count) {
         return std::make_tuple(std::list<int32_t>{}, false);
      }
      output += std::string(readBuffer, count);
   } while (count > 0);
   count = 0;
   do {
      count = read(stderrChannel[0], readBuffer, 256);
      if (-1 == count) {
         return std::make_tuple(std::list<int32_t>{}, false);
      }
      errMsg += std::string(readBuffer, count);
   } while (count > 0);
   // here ignore errmsg
   if (exitCode != 0) {
      return std::make_tuple(std::list<int32_t>{}, false);
   }
   std::list<int32_t> pids;
   for (const std::string &item : split_string(output, '\n')) {
      pids.push_back(std::stoi(item));
   }
   return std::make_tuple(pids, true);
}

} // lit
} // polar
