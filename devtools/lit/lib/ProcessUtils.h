// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/29.

#ifndef POLAR_DEVLTOOLS_LIT_PROCESS_UTILS_H
#define POLAR_DEVLTOOLS_LIT_PROCESS_UTILS_H

#include <string>
#include <optional>
#include <tuple>
#include <list>
#include <map>
#include "Global.h"

namespace polar {
namespace lit {

namespace internal {

void do_run_program(const std::string &cmd, int &exitCode,
                    const std::optional<std::string> &cwd,
                    const std::optional<std::map<std::string, std::string>> &env,
                    const std::optional<std::string> &input,
                    std::string &output, std::string &errMsg,
                    const int count, ...);

inline char *run_program_arg_filter(char *arg)
{
   return arg;
}

inline const char *run_program_arg_filter(const char *arg)
{
   return arg;
}

inline const char *run_program_arg_filter(const std::string &arg)
{
   return arg.c_str();
}

} // internal

using RunCmdResponse = std::tuple<bool, std::string, std::string>;
bool find_executable(const fs::path &filepath) noexcept;
std::optional<std::string> look_path(const std::string &file) noexcept;

std::tuple<std::list<pid_t>, bool> retrieve_children_pids(pid_t pid, bool recursive = false) noexcept;
std::tuple<std::list<pid_t>, bool> call_pgrep_command(pid_t pid) noexcept;
template <typename... ArgTypes>
RunCmdResponse run_program(const std::string &cmd,
                           const std::optional<std::string> &cwd = std::nullopt,
                           const std::optional<std::map<std::string, std::string>> &env = std::nullopt,
                           const std::optional<std::string> &input = std::nullopt,
                           ArgTypes&&... args) noexcept
{
   std::string output;
   std::string errMsg;
   int exitCode;
   internal::do_run_program(cmd, exitCode, cwd,
                            env, input, output, errMsg,
                            sizeof...(args) + 2, internal::run_program_arg_filter(std::forward<ArgTypes>(args))...);
   bool status = 0 == exitCode ? true : false;
   return std::make_tuple(status, output, errMsg);
}

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_PROCESS_UTILS_H
